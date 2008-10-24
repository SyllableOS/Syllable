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
#include <stdlib.h>
#include <posix/errno.h>
#include <atheos/atomic.h>
#include <atheos/semaphore.h>
#include <stdio.h>

#include <bits.h>
#include <debug.h>

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	if( mutex == NULL )
		return( EINVAL );

	if( mutex->__mutex != 0 )
		delete_semaphore( mutex->__mutex );

	if( mutex->__attr != NULL )
	{
		pthread_mutexattr_destroy( mutex->__attr );
	}

	return( 0 );
}

int pthread_mutex_getprioceiling(const pthread_mutex_t *mutex, int *prioceiling)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	int flags;
	pthread_mutexattr_t* mutex_attr = NULL;

	if( mutex == NULL )
		return( EINVAL );

	if( attr == NULL )
	{
		mutex_attr = &mutex->__def_attr;
		pthread_mutexattr_init( mutex_attr ); 
	}
	else
		mutex_attr = (pthread_mutexattr_t*)attr;

	mutex->__owner = pthread_self();		/* Mutex owner */
	mutex->__count = 0;						/* Lock count  */
	mutex->__attr = mutex_attr;				/* Our own copy of the mutex attributes */

	switch( mutex_attr->__mutexkind )
	{
		case PTHREAD_MUTEX_DEFAULT:			/* DEFAULT is undefined, this'll do */
		case PTHREAD_MUTEX_NORMAL:			/* As per the standard              */
			flags = 0;
			break;

		case PTHREAD_MUTEX_ERRORCHECK:
			flags = SEM_WARN_DBL_LOCK | SEM_WARN_DBL_UNLOCK;
			break;

		case PTHREAD_MUTEX_RECURSIVE:
			flags = SEM_RECURSIVE;
			break;

		default:
		{
			pthread_mutex_destroy( mutex );
			return( EINVAL );
		}
	};

	if( mutex_attr->__pshared == PTHREAD_PROCESS_SHARED )
		flags |= SEM_GLOBAL;		/* Technically not 100% correct, but close enough */
									/* for our purposes                               */

	/* Create the mutex itself */
	mutex->__mutex = create_semaphore( "pthread_sem", 1, flags | SEM_WARN_DBL_LOCK );

	return( 0 );
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	__pt_assert( mutex != NULL );

	if( mutex == NULL )
		return( EINVAL );

	if( ( mutex->__attr == 0 ) || ( mutex->__mutex == 0 ) )
		pthread_mutex_init( mutex, NULL );

	if( mutex->__attr->__mutexkind == PTHREAD_MUTEX_ERRORCHECK )
		if( ( mutex->__count > 0 ) && ( pthread_equal( mutex->__owner, pthread_self() ) ) )
			return( EDEADLK );

	__pt_lock_mutex( mutex->__mutex );
	atomic_add( (atomic_t*)&mutex->__count, 1 );		/* This takes care of PTHREAD_MUTEX_RECURSIVE */

	return( 0 );
}

int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	__pt_assert( mutex != NULL );

	if( mutex == NULL )
		return( EINVAL );

	if( ( mutex->__attr == 0 ) || ( mutex->__mutex == 0 ) )
		pthread_mutex_init( mutex, NULL );

	if( mutex->__count > 0 )
		return( EBUSY );

	return( pthread_mutex_lock( mutex ) );
}

int __pt_mutex_cannot_unlock(pthread_mutex_t *mutex)
{
	if( mutex == NULL )
		return( EINVAL );

	if( ( mutex->__attr == 0 ) || ( mutex->__mutex == 0 ) )
		return( pthread_mutex_init( mutex, NULL ) );

	if( ( mutex->__attr->__mutexkind == PTHREAD_MUTEX_ERRORCHECK ) || ( mutex->__attr->__mutexkind == PTHREAD_MUTEX_RECURSIVE ))
		if( ( mutex->__count == 0 ) || ( pthread_equal( mutex->__owner, pthread_self() ) == 0 ) )
			return( EPERM );

	return( 0 );
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	int error;
	if( (error = __pt_mutex_cannot_unlock(mutex)) )
		return error;

	atomic_add( (atomic_t*)&mutex->__count, -1 );		/* This takes care of PTHREAD_MUTEX_RECURSIVE */
	__pt_unlock_mutex( mutex->__mutex );

	return 0;
}		

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	if( attr == NULL )
		return( EINVAL );

	return( 0 );
}

int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr, int *prioceiling)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *protocol)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr, int *shared)
{
	if( attr == NULL )
		return( EINVAL );

	*shared = attr->__pshared;

	return( 0 );
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
	if( attr == NULL )
		return( EINVAL );

	*type = attr->__mutexkind;

	return( 0 );
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	if( attr == NULL )
		return( EINVAL );

	attr->__mutexkind = PTHREAD_MUTEX_DEFAULT;
	attr->__pshared = PTHREAD_PROCESS_PRIVATE;

	return( 0 );
}

int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
{
	return( ENOSYS );		/* Not implemented */
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared)
{
	if( attr == NULL )
		return( EINVAL );

	if( ( pshared != PTHREAD_PROCESS_SHARED ) && ( pshared != PTHREAD_PROCESS_PRIVATE ) )
		return( EINVAL );

	attr->__pshared = pshared;

	return( 0 );
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	if( attr == NULL )
		return( EINVAL );

	if( ( type != PTHREAD_MUTEX_DEFAULT ) &&
	    ( type != PTHREAD_MUTEX_ERRORCHECK ) &&
	    ( type != PTHREAD_MUTEX_NORMAL ) &&
	    ( type != PTHREAD_MUTEX_RECURSIVE ) )
			return( EINVAL );

	attr->__mutexkind = type;

	return( 0 );
}

