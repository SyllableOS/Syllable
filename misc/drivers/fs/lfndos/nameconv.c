/*
 *  lfndos - AtheOS filesystem that use MS-DOS to store/retrive files
 *  Copyright (C) 1999  Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <atheos/ctype.h>
#include <posix/errno.h>
#include <atheos/types.h>
#include <atheos/kernel.h>

#include "lfndos.h"

#include <macros.h>


#define	MSDOS_NAME	11


static const char *reserved_names[] = {
	"CON     ","PRN     ","NUL     ","AUX     ",
	"LPT1    ","LPT2    ","LPT3    ","LPT4    ",
	"COM1    ","COM2    ","COM3    ","COM4    ",
	NULL };


/* Characters that are undesirable in an MS-DOS file name */

static char bad_chars[] = "*?<>|\":/\\";
static char bad_if_strict[] = "+=,; []";
static char replace_chars[] = "[];,+=";


/* Checks the validity of an long MS-DOS filename */
/* Returns negative number on error, 0 for a normal
 * return, and 1 for . or .. */

/* Takes a short filename and converts it to a formatted MS-DOS filename.
 * If the short filename is not a valid MS-DOS filename, an error is
 * returned.  The formatted short filename is returned in 'res'.
 */

static int lfd_FormatName( char conv,const char *name,int len,char *res, int dot_dirs )
{
    char *walk;
    const char **reserved;
    unsigned char c;
    int space;

/*	if (IS_FREE(name)) return -EINVAL; */
    if (name[0] == '.' && (len == 1 || (len == 2 && name[1] == '.'))) {
	if (!dot_dirs) return -EEXIST;
	memset(res+1,' ',10);
	while (len--) *res++ = '.';
	return 0;
    }

    space = 1; /* disallow names starting with a dot */
    c = 0;
    for (walk = res; len && walk-res < 8; walk++) {
	c = *name++;
	len--;
	if (conv != 'r' && strchr(bad_chars,c)) return -EINVAL;
	if (conv == 's' && strchr(bad_if_strict,c)) return -EINVAL;
	if (conv == 'x' && strchr(replace_chars,c)) return -EINVAL;
	if (c >= 'A' && c <= 'Z' && (conv == 's' || conv == 'x')) return -EINVAL;
	if (c < ' ' || c == ':' || c == '\\') return -EINVAL;
	if (c == '.') break;
	space = c == ' ';
	*walk = c >= 'a' && c <= 'z' ? c-32 : c;
    }
    if (space) return -EINVAL;
    if ((conv == 's' || conv == 'x') && len && c != '.') {
	c = *name++;
	len--;
	if (c != '.') return -EINVAL;
    }
    while (c != '.' && len--) c = *name++;
    if (c == '.') {
	while (walk-res < 8) *walk++ = ' ';
	while (len > 0 && walk-res < MSDOS_NAME) {
	    c = *name++;
	    len--;
	    if (conv != 'r' && strchr(bad_chars,c)) return -EINVAL;
	    if (conv == 's' && strchr(bad_if_strict,c))
		return -EINVAL;
	    if (conv == 'x' && strchr(replace_chars,c))
		return -EINVAL;
	    if (c < ' ' || c == ':' || c == '\\' || c == '.')
		return -EINVAL;
	    if (c >= 'A' && c <= 'Z' && (conv == 's' || conv == 'x')) return -EINVAL;
	    space = c == ' ';
	    *walk++ = c >= 'a' && c <= 'z' ? c-32 : c;
	}
	if (space) return -EINVAL;
	if ((conv == 's' || conv == 'x') && len) return -EINVAL;
    }
    while (walk-res < MSDOS_NAME) *walk++ = ' ';
    for (reserved = reserved_names; *reserved; reserved++)
	if (!strncmp(res,*reserved,8)) return -EINVAL;

    return 0;
}

static char skip_chars[] = ".:\"?<>| ";

/* Given a valid longname, create a unique shortname.  Make sure the
 * shortname does not exist
 */
