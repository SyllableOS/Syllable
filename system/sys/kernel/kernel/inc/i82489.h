
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

#ifndef __F_ATHEOS_I82489_H__
#define __F_ATHEOS_I82489_H__

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif

/*** 82489 and Pentium integrated APIC register layout ***/

#define	APIC_ID			0x20
#define	GET_APIC_ID(x)		(((x)>>24)&0x0F)
#define	SET_APIC_ID(x)		((x)<<24)
static const uint8_t APIC_VERSION	= 0x30;
static const uint8_t APIC_TASKPRI	= 0x80;
static const uint8_t APIC_TPRI_MASK	= 0xFF;
static const uint8_t APIC_ARBPRI	= 0x90;
static const uint8_t APIC_PROCPRI	= 0xA0;
static const uint8_t APIC_EOI		= 0xB0;
static const uint8_t APIC_EIO_ACK	= 0x0;	/* Write this to the EOI register */
static const uint8_t APIC_RRR		= 0xC0;
static const uint8_t APIC_LDR		= 0xD0;
#define	GET_APIC_LOGICAL_ID(x)	(((x)>>24)&0xFF)
static const uint8_t APIC_DFR		= 0xE0;

#define	GET_APIC_DFR(x)		(((x)>>28)&0x0F)
#define	SET_APIC_DFR(x)		((x)<<28)

static const uint8_t APIC_SPIV			= 0xF0;	///< Spurious interupt vector
static const uint32_t APIC_ISR			= 0x100;
static const uint32_t APIC_TMR			= 0x180;
static const uint32_t APIC_IRR			= 0x200;
static const uint32_t APIC_ESR			= 0x280;
static const uint32_t APIC_ESR_SEND_CS		= 0x00001;
static const uint32_t APIC_ESR_RECV_CS		= 0x00002;
static const uint32_t APIC_ESR_SEND_ACC		= 0x00004;
static const uint32_t APIC_ESR_RECV_ACC		= 0x00008;
static const uint32_t APIC_ESR_SENDILL		= 0x00020;
static const uint32_t APIC_ESR_RECVILL		= 0x00040;
static const uint32_t APIC_ESR_ILLREGA		= 0x00080;
static const uint32_t APIC_ICR			= 0x300;
static const uint32_t APIC_DEST_FIELD		= 0x00000;
static const uint32_t APIC_DEST_SELF		= 0x40000;
static const uint32_t APIC_DEST_ALLINC		= 0x80000;
static const uint32_t APIC_DEST_ALLBUT		= 0xC0000;
static const uint32_t APIC_DEST_RR_MASK		= 0x30000;
static const uint32_t APIC_DEST_RR_INVALID	= 0x00000;
static const uint32_t APIC_DEST_RR_INPROG	= 0x10000;
static const uint32_t APIC_DEST_RR_VALID	= 0x20000;
static const uint32_t APIC_DEST_LEVELTRIG	= 0x08000;
static const uint32_t APIC_DEST_ASSERT		= 0x04000;
static const uint32_t APIC_DEST_BUSY		= 0x01000;
static const uint32_t APIC_DEST_LOGICAL		= 0x00800;
static const uint32_t APIC_DEST_DM_FIXED	= 0x00000;
static const uint32_t APIC_DEST_DM_LOWEST	= 0x00100;
static const uint32_t APIC_DEST_DM_SMI		= 0x00200;
static const uint32_t APIC_DEST_DM_REMRD	= 0x00300;
static const uint32_t APIC_DEST_DM_NMI		= 0x00400;
static const uint32_t APIC_DEST_DM_INIT		= 0x00500;
static const uint32_t APIC_DEST_DM_STARTUP	= 0x00600;
static const uint32_t APIC_DEST_VECTOR_MASK	= 0x000FF;
static const uint32_t APIC_ICR2			= 0x310;

#define	GET_APIC_DEST_FIELD(x)	(((x)>>24)&0xFF)
#define	SET_APIC_DEST_FIELD(x)	((x)<<24)

static const uint32_t APIC_LVTT			= 0x320; ///< Local vector table (Timer)
static const uint32_t APIC_LVT0			= 0x350; ///< Local vector table (LINT0)
static const uint32_t APIC_LVT_TIMER_PERIODIC	= (1<<17);
static const uint32_t APIC_LVT_MASKED		= (1<<16);
static const uint32_t APIC_LVT_LEVEL_TRIGGER	= (1<<15);
static const uint32_t APIC_LVT_REMOTE_IRR	= (1<<14);
static const uint32_t APIC_INPUT_POLARITY	= (1<<13);
static const uint32_t APIC_SEND_PENDING	= (1<<12);

#define	GET_APIC_DELIVERY_MODE(x)	(((x)>>8)&0x7)
#define	SET_APIC_DELIVERY_MODE(x,y)	(((x)&~0x700)|((y)<<8))

static const uint8_t APIC_MODE_FIXED		= 0x0;
static const uint8_t APIC_MODE_NMI		= 0x4;
static const uint8_t APIC_MODE_EXINT		= 0x7;
static const uint32_t APIC_LVT1			= 0x360;/* Local vector table (LINT1) */
static const uint32_t APIC_LVERR		= 0x370;
static const uint32_t APIC_TMICT		= 0x380;/* Timer initial count */
static const uint32_t APIC_TMCCT		= 0x390;/* Timer current count */
static const uint32_t APIC_TDCR			= 0x3E0;/* Timer divide configure register */
static const uint8_t APIC_TDCR_MASK		= 0x0f;
static const uint8_t APIC_TDR_DIV_1		= 0xB;
static const uint8_t APIC_TDR_DIV_2		= 0x0;
static const uint8_t APIC_TDR_DIV_4		= 0x1;
static const uint8_t APIC_TDR_DIV_8		= 0x2;
static const uint8_t APIC_TDR_DIV_16		= 0x3;
static const uint8_t APIC_TDR_DIV_32		= 0x8;
static const uint8_t APIC_TDR_DIV_64		= 0x9;
static const uint8_t APIC_TDR_DIV_128		= 0xA;

#ifdef __cplusplus
}
#endif

#endif				/* __F_ATHEOS_I82489_H__ */
