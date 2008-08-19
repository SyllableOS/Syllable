/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

typedef long long dr9_off_t;

#include <atheos/stdlib.h>
#include <atheos/string.h>
#include <atheos/kernel.h>
#include <atheos/device.h>

#include <posix/stat.h>
#include <posix/errno.h>
#include <posix/ioctl.h>

#include <atheos/time.h>

#include <atheos/filesystem.h>
#include <atheos/bcache.h>

#include "dosfs.h"
#include "attr.h"
#include "dir.h"
#include "dlist.h"
#include "fat.h"
#include "file.h"
#include "iter.h"
#include "util.h"
#include "vcache.h"

#include <macros.h>

extern const char *build_time, *build_date;

/* debug levels */
int debug_attr = 0, debug_dir = 0, debug_dlist = 0, debug_dosfs = 0,
		debug_encodings = 0, debug_fat = 0, debug_file = 0,
		debug_iter = 0, debug_vcache = 0;

#define DPRINTF(a,b) if (debug_dosfs > (a)) printk b

CHECK_MAGIC(vnode,struct vnode,VNODE_MAGIC)
CHECK_MAGIC(nspace,struct _nspace, NSPACE_MAGIC)

static int lock_removable_device(int fd, bool state)
{
//    return ioctl(fd, B_SCSI_PREVENT_ALLOW, &state, sizeof(state));
    return( 0 );
}

