/*
 *  The Syllable kernel
 *  Copyright (C) 2004 Kristian Van Der Vliet
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
#include <atheos/areas.h>
#include <atheos/irq.h>
#include <atheos/ptrace.h>
#include <posix/errno.h>
#include <posix/signal.h>
#include <macros.h>

#include "inc/intel.h"
#include "inc/smp.h"
#include "inc/areas.h"
#include "inc/ptrace.h"


static ptrace_func_t ptrace_traceme;
static ptrace_func_t ptrace_peekmem;
static ptrace_func_t ptrace_peekuser;
static ptrace_func_t ptrace_pokemem;
static ptrace_func_t ptrace_pokeuser;
static ptrace_func_t ptrace_cont;
static ptrace_func_t ptrace_kill;
static ptrace_func_t ptrace_singlestep;
static ptrace_func_t ptrace_getregs;
static ptrace_func_t ptrace_setregs;
static ptrace_func_t ptrace_getfpregs;
static ptrace_func_t ptrace_setfpregs;
static ptrace_func_t ptrace_attach;
static ptrace_func_t ptrace_detach;
static ptrace_func_t ptrace_getfpxregs;
static ptrace_func_t ptrace_setfpxregs;

static const ptrace_func_ptr g_pPTraceFunc[MAX_REQUEST + 1] =
{
	ptrace_traceme,
	ptrace_peekmem,		/* PTRACE_PEEKTEXT */
	ptrace_peekmem,		/* PTRACE_PEEKDATA */
	ptrace_peekuser,
	ptrace_pokemem,		/* PTRACE_POKETEXT */
	ptrace_pokemem,		/* PTRACE_POKEDATA */
	ptrace_pokeuser,
	ptrace_cont,
	ptrace_kill,
	ptrace_singlestep,
	NULL,				/* -               */
	NULL,				/* -               */
	ptrace_getregs,
	ptrace_setregs,
	ptrace_getfpregs,
	ptrace_setfpregs,
	ptrace_attach,
	ptrace_detach,
	ptrace_getfpxregs,
	ptrace_setfpxregs,
	NULL,				/* -               */
	NULL,				/* -               */
	NULL,				/* -               */
	NULL,				/* -               */
	NULL				/* PTRACE_SYSCALL  */
};


static int continue_with_signal( Thread_s *psThread, uint32 nSigNum )
{
	if ( nSigNum > _NSIG )
		return -EIO;

	if ( ( nSigNum != 0 ) && ( nSigNum != SIGSTOP ) )
	{
		atomic_or( &psThread->tr_nPTraceFlags, PT_ALLOW_SIGNAL );
		sigaddset( &psThread->tr_sSigPend, nSigNum );
	}

	wakeup_thread( psThread->tr_hThreadID, true );
	return 0;
}

static void untrace( Thread_s *psThread )
{
	psThread->tr_hParent = psThread->tr_hRealParent;
	psThread->tr_hRealParent = 0;
	psThread->tr_nDebugReg[7] = 0;
	psThread->tr_psPTraceRegs->eflags &= ~EFLG_TRAP;

	if ( psThread->tr_hPTraceVMClone != 0 )
	{
		delete_area( psThread->tr_hPTraceVMClone );
		psThread->tr_hPTraceVMClone = 0;
	}
	psThread->tr_nPTraceVMCloneBase = 0;

	atomic_set( &psThread->tr_nPTraceFlags, 0 );
}

