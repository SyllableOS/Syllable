/* Copyright (C) 1996, 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
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

#include <bits/libc-lock.h>
#include <atheos/kernel.h>
#include <atheos/atomic.h>
#include <atheos/semaphore.h>
#include <atheos/tld.h>

static void __create_libc_sem( __libc_lock_t* pnLock, const char* pzName, int nCount, int nFlags )
{
	if( atomic_swap( &pnLock->nIsInited, 1 ) == 0 )
	{
		pnLock->hMutex = create_semaphore( pzName, nCount, 0 );
		if( pnLock->hMutex < 0 )
		{
			atomic_swap( &pnLock->nIsInited, 0 );
			return;
		}
	}
	while( pnLock->hMutex < 0 )
		snooze( 1000LL );
}

void __lock_fini( __libc_lock_t* pnLock )
{
	if( pnLock->hMutex != -1 )
	{
		delete_semaphore( pnLock->hMutex );
		pnLock->hMutex = -1;
	}
}

int __lock_lock( __libc_lock_t* pnLock )
{
	int nError = 0;

	if( pnLock->hMutex == -1 )
		__create_libc_sem( pnLock, "libc_lock", 0, 0 );

	if( atomic_add( &pnLock->nLockCount, 1 ) > 0 )
	{
		while( 1 )
		{
			nError = lock_semaphore( pnLock->hMutex );
			if( nError >= 0 || nError != EINTR )
				break;
		}
	}

	return( nError );
}

int __lock_lock_recursive( __libc_lock_t* pnLock )
{
	int nError = 0;

	if( pnLock->hMutex == -1 )
		__create_libc_sem( pnLock, "libc_lock", 0, SEM_RECURSIVE );

	if( atomic_add( &pnLock->nLockCount, 1 ) > 0 )
	{
		while( 1 )
		{
			nError = lock_semaphore( pnLock->hMutex );
			if( nError >= 0 || nError != EINTR )
				break;
		}
	}

	return( nError );
}

int __lock_trylock( __libc_lock_t* pnLock )
{
	int nError = 0;

	if( pnLock->hMutex == -1 )
		__create_libc_sem( pnLock, "libc_lock", 0, 0 );

	if( atomic_add( &pnLock->nLockCount, 1 ) > 0 )
	{
		atomic_add( &pnLock->nLockCount, -1 );
		unlock_semaphore_x( pnLock->hMutex, 0, 0 );
		errno = EWOULDBLOCK;
		nError = -1;
	}

	return( nError );
}

void __lock_unlock( __libc_lock_t* pnLock )
{
	if( atomic_add( &pnLock->nLockCount, -1 ) > 1 )
		unlock_semaphore( pnLock->hMutex );
}

int __libc_key_create( __libc_key_t* pnKey, void* pDestr )
{
	*pnKey = alloc_tld( pDestr );
	return( ( *pnKey < 0 ) ? -1 : 0 );
}

int __libc_setspecific( __libc_key_t nKey, void* pData )
{
	set_tld( nKey, pData );
	return( 0 );
}

void* __libc_getspecific( __libc_key_t nKey )
{
	return( get_tld( nKey ) );
}
