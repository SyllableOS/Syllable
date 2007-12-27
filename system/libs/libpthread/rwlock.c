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
#include <posix/errno.h>

#include <debug.h>

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *attr, int *shared)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
	debug( "not implemented\n" );
	return ENOSYS;
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int shared)
{
	debug( "not implemented\n" );
	return ENOSYS;
}