static status_t access_vm( Thread_s *psThread, uintptr_t nAddr, uint32 *pnValue,
                           bool bWrite )
{
	uintptr_t nPageBase, nPageOffset;
	size_t nSize;
	MemArea_s *psSrcArea;
	area_id hCloneArea;
	AreaInfo_s sInfo;
	uint32 *pnLocation;
	int nResult = 0;

	psSrcArea = get_area( psThread->tr_psProcess->tc_psMemSeg, nAddr );
	if ( !psSrcArea )
		return -EINVAL;

	nPageBase = nAddr & PAGE_MASK;
	nPageOffset = nAddr & ~PAGE_MASK;

	if ( psThread->tr_nPTraceVMCloneBase != nPageBase )
	{
		/* map two pages if the address crosses a page boundary */
		if ( ( nPageOffset + 3 ) < PAGE_SIZE )
			nSize = PAGE_SIZE;
		else
			nSize = 2 * PAGE_SIZE;

		hCloneArea = clone_from_inactive_ctx( psSrcArea,
			nPageBase - psSrcArea->a_nStart, nSize );
		if ( hCloneArea <= 0 )
		{
			printk( "access_vm: error (%d) cloning area\n", hCloneArea );
			put_area( psSrcArea );
			return -EINVAL;
		}

		psThread->tr_nPTraceVMCloneBase = nPageBase;
		psThread->tr_hPTraceVMClone = hCloneArea;
	}
	else
		hCloneArea = psThread->tr_hPTraceVMClone;

	kassertw( get_area_info( hCloneArea, &sInfo ) == 0 );
	pnLocation = (void *) ( sInfo.pAddress + nPageOffset );

	if ( bWrite )
	{
		*pnLocation = *pnValue;
		nResult = update_inactive_ctx( psSrcArea, hCloneArea,
		                               nPageBase - psSrcArea->a_nStart );
	}
	else
		*pnValue = *pnLocation;

	put_area( psSrcArea );
	return nResult;
}


static int ptrace_traceme( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                           void *pAddr, void *pData )
{
	if ( atomic_read( &psCurrentThread->tr_nPTraceFlags ) & PT_PTRACED )
		return -EPERM;

	psCurrentThread->tr_hRealParent = psCurrentThread->tr_hParent;
	atomic_or( &psCurrentThread->tr_nPTraceFlags, PT_PTRACED );
	return 0;
}

static int ptrace_peekmem( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                           void *pAddr, void *pData )
{
	uint32 nValue;
	int nResult;

	nResult = access_vm( psTargetThread, (uintptr_t) pAddr, &nValue, false );
	if ( nResult == 0 )
		nResult = memcpy_to_user( pData, &nValue, 4 );

	return nResult;
}

static int ptrace_peekuser( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                            void *pAddr, void *pData )
{
	uintptr_t nOffset = (uintptr_t) pAddr;
	int nResult = -EIO;

	if ( ( nOffset & 3 ) != 0 )
		return -EINVAL;

	if ( nOffset < sizeof( struct user ) )
	{
		if ( nOffset < offsetof(struct user, u_debugreg) )
		{
			/* read register */
			nOffset += (uintptr_t) psTargetThread->tr_psPTraceRegs;
			nResult = memcpy_to_user( pData, (void *) nOffset, 4 );
		}
		else
		{
			/* read debug register */
			int nDReg = ( nOffset - offsetof(struct user, u_debugreg) ) / 4;
			printk( "reading debug register %d\n", nDReg );
			nResult = memcpy_to_user( pData,
				(void *) &psTargetThread->tr_nDebugReg[nDReg], 4 );
		}
	}

	return nResult;
}

static int ptrace_pokemem( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                           void *pAddr, void *pData )
{
	return access_vm( psTargetThread, (uintptr_t) pAddr, (uint32 *) &pData,
	                  true );
}

static int ptrace_pokeuser( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                            void *pAddr, void *pData )
{
	uintptr_t nOffset = (uintptr_t) pAddr;
	uint32 nValue = (uint32) pData;

	if ( ( nOffset & 3 ) != 0 )
		return -EINVAL;

	if ( nOffset < sizeof( struct user ) )
	{
		if ( nOffset < offsetof(struct user, u_debugreg) )
		{
			/* write register */
			void *pDst = ( (void *) psTargetThread->tr_psPTraceRegs ) + nOffset;

			switch ( nOffset )
			{
				case DS * 4 :
				case ES * 4 :
				case FS * 4 :
				case GS * 4 :
				case CS * 4 :
				case SS * 4 :
					*( (uint16 *) pDst ) = (uint16) ( nValue & 0xffff );
					break;
				case EFL * 4 :
					nValue &= EFLAG_MASK;
					nValue |= psTargetThread->tr_psPTraceRegs->eflags &
						~EFLAG_MASK;
					/* fall through */
				default :
					*( (uint32 *) pDst ) = nValue;
			}
		}
		else
		{
			/* write debug register */
			int nDReg = ( nOffset - offsetof(struct user, u_debugreg) ) / 4;
			printk( "writing 0x%X to debug register %d\n", nValue, nDReg );

			if ( nDReg == 7 )
				nValue &= DR7_MASK;
			else if ( nDReg < 4 )
			{
				if ( ( nValue != 0 ) && ( nValue < AREA_FIRST_USER_ADDRESS ) )
					return -EIO;
			}

			psTargetThread->tr_nDebugReg[nDReg] = nValue;
		}
	}

	return 0;
}

