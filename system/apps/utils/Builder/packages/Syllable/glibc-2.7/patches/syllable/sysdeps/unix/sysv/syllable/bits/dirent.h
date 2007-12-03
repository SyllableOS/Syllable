/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _DIRENT_H
# error "Never use <bits/dirent.h> directly; include <dirent.h> instead."
#endif

#include <posix/dirent.h>

/* Syllable dirents are always 64bits wide so there is no specific 64bit ino_t,
   and there is no difference between dirent & dirent64 */

struct dirent
{
  __ino_t d_ino;
  __off_t d_off;
  short	  d_reclen;
  short	  d_namlen;
  char	  d_type;
  char    d_name[256];
};

#ifdef __USE_LARGEFILE64
#define dirent64 dirent
#endif

#define d_fileno	d_ino	/* Backwards compatibility.  */

#define _DIRENT_HAVE_D_NAMLEN
#define _DIRENT_HAVE_D_RECLEN
#undef _DIRENT_HAVE_D_OFF
#undef _DIRENT_HAVE_D_TYPE

/* Syllable extends the dirent API with fopendir() */
#define __FOPENDIR		1

