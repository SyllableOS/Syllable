/* Copyright (C) 1991, 1995, 1996, 1997 Free Software Foundation, Inc.
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
#include <sys/resource.h>
#include <sys/types.h>
#include <atheos/threads.h>

/* KV: This is a stub, until we add a proper getrusage syscall.
   The values returned from this version should at least keep
   programs that rely on getrusage( RUSAGE_SELF ) happy. */

/* Return resource usage information on process indicated by WHO
   and put it in *USAGE.  Returns 0 for success, -1 for failure.  */
int
__getrusage (who, usage)
     enum __rusage_who who;
     struct rusage *usage;
{
  int error = -1;

  if (usage == NULL)
    __set_errno (EINVAL);
  else if (who != RUSAGE_SELF)
    __set_errno (ENOSYS);
  else
  {
    thread_id thread;
    thread_info info;

    thread = get_thread_id(NULL);
    if (get_thread_info( thread, &info ) == 0)
    {
      usage->ru_utime.tv_sec = info.ti_user_time / 100000;
      usage->ru_utime.tv_usec = info.ti_user_time % usage->ru_utime.tv_sec;

      usage->ru_stime.tv_sec = info.ti_sys_time / 100000;
      usage->ru_stime.tv_usec = info.ti_sys_time % usage->ru_stime.tv_sec;

      error = 0;
    }
    else
      __set_errno (ESRCH);
  }

  return error;
}
weak_alias (__getrusage, getrusage)

