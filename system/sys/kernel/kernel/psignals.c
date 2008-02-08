
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
#include <posix/time.h>

#include <atheos/types.h>
#include <atheos/sigcontext.h>
#include <atheos/kernel.h>
#include <atheos/threads.h>
#include <atheos/syscall.h>
#include <atheos/spinlock.h>
#include <atheos/kdebug.h>
#include <atheos/time.h>
#include <atheos/strace.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/ptrace.h"
#include "inc/sysbase.h"

static const unsigned long _BLOCKABLE = ~( (1L << (SIGKILL - 1)) | (1L << (SIGSTOP - 1)) );

/* POSIX timers */
typedef struct _TimerNode TimerNode_s;
struct _TimerNode
{
	TimerNode_s *psNext;
	TimerNode_s *psPrev;
	uint64 nTimeOut;
	thread_id hThread;
	int nWhich;
	struct kernel_itimerval sValue;
};

static TimerNode_s *g_psFirstTimerNode = NULL;

SPIN_LOCK( g_sTimerListSpinLock, "timer_list_slock" );


extern int save_i387( struct _fpstate *buf );
extern void restore_i387( struct _fpstate *buf );

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if 0
void signal_parent( Thread_s *psThread )
{
	if ( -1 != psThread->tr_hParent )
	{
		sys_kill( psThread->tr_hParent, SIGCHLD );
	}
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if 0
void EnterSyscall( int dummy )
{
	Thread_s *psThread = CURRENT_THREAD;
	SysCallRegs_s *psRegs = ( SysCallRegs_s * ) & dummy;

	if ( NULL != psThread )
	{
//    psThread->tr_nSysCallCount++;
		psThread->tr_nLastCS = psRegs->cs;
		psThread->tr_pLastEIP = ( void * )psRegs->eip;
	}
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if 0
void ExitSyscall( int dummy )
{
	Thread_s *psThread = CURRENT_THREAD;
	SysCallRegs_s *psRegs = ( SysCallRegs_s * ) & dummy;

	if ( psRegs->cs == 0x13 && ( psRegs->eflags & EFLG_IF ) == 0 && psRegs->orig_eax >= 0 )
	{
		printk( "PANIC : return from syscall with interrupts disabled!! I turn them on again.\n" );
		psRegs->eflags |= EFLG_IF;
		psThread->tr_nSysTraceMask = SYSC_GROUP_ALL;	/* Temporary strace (this function only) */
	}
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int is_signal_pending( void )
{
	Thread_s *psThread = CURRENT_THREAD;

	if ( psThread->tr_sSigPend.__val[0] & ~psThread->tr_sSigBlock.__val[0] ||
	     psThread->tr_sSigPend.__val[1] & ~psThread->tr_sSigBlock.__val[1] )
	{
		return ( 1 );
	}
	else
	{
		return ( 0 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_signal_mode( int nSigNum )
{
	Thread_s *psThread = CURRENT_THREAD;

	if ( nSigNum < 1 || nSigNum > _NSIG )
	{
		return ( -EINVAL );
	}

	if ( sigismember( &psThread->tr_sSigBlock, nSigNum ) )
	{
		return ( SIG_BLOCKED );
	}
	else
	{
		SigAction_s *psHandler = &psThread->tr_apsSigHandlers[nSigNum - 1];

		if ( SIG_DFL == psHandler->sa_handler )
		{
			return ( SIG_DEFAULT );
		}
		else if ( SIG_IGN == psHandler->sa_handler )
		{
			return ( SIG_IGNORED );
		}
		else
		{
			return ( SIG_HANDLED );
		}
	}
}

/*
 * POSIX 3.3.1.3:
 *  "Setting a signal action to SIG_IGN for a signal that is pending
 *   shall cause the pending signal to be discarded, whether or not
 *   it is blocked."
 *
 *  "Setting a signal action to SIG_DFL for a signal that is pending
 *   and whose default action is to ignore the signal (for example,
 *   SIGCHLD), shall cause the pending signal to be discarded, whether
 *   or not it is blocked"
 *
 * Note the silly behaviour of SIGCHLD: SIG_IGN means that the signal
 * isn't actually ignored, but does automatic child reaping, while
 * SIG_DFL is explicitly said by POSIX to force the signal to be ignored..
 */

int sys_sig_return( const int dummy )
{
	Thread_s *psThread = CURRENT_THREAD;
	SysCallRegs_s *psRegs = ( SysCallRegs_s * ) & dummy;
	SigContext_s sContext;

	memcpy( &sContext, ( void * )psRegs->oldesp, sizeof( sContext ) );

	psThread->tr_sSigBlock.__val[0]	= sContext.oldmask & _BLOCKABLE;
	psThread->tr_sSigBlock.__val[1]	= sContext.oldmask2;
	psThread->tr_nCR2 = sContext.cr2;

	psRegs->ds = sContext.ds;
	psRegs->es = sContext.es;
	psRegs->fs = sContext.fs;
	psRegs->gs = sContext.gs;
	psRegs->oldss = sContext.ss;
	psRegs->cs = sContext.cs;
	psRegs->eip = sContext.eip;
	psRegs->ecx = sContext.ecx;
	psRegs->edx = sContext.edx;
	psRegs->oldesp = sContext.esp;
	psRegs->ebp = sContext.ebp;
	psRegs->edi = sContext.edi;
	psRegs->esi = sContext.esi;

	if ( sContext.fpstate )
		restore_i387( sContext.fpstate );

	psRegs->eflags = sContext.eflags;

	psRegs->orig_eax = -1;	/* Disable syscall checks */

	return ( sContext.eax );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Set up a signal frame. Make the stack look the way iBCS2 expects
 *	it to look.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void AddSigHandlerFrame( Thread_s *psThread, SysCallRegs_s * psRegs, SigAction_s * psHandler, int nSigNum, sigset_t sOldBlockMask )
{
	uint32 *pStack = ( uint32 * )psRegs->oldesp;
	uint32 *pCode;
	uint32 *pFpuState;
	uint32 *pSigInfo;	// pointer to siginfo_t on stack
	uint32 *pStackTop;	// top of stack after parameters pushed

	pStack -= (23 + 3 + 156);    // SigContext_s + trampoline + FPUState_s
	pStackTop = pStack - 2;		// return trampoline and signal number
	
	

	if ( psHandler->sa_flags & SA_SIGINFO )
	{
		pStack -= 32;			// size of siginfo_t struct
		pStackTop = pStack - 9;		// 2nd and 3rd arg and ucontext
		pCode = pStack + 22;
		pSigInfo = pStack + 181;	// siginfo_t struct
		verify_mem_area( pStackTop, PAGE_SIZE, true );
		pStackTop[0] = ( uint32 )pCode;	// return trampoline address
		pStackTop[1] = nSigNum;
		pStackTop[2] = ( uint32 )pSigInfo;
		pStackTop[3] = ( uint32 )(&pStackTop[4]);
		pStackTop[4] = 0x0;		// uc_flags
		pStackTop[5] = 0x0;		// uc_link
		pStackTop[6] = 0x0;		// stack_t.ss_sp
		pStackTop[7] = 0x0;		// stack_t.ss_size
		pStackTop[8] = 0x0;		// stack_t.ss_flags
		pSigInfo[0]  = nSigNum;		// si_signo
		pSigInfo[1]  = 0x0;		// si_errno
		pSigInfo[2]  = 0x0;		// si_code
		// TODO fill in the rest of siginfo for different signals
	}
	else
	{
		pStackTop = pStack - 2;		// return trampoline and signal
		verify_mem_area( pStackTop, PAGE_SIZE, true );
		pCode = pStack + 22;
		pStackTop[0] = ( uint32 )( &pCode[1] );	// only pop 1 arg
		pStackTop[1] = nSigNum;
	}

	pFpuState = pStack + 25;

	pStack[0] = psRegs->gs;
	pStack[1] = psRegs->fs;
	pStack[2] = psRegs->es;
	pStack[3] = psRegs->ds;
	pStack[4] = psRegs->edi;
	pStack[5] = psRegs->esi;
	pStack[6] = psRegs->ebp;
	pStack[7] = psRegs->oldesp;
	pStack[8] = psRegs->ebx;
	pStack[9] = psRegs->edx;
	pStack[10] = psRegs->ecx;
	pStack[11] = psRegs->eax;
	pStack[12] = nSigNum;
	pStack[13] = 0;			// error code
	pStack[14] = psRegs->eip;
	pStack[15] = psRegs->cs;
	pStack[16] = psRegs->eflags;
	pStack[17] = psRegs->oldesp;
	pStack[18] = psRegs->oldss;
	pStack[19] = ( uint32 )pFpuState;
	pStack[20] = sOldBlockMask.__val[0];
	pStack[21] = CURRENT_THREAD->tr_nCR2;
	pStack[22] = sOldBlockMask.__val[1];

	pCode[0] = 0x1cc48390;	/* add $0x1c,%esp */
	pCode[1] = 0x0000b858;	/* popl %eax ; movl $,%eax */
	pCode[2] = 0x80cd0000;	/* int $0x80 */
	( ( uint32 * )( ( ( uintptr_t )pCode ) + 6 ) )[0] = __NR_sig_return;

	if ( save_i387( (struct _fpstate * )pFpuState ) == 0 )
	{
		pStack[19] = 0;		// FPU hasn't been used
	}

	psRegs->oldesp = ( uint32 )pStackTop;
	psRegs->eip = ( uint32 )psHandler->sa_handler;
/*
  psRegs->cs = 0x10;
  psRegs->oldss = 0x20;
  psRegs->ds = 0x20;
  psRegs->es = 0x20;
  psRegs->fs = 0x20;
  psRegs->gs = 0x20;
  */
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void handle_signal( int nSigNum, SigAction_s * psHandler, SysCallRegs_s * psRegs, sigset_t sOldBlockMask )
{
	Thread_s *psThread = CURRENT_THREAD;

	/* are we from a system call? */
	if ( psRegs->orig_eax >= 0 )
	{
		/* If so, check system call restarting.. */
		switch ( psRegs->eax )
		{
		case -ERESTARTNOHAND:
			psRegs->eax = -EINTR;
			break;

		case -EINTR:
		case -ERESTARTSYS:
			if ( !( psHandler->sa_flags & SA_RESTART ) )
			{
				psRegs->eax = -EINTR;
				break;
			}
			/* fallthrough */
		case -ERESTARTNOINTR:

/*				printk( "handle_signal() : Restart syscall %d after sig %d\n", psRegs->orig_eax, nSigNum ); */
			psRegs->eax = psRegs->orig_eax;
			psRegs->eip -= 2;

			kassertw( 0x80cd == *( ( uint16 * )psRegs->eip ) );	/* int 0x80 */
			break;
		}
	}

	AddSigHandlerFrame( psThread, psRegs, psHandler, nSigNum, sOldBlockMask );

	if ( psHandler->sa_flags & SA_ONESHOT )
	{
		psHandler->sa_handler = SIG_DFL;
	}

	if ( !( psHandler->sa_flags & SA_NOMASK ) )
	{
		sigaddset( &psThread->tr_sSigBlock, nSigNum );
		sigorset( &psThread->tr_sSigBlock, &psThread->tr_sSigBlock, &psHandler->sa_mask );
		psThread->tr_sSigBlock.__val[0]	&= _BLOCKABLE;
	}
}

void handle_signal_ptrace( Thread_s *psThread, SysCallRegs_s *psRegs )
{
	uint64 nSignals;
	uint32 nSigNum = 0;

	if ( atomic_read( &psThread->tr_nPTraceFlags ) & PT_ALLOW_SIGNAL )
	{
		atomic_and( &psThread->tr_nPTraceFlags, ~PT_ALLOW_SIGNAL );
		return;
	}

	// copy signal mask into a single 64-bit variable
	nSignals = psThread->tr_sSigPend.__val[0] |
		( ( uint64_t ) psThread->tr_sSigPend.__val[1] << 32 );

	if ( nSignals & ( 1 << (SIGKILL - 1) ) )
		return;

	while ( !( nSignals & ( 1 << nSigNum ) ) )
		nSigNum++;
	sigemptyset( &psThread->tr_sSigPend );
	psThread->tr_nExitCode = nSigNum + 1;
	psThread->tr_psPTraceRegs = psRegs;

	printk( "ptraced thread received signal, stopping...\n" );
	stop_thread( true );  // does not return, will reschedule
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int handle_signals( int dummy )
{
	SysCallRegs_s *psRegs = ( SysCallRegs_s * ) & dummy;
	Thread_s *psThread = CURRENT_THREAD;
	Process_s *psProc = psThread->tr_psProcess;
	sigset_t sOldBlockMask = psThread->tr_sSigBlock;

	if ( psRegs->eflags & EFLG_VM )
	{
		return ( -EBUSY );
	}

	if ( NULL == psThread )
	{
		return ( -EINVAL );
	}
	for( int i = 0; i < MAX_CPU_COUNT; i++ )
	{
		if( g_asProcessorDescs[i].pi_bIsRunning && g_asProcessorDescs[i].pi_psIdleThread->tr_hThreadID == psThread->tr_hThreadID )
			return ( -EINVAL );	/* Dont do anything silly with the idle thread */
	}
	if ( is_signal_pending() )
	{
		// If the current thread is being ptraced, we might need to stop it.
		if ( atomic_read( &psThread->tr_nPTraceFlags ) & PT_PTRACED )
			handle_signal_ptrace( psThread, psRegs );

		// copy signal mask into a 64-bit variable
		uint64_t nSigMask = ( psThread->tr_sSigPend.__val[0] & ( ~sOldBlockMask.__val[0] ) ) |
			( ( uint64_t )( psThread->tr_sSigPend.__val[1] & ( ~sOldBlockMask.__val[1] ) ) << 32 );
		for ( int i = 0; i < _NSIG; ++i )
		{
			if ( nSigMask & 0x01 )
			{
				int nSigNum = i + 1;
				SigAction_s *psHandler = &psThread->tr_apsSigHandlers[i];

				nSigMask >>= 1;

				sigdelset( &psThread->tr_sSigPend, nSigNum );

				if ( SIG_IGN == psHandler->sa_handler )
				{
					if ( nSigNum != SIGCHLD )
					{
						continue;
					}
					else
					{
						/* Ignoring SIGCHLD means that we should do automatic zombie cleaning */
						while ( sys_waitpid( -1, NULL, WNOHANG ) > 0 )
							/* EMPTY */ ;
						continue;
					}
				}

				if ( SIG_DFL == psHandler->sa_handler )
				{
					switch ( nSigNum )
					{
					case SIGCONT:
					case SIGCHLD:
					case SIGWINCH:
						continue;

					case SIGTSTP:
					case SIGTTIN:
					case SIGTTOU:
						if ( is_orphaned_pgrp( psProc->pr_hPGroupID ) )
						{
							printk( "handle_signals() reject signal %d. pgroup is orphaned\n", nSigNum );
							continue;
						}

/*		continue; */
					case SIGSTOP:
						printk( "Halted by signal %d\n", nSigNum );
						psThread->tr_nExitCode = nSigNum;
						stop_thread( ( psThread->tr_apsSigHandlers[SIGCHLD - 1].sa_flags & SA_NOCLDSTOP ) == 0 );
						continue;

					case SIGQUIT:
					case SIGILL:
					case SIGTRAP:
					case SIGABRT:
					case SIGFPE:
					case SIGSEGV:
						/* Core dump */
						nSigNum |= 0x80;
						/* fall through */
						trace_stack( psRegs->eip, ( uint32 * )psRegs->ebp );
						printk( "Killed by signal %d\n", i + 1 );
					case SIGKILL:

					default:

/*		current->signal |= _S(signr & 0x7f);
		current->flags |= PF_SIGNALED;
		*/
//      psThread->tr_nExitCode = nSigNum;
						do_exit( nSigNum );
					}
				}
				if ( nSigNum == SIGCHLD && psHandler->sa_flags & SA_NOCLDWAIT )
				{
					while ( sys_waitpid( -1, NULL, WNOHANG ) > 0 )
						/* EMPTY */ ;
				}
				else
				{
					handle_signal( nSigNum, psHandler, psRegs, sOldBlockMask );
					return ( 1 );
				}
			}
			else
			{
				nSigMask >>= 1;
			}
		}
	}

	/* are we from a system call? */
	if ( psRegs->orig_eax >= 0 )
	{
		/* If so, check system call restarting.. */
		switch ( psRegs->eax )
		{
		case -ERESTARTNOHAND:
		case -EINTR:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			psRegs->eax = psRegs->orig_eax;
			psRegs->eip -= 2;
			kassertw( 0x80cd == *( ( uint16 * )psRegs->eip ) );	/* int 0x80 */
			break;
		}
	}
	return ( 0 );
}

static status_t remove_timer( int nWhich, TimerNode_s **ppsOld )
{
	TimerNode_s *psNode = NULL;
	int nOldFlags;
	thread_id hThread = sys_get_thread_id( NULL );
	status_t nError = EOK;

	nOldFlags = spinlock_disable( &g_sTimerListSpinLock );

	for( psNode = g_psFirstTimerNode; psNode != NULL; psNode = psNode->psNext )
	{
		if( psNode->hThread == hThread && psNode->nWhich == nWhich )
		{
			if( psNode->psNext != NULL )
				psNode->psNext->psPrev = psNode->psPrev;
			if( psNode->psPrev != NULL )
				psNode->psPrev->psNext = psNode->psNext;
			if( g_psFirstTimerNode == psNode )
				g_psFirstTimerNode = NULL;

			break;
		}
	}

	*ppsOld = psNode;
	if( NULL == psNode )
		nError = ESRCH;

	spinunlock_enable( &g_sTimerListSpinLock, nOldFlags );
	return nError;
}

static status_t insert_timer( int nWhich, struct kernel_itimerval *psValue )
{
	TimerNode_s *psNode = NULL;
	int nOldFlags;
	thread_id hThread = sys_get_thread_id( NULL );
	status_t nError = EOK;

	psNode = kmalloc( sizeof( TimerNode_s ), MEMF_KERNEL | MEMF_OKTOFAIL );
	if( NULL == psNode )
	{
		nError = ENOMEM;
	}
	else
	{
		bigtime_t nTimeOut;

		psNode->psPrev = NULL;
		psNode->psNext = NULL;

		memcpy( &psNode->sValue, psValue, sizeof( struct kernel_itimerval ) );

		nTimeOut = get_system_time();
		nTimeOut += (psNode->sValue.it_value.tv_sec * 1000000);
		nTimeOut += psNode->sValue.it_value.tv_usec;

		psNode->nTimeOut = nTimeOut;
		psNode->hThread = hThread;
		psNode->nWhich = nWhich;

		nOldFlags = spinlock_disable( &g_sTimerListSpinLock );

		/* Is this the first timer in the list? */
		if ( g_psFirstTimerNode == NULL )
		{
			g_psFirstTimerNode = psNode;
		}
		else
		{
			TimerNode_s *psTmp;

			/* No. Append the timer at the end of the list */
			psTmp = g_psFirstTimerNode;
			while( psTmp->psNext != NULL )
				psTmp = psTmp->psNext;

			psNode->psPrev = psTmp;
			psTmp->psNext = psNode;
		}

		spinunlock_enable( &g_sTimerListSpinLock, nOldFlags );
	}
	return nError;
}

void send_timer_signals( bigtime_t nCurTime )
{
	TimerNode_s *psNode;
	int nOldFlags;
	int i = 0;

	nOldFlags = spinlock_disable( &g_sTimerListSpinLock );

	for( psNode = g_psFirstTimerNode; psNode != NULL; psNode = psNode->psNext )
	{
		Thread_s *psThread;
		int nSignal = 0;

		if( psNode->nWhich == ITIMER_REAL && psNode->nTimeOut < nCurTime )
		{
			nSignal = SIGALRM;
		}
		else
		{
			psThread = get_thread_by_handle( psNode->hThread );
			if( NULL == psThread )
				continue;

			if( psNode->nTimeOut < psThread->tr_nCPUTime )
			{
				if( psNode->nWhich == ITIMER_VIRTUAL )
					nSignal = SIGVTALRM;
				else if( psNode->nWhich == ITIMER_PROF )
					nSignal = SIGTRAP;
			}
		}
		if( nSignal == 0 )
			continue;

		sys_kill( psNode->hThread, nSignal );

		if( psNode->sValue.it_interval.tv_sec > 0 || psNode->sValue.it_interval.tv_usec > 0 )
		{
			/* Re-schedule the timer */
			bigtime_t nTimeOut;

			nTimeOut = nCurTime;
			nTimeOut += (psNode->sValue.it_interval.tv_sec * 1000000);
			nTimeOut += psNode->sValue.it_interval.tv_usec;

			psNode->nTimeOut = nTimeOut;
		}
		else
		{
			/* Remove the timer */
			if( psNode->psNext != NULL )
				psNode->psNext->psPrev = psNode->psPrev;
			if( psNode->psPrev != NULL )
				psNode->psPrev->psNext = psNode->psNext;
			if( g_psFirstTimerNode == psNode )
				g_psFirstTimerNode = NULL;

			kfree( psNode );
		}

		if( i++ > 10000 )
		{
			printk( "send_timer_signals() looped 10000 times, give up\n" );
			break;
		}
	}

	spinunlock_enable( &g_sTimerListSpinLock, nOldFlags );
}

int sys_setitimer( int nWhich, struct kernel_itimerval *psNew, struct kernel_itimerval *psOld )
{
	int nError = 0;

	if( nWhich > ITIMER_PROF )
		nError = -EINVAL;
	else
	{
		TimerNode_s *psOldNode;
		struct kernel_itimerval sValue;

		if( remove_timer( nWhich, &psOldNode ) == EOK )
		{
			if( NULL != psOld )
				memcpy_to_user( psOld, &psOldNode->sValue, sizeof( struct kernel_itimerval ) );
			kfree( psOldNode );
		}

		memcpy_from_user( &sValue, psNew, sizeof( sValue ) );
		if( sValue.it_value.tv_sec > 0 || sValue.it_value.tv_usec > 0 )
			nError = insert_timer( nWhich, &sValue );
	}

	return nError;
}

int sys_getitimer( int nWhich, struct kernel_itimerval *psValue )
{
	TimerNode_s *psNode;
	thread_id hThread = sys_get_thread_id( NULL );
	int nOldFlags;
	int nError = 0;

	if( nWhich > ITIMER_PROF || psValue == NULL )
		nError = -EINVAL;
	else
	{
		nOldFlags = spinlock_disable( &g_sTimerListSpinLock );

		for( psNode = g_psFirstTimerNode; psNode != NULL; psNode = psNode->psNext )
		{
			if( psNode->hThread == hThread && psNode->nWhich == nWhich )
			{
				memcpy_to_user( psValue, &psNode->sValue, sizeof( struct kernel_itimerval ) );
				break;
			}
		}

		spinunlock_enable( &g_sTimerListSpinLock, nOldFlags );
	}

	return nError;
}

unsigned int sys_alarm( const unsigned int nSeconds )
{
	TimerNode_s *psOldNode;
	unsigned int nElapsed = 0;

	if( remove_timer( ITIMER_REAL, &psOldNode ) == EOK )
	{
		bigtime_t nRemaining;

		nRemaining = psOldNode->nTimeOut - get_system_time();
		nElapsed = (int)( nRemaining / 1000000 );

		kfree( psOldNode );
	}

	if( nSeconds > 0 )
	{
		struct kernel_itimerval sValue;
		int nError;

		memset( &sValue, 0, sizeof( sValue ) );

		sValue.it_value.tv_sec = nSeconds;
		nError = insert_timer( ITIMER_REAL, &sValue );
		if( nError != EOK )
			kerndbg( KERN_WARNING, "sys_alarm(): insert_timer() failed (%d).\n", nError );
	}

	return nElapsed;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int send_signal( Thread_s *psThread, int nSigNum, bool bBypassChecks )
{
	Process_s *psProc = CURRENT_PROC;
	Process_s *psDestProc = psThread->tr_psProcess;

	if ( psThread->tr_nState == TS_ZOMBIE )
	{
		return ( -ESRCH );
	}
	// The real or effective user ID of the calling process must match the
	// real or saved user ID of the receiving process, or the caller
	// must be root, or nSigNum is SIGCONT and the calling process has the
	// same session ID as the receiving process.
	if ( !( bBypassChecks ) &&
	     ( psProc->pr_nEUID != psDestProc->pr_nUID ) &&
	     ( psProc->pr_nUID != psDestProc->pr_nUID ) &&
	     ( psProc->pr_nEUID != psDestProc->pr_nSUID ) &&
	     ( psProc->pr_nUID != psDestProc->pr_nSUID ) &&
	    !( nSigNum == SIGCONT && psProc->pr_nSession == psDestProc->pr_nSession ) &&
	    !is_root( false ) )
	{
		return ( -EPERM );
	}
	// Signal 0 performs error checking only.
	if ( nSigNum == 0 )
	{
		return ( 0 );
	}

	// Signals to 'init' will only wake it up. We dont change the signal mask
	if ( 1 != psThread->tr_hThreadID )
	{
		sigaddset( &psThread->tr_sSigPend, nSigNum );

		if ( nSigNum == SIGCONT || nSigNum == SIGKILL )
		{
			do_wakeup_thread( psThread->tr_hThreadID, true );
		}
		else
		{
			if ( psThread->tr_sSigPend.__val[0] & ( ( ~psThread->tr_sSigBlock.__val[0] ) | ( 1L << ( SIGCHLD - 1 ) ) ) ||
			     psThread->tr_sSigPend.__val[1] & ( ~psThread->tr_sSigBlock.__val[1] ) )
			{
				do_wakeup_thread( psThread->tr_hThreadID, false );
			}
		}
	}
	else
	{
		do_wakeup_thread( psThread->tr_hThreadID, false );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_killpg( const pid_t nGrp, const int nSigNum )
{
	thread_id hThread = 0;
	Thread_s *psThread;
	bool bFound = false;
	bool bPermError = false;
	int nFlg = cli();

	if ( nSigNum < 0 || nSigNum > _NSIG )
	{
		return ( -EINVAL );
	}

	sched_lock();
	FOR_EACH_THREAD( hThread, psThread )
	{
		if ( psThread->tr_psProcess->pr_hPGroupID == nGrp )
		{
			if ( send_signal( psThread, nSigNum, false ) == -EPERM )
			{
				bPermError = true;
			}
			bFound = true;
		}
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( ( bFound ) ? ( ( bPermError ) ? -EPERM : 0 ) : -ESRCH );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_kill_proc( proc_id hProcess, int nSigNum )
{
	thread_id hThread = 0;
	Thread_s *psThread;
	bool bFound = false;
	bool bPermError = false;
	int nFlg = cli();

	if ( nSigNum < 0 || nSigNum > _NSIG )
	{
		return ( -EINVAL );
	}

	sched_lock();
	FOR_EACH_THREAD( hThread, psThread )
	{
		if ( psThread->tr_psProcess->tc_hProcID == hProcess )
		{
			if ( send_signal( psThread, nSigNum, false ) == -EPERM )
			{
				bPermError = true;
			}
			bFound = true;
		}
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( ( bFound ) ? ( ( bPermError ) ? -EPERM : 0 ) : -ESRCH );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_kill( const thread_id hThread, const int nSigNum )
{
	int nError = 0;

	if ( nSigNum < 0 || nSigNum > _NSIG )
	{
		return ( -EINVAL );
	}

	if ( hThread == 0 )
	{
		Thread_s *psThread = CURRENT_THREAD;

		nError = sys_killpg( psThread->tr_psProcess->pr_hPGroupID, nSigNum );
	}
	else if ( hThread > 0 )
	{
		int nFlg = cli();
		Thread_s *psThread;

		sched_lock();

		psThread = get_thread_by_handle( hThread );

		if ( NULL != psThread && psThread->tr_nState != TS_ZOMBIE )
		{
			nError = send_signal( psThread, nSigNum, false );
		}
		else
		{
			nError = -ESRCH;
		}
		sched_unlock();
		put_cpu_flags( nFlg );
	}
	else
	{
		nError = sys_killpg( -hThread, nSigNum );
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

/*** OBSOLETE ***/

void ( *sys_signal( const int nSigNum, void ( *const pHandler ) ( int ) ) ) ( int )
{
	struct libc_sigaction sNew;
	struct libc_sigaction sOld;
	int nError;

	memset( &sNew, 0, sizeof( sNew ) );

	sNew.sa_handler = pHandler;
	sNew.sa_flags = SA_ONESHOT | SA_NOMASK;

	nError = sys_sigaction( nSigNum, &sNew, &sOld );

	if ( 0 == nError )
	{
		return ( sOld.sa_handler );
	}
	else
	{
		return ( SIG_ERR );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigaction( int nSigNum, const struct libc_sigaction *__restrict psAction,
		   struct libc_sigaction *__restrict psOldAct )
{
	Thread_s *psThread = CURRENT_THREAD;
	SigAction_s *psHandler;
	struct libc_sigaction sNewHandler;

	if ( nSigNum < 1 || nSigNum > _NSIG )
	{
		return ( -EINVAL );
	}

	psHandler = &psThread->tr_apsSigHandlers[nSigNum - 1];

	if ( SIGKILL == nSigNum || SIGSTOP == nSigNum )
	{
		return ( -EINVAL );
	}

	if ( NULL != psOldAct )
	{
		struct libc_sigaction sOldAction;
		memset( &sOldAction, 0, sizeof( sOldAction ) );
		sOldAction.sa_handler = psHandler->sa_handler;
		sOldAction.sa_mask.__val[0] = psHandler->sa_mask.__val[0];
		sOldAction.sa_mask.__val[1] = psHandler->sa_mask.__val[1];
		sOldAction.sa_flags = psHandler->sa_flags;
		sOldAction.sa_restorer = psHandler->sa_restorer;

		if ( memcpy_to_user( psOldAct, &sOldAction, sizeof( sOldAction) ) < 0 )
		{
			return ( -EFAULT );
		}
	}

	if ( NULL != psAction )
	{
		if ( memcpy_from_user( &sNewHandler, psAction, sizeof( sNewHandler ) ) < 0 )
		{
			return ( -EFAULT );
		}
		psHandler->sa_handler = sNewHandler.sa_handler;
		psHandler->sa_mask.__val[0] = sNewHandler.sa_mask.__val[0];
		psHandler->sa_mask.__val[1] = sNewHandler.sa_mask.__val[1];
		psHandler->sa_flags = sNewHandler.sa_flags;
		psHandler->sa_restorer = sNewHandler.sa_restorer;

		if ( SIG_IGN == psHandler->sa_handler )
		{
			sigdelset( &psThread->tr_sSigPend, nSigNum );
		}
		if ( SIG_DFL == psHandler->sa_handler )
		{
			if ( nSigNum != SIGCONT && nSigNum != SIGCHLD && nSigNum != SIGWINCH )
			{
				return ( 0 );
			}
			sigdelset( &psThread->tr_sSigPend, nSigNum );
			return ( 0 );
		}
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigaddset( sigset_t *const pSet, const int nSigNum )
{
	return sigaddset( pSet, nSigNum );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigdelset( sigset_t *const pSet, const int nSigNum )
{
	return sigdelset( pSet, nSigNum );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigemptyset( sigset_t *const pSet )
{
	return sigemptyset( pSet );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigfillset( sigset_t *const pSet )
{
	return sigfillset( pSet );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigismember( const sigset_t *pSet, int nSigNum )
{
	return sigismember( pSet, nSigNum );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigpending( libc_sigset_t *pSet )
{
	Thread_s *psThread = CURRENT_THREAD;
	sigset_t sSet;
       	sigandset( &sSet, &psThread->tr_sSigPend, &psThread->tr_sSigBlock );

	// Copies sigset_t into user's larger libc_sigset_t structure.
	if ( memcpy_to_user( pSet, &sSet, sizeof( sSet ) ) < 0 )
	{
		return ( -EFAULT );
	}

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigprocmask( int nHow, const libc_sigset_t *__restrict pSet,
		     libc_sigset_t *__restrict pOldSet )
{
	Thread_s *psThread = CURRENT_THREAD;

	sigset_t new_set;
	sigset_t old_set = psThread->tr_sSigBlock;

	if ( NULL != pSet )
	{
		if ( memcpy_from_user( &new_set, pSet, sizeof( new_set ) ) < 0 )
		{
			return ( -EFAULT );
		}
		new_set.__val[0] &= _BLOCKABLE;

		switch ( nHow )
		{
		case SIG_BLOCK:
			sigorset( &psThread->tr_sSigBlock, &psThread->tr_sSigBlock, &new_set );
			break;
		case SIG_UNBLOCK:
			sigandnotset( &psThread->tr_sSigBlock, &psThread->tr_sSigBlock, &new_set );
			break;
		case SIG_SETMASK:
			psThread->tr_sSigBlock = new_set;
			break;
		default:
			return ( -EINVAL );
		}
	}

	if ( NULL != pOldSet )
	{
		// Copies sigset_t into user's larger libc_sigset_t structure.
		if ( memcpy_to_user( pOldSet, &old_set, sizeof( old_set ) ) < 0 )
		{
			return ( -EFAULT );
		}
	}
	return 0;

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigsuspend( unsigned long int nSetHigh, unsigned long int nSetMid,
		    unsigned long int nSetLow )
{
	Thread_s *psThread = CURRENT_THREAD;

	sigset_t old_set = psThread->tr_sSigBlock;

/*
    sigemptyset( &new_set );

    if ( pSet != NULL ) {
	if ( memcpy_from_user( &new_set, pSet, sizeof(new_set) ) < 0 ) {
	    return( -EFAULT );
	}
    } else {
	printk( "Warning: sys_sigsuspend() called with pSet == NULL\n" );
    }
    */
	psThread->tr_sSigBlock.__val[0] = nSetLow & _BLOCKABLE;
	psThread->tr_sSigBlock.__val[1] = nSetMid;
	
	suspend();

	psThread->tr_sSigBlock = old_set;

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigaltstack( const stack_t *__restrict ss, stack_t *__restrict oss )
{
	printk( "sys_sigaltstack( %p, %p ) stub called\n", ss, oss );
	return ( -ENOSYS );
}
