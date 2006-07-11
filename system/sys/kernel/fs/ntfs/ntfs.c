/**
 * inode.c - NTFS kernel inode handling. Part of the Linux-NTFS project.
 *
 * Copyright (c) 2006 Jarek Pelczar <jpelczar@gmail.com>
 * Copyright (c) 2001-2005 Anton Altaparmakov
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Builder sets CFLAGS with -O2, but someone compiling manually may not have CFLAGS set */
#ifndef __OPTIMIZE__
# error You must compile this driver with "-O".
#endif

#include <ntfs.h>

#include <atheos/bitops.h>
#include <atheos/semaphore.h>
#include <posix/fcntl.h>
#include <posix/unistd.h>

#define ITEM_SIZE	2040

const char *fsname = "ntfs";
int32 api_version = FSDRIVER_API_VERSION;

static int GetDeviceBlockSize( int fd )
{
	struct stat st;
	device_geometry dg;

	kerndbg( KERN_DEBUG_LOW, "GetDeviceBlockSize( int fd = %d )\n", fd );
	if( ioctl( fd, IOCTL_GET_DEVICE_GEOMETRY, &dg ) != -EOK )
	{
		if( fstat( fd, &st ) < 0 || S_ISDIR( st.st_mode ) )
			return 0;
		return 512;	/* just assume it's a plain old file or something */
	}

	return dg.bytes_per_sector;
}

static int is_boot_sector_ntfs( uint8 *b )
{
	uint32 i;

	/* Check magic is "NTFS    " */
	if( b[3] != 0x4e )
		goto not_ntfs;
	if( b[4] != 0x54 )
		goto not_ntfs;
	if( b[5] != 0x46 )
		goto not_ntfs;
	if( b[6] != 0x53 )
		goto not_ntfs;
	for( i = 7; i < 0xb; ++i )
		if( b[i] != 0x20 )
			goto not_ntfs;
	/* Check bytes per sector value is between 512 and 4096. */
	if( b[0xb] != 0 )
		goto not_ntfs;
	if( b[0xc] > 0x10 )
		goto not_ntfs;
	/* Check sectors per cluster value is valid. */
	switch ( b[0xd] )
	{
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
		case 32:
		case 64:
		case 128:
			break;
		default:
			goto not_ntfs;
	}
	/* Check reserved sectors value and four other fields are zero. */
	for( i = 0xe; i < 0x15; ++i )
		if( b[i] != 0 )
			goto not_ntfs;
	if( b[0x16] != 0 )
		goto not_ntfs;
	if( b[0x17] != 0 )
		goto not_ntfs;
	for( i = 0x20; i < 0x24; ++i )
		if( b[i] != 0 )
			goto not_ntfs;
	/* Check clusters per file record segment value is valid. */
	if( b[0x40] < 0xe1 || b[0x40] > 0xf7 )
	{
		switch ( b[0x40] )
		{
			case 1:
			case 2:
			case 4:
			case 8:
			case 16:
			case 32:
			case 64:
				break;
			default:
				goto not_ntfs;
		}
	}
	/* Check clusters per index block value is valid. */
	if( b[0x44] < 0xe1 || b[0x44] > 0xf7 )
	{
		switch ( b[0x44] )
		{
			case 1:
			case 2:
			case 4:
			case 8:
			case 16:
			case 32:
			case 64:
				break;
			default:
				goto not_ntfs;
		}
	}
	return 1;

not_ntfs:
	return 0;
}

