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
#include <time.h>
#include <sys/time.h>

#include <atheos/time.h>

#define MAX_SEC	1000000L
#define MIN_SEC	-1000000L

/* Adjust the current time of day by the amount in DELTA.
   If OLDDELTA is not NULL, it is filled in with the amount
   of time adjustment remaining to be done from the last `__adjtime' call.
   This call is restricted to the super-user.  */
int
__adjtime (delta, olddelta)
     const struct timeval *delta;
     struct timeval *olddelta;
{
  bigtime_t time, adj;

  if (delta->tv_usec > MAX_SEC || delta->tv_usec < MIN_SEC)
	{
	  __set_errno (EINVAL);
	  return -1;
	}

  time = get_real_time();
  adj = (delta->tv_sec * 1000000LL) + delta->tv_usec;

  time += adj;

  if (set_real_time(time) < 0)
    return -1;

  if (olddelta)
    olddelta->tv_sec = olddelta->tv_usec = 0;

  return 0;
}
weak_alias (__adjtime, adjtime)

