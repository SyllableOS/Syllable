/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef __F_POSIX_LIMITS_H__
#define __F_POSIX_LIMITS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CHAR_BIT 8
#define CHAR_MAX 127
#define CHAR_MIN (-128)
#define INT_MAX 2147483647
#define INT_MIN (-2147483647-1)
#define LONG_MAX 2147483647L
#define LONG_MIN (-2147483647L-1L)
#define SCHAR_MAX 127
#define SCHAR_MIN (-128)
#define SHRT_MAX 32767
#define SHRT_MIN (-32768)
#define UCHAR_MAX 255
#define UINT_MAX 4294967295U
#define ULONG_MAX 4294967295UL
#define USHRT_MAX 65535

#ifndef __STRICT_ANSI__

#if 0

#define _POSIX_ARG_MAX		131072	/* but only for exec's to other djgpp programs */
#define _POSIX_CHILD_MAX	4096	/* really limited by memory */

#ifndef _POSIX_LINK_MAX
#define _POSIX_LINK_MAX		1	
#endif
  
#ifndef _POSIX_MAX_CANON
#define _POSIX_MAX_CANON	126	
#endif
  
#ifndef _POSIX_MAX_INPUT
#define _POSIX_MAX_INPUT	126	
#endif

#ifndef _POSIX_NAME_MAX
#define _POSIX_NAME_MAX		256
#endif
  
#define _POSIX_NGROUPS_MAX	32

#ifndef _POSIX_OPEN_MAX
#define _POSIX_OPEN_MAX		256
#endif

#define _POSIX_PATH_MAX		255
#define _POSIX_PIPE_BUF		512

#ifndef _POSIX_SSIZE_MAX
#define _POSIX_SSIZE_MAX	2147483647
#endif
  
#ifndef _POSIX_STREAM_MAX
#define _POSIX_STREAM_MAX	256
#endif

#ifndef _POSIX_TZNAME_MAX
#define _POSIX_TZNAME_MAX	5
#endif

#endif	/* 0 */
  
/* constants used in Solaris */
#define LLONG_MIN       (-9223372036854775807LL-1LL)
#define LLONG_MAX       9223372036854775807LL
#define ULLONG_MAX      18446744073709551615ULL
/* gnuc ones */
#define LONG_LONG_MIN	LLONG_MIN
#define LONG_LONG_MAX	LLONG_MAX
#define ULONG_LONG_MAX	ULLONG_MAX

#define MAXSYMLINKS	16

#ifndef SSIZE_MAX
#define SSIZE_MAX		2147483647
#endif
  
#endif /* !__STRICT_ANSI__ */

#ifndef NGROUPS_MAX
#define NGROUPS_MAX		32
#endif

#ifndef ARG_MAX
#define ARG_MAX			131072
#endif

#ifndef CHILD_MAX
#define CHILD_MAX		4096
#endif

#ifndef OPEN_MAX
#define OPEN_MAX		256
#endif

#ifndef LINK_MAX
#define LINK_MAX		1
#endif
  
#ifndef MAX_CANON
#define MAX_CANON		126
#endif

#ifndef MAX_INPUT
#define MAX_INPUT		126
#endif

#ifndef NAME_MAX
#define NAME_MAX		255
#endif

#ifndef PATH_MAX
#define PATH_MAX		4096
#endif

#ifndef PIPE_BUF
#define PIPE_BUF		4096
#endif

#ifdef __cplusplus
}
#endif

#endif /* !__F_POSIX_LIMITS_H__ */
