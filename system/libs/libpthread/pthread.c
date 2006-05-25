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

#include <stdio.h>

#include <posix/errno.h>
#include <atheos/atomic.h>
#include <atheos/tld.h>			/* Thread specific local data       */
#include <atheos/threads.h>		/* Kernel threads interface         */
#include <atheos/kernel.h>		/* Additional kernel & thread API's */

#include "inc/bits.h"


static pthread_key_t cleanupKey = 0;

void __attribute__((constructor)) __pt_init(void)
{
	cleanupKey = alloc_tld( NULL );
}

void __attribute__((destructor)) __pt_uninit(void)
{
	free_tld( cleanupKey );
}

int pthread_cancel(pthread_t thread)
{
	return -ENOSYS;
}

void pthread_cleanup_push(void (*routine)(void*), void *arg)
{
	__pt_cleanup *cleanup;

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
		if ( execute && cleanup->routine )
			(*cleanup->routine) (cleanup->arg);
		pthread_setspecific( cleanupKey, (void *) cleanup->prev );
		free( cleanup );
	}
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
	thread_id kthread;
	pthread_attr_t *thread_attr = NULL;

	if( start_routine == NULL )
		return( EINVAL );

	if( attr == NULL )
	{	
		thread_attr = malloc( sizeof( pthread_attr_t ) );
		if( thread_attr == 0 )
			return( ENOMEM );

		pthread_attr_init( thread_attr );		/* Use default values */
	}
	else
		thread_attr = (pthread_attr_t*)attr;

	kthread = spawn_thread( thread_attr->__name, start_routine, thread_attr->__schedpriority, thread_attr->__stacksize, arg);

	if( attr == NULL )
	{
		pthread_attr_destroy( thread_attr );
		free( thread_attr );		/* Ensure we free the default attr, or we'll have a leak here */
	}

	if( kthread < 0 )
		return( kthread );

	*thread = kthread;

	resume_thread( kthread );

	return( 0 );
}

int pthread_detach(pthread_t thread)
{
	return -ENOSYS;
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
	if( t1 == t2 )
		return( 1 );

	return( 0 );
}

void pthread_exit(void *value_ptr)
{
	__pt_cleanup *cleanup;
	
	/* value_ptr ignored for now */
	exit_thread(0);
	
	/* call all cleanup routines */
	cleanup = (__pt_cleanup *) pthread_getspecific( cleanupKey );
	while ( cleanup )
	{
		if ( cleanup->routine )
			(*cleanup->routine) (cleanup->arg);
		cleanup = cleanup->prev;
		free( cleanup );
	}

	return;
}

int pthread_getconcurrency(void)
{
	return -ENOSYS;
}

int pthread_getschedparam(pthread_t thread, int *foo, struct sched_param *param)
{
	return -ENOSYS;
}

void *pthread_getspecific(pthread_key_t key)
{
	return( get_tld( key ) );
}

int pthread_join(pthread_t thread, void **foo)
{
	wait_for_thread( thread );
	return( 0 );
	//return -ENOSYS;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	int tld = alloc_tld( destructor );

	if ( tld >= 0 )
	{
		*key = tld;
		return( 0 );
	}
	else
		return( -1 );
}

int pthread_key_delete(pthread_key_t key)
{
	return( free_tld( key ) );
}

int pthread_kill(pthread_t thread, int sig)
{
	return( kill( (pid_t)thread, sig ) );
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	if ( atomic_swap( once_control, 1 ) == 0 )
		init_routine();

	return( 0 );
}

pthread_t pthread_self(void)
{
	return( get_thread_id( NULL ) );
}

int pthread_setcancelstate(int state, int *oldstate)
{
	if( ( state != PTHREAD_CANCEL_ENABLE ) || ( state != PTHREAD_CANCEL_DISABLE ) )
		return( EINVAL );

	return( ENOSYS );
}

int pthread_setcanceltype(int type, int *oldtype)
{
	if( ( type != PTHREAD_CANCEL_DEFERRED ) || ( type != PTHREAD_CANCEL_ASYNCHRONOUS ) )
		return( EINVAL );

	return( ENOSYS );
}

int pthread_setconcurrency(int foo)
{
	return -ENOSYS;
}

int pthread_setschedparam(pthread_t thread, int foo, const struct sched_param *param)
{
	return -ENOSYS;
}

int pthread_setspecific(pthread_key_t key, const void *data)
{
	set_tld( key, (void*)data );
	return( 0 );
}

void pthread_testcancel(void)
{
	return;
}

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
	return	-ENOSYS;
}







