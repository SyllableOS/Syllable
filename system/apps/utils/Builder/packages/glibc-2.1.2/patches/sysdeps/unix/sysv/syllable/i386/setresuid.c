/* Copyright (C) 1998 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <sys/types.h>

//#include </posix_types.h>

#include <sysdep.h>
#include <sys/syscall.h>

//#ifdef __NR_setresuid

extern int __syscall_setresuid( uid_t rgid, uid_t egid,
				uid_t sgid);

int __setresuid (uid_t ruid, uid_t euid, uid_t suid)
{
  return INLINE_SYSCALL (setresuid, 3, ruid, euid, suid);
}
weak_alias (__setresuid, setresuid)
//#endif
