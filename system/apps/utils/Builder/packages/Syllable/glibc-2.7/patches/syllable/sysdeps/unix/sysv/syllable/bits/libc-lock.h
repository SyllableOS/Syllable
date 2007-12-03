/* libc-internal interface for mutex locks.  Syllable version.
   Copyright (C) 1996,97,99,2000,01,02 Free Software Foundation, Inc.
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

#ifndef _BITS_LIBC_LOCK_H
#define _BITS_LIBC_LOCK_H 1

#include <errno.h>
#include <pthread.h>
#include <sys/debug.h>
#include <atheos/semaphore.h>
#include <atheos/atomic.h>

/* KV: Testing for thread-m.h  This is temporary!*/
#define __register_atfork(prepare, parent, child, __dso_handle) do{}while(0)

#define MUTEX_INITIALIZER { -1, ATOMIC_INIT(0), ATOMIC_INIT(0) }
#define _LIBC_LOCK_RECURSIVE_INITIALIZER MUTEX_INITIALIZER

typedef struct __libc_lock__ __libc_lock_t;

struct __libc_lock__
{
	sem_id hMutex;
	atomic_t nLockCount;
	atomic_t nIsInited;
};

/* Syllable specific locking functions */
void __lock_fini( __libc_lock_t* lock );
int __lock_lock( __libc_lock_t* lock );
int __lock_lock_recursive( __libc_lock_t* lock );
void __lock_unlock( __libc_lock_t* lock );

/* Define a lock variable NAME with storage class CLASS.  The lock must be
   initialized with __libc_lock_init before it can be used (or define it
   with __libc_lock_define_initialized, below).  Use `extern' for CLASS to
   declare a lock defined in another module.  In public structure
   definitions you must use a pointer to the lock structure (i.e., NAME
   begins with a `*'), because its storage size will not be known outside
   of libc.  */
#define __libc_lock_define(CLASS,NAME) \
	CLASS __libc_lock_t NAME;
#define __libc_lock_define_recursive(CLASS,NAME) \
	CLASS __libc_lock_t NAME;

#define __rtld_lock_define_recursive(CLASS,NAME)
#define _RTLD_LOCK_RECURSIVE_INITIALIZER \
  _LIBC_LOCK_RECURSIVE_INITIALIZER
#define __libc_rwlock_define(CLASS,NAME)

/* Define an initialized lock variable NAME with storage class CLASS.  */
#define __libc_lock_define_initialized(CLASS,NAME) \
	CLASS __libc_lock_t NAME = MUTEX_INITIALIZER;

#define __libc_rwlock_define_initialized(CLASS,NAME)

/* Define an initialized recursive lock variable NAME with storage
   class CLASS.  */
#define __libc_lock_define_initialized_recursive(CLASS,NAME) \
	CLASS __libc_lock_t NAME = MUTEX_INITIALIZER;

/* Initialize the named lock variable, leaving it in a consistent, unlocked
   state.  */
#define __libc_lock_init(NAME) \
	({ (NAME).hMutex = -1; atomic_set(&((NAME).nLockCount),0); atomic_set(&((NAME).nIsInited),0); })
#define __libc_rwlock_init(NAME)

/* Same as last but this time we initialize a recursive mutex.  */
#define __libc_lock_init_recursive(NAME) \
	({ (NAME).hMutex = -1; atomic_set(&((NAME).nLockCount),0); atomic_set(&((NAME).nIsInited),0); })
#define __rtld_lock_init_recursive(NAME)

/* Finalize the named lock variable, which must be locked.  It cannot be
   used again until __libc_lock_init is called again on it.  This must be
   called on a lock variable before the containing storage is reused.  */
#define __libc_lock_fini(NAME) __lock_fini( &NAME );
#define __libc_rwlock_fini(NAME)

/* Finalize recursive named lock.  */
#define __libc_lock_fini_recursive(NAME) __libc_lock_fini(NAME)

/* Lock the named lock variable.  */
#define __libc_lock_lock(NAME) __lock_lock( &NAME )
#define __libc_rwlock_rdlock(NAME)
#define __libc_rwlock_wrlock(NAME)

/* Lock the recursive named lock variable.  */
#define __libc_lock_lock_recursive(NAME) __lock_lock_recursive( &NAME )
#define __rtld_lock_lock_recursive(NAME)

/* Try to lock the named lock variable.  */
#define __libc_lock_trylock(NAME) __lock_lock( &NAME )
#define __libc_rwlock_tryrdlock(NAME) 0
#define __libc_rwlock_trywrlock(NAME) 0

/* Try to lock the recursive named lock variable.  */
#define __libc_lock_trylock_recursive(NAME) __lock_lock_recursive( &NAME )

/* Unlock the named lock variable.  */
#define __libc_lock_unlock(NAME) __lock_unlock( &NAME )
#define __libc_rwlock_unlock(NAME)

/* Unlock the recursive named lock variable.  */
#define __libc_lock_unlock_recursive(NAME) __libc_lock_unlock(NAME)
#define __rtld_lock_unlock_recursive(NAME)

typedef atomic_t __libc_once_t;

/* Define once control variable.  */
#define __libc_once_define(CLASS, NAME) \
	CLASS __libc_once_t NAME = ATOMIC_INIT(0)

/* Call handler if the first call.  */
#define __libc_once(ONCE_CONTROL, INIT_FUNCTION)	\
  do {							\
    if ( atomic_swap( &(ONCE_CONTROL), 1 ) == 0 )	\
      (INIT_FUNCTION) ();				\
  } while(0)

/* Get the scaler value of a __libc_once type */
#define __libc_once_read(ONCE_CONTROL) \
	atomic_read( &(ONCE_CONTROL) )

/* Start a critical region with a cleanup function */
#define __libc_cleanup_region_start(DOIT, FCT, ARG)			    \
{									    \
  typeof (***(FCT)) *__save_FCT = (DOIT) ? (FCT) : 0;			    \
  typeof (ARG) __save_ARG = ARG;					    \
  /* close brace is in __libc_cleanup_region_end below. */

/* End a critical region started with __libc_cleanup_region_start. */
#define __libc_cleanup_region_end(DOIT)					    \
  if ((DOIT) && __save_FCT != 0)					    \
    (*__save_FCT)(__save_ARG);						    \
}

/* Sometimes we have to exit the block in the middle.  */
#define __libc_cleanup_end(DOIT)					    \
  if ((DOIT) && __save_FCT != 0)					    \
    (*__save_FCT)(__save_ARG);						    \

#define __libc_cleanup_push(fct, arg) __libc_cleanup_region_start (1, fct, arg)
#define __libc_cleanup_pop(execute) __libc_cleanup_region_end (execute)

/* We need portable names for some of the functions.  */
#define __libc_mutex_unlock(NAME) __lock_unlock( &NAME )

/* Type for key of thread specific data.  */
typedef int __libc_key_t;

/* Create key for thread specific data.  */
int __libc_key_create( __libc_key_t* key, void* destr );
/* Set thread-specific data associated with KEY to VAL.  */
int __libc_setspecific( __libc_key_t key, void* data );
/* Get thread-specific data associated with KEY.  */
void* __libc_getspecific( __libc_key_t key );

#endif	/* bits/libc-lock.h */

