/* O_*, F_*, FD_* bit values for Syllable.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
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

#ifndef	_FCNTL_H
# error "Never use <bits/fcntl.h> directly; include <fcntl.h> instead."
#endif

#include <sys/types.h>
#include <posix/fcntl.h>

/* XXX missing */
#ifdef __USE_LARGEFILE64
# define O_LARGEFILE	0
#endif

/* For now Linux has synchronisity options for data and read operations.
   We define the symbols here but let them do the same as O_SYNC since
   this is a superset.  */
#if defined __USE_POSIX199309 || defined __USE_UNIX98
# define O_DSYNC	O_SYNC	/* Synchronize data.  */
# define O_RSYNC	O_SYNC	/* Synchronize read operations.  */
#endif

/* XXX missing */
#define F_GETLK64	F_GETLK		/* Get record locking info.  */
#define F_SETLK64	F_SETLK		/* Set record locking info (non-blocking).  */
#define F_SETLKW64	F_SETLKW	/* Set record locking info (blocking).  */


/* Define some more compatibility macros to be backward compatible with
   BSD systems which did not managed to hide these kernel macros.  */
#ifdef	__USE_BSD
# define FAPPEND	O_APPEND
# define FFSYNC		O_FSYNC
# define FASYNC		O_ASYNC
# define FNONBLOCK	O_NONBLOCK
# define FNDELAY	O_NDELAY
#endif /* Use BSD.  */
