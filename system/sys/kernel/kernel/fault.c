
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
#include <atheos/irq.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/areas.h"
#include "inc/sysbase.h"
#include "inc/mman.h"
#include "inc/smp.h"
#include "inc/ptrace.h"


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void format_eflags( uint32 nFlags, char *pzBuffer )
{
	pzBuffer[0] = '\0';

	if ( nFlags & EFLG_CARRY )
		strcat( pzBuffer, "CF " );
	if ( nFlags & EFLG_PARITY )
		strcat( pzBuffer, "PF " );
	if ( nFlags & EFLG_AUX_CARRY )
		strcat( pzBuffer, "AF " );
	if ( nFlags & EFLG_ZERO )
		strcat( pzBuffer, "ZF " );
	if ( nFlags & EFLG_SIGN )
		strcat( pzBuffer, "SF " );
	if ( nFlags & EFLG_TRAP )
		strcat( pzBuffer, "TF " );
	if ( nFlags & EFLG_IF )
		strcat( pzBuffer, "IF " );
	if ( nFlags & EFLG_DF )
		strcat( pzBuffer, "DF " );
	if ( nFlags & EFLG_OF )
		strcat( pzBuffer, "OF " );
	if ( nFlags & EFLG_NT )
		strcat( pzBuffer, "NT " );
	if ( nFlags & EFLG_RESUME )
		strcat( pzBuffer, "RF " );
	if ( nFlags & EFLG_VM )
		strcat( pzBuffer, "VM " );
	if ( nFlags & EFLG_AC )
		strcat( pzBuffer, "AC " );
	if ( nFlags & EFLG_VIF )
		strcat( pzBuffer, "VIF " );
	if ( nFlags & EFLG_VIP )
		strcat( pzBuffer, "VIP " );
	if ( nFlags & EFLG_ID )
		strcat( pzBuffer, "ID " );

/*	if ( nFlags & EFLG_IOPL )				strcat( pzBuffer, " " ); */

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void print_symbol( int nIndex, uint32 nAddress )
{
	int nModule;
	char zSymName[256];
	char zModuleName[64];
	void *pSymAddr = NULL;
	uint32 nTxtAddr = 0;

	printk( "%d -> %08x\n", nIndex, nAddress );

	nModule = find_module_by_address( ( void * )nAddress );
	if ( nModule >= 0 )
	{
		image_info sInfo;

		if ( get_image_info( nAddress < AREA_FIRST_USER_ADDRESS, nModule, -1, &sInfo ) >= 0 )
		{
			strncpy( zModuleName, sInfo.ii_name, 64 );
			zModuleName[63] = '\0';
			nTxtAddr = ( uint32 )sInfo.ii_text_addr;
		}
		else
		{			
			strcpy( zModuleName, "*unknown*" );
		}
		if ( get_symbol_by_address( nModule, ( const char * )nAddress, zSymName, 256, &pSymAddr ) < 0 )
		{
			strcpy( zSymName, "*unknown*" );
		}
	}
	else
	{
		strcpy( zModuleName, "*unknown*" );
		strcpy( zSymName, "*unknown*" );
	}
	
	printk( "   %s + %08x -> %s + %08x\n", zModuleName, nAddress - nTxtAddr, zSymName, nAddress - ( uint32 )pSymAddr );
}

void trace_stack( uint32 nEIP, uint32 *pStack )
{
	int i;

	if ( pStack == NULL )
	{
	      __asm__( "movl %%ebp,%0":"=r"( pStack ) );
	}
	else
	{
		print_symbol( 0, nEIP );
	}
	for ( i = 0; i < 100; ++i )
	{
		MemArea_s *psArea;

		if ( ( ( uint32 )pStack ) >= 1024 * 1024 && ( ( uint32 )pStack + 8 ) < ( g_sSysBase.ex_nTotalPageCount * PAGE_SIZE ) )
		{
			psArea = NULL;
		}
		else
		{
			psArea = verify_area( pStack, 8, false );
			if ( psArea == NULL )
			{
				return;
			}
		}
		print_symbol( i + 1, pStack[1] );
		pStack = ( uint32 * )pStack[0];
		if ( psArea != NULL )
		{
			put_area( psArea );
		}
	}
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void print_registers( SysCallRegs_s * psRegs )
{
	char zBuffer[128];
	int nFlg = cli();

	format_eflags( psRegs->eflags, zBuffer );

	printk( "EAX = %08lx : EBX = %08lx : ECX = %08lx : EDX = %08lx\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	printk( "ESI = %08lx : EDI = %08lx : EBP = %08lx\n", psRegs->esi, psRegs->edi, psRegs->ebp );
	printk( "SS::ESP = %04x::%08lx\n", psRegs->oldss, psRegs->oldesp );
	printk( "CS::EIP = %04x::%08lx\n", psRegs->cs, psRegs->eip );
	printk( "DS = %04x : ES = %04x : FS = %04x : GS = %04x\n", psRegs->ds, psRegs->es, psRegs->fs, psRegs->gs );
	printk( "EFLAGS = %08lx (%s)\n", psRegs->eflags, zBuffer );
	printk( "CPU ID = %d : kernel stack = %08x\n", get_processor_id(), ( uint32 )CURRENT_THREAD->tc_plKStack );

	trace_stack( psRegs->eip, ( uint32 * )psRegs->ebp );

	put_cpu_flags( nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_general_protection( SysCallRegs_s * psRegs, int nErrorCode )
{
	Thread_s *psThread = CURRENT_THREAD;
	if ( psRegs->eflags & EFLG_VM )
	{
		handle_v86_fault( ( Virtual86Regs_s * ) psRegs, nErrorCode );
		return;
	}

	printk( "**general protection fault**\n" );
	printk( "ERROR CODE = %08x\n", nErrorCode );
	print_registers( psRegs );
	printk( "\n" );
	
	printk( "Areas :\n" );
	list_areas( CURRENT_PROC->tc_psMemSeg );

	send_signal( psThread, SIGSEGV, true );

//	do_exit( 12 << 8 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_divide_exception( SysCallRegs_s * psRegs, int nErrorCode )
{
	Thread_s *psThread = CURRENT_THREAD;
	if ( psRegs->eflags & EFLG_VM )
	{
		handle_v86_divide_exception( ( Virtual86Regs_s * ) psRegs, nErrorCode );
		return;
	}

	printk( "**Divide error**\n" );
	printk( "ERROR CODE = %08x\n", nErrorCode );
	print_registers( psRegs );
	printk( "\n" );

//  printk( "Areas :\n" );
//  list_areas( CURRENT_PROC->tc_psMemSeg );

	send_signal( psThread, SIGFPE, true );

//	do_exit( 12 << 8 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_fpu_exception( SysCallRegs_s * psRegs, int nErrorCode )
{
	Thread_s *psThread = CURRENT_THREAD;
	printk( "**Floating-point error**\n" );
	printk( "ERROR CODE = %08x\n", nErrorCode );
	print_registers( psRegs );
	printk( "\n" );

//  printk( "Areas :\n" );
//  list_areas( CURRENT_PROC->tc_psMemSeg );

	send_signal( psThread, SIGFPE, true );

//	do_exit( 12 << 8 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void math_state_restore( SysCallRegs_s * psRegs, int nErrorCode )
{
	Thread_s *psThread = CURRENT_THREAD;
	uint32 nFlags = cli();
	clts();		// Allow math ops (or we recurse)
	if ( ( psThread->tr_nFlags & TF_FPU_USED ) == 0 )
	{
		// Initialize the FPU state
		if ( g_asProcessorDescs[g_nBootCPU].pi_bHaveFXSR )
		{
			memset( &psThread->tc_FPUState.fpu_sFXSave, 0, sizeof( struct i3FXSave_t ) );
			psThread->tc_FPUState.fpu_sFXSave.cwd = 0x37f;
			if ( g_asProcessorDescs[g_nBootCPU].pi_bHaveXMM )
			{
				psThread->tc_FPUState.fpu_sFXSave.mxcsr = 0x1f80;
			}
		}
		else
		{
			memset( &psThread->tc_FPUState.fpu_sFSave, 0, sizeof( struct i3FSave_t ) );
			psThread->tc_FPUState.fpu_sFSave.cwd = 0xffff037fu;
			psThread->tc_FPUState.fpu_sFSave.swd = 0xffff0000u;
			psThread->tc_FPUState.fpu_sFSave.twd = 0xffffffffu;
			psThread->tc_FPUState.fpu_sFSave.fos = 0xffff0000u;
		}
		psThread->tr_nFlags |= TF_FPU_USED;
	}
	load_fpu_state( &psThread->tc_FPUState );
	psThread->tr_nFlags |= TF_FPU_DIRTY;
	put_cpu_flags( nFlags );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_sse_exception( SysCallRegs_s * psRegs, int nErrorCode )
{
	Thread_s *psThread = CURRENT_THREAD;
	printk( "**SSE floating-point error**\n" );
	printk( "ERROR CODE = %08x\n", nErrorCode );
	print_registers( psRegs );
	printk( "\n" );

//  printk( "Areas :\n" );
//  list_areas( CURRENT_PROC->tc_psMemSeg );

	send_signal( psThread, SIGFPE, true );

//	do_exit( 12 << 8 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_illegal_inst_exception( SysCallRegs_s * psRegs, int nErrorCode )
{
	Thread_s *psThread = CURRENT_THREAD;
	printk( "**Illegal instruction**\n" );
	printk( "ERROR CODE = %08x\n", nErrorCode );
	print_registers( psRegs );
	printk( "\n" );

//	printk( "Areas :\n" );
//	list_areas( CURRENT_PROC->tc_psMemSeg );

	send_signal( psThread, SIGILL, true );

//	do_exit( 12 << 8 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 handle_exec_ptrace( int dummy )
{
	SysCallRegs_s *psRegs = (SysCallRegs_s *) &dummy;
	Thread_s *psThread = CURRENT_THREAD;

	if ( atomic_read( &psThread->tr_nPTraceFlags ) & PT_PTRACED )
	{
		psThread->tr_nExitCode = SIGTRAP;
		psThread->tr_psPTraceRegs = psRegs;

		printk( "ptraced thread called exec, stopping...\n" );
		stop_thread( true );
	}

	return psRegs->orig_eax;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_debug_exception( SysCallRegs_s *psRegs, int nType )
{
	Thread_s *psThread = CURRENT_THREAD;

	if ( nType == 1 )
		psThread->tr_nDebugReg[6] = read_debug_status();

	if ( atomic_read( &psThread->tr_nPTraceFlags ) & PT_PTRACED )
	{
		psThread->tr_psPTraceRegs = psRegs;
		psThread->tr_nExitCode = SIGTRAP;
		stop_thread( true );
	}
	else
	{
		printk( "caught unexpected debug exception %d!\n", nType );
		send_signal( psThread, SIGSEGV, true );
	}
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void TrackStack( uint32 *esp )
{
	int32 i, j;

	for ( i = 0, j = 0; i < 4096 && j < 20; i++ )
	{

/*		if ( ((esp[i] + 4096) > CUL(&esp[i]) ) && ((esp[i] - 4096) < CUL(&esp[i]) ) )	*/
		if ( ( esp[i] & 0xfff00000 ) == 0x00100000 )
		{

/*			printk( "%d -> %.8x  (%d)\n", j, esp[i-1], i-1 );	*/
			printk( "%d -> %.8x  (%d)\n", j, esp[i + 0], i );

/*			printk( "%d -> %.8x  (%d)\n", j, esp[i+1], i+1 );	*/

/*			printk( "\n" );	*/
			j++;
		}
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void ExceptionHand( SysCallRegs_s * psRegs, int nException, int nErrorCode )
{
	printk( "Error: Exception %x (%08x,%04x:%08lx)\n", nException, nErrorCode, psRegs->cs, psRegs->eip );
	print_registers( psRegs );
	printk( "\n" );
//	list_areas( CURRENT_PROC->tc_psMemSeg );

	if ( CURRENT_THREAD != NULL )
	{
		send_signal( CURRENT_THREAD, SIGSEGV, true );
//		do_exit( 1 );
	}
	else
	{
		Schedule();
	}
	return;
}
