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
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <atheos/threads.h>

/* KV: This is a stub, until we add a proper getrlimit/setrlimit pair
   of syscalls.  The values returned from this version should at least
   keep programs that rely on getrlimit() happy. */

/* Put the soft and hard limits for RESOURCE in *RLIMITS.
   Returns 0 if successful, -1 if not (and sets errno).  */
int
__getrlimit (enum __rlimit_resource resource, struct rlimit *rlimits)
{
  int error = 0;

  if (rlimits == NULL)
  {
    __set_errno (EINVAL);
    error = -1;
  }
  else
  {
    switch ( resource )
    {
      case RLIMIT_CPU:
      {
        rlimits->rlim_cur = rlimits->rlim_max = RLIM_INFINITY;
        break;
      }

      case RLIMIT_FSIZE:
      {
        rlimits->rlim_cur = rlimits->rlim_max = RLIM_INFINITY;
        break;
      }

      case RLIMIT_DATA:
      {
        rlimits->rlim_cur = rlimits->rlim_max = RLIM_INFINITY;
        break;
      }

      case RLIMIT_STACK:
      {
        thread_id thread;
        thread_info info;

        thread = get_thread_id(NULL);
        if (get_thread_info( thread, &info ) == 0)
        {
          rlimits->rlim_cur = rlimits->rlim_max = info.ti_stack_size;
        }
        else
        {
          __set_errno (ESRCH);
          error = -1;
        }

        break;
      }

      case RLIMIT_CORE:
      {
        rlimits->rlim_cur = rlimits->rlim_max = 0;
        break;
      }

      case RLIMIT_NOFILE:
      {
        rlimits->rlim_cur = rlimits->rlim_max = 256;
        break;
      }

      case RLIMIT_AS:
      {
        rlimits->rlim_cur = rlimits->rlim_max = RLIM_INFINITY;
        break;
      }

      default:
      {
        __set_errno (ENOSYS);
        error = -1;
      }
    }
  }

  return error;
}
weak_alias (__getrlimit, getrlimit)

