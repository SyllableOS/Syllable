/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

typedef long long dr9_off_t;

#include <atheos/filesystem.h>
#include <atheos/semaphore.h>
#include <atheos/bcache.h>

#include <atheos/string.h>
#include <atheos/time.h>
#include <atheos/kdebug.h>
#include <atheos/kernel.h>

#include <posix/errno.h>

#include <macros.h>

#include "iter.h"
#include "dosfs.h"
#include "attr.h"
#include "dir.h"
#include "dlist.h"
#include "fat.h"
#include "util.h"
#include "vcache.h"

#include "encodings.h"

#define DPRINTF(a,b) if (debug_dir > (a)) printk b

#define DIRCOOKIE_MAGIC 'AiC '

typedef struct dircookie
{
	uint32		magic;
	uint32		current_index;
} dircookie;

static CHECK_MAGIC(dircookie,struct dircookie, DIRCOOKIE_MAGIC)

// private structure for returning data from _next_dirent_()
struct _dirent_info_ {
	uint32 sindex;
	uint32 eindex;
	uint32 mode;
	uint32 cluster;
	uint32 size;
	uint32 time;
};

// scans dir for the next entry, using the state stored in a struct diri.
static status_t _next_dirent_(struct diri *iter, struct _dirent_info_ *oinfo,
	char *filename, int len)
{
	uint8		*buffer, hash = 0; /* quiet warning */
	uchar		uni[1024], *puni;
	uint32		i;

	// lfn state
	uint32		start_index = 0xffff, filename_len = 0 /* quiet warning */,
				lfn_count = 0 /* quiet warning */;

	if (check_diri_magic(iter, "_next_dirent_")) return -EINVAL;

	if (iter->current_block == NULL)
		return -ENOENT;

	if (len < 15) {
		DPRINTF(0, ("_next_dirent_: len too short (%x)\n", len));
		return -ENOMEM;
	}

	buffer = iter->current_block + ((iter->current_index) % (iter->csi.vol->bytes_per_sector / 0x20)) * 0x20;

	for (;buffer != NULL;buffer = diri_next_entry(iter)) {
		DPRINTF(2, ("_next_dirent_: %x/%x/%x\n", iter->csi.cluster, iter->csi.sector, iter->current_index));
		if (buffer[0] == 0) { // quit if at end of table
			if (start_index != 0xffff) {
				printk("dosfs: lfn entry (%s) with no alias\n", filename);
			}
			return -ENOENT;
		}
	
		if (buffer[0] == 0xe5) { // skip erased entries
			if (start_index != 0xffff) {
				printk("dosfs: lfn entry (%s) with intervening erased entries\n", filename);
				start_index = 0xffff;
			}
			DPRINTF(2, ("entry erased, skipping...\n"));
			continue;
		}
		
		if (buffer[0xb] == 0xf) { // long file name
			if ((buffer[0xc] != 0) || 
				(buffer[0x1a] != 0) || (buffer[0x1b] != 0)) {
				printk("dosfs: invalid long file name: reserved fields munged\n");
				continue;
			}
			if (start_index == 0xffff) {
				if ((buffer[0] & 0x40) == 0) {
					printk("dosfs: bad lfn start entry in directory\n");
					continue;
				}
				hash = buffer[0xd];
				lfn_count = buffer[0] & 0x1f;
				start_index = iter->current_index;
				puni = uni + 2*13*(lfn_count - 1);
				for (i=1;i<0x20;i+=2) {
					if ((buffer[i] == 0xff) && (buffer[i+1] == 0xff))
						break;
					*(puni++) = buffer[i+1];
					*(puni++) = buffer[i];
					if (i == 0x9) i+=3;
					if (i == 0x18) i+=2;
				}
				*(puni++) = 0;
				*(puni++) = 0;
				filename_len = (puni+1) - uni;

				continue;
			} else {
				if (buffer[0xd] != hash) {
					printk("dosfs: error in long file name: hash values don't match\n");
					start_index = 0xffff;
					continue;
				}
				if (buffer[0] != --lfn_count) {
					printk("dosfs: bad lfn entry in directory\n");
					start_index = 0xffff;
					continue;
				}

				puni = uni + 2*13*(lfn_count - 1);
				for (i=1;i<0x20;i+=2) {
					if ((buffer[i] == 0xff) && (buffer[i+1] == 0xff)) {
						printk("dosfs: bad lfn entry in directory\n");
						start_index = 0xffff;
						break;
					}
					*(puni++) = buffer[i+1];
					*(puni++) = buffer[i];
					if (i == 0x9) i+=3;
					if (i == 0x18) i+=2;
				}
				continue;
			}
		}

		break;
	}

	// hit end of directory entries with no luck
	if (buffer == NULL)
		return -ENOENT;

	// process long name
	if (start_index != 0xffff) {
		if (lfn_count != 1) {
			printk("dosfs: unfinished lfn in directory\n"); 
			start_index = 0xffff;
		} else {
			if (unicode_to_utf8(uni, filename_len, (uint8*)filename, len)) {
				// rewind to beginning of call
				printk("dosfs: error: long file name too long\n");
				diri_init(iter->csi.vol, iter->starting_cluster, start_index, iter);
				return -ENAMETOOLONG;
			} else if (hash_msdos_name((const char *)buffer) != hash) {
				printk("dosfs: error: long file name (%s) hash and short file name don't match\n", filename);
				start_index = 0xffff;
			}
		}
	}

	// process short name
	if (start_index == 0xffff) {
        start_index = iter->current_index;
		// buffer[0xc] contains lowercase flags for base and extension
		msdos_to_utf8(buffer, (uchar *)filename, len, buffer[0xc]);
	}

	if (oinfo) {
		oinfo->sindex = start_index;
		oinfo->eindex = iter->current_index;
		oinfo->mode = buffer[0xb];
		oinfo->cluster = read16(buffer,0x1a);
		if (iter->csi.vol->fat_bits == 32)
			oinfo->cluster += 0x10000*read16(buffer,0x14);
		oinfo->size = read32(buffer,0x1c);
		oinfo->time = read32(buffer,0x16);
	}

	diri_next_entry(iter);

	return B_NO_ERROR;
}

