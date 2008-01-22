
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/semaphore.h>
#include <atheos/filesystem.h>
#include <posix/fcntl.h>
#include <posix/unistd.h>
#include <posix/errno.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "vfs.h"


static FileLockRec_s *g_psFirstFileLock = NULL;
static sem_id g_hFileLockMutex = -1;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int detect_deadlock( FileLockRec_s *psNewLock, FileLockRec_s *psOldLock )
{
	FileLockRec_s *psLock;
	thread_id hOldLockThread;

	hOldLockThread = psOldLock->fl_hOwner;

      next_task:
	if ( psNewLock->fl_hOwner == hOldLockThread )
	{
		return ( 1 );
	}
	// Loop over all locks
	for ( psLock = g_psFirstFileLock; psLock != NULL; psLock = psLock->fl_psNextGlobal )
	{
		FileLockRec_s *psWaiter;

		if ( psLock->fl_psNextWaiter == NULL )
		{
			continue;
		}
		// Check any threads that waits for this lock
		for ( psWaiter = psLock->fl_psNextWaiter; psWaiter != psLock; psWaiter = psWaiter->fl_psNextWaiter )
		{
			if ( psWaiter->fl_hOwner == hOldLockThread )
			{
				if ( psLock->fl_hOwner == psNewLock->fl_hOwner )
				{
					return ( 1 );
				}
				hOldLockThread = psLock->fl_hOwner;
				goto next_task;
			}
		}
	}
	return ( 0 );
}

static bool locks_overlap( FileLockRec_s *psLock1, FileLockRec_s *psLock2 )
{
	return ( psLock1->fl_nEnd >= psLock2->fl_nStart && psLock2->fl_nEnd >= psLock1->fl_nStart );
}

/** Check if psNewLock conflicts with psOldLock
 * \par Description:
 *	Returnes true if the two locks is overlapping and one of the locks
 *	is in F_WRLCK mode.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *****************************************************************************/

static bool locks_conflict( FileLockRec_s *psNewLock, FileLockRec_s *psOldLock )
{
	if ( locks_overlap( psNewLock, psOldLock ) == false )
	{
		return ( false );
	}
	else
	{
		return ( psNewLock->fl_nMode == F_WRLCK || psOldLock->fl_nMode == F_WRLCK );
	}
}

static bool maybe_replace_lock( FileLockRec_s *psNewLock, FileLockRec_s *psLock )
{
	bool bRet = false;

	if( psNewLock->fl_nStart <= psLock->fl_nStart && psNewLock->fl_nEnd >= psLock->fl_nEnd )
	{
		/* Completely replace the existing lock */
		psLock->fl_nStart = psNewLock->fl_nStart;
		psLock->fl_nEnd = psNewLock->fl_nEnd;
		psLock->fl_nMode = psNewLock->fl_nMode;

		bRet = true;
	}

	return bRet;
}

static void adjust_lock( FileLockRec_s *psNewLock, FileLockRec_s *psOldLock )
{
	if( psNewLock->fl_nEnd >= psOldLock->fl_nStart )
	{
		psOldLock->fl_nStart = psNewLock->fl_nEnd + 1;
	}
	else if( psNewLock->fl_nStart <= psOldLock->fl_nEnd )
	{
		psOldLock->fl_nEnd = psNewLock->fl_nStart - 1;
	}
	psOldLock->fl_nMode = psNewLock->fl_nMode;
}

static void restart_waiters( Inode_s *psInode, FileLockRec_s *psNewLock )
{
	FileLockRec_s *psLock;

	for ( psLock = psInode->i_psFirstLock; psLock != NULL; psLock = psLock->fl_psNextInInode )
	{
		if( locks_overlap( psLock, psNewLock ) )
		{
			/* Unlock the wait queue for the overlapping lock. This will cause the thread
			   to wakeup and re-start the process of acquiring its lock. Note that because
			   we currently hold g_hFileLockMutex, any threads we wake here wont restart
			   until we're done and the new lock has been added. */

			wakeup_sem( psLock->fl_hLockQueue, true );
		}
	}
}