static status_t mount_fat_disk(const char *path, fs_id nsid,
		const int flags, nspace** newVol )
{
    nspace		*vol = NULL;
    uint8		buf[512];
    int			i;
    device_geometry geo;
    status_t err;

    *newVol = NULL;
    if ((vol = (nspace*)calloc(sizeof(nspace), 1)) == NULL) {
	printk("dosfs error: out of memory\n");
	return -ENOMEM;
    }
	
    vol->magic = NSPACE_MAGIC;
    vol->flags = FS_IS_PERSISTENT | FS_IS_BLOCKBASED | FS_HAS_MIME;

      // open read-only for now
    if ((err = (vol->fd = open(path, O_RDONLY))) < 0) {
	printk("dosfs: unable to open %s (%d)\n", path, err);
	goto error0;
    }
      // get device characteristics
    if (ioctl(vol->fd, IOCTL_GET_DEVICE_GEOMETRY, &geo) < 0) {
	struct stat st;
	if ((fstat(vol->fd, &st) >= 0) &&
	    S_ISREG(st.st_mode)) {
	      /* support mounting disk images */
	    geo.bytes_per_sector = 0x200;
	    geo.sectors_per_track = 1;
	    geo.cylinder_count = st.st_size / 0x200;
	    geo.sector_count = geo.cylinder_count;
	    geo.head_count = 1;
	    geo.read_only = !(st.st_mode & S_IWUSR);
	    geo.removable = true;
	} else {
	    printk("dosfs: error getting device geometry\n");
	    goto error0;
	}
    }
    if ((geo.bytes_per_sector != 0x200) && (geo.bytes_per_sector != 0x400) && (geo.bytes_per_sector != 0x800)) {
	printk("dosfs: unsupported device block size (%u)\n", geo.bytes_per_sector );
	goto error0;
    }
    if (geo.removable) {
	DPRINTF(0, ("%s is removable\n", path));
	vol->flags |= FS_IS_REMOVABLE;
    }
    if (geo.read_only || (flags & B_MOUNT_READ_ONLY)) {
	DPRINTF(0, ("%s is read-only\n", path));
	vol->flags |= FS_IS_READONLY;
    } else {
	  // reopen it with read/write permissions
	close(vol->fd);
	if ((err = (vol->fd = open(path, O_RDWR))) < 0) {
	    printk("dosfs: unable to reopen %s (%s)\n", path, strerror(err));
	    goto error0;
	}
	if (vol->flags & FS_IS_REMOVABLE)
	    lock_removable_device(vol->fd, true);
    }

      // read in the boot sector
    if ((err = read_pos(vol->fd, 0, (void*)buf, 512)) != 512) {
	printk("dosfs: error reading boot sector\n");
	goto error;
    }
      // only check boot signature on hard disks to account for broken mtools
      // behavior
    if (((buf[0x1fe] != 0x55) || (buf[0x1ff] != 0xaa)) && (buf[0x15] == 0xf8))
	goto error;

    if (!memcmp(buf+3, "NTFS    ", 8) || !memcmp(buf+3, "HPFS    ", 8)) {
	printk("%4.4s, not FAT\n", buf+3);
	goto error;
    }
      // first fill in the universal fields from the bpb
    vol->bytes_per_sector = read16(buf,0xb);
    if ((vol->bytes_per_sector != 0x200) && (vol->bytes_per_sector != 0x400) && (vol->bytes_per_sector != 0x800)) {
	printk("dosfs error: unsupported bytes per sector (%u)\n",
	       vol->bytes_per_sector);
	goto error;
    }
	
    vol->sectors_per_cluster = i = buf[0xd];
    if ((i != 1) && (i != 2) && (i != 4) && (i != 8) && 
	(i != 0x10) && (i != 0x20) && (i != 0x40) && (i != 0x80)) {
	printk("dosfs: sectors/cluster = %d\n", i);
	goto error;
    }

    vol->reserved_sectors = read16(buf,0xe);

    vol->fat_count = buf[0x10];
    if ((vol->fat_count == 0) || (vol->fat_count > 8)) {
	printk("dosfs: unreasonable fat count (%u)\n", vol->fat_count);
	goto error;
    }

    vol->media_descriptor = buf[0x15];
      // check media descriptor versus known types
    if ((buf[0x15] != 0xF0) && (buf[0x15] < 0xf8)) {
	printk("dosfs error: invalid media descriptor byte\n");
	goto error;
    }

    vol->vol_entry = -2;	// for now, assume there is no volume entry
    memset(vol->vol_label, ' ', 11);

    
      // now become more discerning citizens
    vol->sectors_per_fat = read16(buf,0x16);
    if (vol->sectors_per_fat == 0) {
	  // fat32 land
	vol->fat_bits = 32;
	vol->sectors_per_fat = read32(buf,0x24);
	vol->total_sectors = read32(buf,0x20);
		
	vol->fsinfo_sector = read16(buf, 0x30);
	if ((vol->fsinfo_sector != 0xffff) && (vol->fsinfo_sector >= vol->reserved_sectors)) {
	    printk("dosfs: fsinfo sector too large (%x)\n", vol->fsinfo_sector);
	    goto error;
	}

	vol->fat_mirrored = !(buf[0x28] & 0x80);
	vol->active_fat = (vol->fat_mirrored) ? (buf[0x28] & 0xf) : 0;

	vol->data_start = vol->reserved_sectors + vol->fat_count*vol->sectors_per_fat;
	vol->total_clusters = (vol->total_sectors - vol->data_start) / vol->sectors_per_cluster;

	vol->root_vnode.cluster = read32(buf,0x2c);
	if (vol->root_vnode.cluster >= vol->total_clusters) {
	    printk("dosfs: root vnode cluster too large (%x)\n", vol->root_vnode.cluster);
	    goto error;
	}
    } else {
	  // fat12 & fat16
	if (vol->fat_count != 2) {
	    printk("dosfs error: claims %d fat tables\n", vol->fat_count);
	    goto error;
	}

	vol->root_entries_count = read16(buf,0x11);
	if (vol->root_entries_count % (vol->bytes_per_sector / 0x20)) {
	    printk("dosfs error: invalid number of root entries\n");
	    goto error;
	}

	vol->fsinfo_sector = 0xffff;
	vol->total_sectors = read16(buf,0x13); // partition size
	if (vol->total_sectors == 0)
	    vol->total_sectors = read32(buf,0x20);

	{
	      /*
		Zip disks that were formatted at iomega have an incorrect number
		of sectors.  They say that they have 196576 sectors but they
		really only have 196192.  This check is a work-around for their
		brain-deadness.
		*/  
	    unsigned char bogus_zip_data[] = {
		0x00, 0x02, 0x04, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00,
		0xf8, 0xc0, 0x00, 0x20, 0x00, 0x40, 0x00, 0x20, 0x00, 0x00
	    };
			
	    if (memcmp(buf+0x0b, bogus_zip_data, sizeof(bogus_zip_data)) == 0 &&
		vol->total_sectors == 196576 &&
		((off_t)geo.sectors_per_track *
		 (off_t)geo.cylinder_count *
		 (off_t)geo.head_count) == 196192) {

		vol->total_sectors = 196192;
	    }
	}


	if (buf[0x26] == 0x29) {
	      // fill in the volume label
	    if (memcmp(buf+0x2b, "           ", 11)) {
		memcpy(vol->vol_label, buf+0x2b, 11);
		vol->vol_entry = -1;
	    }
	}

	vol->fat_mirrored = true;
	vol->active_fat = 0;

	vol->root_start = vol->reserved_sectors + vol->fat_count * vol->sectors_per_fat;
	vol->root_sectors = vol->root_entries_count * 0x20 / vol->bytes_per_sector;
	vol->root_vnode.cluster = 1;
	vol->root_vnode.end_cluster = 1;
	vol->root_vnode.st_size = vol->root_sectors * vol->bytes_per_sector;

	vol->data_start = vol->root_start + vol->root_sectors;
	vol->total_clusters = (vol->total_sectors - vol->data_start) / vol->sectors_per_cluster;	
		
	  // XXX: uncertain about border cases; win32 sdk says cutoffs are at
	  //      at ff6/ff7 (or fff6/fff7), but that doesn't make much sense
	if (vol->total_clusters > 0xff1)
	    vol->fat_bits = 16;
	else
	    vol->fat_bits = 12;
    }

      /* check that the partition is large enough to contain the file system */
#if 0
    if ( vol->total_sectors -  > geo.sector_count ) {
	printk("dosfs: volume extends past end of partition (%ld > %Ld)\n", vol->total_sectors, geo.sector_count );
	err = -EIO; //B_PARTITION_TOO_SMALL;
	goto error;
    }
#endif
      // perform sanity checks on the FAT
		
      // the media descriptor in active FAT should match the one in the BPB
    if ((err = read_pos(vol->fd, vol->bytes_per_sector*(vol->reserved_sectors + vol->active_fat * vol->sectors_per_fat),
			(void *)buf, 0x200)) != 0x200) {
	printk("dosfs: error reading FAT\n");
	goto error;
    }

    if (buf[0] != vol->media_descriptor) {
	printk("dosfs error: media descriptor mismatch (%x != %x)\n", buf[0], vol->media_descriptor);
	goto error;
    }

    if (vol->fat_mirrored) {
	uint32 i;
	uint8 buf2[512];
	for (i=0;i<vol->fat_count;i++) {
	    if (i != vol->active_fat) {
		DPRINTF(1, ("checking fat #%d\n", i));
		buf2[0] = ~buf[0];
		if ((err = read_pos(vol->fd, vol->bytes_per_sector*(vol->reserved_sectors + vol->sectors_per_fat*i), (void *)buf2, 0x200)) != 0x200) {
		    printk("dosfs: error reading FAT %d\n", i);
		    goto error;
		}

		if (buf2[0] != vol->media_descriptor) {
		    printk("dosfs error: media descriptor mismatch in fat # %d (%x != %x)\n", i, buf2[0], vol->media_descriptor);
		    goto error;
		}
#if 0			
		  // checking for exact matches of fats is too
		  // restrictive; allow these to go through in
		  // case the fat is corrupted for some reason
		if (memcmp(buf, buf2, 0x200)) {
		    printk("dosfs error: fat %d doesn't match active fat (%d)\n", i, vol->active_fat);
		    goto error;
		}
#endif
	    }
	}
    }

      // now we are convinced of the drive's validity

    vol->id = nsid;
    strncpy(vol->device,path,256);


      // XXX: if fsinfo exists, read from there?
    vol->last_allocated = 2;

      // initialize block cache
    err = setup_device_cache(vol->fd, vol->id, (dr9_off_t)vol->total_sectors);
    if ( err < 0) {
	printk("dosfs: error initializing block cache (%d)\n", err );
	goto error;
    }
    
      // as well as the vnode cache
    if (init_vcache(vol) != B_OK) {
	printk("dosfs: error initializing vnode cache\n");
	goto error1;
    }

      // and the dlist cache
    if (dlist_init(vol) != B_OK) {
	printk("dosfs: error initializing dlist cache\n");
	goto error2;
    }

    if (vol->flags & FS_IS_READONLY)
	vol->free_clusters = 0;
    else {
	if ((err = count_free_clusters(vol)) < 0) {
	    printk("dosfs: error counting free clusters (%s)\n", strerror(err));
	    goto error3;
	}
	vol->free_clusters = err;
    }

    DPRINTF(0, ("built at %s on %s\n", build_time, build_date));
    DPRINTF(0, ("mounting %s (id %x, device %x, media descriptor %x)\n", vol->device, vol->id, vol->fd, vol->media_descriptor));
    DPRINTF(0, ("%x bytes/sector, %x sectors/cluster\n", vol->bytes_per_sector, vol->sectors_per_cluster));
    DPRINTF(0, ("%x reserved sectors, %x total sectors\n", vol->reserved_sectors, vol->total_sectors));
    DPRINTF(0, ("%x %d-bit fats, %x sectors/fat, %x root entries\n", vol->fat_count, vol->fat_bits, vol->sectors_per_fat, vol->root_entries_count));
    DPRINTF(0, ("root directory starts at sector %x (cluster %x), data at sector %x\n", vol->root_start, vol->root_vnode.cluster, vol->data_start));
    DPRINTF(0, ("%x total clusters, %x free\n", vol->total_clusters, vol->free_clusters));
    DPRINTF(0, ("fat mirroring is %s, fs info sector at sector %x\n", (vol->fat_mirrored) ? "on" : "off", vol->fsinfo_sector));
    DPRINTF(0, ("last allocated cluster = %x\n", vol->last_allocated));

    if (vol->fat_bits == 32) {
	  // now that the block cache has been initialised, we can figure
	  // out the length of the root directory with count_clusters()
	vol->root_vnode.st_size = count_clusters(vol, vol->root_vnode.cluster) * vol->bytes_per_sector * vol->sectors_per_cluster;
	vol->root_vnode.end_cluster = get_nth_fat_entry(vol, vol->root_vnode.cluster, vol->root_vnode.st_size / vol->bytes_per_sector / vol->sectors_per_cluster - 1);
    }

      // initialize root vnode
    vol->root_vnode.magic = VNODE_MAGIC;
    vol->root_vnode.vnid = vol->root_vnode.dir_vnid = GENERATE_DIR_CLUSTER_VNID(vol->root_vnode.cluster,vol->root_vnode.cluster);
    vol->root_vnode.sindex = vol->root_vnode.eindex = 0xffffffff;
    vol->root_vnode.fn_nMode = FAT_SUBDIR;
    time(&(vol->root_vnode.st_time));
    vol->root_vnode.mime = NULL;
    dlist_add(vol, vol->root_vnode.vnid);

      // find volume label (supercedes any label in the bpb)
    {
	struct diri diri;
	uint8 *buffer;
	buffer = diri_init(vol, vol->root_vnode.cluster, 0, &diri);
	for (;buffer;buffer=diri_next_entry(&diri)) {
	    if ((buffer[0x0b] & FAT_VOLUME) && (buffer[0x0b] != 0xf) && (buffer[0] != 0xe5)) {
		vol->vol_entry = diri.current_index;
		memcpy(vol->vol_label, buffer, 11);
		break;
	    }
	}
	diri_free(&diri);
    }

    DPRINTF(0, ("root vnode id = %Lx\n", vol->root_vnode.vnid));
    DPRINTF(0, ("volume label [%11.11s] (%x)\n", vol->vol_label, vol->vol_entry));

      // steal a trick from bfs
    if (!memcmp(vol->vol_label, "__RO__     ", 11)) {
	vol->flags |= FS_IS_READONLY;
    }
	
    *newVol = vol;
    return B_NO_ERROR;

error3:
    dlist_uninit(vol);
error2:
    uninit_vcache(vol);
error1:
    shutdown_device_cache(vol->fd/*, NO_WRITES*/);
error:
    if (!(vol->flags & FS_IS_READONLY) && (vol->flags & FS_IS_REMOVABLE))
	lock_removable_device(vol->fd, false);
error0:
    close(vol->fd);
    free(vol);
    return (err >= B_NO_ERROR) ? -EINVAL : err;
}