static status_t get_next_dirent(nspace *vol, vnode *dir, struct diri *iter,
				ino_t *vnid, char *filename, int len)
{
	struct _dirent_info_ info;
	status_t result;

	if (check_nspace_magic(vol, "get_next_dirent")) return -EINVAL;

	do {
		result = _next_dirent_(iter, &info, filename, len);
	
		if (result < 0) return result;
		// only hide volume label entries in the root directory
	} while ((info.mode & FAT_VOLUME) && (dir->vnid == vol->root_vnode.vnid));
	
	if (!strcmp(filename, ".")) {
		// assign vnode based on parent
		if (vnid) *vnid = dir->vnid;
	} else if (!strcmp(filename, "..")) {
		// assign vnode based on parent of parent
		if (vnid) *vnid = dir->dir_vnid;
	} else {
		if (vnid) {
			ino_t loc = (IS_DATA_CLUSTER(info.cluster)) ?
					GENERATE_DIR_CLUSTER_VNID(dir->vnid, info.cluster) :
					GENERATE_DIR_INDEX_VNID(dir->vnid, info.sindex);

			/* if it matches a loc in the lookup table, we are done. */
			if (vcache_loc_to_vnid(vol, loc, vnid) != B_OK) {
				/* ...else check if it matches any vnid's in the lookup table */
				if (find_vnid_in_vcache(vol, loc) == B_OK) {
					/* if it does, create a random one since we can't reuse
					 * existing vnid's */
					*vnid = generate_unique_vnid(vol);
					/* and add it to the vcache */
					if ((result = add_to_vcache(vol, *vnid, loc)) < 0)
						return result;
				} else {
					/* otherwise we are free to use it */
					*vnid = loc;
				}
			}

			if (info.mode & FAT_SUBDIR) {
				if (dlist_find(vol, info.cluster) == -1LL) {
					if ((result = dlist_add(vol, *vnid)) < 0) {
						return result;
					}
				}
			}
		}
	}

	DPRINTF(2, ("get_next_dirent: found %s (vnid %Lx)\n", filename, *vnid));

	return B_NO_ERROR;
}

status_t check_dir_empty(nspace *vol, vnode *dir)
{
	int i;
	struct diri iter;
	status_t result = -EIO; /* quiet warning */

	if (check_nspace_magic(vol, "check_dir_empty")) return -EINVAL;
	if (check_vnode_magic(dir, "check_dir_empty")) return -EINVAL;

	if (diri_init(vol, dir->cluster, 0, &iter) == NULL) {
		printk("dosfs: check_dir_empty: error opening directory\n");
		return -EIO;
	}

	i = (dir->vnid == vol->root_vnode.vnid) ? 2 : 0;

	for (;i<3;i++) {
		char filename[512];
		result = _next_dirent_(&iter, NULL, filename, 512);

		if (result < 0) {
			if ((i == 2) && (result == -ENOENT)) result = B_OK;
			break;
		}

		if (((i == 0) && strcmp(filename, ".")) ||
			((i == 1) && strcmp(filename, "..")) ||
			// weird case where ./.. are stored as long file names
			((i < 2) && (iter.current_index != i+1))) {
			printk("dosfs: check_dir_empty: malformed directory\n");
			return -ENOTDIR;
		}

		result = -ENOTEMPTY;
	}

	diri_free(&iter);

	return result;
}