int ntfs_init_volume( ntfs_volume_s * vol, char *boot )
{
	int sectors_per_cluster_bits;
	int64 ll;
	off_t mft_zone_size, tc;

	/* System defined default values, in case we don't load $AttrDef. */
	vol->at_standard_information = 0x10;
	vol->at_attribute_list = 0x20;
	vol->at_file_name = 0x30;
	vol->at_volume_version = 0x40;
	vol->at_security_descriptor = 0x50;
	vol->at_volume_name = 0x60;
	vol->at_volume_information = 0x70;
	vol->at_data = 0x80;
	vol->at_index_root = 0x90;
	vol->at_index_allocation = 0xA0;
	vol->at_bitmap = 0xB0;
	vol->at_symlink = 0xC0;
	/* Sector size. */
	vol->sector_size = *( uint16 * )( boot + 0xB );
	ntfs_debug( "ntfs_init_volume: vol->sector_size = 0x%x\n", vol->sector_size );
	ntfs_debug( "ntfs_init_volume: sectors_per_cluster = " "0x%x\n", NTFS_GETU8( boot + 0xD ) );
	sectors_per_cluster_bits = ffs( NTFS_GETU8( boot + 0xD ) ) - 1;
	ntfs_debug( "ntfs_init_volume: sectors_per_cluster_bits " "= 0x%x\n", sectors_per_cluster_bits );
	vol->mft_clusters_per_record = NTFS_GETS8( boot + 0x40 );
	ntfs_debug( "ntfs_init_volume: vol->mft_clusters_per_record" " = 0x%x\n", vol->mft_clusters_per_record );
	vol->index_clusters_per_record = NTFS_GETS8( boot + 0x44 );
	ntfs_debug( "ntfs_init_volume: " "vol->index_clusters_per_record = 0x%x\n", vol->index_clusters_per_record );
	vol->cluster_size = vol->sector_size << sectors_per_cluster_bits;
	vol->cluster_size_bits = ffs( vol->cluster_size ) - 1;
	ntfs_debug( "ntfs_init_volume: vol->cluster_size = 0x%x\n", vol->cluster_size );
	ntfs_debug( "ntfs_init_volume: vol->cluster_size_bits = " "0x%x\n", vol->cluster_size_bits );
	if( vol->mft_clusters_per_record > 0 )
		vol->mft_record_size = vol->cluster_size << ( ffs( vol->mft_clusters_per_record ) - 1 );
	else
		/*
		 * When mft_record_size < cluster_size, mft_clusters_per_record
		 * = -log2(mft_record_size) bytes. mft_record_size normaly is
		 * 1024 bytes, which is encoded as 0xF6 (-10 in decimal).
		 */
		vol->mft_record_size = 1 << -vol->mft_clusters_per_record;
	vol->mft_record_size_bits = ffs( vol->mft_record_size ) - 1;
	ntfs_debug( "ntfs_init_volume: vol->mft_record_size = 0x%x" "\n", vol->mft_record_size );
	ntfs_debug( "ntfs_init_volume: vol->mft_record_size_bits = " "0x%x\n", vol->mft_record_size_bits );
	if( vol->index_clusters_per_record > 0 )
		vol->index_record_size = vol->cluster_size << ( ffs( vol->index_clusters_per_record ) - 1 );
	else
		/*
		 * When index_record_size < cluster_size,
		 * index_clusters_per_record = -log2(index_record_size) bytes.
		 * index_record_size normaly equals 4096 bytes, which is
		 * encoded as 0xF4 (-12 in decimal).
		 */
		vol->index_record_size = 1 << -vol->index_clusters_per_record;
	vol->index_record_size_bits = ffs( vol->index_record_size ) - 1;
	ntfs_debug( "ntfs_init_volume: vol->index_record_size = " "0x%x\n", vol->index_record_size );
	ntfs_debug( "ntfs_init_volume: vol->index_record_size_bits " "= 0x%x\n", vol->index_record_size_bits );
	/*
	 * Get the size of the volume in clusters (ofs 0x28 is nr_sectors) and
	 * check for 64-bit-ness. Windows currently only uses 32 bits to save
	 * the clusters so we do the same as it is much faster on 32-bit CPUs.
	 */
	ll = NTFS_GETS64( boot + 0x28 ) >> sectors_per_cluster_bits;
	if( ll >= ( int64 )1 << 31 )
	{
		ntfs_error( "Cannot handle 64-bit clusters. Please inform linux-ntfs-dev@lists.sf.net that you got this error.\n" );
		return -1;
	}
	vol->nr_clusters = ( off_t )ll;
	ntfs_debug( "ntfs_init_volume: vol->nr_clusters = 0x%Lx\n", vol->nr_clusters );
	vol->mft_lcn = ( off_t )NTFS_GETS64( boot + 0x30 );
	vol->mft_mirr_lcn = ( off_t )NTFS_GETS64( boot + 0x38 );
	/* Determine MFT zone size. */
	mft_zone_size = vol->nr_clusters;
	switch ( vol->mft_zone_multiplier )
	{			/* % of volume size in clusters */
		case 4:
			mft_zone_size >>= 1;	/* 50%   */
			break;
		case 3:
			mft_zone_size = mft_zone_size * 3 >> 3;	/* 37.5% */
			break;
		case 2:
			mft_zone_size >>= 2;	/* 25%   */
			break;
		/* case 1: */
		default:
			mft_zone_size >>= 3;	/* 12.5% */
			break;
	}
	/* Setup mft zone. */
	vol->mft_zone_start = vol->mft_zone_pos = vol->mft_lcn;
	ntfs_debug( "ntfs_init_volume: vol->mft_zone_pos = %Lx\n", vol->mft_zone_pos );
	/*
	 * Calculate the mft_lcn for an unmodified NTFS volume (see mkntfs
	 * source) and if the actual mft_lcn is in the expected place or even
	 * further to the front of the volume, extend the mft_zone to cover the
	 * beginning of the volume as well. This is in order to protect the
	 * area reserved for the mft bitmap as well within the mft_zone itself.
	 * On non-standard volumes we don't protect it as well as the overhead
	 * would be higher than the speed increase we would get by doing it.
	 */
	tc = ( 8192 + 2 * vol->cluster_size - 1 ) / vol->cluster_size;
	if( tc * vol->cluster_size < 16 * 1024 )
		tc = ( 16 * 1024 + vol->cluster_size - 1 ) / vol->cluster_size;
	if( vol->mft_zone_start <= tc )
		vol->mft_zone_start = ( off_t )0;
	ntfs_debug( "ntfs_init_volume: vol->mft_zone_start = %Lx\n", vol->mft_zone_start );
	/*
	 * Need to cap the mft zone on non-standard volumes so that it does
	 * not point outside the boundaries of the volume, we do this by
	 * halving the zone size until we are inside the volume.
	 */
	vol->mft_zone_end = vol->mft_lcn + mft_zone_size;
	while( vol->mft_zone_end >= vol->nr_clusters )
	{
		mft_zone_size >>= 1;
		vol->mft_zone_end = vol->mft_lcn + mft_zone_size;
	}
	ntfs_debug( "ntfs_init_volume: vol->mft_zone_end = %Lx\n", vol->mft_zone_end );
	/*
	 * Set the current position within each data zone to the start of the
	 * respective zone.
	 */
	vol->data1_zone_pos = vol->mft_zone_end;
	ntfs_debug( "ntfs_init_volume: vol->data1_zone_pos = %Lx\n", vol->data1_zone_pos );
	vol->data2_zone_pos = ( off_t )0;
	ntfs_debug( "ntfs_init_volume: vol->data2_zone_pos = %Lx\n", vol->data2_zone_pos );
	/* Set the mft data allocation position to mft record 24. */
	vol->mft_data_pos = 24UL;
	/* This will be initialized later. */
	vol->upcase = 0;
	vol->upcase_length = 0;
	vol->mft_ino = 0;
	return 0;
}

