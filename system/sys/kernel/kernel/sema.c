
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <posix/errno.h>

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/semaphore.h>
#include <atheos/time.h>
#include <atheos/spinlock.h>

#include <macros.h>

#include "inc/semaphore.h"
#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/smp.h"

/* Selective debugging level overrides */
#ifdef KERNEL_DEBUG_SEMA
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_SEMA
#endif

static MultiArray_s g_sSemaphores;
static SemContext_s *g_psKernelSemContext = NULL;

#ifdef __DETECT_DEADLOCK
static int g_nDeadLockDetectRun = 1;
#endif

//static int    g_nSemListSpinLock = 0;

SPIN_LOCK( g_sSemListSpinLock, "semlist_slock" );

//extern SpinLock_s g_sSchedSpinLock;

//#define g_sSemListSpinLock g_sSchedSpinLock

void dump_semaphore( Semaphore_s *psSema, bool bShort );

/**
 * Create a new (empty) semaphore context.
 * This is called during the creation of the first process (init) only.
 * Thereafter, process semaphore contexts are created by cloning the
 * existing process context using clone_semaphore_context().  At
 * exec() time, the process semaphore context is cleared, not created
 * again.
 *
 * \return Returns a new semaphore context with an empty allocation map.
 */
SemContext_s *create_semaphore_context( void )
{
	SemContext_s *psCtx;

	psCtx = kmalloc( sizeof( SemContext_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );
	if ( psCtx != NULL )
	{
		psCtx->sc_nLastID = 1;
		psCtx->sc_nAllocCount = 1;
	}
	return ( psCtx );
}

/**
 * \par Description:
 * Clones a semaphore or reader-writer lock.
 */
static Semaphore_s *clone_semaphore( Semaphore_s *psSema, proc_id hNewOwner )
{
	Semaphore_s *psClone = NULL;

	switch ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) )
	{
	case SEMSTYLE_COUNTING:
		psClone = kmalloc( sizeof( Semaphore_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );
		break;
	case SEMSTYLE_RWLOCK:
		psClone = kmalloc( sizeof( RWLock_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );
		break;
	default:
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Asked to clone unknown semaphore style.\n" );
		break;
	}

	if ( psClone == NULL )
		return ( NULL );

	/* Copy semaphore fields */
	memcpy( psClone->ss_zName, psSema->ss_zName, OS_NAME_LENGTH );
	psClone->ss_hSemaID = psSema->ss_hSemaID;
	psClone->ss_lNestCount = psSema->ss_lNestCount;
	psClone->ss_nFlags = psSema->ss_nFlags;

	/* No one holds cloned semaphore. */
	psClone->ss_hHolder = -1;
	psClone->ss_hOwner = hNewOwner;

	/* Don't clone the extra fields for a R-W lock. */

	/* Increment global semaphore count */
	atomic_inc( &g_sSysBase.ex_nSemaphoreCount );

	return ( psClone );
}

/**
 * \par Description:
 * Copy the current semaphore context (containing process semaphores) for use
 * in a new process.
 *
 * This function is called by sys_Fork() as part of process forking.
 * All semaphore ownership information is lost at this time, as the old thread
 * ids will not make sense in the new process.
 */
SemContext_s *clone_semaphore_context( SemContext_s * psOrig, proc_id hNewOwner )
{
	SemContext_s *psCtx;
	uint32 nFlg;
	int nError;

	psCtx = kmalloc( sizeof( SemContext_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );
	if ( psCtx == NULL )
	{
		return ( NULL );
	}

      retry:
	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psCtx->sc_nLastID = psOrig->sc_nLastID;
	psCtx->sc_nAllocCount = psOrig->sc_nAllocCount;
	psCtx->sc_nMaxCount = psOrig->sc_nMaxCount;

	nError = 0;
	if ( psCtx->sc_nMaxCount > 0 )
	{
		uint8 *pAllocMap = kmalloc( psCtx->sc_nMaxCount * 4 + psCtx->sc_nMaxCount / 8,
			MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );
		int i;

		if ( pAllocMap == NULL )
		{
			nError = -ENOMEM;
			goto error;
		}
		memcpy( pAllocMap + psOrig->sc_nMaxCount * 4, psOrig->sc_pnAllocMap, psOrig->sc_nMaxCount / 8 );
		psCtx->sc_apsSemaphores = ( Semaphore_s ** )pAllocMap;
		psCtx->sc_pnAllocMap = ( uint32 * )( pAllocMap + psCtx->sc_nMaxCount * 4 );

		for ( i = 0; i < psCtx->sc_nMaxCount; ++i )
		{
			if ( psOrig->sc_apsSemaphores[i] != NULL )
			{
				psCtx->sc_apsSemaphores[i] = clone_semaphore( psOrig->sc_apsSemaphores[i], hNewOwner );
				if ( psCtx->sc_apsSemaphores[i] == NULL )
				{
					nError = -ENOMEM;
					break;
				}
			}
		}
	}
      error:
	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	if ( nError < 0 )
	{
		if ( psCtx->sc_apsSemaphores != NULL )
		{
			int i;

			for ( i = 0; i < psCtx->sc_nMaxCount; ++i )
			{
				if ( psCtx->sc_apsSemaphores[i] != NULL )
				{
					kfree( psCtx->sc_apsSemaphores[i] );
					atomic_dec( &g_sSysBase.ex_nSemaphoreCount );
				}
			}
			kfree( psCtx->sc_apsSemaphores );
			psCtx->sc_apsSemaphores = NULL;
		}
		if ( nError == -ENOMEM )
		{
			shrink_caches( 65536 );
			goto retry;
		}
		else
		{
			kfree( psCtx );
			return ( NULL );
		}
	}

	return ( psCtx );
}

void update_semaphore_context( SemContext_s * psCtx, proc_id hOwner )
{
	int i;

	for ( i = 0; i < psCtx->sc_nMaxCount; ++i )
	{
		if ( psCtx->sc_apsSemaphores[i] != NULL )
		{
			psCtx->sc_apsSemaphores[i]->ss_hOwner = hOwner;
		}
	}
}

/**
 * \par Description:
 * Expands the allocation map used for accessing per-process semaphores.
 */
static status_t expand_sem_context( SemContext_s * psCtx )
{
	int nNewCount = ( psCtx->sc_nMaxCount == 0 ) ? 256 : ( psCtx->sc_nMaxCount << 1 );
	uint8 *pAllocMap = kmalloc( nNewCount * 4 + nNewCount / 8, MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );

	if ( pAllocMap == NULL )
	{
		return ( -ENOMEM );
	}
	if ( psCtx->sc_nMaxCount != 0 )
	{
		memcpy( pAllocMap, psCtx->sc_apsSemaphores, psCtx->sc_nMaxCount * 4 );
		memcpy( pAllocMap + nNewCount * 4, psCtx->sc_pnAllocMap, psCtx->sc_nMaxCount / 8 );
		kfree( psCtx->sc_apsSemaphores );
	}
	psCtx->sc_apsSemaphores = ( Semaphore_s ** )pAllocMap;
	psCtx->sc_pnAllocMap = ( uint32 * )( pAllocMap + nNewCount * 4 );
	psCtx->sc_nMaxCount = nNewCount;
	return ( 0 );
}

/**
 * \par Description:
 * Inserts a new semaphore into the global semaphore array (if bGlobal), the
 * kernel semaphore context (if bKernel), or the current process semaphore
 * context (otherwise).
 *
 * This function may only be called while the semaphore list lock is held.
 *
 * \return The new semaphore id is returned, combined with the appropriate
 * type bits.  If an error occurs the result will be a negated errno error
 * code.
 */
static sem_id add_semaphore( bool bKernel, Semaphore_s *psSema, bool bGlobal )
{
	SemContext_s *psCtx;
	sem_id hHandle;

	if ( bGlobal )
	{
		hHandle = MArray_Insert( &g_sSemaphores, psSema, true );
		if ( hHandle < 0 )
		{
			return ( hHandle );
		}
		else
		{
			return ( hHandle | SEMTYPE_GLOBAL );
		}
	}

	psCtx = ( bKernel ) ? g_psKernelSemContext : CURRENT_PROC->pr_psSemContext;

	if ( psCtx->sc_nAllocCount * 2 > psCtx->sc_nMaxCount )
	{
		expand_sem_context( psCtx );
	}

	if ( psCtx->sc_nAllocCount == psCtx->sc_nMaxCount )
	{
		return ( -ENOMEM );
	}
	while ( GETBIT( psCtx->sc_pnAllocMap, psCtx->sc_nLastID ) )
	{
		if ( ++psCtx->sc_nLastID >= psCtx->sc_nMaxCount )
		{
			psCtx->sc_nLastID = 1;
		}
	}
	hHandle = psCtx->sc_nLastID++;
	if ( psCtx->sc_nLastID >= psCtx->sc_nMaxCount )
	{
		psCtx->sc_nLastID = 1;
	}
	SETBIT( psCtx->sc_pnAllocMap, hHandle, true );
	psCtx->sc_apsSemaphores[hHandle] = psSema;
	psCtx->sc_nAllocCount++;

	if ( hHandle == 0 )
	{
		panic( "add_semaphore() got semaphore handle equal 0\n" );
	}
	if ( hHandle >= 0 && bKernel )
	{
		hHandle |= SEMTYPE_KERNEL;
	}
	return ( hHandle );
}

/**
 * \par Description:
 * Removes a semaphore from its context (global, kernel or process).
 */
static void remove_semaphore( sem_id hSema )
{
	if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_GLOBAL )
	{
		MArray_Remove( &g_sSemaphores, hSema & SEM_ID_MASK );
	}
	else
	{
		SemContext_s *psCtx = ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_KERNEL ) ? g_psKernelSemContext : CURRENT_PROC->pr_psSemContext;

		psCtx->sc_apsSemaphores[hSema & SEM_ID_MASK] = NULL;
		SETBIT( psCtx->sc_pnAllocMap, hSema & SEM_ID_MASK, false );
		psCtx->sc_nAllocCount--;
		if ( psCtx->sc_nAllocCount < 1 )
		{
			printk( "Panic: remove_semaphore() sc_nAllocCount = %d (to low)\n", psCtx->sc_nAllocCount );
			psCtx->sc_nAllocCount = 1;
		}
	}
}

/**
 * \par Description:
 * Returns a pointer to the semaphore structure for the id hSema.
 * This function must only be executed while the semaphore list lock
 * is held.  The pointer returned is only valid while that lock
 * is still held.
 */
Semaphore_s *get_semaphore_by_handle( proc_id hProcess, sem_id hSema )
{
	if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_GLOBAL )
	{
		return ( MArray_GetObj( &g_sSemaphores, hSema & SEM_ID_MASK ) );
	}
	else
	{
		SemContext_s *psCtx;

		if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_KERNEL )
		{
			psCtx = g_psKernelSemContext;
		}
		else if ( hProcess == -1 )
		{
			psCtx = CURRENT_PROC->pr_psSemContext;
		}
		else
		{
			Process_s *psProc = get_proc_by_handle( hProcess );

			if ( psProc == NULL )
			{
				return ( NULL );
			}
			else
			{
				psCtx = psProc->pr_psSemContext;
			}
		}
		hSema &= SEM_ID_MASK;
		if ( hSema >= 0 && hSema < psCtx->sc_nMaxCount )
		{
			return ( psCtx->sc_apsSemaphores[hSema] );
		}
		else
		{
			return ( NULL );
		}
	}
}


