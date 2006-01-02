
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


#include <posix/errno.h>
#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/syscall.h>
#include <atheos/irq.h>
#include <atheos/smp.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/areas.h"
#include "inc/intel.h"
#include "inc/smp.h"

static const uint16_t V86_STACK_SIZE = 0x4000;

static Virtual86State_s *g_psFirstV86State = NULL;

//static sem_id g_hV86Lock = -1;

static inline void return_to_32bit( Virtual86Regs_s * regs16, int retval );

/*
 * Boy are these ugly, but we need to do the correct 16-bit arithmetic.
 * Gcc makes a mess of it, so we do it inline and use non-obvious calling
 * conventions..
 */

/*
#define pushb(base, ptr, val) \
__asm__ __volatile__( \
	"decw %w0\n\t" \
	"movb %2,0(%1,%0)" \
	: "=r" (ptr) \
	: "r" (base), "q" (val), "0" (ptr))
	*/
#define pushw(base, ptr, val) \
__asm__ __volatile__( \
	"decw %w0\n\t" \
	"movb %h2,0(%1,%0)\n\t" \
	"decw %w0\n\t" \
	"movb %b2,0(%1,%0)" \
	: "=r" (ptr) \
	: "r" (base), "q" (val), "0" (ptr))

#define pushl(base, ptr, val) \
__asm__ __volatile__( \
	"decw %w0\n\t" \
	"rorl $16,%2\n\t" \
	"movb %h2,0(%1,%0)\n\t" \
	"decw %w0\n\t" \
	"movb %b2,0(%1,%0)\n\t" \
	"decw %w0\n\t" \
	"rorl $16,%2\n\t" \
	"movb %h2,0(%1,%0)\n\t" \
	"decw %w0\n\t" \
	"movb %b2,0(%1,%0)" \
	: "=r" (ptr) \
	: "r" (base), "q" (val), "0" (ptr))

#define popb(base, ptr) \
({ unsigned long __res; \
__asm__ __volatile__( \
	"movb 0(%1,%0),%b2\n\t" \
	"incw %w0" \
	: "=r" (ptr), "=r" (base), "=q" (__res) \
	: "0" (ptr), "1" (base), "2" (0)); \
__res; })

#define popw(base, ptr) \
({ unsigned long __res; \
__asm__ __volatile__( \
	"movb 0(%1,%0),%b2\n\t" \
	"incw %w0\n\t" \
	"movb 0(%1,%0),%h2\n\t" \
	"incw %w0" \
	: "=r" (ptr), "=r" (base), "=q" (__res) \
	: "0" (ptr), "1" (base), "2" (0)); \
__res; })

