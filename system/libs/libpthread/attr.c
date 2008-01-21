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
#include <string.h>
#include <posix/errno.h>
#include <atheos/threads.h>		/* Kernel threads interface */

#include <bits.h>
#include <debug.h>

int pthread_attr_destroy(pthread_attr_t *attr)
{
	if( attr == NULL || (attr && attr->destroyed == true ) )
		return( EINVAL );

	debug( "destroy attr at %p\n", attr );

	attr->destroyed = true;

	debug( "attr->__name=%p\n", attr->__name );
	if( attr->__name != NULL )
		free( attr->__name );

	debug( "attr->__sched_param=%p\n", attr->__sched_param );
	if( attr->__sched_param != NULL )
		free( attr->__sched_param );

	debug( "attr->cancellation=%p\n", attr->cancellation );
	if( attr->cancellation != NULL )
		free( attr->cancellation );

	return( 0 );
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *state)
{
	if( attr == NULL )
		return( EINVAL );

	*state = attr->__detachstate;

	return( 0 );
}

int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *size)
{
	if( attr == NULL )
		return( EINVAL );

	*size = attr->__guardsize;

	return( 0 );
}

int pthread_attr_getinheritsched(const pthread_attr_t *attr, int *sched)
{
	return( ENOSYS );			/* Not implemented */
}

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param)
{
	if( attr == NULL )
		return( EINVAL );

	/* This is probably wrong */
	param = memcpy( param, attr->__sched_param, sizeof( struct sched_param ) );

	return( 0 );
}

int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *sched)
{
	if( attr == NULL || sched == NULL )
		return EINVAL;

	/* Always using SCHED_RR */
	*sched = SCHED_RR;

	return 0;	
}

int pthread_attr_getscope(const pthread_attr_t *attr, int *scope)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_attr_init(pthread_attr_t *attr)
{
	if( attr == NULL )
		return( EINVAL );

	debug( "initalise attr at %p\n", attr );

	attr->__stacksize = PTHREAD_STACK_MIN;
	attr->__stackaddr = NULL;						/* FIXME: Place holder */
	attr->__guardsize = 0;							/* Not implemented */
	attr->__schedpriority = NORMAL_PRIORITY;
	attr->__detachstate = PTHREAD_CREATE_JOINABLE;
	attr->__name = NULL;
	attr->__sched_param = NULL;
	attr->cancellation = NULL;

	attr->__name = malloc( 8 );
	if( attr->__name == NULL )
		return( ENOMEM );
	debug( "attr->__name=%p\n", attr->__name );
	attr->__name = strcpy( attr->__name, "pthread" );

	attr->__sched_param = malloc( sizeof( struct sched_param ) );
	if( attr->__sched_param == NULL )
	{
		pthread_attr_destroy( attr );
		return( ENOMEM );
	}
	debug( "attr->__sched_param=%p\n", attr->__sched_param );

	attr->cancellation = malloc( sizeof( struct __pthread_cancel_s ) );
	if( attr->cancellation == NULL )
	{
		pthread_attr_destroy( attr );
		return( ENOMEM );
	}
	debug( "attr->cancellation=%p\n", attr->cancellation );

	attr->cancellation->state = PTHREAD_CANCEL_ENABLE;
	attr->cancellation->type = PTHREAD_CANCEL_DEFERRED;
	attr->cancellation->cancelled = false;

	attr->internal_malloc = false;
	attr->destroyed = false;

	debug( "attr initialised\n" );

	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int state)
{
	if( attr == NULL )
		return( EINVAL );

	if( ( state != PTHREAD_CREATE_JOINABLE ) && ( state != PTHREAD_CREATE_DETACHED ) )
		return( EINVAL );

	attr->__detachstate = state;

	return 0;
}

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t size)
{
	if( attr == NULL )
		return( EINVAL );

	if( attr->__stackaddr != 0 )	/* Guards do not apply if the thread is using its own stack allocator */
		return( EINVAL );

	if( size < 0 )
		return( EINVAL );

	attr->__guardsize = size;		/* Note that we do nothing other than store the value */

	return( 0 );
}

int pthread_attr_setinheritsched(pthread_attr_t *attr, int sched)
{
	return( ENOSYS );			/* Not implemented */
}

int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
{
	if( attr == NULL )
		return( EINVAL );

	attr->__sched_param = (struct sched_param*)param;

	return( 0 );
}

int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_attr_setscope(pthread_attr_t *attr, int scope)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **addr)
{
	return pthread_attr_getstack( attr, addr, NULL );
}

int pthread_attr_setstackaddr(pthread_attr_t *attr, void *addr)
{
	return pthread_attr_setstack( attr, addr, attr->__stacksize );
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *size)
{
	return pthread_attr_getstack( attr, NULL, size );
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t size)
{
	return pthread_attr_setstack( attr, attr->__stackaddr, size );
}

int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize)
{
	thread_info info;
	int ret;

	if( attr == NULL )
		return EINVAL;

	ret = get_thread_info( get_thread_id(0), &info );
	if( ret == EOK )
	{
		if( stackaddr != NULL )
			*stackaddr = info.ti_stack;
		if( stacksize != NULL )
			*stacksize = info.ti_stack_size;
	}

	return ret;
}

int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize)
{
	if( attr == NULL )
		return EINVAL;

	if( ( stackaddr == NULL ) || ( stacksize < PTHREAD_STACK_MIN ) )
		return EINVAL;

	/* XXXKV: Store the stack details but we can't adjust the stack on a running thread */
	attr->__stackaddr = stackaddr;
	attr->__stacksize = stacksize;

	return ENOSYS;
}