static int ptrace_cont( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                        void *pAddr, void *pData )
{
	psTargetThread->tr_psPTraceRegs->eflags &= ~EFLG_TRAP;
	return continue_with_signal( psTargetThread, (uint32) pData );
}

static int ptrace_kill( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                        void *pAddr, void *pData )
{
	if ( psTargetThread->tr_nState == TS_STOPPED )
		return ptrace_detach( NULL, psTargetThread, NULL, (void *) SIGKILL );
	else
	{
		untrace( psTargetThread );
		return send_signal( psTargetThread, SIGKILL, true );
	}
}

static int ptrace_singlestep( Thread_s *psCurrentThread,
                              Thread_s *psTargetThread,
                              void *pAddr, void *pData )
{
	psTargetThread->tr_psPTraceRegs->eflags |= EFLG_TRAP;
	return continue_with_signal( psTargetThread, (uint32) pData );	
}

static int ptrace_getregs( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                           void *pAddr, void *pData )
{
	return memcpy_to_user( pData, (void *) psTargetThread->tr_psPTraceRegs,
	                       sizeof(SysCallRegs_s) );
}

static int ptrace_setregs( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                           void *pAddr, void *pData )
{
	SysCallRegs_s sRegs;

	if ( memcpy_from_user( &sRegs, pData, sizeof(SysCallRegs_s) ) < 0 )
		return -EFAULT;

	sRegs.eflags &= EFLAG_MASK;
	sRegs.eflags |= psTargetThread->tr_psPTraceRegs->eflags & ~EFLAG_MASK;

	memcpy( (void *) psTargetThread->tr_psPTraceRegs, &sRegs,
	        sizeof(SysCallRegs_s) );
	return 0;
}

static int ptrace_getfpregs( Thread_s *psCurrentThread,
                             Thread_s *psTargetThread,
                             void *pAddr, void *pData )
{
	return memcpy_to_user( pData, &psTargetThread->tc_FPUState.fpu_sFSave,
	                       sizeof( struct i3FSave_t ) - 4 );
}

static int ptrace_setfpregs( Thread_s *psCurrentThread,
                             Thread_s *psTargetThread,
                             void *pAddr, void *pData )
{
	int nResult;

	nResult = memcpy_from_user( &psTargetThread->tc_FPUState.fpu_sFSave, pData,
	                            sizeof( struct i3FSave_t ) - 4 );
	psTargetThread->tc_FPUState.fpu_sFSave.status =
		psTargetThread->tc_FPUState.fpu_sFSave.swd;

	return nResult;
}

static int ptrace_attach( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                          void *pAddr, void *pData )
{
	Process_s *psCurrentProcess, *psTargetProcess;

	if ( atomic_read( &psTargetThread->tr_nPTraceFlags ) & PT_PTRACED )
		return -EPERM;
	if ( psCurrentThread->tr_hThreadID == psTargetThread->tr_hThreadID )
		return -EPERM;

	psCurrentProcess = psCurrentThread->tr_psProcess;
	psTargetProcess = psTargetThread->tr_psProcess;

	if ( ( psCurrentProcess->pr_nUID != psTargetProcess->pr_nEUID ) ||
	     ( psCurrentProcess->pr_nUID != psTargetProcess->pr_nSUID ) ||
	     ( psCurrentProcess->pr_nUID != psTargetProcess->pr_nUID  ) ||
	     ( psCurrentProcess->pr_nGID != psTargetProcess->pr_nEGID ) ||
	     ( psCurrentProcess->pr_nGID != psTargetProcess->pr_nSGID ) ||
	     ( psCurrentProcess->pr_nGID != psTargetProcess->pr_nGID  ) )
		return -EPERM;

	psTargetThread->tr_hRealParent = psTargetThread->tr_hParent;
	psTargetThread->tr_hParent = psCurrentThread->tr_hThreadID;
	atomic_or( &psTargetThread->tr_nPTraceFlags, PT_PTRACED );

	send_signal( psTargetThread, SIGSTOP, true );
	return 0;
}

