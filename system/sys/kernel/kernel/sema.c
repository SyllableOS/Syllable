
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
#include <atheos/semaphore.h>
#include <atheos/time.h>
#include <atheos/spinlock.h>

#include <macros.h>

#include "inc/semaphore.h"
#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/smp.h"

static MultiArray_s g_sSemaphores;
static SemContext_s *g_psKernelSemContext = NULL;

#ifdef __DETECT_DEADLOCK
static int g_nDeadLockDetectRun = 1;
#endif

//static int    g_nSemListSpinLock = 0;

//SPIN_LOCK( g_sSemListSpinLock, "semlist_slock" );

extern SpinLock_s g_sSchedSpinLock;

#define g_sSemListSpinLock g_sSchedSpinLock

SemContext_s *create_semaphore_context()
{
	SemContext_s *psCtx;

	psCtx = kmalloc( sizeof( SemContext_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if ( psCtx != NULL )
	{
		psCtx->sc_nLastID = 1;
		psCtx->sc_nAllocCount = 1;
	}
	return ( psCtx );
}

SemContext_s *clone_semaphore_context( SemContext_s * psOrig, proc_id hNewOwner )
{
	SemContext_s *psCtx;
	uint32 nFlg;
	int nError;

	psCtx = kmalloc( sizeof( SemContext_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if ( psCtx == NULL )
	{
		return ( NULL );
	}

      retry:
	nFlg = cli();
	spinlock( &g_sSemListSpinLock );

	psCtx->sc_nLastID = psOrig->sc_nLastID;
	psCtx->sc_nAllocCount = psOrig->sc_nAllocCount;
	psCtx->sc_nMaxCount = psOrig->sc_nMaxCount;

	nError = 0;
	if ( psCtx->sc_nMaxCount > 0 )
	{
		uint8 *pAllocMap = kmalloc( psCtx->sc_nMaxCount * 4 + psCtx->sc_nMaxCount / 8, MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );
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
				psCtx->sc_apsSemaphores[i] = kmalloc( sizeof( Semaphore_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );

				if ( psCtx->sc_apsSemaphores[i] == NULL )
				{
					nError = -ENOMEM;
					break;
				}

				memcpy( psCtx->sc_apsSemaphores[i]->ss_zName, psOrig->sc_apsSemaphores[i], OS_NAME_LENGTH );
				psCtx->sc_apsSemaphores[i]->ss_hSemaID = psOrig->sc_apsSemaphores[i]->ss_hSemaID;
				psCtx->sc_apsSemaphores[i]->ss_lNestCount = psOrig->sc_apsSemaphores[i]->ss_lNestCount;
				psCtx->sc_apsSemaphores[i]->ss_nFlags = psOrig->sc_apsSemaphores[i]->ss_nFlags;
				psCtx->sc_apsSemaphores[i]->ss_hHolder = -1;
				psCtx->sc_apsSemaphores[i]->ss_hOwner = hNewOwner;
				atomic_add( &g_sSysBase.ex_nSemaphoreCount, 1 );
			}
		}
	}
      error:
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

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
					atomic_add( &g_sSysBase.ex_nSemaphoreCount, -1 );
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

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

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
 *	used to aquire/release the semaphore.
 *
 *	A semaphore consist of a counter and a thread-list keeping
 *	track on threads waiting for the semaphore to be released.
 *
 *	When used as a counting semaphore the functions
 *	lock_semaphore() and unlock_semaphore() are used to aquire and
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
 *		- SEM_REQURSIVE	Don't block if the semaphore is
 *				aquired multiple times by the same
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

	psSema = kmalloc( sizeof( Semaphore_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

	if ( psSema == NULL )
	{
		return ( -ENOMEM );
	}

	strncpy( psSema->ss_zName, pzName, OS_NAME_LENGTH );
	psSema->ss_zName[OS_NAME_LENGTH - 1] = '\0';
	psSema->ss_lNestCount = nCount;
	psSema->ss_nFlags = nFlags;
	psSema->ss_hHolder = ( ( nFlags & SEM_REQURSIVE ) && nCount <= 0 ) ? CURRENT_THREAD->tr_hThreadID : -1;
	psSema->ss_hOwner = ( bKernel ) ? -1 : CURRENT_PROC->tc_hProcID;
      again:
	nFlg = cli();
	spinlock( &g_sSemListSpinLock );

	psSema->ss_hSemaID = add_semaphore( bKernel, psSema, ( nFlags & SEM_GLOBAL ) != 0 );

	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

	if ( psSema->ss_hSemaID == -ENOMEM )
	{
		shrink_caches( 65536 );
		goto again;
	}
	if ( psSema->ss_hSemaID >= 0 )
	{
		atomic_add( &g_sSysBase.ex_nSemaphoreCount, 1 );
		return ( psSema->ss_hSemaID );
	}
	else
	{
		printk( "Error : CreateSemphore() failed to insert semaphore!\n" );
	}
	kfree( psSema );
	return ( -ENOMEM );
}

sem_id create_semaphore( const char *pzName, int nCount, uint32 nFlags )
{
	return ( do_create_semaphore( true, pzName, nCount, nFlags ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

sem_id sys_create_semaphore( const char *pzName, int nCount, int nFlags )
{
	sem_id hSema;
	char zName[OS_NAME_LENGTH];
	int nError;

	nError = strncpy_from_user( zName, pzName, OS_NAME_LENGTH );

	if ( nError < 0 )
	{
		printk( "Error: sys_create_semaphore() failed to copy name\n" );
		return ( nError );
	}
	zName[OS_NAME_LENGTH - 1] = '\0';
	hSema = do_create_semaphore( false, zName, nCount, nFlags );

	if ( hSema >= 0 && ( nFlags & SEM_GLOBAL ) != 0 )
	{
		Process_s *psProc = CURRENT_PROC;
		Semaphore_s *psSema;
		int nFlg = cli();

		spinlock( &g_sSemListSpinLock );

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
		spinunlock( &g_sSemListSpinLock );
		put_cpu_flags( nFlg );
	}

	return ( hSema );
}


static status_t do_delete_semaphore( sem_id hSema )
{
	Semaphore_s *psSema;
	int nFlg = cli();

	if ( 0 == hSema )
	{
		printk( "Error : attempt to delete semaphore 0\n" );
		return ( -EINVAL );
	}

	spinlock( &g_sSemListSpinLock );
	psSema = get_semaphore_by_handle( -1, hSema );
	if ( psSema != NULL )
	{
		remove_semaphore( hSema );
	}
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

	if ( psSema != NULL )
	{
		wake_up_queue( psSema->ss_psWaitQueue, 0, true );
		kfree( psSema );
		atomic_add( &g_sSysBase.ex_nSemaphoreCount, -1 );
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

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_delete_semaphore( sem_id hSema )
{
	Process_s *psProc = CURRENT_PROC;
	Semaphore_s *psSema;
	int nFlg = cli();

	if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_KERNEL )
	{
		printk( "sys_delete_semaphore() attempt to delete kernel semaphore\n" );
		return ( -EINVAL );
	}

	spinlock( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );

	if ( psSema == NULL )
	{
		printk( "Error : attempt to delete invalid semaphore %d\n", hSema );
		spinunlock( &g_sSemListSpinLock );
		put_cpu_flags( nFlg );
		return ( -EINVAL );
	}

	if ( psProc->tc_hProcID != psSema->ss_hOwner )
	{
		printk( "Error: sys_delete_semaphore() attempt to delete semaphore %s not owned by us\n", psSema->ss_zName );
		spinunlock( &g_sSemListSpinLock );
		put_cpu_flags( nFlg );
		return ( -EPERM );
	}
	if ( ( hSema & SEM_TYPE_MASK ) == SEMTYPE_GLOBAL )
	{
		unlink_sema( psProc, psSema );
	}
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

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
		int nFlg = cli();
		Semaphore_s *psSema;
		sem_id hSema;

		spinlock( &g_sSemListSpinLock );
		psSema = psProc->pr_psFirstSema;
		hSema = psSema->ss_hSemaID;
		unlink_sema( psProc, psSema );

		spinunlock( &g_sSemListSpinLock );
		put_cpu_flags( nFlg );

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
		int nFlg = cli();
		Semaphore_s *psSema;
		sem_id hSema;

		spinlock( &g_sSemListSpinLock );
		psSema = psProc->pr_psFirstSema;
		hSema = psSema->ss_hSemaID;
		unlink_sema( psProc, psSema );

		spinunlock( &g_sSemListSpinLock );
		put_cpu_flags( nFlg );

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
		printk( "Error : detect_deadlock_thread() called width invalid thread %lx\n", hThread );
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

	nFlg = cli();
	spinlock( &g_sSemListSpinLock );

	if ( g_sSchedSpinLock.sl_nNest != 1 )
	{
		printk( "Panic: do_lock_semaphore_ex() spinlock nest count is %d, dumping call-stack...\n", g_sSchedSpinLock.sl_nNest );
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


		if ( psSema->ss_lNestCount - nCount >= 0 || ( ( psSema->ss_nFlags & SEM_REQURSIVE ) && psSema->ss_hHolder == hMyThread ) )
		{
			psSema->ss_lNestCount -= nCount;
			psSema->ss_hHolder = hMyThread;
			nError = 0;

#ifdef __DETECT_DEADLOCK
			if ( ( psSema->ss_nFlags & SEM_REQURSIVE ) && INFINITE_TIMEOUT == nTimeOut )
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
				printk( "Warning: %s semaphore %s(%d) taken twice\n", ( ( psSema->ss_nFlags & SEM_REQURSIVE ) ? "Recursive" : "None recursive" ), psSema->ss_zName, hSema );
				bDumpStack = true;
			}

			if ( 0 == nTimeOut )
			{
				nError = -EWOULDBLOCK;
				break;
			}

			sWaitNode.wq_hThread = hMyThread;
			psMyThread->tr_hBlockSema = hSema;

			if ( INFINITE_TIMEOUT == nTimeOut )
			{
#ifdef __DETECT_DEADLOCK
				if ( psSema->ss_nFlags & SEM_REQURSIVE )
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
					break;
				}
				else
				{
					psMyThread->tr_nState = TS_SLEEP;
					sSleepNode.wq_hThread = hMyThread;
					sSleepNode.wq_nResumeTime = nResumeTime;
					add_to_sleeplist( &sSleepNode );
				}
			}
			add_to_waitlist( &psSema->ss_psWaitQueue, &sWaitNode );

			spinunlock( &g_sSemListSpinLock );
			put_cpu_flags( nFlg );

			Schedule();

			nFlg = cli();
			spinlock( &g_sSemListSpinLock );

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
			remove_from_waitlist( &psSema->ss_psWaitQueue, &sWaitNode );

			nRestDelay = nResumeTime - get_system_time();
			psMyThread->tr_hBlockSema = -1;

			if ( ( nFlags & SEM_NOSIG ) == 0 && is_signals_pending() )
			{
				nError = -EINTR;
			}
		}
		if ( 0 != nError )
		{

	      /*** If we give up we better tell the next one to give it a try	***/
			wake_up_queue( psSema->ss_psWaitQueue, 0, false );
			break;
		}
	}
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

//    if ( bDumpStack ) {
//      trace_stack( 0, NULL );
//    }
	return ( nError );
}

/** Aquire a semaphore.
 * \ingroup DriverAPI
 * \ingroup SysCalls
 * \par Description:
 *	Aquire a semaphore. lock_semaphore_ex() will decrease the
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
 *	not be aquired. If the timeout is INFINITE_TIMEOUT the thread
 *	will be blocked until it can be successfully aquired or a
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
 *	until the semaphore can be aquired or a signel is caught.
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
 *	- \b ETIME  the semaphore could not be aquired before the timeout expiered.
 *	- \b EINTR  a signal was caught before the semaphore could be aquired.
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

	nFlg = cli();
	spinlock( &g_sSemListSpinLock );
	psSema = get_semaphore_by_handle( -1, hSema );

	while ( psSema->ss_lNestCount <= 0 )
	{
		WaitQueue_s sWaitNode;

		sWaitNode.wq_hThread = hMyThread;
		psMyThread->tr_hBlockSema = hSema;
		psMyThread->tr_nState = TS_WAIT;

		add_to_waitlist( &psSema->ss_psWaitQueue, &sWaitNode );

		spinunlock( &g_sSemListSpinLock );
		put_cpu_flags( nFlg );

		Schedule();

		nFlg = cli();
		spinlock( &g_sSemListSpinLock );

	  /*** We must make sure nobody deleted the semaphore while we snoozed! ***/

		psSema = get_semaphore_by_handle( -1, hSema );

		if ( NULL == psSema )
		{
			printk( "Error : lock_mutex( %d ) semaphore deleted while we snoozed!!\n", hSema );
			nError = -EINVAL;
			break;
		}
		remove_from_waitlist( &psSema->ss_psWaitQueue, &sWaitNode );

		psMyThread->tr_hBlockSema = -1;

		if ( bIgnoreSignals == false && is_signals_pending() )
		{
			nError = -EINTR;
			break;
		}
		if ( 0 != nError )
		{

	      /*** If we give up we better tell the next one to give it a try	***/
			wake_up_queue( psSema->ss_psWaitQueue, 0, false );
			break;
		}
	}
	if ( nError == 0 )
	{
		psSema->ss_lNestCount--;
	}
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );
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

	nFlg = cli();
	spinlock( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	if ( psSema == NULL )
	{
		spinunlock( &g_sSemListSpinLock );
		put_cpu_flags( nFlg );
		printk( "Error : ReleaseSemaphore() called with invalid semaphore %d!!\n", hSema );
		return ( -EINVAL );
	}

	psSema->ss_lNestCount++;

	if ( psSema->ss_lNestCount > 0 )
	{
		psSema->ss_hHolder = -1;
		wake_up_queue( psSema->ss_psWaitQueue, 0, false );
	}
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

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
 *	until the semaphore can be aquired or a signel is caught.
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
 *	- \b ETIME  the semaphore could not be aquired before the timeout expiered.
 *	- \b EINTR  a signal was caught before the semaphore could be aquired.
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

status_t LockSemaphore( sem_id hSema, uint32 nFlags, uint64 nTimeOut )
{
	return ( lock_semaphore( hSema, nFlags, nTimeOut ) );
}

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

	nFlg = cli();
	spinlock( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	if ( psSema == NULL )
	{
		spinunlock( &g_sSemListSpinLock );
		put_cpu_flags( nFlg );
		printk( "ERROR : ReleaseSemaphore() called with invalid semaphore %d!!\n", hSema );
		return ( -EINVAL );
	}

	if ( ( psSema->ss_nFlags & SEM_REQURSIVE ) || ( psSema->ss_nFlags & SEM_WARN_DBL_UNLOCK ) )
	{
		if ( psThread->tr_hThreadID != psSema->ss_hHolder )
		{
			if ( psThread != CURRENT_THREAD )
			{
				printk( "PANIC: We changed identity! Before spinlock : %s(%d) after: %s(%d)\n", psThread->tr_zName, psThread->tr_hThreadID, CURRENT_THREAD->tr_zName, CURRENT_THREAD->tr_hThreadID );
			}
			printk( "Prev CPU: %d Cur CPU: %d\n", psThread->tr_nCurrentCPU, CURRENT_THREAD->tr_nCurrentCPU );
			printk( "APIC ID=%d Lock Flg=%d Lock nest=%d Lock owner=%d\n", get_processor_id(), g_sSchedSpinLock.sl_nLocked, g_sSchedSpinLock.sl_nNest, g_sSchedSpinLock.sl_nProc );
			printk( "Error: unlock_semaphore_ex() sem %s (%d) not owned by us (%d) but by %d\n", psSema->ss_zName, hSema, psThread->tr_hThreadID, psSema->ss_hHolder );
		}
	}

	psSema->ss_lNestCount += nCount;

	if ( psSema->ss_lNestCount > 0 )
	{
		psSema->ss_hHolder = -1;

		if ( psSema->ss_lNestCount > 1 && psSema->ss_nFlags & SEM_WARN_DBL_UNLOCK )
		{
			printk( "Error: unlock_semaphore_ex() sem %s (%d) got count of %ld\n", psSema->ss_zName, hSema, psSema->ss_lNestCount );
		}

#ifdef __DETECT_DEADLOCK
		if ( psSema->ss_nFlags & SEM_REQURSIVE )
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
		wake_up_queue( psSema->ss_psWaitQueue, 0, nCount > 1 );
	}
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );
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
	Semaphore_s *psWaitQueue;
	Thread_s *psThread = CURRENT_THREAD;
	WaitQueue_s sWaitNode;
	int nError;
	int nFlg;

	sWaitNode.wq_hThread = psThread->tr_hThreadID;

	nFlg = cli();
	spinlock( &g_sSemListSpinLock );

	psWaitQueue = get_semaphore_by_handle( -1, hWaitQueue );
	if ( psWaitQueue == NULL )
	{
		printk( "Error: unlock_and_suspend() called with invalid waitqueue ID %d\n", hWaitQueue );
		nError = -EINVAL;
		goto error;
	}

	psThread->tr_nState = TS_WAIT;
	psThread->tr_hBlockSema = hWaitQueue;

	add_to_waitlist( &psWaitQueue->ss_psWaitQueue, &sWaitNode );

	// NOTE: Leave interrupt's disabled, we don't want to be pre-empted
	//       until the semaphore is unlocked.

	spinunlock( &g_sSemListSpinLock );

	// unlock_semaphore() might call schedule(), but that should be ok
	// as long as we already is added to the wait-list, and the
	// thread state is set to TS_WAIT.

	nError = unlock_semaphore( hSema );

	put_cpu_flags( nFlg );

	Schedule();

	nFlg = cli();
	spinlock( &g_sSemListSpinLock );

	remove_from_waitlist( &psWaitQueue->ss_psWaitQueue, &sWaitNode );

	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

	if ( nError == 0 && is_signals_pending() )
	{
		return ( -EINTR );
	}
	else
	{
		return ( nError );
	}
      error:
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );
	return ( nError );
}

/** Release a sinlock and block on a wait-queue in one atomic operation.
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
 * \param
 * \return
 * \sa unlock_and_suspend()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t spinunlock_and_suspend( sem_id hWaitQueue, SpinLock_s * psLock, uint32 nCPUFlags, bigtime_t nTimeout )
{
	Semaphore_s *psWaitQueue;
	Thread_s *psThread = CURRENT_THREAD;
	WaitQueue_s sWaitNode;
	WaitQueue_s sSleepNode;
	bigtime_t nResumeTime = get_system_time() + nTimeout;
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
	psThread->tr_hBlockSema = hWaitQueue;

	if ( nTimeout == INFINITE_TIMEOUT )
	{
		psThread->tr_nState = TS_WAIT;
	}
	else
	{
		if ( get_system_time() > nResumeTime )
		{
			nError = -ETIME;
			spinunlock( psLock );
			goto error;
		}
		else
		{
			psThread->tr_nState = TS_SLEEP;
			sSleepNode.wq_nResumeTime = nResumeTime;
			add_to_sleeplist( &sSleepNode );
		}
	}
	add_to_waitlist( &psWaitQueue->ss_psWaitQueue, &sWaitNode );

	// NOTE: Leave interrupt's disabled, we don't want to be pre-empted
	//             until the spinlock is released.

	spinunlock( &g_sSemListSpinLock );
	spinunlock( psLock );

	put_cpu_flags( nCPUFlags );

	Schedule();

	cli();
	spinlock( &g_sSemListSpinLock );

	if ( INFINITE_TIMEOUT != nTimeout )
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

	remove_from_waitlist( &psWaitQueue->ss_psWaitQueue, &sWaitNode );

	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nCPUFlags );

	if ( is_signals_pending() )
	{
		return ( -EINTR );
	}
	else if ( nTimeout != INFINITE_TIMEOUT && get_system_time() > nResumeTime )
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
 * \param nTimeout
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

	if ( g_sSchedSpinLock.sl_nNest != 1 )
	{
		printk( "PANIC: sleep_on_sem() spinlock nest count is %d\n", g_sSchedSpinLock.sl_nNest );
	}

	psSema = get_semaphore_by_handle( -1, hSema );

	if ( NULL == psSema )
	{
		printk( "Error : sleep_on_sem( %d ) called with invalid semaphore id!!\n", hSema );
		nError = -EINVAL;
		goto error;
	}

	if ( 0 == nTimeOut )
	{
		nError = -ETIME;
		goto error;
	}

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
			goto error;
		}
		else
		{
			psMyThread->tr_nState = TS_SLEEP;
			sSleepNode.wq_hThread = hMyThread;
			sSleepNode.wq_nResumeTime = nResumeTime;
			add_to_sleeplist( &sSleepNode );
		}
	}
	add_to_waitlist( &psSema->ss_psWaitQueue, &sWaitNode );

	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

	Schedule();

	nFlg = cli();
	spinlock( &g_sSemListSpinLock );

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

	remove_from_waitlist( &psSema->ss_psWaitQueue, &sWaitNode );

	psMyThread->tr_hBlockSema = -1;

      error:
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );
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

	nFlg = cli();
	spinlock( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );
	if ( psSema == NULL )
	{
		printk( "Error : wakeup_sem() called with invalid semaphore %d!!\n", hSema );
		nError = -EINVAL;
		goto error;
	}

	if ( psSema->ss_psWaitQueue != NULL )
	{
		bNeetSchedule = true;
		nError = wake_up_queue( psSema->ss_psWaitQueue, 0, bAll );
	}

      error:
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

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
	int nFlg = cli();
	sem_id hRealSema = hSema;
	int nError;

	spinlock( &g_sSemListSpinLock );

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

	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

	return ( nError );
}

static status_t do_get_next_semaphore_info( proc_id hOwner, sem_id hPrevSema, sem_info * psInfo )
{
	Semaphore_s *psSema;
	sem_id hSema = -1;
	int nFlg = cli();
	int nError;

	spinlock( &g_sSemListSpinLock );

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
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

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
	int nFlg = cli();

	if ( 0 == hSema )
	{
		printk( "Error : attempt to get count for semaphore 0\n" );
		return ( -EINVAL );
	}

	spinlock( &g_sSemListSpinLock );

	if ( ( psSema = get_semaphore_by_handle( -1, hSema ) ) )
	{
		nCount = psSema->ss_lNestCount;
	}
	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

	return ( nCount );
}

int get_semaphore_count( sem_id hSema )
{
	return ( do_get_semaphore_count( true, hSema ) );
}

int sys_get_semaphore_count( sem_id hSema )
{
	return ( do_get_semaphore_count( false, hSema ) );
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
	int nFlg = cli();

	spinlock( &g_sSemListSpinLock );

	if ( ( psSema = get_semaphore_by_handle( -1, hSema ) ) )
	{
		hOwner = psSema->ss_hHolder;
	}

	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

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
	int nResult;
	int nFlg = cli();

	spinlock( &g_sSemListSpinLock );

	psSema = get_semaphore_by_handle( -1, hSema );

	if ( NULL != psSema )
	{
		nResult = psSema->ss_hHolder == sys_get_thread_id( NULL ) && psSema->ss_lNestCount < 1;
	}
	else
	{
		nResult = 0;
	}

	spinunlock( &g_sSemListSpinLock );
	put_cpu_flags( nFlg );

	return ( nResult );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void dump_sem_list( int argc, char **argv )
{
	sem_id hSem;

	for ( hSem = 0; hSem != -1; hSem = MArray_GetNextIndex( &g_sSemaphores, hSem ) )
	{
		Semaphore_s *psSema = get_semaphore_by_handle( -1, hSem );

		if ( NULL != psSema )
		{
			dbprintf( DBP_DEBUGGER, "%05d cnt=%ld flg=%s owner=%d %s\n", psSema->ss_hSemaID, psSema->ss_lNestCount, ( ( psSema->ss_nFlags & SEM_REQURSIVE ) ? "SEM_REQURSIVE" : "            0" ), psSema->ss_hHolder, psSema->ss_zName );
		}
	}
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
		dbprintf( DBP_DEBUGGER, "usage : %s sem_id\n", argv[0] );
		return;
	}
	hSem = atol( argv[1] );

	psSema = get_semaphore_by_handle( -1, hSem );

	if ( NULL != psSema )
	{
		dbprintf( DBP_DEBUGGER, "Toggle semaphore '%s'\n", psSema->ss_zName );
		UNLOCK( hSem );
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

static void db_unlock_sema( int argc, char **argv )
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
		dbprintf( DBP_DEBUGGER, "Unlock semaphore '%s'\n", psSema->ss_zName );
		UNLOCK( hSem );
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

void InitSemaphores( void )
{
	MArray_Init( &g_sSemaphores );

	g_psKernelSemContext = kmalloc( sizeof( SemContext_s ), MEMF_KERNEL | MEMF_CLEAR );
	g_psKernelSemContext->sc_nLastID = 1;
	g_psKernelSemContext->sc_nAllocCount = 1;

	register_debug_cmd( "ls_sem", "dump the global semaphore list.", dump_sem_list );
	register_debug_cmd( "toggle_sem", "unlock, then lock a semaphore.", db_toggle_sema );
	register_debug_cmd( "lock", "lock a semaphore.", db_lock_sema );
	register_debug_cmd( "unlock", "unlock a semaphore.", db_unlock_sema );
}
