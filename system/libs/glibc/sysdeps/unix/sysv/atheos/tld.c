/* Thread termination API
   
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
#include <malloc.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <atheos/threads.h>
#include <atheos/tld.h>
#include <bits/libc-lock.h>

static __libc_lock_t g_hDesctructorListMutex = MUTEX_INITIALIZER;

typedef void tld_destructor( void* data, int tld );

typedef struct tld_destructor_node tld_destructor_node;

struct tld_destructor_node
{
    tld_destructor_node* next;
    tld_destructor*	 destructor;
    int 		 tld;
};

static tld_destructor_node* g_psFirstDestructor = NULL;


int alloc_tld( void* destructor )
{
    tld_destructor_node* destr_node = NULL;
    int tld;
    
    if ( destructor != NULL ) {
	destr_node = malloc( sizeof( tld_destructor_node ) );
	if ( destr_node == NULL ) {
	    return( -1 );
	}
    }
    tld = INLINE_SYSCALL( alloc_tld, 0 );
    if ( tld < 0 ) {
	if ( destr_node != NULL ) {
	    free( destr_node );
	}
	return( tld );
    }
    if ( destr_node != NULL ) {
	int error;
	while( (error = __lock_lock(&g_hDesctructorListMutex) ) < 0 && errno == EINTR );
	if ( error < 0 ) {
	    free_tld( tld );
	    free( destr_node );
	    return( error );
	}
	destr_node->next = g_psFirstDestructor;
	destr_node->destructor = destructor;
	destr_node->tld = tld;
	g_psFirstDestructor = destr_node;
	__lock_unlock( &g_hDesctructorListMutex );
    }
    if ( tld >= 0 ) {
	set_tld( tld, NULL );
    }
    return( tld );
}


int free_tld( int tld )
{
    if ( g_psFirstDestructor != NULL ) {
	int error;
	while( (error = __lock_lock(&g_hDesctructorListMutex) ) < 0 && errno == EINTR );
	if ( error >= 0 ) {
	    tld_destructor_node** tmp;
	    
	    for ( tmp = &g_psFirstDestructor ; *tmp != NULL ; tmp = &(*tmp)->next ) {
		tld_destructor_node* node = *tmp;
		if ( node->tld == tld ) {
		    *tmp = node->next;
		    free( node );
		    break;
		}
	    }
	}
	__lock_unlock( &g_hDesctructorListMutex );
    }
    return( INLINE_SYSCALL( alloc_tld, 1, tld ) );
}

void set_tld( int handle, void* value )
{
    __asm__("movl %0,%%gs:(%1)" : : "r" ((int)value), "r" (handle) );    
}

void* get_tld( int handle )
{
    register int value;
    __asm__("movl %%gs:(%1),%0" : "=r" (value) : "r" (handle) );
    return( (void*)value );
}

void __libc_destruct_tlds( void )
{
    int error;
    while( (error = __lock_lock(&g_hDesctructorListMutex) ) < 0 && errno == EINTR );
    if ( error >= 0 ) {
	tld_destructor_node* node;

	for ( node = g_psFirstDestructor ; node != NULL ; node = node->next ) {
	    node->destructor( get_tld( node->tld ), node->tld );
	}
    }
    __lock_unlock( &g_hDesctructorListMutex );
}