int lfd_CreateShortName( FileEntry_s* psParent, const char *name, int len, char *pzDst )
{
    const char *ip, *ext_start, *end;
    char *p;
    int sz, extlen, baselen, totlen;
    char msdos_name[13];
    char base[9], ext[4];
    int i, j;
    int res;
    int spaces;
    char buf[8];
    const char *name_start;
    char	name_res[16];

    sz = 0; /* Make compiler happy */

    if (len && name[len-1]==' ') {
	return -EINVAL;
    }

    if ( len <= 12 ) {
	  /* Do a case insensitive search if the name would be a valid
	   * shortname if is were all capitalized */
	for (i = 0, p = msdos_name, ip = name; i < len; i++, p++, ip++) {
	    if (*ip >= 'A' && *ip <= 'Z') {
		*p = *ip + 32;
	    } else {
		*p = *ip;
	    }
	}
	res = lfd_FormatName( 'x', msdos_name, len, name_res, 1 );

	if (res > -1) {
	    res = lfd_LookupFile( psParent, msdos_name, len, NULL );

	    if (res > -1) return -EEXIST;
	    memcpy( pzDst, msdos_name, len );
	    pzDst[len] = '\0';
	    return 0;
	}
    }

      /* Now, we need to create a shortname from the long name */

    ext_start = end = &name[len];
    while (--ext_start >= name) {
	if (*ext_start == '.') {
	    if (ext_start == end - 1) {
		sz = len;
		ext_start = NULL;
	    }
	    break;
	}
    }
    if (ext_start == name - 1) {
	sz = len;
	ext_start = NULL;
    } else if (ext_start) {
	  /*
	   * Names which start with a dot could be just
	   * an extension eg. "...test".  In this case Win95
	   * uses the extension as the name and sets no extension.
	   */
	name_start = &name[0];
	while (name_start < ext_start)
	{
	    if (!strchr(skip_chars,*name_start)) break;
	    name_start++;
	}
	if (name_start != ext_start) {
	    sz = ext_start - name;
	    ext_start++;
	} else {
	    sz = len;
	    ext_start=NULL;
	}
    }

    for (baselen = i = 0, p = base, ip = name; i < sz && baselen < 8; i++) {
	if (!strchr(skip_chars, *ip)) {
	    if (*ip >= 'A' && *ip <= 'Z') {
		*p = *ip + 32;
	    } else {
		*p = *ip;
	    }
	    if (strchr(replace_chars, *p)) *p='_';
	    p++; baselen++;
	}
	ip++;
    }
    if (baselen == 0) {
	return -EINVAL;
    }

    spaces = 8 - baselen;

    if (ext_start) {
	extlen = 0;
	for (p = ext, ip = ext_start; extlen < 3 && ip < end; ip++) {
	    if (!strchr(skip_chars, *ip)) {
		if (*ip >= 'A' && *ip <= 'Z') {
		    *p = *ip + 32;
		} else {
		    *p = *ip;
		}
		if (strchr(replace_chars, *p)) *p='_';
		extlen++;
		p++;
	    }
	}
    } else {
	extlen = 0;
    }
    ext[extlen] = '\0';
    base[baselen] = '\0';

    strcpy(msdos_name, base);
    msdos_name[baselen] = '.';
    strcpy(&msdos_name[baselen+1], ext);

    totlen = baselen + extlen + (extlen > 0);

    res = 0;
/*
  if (MSDOS_SB(dir->i_sb)->options.numtail == 0) {
  res = vfat_find(dir, msdos_name, totlen, 0, 0, 0, &sinfo);
  }
  */

    i = 0;

    while (res > -1) {
	  /* Create the next shortname to try */
	i++;
	if (i == 10000000) return -EEXIST;
	sprintf(buf, "%d", i);
	sz = strlen(buf);
	if (sz + 1 > spaces) {
	    baselen = baselen - (sz + 1 - spaces);
	    spaces = sz + 1;
	}

	strncpy(msdos_name, base, baselen);
	msdos_name[baselen] = '~';
	strcpy(&msdos_name[baselen+1], buf);
	msdos_name[baselen+sz+1] = '.';
	strcpy(&msdos_name[baselen+sz+2], ext);

	totlen = baselen + sz + 1 + extlen + (extlen > 0);

	res = lfd_LookupFile( psParent, msdos_name, totlen, NULL );
    }
    res = lfd_FormatName('x', msdos_name, totlen, name_res, 1);

    for ( len = 10 ; len >=0 ; --len ) {
	if ( name_res[len] != ' ' ) {
	    break;
	}
    }
    len++;
      
    for ( i = 0, j = 0 ; i < len ; ++i ) {
	pzDst[j++] = name_res[i];
	if ( 7 == i && strcmp( name_res + 8, "   " ) != 0 ) {
	    pzDst[ j++ ] = '.';
	}
    }
    pzDst[ j++ ] = '\0';
    memcpy( pzDst, msdos_name, totlen );
    pzDst[totlen] = '\0';
    return res;
}
