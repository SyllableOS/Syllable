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

typedef struct
{
	int recno;
	unsigned char *record;
} ntfs_mft_record;

typedef struct
{
	int size;
	int count;
	ntfs_mft_record *records;
} ntfs_disk_inode;

int splice_runlists( ntfs_runlist ** rl1, int *r1len, const ntfs_runlist * rl2, int r2len );

#if 0
static void ntfs_fill_mft_header( uint8 *mft, int rec_size, int seq_no, int links, int flags )
{
	int fixup_ofs = 0x2a;
	int fixup_cnt = rec_size / NTFS_SECTOR_SIZE + 1;
	int attr_ofs = ( fixup_ofs + 2 * fixup_cnt + 7 ) & ~7;

	NTFS_PUTU32( mft + 0x00, 0x454c4946 );	/* FILE */
	NTFS_PUTU16( mft + 0x04, fixup_ofs );	/* Offset to fixup. */
	NTFS_PUTU16( mft + 0x06, fixup_cnt );	/* Number of fixups. */
	NTFS_PUTU64( mft + 0x08, 0 );	/* Logical sequence number. */
	NTFS_PUTU16( mft + 0x10, seq_no );	/* Sequence number. */
	NTFS_PUTU16( mft + 0x12, links );	/* Hard link count. */
	NTFS_PUTU16( mft + 0x14, attr_ofs );	/* Offset to attributes. */
	NTFS_PUTU16( mft + 0x16, flags );	/* Flags: 1 = In use,
						   2 = Directory. */
	NTFS_PUTU32( mft + 0x18, attr_ofs + 8 );	/* Bytes in use. */
	NTFS_PUTU32( mft + 0x1c, rec_size );	/* Total allocated size. */
	NTFS_PUTU64( mft + 0x20, 0 );	/* Base mft record. */
	NTFS_PUTU16( mft + 0x28, 0 );	/* Next attr instance. */
	NTFS_PUTU16( mft + fixup_ofs, 1 );	/* Fixup word. */
	NTFS_PUTU32( mft + attr_ofs, ( uint32 )-1 );	/* End of attributes marker. */
}
#endif

ntfs_attribute *ntfs_find_attr( ntfs_inode_s * ino, int type, char *name )
{
	int i;

	if( !ino )
	{
		ntfs_error( "ntfs_find_attr: NO INODE!\n" );
		return 0;
	}

	for( i = 0; i < ino->attr_count; i++ )
	{
		if( type < ino->attrs[i].type )
			return 0;
		if( type == ino->attrs[i].type )
		{
			if( !name )
			{
				if( !ino->attrs[i].name )
					return ino->attrs + i;
			}
			else if( ino->attrs[i].name && !ntfs_ua_strncmp( ino->attrs[i].name, name, strlen( name ) ) )
				return ino->attrs + i;
		}
	}
	return 0;
}

static int ntfs_insert_mft_attributes( ntfs_inode_s * ino, char *mft, int mftno )
{
	int i, error, type, len, present = 0;
	char *it;

	/* Check for duplicate extension record. */
	for( i = 0; i < ino->record_count; i++ )
		if( ino->records[i] == mftno )
		{
			if( i )
				return 0;
			present = 1;
			break;
		}
	if( !present )
	{
		/* (re-)allocate space if necessary. */
		if( ino->record_count % 8 == 0 )
		{
			int *new;

			new = kmalloc( ( ino->record_count + 8 ) * sizeof( int ), MEMF_KERNEL | MEMF_OKTOFAILHACK );
			if( !new )
				return -ENOMEM;
			if( ino->records )
			{
				for( i = 0; i < ino->record_count; i++ )
					new[i] = ino->records[i];
				kfree( ino->records );
			}
			ino->records = new;
		}
		ino->records[ino->record_count] = mftno;
		ino->record_count++;
	}
	it = mft + NTFS_GETU16( mft + 0x14 );	/* mft->attrs_offset */
	do
	{
		type = NTFS_GETU32( it );
		len = NTFS_GETU32( it + 4 );
		if( type != -1 )
		{
			error = ntfs_insert_attribute( ino, it );
			if( error )
				return error;
		}
		/* If we have just processed the attribute list and this is
		 * the first time we are parsing this (base) mft record then we
		 * are done so that the attribute list gets parsed before the
		 * entries in the base mft record. Otherwise we run into
		 * problems with encountering attributes out of order and when
		 * this happens with different attribute extents we die. )-:
		 * This way we are ok as the attribute list is always sorted
		 * fully and correctly. (-: */
		if( type == 0x20 && !present )
			return 0;
		it += len;
	}
	while( type != -1 );	/* Attribute listing ends with type -1. */
	return 0;
}

static int ntfs_insert_mft_attribute( ntfs_inode_s * ino, int mftno, uint8 *attr )
{
	int i, error, present = 0;

	/* Check for duplicate extension record. */
	for( i = 0; i < ino->record_count; i++ )
		if( ino->records[i] == mftno )
		{
			present = 1;
			break;
		}
	if( !present )
	{
		/* (re-)allocate space if necessary. */
		if( ino->record_count % 8 == 0 )
		{
			int *new;

			new = ntfs_malloc( ( ino->record_count + 8 ) * sizeof( int ) );
			if( !new )
				return -ENOMEM;
			if( ino->records )
			{
				for( i = 0; i < ino->record_count; i++ )
					new[i] = ino->records[i];
				ntfs_free( ino->records );
			}
			ino->records = new;
		}
		ino->records[ino->record_count] = mftno;
		ino->record_count++;
	}
	if( NTFS_GETU32( attr ) == -1 )
	{
		ntfs_error( "ntfs_insert_mft_attribute: attribute type is -1.\n" );
		return 0;
	}
	error = ntfs_insert_attribute( ino, attr );
	if( error )
		return error;
	return 0;
}

/* Read and insert all the attributes of an 'attribute list' attribute.
 * Return the number of remaining bytes in *plen. */
