/* Spawn thread API
   
   Copyright (C) 2001 Kurt Skauen
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <errno.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <atheos/threads.h>

typedef int thread_entry( void* arg );

struct entry_params
{
    thread_entry* entry;
    void*	  arg;
};

static int thread_start( struct entry_params* param )
{
    thread_entry* entry = param->entry;
    void* 	  arg = param->arg;
    free( param );
    exit_thread( entry( arg ) );
    return( 1 ); /* Should never happen */
}


thread_id __spawn_thread( const char* name, void* const entry,
			  const int pri, int stack_size, void* arg )
{
    struct entry_params* params;
    thread_id thid;

    params = malloc( sizeof(struct entry_params) );
    if ( params == NULL ) {
	return( -1 );
    }
    params->entry = entry;
    params->arg   = arg;
    
    thid = INLINE_SYSCALL( spawn_thread, 5, name, thread_start, pri, stack_size, params );
    if ( thid < 0 ) {
	free( params );
    }
    return( thid );
}

weak_alias (__spawn_thread, spawn_thread)