#define popl(base, ptr) \
({ unsigned long __res; \
__asm__ __volatile__( \
	"movb 0(%1,%0),%b2\n\t" \
	"incw %w0\n\t" \
	"movb 0(%1,%0),%h2\n\t" \
	"incw %w0\n\t" \
	"rorl $16,%2\n\t" \
	"movb 0(%1,%0),%b2\n\t" \
	"incw %w0\n\t" \
	"movb 0(%1,%0),%h2\n\t" \
	"incw %w0\n\t" \
	"rorl $16,%2" \
	: "=r" (ptr), "=r" (base), "=q" (__res) \
	: "0" (ptr), "1" (base)); \
__res; })

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void do_int( Virtual86Regs_s * regs, int i, unsigned char *ssp, unsigned long sp )
{
	unsigned long *intr_ptr, segoffs;

	intr_ptr = ( unsigned long * )( i << 2 );

	segoffs = *intr_ptr;

	pushw( ssp, sp, regs->eflags & 0xffff );
	pushw( ssp, sp, regs->cs );
	pushw( ssp, sp, regs->eip & 0xffff );

	regs->eflags &= ~EFLG_IF;

	regs->cs = segoffs >> 16;
	regs->esp = ( regs->esp - 6 ) & 0xffff;
	regs->eip = segoffs & 0xffff;

/*	clear_IF(regs); */
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_v86_fault( Virtual86Regs_s * regs, uint32 nErrorCode )
{
	unsigned char *csp, *ssp;
	unsigned long ip, sp;
	int nInst;

//  if ( 0xffff == (regs->eip & 0xffff) && 0xffff == regs->cs ) {
//    return_to_32bit( regs, 0 );
//    return;
//  }


	csp = ( unsigned char * )( regs->cs << 4 );
	ssp = ( unsigned char * )( regs->ss << 4 );

	sp = regs->esp & 0xffff;
	ip = regs->eip & 0xffff;

	nInst = popb( csp, ip );
	switch ( nInst )
	{
		/* Operand size override */
	case 0x66:
		printk( "WARNING : 32 bit code run in v86 mode! Flags are not handled properly!\n" );
		nInst = popb( csp, ip );
		switch ( nInst )
		{
			/* pushfd */
		case 0x9c:
			regs->esp = ( regs->esp - 4 ) & 0xffff;
			regs->eip = ( regs->eip + 2 ) & 0xffff;
			pushl( ssp, sp, regs->eflags );
			return;

			/* popfd */
		case 0x9d:
			regs->esp = ( regs->esp + 4 ) & 0xffff;
			regs->eip = ( regs->eip + 2 ) & 0xffff;
			regs->eflags = popl( ssp, sp );
			return;

			/* iretd */
		case 0xcf:
			regs->esp = ( regs->esp + 12 ) & 0xffff;
			regs->eip = ( uint16 )( popl( ssp, sp ) & 0xffff );
			regs->cs = ( uint16 )popl( ssp, sp );
			return;
			/* need this to avoid a fallthrough */
		default:
			printk( "ERROR : unknown v86 32 bit instruction %x\n", nInst );
			return_to_32bit( regs, -EFAULT );
		}

		/* pushf */
	case 0x9c:
		regs->esp = ( regs->esp - 2 ) & 0xffff;
		regs->eip = ( regs->eip + 1 ) & 0xffff;
		pushw( ssp, sp, regs->eflags & 0xffff );
		return;

		/* popf */
	case 0x9d:
		regs->esp = ( regs->esp + 2 ) & 0xffff;
		regs->eip = ( regs->eip + 1 ) & 0xffff;
		regs->eflags = ( regs->eflags & 0xffff0000 ) | ( popw( ssp, sp ) & 0xffff );
		return;

		/* int xx */
	case 0xcd:
		{
			int intno = popb( csp, ip );

			regs->eip = ( regs->eip + 2 ) & 0xffff;
			do_int( regs, intno, ssp, sp );
			return;
		}

		/* iret */
	case 0xcf:
		regs->esp = ( regs->esp + 6 ) & 0xffff;
		regs->eip = popw( ssp, sp ) & 0xffff;
		regs->cs = popw( ssp, sp );

		regs->eflags = ( regs->eflags & 0xffff0000 ) | ( popw( ssp, sp ) & 0xffff );

		if ( 0xffff == ( regs->eip & 0xffff ) && 0xffff == regs->cs )
		{
			return_to_32bit( regs, 0 );
		}

		return;

		/* cli */
	case 0xfa:
		regs->eip = ( regs->eip + 1 ) & 0xffff;
		regs->eflags &= ~EFLG_IF;
		return;

		/* sti */
	case 0xfb:
		/* The interrupts should actually be restored after the NEXT instruction!
		 * Hope this works. As long as no DOS/BIOS code swaps the stack,
		 * nothing bad should happen.
		 */
		regs->eip = ( regs->eip + 1 ) & 0xffff;
		regs->eflags |= EFLG_IF;
		return;
	default:
		printk( "ERROR : unknown v86 16 bit instruction %x\n", nInst );
		return_to_32bit( regs, -EFAULT );
	}
}


void handle_v86_pagefault( Virtual86Regs_s * regs, uint32 nErrorCode )
{
	if ( 0xffff == ( regs->eip & 0xffff ) && 0xffff == regs->cs )
	{
		return_to_32bit( regs, 0 );
	}
	else
	{
		printk( "Error: invalid page-fault in BIOS\n" );
		return_to_32bit( regs, -EFAULT );
	}
}

void handle_v86_divide_exception( Virtual86Regs_s * regs, uint32 nErrorCode )
{
	printk( "Error: division by zero in BIOS\n" );
	return_to_32bit( regs, -EFAULT );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_call_v86( Virtual86Struct_s * psState, SysCallRegs_s * psCallRegs )
{
	Virtual86State_s sState;
	Thread_s *psThread = CURRENT_THREAD;
	int nFlags;


	nFlags = cli();	/* Will be reset when we exit to v86 mode */

	atomic_inc( &psThread->tr_nInV86 );
	while ( get_processor_id() != g_nBootCPU )
	{
		printk( "do_call_v86() wrong CPU (%d), will schedule\n", get_processor_id() );
		Schedule();
	}

      //printk( "Enter v86\n" );

	memcpy( &sState.regs, &psState->regs, sizeof( sState.regs ) );

	sState.regs32 = psCallRegs;	// (SysCallRegs_s*) &psState;
	sState.psRegs16 = &psState->regs;

	if ( NULL == g_psFirstV86State )
	{
		unprotect_dos_mem();
	}
	sState.psNext = g_psFirstV86State;
	g_psFirstV86State = &sState;

	sState.regs.__null_ds = 0;
	sState.regs.__null_es = 0;
	sState.regs.__null_fs = 0;
	sState.regs.__null_gs = 0;

	sState.regs.eflags = sState.regs32->eflags | EFLG_VM;
	sState.regs.eflags &= ~EFLG_IOPL;

	sState.regs32->eax = 0;

	sState.pSavedStack = psThread->tr_pESP0;
	psThread->tr_pESP0 = ( void * )&sState.VM86_TSS_ESP0;

	g_asProcessorDescs[get_processor_id()].pi_sTSS.esp0 = psThread->tr_pESP0;
	
	__asm__ __volatile__( "movl %0,%%esp\n\t" "jmp ret_from_sys_call":	/* no outputs */
		:"r"( &sState.regs ) );

	/* we never return here */
	return ( 0 );
}

int sys_call_v86( Virtual86Struct_s * psState )
{
	int nError = do_call_v86( psState, ( SysCallRegs_s * ) & psState );

	return ( nError );
}

int call_v86( Virtual86Struct_s * psState )
{
	int nError;
	__asm__ volatile ( "int $0x80":"=a" ( nError ):"0"( __NR_call_v86 ), "b"( ( int )psState ) );

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static inline void return_to_32bit( Virtual86Regs_s * regs16, int retval )
{
	Thread_s *psThread = CURRENT_THREAD;
	SysCallRegs_s *psCallerRegs;
	int nGS;

	cli();		/* Will be reset when we return to caller */

	nGS = g_asProcessorDescs[get_processor_id()].pi_nGS;
	__asm__ __volatile__( "mov %0,%%gs"::"r"( nGS ) );

	set_gdt_desc_base( nGS, ( uint32 )psThread->tr_pThreadData );
	
	__asm__ __volatile__( "movl %0,%%gs\n\t" :	/* no outputs */
		:"r"( nGS ) );

	if ( NULL == g_psFirstV86State )
	{
		printk( "ERROR : return_to_32bit() called while g_psFirstV86State == NULL! Propably game over :(\n" );
		return;
	}

	memcpy( g_psFirstV86State->psRegs16, regs16, sizeof( *regs16 ) );

	g_psFirstV86State->regs32->eax = retval;
	psCallerRegs = g_psFirstV86State->regs32;

	psThread->tr_pESP0 = g_psFirstV86State->pSavedStack;
	g_asProcessorDescs[get_processor_id()].pi_sTSS.esp0 = psThread->tr_pESP0;


	g_psFirstV86State = g_psFirstV86State->psNext;

	kassertw( get_processor_id() == g_nBootCPU );

	if ( NULL == g_psFirstV86State )
	{
		protect_dos_mem();
	}
	atomic_dec( &psThread->tr_nInV86 );
	psCallerRegs->eax = retval;

	__asm__ __volatile__( "movl %0,%%esp\n\t" "jmp exit_from_sys_call"::"r"( psCallerRegs ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

extern uint32 v86Stack_seg;
extern uint32 v86Stack_off;

int reflect_irq_to_realmode( SysCallRegs_s * psCallRegs, int num )
{
	pgd_t *pPgd = pgd_offset( g_psKernelSeg, 0 );
	pte_t *pPte = pte_offset( pPgd, 0 );
	Virtual86Struct_s sRegs;
	uint32 *pIntVects = NULL;
	uint32 *pnStack;
	uint32 nFlags;

	num &= 0xff;


	memset( &sRegs, 0, sizeof( sRegs ) );

	nFlags = cli();
	kassertw( get_processor_id() == g_nBootCPU );


	// We need access to the first page to read the IVT
	PTE_VALUE( *pPte ) |= PTE_PRESENT;
	flush_tlb_page( 0 );


	sRegs.regs.eip = pIntVects[num] & 0xffff;
	sRegs.regs.cs = pIntVects[num] >> 16;


	pnStack = ( uint32 * )( ( v86Stack_seg << 4 ) + v86Stack_off );
	pnStack[0] = 0xffffffff;

	sRegs.regs.esp = v86Stack_off;
	sRegs.regs.ss = v86Stack_seg;
	v86Stack_off -= V86_STACK_SIZE;
	put_cpu_flags( nFlags );

	call_v86( &sRegs );
//  do_call_v86( &sRegs, psCallRegs );

	v86Stack_off += V86_STACK_SIZE;

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_realint( int num, struct RMREGS *rm )
{
	pgd_t *pPgd = pgd_offset( g_psKernelSeg, 0 );
	pte_t *pPte = pte_offset( pPgd, 0 );
	Thread_s *psThread = CURRENT_THREAD;
	Virtual86Struct_s sRegs;
	uint32 *pIntVects = NULL;
	uint32 *pnStack;
	uint32 nFlags;
	
	sRegs.regs.eax = rm->EAX;
	sRegs.regs.orig_eax = rm->EAX;
	sRegs.regs.ebx = rm->EBX;
	sRegs.regs.ecx = rm->ECX;
	sRegs.regs.edx = rm->EDX;
	sRegs.regs.edi = rm->EDI;
	sRegs.regs.esi = rm->ESI;
	sRegs.regs.ebp = rm->EBP;
	sRegs.regs.eflags = rm->flags;
	sRegs.regs.ds = rm->DS;
	sRegs.regs.es = rm->ES;
	sRegs.regs.fs = rm->FS;
	sRegs.regs.gs = rm->GS;


	nFlags = cli();
	// We need access to the first page to read the IVT
	PTE_VALUE( *pPte ) |= PTE_PRESENT;
	flush_tlb_page( 0 );

	sRegs.regs.eip = pIntVects[num] & 0xffff;
	sRegs.regs.cs = pIntVects[num] >> 16;

	//printk( "sys_realint(%d) -> %04x:%04lx\n", num, sRegs.regs.cs, sRegs.regs.eip );

	atomic_inc( &psThread->tr_nInV86 );

	kassertw( atomic_read( &psThread->tr_nInV86 ) == 1 );

	while ( get_processor_id() != g_nBootCPU )
	{
//    printk( "sys_call_v86() wrong CPU (%d), will schedule\n", get_processor_id() );
		Schedule();
	}

	pnStack = ( uint32 * )( ( v86Stack_seg << 4 ) + v86Stack_off );
	pnStack[0] = 0xffffffff;
	sRegs.regs.esp = v86Stack_off;
	sRegs.regs.ss = v86Stack_seg;
	v86Stack_off -= V86_STACK_SIZE;

	put_cpu_flags( nFlags );

	call_v86( &sRegs );

	v86Stack_off += V86_STACK_SIZE;

	atomic_dec( &psThread->tr_nInV86 );


	rm->EAX = sRegs.regs.eax;
	rm->EBX = sRegs.regs.ebx;
	rm->ECX = sRegs.regs.ecx;
	rm->EDX = sRegs.regs.edx;
	rm->EDI = sRegs.regs.edi;
	rm->ESI = sRegs.regs.esi;
	rm->EBP = sRegs.regs.ebp;
	rm->flags = sRegs.regs.eflags;
	rm->DS = sRegs.regs.ds;
	rm->ES = sRegs.regs.es;
	rm->FS = sRegs.regs.fs;
	rm->GS = sRegs.regs.gs;

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int realint( int num, struct RMREGS *rm )
{
	return ( sys_realint( num, rm ) );
}
