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

static int to_utf8( uint16 c, unsigned char *buf )
{
	if( c == 0 )
		return 0;	/* No support for embedded 0 runes. */
	if( c < 0x80 )
	{
		if( buf )
			buf[0] = ( unsigned char )c;
		return 1;
	}
	if( c < 0x800 )
	{
		if( buf )
		{
			buf[0] = 0xc0 | ( c >> 6 );
			buf[1] = 0x80 | ( c & 0x3f );
		}
		return 2;
	}
	/* c < 0x10000 */
	if( buf )
	{
		buf[0] = 0xe0 | ( c >> 12 );
		buf[1] = 0x80 | ( ( c >> 6 ) & 0x3f );
		buf[2] = 0x80 | ( c & 0x3f );
	}
	return 3;
}

static int from_utf8( const unsigned char *str, uint16 *c )
{
	int l = 0, i;

	if( *str < 0x80 )
	{
		*c = *str;
		return 1;
	}
	if( *str < 0xc0 )	/* Lead byte must not be 10xxxxxx. */
		return 0;	/* Is c0 a possible lead byte? */
	if( *str < 0xe0 )
	{			/* 110xxxxx */
		*c = *str & 0x1f;
		l = 2;
	}
	else if( *str < 0xf0 )
	{			/* 1110xxxx */
		*c = *str & 0xf;
		l = 3;
	}
	else if( *str < 0xf8 )
	{			/* 11110xxx */
		*c = *str & 7;
		l = 4;
	}
	else			/* We don't support characters above 0xFFFF in NTFS. */
		return 0;
	for( i = 1; i < l; i++ )
	{
		/* All other bytes must be 10xxxxxx. */
		if( ( str[i] & 0xc0 ) != 0x80 )
			return 0;
		*c <<= 6;
		*c |= str[i] & 0x3f;
	}
	return l;
}


static int ntfs_dupuni2utf8( uint16 *in, int in_len, char **out, int *out_len )
{
	int i, tmp;
	int len8;
	unsigned char *result;

	ntfs_debug("converting l = %d\n", in_len);

	/* Count the length of the resulting UTF-8. */
	for( i = len8 = 0; i < in_len; i++ )
	{
		tmp = to_utf8( NTFS_GETU16( in + i ), 0 );
		if( !tmp )
			/* Invalid character. */
			return -EILSEQ;
		len8 += tmp;
	}
	result = ( uint8 * )kmalloc( len8 + 1, MEMF_KERNEL | MEMF_OKTOFAILHACK );	/* allow for zero-termination */
	*out = ( char * )result;

	if( !result )
		return -ENOMEM;
	result[len8] = '\0';
	*out_len = len8;
	for( i = len8 = 0; i < in_len; i++ )
		len8 += to_utf8( NTFS_GETU16( in + i ), result + len8 );

	ntfs_debug("result %p:%s\n", result, result);

	return 0;
}

static int ntfs_duputf82uni( unsigned char *in, int in_len, uint16 **out, int *out_len )
{
	int i, tmp;
	int len16;
	uint16 *result;
	uint16 wtmp;

	for( i = len16 = 0; i < in_len; i += tmp, len16++ )
	{
		tmp = from_utf8( in + i, &wtmp );
		if( !tmp )
			return -EILSEQ;
	}
	*out = result = kmalloc( 2 * ( len16 + 1 ), MEMF_KERNEL | MEMF_OKTOFAILHACK );
	if( !result )
		return -ENOMEM;
	result[len16] = 0;
	*out_len = len16;
	for( i = len16 = 0; i < in_len; i += tmp, len16++ )
	{
		tmp = from_utf8( in + i, &wtmp );
		NTFS_PUTU16( result + len16, wtmp );
	}
	return 0;
}

int ntfs_encodeuni( uint16 *in, int in_len, char **out, int *out_len )
{
	return ntfs_dupuni2utf8( in, in_len, out, out_len );
}

int ntfs_decodeuni( char *in, int in_len, uint16 **out, int *out_len )
{
	return ntfs_duputf82uni( in, in_len, out, out_len );
}

void ntfs_put( ntfs_io * dest, void *src, size_t n )
{
	memcpy( dest->param, src, n );
	dest->param = ( char * )dest->param + n;
}

void ntfs_get( void *dest, ntfs_io * src, size_t n )
{
	memcpy( dest, src->param, n );
	src->param = ( char * )src->param + n;
}

void *ntfs_calloc( int size )
{
	return kmalloc( size, MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
}

void ntfs_ascii2uni( short int *to, char *from, int len )
{
	int i;

	for( i = 0; i < len; i++ )
		NTFS_PUTU16( to + i, from[i] );
	to[i] = 0;
}

int ntfs_uni_strncmp( short int *a, short int *b, int n )
{
	int i;

	for( i = 0; i < n; i++ )
	{
		if( NTFS_GETU16( a + i ) < NTFS_GETU16( b + i ) )
			return -1;
		if( NTFS_GETU16( b + i ) < NTFS_GETU16( a + i ) )
			return 1;
		if( NTFS_GETU16( a + i ) == 0 )
			break;
	}
	return 0;
}

int ntfs_ua_strncmp( short int *a, char *b, int n )
{
	int i;

	for( i = 0; i < n; i++ )
	{
		if( NTFS_GETU16( a + i ) < b[i] )
			return -1;
		if( b[i] < NTFS_GETU16( a + i ) )
			return 1;
		if( b[i] == 0 )
			return 0;
	}
	return 0;
}

#define NTFS_TIME_OFFSET ((bigtime_t)(369*365 + 89) * 24 * 3600 * 10000000)

bigtime_t ntfs_ntutc2syllable( bigtime_t ntutc )
{
	return ( ntutc - NTFS_TIME_OFFSET ) * 10LL;
}

bigtime_t ntfs_syllable2ntutc( bigtime_t t )
{
	return ( t / 10LL ) + NTFS_TIME_OFFSET;
}

#undef NTFS_TIME_OFFSET

/* Fill index name. */
void ntfs_indexname( char *buf, int type )
{
	char hex[] = "0123456789ABCDEF";
	int index;

	*buf++ = '$';
	*buf++ = 'I';
	for( index = 24; index > 0; index -= 4 )
		if( ( 0xF << index ) & type )
			break;
	while( index >= 0 )
	{
		*buf++ = hex[( type >> index ) & 0xF];
		index -= 4;
	}
	*buf = '\0';
}