static void update_fsinfo(nspace *vol)
{
    if ((vol->fat_bits == 32) && (vol->fsinfo_sector != 0xffff) &&
	((vol->flags & FS_IS_READONLY) == false)) {
	uchar *buffer;
	if ((buffer = (uchar *)get_cache_block(vol->fd, vol->fsinfo_sector, vol->bytes_per_sector)) != NULL) {
	    if ((read32(buffer,0) == 0x41615252) && (read32(buffer,0x1e4) == 0x61417272) && (read16(buffer,0x1fe) == 0xaa55)) {
		buffer[0x1e8] = (vol->free_clusters & 0xff);
		buffer[0x1e9] = ((vol->free_clusters >> 8) & 0xff);
		buffer[0x1ea] = ((vol->free_clusters >> 16) & 0xff);
		buffer[0x1eb] = ((vol->free_clusters >> 24) & 0xff);
		mark_blocks_dirty(vol->fd, vol->fsinfo_sector, 1);

		  // should also store next free cluster
	    } else {
		printk("update_fsinfo: fsinfo block has invalid magic number\n");
	    }
	    release_cache_block(vol->fd, vol->fsinfo_sector);
	} else {
	    printk("update_fsinfo: error getting fsinfo sector %x\n", vol->fsinfo_sector);
	}
    }
}

