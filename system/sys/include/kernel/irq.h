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

#ifndef __F_KERNEL_IRQ_H_
#define __F_KERNEL_IRQ_H_

#include <kernel/types.h>
#include <kernel/syscall.h>

#define	IRQ_COUNT	16
#define	TIMER_IRQ	0

typedef struct irqaction IrqAction_s;

#define IRQRET_CONTINUE 0x0000
#define IRQRET_RUN_BH	0x0001
#define IRQRET_HANDLED  0x0002
/* For backwards compatability */
#define IRQRET_BREAK    IRQRET_HANDLED
#define IRQRET_SCHEDULE 0x0004

typedef int irq_top_handler( int nIrqNum, void* pdata, SysCallRegs_s* psRegs );
typedef int irq_bottom_handler( int nIrqNum, void* pdata );

#define	IRQF_BH_ACTIVE	0x0001
#define IRQF_BH_IN_LIST	0x0002

struct irqaction
{
  IrqAction_s*		psNext;
  irq_top_handler*	pTopHandler;
  void*			pData;
  uint32 		nMask;
  irq_bottom_handler*	pBottomHandler;
  uint32 		nFlags;
  int			nIrqNum;
  int			nHandle;
  const char*		pzName;
};

#define INT_IRQ0	0x20
#define INT_IRQ1	0x21
#define INT_IRQ2	0x22
#define INT_IRQ3	0x23
#define INT_IRQ4	0x24
#define INT_IRQ5	0x25
#define INT_IRQ6	0x26
#define INT_IRQ7	0x27
#define INT_IRQ8	0x28
#define INT_IRQ9	0x29
#define INT_IRQ10	0x2a
#define INT_IRQ11	0x2b
#define INT_IRQ12	0x2c
#define INT_IRQ13	0x2d
#define INT_IRQ14	0x2e
#define INT_IRQ15	0x2f

#define	INT_INVAL_PGT	0xf0	/* Used to broadcast page-table invalidations on SMP machines	*/
#define	INT_SCHEDULE	0xf1	/* Driven by the local APIC timer on SMP machines		*/
#define	INT_SPURIOUS	0xf2	/* Intel APIC spurious(?) interupt vector			*/


uint32	cli( void );
void	sti( void );
void	put_cpu_flags( int nFlags );
int	get_cpu_flags( void );

void disable_irq_nosync( int nIrqNum );
void enable_irq( int nIrqNum );

int request_irq( int nIrqNum, irq_top_handler* pTopHandler, irq_bottom_handler* pBottomHandler,
		 uint32 nFlags, const char* pzDevName, void* pData );
int release_irq( int nIrqNum, int nHandle );
  
int reflect_irq_to_realmode( SysCallRegs_s* pCallsRegs, int num );

#endif /* __F_KERNEL_IRQ_H_ */