/** Create a semaphore.
 * \ingroup DriverAPI
 * \ingroup SysCalls
 * \par Description:
 *	Create a kernel semaphore. Since AtheOS is heavily
 *	multithreaded semaphores are a frequently used object both in
 *	user-space and inside the kernel. Semaphores are used for
 *	various types of syncronizations. Most often it is used to
 *	give mutual exclusive access to a shared resource but it can
 *	also be used as a wait-list (conditional variable) where
 *	threads add themself to the list waiting for a condition to be
 *	triggered and a external thread wake up one or more of the
 *	blocked threads.
 *
 *	The object returned by create_semaphore() can be used both as a
 *	counting semaphore, and as a condition variable. Whether it is
 *	used as a semaphore or a conditional variable depend on the functions
 *	used to acquire/release the semaphore.
 *
 *	A semaphore consist of a counter and a thread-list keeping
 *	track on threads waiting for the semaphore to be released.
 *
 *	When used as a counting semaphore the functions
 *	lock_semaphore() and unlock_semaphore() are used to acquire and
 *	release the semaphore. The counter is given a initial value
 *	through create_semaphore() and lock_semaphore() will decrease
 *	the counter and unlock_semaphore() will increase it. If
 *	lock_semaphore() cause the counter to be negative the calling
 *	thread will be blocked until the counter become positive
 *	again. If a call to unlock_semaphore() cause the counter to go
 *	from negative to positive the first thread in the wait-list
 *	will be waked up.
 *
 *	When used as a conditional variable the counter is ignored and
 *	the semaphore are controlled with sleep_on_sem(), wakeup_sem()
 *	and a few other functions. When a thread call sleep_on_sem()
 *	it will unconditionally be added to the semaphores thread-list
 *	and blocked. Another thread can then wake up the first or all
 *	threads in the list with wakeup_sem().
 *	
 * \param pzName
 *	Like most other objects in AtheOS a semaphore is named. The name
 *	is mostly used for debugging and don't have to be unique.
 * \param nCount
 *	The initial nesting count. Normally this is 1, but can be other
 *	values if the semaphore is not used as a simple mutex. If the
 *	semaphore will be used as a conditional variable the count value
 *	is not used.
 * \param nFlags
 *	Various flags describing how to create the semaphore.
 *		- SEM_RECURSIVE	Don't block if the semaphore is
 *				acquired multiple times by the same
 *				thread.
 * \return
 *	On success a positive handle is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error -1 is returned "errno" will receive the error code.
 *	\endif
 * \sa	delete_semaphore(), lock_semaphore(), lock_semaphore_ex(),
 *	unlock_semaphore(), unlock_semaphore_ex(), sleep_on_sem(),
 *	wakeup_sem(), unlock_and_suspend()
 *	\if document_driver_api
 *	,spinunlock_and_suspend()
 *	\endif
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static sem_id do_create_semaphore( bool bKernel, const char *pzName, int nCount, uint32 nFlags )
{
	Semaphore_s *psSema;
	int nFlg;

	switch ( ( nFlags & SEM_STYLE_MASK ) )
	{
	case SEMSTYLE_COUNTING:
		psSema = kmalloc( sizeof( Semaphore_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );
		psSema->ss_hHolder = ( ( nFlags & SEM_RECURSIVE ) && nCount <= 0 ) ? CURRENT_THREAD->tr_hThreadID : -1;
		psSema->ss_lNestCount = nCount;
		break;
	case SEMSTYLE_RWLOCK:
		psSema = kmalloc( sizeof( RWLock_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );
		psSema->ss_hHolder = -1;
		psSema->ss_lNestCount = 0;
		break;
	default:
		return ( -EINVAL );
	}

	if ( psSema == NULL )
	{
		return ( -ENOMEM );
	}

	/* Shared initialisation */
	strncpy( psSema->ss_zName, pzName, OS_NAME_LENGTH );
	psSema->ss_zName[OS_NAME_LENGTH - 1] = '\0';
	psSema->ss_nFlags = nFlags;
	psSema->ss_hOwner = ( bKernel ) ? -1 : CURRENT_PROC->tc_hProcID;

	/* Try to insert into the appropriate semaphore context */
      again:
	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema->ss_hSemaID = add_semaphore( bKernel, psSema, ( nFlags & SEM_GLOBAL ) != 0 );

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	/* If there was not enough memory to expand the current context,
	 * shrink caches and try again.
	 */
	if ( psSema->ss_hSemaID == -ENOMEM )
	{
		shrink_caches( 65536 );
		goto again;
	}

	if ( psSema->ss_hSemaID >= 0 )
	{
		atomic_inc( &g_sSysBase.ex_nSemaphoreCount );
		return ( psSema->ss_hSemaID );
	}
	else
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Failed to insert new semaphore!\n" );
	}

	kfree( psSema );

	return ( -ENOMEM );
}

/**
 * \par Description:
 * Creates a semaphore for kernel space.
 * This function only creates counting semaphores.
 * \sa create_rwlock()
 */
sem_id create_semaphore( const char *pzName, int nCount, uint32 nFlags )
{
	return ( do_create_semaphore( true, pzName, nCount, nFlags ) );
}

/**
 * \par Description:
 * System call that creates a semaphore for userspace.
 * Global semaphores created by a process are inserted into a list
 * to make it easier to iterate over all process global semaphores 
 * at exit and exec.
 */
sem_id sys_create_semaphore( const char *pzName, int nCount, int nFlags )
{
	sem_id hSema;
	char zName[OS_NAME_LENGTH];
	int nError;

	nError = strncpy_from_user( zName, pzName, OS_NAME_LENGTH );

	if ( nError < 0 )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Failed to copy name\n" );
		return ( nError );
	}

	zName[OS_NAME_LENGTH - 1] = '\0';
	hSema = do_create_semaphore( false, zName, nCount, nFlags );

	if ( hSema >= 0 && ( nFlags & SEM_GLOBAL ) != 0 )
	{
		Process_s *psProc = CURRENT_PROC;
		Semaphore_s *psSema;
		int nFlg = spinlock_disable( &g_sSemListSpinLock );

		psSema = get_semaphore_by_handle( -1, hSema );

		if ( psSema != NULL )
		{
			if ( psProc->pr_psLastSema != NULL )
			{
				psProc->pr_psLastSema->ss_psNext = psSema;
			}
			if ( psProc->pr_psFirstSema == NULL )
			{
				psProc->pr_psFirstSema = psSema;
			}
			psSema->ss_psNext = NULL;
			psSema->ss_psPrev = psProc->pr_psLastSema;
			psProc->pr_psLastSema = psSema;
			psProc->pr_nSemaCount++;
		}
		spinunlock_enable( &g_sSemListSpinLock, nFlg );
	}

	return ( hSema );
}

/**
 * \par Description:
 * Removes a semaphore and disposes of the associated memory.
 */
static status_t do_delete_semaphore( sem_id hSema )
{
	Semaphore_s *psSema;
	int nFlg;

	if ( 0 == hSema )
	{
		printk( "Error : attempt to delete semaphore 0\n" );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );
	psSema = get_semaphore_by_handle( -1, hSema );
	if ( psSema != NULL )
	{
		remove_semaphore( hSema );
	}
	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	if ( psSema != NULL )
	{
		/* Wake up all waiting threads */
		wake_up_queue( true, psSema->ss_psWaitQueue, 0, true );

		/* Delete holder and waiter queues for reader-writer lock */
		if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) == SEMSTYLE_RWLOCK )
		{
			RWLock_s *psRWLock = ( RWLock_s * ) psSema;
			RWWaiter_s *psWaiter;
			RWHolder_s *psHolder;

			while ( ( psWaiter = psRWLock->rw_psWaiters ) != NULL )
			{
				psRWLock->rw_psWaiters = psWaiter->rww_psNext;

				wake_up_queue( true, psWaiter->rww_psWaitQueue, 0, true );

				kfree( psWaiter );
			}

			while ( ( psHolder = psRWLock->rw_psHolders ) != NULL )
			{
				psRWLock->rw_psHolders = psHolder->rwh_psNext;

				kfree( psHolder );
			}
		}

		kfree( psSema );
		atomic_dec( &g_sSysBase.ex_nSemaphoreCount );

		return ( 0 );
	}
	else
	{
		return ( -EINVAL );
	}
}

/** Delete a kernel semaphore.
 * \ingroup DriverAPI
 * \ingroup SysCalls
 * \par Description:
 *	Delete a semaphore previously created with create_semaphore().
 *	Resources consumed by the semaphore will be released and
 *	all threads currently blocked on the semaphore will be waked
 *	up and the blocking function will return an error code.
 * \param hSema
 *	Semaphore handle returned by a previous call to create_semaphore()
 * \return
 *	On success 0 is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error -1 is returned "errno" will receive the error code.
 *	\endif
 * \sa	create_semaphore(), lock_semaphore(), lock_semaphore_ex(),
 *	unlock_semaphore(), unlock_semaphore_ex(), sleep_on_sem(),
 *	wakeup_sem(), unlock_and_suspend()
 *	\if document_driver_api
 *	,spinunlock_and_suspend()
 *	\endif
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t delete_semaphore( sem_id hSema )
{
	return ( do_delete_semaphore( hSema ) );
}

/**
 * \par Description:
 * Remove the global semaphore psSema from the process semaphore list.
 */
static void unlink_sema( Process_s *psProc, Semaphore_s *psSema )
{
	kassertw( psProc->tc_hProcID == psSema->ss_hOwner );
//    psSema->ss_hOwner = -1;

	if ( NULL != psSema->ss_psNext )
	{
		psSema->ss_psNext->ss_psPrev = psSema->ss_psPrev;
	}
	if ( NULL != psSema->ss_psPrev )
	{
		psSema->ss_psPrev->ss_psNext = psSema->ss_psNext;
	}

	if ( psProc->pr_psFirstSema == psSema )
	{
		psProc->pr_psFirstSema = psSema->ss_psNext;
	}
	if ( psProc->pr_psLastSema == psSema )
	{
		psProc->pr_psLastSema = psSema->ss_psPrev;
	}

	psSema->ss_psNext = NULL;
	psSema->ss_psPrev = NULL;
	psProc->pr_nSemaCount--;

	if ( psProc->pr_nSemaCount < 0 )
	{
		panic( "sys_delete_semaphore() proc %s got sem-count of %d\n", psProc->tc_zName, psProc->pr_nSemaCount );
	}
}

/**
 * \par Description:
 * System call to delete a semaphore.
 */
