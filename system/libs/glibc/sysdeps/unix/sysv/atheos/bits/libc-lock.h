/* libc-internal interface for mutex locks.  Stub version.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
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

#ifndef _BITS_LIBC_LOCK_H
#define _BITS_LIBC_LOCK_H 1

#include <atheos/semaphore.h>
#include <atheos/atomic.h>
#include <errno.h>
#include <sys/debug.h>


#define MUTEX_INITIALIZER { -1, 0, 0 }

typedef struct __libc_lock__ __libc_lock_t;

struct __libc_lock__
{
    sem_id	 hMutex;
    atomic_t	 nLock;
    atomic_t	 nIsInited;
};


void	__libc_lock_atfork( void );
void	__lock_fini( __libc_lock_t* lock );
int	__lock_lock( __libc_lock_t* lock );
int	__lock_lock_recursive( __libc_lock_t* lock );
int	__lock_trylock( __libc_lock_t* lock );
void	__lock_unlock( __libc_lock_t* lock );



/* Define a lock variable NAME with storage class CLASS.  The lock must be
   initialized with __libc_lock_init before it can be used (or define it
   with __libc_lock_define_initialized, below).  Use `extern' for CLASS to
   declare a lock defined in another module.  In public structure
   definitions you must use a pointer to the lock structure (i.e., NAME
   begins with a `*'), because its storage size will not be known outside
   of libc.  */

#define __libc_lock_define(CLASS,NAME) \
  CLASS __libc_lock_t NAME;

/* Define an initialized lock variable NAME with storage class CLASS.  */
#define __libc_lock_define_initialized(CLASS,NAME) \
  CLASS __libc_lock_t NAME = MUTEX_INITIALIZER;

/* Define an initialized recursive lock variable NAME with storage
   class CLASS.  */
#define __libc_lock_define_initialized_recursive(CLASS,NAME) \
  CLASS __libc_lock_t NAME = MUTEX_INITIALIZER;

/* Initialize the named lock variable, leaving it in a consistent, unlocked
   state.  */

#define __libc_lock_init(NAME) ({ (NAME).hMutex = -1; (NAME).nLock = 0; (NAME).nIsInited = 0; })

/* Same as last but this time we initialize a recursive mutex.  */

#define __libc_lock_init_recursive(NAME)	\
   do { (NAME).hMutex = -1; (NAME).nLock = 0; (NAME).nIsInited = 0; } while(0)

/* Finalize the named lock variable, which must be locked.  It cannot be
   used again until __libc_lock_init is called again on it.  This must be
   called on a lock variable before the containing storage is reused.  */
#define __libc_lock_fini(NAME) __lock_fini( &NAME )

/* Finalize recursive named lock.  */
#define __libc_lock_fini_recursive __libc_lock_fini


/* Lock the named lock variable.  */
#define __libc_lock_lock(NAME) __lock_lock( &NAME )

/* Lock the recursive named lock variable.  */
#define __libc_lock_lock_recursive(NAME) __lock_lock_recursive( &NAME )

/* Try to lock the named lock variable.  */
#define __libc_lock_trylock(NAME) __lock_trylock(&NAME)

/* Try to lock the recursive named lock variable.  */
//#define __libc_lock_trylock_recursive(NAME) 0

/* Unlock the named lock variable.  */
#define __libc_lock_unlock(NAME) __lock_unlock( &(NAME) )

/* Unlock the recursive named lock variable.  */
#define __libc_lock_unlock_recursive(NAME) __lock_unlock( &(NAME) )

typedef atomic_t __libc_once_t;

#define __libc_once_define(CLASS,NAME) \
  CLASS __libc_once_t NAME = 0

/* Call handler if the first call.  */

#define __libc_once(ONCE_CONTROL, INIT_FUNCTION)	\
  do {							\
	if ( atomic_swap( &(ONCE_CONTROL), 1 ) == 0 ) { \
	    (INIT_FUNCTION) ();				\
	}						\
  } while(0)

#define __libc_cleanup_region_start(FCT, ARG)				    \
{									    \
  typeof (FCT) __save_FCT = FCT;					    \
  typeof (ARG) __save_ARG = ARG;					    \
  /* close brace is in __libc_cleanup_region_end below. */

/* End a critical region started with __libc_cleanup_region_start. */
#define __libc_cleanup_region_end(DOIT)					    \
  if (DOIT)								    \
    (*__save_FCT)(__save_ARG);						    \
}

/* Used in stdio-common/vfscanf.c but was not defined, looks like a bug but I'm not sure (KHS) */
#define __libc_cleanup_end(a) /*__libc_cleanup_region_end*/

/* We need portable names for some of the functions.  */
static inline void __libc_mutex_unlock( __libc_lock_t* lock )
{
  __lock_unlock( lock );
}

/* Type for key of thread specific data.  */
typedef int __libc_key_t;

int   __libc_key_create( __libc_key_t* key, void* destr );
int   __libc_setspecific( __libc_key_t key, void* data );
void* __libc_getspecific( __libc_key_t key );

#endif	/* bits/libc-lock.h */
