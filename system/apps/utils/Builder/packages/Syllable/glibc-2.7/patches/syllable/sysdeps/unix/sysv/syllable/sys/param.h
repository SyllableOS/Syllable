/* Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */
#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H 1

#include <posix/limits.h>
#include <posix/param.h>

#include <sys/types.h>

/* Macros for counting and rounding.  */
#ifndef howmany
# define howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#define powerof2(x)	((((x)-1)&(x))==0)

/* Macros for min/max.  */
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))

/* BSD names for some <limits.h> values.  */

#define	NBBY		CHAR_BIT
#ifndef	NGROUPS
# define NGROUPS	NGROUPS_MAX
#endif
#define	CANBSIZ		MAX_CANON
#define	NCARGS		ARG_MAX
#define MAXPATHLEN	PATH_MAX
/* The following is not really correct but it is a value we used for a
   long time and which seems to be usable.  People should not use NOFILE
   anyway.  */
#define NOFILE		OPEN_MAX

#endif	/* _SYS_PARAM_H */
