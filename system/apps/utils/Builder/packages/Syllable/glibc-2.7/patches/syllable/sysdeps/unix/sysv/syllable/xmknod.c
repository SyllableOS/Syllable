/* xmknod call using old-style Unix mknod system call.
   Copyright (C) 1991, 1993, 1995, 1996, 1997, 1998, 2000, 2002, 2003
   Free Software Foundation, Inc.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include <sysdep.h>
#include <sys/syscall.h>

/* Create a device file named PATH, with permission and special bits MODE
   and device number DEV (which can be constructed from major and minor
   device numbers with the `makedev' macro above).  */
int
__xmknod (int vers, const char *path, mode_t mode, dev_t *dev)
{
  return INLINE_SYSCALL (mknod, 3, path, mode, *dev);
}

weak_alias (__xmknod, _xmknod)
libc_hidden_def (__xmknod)