status_t findfile(nspace *vol, vnode *dir, const char *file,
		ino_t *vnid, vnode **node)
{
	/* Starting at the base, find the file in the subdir
	   and return its vnode id */
	int		result = 0;
	ino_t	_vnid;

	if (check_nspace_magic(vol, "findfile")) return -EINVAL;
	if (check_vnode_magic(dir, "findfile")) return -EINVAL;

	DPRINTF(1, ("findfile: %s in %Lx\n", file, dir->vnid));

	if ((strcmp(file,".") == 0) && (dir->vnid == vol->root_vnode.vnid)) {
		_vnid = dir->vnid;
	} else if ((strcmp(file, "..") == 0) && (dir->vnid == vol->root_vnode.vnid)) {
		_vnid = dir->dir_vnid;
	} else {
		char filename[512];
		struct diri diri;

		// XXX: do it in a smarter way
		diri_init(vol, dir->cluster, 0, &diri);

		while (1)
		{
			result = get_next_dirent(vol, dir, &diri, &_vnid, filename, 512);

			if (result != B_NO_ERROR) {
				result = -ENOENT;
				break;
			}
			
			if (!strcmp(filename, file)) {
				result = B_OK;
				break;
			}
		}
		diri_free(&diri);
	}
	if (result == B_OK) {
		if (vnid) *vnid = _vnid;
		if (node) result = get_vnode(vol->id, _vnid, (void **)node);
	}
	return result;
}

status_t erase_dir_entry(nspace *vol, vnode *node)
{
	status_t result;
	uint32 i;
	char filename[512];
	uint8 *buffer;
	struct _dirent_info_ info;
	struct diri diri;
	
	DPRINTF(0, ("erasing directory entries %x through %x\n", node->sindex, node->eindex));
	buffer = diri_init(vol,VNODE_PARENT_DIR_CLUSTER(node), node->sindex, &diri);

	// first pass: check if the entry is still valid
	if (buffer == NULL) {
		printk("dosfs: erase_dir_entry: error reading directory\n");
		return -ENOENT;
	}

	result = _next_dirent_(&diri, &info, filename, 512);
	diri_free(&diri);

	if (result < 0) return result;
	
	if ((info.sindex != node->sindex) ||
		(info.eindex != node->eindex)) {
		// any other attributes may be in a state of flux due to wstat calls
		printk("dosfs: erase_dir_entry: directory entry doesn't match\n");
		return -EIO;
	}

	// second pass: actually erase the entry
	buffer = diri_init(vol, VNODE_PARENT_DIR_CLUSTER(node), node->sindex, &diri);
	for (i=node->sindex;(i<=node->eindex)&&(buffer);buffer=diri_next_entry(&diri),i++) {
		buffer[0] = 0xe5; // mark entry erased
		diri_mark_dirty(&diri);
	}
	diri_free(&diri);

	return 0;
}

// shrink directory to the size needed
// errors here are neither likely nor problematic
// w95 doesn't seem to do this, so it's possible to create a
// really large directory that consumes all available space!
status_t compact_directory(nspace *vol, vnode *dir)
{
	uint32 last = 0;
	struct diri diri;
	status_t error = -EIO; /* quiet warning */
	int loops=0;
	DPRINTF(0, ("compacting directory with vnode id %Lx\n", dir->vnid));

	// root directory can't shrink in fat12 and fat16
	if (IS_FIXED_ROOT(dir->cluster))
		return 0;

	diri_init(vol, dir->cluster, 0, &diri);
	while (diri.current_block) {
		char filename[512];
		struct _dirent_info_ info;

		if ( loops++ > 1000 ) {
		    printk( "dosfs: Infinite loop in _create_dir_entry_()\n" );
		    break;
		}
		
		error = _next_dirent_(&diri, &info, filename, 512);

		if (error == B_OK) {
			// don't compact away volume labels in the root dir
			if (!(info.mode & FAT_VOLUME) || (dir->vnid != vol->root_vnode.vnid))
				last = diri.current_index;
		} else if (error == -ENOENT) {
			uint32 clusters = (last + vol->bytes_per_sector / 0x20 * vol->sectors_per_cluster - 1) / (vol->bytes_per_sector / 0x20) / vol->sectors_per_cluster;
			error = 0;

			// special case for fat32 root directory; we don't want
			// it to disappear
			if (clusters == 0) clusters = 1;

			if (clusters * vol->bytes_per_sector * vol->sectors_per_cluster < dir->st_size) {
				DPRINTF(0, ("shrinking directory to %x clusters\n", clusters));
				error = set_fat_chain_length(vol, dir, clusters, true);
				dir->st_size = clusters*vol->bytes_per_sector*vol->sectors_per_cluster;
				dir->iteration++;
			}
			break;
		} else {
			printk("dosfs: compact_directory: unknown error from _next_dirent_ (%s)\n", strerror(error));
			break;
		}
	}
	diri_free(&diri);

	return error;
}