static int parse_attributes( ntfs_inode_s * ino, uint8 *alist, int *plen )
{
	uint8 *mft, *attr;
	int mftno, l, error;
	int last_mft = -1;
	int len = *plen;
	int tries = 0;

	if( !ino->attr )
	{
		ntfs_error( "parse_attributes: called on inode %Ld without a loaded base mft record.\n", ( int64 )ino->i_ino );
		return -EINVAL;
	}
	mft = ntfs_malloc( ino->vol->mft_record_size );
	if( !mft )
		return -ENOMEM;
	while( len > 8 )
	{
		l = NTFS_GETU16( alist + 4 );
		if( l > len )
			break;
		/* Process an attribute description. */
		mftno = NTFS_GETU32( alist + 0x10 );
		/* FIXME: The mft reference (alist + 0x10) is __s64.
		 * - Not a problem unless we encounter a huge partition.
		 * - Should be consistency checking the sequence numbers
		 *   though! This should maybe happen in 
		 *   ntfs_read_mft_record() itself and a hotfix could
		 *   then occur there or the user notified to run
		 *   ntfsck. (AIA) */
		if( mftno != ino->i_ino && mftno != last_mft )
		{
		      continue_after_loading_mft_data:
			last_mft = mftno;
			error = ntfs_read_mft_record( ino->vol, mftno, mft );
			if( error )
			{
				if( error == -EINVAL && !tries )
					goto force_load_mft_data;
			      failed_reading_mft_data:
				ntfs_error( "parse_attributes: " "ntfs_read_mft_record(mftno = 0x%x) " "failed\n", mftno );
				ntfs_free( mft );
				return error;
			}
		}
		attr = ntfs_find_attr_in_mft_rec( ino->vol,	/* ntfs volume */
			mftno == ino->i_ino ?	/* mft record is: */
			ino->attr :	/*   base record */
			mft,	/*   extension record */
			NTFS_GETU32( alist + 0 ),	/* type */
			( wchar_t * )( alist + alist[7] ),	/* name */
			alist[6],	/* name length */
			1,	/* ignore case */
			NTFS_GETU16( alist + 24 )	/* instance number */
			 );
		if( !attr )
		{
			ntfs_error( "parse_attributes: mft records %Ld and/or 0x%x corrupt!\n", ( int64 )ino->i_ino, mftno );
			ntfs_free( mft );
			return -EINVAL;	/* FIXME: Better error code? (AIA) */
		}
		error = ntfs_insert_mft_attribute( ino, mftno, attr );
		if( error )
		{
			ntfs_error( "parse_attributes: ntfs_insert_mft_attribute(mftno 0x%x, " "attribute type 0x%x) failed\n", mftno, NTFS_GETU32( alist + 0 ) );
			ntfs_free( mft );
			return error;
		}
		len -= l;
		alist += l;
	}
	ntfs_free( mft );
	*plen = len;
	return 0;

force_load_mft_data:
	{
		uint8 *mft2, *attr2;
		int mftno2;
		int last_mft2 = last_mft;
		int len2 = len;
		int error2;
		int found2 = 0;
		uint8 *alist2 = alist;

		/*
		 * We only get here if $DATA wasn't found in $MFT which only happens
		 * on volume mount when $MFT has an attribute list and there are
		 * attributes before $DATA which are inside extent mft records. So
		 * we just skip forward to the $DATA attribute and read that. Then we
		 * restart which is safe as an attribute will not be inserted twice.
		 *
		 * This still will not fix the case where the attribute list is non-
		 * resident, larger than 1024 bytes, and the $DATA attribute list entry
		 * is not in the first 1024 bytes. FIXME: This should be implemented
		 * somehow! Perhaps by passing special error code up to
		 * ntfs_load_attributes() so it keeps going trying to get to $DATA
		 * regardless. Then it would have to restart just like we do here.
		 */
		mft2 = ntfs_malloc( ino->vol->mft_record_size );
		if( !mft2 )
		{
			ntfs_free( mft );
			return -ENOMEM;
		}
		ntfs_memcpy( mft2, mft, ino->vol->mft_record_size );
		while( len2 > 8 )
		{
			l = NTFS_GETU16( alist2 + 4 );
			if( l > len2 )
				break;
			if( NTFS_GETU32( alist2 + 0x0 ) < ino->vol->at_data )
			{
				len2 -= l;
				alist2 += l;
				continue;
			}
			if( NTFS_GETU32( alist2 + 0x0 ) > ino->vol->at_data )
			{
				if( found2 )
					break;
				/* Uh-oh! It really isn't there! */
				ntfs_error( "Either the $MFT is corrupt or, equally likely, the $MFT is too complex for the current driver to handle. Please email the ntfs maintainer that you saw this message. Thank you.\n" );
				goto failed_reading_mft_data;
			}
			/* Process attribute description. */
			mftno2 = NTFS_GETU32( alist2 + 0x10 );
			if( mftno2 != ino->i_ino && mftno2 != last_mft2 )
			{
				last_mft2 = mftno2;
				error2 = ntfs_read_mft_record( ino->vol, mftno2, mft2 );
				if( error2 )
				{
					ntfs_error( "parse_attributes: ntfs_read_mft_record(mftno2 = 0x%x) failed\n", mftno2 );
					ntfs_free( mft2 );
					goto failed_reading_mft_data;
				}
			}
			attr2 = ntfs_find_attr_in_mft_rec( ino->vol,	/* ntfs volume */
				mftno2 == ino->i_ino ?	/* mft record is: */
				ino->attr :	/*  base record */
				mft2,	/*  extension record */
				NTFS_GETU32( alist2 + 0 ),	/* type */
				( wchar_t * )( alist2 + alist2[7] ),	/* name */
				alist2[6],	/* name length */
				1,	/* ignore case */
				NTFS_GETU16( alist2 + 24 )	/* instance number */
				 );
			if( !attr2 )
			{
				ntfs_error( "parse_attributes: mft records %Ld and/or0x%x corrupt!\n", ( int64 )ino->i_ino, mftno2 );
				ntfs_free( mft2 );
				goto failed_reading_mft_data;
			}
			error2 = ntfs_insert_mft_attribute( ino, mftno2, attr2 );
			if( error2 )
			{
				ntfs_error( "parse_attributes: ntfs_insert_mft_attribute(mftno2 0x%x, attribute2 type 0x%x) failed\n", mftno2, NTFS_GETU32( alist2 + 0 ) );
				ntfs_free( mft2 );
				goto failed_reading_mft_data;
			}
			len2 -= l;
			alist2 += l;
			found2 = 1;
		}
		ntfs_free( mft2 );
		tries = 1;
		goto continue_after_loading_mft_data;
	}
}

