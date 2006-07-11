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

#define le16_to_cpu(x)	((uint16_t)(x))

const uint8 legal_ansi_char_array[0x40] = {
	0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,

	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,

	0x17, 0x07, 0x18, 0x17, 0x17, 0x17, 0x17, 0x17,
	0x17, 0x17, 0x18, 0x16, 0x16, 0x17, 0x07, 0x00,

	0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
	0x17, 0x17, 0x04, 0x16, 0x18, 0x16, 0x18, 0x18,
};

int ntfs_are_names_equal( wchar_t *s1, size_t s1_len, wchar_t *s2, size_t s2_len, int ic, wchar_t *upcase, uint32 upcase_size )
{
	if( s1_len != s2_len )
		return 0;
	if( !ic )
		return memcmp( s1, s2, s1_len << 1 ) ? 0 : 1;
	return ntfs_wcsncasecmp( s1, s2, s1_len, upcase, upcase_size ) ? 0 : 1;
}

int ntfs_collate_names( wchar_t *upcase, uint32 upcase_len, wchar_t *name1, uint32 name1_len, wchar_t *name2, uint32 name2_len, int ic, int err_val )
{
	uint32 cnt, min_len;
	wchar_t c1, c2;

	min_len = name1_len;
	if( min_len > name2_len )
		min_len = name2_len;
	for( cnt = 0; cnt < min_len; ++cnt )
	{
		c1 = le16_to_cpu( *name1++ );
		c2 = le16_to_cpu( *name2++ );
		if( ic )
		{
			if( c1 < upcase_len )
				c1 = le16_to_cpu( upcase[c1] );
			if( c2 < upcase_len )
				c2 = le16_to_cpu( upcase[c2] );
		}
		if( c1 < 64 && legal_ansi_char_array[c1] & 8 )
			return err_val;
		if( c1 < c2 )
			return -1;
		if( c1 > c2 )
			return 1;
		++name1;
		++name2;
	}
	if( name1_len < name2_len )
		return -1;
	if( name1_len == name2_len )
		return 0;
	/* name1_len > name2_len */
	c1 = le16_to_cpu( *name1 );
	if( c1 < 64 && legal_ansi_char_array[c1] & 8 )
		return err_val;
	return 1;
}

int ntfs_wcsncasecmp( wchar_t *s1, wchar_t *s2, size_t n, wchar_t *upcase, uint32 upcase_size )
{
	wchar_t c1, c2;
	size_t i;

	for( i = 0; i < n; ++i )
	{
		if( ( c1 = le16_to_cpu( s1[i] ) ) < upcase_size )
			c1 = le16_to_cpu( upcase[c1] );
		if( ( c2 = le16_to_cpu( s2[i] ) ) < upcase_size )
			c2 = le16_to_cpu( upcase[c2] );
		if( c1 < c2 )
			return -1;
		if( c1 > c2 )
			return 1;
		if( !c1 )
			break;
	}
	return 0;
}
