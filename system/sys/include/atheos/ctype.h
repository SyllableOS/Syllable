/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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

#ifndef __F_ATHEOS_CTYPE_H__
#define __F_ATHEOS_CTYPE_H__

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

#define __ISALNUM 	0x0001
#define __ISALPHA	0x0002
#define __ISCNTRL	0x0004
#define __ISDIGIT	0x0008
#define __ISGRAPH	0x0010
#define __ISLOWER	0x0020
#define __ISPRINT	0x0040
#define __ISPUNCT	0x0080
#define __ISSPACE	0x0100
#define __ISUPPER	0x0200
#define __ISXDIGIT	0x0400

extern unsigned short __ctype_flags[];
extern unsigned char __ctype_toupper[];
extern unsigned char __ctype_tolower[];

#define isalnum(c) (__ctype_flags[((c)&0xff)+1] & __ISALNUM)
#define isalpha(c) (__ctype_flags[((c)&0xff)+1] & __ISALPHA)
#define iscntrl(c) (__ctype_flags[((c)&0xff)+1] & __ISCNTRL)
#define isdigit(c) (__ctype_flags[((c)&0xff)+1] & __ISDIGIT)
#define isgraph(c) (__ctype_flags[((c)&0xff)+1] & __ISGRAPH)
#define islower(c) (__ctype_flags[((c)&0xff)+1] & __ISLOWER)
#define isprint(c) (__ctype_flags[((c)&0xff)+1] & __ISPRINT)
#define ispunct(c) (__ctype_flags[((c)&0xff)+1] & __ISPUNCT)
#define isspace(c) (__ctype_flags[((c)&0xff)+1] & __ISSPACE)
#define isupper(c) (__ctype_flags[((c)&0xff)+1] & __ISUPPER)
#define isxdigit(c) (__ctype_flags[((c)&0xff)+1] & __ISXDIGIT)

#define tolower(c) (__ctype_tolower[((c)&0xff)+1])
#define toupper(c) (__ctype_toupper[((c)&0xff)+1])

#define isascii(c) (!((c)&(~0x7f)))
#define toascii(c) ((c)&0x7f)

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_CTYPE_H__ */