static int fatfs_unmount(void *_vol)
{
    int result = B_NO_ERROR;

    nspace* vol = (nspace*)_vol;

    LOCK_VOL(vol);
	
    if (check_nspace_magic(vol, "dosfs_unmount")) {
	UNLOCK_VOL(vol);
	return -EINVAL;
    }
	
    DPRINTF(0, ("dosfs_unmount volume %x\n", vol->id));

    update_fsinfo(vol);
    flush_device_cache(vol->fd, false );
    shutdown_device_cache( vol->fd/*, ALLOW_WRITES*/ );
    dlist_uninit(vol);
    uninit_vcache(vol);

    if (!(vol->flags & FS_IS_READONLY) && (vol->flags & FS_IS_REMOVABLE))
	lock_removable_device(vol->fd, false);
    result = close(vol->fd);
    free_lock(&(vol->vlock));
    vol->magic = ~VNODE_MAGIC;
    free(vol);

#ifdef USE_DMALLOC
    check_mem();
#endif

    return result;
}



static int fatfs_probe( const char* pzDevPath, fs_info* fss )

{
    int	result;
    nspace	*vol;
    debug_attr = 0;
    debug_dir = 0;
    debug_dlist = 0;
    debug_dosfs = 0;
    debug_encodings = 0;
    debug_fat = 0;
    debug_file = 0;
    debug_iter = 0;
    debug_vcache = 0;

      // Try and mount volume as a FAT volume
    if ((result = mount_fat_disk(pzDevPath, -1, 0, &vol)) == B_NO_ERROR) {
	char name[32];
	int  i;

	if (check_nspace_magic(vol, "dosfs_mount")) return -EINVAL;
		
	sprintf(name, "fat lock %x", vol->id);
	if ((result = new_lock(&(vol->vlock), name)) != 0) {
	    printk("error creating lock (%s)\n", strerror(result));
	    goto error;
	}

	  // File system flags.
	fss->fi_flags = vol->flags | FS_CAN_MOUNT;
	  // FS block size.
	fss->fi_block_size = vol->bytes_per_sector * vol->sectors_per_cluster;
	  // IO size - specifies buffer size for file copying
	fss->fi_io_size = 65536;
	  // Total blocks
	fss->fi_total_blocks = vol->total_clusters;
	  // Free blocks
	fss->fi_free_blocks = vol->free_clusters;
	fss->fi_free_user_blocks = fss->fi_free_blocks;

	if (vol->vol_entry > -2)
	    strncpy(fss->fi_volume_name, vol->vol_label, sizeof(fss->fi_volume_name));
	else
	    strcpy(fss->fi_volume_name, "no name    ");

	  // XXX: should sanitize name as well
	for (i=10;i>0;i--)
	    if (fss->fi_volume_name[i] != ' ')
		break;
	fss->fi_volume_name[i+1] = 0;
	for (;i>=0;i--) {
	    if ((fss->fi_volume_name[i] >= 'A') && (fss->fi_volume_name[i] <= 'Z'))
		fss->fi_volume_name[i] += 'a' - 'A';
	}
	fatfs_unmount( vol );
    }
    return result;
error:
    shutdown_device_cache(vol->fd/*, NO_WRITES*/);
    dlist_uninit(vol);
    uninit_vcache(vol);
    free(vol);
    return -EINVAL;
}




