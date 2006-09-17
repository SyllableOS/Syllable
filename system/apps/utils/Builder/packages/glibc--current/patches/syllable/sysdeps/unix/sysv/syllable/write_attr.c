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

#include <errno.h>
#include <sysdep.h>
#include <sys/syscall.h>

#include <atheos/filesystem.h>

ssize_t __write_attr( int fd, const char *name, int flags, int type, const void *buf, off_t pos, size_t len )
{
  WriteAttrParams_s params;

  params.wa_nFile = fd;
  params.wa_pzName = name;
  params.wa_nFlags = flags;
  params.wa_nType = type;
  params.wa_pBuffer = buf;
  params.wa_nPos = pos;
  params.wa_nLen = len;

  return INLINE_SYSCALL(write_attr, 1, &params);
}
weak_alias(__write_attr,write_attr)