static void ntfs_load_attributes( ntfs_inode_s * ino )
{
	ntfs_attribute *alist;
	int datasize;
	int offset, len, delta;
	char *buf;
	ntfs_volume_s *vol = ino->vol;

	if( ntfs_insert_mft_attributes( ino, ino->attr, ino->i_ino ) )
		return;
	alist = ntfs_find_attr( ino, vol->at_attribute_list, 0 );
	if( !alist )
		return;
	datasize = alist->size;
	if( alist->resident )
	{
		parse_attributes( ino, alist->d.data, &datasize );
		return;
	}
	buf = ntfs_malloc( 1024 );
	if( !buf )		/* FIXME: Should be passing error code to caller. (AIA) */
		return;
	delta = 0;
	for( offset = 0; datasize; datasize -= len, offset += len )
	{
		ntfs_io io;

		io.fn_put = ntfs_put;
		io.fn_get = 0;
		io.param = buf + delta;
		len = 1024 - delta;
		if( len > datasize )
			len = datasize;
		io.size = len;
		if( ntfs_read_attr( ino, vol->at_attribute_list, 0, offset, &io ) )
			ntfs_error( "error in load_attributes\n" );
		delta += len;
		parse_attributes( ino, buf, &delta );
		if( delta )
			/* Move remaining bytes to buffer start. */
			ntfs_memmove( buf, buf + len - delta, delta );
	}
	ntfs_free( buf );
}

int ntfs_init_inode( ntfs_inode_s * ino, ntfs_volume_s * vol, ino_t innum )
{
	char *buf;
	int error;

	ino->i_ino = innum;
	ino->vol = vol;
	buf = kmalloc( vol->mft_record_size, MEMF_KERNEL | MEMF_OKTOFAILHACK );
	ino->attr = ( uint8 * )buf;
	if( !buf )
		return -ENOMEM;
	error = ntfs_read_mft_record( vol, innum, ino->attr );
	if( error )
	{
		ntfs_error( "unable to read MFT record\n" );
		return error;
	}
	ino->sequence_number = NTFS_GETU16( buf + 0x10 );
	ntfs_debug( "loading inode attributes\n" );
	ntfs_load_attributes( ino );
	ntfs_debug( "inode loaded from disk\n" );
	return 0;
}

int ntfs_allocate_attr_number( ntfs_inode_s * ino, int *result )
{
	if( ino->record_count != 1 )
		return -EOPNOTSUPP;
	*result = NTFS_GETU16( ino->attr + 0x28 );
	NTFS_PUTU16( ino->attr + 0x28, ( *result ) + 1 );
	return 0;
}

char *ntfs_get_attr( ntfs_inode_s * ino, int attr, char *name )
{
	/* Location of first attribute. */
	char *it = ( char * )( ino->attr + NTFS_GETU16( ino->attr + 0x14 ) );
	int type;
	int len;

	/* Only check for magic DWORD here, fixup should have happened before. */
	if( !IS_MFT_RECORD( ino->attr ) )
		return 0;
	do
	{
		type = NTFS_GETU32( it );
		len = NTFS_GETU16( it + 4 );
		/* We found the attribute type. Is the name correct, too? */
		if( type == attr )
		{
			int namelen = NTFS_GETU8( it + 9 );
			char *name_it, *n = name;

			/* Match given name and attribute name if present.
			   Make sure attribute name is Unicode. */
			if( !name )
			{
				goto check_namelen;
			}
			else if( namelen )
			{
				for( name_it = it + NTFS_GETU16( it + 10 ); namelen; n++, name_it += 2, namelen-- )
					if( *name_it != *n || name_it[1] )
						break;
			      check_namelen:
				if( !namelen )
					break;
			}
		}
		it += len;
	}
	while( type != -1 );	/* List of attributes ends with type -1. */
	if( type == -1 )
		return 0;
	return it;
}

int64 ntfs_get_attr_size( ntfs_inode_s * ino, int type, char *name )
{
	ntfs_attribute *attr = ntfs_find_attr( ino, type, name );

	if( !attr )
		return 0;
	return attr->size;
}

int ntfs_attr_is_resident( ntfs_inode_s * ino, int type, char *name )
{
	ntfs_attribute *attr = ntfs_find_attr( ino, type, name );

	if( !attr )
		return 0;
	return attr->resident;
}

int ntfs_decompress_run( unsigned char **data, int *length, off_t *cluster, int *ctype )
{
	unsigned char type = *( *data )++;

	*ctype = 0;
	switch ( type & 0xF )
	{
		case 1:
			*length = NTFS_GETS8( *data );
			break;
		case 2:
			*length = NTFS_GETS16( *data );
			break;
		case 3:
			*length = NTFS_GETS24( *data );
			break;
		case 4:
			*length = NTFS_GETS32( *data );
			break;
		/* Note: cases 5-8 are probably pointless to code, since how
		 * many runs > 4GB of length are there? At the most, cases 5
		 * and 6 are probably necessary, and would also require making
		 * length 64-bit throughout. */
		default:
			ntfs_error( "Can't decode run type field 0x%x\n", type );
			return -1;
	}
	ntfs_debug("ntfs_decompress_run: length = 0x%x\n",*length);

	if( *length < 0 )
	{
		ntfs_error( "Negative run length decoded\n" );
		return -1;
	}
	*data += ( type & 0xF );
	switch ( type & 0xF0 )
	{
		case 0:
			*ctype = 2;
			break;
		case 0x10:
			*cluster += NTFS_GETS8( *data );
			break;
		case 0x20:
			*cluster += NTFS_GETS16( *data );
			break;
		case 0x30:
			*cluster += NTFS_GETS24( *data );
			break;
		case 0x40:
			*cluster += NTFS_GETS32( *data );
			break;
#if 0				/* Keep for future, in case ntfs_cluster_t ever becomes 64bit. */
		case 0x50:
			*cluster += NTFS_GETS40( *data );
			break;
		case 0x60:
			*cluster += NTFS_GETS48( *data );
			break;
		case 0x70:
			*cluster += NTFS_GETS56( *data );
			break;
		case 0x80:
			*cluster += NTFS_GETS64( *data );
			break;
#endif
		default:
			ntfs_error( "Can't decode run type field 0x%x\n", type );
			return -1;
	}

	*data += ( type >> 4 );
	return 0;
}