static int fatfs_mount( kdev_t nFsID, const char* pzDevPath, uint32 nFlags, const void* pArgs, int nArgLen,
			void** ppVolData, ino_t* pnRootIno )

{
    int	result;
    nspace	*vol;
    
#if 0    
    void *handle;

    handle = load_driver_settings("dos");
    debug_attr = strtoul(get_driver_parameter(handle, "debug_attr", "0", "0"), NULL, 0);
    debug_dir = strtoul(get_driver_parameter(handle, "debug_dir", "0", "0"), NULL, 0);
    debug_dlist = strtoul(get_driver_parameter(handle, "debug_dlist", "0", "0"), NULL, 0);
    debug_dosfs = strtoul(get_driver_parameter(handle, "debug_dosfs", "0", "0"), NULL, 0);
    debug_encodings = strtoul(get_driver_parameter(handle, "debug_encodings", "0", "0"), NULL, 0);
    debug_fat = strtoul(get_driver_parameter(handle, "debug_fat", "0", "0"), NULL, 0);
    debug_file = strtoul(get_driver_parameter(handle, "debug_file", "0", "0"), NULL, 0);
    debug_iter = strtoul(get_driver_parameter(handle, "debug_iter", "0", "0"), NULL, 0);
    debug_vcache = strtoul(get_driver_parameter(handle, "debug_vcache", "0", "0"), NULL, 0);
    unload_driver_settings(handle);
#elif 0
    debug_attr = 1;
    debug_dir = 1;
    debug_dlist = 1;
    debug_dosfs = 1;
    debug_encodings = 1;
    debug_fat = 1;
    debug_file = 1;
    debug_iter = 1;
    debug_vcache = 1;
#else
    debug_attr = 1;
    debug_dir = 0;
    debug_dlist = 0;
    debug_dosfs = 0;
    debug_encodings = 0;
    debug_fat = 0;
    debug_file = 0;
    debug_iter = 0;
    debug_vcache = 0;
#endif
      /* parms and len are command line options; dosfs doesn't use any so
	 we can ignore these arguments */
//    TOUCH(parms); TOUCH(len);

#ifdef __RO__
      // make it read-only
    nFlags |= 1;
#endif

    if (ppVolData == NULL) {
	printk("dosfs_mount passed NULL ppVolData pointer\n");
	return -EINVAL;
    }
      // Try and mount volume as a FAT volume
    if ((result = mount_fat_disk(pzDevPath, nFsID, nFlags, &vol)) == B_NO_ERROR) {
	char name[32];


	if (check_nspace_magic(vol, "dosfs_mount")) return -EINVAL;
		
	*pnRootIno = vol->root_vnode.vnid;
	*ppVolData = (void*)vol;

#ifndef __ATHEOS__	
	  // You MUST do this. Create the vnode for the root.
	result = new_vnode(nFsID, *pnRootIno, (void*)&(vol->root_vnode));
	if (result != B_NO_ERROR) {
	    printk("error creating new vnode (%s)\n", strerror(result));
	    goto error;
	}
#endif	
	sprintf(name, "fat lock %x", vol->id);
	if ((result = new_lock(&(vol->vlock), name)) != 0) {
	    printk("error creating lock (%s)\n", strerror(result));
	    goto error;
	}

#ifdef DEBUG
	load_driver_symbols("dos");
#endif
    }
    return result;

error:
    shutdown_device_cache(vol->fd/*, NO_WRITES*/);
    dlist_uninit(vol);
    uninit_vcache(vol);
    free(vol);
    return -EINVAL;
}