status_t sys_delete_semaphore( sem_id hSema )
{
	Process_s *psProc = CURRENT_PROC;
	Semaphore_s *psSema;
	int nFlg;

	if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_KERNEL )
	{
		printk( "sys_delete_semaphore() attempt to delete kernel semaphore\n" );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );

	if ( psSema == NULL )
	{
		printk( "Error : attempt to delete invalid semaphore %d\n", hSema );
		spinunlock_enable( &g_sSemListSpinLock, nFlg );
		return ( -EINVAL );
	}

	if ( psProc->tc_hProcID != psSema->ss_hOwner )
	{
		printk( "Error: sys_delete_semaphore() attempt to delete semaphore %s not owned by us\n", psSema->ss_zName );
		spinunlock_enable( &g_sSemListSpinLock, nFlg );
		return ( -EPERM );
	}
	if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_GLOBAL )
	{
		unlink_sema( psProc, psSema );
	}
	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( do_delete_semaphore( hSema ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void exit_free_semaphores( Process_s *psProc )
{
	int i;

	while ( psProc->pr_psFirstSema != NULL )
	{
		int nFlg;
		Semaphore_s *psSema;
		sem_id hSema;

		nFlg = spinlock_disable( &g_sSemListSpinLock );
		psSema = psProc->pr_psFirstSema;
		hSema = psSema->ss_hSemaID;
		unlink_sema( psProc, psSema );

		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		do_delete_semaphore( hSema );
	}
	for ( i = 0; i < psProc->pr_psSemContext->sc_nMaxCount / 32; ++i )
	{
		if ( psProc->pr_psSemContext->sc_pnAllocMap[i] != 0 )
		{
			int j;
			uint32 nMask;

			for ( j = 0, nMask = 1; j < 32; ++j, nMask <<= 1 )
			{
				if ( ( psProc->pr_psSemContext->sc_pnAllocMap[i] & nMask ) != 0 )
				{
					do_delete_semaphore( i * 32 + j );
				}
			}
		}
	}
	kfree( psProc->pr_psSemContext->sc_apsSemaphores );
	kfree( psProc->pr_psSemContext );
	psProc->pr_psSemContext = NULL;
}

void exec_free_semaphores( Process_s *psProc )
{
	int i;

	while ( psProc->pr_psFirstSema != NULL )
	{
		int nFlg;
		Semaphore_s *psSema;
		sem_id hSema;

		nFlg = spinlock_disable( &g_sSemListSpinLock );
		psSema = psProc->pr_psFirstSema;
		hSema = psSema->ss_hSemaID;
		unlink_sema( psProc, psSema );

		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		do_delete_semaphore( hSema );
	}
	for ( i = 0; i < psProc->pr_psSemContext->sc_nMaxCount / 32; ++i )
	{
		if ( psProc->pr_psSemContext->sc_pnAllocMap[i] != 0 )
		{
			int j;
			uint32 nMask;

			for ( j = 0, nMask = 1; j < 32; ++j, nMask <<= 1 )
			{
				if ( ( psProc->pr_psSemContext->sc_pnAllocMap[i] & nMask ) != 0 )
				{
					do_delete_semaphore( i * 32 + j );
				}
			}
		}
	}
	if ( psProc->pr_psSemContext->sc_nMaxCount > 0 )
	{
		kfree( psProc->pr_psSemContext->sc_apsSemaphores );
	}

	psProc->pr_psSemContext->sc_apsSemaphores = NULL;
	psProc->pr_psSemContext->sc_pnAllocMap = NULL;
	psProc->pr_psSemContext->sc_nMaxCount = 0;
	psProc->pr_psSemContext->sc_nAllocCount = 1;
	psProc->pr_psSemContext->sc_nLastID = 1;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

#ifdef __DETECT_DEADLOCK

static bool detect_deadlock( int hSema );

static bool detect_deadlock_thread( int hThread )
{
	int i;

	Thread_s *psThread = get_thread_by_handle( hThread );


	if ( NULL == psThread )
	{
		printk( "Error : detect_deadlock_thread() called width invalid thread %x\n", hThread );
		return ( TRUE );
	}

	if ( psThread->tr_nDeadLockDetectRun == g_nDeadLockDetectRun )
	{
		return ( FALSE );
	}
	else
	{
		psThread->tr_nDeadLockDetectRun = g_nDeadLockDetectRun;
	}

	if ( TS_SLEEP == psThread->tr_nState )
	{			// don't bitch if the lock was taken with timeout
		return ( FALSE );
	}

	if ( hThread == sys_get_thread_id( NULL ) )
	{
		psThread->tr_nFlags |= TF_DEADLOCK;
		printk( "DeadLock detected on thread '%s' (%lx)\n", psThread->tr_zName, hThread );
		return ( TRUE );
	}

	if ( -1 != psThread->tr_hBlockSema )
	{
		if ( detect_deadlock( psThread->tr_hBlockSema ) )
		{
			printk( "DeadLock detected on thread '%s' (%lx)\n", psThread->tr_zName, hThread );
			return ( TRUE );
		}
	}

	for ( i = 0; i < 256; ++i )
	{
		if ( -1 != psThread->tr_ahObtainedSemas[i] )
		{
			if ( detect_deadlock( psThread->tr_ahObtainedSemas[i] ) )
			{
				printk( "DeadLock detected on thread '%s' (%lx)\n", psThread->tr_zName, hThread );
				return ( TRUE );
			}
		}
	}
	return ( FALSE );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static bool detect_deadlock( int hSema )
{
	Semaphore_s *psSema;


	if ( ( psSema = get_semaphore_by_handle( hSema ) ) )
	{
		WaitQueue_s *psWaitNode;
		BOOL bFirst = TRUE;

		if ( psSema->ss_nDeadLockDetectRun == g_nDeadLockDetectRun )
		{
			return ( FALSE );
		}
		else
		{
			psSema->ss_nDeadLockDetectRun = g_nDeadLockDetectRun;
		}

		if ( -1 != psSema->ss_hHolder )
		{
			if ( detect_deadlock_thread( psSema->ss_hHolder ) )
			{
				printk( "DeadLock detected on semaphore '%s' (%ld)\n", psSema->ss_zName, hSema );
				return ( TRUE );
			}
		}

		if ( NULL != psSema->ss_psWaitQueue )
		{
			for ( psWaitNode = psSema->ss_psWaitQueue; bFirst || psWaitNode != psSema->ss_psWaitQueue; psWaitNode = psWaitNode->wq_psNext )
			{
//                              Thread_s*       psThread = GetThreadAddr( psWaitNode->wq_hThread );

				bFirst = FALSE;

				if ( -1 != psWaitNode->wq_hThread )
				{
					if ( detect_deadlock_thread( psWaitNode->wq_hThread ) )
					{
						printk( "DeadLock detected on semaphore '%s' (%ld)\n", psSema->ss_zName, hSema );
						return ( TRUE );
					}
				}

			}
		}
	}
	else
	{
		printk( "ERROR : detect_deadlock() called with invalid semaphore %ld\n", hSema );
		return ( TRUE );
	}
	return ( FALSE );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void add_sem_to_thread( Thread_s *psThread, sem_id hSema )
{
	int i;

	for ( i = 0; i < 256; ++i )
	{
		if ( hSema == psThread->tr_ahObtainedSemas[i] )
		{
			break;
		}
	}
	if ( 256 == i )
	{
		for ( i = 0; i < 256; ++i )
		{
			if ( -1 == psThread->tr_ahObtainedSemas[i] )
			{
				psThread->tr_ahObtainedSemas[i] = hSema;
				break;
			}
		}
	}
	if ( 256 == i )
	{
		printk( "WARNING : More than 256 semaphores obtained by this thread\n" );
	}
}
#endif // __DETECT_DEADLOCK

/**
 * \par Description:
 * Releases all the kernel r-w semaphores currently held by this thread.
 */
void release_thread_semaphores( thread_id hThreadId )
{
	SemContext_s *psCtx = g_psKernelSemContext;
	Semaphore_s *psSema;
	RWLock_s *psRWLock;
	int i;

	for ( i = 0; i < psCtx->sc_nMaxCount / 32; ++i )
	{
		if ( psCtx->sc_pnAllocMap[i] != 0 )
		{
			int j;
			uint32 nMask;

			for ( j = 0, nMask = 1; j < 32; ++j, nMask <<= 1 )
			{
				if ( ( psSema = psCtx->sc_apsSemaphores[i * 32 + j] ) == NULL )
					continue;

				if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) == SEMSTYLE_RWLOCK )
				{
					RWWaiter_s **ppsWaiter;
					RWHolder_s **ppsHolder;
					int loops = 0;

					/* Look for hold & wait requests referring to the thread */
					psRWLock = ( RWLock_s * ) psSema;

					/* Examine all waiters on all wait nodes */
					for ( ppsWaiter = &psRWLock->rw_psWaiters; ( *ppsWaiter ) != NULL; )
					{
						WaitQueue_s *psWaitNode = ( *ppsWaiter )->rww_psWaitQueue;
						RWWaiter_s *psRemove;

						if ( loops++ > 100000 )
						{
							printk( "Too many loops for waiter.\n" );
							break;
						}

						if ( psWaitNode != NULL )
						{
							/* Iterate over wait queue */
							do
							{
								if ( loops++ > 100000 )
								{
									printk( "Too many loops for wait node.\n" );
									break;
								}

								if ( psWaitNode->wq_hThread == hThreadId )
								{
									kerndbg( KERN_DEBUG, __FUNCTION__ "(): Found thread id %d " "waiting on RWLock %d.\n", hThreadId, psSema->ss_hSemaID );
									// remove_from_waitlist( &(*ppsWaiter)->rww_psWaitQueue, psWaitNode );

									/* Break if removing the last node */
									// if( psWaitNode->wq_psNext == psWaitNode )
									//      break;
								}


								/* Ignore memory -- it is allocated on someone's stack */
								psWaitNode = psWaitNode->wq_psNext;
							}
							while ( psWaitNode != ( *ppsWaiter )->rww_psWaitQueue );
						}

						/* If waiter is now empty, remove it */
						if ( ( *ppsWaiter )->rww_psWaitQueue == NULL )
						{
							kerndbg( KERN_DEBUG, __FUNCTION__ "(): Removing empty waiter from sema id %d.\n", psSema->ss_hSemaID );
							psRemove = ( *ppsWaiter );
							( *ppsWaiter ) = psRemove->rww_psNext;

							kfree( psRemove );
						}
						else
						{
							ppsWaiter = &( *ppsWaiter )->rww_psNext;
						}
					}

					/* Examine all holders */
					loops = 0;
					for ( ppsHolder = &psRWLock->rw_psHolders; ( *ppsHolder ) != NULL; )
					{
						RWHolder_s *psRemove;

						if ( loops++ > 100000 )
						{
							kerndbg( KERN_DEBUG, "Too many loops looking for holders.\n" );
							break;
						}

						if ( ( *ppsHolder )->rwh_hThreadId == hThreadId )
						{
							psRemove = ( *ppsHolder );
							( *ppsHolder ) = psRemove->rwh_psNext;

							kerndbg( KERN_DEBUG, __FUNCTION__ "(): Thread %d forgot to release r-w lock %d (%ld read, %ld write)!\n", hThreadId, psSema->ss_hSemaID, psRemove->rwh_nReadCount, psRemove->rwh_nWriteCount );

							kfree( psRemove );

							if ( psRWLock->rw_psHolders == NULL && psRWLock->rw_psWaiters != NULL )
							{
								/* We just removed the only holder -- wake up the next queue node */
								wake_up_queue( true, psRWLock->rw_psWaiters->rww_psWaitQueue, 0, true );
							}
						}
						else
						{
							ppsHolder = &( *ppsHolder )->rwh_psNext;
						}
					}
				}
			}
		}
	}
}


static status_t do_lock_semaphore_ex( bool bKernel, sem_id hSema, int nCount, uint32 nFlags, bigtime_t nTimeOut )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	thread_id hMyThread = psMyThread->tr_hThreadID;
	int nFlg;
	int nError;
	bigtime_t nResumeTime;
	bool bDumpStack = true;
	
	if ( 0 == hSema )
	{
		printk( "Error : attempt to lock semaphore 0\n" );
//      trace_stack( 0, NULL );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	if ( atomic_read(&g_sSemListSpinLock.sl_nNest) != 1 )
	{
		printk( "Panic: do_lock_semaphore_ex() spinlock nest count is %d, dumping call-stack...\n", atomic_read(&g_sSemListSpinLock.sl_nNest) );
		bDumpStack = true;
	}

	nResumeTime = get_system_time() + nTimeOut;

	for ( ;; )
	{
		Semaphore_s *psSema;

		nError = 0;

		psSema = get_semaphore_by_handle( -1, hSema );

		if ( NULL == psSema )
		{
			printk( "Error : do_lock_semaphore_ex( %d ) called with invalid semaphore id!!\n", hSema );
			bDumpStack = true;
			nError = -EINVAL;
			break;
		}
	

		if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
		{
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style (id %d).\n", hSema );
			nError = -EINVAL;
			break;
		}

		if ( psSema->ss_lNestCount - nCount >= 0 || ( ( psSema->ss_nFlags & SEM_RECURSIVE ) && psSema->ss_hHolder == hMyThread ) )
		{
			psSema->ss_lNestCount -= nCount;
			psSema->ss_hHolder = hMyThread;
			nError = 0;

#ifdef __DETECT_DEADLOCK
			if ( ( psSema->ss_nFlags & SEM_RECURSIVE ) && INFINITE_TIMEOUT == nTimeOut )
			{
				add_sem_to_thread( psMyThread, hSema );
			}
#endif
			break;
		}
		else
		{
			WaitQueue_s sSleepNode;
			WaitQueue_s sWaitNode;
			bigtime_t nRestDelay = 0;


			if ( ( psSema->ss_nFlags & SEM_WARN_DBL_LOCK ) && psSema->ss_hHolder == hMyThread )
			{
				printk( "Warning: %s semaphore %s(%d) taken twice\n", ( ( psSema->ss_nFlags & SEM_RECURSIVE ) ? "Recursive" : "None recursive" ), psSema->ss_zName, hSema );
				bDumpStack = true;
			}

			if ( 0 == nTimeOut )
			{
				nError = -EWOULDBLOCK;
				break;
			}
			
			sched_lock();
			sWaitNode.wq_hThread = hMyThread;
			psMyThread->tr_hBlockSema = hSema;

			if ( INFINITE_TIMEOUT == nTimeOut )
			{
#ifdef __DETECT_DEADLOCK
				if ( psSema->ss_nFlags & SEM_RECURSIVE )
				{
					detect_deadlock( hSema );
				}
				g_nDeadLockDetectRun++;
#endif
				psMyThread->tr_nState = TS_WAIT;
			}
			else
			{
				if ( get_system_time() > nResumeTime )
				{
					nError = -ETIME;
					sched_unlock();
					break;
				}
				else
				{
					psMyThread->tr_nState = TS_SLEEP;
					sSleepNode.wq_bIsMember = false;
					sSleepNode.wq_hThread = hMyThread;
					sSleepNode.wq_nResumeTime = nResumeTime;
					add_to_sleeplist( false, &sSleepNode );
				}
			}
			add_to_waitlist( false, &psSema->ss_psWaitQueue, &sWaitNode );
			sched_unlock();
			spinunlock_enable( &g_sSemListSpinLock, nFlg );

			Schedule();

			nFlg = spinlock_disable( &g_sSemListSpinLock );

			if ( INFINITE_TIMEOUT != nTimeOut )
			{
				remove_from_sleeplist( &sSleepNode );
			}

	      /*** We must make sure nobody deleted the semaphore while we snoozed! ***/

			psSema = get_semaphore_by_handle( -1, hSema );

			if ( NULL == psSema )
			{
				printk( "Error : do_lock_semaphore_ex( %d ) semaphore deleted while we snoozed!!\n", hSema );
				nError = -EINVAL;
				break;
			}
			remove_from_waitlist( true, &psSema->ss_psWaitQueue, &sWaitNode );

			nRestDelay = nResumeTime - get_system_time();
			psMyThread->tr_hBlockSema = -1;

			if ( ( nFlags & SEM_NOSIG ) == 0 && is_signal_pending() )
			{
				nError = -EINTR;
			}
		}
		if ( 0 != nError )
		{

	      /*** If we give up we better tell the next one to give it a try	***/
			wake_up_queue( true, psSema->ss_psWaitQueue, 0, false );
			break;
		}
	}
	spinunlock_enable( &g_sSemListSpinLock, nFlg );

//    if ( bDumpStack ) {
//      trace_stack( 0, NULL );
//    }
	return ( nError );
}

/** Acquire a semaphore.
 * \ingroup DriverAPI
 * \ingroup SysCalls
 * \par Description:
 *	Acquire a semaphore. lock_semaphore_ex() will decrease the
 *	semaphore's nesting counter with "nCount".
 *
 *	If the timeout is larger than 0 and the nesting counter
 *	becomes negative the calling thread will be blocked.
 *	The thread will then stay blocked until one of the
 *	following conditions are met:
 *
 *		- Another thread release the semaphore and cause
 *		  the nesting counter to become positive again
 *		  and the function returns successfully.
 *		- The timeout runs out and the function returns
 *		  with an ETIME error.
 *		- The calling threads catch a signal and the
 *		  function return the EINTR error code.
 *
 *	If the timeout is 0 lock_semaphore_ex() will never block, but
 *	instead returns immediatly with ETIME if the semaphore can
 *	not be acquired. If the timeout is INFINITE_TIMEOUT the thread
 *	will be blocked until it can be successfully acquired or a
 *	signal is caught.
 *
 * \if document_driver_api
 * \par Warning:
 *	Since lock_semaphore_ex() called with a non-0 timeout might
 *	block it must never be called from a interrupt
 *	handler. Non-atomic resources accessed from interrupt handlers
 *	must be protected with spinlocks.
 * \endif
 * \param hSema
 *	Semaphore handle to lock.
 * \param nCount
 *	Positive value that should be subtracted from the current
 *	nesting counter.
 * \param nFlags
 * \if document_syscalls
 *	No flags currently defined
 * \endif
 * \if document_driver_api
 *		- SEM_NOSIG Ignore signals while blocking on the semaphore.
 *			    Only set this flag on semaphores that are known
 *			    to be released realtively soon. Otherwhise the
 *			    caller should handle the EINTR error and exit
 *			    from the kernel as soon as possible if a signal
 *			    is caught.
 * \endif
 * \param nTimeOut
 *	Maximum number of micro seconds to wait for the semaphore to
 *	be released. If the timeout is 0 lock_semaphore_ex() will
 *	never return, and if it is INFINITE_TIMEOUT it will block
 *	until the semaphore can be acquired or a signel is caught.
 * \return
 *	On success 0 is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error -1 is returned "errno" will receive the error code.
 *	\endif
 *
 * \par Error codes:
 *	- \b ETIME  the semaphore could not be acquired before the timeout expiered.
 *	- \b EINTR  a signal was caught before the semaphore could be acquired.
 *	- \b EINVAL hSema was not a valid semaphore handle, or the semaphore
 *		    was deleted while we blocked.
 *
 * \sa	unlock_semaphore_ex(), lock_semaphore(), unlock_semaphore_ex(),
 *	create_semaphore(),  delete_semaphore(), unlock_semaphore(),
 *	unlock_semaphore_ex(), sleep_on_sem(), wakeup_sem(),
 *	unlock_and_suspend()
 *	\if document_driver_api
 *	spinunlock_and_suspend()
 *	\endif
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t lock_semaphore_ex( sem_id hSema, int nCount, uint32 nFlags, bigtime_t nTimeOut )
{
	return ( do_lock_semaphore_ex( true, hSema, nCount, nFlags, nTimeOut ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t lock_mutex( sem_id hSema, bool bIgnoreSignals )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	thread_id hMyThread = psMyThread->tr_hThreadID;
	Semaphore_s *psSema;
	int nFlg;
	int nError = 0;

	if ( 0 == hSema )
	{
		printk( "Error : lock_mutex() attempt to lock semaphore 0\n" );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );
	psSema = get_semaphore_by_handle( -1, hSema );

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );

		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		return ( -EINVAL );
	}

	while ( psSema->ss_lNestCount <= 0 )
	{
		WaitQueue_s sWaitNode;

		sWaitNode.wq_hThread = hMyThread;
		sched_lock();
		psMyThread->tr_hBlockSema = hSema;
		psMyThread->tr_nState = TS_WAIT;

		add_to_waitlist( false, &psSema->ss_psWaitQueue, &sWaitNode );
		sched_unlock();
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		Schedule();

		nFlg = spinlock_disable( &g_sSemListSpinLock );

	  /*** We must make sure nobody deleted the semaphore while we snoozed! ***/

		psSema = get_semaphore_by_handle( -1, hSema );

		if ( NULL == psSema )
		{
			printk( "Error : lock_mutex( %d ) semaphore deleted while we snoozed!!\n", hSema );
			nError = -EINVAL;
			break;
		}
		remove_from_waitlist( true, &psSema->ss_psWaitQueue, &sWaitNode );

		psMyThread->tr_hBlockSema = -1;

		if ( bIgnoreSignals == false && is_signal_pending() )
		{
			nError = -EINTR;
			break;
		}
		if ( 0 != nError )
		{

	      /*** If we give up we better tell the next one to give it a try	***/
			wake_up_queue( true, psSema->ss_psWaitQueue, 0, false );
			break;
		}
	}
	if ( nError == 0 )
	{
		psSema->ss_lNestCount--;
	}
	spinunlock_enable( &g_sSemListSpinLock, nFlg );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t unlock_mutex( sem_id hSema )
{
	Semaphore_s *psSema;
	int nFlg;

	if ( 0 == hSema )
	{
		printk( "Error : attempt to unlock semaphore 0\n" );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	if ( psSema == NULL )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );
		printk( "Error : ReleaseSemaphore() called with invalid semaphore %d!!\n", hSema );
		return ( -EINVAL );
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );

		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		return ( -EINVAL );
	}

	psSema->ss_lNestCount++;

	if ( psSema->ss_lNestCount > 0 )
	{
		psSema->ss_hHolder = -1;
		wake_up_queue( true, psSema->ss_psWaitQueue, 0, false );
	}
	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( 0 );
}