int ntfs_create_vol( const char *dev, ntfs_volume_s ** ppVol, kdev_t id )
{
	ntfs_volume_s *vol;
	int fd, i, error;

	fd = open( dev, O_RDONLY );
	if( fd < 0 )
		return fd;

	vol = ( ntfs_volume_s * ) kmalloc( sizeof( ntfs_volume_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if( !vol )
	{
		close( fd );
		return -ENOMEM;
	}

	strcpy( vol->volumeLabel, "Unknown" );
	vol->id = id;
	vol->fd = fd;
	vol->umask = 0222;
	vol->ngt = ngt_nt;
	vol->nls_map = NLS_ASCII;
	vol->mft_zone_multiplier = -1;

	int blocksize = GetDeviceBlockSize( vol->fd );

	if( blocksize < 512 )
		blocksize = 512;

	char *bootblock = ( char * )kmalloc( blocksize, MEMF_KERNEL | MEMF_OKTOFAILHACK );

	if( !bootblock )
	{
		kfree( vol );
		close( fd );
		return -ENOMEM;
	}

	if( read_pos( vol->fd, 0, ( void * )bootblock, blocksize ) != blocksize )
	{
		ntfs_error( "ntfs: Unable to read bootblock\n" );
		kfree( bootblock );
		kfree( vol );
		close( fd );
		return -EIO;
	}

	if( !is_boot_sector_ntfs( ( uint8 * )bootblock ) )
	{
		kfree( bootblock );
		kfree( vol );
		close( fd );
		return -EINVAL;
	}

	if( ntfs_init_volume( vol, bootblock ) )
	{
		kfree( bootblock );
		kfree( vol );
		close( fd );
		return -EINVAL;
	}

	kfree( bootblock );

	ntfs_debug( "$Mft at cluster 0x%Lx\n", vol->mft_lcn );
	ntfs_debug( "Done to init volume\n" );

	i = vol->cluster_size;
	if( i < vol->mft_record_size )
		i = vol->mft_record_size;

	if( !( vol->mft = ( uint8 * )kmalloc( i, MEMF_KERNEL | MEMF_OKTOFAILHACK ) ) )
	{
		ntfs_error( "Unable to allocate %d bytes for MFT\n", i );
		close( vol->fd );
		kfree( vol );
		return -ENOMEM;
	}

	int to_read = vol->mft_clusters_per_record;

	if( to_read < 1 )
		to_read = 1;

	if( read_pos( vol->fd, vol->mft_lcn * vol->cluster_size, ( void * )vol->mft, to_read * vol->cluster_size ) != ( to_read * vol->cluster_size ) )
	{
		ntfs_error( "Unable to read MFT\n" );
		error = -EIO;
		goto err1;
	}

	if( !ntfs_check_mft_record( vol, ( char * )vol->mft ) )
	{
		ntfs_error( "Invalid $Mft record 0\n" );
		error = -EINVAL;
		goto err1;
	}

	*ppVol = vol;
	return 0;

err1:
	kfree( vol->mft );
	close( vol->fd );
	kfree( vol );
	return error;
}

static int simple_ntfs_read_fs_stat( void *_vol, fs_info * fsinfo )
{
	ntfs_volume_s *vol = ( ntfs_volume_s * ) _vol;

	fsinfo->fi_dev = vol->id;
	fsinfo->fi_root = FILE_root;
	fsinfo->fi_flags = FS_IS_PERSISTENT | FS_IS_READONLY | FS_IS_BLOCKBASED | FS_CAN_MOUNT;
	fsinfo->fi_block_size = vol->cluster_size;
	fsinfo->fi_io_size = 65536;
	fsinfo->fi_total_blocks = vol->nr_clusters;
	fsinfo->fi_free_blocks = 0;
	fsinfo->fi_free_user_blocks = 0;
	fsinfo->fi_total_inodes = vol->nr_clusters;	// XXXX - FIXME
	fsinfo->fi_free_inodes = 0;

	strncpy( fsinfo->fi_volume_name, "Unknown", 16 );
	strcpy( fsinfo->fi_driver_name, fsname );

	return -EOK;
}

static int ntfs_read_fs_stat( void *_vol, fs_info * fsinfo )
{
	ntfs_volume_s *vol = ( ntfs_volume_s * ) _vol;

	memset( fsinfo, 0, sizeof( fs_info ) );

	fsinfo->fi_dev = vol->id;
	fsinfo->fi_root = FILE_root;
	fsinfo->fi_flags = FS_IS_PERSISTENT | FS_IS_READONLY | FS_IS_BLOCKBASED | FS_CAN_MOUNT;
	fsinfo->fi_block_size = vol->cluster_size;
	fsinfo->fi_io_size = 65536;

	int64 size;

	ntfs_get_volumesize( vol, &size );
	fsinfo->fi_total_blocks = size;

	size = ( int64 )ntfs_get_free_cluster_count( vol->bitmap );
	if( size < 0LL )
		size = 0;

	fsinfo->fi_free_blocks = size;
	fsinfo->fi_free_user_blocks = size;

	ntfs_inode_s *mft;

	if( get_vnode( vol->id, FILE_Mft, ( void ** )&mft ) == 0 )
	{
		fsinfo->fi_total_inodes = mft->i_size >> vol->mft_record_size_bits;
		put_vnode( vol->id, FILE_Mft );
	}
	else
	{
		return -EIO;
	}

	strncpy( fsinfo->fi_volume_name, vol->volumeLabel, 16 );
	strcpy( fsinfo->fi_driver_name, fsname );

	return -EOK;
}

static int ntfs_probe( const char *dev, fs_info * fsinfo )
{
	ntfs_volume_s *vol;

	if( ntfs_create_vol( dev, &vol, 0 ) == 0 )
	{
		simple_ntfs_read_fs_stat( vol, fsinfo );
		close( vol->fd );
		kfree( vol->mft );
		kfree( vol );
		return 0;
	}
	return -EINVAL;
}

int ntfs_fixup_record( char *record, char *magic, int size )
{
	int start, count, offset;
	uint16 fixup;

	if( !IS_MAGIC( record, magic ) )
		return 0;
	start = NTFS_GETU16( record + 4 );
	count = NTFS_GETU16( record + 6 ) - 1;
	if( size & ( NTFS_SECTOR_SIZE - 1 ) || start & 1 || start + count * 2 > size || size >> 9 != count )
	{
		if( size <= 0 )
			ntfs_error( "NTFS: BUG: ntfs_fixup_record() got zero size! Please report this to linux-ntfs-dev@lists.sf.net\n" );
		return 0;
	}
	fixup = NTFS_GETU16( record + start );
	start += 2;
	offset = NTFS_SECTOR_SIZE - 2;
	while( count-- )
	{
		if( NTFS_GETU16( record + offset ) != fixup )
			return 0;
		NTFS_PUTU16( record + offset, NTFS_GETU16( record + start ) );
		start += 2;
		offset += NTFS_SECTOR_SIZE;
	}
	return 1;
}

int ntfs_check_mft_record( ntfs_volume_s * vol, char *record )
{
	return ntfs_fixup_record( record, "FILE", vol->mft_record_size );
}

void ntfs_read_label( ntfs_volume_s * vol )
{
}

static int ntfs_mount( kdev_t nsid, const char *dev, uint32 flags, const void *_params, int len, void **_data, ino_t *vnid )
{
	ntfs_volume_s *vol;
	int error;

	if( ntfs_create_vol( dev, &vol, nsid ) != 0 )
	{
		return -EINVAL;
	}

	vol->lock = create_semaphore( "ntfs_vol_lock", 1, 0 );

	error = setup_device_cache( vol->fd, vol->id, vol->nr_clusters );
	if( error )
		goto err1;

	if( ( error = ntfs_load_special_files( vol ) ) )
	{
		ntfs_error( "Error loading special files\n" );
		goto err2;
	}

	ntfs_read_label( vol );

	*vnid = FILE_root;
	*_data = ( void * )vol;

	return 0;

err2:
	shutdown_device_cache( vol->fd );
err1:
	delete_semaphore( vol->lock );
	close( vol->fd );
	kfree( vol->mft );
	kfree( vol );
	return error;
}

int ntfs_unmount( void *pVolume )
{
	ntfs_volume_s *vol = ( ntfs_volume_s * ) pVolume;

	delete_semaphore( vol->lock );

	ntfs_release_volume( vol );
	shutdown_device_cache( vol->fd );
	close( vol->fd );
	kfree( vol );

	return 0;
}

static void ntfs_putuser( ntfs_io * dest, void *src, size_t len )
{
	memcpy( dest->param, src, len );
	dest->param += len;
}

int ntfs_read( void *pVolume, void *pNode, void *pCookie, off_t nPos, void *pBuf, size_t nLen )
{
	ntfs_volume_s *vol = ( ntfs_volume_s * ) pVolume;
	ntfs_inode_s *ino = ( ntfs_inode_s * ) pNode;
	ntfs_io io;
	ntfs_attribute *attr;

	if( !ino )
		return -EINVAL;

	attr = ntfs_find_attr( ino, vol->at_data, NULL );
	if( !attr )
	{
		ntfs_error( "ntfs_read: $DATA not found !\n" );
		return -EINVAL;
	}

	if( attr->flags & ATTR_IS_ENCRYPTED )
		return -EACCES;

	io.fn_put = ntfs_putuser;
	io.fn_get = 0;
	io.param = pBuf;
	io.size = nLen;
	int error = ntfs_read_attr( ino, vol->at_data, NULL, nPos, &io );

	if( error && !io.size )
	{
		return error;
	}

	return io.size;
}

int ntfs_write_inode( void *_vol, void *_node )
{
	ntfs_volume_s *vol = ( ntfs_volume_s * ) _vol;
	ntfs_inode_s *ino = ( ntfs_inode_s * ) _node;
	ntfs_inode_s *new_ino;

	switch ( ino->i_ino )
	{
		case FILE_Mft:
			if( vol->mft_ino && !( vol->ino_flags & 1 ) )
			{
				new_ino = ( ntfs_inode_s * ) ntfs_malloc( sizeof( ntfs_inode_s ) );
				memcpy( new_ino, ino, sizeof( ntfs_inode_s ) );
				vol->mft_ino = new_ino;
				vol->ino_flags |= 1;
				return 0;
			}

		case FILE_MftMirr:
			if( vol->mftmirr && !( vol->ino_flags & 2 ) )
			{
				new_ino = ( ntfs_inode_s * ) ntfs_malloc( sizeof( ntfs_inode_s ) );
				memcpy( new_ino, ino, sizeof( ntfs_inode_s ) );
				vol->mftmirr = new_ino;
				vol->ino_flags |= 2;
				return 0;
			}

		case FILE_BitMap:
			if( vol->bitmap && !( vol->ino_flags & 4 ) )
			{
				new_ino = ( ntfs_inode_s * ) ntfs_malloc( sizeof( ntfs_inode_s ) );
				memcpy( new_ino, ino, sizeof( ntfs_inode_s ) );
				vol->bitmap = new_ino;
				vol->ino_flags |= 4;
				return 0;
			}
	}

	ntfs_clear_inode( ino );
	kfree( ino );
	return 0;
}

int ntfs_walk( void *_vol, void *_parent, const char *name, int name_len, ino_t *inode )
{
	/*ntfs_volume_s *vol = ( ntfs_volume_s * ) _vol;*/
	ntfs_inode_s *parent = ( ntfs_inode_s * ) _parent;

	char *item = 0;
	ntfs_iterate_s walk;
	int err;

	walk.name = NULL;
	walk.namelen = 0;

	err = ntfs_decodeuni( (char *)name, name_len, &walk.name, &walk.namelen );
	if( err )
		return err;

	item = ntfs_malloc( ITEM_SIZE );
	if( !item )
	{
		kfree( walk.name );
		return -ENOMEM;
	}

	walk.type = BY_NAME;
	walk.dir = parent;
	walk.result = item;
	if( ntfs_getdir_byname( &walk ) )
	{
		err = 0;
		*inode = NTFS_GETU32( item );
	}
	else
	{
		err = -ENOENT;
	}
	kfree( item );
	kfree( walk.name );
	return err;
}

static int ntfs_open( void *_vol, void *_node, int mode, void **_cookie )
{
	/*ntfs_volume_s *vol = ( ntfs_volume_s * ) _vol;*/
	ntfs_inode_s *ino = ( ntfs_inode_s * ) _node;

	if( mode == O_WRONLY || mode == O_RDWR || mode & O_TRUNC || mode & O_CREAT )
		return -EROFS;

	ntfs_debug( "ntfs_open: for node %d\n",(int)ino->i_ino);

	if( ino->i_mode & S_IFDIR )
	{
		kerndbg( KERN_DEBUG_LOW, "ntfs_open: opening dir cookie\n");
		struct ntfs_dir_cookie *cookie = ntfs_calloc( sizeof( struct ntfs_dir_cookie ) );

		cookie->dir = ino;
		cookie->pos = 0;
		*_cookie = cookie;
	}
	else
	{
		ntfs_debug( "ntfs_open: opening regular file\n");
	}

	return -EOK;
}

static int ntfs_close( void *_vol, void *_node, void *_cookie )
{
	/*ntfs_volume_s *vol = ( ntfs_volume_s * ) _vol;*/
	ntfs_inode_s *ino = ( ntfs_inode_s * ) _node;

	ntfs_debug( "ntfs_close: ino %d\n",(int)ino->i_ino);

	if( ino->i_mode & S_IFDIR )
	{
		ntfs_debug( "ntfs_close: freeing dir cookie\n");
		kfree( _cookie );
	}
	return 0;
}

static int ntfs_access( void *_vol, void *_node, int mode )
{
	return 0;
}

static int ntfs_rewind_dir( void *_vol, void *_node, void *_cookie )
{
	struct ntfs_dir_cookie *cookie = ( struct ntfs_dir_cookie * )_cookie;
	ntfs_inode_s *ino = ( ntfs_inode_s * ) _node;

	ntfs_debug( "ntfs_rewind_dir: for node %Ld\n",ino->i_ino);
	cookie->pos = 0;
	return 0;
}

static int ntfs_read_stat( void *_vol, void *_node, struct stat *st )
{
	ntfs_volume_s *vol = ( ntfs_volume_s * ) _vol;
	ntfs_inode_s *ino = ( ntfs_inode_s * ) _node;

	ntfs_debug( "ntfs_read_stat: node %Ld\n",ino->i_ino);

	st->st_dev = vol->id;

	st->st_ino = ino->i_ino;
	st->st_nlink = 1;
	st->st_uid = ino->i_uid;
	st->st_gid = ino->i_gid;
	st->st_blksize = vol->cluster_size;
	st->st_mode = ino->i_mode;
	st->st_size = ino->i_size;
	st->st_ctime = ino->ctime / 1000000;
	st->st_mtime = ino->mtime / 1000000;
	st->st_atime = ino->atime / 1000000;

	ntfs_debug( "uid=%d gid=%d mode=%o size=%d\n", st->st_uid, st->st_gid, st->st_mode, (int)st->st_size);

	return -EOK;
}

static int filldir( struct ntfs_dir_cookie *cookie, const char *name, int namelen, off_t offset, ino_t ino )
{
	struct kernel_dirent *de;

	if( cookie->count )
		return -EINVAL;
	cookie->count++;

	de = cookie->dirent;
	de->d_ino = ino;

	int name_buf_size = ( cookie->entsize - ( ( sizeof( dev_t ) << 1 ) + ( sizeof( ino_t ) << 1 ) + sizeof( unsigned short ) ) );

	if( namelen > name_buf_size )
		namelen = name_buf_size;

	de->d_namlen = namelen;
	strncpy( de->d_name, name, namelen );

	ntfs_debug( "de->d_name='%s' name='%s' namelen %d\n",de->d_name,name,namelen);

	return 0;
}

static int ntfs_printcb( uint8 *entry, void *param )
{
	struct ntfs_dir_cookie *nf = ( struct ntfs_dir_cookie * )param;
	int flags = NTFS_GETU8( entry +0x51 );
	int show_hidden = 0;
	int length = NTFS_GETU8( entry +0x50 );
	int inum = NTFS_GETU32( entry );
	int error;

	switch ( nf->type )
	{
		case ngt_dos:
			if( ( flags & 2 ) == 0 )
				return 0;
			break;

		case ngt_nt:
			switch ( flags & 3 )
			{
				case 2:
					return 0;
			}
			break;

		case ngt_posix:
			break;

		case ngt_full:
			show_hidden = 1;
			break;
	}

	if( !show_hidden && ( ( NTFS_GETU8( entry +0x48 ) & 2 ) == 2 ) )
	{
		ntfs_debug( "ntfs: skipping hidden file\n");
		return 0;
	}

	nf->name = 0;

	if( ntfs_encodeuni( ( uint16 * )( entry +0x52 ), length, &nf->name, &nf->namelen ) )
	{
		ntfs_debug( "Skipping unrepresentable file\n");
		if( nf->name )
			ntfs_free( nf->name );
		return 0;
	}

	if( length == 1 && *nf->name == '.' )
		return 0;

	nf->name[nf->namelen] = 0;


	error = filldir( nf, nf->name, nf->namelen, ( nf->ph << 16 ) | nf->pl, inum );

	ntfs_debug("readdir got %s,len %d, filldir=%d\n",nf->name,nf->namelen,error);

	if( error )
		nf->ret_code = error;

	ntfs_free( nf->name );
	nf->name = 0;
	nf->namelen = 0;

	return error;
}

static int do_ntfs_read_dir( void *_vol, void *_node, void *_cookie, int nPos, struct kernel_dirent *ent, size_t entsize )
{
	int error;
	ntfs_volume_s *vol = ( ntfs_volume_s * ) _vol;
	ntfs_inode_s *ino = ( ntfs_inode_s * ) _node;
	struct ntfs_dir_cookie *cookie = ( struct ntfs_dir_cookie * )_cookie;

	ntfs_debug( "ntfs_read_dir: begin with pos %d\n",nPos);

	cookie->pl = cookie->pos & 0xffff;
	cookie->ph = ( cookie->pos >> 16 );
	cookie->pos = ( off_t )( cookie->ph << 16 ) | cookie->pl;
	cookie->dirent = ent;
	cookie->ret_code = 0;
	cookie->entsize = entsize;

	ntfs_debug( "ntfs_read_dir: for inode %d, pos at ph=%d/pl=%d\n",(int)ino->i_ino, cookie->ph, cookie->pl);

	if( !cookie->ph )
	{
		if( !cookie->pl )
		{
			cookie->ret_code = filldir( cookie, ".", 1, cookie->pos, ino->i_ino );
			if( cookie->ret_code )
				return 0;
			cookie->pl++;
			cookie->pos = ( off_t )( cookie->ph << 16 ) | cookie->pl;
		}
		if( cookie->pl == ( uint32 )1 )
		{
			cookie->ret_code = filldir( cookie, "..", 2, cookie->pos, ino->i_ino );
			if( cookie->ret_code )
				return 0;
			cookie->pl++;
			cookie->pos = ( off_t )( cookie->ph << 16 ) | cookie->pl;
		}
	}
	else if( cookie->ph >= 0x7fff )  /* End of directory. */
		return 0;

	cookie->dir = ino;
	cookie->type = vol->ngt;
	cookie->count = 0;

	do
	{
		error = ntfs_getdir_unsorted( ino, &cookie->ph, &cookie->pl, ntfs_printcb, _cookie );
	}
	while( !error && !cookie->ret_code && cookie->ph < 0x7fff );

	cookie->pos = ( off_t )( cookie->ph << 16 ) | cookie->pl;

	return error;
}

static int ntfs_read_dir( void *_vol, void *_node, void *_cookie, int po, struct kernel_dirent *ent, size_t entsize )
{
	struct ntfs_dir_cookie *cookie = ( struct ntfs_dir_cookie * )_cookie;

	cookie->count = 0;

	ntfs_debug("begin readdir\n");

	int error = do_ntfs_read_dir( _vol, _node, _cookie, po, ent, entsize );

	if( error < 0 )
	{
		ntfs_debug( "ntfs_read_dir: error reading directory\n");
		return error;
	}

	ntfs_debug("end readdir with return %d\n",cookie->count);
	if(cookie->count == 1)
		ntfs_debug("name='%s' namlen='%d' ino='%d'\n", ent->d_name, ent->d_namlen, (int)ent->d_ino);

	return cookie->count;
}

static FSOperations_s ntfs_operations = {
	.probe = ntfs_probe,
	.mount = ntfs_mount,
	.unmount = ntfs_unmount,
	.read_inode = ntfs_read_inode,
	.write_inode = ntfs_write_inode,
	.read = ntfs_read,
	.locate_inode = ntfs_walk,
	.open = ntfs_open,
	.close = ntfs_close,
	.access = ntfs_access,
	.rewinddir = ntfs_rewind_dir,
	.rstat = ntfs_read_stat,
	.readdir = ntfs_read_dir,
	.rfsstat = ntfs_read_fs_stat,
};

int fs_init( const char *name, FSOperations_s ** ops )
{
	*ops = &ntfs_operations;
	return ( api_version );
}