int ntfs_readwrite_attr( ntfs_inode_s * ino, ntfs_attribute * attr, int64 offset, ntfs_io * dest )
{
	int rnum, s_vcn, error, clustersizebits;
	off_t cluster, s_cluster, vcn, len;
	int64 l, chunk, copied;

	l = dest->size;
	if( l == 0 )
		return 0;
	if( dest->do_read )
	{
		/* If read _starts_ beyond end of stream, return nothing. */
		if( offset >= attr->size )
		{
			dest->size = 0;
			return 0;
		}
		/* If read _extends_ beyond end of stream, return as much
		 * initialised data as we have. */
		if( offset + l >= attr->size )
			l = dest->size = attr->size - offset;
	}
	else
	{
		/*
		 * If write extends beyond _allocated_ size, extend attribute,
		 * updating attr->allocated and attr->size in the process. (AIA)
		 */
		if( ( !attr->resident && offset + l > attr->allocated ) || ( attr->resident && offset + l > attr->size ) )
		{
			error = ntfs_resize_attr( ino, attr, offset + l );
			if( error )
				return error;
		}
		if( !attr->resident )
		{
			/* Has amount of data increased? */
			if( offset + l > attr->size )
				attr->size = offset + l;
			/* Has amount of initialised data increased? */
			if( offset + l > attr->initialized )
			{
				/* FIXME: Clear the section between the old
				 * initialised length and the write start.
				 * (AIA) */
				attr->initialized = offset + l;
			}
		}
	}
	if( attr->resident )
	{
		if( dest->do_read )
			dest->fn_put( dest, ( uint8 * )attr->d.data + offset, l );
		else
			dest->fn_get( ( uint8 * )attr->d.data + offset, dest, l );
		dest->size = l;
		return 0;
	}
	if( dest->do_read )
	{
		/* Read uninitialized data. */
		if( offset >= attr->initialized )
			return ntfs_read_zero( dest, l );
		if( offset + l > attr->initialized )
		{
			dest->size = chunk = attr->initialized - offset;
			error = ntfs_readwrite_attr( ino, attr, offset, dest );
			if( error || ( dest->size != chunk && ( error = -EIO, 1 ) ) )
				return error;
			dest->size += l - chunk;
			return ntfs_read_zero( dest, l - chunk );
		}
		if( attr->flags & ATTR_IS_COMPRESSED )
			return ntfs_read_compressed( ino, attr, offset, dest );
	}
	else
	{
		if( attr->flags & ATTR_IS_COMPRESSED )
			return ntfs_write_compressed( ino, attr, offset, dest );
	}
	vcn = 0;
	clustersizebits = ino->vol->cluster_size_bits;
	s_vcn = offset >> clustersizebits;
	for( rnum = 0; rnum < attr->d.r.len && vcn + attr->d.r.runlist[rnum].len <= s_vcn; rnum++ )
		vcn += attr->d.r.runlist[rnum].len;
	if( rnum == attr->d.r.len )
	{
		ntfs_error( "%s(): EOPNOTSUPP: inode = %Ld, rnum = %i, offset = 0x%Lx, vcn = 0x%Lx, s_vcn = 0x%x.\n", __FUNCTION__, ( int64 )ino->i_ino, rnum, offset, vcn, s_vcn );
		return -EOPNOTSUPP;
	}
	copied = 0;
	while( l )
	{
		s_vcn = offset >> clustersizebits;
		cluster = attr->d.r.runlist[rnum].lcn;
		len = attr->d.r.runlist[rnum].len;
		s_cluster = cluster + s_vcn - vcn;
		chunk = ( ( int64 )( vcn + len ) << clustersizebits ) - offset;
		if( chunk > l )
			chunk = l;
		dest->size = chunk;
		error = ntfs_getput_clusters( ino->vol, s_cluster, offset - ( ( int64 )s_vcn << clustersizebits ), dest );
		if( error )
		{
			ntfs_error( "Read/write error.\n" );
			dest->size = copied;
			return error;
		}
		l -= chunk;
		copied += chunk;
		offset += chunk;
		if( l && offset >= ( ( int64 )( vcn + len ) << clustersizebits ) )
		{
			rnum++;
			vcn += len;
			cluster = attr->d.r.runlist[rnum].lcn;
			len = attr->d.r.runlist[rnum].len;
		}
	}
	dest->size = copied;
	return 0;
}

int ntfs_read_attr( ntfs_inode_s * ino, int type, char *name, int64 offset, ntfs_io * buf )
{
	ntfs_attribute *attr;

	buf->do_read = 1;
	attr = ntfs_find_attr( ino, type, name );
	if( !attr )
	{
		ntfs_error( "%s(): attr 0x%x not found in inode 0x%Lx\n", __FUNCTION__, type, ino->i_ino );
		return -EINVAL;
	}
	return ntfs_readwrite_attr( ino, attr, offset, buf );
}

int ntfs_write_attr( ntfs_inode_s * ino, int type, char *name, int64 offset, ntfs_io * buf )
{
	ntfs_attribute *attr;

	buf->do_read = 0;
	attr = ntfs_find_attr( ino, type, name );
	if( !attr )
	{
		ntfs_error( "%s(): attr 0x%x not found in inode 0x%Lx\n", __FUNCTION__, type, ino->i_ino );
		return -EINVAL;
	}
	return ntfs_readwrite_attr( ino, attr, offset, buf );
}