/** Lock a semaphore.
 * \ingroup DriverAPI
 * \ingroup SysCalls
 * \par Description:
 *	Locks a semaphore. Equivalent of calling lock_semaphore_ex()
 *	with a count of 1.
 *
 * \param hSema
 *	Semaphore handle to lock.
 * \param nFlags
 * \if document_syscalls
 *	No flags currently defined
 * \endif
 * \if document_driver_api
 *		- SEM_NOSIG Ignore signals while blocking on the semaphore.
 *			    Only set this flag on semaphores that are known
 *			    to be released realtively soon. Otherwhise the
 *			    caller should handle the EINTR error and exit
 *			    from the kernel as soon as possible if a signal
 *			    is caught.
 * \endif
 * \param nTimeOut
 *	Maximum number of micro seconds to wait for the semaphore to
 *	be released. If the timeout is 0 lock_semaphore_ex() will
 *	never return, and if it is INFINITE_TIMEOUT it will block
 *	until the semaphore can be acquired or a signel is caught.
 * \return
 *	On success 0 is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error -1 is returned "errno" will receive the error code.
 *	\endif
 *
 * \par Error codes:
 *	- \b ETIME  the semaphore could not be acquired before the timeout expiered.
 *	- \b EINTR  a signal was caught before the semaphore could be acquired.
 *	- \b EINVAL hSema was not a valid semaphore handle, or the semaphore
 *		    was deleted while we blocked.
 *
 * \sa lock_semaphore_ex()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t lock_semaphore( sem_id hSema, uint32 nFlags, bigtime_t nTimeOut )
{
	return ( lock_semaphore_ex( hSema, 1, nFlags, nTimeOut ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_lock_semaphore( sem_id hSema )
{
	if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_KERNEL )
	{
		printk( "sys_lock_semaphore() attempt to lock kernel semaphore\n" );
		return ( -EINVAL );
	}

	if ( 0 == hSema )
	{
		printk( "Error : sys_lock_semaphore() attempt to lock semaphore 0\n" );
		return ( -EINVAL );
	}

	return ( do_lock_semaphore_ex( false, hSema, 1, 0, INFINITE_TIMEOUT ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_raw_lock_semaphore_x( sem_id hSema, int nCount, uint32 nFlags, const bigtime_t *pnTimeOut )
{
	bigtime_t nTimeOut = ( NULL != pnTimeOut ) ? *pnTimeOut : INFINITE_TIMEOUT;

	if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_KERNEL )
	{
		printk( "sys_raw_lock_semaphore_x() attempt to lock kernel semaphore\n" );
		return ( -EINVAL );
	}

	if ( 0 == hSema )
	{
		printk( "Error : sys_raw_lock_semaphore_x() attempt to lock semaphore 0\n" );
		return ( -EINVAL );
	}

	return ( do_lock_semaphore_ex( false, hSema, nCount, nFlags, nTimeOut ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t do_unlock_semaphore_ex( bool bKernel, sem_id hSema, int nCount )
{
	Thread_s *psThread = CURRENT_THREAD;
	Semaphore_s *psSema;
	int nFlg;

	if ( 0 == hSema )
	{
		printk( "Error : attempt to unlock semaphore 0\n" );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	if ( psSema == NULL )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Called with invalid semaphore %d!\n", hSema );

		return ( -EINVAL );
	}
	

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );

		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		return ( -EINVAL );
	}

	if ( ( psSema->ss_nFlags & SEM_RECURSIVE ) || ( psSema->ss_nFlags & SEM_WARN_DBL_UNLOCK ) )
	{
		if ( psThread->tr_hThreadID != psSema->ss_hHolder )
		{
			if ( psThread != CURRENT_THREAD )
			{
				printk( "PANIC: We changed identity! Before spinlock : %s(%d) after: %s(%d)\n", psThread->tr_zName, psThread->tr_hThreadID, CURRENT_THREAD->tr_zName, CURRENT_THREAD->tr_hThreadID );
			}
			kerndbg( KERN_DEBUG, "Prev CPU: %d Cur CPU: %d\n", psThread->tr_nCurrentCPU, CURRENT_THREAD->tr_nCurrentCPU );
			kerndbg( KERN_DEBUG, "APIC ID=%d Lock Flg=%d Lock nest=%d Lock owner=%d\n", get_processor_id(), g_sSchedSpinLock.sl_nLocked, g_sSchedSpinLock.sl_nNest, g_sSchedSpinLock.sl_nProc );
			kerndbg( KERN_DEBUG, "Error: unlock_semaphore_ex() sem %s (%d) not owned by us (%d) but by %d\n", psSema->ss_zName, hSema, psThread->tr_hThreadID, psSema->ss_hHolder );
		}
	}

	psSema->ss_lNestCount += nCount;

	if ( psSema->ss_lNestCount > 0 )
	{
		psSema->ss_hHolder = -1;

		if ( psSema->ss_lNestCount > 1 && psSema->ss_nFlags & SEM_WARN_DBL_UNLOCK )
		{
			printk( "Error: unlock_semaphore_ex() sem %s (%d) got count of %d\n", psSema->ss_zName, hSema, psSema->ss_lNestCount );
		}

#ifdef __DETECT_DEADLOCK
		if ( psSema->ss_nFlags & SEM_RECURSIVE )
		{
			for ( i = 0; i < 256; ++i )
			{
				if ( psThread->tr_ahObtainedSemas[i] == hSema )
				{
					psThread->tr_ahObtainedSemas[i] = -1;
					break;
				}
			}
		}
#endif // __DETECT_DEADLOCK
		wake_up_queue( true, psSema->ss_psWaitQueue, 0, nCount > 1 );
	}
	spinunlock_enable( &g_sSemListSpinLock, nFlg );
	return ( 0 );
}

/** Release a semaphore.
 * \ingroup DriverAPI
 * \ingroup SysCalls
 * \par Description:
 *	Release a semaphore "nCount" number of times. The value of "nCount"
 *	will be added to the semaphores nesting counter and if the counter
 *	become positive (>=0) the first thread blocking on the semaphore
 *	will be unblocked.
 * \par Note:
 *	If "nCount" is larger than 1 all blocked threads will be
 *	unblocked and they will then compeat for the semaphore based
 *	on their scheduler priority. If "nCount" is 1 only the first
 *	blocked thread will be unblocked.
 *	
 * \param hSema
 *	The semaphore to release.
 * \param nCount
 *	Number of times to release the semaphore.
 * \return
 *	On success 0 is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error -1 is returned "errno" will receive the error code.
 *	\endif
 * \par Error codes:
 *	- EINVAL hSema is not a valid semaphore handle.
 * \sa unlock_semaphore(), lock_semaphore_ex()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t unlock_semaphore_ex( sem_id hSema, int nCount )
{
	return ( do_unlock_semaphore_ex( true, hSema, nCount ) );
}

/** Release a semaphore.
 * \ingroup DriverAPI
 * \ingroup SysCalls
 * \par Description:
 *	Same as unlock_semaphore_x() with a count of 1.
 * \sa unlock_semaphore_x()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


status_t unlock_semaphore( sem_id hSema )
{
	return ( unlock_semaphore_ex( hSema, 1 ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_unlock_semaphore_x( sem_id hSema, int nCount, uint32 nFlags )
{
	if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_KERNEL )
	{
		printk( "sys_unlock_semaphore_x() attempt to unlock kernel semaphore\n" );
		return ( -EINVAL );
	}

	return ( do_unlock_semaphore_ex( false, hSema, nCount ) );
}

/** Unlock a reqular semaphore and block on a wait-queue.
 * \ingroup DriverAPI
 * \par Description:
 *	Unlocks an regular semaphore, and sleep on a wait-queue in
 *	one atomic operation.
 *	No other thread's will be able to lock the semaphore until
 *	we are safely added to the wait-queue.
 *	This is useful for example when waiting for data to arrive
 *	to an object, since it can guaranty that nobody can lock
 *	the object, and attempt to wake up sleeper's while you are
 *	in the gap between unlocking the object and blocking on
 *	the wait-queue.
 * \param hWaitQueue
 *	Handle to the wait-queue to block on (created earlier with
 *	create_semaphore())
 * \param hSema
 *	Semaphore to release.
 *
 * \return
 *	0 on success or a negative error code on failure.
 * \sa unlock_semaphore(), sleep_on_sem(), spinunlock_and_suspend()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t unlock_and_suspend( sem_id hWaitQueue, sem_id hSema )
{
	Semaphore_s *psWaitQueue, *psSema;
	Thread_s *psThread = CURRENT_THREAD;
	WaitQueue_s sWaitNode;
	int nError;
	int nFlg;

	sWaitNode.wq_hThread = psThread->tr_hThreadID;

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psWaitQueue = get_semaphore_by_handle( -1, hWaitQueue );
	if ( psWaitQueue == NULL )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Called with invalid waitqueue ID %d\n", hWaitQueue );
		nError = -EINVAL;
		goto error;
	}

	if ( ( psWaitQueue->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect wait queue style.\n" );

		nError = -EINVAL;
		goto error;
	}

	psSema = get_semaphore_by_handle( -1, hSema );
	if ( psSema == NULL )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Called with invalid semaphore %d!\n", hSema );

		nError = -EINVAL;
		goto error;
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );

		nError = -EINVAL;
		goto error;
	}

	sched_lock();
	psThread->tr_nState = TS_WAIT;
	psThread->tr_hBlockSema = hWaitQueue;

	add_to_waitlist( false, &psWaitQueue->ss_psWaitQueue, &sWaitNode );
	sched_unlock();
	// NOTE: Leave interrupt's disabled, we don't want to be pre-empted
	//       until the semaphore is unlocked.

	spinunlock( &g_sSemListSpinLock );

	// unlock_semaphore() might call schedule(), but that should be ok
	// as long as we already is added to the wait-list, and the
	// thread state is set to TS_WAIT.

	nError = unlock_semaphore( hSema );

	put_cpu_flags( nFlg );

	Schedule();

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	remove_from_waitlist( true, &psWaitQueue->ss_psWaitQueue, &sWaitNode );

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	if ( nError == 0 && is_signal_pending() )
	{
		return ( -EINTR );
	}
	else
	{
		return ( nError );
	}
      error:
	spinunlock_enable( &g_sSemListSpinLock, nFlg );
	return ( nError );
}

/** Release a spinlock and block on a wait-queue in one atomic operation.
 * \ingroup DriverAPI
 * \par Description:
 *	Same as unlock_and_suspend() except that it releases a spinlock
 *	instead of a regular semaphore. This is usefull for device-drivers
 *	that receive or transmitt data through interrupt handlers, and then
 *	need to syncronize this with threads that deliver or use the data.
 * \par Note:
 *	Interrupts *MUST* be disabled prior to calling this function.
 *	The CPU flags will be restored to the value of nCPUFlags before
 *	returning unless an error (other than receiving a signal) occured.
 *
 *	This function can only be called from the kernel, or from device
 *	drivers. From user-space you should use unlock_and_suspend()
 * \par Warning:
 * \param hWaitQueue
 *	Handle to the wait-queue to block on (created earlier with
 *	create_semaphore()).
 * \param psLock
 *	Spinlock to release.
 * \param nCPUFlags
 *	The CPU flags to restore before returning.
 * \param nTimeOut
 *	Maximum number of microseconds to wait for someone to wake us
 *	up before ETIME is returned.
 * \return
 *	On success 0 is returned.
 *	On error a negative error code is returned.
 * \sa unlock_and_suspend()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t spinunlock_and_suspend( sem_id hWaitQueue, SpinLock_s * psLock, uint32 nCPUFlags, bigtime_t nTimeOut )
{
	Semaphore_s *psWaitQueue;
	Thread_s *psThread = CURRENT_THREAD;
	WaitQueue_s sWaitNode;
	WaitQueue_s sSleepNode;
	bigtime_t nResumeTime = get_system_time() + nTimeOut;
	int nError;

	sWaitNode.wq_hThread = psThread->tr_hThreadID;
	sSleepNode.wq_hThread = psThread->tr_hThreadID;

	// We just have to pray that the callee remembered to disable interrupts
	spinlock( &g_sSemListSpinLock );

	psWaitQueue = get_semaphore_by_handle( -1, hWaitQueue );
	if ( psWaitQueue == NULL )
	{
		printk( "Error: unlock_and_suspend() called with invalid waitqueue ID %d\n", hWaitQueue );
		nError = -EINVAL;
		spinunlock( psLock );
		goto error;
	}

	if ( ( psWaitQueue->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect wait queue style.\n" );

		nError = -EINVAL;
		spinunlock( psLock );
		goto error;
	}
	sched_lock();
	psThread->tr_hBlockSema = hWaitQueue;

	if ( nTimeOut == INFINITE_TIMEOUT )
	{
		psThread->tr_nState = TS_WAIT;
	}
	else
	{
		if ( get_system_time() > nResumeTime )
		{
			nError = -ETIME;
			sched_unlock();
			spinunlock( psLock );
			goto error;
		}
		else
		{
			psThread->tr_nState = TS_SLEEP;
			sSleepNode.wq_bIsMember = false;
			sSleepNode.wq_hThread = -1;
			sSleepNode.wq_nResumeTime = nResumeTime;
			add_to_sleeplist( false, &sSleepNode );
		}
	}
	add_to_waitlist( false, &psWaitQueue->ss_psWaitQueue, &sWaitNode );
	sched_unlock();
	// NOTE: Leave interrupt's disabled, we don't want to be pre-empted
	//             until the spinlock is released.

	spinunlock( &g_sSemListSpinLock );
	spinunlock( psLock );

	put_cpu_flags( nCPUFlags );

	Schedule();

	cli();
	spinlock( &g_sSemListSpinLock );

	if ( INFINITE_TIMEOUT != nTimeOut )
	{
		remove_from_sleeplist( &sSleepNode );
	}

	psWaitQueue = get_semaphore_by_handle( -1, hWaitQueue );
	if ( psWaitQueue == NULL )
	{
		printk( "Error: unlock_and_suspend() waitqueue %d deleted while we waited for it\n", hWaitQueue );
		nError = -EINVAL;
		goto error;
	}

	remove_from_waitlist( true, &psWaitQueue->ss_psWaitQueue, &sWaitNode );

	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nCPUFlags );

	if ( is_signal_pending() )
	{
		return ( -EINTR );
	}
	else if ( nTimeOut != INFINITE_TIMEOUT && get_system_time() > nResumeTime )
	{
		return ( -ETIME );
	}
	else
	{
		return ( 0 );
	}

      error:
	spinunlock( &g_sSemListSpinLock );
	return ( nError );
}

/** Reset the count on the given semaphore
 * \ingroup DriverAPI
 * \par Description:
 * Reset the count of the given semaphore.  The semaphore must
 * be a SEMSTYLE_COUNTING type semaphore.
 *
 * \par Warning
 * Calling this function when a thread has locked the semaphore
 * or is sleeping on the semaphore is dangerous.
 * \param hSema
 * The semaphore to modify.
 * \param nCount
 * The new count for the semaphore.
 * \return
 * On success 0 is returned.
 * On error a negative error code is returned.
 * \par Error codes:
 *  - \b EINVAL hSema os not a valid semaphore handle.
 * \sa lock_semaphore(), unlock_semaphore()
 * \author Kristian Van Der Vliet (vanders@liqwyd.com)
 *****************************************************************************/
 
