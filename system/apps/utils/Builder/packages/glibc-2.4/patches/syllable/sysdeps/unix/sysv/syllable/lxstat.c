/* lxstat using old-style Unix lstat system call.
   Copyright (C) 1991,1995-1998,2000,2002,2003 Free Software Foundation, Inc.
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
#include <sys/stat.h>
#include <sys/debug.h>

#include <sysdep.h>
#include <sys/syscall.h>

/* Get information about the file NAME in BUF.  */
int
__lxstat (int vers, const char *name, struct stat *buf)
{
  if (vers != _STAT_VER_KERNEL)
    dbprintf("Warning: %s wrong version.  Got %i but expected %i.\n", __FUNCTION__, vers, _STAT_VER_KERNEL);

  return INLINE_SYSCALL (lstat, 2, name, buf);
}

hidden_def (__lxstat)
weak_alias (__lxstat, _lxstat);

