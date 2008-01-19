/* POSIX threads for Syllable - A simple PThreads implementation             */
/*                                                                           */
/* Copyright (C) 2002 Kristian Van Der Vliet (vanders@users.sourceforge.net) */
/* Copyright (C) 2000 Kurt Skauen                                            */
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
#include <signal.h>				/* pthread_signal() etc.           */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <posix/errno.h>
#include <posix/signal.h>
#include <atheos/atomic.h>
#include <atheos/tld.h>			/* Thread specific local data       */
#include <atheos/threads.h>		/* Kernel threads interface         */
#include <atheos/kernel.h>		/* Additional kernel & thread API's */

#include <bits.h>
#include <debug.h>

static pthread_key_t cleanupKey = 0;
static pthread_key_t selfKey = 0;

static int __pt_thread_exists(pthread_t thread)
{
	thread_info info;
	return ( get_thread_info( PT_TID(thread), &info ) == EOK ) ? 1 : 0;
}

static void __pt_cleanup_data(void *data)
{
	pthread_t thread = (pthread_t)data;

	debug( "cleaning up thread data\n" );

	/* delete the attribute data if we created it */
	if( thread && thread->attr && thread->attr->internal_malloc )
		free( thread->attr );
	/* delete the thread data (we always allocate it) */
	if( thread )
		free( thread );
}

static void __pt_handle_cancel( int signo )
{
	pthread_t thread = pthread_getspecific( selfKey );
	pthread_attr_t *attr = PT_ATTR(thread);

	debug( "cancellation requested\n" );

	if( attr != NULL )
	{
		if( attr->cancellation->state == PTHREAD_CANCEL_ENABLE )
			attr->cancellation->cancelled = true;

		if( attr->cancellation->type == PTHREAD_CANCEL_ASYNCHRONOUS )
			pthread_exit( NULL );
	}
}

void __attribute__((constructor)) __pt_init(void)
{
	cleanupKey = alloc_tld( NULL );
	selfKey = alloc_tld( __pt_cleanup_data );
}

void __attribute__((destructor)) __pt_uninit(void)
{
	free_tld( cleanupKey );
	free_tld( selfKey );
}

struct __pt_entry_data
{
	void *(*start_routine)(void *);
	void *arg;
	pthread_t pthread;
};

static void *__pt_entry(void *arg)
{
	struct __pt_entry_data *entry_data = (struct __pt_entry_data *)arg;
	struct sigaction sa;
	void *result;

	pthread_setspecific( selfKey, (void *)entry_data->pthread );

	/* catch SIGCANCEL from pthread_cancel() */
	sa.sa_handler = __pt_handle_cancel;
	sigaction( SIGCANCEL, &sa, NULL );

	result = entry_data->start_routine(entry_data->arg);
	free( entry_data );
	pthread_exit( result );
}

int pthread_cancel(pthread_t thread)
{
	debug( "attempting to cancel thread %d\n", PT_TID(thread) );
	return pthread_kill( thread, SIGCANCEL );
}

void pthread_cleanup_push(void (*routine)(void*), void *arg)
{
	__pt_cleanup *cleanup;

	debug( "adding cleanup handler at %p(%p) for thread %d\n", routine, arg, PT_TID(pthread_self()) );

	cleanup = malloc( sizeof( __pt_cleanup ) );
	cleanup->routine = routine;
	cleanup->arg = arg;
	cleanup->prev = (__pt_cleanup *) pthread_getspecific( cleanupKey );

	pthread_setspecific( cleanupKey, (void *) cleanup );
}

void pthread_cleanup_pop(int execute)
{
	__pt_cleanup *cleanup;

	cleanup = (__pt_cleanup *) pthread_getspecific( cleanupKey );
	if ( cleanup )
	{
		debug( "running cleanup for thread %d\n", PT_TID(pthread_self()) );

		if ( execute && cleanup->routine )
			(*cleanup->routine) (cleanup->arg);
		pthread_setspecific( cleanupKey, (void *) cleanup->prev );
		free( cleanup );
	}
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
	pthread_t pthread;
	thread_id kthread;
	pthread_attr_t *thread_attr = NULL;
	struct __pt_entry_data *entry_data = NULL;

	if( thread == NULL || start_routine == NULL )
		return EINVAL;

	if( attr && attr->destroyed == true )
		return EINVAL;

	debug( "creating new thread\n" );

	pthread = malloc( sizeof( pthread_t ) );
	if( pthread == NULL )
		return ENOMEM;

	if( attr == NULL )
	{
		debug( "creating default attributes\n" );

		thread_attr = malloc( sizeof( pthread_attr_t ) );
		if( thread_attr == NULL )
			return ENOMEM;

		pthread_attr_init( thread_attr );		/* Use default values */

		thread_attr->internal_malloc = true;
	}
	else
	{
		debug( "using supplied attributes\n" );

		thread_attr = (pthread_attr_t*)attr;
	}

	entry_data = malloc( sizeof( entry_data ) );
	if( entry_data == NULL )
		return ENOMEM;
	entry_data->start_routine = start_routine;
	entry_data->arg = arg;
	entry_data->pthread = pthread;

	kthread = spawn_thread( thread_attr->__name, __pt_entry, thread_attr->__schedpriority, thread_attr->__stacksize, entry_data);
	if( kthread < 0 )
		return( kthread );

	debug( "spawn_thread gives thread id %d\n", kthread );

	pthread->id = kthread;
	pthread->attr = thread_attr;

	*thread = pthread;

	debug( "resuming new thread\n" );

	resume_thread( kthread );

	return 0;
}

