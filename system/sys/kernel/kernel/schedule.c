
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
#include <posix/wait.h>
#include <posix/resource.h>

#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <atheos/time.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/global.h"
#include "inc/smp.h"

static WaitQueue_s *g_psFirstSleeping = NULL;

SPIN_LOCK( g_sSchedSpinLock, "sched_slock" );

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void db_print_sleep_list( int argc, char **argv )
{
	WaitQueue_s *psTmp;
	int nFlg;
	int i = 0;

	nFlg = cli();
	sched_lock();
	for ( psTmp = g_psFirstSleeping; NULL != psTmp; psTmp = psTmp->wq_psNext )
	{
		Thread_s *psThread;

		kassertw( psTmp->wq_psNext == NULL || psTmp->wq_nResumeTime <= psTmp->wq_psNext->wq_nResumeTime );

		psThread = get_thread_by_handle( psTmp->wq_hThread );

		if ( psTmp->wq_hThread < 0 )
		{
			printk( " %d ->  %d.%06d - (%d)\n", i++, ( int )( psTmp->wq_nResumeTime / 1000000 ), ( int )( psTmp->wq_nResumeTime % 1000000 ), psTmp->wq_hThread );
		}
		else
		{
			printk( " %d ->  %d.%06d - %s(%d)\n", i++, ( int )( psTmp->wq_nResumeTime / 1000000 ), ( int )( psTmp->wq_nResumeTime % 1000000 ), psThread->tr_zName, psTmp->wq_hThread );
		}
	}
	sched_unlock();
	put_cpu_flags( nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_scheduler( void )
{
	register_debug_cmd( "ls_sleep", "list sleep-list nodes", db_print_sleep_list );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int __sched_lock( void )
{
	return ( spinlock( &g_sSchedSpinLock ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void sched_unlock( void )
{
	spinunlock( &g_sSchedSpinLock );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void add_to_waitlist( WaitQueue_s **psList, WaitQueue_s *psNode )
{
	int nFlg;
	WaitQueue_s *psHead;

	nFlg = cli();
	sched_lock();

	psHead = *psList;

	if ( psNode >= ( WaitQueue_s * )0x80000000 )
	{
		printk( "PANIC : add_to_waitlist() attempt to add node from user-space!!!\n" );

		sched_unlock();

		put_cpu_flags( nFlg );
		return;
	}

	psNode->wq_bIsMember = true;

	if ( psHead == NULL )
	{
		*psList = psNode;
		psNode->wq_psNext = psNode;
		psNode->wq_psPrev = psNode;
	}
	else
	{
		psNode->wq_psNext = psHead;
		psNode->wq_psPrev = psHead->wq_psPrev;

		psHead->wq_psPrev->wq_psNext = psNode;
		psHead->wq_psPrev = psNode;
	}
	sched_unlock();
	put_cpu_flags( nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void do_add_to_sleeplist( WaitQueue_s *psNode )
{
	WaitQueue_s *psTmp;
	int i = 0;

	psNode->wq_bIsMember = true;

	if ( g_psFirstSleeping == NULL || psNode->wq_nResumeTime <= g_psFirstSleeping->wq_nResumeTime )
	{
		psNode->wq_psPrev = NULL;
		psNode->wq_psNext = g_psFirstSleeping;
		if ( g_psFirstSleeping != NULL )
		{
			g_psFirstSleeping->wq_psPrev = psNode;
		}
		g_psFirstSleeping = psNode;
		return;
	}

	for ( psTmp = g_psFirstSleeping;; psTmp = psTmp->wq_psNext )
	{
		if ( i++ > 10000 )
		{
			printk( "add_to_sleeplist() looped 10000 times, give up\n" );
			break;
		}
		if ( psNode->wq_nResumeTime < psTmp->wq_nResumeTime )
		{
			psNode->wq_psPrev = psTmp->wq_psPrev;
			psNode->wq_psNext = psTmp;

			if ( psTmp->wq_psPrev != NULL )
			{
				psTmp->wq_psPrev->wq_psNext = psNode;
			}
			psTmp->wq_psPrev = psNode;
			return;
		}
		if ( psTmp->wq_psNext == NULL )
		{
			break;
		}
	}

      /*** Add to end of list ***/
	psNode->wq_psNext = NULL;
	psNode->wq_psPrev = psTmp;
	psTmp->wq_psNext = psNode;
	
//done:
}

void add_to_sleeplist( WaitQueue_s *psNode )
{
	int nFlg;

	nFlg = cli();
	sched_lock();
	do_add_to_sleeplist( psNode );
	sched_unlock();
	put_cpu_flags( nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void do_remove_from_sleeplist( WaitQueue_s *psNode )
{
	if ( psNode->wq_bIsMember )
	{

		psNode->wq_bIsMember = false;

		if ( psNode == g_psFirstSleeping )
		{
			g_psFirstSleeping = psNode->wq_psNext;
		}

		if ( psNode->wq_psPrev != NULL )
		{
			psNode->wq_psPrev->wq_psNext = psNode->wq_psNext;
		}
		if ( psNode->wq_psNext != NULL )
		{
			psNode->wq_psNext->wq_psPrev = psNode->wq_psPrev;
		}
	}
}

void remove_from_sleeplist( WaitQueue_s *psNode )
{
	int nFlg;

	nFlg = cli();
	sched_lock();
	do_remove_from_sleeplist( psNode );
	sched_unlock();
	put_cpu_flags( nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void remove_from_waitlist( WaitQueue_s **ppsList, WaitQueue_s *psNode )
{
	int nFlg = cli();

	sched_lock();

	kassertw( psNode->wq_bIsMember );

	psNode->wq_bIsMember = FALSE;

	if ( *ppsList == psNode )
	{
		if ( psNode->wq_psNext == psNode )
		{
			*ppsList = NULL;
		}
		else
		{
			*ppsList = psNode->wq_psNext;
		}
	}
	psNode->wq_psPrev->wq_psNext = psNode->wq_psNext;
	psNode->wq_psNext->wq_psPrev = psNode->wq_psPrev;

	sched_unlock();
	put_cpu_flags( nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int wake_up_queue( WaitQueue_s *psList, int nReturnCode, bool bAll )
{
	int nThreadsWoken = 0;
	int nFlg = cli();

	sched_lock();

	if ( psList != NULL )
	{
		if ( bAll )
		{
			WaitQueue_s *psNode;
			bool bFirst = true;

			for ( psNode = psList; bFirst || psNode != psList; psNode = psNode->wq_psNext )
			{
				Thread_s *psThread = get_thread_by_handle( psNode->wq_hThread );

				bFirst = false;

				if ( psThread != NULL )
				{
					psNode->wq_nCode = nReturnCode;

					if ( psThread->tr_nState == TS_WAIT || psThread->tr_nState == TS_SLEEP )
					{
						add_thread_to_ready( psThread );
						nThreadsWoken++;
					}
				}
				else
				{
					printk( "WARNING : Found dead thread %d in waitlist!\n", psNode->wq_hThread );
				}
			}
		}
		else
		{
			Thread_s *psThread = get_thread_by_handle( psList->wq_hThread );

			if ( psThread != NULL )
			{
				if ( psThread->tr_nState == TS_WAIT || psThread->tr_nState == TS_SLEEP )
				{
					add_thread_to_ready( psThread );
					nThreadsWoken++;
				}
			}
		}
	}

	sched_unlock();
	put_cpu_flags( nFlg );
	if ( nThreadsWoken > 0 )
	{
		g_bNeedSchedule = true;
	}
	return ( nThreadsWoken );
}

/*****************************************************************************
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

int sleep_on_queue( WaitQueue_s **ppsList )
{
	Thread_s *psThread = CURRENT_THREAD;
	WaitQueue_s sWaitNode;
	int nFlg;

	sWaitNode.wq_hThread = psThread->tr_hThreadID;

	nFlg = cli();
	sched_lock();

	psThread->tr_nState = TS_WAIT;
	add_to_waitlist( ppsList, &sWaitNode );

	sched_unlock();
	put_cpu_flags( nFlg );

      /*** NOTE: To get good real-time performance, we must reprogram the PIT timer here	***/

	Schedule();

	nFlg = cli();
	sched_lock();

	remove_from_waitlist( ppsList, &sWaitNode );

	sched_unlock();
	put_cpu_flags( nFlg );

	return ( sWaitNode.wq_nCode );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Called periodically from timer interrupt to
 *	wake up sleeping threads.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void wake_up_sleepers( bigtime_t nCurTime )
{
	WaitQueue_s *psFirstTimer = NULL;

	int nFlg;
	int i = 0;

	nFlg = cli();
	sched_lock();

	while ( g_psFirstSleeping != NULL && g_psFirstSleeping->wq_nResumeTime <= nCurTime )
	{
		Thread_s *psThread;
		WaitQueue_s *psNode = g_psFirstSleeping;

		if ( i++ > 10000 )
		{
			printk( "wake_up_sleepers() looped 10000 times, give up\n" );
			break;
		}

		g_psFirstSleeping = psNode->wq_psNext;

		if ( g_psFirstSleeping != NULL )
		{
			g_psFirstSleeping->wq_psPrev = NULL;
		}
		psNode->wq_bIsMember = false;

		if ( psNode->wq_hThread != -1 )
		{

			psThread = get_thread_by_handle( psNode->wq_hThread );
			if ( psThread == NULL )
			{
				printk( "PANIC : Found dead thread %d in sleep-list!!!!\n", psNode->wq_hThread );
				continue;
			}
			if ( psThread->tr_nState != TS_SLEEP )
			{
				continue;
			}
			add_thread_to_ready( psThread );
		}
		else
		{
			psNode->wq_psNext = psFirstTimer;
			psFirstTimer = psNode;
		}
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	while ( psFirstTimer != NULL )
	{
		WaitQueue_s *psNode = psFirstTimer;

		psFirstTimer = psNode->wq_psNext;
		psNode->wq_pfCallBack( psNode->wq_pUserData );
		if ( psNode->wq_bOneShot == false )
		{
			psNode->wq_nResumeTime = nCurTime + psNode->wq_nTimeout;
			add_to_sleeplist( psNode );
		}
	}
}

ktimer_t create_timer( void )
{
	WaitQueue_s *psNode = kmalloc( sizeof( WaitQueue_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

	if ( psNode == NULL )
	{
		return ( NULL );
	}
	psNode->wq_hThread = -1;
	return ( psNode );
}

void start_timer( ktimer_t hTimer, timer_callback *pfCallback, void *pData, bigtime_t nPeriode, bool bOneShot )
{
	WaitQueue_s *psNode = hTimer;
	int nFlg;

	if ( psNode == NULL )
	{
		return;
	}
	nFlg = cli();
	sched_lock();

	if ( psNode->wq_bIsMember )
	{
		do_remove_from_sleeplist( psNode );
	}
	if ( pfCallback != NULL )
	{
		psNode->wq_nResumeTime = get_system_time() + nPeriode;
		psNode->wq_bOneShot = bOneShot;
		psNode->wq_nTimeout = nPeriode;
		psNode->wq_pfCallBack = pfCallback;
		psNode->wq_pUserData = pData;
		do_add_to_sleeplist( psNode );
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return;
}

void delete_timer( ktimer_t hTimer )
{
	WaitQueue_s *psNode = hTimer;

	if ( psNode == NULL )
	{
		return;
	}
	remove_from_sleeplist( psNode );
	kfree( psNode );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 old_Delay( const uint32 nMicros )
{
	Thread_s *psThread;
	WaitQueue_s sWaitNode;
	int nFlg;
	uint64 nRestTime;
	bigtime_t nCurTime = get_system_time();

	psThread = CURRENT_THREAD;

	kassertw( NULL != psThread );

	printk( "Whiiiii some dude called Delay()!\n" );

	sWaitNode.wq_nResumeTime = nCurTime + ( ( uint64 )nMicros );
	sWaitNode.wq_hThread = psThread->tr_hThreadID;

	nFlg = cli();
	sched_lock();

	psThread->tr_nState = TS_SLEEP;

	add_to_sleeplist( &sWaitNode );

	sched_unlock();
	put_cpu_flags( nFlg );

      /*** NOTE: To get good real-time performance, we must reprogram the PIT timer here	***/

	Schedule();

	nFlg = cli();
	sched_lock();

	remove_from_sleeplist( &sWaitNode );

	sched_unlock();
	put_cpu_flags( nFlg );

	nRestTime = sWaitNode.wq_nResumeTime - nCurTime;

	if ( nRestTime < 0 )
	{
		nRestTime = 0;
	}
	return ( ( uint32 )nRestTime );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t snooze( bigtime_t nTimeout )
{
	Thread_s *psThread;
	WaitQueue_s sWaitNode;
	int nFlg;

	psThread = CURRENT_THREAD;

	kassertw( psThread != NULL );


	sWaitNode.wq_nResumeTime = get_system_time() + nTimeout;
	sWaitNode.wq_hThread = psThread->tr_hThreadID;

	nFlg = cli();
	sched_lock();

	psThread->tr_nState = TS_SLEEP;

	add_to_sleeplist( &sWaitNode );

	sched_unlock();
	put_cpu_flags( nFlg );

      /*** NOTE: To get good real-time performance, we must reprogram the PIT timer here	***/

	Schedule();

	nFlg = cli();
	sched_lock();

	remove_from_sleeplist( &sWaitNode );

	sched_unlock();
	put_cpu_flags( nFlg );
	return ( ( sWaitNode.wq_nResumeTime > get_system_time() )? -EINTR : 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_snooze( bigtime_t nTimeout )
{
	return ( snooze( nTimeout ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t set_thread_priority( const thread_id hThread, const int nPriority )
{
	Thread_s *psThread;
	int nOldPri = 0;
	int nFlg = cli();

	sched_lock();

	if ( hThread == -1 )
	{
		psThread = get_thread_by_name( NULL );
	}
	else
	{
		psThread = get_thread_by_handle( hThread );
	}

	if ( psThread != NULL )
	{
		nOldPri = psThread->tr_nPriority;
		psThread->tr_nPriority = nPriority;
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	Schedule();
	return ( nOldPri );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_set_thread_priority( const thread_id hThread, const int nPriority )
{
	return ( set_thread_priority( hThread, nPriority ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t suspend( void )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	int nError;
	int nFlg = cli();

	sched_lock();

	if ( is_signal_pending() == false )
	{
		psMyThread->tr_nState = TS_WAIT;

		sched_unlock();
		put_cpu_flags( nFlg );
		Schedule();
	}
	else
	{
		sched_unlock();
		put_cpu_flags( nFlg );
	}

	if ( is_signal_pending() )
	{
		nError = -EINTR;
	}
	else
	{
		nError = 0;
	}

	return ( nError );
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_suspend_thread( const thread_id hThread )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	int nError;
	int nFlg = cli();

	sched_lock();

	if ( hThread == psMyThread->tr_hThreadID )	/* We are about to suspend our self     */
	{
		if ( is_signal_pending() == false )
		{
			psMyThread->tr_nState = TS_WAIT;

			sched_unlock();
			put_cpu_flags( nFlg );
			Schedule();
		}
		else
		{
			sched_unlock();
			put_cpu_flags( nFlg );
		}

		if ( is_signal_pending() )
		{
			nError = -EINTR;
		}
		else
		{
			nError = 0;
		}
		return ( nError );
	}
	else
	{
		nError = sys_kill( hThread, SIGSTOP );
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t stop_thread( bool bNotifyParent )
{
	Thread_s *psThread = CURRENT_THREAD;
	int nFlg = cli();

	sched_lock();

	if ( bNotifyParent && psThread->tr_hParent != -1 )
	{
		Thread_s *psParentThread = get_thread_by_handle( psThread->tr_hParent );
		if ( psParentThread != NULL )
		{
			send_signal( psParentThread, SIGCHLD, true );
		}
	}

	psThread->tr_nState = TS_STOPPED;
	sched_unlock();
	put_cpu_flags( nFlg );

	Schedule();

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_resume_thread( const thread_id hThread )
{
	Thread_s *psThread;
	int nError = 0;
	int nFlags = cli();

	sched_lock();

	psThread = get_thread_by_handle( hThread );

	if ( psThread != NULL )
	{
		if ( psThread->tr_nState == TS_WAIT || psThread->tr_nState == TS_SLEEP )
		{
			add_thread_to_ready( psThread );

			sched_unlock();
			put_cpu_flags( nFlags );
			return ( 0 );
		}
	}
	else
	{
		nError = -EINVAL;
	}
	sched_unlock();
	put_cpu_flags( nFlags );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t wakeup_thread( thread_id hThread, bool bWakeupSuspended )
{
	Thread_s *psThread;
	int nError = 0;
	int nFlags = cli();

	sched_lock();

	psThread = get_thread_by_handle( hThread );

	if ( psThread != NULL )
	{
		if ( psThread->tr_nState == TS_WAIT || psThread->tr_nState == TS_SLEEP || ( bWakeupSuspended && psThread->tr_nState == TS_STOPPED ) )
		{
			add_thread_to_ready( psThread );

			sched_unlock();
			put_cpu_flags( nFlags );
			return ( 0 );
		}
	}
	else
	{
		nError = -EINVAL;
	}
	sched_unlock();
	put_cpu_flags( nFlags );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t start_thread( thread_id hThread )
{
	Thread_s *psThread;
	int nError = 0;
	int nFlags = cli();

	sched_lock();

	psThread = get_thread_by_handle( hThread );

	if ( psThread != NULL )
	{
		if ( psThread->tr_nState == TS_STOPPED || psThread->tr_nState == TS_WAIT || psThread->tr_nState == TS_SLEEP )
		{
			add_thread_to_ready( psThread );

			sched_unlock();
			put_cpu_flags( nFlags );
			return ( 0 );
		}
	}
	else
	{
		nError = -EINVAL;
	}
	sched_unlock();
	put_cpu_flags( nFlags );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static Thread_s *find_dead_child( pid_t hPid, pid_t hGroup, int nOptions )
{
	Thread_s *psThread = CURRENT_THREAD;
	Thread_s *psChild;
	pid_t hTmpID;


	FOR_EACH_THREAD( hTmpID, psChild )
	{
		if ( psChild->tr_hParent == psThread->tr_hThreadID && ( psChild->tr_nState == TS_ZOMBIE || psChild->tr_nState == TS_STOPPED ) )
		{
			if ( psChild->tr_nState == TS_STOPPED && ( psChild->tr_nExitCode == 0 || ( nOptions & WUNTRACED ) == 0 ) )
			{
				continue;
			}

			if ( hPid == -1 )
			{
				return ( psChild );
				break;
			}

			if ( hPid > 0 )
			{
				if ( hPid == psChild->tr_hThreadID )
				{
					return ( psChild );
					break;
				}
				else
				{
					continue;
				}
			}
			if ( hPid < -1 || 0 == hPid )
			{	/* Any children belonging to group -hPid, or same group as us */
				if ( psChild->tr_psProcess->pr_hPGroupID == hGroup )
				{
					return ( psChild );
					break;
				}
				continue;
			}
		}
	}
	return ( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

pid_t sys_wait4( const pid_t hPid, int *const pnStatLoc, const int nOptions, struct rusage *const psRUsage )
{
	volatile Thread_s *psThread = CURRENT_THREAD;
	Process_s *psProc = CURRENT_PROC;
	thread_id hTmpID;
	Thread_s *psTmp;
	bool bFound = false;
	pid_t nRes = -1;
	pid_t hGroup = -1;
	int nFlg = cli();

	sched_lock();

      /*** First we verify that we have a child that can satisfy wait4() ***/
	if ( hPid > 0 )
	{
		bFound = NULL != get_thread_by_handle( hPid );
	}
	else
	{
		/* Any children belonging to group -hPid, or same group as us */
		if ( hPid < -1 || 0 == hPid )
		{
			hGroup = ( 0 == hPid ) ? psProc->pr_hPGroupID : -hPid;

			FOR_EACH_THREAD( hTmpID, psTmp )
			{
				if ( psTmp->tr_hParent == psThread->tr_hThreadID && psTmp->tr_psProcess->pr_hPGroupID == hGroup )
				{
					bFound = true;
					break;
				}
			}
		}
		else
		{
			if ( -1 == hPid )
			{	/* Wait for any children */
				FOR_EACH_THREAD( hTmpID, psTmp )
				{
					if ( psTmp->tr_hParent == psThread->tr_hThreadID )
					{
						bFound = true;
						break;
					}
				}
			}	/* (-1 == hPid) */
		}		/* ( hPid < -1 || 0 == hPid ) */
	}			/* (hDid > 0) */

	sched_unlock();
	put_cpu_flags( nFlg );


	if ( bFound == false )
	{
		return ( -ECHILD );
	}

      /*** Then we wait for it to die ***/

	for ( ;; )
	{
		Thread_s *psChild;
		int nOldFlg = cli();

		sched_lock();
		psChild = find_dead_child( hPid, hGroup, nOptions );

		if ( psChild != NULL )
		{
			switch ( psChild->tr_nState )
			{
			case TS_STOPPED:
				{
					int nStatLoc = psChild->tr_nExitCode << 8 | 0x7f;

					psChild->tr_nExitCode = 0;	// Only report stopped children once
					nRes = psChild->tr_hThreadID;
					sched_unlock();
					put_cpu_flags( nOldFlg );

					if ( pnStatLoc != NULL && memcpy_to_user( pnStatLoc, &nStatLoc, sizeof( nStatLoc ) ) < 0 )
					{
						printk( "Error: sys_wait4:1() got invalid pointer (%p) from userspace\n", pnStatLoc );
						nRes = -EFAULT;
					}
					return ( nRes );
				}
			case TS_ZOMBIE:
				{

		      /*** TODO: Add child's rusage to ours ***/

					sched_unlock();
					put_cpu_flags( nOldFlg );

					if ( pnStatLoc != NULL )
					{
						int nStatLoc = psChild->tr_nExitCode;

						nRes = memcpy_to_user( pnStatLoc, &nStatLoc, sizeof( nStatLoc ) );
						if ( nRes < 0 )
						{
							printk( "Error: sys_wait4:2() got invalid pointer (%p) from userspace\n", pnStatLoc );
						}
					}
					else
					{
						nRes = 0;
					}
					if ( nRes >= 0 )
					{
						wakeup_thread( g_sSysBase.ex_hInitProc, false );
						nRes = psChild->tr_hThreadID;

						Thread_Delete( psChild );
					}
					return ( nRes );
				}
			}
		}
		if ( nOptions & WNOHANG )
		{
			sched_unlock();
			put_cpu_flags( nOldFlg );
			return ( 0 );
		}
		if ( is_signal_pending() == false )
		{
			psThread->tr_nState = TS_WAIT;
			sched_unlock();
			put_cpu_flags( nOldFlg );
			Schedule();
		}
		else
		{
			sched_unlock();
			put_cpu_flags( nOldFlg );
			return ( -EINTR );
		}
	}
	printk( "Panic : sys_wait4() returned void!!!\n" );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

pid_t sys_waitpid( const pid_t hPid, int *const pnStatLoc, int const nOptions )
{
	return ( sys_wait4( hPid, pnStatLoc, nOptions, NULL ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_wait_for_thread( const thread_id hThread )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	int nFlg;
	Thread_s *psThread;
	int nResult;

	nFlg = cli();
	sched_lock();

	psThread = get_thread_by_handle( hThread );

	if ( psThread != NULL && psThread->tr_nState != TS_ZOMBIE )
	{
		WaitQueue_s sWaitNode;

		sWaitNode.wq_hThread = psMyThread->tr_hThreadID;

		psMyThread->tr_nState = TS_WAIT;
		add_to_waitlist( &psThread->tr_psTermWaitList, &sWaitNode );

		sched_unlock();
		put_cpu_flags( nFlg );

		Schedule();

		nFlg = cli();
		sched_lock();

		psThread = get_thread_by_handle( hThread );
		if ( psThread != NULL )
		{
			remove_from_waitlist( &psThread->tr_psTermWaitList, &sWaitNode );
		}

		nResult = ( is_signal_pending() )? -EINTR : sWaitNode.wq_nCode;
	}
	else
	{
		nResult = -ECHILD;
	}

	sched_unlock();
	put_cpu_flags( nFlg );

	return ( nResult );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void add_thread_to_ready( Thread_s *psThread )
{
	Thread_s **ppsTmp;

	if ( IDLE_THREAD == psThread )
	{
		printk( "Error: add_thread_to_ready() attempt to add idle thread to ready list\n" );
		return;
	}

	if ( TS_READY != psThread->tr_nState )
	{
		psThread->tr_nState = TS_READY;

		for ( ppsTmp = &g_sSysBase.ex_psFirstReady; NULL != *ppsTmp; ppsTmp = &( ( *ppsTmp )->tr_psNext ) )
		{
			if ( psThread->tr_nPriority > ( *ppsTmp )->tr_nPriority )
			{
				psThread->tr_psNext = *ppsTmp;
				*ppsTmp = psThread;
				return;
			}
		}

	  /*** Add to end of list ***/
		psThread->tr_psNext = NULL;
		*ppsTmp = psThread;
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void remove_thread_from_ready( Thread_s *psThread )
{
	Thread_s **ppsTmp;
	bool bRemoved = false;

	for ( ppsTmp = &g_sSysBase.ex_psFirstReady; NULL != *ppsTmp; ppsTmp = &( *ppsTmp )->tr_psNext )
	{
		if ( *ppsTmp == psThread )
		{
			*ppsTmp = psThread->tr_psNext;
			psThread->tr_psNext = NULL;
			bRemoved = true;
			break;
		}
	}
	if ( bRemoved == false )
	{
		printk( "Error: remove_thread_from_ready() failed to unlink %s from ready list\n", psThread->tr_zName );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static Thread_s *select_thread( void )
{
	int nThisProc = get_processor_id();
	Thread_s *psPrev = CURRENT_THREAD;
	Thread_s *psNext = psPrev;
	Thread_s *psTopProc = g_sSysBase.ex_psFirstReady;
	bigtime_t nCurTime = get_system_time();
	bool bTimedOut;
	int nState;

	if ( psPrev != NULL )
	{
		kassertw( psPrev->tr_nState != TS_READY );
		if ( psPrev->tr_nCurrentCPU != nThisProc )
		{
			printk( "Panic: select_thread() Current thread is running on CPU %d while we run on %d\n", psPrev->tr_nCurrentCPU, nThisProc );
		}

		if ( atomic_read( &psPrev->tr_nInV86 ) > 0 && nThisProc == g_nBootCPU )
		{
			psNext = psPrev;
			goto found;
		}
	}

	if ( psPrev == NULL || psPrev->tr_nState == TS_ZOMBIE )
	{			/* previous task was killed       */
		psNext = g_sSysBase.ex_psFirstReady;

		if ( psPrev != NULL )
		{
			bigtime_t nCPUTime = nCurTime - psPrev->tr_nLaunchTime;

			psPrev->tr_nCPUTime += nCPUTime;
			if ( psPrev->tr_nPriority <= IDLE_PRIORITY )
			{
				g_asProcessorDescs[nThisProc].pi_nIdleTime += nCPUTime;
			}
		}
		g_asProcessorDescs[nThisProc].pi_psCurrentThread = NULL;
		goto found;
	}
	nState = psPrev->tr_nState;

	if ( nState == TS_WAIT || nState == TS_SLEEP || nState == TS_STOPPED )
	{			/* previous task went to sleep  */
		kassertw( atomic_read( &psPrev->tr_nInV86 ) == 0 || nThisProc != g_nBootCPU );
		psNext = g_sSysBase.ex_psFirstReady;
		goto found;
	}

	/* normal preemption cycle        */

	if ( g_sSysBase.ex_psFirstReady == NULL )
	{
		psNext = psPrev;
		goto found;
	}

	kassertw( psPrev->tr_nState == TS_RUN );

	if ( psPrev->tr_nState != TS_RUN )
	{
		printk( "Running thread <%s> has state %ld!!!!!\n", psPrev->tr_zName, psPrev->tr_nState );
	}

	if ( psTopProc->tr_nPriority > psPrev->tr_nPriority )
	{
		psNext = psTopProc;
		goto found;
	}
	bTimedOut = ( nCurTime >= ( psPrev->tr_nLaunchTime + DEFAULT_QUANTUM ) );

	if ( psTopProc->tr_nPriority == psPrev->tr_nPriority && bTimedOut )
	{
		psNext = psTopProc;
		goto found;
	}
	psNext = psPrev;
      found:
	return ( psNext );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void Schedule( void )
{
	Thread_s *psPrev;
	Thread_s *psNext;
	int nFlg;
	int nThisProc;
	bigtime_t nCurTime = get_system_time();

	nFlg = cli();
	sched_lock();

	nThisProc = get_processor_id();

	// If the previous thread was in ready state
	// it means that it tried to sleep, but was woken
	// up by a second CPU between setting its state
	// to TS_WAIT/SLEEP/STOP and the call to schedule()
	// This means that a running thread is in the ready-list

	psPrev = CURRENT_THREAD;

	if ( psPrev != NULL && psPrev->tr_nState == TS_READY )
	{
		remove_thread_from_ready( psPrev );
		psPrev->tr_nState = TS_RUN;
	}

	if ( atomic_read( &g_sSchedSpinLock.sl_nNest ) != 1 )
	{
		if ( CURRENT_THREAD != NULL )
		{
			sched_unlock();
			put_cpu_flags( nFlg );

			printk( "Schedule called while g_sSchedSpinLock.sl_nNest == %d\n", atomic_read( &g_sSchedSpinLock.sl_nNest ) );
			return;
		}
		else
		{
			printk( "PANIC: schedule() no current thread, and spinlock nest count is %d\n", atomic_read( &g_sSchedSpinLock.sl_nNest ) );
		}
	}


	g_bNeedSchedule = false;

	// NOTE: Order is important, since select_thread() might alter CURRENT_THREAD!
	psNext = select_thread();
	psPrev = CURRENT_THREAD;

	// Skip threads we don't like
	for ( ; psNext != NULL; psNext = psNext->tr_psNext )
	{
		if ( atomic_read( &psNext->tr_nInV86 ) > 0 && nThisProc != g_nBootCPU )
		{
			continue;
		}
		if ( psNext->tr_nCurrentCPU != -1 && psNext->tr_nCurrentCPU != nThisProc )
		{
			continue;
		}
		break;
	}

	if ( psNext == NULL )
	{
		psNext = IDLE_THREAD;
	}

	kassertw( psNext != NULL );

	if ( psNext == psPrev )
	{
		sched_unlock();
		put_cpu_flags( nFlg );
		return;
	}

	psNext->tr_nState = TS_RUN;

	if ( psPrev != NULL )
	{
		int64 nCPUTime = nCurTime - psPrev->tr_nLaunchTime;

		psPrev->tr_nCPUTime += nCPUTime;
		if ( psPrev->tr_nPriority <= IDLE_PRIORITY )
		{
			g_asProcessorDescs[nThisProc].pi_nIdleTime += nCPUTime;
		}
		if ( IDLE_THREAD == psPrev )
		{
			psPrev->tr_nState = TS_READY;
		}
		else
		{
			if ( psPrev->tr_nState == TS_RUN )
			{
				add_thread_to_ready( psPrev );
			}
		}
		psPrev->tr_nCurrentCPU = -1;
	}


	if ( psNext != IDLE_THREAD )
	{
		remove_thread_from_ready( psNext );
	}

	psNext->tr_nLaunchTime = nCurTime;

	if ( psPrev != NULL )
	{
		if ( psPrev->tr_nFlags & TF_FPU_DIRTY )
		{
			save_fpu_state( &psPrev->tc_FPUState );
			psPrev->tr_nFlags &= ~TF_FPU_DIRTY;
			stts();
		}
		__asm__ __volatile__( "movl %%cr2,%0":"=r"( psPrev->tr_nCR2 ) );
	}
	g_asProcessorDescs[nThisProc].pi_psCurrentThread = psNext;

//	load_fpu_state( psNext->tc_FPUState );
	__asm__ __volatile__( "movl %0,%%cr2"::"r"( psNext->tr_nCR2 ) );

	psNext->tr_nCurrentCPU = nThisProc;
	psNext->tc_sTSS.gs = g_asProcessorDescs[psNext->tr_nCurrentCPU].pi_nGS;
	Desc_SetBase( psNext->tc_sTSS.gs, ( uint32 )psNext->tr_pThreadData );

	if ( atomic_read( &g_sSchedSpinLock.sl_nNest ) != 1 )
	{
		printk( "Error: Schedule() called with g_nSchedSpinLockNest = %d\n", atomic_read( &g_sSchedSpinLock.sl_nNest ) );
	}

	sched_unlock();
	SwitchCont( psNext->tc_TSSDesc );
	put_cpu_flags( nFlg );
	g_bNeedSchedule = false;
}
