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

#include <errno.h>
#include <stddef.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>

/* atheos/kernel.h defines a get_system_info macro wrapping get_system_info_v() with the current SYS_INFO_VERSION. */
#undef get_system_info

/* The get_system_info() function is deprecated because it does not properly handle the get_system_info syscall versioning.
   It is included for compatibility with existing binaries, which assume SYS_INFO_VERSION == 3.
   New apps should use get_system_info_v() or the get_system_info() macro.
*/
status_t __get_system_info( system_info* info )
{
    dbprintf( "Deprecated get_system_info() [version 3] was used!\n" );
    return( INLINE_SYSCALL(get_system_info, 2, info, 3 ) );  /* assuming SYS_INFO_VERSION == 3 */
}

weak_alias (__get_system_info, get_system_info)
