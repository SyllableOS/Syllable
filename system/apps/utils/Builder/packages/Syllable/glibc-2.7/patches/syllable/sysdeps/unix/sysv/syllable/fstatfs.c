/* Return information about the filesystem on which FD resides.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
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
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <malloc.h>
#include <sys/statfs.h>
#include <atheos/filesystem.h>

/* Return information about the filesystem on which FD resides.  */
int
__fstatfs (int fd, struct statfs *buf)
{
  fs_info info;

  if( get_fs_info( fd, &info ) < 0 )
  {
    __set_errno(ENXIO);
    return -1;
  }

  buf->f_type = 0;	/* No data available from get_fs_info() */
  buf->f_bsize = info.fi_block_size;
  buf->f_blocks = info.fi_total_blocks;
  buf->f_bfree = info.fi_free_blocks;
  buf->f_bavail = info.fi_free_user_blocks;
  buf->f_files = info.fi_total_inodes;
  buf->f_ffree = info.fi_free_inodes;
  buf->f_fsid = 0;	/* No data available from get_fs_info() */
  buf->f_namelen = _POSIX_NAME_MAX;	/* No data available from get_fs_info() */
  memset( buf->f_spare, 0, sizeof( buf->f_spare));

  return 0;
}
weak_alias (__fstatfs, fstatfs)