// name is array of char[11] as returned by findfile
static status_t find_short_name(nspace *vol, vnode *dir, const uchar *name)
{
	struct diri diri;
	uint8 *buffer;
	status_t result = -ENOENT;
	
	buffer = diri_init(vol, dir->cluster, 0, &diri);
	while (buffer) {
		if (buffer[0] == 0)
			break;

		if (buffer[0xb] != 0xf) { // not long file name
			if (!memcmp(name, buffer, 11)) {
				result = B_OK;
				break;
			}
		}

		buffer = diri_next_entry(&diri);
	}

	diri_free(&diri);

	return result;
}

struct _entry_info_ {
	uint32 mode;
	uint32 cluster;
	uint32 size;
	time_t time;
};

static status_t _create_dir_entry_(nspace *vol, vnode *dir, struct _entry_info_ *info,
	const char nshort[11], const char *nlong, uint32 len, uint32 *ns, uint32 *ne)
{
	status_t error = -EIO; /* quiet warning */
	uint32 required_entries, i;
	uint8 *buffer, hash;
	bool last_entry;
	struct diri diri;
	int loops = 0;
	// short name cannot be the same as that of a device
	// this list was created by running strings on io.sys
	const char *device_names[] = {
		"CON        ",
		"AUX        ",
		"PRN        ",
		"CLOCK$     ",
		"COM1       ",
		"LPT1       ",
		"LPT2       ",
		"LPT3       ",
		"COM2       ",
		"COM3       ",
		"COM4       ",
		"CONFIG$    ",
		NULL
	};

	// check short name against device names
	for (i=0;device_names[i];i++) {
		if (!memcmp(nshort, device_names[i], 11))
			return -EPERM;
	}

	if ((info->cluster != 0) && !IS_DATA_CLUSTER(info->cluster)) {
		printk("dosfs: _create_dir_entry_ for bad cluster (%x)\n", info->cluster);
		return -EINVAL;
	}

	/* convert byte length of unicode name to directory entries */
	required_entries = (len + 24) / 26 + 1;

	// find a place to put the entries
	*ns = 0;
	last_entry = true;
	diri_init(vol, dir->cluster, 0, &diri);
	while (diri.current_block) {
		char filename[512];
		struct _dirent_info_ info;

		if ( loops++ > 1000 ) {
		    printk( "dosfs: Infinite loop in _create_dir_entry_()\n" );
		    break;
		}
		error = _next_dirent_(&diri, &info, filename, 512);
		if (error == B_OK) {
			if (info.sindex - *ns >= required_entries) {
				last_entry = false;
				break;
			}
			*ns = diri.current_index;
		} else if (error == -ENOENT) {
			// hit end of directory marker
			break;
		} else {
			printk("dosfs: _create_dir_entry_: unknown error from _next_dirent_ (%s)\n", strerror(error));
			break;
		}
	}

	// if at end of directory, last_entry flag will be true as it should be

	diri_free(&diri);
	
	if ((error != B_OK) && (error != -ENOENT)) return error;

	*ne = *ns + required_entries - 1;

	DPRINTF(0, ("directory entry runs from %x to %x (dirsize = %Lx) (is%s last entry)\n", *ns, *ne, dir->st_size, last_entry ? "" : "n't"));

	// check if the directory needs to be expanded
	if (*ne * 0x20 >= dir->st_size) {
		uint32 clusters_needed;

		// can't expand fat12 and fat16 root directories :(
		if (IS_FIXED_ROOT(dir->cluster)) {
			DPRINTF(0, ("_create_dir_entry_: out of space in root directory\n"));
			return -ENOSPC;
		}
	
		// otherwise grow directory to fit
		clusters_needed = ((*ne + 1) * 0x20 +
			vol->bytes_per_sector*vol->sectors_per_cluster - 1) /
			vol->bytes_per_sector / vol->sectors_per_cluster;

		DPRINTF(0, ("expanding directory from %Lx to %x clusters\n", dir->st_size/vol->bytes_per_sector/vol->sectors_per_cluster, clusters_needed));
		if ((error = set_fat_chain_length(vol, dir, clusters_needed, true)) < 0)
			return error;
		dir->st_size = vol->bytes_per_sector*vol->sectors_per_cluster*clusters_needed;
		dir->iteration++;
	}

	// starting blitting entries
	buffer = diri_init(vol,dir->cluster, *ns, &diri);
	hash = hash_msdos_name(nshort);

	// write lfn entries
	for (i=1;(i<required_entries)&&buffer;i++) {
		const char *p = nlong + (required_entries - i - 1)*26; // go to unicode offset
		memset(buffer, 0, 0x20);
		buffer[0] = required_entries - i + ((i == 1) ? 0x40 : 0);
		buffer[0x0b] = 0x0f;
		buffer[0x0d] = hash;
		memcpy(buffer+1,p,10);
		memcpy(buffer+0x0e,p+10,12);
		memcpy(buffer+0x1c,p+22,4);
		diri_mark_dirty(&diri);
		buffer = diri_next_entry(&diri);
	}

	ASSERT(buffer != NULL);
	if (buffer == NULL)	{	// this should never happen...
		DPRINTF(0, ("_create_dir_entry_: the unthinkable has occured\n"));
		diri_free(&diri);
		return -EIO;
	}

	// write directory entry
	memcpy(buffer, nshort, 11);
	buffer[0x0b] = info->mode;
	memset(buffer+0xc, 0, 0x16-0xc);
	i = time_t2dos(info->time);
	buffer[0x16] = i & 0xff;
	buffer[0x17] = (i >> 8) & 0xff;
	buffer[0x18] = (i >> 16) & 0xff;
	buffer[0x19] = (i >> 24) & 0xff;
	i = info->cluster;
	if (info->size == 0) i = 0;		// cluster = 0 for 0 byte files
	buffer[0x1a] = i & 0xff;
	buffer[0x1b] = (i >> 8) & 0xff;
	if (vol->fat_bits == 32) {
		buffer[0x14] = (i >> 16) & 0xff;
		buffer[0x15] = (i >> 24) & 0xff;
	}
	i = (info->mode & FAT_SUBDIR) ? 0 : info->size;
	buffer[0x1c] = i & 0xff;
	buffer[0x1d] = (i >> 8) & 0xff;
	buffer[0x1e] = (i >> 16) & 0xff;
	buffer[0x1f] = (i >> 24) & 0xff;
	diri_mark_dirty(&diri);
	
	if (last_entry) {
		// add end of directory markers to the rest of the
		// cluster; need to clear all the other entries or else
		// scandisk will complain.
		while ((buffer = diri_next_entry(&diri)) != NULL) {
			memset(buffer, 0, 0x20);
			diri_mark_dirty(&diri);
		}
	}
	
	diri_free(&diri);

	return 0;
}