/* -2 = error, -1 = hole, >= 0 means real disk cluster (lcn). */
int ntfs_vcn_to_lcn( ntfs_inode_s * ino, int vcn )
{
	int rnum;
	ntfs_attribute *data;

	data = ntfs_find_attr( ino, ino->vol->at_data, 0 );
	if( !data || data->resident || data->flags & ( ATTR_IS_COMPRESSED | ATTR_IS_ENCRYPTED ) )
		return -2;
	if( data->size <= ( int64 )vcn << ino->vol->cluster_size_bits )
		return -2;
	if( data->initialized <= ( int64 )vcn << ino->vol->cluster_size_bits )
		return -1;
	for( rnum = 0; rnum < data->d.r.len && vcn >= data->d.r.runlist[rnum].len; rnum++ )
		vcn -= data->d.r.runlist[rnum].len;
	if( data->d.r.runlist[rnum].lcn >= 0 )
		return data->d.r.runlist[rnum].lcn + vcn;
	return data->d.r.runlist[rnum].lcn + vcn;
}

static int allocate_store( ntfs_volume_s * vol, ntfs_disk_inode * store, int count )
{
	int i;

	if( store->count > count )
		return 0;
	if( store->size < count )
	{
		ntfs_mft_record *n = ntfs_malloc( ( count + 4 ) * sizeof( ntfs_mft_record ) );

		if( !n )
			return -ENOMEM;
		if( store->size )
		{
			for( i = 0; i < store->size; i++ )
				n[i] = store->records[i];
			ntfs_free( store->records );
		}
		store->size = count + 4;
		store->records = n;
	}
	for( i = store->count; i < count; i++ )
	{
		store->records[i].record = ntfs_malloc( vol->mft_record_size );
		if( !store->records[i].record )
			return -ENOMEM;
		store->count++;
	}
	return 0;
}

static void deallocate_store( ntfs_disk_inode * store )
{
	int i;

	for( i = 0; i < store->count; i++ )
		ntfs_free( store->records[i].record );
	ntfs_free( store->records );
	store->count = store->size = 0;
	store->records = 0;
}

static int layout_runs( ntfs_attribute * attr, char *rec, int *offs, int size )
{
	int i, len, offset, coffs;

	/* ntfs_cluster_t MUST be signed! (AIA) */
	off_t cluster, rclus;
	ntfs_runlist *rl = attr->d.r.runlist;

	cluster = 0;
	offset = *offs;
	for( i = 0; i < attr->d.r.len; i++ )
	{
		/*
		 * We cheat with this check on the basis that lcn will never
		 * be less than -1 and the lcn delta will fit in signed
		 * 32-bits (ntfs_cluster_t). (AIA)
		 */
		if( rl[i].lcn < ( off_t )-1 )
		{
			ntfs_error( "layout_runs() encountered an out of bounds cluster delta, lcn = %Ld\n", rl[i].lcn );
			return -ERANGE;
		}
		rclus = rl[i].lcn - cluster;
		len = rl[i].len;
		rec[offset] = 0;
		if( offset + 9 > size )
			return -E2BIG;	/* It might still fit, but this
					 * simplifies testing. */
		/*
		 * Run length is stored as signed number, so deal with it
		 * properly, i.e. observe that a negative number will have all
		 * its most significant bits set to 1 but we don't store that
		 * in the mapping pairs array. We store the smallest type of
		 * negative number required, thus in the first if we check
		 * whether len fits inside a signed byte and if so we store it
		 * as such, the next ifs check for a signed short, then a signed
		 * 24-bit and finally the full blown signed 32-bit. Same goes
		 * for rlus below. (AIA)
		 */
		if( len >= -0x80 && len <= 0x7f )
		{
			NTFS_PUTU8( rec + offset + 1, len & 0xff );
			coffs = 1;
		}
		else if( len >= -0x8000 && len <= 0x7fff )
		{
			NTFS_PUTU16( rec + offset + 1, len & 0xffff );
			coffs = 2;
		}
		else if( len >= -0x800000 && len <= 0x7fffff )
		{
			NTFS_PUTU24( rec + offset + 1, len & 0xffffff );
			coffs = 3;
		}
		else		/* if (len >= -0x80000000LL && len <= 0x7fffffff */
		{
			NTFS_PUTU32( rec + offset + 1, len );
			coffs = 4;
		}		/* else ... FIXME: When len becomes 64-bit we need to extend
				 *                 the else if () statements. (AIA) */
		*( rec + offset ) |= coffs++;
		if( rl[i].lcn == ( off_t )-1 )	/* Compressed run. */
			/* Nothing */ ;
		else if( rclus >= -0x80 && rclus <= 0x7f )
		{
			*( rec + offset ) |= 0x10;
			NTFS_PUTS8( rec + offset + coffs, rclus & 0xff );
			coffs += 1;
		}
		else if( rclus >= -0x8000 && rclus <= 0x7fff )
		{
			*( rec + offset ) |= 0x20;
			NTFS_PUTS16( rec + offset + coffs, rclus & 0xffff );
			coffs += 2;
		}
		else if( rclus >= -0x800000 && rclus <= 0x7fffff )
		{
			*( rec + offset ) |= 0x30;
			NTFS_PUTS24( rec + offset + coffs, rclus & 0xffffff );
			coffs += 3;
		}
		else		/* if (rclus >= -0x80000000LL && rclus <= 0x7fffffff) */
		{
			*( rec + offset ) |= 0x40;
			NTFS_PUTS32( rec + offset + coffs, rclus
				/* & 0xffffffffLL */  );
			coffs += 4;
		}		/* FIXME: When rclus becomes 64-bit.
				   else if (rclus >= -0x8000000000 && rclus <= 0x7FFFFFFFFF) {
				   *(rec + offset) |= 0x50;
				   NTFS_PUTS40(rec + offset + coffs, rclus &
				   0xffffffffffLL);
				   coffs += 5;
				   } else if (rclus >= -0x800000000000 && 
				   rclus <= 0x7FFFFFFFFFFF) {
				   *(rec + offset) |= 0x60;
				   NTFS_PUTS48(rec + offset + coffs, rclus &
				   0xffffffffffffLL);
				   coffs += 6;
				   } else if (rclus >= -0x80000000000000 && 
				   rclus <= 0x7FFFFFFFFFFFFF) {
				   *(rec + offset) |= 0x70;
				   NTFS_PUTS56(rec + offset + coffs, rclus &
				   0xffffffffffffffLL);
				   coffs += 7;
				   } else {
				   *(rec + offset) |= 0x80;
				   NTFS_PUTS64(rec + offset + coffs, rclus);
				   coffs += 8;
				   } */
		offset += coffs;
		if( rl[i].lcn )
			cluster = rl[i].lcn;
	}
	if( offset >= size )
		return -E2BIG;
	/* Terminating null. */
	*( rec + offset++ ) = 0;
	*offs = offset;
	return 0;
}

