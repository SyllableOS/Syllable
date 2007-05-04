
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
#include <posix/select.h>

#include <atheos/kernel.h>
#include <atheos/semaphore.h>
#include <atheos/msgport.h>
#include <atheos/spinlock.h>
#include <atheos/areas.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/semaphore.h"
#include "inc/areas.h"
#include "inc/sysbase.h"
#include "inc/smp.h"

extern MultiArray_s g_sProcessTable;
extern MultiArray_s g_sThreadTable;	// Global thread table

void system_init( void );
void idle_loop( void );

/*****************************************************************************
 * NAME:
 * DESC:
 *	Determine if a process group is "orphaned", according to the POSIX
 *	definition in 2.2.2.52.  Orphaned process groups are not to be affected
 *	by terminal-generated stop signals.  Newly orphaned process groups are 
 *	to receive a SIGHUP and a SIGCONT.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int will_become_orphaned_pgrp( int nPGroup, Thread_s *psIgnoredThread )
{
	thread_id hThread;
	int nFlg;
	bool bResult = true;

	nFlg = cli();
	sched_lock();
	for ( hThread = 0; -1 != hThread; hThread = get_next_thread( hThread ) )
	{
		Thread_s *psThread = get_thread_by_handle( hThread );
		Process_s *psProc = psThread->tr_psProcess;
		Process_s *psParent = get_thread_by_handle( psThread->tr_hParent )->tr_psProcess;

		if ( ( psThread == psIgnoredThread ) || ( psProc->pr_hPGroupID != nPGroup ) || ( psThread->tr_nState == TS_ZOMBIE ) || ( psThread->tr_hParent == 1 ) )
		{
			continue;
		}
		if ( ( psParent->pr_hPGroupID != nPGroup ) && ( psParent->pr_nSession == psProc->pr_nSession ) )
		{
			bResult = false;;
			break;
		}
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	if ( bResult )
	{
		printk( "Process group %d will be orphaned\n", nPGroup );
	}
	return ( bResult );	/* (sighing) "Often!" */
}

int is_orphaned_pgrp( int pgrp )
{
	return will_become_orphaned_pgrp( pgrp, 0 );
}