// doesn't do any name checking
status_t create_volume_label(nspace *vol, const char name[11], uint32 *index)
{
	uint32 dummy;
	struct _entry_info_ info = {
		FAT_ARCHIVE | FAT_VOLUME, 0, 0, 0
	};
	time(&info.time);

	// check if name already exists
	if (find_short_name(vol, &(vol->root_vnode), (uchar *)name) == B_OK)
		return -EEXIST;

	return _create_dir_entry_(vol, &(vol->root_vnode), &info, name, NULL, 0, index, &dummy);
}

status_t create_dir_entry(nspace *vol, vnode *dir, vnode *node, 
	const char *name, uint32 *ns, uint32 *ne)
{
	status_t error;
	int32 len;
	unsigned char nlong[512], nshort[11];
	int encoding;
	struct _entry_info_ info;

	// check if name already exists
	if (findfile(vol,dir,name,NULL,NULL) == B_OK) {
		DPRINTF(0, ("%s already found in directory %Lx\n", name, dir->vnid));
		return -EEXIST;
	}

	// check name legality while converting. we ignore the case conversion
	// flag, i.e. (filename "blah" will always have a patched short name),
	// because the whole case conversion system in dos is brain damaged;
	// remanants of CP/M no less.

	// existing names pose a problem; in these cases, we'll just live with
	// two identical short names. not a great solution, but there's little
	// we can do about it.
	len = utf8_to_unicode(name, nlong, 512);
	if (len <= 0) {
		DPRINTF(0, ("Error converting utf8 name '%s' to unicode\n", name));
		return len ? len : -EINVAL;
	}
	memset(nlong + len, 0xff, 512 - len); /* pad with 0xff */

	error = generate_short_name((uchar *)name, nlong, len, nshort, &encoding);
	if (error) {
		DPRINTF(0, ("Error generating short name for '%s'\n", name));
		return error;
	}

	// if there is a long name, patch short name and check for duplication
	if (requires_long_name(name, nlong)) {
		char tshort[11]; // temporary short name
		int iter = 1;

		memcpy(tshort, nshort, 11);

		do {
			memcpy(nshort, tshort, 11);
			DPRINTF(0, ("trying short name %11.11s\n", nshort));
			munge_short_name1(nshort, iter, encoding);
		} while (((error = find_short_name(vol, dir, nshort)) == B_OK) && (++iter < 10));

		if ((error != B_OK) && (error != -ENOENT)) return error;

		if (error == B_OK) {
			// XXX: possible infinite loop here
			do {
				memcpy(nshort, tshort, 11);
				DPRINTF(0, ("trying short name %11.11s\n", nshort));
				munge_short_name2(nshort, encoding);
			} while ((error = find_short_name(vol, dir, nshort)) == B_OK);

			if (error != -ENOENT) return error;
		}
	} else {
		len = 0; /* entry doesn't need a long name */
	}

	DPRINTF(0, ("creating directory entry (%11.11s)\n", nshort));

	info.mode = node->fn_nMode;
	info.cluster = node->cluster;
	info.size = node->st_size;
	info.time = node->st_time;

	return _create_dir_entry_(vol, dir, &info, (char *)nshort, (char *)nlong, len, ns, ne);
}

