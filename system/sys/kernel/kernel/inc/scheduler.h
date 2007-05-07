
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

#ifndef	__F_SCHEDULER_H__
#define	__F_SCHEDULER_H__

#include <atheos/types.h>
#include <atheos/tld.h>
#include <atheos/filesystem.h>
#include <atheos/strace.h>

#include <posix/param.h>
#include <posix/signal.h>
#include <atheos/types.h>
#include <atheos/dlist.h>

#include "typedefs.h"
#include "semaphore.h"
#include "intel.h"
#include "mman.h"
#include "msgport.h"

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif

#include "image.h"

extern int g_bNeedSchedule;	/* If true the scheduler will be called when returning from syscall */

static const size_t MAIN_THREAD_STACK_SIZE	= (1024*1024*4);
static const size_t DEFAULT_KERNEL_STACK	= (4096);
static const size_t OLD_DEFAULT_STACK_SIZE	= (1024 * 256);
static const size_t DEFAULT_STACK_SIZE		= (1024 * 128);

static const size_t DEFAULT_QUANTUM = 5000;

static const size_t NUM_SIGS = 32;

typedef struct WaitThreadNode WaitThreadNode_s;
struct WaitThreadNode
{
	WaitThreadNode_s *psNext;
	thread_id hOwner;
	int nResult;		/* Thread's exit code   */
};

struct _WaitQueue
{
	WaitQueue_s *wq_psNext;
	WaitQueue_s *wq_psPrev;
	thread_id wq_hThread;
	int wq_nCode;
	bigtime_t wq_nResumeTime;
	bool wq_bIsMember;
	bool wq_bOneShot;
	bigtime_t wq_nTimeout;
	timer_callback *wq_pfCallBack;
	void *wq_pUserData;
};

typedef DLIST_HEAD(, _Thread) ThreadList_s;

struct _Thread
{
	DLIST_ENTRY(_Thread)	tr_psNext;	///< Link into thread lists
	Thread_s *tr_psNextInProc;	/* Next thread in process               */

	Process_s *tr_psProcess;	/* Our process                          */
	thread_id tr_hThreadID;	/* Our thread ID                        */

	thread_id tr_hParent;	/* The thread that created us */
	bool tr_bHasExeced;

	char tr_zName[OS_NAME_LENGTH];	/* Thread name                          */
	int tr_nExitCode;

	// Hack alert!  kmalloc() alignment is off by 8 due to block_header,
	// and we need 16 byte alignment in order to use fxsave.
	long padding[2] __attribute__(( aligned(16) ));
	union i3FPURegs_u tc_FPUState;	/* Intel 387 specific FPU state buffer  */
	int32 tr_nState;	/* Current task state.                  */
	uint32 tr_nFlags;

      /*** Timing members	***/

	int64 tr_nSysTime;	/* Micros in kernel mode                */
	int64 tr_nCPUTime;	/* Total micros of execution            */
	int64 tr_nQuantum;	/* Maximum allowed micros of execution before preemption        */
	int64 tr_nLaunchTime;

	/* Registers */
	int tr_nCR2;
	void* tr_pEIP;
	void* tr_pESP;
	void* tr_pESP0;
	uint32 tr_nSS0;

	int tr_nPriority;

	int tr_nSymLinkDepth;	/* Level of reqursion while hunting for symlinks        */

      /*** Stack pointers, and sizes	***/

	uint8 *tr_pThreadData;	/* Pointer to this thread's TLD area                    */
	void *tr_pUserStack;	/* Pointer to the lowest address of the user stack      */
	uint32 tr_nUStackSize;	/* Number of bytes in user stack                        */
	area_id tr_nStackArea;

	uint32 *tc_plKStack;	/* Top of kernel stack                                  */
	uint32 tc_lKStackSize;	/* Size (in bytes) of kernel stack                      */


	void *tr_pData;		/* Data sendt by som other thread                       */
	uint32 tr_nCode;	/* Code assosiated with tr_pData                        */
	uint32 tr_nCurDataSize;	/* Number of bytes in tr_pData                          */
	uint32 tr_nMaxDataSize;

	thread_id tr_hDataSender;	/* ID of the thread that delivered last data package, or -1 if there is no data */
	sem_id tr_hRecvSem;	/* Released each time we receive data */
	WaitQueue_s *tr_psSendWaitQueue;	/* Threads waiting for us to fetch the receive buffer */
	WaitQueue_s *tr_psTermWaitList;	/* Threads waiting for us to die */

	int tr_hBlockSema;	/* The semaphore we wait's for, or -1   */
#if 0
	int tr_ahObtainedSemas[256];
	int tr_nDeadLockDetectRun;
#endif
	FileLockRec_s *tr_psFirstFileLock;

	int tr_nUMask;

	sigset_t tr_sSigPend;
	sigset_t tr_sSigBlock;

	SigAction_s tr_apsSigHandlers[_NSIG];

	/* SMP stuff */
	int tr_nCurrentCPU;
	int tr_nTargetCPU;
	atomic_t tr_nInV86;

