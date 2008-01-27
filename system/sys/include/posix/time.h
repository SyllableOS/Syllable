/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __F_POSIX_TIME_H__
#define __F_POSIX_TIME_H__

#include <posix/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCKS_PER_SEC	1000000

#if !defined __timespec_defined
/* POSIX.1b structure for a time value.  This is like a `struct timeval' but
   has nanoseconds instead of microseconds.  */
struct timespec
{
	time_t tv_sec;		/* Seconds.  */
	long int tv_nsec;	/* Nanoseconds.  */
};
# define __timespec_defined
#endif

struct kernel_timeval
{
  time_t tv_sec;	/* Seconds.  */
  time_t tv_usec;	/* Microseconds.  */
};

struct kernel_itimerval
{
  struct kernel_timeval it_interval;
  struct kernel_timeval it_value;
};

#ifdef __KERNEL__

#define ITIMER_REAL		0
#define ITIMER_VIRTUAL	1
#define ITIMER_PROF		2

struct tm {
  int 	tm_sec;
  int 	tm_min;
  int 	tm_hour;
  int 	tm_mday;
  int 	tm_mon;
  int 	tm_year;
  int 	tm_wday;
  int 	tm_yday;
  int 	tm_isdst;
  char* tm_zone;
  int 	tm_gmtoff;
};

#endif

#ifdef __cplusplus
}
#endif

#endif /* __F_POSIX_TIME_H__ */