status_t reset_semaphore( sem_id hSema, int nCount )
{
	Semaphore_s *psSema;
	int nFlg;

	if ( 0 == hSema )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Attempt to set count for semaphore 0\n" );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	if ( NULL == psSema )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Invalid semaphore handle %d\n", hSema );
		return ( -EINVAL );
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );
		return ( -EINVAL );
	}

	psSema->ss_lNestCount = nCount;

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return 0;
}

/** Non conditionally block on a semaphore (wait-queue)
 * \ingroup DriverAPI
 * \par Description:
 *	Non conditionaly block on an semaphore until someone does a
 *	wakeup_sem(), we reach the timeout, or we catch a signal.
 *
 *	A semaphore (as created with create_semaphore()) can be used as
 *	a wait-queue rather than a semaphore/mutex by using sleep_on_sem()
 *	and wakeup_sem() instead of lock_semaphore() and unlock_semaphore().
 *
 * \par Warning:
 *	You should not mix calls to lock_semaphore()/unlock_semaphore()
 *	with sleep_on_sem()/wakeup_sem().
 * \param hSema
 *	The wait-queue to sleep on (created with create_semaphore()).
 * \param nTimeOut
 *	Maximum number of microseconds to wait for someone to wake us
 *	up before ETIME is returned.
 *	
 * \return
 *	On success 0 is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error -1 is returned "errno" will receive the error code.
 *	\endif
 *
 * \par Error codes:
 *	- \b EINVAL hSema is not a valid semaphore handle.
 *	- \b ETIME Nobody woke us up before the timeout.
 *
 * \sa create_semaphore(), wakeup_sem()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t sleep_on_sem( sem_id hSema, bigtime_t nTimeOut )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	thread_id hMyThread = psMyThread->tr_hThreadID;
	Semaphore_s *psSema;
	WaitQueue_s sSleepNode;
	WaitQueue_s sWaitNode;
	bigtime_t nResumeTime = get_system_time() + nTimeOut;
	int nFlg;
	int nError = 0;

	if ( 0 == hSema )
	{
		printk( "Error : attempt to sleep on semaphore 0\n" );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	if ( atomic_read(&g_sSemListSpinLock.sl_nNest) != 1 )
	{
		printk( "PANIC: sleep_on_sem() spinlock nest count is %d\n", atomic_read(&g_sSemListSpinLock.sl_nNest) );
	}

	psSema = get_semaphore_by_handle( -1, hSema );

	if ( NULL == psSema )
	{
		printk( "Error : sleep_on_sem( %d ) called with invalid semaphore id!!\n", hSema );
		nError = -EINVAL;
		goto error;
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );

		nError = -EINVAL;
		goto error;
	}

	if ( 0 == nTimeOut )
	{
		nError = -ETIME;
		goto error;
	}

	sched_lock();
	sWaitNode.wq_hThread = hMyThread;
	psMyThread->tr_hBlockSema = hSema;

	if ( INFINITE_TIMEOUT == nTimeOut )
	{
		psMyThread->tr_nState = TS_WAIT;
	}
	else
	{
		if ( get_system_time() > nResumeTime )
		{
			nError = -ETIME;
			sched_unlock();
			goto error;
		}
		else
		{
			psMyThread->tr_nState = TS_SLEEP;
			sSleepNode.wq_bIsMember = false;
			sSleepNode.wq_hThread = hMyThread;
			sSleepNode.wq_nResumeTime = nResumeTime;
			add_to_sleeplist( false, &sSleepNode );
		}
	}
	add_to_waitlist( false, &psSema->ss_psWaitQueue, &sWaitNode );
	sched_unlock();
	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	Schedule();

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	if ( INFINITE_TIMEOUT != nTimeOut )
	{
		remove_from_sleeplist( &sSleepNode );
	}

      /*** We must make sure nobody deleted the semaphore while we snoozed! ***/

	psSema = get_semaphore_by_handle( -1, hSema );

	if ( NULL == psSema )
	{
		printk( "ERROR : sleep_on_sem( %d ) semaphore deleted while we snoozed!!\n", hSema );
		nError = -EINVAL;
		goto error;
	}

	remove_from_waitlist( true, &psSema->ss_psWaitQueue, &sWaitNode );

	psMyThread->tr_hBlockSema = -1;

      error:
	spinunlock_enable( &g_sSemListSpinLock, nFlg );
	return ( nError );
}

/** Wake up threads blocked on a wait-queue
 * \ingroup DriverAPI
 * \par Description:
 
 *	Wake up eighter the first thread (the one that first blocked)
 *	or all threads blocked on a wait-queue.
 *
 *	If "bAll" is true all semaphores blocked on the queue will be
 *	released otherwhice only the first thread will be unblocked.
 * \par Warning:
 *	You should not mix calls to lock_semaphore()/unlock_semaphore()
 *	with sleep_on_sem()/wakeup_sem().
 * \param hSema
 *	Wait-queue to release (created with create_semaphore()).
 * \param bAll
 *	true to wake up all threads, false to only wake up the first
 *	thread in the queue.
 * \return
 *	On success 0 is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error -1 is returned "errno" will receive the error code.
 *	\endif
 *
 * \par Error codes:
 *	- \b EINVAL hSema is not a valid semaphore handle.
 *	
 * \sa sleep_on_sem(), create_semaphore()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t wakeup_sem( sem_id hSema, bool bAll )
{
	Semaphore_s *psSema;
	bool bNeetSchedule = false;
	int nFlg;
	int nError = 0;

	if ( 0 == hSema )
	{
		printk( "Error : wakeup_sem() attempt to unlock semaphore 0\n" );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	if ( psSema == NULL )
	{
		printk( "Error : wakeup_sem() called with invalid semaphore %d!!\n", hSema );
		nError = -EINVAL;
		goto error;
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );

		nError = -EINVAL;
		goto error;
	}

	if ( psSema->ss_psWaitQueue != NULL )
	{
		bNeetSchedule = true;
		nError = wake_up_queue( true, psSema->ss_psWaitQueue, 0, bAll );
	}

      error:
	spinunlock_enable( &g_sSemListSpinLock, nFlg );

/*  
    if ( bNeetSchedule ) {
    Schedule();
    }
    */
	return ( nError );
}

