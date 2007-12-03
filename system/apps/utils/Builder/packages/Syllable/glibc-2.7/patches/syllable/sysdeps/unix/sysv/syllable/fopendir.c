/* Copyright (C) 1996, 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
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

#include <dirent.h>
#include <dirstream.h>
#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <bits/libc-lock.h>

DIR* __fopendir( int fd )
{
  struct stat fd_stat;
  size_t allocation;
  DIR *dir;

  if( fd < 0 )
  {
    __set_errno( EINVAL );
    return NULL;
  }

  /* Check the descriptor refers to a directory */
  if( __fstat( fd, &fd_stat) < 0 )
    return NULL;
  if( S_ISDIR( fd_stat.st_mode) == 0 )
    return NULL;

  /* Set flags */
  if( __fcntl( fd, F_SETFD, FD_CLOEXEC ) < 0 )
    return NULL;

#ifdef _STATBUF_ST_BLKSIZE
  if( fd_stat.st_blksize < sizeof( struct dirent ) )
  {
    allocation = sizeof( struct dirent );
  }
  else
    allocation = fd_stat.st_blksize;
#else
  if( BUFSIZ < sizeof( struct dirent ) )
  {
    allocation = sizeof( struct dirent );
  }
  else
    allocation = BUFSIZ;
#endif

  /* Create directory descriptor */
  dir = (DIR*) calloc( 1, sizeof( DIR ) + allocation );
  if( NULL == dir )
    return NULL;

  dir->allocation = allocation;
  dir->fd = fd;

  __libc_lock_init( dir->lock );

  return dir;
}
weak_alias(__fopendir,fopendir)

