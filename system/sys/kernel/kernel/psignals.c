
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
#include "inc/sysbase.h"

#define _BLOCKABLE ( ~( (1L << (SIGKILL - 1)) | (1L << (SIGSTOP - 1)) ) )


typedef struct _AlarmNode AlarmNode_s;


struct _AlarmNode
{
	AlarmNode_s *psNext;
	AlarmNode_s *psPrev;
	uint64 nTimeOut;
	thread_id hThread;
};

static AlarmNode_s *g_psFirstAlarmNode = NULL;
static AlarmNode_s *g_psFirstFreeAlarmNode = NULL;

//static spinlock_t g_nAlarmListSpinLock = 0;
SPIN_LOCK( g_sAlarmListSpinLock, "alarm_list_slock" );

extern void save_i387( struct _fpstate *buf );
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

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void ExitSyscall( int dummy )
{
	Thread_s *psThread = CURRENT_THREAD;
	SysCallRegs_s *psRegs = ( SysCallRegs_s * ) & dummy;

	if ( psRegs->cs == 0x13 && ( psRegs->eflags & EFLG_IF ) == 0 && psRegs->orig_eax >= 0 )
	{
		printk( "PANIC : return from syscall whith interrupts disabled!! I turn them on again.\n" );
		psRegs->eflags |= EFLG_IF;
		psThread->tr_nSysTraceMask = SYSC_GROUP_ALL;	/* Temporary strace (this function only) */
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int is_signal_pending( void )
{
	Thread_s *psThread = CURRENT_THREAD;

	return ( 0 != ( psThread->tr_nSigPend & ~psThread->tr_nSigBlock ) );
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

	if ( nSigNum < 1 || nSigNum >= NSIG )
	{
		return ( -EINVAL );
	}

	if ( psThread->tr_nSigBlock & ( 1L << ( nSigNum - 1 ) ) )
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

	psThread->tr_nSigBlock = sContext.oldmask & _BLOCKABLE;

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

void AddSigHandlerFrame( Thread_s *psThread, SysCallRegs_s * psRegs, SigAction_s * psHandler, int nSigNum, sigset_t nOldBlockMask )
{
	uint32 *pStack = ( uint32 * )psRegs->oldesp;
	uint32 *pCode;
	uint32 *pFpuState;
	uint32 *pStackTop;	// top of stack after parameters pushed

	pStack -= (22 + 2 + 156);    // SigContext_s + trampoline + FPUState_s
	pCode = pStack + 22;
	pFpuState = pStack + 24;
	pStackTop = pStack - 2;		// subtract 4 for SA_SIGINFO

	pStackTop[0] = ( uint32 )pCode;	// return trampoline address
	pStackTop[1] = nSigNum;
	// pStackTop[2] = siginfo_t;
	// pStackTop[3] = ucontext_t;

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
	pStack[20] = nOldBlockMask;
	pStack[21] = CURRENT_THREAD->tr_nCR2;

	pCode[0] = 0x0000b858;	/* popl %eax ; movl $,%eax */
	pCode[1] = 0x80cd0000;	/* int $0x80 */
	( ( uint32 * )( ( ( uint32 )pCode ) + 2 ) )[0] = __NR_sig_return;

	save_i387( (struct _fpstate * )pFpuState );

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

static void handle_signal( int nSigNum, SigAction_s * psHandler, SysCallRegs_s * psRegs, uint32 nOldBlockMask )
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

/*				printk( "handle_signal() : Restart syscall %d after sig %d\n", psRegs->orig_eax, nSigNum + 1 ); */
			psRegs->eax = psRegs->orig_eax;
			psRegs->eip -= 2;

			kassertw( 0x80cd == *( ( uint16 * )psRegs->eip ) );	/* int 0x80 */
			break;
		}
	}

	AddSigHandlerFrame( psThread, psRegs, psHandler, nSigNum + 1, nOldBlockMask );

	if ( psHandler->sa_flags & SA_ONESHOT )
	{
		psHandler->sa_handler = SIG_DFL;
	}

	if ( !( psHandler->sa_flags & SA_NOMASK ) )
	{
		psThread->tr_nSigBlock |= ( psHandler->sa_mask | ( 1L << nSigNum ) ) & _BLOCKABLE;
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int handle_signals( int dummy )
{
	int nSigCount = 0;
	SysCallRegs_s *psRegs = ( SysCallRegs_s * ) & dummy;
	Thread_s *psThread = CURRENT_THREAD;
	Process_s *psProc = psThread->tr_psProcess;
	sigset_t nOldBlockMask = psThread->tr_nSigBlock;

	if ( psRegs->eflags & EFLG_VM )
	{
		return ( -EBUSY );
	}

	if ( NULL == psThread )
	{
		return ( -EINVAL );
	}
	if ( 0 == psThread->tr_hThreadID )
	{
		return ( -EINVAL );	/* Dont do anything silly with the idle thread */
	}
	if ( psThread->tr_nSigPend & ( ~nOldBlockMask ) )
	{
		uint32 nSigMask = psThread->tr_nSigPend & ( ~nOldBlockMask );
		int i;

/*				psThread->tr_nSigPend &= ~nSigMask; */
		for ( i = 0; i < NSIG; ++i )
		{
			if ( nSigMask & 0x01 )
			{
				int nSigNum = i + 1;
				SigAction_s *psHandler = &psThread->tr_apsSigHandlers[i];

				nSigMask >>= 1;

				psThread->tr_nSigPend &= ~( 1L << i );

				if ( i == ( SIGCHLD - 1 ) )
				{

/*								printk( "Got SIGCHLD\n" ); */
				}

				if ( SIG_IGN == psHandler->sa_handler )
				{
					if ( i != ( SIGCHLD - 1 ) )
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
				handle_signal( i, psHandler, psRegs, nOldBlockMask );
				return ( 1 );
				nSigCount++;
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

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void send_alarm_signals( bigtime_t nCurTime )
{
	int nOldFlags;
	int i = 0;

	nOldFlags = spinlock_disable( &g_sAlarmListSpinLock );
	while ( g_psFirstAlarmNode != NULL && g_psFirstAlarmNode->nTimeOut <= nCurTime )
	{
		AlarmNode_s *psNode = g_psFirstAlarmNode;

		g_psFirstAlarmNode = psNode->psNext;

		if ( g_psFirstAlarmNode != NULL )
		{
			g_psFirstAlarmNode->psPrev = NULL;
		}
		psNode->psNext = g_psFirstFreeAlarmNode;
		g_psFirstFreeAlarmNode = psNode;

		sys_kill( psNode->hThread, SIGALRM );

		if ( i++ > 10000 )
		{
			printk( "send_alarm_signals() looped 10000 times, give up\n" );
			break;
		}
	}

	spinunlock_enable( &g_sAlarmListSpinLock, nOldFlags );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

unsigned int sys_alarm( const unsigned int nSeconds )
{
	AlarmNode_s *psNode = NULL;
	AlarmNode_s **ppsTmp;
	AlarmNode_s *psTmp;
	thread_id hThread = sys_get_thread_id( NULL );
	int nOldFlags;
	int nPrevTime = 0;
	bigtime_t nCurTime = get_system_time();

	nOldFlags = spinlock_disable( &g_sAlarmListSpinLock );

	for ( ppsTmp = &g_psFirstAlarmNode; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->psNext )
	{
		if ( ( *ppsTmp )->hThread == hThread )
		{
			psNode = *ppsTmp;

			if ( psNode->psNext != NULL )
			{
				psNode->psNext->psPrev = psNode->psPrev;
			}
			*ppsTmp = psNode->psNext;

			nPrevTime = ( psNode->nTimeOut - nCurTime ) / 1000000;
			if ( nPrevTime < 0 )
			{
				nPrevTime = 0;
			}
			break;
		}
	}

	if ( nSeconds == 0 )
	{
		if ( psNode != NULL )
		{
			psNode->psNext = g_psFirstFreeAlarmNode;
			g_psFirstFreeAlarmNode = psNode;
		}
	}
	else
	{
		if ( psNode == NULL && g_psFirstFreeAlarmNode != NULL )
		{
			psNode = g_psFirstFreeAlarmNode;
			g_psFirstFreeAlarmNode = psNode->psNext;
		}
	}
	spinunlock_enable( &g_sAlarmListSpinLock, nOldFlags );

	if ( nSeconds == 0 )
	{
		return ( nPrevTime );
	}

	if ( NULL == psNode )
	{
		psNode = kmalloc( sizeof( AlarmNode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_LOCKED );
	}

	if ( NULL != psNode )
	{
		psNode->nTimeOut = nCurTime + ( ( uint64 )nSeconds ) * 1000000;
		psNode->hThread = sys_get_thread_id( NULL );


		nOldFlags = spinlock_disable( &g_sAlarmListSpinLock );

		if ( g_psFirstAlarmNode == NULL || psNode->nTimeOut <= g_psFirstAlarmNode->nTimeOut )
		{
			psNode->psPrev = NULL;
			psNode->psNext = g_psFirstAlarmNode;
			if ( NULL != g_psFirstAlarmNode )
			{
				g_psFirstAlarmNode->psPrev = psNode;
			}
			g_psFirstAlarmNode = psNode;
			goto done;
		}

		for ( psTmp = g_psFirstAlarmNode;; psTmp = psTmp->psNext )
		{
			if ( psNode->nTimeOut < psTmp->nTimeOut )
			{
				psNode->psPrev = psTmp->psPrev;
				psNode->psNext = psTmp;

				if ( psTmp->psPrev != NULL )
				{
					psTmp->psPrev->psNext = psNode;
				}
				psTmp->psPrev = psNode;
				goto done;
			}
			if ( psTmp->psNext == NULL )
			{
				break;
			}
		}

	  /*** Add to end of list ***/
		psNode->psNext = NULL;
		psNode->psPrev = psTmp;
		psTmp->psNext = psNode;
	      done:



/*
    
  psNode->psNext     = g_psFirstAlarmNode;
  g_psFirstAlarmNode = psNode;
  */
		spinunlock_enable( &g_sAlarmListSpinLock, nOldFlags );

		return ( 0 );
	}
	return ( -ENOMEM );
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
		psThread->tr_nSigPend |= 1L << ( nSigNum - 1 );


		if ( nSigNum == SIGCONT || nSigNum == SIGKILL )
		{
			wakeup_thread( psThread->tr_hThreadID, true );
		}
		else
		{
			if ( psThread->tr_nSigPend & ( ( ~psThread->tr_nSigBlock ) | ( 1L << ( SIGCHLD - 1 ) ) ) )
			{
				wakeup_thread( psThread->tr_hThreadID, false );
			}
		}
	}
	else
	{
		wakeup_thread( psThread->tr_hThreadID, false );
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
	bool bFound = FALSE;
	bool bPermError = FALSE;
	int nFlg = cli();

	if ( nSigNum < 0 || nSigNum > NSIG )
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
				bPermError = TRUE;
			}
			bFound = TRUE;
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
	bool bFound = FALSE;
	bool bPermError = FALSE;
	int nFlg = cli();

	if ( nSigNum < 0 || nSigNum > NSIG )
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
				bPermError = TRUE;
			}
			bFound = TRUE;
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

	if ( nSigNum < 0 || nSigNum > NSIG )
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
	struct sigaction sNew;
	struct sigaction sOld;
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

int sys_sigaction( const int nSigNum, const struct sigaction *const psAction, struct sigaction *const psOldAct )
{
	Thread_s *psThread = CURRENT_THREAD;
	SigAction_s *psHandler;
	SigAction_s sNewHandler;

	if ( nSigNum < 1 || nSigNum > NSIG )
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
		if ( memcpy_to_user( psOldAct, psHandler, sizeof( *psHandler ) ) < 0 )
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
		*psHandler = sNewHandler;

		if ( SIG_IGN == psHandler->sa_handler )
		{
			psThread->tr_nSigPend &= ~( 1L << ( nSigNum - 1 ) );
		}
		if ( SIG_DFL == psHandler->sa_handler )
		{
			if ( nSigNum != SIGCONT && nSigNum != SIGCHLD && nSigNum != SIGWINCH )
			{
				return ( 0 );
			}
			psThread->tr_nSigPend &= ~( 1L << ( nSigNum - 1 ) );
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
	*pSet |= 1L << ( nSigNum - 1 );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigdelset( sigset_t *const pSet, const int nSigNum )
{
	*pSet &= ~( 1L << ( nSigNum - 1 ) );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigemptyset( sigset_t *const pSet )
{
	*pSet = 0;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigfillset( sigset_t *const pSet )
{
	*pSet = ~0;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_sigismember( const sigset_t *const pSet, const int nSigNum )
{
	if ( *pSet & ( 1L << ( nSigNum - 1 ) ) )
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

int sys_sigpending( sigset_t *const pSet )
{
	Thread_s *psThread = CURRENT_THREAD;
	sigset_t nSet = psThread->tr_nSigPend & psThread->tr_nSigBlock;

	if ( memcpy_to_user( pSet, &nSet, sizeof( nSet ) ) < 0 )
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

int sys_sigprocmask( const int nHow, const sigset_t *const pSet, sigset_t *const pOldSet )
{
	Thread_s *psThread = CURRENT_THREAD;

	sigset_t new_set;
	sigset_t old_set = psThread->tr_nSigBlock;

	if ( NULL != pSet )
	{
		if ( memcpy_from_user( &new_set, pSet, sizeof( new_set ) ) < 0 )
		{
			return ( -EFAULT );
		}
		new_set &= _BLOCKABLE;

		switch ( nHow )
		{
		case SIG_BLOCK:
			psThread->tr_nSigBlock |= new_set;
			break;
		case SIG_UNBLOCK:
			psThread->tr_nSigBlock &= ~new_set;
			break;
		case SIG_SETMASK:
			psThread->tr_nSigBlock = new_set;
			break;
		default:
			return ( -EINVAL );
		}
	}

	if ( NULL != pOldSet )
	{
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

//int sys_sigsuspend( const sigset_t* const pSet )
int sys_sigsuspend( uint32 nSetHigh, uint32 nSetMid, sigset_t nSet )
{
	Thread_s *psThread = CURRENT_THREAD;

//    sigset_t new_set;
	sigset_t old_set = psThread->tr_nSigBlock;

/*
    sys_sigemptyset( &new_set );

    if ( pSet != NULL ) {
	if ( memcpy_from_user( &new_set, pSet, sizeof(new_set) ) < 0 ) {
	    return( -EFAULT );
	}
    } else {
	printk( "Warning: sys_sigsuspend() called with pSet == NULL\n" );
    }
    */
	psThread->tr_nSigBlock = nSet & _BLOCKABLE;

	suspend();

	psThread->tr_nSigBlock = old_set;

	return ( 0 );
}
