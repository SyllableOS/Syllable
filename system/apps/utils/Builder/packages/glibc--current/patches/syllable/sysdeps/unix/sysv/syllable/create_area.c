/* Copyright (C) 1991,1995-1998,2000,2002,2003 Free Software Foundation, Inc.
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
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sysdep.h>
#include <sys/syscall.h>

#include <atheos/areas.h>

/* create_area can not simply be included in syscalls.list as it causes the error
   "invalid symbol "-" in mnenomic" to be thrown.  I suspect it's caused by a macro
   expansion or regexp munging triggered by the syscall name.  Doing it the long way
   'round here is safe as no munging is performed. */

area_id	__create_area ( const char* name, void** addr, size_t size, flags_t protection, flags_t lock )
{
  return INLINE_SYSCALL (create_area, 5, name, addr, size, protection, lock);
}
weak_alias (__create_area, create_area)

