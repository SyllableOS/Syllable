
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

#include <atheos/kernel.h>
#include <atheos/syscall.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/areas.h"
#include "inc/semaphore.h"
#include "inc/sysbase.h"
#include "inc/smp.h"
#include "inc/array.h"

MultiArray_s g_sThreadTable;	// Global thread table

static const uint32_t KSTACK_MAGIC = 0x23145567;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t do_get_thread_info( thread_id hThread, thread_info * psInfo )
{
	Thread_s *psThread;

	psThread = get_thread_by_handle( hThread );

	if ( psThread == NULL )
	{
		return ( -ESRCH );
	}
	strcpy( psInfo->ti_thread_name, psThread->tr_zName );	/* Thread name        */
	strcpy( psInfo->ti_process_name, psThread->tr_psProcess->tc_zName );	/* Process name        */

	psInfo->ti_thread_id = hThread;
	psInfo->ti_process_id = psThread->tr_psProcess->tc_hProcID;

	psInfo->ti_state = psThread->tr_nState;	/* Current task state.  */
	psInfo->ti_flags = psThread->tr_nFlags;
	psInfo->ti_blocking_sema = psThread->tr_hBlockSema;

	psInfo->ti_priority = psThread->tr_nPriority;
	psInfo->ti_dynamic_pri = psThread->tr_nPriority;
	psInfo->ti_sys_time = psThread->tr_nSysTime;	/* Micros in user mode  */
	psInfo->ti_user_time = psThread->tr_nCPUTime - psThread->tr_nSysTime;	/*      Micros in kernal mode   */
	psInfo->ti_real_time = psThread->tr_nCPUTime;	/* Total micros of execution            */
	psInfo->ti_quantum = psThread->tr_nQuantum;	/* Maximum allowed micros of execution before preemption */

	psInfo->ti_stack = psThread->tr_pUserStack;	/* lowest address of the user stack     */
	psInfo->ti_stack_size = psThread->tr_nUStackSize;	/* size (in bytes) of user stack        */

	psInfo->ti_kernel_stack = psThread->tc_plKStack - psThread->tc_lKStackSize / 4 - 1;	/* lowest address of the kernel stack     */
	psInfo->ti_kernel_stack_size = psThread->tc_lKStackSize;	/* Size (in bytes) of kernel stack     */
	return ( 0 );
}

status_t sys_get_thread_info( const thread_id hThread, thread_info * const psInfo )
{
	thread_id hRealThread;
	uint32 nOldFlags;
	status_t nError;

	nError = verify_mem_area( psInfo, sizeof( *psInfo ), true );
	if ( nError < 0 )
	{
		printk( "Error: sys_get_thread_info() failed to lock info buffer\n" );
		return ( nError );
	}

	nOldFlags = cli();
	sched_lock();

	if ( hThread == -1 )
	{
		hRealThread = MArray_GetNextIndex( &g_sThreadTable, 0 );
	}
	else
	{
		hRealThread = hThread;
	}


	if ( hRealThread != -1 )
	{
		nError = do_get_thread_info( hRealThread, psInfo );
	}
	else
	{
		nError = -ESRCH;
	}

	sched_unlock();
	put_cpu_flags( nOldFlags );

	return ( nError );
}