int dosfs_read_vnode(void *_vol, ino_t vnid/*, char reenter*/, void **_node)
{
    char reenter = false;
    nspace	*vol = (nspace*)_vol;
    int		result = B_NO_ERROR;
    ino_t loc, dir_vnid;
    vnode *entry;
    struct _dirent_info_ info;
    struct diri iter;
    char filename[512]; /* need this for setting mime type */

    if (!reenter) { LOCK_VOL(vol); }

    *_node = NULL;

    if (check_nspace_magic(vol, "dosfs_read_vnode")) {
	if (!reenter) UNLOCK_VOL(vol);
	return -EINVAL;
    }

    DPRINTF(0, ("dosfs_read_vnode (vnode id %Lx)\n", vnid));

    if (vnid == vol->root_vnode.vnid)
    {
//	printk("??? dosfs_read_vnode called on root node ???\n");
	*_node = (void *)&(vol->root_vnode);
	goto bi;
    }

    if (vcache_vnid_to_loc(vol, vnid, &loc) != B_OK)
	loc = vnid;

    if (IS_ARTIFICIAL_VNID(loc) || IS_INVALID_VNID(loc)) {
	DPRINTF(0, ("dosfs_read_vnode: unknown vnid %Lx (loc %Lx)\n", vnid, loc));
	result = -ENOENT;
	goto bi;
    }

    if ((dir_vnid = dlist_find(vol, DIR_OF_VNID(loc))) == -1LL) {
	DPRINTF(0, ("dosfs_read_vnode: unknown directory at cluster %x\n", DIR_OF_VNID(loc)));
	result = -ENOENT;
	goto bi;
    }

    if (diri_init(vol, DIR_OF_VNID(loc),
		  IS_DIR_CLUSTER_VNID(loc) ? 0 : INDEX_OF_DIR_INDEX_VNID(loc),
		  &iter) == NULL) {
	printk("dosfs_read_vnode: error initializing directory for vnid %Lx (loc %Lx)\n", vnid, loc);
	result = -ENOENT;
	goto bi;
    }
	
    while (1) {
	result = _next_dirent_(&iter, &info, filename, 512);
		
	if (result < 0) {
	    printk("dosfs_read_vnode: error finding vnid %Lx (loc %Lx) (%s)\n", vnid, loc, strerror(result));
	    goto bi2;
	}

	if (IS_DIR_CLUSTER_VNID(loc)) {
	    if (info.cluster == CLUSTER_OF_DIR_CLUSTER_VNID(loc))
		break;
	} else {
	    if (info.sindex == INDEX_OF_DIR_INDEX_VNID(loc))
		break;
	    printk("dosfs_read_vnode: error finding vnid %Lx (loc %Lx) (%s)\n", vnid, loc, strerror(result));
	    result = -ENOENT;
	    goto bi2;
	}
    }
    diri_free(&iter);
    if ((entry = calloc(sizeof(struct vnode), 1)) == NULL) {
	DPRINTF(0, ("dosfs_read_vnode: out of memory\n"));
	result = -ENOMEM;
	goto bi;
    }

    entry->magic = VNODE_MAGIC;
    entry->vnid = vnid;
    entry->dir_vnid = dir_vnid;
    entry->deleted   = false;
    entry->iteration = 0;
    entry->sindex = info.sindex;
    entry->eindex = info.eindex;
    entry->cluster = info.cluster;
    entry->fn_nMode = info.mode;
    entry->st_size = info.size;
    if (info.mode & FAT_SUBDIR) {
	entry->st_size = count_clusters(vol,entry->cluster) * vol->sectors_per_cluster * vol->bytes_per_sector;
    }
    if (entry->cluster)
	entry->end_cluster = get_nth_fat_entry(vol, info.cluster, 
					       (entry->st_size + vol->bytes_per_sector * vol->sectors_per_cluster - 1) /
					       vol->bytes_per_sector / vol->sectors_per_cluster - 1);
    else
	entry->end_cluster = 0;
    entry->st_time = dos2time_t(info.time);

    set_mime_type(entry, filename);

    *_node = entry;

    if (!reenter) UNLOCK_VOL(vol);
    return result;
bi2:
    diri_free(&iter);
bi:
    if (!reenter) UNLOCK_VOL(vol);

    if (result != B_OK) DPRINTF(0, ("dosfs_read_vnode (%s)\n", strerror(result)));

    return result;
}