static void add_lock_global( FileLockRec_s *psLock )
{
	psLock->fl_psNextGlobal = g_psFirstFileLock;
	psLock->fl_psPrevGlobal = NULL;
	if ( g_psFirstFileLock != NULL )
	{
		g_psFirstFileLock->fl_psPrevGlobal = psLock;
	}
	g_psFirstFileLock = psLock;
}

static void rem_lock_global( FileLockRec_s *psLock )
{
	if ( psLock->fl_psNextGlobal != NULL )
	{
		psLock->fl_psNextGlobal->fl_psPrevGlobal = psLock->fl_psPrevGlobal;
	}
	if ( psLock->fl_psPrevGlobal != NULL )
	{
		psLock->fl_psPrevGlobal->fl_psNextGlobal = psLock->fl_psNextGlobal;
	}
	else
	{
		g_psFirstFileLock = psLock->fl_psNextGlobal;
	}
	psLock->fl_psNextGlobal = NULL;
	psLock->fl_psPrevGlobal = NULL;
}

static void add_lock_waiter( FileLockRec_s *psLock, FileLockRec_s *psWaiter )
{
	if ( psLock->fl_psNextWaiter == NULL )
	{
		psLock->fl_psNextWaiter = psLock->fl_psPrevWaiter = psLock;
	}
	psWaiter->fl_psNextWaiter = psLock;
	psWaiter->fl_psPrevWaiter = psLock->fl_psPrevWaiter;
	psLock->fl_psPrevWaiter->fl_psNextWaiter = psWaiter;
	psLock->fl_psPrevWaiter = psWaiter;

/*	
    if ( psLock->fl_psFirstWaiter == NULL ) {
	psLock->fl_psFirstWaiter  = psWaiter;
	psWaiter->fl_psNextWaiter = psWaiter;
	psWaiter->fl_psPrevWaiter = psWaiter;
    } else {
	psWaiter->fl_psPrevWaiter = psLock->fl_psFirstWaiter->fl_psPrevWaiter;
	psWaiter->fl_psNextWaiter = psLock->fl_psFirstWaiter;
	psLock->fl_psFirstWaiter->fl_psPrevWaiter->fl_psNextWaiter = psWaiter;
	psLock->fl_psFirstWaiter->fl_psPrevWaiter = psWaiter;
    }
    */
}

static void rem_lock_waiter( FileLockRec_s *psLock, FileLockRec_s *psWaiter )
{
	if ( psWaiter->fl_psNextWaiter == NULL )
	{
		return;		// psLock have been deleted, and psWaiter is already unlinked.
	}
	psWaiter->fl_psNextWaiter->fl_psPrevWaiter = psWaiter->fl_psPrevWaiter;
	psWaiter->fl_psPrevWaiter->fl_psNextWaiter = psWaiter->fl_psNextWaiter;

	if ( psLock->fl_psNextWaiter == psLock )
	{
		psLock->fl_psNextWaiter = psLock->fl_psPrevWaiter = NULL;
	}

/*    
    if ( psWaiter->fl_psNextWaiter == psWaiter->fl_psPrevWaiter ) {
	psLock->fl_psFirstWaiter = NULL;
    } else {
	if ( psLock->fl_psFirstWaiter == psWaiter ) {
	    psLock->fl_psFirstWaiter = psWaiter->fl_psNextWaiter;
	}
	psWaiter->fl_psPrevWaiter->fl_psNextWaiter = psWaiter->fl_psNextWaiter;
	psWaiter->fl_psNextWaiter->fl_psPrevWaiter = psWaiter->fl_psPrevWaiter;
    }
    */
	psWaiter->fl_psNextWaiter = NULL;
	psWaiter->fl_psPrevWaiter = NULL;

}

