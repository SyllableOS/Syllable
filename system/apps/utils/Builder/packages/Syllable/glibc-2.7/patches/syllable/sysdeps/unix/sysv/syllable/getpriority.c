/* Copyright (C) 1991,95,96,97,2000,02 Free Software Foundation, Inc.
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
#include <atheos/threads.h>

/* Return the highest priority of any process specified by WHICH and WHO
   (see <sys/resource.h>); if WHO is zero, the current process, process group,
   or user (as specified by WHO) is used.  A lower priority number means higher
   priority.  Priorities range from PRIO_MIN to PRIO_MAX.  */
int
getpriority (which, who)
     enum __priority_which which;
     id_t who;
{
  thread_id tid;
  thread_info info;
  int prio;

  /* Syllable has no concept of process group or user priorities */
  if ( which != PRIO_PROCESS )
  {
    __set_errno (ENOSYS);
    return -1;
  }

  if ( who > 0 )
    tid = who;
  else
    tid = -1;  /* Current thread */

  if ( get_thread_info (tid, &info) != EOK )
  {
    __set_errno (ESRCH);
    return -1;
  }

  /* Adjust the scale and invert the priority value */
  prio = info.ti_priority;
  prio += 99;
  prio = ~prio;
  prio += 250;

  return prio;
}
libc_hidden_def (getpriority)

