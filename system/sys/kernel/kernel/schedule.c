
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
#include "inc/areas.h"
#include "inc/ptrace.h"

static WaitQueue_s *g_psFirstSleeping = NULL;

SPIN_LOCK( g_sSchedSpinLock, "sched_slock" );

/** Print the list of sleeping threads
 * \par Description:
 * Walk the list of sleeping threads, and print information about each one.  Currently, this
 * information consists of the resume time in seconds, and the thread handle.  This function is
 * added to the serial debug menu under the commmand "ls sleep"
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * \param argc	Number of elements in argv[]
 * \param argv	Arguments to debug command.  Currently ignored
 * \sa
 ****************************************************************************/
static void db_print_sleep_list( int argc, char **argv )
{
	WaitQueue_s *psTmp;
	int nFlg;
	int i = 0;

	nFlg = cli();
	sched_lock();
	for ( psTmp = g_psFirstSleeping; NULL != psTmp; psTmp = psTmp->wq_psNext )
	{
		Thread_s *psThread;
		
		if( i >= 10000 )
			break;

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

/** Initialize the scheduler module
 * \par Description:
 * This function will initialize the scheduler module.  It's called, along with other module
 * initalization functions, from kernel_init().  Currently, this initializes globals scheduler
 * lists, and registers the scheduler debug commands.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param none
 * \sa kernel_init
 ****************************************************************************/
void init_scheduler( void )
{
	DLIST_HEAD_INIT( &g_sSysBase.ex_sFirstReady );
	DLIST_HEAD_INIT( &g_sSysBase.ex_sFirstExpired );
	register_debug_cmd( "ls_sleep", "list sleep-list nodes", db_print_sleep_list );
}

/** Lock the scheduler
 * \par Description:
 * This function locks the scheduler.  It's wrapped by the sched_lock() macro, which asserts that
 * the lock was successfully obtained.  Officially, this locks all the various data structures owned
 * by the scheduler, but primarily the thread lists.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Scheduler lock
 * \par Warning:
 * This version should not be called.  Use the sched_lock() wrapper.
 * \param none
 * \return Zero on success, negative error code on failure
 * \sa sched_lock, sched_unlock
 ****************************************************************************/
int __sched_lock( void )
{
	return ( spinlock( &g_sSchedSpinLock ) );
}

/** Unlock the scheduler
 * \par Description:
 * This function unlocks the scheduler.  See __sched_lock() for a description of the scheduler lock.
 * \par Note:
 * \par Locks Required:
 * Scheduler lock
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param none
 * \sa sched_lock, __sched_lock
 ****************************************************************************/
void sched_unlock( void )
{
	spinunlock( &g_sSchedSpinLock );
}

#define CHECK_SCHED_LOCK kassertw( atomic_read( &g_sSchedSpinLock.sl_nLocked ) == 1 );

/** Add an entry to a wait queue
 * \par Description:
 * Add the given entry to the given wait queue.  Wait queues are used to put threads to sleep on a
 * certain event.  When the even occurs, some or all of the threads waiting on the queue are woken
 * up.  Note that this does not change the thread itself, it merely manages the queue.  Generally,
 * sleep_on_queue() would be used to put a thread to sleep.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * \param bLock		Take the scheduler lock
 * \param ppsList	Waitqueue to add to
 * \param psNode	Entry to add
 * \sa sleep_on_queue, remove_from_waitlist
 ****************************************************************************/
void add_to_waitlist( bool bLock, WaitQueue_s **ppsList, WaitQueue_s *psNode )
{
	WaitQueue_s *psHead;
	
	int nFlg = cli();
	if( bLock )
		sched_lock();
		
	CHECK_SCHED_LOCK

	psHead = *ppsList;

	if ( psNode >= ( WaitQueue_s * )0x80000000 )
	{
		printk( "PANIC : add_to_waitlist() attempt to add node from user-space!!!\n" );
		if( bLock )
			sched_unlock();
		put_cpu_flags( nFlg );
		return;
	}

	psNode->wq_bIsMember = true;

	if ( psHead == NULL )
	{
		*ppsList = psNode;
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
	if( bLock )
		sched_unlock();
	put_cpu_flags( nFlg );
}

/** Add a wait queue to the global sleep list
 * \par Description:
 * This will add the given wait queue to the global sleep list. This list is sorted by expire 
 * time of the wait queues.  First, see if it should be added first.  
 * If so, add it, otherwise, walk the list to find the insertion point.
 * \par Note:
 * \par Locks Required:
 * Interrupts
 * Scheduler lock
 * \par Locks Taken:
 * none
 * \par Warning:
 * \param bLock		Take the scheduler lock
 * \param psNode	Wait queue to add
 * \sa add_to_sleeplist
 ****************************************************************************/

void add_to_sleeplist( bool bLock, WaitQueue_s *psNode )
{
	WaitQueue_s *psTmp;
	int i = 0;
	int nFlg = cli();
	if( bLock )
		sched_lock();
		
	CHECK_SCHED_LOCK
	
	if( psNode->wq_bIsMember )
		printk( "Error: Trying to add already present node %i %i\n", psNode->wq_hThread, (int)psNode->wq_nResumeTime );

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
		if( bLock )
			sched_unlock();
		put_cpu_flags( nFlg );
		return;
	}

	for ( psTmp = g_psFirstSleeping;; psTmp = psTmp->wq_psNext )
	{
		if ( i++ > 10000 )
		{
			printk( "add_to_sleeplist() looped 10000 times, give up\n" );
			db_print_sleep_list( 0, NULL );
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
			goto end;
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
end:
	if( bLock )
		sched_unlock();
	put_cpu_flags( nFlg );
	
//done:
}


/** Internal, unlocked implementation of remove_from_sleeplist
 * \par Description:
 * This does the actual work of remove_from_sleeplist.  It removes the given wait queue from the
 * global list of sleeping wait queues.
 * \par Note:
 * \par Locks Required:
 * Interrutps
 * Scheduler lock
 * \par Locks Taken:
 * none
 * \par Warning:
 * \param psNode	Wait queue to remove
 * \sa remove_from_sleeplist
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

/** Remove a wait queue from the global sleep list
 * \par Description:
 * This will remove the given wait queue from the global sleep list.  This is necessary before
 * freeing the queue.  The actual work is done by do_remove_from_sleeplist().
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * \param psNode	Wait queue to remove
 * \sa do_remove_from_sleeplist, add_to_sleeplist
 ****************************************************************************/
void remove_from_sleeplist( WaitQueue_s *psNode )
{
	int nFlg;

	nFlg = cli();
	sched_lock();
	do_remove_from_sleeplist( psNode );
	sched_unlock();
	put_cpu_flags( nFlg );
}

/** Remove an entry from a wait queue
 * \par Description:
 * Remove the given entry from the given wait queue.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * \param ppsList	Wait queue to remove from
 * \param psNode	Entry to remove
 * \return
 * \sa
 ****************************************************************************/
void remove_from_waitlist( bool bLock, WaitQueue_s **ppsList, WaitQueue_s *psNode )
{
	int nFlg = cli();

	if( bLock )
		sched_lock();
		
	CHECK_SCHED_LOCK

	kassertw( psNode->wq_bIsMember );

	psNode->wq_bIsMember = false;

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

	if( bLock )
		sched_unlock();
	put_cpu_flags( nFlg );
}

/** Wake up the given wait queue
 * \par Description:
 * Wake up one or all threads from the given wait queue.  A woken thread is marked as ready and
 * added to the ready list.  The wait queue element containing the woken thread has it's return
 * value set to the given return value.  If <bAll> is true, then all the threads on the queue are
 * woken, otherwise only the first thread is woken.  No threads are removed from the queue.  If any
 * threads were woken, then g_bNeedSchedule is set so that a schedule will happen at some point in
 * the future.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * \param bLock		Take the scheduler lock
 * \param psList	Wait queue to wake
 * \param nReturnCode	Return value to give the newly woken thread
 * \param bAll		If true, wake all threads, otherwise wake first.
 * \return Number of threads woken.
 * \sa
 ****************************************************************************/
int wake_up_queue( bool bLock, WaitQueue_s *psList, int nReturnCode, bool bAll )
{
	int nThreadsWoken = 0;
	
	int nFlg = cli();
	
	if( bLock )
		sched_lock();
		
	CHECK_SCHED_LOCK

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

	
	if ( nThreadsWoken > 0 )
	{
		g_bNeedSchedule = true;
	}
	if( bLock )
		sched_unlock();
	put_cpu_flags( nFlg );
	return ( nThreadsWoken );
}

/** Put the current thread to sleep on a wait queue
 * \par Description:
 * Put the current thread to sleep on the given wait queue.  This marks the thread as waiting, adds
 * it to the wait queue, and schedules.  On return from schedule, the thread is removed from the
 * wait queue again.  When this returns, the thread has been woken up again.  The wait queue must
 * already be initialized.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * \param ppsList	Wait queue on which to sleep
 * \return return value from wakeup of wait queue
 * \sa
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
	add_to_waitlist( false, ppsList, &sWaitNode );

	sched_unlock();
	put_cpu_flags( nFlg );

      /*** NOTE: To get good real-time performance, we must reprogram the PIT timer here	***/

	Schedule();

	nFlg = cli();
	sched_lock();

	remove_from_waitlist( false, ppsList, &sWaitNode );

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
/** Wake up sleeping wait queues
 * \par Description:
 * Wake up any wait queues that have timed out.  It is called periodically from the timer
 * interrupt.  Walk the sleep list, and wake any queues that have expired.  If the queue has a
 * thread, wake that thread.  Otherwise, the queue is a timer, call it's callback.  If the timer was
 * not a oneshot timer, re-add it to the sleeplist at it's next timeout.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * \param nCurTime	The current tim
 * \sa
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
		kassertw( psNode->wq_bIsMember == true );
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
	while ( psFirstTimer != NULL )
	{
		WaitQueue_s *psNode = psFirstTimer;

		psFirstTimer = psNode->wq_psNext;
		if ( psNode->wq_bOneShot == false )
		{
			psNode->wq_nResumeTime = nCurTime + psNode->wq_nTimeout;
			add_to_sleeplist( true, psNode );
		}
		psNode->wq_pfCallBack( psNode->wq_pUserData );
	}
	put_cpu_flags( nFlg );
}

/** Create a timer
 * \par Description:
 * Create a new timer. Internally, a timer is a wait queue with no associated thread.  Allocate the
 * new wait queue, initialize it, and return it.
 * \par Note:
 * delete_timer() must be called to free the timer.
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param none
 * \return Newly allocated timer
 * \sa start_timer, delete_timer
 ****************************************************************************/
ktimer_t create_timer( void )
{
	WaitQueue_s *psNode = kmalloc( sizeof( WaitQueue_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );

	if ( psNode == NULL )
	{
		return ( NULL );
	}
	psNode->wq_hThread = -1;
	psNode->wq_bIsMember = false;
	return ( psNode );
}

/** Start a timer
 * \par Description:
 * Start the given timer. The timer is set to expire in <nPeriode> ticks.  If <bOneShot> is true,
 * the timer is not repeating.  When fired, the timer will call the given callback with the given
 * data.  Initialize the timer with the given information, and add it to the sleeplist, removing it
 * first if it was already there.
 * \par Note:
 * If <pfCallback> is NULL, the timer is not added, merely removed
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * \param hTimer	Timer to start
 * \param pfCallback	Callback to call when timer fires
 * \param pData		Data to pass to the callback
 * \param nPeriode	Time to fire in ticks
 * \param bOneShot	If true, timer is one shot, otherwise, timer is periodic
 * \sa
 ****************************************************************************/
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
		add_to_sleeplist( false, psNode );
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return;
}

/** Delete a timer
 * \par Description:
 * Delete the give timer, removing it from the sleeplist first.  The timer is freed.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param hTimer	Timer to delete
 * \sa create_timer
 ****************************************************************************/
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

uint32 old_Delay( const uint32 nMicros )
{
#if 0
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
#endif
	return ( -ENOSYS );
}

/** Put the current thread to sleep for some time
 * \par Description:
 * Put the current thread to sleep for the given number of ticks.  Create a sleeping wait queue, and
 * set it to time out after <nTimeout> ticks.  Add the current thread to it, and put the current
 * thread to sleep.  Add the wait queue to the sleep list, and schedule.  When schedule returns, the
 * thread has been woken up.  Remove the thread from the sleep list.
 * \par Note:
 * We need the scheduler lock to modify the thread's state, so we call do_add_to_sleeplist().
 * However, rather than take the locks ourselves after schedule returns, we just call the locked
 * version of remove_from_sleeplist.
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * \param nTimeout	Number of ticks to sleep
 * \return 0 if the thread slept the whole timeout, -EINTR if it was woken up early.
 * \sa
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
	sWaitNode.wq_bIsMember = false;

	nFlg = cli();
	sched_lock();

	psThread->tr_nState = TS_SLEEP;

	add_to_sleeplist( false, &sWaitNode );

	sched_unlock();
	put_cpu_flags( nFlg );

      /*** NOTE: To get good real-time performance, we must reprogram the PIT timer here	***/

	Schedule();

	remove_from_sleeplist( &sWaitNode );

	return ( ( sWaitNode.wq_nResumeTime > get_system_time() )? -EINTR : 0 );
}

/** Put the current thread to sleep
 * \par Description:
 * This is the syscall entry point for putting a thread to sleep.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param nTimeout	Number of ticks to sleep
 * \return 0 if slept the entire timeout, -EINTR otherwise.
 * \sa
 ****************************************************************************/
status_t sys_snooze( bigtime_t nTimeout )
{
	return ( snooze( nTimeout ) );
}

/** Set the scheduling priority of a thread
 * \par Description:
 * Set the scheduling priority of the thread with the given id to the given value, then schedule.
 * Look up the given thread id.  If such a thread exists, set it's priority to the given value, and
 * schedule.  Once schedule returns, return the old priority
 * \par Note:
 * XXXDFG constrain given priority to some limits.
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * Schedules
 * \param hThread	ID of thread.  If -1, use current thread.
 * \param nPriority	New priority of thread
 * \return old priority of thread
 * \sa
 ****************************************************************************/
status_t set_thread_priority( const thread_id hThread, const int nPriority )
{
	Thread_s *psThread;
	int nOldPri = 0;
	int nFlg = cli();

	sched_lock();

	psThread = get_thread_by_handle( hThread );

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

/** Set the priority of a thread
 * \par Description:
 * Syscall entry for set_thread_priority.  Set the priority of the given thread to the given value.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * None
 * \par Warning:
 * Schedules
 * \param hThread	ID of thread.  If -1, use current thread.
 * \param nPriority	New priority of thread
 * \return old priority of thread
 * \sa
 ****************************************************************************/
int sys_set_thread_priority( const thread_id hThread, const int nPriority )
{
	return ( set_thread_priority( hThread, nPriority ) );
}


/** Set the target cpu of one thread
 * \par Description:
 * Set the target cpu of one thread. Call Schedule() to be sure that the
 * thread is really running on the target cpu.
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \param hThread	ID of thread.  If -1, use current thread.
 * \param nCpu		Processor
 * \return 0 if sucessful.
 * \sa
 ****************************************************************************/
status_t set_thread_target_cpu( const thread_id hThread, const int nCpu )
{
	Thread_s *psThread;
	int nError = 0;
	int nFlg = cli();
	
	sched_lock();

	psThread = get_thread_by_handle( hThread );

	if ( psThread == NULL ) {
		nError = -EINVAL;
		goto exit;
	}
	
	if( nCpu == -1 ) {
		psThread->tr_nTargetCPU = -1;
		goto exit;
	}
	
	if( nCpu < 0 || nCpu >= MAX_CPU_COUNT ) {
		nError = -EINVAL;
		goto exit;		
	}
	
	if( g_asProcessorDescs[nCpu].pi_bIsRunning == false ) {
		nError = -EINVAL;
		goto exit;				
	}
	psThread->tr_nTargetCPU = nCpu;
exit:
	sched_unlock();
	put_cpu_flags( nFlg );
	Schedule();
	return ( nError );
}




/** Suspend the current thread
 * \par Description:
 * Suspend the current thread.  If there's no pending signals, mark the thread as waiting, and
 * schedule.   Note that there must be some other way to wake up this thread, or it will never wake
 * up.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * Schedules
 * \param None
 * \return 0 if suspended, -EINTR if pending signals
 * \sa
 ****************************************************************************/
status_t suspend( void )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	int nError;
	int nFlg;

	if ( is_signal_pending() == false )
	{
		nFlg = cli();
		sched_lock();
		psMyThread->tr_nState = TS_WAIT;
		sched_unlock();
		put_cpu_flags( nFlg );

		Schedule();
		nError = 0;
	}
	else
	{
		nError = -EINTR;
	}

	return ( nError );
}

/** Suspend a thread
 * \par Description:
 * Suspend the given thread.  If the given thread is the current thread, call suspend().  Otherwise,
 * send a STOP signal to the thread.
 * XXXDFG should probably error if current thread
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * None
 * \par Warning:
 * May schedule
 * \param hThread	ID of thread to suspend
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
status_t sys_suspend_thread( const thread_id hThread )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	int nError;

	if ( hThread == psMyThread->tr_hThreadID )	/* We are about to suspend our self     */
	{
		nError = suspend();
	}
	else
	{
		nError = sys_kill( hThread, SIGSTOP );
	}
	return ( nError );
}

/** Stop the current thread
 * \par Description:
 * Stop the current thread.  If <bNotifyParent> is true, and there is a parent to this thread, send
 * the parent a SIGCHLD.  Mark the thread as stopped, and schedule.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * Schedules
 * \param bNotifyParent		If true, send a SIGCHLD to the parent of this thread
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Resume a thread
 * \par Description:
 * Resume the given thread.  It must be either waiting or sleeping.  call wakeup_thread() to do the
 * work.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * \par Warning:
 * \param nThread	ID of thread to resume
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
status_t sys_resume_thread( const thread_id hThread )
{
	return ( wakeup_thread( hThread, false ) );
}

/** Wake up a thread
 * \par Description:
 * Wake up the given thread from waiting, sleeping, or, if <bWakeUpSuspended>, stopped states.  Look
 * up the thread from the handle.  If it exists, and the above state holds true, make it ready.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupt
 * Scheduler lock
 * \par Warning:
 * \param hThread		ID of thread to wake
 * \param bWakeupSuspended	If true, wake from stopped state as well
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
status_t do_wakeup_thread( thread_id hThread, bool bWakeupSuspended )
{
	Thread_s *psThread;
	int nError = 0;
	
	psThread = get_thread_by_handle( hThread );

	if ( psThread != NULL )
	{
		if ( psThread->tr_nState == TS_WAIT || psThread->tr_nState == TS_SLEEP || ( bWakeupSuspended && psThread->tr_nState == TS_STOPPED ) )
		{
			add_thread_to_ready( psThread );
		}
	}
	else
	{
		nError = -EINVAL;
	}
	return ( nError );
}

status_t wakeup_thread( thread_id hThread, bool bWakeupSuspended )
{
	int nError = 0;
	int nFlags = cli();

	sched_lock();
	nError = do_wakeup_thread( hThread, bWakeupSuspended );
	sched_unlock();
	put_cpu_flags( nFlags );
	return ( nError );
}

/** Start a thread
 * \par Description:
 * Start a thread that is not running.  It may be in stopped, wait, or sleep.  Call wakeup_thread()
 * to do the work.
 * \par Note:
 * \par Locks Required:
 * \par Locks Taken:
 * \par Warning:
 * \param
 * \return
 * \sa
 ****************************************************************************/
status_t start_thread( thread_id hThread )
{
	return ( wakeup_thread( hThread, true ) );
}

/** Find a dead child thread
 * \par Description:
 * Find a dead child thread of the current thread.  If <hPid> is -1, find any dead child.  If <hPid>
 * is > 0, find the child with that thread ID.  Otherwise, if <hGroup> is not -1, find any child in
 * the group <hGroup>.  Loop all threads looking for children of the current thread.  Check for the
 * above circumstances, and return appropriately.
 * XXXDFG Maybe optimize by adding an actual list of children into the parent.
 * \par Note:
 * \par Locks Required:
 * Scheduler lock
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param hPid		Thread ID to find (>0), or any (-1), or group (otherwise)
 * \param hGroup	Group ID to find.
 * \param nOptions	Options.  Currently checked for untraced.
 * \return Thread if found, NULL otherwise
 * \sa sys_wait4
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
			if (  ( psChild->tr_nState == TS_STOPPED ) &&
			     !( atomic_read( &psChild->tr_nPTraceFlags ) & PT_PTRACED ) &&
			      ( psChild->tr_nExitCode == 0 || ( nOptions & WUNTRACED ) == 0 ) )
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
			if ( hGroup != -1 )
			{	/* Any children belonging to group hGroup */
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

/** Wait for children to die
 * \par Description:
 * Implementation of the wait4 syscall.  Wait for children to die.  What children to wait for is
 * determined as follows: if <hPid> > 0, wait only for that PID.  If <hPid> is -1, wait for any
 * children.  If <hPid> is 0, wait for any child in the same thread group as the current thread.
 * Otherwise, wait for any child in the thread group -<hPid>.  First, check to see if any such
 * children exist.  If not, return error.  Next, loop waiting for a correct child to die.  In each
 * loop iteration, if no child had died, return if WNOHANG was given, or if a signal is pending.
 * Otherwise, schedule, waiting for a SIGCHLD.  If a child was found, check it's state.  If it was
 * stopped, set it's return value to 0, so it won't be returned again, copy it's info to userspace,
 * and return it.  If was zombie (ie, it exited), copy it's info to userspace, wake up Init, delete
 * the child, and return.
 * \par Note:
 * \par Locks Required:
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * may schedule
 * \param hPid		Thread ID to wait for (or any or group, see description)
 * \param pnStatLoc	Stat buffer to fill with child's information (or NULL)
 * \param nOptions	Options flags.
 * \param psRUsage	Rusage storage buffer.  Currently unused.
 * \return PID of dead child on success, 0 if no child died and WNOHANG was given, negative error code on failure
 * \sa
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

	/* Return immediately if SA_NOCLDWAIT is set on SIGCHLD handler.  */
	if ( psThread->tr_apsSigHandlers[SIGCHLD - 1].sa_flags & SA_NOCLDWAIT )
	{
		return ( -ECHILD );
	}

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

/** Wait for a given child to exit
 * \par Description:
 * Wait for the child with the given thread ID to exit.  Calls sys_wait4 to do the work.
 * \par Note:
 * \par Locks Required:
 * none
 * \par Locks Taken:
 * none
 * \par Warning:
 * \param hPid		Thread ID to wait for (or any or group, see description)
 * \param pnStatLoc	Stat buffer to fill with child's information (or NULL)
 * \param nOptions	Options flags.
 * \return PID of dead child on success, 0 if no child died and WNOHANG was given, negative error code on failure
 * \sa
 ****************************************************************************/
pid_t sys_waitpid( const pid_t hPid, int *const pnStatLoc, int const nOptions )
{
	return ( sys_wait4( hPid, pnStatLoc, nOptions, NULL ) );
}

/** Wait for a thread to exit
 * \par Description:
 * Wait for the thread with the given id to exit.  First, check to see if the thread exists.  If
 * not, return error.  If the thread is not already zombie, add the current thread to it's
 * termination wait list, and schedule.  When schedule returns, if the target thread still exists,
 * remove is from it's waitlist.
 * \par Note:
 * \par Locks Required:
 * None
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * Schedules
 * \param hThread	ID of thread to wait for
 * \return Return value of thread, or negative error code on failure
 * \sa
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
		add_to_waitlist( false, &psThread->tr_psTermWaitList, &sWaitNode );

		sched_unlock();
		put_cpu_flags( nFlg );

		Schedule();

		nFlg = cli();
		sched_lock();

		psThread = get_thread_by_handle( hThread );
		if ( psThread != NULL )
		{
			remove_from_waitlist( false, &psThread->tr_psTermWaitList, &sWaitNode );
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

/** Add a thread to the ready list
 * \par Description:
 * Add the given thread to the list of threads ready to run.  The list is sorted in descending
 * priority order.  Find the insertion point, and insert, otherwise append.
 * \par Note:
 * \par Locks Required:
 * Scheduler lock
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param psThread	Thread to add to the ready list
 * \sa
 ****************************************************************************/
void add_thread_to_ready( Thread_s *psThread )
{
	Thread_s *psTmp;

	if ( IDLE_THREAD == psThread )
	{
		printk( "Error: add_thread_to_ready() attempt to add idle thread to ready list\n" );
		return;
	}

	if ( TS_READY != psThread->tr_nState )
	{
		reset_thread_quantum( psThread );
		psThread->tr_nState = TS_READY;

		for ( psTmp = DLIST_FIRST( &g_sSysBase.ex_sFirstReady ); NULL != psTmp; psTmp = DLIST_NEXT( psTmp, tr_psNext ) )
		{
			if ( psThread->tr_nPriority > ( psTmp )->tr_nPriority )
			{
				DLIST_PREPEND( psTmp, psThread, tr_psNext );
				return;
			}
			if ( DLIST_NEXT( psTmp, tr_psNext ) == NULL )
				break;
		}

		if ( DLIST_IS_EMPTY( &g_sSysBase.ex_sFirstReady ) )
		{
			DLIST_ADDHEAD( &g_sSysBase.ex_sFirstReady, psThread, tr_psNext );
		}
		else
		{
			DLIST_APPEND( psTmp, psThread, tr_psNext );
		}
	}
}

/** Add a thread to the expired list
 * \par Description:
 * Add the given thread to the list of threads that have exhausted their quantum and are still ready to run.
 *  The list is sorted in descending priority order.  Find the insertion point, and insert, otherwise append.
 * \par Note:
 * \par Locks Required:
 * Scheduler lock
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param psThread	Thread to add to the expired list
 * \sa
 ****************************************************************************/
void add_thread_to_expired( Thread_s *psThread )
{
	Thread_s *psTmp;

	if ( IDLE_THREAD == psThread )
	{
		printk( "Error: add_thread_to_expired() attempt to add idle thread to expired list\n" );
		return;
	}

	if ( TS_READY != psThread->tr_nState )
	{
		// For expired thread, always give it back it's full quantum
		reset_thread_quantum( psThread );

		psThread->tr_nState = TS_READY;

		for ( psTmp = DLIST_FIRST( &g_sSysBase.ex_sFirstExpired ); NULL != psTmp; psTmp = DLIST_NEXT( psTmp, tr_psNext ) )
		{
			if ( psThread->tr_nPriority > ( psTmp )->tr_nPriority )
			{
				DLIST_PREPEND( psTmp, psThread, tr_psNext );
				return;
			}
			if ( DLIST_NEXT( psTmp, tr_psNext ) == NULL )
				break;
		}

		if ( DLIST_IS_EMPTY( &g_sSysBase.ex_sFirstExpired ) )
		{
			DLIST_ADDHEAD( &g_sSysBase.ex_sFirstExpired, psThread, tr_psNext );
		}
		else
		{
			DLIST_APPEND( psTmp, psThread, tr_psNext );
		}
	}
}

/** Remove a thread from the ready or expired list (can be either since it's done in the same way)
 * \par Description:
 * Remove the given thread from the list of threads ready to run.  This is just a DLIST_REMOVE.
 * \par Note:
 * \par Locks Required:
 * None XXXDFG  Is the ready list locked somehow?
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param psThread	Thread to remove
 * \sa
 ****************************************************************************/
static void remove_thread_from_ready( Thread_s *psThread )
{
	DLIST_REMOVE( psThread, tr_psNext );
}

/** Get next thread from ready queue
 * \par Description:
 * Check if ready queue is empty.  If empty, swap ready and expired queues.
 * Return first thread on ready queue.
 * \par Note:
 * \par Locks Required:
 * Interrupts
 * Scheduler lock
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param none
 * \return Pointer to first thread on ready queue.
 * \sa Schedule
 ****************************************************************************/
Thread_s *swap_and_get_next_ready_thread( void )
{
	if ( DLIST_IS_EMPTY( &g_sSysBase.ex_sFirstReady ) && ! DLIST_IS_EMPTY( &g_sSysBase.ex_sFirstExpired ) )
	{
		// Swap ready and expired queues
		g_sSysBase.ex_sFirstReady.list_head = g_sSysBase.ex_sFirstExpired.list_head;
		g_sSysBase.ex_sFirstReady.list_head->tr_psNext.list_pprev = 
			&(g_sSysBase.ex_sFirstReady.list_head);
		g_sSysBase.ex_sFirstExpired.list_head = NULL;
	}

	return DLIST_FIRST( &g_sSysBase.ex_sFirstReady );
}

/** Calculate quantum for thread
 * \par Description:
 * Higher-priority threads should get a larger timeslice.  This function calculates
 * the timeslice and sets tr_nQuantum, which is then decremented as the process runs.
 * \par Note:
 * \par Locks Required:
 * Interrupts
 * Scheduler lock
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param none
 * \return void
 * \sa Schedule
 ****************************************************************************/
void reset_thread_quantum( Thread_s *psThread )
{
        /*
         * Calculate quantum for this process.  It is scaled between 5ms and 800ms.
         * processes with "idle" priority (-100 and under) get 5ms
         * those with "normal priority" (0) get 100ms
         * those with "realtime priority" (150 and above) get 800ms
         * everything else is scaled.
         *
         * if constants in atheos/threads.h change, this should probably change.
         */
       if ( psThread->tr_nPriority == NORMAL_PRIORITY )
                psThread->tr_nQuantum = 100000;
        else
        {
                if ( psThread->tr_nPriority < NORMAL_PRIORITY )
                        psThread->tr_nQuantum = max(0, psThread->tr_nPriority + 100) * 950 + 5000;
                else
                        psThread->tr_nQuantum = min(150, psThread->tr_nPriority) * 4666 + 100000;
        }
}

/** Select the best thread to run
 * \par Description:
 * Select the best thread to run next.  This is the guts of the scheduler, the function that
 * actually determines what get's scheduled.  First, if there is a current thread, and it's in
 * virtual 8086 mode, and we're on the first CPU, then reschedule it.  Otherwise, if there is no
 * current thread, or it's a zombie, grab the first thread off the ready list.  If the current
 * thread is waiting, asleep, or stopped, grab the first thread off the ready list.  If the ready
 * list is empty, reschedule the current thread.  If the first process on the ready list has a
 * higher priority than the current thread, preempt the current thread.  If their priorities are the
 * same, and the current thread has used up it's quantum, preempt the current thread.  Otherwise,
 * reschedule the current thread.  This function makes no changes to any list, it merely returns a
 * pointer to the next thread to be run.
 * \par Note:
 * \par Locks Required:
 * Interrupts
 * Scheduler lock
 * \par Locks Taken:
 * None
 * \par Warning:
 * \param none
 * \return Pointer to next thread to run
 * \sa Schedule
 ****************************************************************************/
static Thread_s *select_thread( int nThisProc, int64 nCurTime, bool *bTimedOut )
{
	Thread_s *psPrev = CURRENT_THREAD;
	Thread_s *psNext = psPrev;
	Thread_s *psTopProc = swap_and_get_next_ready_thread();
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
	{
		/* previous task was killed       */
		psNext = psTopProc;

#if 0
		if ( psPrev != NULL )
		{
			uint64 nCPUTime = nCurTime - psPrev->tr_nLaunchTime;

			psPrev->tr_nCPUTime += nCPUTime;
			if ( psPrev->tr_nPriority <= IDLE_PRIORITY )
			{
				g_asProcessorDescs[nThisProc].pi_nIdleTime += nCPUTime;
			}
		}
#endif
		g_asProcessorDescs[nThisProc].pi_psCurrentThread = NULL;
		goto found;
	}
	nState = psPrev->tr_nState;

	if ( nState == TS_WAIT || nState == TS_SLEEP || nState == TS_STOPPED )
	{
		/* previous task went to sleep  */
		kassertw( atomic_read( &psPrev->tr_nInV86 ) == 0 || nThisProc != g_nBootCPU );
		psNext = psTopProc;
		goto found;
	}

	/* normal preemption cycle */

	if ( psTopProc == NULL )
	{
		psNext = psPrev;
		goto found;
	}

	kassertw( psPrev->tr_nState == TS_RUN );

	if ( psPrev->tr_nState != TS_RUN )
	{
		printk( "Running thread <%s> has state %d!!!!!\n", psPrev->tr_zName, psPrev->tr_nState );
	}

	if ( psTopProc->tr_nPriority > psPrev->tr_nPriority )
	{
		psNext = psTopProc;
		goto found;
	}
	*bTimedOut = ( nCurTime >= ( psPrev->tr_nLaunchTime + psPrev->tr_nQuantum ) );

	if (psTopProc != NULL && *bTimedOut) // NEW
	{
		psNext = psTopProc;
		goto found;
	}
	psNext = psPrev;
found:
	return ( psNext );
}

/** Schedule the next thread to run
 * \par Description:
 * Schedule a (possibly) different thread on the current CPU.   First, check if the current thread
 * was marked ready by another CPU.  If so, remove it from the ready list and mark it running.  If
 * the Scheduler lock was taken already, bail, because that lock protects CURRENT_THREAD.  Call
 * select_thread() to pick the next thread to run.  If the thread returned was in virtual x86 mode
 * and this isn't the boot CPU, or the thread returned was bound to a different CPU, skip to the
 * next thread.  If we didn't find a suitable thread, use the idle thread.  If, at this point, we
 * are switching to the current thread, there's nothing to do, return.  Otherwise, mark the next
 * thread running, charge the current thread for it's CPU time, and add it to the ready list if it's
 * still runnable.  Remove the next thread from the ready list, and set it's launch time.  If the
 * previous thread dirtied the FPU, save the FPU sate for it.  Actually switch the CPU contexts to
 * the new thread.  Return into the new thread.
 * \par Note:
 * \par Locks Required:
 * Must not have Scheduler Lock taken
 * \par Locks Taken:
 * Interrupts
 * Scheduler lock
 * \par Warning:
 * Schedules (duh)
 * \param none
 * \return Nothing, but returns into new thread.
 * \sa
 ****************************************************************************/
void DoSchedule( SysCallRegs_s* psRegs )
{
	Thread_s *psPrev;
	Thread_s *psNext;
	int nFlg;
	int nThisProc;
	int64 nCurTime;
	
	bool bTimedOut = false;

	nFlg = cli();
	sched_lock();
		
	nThisProc = get_processor_id();
	nCurTime = get_cpu_time( nThisProc );
	g_asProcessorDescs[nThisProc].pi_nCPUTime = nCurTime;


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

	psNext = select_thread( nThisProc, nCurTime, &bTimedOut );

	// Skip threads we don't like
	for ( ; psNext != NULL; psNext = DLIST_NEXT(psNext, tr_psNext) )
	{
		if ( atomic_read( &psNext->tr_nInV86 ) > 0 && nThisProc != g_nBootCPU )
		{
			continue;
		}
		if ( psNext->tr_nCurrentCPU != -1 && psNext->tr_nCurrentCPU != nThisProc )
		{
			continue;
		}
		if ( psNext->tr_nTargetCPU != -1 && psNext->tr_nTargetCPU != nThisProc )
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
		if ( psPrev->tr_nPriority <= IDLE_PRIORITY )
		{
			g_asProcessorDescs[nThisProc].pi_nIdleTime += nCPUTime;
		}
		psPrev->tr_nCPUTime += nCPUTime;
		if ( IDLE_THREAD == psPrev )
		{
			psPrev->tr_nState = TS_READY;
		}
		else
		{
			// Charge thread's quantum for it's use
			psPrev->tr_nQuantum -= nCPUTime;

			if ( psPrev->tr_nState == TS_RUN )
			{
				// If the thread had used it's quantum (bTimedOut), then add to expired queue.
				// otherwise add to ready queue
				if (bTimedOut)
					add_thread_to_expired( psPrev );
				else
					add_thread_to_ready( psPrev );
			}
		}
		psPrev->tr_nCurrentCPU = -1;
	}

	/* Check if we should shutdown this cpu */
	if( g_bKernelInitialized && g_asProcessorDescs[nThisProc].pi_bIsRunning == false ) {
		sched_unlock();
		shutdown_processor(); // never returns
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
		}
		
		__asm__ __volatile__( "movl %%cr2,%0":"=r"( psPrev->tr_nCR2 ) );
		psPrev->tr_pESP = (void*)psRegs;
		psPrev->tr_nLastEIP = psRegs->eip;
	}
	g_asProcessorDescs[nThisProc].pi_psCurrentThread = psNext;

	//load_fpu_state( &psNext->tc_FPUState );
	__asm__ __volatile__( "movl %0,%%cr2"::"r"( psNext->tr_nCR2 ) );

	psNext->tr_nCurrentCPU = nThisProc;
	set_gdt_desc_base( g_asProcessorDescs[psNext->tr_nCurrentCPU].pi_nGS, ( uint32 )psNext->tr_pThreadData );

	if ( atomic_read( &g_sSchedSpinLock.sl_nNest ) != 1 )
	{
		printk( "Error: Schedule() called with g_nSchedSpinLockNest = %d\n", atomic_read( &g_sSchedSpinLock.sl_nNest ) );
	}
	
	g_asProcessorDescs[psNext->tr_nCurrentCPU].pi_sTSS.esp0 = psNext->tr_pESP0;
	g_asProcessorDescs[psNext->tr_nCurrentCPU].pi_sTSS.IOMapBase = 104;

	if ( psNext->tr_nDebugReg[7] != 0 )
		load_debug_regs( psNext->tr_nDebugReg );
	else
		/* With an x86 CPU, clearing the debug control register on every task switch is less expensive than
		   keeping track of the register's contents and clearing it only when neccessary. */
		clear_debug_regs();
	
	stts();
	set_page_directory_base_reg( psNext->tr_psProcess->tc_psMemSeg->mc_pPageDir );
	g_bNeedSchedule = false;
	
	sched_unlock();

	__asm__ __volatile__( "movl %0,%%gs\n\t" "movl %1,%%esp\n\t" "jmp ret_from_sys_call":	/* no outputs */
 		:"r"( g_asProcessorDescs[psNext->tr_nCurrentCPU].pi_nGS ), "r"( psNext->tr_pESP ) );
}

void sys_do_schedule( void* pPtr )
{
	SysCallRegs_s *psRegs = ( SysCallRegs_s * ) &pPtr;
	DoSchedule( psRegs );
}







