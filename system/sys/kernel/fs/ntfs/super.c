/**
 * inode.c - NTFS kernel inode handling. Part of the Linux-NTFS project.
 *
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

#include <ntfs.h>

static void ntfs_init_upcase( ntfs_inode_s * upcase )
{
	ntfs_io io;

#define UPCASE_LENGTH  256
	upcase->vol->upcase = ntfs_malloc( UPCASE_LENGTH << 1 );
	if( !upcase->vol->upcase )
		return;
	io.fn_put = ntfs_put;
	io.fn_get = 0;
	io.param = ( char * )upcase->vol->upcase;
	io.size = UPCASE_LENGTH << 1;
	ntfs_read_attr( upcase, upcase->vol->at_data, 0, 0, &io );
	upcase->vol->upcase_length = io.size >> 1;
}

static int process_attrdef( ntfs_inode_s * attrdef, uint8 *def )
{
	int type = NTFS_GETU32( def +0x80 );
	int check_type = 0;
	ntfs_volume_s *vol = attrdef->vol;
	uint16 *name = ( uint16 * )def;

	if( !type )
	{
		return 1;
	}
	if( ntfs_ua_strncmp( name, "$STANDARD_INFORMATION", 64 ) == 0 )
	{
		vol->at_standard_information = type;
		check_type = 0x10;
	}
	else if( ntfs_ua_strncmp( name, "$ATTRIBUTE_LIST", 64 ) == 0 )
	{
		vol->at_attribute_list = type;
		check_type = 0x20;
	}
	else if( ntfs_ua_strncmp( name, "$FILE_NAME", 64 ) == 0 )
	{
		vol->at_file_name = type;
		check_type = 0x30;
	}
	else if( ntfs_ua_strncmp( name, "$VOLUME_VERSION", 64 ) == 0 )
	{
		vol->at_volume_version = type;
		check_type = 0x40;
	}
	else if( ntfs_ua_strncmp( name, "$SECURITY_DESCRIPTOR", 64 ) == 0 )
	{
		vol->at_security_descriptor = type;
		check_type = 0x50;
	}
	else if( ntfs_ua_strncmp( name, "$VOLUME_NAME", 64 ) == 0 )
	{
		vol->at_volume_name = type;
		check_type = 0x60;
	}
	else if( ntfs_ua_strncmp( name, "$VOLUME_INFORMATION", 64 ) == 0 )
	{
		vol->at_volume_information = type;
		check_type = 0x70;
	}
	else if( ntfs_ua_strncmp( name, "$DATA", 64 ) == 0 )
	{
		vol->at_data = type;
		check_type = 0x80;
	}
	else if( ntfs_ua_strncmp( name, "$INDEX_ROOT", 64 ) == 0 )
	{
		vol->at_index_root = type;
		check_type = 0x90;
	}
	else if( ntfs_ua_strncmp( name, "$INDEX_ALLOCATION", 64 ) == 0 )
	{
		vol->at_index_allocation = type;
		check_type = 0xA0;
	}
	else if( ntfs_ua_strncmp( name, "$BITMAP", 64 ) == 0 )
	{
		vol->at_bitmap = type;
		check_type = 0xB0;
	}
	else if( ntfs_ua_strncmp( name, "$SYMBOLIC_LINK", 64 ) == 0 || ntfs_ua_strncmp( name, "$REPARSE_POINT", 64 ) == 0 )
	{
		vol->at_symlink = type;
		check_type = 0xC0;
	}
	if( check_type && check_type != type )
	{
		ntfs_error( "process_attrdef: unexpected type 0x%x for 0x%x\n", type, check_type );
		return -EINVAL;
	}
	ntfs_debug( "process_attrdef: found %s attribute of type " "0x%x\n", check_type ? "known" : "unknown", type );
	return 0;
}

int ntfs_init_attrdef( ntfs_inode_s * attrdef )
{
	uint8 *buf;
	ntfs_io io;
	int64 offset;
	unsigned i;
	int error;
	ntfs_attribute *data;

	buf = ntfs_malloc( 4050 );	/* 90*45 */
	if( !buf )
		return -ENOMEM;
	io.fn_put = ntfs_put;
	io.fn_get = ntfs_get;
	io.do_read = 1;
	offset = 0;
	data = ntfs_find_attr( attrdef, attrdef->vol->at_data, 0 );

	if( !data )
	{
		ntfs_free( buf );
		return -EINVAL;
	}
	do
	{
		io.param = buf;
		io.size = 4050;
		error = ntfs_readwrite_attr( attrdef, data, offset, &io );
		for( i = 0; !error && i <= io.size - 0xA0; i += 0xA0 )
		{
			error = process_attrdef( attrdef, buf + i );
		}
		offset += 4096;
	}
	while( !error && io.size );
	ntfs_free( buf );
	return error == 1 ? 0 : error;
}

