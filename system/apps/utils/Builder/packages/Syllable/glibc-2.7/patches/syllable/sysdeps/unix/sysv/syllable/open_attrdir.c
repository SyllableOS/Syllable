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
#include <malloc.h>
#include <sysdep.h>
#include <errno.h>
#include <sys/syscall.h>

DIR* __open_attrdir( int file )
{
  DIR *dir;
  int attrdir;
  size_t allocation;

  allocation = sizeof(struct dirent);
  dir=malloc(sizeof(DIR) + allocation );
  if( NULL == dir )
    return NULL;

  attrdir = INLINE_SYSCALL( open_attrdir, 1, file );
  if( attrdir < 0 )
  {
    free( dir );
    return NULL;
  }

  dir->fd = attrdir;
  dir->allocation = allocation;
  dir->size = 0;
  dir->offset = 0;
  dir->filepos = 0;

  return dir;
}
weak_alias(__open_attrdir, open_attrdir)