int pthread_detach(pthread_t thread)
{
	debug( "not implemented\n" );
	/* XXXKV: Not needed on Syllable? */
	return 0;
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
	if( t1 == NULL || t2 == NULL )
		return 0;

	return ( PT_TID(t1) == PT_TID(t2) );
}

void pthread_exit(void *value_ptr)
{
	__pt_cleanup *cleanup;
	int result = 0;

	debug( "exit thread %d\n", PT_TID(pthread_self()) );

	/* call all cleanup routines */
	cleanup = (__pt_cleanup *) pthread_getspecific( cleanupKey );
	while ( cleanup )
	{
		if ( cleanup->routine )
			(*cleanup->routine) (cleanup->arg);
		cleanup = cleanup->prev;
		free( cleanup );
	}

	if( NULL != value_ptr )
		result = (int)value_ptr;
	exit_thread( result );
}

int pthread_getconcurrency(void)
{
	return ENOSYS;
}

int pthread_getschedparam(pthread_t thread, int *foo, struct sched_param *param)
{
	return ENOSYS;
}

void *pthread_getspecific(pthread_key_t key)
{
	/* get_tld() will perform additional validation */
	if( key < TLD_USER )
		return NULL;

	return get_tld( key );
}

int pthread_join(pthread_t thread, void **value_ptr)
{
	int result;
	pthread_attr_t *target_attr = PT_ATTR(thread);

	debug( "thread %d joins on thread %d\n", PT_TID(pthread_self()), PT_TID(thread) );

	/* if the thread has exited via. pthread_exit() before another thread attempts to join
	   we will return ESRCH. */
	if( !__pt_thread_exists( thread ) )
		return ESRCH;

	if( pthread_equal( pthread_self(), thread ) )
		return EDEADLK;

	if( target_attr && target_attr->__detachstate == PTHREAD_CREATE_DETACHED )
		return EINVAL;

	/* wait for the target thread to exit. if the target thread has exited between the check
	   above and the call to wait_for_thread(), the result will be inconsistent. */
	result = wait_for_thread( PT_TID(thread) );
	debug( "result was %d\n", result );
	if( NULL != value_ptr )
		*value_ptr = (void*)result;

	return 0;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	int tld = alloc_tld( destructor );

	debug( "allocated new TLD %d with destructor at %p for thread %d\n", tld, destructor, PT_TID(pthread_self()) );

	if ( tld >= 0 )
	{
		*key = tld;
		return 0;
	}
	else
		return -1;
}

int pthread_key_delete(pthread_key_t key)
{
	debug( "freeing TLD %d for thread %d\n", key, PT_TID(pthread_self()) );
	return free_tld( key );
}

int pthread_kill(pthread_t thread, int sig)
{
	int ret;

	if( sig < 0 || sig > _NSIG )
		return EINVAL;

	if( __pt_thread_exists( thread ) )
		ret = kill( (pid_t)PT_TID(thread), sig );
	else
		ret = ESRCH;

	return ret;
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	if ( atomic_swap( once_control, 1 ) == 0 )
		init_routine();

	return 0;
}

pthread_t pthread_self(void)
{
	pthread_t thread;

	thread = (pthread_t)get_tld( selfKey );
	/* if the calling thread is the "main" thread then no selfKey will have
	   been set, so we must create one */
	if( thread == NULL )
	{
		thread = malloc( sizeof( pthread_t ) );
		if( thread == NULL )
			return NULL;

		thread->id = get_thread_id( NULL );

		thread->attr = malloc( sizeof( pthread_attr_t ) );
		if( thread->attr == NULL )
			return NULL;

		pthread_attr_init( thread->attr );		/* Use default values */
		thread->attr->internal_malloc = true;

		pthread_setspecific( selfKey, (void *)thread );
	}
	__pt_assert( thread != NULL );

	return thread;
}

int pthread_setcancelstate(int state, int *oldstate)
{
	pthread_t thread = pthread_getspecific( selfKey );
	pthread_attr_t *attr = PT_ATTR(thread);

	if( ( state != PTHREAD_CANCEL_ENABLE ) && ( state != PTHREAD_CANCEL_DISABLE ) )
		return EINVAL;

	if( oldstate != NULL )
		*oldstate = attr->cancellation->state;
	attr->cancellation->state = state;

	return 0;
}

int pthread_setcanceltype(int type, int *oldtype)
{
	pthread_t thread = pthread_getspecific( selfKey );
	pthread_attr_t *attr = PT_ATTR(thread);

	if( ( type != PTHREAD_CANCEL_DEFERRED ) && ( type != PTHREAD_CANCEL_ASYNCHRONOUS ) )
		return EINVAL;

	if( oldtype != NULL )
		*oldtype = attr->cancellation->type;
	attr->cancellation->type = type;

	return 0;
}

int pthread_setconcurrency(int foo)
{
	return ENOSYS;
}

int pthread_setschedparam(pthread_t thread, int foo, const struct sched_param *param)
{
	return ENOSYS;
}

int pthread_setspecific(pthread_key_t key, const void *data)
{
	/* set_tld() will perform additional validation */
	if( key < TLD_USER )
		return EINVAL;

	set_tld( key, (void*)data );
	return 0;
}

void pthread_testcancel(void)
{
	pthread_t thread = pthread_getspecific( selfKey );
	pthread_attr_t *attr = PT_ATTR(thread);

	if( attr->cancellation->state == PTHREAD_CANCEL_ENABLE && attr->cancellation->cancelled )
		pthread_exit( NULL );
}

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
	return sigprocmask( how, set, oset );
}