int ntfs_get_version( ntfs_inode_s * volume )
{
	ntfs_attribute *volinfo;

	volinfo = ntfs_find_attr( volume, volume->vol->at_volume_information, 0 );
	if( !volinfo )
		return -EINVAL;
	if( !volinfo->resident )
	{
		ntfs_error( "Volume information attribute is not resident!\n" );
		return -EINVAL;
	}
	return ( ( uint8 * )volinfo->d.data )[8] << 8 | ( ( uint8 * )volinfo->d.data )[9];
}

int ntfs_load_special_files( ntfs_volume_s * vol )
{
	int error;
	ntfs_inode_s upcase, attrdef, volume;

	memset( &upcase, 0, sizeof( ntfs_inode_s ) );
	memset( &attrdef, 0, sizeof( ntfs_inode_s ) );
	memset( &volume, 0, sizeof( ntfs_inode_s ) );

	vol->mft_ino = ( ntfs_inode_s * ) ntfs_calloc( sizeof( ntfs_inode_s ) );
	vol->mftmirr = ( ntfs_inode_s * ) ntfs_calloc( sizeof( ntfs_inode_s ) );
	vol->bitmap = ( ntfs_inode_s * ) ntfs_calloc( sizeof( ntfs_inode_s ) );
	vol->ino_flags = 4 | 2 | 1;
	error = -ENOMEM;
	ntfs_debug( "Going to load MFT\n" );
	if( !vol->mft_ino || ( error = ntfs_init_inode( vol->mft_ino, vol, FILE_Mft ) ) )
	{
		ntfs_error( "Problem loading MFT\n" );
		return error;
	}
	ntfs_debug( "Going to load MIRR\n" );
	if( ( error = ntfs_init_inode( vol->mftmirr, vol, FILE_MftMirr ) ) )
	{
		ntfs_error( "Problem %d loading MFTMirr\n", error );
		return error;
	}
	ntfs_debug( "Going to load BITMAP\n" );
	if( ( error = ntfs_init_inode( vol->bitmap, vol, FILE_BitMap ) ) )
	{
		ntfs_error( "Problem loading Bitmap\n" );
		return error;
	}
	ntfs_debug( "Going to load UPCASE\n" );
	error = ntfs_init_inode( &upcase, vol, FILE_UpCase );
	if( error )
		return error;
	ntfs_init_upcase( &upcase );
	ntfs_clear_inode( &upcase );
	ntfs_debug( "Going to load ATTRDEF\n" );
	error = ntfs_init_inode( &attrdef, vol, FILE_AttrDef );
	if( error )
		return error;
	error = ntfs_init_attrdef( &attrdef );
	ntfs_clear_inode( &attrdef );
	if( error )
		return error;

	/* Check for NTFS version and if Win2k version (ie. 3.0+) do not allow
	 * write access since the driver write support is broken. */
	ntfs_debug( "Going to load VOLUME\n" );
	error = ntfs_init_inode( &volume, vol, FILE_Volume );
	if( error )
		return error;
	error = ntfs_get_version( &volume );
	ntfs_clear_inode( &volume );
	if( error < 0 )
		return error;
	ntfs_debug( "NTFS volume is v%d.%d\n", error >> 8, error & 0xff );
	return 0;
}

int ntfs_release_volume( ntfs_volume_s * vol )
{
	if( ( ( vol->ino_flags & 1 ) == 1 ) && vol->mft_ino )
	{
		ntfs_clear_inode( vol->mft_ino );
		ntfs_free( vol->mft_ino );
		vol->mft_ino = 0;
	}
	if( ( ( vol->ino_flags & 2 ) == 2 ) && vol->mftmirr )
	{
		ntfs_clear_inode( vol->mftmirr );
		ntfs_free( vol->mftmirr );
		vol->mftmirr = 0;
	}
	if( ( ( vol->ino_flags & 4 ) == 4 ) && vol->bitmap )
	{
		ntfs_clear_inode( vol->bitmap );
		ntfs_free( vol->bitmap );
		vol->bitmap = 0;
	}
	ntfs_free( vol->mft );
	ntfs_free( vol->upcase );
	return 0;
}

