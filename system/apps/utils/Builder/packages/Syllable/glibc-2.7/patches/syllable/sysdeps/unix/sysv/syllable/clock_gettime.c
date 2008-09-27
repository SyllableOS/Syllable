/* clock_gettime -- Get the current time from a POSIX clockid_t.  Unix version.
   Copyright (C) 1999-2004, 2005, 2007 Free Software Foundation, Inc.
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
#include <sys/time.h>
#include <sys/systime.h>

/* Get current value of CLOCK and store it in TP.  */
int
clock_gettime (clockid_t clock_id, struct timespec *tp)
{
  int retval = -1;

  switch (clock_id)
  {
    case CLOCK_REALTIME:
	{
      struct timeval tv;
      retval = gettimeofday (&tv, NULL);
      if (retval == 0)
        TIMEVAL_TO_TIMESPEC (&tv, tp);
      break;
    }

    case CLOCK_MONOTONIC:
    {
      bigtime_t systime;
      struct timeval tv;

      systime = get_system_time();

      tv.tv_sec = (long int) (systime / 1000000);
      tv.tv_usec = (long int) (systime % 1000000);

      TIMEVAL_TO_TIMESPEC (&tv, tp);
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
librt_hidden_def (clock_gettime)