/** Get information about a kernel semaphore.
 * \par Description:
 *	get_semaphore_info() retrieve information about a kernel semaphore.
 *	You can obtain info about global-semaphores, kernel-semaphores
 *	and semaphores in the local address-space of any running process.
 *
 *	
 * \par Note:
 * \par Warning:
 * \param hSema
 *	Identity of the semaphore to inspect.
 * \param hOwner
 *	Specifies what "name-space" \p hSema lives in. If \p hOwner
 *	is a valid process-id \p hSema should be a local semaphore
 *	id in that process. If \p hOwner is -1 \p hSema should be
 *	a global ID. If \p hOwner is 0 \p hSema should be in the
 *	kernel name-space.
 * \return
 * \par Error codes:
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


status_t get_semaphore_info( sem_id hSema, proc_id hOwner, sem_info * psInfo )
{
	Semaphore_s *psSema;
	int nFlg;
	sem_id hRealSema = hSema;
	int nError;

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	if ( hSema == -1 )
	{
		if ( hOwner == -1 )
		{
			hRealSema = MArray_GetNextIndex( &g_sSemaphores, 0 );
		}
		else
		{
			SemContext_s *psCtx = NULL;

			if ( hOwner == 0 )
			{
				psCtx = g_psKernelSemContext;
			}
			else
			{
				Process_s *psProc = get_proc_by_handle( hOwner );

				if ( psProc != NULL )
				{
					psCtx = psProc->pr_psSemContext;
				}
			}
			hRealSema = -1;
			if ( psCtx != NULL )
			{
				int i;

				for ( i = 0; i < psCtx->sc_nMaxCount / 32 && hRealSema == -1; ++i )
				{
					if ( psCtx->sc_pnAllocMap[i] != 0 )
					{
						uint32 nMask = 1;
						int j;

						for ( j = 0; j < 32; ++j, nMask <<= 1 )
						{
							if ( psCtx->sc_pnAllocMap[i] & nMask )
							{
								hRealSema = i * 32 + j;
								break;
							}
						}
					}
				}
			}
		}
	}

	if ( hRealSema != -1 )
	{
		psSema = get_semaphore_by_handle( ( ( hOwner == 0 ) ? -1 : hOwner ), hRealSema );
	}
	else
	{
		psSema = NULL;
	}

	if ( NULL != psSema )
	{
		strcpy( psInfo->si_name, psSema->ss_zName );
		psInfo->si_sema_id = psSema->ss_hSemaID;
		psInfo->si_owner = psSema->ss_hOwner;
		psInfo->si_count = psSema->ss_lNestCount;
		psInfo->si_latest_holder = psSema->ss_hHolder;
		nError = 0;
	}
	else
	{
		nError = -EINVAL;
	}

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( nError );
}

static status_t do_get_next_semaphore_info( proc_id hOwner, sem_id hPrevSema, sem_info * psInfo )
{
	Semaphore_s *psSema;
	sem_id hSema = -1;
	int nFlg;
	int nError;

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	if ( ( hPrevSema & SEM_TYPE_MASK ) == SEMTYPE_GLOBAL )
	{
		hSema = MArray_GetNextIndex( &g_sSemaphores, hPrevSema & SEM_ID_MASK );
	}
	else
	{
		SemContext_s *psCtx = NULL;

		if ( ( hPrevSema & SEM_TYPE_MASK ) == SEMTYPE_KERNEL )
		{
			psCtx = g_psKernelSemContext;
		}
		else
		{
			Process_s *psProc = get_proc_by_handle( hOwner );

			if ( psProc != NULL )
			{
				psCtx = psProc->pr_psSemContext;
			}
		}
		hPrevSema &= SEM_ID_MASK;
		if ( psCtx != NULL )
		{
			int i;
			int j = ( hPrevSema + 1 ) % 32;
			uint32 nMask = 1 << j;

			for ( i = ( hPrevSema + 1 ) / 32; i < psCtx->sc_nMaxCount / 32; ++i )
			{
				for ( ; j < 32; ++j, nMask <<= 1 )
				{
					if ( psCtx->sc_pnAllocMap[i] & nMask )
					{
						hSema = ( i * 32 + j ) | ( hPrevSema & SEM_TYPE_MASK );
						break;
					}
				}
				j = 0;
				nMask = 1;
			}
		}
	}
	if ( hSema == -1 )
	{
		nError = 0;
		goto done;
	}

	psSema = get_semaphore_by_handle( hOwner, hSema );

	if ( NULL != psSema )
	{
		strcpy( psInfo->si_name, psSema->ss_zName );
		psInfo->si_sema_id = psSema->ss_hSemaID;
		psInfo->si_owner = psSema->ss_hOwner;
		psInfo->si_count = psSema->ss_lNestCount;
		psInfo->si_latest_holder = psSema->ss_hHolder;
		nError = 1;
	}
	else
	{
		nError = -EINVAL;
	}
      done:
	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( nError );
}

status_t sys_get_semaphore_info( sem_id hSema, proc_id hOwner, sem_info * psInfo )
{
	sem_info sInfo;
	int nError;

	nError = get_semaphore_info( hSema, hOwner, &sInfo );
	if ( nError >= 0 )
	{
		if ( memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) ) < 0 )
		{
			return ( -EFAULT );
		}
	}
	return ( nError );
}

status_t get_next_semaphore_info( sem_info * psInfo )
{
	return ( do_get_next_semaphore_info( psInfo->si_owner, psInfo->si_sema_id, psInfo ) );
}

status_t sys_get_next_semaphore_info( sem_info * psInfo )
{
	sem_info sInfo;
	proc_id hOwner;
	sem_id hSema;
	int nError;

	if ( memcpy_from_user( &hSema, &psInfo->si_sema_id, sizeof( hSema ) ) < 0 )
	{
		return ( -EFAULT );
	}
	if ( memcpy_from_user( &hOwner, &psInfo->si_owner, sizeof( hOwner ) ) < 0 )
	{
		return ( -EFAULT );
	}

	nError = do_get_next_semaphore_info( hOwner, hSema, &sInfo );
	if ( nError >= 0 )
	{
		if ( memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) ) < 0 )
		{
			return ( -EFAULT );
		}
	}
	return ( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_get_semaphore_count( bool bKernel, sem_id hSema )
{
	int nCount = 1;
	Semaphore_s *psSema;
	int nFlg;

	if ( 0 == hSema )
	{
		printk( "Error : attempt to get count for semaphore 0\n" );
		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	if ( ( psSema = get_semaphore_by_handle( -1, hSema ) ) )
	{
		nCount = psSema->ss_lNestCount;
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );

		nCount = -EINVAL;
	}

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( nCount );
}

int get_semaphore_count( sem_id hSema )
{
	return ( do_get_semaphore_count( true, hSema ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static thread_id do_get_semaphore_owner( bool bKernel, sem_id hSema )
{
	thread_id hOwner = -1;
	Semaphore_s *psSema;
	int nFlg;

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	if ( ( psSema = get_semaphore_by_handle( -1, hSema ) ) )
	{
		hOwner = psSema->ss_hHolder;
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_COUNTING )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );

		hOwner = -1;
	}

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( hOwner );
}

thread_id get_semaphore_owner( sem_id hSema )
{
	return ( do_get_semaphore_owner( true, hSema ) );
}

thread_id sys_get_semaphore_owner( sem_id hSema )
{
	return ( do_get_semaphore_owner( false, hSema ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
int is_semaphore_locked( sem_id hSema )
{
	Semaphore_s *psSema;
	RWLock_s *psRWLock;
	int nResult;
	int nFlg;

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	psRWLock = ( RWLock_s * ) psSema;

	if ( NULL != psSema )
	{
		switch ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) )
		{
		case SEMSTYLE_COUNTING:
			nResult = psSema->ss_hHolder == sys_get_thread_id( NULL ) && psSema->ss_lNestCount < 1;
			break;

		case SEMSTYLE_RWLOCK:
			nResult = ( psRWLock->rw_psHolders != NULL );
			break;

		default:
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Unknown semaphore style.\n" );
			nResult = 0;
			break;
		}
	}
	else
	{
		nResult = 0;
	}

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( nResult );
}

/**
 * \par Description:
 * Reader-writer locks maintain a list of threads that hold the lock and
 * the ways in which they hold the lock (e.g. for reading or for writing).
 * 
 * This function will look through the list of threads that hold this
 * lock to see if the current lock request can be added to an existing
 * entry.  If not, a new entry is created and the lock request is added
 * to it.
 *
 * \return If an error occurs while allocating memory, -ENOMEM is returned.
 * Otherwise zero is returned and the operation will complete successfully.
 */
static status_t rwlock_add_thread_hold( RWLock_s * psRWLock, thread_id hMyThread, bool bReading )
{
	RWHolder_s *psOurHold = NULL;

	/* We can acquire it right now! */
	psOurHold = kmalloc( sizeof( RWHolder_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );
	if ( psOurHold == NULL )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Ran out of memory for holders.\n" );

		return ( -ENOMEM );
	}

	/* Set up with details */
	psOurHold->rwh_hThreadId = hMyThread;

	if ( bReading )
		psOurHold->rwh_nReadCount = 1;
	else
		psOurHold->rwh_nWriteCount = 1;

	/* Insert into holder list */
	psOurHold->rwh_psNext = psRWLock->rw_psHolders;
	psRWLock->rw_psHolders = psOurHold;

	return ( 0 );
}

/**
 * \par Description:
 * Reader-writer locks maintain a list of threads that hold the lock and
 * the ways in which they hold the lock (e.g. for reading or for writing).
 * 
 * This function looks through the list of lock holders for the current
 * thread and removes a reference to it if found.
 *
 * \return If the current thread id is not found, -EACCES is returned.
 * Otherwise the function completes successfully and returns zero.
 */
static status_t rwlock_rem_thread_hold( RWLock_s * psRWLock, thread_id hMyThread, bool bReading )
{
	RWHolder_s **ppsCurrent;

	for ( ppsCurrent = &psRWLock->rw_psHolders; ( *ppsCurrent ) != NULL; ppsCurrent = &( *ppsCurrent )->rwh_psNext )
	{
		if ( ( *ppsCurrent )->rwh_hThreadId == hMyThread )
			break;
	}

	if ( ( *ppsCurrent ) == NULL )
		return ( -EACCES );

	if ( bReading )
	{
		if ( ( *ppsCurrent )->rwh_nReadCount < 1 )
			return ( -EACCES );

		--( *ppsCurrent )->rwh_nReadCount;
	}
	else
	{
		if ( ( *ppsCurrent )->rwh_nWriteCount < 1 )
			return -EACCES;

		--( *ppsCurrent )->rwh_nWriteCount;
	}

	if ( ( *ppsCurrent )->rwh_nReadCount < 1 && ( *ppsCurrent )->rwh_nWriteCount < 1 )
	{
		RWHolder_s *psRemove = ( *ppsCurrent );

		( *ppsCurrent ) = psRemove->rwh_psNext;

		kfree( psRemove );
	}

	return ( 0 );
}

/**
 * \par Description:
 * Reader-writer locks maintain a list of waiting nodes that preserves FIFO
 * scheduling for readers and writers.  Thus a thread must be removed from
 * the waiting list as well as the wait queue when it is ready to leave
 * do_rwlock_lock().
 *
 * This function must only be called from when the semaphore list lock is held.
 */
static status_t rwlock_remove_wait_node( RWLock_s * psRWLock, RWWaiter_s * psWaiter )
{
	RWWaiter_s **ppsCurrent;

	if ( psWaiter->rww_nReading > 1 )
	{
		/* There are other readers using this wait node */
		--psWaiter->rww_nReading;

		return ( 0 );
	}

	/* This wait node should be freed */
	for ( ppsCurrent = &psRWLock->rw_psWaiters; ( *ppsCurrent ) != NULL; ppsCurrent = &( *ppsCurrent )->rww_psNext )
	{
		if ( ( *ppsCurrent ) == psWaiter )
			break;
	}

	if ( ( *ppsCurrent ) == NULL )
		return ( -ENOENT );

	/* Remove from list */
	( *ppsCurrent ) = psWaiter->rww_psNext;

	if ( psWaiter->rww_psWaitQueue != NULL )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Freeing wait node, but queue is not empty!\n" );
	}

	kfree( psWaiter );

	return ( 0 );
}

/**
 * \par Description:
 * Finds an appropriate wait node in the lock's wait node list, creating a
 * new node if necessary.
 *
 * \return Returns the wait node found, or NULL if a new node could not be
 * allocated.
 */
static RWWaiter_s *rwlock_find_wait_node( RWLock_s * psRWLock, bool bReading )
{
	RWWaiter_s *psWaiter, *psOurWait = NULL;

	/* Find the last element in the wait node list */
	for ( psWaiter = psRWLock->rw_psWaiters; psWaiter != NULL; psWaiter = psWaiter->rww_psNext )
	{
		psOurWait = psWaiter;
	}

	/* If there is an entry, is it a reader entry? Are we also reading? */
	if ( psOurWait != NULL && psOurWait->rww_nReading > 0 && bReading )
	{
		/* Yes! Add to this wait node. */
		++psOurWait->rww_nReading;
		return ( psOurWait );
	}

	/* No suitable wait node, make a new wait entry */
	psWaiter = kmalloc( sizeof( RWWaiter_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );
	if ( psWaiter == NULL )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Ran out of memory for waiters.\n" );
		return NULL;
	}

	if ( bReading )
		psWaiter->rww_nReading = 1;
	else
		psWaiter->rww_nReading = -1;

	/* Add to list */
	if ( psOurWait )
		psOurWait->rww_psNext = psWaiter;
	else
		psRWLock->rw_psWaiters = psWaiter;

	return ( psWaiter );
}

/**
 * \par Description:
 * Returns the rw-lock count for the either the current thread or
 * all threads, for either read or write.
 */
static status_t do_rwlock_get_count( sem_id hSema, bool bShared, bool bAll )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	thread_id hMyThread = psMyThread->tr_hThreadID;
	Semaphore_s *psSema;
	int nCount = 0;
	int nFlg;

	if ( 0 == hSema )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Attempt to get count for semaphore 0\n" );

		return ( -EINVAL );
	}

	nFlg = spinlock_disable( &g_sSemListSpinLock );

	if ( ( psSema = get_semaphore_by_handle( -1, hSema ) ) )
	{
		if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) == SEMSTYLE_RWLOCK )
		{
			RWLock_s *psRWLock = ( RWLock_s * ) psSema;
			RWHolder_s *psHolder;

			for ( psHolder = psRWLock->rw_psHolders; psHolder != NULL; psHolder = psHolder->rwh_psNext )
			{
				if ( bAll || psHolder->rwh_hThreadId == hMyThread )
				{
					if ( bShared )
						nCount += psHolder->rwh_nReadCount;
					else
						nCount += psHolder->rwh_nWriteCount;
				}
			}
		}
		else
		{
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style.\n" );
			dump_semaphore( psSema, false );

			nCount = -EINVAL;
		}
	}
	else
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Invalid semaphore id %d.\n", hSema );

		nCount = -EINVAL;
	}

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( nCount );
}

status_t rwl_count_readers( sem_id hSema )
{
	return do_rwlock_get_count( hSema, true, false );
}

status_t rwl_count_all_readers( sem_id hSema )
{
	return do_rwlock_get_count( hSema, true, true );
}

status_t rwl_count_writers( sem_id hSema )
{
	return do_rwlock_get_count( hSema, false, false );
}

status_t rwl_count_all_writers( sem_id hSema )
{
	return do_rwlock_get_count( hSema, false, true );
}

/**
 * \par Description:
 * Tries to lock the reader-writer lock hSema for shared or exclusive access.
 * If exclusive access is requested and the current thread holds a shared
 * lock, a lock upgrade is attempted.
 */