/* dosfs_walk - the walk function just "walks" through a directory looking for
	the specified file. When you find it, call get_vnode on its vnid to init
	it for the kernel.
*/

//int dosfs_walk(void *_vol, void *_dir, const char *file, char **newpath, ino_t *vnid)
int fatfs_locate_inode( void* pVolume, void* pParentNode, const char* pzName, int nNameLen, ino_t* pnInodeNum )
{
      /* Starting at the base, find file in the subdir, and return path
	 string and vnode id of file. */
    nspace	*vol = (nspace*)pVolume;
    vnode	*dir = (vnode*)pParentNode;
//    vnode	*vnode = NULL;
    int		result = -ENOENT;
    char	file[256];

    if ( nNameLen > 255 ) {
	return( -ENAMETOOLONG );
    }
    memcpy( file, pzName, nNameLen );
    file[nNameLen] = '\0';

    LOCK_VOL(vol);

    if (check_nspace_magic(vol, "dosfs_walk") ||
	check_vnode_magic(dir, "dosfs_walk")) {
	UNLOCK_VOL(vol);
	return -EINVAL;
    }

    DPRINTF(0, ("dosfs_walk: find %Lx/%s\n", dir->vnid, file));

    if ((result = findfile(vol, dir, file, pnInodeNum, NULL/*&vnode*/)) != B_OK) {
	DPRINTF(1, ("error finding vnode id for file %s (%s)\n", file, strerror(result)));
	result = -ENOENT;
    }
    if (result != B_OK) {
	DPRINTF(0, ("dosfs_walk (%s)\n", strerror(result)));
    } else {
	DPRINTF(0, ("dosfs_walk: found vnid %Lx\n", *pnInodeNum));
    }

    UNLOCK_VOL(vol);

    return result;
}
#ifndef __ATHEOS__
// dosfs_access - checks permissions for access.
int dosfs_access(void *_vol, void *_node, int mode)
{
	status_t result = B_OK;
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_access") ||
		check_vnode_magic(node, "dosfs_access")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	DPRINTF(0, ("dosfs_access (vnode id %Lx, mode %x)\n", node->vnid, mode));

	if ((mode & O_RWMASK) != O_RDONLY) {
		if (vol->flags & B_FS_IS_READONLY) {
			printk("dosfs_access: can't write on read-only volume\n");
			result = -EROFS;
		} else if (node->mode & FAT_READ_ONLY) {
			printk("dosfs: can't open read-only file for writing\n");
			result = -EPERM;
		}
	}

	UNLOCK_VOL(vol);

	return result;
}
#endif // __ATHEOS__

int dosfs_readlink(void *_vol, void *_node, char *buf, size_t *bufsize)
{
	TOUCH(_vol); TOUCH(_node); TOUCH(buf); TOUCH(bufsize);

	// no links in fat...
	DPRINTF(0, ("dosfs_readlink called\n"));

	return -EINVAL;
}

int dosfs_opendir(void *_vol, void *_node, void **_cookie)
{
	nspace		*vol = (nspace*)_vol;
	vnode		*node = (vnode*)_node;
	dircookie	*cookie = NULL;
	int			result = -EINVAL;

	if (_cookie == NULL) {
		printk("dosfs_opendir called with null _cookie\n");
		return -EINVAL;
	}

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_opendir") ||
		check_vnode_magic(node, "dosfs_opendir")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	DPRINTF(0, ("dosfs_opendir (vnode id %Lx)\n", node->vnid));

	*_cookie = NULL;

	if (!(node->fn_nMode & FAT_SUBDIR)) {
		/* bash will try to opendir files unless OPENDIR_NOT_ROBUST is
		 * defined, so we'll suppress this message; it's more of a problem
		 * with the application than with the file system, anyway
		 */
		DPRINTF(0, ("dosfs_opendir error: vnode not a directory\n"));
		result = -ENOTDIR;
		goto bi;
	}

	if ((cookie = (dircookie *)malloc(sizeof(dircookie))) == NULL) {
		printk("dosfs_opendir: out of memory error\n");
		goto bi;
	}

	cookie->magic = DIRCOOKIE_MAGIC;
	cookie->current_index = 0;

	result = B_NO_ERROR;
	
bi:
	*_cookie = (void*)cookie;

	if (result != B_OK) DPRINTF(0, ("dosfs_opendir (%s)\n", strerror(result)));

	UNLOCK_VOL(vol);

	return result;
}

