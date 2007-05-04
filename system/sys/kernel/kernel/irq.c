
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999  Kurt Skauen
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
#include <atheos/isa_io.h>

#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/irq.h>
#include <atheos/smp.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/io_ports.h"
#include "inc/smp.h"

static IrqAction_s *g_psIrqHandlerLists[IRQ_COUNT] = { 0, };

static unsigned int cached_irq_mask = 0xffff;

#define __byte(x,y) 	(((uint8*)&(y))[x])
#define cached_21	(__byte(0,cached_irq_mask))
#define cached_A1	(__byte(1,cached_irq_mask))

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void disable_8259A_irq( int irq )
{
	unsigned int mask = 1 << irq;

	cached_irq_mask |= mask;
	if ( irq & 8 )
	{
		outb( cached_A1, PIC_SLAVE_IMR );
	}
	else
	{
		outb( cached_21, PIC_MASTER_IMR );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void enable_8259A_irq( int irq )
{
	unsigned int mask = ~( 1 << irq );

	cached_irq_mask &= mask;
	if ( irq & 8 )
	{
		outb( cached_A1, PIC_SLAVE_IMR );
	}
	else
	{
		outb( cached_21, PIC_MASTER_IMR );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void disable_irq_nosync( int nIrqNum )
{
	uint32 nFlags;

	nFlags = cli();
	disable_8259A_irq( nIrqNum );
	put_cpu_flags( nFlags );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void enable_irq( int nIrqNum )
{
	uint32 nFlags;

	nFlags = cli();
	enable_8259A_irq( nIrqNum );
	put_cpu_flags( nFlags );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int request_irq( int nIrqNum, irq_top_handler *pTopHandler, irq_bottom_handler *pBottomHandler, uint32 nFlags, const char *pzDevName, void *pData )
{
	IrqAction_s *psAction;
	int nEFlags;
	int nError;
	static atomic_t nIrqHandle = ATOMIC_INIT(1);

	if ( nIrqNum < 0 || nIrqNum >= IRQ_COUNT )
	{
		return ( -EINVAL );
	}

	psAction = kmalloc( sizeof( IrqAction_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_LOCKED | MEMF_OKTOFAIL );

	if ( psAction == NULL )
	{
		return ( -ENOMEM );
	}

	psAction->pTopHandler = pTopHandler;
	psAction->pBottomHandler = pBottomHandler;
	psAction->nFlags = nFlags;
	psAction->nIrqNum = nIrqNum;
	psAction->pzName = pzDevName;
	psAction->pData = pData;
	psAction->psNext = NULL;
	psAction->nHandle = atomic_inc_and_read( &nIrqHandle );


	nEFlags = cli();
	
	
	if ( g_psIrqHandlerLists[nIrqNum] != NULL )
	{
		if ( ( nFlags & SA_SHIRQ ) == 0 || ( g_psIrqHandlerLists[nIrqNum]->nFlags & SA_SHIRQ ) == 0 )
		{
			nError = -EBUSY;
		}
		else
		{
			psAction->psNext = g_psIrqHandlerLists[nIrqNum];
			g_psIrqHandlerLists[nIrqNum] = psAction;
			nError = psAction->nHandle;
		}
	}
	else
	{
		g_psIrqHandlerLists[nIrqNum] = psAction;
		nError = psAction->nHandle;
		enable_8259A_irq( nIrqNum );
		printk( "IRQ %i enabled\n", nIrqNum );
	}

	put_cpu_flags( nEFlags );

	if ( nError < 0 )
	{
		kfree( psAction );
	}

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int release_irq( int nIrqNum, int nHandle )
{
	IrqAction_s **ppsTmp;
	IrqAction_s *psAction = NULL;
	uint32 nEFlags;

	nEFlags = cli();
	for ( ppsTmp = &g_psIrqHandlerLists[nIrqNum]; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->psNext )
	{
		if ( ( *ppsTmp )->nHandle == nHandle )
		{
			psAction = *ppsTmp;
			*ppsTmp = psAction->psNext;
			break;
		}
	}
	if ( g_psIrqHandlerLists[nIrqNum] == NULL )
	{
		disable_8259A_irq( nIrqNum );
	}
	put_cpu_flags( nEFlags );

	if ( psAction != NULL )
	{
		kfree( psAction );
		return ( 0 );
	}
	else
	{
		printk( "Error: release_irq() invalid handler %d/%d\n", nIrqNum, nHandle );
		return ( -EINVAL );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * 	Careful! The 8259A is a fragile beast, it pretty
 * 	much _has_ to be done exactly like this (mask it
 * 	first, _then_ send the EOI, and the order of EOI
 * 	to the two 8259s is important!
 *
 * SEE ALSO:
 ****************************************************************************/

static inline void mask_and_ack_8259A( int irq )
{
	cached_irq_mask |= 1 << irq;
	if ( irq & 8 )
	{
		inb( PIC_SLAVE_IMR );	/* DUMMY */
		outb( cached_A1, PIC_SLAVE_IMR );
		outb( 0x60 + ( irq & 7 ), PIC_SLAVE_CMD );	/* Specific EOI to cascade */
		outb( 0x60 + PIC_CASCADE_IR, PIC_MASTER_CMD );
	}
	else
	{
		inb( PIC_MASTER_IMR );	/* DUMMY */
		outb( cached_21, PIC_MASTER_IMR );
		outb( 0x60 + irq, PIC_MASTER_CMD );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_irq( SysCallRegs_s * psRegs, int nIrqNum )
{
	IrqAction_s *psAction;
	static IrqAction_s *apBottomHandler[1024];
	static int nBottomIn = 0;
	static atomic_t nBottomOut = ATOMIC_INIT(0);
	int nIn;
	bool bNeedSchedule = false;
	
	if( get_processor_id() != g_nBootCPU )
		printk( "Error: Got hardware interrupt on non boot cpu!\n" );
	
	
	if ( NULL != g_psIrqHandlerLists[nIrqNum] )
	{
		mask_and_ack_8259A( nIrqNum );

		for ( psAction = g_psIrqHandlerLists[nIrqNum]; NULL != psAction; psAction = psAction->psNext )
		{
			if ( psAction->pTopHandler != NULL )
			{
				int nRet = psAction->pTopHandler( nIrqNum, psAction->pData, psRegs );

				if ( ( nRet & IRQRET_RUN_BH ) && psAction->pBottomHandler != NULL )
				{
					psAction->nFlags |= IRQF_BH_IN_LIST;
					apBottomHandler[( nBottomIn++ ) & 1023] = psAction;
				}
				if ( nRet & IRQRET_SCHEDULE )
				{
					bNeedSchedule = true;
				}
				if ( nRet & IRQRET_BREAK )
				{
					break;
				}
			}
			else
			{
				if ( psAction->pBottomHandler != NULL )
				{
					psAction->nFlags |= IRQF_BH_IN_LIST;
					apBottomHandler[( nBottomIn++ ) & 1023] = psAction;
				}
			}
		}
		enable_8259A_irq( nIrqNum );

		nIn = nBottomIn & 1023;

		for ( ;; )
		{
			int nOut = atomic_read( &nBottomOut ) & 1023;

			if ( nIn == nOut )
			{
				break;
			}

			atomic_inc( &nBottomOut );

			psAction = apBottomHandler[nOut];
			psAction->nFlags &= ~IRQF_BH_IN_LIST;

			if ( ( psAction->nFlags & IRQF_BH_ACTIVE ) == 0 )
			{
				psAction->nFlags |= IRQF_BH_ACTIVE;

				sti();
				psAction->pBottomHandler( psAction->nIrqNum, psAction->pData );
				cli();
				psAction->nFlags &= ~IRQF_BH_ACTIVE;
			}
			else
			{	// If the handler was active we re-insert it to the list, so the next IRQ can handle it
				psAction->nFlags |= IRQF_BH_IN_LIST;
				apBottomHandler[( nBottomIn++ ) & 1023] = psAction;
			}
		}
	}
	else
	{
		if ( nIrqNum == 1 )
		{
			int n;

			mask_and_ack_8259A( nIrqNum );
			inb_p( 0x60 );
			n = inb_p( 0x61 );
			outb_p( n | 0x80, 0x61 );
			outb_p( n & ~0x80, 0x61 );
			enable_8259A_irq( nIrqNum );
		}
		else if ( nIrqNum == 13 )
		{
			// reset FPU busy latch
			mask_and_ack_8259A( nIrqNum );
			outb( 0, 0xF0 );
			enable_8259A_irq( nIrqNum );
			printk( "IRQ13 received!\n" );
		}
		else
		{
			mask_and_ack_8259A( nIrqNum );
			reflect_irq_to_realmode( psRegs, nIrqNum + ( ( nIrqNum < 8 ) ? 0x08 : 0x68 ) );
			enable_8259A_irq( nIrqNum );
		}
	}
	if ( bNeedSchedule )
	{
		DoSchedule( psRegs );
	}
}


void init_irq_controller( void )
{
	/* Mask all interrupts */
	outb( 0xff, PIC_SLAVE_IMR );
	outb( 0xff, PIC_MASTER_IMR );
	
	/* Initialize the PIC */
	outb( 0x11, PIC_MASTER_CMD );
	outb( 0x20, PIC_MASTER_IMR ); // First PIC start at vector 0x20
	outb( 0x04, PIC_MASTER_IMR );
	outb( MASTER_ICW4_DEFAULT, PIC_MASTER_IMR );
	
	outb( 0x11, PIC_SLAVE_CMD );
	outb( 0x28, PIC_SLAVE_IMR ); // Second PIC start at vector 0x28
	outb( 0x02, PIC_SLAVE_IMR );
	outb( SLAVE_ICW4_DEFAULT, PIC_SLAVE_IMR );
	
	/* Mask all interrupts */
	outb( cached_A1, PIC_SLAVE_IMR );
	outb( cached_21, PIC_MASTER_IMR );
	
	/* Enable IRQ 0 (Timer), 2 (Connection to slave) and 6 (floppy) */
	enable_8259A_irq( 0 );
	enable_8259A_irq( 2 );
	enable_8259A_irq( 6 );
}