int ntfs_get_volumesize( ntfs_volume_s * vol, int64 *vol_size )
{
	ntfs_io io;
	char *cluster0;

	if( !vol_size )
		return -EFAULT;
	cluster0 = ntfs_malloc( vol->cluster_size );
	if( !cluster0 )
		return -ENOMEM;
	io.fn_put = ntfs_put;
	io.fn_get = ntfs_get;
	io.param = cluster0;
	io.do_read = 1;
	io.size = vol->cluster_size;
	ntfs_getput_clusters( vol, 0, 0, &io );
	*vol_size = NTFS_GETU64( cluster0 + 0x28 ) >> ( ffs( NTFS_GETU8( cluster0 + 0xD ) ) - 1 );
	ntfs_free( cluster0 );
	return 0;
}

static int nc[16] = { 4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0 };

int ntfs_get_free_cluster_count( ntfs_inode_s * bitmap )
{
	ntfs_io io;
	int offset, error, clusters;
	unsigned char *bits = ntfs_malloc( 2048 );

	if( !bits )
		return -ENOMEM;
	offset = clusters = 0;
	io.fn_put = ntfs_put;
	io.fn_get = ntfs_get;
	while( 1 )
	{
		register int i;

		io.param = bits;
		io.size = 2048;
		error = ntfs_read_attr( bitmap, bitmap->vol->at_data, 0, offset, &io );
		if( error || io.size == 0 )
			break;
		/* I never thought I would do loop unrolling some day */
		for( i = 0; i < io.size - 8; )
		{
			clusters += nc[bits[i] >> 4];
			clusters += nc[bits[i++] & 0xF];
			clusters += nc[bits[i] >> 4];
			clusters += nc[bits[i++] & 0xF];
			clusters += nc[bits[i] >> 4];
			clusters += nc[bits[i++] & 0xF];
			clusters += nc[bits[i] >> 4];
			clusters += nc[bits[i++] & 0xF];
			clusters += nc[bits[i] >> 4];
			clusters += nc[bits[i++] & 0xF];
			clusters += nc[bits[i] >> 4];
			clusters += nc[bits[i++] & 0xF];
			clusters += nc[bits[i] >> 4];
			clusters += nc[bits[i++] & 0xF];
			clusters += nc[bits[i] >> 4];
			clusters += nc[bits[i++] & 0xF];
		}
		while( i < io.size )
		{
			clusters += nc[bits[i] >> 4];
			clusters += nc[bits[i++] & 0xF];
		}
		offset += io.size;
	}
	ntfs_free( bits );
	return clusters;
}

/*
 * Insert the fixups for the record. The number and location of the fixes
 * is obtained from the record header but we double check with @rec_size and
 * use that as the upper boundary, if necessary overwriting the count value in
 * the record header.
 *
 * We return 0 on success or -1 if fixup header indicated the beginning of the
 * update sequence array to be beyond the valid limit.
 */
int ntfs_insert_fixups( unsigned char *rec, int rec_size )
{
	int first;
	int count;
	int offset = -2;
	uint16 fix;

	first = NTFS_GETU16( rec + 4 );
	count = ( rec_size >> NTFS_SECTOR_BITS ) + 1;
	if( first + count * 2 > NTFS_SECTOR_SIZE - 2 )
	{
		ntfs_error( "NTFS: ntfs_insert_fixups() detected corrupt NTFS record update sequence array position. - Cannot hotfix.\n" );
		return -1;
	}
	if( count != NTFS_GETU16( rec + 6 ) )
	{
		ntfs_error( "NTFS: ntfs_insert_fixups() detected corrupt NTFS record update sequence array size. - Applying hotfix.\n" );
		NTFS_PUTU16( rec + 6, count );
	}
	fix = ( NTFS_GETU16( rec + first ) + 1 ) & 0xffff;
	if( fix == 0xffff || !fix )
		fix = 1;
	NTFS_PUTU16( rec + first, fix );
	count--;
	while( count-- )
	{
		first += 2;
		offset += NTFS_SECTOR_SIZE;
		NTFS_PUTU16( rec + first, NTFS_GETU16( rec + offset ) );
		NTFS_PUTU16( rec + offset, fix );
	}
	return 0;
}

int ntfs_allocate_clusters( ntfs_volume_s * vol, off_t *location, off_t *count, ntfs_runlist ** rl, int *rl_len, const NTFS_CLUSTER_ALLOCATION_ZONES zone )
{
	return -EIO;
}

int ntfs_deallocate_cluster_run( const ntfs_volume_s * vol, const off_t lcn, const off_t len )
{
	return -EIO;
}

int ntfs_deallocate_clusters( const ntfs_volume_s * vol, const ntfs_runlist * rl, const int rl_len )
{
	return -EIO;
}