FileLockRec_s *create_file_lock( Inode_s *psInode, thread_id hOwner, int nMode )
{
	Thread_s *psThread = get_thread_by_handle( hOwner );
	FileLockRec_s *psLock;

	if ( psThread == NULL )
	{
		printk( "Panic: create_file_lock() invalid owner %d\n", hOwner );
	}

	psLock = kmalloc( sizeof( FileLockRec_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( psLock == NULL )
	{
		return ( NULL );
	}
	psLock->fl_hLockQueue = create_semaphore( "flock_queue", 0, 0 );
	if ( psLock->fl_hLockQueue < 0 )
	{
		kfree( psLock );
		return ( NULL );
	}
	psLock->fl_psInode = psInode;
	psLock->fl_hOwner = hOwner;
	psLock->fl_nMode = nMode;

	psLock->fl_psNextInThread = psThread->tr_psFirstFileLock;
	psLock->fl_psPrevInThread = NULL;
	if ( psThread->tr_psFirstFileLock != NULL )
	{
		psThread->tr_psFirstFileLock->fl_psPrevInThread = psLock;
	}
	psThread->tr_psFirstFileLock = psLock;

	atomic_inc( &psInode->i_nCount );
	return ( psLock );
}

void delete_file_lock( FileLockRec_s *psLock )
{
	if ( psLock->fl_psNextInThread != NULL )
	{
		psLock->fl_psNextInThread->fl_psPrevInThread = psLock->fl_psPrevInThread;
	}
	if ( psLock->fl_psPrevInThread != NULL )
	{
		psLock->fl_psPrevInThread->fl_psNextInThread = psLock->fl_psNextInThread;
	}
	else
	{
		Thread_s *psThread = get_thread_by_handle( psLock->fl_hOwner );

		if ( psThread != NULL )
		{
			psThread->tr_psFirstFileLock = psLock->fl_psNextInThread;
		}
		else
		{
			printk( "Panic: delete_file_lock() lock belongs to invalid thread %d\n", psLock->fl_hOwner );
		}
	}

	if ( psLock->fl_psNextWaiter != NULL )
	{
		while ( psLock->fl_psNextWaiter != NULL )
		{
			rem_lock_waiter( psLock, psLock->fl_psNextWaiter );
		}
		wakeup_sem( psLock->fl_hLockQueue, true );
	}
	delete_semaphore( psLock->fl_hLockQueue );
	put_inode( psLock->fl_psInode );
	kfree( psLock );
}

static int insert_lock( Inode_s *psInode, FileLockRec_s *psLock, bool bInsert )
{
	FileLockRec_s **ppsPrev;
	FileLockRec_s **ppsFirst;
	FileLockRec_s **ppsInsertPoint = NULL;
	FileLockRec_s *psLeft = NULL;
	int nError = 0;

//    printf( "Insert: %Ld -> %Ld (%d) (%d)\n", psLock->fl_nStart, psLock->fl_nEnd, psLock->fl_hOwner, bInsert );
	// If the list is empty we just insert the node and return
	if ( psInode->i_psFirstLock == NULL )
	{
		if ( bInsert )
		{
			psInode->i_psFirstLock = psLock;
			psLock->fl_psNextInInode = NULL;
		}
		add_lock_global( psLock );
		return ( 0 );
	}
	// Locks are sorted on owner firts and then range, so we must fine the first lock belonging to us.
	for ( ppsFirst = &psInode->i_psFirstLock; *ppsFirst != NULL; ppsFirst = &( *ppsFirst )->fl_psNextInInode )
	{
		FileLockRec_s *psIter = *ppsFirst;

		if ( psIter->fl_hOwner == psLock->fl_hOwner )
		{
			break;
		}
	}
	ppsInsertPoint = ppsFirst;
	// Then we "cut a whole" where we are going to insert the new node. This assures no locks is overlapping
	for ( ppsPrev = ppsFirst; *ppsPrev != NULL; )
	{
		FileLockRec_s *psIter = *ppsPrev;

		if ( psIter->fl_nStart > psLock->fl_nEnd || psLock->fl_hOwner != psIter->fl_hOwner )
		{
			break;	// Ok, we reach the end of our locks.
		}
		if ( psIter->fl_nEnd < psLock->fl_nStart )
		{
			ppsPrev = &( *ppsPrev )->fl_psNextInInode;
			ppsInsertPoint = &psIter->fl_psNextInInode;
			psLeft = psIter;
			continue;	// We are have still not reach the first overlaping lock
		}
		if ( psLock->fl_nStart > psIter->fl_nStart )
		{
			ppsInsertPoint = &psIter->fl_psNextInInode;
			psLeft = psIter;
		}

		if ( psIter->fl_nStart >= psLock->fl_nStart && psIter->fl_nEnd <= psLock->fl_nEnd )
		{
			*ppsPrev = psIter->fl_psNextInInode;	// The new lock fully overlap the old lock. We delete the old lock.
			rem_lock_global( psIter );
			delete_file_lock( psIter );
			continue;
		}
		if ( psIter->fl_nStart < psLock->fl_nStart && psIter->fl_nEnd > psLock->fl_nEnd )
		{
			// Here the new lock is fully overlaped by an old lock. We must split the old lock.
			FileLockRec_s *psExtraLock;

			psExtraLock = create_file_lock( psInode, psIter->fl_hOwner, psIter->fl_nMode );
			if ( psExtraLock == NULL )
			{
				nError = -ENOMEM;
				break;
			}
			psExtraLock->fl_nStart = psLock->fl_nEnd + 1;
			psExtraLock->fl_nEnd = psIter->fl_nEnd;
			psIter->fl_nEnd = psLock->fl_nStart - 1;

			psExtraLock->fl_psNextInInode = psIter->fl_psNextInInode;
			psIter->fl_psNextInInode = psExtraLock;

			add_lock_global( psExtraLock );
			break;
		}
		// Ok, we have a partial overlapping lock.
		if ( psIter->fl_nStart < psLock->fl_nStart )
		{
			// Truncate the old lock in the end
			psIter->fl_nEnd = psLock->fl_nStart - 1;
		}
		else
		{
			// Truncate the old lock in the start
			psIter->fl_nStart = psLock->fl_nEnd + 1;
		}
		ppsPrev = &( *ppsPrev )->fl_psNextInInode;
	}
	// Then we insert the new lock (unless we are in fact unlocking).
	if ( bInsert && nError >= 0 )
	{
		FileLockRec_s *psRight = *ppsInsertPoint;

		psLock->fl_psNextInInode = *ppsInsertPoint;
		*ppsInsertPoint = psLock;
		add_lock_global( psLock );

		// Check if we can join with the previous lock
		if ( psLeft != NULL && psLeft->fl_nEnd + 1 == psLock->fl_nStart && psLeft->fl_hOwner == psLock->fl_hOwner && psLeft->fl_nMode == psLock->fl_nMode )
		{
			psLeft->fl_nEnd = psLock->fl_nEnd;
			psLeft->fl_psNextInInode = psRight;
			rem_lock_global( psLock );
			delete_file_lock( psLock );
			psLock = psLeft;
		}
		// Check if we can join with the next lock
		if ( psRight != NULL && psLock->fl_nEnd + 1 == psRight->fl_nStart && psLock->fl_hOwner == psRight->fl_hOwner && psLock->fl_nMode == psRight->fl_nMode )
		{
			psLock->fl_nEnd = psRight->fl_nEnd;
			psLock->fl_psNextInInode = psRight->fl_psNextInInode;
			rem_lock_global( psRight );
			delete_file_lock( psRight );
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

int lock_inode_record( Inode_s *psInode, off_t nStart, off_t nEnd, int nMode, bool bWait )
{
	FileLockRec_s *psNewLock;
	FileLockRec_s *psLock;
	int nError = 0;
	bool bDoAddLock = true;

	psNewLock = create_file_lock( psInode, get_thread_id( NULL ), nMode );

	if ( psNewLock == NULL )
	{
		return ( -ENOMEM );
	}
	psNewLock->fl_nStart = nStart;
	psNewLock->fl_nEnd = nEnd;

	LOCK( g_hFileLockMutex );
      again:
	for ( psLock = psInode->i_psFirstLock; psLock != NULL; psLock = psLock->fl_psNextInInode )
	{
		if ( locks_conflict( psLock, psNewLock ) == false )
		{
			continue;
		}
		if ( bWait == false )
		{
			nError = -EWOULDBLOCK;
			break;
		}
		if( psNewLock->fl_hOwner == psLock->fl_hOwner )
		{
			if( maybe_replace_lock( psNewLock, psLock ) == true )
				bDoAddLock = false;
			else
				adjust_lock( psNewLock, psLock );

			/* The existing lock has changed, so other threads that may have been waiting on it
			   may now be waiting on the wrong lock. */
			restart_waiters( psInode, psNewLock );

			break;
		}
		if ( detect_deadlock( psNewLock, psLock ) )
		{
			nError = -EDEADLK;
			break;
		}

		add_lock_waiter( psLock, psNewLock );
		nError = unlock_and_suspend( psLock->fl_hLockQueue, g_hFileLockMutex );
		LOCK( g_hFileLockMutex );
		// Not that psLock might have been deleted at this stage!
		// This is detected by rem_lock_waiter() but be sure to not
		// do anything else with psLock
		rem_lock_waiter( psLock, psNewLock );
		if ( nError < 0 )
		{
			break;
		}
		goto again;
	}
	if ( nError >= 0 && bDoAddLock )
	{
		nError = insert_lock( psInode, psNewLock, true );
		if ( nError < 0 )
		{
			delete_file_lock( psNewLock );
		}
	}
	else
	{
		delete_file_lock( psNewLock );
	}
	UNLOCK( g_hFileLockMutex );
	return ( nError );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt.skauen@c2i.net)
 *****************************************************************************/

int get_inode_lock_record( Inode_s *psInode, off_t nStart, off_t nEnd, int nMode, struct flock *psFLock )
{
	FileLockRec_s sNewLock;
	FileLockRec_s *psLock;

	sNewLock.fl_hOwner = get_thread_id( NULL );
	sNewLock.fl_nMode = nMode;
	sNewLock.fl_nStart = nStart;
	sNewLock.fl_nEnd = nEnd;

	LOCK( g_hFileLockMutex );
	psFLock->l_type = F_UNLCK;
	for ( psLock = psInode->i_psFirstLock; psLock != NULL; psLock = psLock->fl_psNextInInode )
	{
		if ( locks_conflict( psLock, &sNewLock ) )
		{
			psFLock->l_pid = psLock->fl_hOwner;
			psFLock->l_type = psLock->fl_nMode;
			psFLock->l_whence = SEEK_SET;
			psFLock->l_start = psLock->fl_nStart;
			psFLock->l_len = ( psLock->fl_nEnd == MAX_FILE_OFFSET ) ? 0 : ( psLock->fl_nEnd - psLock->fl_nStart + 1LL );
			break;
		}
	}
	UNLOCK( g_hFileLockMutex );
	return ( 0 );
}

int unlock_inode_record( Inode_s *psInode, off_t nStart, off_t nEnd )
{
	FileLockRec_s sLock;
	int nError;

	memset( &sLock, 0, sizeof( sLock ) );

	sLock.fl_hOwner = get_thread_id( NULL );
	sLock.fl_nStart = nStart;
	sLock.fl_nEnd = nEnd;

	LOCK( g_hFileLockMutex );
	nError = insert_lock( psInode, &sLock, false );
	UNLOCK( g_hFileLockMutex );

	return ( nError );
}

void flock_thread_died( Thread_s *psThread )
{
	FileLockRec_s sLock;
	int nError;

	memset( &sLock, 0, sizeof( sLock ) );

	sLock.fl_hOwner = psThread->tr_hThreadID;
	sLock.fl_nStart = 0;
	sLock.fl_nEnd = MAX_FILE_OFFSET;

	LOCK( g_hFileLockMutex );
	while ( psThread->tr_psFirstFileLock != NULL )
	{
		nError = insert_lock( psThread->tr_psFirstFileLock->fl_psInode, &sLock, false );
	}
	UNLOCK( g_hFileLockMutex );
}

void init_flock( void )
{
	g_hFileLockMutex = create_semaphore( "flock_mutex", 1, 0 );
}
