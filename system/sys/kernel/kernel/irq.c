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

static IrqAction_s* g_psIrqHandlerLists[ IRQ_COUNT ] = { 0, };

static unsigned int cached_irq_mask = 0x0; // 0xffff;

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
    if (irq & 8) {
	outb(cached_A1,0xA1);
    } else {
	outb(cached_21,0x21);
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
    unsigned int mask = ~(1 << irq);
    cached_irq_mask &= mask;
    if (irq & 8) {
	outb(cached_A1,0xA1);
    } else {
	outb(cached_21,0x21);
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
    put_cpu_flags(nFlags);
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
    put_cpu_flags(nFlags);
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int request_irq( int nIrqNum, irq_top_handler* pTopHandler, irq_bottom_handler* pBottomHandler,
		 uint32 nFlags, const char* pzDevName, void* pData )
{
    IrqAction_s*	psAction;
    int			nEFlags;
    int			nError;
    static int 		nIrqHandle = 1;

    if ( nIrqNum < 0 || nIrqNum >= IRQ_COUNT ) {
	return( -EINVAL );
    }

    psAction = kmalloc( sizeof( IrqAction_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_LOCKED | MEMF_OKTOFAILHACK );

    if ( psAction == NULL ) {
	return( -ENOMEM );
    }

    psAction->pTopHandler    = pTopHandler;
    psAction->pBottomHandler = pBottomHandler;
    psAction->nFlags	   = nFlags;
    psAction->nIrqNum	   = nIrqNum;
    psAction->pzName	   = pzDevName;
    psAction->pData	   = pData;
    psAction->psNext	   = NULL;
    psAction->nHandle	   = atomic_add( &nIrqHandle, 1 );

    enable_8259A_irq( nIrqNum );
    nEFlags = cli();


    if ( g_psIrqHandlerLists[ nIrqNum ] != NULL ) {
	if ( (nFlags & SA_SHIRQ) == 0 || (g_psIrqHandlerLists[ nIrqNum ]->nFlags & SA_SHIRQ) == 0 ) {
	    nError = -EBUSY;
	} else {
	    psAction->psNext = g_psIrqHandlerLists[ nIrqNum ];
	    g_psIrqHandlerLists[ nIrqNum ] = psAction;
	    nError = psAction->nHandle;
	}
    } else {
	g_psIrqHandlerLists[ nIrqNum ] = psAction;
	nError = psAction->nHandle;
    }

    put_cpu_flags( nEFlags );

    if ( nError < 0 ) {
	kfree( psAction );
    }

    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int release_irq( int nIrqNum, int nHandle )
{
    IrqAction_s** ppsTmp;
    IrqAction_s*	psAction = NULL;
    uint32	nEFlags;
  
    nEFlags = cli();
    for ( ppsTmp = &g_psIrqHandlerLists[ nIrqNum ] ; *ppsTmp != NULL ; ppsTmp = &(*ppsTmp)->psNext ) {
	if ( (*ppsTmp)->nHandle == nHandle ) {
	    psAction = *ppsTmp;
	    *ppsTmp = psAction->psNext;
	    break;
	}
    }
    if ( g_psIrqHandlerLists[ nIrqNum ] == NULL ) {
	disable_8259A_irq( nIrqNum );
    }
    put_cpu_flags( nEFlags );
  
    if ( psAction != NULL ) {
	kfree( psAction );
	return( 0 );
    } else {
	printk( "Error: release_irq() invalid handler %d/%d\n", nIrqNum, nHandle );
	return( -EINVAL );
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
    if (irq & 8) {
	inb(0xA1);	/* DUMMY */
	outb(cached_A1,0xA1);
	outb(0x62,0x20);	/* Specific EOI to cascade */
	outb(0x20,0xA0);
    } else {
	inb(0x21);	/* DUMMY */
	outb(cached_21,0x21);
	outb(0x20,0x20);
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_irq( SysCallRegs_s* psRegs, int nIrqNum )
{
    IrqAction_s* psAction;
    static IrqAction_s* apBottomHandler[1024];
    static int	      nBottomIn = 0;
    static int	      nBottomOut = 0;
    int  nIn;
    bool bNeedSchedule = false;
    
    if ( NULL != g_psIrqHandlerLists[ nIrqNum ] ) {
	mask_and_ack_8259A( nIrqNum );

	for ( psAction = g_psIrqHandlerLists[ nIrqNum ] ; NULL != psAction ; psAction = psAction->psNext ) {
	    if ( psAction->pTopHandler != NULL ) {
		int nRet = psAction->pTopHandler( nIrqNum, psAction->pData, psRegs );
      
		if ( (nRet & IRQRET_RUN_BH) && psAction->pBottomHandler != NULL ) {
		    psAction->nFlags |= IRQF_BH_IN_LIST;
		    apBottomHandler[(nBottomIn++) & 1023] = psAction;
		}
		if ( nRet & IRQRET_SCHEDULE ) {
		    bNeedSchedule = true;
		}
		if ( nRet & IRQRET_BREAK ) {
		    break;
		}
	    } else {
		if ( psAction->pBottomHandler != NULL ) {
		    psAction->nFlags |= IRQF_BH_IN_LIST;
		    apBottomHandler[(nBottomIn++) & 1023] = psAction;
		}
	    }
	}
	enable_8259A_irq( nIrqNum );
    
	nIn = nBottomIn & 1023;
    
	for(;;) {
	    int nOut = nBottomOut & 1023;
      
	    if ( nIn == nOut ) {
		break;
	    }
      
	    atomic_add( &nBottomOut, 1 );
      
	    psAction = apBottomHandler[nOut];
	    psAction->nFlags &= ~IRQF_BH_IN_LIST;

	    if ( (psAction->nFlags & IRQF_BH_ACTIVE) == 0 ) {
		psAction->nFlags |= IRQF_BH_ACTIVE;

		sti();
		psAction->pBottomHandler( psAction->nIrqNum, psAction->pData );
		cli();
		psAction->nFlags &= ~IRQF_BH_ACTIVE;
	    } else { // If the handler was active we re-insert it to the list, so the next IRQ can handle it
		psAction->nFlags |= IRQF_BH_IN_LIST;
		apBottomHandler[(nBottomIn++) & 1023] = psAction;
	    }
	}
    } else {
	if ( nIrqNum == 1 ) {
	    int n;
	    mask_and_ack_8259A( nIrqNum );
	    inb_p( 0x60 );
	    n = inb_p( 0x61 );
	    outb_p( n | 0x80, 0x61 );
	    outb_p( n & ~0x80, 0x61 );
	    enable_8259A_irq( nIrqNum );
	} else {
	    reflect_irq_to_realmode( psRegs, nIrqNum + ((nIrqNum < 8 ) ? 0x08 : 0x68) );
	}
    }
    if ( bNeedSchedule ) {
	Schedule();
    }
}
