/* Threads compatibily routines for libgcc2.  */
/* Compile this one with gcc.  */
/* Copyright (C) 1997 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* As a special exception, if you link this library with other files,
   some of which are compiled with GCC, to produce an executable,
   this library does not by itself cause the resulting executable
   to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  */

#ifndef __gthr_syllable_h
#define __gthr_syllable_h

/* Syllable threads implementation */
#define __GTHREADS 1

#include <errno.h>
#include <atheos/semaphore.h>
#include <atheos/atomic.h>
#include <atheos/tld.h>

typedef int __gthread_key_t;
typedef atomic_t __gthread_once_t;
typedef struct
{
    atomic_t count;
    sem_id   mutex;
} __gthread_mutex_t;

#define __GTHREAD_ONCE_INIT 0
#define __GTHREAD_MUTEX_INIT_FUNCTION(lock) do { (lock)->count = 0; (lock)->mutex = create_semaphore( "gcc_mutex", 0, 0 ); } while(0)

static inline int
__gthread_active_p ()
{
  return 1;
}


static inline int
__gthread_once (__gthread_once_t *once, void (*func) ())
{
    if (__gthread_active_p ()) {
	if ( atomic_swap( once, 1 ) == 0 ) {
	    func();
	}
	return( 0 );
    } else {
	return -1;
    }
}

static inline int
__gthread_key_create (__gthread_key_t *key, void (*dtor) (void *))
{
    int _key = alloc_tld( (void*)dtor );
    if ( _key >= 0 ) {
	*key = _key;
	return( 0 );
    } else {
	return( -1 );
    }
}

static inline int
__gthread_key_dtor (__gthread_key_t key, void *ptr)
{
  /* Just reset the key value to zero. */
    if (ptr) {
	set_tld(key, 0);
	return 0;
    } else {
	return 0;
    }
}

static inline int
__gthread_key_delete (__gthread_key_t key)
{
  return free_tld(key);
}

static inline void *
__gthread_getspecific (__gthread_key_t key)
{
  return get_tld (key);
}

static inline int
__gthread_setspecific (__gthread_key_t key, const void *ptr)
{
  set_tld (key, (void*)ptr);
  return 0;
}

static inline int
__gthread_mutex_lock (__gthread_mutex_t *mutex)
{
    if (__gthread_active_p ()) {
	atomic_t old = atomic_add( &mutex->count, 1 );
	if ( old > 0 ) {
	    for (;;) {
		int error = lock_semaphore( mutex->mutex );
		if ( error >= 0 || errno != EINTR ) {
		    return error;
		}
	    }
	} else {
	    return 0;
	}
    } else {
	return 0;
    }
}

static inline int
__gthread_mutex_trylock (__gthread_mutex_t *mutex)
{
    if (__gthread_active_p ()) {
	atomic_t old = atomic_add( &mutex->count, 1 );
	if (old > 0) {
	    atomic_add( &mutex->count, -1 );
	    unlock_semaphore_x( mutex->mutex, 0, 0 );
	    errno = EWOULDBLOCK;
	    return -1;
	} else {
	    return 0;
	}
    } else {
	return 0;
    }
}

static inline int
__gthread_mutex_unlock (__gthread_mutex_t *mutex)
{
    if (__gthread_active_p ()) {
	atomic_t old = atomic_add( &mutex->count, -1 );
	if ( old > 1) {
	    unlock_semaphore( mutex->mutex );
	}
	return 0;
    } else {
	return 0;
    }
}

#endif /* not __gthr_syllable_h */





