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
#include <unistd.h>

ssize_t getdirentries(int fd, char* buf, size_t nbytes, off_t* basep )
{
  off_t base;
  ssize_t rbytes;

  base = __lseek(fd, (off_t)0, SEEK_CUR);
  rbytes = __getdents(fd, buf, nbytes);
  if(NULL != basep && rbytes >= 0)
    *basep = rbytes;

  return(rbytes);
}