static status_t do_rwlock_lock( bool bIgnored, sem_id hSema, bool bWantShared, uint32 nFlags, bigtime_t nTimeOut )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	thread_id hMyThread = psMyThread->tr_hThreadID;
	int nFlg;
	int nError;
	bigtime_t nResumeTime;
	bool bWakeNext;
	WaitQueue_s **ppsWaitQueue = NULL;
	RWWaiter_s *psWaiter = NULL;
	RWLock_s *psRWLock = NULL;

	if ( 0 == hSema )
	{
		kerndbg( KERN_WARNING, __FUNCTION__ "(): Attempt to lock rwlock 0\n" );
		return ( -EINVAL );
	}

	/* Disable interrupts and acquire semaphore list lock */
	nFlg = spinlock_disable( &g_sSemListSpinLock );

	if ( atomic_read(&g_sSemListSpinLock.sl_nNest) != 1 )
	{
		kerndbg( KERN_FATAL, __FUNCTION__ "(): Semaphore list spinlock held more than once - BAD.\n" );
	}

	/* Calculate resume time (ignored if nTimeOut == INFINITE_TIMEOUT) */
	nResumeTime = get_system_time() + nTimeOut;

	/* Loop until we acquire the rw-lock or an error is detected. */

	/**
	 * Store the wait queue we have been using so we can remove
	 * from the correct queue.
	 **/
	for ( ;; )
	{
		Semaphore_s *psSema;
		RWHolder_s *psHolder, *psOurHold = NULL;
		WaitQueue_s sSleepNode;
		WaitQueue_s sWaitNode;
		bool bLockHeld = false, bHeldShared = false;
		bool bOtherReaders = false, bOtherWriters = false;


		nError = 0;
		bWakeNext = false;

		/* Get the RW lock structure */
		psSema = get_semaphore_by_handle( -1, hSema );
		psRWLock = ( RWLock_s * ) psSema;

		if ( NULL == psSema )
		{
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Invalid semaphore id: %d\n", hSema );
			nError = -EINVAL;
			break;
		}

		if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_RWLOCK )
		{
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Incorrect semaphore style (id %d).\n", hSema );
			nError = -EINVAL;
			break;
		}

		/* See if we hold the mutex and who else holds it */
		for ( psHolder = psRWLock->rw_psHolders; psHolder != NULL; psHolder = psHolder->rwh_psNext )
		{

			if ( psHolder->rwh_hThreadId == hMyThread )
			{
				psOurHold = psHolder;
				bLockHeld = true;
				bHeldShared = ( psHolder->rwh_nWriteCount == 0 );
			}
			else
			{
				bOtherReaders = ( bOtherReaders || psHolder->rwh_nReadCount > 0 );
				bOtherWriters = ( bOtherWriters || psHolder->rwh_nWriteCount > 0 );
			}
		}

		/**
		 * What are we trying to do?
		 * Read-lock? Write-lock? Upgrade? Recursive-lock?
		 **/
		if ( bLockHeld )
		{
			/* We hold a lock. */
			if ( bHeldShared )
			{
				/* We have a read lock */
				if ( bWantShared )
				{
					/* Recursive read lock -- always granted */
					++psOurHold->rwh_nReadCount;
					break;
				}
				else	// bWantShared == false
				{

					/**
					 * Upgrade read lock to add write -- will fail if someone else is
					 * also trying to upgrade.
					 **/
					if ( bOtherReaders )
					{

						/**
						 * Look to see if another thread is registered as wanting to
						 * upgrade.
						 **/
						if ( psSema->ss_hHolder != hMyThread && psSema->ss_hHolder != -1 )
						{
							/* Upgrade attempt fails.  Return to the user. */
							kerndbg( KERN_DEBUG, __FUNCTION__ "(): Another upgrade attempt is underway.\n" );
							nError = -EBUSY;
							break;
						}
						else
						{
							/* Register as wanting to upgrade.  Add self to wait queue. */
							psSema->ss_hHolder = hMyThread;
							ppsWaitQueue = &psSema->ss_psWaitQueue;
						}
					}
					else	// bOtherReaders == false
					{
						/* No other readers -- we can safely upgrade now */
#ifdef __ENABLE_DEBUG__
						/* Consistency checks */
						if ( bOtherWriters )
						{
							kerndbg( KERN_DEBUG, __FUNCTION__ "(): We have read lock, but someone else is writing!\n" );
							nError = -EINVAL;
							break;
						}

						if ( psRWLock->rw_psHolders == NULL )
						{
							kerndbg( KERN_FATAL, __FUNCTION__ "(): We hold lock, but we aren't in the list.\n" );
							nError = -EINVAL;
							break;
						}

						if ( psRWLock->rw_psHolders->rwh_hThreadId != hMyThread )
						{
							kerndbg( KERN_FATAL, __FUNCTION__ "(): Only we (%d) hold the lock, but we aren't the first in the list.\n", hMyThread );
							dump_semaphore( psSema, false );
							nError = -EINVAL;
							break;
						}
#endif
						if ( psSema->ss_hHolder != hMyThread && psSema->ss_hHolder != -1 )
						{
							/* Upgrade attempt fails.  Return to the user. */
							kerndbg( KERN_DEBUG, __FUNCTION__ "(): Another upgrade attempt is underway.\n" );
							nError = -EBUSY;
							break;
						}

						psSema->ss_hHolder = -1;
						++psRWLock->rw_psHolders->rwh_nWriteCount;
						break;
					}
				}
			}
			else	// bHeldShared == false, i.e. we hold a write lock
			{
#ifdef __ENABLE_DEBUG__
				/* Consistency checks */
				if ( bOtherWriters )
				{
					kerndbg( KERN_DEBUG, __FUNCTION__ "(): We have write lock, but someone else is writing!\n" );
					nError = -EINVAL;
					break;
				}

				if ( bOtherReaders )
				{
					kerndbg( KERN_DEBUG, __FUNCTION__ "(): We have write lock, but someone else is reading!\n" );
					nError = -EINVAL;
					break;
				}

				if ( psRWLock->rw_psHolders == NULL )
				{
					kerndbg( KERN_FATAL, __FUNCTION__ "(): We hold write lock, but we aren't in the list.\n" );
					nError = -EINVAL;
					break;
				}

				if ( psRWLock->rw_psHolders->rwh_hThreadId != hMyThread )
				{
					kerndbg( KERN_FATAL, __FUNCTION__ "(): Only we (%d) hold the lock, but we aren't the first in the list.\n", hMyThread );
					dump_semaphore( psSema, false );
					nError = -EINVAL;
					break;
				}
#endif
				/* We have a write lock */
				if ( bWantShared )
				{
					/* Read lock on write lock -- will always be granted */
					++psRWLock->rw_psHolders->rwh_nReadCount;
					break;
				}
				else
				{
					/* Recursive write lock -- should be allowed? */
					++psRWLock->rw_psHolders->rwh_nWriteCount;
					break;
				}
			}
		}
		else		// bLockHeld == false
		{
			/* We don't hold the lock. */
			if ( bWantShared )
			{
				/* We want a shared lock */
				if ( bOtherWriters )
				{
					/* We have to wait -- is this the first time? */
					if ( psWaiter == NULL )
					{
						psWaiter = rwlock_find_wait_node( psRWLock, true );
						if ( psWaiter == NULL )
						{
							nError = -ENOMEM;
							break;
						}

						ppsWaitQueue = &psWaiter->rww_psWaitQueue;
					}
					/* else { We will wait on the same queue again. } */
				}
				else	// bOtherWriters == false
				{
					/* See if there are others waiting -- can't add readers while writers are waiting */
					if ( psRWLock->rw_psWaiters == NULL )
					{
						/* We can acquire a shared lock now */
						nError = rwlock_add_thread_hold( psRWLock, hMyThread, true );
					}
					else
					{
						/* We have to wait -- is this the first time? */
						if ( psWaiter == NULL )
						{
							psWaiter = rwlock_find_wait_node( psRWLock, true );
							if ( psWaiter == NULL )
							{
								nError = -ENOMEM;
								break;
							}

							ppsWaitQueue = &psWaiter->rww_psWaitQueue;
						}
						/* else { We will wait on the same queue again. } */
					}
					break;
				}
			}
			else	// bWantShared == false
			{
				/* We want an exclusive lock */
				if ( bOtherReaders || bOtherWriters )
				{
					/* We have to wait -- is this the first time? */
					if ( psWaiter == NULL )
					{
						psWaiter = rwlock_find_wait_node( psRWLock, false );
						if ( psWaiter == NULL )
						{
							nError = -ENOMEM;
							break;
						}

						ppsWaitQueue = &psWaiter->rww_psWaitQueue;
					}
					/* else { We will wait on the same queue again. } */
				}
				else
				{
					/* We can acquire it now! */
					nError = rwlock_add_thread_hold( psRWLock, hMyThread, false );
					break;
				}
			}
		}

		/* Now check if we have a zero timeout */
		if ( 0 == nTimeOut )
		{
			nError = -EWOULDBLOCK;
			break;
		}
		sched_lock();
		/* Do we have a timeout at all? */
		if ( INFINITE_TIMEOUT == nTimeOut )
		{
			psMyThread->tr_nState = TS_WAIT;
		}
		else
		{
			/* Has the timeout expired? */
			if ( get_system_time() > nResumeTime )
			{
				nError = -ETIME;
				sched_unlock();
				break;
			}
			else
			{
				/* Add to sleep queue as well so we will be woken after timeout */
				psMyThread->tr_nState = TS_SLEEP;
				sSleepNode.wq_bIsMember = false;
				sSleepNode.wq_hThread = hMyThread;
				sSleepNode.wq_nResumeTime = nResumeTime;
				add_to_sleeplist( false, &sSleepNode );
			}
		}

		/* If we get here, we are waiting this time. */
		sWaitNode.wq_hThread = hMyThread;
		psMyThread->tr_hBlockSema = hSema;

		add_to_waitlist( false, ppsWaitQueue, &sWaitNode );
		sched_unlock();
		/* Unlock semaphore list lock, yield processor until we are woken up */
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		Schedule();

		nFlg = spinlock_disable( &g_sSemListSpinLock );

		/**
		 * Okay, someone has woken us up.  Check our initial assumptions,
		 * then try the whole process again.
		 **/
		if ( INFINITE_TIMEOUT != nTimeOut )
		{
			remove_from_sleeplist( &sSleepNode );
		}

		psSema = get_semaphore_by_handle( -1, hSema );
		psRWLock = ( RWLock_s * ) psSema;
		if ( NULL == psSema )
		{
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): RW lock %d deleted while we waited for it!\n", hSema );
			nError = -EINVAL;
			psWaiter = NULL;	/* Will no longer be valid */
			break;
		}

		/* Remove from the current wait queue */
		bWakeNext = true;	/* We are consuming a wake-up message */
		remove_from_waitlist( true, ppsWaitQueue, &sWaitNode );
		psMyThread->tr_hBlockSema = -1;

		if ( ( nFlags & SEM_NOSIG ) == 0 && is_signal_pending() )
		{
			nError = -EINTR;
			break;
		}
	}

	/* If the lock is still intact, clean up before exiting */
	if ( psRWLock != NULL )
	{
		/* Remove our wait node, if any */
		if ( psWaiter != NULL )
		{
			if ( rwlock_remove_wait_node( psRWLock, psWaiter ) < 0 )
			{
				kerndbg( KERN_DEBUG, __FUNCTION__ "(): Tried to remove non-existent wait node!\n" );
			}
		}

		/* Remove our upgrade marker if set */
		if ( psRWLock->rw_sSemaphore.ss_hHolder == hMyThread )
			psRWLock->rw_sSemaphore.ss_hHolder = -1;

		if ( 0 != nError && bWakeNext && psRWLock->rw_psWaiters != NULL )
		{
			/* If we give up, we'd better wake up the next wait node */
			wake_up_queue( true, psRWLock->rw_psWaiters->rww_psWaitQueue, 0, true );
		}
	}

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( nError );
}


/**
 * \par Description:
 * Kernel library call.
 */
status_t rwl_lock_read_ex( sem_id hSema, uint32 nFlags, bigtime_t nTimeOut )
{
	return do_rwlock_lock( true, hSema, true, nFlags, nTimeOut );
}

status_t rwl_lock_read( sem_id hSema )
{
	return do_rwlock_lock( true, hSema, true, 0, INFINITE_TIMEOUT );
}

status_t rwl_lock_write_ex( sem_id hSema, uint32 nFlags, bigtime_t nTimeOut )
{
	return do_rwlock_lock( true, hSema, false, nFlags, nTimeOut );
}

status_t rwl_lock_write( sem_id hSema )
{
	return do_rwlock_lock( true, hSema, false, 0, INFINITE_TIMEOUT );
}

/**
 * \par Description:
 * Converts one read lock held by the current thread into an anonymous
 * read lock that can be recaptured by another thread.
 */
status_t rwl_convert_to_anon( sem_id hSema )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	thread_id hMyThread = psMyThread->tr_hThreadID;
	Semaphore_s *psSema;
	RWLock_s *psRWLock;
	int nFlg;

	if ( 0 == hSema )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Attempt to convert r-w lock 0\n" );
		return ( -EINVAL );
	}

	/* Acquire semaphore list lock */
	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	psRWLock = ( RWLock_s * ) psSema;
	if ( psSema == NULL )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Called with invalid r-w lock %d!\n", hSema );
		return ( -EINVAL );
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_RWLOCK )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Called with invalid semaphore style.\n" );

		return ( -EINVAL );
	}

	/* Check that we hold the kind of lock we are unlocking */
	if ( rwlock_rem_thread_hold( psRWLock, hMyThread, true ) < 0 )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Thread ID %d does not hold read lock for sema %d.\n", hMyThread, hSema );

		dump_semaphore( psSema, false );

		return ( -EACCES );
	}

	/* Check if upgrade is in progress */
	if ( rwlock_add_thread_hold( psRWLock, -2, true ) < 0 )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Lock conversion failed due to insufficient memory.\n" );

		dump_semaphore( psSema, false );

		return ( -ENOMEM );
	}

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( 0 );
}

/**
 * \par Description:
 * Converts one anonymous read lock into a read lock held by this thread.
 */
