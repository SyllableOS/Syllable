/* POSIX threads for Syllable - A simple PThreads implementation             */
/*                                                                           */
/* Copyright (C) 2002 Kristian Van Der Vliet (vanders@users.sourceforge.net) */
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

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

#include <posix/errno.h>
#include <atheos/atomic.h>
#include <atheos/threads.h>
#include <atheos/kernel.h>

#include "inc/bits.h"

int __pt_mutex_cannot_unlock(pthread_mutex_t *mutex);

static void __pt_timer_thread_entry( __pt_timer_args *arg );
static int __pt_do_cond_wait( pthread_cond_t *cond, pthread_mutex_t *mutex );

int pthread_cond_broadcast(pthread_cond_t *cond)
{
	__pt_thread_list_t* __current_thread;
	__pt_thread_list_t* __next_thread;

	if( cond == NULL )
		return( EINVAL );

	if( cond->__head == NULL )
		return( 0 );	/* There are no threads to signal, so we return immediatly */

	__pt_lock_mutex( cond->__lock );

	/* All we need to do is walk the list and resume every thread there, */
	/* free()'ing as we go.                                              */

	__current_thread = cond->__head;

	do
	{
		/* The thread might not sleep yet */
		thread_info sInfo;
		while( get_thread_info( PT_TID(__current_thread->__thread_id), &sInfo ) == 0 )
		{
			if( sInfo.ti_state != TS_READY )
				break;
		}
		resume_thread( PT_TID(__current_thread->__thread_id) );
		__next_thread = __current_thread->__next;

		free( __current_thread );

		__current_thread = __next_thread;
	}
	while( __current_thread != NULL );

	cond->__head = NULL;
	cond->__count = 0;

	__pt_unlock_mutex( cond->__lock );

	return( 0 );
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
	//printf( "pthread_cond_destroy() %x %i\n", (uint)cond, cond->__lock );
	if( cond == NULL )
		return( EINVAL );

	if( ( cond->__count > 0 ) || ( cond->__head != NULL ) )
		return( EBUSY );

	if( cond->__attr != NULL )
	{
		pthread_condattr_destroy( cond->__attr );
	}

	delete_semaphore( cond->__lock );

	return( 0 );
}

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	pthread_condattr_t* cond_attr = NULL;
	
	//printf( "pthread_cond_init() %x\n", (uint)cond );

	if( cond == NULL )
		return( EINVAL );

/* If cond was allocated on the heap it might contain random garbage and look
	   like a condvar that has already been initialized. There's at least one
	   program that does this so I've commented out the code below.  - Jan */
	#if 0
	if( ( cond->__owner > 0) && ( ( cond->__count > 0 ) || ( cond->__head != NULL ) ) )
		return( EBUSY );		/* The conditional has (probably) already been initialiased. */
								/* FIXME: This is a little dodgy, as we don't know what      */
								/* values will be held in an uninitialised cond...           */
	#endif
	if( attr == NULL )
	{
		cond_attr = &cond->__def_attr;

		pthread_condattr_init( cond_attr ); 
	}
	else
		cond_attr = (pthread_condattr_t*)attr;

	cond->__owner = pthread_self();
	cond->__count = 0;				/* Number of threads waiting on this conditional  */
	cond->__attr = cond_attr;		/* Our own copy of the cond attributes            */
	cond->__head = NULL;				/* No threads are yet waiting on this conditional */
	cond->__lock = create_semaphore( "pthread_cond_lock", 1, SEM_WARN_DBL_LOCK );

	
	return( 0 );
}

int pthread_cond_signal(pthread_cond_t *cond)
{
	__pt_thread_list_t* __waiting_thread;
	//printf( "pthread_cond_signal() %x\n", (uint)cond );

	if( cond == NULL )
	{
		return( EINVAL );
	}
	
	__pt_lock_mutex( cond->__lock );
	

	if( cond->__head == NULL )
	{
		__pt_unlock_mutex( cond->__lock );
		return( 0 );	/* There are no threads to signal, so we return immediatly */
	}

	/* We wake up threads in a FIFO manner.  The spec tells us we should wake threads */
	/* according to the scheduling policy, but we don't have one, so this will do.    */

	__waiting_thread = cond->__head;
	
	cond->__head = __waiting_thread->__next;
	if( cond->__head != NULL )
		cond->__head->__prev = __waiting_thread->__prev;
		
	atomic_dec( (atomic_t*)&cond->__count );

	__pt_unlock_mutex( cond->__lock );
	
	/* The thread might not sleep yet */
	thread_info sInfo;
	while( get_thread_info( PT_TID(__waiting_thread->__thread_id), &sInfo ) == 0 )
	{
		if( sInfo.ti_state != TS_READY )
			break;
	}
	
	resume_thread( PT_TID(__waiting_thread->__thread_id) );

	free( __waiting_thread );

	return( 0 );
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
	//printf("pthread_cond_timed_wait %i\n", get_thread_id( NULL ));
	thread_id kthread;
	__pt_timer_args* arg;	
	int ret;
	
	if( cond == NULL )
		return( EINVAL );

	if( mutex == NULL )
		return( EINVAL );

	if( abstime == NULL )
		return( EINVAL );
		
	arg = malloc( sizeof( __pt_timer_args ) );
	arg->abstime = (struct timespec*)abstime;
	arg->thread = pthread_self();
	arg->cond = cond;
	arg->error = 0;

	kthread = spawn_thread( "pthread_cond_timer", __pt_timer_thread_entry, NORMAL_PRIORITY, PTHREAD_STACK_MIN, arg);

	if( kthread < 0 )
		return( kthread );

	/* Start the "timer" running */
	resume_thread( kthread );

	/* Wait on the conditional.  Either pthread_signal(), pthread_broadcast() or the */
	/* timer thread will wake us up, whichever comes first                           */
	ret = __pt_do_cond_wait( cond, mutex );
	if( arg->error == 0 )
	{
		/* Wakeup up -> tell the timer to exit */
		resume_thread( kthread );
	} else
		ret = ETIMEDOUT;
	
	return( ret );
}

