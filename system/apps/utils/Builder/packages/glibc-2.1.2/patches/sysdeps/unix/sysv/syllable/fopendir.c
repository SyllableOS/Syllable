/* Copyright (C) 2001 Kurt Skauen
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

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#include <dirstream.h>

/* Open a directory stream on NAME.  */
DIR* __fopendir( int fd )
{
    DIR *dirp;
    struct stat statbuf;
    size_t allocation;

    if (fd < 0)
	return NULL;

      /* Now make sure this really is a directory */
    if (__fstat (fd, &statbuf) < 0)
	return NULL;
    if ( S_ISDIR(statbuf.st_mode) == false ) {
	return NULL;
    }

    if ( __fcntl (fd, F_SETFD, FD_CLOEXEC) < 0 )
	return NULL;

#ifdef _STATBUF_ST_BLKSIZE
    if (statbuf.st_blksize < sizeof (struct dirent))
	allocation = sizeof (struct dirent);
    else
	allocation = statbuf.st_blksize;
#else
    allocation = (BUFSIZ < sizeof (struct dirent)
		  ? sizeof (struct dirent) : BUFSIZ);
#endif

    dirp = (DIR *) calloc (1, sizeof (DIR) + allocation); /* Zero-fill.  */
    if (dirp == NULL) {
	return NULL;
    }
    dirp->data = (char *) (dirp + 1);
    dirp->allocation = allocation;
    dirp->fd = fd;

    __libc_lock_init (dirp->lock);

    return dirp;
}
weak_alias (__fopendir, fopendir)