static void count_runs( ntfs_attribute * attr, char *buf )
{
	uint32 first, count, last, i;

	first = 0;
	for( i = 0, count = 0; i < attr->d.r.len; i++ )
		count += attr->d.r.runlist[i].len;
	last = first + count - 1;
	NTFS_PUTU64( buf + 0x10, first );
	NTFS_PUTU64( buf + 0x18, last );
}

static int layout_attr( ntfs_attribute * attr, char *buf, int size, int *psize )
{
	int nameoff, hdrsize, asize;

	if( attr->resident )
	{
		nameoff = 0x18;
		hdrsize = ( nameoff + 2 * attr->namelen + 7 ) & ~7;
		asize = ( hdrsize + attr->size + 7 ) & ~7;
		if( size < asize )
			return -E2BIG;
		NTFS_PUTU32( buf + 0x10, attr->size );
		NTFS_PUTU8( buf + 0x16, attr->indexed );
		NTFS_PUTU16( buf + 0x14, hdrsize );
		if( attr->size )
			ntfs_memcpy( buf + hdrsize, attr->d.data, attr->size );
	}
	else
	{
		int error;

		if( attr->flags & ATTR_IS_COMPRESSED )
			nameoff = 0x48;
		else
			nameoff = 0x40;
		hdrsize = ( nameoff + 2 * attr->namelen + 7 ) & ~7;
		if( size < hdrsize )
			return -E2BIG;
		/* Make asize point at the end of the attribute record header,
		   i.e. at the beginning of the mapping pairs array. */
		asize = hdrsize;
		error = layout_runs( attr, buf, &asize, size );
		/* Now, asize points one byte beyond the end of the mapping
		   pairs array. */
		if( error )
			return error;
		/* The next attribute has to begin on 8-byte boundary. */
		asize = ( asize + 7 ) & ~7;
		/* FIXME: fragments */
		count_runs( attr, buf );
		NTFS_PUTU16( buf + 0x20, hdrsize );
		NTFS_PUTU16( buf + 0x22, attr->cengine );
		NTFS_PUTU32( buf + 0x24, 0 );
		NTFS_PUTS64( buf + 0x28, attr->allocated );
		NTFS_PUTS64( buf + 0x30, attr->size );
		NTFS_PUTS64( buf + 0x38, attr->initialized );
		if( attr->flags & ATTR_IS_COMPRESSED )
			NTFS_PUTS64( buf + 0x40, attr->compsize );
	}
	NTFS_PUTU32( buf, attr->type );
	NTFS_PUTU32( buf + 4, asize );
	NTFS_PUTU8( buf + 8, attr->resident ? 0 : 1 );
	NTFS_PUTU8( buf + 9, attr->namelen );
	NTFS_PUTU16( buf + 0xa, nameoff );
	NTFS_PUTU16( buf + 0xc, attr->flags );
	NTFS_PUTU16( buf + 0xe, attr->attrno );
	if( attr->namelen )
		ntfs_memcpy( buf + nameoff, attr->name, 2 * attr->namelen );
	*psize = asize;
	return 0;
}

int layout_inode( ntfs_inode_s * ino, ntfs_disk_inode * store )
{
	int offset, i, size, psize, error, count, recno;
	ntfs_attribute *attr;
	unsigned char *rec;

	error = allocate_store( ino->vol, store, ino->record_count );
	if( error )
		return error;
	size = ino->vol->mft_record_size;
	count = i = 0;
	do
	{
		if( count < ino->record_count )
		{
			recno = ino->records[count];
		}
		else
		{
			error = allocate_store( ino->vol, store, count + 1 );
			if( error )
				return error;
			recno = -1;
		}
		/*
		 * FIXME: We need to support extension records properly.
		 * At the moment they wouldn't work. Probably would "just" get
		 * corrupted if we write to them... (AIA)
		 */
		store->records[count].recno = recno;
		rec = store->records[count].record;
		count++;
		/* Copy mft record header. */
		offset = NTFS_GETU16( ino->attr + 0x14 );	/* attrs_offset */
		ntfs_memcpy( rec, ino->attr, offset );
		/* Copy attributes. */
		while( i < ino->attr_count )
		{
			attr = ino->attrs + i;
			error = layout_attr( attr, rec + offset, size - offset - 8, &psize );
			if( error == -E2BIG && offset != NTFS_GETU16( ino->attr + 0x14 ) )
				break;
			if( error )
				return error;
			offset += psize;
			i++;
		}
		/* Terminating attribute. */
		NTFS_PUTU32( rec + offset, 0xFFFFFFFF );
		offset += 4;
		NTFS_PUTU32( rec + offset, 0 );
		offset += 4;
		NTFS_PUTU32( rec + 0x18, offset );
	}
	while( i < ino->attr_count || count < ino->record_count );
	return count - ino->record_count;
}

