/* clock_getres -- Get the resolution of a POSIX clockid_t.
   Copyright (C) 1999, 2000, 2001, 2003, 2004 Free Software Foundation, Inc.
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
#include <stdint.h>
#include <time.h>
#include <unistd.h>

/* Get resolution of clock.  */
int
clock_getres (clockid_t clock_id, struct timespec *res)
{
  int retval = -1;

  switch (clock_id)
  {
    case CLOCK_REALTIME:
    case CLOCK_MONOTONIC:
    {
      res->tv_sec = 0;
      res->tv_nsec = 1000000;

      retval = 0;
      break;
    }

    default:
    {
      __set_errno (EINVAL);
      break;
    }
  }

  return retval;
}