status_t sys_get_next_thread_info( thread_info * const psInfo )
{
	thread_id hThread;
	int nOldFlags;
	status_t nError;

	nError = verify_mem_area( psInfo, sizeof( *psInfo ), true );
	if ( nError < 0 )
	{
		printk( "Error: sys_get_next_thread_info() failed to lock info buffer\n" );
		return ( nError );
	}

	nOldFlags = cli();
	sched_lock();

	hThread = MArray_GetNextIndex( &g_sThreadTable, psInfo->ti_thread_id );

	if ( hThread != -1 )
	{
		nError = do_get_thread_info( hThread, psInfo );
	}
	else
	{
		nError = -ESRCH;
	}

	sched_unlock();
	put_cpu_flags( nOldFlags );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

proc_id sys_get_thread_proc( const thread_id hThread )
{
	proc_id hProc;
	Thread_s *psThread;
	uint32 nFlg = cli();

	sched_lock();

	psThread = get_thread_by_handle( hThread );
	if ( psThread != NULL )
	{
		hProc = psThread->tr_psProcess->tc_hProcID;
	}
	else
	{
		hProc = -1;
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( hProc );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

thread_id get_next_thread( thread_id hPrev )
{
	thread_id hNext;

	hNext = MArray_GetNextIndex( &g_sThreadTable, hPrev );

	return ( hNext );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

thread_id get_prev_thread( thread_id hPrev )
{
	thread_id hNext;
	int nFlg = cli();

	sched_lock();

	hNext = MArray_GetPrevIndex( &g_sThreadTable, hPrev );

	sched_unlock();
	put_cpu_flags( nFlg );
	return ( hNext );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

Thread_s *get_thread_by_name( const char *pzName )
{
	if ( pzName == NULL )
	{
		return ( CURRENT_THREAD );
	}

	printk( "Error : FindThread() with name is not yet supported!!\n" );
	return ( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

Thread_s *GetThreadAddr( thread_id hThread )
{
	if ( hThread == -1 )
	{
		return ( CURRENT_THREAD );
	}

	return ( MArray_GetObj( &g_sThreadTable, hThread ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

Thread_s *get_thread_by_handle( thread_id hThread )
{
	if ( hThread == -1 )
	{
		return ( CURRENT_THREAD );
	}

	return ( MArray_GetObj( &g_sThreadTable, hThread ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

thread_id get_thread_id( const char *const pzName )
{
	if ( pzName == NULL )
	{
		return ( CURRENT_THREAD->tr_hThreadID );
	}
	else
	{
		thread_id hThread;
		Thread_s *psThread;
		uint32 nOldFlags = cli();

		sched_lock();
		FOR_EACH_THREAD( hThread, psThread )
		{
			if ( strcmp( psThread->tr_zName, pzName ) == 0 )
			{
				sched_unlock();
				return ( hThread );
			}
		}
		sched_unlock();
		put_cpu_flags( nOldFlags );
	}
	return ( -ENOENT );
}

thread_id sys_get_thread_id( const char *const pzName )
{

	if ( pzName == NULL )
	{
		return ( get_thread_id( NULL ) );
	}
	else
	{
		char zName[OS_NAME_LENGTH];
		int nError;

		nError = strncpy_from_user( zName, pzName, OS_NAME_LENGTH - 1 );
		zName[OS_NAME_LENGTH - 1] = '\0';
		if ( nError < 0 )
		{
			printk( "Error: sys_get_thread_id() invalid name pointer\n" );
			return ( nError );
		}
		return ( get_thread_id( zName ) );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_SetThreadName( const char *const pzName )
{
	Thread_s *psThread = CURRENT_THREAD;

	printk( "WARNING: OBSOLETE FUNCTION sys_SetThreadName() CALLED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
	strncpy_from_user( psThread->tr_zName, pzName, OS_NAME_LENGTH - 1 );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t rename_thread( thread_id hThread, const char *const pzName )
{
	Thread_s *psThread;
	int nError;
	int nFlg;

	nFlg = cli();
	sched_lock();
	if ( hThread == -1 )
	{
		psThread = CURRENT_THREAD;
	}
	else
	{
		psThread = get_thread_by_handle( hThread );
	}
	if ( psThread != NULL )
	{
		strncpy( psThread->tr_zName, pzName, OS_NAME_LENGTH - 1 );
		nError = 0;
	}
	else
	{
		nError = -EINVAL;
	}
	sched_unlock();
	put_cpu_flags( nFlg );

	return ( nError );
}

status_t sys_rename_thread( thread_id hThread, const char *const pzName )
{
	char zName[OS_NAME_LENGTH];
	int nError;

	nError = strncpy_from_user( zName, pzName, OS_NAME_LENGTH - 1 );
	if ( nError < 0 )
	{
		printk( "Error: sys_rename_thread() invalid name pointer\n" );
		return ( nError );
	}
	zName[OS_NAME_LENGTH - 1] = '\0';
	return ( rename_thread( hThread, zName ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_send_data( const thread_id hThread, const uint32 nCode, void *const pData, const uint32 nSize )
{
	Thread_s *psMyThread = CURRENT_THREAD;
	Thread_s *psThread;
	void *pBuffer;
	int nFlg;
	int nError;

	pBuffer = kmalloc( nSize, MEMF_KERNEL | MEMF_OKTOFAIL );
	if ( pBuffer == NULL )
	{
		return ( -ENOMEM );
	}

	nError = memcpy_from_user( pBuffer, pData, nSize );
	if ( nError < 0 )
	{
		kfree( pBuffer );
		return ( nError );
	}

	nFlg = cli();
	sched_lock();

	psThread = get_thread_by_handle( hThread );

	if ( psThread == NULL )
	{
		nError = -ESRCH;
		goto error;
	}

	while ( psThread->tr_pData != NULL )
	{
		WaitQueue_s sWaitNode;

		psMyThread->tr_nState = TS_WAIT;
		sWaitNode.wq_hThread = psMyThread->tr_hThreadID;
		add_to_waitlist( false, &psThread->tr_psSendWaitQueue, &sWaitNode );

		sched_unlock();
		put_cpu_flags( nFlg );

		Schedule();

		nFlg = cli();
		sched_lock();

		psThread = get_thread_by_handle( hThread );

		if ( psThread == NULL )
		{
			printk( "sys_send_data(): thread %d lost while waiting to send data\n", hThread );
			nError = -ESRCH;
			goto error;
		}
		remove_from_waitlist( true, &psThread->tr_psSendWaitQueue, &sWaitNode );
		if ( psThread->tr_pData != NULL && is_signal_pending() )
		{
			nError = -EINTR;
			goto error;
		}
	}
	psThread->tr_pData = pBuffer;
	psThread->tr_nCurDataSize = nSize;
	psThread->tr_nMaxDataSize = nSize;
	psThread->tr_hDataSender = sys_get_thread_id( NULL );

	sched_unlock();
	put_cpu_flags( nFlg );

	UNLOCK( psThread->tr_hRecvSem );
	return ( 0 );
      error:
	kfree( pBuffer );
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

uint32 sys_receive_data( thread_id *const phSender, void *const pData, const uint32 nSize )
{
	Thread_s *psThread = CURRENT_THREAD;
	int nError;

      retry:
	nError = lock_semaphore( psThread->tr_hRecvSem, 0, INFINITE_TIMEOUT );

	if ( nError < 0 )
	{
		return ( nError );
	}
	if ( psThread->tr_pData == NULL )
	{
		printk( "Panic: sys_receive_data() aliens are steling our data\n" );
		goto retry;
	}
	nError = memcpy_to_user( pData, psThread->tr_pData, min( nSize, psThread->tr_nCurDataSize ) );
	if ( nError >= 0 )
	{
		int nFlg;

		kfree( psThread->tr_pData );
		nError = psThread->tr_nCode;
		nFlg = cli();
		sched_lock();

		psThread->tr_pData = NULL;
		psThread->tr_nCurDataSize = 0;
		psThread->tr_nMaxDataSize = 0;
		psThread->tr_hDataSender = -1;

		wake_up_queue( false, psThread->tr_psSendWaitQueue, 0, true );

		sched_unlock();
		put_cpu_flags( nFlg );
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_thread_data_size( void )
{
	Thread_s *psThread = CURRENT_THREAD;
	int nError;

      /*** BROKEN BROKEN BROKEN ***/
	psThread = CURRENT_THREAD;
      retry:
	nError = lock_semaphore( psThread->tr_hRecvSem, 0, INFINITE_TIMEOUT );

	if ( nError < 0 )
	{
		return ( nError );
	}

	if ( psThread->tr_pData == NULL )
	{
		printk( "Panic: sys_thread_data_size() aliens are stealing our data\n" );
		goto retry;
	}
	UNLOCK( psThread->tr_hRecvSem );	// We did not drain the buffer, so ...
	return ( psThread->tr_nCurDataSize );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_has_data( const thread_id hThread )
{
	int nStatus;
	Thread_s *psThread;
	int nFlg;

	nFlg = cli();
	sched_lock();

	psThread = get_thread_by_handle( hThread );

	if ( psThread == NULL )
	{
		nStatus = -ESRCH;
		goto error;
	}
	nStatus = ( psThread->tr_pData == NULL ) ? 0 : 1;
      error:
	sched_unlock();
	put_cpu_flags( nFlg );

	return ( nStatus );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

Thread_s *Thread_New( Process_s *psProc )
{
	Thread_s *psThread;

	psThread = kmalloc( sizeof( Thread_s ), MEMF_CLEAR | MEMF_KERNEL | MEMF_LOCKED | MEMF_OKTOFAIL );

	if ( psThread == NULL )
	{
		goto error1;
	}
	psThread->tr_nStackArea = -1;

	psThread->tc_lKStackSize = PAGE_SIZE * 5;

	psThread->tc_plKStack = kmalloc( psThread->tc_lKStackSize, MEMF_KERNEL | MEMF_OKTOFAIL );

	if ( psThread->tc_plKStack == NULL )
	{
		goto error2;
	}
	psThread->tc_plKStack[0] = KSTACK_MAGIC;
	psThread->tc_plKStack += psThread->tc_lKStackSize / 4 - 1;

	psThread->tr_hRecvSem = create_semaphore( "thread_recv", 0, 0 );

	if ( psThread->tr_hRecvSem < 0 )
	{
		goto error3;
	}

#ifdef __DETECT_DEADLOCK
	memset( psThread->tr_ahObtainedSemas, -1, 256 * sizeof( int ) );
#endif

	psThread->tr_nCurrentCPU = -1;
	psThread->tr_nTargetCPU = -1;
	psThread->tr_hBlockSema = -1;
	psThread->tr_hThreadID = -1;
	psThread->tr_hDataSender = -1;
	psThread->tr_hParent = 1;
	psThread->tr_psProcess = psProc;
	psThread->tr_nPriority = 0;
	psThread->tr_nQuantum = DEFAULT_QUANTUM;

	strcpy( psThread->tr_zName, "AtheOS" );

	psThread->tr_nSysTraceMask = STRACE_DISABLED;
	psThread->psExc = NULL;

	psThread->tr_pESP = psThread->tc_plKStack;
	psThread->tr_pESP0 = psThread->tc_plKStack;
	psThread->tr_nSS0 = 0x18;

//	save_fpu_state( &psThread->tc_FPUState );

	atomic_inc( &psProc->pr_nThreadCount );
	atomic_inc( &psProc->pr_nLivingThreadCount );
	atomic_inc( &g_sSysBase.ex_nThreadCount );

	return ( psThread );
	delete_semaphore( psThread->tr_hRecvSem );
      error3:
	kfree( psThread->tc_plKStack - ( psThread->tc_lKStackSize / 4 - 1 ) );
      error2:
	kfree( psThread );
      error1:
	return ( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void Thread_Delete( Thread_s *psThread )
{
	Process_s *psProc = psThread->tr_psProcess;
	uint32 *pStack = psThread->tc_plKStack - ( psThread->tc_lKStackSize / 4 - 1 );
	int nOldFlg;

	if ( pStack[0] != KSTACK_MAGIC )
	{
		printk( "Panic: kernel-stack has been overflowed by thread %s!\n", psThread->tr_zName );
	}

	flock_thread_died( psThread );

	nOldFlg = cli();

	sched_lock();

	MArray_Remove( &g_sThreadTable, psThread->tr_hThreadID );

	release_thread_semaphores( psThread->tr_hThreadID );

	sched_unlock();
	put_cpu_flags( nOldFlg );

	kfree( pStack );

	delete_semaphore( psThread->tr_hRecvSem );

	if ( psThread->tr_nStackArea != -1 )
	{
		delete_area( psThread->tr_nStackArea );
	}

	if( psThread->psExc != NULL )
	{
		SyscallExc_s *psExc;

		psExc = psThread->psExc;

		while( psExc != NULL )
		{
			if( psExc->psPrev )
				psExc->psPrev->psNext = psExc->psNext;
			if( psExc->psNext )
				psExc->psNext->psPrev = psExc->psPrev;
			else
				psThread->psExc = psExc->psPrev;

			kfree( psExc );

			psExc = psExc->psPrev;
		}
	}

	atomic_dec( &psProc->pr_nThreadCount );

	kassertw( atomic_read( &psProc->pr_nThreadCount ) >= 0 );

	if ( atomic_read( &psProc->pr_nThreadCount ) == 0 )
	{
		remove_process( psProc );
		delete_semaphore( psProc->pr_hSem );
		delete_mem_context( psProc->tc_psMemSeg );
		kfree( psProc );
		atomic_dec( &g_sSysBase.ex_nProcessCount );
	}
	kfree( psThread );
	atomic_dec( &g_sSysBase.ex_nThreadCount );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_SetThreadExitCode( const thread_id hThread, const int nCode )
{
	int nError;
	int noldFlg = cli();
	Thread_s *psThread;

	sched_lock();

	psThread = get_thread_by_handle( hThread );

	if ( psThread != NULL )
	{
		psThread->tr_nExitCode = ( nCode & 0xff ) << 8;
		nError = 0;
	}
	else
	{
		nError = -ESRCH;
	}
	sched_unlock();
	put_cpu_flags( noldFlg );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

thread_id sys_spawn_thread( const char *const pzName, void *const pfEntry, const int nPri, int nStackSize, void *const pData )
{
	Thread_s *psParentThread = CURRENT_THREAD;
	Process_s *psParent = psParentThread->tr_psProcess;
	thread_id hNewThread = -1;
	Thread_s *psNewThread;
	AreaInfo_s sAreaInfo;
	uint32 *pnUserStack;
	int nFlg;
	uint8 *pCode;
	uint32 anStackInit[5];
	uint8 *pExitFunction;
	int *pErrnoPtr;
	int nError;

	if ( nStackSize == 0 )
	{
		nStackSize = DEFAULT_STACK_SIZE;
	}
	else if ( nStackSize < 32 * 1024 )
	{
		nStackSize = 32 * 1024;
	}
	nStackSize += TLD_SIZE;
	nStackSize = ( nStackSize + PAGE_SIZE - 1 ) & PAGE_MASK;

	psNewThread = Thread_New( psParent );

	if ( psNewThread == NULL )
	{
		nError = -ENOMEM;
		goto error1;
	}
	nError = strncpy_from_user( psNewThread->tr_zName, pzName, OS_NAME_LENGTH - 1 );
	psNewThread->tr_zName[OS_NAME_LENGTH - 1] = '\0';
	if ( nError < 0 )
	{
		goto error2;
	}

	psNewThread->tr_nSysTraceMask = psParentThread->tr_nSysTraceMask;
	psNewThread->tr_nUMask = psParentThread->tr_nUMask;
	psNewThread->tr_nUStackSize = nStackSize;
	psNewThread->tr_pUserStack = NULL;
	psNewThread->tr_nStackArea = create_area( "stack", &psNewThread->tr_pUserStack, psNewThread->tr_nUStackSize, psNewThread->tr_nUStackSize, AREA_ANY_ADDRESS | AREA_FULL_ACCESS, AREA_NO_LOCK );

	if ( psNewThread->tr_nStackArea < 0 )
	{
		nError = psNewThread->tr_nStackArea;
		goto error2;
	}
	get_area_info( psNewThread->tr_nStackArea, &sAreaInfo );
	psNewThread->tr_pThreadData = ( ( ( uint8 * )sAreaInfo.pAddress ) - TLD_SIZE ) + psNewThread->tr_nUStackSize;
	pnUserStack = ( ( uint32 * )psNewThread->tr_pThreadData ) - 1;

	psNewThread->tr_nPriority = nPri;
	psNewThread->tr_pData = NULL;
	psNewThread->tr_hDataSender = -1;

	pCode = ( uint8 * )&pnUserStack[-5];

	/*  [0] - alignment to 16 bytes
	 *  [1]
	 *  [2]
	 *  [3] - exit function
	 *  [4]
	 *  [5]
	 *  [6] - thread entry point arg
	 *  [7] - exit return pointer
	 */

	anStackInit[0] = ( uint32 )pCode;
	anStackInit[1] = ( uint32 )pData;
	pExitFunction = ( uint8 * )( anStackInit + 2 );


	// movl   %eax,%ebx
	pExitFunction[0] = 0x89;
	pExitFunction[1] = 0xc3;

	// movl   $__NR_exit_thread,%eax
	pExitFunction[2] = 0xb8;
	( ( uint32 * )&pExitFunction[3] )[0] = __NR_exit_thread;

	// int $0x80
	pExitFunction[7] = 0xcd;
	pExitFunction[8] = 0x80;

	nError = memcpy_to_user( pnUserStack - 7, anStackInit, 5 * 4 );
	if ( nError < 0 )
	{
		goto error2;
	}
	
	/* Prepare kernel stack */
	SysCallRegs_s* psRegs = (SysCallRegs_s*)( psNewThread->tc_plKStack - sizeof( SysCallRegs_s ) / 4 - 1 );
	memset( psRegs, 0, sizeof( SysCallRegs_s ) );
		
	psRegs->ds = DS_USER;
	psRegs->es = DS_USER;
	psRegs->fs = DS_USER;
	psRegs->gs = DS_USER;
	psRegs->eip = (uint32)pfEntry;
	psRegs->cs = CS_USER;
	psRegs->eflags = 0x203246;
	psRegs->oldesp = (uint32)&pnUserStack[-7];
	psRegs->oldss = DS_USER;

	psNewThread->tr_pESP = ( void* )psRegs;
	psNewThread->tr_pEIP = exit_from_sys_call;

	psNewThread->tr_nState = TS_WAIT;

      again:
	nFlg = cli();
	sched_lock();
	psNewThread->tr_hThreadID = MArray_Insert( &g_sThreadTable, psNewThread, true );
	hNewThread = psNewThread->tr_hThreadID;
	sched_unlock();
	put_cpu_flags( nFlg );

	if ( hNewThread == -ENOMEM )
	{
		shrink_caches( 65536 );
		goto again;
	}

	pErrnoPtr = ( int * )( psNewThread->tr_pThreadData + TLD_ERRNO );
	memcpy_to_user( psNewThread->tr_pThreadData + TLD_THID, &psNewThread->tr_hThreadID, sizeof( psNewThread->tr_hThreadID ) );
	memcpy_to_user( psNewThread->tr_pThreadData + TLD_PRID, &psParent->tc_hProcID, sizeof( psParent->tc_hProcID ) );
	memcpy_to_user( psNewThread->tr_pThreadData + TLD_ERRNO_ADDR, &pErrnoPtr, sizeof( pErrnoPtr ) );
	memcpy_to_user( psNewThread->tr_pThreadData + TLD_BASE, &psNewThread->tr_pThreadData, sizeof( psNewThread->tr_pThreadData ) );

	return ( hNewThread );
      error2:
	Thread_Delete( psNewThread );
      error1:
	return ( nError );
}

thread_id old_spawn_thread( const char *const pzName, void *const pfEntry, const int nPri, void *const pData )
{
	return ( sys_spawn_thread( pzName, pfEntry, nPri, OLD_DEFAULT_STACK_SIZE, pData ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void kthread_exit( void )
{
	do_exit( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

thread_id spawn_kernel_thread( const char *const pzName, void *const pfEntry, const int nPri, int nStackSize, void *const pData )
{
	Process_s *psParent = get_proc_by_handle( 0 );	/* The first process is used for kernel threads */
	thread_id hNewThread = -1;
	Thread_s *psNewThread;
	int nFlg;
	int nError;

	psNewThread = Thread_New( psParent );

	if ( psNewThread == NULL )
	{
		nError = -ENOMEM;
		goto error1;
	}

	psNewThread->tr_nUMask = S_IWGRP | S_IWOTH;

	strncpy( psNewThread->tr_zName, pzName, 32 );
	psNewThread->tr_zName[31] = '\0';

	psNewThread->tr_nPriority = nPri;
	psNewThread->tr_pData = NULL;
	psNewThread->tr_hDataSender = -1;

	/* Prepare kernel stack */
	SysCallRegs_s* psRegs = (SysCallRegs_s*)( ( uint8* )( psNewThread->tc_plKStack - 5 ) - sizeof( SysCallRegs_s ) + 8 );
	memset( psRegs, 0, sizeof( SysCallRegs_s ) );
		
	psRegs->ds = DS_USER;
	psRegs->es = DS_USER;
	psRegs->fs = DS_USER;
	psRegs->gs = DS_USER;
	psRegs->eip = (uint32)pfEntry;
	psRegs->cs = CS_KERNEL;
	psRegs->eflags = 0x203246;

	/* Yes, this overwrites the last 8 bytes of the SysCallRegs_s structure. We are jumping back from kernel space
	   to kernel space and so the stack pointer and stack segment are not used */
	psNewThread->tc_plKStack[-5] = ( uint32 )kthread_exit;	// sys_TaskEnd;
	psNewThread->tc_plKStack[-4] = ( uint32 )pData;

	psNewThread->tr_pESP = (void*)psRegs;

	psNewThread->tr_pEIP = exit_from_sys_call;


      again:
	nFlg = cli();
	sched_lock();
	psNewThread->tr_hThreadID = MArray_Insert( &g_sThreadTable, psNewThread, true );
	hNewThread = psNewThread->tr_hThreadID;	// add_thread( psNewThread );

	psNewThread->tr_nState = TS_WAIT;
	sched_unlock();
	put_cpu_flags( nFlg );

	if ( hNewThread < 0 )
	{
		nError = hNewThread;
		if ( nError == -ENOMEM )
		{
			shrink_caches( 65536 );
			goto again;
		}
		goto error2;
	}
	return ( hNewThread );
      error2:
	Thread_Delete( psNewThread );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void db_dump_thread( int argc, char **argv )
{
	thread_id hThread;
	Thread_s *psThread;

	if ( argc != 2 )
	{
		dbprintf( DBP_DEBUGGER, "Usage: %s thread_id\n", argv[0] );
		return;
	}
	hThread = atol( argv[1] );

	psThread = get_thread_by_handle( hThread );

	if ( NULL == psThread )
	{
		dbprintf( DBP_DEBUGGER, "Error: Thread %d does not exist\n", hThread );
		return;
	}

	dbprintf( DBP_DEBUGGER, "Name=%s : id=%d : parent=%d\n", psThread->tr_zName, psThread->tr_hThreadID, psThread->tr_hParent );

	dbprintf( DBP_DEBUGGER, "Proc = %p : Next=%p : NextInProc=%p\n", psThread->tr_psProcess, DLIST_NEXT( psThread, tr_psNext ), psThread->tr_psNextInProc );

	dbprintf( DBP_DEBUGGER, "HasExeced=%d : ExitCode=%08x\n", psThread->tr_bHasExeced, psThread->tr_nExitCode );


	dbprintf( DBP_DEBUGGER, "State=%d\n", psThread->tr_nState );
	dbprintf( DBP_DEBUGGER, "Flags = %08x\n", psThread->tr_nFlags );


      /*** Timing members	***/

/*  
    psThread->tr_nSysTime;
    psThread->tr_nCPUTime;
    psThread->tr_nQuantum;
    psThread->tr_nLaunchTime;

    psThread->tr_nCR2;
    */
	dbprintf( DBP_DEBUGGER, "Pri=%d\n", psThread->tr_nPriority );

//  psThread->tr_nResumeTime;           /* Time when a sleeping thread should be woken  */

      /*** Nesting variables for various states	***/

//  dbprintf( DBP_DEBUGGER, "TDNestCnt=%d\n", psThread->tc_TDNestCnt );
	dbprintf( DBP_DEBUGGER, "SymLinkDepth=%d\n", psThread->tr_nSymLinkDepth );

      /*** Stack pointers, and sizes	***/

	dbprintf( DBP_DEBUGGER, "Stack size=0x%08x\n", psThread->tr_nUStackSize );
	dbprintf( DBP_DEBUGGER, "StackArea=%d\n", psThread->tr_nStackArea );

	dbprintf( DBP_DEBUGGER, "SysStackTop=0x%p, size=0x%08x\n", psThread->tc_plKStack, psThread->tc_lKStackSize );


	dbprintf( DBP_DEBUGGER, "Sendt data     = 0x%p\n", psThread->tr_pData );
	dbprintf( DBP_DEBUGGER, "Sendt code     = %d\n", psThread->tr_nCode );
	dbprintf( DBP_DEBUGGER, "CurDataSize    = %d\n", psThread->tr_nCurDataSize );
	dbprintf( DBP_DEBUGGER, "MaxDataSize    = %d\n", psThread->tr_nMaxDataSize );
//  dbprintf( DBP_DEBUGGER, "IsWaingForData = %d\n", psThread->tr_isWaitingForData );
	dbprintf( DBP_DEBUGGER, "DataSender     = %d\n", psThread->tr_hDataSender );

	dbprintf( DBP_DEBUGGER, "BlockSema = %d\n", psThread->tr_hBlockSema );

	dbprintf( DBP_DEBUGGER, "Target CPU = %d\n", psThread->tr_nTargetCPU );
	dbprintf( DBP_DEBUGGER, "Current CPU  = %d\n", psThread->tr_nCurrentCPU );
	dbprintf( DBP_DEBUGGER, "V86 nest cnt = %d\n", atomic_read( &psThread->tr_nInV86 ) );
	dbprintf( DBP_DEBUGGER, "SigPend      = %08lx%08lx\n", psThread->tr_sSigPend.__val[1], psThread->tr_sSigPend.__val[0] );
	dbprintf( DBP_DEBUGGER, "SigBlock     = %08lx%08lx\n", psThread->tr_sSigBlock.__val[1], psThread->tr_sSigBlock.__val[0] );

//  SigAction_s  tr_apsSigHandlers[ NSIG ];
	dbprintf( DBP_DEBUGGER, "SysTraceMask   = %d\n", psThread->tr_nSysTraceMask );
	dbprintf( DBP_DEBUGGER, "EIP            = 0x%08x\n", psThread->tr_nLastEIP );
//  dbprintf( DBP_DEBUGGER, "NewRetValMethod = %d\n", psThread->tr_bNewRetValMethode );

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void db_nice( int argc, char **argv )
{
	thread_id hThread;
	Thread_s *psThread;
	int nPri;

	if ( argc != 3 )
	{
		dbprintf( DBP_DEBUGGER, "Usage: %s pri thread_id\n", argv[0] );
		return;
	}
	nPri = atol( argv[1] );
	hThread = atol( argv[2] );

	psThread = get_thread_by_handle( hThread );

	if ( NULL == psThread )
	{
		dbprintf( DBP_DEBUGGER, "Error: Thread %d does not exist\n", hThread );
		return;
	}
	psThread->tr_nPriority = nPri;
	dbprintf( DBP_DEBUGGER, "Thread %s (%d) got pri %d\n", psThread->tr_zName, hThread, nPri );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void db_kill( int argc, char **argv )
{
	thread_id hThread;
	Thread_s *psThread;
	int nSig;

	if ( argc != 3 )
	{
		dbprintf( DBP_DEBUGGER, "Usage: %s sig thread_id\n", argv[0] );
		return;
	}
	nSig = atol( argv[1] );
	hThread = atol( argv[2] );

	psThread = get_thread_by_handle( hThread );

	if ( NULL == psThread )
	{
		dbprintf( DBP_DEBUGGER, "Error: Thread %d does not exist\n", hThread );
		return;
	}
	dbprintf( DBP_DEBUGGER, "Send signal %d to %s (%d)\n", nSig, psThread->tr_zName, hThread );
	sys_kill( hThread, nSig );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void db_list_threads( int argc, char **argv )
{
	thread_id hThread;
	Thread_s *psThread;

	dbprintf( DBP_DEBUGGER, "ID      PROC        STATE      PRI\n" );
	FOR_EACH_THREAD( hThread, psThread )
	{
		char *pzState;
		char zState[128];

		switch ( psThread->tr_nState )
		{
		case TS_STOPPED:
			pzState = "Stopped";
			break;
		case TS_RUN:
			pzState = "Run";
			break;
		case TS_READY:
			pzState = "Ready";
			break;
		case TS_WAIT:
			pzState = "Wait";
			break;
		case TS_SLEEP:
			pzState = "Sleep";
			break;
		case TS_ZOMBIE:
			pzState = "Zombie";
			break;
		default:
			pzState = "Invalid";
			break;
		}

		if ( psThread->tr_hBlockSema >= 0 )
		{
			Semaphore_s *psSema = get_semaphore_by_handle( psThread->tr_psProcess->tc_hProcID,
				psThread->tr_hBlockSema );

			if ( NULL == psSema )
			{
				sprintf( zState, "inval_sema(%d)", psThread->tr_hBlockSema );
				pzState = zState;
			}
			else
			{
				sprintf( zState, "%s(%d)", psSema->ss_zName, psThread->tr_hBlockSema );
				pzState = zState;
			}
		}
		dbprintf( DBP_DEBUGGER, "%05d - %05d %-12s %04d %s::%s\n", hThread, psThread->tr_psProcess->tc_hProcID, pzState, psThread->tr_nPriority, psThread->tr_zName, psThread->tr_psProcess->tc_zName );
	}

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_threads( void )
{
	MArray_Init( &g_sThreadTable );
	register_debug_cmd( "ps", "list all threads.", db_list_threads );
	register_debug_cmd( "dump_thread", "print information about a specified thread.", db_dump_thread );
	register_debug_cmd( "nice", "set a threads scheduler priority.", db_nice );
	register_debug_cmd( "kill", "send a signal to a thread.", db_kill );
}