// dosfs_rfsstat - Fill in fs_info struct for device.
static int dosfs_rfsstat(void* _vol, fs_info* fss )
{
	nspace* vol = (nspace*)_vol;
	int i;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_rfsstat")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	DPRINTF(1, ("dosfs_rfsstat called\n"));

	// fss->dev and fss->root filled in by kernel
	
	// File system flags.
	fss->fi_flags = vol->flags;
	
	// FS block size.
	fss->fi_block_size = vol->bytes_per_sector * vol->sectors_per_cluster;

	// IO size - specifies buffer size for file copying
	fss->fi_io_size = 65536;
	
	// Total blocks
	fss->fi_total_blocks = vol->total_clusters;

	// Free blocks
	fss->fi_free_blocks = vol->free_clusters;
	fss->fi_free_user_blocks = fss->fi_free_blocks;

	// Device name.
//	strncpy(fss->device_name, vol->device, sizeof(fss->device_name));

	if (vol->vol_entry > -2)
		strncpy(fss->fi_volume_name, vol->vol_label, sizeof(fss->fi_volume_name));
	else
		strcpy(fss->fi_volume_name, "no name    ");

	// XXX: should sanitize name as well
	for (i=10;i>0;i--)
		if (fss->fi_volume_name[i] != ' ')
			break;
	fss->fi_volume_name[i+1] = 0;
	for (;i>=0;i--) {
		if ((fss->fi_volume_name[i] >= 'A') && (fss->fi_volume_name[i] <= 'Z'))
			fss->fi_volume_name[i] += 'a' - 'A';
	}

	// File system name
//	strcpy(fss->fsh_name, "fat");

	ioctl( vol->fd, IOCTL_GET_DEVICE_PATH, fss->fi_device_path );
	
	UNLOCK_VOL(vol);

	return 0;
}


