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


static struct i3Task g_sInitialTSS;

extern uint32 g_anKernelStackEnd[];


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool Desc_SetBase( uint16 desc, uint32 base )
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

uint32 Desc_GetBase( uint16 desc )
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

bool Desc_SetLimit( uint16 desc, uint32 limit )
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

uint32 Desc_GetLimit( uint16 desc )
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

bool Desc_SetAccess( uint16 desc, uint8 acc )
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

uint8 Desc_GetAccess( uint16 desc )
{
	return ( g_sSysBase.ex_GDT[desc >> 3].desc_acc );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint16 Desc_Alloc( int32 table )
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

void Desc_Free( uint16 desc )
{
	uint32 nFlg = cli();

	sched_lock();
	g_sSysBase.ex_DTAllocList[desc >> 3] &= ~DTAL_GDT;
	sched_unlock();
	put_cpu_flags( nFlg );
}


void enable_mmu( void )
{
	
}

//****************************************************************************/
/** Initializes the intel descriptors for code and data segments. Also enables
 * the mmu.
 * Called by init_kernel() (init.c).
 * \internal
 * \ingroup CPU
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
 
void init_descriptors()
{
	struct i3DescrTable IDT;
	int i;
	unsigned int nDummy;
	
	/* Init the code and data descriptors */
	IDT.Base = ( uint32 )g_sSysBase.ex_IDT;
	IDT.Limit = 0x7ff;
	SetIDT( &IDT );


	Desc_SetBase( CS_KERNEL, 0x00000000 );
	Desc_SetLimit( CS_KERNEL, 0xffffffff );
	Desc_SetAccess( CS_KERNEL, 0x9a );

	Desc_SetBase( DS_KERNEL, 0x00000000 );
	Desc_SetLimit( DS_KERNEL, 0xffffffff );
	Desc_SetAccess( DS_KERNEL, 0x92 );

	Desc_SetBase( CS_USER, 0x00000000 );
	Desc_SetLimit( CS_USER, 0xffffffff );
	Desc_SetAccess( CS_USER, 0xfa );

	Desc_SetBase( DS_USER, 0x00000000 );
	Desc_SetLimit( DS_USER, 0xffffffff );
	Desc_SetAccess( DS_USER, 0xf2 );

	memset( &g_sInitialTSS, 0, sizeof( g_sInitialTSS ) );

	g_sInitialTSS.cs = CS_KERNEL;
	g_sInitialTSS.ss = DS_KERNEL;
	g_sInitialTSS.ds = DS_KERNEL;
	g_sInitialTSS.es = DS_KERNEL;
	g_sInitialTSS.fs = DS_KERNEL;
	g_sInitialTSS.gs = DS_KERNEL;
	g_sInitialTSS.eflags = 0x203246;

	g_sInitialTSS.cr3 = 0;
	g_sInitialTSS.ss0 = DS_KERNEL;
	g_sInitialTSS.esp0 = g_anKernelStackEnd;
	g_sInitialTSS.IOMapBase = 104;

	Desc_SetLimit( 0x40, 0xffff );
	Desc_SetBase( 0x40, ( uint32 )&g_sInitialTSS );
	Desc_SetAccess( 0x40, 0x89 );
	g_sSysBase.ex_GDT[0x40 >> 3].desc_lmh &= 0x8f;	// TSS descriptor has bit 22 clear (as opposed to 32 bit data and code descriptors)


	IDT.Base = ( uint32 )g_sSysBase.ex_GDT;
	IDT.Limit = 0xffff;
	SetGDT( &IDT );
	SetTR( 0x40 );
	__asm__ volatile ( "mov %0,%%ds;mov %0,%%es;mov %0,%%fs;mov %0,%%gs;mov %0,%%ss;"::"r" ( 0x18 ) );

	/* mark the first descriptors in GDT as used      */
	for ( i = 0; i < 8 + MAX_CPU_COUNT; i++ )
	{
		g_sSysBase.ex_DTAllocList[i] |= DTAL_GDT;
	}
	
	/* Enable the mmu */
	g_sInitialTSS.cr3 = ( void * )&g_psKernelSeg->mc_pPageDir;
	__asm__ __volatile__( "movl %%cr0,%0; orl $0x80010000,%0; movl %0,%%cr0" : "=r" (nDummy) );	// set PG & WP bit in cr0
}
 
 