static int ptrace_detach( Thread_s *psCurrentThread, Thread_s *psTargetThread,
                          void *pAddr, void *pData )
{
	untrace( psTargetThread );
	return ptrace_cont( NULL, psTargetThread, NULL, pData );
}

static int ptrace_getfpxregs( Thread_s *psCurrentThread,
                              Thread_s *psTargetThread,
                              void *pAddr, void *pData )
{
	int nResult = -EIO;

	if ( g_asProcessorDescs[g_nBootCPU].pi_bHaveFXSR )
		nResult = memcpy_to_user( pData,
		                          &psTargetThread->tc_FPUState.fpu_sFXSave,
		                          sizeof( struct i3FXSave_t ) );

	return nResult;
}

static int ptrace_setfpxregs( Thread_s *psCurrentThread,
                              Thread_s *psTargetThread,
                              void *pAddr, void *pData )
{
	int nResult = -EIO;

	if ( g_asProcessorDescs[g_nBootCPU].pi_bHaveFXSR )
	{
		nResult = memcpy_from_user( &psTargetThread->tc_FPUState.fpu_sFXSave,
		                            pData, sizeof( struct i3FXSave_t ) );

		/* mask out reserved bits that might trigger a GPF */
		psTargetThread->tc_FPUState.fpu_sFXSave.mxcsr &= 0x0000ffbf;
	}

	return nResult;
}


int sys_ptrace( int nRequest, thread_id hThread, void *pAddr, void *pData )
{
	Thread_s *psTargetThread;
	Thread_s *psCurrentThread = CURRENT_THREAD;
	ptrace_func_ptr f = NULL;
	int nCpuFlags, nResult = 0;

	printk( "sys_ptrace: request = %d, tid = %d, addr = %p, data = %p\n",
	        nRequest, hThread, pAddr, pData );

	if ( ( nRequest >= 0 ) && ( nRequest <= MAX_REQUEST ) )
		f = g_pPTraceFunc[nRequest];

	if ( f == NULL )
	{
		printk( "sys_ptrace: invalid request %d\n", nRequest );
		return -EINVAL;
	}


	nCpuFlags = cli();
	sched_lock();

	if ( nRequest == PTRACE_TRACEME )
	{
		nResult = f( psCurrentThread, NULL, pAddr, pData );
		goto out;
	}

	psTargetThread = get_thread_by_handle( hThread );
	if ( !psTargetThread )
	{
		nResult = -ESRCH;
		goto out;
	}

	if ( psTargetThread->tr_psProcess->tc_hProcID <= 1 )
	{
		nResult = -EPERM;	/* don't trace kernel or init */
		goto out;
	}

	if ( nRequest == PTRACE_ATTACH )
	{
		nResult = f( psCurrentThread, psTargetThread, pAddr, pData );
		goto out;
	}

	if ( !( atomic_read( &psTargetThread->tr_nPTraceFlags ) & PT_PTRACED ) ||
	     !( psTargetThread->tr_hParent == psCurrentThread->tr_hThreadID ) )
	{
		nResult = -EPERM;
		goto out;
	}

	if ( nRequest == PTRACE_KILL )
	{
		nResult = f( psCurrentThread, psTargetThread, pAddr, pData );
		goto out;
	}

	/* all other functions require the target thread to be stopped */
	if ( psTargetThread->tr_nState == TS_STOPPED )
		nResult = f( psCurrentThread, psTargetThread, pAddr, pData );
	else
		nResult = -ESRCH;

out:
	sched_unlock();
	put_cpu_flags( nCpuFlags );

	printk( "sys_ptrace: result = %d\n", nResult );
	return nResult;
}