static int dosfs_wfsstat(void *_vol, const fs_info* fss, uint32 mask )
{
	int result = -EIO;
	nspace* vol = (nspace*)_vol;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_wfsstat")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	DPRINTF(0, ("dosfs_wfsstat called\n"));

	/* if it's a r/o file system and not the special hack, then don't allow
	 * volume renaming */
	if ((vol->flags & FS_IS_READONLY) && memcmp(vol->vol_label, "__RO__     ", 11)) {
		UNLOCK_VOL(vol);
		return -EROFS;
	}

	if (mask & WFSSTAT_NAME) {
		// sanitize name
		char name[11];
		int i,j;
		memset(name, ' ', 11);
		DPRINTF(1, ("wfsstat: setting name to %s\n", fss->fi_volume_name));
		for (i=j=0;(i<11)&&(fss->fi_volume_name[j]);j++) {
			static char acceptable[] = "!#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~";
			char c = fss->fi_volume_name[j];
			if ((c >= 'a') && (c <= 'z')) c += 'A' - 'a';
			// spaces acceptable in volume names
			if (strchr(acceptable, c) || (c == ' '))
				name[i++] = c;
		}
		if (i == 0) { // bad name, kiddo
			result = -EINVAL;
			goto bi;
		}
		DPRINTF(1, ("wfsstat: sanitized to [%11.11s]\n", name));

		if (vol->vol_entry == -1) {
			// stored in the bpb
			uchar *buffer;
			if ((buffer = get_cache_block(vol->fd, 0, vol->bytes_per_sector)) == NULL) {
				result = -EIO;
				goto bi;
			}
			if ((buffer[0x26] != 0x29) || memcmp(buffer + 0x2b, vol->vol_label, 11)) {
				printk("dosfs_wfsstat: label mismatch\n");
				result = -EINVAL;
			} else {
				memcpy(buffer + 0x2b, name, 11);
				mark_blocks_dirty(vol->fd, 0, 1);
				result = 0;
			}
			release_cache_block(vol->fd, 0);
		} else if (vol->vol_entry >= 0) {
			struct diri diri;
			uint8 *buffer;
			buffer = diri_init(vol, vol->root_vnode.cluster, vol->vol_entry, &diri);

			// check if it is the same as the old volume label
			if ((buffer == NULL) || (memcmp(buffer, vol->vol_label, 11))) {
				printk("dosfs_wfsstat: label mismatch\n");
				diri_free(&diri);
				result = -EINVAL;
				goto bi;
			}
			memcpy(buffer, name, 11);
			diri_mark_dirty(&diri);
			diri_free(&diri);
			result = 0;
		} else {
			uint32 index;
			result = create_volume_label(vol, name, &index);
			if (result == B_OK) vol->vol_entry = index;
		}

		if (result == 0)
			memcpy(vol->vol_label, name, 11);
	}
	
bi:	UNLOCK_VOL(vol);

	return result;
}

static int fatfs_ioctl(void *_vol, void *_node, void *cookie, int code, 
	void *buf, bool bFromKernel )
{
    status_t result = B_OK;
    nspace *vol = (nspace *)_vol;
    vnode *node = (vnode *)_node;

    TOUCH(cookie); TOUCH(buf); TOUCH(bFromKernel);

    LOCK_VOL(vol);

    if (check_nspace_magic(vol, "fatfs_ioctl") ||
	check_vnode_magic(node, "fatfs_ioctl")) {
	UNLOCK_VOL(vol);
	return -EINVAL;
    }

    switch (code) {
	case 10002 : /* return real creation time */
	    if (buf) *(bigtime_t *)buf = node->st_time;
	    break;
	case 10003 : /* return real last modification time */
	    if (buf) *(bigtime_t *)buf = node->st_time;
	    break;
	case 100000 :
	    printk("built at %s on %s\n", build_time, build_date);
	    printk("vol info: %s (device %x, media descriptor %x)\n", vol->device, vol->fd, vol->media_descriptor);
	    printk("%x bytes/sector, %x sectors/cluster\n", vol->bytes_per_sector, vol->sectors_per_cluster);
	    printk("%x reserved sectors, %x total sectors\n", vol->reserved_sectors, vol->total_sectors);
	    printk("%x %d-bit fats, %x sectors/fat, %x root entries\n", vol->fat_count, vol->fat_bits, vol->sectors_per_fat, vol->root_entries_count);
	    printk("root directory starts at sector %x (cluster %x), data at sector %x\n", vol->root_start, vol->root_vnode.cluster, vol->data_start);
	    printk("%x total clusters, %x free\n", vol->total_clusters, vol->free_clusters);
	    printk("fat mirroring is %s, fs info sector at sector %x\n", (vol->fat_mirrored) ? "on" : "off", vol->fsinfo_sector);
	    printk("last allocated cluster = %x\n", vol->last_allocated);
	    printk("root vnode id = %Lx\n", vol->root_vnode.vnid);
	    printk("volume label [%11.11s]\n", vol->vol_label);
	    break;
			
	case 100001 :
	    printk("vnode id %Lx, dir vnid = %Lx\n", node->vnid, node->dir_vnid);
	    printk("si = %x, ei = %x\n", node->sindex, node->eindex);
	    printk("cluster = %x (%x), mode = %x, size = %Lx\n", node->cluster, vol->data_start + vol->sectors_per_cluster * (node->cluster - 2), node->fn_nMode, node->st_size);
	    printk("mime = %s\n", node->mime ? node->mime : "(null)");
	    dump_fat_chain(vol, node->cluster);
	    break;

	case 100002 :
	{struct diri diri;
	uint8 *buffer;
	uint32 i;
	for (i=0,buffer=diri_init(vol,node->cluster, 0, &diri);buffer;buffer=diri_next_entry(&diri),i++) {
	    if (buffer[0] == 0) break;
	    printk("entry %x:\n", i);
	    dump_directory(buffer);
	}
	diri_free(&diri);}
	break;

	case 100003 :
	    printk("vcache validation not yet implemented\n");
#if 0
	    printk("validating vcache for %lx\n", vol->id);
	    validate_vcache(vol);
	    printk("validation complete for %lx\n", vol->id);
#endif
	    break;

	case 100004 :
	    printk("dumping vcache for %x\n", vol->id);
	    dump_vcache(vol);
	    break;

	case 100005 :
	    printk("dumping dlist for %x\n", vol->id);
	    dlist_dump(vol);
	    break;

	default :
	    printk("fatfs_ioctl: vol %x, vnode %Lx code = %d\n", vol->id, node->vnid, code);
	    result = -EINVAL;
	    break;
    }

    UNLOCK_VOL(vol);

    return result;
}

