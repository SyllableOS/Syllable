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

int ntfs_read_mft_record( ntfs_volume_s * vol, int mftno, char *buf )
{
	int error;
	ntfs_io io;

	if( mftno == FILE_Mft )
	{
		ntfs_memcpy( buf, vol->mft, vol->mft_record_size );
		return 0;
	}
	if( !vol->mft_ino )
	{
		ntfs_error( "NTFS: mft_ino is NULL. Something is terribly wrong here!\n" );
		return -ENODATA;
	}
	io.fn_put = ntfs_put;
	io.fn_get = 0;
	io.param = buf;
	io.size = vol->mft_record_size;
	error = ntfs_read_attr( vol->mft_ino, vol->at_data, NULL, ( int64 )mftno << vol->mft_record_size_bits, &io );
	if( error || ( io.size != vol->mft_record_size ) )
	{
		return error ? error : -ENODATA;
	}
	if( !ntfs_check_mft_record( vol, buf ) )
	{
		/* FIXME: This is incomplete behaviour. We might be able to
		 * recover at this stage. ntfs_check_mft_record() is too
		 * conservative at aborting it's operations. It is OK for
		 * now as we just can't handle some on disk structures
		 * this way. (AIA) */
		ntfs_error( "NTFS: Invalid MFT record for 0x%x\n", mftno );
		return -EIO;
	}
	return 0;
}

int ntfs_getput_clusters( ntfs_volume_s * vol, int cluster, size_t start_offs, ntfs_io * buf )
{
	int length = buf->size;
	int error = 0;
	size_t to_copy;
	char *bh;

	to_copy = vol->cluster_size - start_offs;
	while( length )
	{
		if( !( bh = get_cache_block( vol->fd, cluster, vol->cluster_size ) ) )
		{
			error = -EIO;
			goto error_ret;
		}
		if( to_copy > length )
			to_copy = length;

		if( buf->do_read )
		{
			buf->fn_put( buf, bh + start_offs, to_copy );
		}
		else
		{
			buf->fn_get( bh + start_offs, buf, to_copy );
			mark_blocks_dirty( vol->fd, cluster, 1 );
		}
		release_cache_block( vol->fd, cluster );
		length -= to_copy;
		start_offs = 0;
		to_copy = vol->cluster_size;
		cluster++;
	}

error_ret:
	return error;
}

bigtime_t ntfs_now( void )
{
	return ntfs_syllable2ntutc( get_system_time() );
}