int dosfs_readdir( void *_vol, void *_dir, void *_cookie, int nPos, struct kernel_dirent *entry, size_t bufsize )
{
    int 		result = 0;
    nspace* 	vol = (nspace*)_vol;
    vnode		*dir = (vnode *)_dir;
    dircookie* 	cookie = (dircookie*)_cookie;
    struct		diri diri;

    LOCK_VOL(vol);
	
    if (check_nspace_magic(vol, "dosfs_readdir") ||
	check_vnode_magic(dir, "dosfs_readdir") ||
	check_dircookie_magic(cookie, "dosfs_readdir")) {
	UNLOCK_VOL(vol);
	return -EINVAL;
    }

    DPRINTF(0, ("dosfs_readdir: vnode id %Lx, index %x\n", dir->vnid, cookie->current_index));

      // simulate '.' and '..' entries for root directory
    if (dir->vnid == vol->root_vnode.vnid) {
	if (cookie->current_index >= 2) {
	    cookie->current_index -= 2;
	} else {
	    if (cookie->current_index++ == 0) {
		strcpy(entry->d_name, ".");
		entry->d_namlen = 1;
//				entry->d_reclen = 2;
	    } else {
		strcpy(entry->d_name, "..");
		entry->d_namlen = 2;
//				entry->d_reclen = 3;
	    }
//			*num = 1;
	    entry->d_ino = vol->root_vnode.vnid;
//			entry->d_dev = vol->id;
	    result = 1;
	    goto bi;
	}
    }

    diri_init(vol, dir->cluster, cookie->current_index, &diri);
    result = get_next_dirent(vol, dir, &diri, &(entry->d_ino), entry->d_name, bufsize/* - sizeof(struct kernel_dirent) - 1*/ );
    cookie->current_index = diri.current_index;
    diri_free(&diri);

    if (dir->vnid == vol->root_vnode.vnid)
	cookie->current_index += 2;
	
    if (result == B_NO_ERROR) {
//		*num = 1;
	result = 1;
//		entry->d_dev = vol->id;
	entry->d_namlen = strlen(entry->d_name);
//		entry->d_reclen = strlen(entry->d_name) + 1;
	DPRINTF(0, ("dosfs_readdir: found file %s\n", entry->d_name));
    } else if (result == -ENOENT) {
	  // When you get to the end, don't return an error, just return 0
	  // in *num.
//		*num = 0;
	result = 0;
    } else {
	printk("dosfs_readdir: error returned by get_next_dirent (%s)\n", strerror(result));
    }
bi:
    if (result != B_OK) DPRINTF(0, ("dosfs_readdir (%s)\n", strerror(result)));

    UNLOCK_VOL(vol);

    return result;
}
			
int dosfs_rewinddir(void *_vol, void *_node, void* _cookie)
{
	nspace		*vol = _vol;
	vnode		*node = _node;
	dircookie	*cookie = (dircookie*)_cookie;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_rewinddir") ||
		check_vnode_magic(node, "dosfs_rewinddir") ||
		check_dircookie_magic(cookie, "dosfs_rewinddir")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	DPRINTF(0, ("dosfs_rewinddir (vnode id %Lx)\n", node->vnid));

	cookie->current_index = 0;

	UNLOCK_VOL(vol);

	return B_OK;
}

int dosfs_closedir(void *_vol, void *_node, void *_cookie)
{
	TOUCH(_vol); TOUCH(_node); TOUCH(_cookie);

	DPRINTF(0, ("dosfs_closedir called\n"));

	return 0;
}

int dosfs_free_dircookie(void *_vol, void *node, void *_cookie)
{
	nspace *vol = _vol;
	dircookie *cookie = _cookie;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_free_dircookie") ||
		check_vnode_magic((vnode *)node, "dosfs_free_dircookie") ||
		check_dircookie_magic((dircookie *)cookie, "dosfs_free_dircookie")) {
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	if (cookie == NULL) {
		printk("error: dosfs_free_dircookie called with null cookie\n");
		UNLOCK_VOL(vol);
		return -EINVAL;
	}

	DPRINTF(0, ("dosfs_free_dircookie (vnode id %Lx)\n", ((vnode*)node)->vnid));

	cookie->magic = ~DIRCOOKIE_MAGIC;
	free(cookie);

	UNLOCK_VOL(vol);

	return 0;
}

