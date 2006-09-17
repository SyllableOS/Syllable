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

#include <unistd.h>
#include <sysdep.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <bits/libc-lock.h>
#include <atheos/threads.h>
#include <atheos/tld.h>

int __fork( void )
{
	int nResult;

	nResult = INLINE_SYSCALL( Fork, 1, NULL );
	if( 0 == nResult )
	{
		thread_id nThid;
		proc_id nPrid;

		nThid = INLINE_SYSCALL( get_thread_id, 1, NULL );
		nPrid = INLINE_SYSCALL( get_process_id, 1, NULL );
		__asm__( "movl %0,%%gs:(%1)" : : "r" (nThid), "r" (TLD_THID) );
		__asm__( "movl %0,%%gs:(%1)" : : "r" (nPrid), "r" (TLD_PRID) );
	}

	return( nResult );
}
libc_hidden_def(__fork);
weak_alias( __fork, fork );