	/* Debugging */
	int tr_nSysTraceMask;
	SyscallExc_s *psExc;
	atomic_t tr_nPTraceFlags;
	thread_id tr_hRealParent;
	SysCallRegs_s *tr_psPTraceRegs;
	uint32 tr_nPTraceVMCloneBase;
	area_id tr_hPTraceVMClone;
	uint32 tr_nDebugReg[8];
	uint32 tr_nLastEIP;

	/* Block cache */
	int tr_nNumLockedCacheBlocks;
};

static const flags_t TF_FPU_USED = 0x0002;	///< thread has used FPU
static const flags_t TF_FPU_DIRTY = 0x0004;	///< thread is currently using FPU

struct _Process
{
	char tc_zName[OS_NAME_LENGTH];	/* Process name */

	Thread_s *pr_psFirstThread;
	atomic_t pr_nThreadCount;	/* Number of thread spawned by this process */
	atomic_t pr_nLivingThreadCount;
	proc_id tc_hParent;	/* Parent process */
	proc_id tc_hProcID;	/* Our process ID */
	int pr_nOldTTYPgrp;

	uid_t pr_nUID, pr_nEUID, pr_nSUID, pr_nFSUID;
	gid_t pr_nGID, pr_nEGID, pr_nSGID, pr_nFSGID;
	int pr_nNumGroups;
	gid_t pr_anGroups[NGROUPS];


	int pr_hPGroupID;
	int pr_nSession;
	bool pr_bIsGroupLeader;

	ImageContext_s *pr_psImageContext;

	MemContext_s *tc_psMemSeg;	/* MMU segment. */

	area_id pr_nHeapArea;

	Semaphore_s *pr_psFirstSema;
	Semaphore_s *pr_psLastSema;
	int pr_nSemaCount;
	MsgPort_s *pr_psFirstPort;
	MsgPort_s *pr_psLastPort;
	int pr_nPortCount;
	IoContext_s *pr_psIoContext;
	SemContext_s *pr_psSemContext;
	sem_id pr_hSem;
	uint32 pr_anTLDBitmap[TLD_SIZE / ( sizeof( int ) * 32 )];
	bool pr_bUsedPriv;	/* The process has taken advantage of being root */
	bool pr_bCanDump;
};


Thread_s *get_current_thread( void );
Thread_s *get_idle_thread( void );
void set_idle_thread( Thread_s *psThread );

#define	CURRENT_THREAD	(get_current_thread())
#define	IDLE_THREAD	(get_idle_thread())
#define	CURRENT_PROC	CURRENT_THREAD->tr_psProcess


#define	FOR_EACH_THREAD( pid, thread )  for ( (pid) = 0 ; \
					      (-1 != (pid)) && ((thread) = get_thread_by_handle( pid )) ; \
					      (pid) = get_next_thread( (pid) ) )


int __sched_lock( void );
void sched_unlock( void );

#define sched_lock() kassertw( __sched_lock() >= 0 );

void init_processes( void );
void init_threads( void );
void init_scheduler( void );

void handle_v86_fault( Virtual86Regs_s * psRegs, uint32 nErrorCode );
void handle_v86_pagefault( Virtual86Regs_s * regs, uint32 nErrorCode );
void handle_v86_divide_exception( Virtual86Regs_s * psRegs, uint32 nErrorCode );
int call_v86( Virtual86Struct_s * psState );
int sys_call_v86( Virtual86Struct_s * psState );

int LockProcess( void );
int UnlockProcess( void );

proc_id add_process( Process_s *psProc );
void remove_process( Process_s *psProc );


Process_s *get_proc_by_handle( proc_id hProcID );

Thread_s *get_thread_by_name( const char *pzName );
Thread_s *get_thread_by_handle( thread_id hThread );


Thread_s *Thread_New( Process_s *psProc );
void Thread_Delete( Thread_s *psThread );

thread_id create_idle_thread( const char *Name );



thread_id add_thread( Thread_s *psThread );
proc_id add_process( Process_s *psProc );

void add_to_sleeplist( bool bLock, WaitQueue_s *psNode );
void remove_from_sleeplist( WaitQueue_s *psNode );
void add_thread_to_ready( Thread_s *psThread );

void add_to_waitlist( bool bLock, WaitQueue_s **psList, WaitQueue_s *psNode );
void remove_from_waitlist( bool bLock, WaitQueue_s **ppsList, WaitQueue_s *psNode );

status_t  do_wakeup_thread( thread_id hThread, bool bWakeupSuspended );

void wake_up_sleepers( bigtime_t nCurTime );
int wake_up_queue( bool bLock, WaitQueue_s *psList, int nCode, bool bAll );
int sleep_on_queue( WaitQueue_s **ppsList );

thread_id get_next_thread( const thread_id hPrev );
thread_id get_prev_thread( const thread_id hPrev );

int send_signal( Thread_s *psThread, int nSigNum, bool bBypassChecks );

void reset_thread_quantum( Thread_s *psThread );

void DoSchedule( SysCallRegs_s* psRegs );

#ifdef __cplusplus
}
#endif

#endif /* __F_SCHEDULER_H__ */
