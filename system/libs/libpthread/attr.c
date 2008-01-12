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

#include "inc/bits.h"

int pthread_attr_destroy(pthread_attr_t *attr)
{
	if( attr == NULL )
		return( EINVAL );

	if( attr->__name != NULL )
		free( attr->__name );

	if( attr->__sched_param != NULL )
		free( attr->__sched_param );

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
	return( ENOSYS) ;		/* Not implemented */
}

int pthread_attr_getscope(const pthread_attr_t *attr, int *scope)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **addr)
{
	if( attr == NULL )
		return( EINVAL );

	*addr = attr->__stackaddr;

	return( 0 );
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *size)
{
	if( attr == NULL )
		return( EINVAL );

	*size = attr->__stacksize;

	return( 0 );
}

int pthread_attr_init(pthread_attr_t *attr)
{
	if( attr == NULL )
		return( EINVAL );

	attr->__stacksize = __DEFAULT_STACK_SIZE;
	attr->__stackaddr = NULL;						/* FIXME: Place holder */
	attr->__guardsize = 0;							/* Not implemented */
	attr->__schedpriority = NORMAL_PRIORITY;

	attr->__name = malloc( 8 );
	if( attr->__name == 0 )
		return( ENOMEM );

	attr->__name = strncpy( attr->__name, "pthread\0", 8 );

	attr->__detachstate = PTHREAD_CREATE_JOINABLE;
	attr->__sched_param = malloc( sizeof( struct sched_param ) );
	if( attr->__sched_param == 0 )
	{
		pthread_attr_destroy( attr );
		return( ENOMEM );
	}

	attr->cancellation.state = PTHREAD_CANCEL_ENABLE;
	attr->cancellation.type = PTHREAD_CANCEL_DEFERRED;
	attr->cancellation.cancelled = false;

	attr->internal_malloc = false;

	return( 0 );
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

int pthread_attr_setstackaddr(pthread_attr_t *attr, void *addr)
{
	if( attr == NULL )
		return( EINVAL );

	if( addr < 0 )
		return( EINVAL );

	attr->__stackaddr = addr;	/* Note that we do nothing other than store the address, */
									/* currently there is no way to change the stack address */

	return( 0 );
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t size)
{
	if( attr == NULL )
		return( EINVAL );

	if( size < 0 )
		return( EINVAL );

	attr->__stacksize = size;	/* Note that we do nothing other than store the size, */
									/* currently there is no way to change the stack size */
									/* once the thread has been created                   */

	return( 0 );
}




