/* POSIX threads for Syllable - A simple PThreads implementation             */
/*                                                                           */
/* Copyright (C) 2002 Kristian Van Der Vliet (vanders@users.sourceforge.net) */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)                   */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU Library General Public License       */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU Library General Public License for more details.                      */

#ifndef __F_SYLLABLE_PTHREAD_H_
#define __F_SYLLABLE_PTHREAD_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <atheos/types.h>
#include <atheos/tld.h>

#include <sched.h>
#include <time.h>

/* The following definitions are required by the XSI/Open Group specification
   although we currently do not guarantee that we support the entire API, nor
   do we guarentee that the parts of the API that we do support are 100%
   compliant. */
#define	_POSIX_THREADS
#define	_POSIX_THREAD_PROCESS_SHARED

/* The stack options are only half-implemented: the stack address and stack
   size can be retrieved but neither of them may be changed. Because of
   this we do not define the following options:

   _POSIX_THREAD_ATTR_STACKADDR
   _POSIX_THREAD_ATTR_STACKSIZE
*/

/* We DO NOT support the following, and the associated functions will return
   ENOSYS when called.  Well behaved code should be checking for the presence 
   (Or lack thereof) of these defines!

   _POSIX_THREAD_PRIORITY_SCHEDULING
   _POSIX_THREAD_PRIO_PROTECT
*/

#define PTHREAD_STACK_MIN	1024 * 128
#define PTHREAD_KEYS_MAX	TLD_SIZE

/* Attribute definitions */
/* Cancellation */

enum
{
	PTHREAD_CANCEL_ENABLE,
	PTHREAD_CANCEL_DISABLE
};

enum
{
	PTHREAD_CANCEL_DEFERRED,
	PTHREAD_CANCEL_ASYNCHRONOUS
};

#define PTHREAD_CANCELED	((void *) -1)

/* Scheduling */

enum
{
	PTHREAD_CREATE_DETACHED,
	PTHREAD_CREATE_JOINABLE
};

enum
{
	PTHREAD_EXPLICIT_SCHED,
	PTHREAD_INHERIT_SCHED
};

#define	PTHREAD_ONCE_INIT	{ 0 }

enum
{
	PTHREAD_PROCESS_SHARED,
	PTHREAD_PROCESS_PRIVATE
};

enum
{
	PTHREAD_SCOPE_PROCESS,
	PTHREAD_SCOPE_SYSTEM
};

/* Mutexes */

enum
{
	PTHREAD_PRIO_INHERIT,
	PTHREAD_PRIO_NONE,
	PTHREAD_PRIO_PROTECT
};

enum
{
	PTHREAD_MUTEX_DEFAULT,
	PTHREAD_MUTEX_ERRORCHECK,
	PTHREAD_MUTEX_NORMAL,
	PTHREAD_MUTEX_RECURSIVE
};

/* Mutex initiliasers */
#define	PTHREAD_MUTEX_INITIALIZER	\
	{ 0, 0, 0, NULL }

#define	PTHREAD_COND_INITIALIZER		\
	{ 0, 0, 0, NULL, 0 }

#define	PTHREAD_RWLOCK_INITIALIZER	\
	{ 0 }

/* Function prototypes as required by POSIX/Open Group */

int		pthread_attr_destroy(pthread_attr_t *);
int		pthread_attr_getdetachstate(const pthread_attr_t *, int *);
int		pthread_attr_getguardsize(const pthread_attr_t *, size_t *);
int		pthread_attr_getinheritsched(const pthread_attr_t *, int *);
int		pthread_attr_getschedparam(const pthread_attr_t *, struct sched_param *);
int		pthread_attr_getschedpolicy(const pthread_attr_t *, int *);
int		pthread_attr_getscope(const pthread_attr_t *, int *);
int		pthread_attr_getstackaddr(const pthread_attr_t *, void **);
int		pthread_attr_getstacksize(const pthread_attr_t *, size_t *);
int		pthread_attr_init(pthread_attr_t *);
int		pthread_attr_setdetachstate(pthread_attr_t *, int);
int		pthread_attr_setguardsize(pthread_attr_t *, size_t);
int		pthread_attr_setinheritsched(pthread_attr_t *, int);
int		pthread_attr_setschedparam(pthread_attr_t *, const struct sched_param *);
int		pthread_attr_setschedpolicy(pthread_attr_t *, int);
int		pthread_attr_setscope(pthread_attr_t *, int);
int		pthread_attr_setstackaddr(pthread_attr_t *, void *);
int		pthread_attr_setstacksize(pthread_attr_t *, size_t);
int		pthread_attr_getstack(const pthread_attr_t *, void **, size_t *);
int		pthread_attr_setstack(pthread_attr_t *, void *, size_t );