int ntfs_update_inode( ntfs_inode_s * ino )
{
	int error, i;
	ntfs_disk_inode store;
	ntfs_io io;

	memset( &store, 0, sizeof( store ) );
	error = layout_inode( ino, &store );
	if( error == -E2BIG )
	{
		i = ntfs_split_indexroot( ino );
		if( i != -ENOTDIR )
		{
			if( !i )
				i = layout_inode( ino, &store );
			error = i;
		}
	}
	if( error == -E2BIG )
	{
		error = ntfs_attr_allnonresident( ino );
		if( !error )
			error = layout_inode( ino, &store );
	}
	if( error > 0 )
	{
		/* FIXME: Introduce extension records. */
		error = -E2BIG;
	}
	if( error )
	{
		if( error == -E2BIG )
			ntfs_error( "Cannot handle saving inode 0x%Lx.\n", ino->i_ino );
		deallocate_store( &store );
		return error;
	}
	io.fn_get = ntfs_get;
	io.fn_put = 0;
	for( i = 0; i < store.count; i++ )
	{
		error = ntfs_insert_fixups( store.records[i].record, ino->vol->mft_record_size );
		if( error )
		{
			ntfs_error( "NTFS: ntfs_update_inode() caught corrupt %s mtf record ntfs record header. Refusing to write corrupt data to disk. Unmount and run chkdsk immediately!\n", i ? "extension" : "base" );
			deallocate_store( &store );
			return -EIO;
		}
		io.param = store.records[i].record;
		io.size = ino->vol->mft_record_size;
		error = ntfs_write_attr( ino->vol->mft_ino, ino->vol->at_data, 0, ( int64 )store.records[i].recno << ino->vol->mft_record_size_bits, &io );
		if( error || io.size != ino->vol->mft_record_size )
		{
			/* Big trouble, partially written file. */
			ntfs_error( "Please unmount: Write error in inode 0x%Lx\n", ino->i_ino );
			deallocate_store( &store );
			return error ? error : -EIO;
		}
	}
	deallocate_store( &store );
	return 0;
}

void ntfs_decompress( unsigned char *dest, unsigned char *src, size_t l )
{
	int head, comp;
	int copied = 0;
	unsigned char *stop;
	int bits;
	int tag = 0;
	int clear_pos;

	while( 1 )
	{
		head = NTFS_GETU16( src ) & 0xFFF;
		/* High bit indicates that compression was performed. */
		comp = NTFS_GETU16( src ) & 0x8000;
		src += 2;
		stop = src + head;
		bits = 0;
		clear_pos = 0;
		if( head == 0 )
			/* Block is not used. */
			return;	/* FIXME: copied */
		if( !comp )
		{		/* uncompressible */
			ntfs_memcpy( dest, src, 0x1000 );
			dest += 0x1000;
			copied += 0x1000;
			src += 0x1000;
			if( l == copied )
				return;
			continue;
		}
		while( src <= stop )
		{
			if( clear_pos > 4096 )
			{
				ntfs_error( "Error 1 in decompress\n" );
				return;
			}
			if( !bits )
			{
				tag = NTFS_GETU8( src );
				bits = 8;
				src++;
				if( src > stop )
					break;
			}
			if( tag & 1 )
			{
				int i, len, delta, code, lmask, dshift;

				code = NTFS_GETU16( src );
				src += 2;
				if( !clear_pos )
				{
					ntfs_error( "Error 2 in decompress\n" );
					return;
				}
				for( i = clear_pos - 1, lmask = 0xFFF, dshift = 12; i >= 0x10; i >>= 1 )
				{
					lmask >>= 1;
					dshift--;
				}
				delta = code >> dshift;
				len = ( code & lmask ) + 3;
				for( i = 0; i < len; i++ )
				{
					dest[clear_pos] = dest[clear_pos - delta - 1];
					clear_pos++;
					copied++;
					if( copied == l )
						return;
				}
			}
			else
			{
				dest[clear_pos++] = NTFS_GETU8( src );
				src++;
				copied++;
				if( copied == l )
					return;
			}
			tag >>= 1;
			bits--;
		}
		dest += clear_pos;
	}
}

/*
 * NOTE: Neither of the ntfs_*_bit functions are atomic! But we don't need
 * them atomic at present as we never operate on shared/cached bitmaps.
 */
static __inline__ int ntfs_test_bit( unsigned char *byte, const int bit )
{
	return byte[bit >> 3] & ( 1 << ( bit & 7 ) ) ? 1 : 0;
}

static __inline__ void ntfs_set_bit( unsigned char *byte, const int bit )
{
	byte[bit >> 3] |= 1 << ( bit & 7 );
}

static __inline__ void ntfs_clear_bit( unsigned char *byte, const int bit )
{
	byte[bit >> 3] &= ~( 1 << ( bit & 7 ) );
}

static __inline__ int ntfs_test_and_clear_bit( unsigned char *byte, const int bit )
{
	unsigned char *ptr = byte +( bit >> 3 );
	int b = 1 << ( bit & 7 );
	int oldbit = *ptr & b ? 1 : 0;

	*ptr &= ~b;
	return oldbit;
}

static void dump_runlist( const ntfs_runlist * rl, const int rlen )
{
}