void __pt_timer_thread_entry( __pt_timer_args *arg )
{
	struct timeval now;
	(void)gettimeofday(&now, NULL);
	__pt_thread_list_t* __current_thread;
	__pt_thread_list_t* __next_thread;
	pthread_cond_t *cond = arg->cond;
	
	long int sleeptime = arg->abstime->tv_sec * 1000000UL + arg->abstime->tv_nsec / 1000;
	sleeptime -= now.tv_sec * 1000000UL + now.tv_usec;
		
	if( sleeptime < 0 )
		sleeptime = 0;
		
	//printf( "Sleep %i\n", sleeptime / 1000 );
		
	sleep( sleeptime / 1000000 );
	snooze( sleeptime % 1000000 );
	
	/* Remove ourself from the list */
	__pt_lock_mutex( cond->__lock );

	if( cond->__head != NULL )
	{
		__current_thread = cond->__head;

		do
		{
			__next_thread = __current_thread->__next;
			if( pthread_equal( __current_thread->__thread_id, arg->thread ) )
			{
				if( cond->__head == __current_thread )
				{
					atomic_dec( (atomic_t*)&cond->__count );
					cond->__head = __current_thread->__next;
					if( cond->__head != NULL )
						cond->__head->__prev = __current_thread->__prev;
					free( __current_thread );
				} else 
				{
					atomic_dec( (atomic_t*)&cond->__count );
					__current_thread->__prev->__next = __current_thread->__next;
					if( __current_thread->__next != NULL )
						__current_thread->__next->__prev = __current_thread->__prev;
					else {
						assert( cond->__head->__prev == __current_thread );
						cond->__head->__prev = __current_thread->__prev;
					}
					free( __current_thread );
				}
			}
			__current_thread = __next_thread;
		} while( __current_thread != NULL );
	}
	__pt_unlock_mutex( cond->__lock );
	arg->error = -ETIMEDOUT;
	
	/* The thread might not sleep yet */
	thread_info sInfo;
	while( get_thread_info( PT_TID(arg->thread), &sInfo ) == 0 )
	{
		if( sInfo.ti_state != TS_READY )
			break;
	}
	resume_thread( PT_TID(arg->thread) );

	free( arg );

	return;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	//printf("pthread_cond_wait %i\n", get_thread_id( NULL ));
	if( cond == NULL )
		return( EINVAL );

	if( mutex == NULL )
		return( EINVAL );
		
	return( __pt_do_cond_wait( cond, mutex ) );
}

int __pt_do_cond_wait( pthread_cond_t *cond, pthread_mutex_t *mutex )
{
	__pt_thread_list_t* __waiting_thread;
	
	if( __pt_mutex_cannot_unlock(mutex) )
 		return( EINVAL );
	
	__waiting_thread = malloc( sizeof( __pt_thread_list_t ) );
	if( __waiting_thread == NULL )
		return( ENOMEM );		/* We're not supposed to return ENOMEM, but we do */

	__waiting_thread->__thread_id = pthread_self();
	
	__pt_lock_mutex( cond->__lock );

	if( cond->__head != NULL)
	{
		__waiting_thread->__next = NULL;
		assert( cond->__head->__prev->__next == NULL );
		__waiting_thread->__prev = cond->__head->__prev;
		cond->__head->__prev->__next = __waiting_thread; 
		cond->__head->__prev = __waiting_thread;
	}
	else
	{
		__waiting_thread->__prev = __waiting_thread;
		__waiting_thread->__next = NULL;
		cond->__head = __waiting_thread;
	}

	atomic_inc( (atomic_t*)&cond->__count );

	__pt_unlock_mutex( cond->__lock );	
	pthread_mutex_unlock( mutex );
	suspend_thread( PT_TID(__waiting_thread->__thread_id) );

	pthread_mutex_lock( mutex );

	return( 0 );
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
	if( attr == NULL )
		return( EINVAL );

	return( 0 );
}

int pthread_condattr_getpshared(const pthread_condattr_t *attr, int *shared)
{
	if( attr == NULL )
		return( EINVAL );

	*shared = attr->__pshared;

	return( 0 );
}

int pthread_condattr_init(pthread_condattr_t *attr)
{
	if( attr == NULL )
		return( EINVAL );

	attr->__pshared = PTHREAD_PROCESS_PRIVATE;

	return( 0 );
}

int pthread_condattr_setpshared(pthread_condattr_t *attr, int shared)
{
	if( attr == NULL )
		return( EINVAL );

	if( ( shared != PTHREAD_PROCESS_PRIVATE ) && ( shared != PTHREAD_PROCESS_SHARED ) )
		return( EINVAL );

	attr->__pshared = shared;

	return( 0 );
}