int		pthread_cancel(pthread_t);
void	pthread_cleanup_push(void (*)(void*), void *);
void	pthread_cleanup_pop(int);

int		pthread_cond_broadcast(pthread_cond_t *);
int		pthread_cond_destroy(pthread_cond_t *);
int		pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
int		pthread_cond_signal(pthread_cond_t *);
int		pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *);
int		pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int		pthread_condattr_destroy(pthread_condattr_t *);
int		pthread_condattr_getpshared(const pthread_condattr_t *, int *);
int		pthread_condattr_init(pthread_condattr_t *);
int		pthread_condattr_setpshared(pthread_condattr_t *, int);

int		pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);

int		pthread_detach(pthread_t);
int		pthread_equal(pthread_t, pthread_t);
void	pthread_exit(void *);
int		pthread_getconcurrency(void);
int		pthread_getschedparam(pthread_t, int *, struct sched_param *);
void	*pthread_getspecific(pthread_key_t);
int		pthread_join(pthread_t, void **);
int		pthread_key_create(pthread_key_t *, void (*)(void *));
int		pthread_key_delete(pthread_key_t);
int		pthread_kill(pthread_t, int);

int		pthread_mutex_destroy(pthread_mutex_t *);
int		pthread_mutex_getprioceiling(const pthread_mutex_t *, int *);
int		pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
int		pthread_mutex_lock(pthread_mutex_t *);
int		pthread_mutex_setprioceiling(pthread_mutex_t *, int, int *);
int		pthread_mutex_trylock(pthread_mutex_t *);
int		pthread_mutex_unlock(pthread_mutex_t *);
int		pthread_mutexattr_destroy(pthread_mutexattr_t *);
int		pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *, int *);
int		pthread_mutexattr_getprotocol(const pthread_mutexattr_t *, int *);
int		pthread_mutexattr_getpshared(const pthread_mutexattr_t *, int *);
int		pthread_mutexattr_gettype(const pthread_mutexattr_t *, int *);
int		pthread_mutexattr_init(pthread_mutexattr_t *);
int		pthread_mutexattr_setprioceiling(pthread_mutexattr_t *, int);
int		pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);
int		pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
int		pthread_mutexattr_settype(pthread_mutexattr_t *, int);

int		pthread_once(pthread_once_t *, void (*)(void));

int		pthread_rwlock_destroy(pthread_rwlock_t *);
int		pthread_rwlock_init(pthread_rwlock_t *, const pthread_rwlockattr_t *);
int		pthread_rwlock_rdlock(pthread_rwlock_t *);
int		pthread_rwlock_tryrdlock(pthread_rwlock_t *);
int		pthread_rwlock_trywrlock(pthread_rwlock_t *);
int		pthread_rwlock_unlock(pthread_rwlock_t *);
int		pthread_rwlock_wrlock(pthread_rwlock_t *);
int		pthread_rwlockattr_destroy(pthread_rwlockattr_t *);
int		pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *, int *);
int		pthread_rwlockattr_init(pthread_rwlockattr_t *);
int		pthread_rwlockattr_setpshared(pthread_rwlockattr_t *, int);

pthread_t
pthread_self(void);

int		pthread_setcancelstate(int, int *);
int		pthread_setcanceltype(int, int *);
int		pthread_setconcurrency(int);
int		pthread_setschedparam(pthread_t, int , const struct sched_param *);
int		pthread_setspecific(pthread_key_t, const void *);
void	pthread_testcancel(void);

#ifdef __cplusplus
}
#endif

#endif	/* __F_SYLLABLE_PTHREAD_H_ */