status_t rwl_convert_from_anon( sem_id hSema )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	thread_id hMyThread = psMyThread->tr_hThreadID;
	Semaphore_s *psSema;
	RWLock_s *psRWLock;
	int nFlg;

	if ( 0 == hSema )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Attempt to convert r-w lock 0\n" );
		return ( -EINVAL );
	}

	/* Acquire semaphore list lock */
	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	psRWLock = ( RWLock_s * ) psSema;
	if ( psSema == NULL )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Called with invalid r-w lock %d!\n", hSema );
		return ( -EINVAL );
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_RWLOCK )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Called with invalid semaphore style.\n" );

		return ( -EINVAL );
	}

	/* Check that we hold the kind of lock we are unlocking */
	if ( rwlock_rem_thread_hold( psRWLock, -2, true ) < 0 )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): No anonymous read locks held for sema %d.\n", hSema );

		dump_semaphore( psSema, false );

		return ( -EACCES );
	}

	/* Check if upgrade is in progress */
	if ( rwlock_add_thread_hold( psRWLock, hMyThread, true ) < 0 )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Lock conversion failed due to insufficient memory.\n" );

		dump_semaphore( psSema, false );

		return ( -ENOMEM );
	}

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( 0 );
}

/**
 * \par Description:
 * Unlocks a reader-writer lock.
 */
static status_t do_rwlock_unlock( bool bIgnored, sem_id hSema, bool bShared )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	thread_id hMyThread = psMyThread->tr_hThreadID;
	Semaphore_s *psSema;
	RWLock_s *psRWLock;
	WaitQueue_s *psWaitQueue = NULL;
	int nFlg;

	if ( 0 == hSema )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Attempt to unlock r-w lock 0\n" );
		return ( -EINVAL );
	}

	/* Acquire semaphore list lock */
	nFlg = spinlock_disable( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	psRWLock = ( RWLock_s * ) psSema;
	if ( psSema == NULL )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Called with invalid r-w lock %d!\n", hSema );
		return ( -EINVAL );
	}

	if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) != SEMSTYLE_RWLOCK )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Called with invalid semaphore style.\n" );

		return ( -EINVAL );
	}

	/* Check that we hold the kind of lock we are unlocking */
	if ( rwlock_rem_thread_hold( psRWLock, hMyThread, bShared ) < 0 )
	{
		spinunlock_enable( &g_sSemListSpinLock, nFlg );

		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Thread ID %d does not hold %s lock for sema %d.\n", hMyThread, bShared ? "read" : "write", hSema );

		dump_semaphore( psSema, false );

		return ( -EACCES );
	}

	/* Check if upgrade is in progress */
	if ( psSema->ss_hHolder != -1 )
	{
		/* Upgrade may proceed? */
		if ( psRWLock->rw_psHolders != NULL )
		{
			if ( psRWLock->rw_psHolders->rwh_psNext == NULL && psRWLock->rw_psHolders->rwh_hThreadId == psSema->ss_hHolder )
			{
				/* Yes. Upgrade may proceed. */
				psWaitQueue = psSema->ss_psWaitQueue;
			}
			else
			{
				/* No. There is more than one person holding the lock,
				 * so don't wake up -- upgrade will fail.
				 **/
				psWaitQueue = NULL;
			}
		}
		else
		{
			/* Inconsistency -- no-one left holding, but upgrade in progress! */
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Upgrade in progress but all locks were undone!\n" );

			psWaitQueue = psSema->ss_psWaitQueue;
		}
	}
	else
	{
		/* Upgrade not in progress -- if there are no holders, wake next */
		if ( psRWLock->rw_psHolders == NULL && psRWLock->rw_psWaiters != NULL )
		{
			psWaitQueue = psRWLock->rw_psWaiters->rww_psWaitQueue;
		}
	}

	if ( psWaitQueue != NULL )
		wake_up_queue( true, psWaitQueue, 0, true );

	spinunlock_enable( &g_sSemListSpinLock, nFlg );

	return ( 0 );
}

/**
 * \par Description:
 * Kernel library call.
 */
status_t rwl_unlock_read( sem_id hSema )
{
	return do_rwlock_unlock( true, hSema, true );
}

/**
 * \par Description:
 * Kernel library call.
 */
status_t rwl_unlock_write( sem_id hSema )
{
	return do_rwlock_unlock( true, hSema, false );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
void dump_semaphore( Semaphore_s *psSema, bool bShort )
{
	RWLock_s *psRWLock = ( RWLock_s * ) psSema;

	if ( psSema == NULL )
		return;

	dbprintf( DBP_DEBUGGER, "Semaphore %05d (\"%s\"):", psSema->ss_hSemaID, psSema->ss_zName );

	switch ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) )
	{
	case SEMSTYLE_COUNTING:
		if ( bShort )
			dbprintf( DBP_DEBUGGER, "  Style: counting  Count: %d  Flags: 0x%8.8X Owner: %d\n", psSema->ss_lNestCount, psSema->ss_nFlags, psSema->ss_hHolder );
		else
			dbprintf( DBP_DEBUGGER, "  Style: counting\n  Count: %d\n  Flags: 0x%8.8X\n  Owner: %d\n  Nest: %d\n", psSema->ss_lNestCount, psSema->ss_nFlags, psSema->ss_hHolder , psSema->ss_lNestCount);
		break;

	case SEMSTYLE_RWLOCK:
		if ( bShort )
		{
			dbprintf( DBP_DEBUGGER, "  Style: reader-writer\n" );
			break;
		}

		dbprintf( DBP_DEBUGGER, "  Style: reader-writer\n  Holders:" );

		if ( psRWLock->rw_psHolders )
		{
			RWHolder_s *psHolder;

			dbprintf( DBP_DEBUGGER, "\n" );

			for ( psHolder = psRWLock->rw_psHolders; psHolder != NULL; psHolder = psHolder->rwh_psNext )
			{
				dbprintf( DBP_DEBUGGER, "    Thread %-6d:  %-3d Read; %-3d Write.\n", psHolder->rwh_hThreadId, psHolder->rwh_nReadCount, psHolder->rwh_nWriteCount );
			}
		}
		else
		{
			dbprintf( DBP_DEBUGGER, " none\n" );
		}
		break;

	default:
		dbprintf( DBP_DEBUGGER, "  Style: unknown\n" );
		break;
	}

	dbprintf( DBP_DEBUGGER, "\n" );
}

void dump_sem_list( int argc, char **argv )
{
	sem_id hSem;
	proc_id hProc = -1;
	bool bShort = true, bGlobal = false, bKernel = true;

	if ( argc > 3 )
	{
		dbprintf( DBP_DEBUGGER, "Usage: %s [short|long] [kernel|global|process-id]\n", *argv );

		return;
	}

	++argv;

	if ( argc > 1 )
	{
		if ( strcmp( *argv, "long" ) == 0 )
		{
			bShort = false;
			++argv, --argc;
		}
		else if ( strcmp( *argv, "short" ) == 0 )
		{
			bShort = true;
			++argv, --argc;
		}
	}

	if ( argc > 1 )
	{
		if ( strcmp( *argv, "kernel" ) == 0 )
		{
			bKernel = true;
			++argv, --argc;
		}
		else if ( strcmp( *argv, "global" ) == 0 )
		{
			bGlobal = true;
			++argv, --argc;
		}
		else
		{
			hProc = atol( *argv );
			if ( hProc > 0 )
				bKernel = false;
		}
	}

	if ( bGlobal )
	{
		for ( hSem = 0; hSem != -1; hSem = MArray_GetNextIndex( &g_sSemaphores, hSem ) )
		{
			Semaphore_s *psSema = get_semaphore_by_handle( -1, hSem );

			if ( NULL != psSema )
			{
				dump_semaphore( psSema, bShort );
			}
		}
	}
	else
	{
		SemContext_s *psCtx;
		int i;

		if ( bKernel )
		{
			dbprintf( DBP_DEBUGGER, "Listing semaphores in kernel context:\n" );
			psCtx = g_psKernelSemContext;
		}
		else
		{
			Process_s *psProc = get_proc_by_handle( hProc );

			if ( psProc == NULL )
			{
				dbprintf( DBP_DEBUGGER, "No such process (id %d).\n", hProc );
				return;
			}

			psCtx = psProc->pr_psSemContext;
		}

		for ( i = 0; i < psCtx->sc_nMaxCount; ++i )
		{
			if ( psCtx->sc_apsSemaphores[i] != NULL )
				dump_semaphore( psCtx->sc_apsSemaphores[i], bShort );
		}
	}
}

static void db_dump_semaphore( int argc, char **argv )
{
	sem_id hSema;
	proc_id hProc = -1;
	Semaphore_s *psSema = NULL;
	bool bShort = true;

	if ( argc < 2 || argc > 4 )
	{
		dbprintf( DBP_DEBUGGER, "Usage: %s [short|long] sem-id [process-id]\n", *argv );

		return;
	}

	++argv;

	if ( argc > 1 )
	{
		if ( strcmp( *argv, "long" ) == 0 )
		{
			bShort = false;
			++argv, --argc;
		}
		else if ( strcmp( *argv, "short" ) == 0 )
		{
			bShort = true;
			++argv, --argc;
		}
	}

	hSema = atol( *argv );
	if ( hSema < 1 )
	{
		dbprintf( DBP_DEBUGGER, "Invalid semaphore id: %d\n", hSema );

		return;
	}
	++argv, --argc;

	if ( argc > 1 )
	{
		hProc = atol( *argv );
		if ( hProc < 1 )
			hProc = -1;
	}

	psSema = get_semaphore_by_handle( hProc, hSema );
	if ( psSema == NULL )
	{
		dbprintf( DBP_DEBUGGER, "Invalid semaphore id: %d\n", hSema );

		return;
	}

	dump_semaphore( psSema, bShort );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void db_toggle_sema( int argc, char **argv )
{
	sem_id hSem;
	Semaphore_s *psSema;

	if ( argc != 2 )
	{
		dbprintf( DBP_DEBUGGER, "Usage : %s sem_id\n", argv[0] );
		return;
	}
	hSem = atol( argv[1] );

	psSema = get_semaphore_by_handle( -1, hSem );

	if ( NULL != psSema )
	{
		if ( ( psSema->ss_nFlags & SEM_STYLE_MASK ) == SEMSTYLE_COUNTING )
		{
			dbprintf( DBP_DEBUGGER, "Toggle semaphore '%s'\n", psSema->ss_zName );
			UNLOCK( hSem );
			psSema->ss_lNestCount--;
		}
		else
		{
			dbprintf( DBP_DEBUGGER, "Can't toggle non-counting semaphore.\n" );
		}
	}
	else
	{
		dbprintf( DBP_DEBUGGER, "Error: semaphore %d does not exist\n", hSem );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void db_unlock_sema( int argc, char **argv )
{
	sem_id hSem;
	Semaphore_s *psSema;
	int nCount = 1;

	if ( argc < 2 || argc > 3 )
	{
		dbprintf( DBP_DEBUGGER, "usage : %s sem_id [count]\n", argv[0] );
		return;
	}
	hSem = atol( argv[1] );

	if ( argc > 2 )
		nCount = ( int )atol( argv[2] );

	psSema = get_semaphore_by_handle( -1, hSem );

	if ( NULL != psSema )
	{
		dbprintf( DBP_DEBUGGER, "Unlock semaphore '%s'\n", psSema->ss_zName );
		unlock_semaphore_ex( hSem, nCount );
	}
	else
	{
		dbprintf( DBP_DEBUGGER, "Error: semaphore %d does not exist\n", hSem );
	}
}

static void db_count_sema( int argc, char **argv )
{
	sem_id hSem;
	Semaphore_s *psSema;
	int nCount;

	if ( argc != 2 )
	{
		dbprintf( DBP_DEBUGGER, "Usage: %s sem_id\n", argv[0] );
		return;
	}
	hSem = atol( argv[1] );

	psSema = get_semaphore_by_handle( -1, hSem );

	if ( NULL != psSema )
	{
		nCount = get_semaphore_count( hSem );
		dbprintf( DBP_DEBUGGER, "Semaphore '%s' count: %d\n", psSema->ss_zName, nCount );
	}
	else
	{
		dbprintf( DBP_DEBUGGER, "Error: semaphore %d does not exist\n", hSem );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void db_lock_sema( int argc, char **argv )
{
	sem_id hSem;
	Semaphore_s *psSema;

	if ( argc != 2 )
	{
		dbprintf( DBP_DEBUGGER, "usage : %s sem_id\n", argv[0] );
		return;
	}
	hSem = atol( argv[1] );

	psSema = get_semaphore_by_handle( -1, hSem );

	if ( NULL != psSema )
	{
		dbprintf( DBP_DEBUGGER, "Lock semaphore '%s'\n", psSema->ss_zName );
		psSema->ss_lNestCount--;
	}
	else
	{
		dbprintf( DBP_DEBUGGER, "Error: semaphore %d does not exist\n", hSem );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_semaphores( void )
{
	MArray_Init( &g_sSemaphores );

	g_psKernelSemContext = kmalloc( sizeof( SemContext_s ), MEMF_KERNEL | MEMF_CLEAR );
	g_psKernelSemContext->sc_nLastID = 1;
	g_psKernelSemContext->sc_nAllocCount = 1;

	register_debug_cmd( "ls_sem", "dump the global semaphore list.", dump_sem_list );
	register_debug_cmd( "show_sem", "show a semaphore entry.", db_dump_semaphore );
	register_debug_cmd( "toggle_sem", "unlock, then lock a semaphore.", db_toggle_sema );
	register_debug_cmd( "lock", "lock a semaphore.", db_lock_sema );
	register_debug_cmd( "unlock", "unlock a semaphore.", db_unlock_sema );
	register_debug_cmd( "sem_count", "show semaphore count", db_count_sema );
}
