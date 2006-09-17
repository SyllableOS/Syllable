/* Return information about the filesystem on which FD resides.
   Copyright (C) 1996, 1997, 1998, 2002 Free Software Foundation, Inc.
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

#include <errno.h>
#include <string.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>

/* Return information about the filesystem on which FD resides.  */
int
fstatvfs (int fd, struct statvfs *buf)
{
  struct statfs fsbuf;

  if(__fstatfs(fd, &fsbuf) < 0 )
    return -1;

  buf->f_frsize = fsbuf.f_bsize;
  buf->f_blocks = fsbuf.f_blocks;
  buf->f_bfree = fsbuf.f_bfree;
  buf->f_bavail = fsbuf.f_bavail;
  buf->f_files = fsbuf.f_files;
  buf->f_ffree = fsbuf.f_ffree;
  buf->f_fsid = fsbuf.f_fsid;
  buf->f_namemax = fsbuf.f_namelen;
  memset (buf->__f_spare, '\0', 6 * sizeof (int));
  buf->f_favail = buf->f_ffree;
  buf->f_flag = 0;	/* No way of getting this information from Syllable */

  return 0;
}
libc_hidden_def (fstatvfs)

