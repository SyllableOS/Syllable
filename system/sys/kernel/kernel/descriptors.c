/*
 *  The Syllable kernel
 *  Intel descriptors support
 *  Copyright (C) 2003 The Syllable Team
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/udelay.h>
#include <atheos/irq.h>
#include <atheos/spinlock.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/areas.h"

extern uint32 g_anKernelStackEnd[];
extern uint32 g_nCpuMask;
extern vuint32 g_nMTRRInvalidateMask;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool set_gdt_desc_base( uint16 desc, uint32 base )
{
	desc >>= 3;
	g_sSysBase.ex_GDT[desc].desc_bsl = base & 0xffff;
	g_sSysBase.ex_GDT[desc].desc_bsm = ( base >> 16 ) & 0xff;
	g_sSysBase.ex_GDT[desc].desc_bsh = ( base >> 24 ) & 0xff;
	return ( true );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 get_gdt_desc_base( uint16 desc )
{
	uint32 base;

	desc >>= 3;

	base = g_sSysBase.ex_GDT[desc].desc_bsl;
	base += g_sSysBase.ex_GDT[desc].desc_bsm << 16;
	base += g_sSysBase.ex_GDT[desc].desc_bsh << 24;
	return ( base );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool set_gdt_desc_limit( uint16 desc, uint32 limit )
{
	desc >>= 3;
//  g_sSysBase.ex_GDT[desc].desc_lmh &= 0x70;   /* mask out hi nibble, and granularity bit      */
	g_sSysBase.ex_GDT[desc].desc_lmh = 0x40;	/* mask out hi nibble, and granularity bit      */

	if ( limit > 0x000fffff )
	{
		g_sSysBase.ex_GDT[desc].desc_lmh |= 0x80;	/* 4K granularity       */
		limit >>= 12;
	}

	g_sSysBase.ex_GDT[desc].desc_lml = limit & 0xffff;
	g_sSysBase.ex_GDT[desc].desc_lmh |= ( limit >> 16 ) & 0x0f;

	return ( true );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 get_gdt_desc_limit( uint16 desc )
{
	uint32 limit;

	desc >>= 3;

	limit = g_sSysBase.ex_GDT[desc].desc_lml;
	limit += ( g_sSysBase.ex_GDT[desc].desc_lmh & 0x0f ) << 16;

	if ( g_sSysBase.ex_GDT[desc].desc_lmh & 0x80 )	/* check granularity bit        */
	{
		limit <<= 12;
	}
	return ( limit );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool set_gdt_desc_access( uint16 desc, uint8 acc )
{
	desc >>= 3;
	g_sSysBase.ex_GDT[desc].desc_acc = acc;
	return ( true );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint8 get_gdt_desc_access( uint16 desc )
{
	return ( g_sSysBase.ex_GDT[desc >> 3].desc_acc );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint16 alloc_gdt_desc( int32 table )
{
	int i;
	uint32 nFlg = cli();

	sched_lock();

	for ( i = 0; i < 8192; i++ )
	{
		if ( !( g_sSysBase.ex_DTAllocList[i] & DTAL_GDT ) )
		{
			g_sSysBase.ex_DTAllocList[i] |= DTAL_GDT;
			sched_unlock();
			put_cpu_flags( nFlg );
			return ( i << 3 );
		}
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void free_gdt_desc( uint16 desc )
{
	uint32 nFlg = cli();

	sched_lock();
	g_sSysBase.ex_DTAllocList[desc >> 3] &= ~DTAL_GDT;
	sched_unlock();
	put_cpu_flags( nFlg );
}

/**
 * Writes the mtrr descriptors into the msr registers.
 * \internal
 * \ingroup CPU
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
void write_mtrr_descs( void )
{
	uint32 nTemp, nCr4;
	uint32 nDefLow, nDefHigh;
	int i;
	
	if( !g_asProcessorDescs[g_nBootCPU].pi_bHaveMTRR )
		return;
	
	/* Disable global pages */
	asm volatile ( "movl  %%cr4, %0\n\t"
		"movl  %0, %1\n\t"
		"andb  $0x7f, %b1\n\t"
		"movl  %1, %%cr4\n\t"
			: "=r" ( nCr4 ), "=q" ( nTemp ) : : "memory" );
		
	/* Disable caching */
	 asm volatile ( "movl  %%cr0, %0\n\t"
		"orl   $0x40000000, %0\n\t"
		"wbinvd\n\t"
		"movl  %0, %%cr0\n\t"
		"wbinvd\n\t"
			: "=r" ( nTemp ) : : "memory" );
		
	rdmsr( MSR_REG_MTRR_DEFTYPE, nDefLow, nDefHigh );
	wrmsr( MSR_REG_MTRR_DEFTYPE, nDefLow & 0xf300UL, nDefHigh );        

	/* Write the mtrr descriptors */
	for ( i = 0; i < g_sSysBase.ex_sMTRR.nNumDesc; i++ )
	{
		wrmsr( MSR_REG_MTRR_BASE( i ), g_sSysBase.ex_sMTRR.sDesc[i].nBaseLow, g_sSysBase.ex_sMTRR.sDesc[i].nBaseHigh );
		wrmsr( MSR_REG_MTRR_MASK( i ), g_sSysBase.ex_sMTRR.sDesc[i].nMaskLow, g_sSysBase.ex_sMTRR.sDesc[i].nMaskHigh );
	}

	/* Restore previous state */
	asm volatile ( "wbinvd" : : : "memory" );
	wrmsr( MSR_REG_MTRR_DEFTYPE, nDefLow, nDefHigh );     
	asm volatile ( "movl  %%cr0, %0\n\t"
		"andl  $0xbfffffff, %0\n\t"
		"movl  %0, %%cr0\n\t"
			: "=r" ( nTemp ) : : "memory" );
	asm volatile ( "movl  %0, %%cr4"
		: : "r" ( nCr4 ) : "memory" );
		
	printk( "MTRR descriptors written\n" );
}


/**
 * Allocates a mtrr descriptor
 * \ingroup CPU
 * \param nBase - Base address of the region.
 * \param nSize - Size of the region.
 * \param nType - Type.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
status_t alloc_mtrr_desc( uint64 nBase, uint64 nSize, int nType )
{
	int i;
	
	if( !g_asProcessorDescs[g_nBootCPU].pi_bHaveMTRR )
		return( -EINVAL );
	
	if( nBase & 0xfff || nSize & 0xfff )
	{
		printk( "Tried to set unaligned mtrr descriptor\n" );
		return( -EINVAL );
	}
	
	uint32 nFlg = cli();
	sched_lock();

	for ( i = 0; i < g_sSysBase.ex_sMTRR.nNumDesc; i++ )
	{
		if ( ~( ( g_sSysBase.ex_sMTRR.sDesc[i].nMaskLow & 0xfffff000UL ) - 1 ) == 0 )
		{
			g_sSysBase.ex_sMTRR.sDesc[i].nBaseLow = ( nBase & 0xfffff000 ) | nType;
			g_sSysBase.ex_sMTRR.sDesc[i].nBaseHigh = nBase >> 32;
			g_sSysBase.ex_sMTRR.sDesc[i].nMaskLow = ~( ( nSize & 0xfffff000 ) - 1 ) | 0x800;
			g_sSysBase.ex_sMTRR.sDesc[i].nMaskHigh = 0;
			sched_unlock();
			put_cpu_flags( nFlg );
			g_nMTRRInvalidateMask = g_nCpuMask;
			flush_tlb_global();
			return( 0 );
		}
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return( -ENOMEM );
}


/**
 * Frees a mtrr descriptor
 * \ingroup CPU
 * \param nBase - Base address of the mtrr descriptor.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
status_t free_mtrr_desc( uint64 nBase )
{
	int i;
	
	if( !g_asProcessorDescs[g_nBootCPU].pi_bHaveMTRR )
		return( -EINVAL );
	
	if( nBase & 0xfff )
	{
		printk( "Tried to free unaligned mtrr descriptor\n" );
		return( -EINVAL );
	}
	
	uint32 nFlg = cli();
	sched_lock();

	for ( i = 0; i < g_sSysBase.ex_sMTRR.nNumDesc; i++ )
	{
		if ( ~( ( g_sSysBase.ex_sMTRR.sDesc[i].nMaskLow & 0xfffff000UL ) - 1 ) > 0 &&
			( g_sSysBase.ex_sMTRR.sDesc[i].nBaseLow & 0xfffff000 ) == ( nBase & 0xffffffff ) )
		{
			g_sSysBase.ex_sMTRR.sDesc[i].nBaseLow = 0;
			g_sSysBase.ex_sMTRR.sDesc[i].nBaseHigh = 0;
			g_sSysBase.ex_sMTRR.sDesc[i].nMaskLow = 0;
			g_sSysBase.ex_sMTRR.sDesc[i].nMaskHigh = 0;
			sched_unlock();
			put_cpu_flags( nFlg );
			g_nMTRRInvalidateMask = g_nCpuMask;
			flush_tlb_global();
			return( 0 );
		}
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return( -EINVAL );
}


/**
 * Lists the mtrr descriptors
 * \internal
 * \ingroup CPU
 * \param argc the number of arguments in the debug command.
 * \param argv an array of pointers to the arguments in the debug command.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
static void db_list_mtrrs( int argc, char **argv )
{
	for( int i = 0; i < g_sSysBase.ex_sMTRR.nNumDesc; i++ )
	{
		const char* zType[] = { "Unchached", "Write combining", "", "", "Write through",
									"Write protected", "Write back" };
		dbprintf( DBP_DEBUGGER, "%x -> %x (%s)\n", ( g_sSysBase.ex_sMTRR.sDesc[i].nBaseLow & 0xfffff000 ),
				( g_sSysBase.ex_sMTRR.sDesc[i].nBaseLow & 0xfffff000 ) + (uint)~( ( g_sSysBase.ex_sMTRR.sDesc[i].nMaskLow & 0xfffff000UL ) - 1 ), 
				zType[( g_sSysBase.ex_sMTRR.sDesc[i].nBaseLow & 0xff ) % 7] );
	}
}


/**
 *  Enables the MMU and reads the MTRR descriptors
 * \internal
 * \ingroup CPU
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
void enable_mmu( void )
{
	unsigned int nDummy;
	int i;
	
	/* Enable the mmu */
	g_asProcessorDescs[get_processor_id()].pi_sTSS.cr3 = ( void * )&g_psKernelSeg->mc_pPageDir;
	__asm__ __volatile__( "movl %%cr0,%0; orl $0x80010000,%0; movl %0,%%cr0" : "=r" (nDummy) );	// set PG & WP bit in cr0
	
	/* Read the MTRR descriptor table from the boot cpu */
	if( !g_asProcessorDescs[g_nBootCPU].pi_bHaveMTRR )
		return;
		
	rdmsr( MSR_REG_MTRR_CAP, g_sSysBase.ex_sMTRR.nNumDesc, nDummy );
	g_sSysBase.ex_sMTRR.nNumDesc &= MSR_REG_MTRR_CAP_NUM;
	for( i = 0; i < g_sSysBase.ex_sMTRR.nNumDesc; i++ )
	{
			rdmsr( MSR_REG_MTRR_BASE( i ), g_sSysBase.ex_sMTRR.sDesc[i].nBaseLow, g_sSysBase.ex_sMTRR.sDesc[i].nBaseHigh );
			rdmsr( MSR_REG_MTRR_MASK( i ), g_sSysBase.ex_sMTRR.sDesc[i].nMaskLow, g_sSysBase.ex_sMTRR.sDesc[i].nMaskHigh );
	}
	
	register_debug_cmd( "ls_mtrr", "list all mtrr entries.", db_list_mtrrs );
}


/**
 *  Initializes the intel descriptors for code and data segments.
 * \internal
 * \ingroup CPU
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
void init_descriptors()
{
	struct i3DescrTable IDT;
	int i;
	
	/* Init the code and data descriptors */
	IDT.Base = ( uint32 )g_sSysBase.ex_IDT;
	IDT.Limit = 0x7ff;
	SetIDT( &IDT );


	set_gdt_desc_base( CS_KERNEL, 0x00000000 );
	set_gdt_desc_limit( CS_KERNEL, 0xffffffff );
	set_gdt_desc_access( CS_KERNEL, 0x9a );

	set_gdt_desc_base( DS_KERNEL, 0x00000000 );
	set_gdt_desc_limit( DS_KERNEL, 0xffffffff );
	set_gdt_desc_access( DS_KERNEL, 0x92 );

	set_gdt_desc_base( CS_USER, 0x00000000 );
	set_gdt_desc_limit( CS_USER, 0xffffffff );
	set_gdt_desc_access( CS_USER, 0xfa );

	set_gdt_desc_base( DS_USER, 0x00000000 );
	set_gdt_desc_limit( DS_USER, 0xffffffff );
	set_gdt_desc_access( DS_USER, 0xf2 );
	
	/* Initialize the TSS descriptors for every CPU */
	for ( i = 0; i < MAX_CPU_COUNT; i++ )
	{
		TaskStateSeg_s* psTSS = &g_asProcessorDescs[i].pi_sTSS;
		
		memset( psTSS, 0, sizeof( TaskStateSeg_s ) );

		psTSS->cs = CS_KERNEL;
		psTSS->ss = DS_KERNEL;
		psTSS->ds = DS_KERNEL;
		psTSS->es = DS_KERNEL;
		psTSS->fs = DS_KERNEL;
		psTSS->gs = DS_KERNEL;
		psTSS->eflags = 0x203246;

		psTSS->cr3 = 0;
		psTSS->ss0 = DS_KERNEL;
		psTSS->esp0 = g_anKernelStackEnd;
		psTSS->IOMapBase = 104;
		
		set_gdt_desc_limit( ( 8 + i ) << 3, 0xffff );
		set_gdt_desc_base( ( 8 + i ) << 3, ( uint32 )psTSS );
		set_gdt_desc_access( ( 8 + i ) << 3, 0x89 );
		g_sSysBase.ex_GDT[8+i].desc_lmh &= 0x8f;	// TSS descriptor has bit 22 clear (as opposed to 32 bit data and code descriptors)
	}

	
	IDT.Base = ( uint32 )g_sSysBase.ex_GDT;
	IDT.Limit = 0xffff;
	SetGDT( &IDT );
	__asm__ volatile ( "mov %0,%%ds;mov %0,%%es;mov %0,%%fs;mov %0,%%gs;mov %0,%%ss;"::"r" ( 0x18 ) );

	/* mark the first descriptors in GDT as used      */
	for ( i = 0; i < 8 + MAX_CPU_COUNT; i++ )
	{
		g_sSysBase.ex_DTAllocList[i] |= DTAL_GDT;
	}
}
 
 