int splice_runlists( ntfs_runlist ** rl1, int *r1len, const ntfs_runlist * rl2, int r2len )
{
	ntfs_runlist *rl;
	int rlen, rl_size, rl2_pos;

	if( *rl1 )
		dump_runlist( *rl1, *r1len );
	else
		ntfs_error( "%s(): Not present.\n", __FUNCTION__ );
	dump_runlist( rl2, r2len );
	rlen = *r1len + r2len + 1;
	rl_size = ( rlen * sizeof( ntfs_runlist ) + PAGE_SIZE - 1 ) & PAGE_MASK;
	/* Do we have enough space? */
	if( rl_size <= ( ( *r1len * sizeof( ntfs_runlist ) + PAGE_SIZE - 1 ) & PAGE_MASK ) )
	{
		/* Have enough space already. */
		rl = *rl1;
	}
	else
	{
		/* Need more space. Reallocate. */
		rl = ntfs_malloc( rlen << sizeof( ntfs_runlist ) );
		if( !rl )
			return -ENOMEM;
		/* Copy over rl1. */
		ntfs_memcpy( rl, *rl1, *r1len * sizeof( ntfs_runlist ) );
		ntfs_free( *rl1 );
		*rl1 = rl;
	}
	/* Reuse rl_size as the current position index into rl. */
	rl_size = *r1len - 1;
	/* Coalesce neighbouring elements, if present. */
	rl2_pos = 0;
	if( rl[rl_size].lcn + rl[rl_size].len == rl2[rl2_pos].lcn )
	{
		rl[rl_size].len += rl2[rl2_pos].len;
		rl2_pos++;
		r2len--;
		rlen--;
	}
	rl_size++;
	/* Copy over rl2. */
	ntfs_memcpy( rl + rl_size, rl2 + rl2_pos, r2len * sizeof( ntfs_runlist ) );
	rlen--;
	rl[rlen].lcn = ( off_t )-1;
	rl[rlen].len = ( off_t )0;
	*r1len = rlen;
	dump_runlist( *rl1, *r1len );
	return 0;
}

void ntfs_clear_inode( ntfs_inode_s * ino )
{
	int i;

	if( !ino->attr )
	{
		ntfs_error( "ntfs_clear_inode: double free\n" );
		return;
	}
	ntfs_free( ino->attr );
	ino->attr = 0;
	if( ino->records )
		ntfs_free( ino->records );
	ino->records = 0;
	for( i = 0; i < ino->attr_count; i++ )
	{
		if( ino->attrs[i].name )
			ntfs_free( ino->attrs[i].name );
		if( ino->attrs[i].resident )
		{
			if( ino->attrs[i].d.data )
				ntfs_free( ino->attrs[i].d.data );
		}
		else
		{
			if( ino->attrs[i].d.r.runlist )
				ntfs_free( ino->attrs[i].d.r.runlist );
		}
	}
	ntfs_free( ino->attrs );
	ino->attrs = 0;
}

int ntfs_read_inode( void *_vol, ino_t inid, void **_node )
{
	ntfs_volume_s *vol = ( ntfs_volume_s * ) _vol;
	ntfs_inode_s *ino, *tmpnode;
	ntfs_attribute *data, *si;

	ino = ( ntfs_inode_s * ) ntfs_calloc( sizeof( ntfs_inode_s ) );
	if( !ino )
		return -ENOMEM;

	ntfs_debug( "reading inode %Ld at %p\n",inid,ino);

	ino->i_ino = inid;

	switch ( inid )
	{
		case FILE_Mft:
			if( !vol->mft_ino || ( ( vol->ino_flags & 1 ) == 0 ) )
				goto sys_file_error;
			ntfs_debug( "Opening $MFT ...\n" );
			memcpy( ino, vol->mft_ino, sizeof( ntfs_inode_s ) );
			tmpnode = vol->mft_ino;
			vol->mft_ino = ino;
			vol->ino_flags &= ~1;
			kfree( tmpnode );
			break;
		case FILE_MftMirr:
			if( !vol->mftmirr || ( ( vol->ino_flags & 2 ) == 0 ) )
				goto sys_file_error;
			ntfs_debug( "Opening $MFTMirr ...\n" );
			memcpy( ino, vol->mftmirr, sizeof( ntfs_inode_s ) );
			tmpnode = vol->mftmirr;
			vol->mftmirr = ino;
			vol->ino_flags &= ~2;
			kfree( ino );
			break;
		case FILE_BitMap:
			if( !vol->bitmap || ( ( vol->ino_flags & 4 ) == 0 ) )
				goto sys_file_error;
			ntfs_debug( "Opening $Bitmap\n" );
			memcpy( ino, vol->bitmap, sizeof( ntfs_inode_s ) );
			tmpnode = vol->bitmap;
			vol->bitmap = ino;
			vol->ino_flags &= ~4;
			kfree( tmpnode );
			break;
		case FILE_LogFile...FILE_AttrDef:
		case FILE_Boot...FILE_UpCase:
			ntfs_debug( "Opening system file %Ld\n", ( int64 )inid );
		default:
			if( ntfs_init_inode( ino, vol, ino->i_ino ) )
			{
				ntfs_error( "ntfs: error loading inode %Ld\n", ( int64 )ino->i_ino );
				return -ENOMEM;
			}
			break;
	}

	ino->i_uid = vol->uid;
	ino->i_gid = vol->gid;

	data = ntfs_find_attr( ino, vol->at_data, NULL );
	if( !data )
		ino->i_size = 0;
	else
		ino->i_size = data->size;

	si = ntfs_find_attr( ino, vol->at_standard_information, NULL );
	if( si )
	{
		char *attr = si->d.data;

		ino->atime = ntfs_ntutc2syllable( NTFS_GETU64( attr + 0x18 ) );
		ino->ctime = ntfs_ntutc2syllable( NTFS_GETU64( attr ) );
		ino->mtime = ntfs_ntutc2syllable( NTFS_GETU64( attr + 8 ) );
	}

	if( ntfs_find_attr( ino, vol->at_index_root, "$I30" ) )
	{
		ntfs_debug( "ntfs_read_inode: this is directory\n");
		ntfs_attribute *at = ntfs_find_attr( ino, vol->at_index_root, "$I30" );

		ino->i_size = at ? at->size : 0;
		ino->i_mode = S_IFDIR | 0666;
	}
	else
	{
		ntfs_debug( "ntfs_read_inode: this is regular file\n" );
		ino->i_mode = S_IFREG | 0666;
	}
	ino->i_mode &= ~vol->umask;

	ntfs_debug( "ino %d mode: %o size %Ld\n",(int)ino->i_ino,ino->i_mode,ino->i_size);
	ntfs_debug( "atime %Ld mtime %Ld ctime %Ld\n",ino->atime,ino->mtime,ino->ctime);

	*_node = ( void * )ino;
	return 0;

sys_file_error:
	ntfs_error( "Critical error. Tried to call ntfs_read_inode() before we have completed read_super() or VFS error.\n" );
	return -EINVAL;
}
