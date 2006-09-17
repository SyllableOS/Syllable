/* Copyright (C) 1996, 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
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

#include <atheos/semaphore.h>
#include <atheos/threads.h>

thread_id __get_semaphore_holder( sem_id sem )
{
  sem_info info;
  int error;

  error = get_semaphore_info( sem, get_process_id( NULL ), &info );
  if( error < 0 )
    return (thread_id)error;

  return info.si_latest_holder;
}
weak_alias(__get_semaphore_holder,get_semaphore_holder)

