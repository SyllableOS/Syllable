
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

#include <atheos/typedefs.h>

#include <atheos/types.h>
#include <atheos/tld.h>
#include <atheos/filesystem.h>
#include <atheos/strace.h>

#include <posix/param.h>
#include <posix/signal.h>
#include <atheos/types.h>

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



typedef struct
{

      /*** normal regs, with special meaning for the segment descriptors.. ***/

	long ebx;
	long ecx;
	long edx;
	long esi;
	long edi;
	long ebp;
	long eax;
	long __null_ds;
	long __null_es;
	long __null_fs;
	long __null_gs;
	long orig_eax;
	long eip;
	uint16 cs, __csh;
	long eflags;
	long esp;
	uint16 ss, __ssh;

      /*** These are part of the v86 interrupt stackframe: ***/
	uint16 es, __esh;
	uint16 ds, __dsh;
	uint16 fs, __fsh;
	uint16 gs, __gsh;
} Virtual86Regs_s;

typedef struct
{
	Virtual86Regs_s regs;
	unsigned long flags;
	unsigned long screen_bitmap;
	unsigned long cpu_type;
} Virtual86Struct_s;

typedef struct kernel_vm86_struct
{
	Virtual86Regs_s regs;

/*
 * the below part remains on the kernel stack while we are in VM86 mode.
 * 'tss.esp0' then contains the address of VM86_TSS_ESP0 below, and when we
 * get forced back from VM86, the CPU and "SAVE_ALL" will restore the above
 * 'struct kernel_vm86_regs' with the then actual values.
 * Therefore, pt_regs in fact points to a complete 'kernel_vm86_struct'
 * in kernelspace, hence we need not reget the data from userspace.
 */
#define VM86_TSS_ESP0 flags
	unsigned long flags;
	unsigned long screen_bitmap;
	unsigned long cpu_type;

	SysCallRegs_s *regs32;	/* here we save the pointer to the old regs */

	Virtual86Regs_s *psRegs16;
	void *pSavedStack;

	struct kernel_vm86_struct *psNext;

} Virtual86State_s;








#define MAIN_THREAD_STACK_SIZE	(1024*1024*4)


#define	DEFAULT_QUANTUM	5000LL

#define	NUM_SIGS	32




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


struct _Thread
{
	Thread_s *tr_psNext;
	Thread_s *tr_psNextInProc;	/* Next thread in process               */

	Process_s *tr_psProcess;	/* Our process                          */
	thread_id tr_hThreadID;	/* Our thread ID                        */

	thread_id tr_hParent;	/* The thread that created us */
	bool tr_bHasExeced;

	char tr_zName[OS_NAME_LENGTH];	/* Thread name                          */
	int tr_nExitCode;

	TaskStateSeg_s tc_sTSS;	/* Intel 386 specific task state buffer */
	uint8 tc_FPUState[128];	/* Intel 387 specific FPU state buffer  */
	int32 tr_nState;	/* Current task state.                  */
	uint32 tr_nFlags;

	int tc_TSSDesc;		/* task descriptor                      */

      /*** Timing members	***/

	int64 tr_nSysTime;	/* Micros in kernal mode                */
	int64 tr_nCPUTime;	/* Total micros of execution            */
	int64 tr_nQuantum;	/* Maximum allowed micros of execution before preemption        */
	int64 tr_nLaunchTime;

	int tr_nCR2;

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

	sigset_t tr_nSigPend;
	sigset_t tr_nSigBlock;

	SigAction_s tr_apsSigHandlers[NSIG];

	/* SMP stuff */
	int tr_nCurrentCPU;
	int tr_nPrevCPU;
	atomic_t tr_nInV86;

	/* Debugging */
	int tr_nSysTraceMask;
	SyscallExc_s *psExc;

	int tr_nLastCS;
	void *tr_pLastEIP;
	int tr_nAfsInodeLockCount;
	int tr_nNumLockedCacheBlocks;
};

#define	TF_TRACED	0x0001


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


#define DEFAULT_KERNEL_STACK	4096
#define	DEFAULT_STACK_SIZE	(256 * 1024)


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

void add_to_sleeplist( WaitQueue_s *psNode );
void remove_from_sleeplist( WaitQueue_s *psNode );
void add_thread_to_ready( Thread_s *psThread );

void add_to_waitlist( WaitQueue_s **psList, WaitQueue_s *psNode );
void remove_from_waitlist( WaitQueue_s **ppsList, WaitQueue_s *psNode );

void wake_up_sleepers( bigtime_t nCurTime );
int wake_up_queue( WaitQueue_s *psList, int nCode, bool bAll );
int sleep_on_queue( WaitQueue_s **ppsList );

thread_id get_next_thread( const thread_id hPrev );
thread_id get_prev_thread( const thread_id hPrev );




#ifdef __cplusplus
}
#endif

#endif /* __F_SCHEDULER_H__ */