static int fatfs_sync(void *_vol)
{
    nspace *vol = (nspace *)_vol;
	
    LOCK_VOL(vol);

    if (check_nspace_magic((nspace*)_vol, "fatfs_sync")) {
	UNLOCK_VOL(vol);
	return -EINVAL;
    }
	
    DPRINTF(0, ("fatfs_sync called on volume %x\n", vol->id));

    update_fsinfo(vol);
    flush_device_cache(vol->fd, false );

    UNLOCK_VOL(vol);

    return 0;
}

static int fatfs_open( void* pVolume, void* pNode, int nMode, void** ppCookie )
{
    vnode* psNode = (vnode*)pNode;
    
    if ( psNode->fn_nMode & FAT_SUBDIR ) {
	return( dosfs_opendir( pVolume, pNode, ppCookie ) );
    } else {
	return( dosfs_open( pVolume, pNode, nMode, ppCookie ) );
    }
}

int fatfs_close( void* pVolume, void* pNode, void* pCookie )
{
    vnode* psNode = (vnode*)pNode;
    int nError;
    
    if ( psNode->fn_nMode & FAT_SUBDIR ) {
	nError = dosfs_closedir( pVolume, pNode, pCookie );
	dosfs_free_dircookie( pVolume, pNode, pCookie );
    } else {
	nError = dosfs_close( pVolume, pNode, pCookie );
	dosfs_free_cookie( pVolume, pNode, pCookie );
    }
    return( nError );
}

static FSOperations_s	g_sOperations =
{
    fatfs_probe,		// op_probe
    fatfs_mount,		// op_mount
    fatfs_unmount,		// op_unmount
    dosfs_read_vnode,		// op_read_inode
    dosfs_write_vnode,		// op_write_inode
    fatfs_locate_inode,		// op_locate_inode
    NULL,			// op_access
    dosfs_create,		// op_create
    dosfs_mkdir,		// op_mkdir
    NULL,			// op_mknod
    NULL,			// op_symlink
    NULL,			// op_link
    dosfs_rename, 		// op_rename
    dosfs_unlink,		// op_unlink
    dosfs_rmdir, 		// op_rmdir
    NULL,			// op_readlink
    NULL, 			// op_opendir
    NULL,			// op_closedir
    dosfs_rewinddir, 		// op_rewinddir
    dosfs_readdir,		// op_readdir
    fatfs_open,			// op_open
    fatfs_close,		// op_close
    NULL, 			// op_free_cookie
    dosfs_read,			// op_read
    dosfs_write,		// op_write
    NULL,			// op_readv
    NULL,			// op_writev
    fatfs_ioctl,		// op_ioctl
    NULL, 			// op_setflags
    dosfs_rstat, 		// op_rstat
    dosfs_wstat,		// op_wstat
    NULL,			// op_fsync
    NULL,		 	// op_initialize
    fatfs_sync,			// op_sync
    dosfs_rfsstat, 		// op_rfsstat
    dosfs_wfsstat,		// op_wfsstat
    NULL,			// op_isatty
    NULL,    			// op_add_select_req
    NULL,    			// op_rem_select_req
    NULL,			// op_open_attrdir
    NULL,			// op_close_attrdir
    NULL,    			// op_rewind_attrdir
    NULL,			// op_read_attrdir
    NULL,			// op_remove_attr
    NULL,    			// op_rename_attr
    NULL,			// op_stat_attr
    NULL,			// op_write_attr
    NULL,			// op_read_attr
    NULL,    			// op_open_indexdir
    NULL,    			// op_close_indexdir
    NULL,    			// op_rewind_indexdir
    NULL,    			// op_read_indexdir
    NULL,    			// op_create_index
    NULL,    			// op_remove_index
    NULL,    			// op_rename_index
    NULL,    			// op_stat_index
    NULL,			// op_get_stream_blocks
    NULL			// op_truncate
};

int fs_init( const char* pzName, FSOperations_s** ppsOps )
{
  *ppsOps = &g_sOperations;
  return( FSDRIVER_API_VERSION );
}
