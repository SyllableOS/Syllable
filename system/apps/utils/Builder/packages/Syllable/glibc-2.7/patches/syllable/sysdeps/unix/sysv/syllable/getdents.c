/* Copyright (C) 1993, 1995-2002 Free Software Foundation, Inc.
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
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>

#include <sysdep.h>
#include <sys/syscall.h>

extern int __syscall_getdents (int fd, char *__unbounded buf, unsigned int nbytes);

#ifndef __GETDENTS
# define __GETDENTS __getdents
#endif

/* The problem here is that we cannot simply read the next NBYTES
   bytes.  We need to take the additional field into account.  We use
   some heuristic.  Assuming the directory contains names with 14
   characters on average we can compute an estimated number of entries
   which fit in the buffer.  Taking this number allows us to specify a
   reasonable number of bytes to read.  If we should be wrong, we can
   reset the file descriptor.  In practice the kernel is limiting the
   amount of data returned much more then the reduced buffer size.  */
ssize_t
internal_function
__GETDENTS (int fd, char *buf, size_t nbytes)
{
  int error;
  struct kernel_dirent kd;
  struct dirent *dp = (struct dirent*)buf;

  error = INLINE_SYSCALL(getdents, 3, fd, (char*)&kd, sizeof(kd));
  if(error < 0)
    return(error);

  /* Copy kernel dirent to libc dirent */
  dp->d_ino = kd.d_ino;
  dp->d_off = 0;
  dp->d_namlen = kd.d_namlen;
  dp->d_reclen = sizeof(*dp);
  dp->d_type = DT_UNKNOWN;
  memcpy(dp->d_name, kd.d_name, kd.d_namlen);
  dp->d_name[kd.d_namlen] = '\0';

  return(error * sizeof(struct dirent));
}