int do_exit( int nErrorCode )
{
	Thread_s *psThread = CURRENT_THREAD;
	Process_s *psProc;
	Thread_s *psParentThread;
	Process_s *psParentProc;
	thread_id hChild;
	thread_id hOurPGrp;
	bool bHasStoppedChildren = false;
	bool bChildrenLeft = false;

	if ( psThread == NULL )
	{
		printk( "do_exit1() no current thread - call schedule\n" );
		for ( ;; )
		{
			Schedule();	/* Does not return */

			printk( "do_exit1(%s) : Schedule() returned!!!\n", psThread->tr_zName );
		}
	}

	psProc = psThread->tr_psProcess;
	psParentThread = get_thread_by_handle( psThread->tr_hParent );
	psParentProc = psParentThread->tr_psProcess;
	hOurPGrp = psParentProc->pr_hPGroupID;

	psThread->tr_nExitCode = nErrorCode;

	kassertw( atomic_read(&psProc->pr_nLivingThreadCount) >= 1 );

	if ( atomic_dec_and_test( &psProc->pr_nLivingThreadCount ) )	/* Last out closing the door    */
	{
		IoContext_s *psIOCtx;
		DR_ThreadDied_s sAppMsg;
		port_id hAppServerPort = get_app_server_port();
		uint32 nFlags;

		// We delete the heap as soon as possible. This helps prevent a
		// deadlock if we are out of memory and the process was killed
		// in an attempt to free some. This way we hopefully won't end
		// up waiting for more RAM ourself when starting to close files
		// and sending the leave message to the application server.

		delete_area( psProc->pr_nHeapArea );
		psProc->pr_nHeapArea = -1;


		close_all_images( psProc->pr_psImageContext );
		delete_image_context( psProc->pr_psImageContext );

		if ( psProc->pr_bIsGroupLeader )
		{
			disassociate_ctty( true );
		}

		nFlags = cli();
		sched_lock();

		psIOCtx = psProc->pr_psIoContext;
		psProc->pr_psIoContext = NULL;

		sched_unlock();
		put_cpu_flags( nFlags );

		fs_free_ioctx( psIOCtx );

		exit_free_ports( psProc );
		exit_free_semaphores( psProc );

		if ( hAppServerPort >= 0 )
		{
			sAppMsg.hThread = psProc->tc_hProcID;
			send_msg_x( hAppServerPort, -1, &sAppMsg, sizeof( sAppMsg ), INFINITE_TIMEOUT );
		}
	}

	// Hand children over to the psycho killer (init thread)
	cli();
	sched_lock();
	for ( hChild = 0; -1 != hChild; hChild = get_next_thread( hChild ) )
	{
		Thread_s *psChild = get_thread_by_handle( hChild );

//    Process_s* psParent = get_thread_by_handle( psChild->tr_hParent )->tr_psProcess;

		kassertw( NULL != psChild );

		if ( psChild == psThread )
		{
			continue;
		}

		if ( psChild->tr_hParent == psThread->tr_hThreadID )
		{
			if ( psChild->tr_psProcess->pr_hPGroupID == hOurPGrp && psChild->tr_nState == TS_STOPPED )
			{
				bHasStoppedChildren = true;
			}

			psChild->tr_hParent = 1;	/* Hand all children over to psycho_killer */
			bChildrenLeft = true;
		}
	}
//  sched_unlock();
	// Might start interrupts???


	/* 
	 * Check to see if any process groups have become orphaned
	 * as a result of our exiting, and if they have any stopped
	 * jobs, send them a SIGHUP and then a SIGCONT.  (POSIX 3.2.2.2)
	 *
	 * Case i: Our father is in a different pgrp than we are
	 * and we were the only connection outside, so our pgrp
	 * is about to become orphaned.
	 */

/*
  if ( bHasStoppedChildren &&
  (psParentProc->pr_hPGroupID != pr_hGroupID) &&
  (psParentProc->session == psProc->session) &&
  will_become_orphaned_pgrp( psProc->pr_hPGroupID, psProc ))
  {
  sys_killpg( psProc->pr_hPGroupID, SIGHUP );
  sys_killpg( psProc->pr_hPGroupID, SIGCONT );
  }
  if ( bHasStoppedChildren && psParentProc->pr_hPGroupID != pr_hPGroupID && psParentProc->session == psProc->session )
  bGrpOrphaned = true;
  cli();
  sched_lock();
  for ( hChild = 0 ; -1 != hChild ; hChild = get_next_thread( hChild ) )
  {
  Thread_s* psChild = get_thread_by_handle( hChild );
  Process_s* psParent = get_thread_by_handle( psChild->tr_hParent )->tr_psProcess;
    
  kassertw( NULL != psChild );

  if ( psChild == psThread ) {
  continue;
  }
  if ( psChild->tr_nState == TS_ZOMBIE ) {
  continue;
  }
    
  if ( psChild->tr_psProcess != hOurPGrp || psChild->tr_hParent == 1 ) {
  continue;
  }
  if ( (psParent->pgrp != pgrp) && (psParent->session == psChild->session) ) {
  bGrpOrphaned = false;
  }
  }
  */
	if ( bChildrenLeft )
	{
		send_signal( get_thread_by_handle( 1 ), SIGCHLD, true );

/*			printk( "Send SIGCHLD to init\n" ); */
	}

	send_signal( psParentThread, SIGCHLD, true );	
	psThread->tr_nState = TS_ZOMBIE;
	wake_up_queue( false, psThread->tr_psTermWaitList, nErrorCode, true );

	g_asProcessorDescs[get_processor_id()].pi_psCurrentThread = NULL;
	sched_unlock();

	for ( ;; )
	{
		Schedule();	/* Does not return */
		printk( "do_exit2(%s) : Schedule() returned!!!\n", psThread->tr_zName );
	}

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_exit_thread( int nErrorCode )
{
	return ( do_exit( nErrorCode ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_exit( int nErrorCode )
{
	Process_s *psProc = CURRENT_PROC;

	sys_kill_proc( psProc->tc_hProcID, SIGKILL );

	return ( do_exit( ( nErrorCode & 0xff ) << 8 ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static Process_s *alloc_process( MemContext_s *psMemSeg )
{
	Process_s *psProc = kmalloc( sizeof( Process_s ), MEMF_CLEAR | MEMF_KERNEL | MEMF_LOCKED | MEMF_OKTOFAIL );

	if ( psProc == NULL )
	{
		printk( "Warning: alloc_process() no memory for process structure\n" );
		return ( NULL );
	}
	psProc->tc_psMemSeg = psMemSeg;

	psProc->pr_hSem = create_semaphore( "Process", 1, SEM_RECURSIVE );

	if ( psProc->pr_hSem < 0 )
	{
		int nError = psProc->pr_hSem;

		printk( "Warning: alloc_process() failed to create mutex (%d)\n", nError );
		kfree( psProc );
		return ( NULL );
	}

	psProc->pr_nHeapArea = -1;
	psProc->tc_hProcID = -1;

	psProc->pr_hPGroupID = -1;
	psProc->pr_nSession = -1;
	psProc->tc_zName[0] = '\0';

	atomic_inc( &g_sSysBase.ex_nProcessCount );

	return ( psProc );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

thread_id create_init_proc( const char *Name )
{
	Process_s *psProc = alloc_process( g_psKernelSeg );
	thread_id hThread = -1;

	if ( psProc != NULL )
	{
		Thread_s *psThread;

		psProc->pr_psImageContext = create_image_ctx();
		psProc->pr_psSemContext = create_semaphore_context();

		psThread = Thread_New( psProc );

		set_idle_thread( psThread );

		psThread->tr_nUMask = S_IWGRP | S_IWOTH;

		psProc->pr_psFirstThread = psThread;
		
		/* Prepare kernel stack */
		SysCallRegs_s* psRegs = (SysCallRegs_s*)( psThread->tc_plKStack - sizeof( SysCallRegs_s ) / 4 - 1 );
		memset( psRegs, 0, sizeof( SysCallRegs_s ) );
		
		psRegs->ds = DS_KERNEL;
		psRegs->es = DS_KERNEL;
		psRegs->fs = DS_KERNEL;
		psRegs->gs = DS_KERNEL;
		psRegs->eip = (uint32)system_init;
		psRegs->cs = CS_KERNEL;
		psRegs->eflags = get_cpu_flags();
		psRegs->oldesp = (uint32)psThread->tc_plKStack;
		psRegs->oldss = DS_KERNEL;	

		hThread = MArray_Insert( &g_sThreadTable, psThread, false );
		psThread->tr_hThreadID = hThread;

		psProc->tc_hProcID = MArray_Insert( &g_sProcessTable, psProc, false );

		strcpy( psProc->tc_zName, Name );
		strcpy( psThread->tr_zName, Name );

		psProc->tc_hParent = -1;

		psProc->pr_psIoContext = fs_alloc_ioctx( FD_SETSIZE );

		psThread->tr_pESP = (void*)psRegs;
		psThread->tr_pEIP = exit_from_sys_call;
	}
	printk( "Start init thread\n" );
	DoSchedule( NULL );
	return ( hThread );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

thread_id create_idle_thread( const char *Name )
{
	Process_s *psProc = get_proc_by_handle( 0 );
	thread_id hThread = -1;
	Thread_s *psThread;

	psThread = Thread_New( psProc );

	set_idle_thread( psThread );

	psThread->tr_nPriority = -999;

	/* Prepare kernel stack */
	SysCallRegs_s* psRegs = (SysCallRegs_s*)( psThread->tc_plKStack - sizeof( SysCallRegs_s ) / 4 - 1 );
	memset( psRegs, 0, sizeof( SysCallRegs_s ) );
		
	psRegs->ds = DS_KERNEL;
	psRegs->es = DS_KERNEL;
	psRegs->fs = DS_KERNEL;
	psRegs->gs = DS_KERNEL;
	psRegs->eip = (uint32)idle_loop;
	psRegs->cs = CS_KERNEL;
	psRegs->eflags = get_cpu_flags();
	psRegs->oldesp = (uint32)psThread->tc_plKStack;
	psRegs->oldss = DS_KERNEL;
	
	hThread = MArray_Insert( &g_sThreadTable, psThread, false );
	psThread->tr_hThreadID = hThread;

	strcpy( psThread->tr_zName, Name );

	psThread->tr_pESP = (void*)psRegs;
	psThread->tr_pEIP = exit_from_sys_call;

	DoSchedule( NULL );
	return ( hThread );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

thread_id sys_Fork( const char *const pzName )
{
	Thread_s *psParentThread;
	Thread_s *psNewThread;
	Process_s *psProc;
	Process_s *psParent;
	MemContext_s *psMemSeg;
	SysCallRegs_s *psRegs = ( SysCallRegs_s * ) & pzName;
	proc_id hParentID;
	thread_id hNewThread = -1;
	int i;
	int nFlg;
	int nError;

	psParentThread = CURRENT_THREAD;
	psParent = psParentThread->tr_psProcess;
	hParentID = psParent->tc_hProcID;
	

	psMemSeg = clone_mem_context( psParent->tc_psMemSeg );

	if ( psMemSeg == NULL )
	{
		nError = -ENOMEM;
		goto error1;
	}
	
	psProc = alloc_process( psMemSeg );

	if ( psProc == NULL )
	{
		nError = -ENOMEM;
		goto error2;
	}
	
	psNewThread = Thread_New( psProc );

	if ( psNewThread == NULL )
	{
		nError = -ENOMEM;
		goto error3;
	}

	psNewThread->tr_hParent = psParentThread->tr_hThreadID;
	psNewThread->tr_nSysTraceMask = psParentThread->tr_nSysTraceMask;
	psNewThread->tr_nUMask = psParentThread->tr_nUMask;
	psNewThread->tr_pThreadData = psParentThread->tr_pThreadData;
	psNewThread->tr_nUStackSize = psParentThread->tr_nUStackSize;

	if ( psParentThread->tr_nStackArea != -1 )
	{
		AreaInfo_s sInfo;
		MemArea_s *psArea;

		get_area_info( psParentThread->tr_nStackArea, &sInfo );
		psArea = get_area( psMemSeg, ( uint32 )sInfo.pAddress );

		psNewThread->tr_nStackArea = -1;
		if ( psArea != NULL )
		{
			psNewThread->tr_nStackArea = psArea->a_nAreaID;
			put_area( psArea );
		}
		else
		{
			printk( "Error: fork() could not find stack area in child\n" );
		}
	}
	
	psProc->pr_nUID = psParent->pr_nUID;
	psProc->pr_nEUID = psParent->pr_nEUID;
	psProc->pr_nSUID = psParent->pr_nSUID;
	psProc->pr_nFSUID = psParent->pr_nFSUID;
	psProc->pr_nGID = psParent->pr_nGID;
	psProc->pr_nEGID = psParent->pr_nEGID;
	psProc->pr_nSGID = psParent->pr_nSGID;
	psProc->pr_nFSGID = psParent->pr_nFSGID;
	psProc->pr_nNumGroups = psParent->pr_nNumGroups;

	memcpy( psProc->pr_anGroups, psParent->pr_anGroups, sizeof( psProc->pr_anGroups ) );
	memcpy( psProc->pr_anTLDBitmap, psParent->pr_anTLDBitmap, sizeof( psProc->pr_anTLDBitmap ) );


	psProc->pr_hPGroupID = psParent->pr_hPGroupID;
	psProc->pr_nSession = psParent->pr_nSession;

	psProc->pr_psFirstThread = psNewThread;

	if ( pzName != NULL )
	{
		if ( strncpy_from_user( psProc->tc_zName, pzName, OS_NAME_LENGTH ) < 0 )
		{
			printk( "fork() got invalid pointer (%p) from user space\n", pzName );
		}
	}
	else
	{
		strcpy( psProc->tc_zName, psParent->tc_zName );
	}
	strcpy( psNewThread->tr_zName, psProc->tc_zName );


	psProc->tc_hParent = hParentID;
	psNewThread->tr_nPriority = psParentThread->tr_nPriority;
	psNewThread->tr_nQuantum = psParentThread->tr_nQuantum;
	
      /*** Clone file descriptors and cwd ***/
  
	psProc->pr_psIoContext = fs_clone_io_context( psParent->pr_psIoContext );

	if ( psProc->pr_psIoContext == NULL )
	{
		nError = -ENOMEM;
		goto error4;
	}
	
	psProc->pr_psSemContext = clone_semaphore_context( psParent->pr_psSemContext, hParentID );

	if ( psProc->pr_psSemContext == NULL )
	{
		nError = -ENOMEM;
		goto error5;
	}
		
	psProc->pr_psImageContext = clone_image_context( psParent->pr_psImageContext, psMemSeg );

	if ( psProc->pr_psImageContext == NULL )
	{
		nError = -ENOMEM;
		goto error6;
	}
	
      /*** Clone signal handlers ***/

	for ( i = 0; i < _NSIG; ++i )
	{
		psNewThread->tr_apsSigHandlers[i] = psParentThread->tr_apsSigHandlers[i];
	}
	if ( psParent->pr_nHeapArea != -1 )
	{
		AreaInfo_s sInfo;
		MemArea_s *psArea;

		get_area_info( psParent->pr_nHeapArea, &sInfo );
		psArea = get_area( psMemSeg, ( uint32 )sInfo.pAddress );

		psProc->pr_nHeapArea = -1;
		if ( psArea == NULL )
		{
			printk( "Error1: fork() could not find heap area in child!!\n" );
		}
		else
		{
			psProc->pr_nHeapArea = psArea->a_nAreaID;
			put_area( psArea );
		}
	}
	
	/* Copy FPU state and flags */
	if ( psParentThread->tr_nFlags & TF_FPU_USED )
	{
		if ( psParentThread->tr_nFlags & TF_FPU_DIRTY )
		{
			save_fpu_state( &psParentThread->tc_FPUState );
			psParentThread->tr_nFlags &= ~TF_FPU_DIRTY;
			stts();
		}
		
		memcpy( &psNewThread->tc_FPUState, &psParentThread->tc_FPUState, sizeof( union i3FPURegs_u ) );
	}
	psNewThread->tr_nFlags = psParentThread->tr_nFlags;

	psNewThread->tr_pESP -= sizeof( SysCallRegs_s );
	memcpy( psNewThread->tr_pESP, psRegs, sizeof( SysCallRegs_s ) );
	( ( SysCallRegs_s * ) psNewThread->tr_pESP )->eax = 0;


	psNewThread->tr_pEIP = exit_from_sys_call;

	if ( NULL != pzName )
	{
		char zBuf[16];

		sprintf( zBuf, "(%d)", psNewThread->tr_hThreadID );
		strcat( psNewThread->tr_zName, zBuf );
	}
      again:
	nFlg = cli();
	sched_lock();
	psProc->tc_hProcID = MArray_Insert( &g_sProcessTable, psProc, true );

	if ( psProc->tc_hProcID >= 0 )
	{
		hNewThread = MArray_Insert( &g_sThreadTable, psNewThread, true );
		psNewThread->tr_hThreadID = hNewThread;

		if ( hNewThread < 0 )
		{
			MArray_Remove( &g_sProcessTable, psProc->tc_hProcID );
		}
		else
		{
			update_semaphore_context( psProc->pr_psSemContext, psProc->tc_hProcID );
			add_thread_to_ready( psNewThread );	/* Add thread to the 'ready' list */
		}
	}
	else
	{
		hNewThread = psProc->tc_hProcID;
	}

	sched_unlock();
	put_cpu_flags( nFlg );


	if ( hNewThread == -ENOMEM )
	{
		shrink_caches( 65536 );
		goto again;
	}

	return ( hNewThread );
      error6:
	exit_free_semaphores( psProc );
//    free_sem_context( psProc->pr_psSemContext );
      error5:
	fs_free_ioctx( psProc->pr_psIoContext );
      error4:
	Thread_Delete( psNewThread );
      error3:
	// Should delete process here, but there is not a 1-1 correspondance between Create/Delete process :(
      error2:
	delete_mem_context( psMemSeg );
      error1:
	return ( nError );
}
