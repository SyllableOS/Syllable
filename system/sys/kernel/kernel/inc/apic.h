/*
 *  The Syllable kernel
 *  Copyright (C) 2006 Kristian Van Der Vliet
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

#ifndef __F_APIC_H__
#define __F_APIC_H__

#include <atheos/types.h>

/* APIC registers */
#define APIC_DEFAULT_PHYS_BASE			0xfee00000

#define APIC_ID							0x20
#define APIC_LVR						0x30
#define		APIC_LVR_MASK				0xff00ff
#define		GET_APIC_VERSION(x)			((x)&0xff)
#define		GET_APIC_MAXLVT(x)			(((x)>>16)&0xff)
#define		APIC_INTEGRATED(x)			((x)&0xf0)
#define		APIC_XACPI(x)				((x)>=0x14)
#define APIC_TASKPRI					0x80
#define		APIC_TPRI_MASK				0xff
#define APIC_ABRPI						0x90
#define		APIC_ABRPRI_MASK			0xff
#define APIC_PROCPRI					0xa0
#define APIC_EOI						0xb0
#define		APIC_EIO_ACK				0x0
#define APIC_RRR						0xc0
#define APIC_LDR						0xd0
#define		APIC_LDR_MASK				(0xff<<24)
#define		GET_APIC_LOGICAL_ID(x)		(((x)>>24)&0xff)
#define		SET_APIC_LOGICAL_ID(x)		((x)<<24)
#define		APIC_ALL_CPUS				0xff
#define APIC_DFR						0xe0
#define		APIC_DFR_CLUSTER			0xfffffffful
#define		APIC_DFT_FLAT				0xfffffffful
#define APIC_SPIV						0xf0
#define		APIC_SPIV_APIC_ENABLED		0x100
#define		APIC_SPIV_FOCUS_DISABLED	0x200
#define APIC_ISR						0x100
#define APIC_TMR						0x180
#define APIC_IRR						0x200
#define APIC_ESR						0x280
#define		APIC_ESR_SEND_CS			0x00001
#define		APIC_ESR_RECV_CS			0x00002
#define		APIC_ESR_SEND_ACC			0x00004
#define		APIC_ESR_RECV_ACC			0x00008
#define		APIC_ESR_SEND_ILL			0x00020
#define		APIC_ESR_RECV_ILL			0x00040
#define		APIC_ESR_ILLREGA			0x00080
#define APIC_ICR						0x300
#define		APIC_DEST_SELF				0x40000
#define		APIC_DEST_ALLINC			0x80000
#define		APIC_DEST_ALLBUT			0xc0000
#define		APIC_ICR_RR_MASK			0x30000
#define		APIC_ICR_RR_INVALID			0x00000
#define		APIC_ICR_RR_INPROG			0x10000
#define		APIC_ICR_RR_VALID			0x20000
#define		APIC_ICR_LEVELTRIG			0x08000
#define		APIC_ICR_ASSERT				0x04000
#define		APIC_ICR_BUSY				0x01000
#define		APIC_DEST_LOGICAL			0x00800
#define		APIC_DM_FIXED				0x00000
#define		APIC_DM_LOWEST				0x00100
#define		APIC_DM_SMI					0x00200
#define		APIC_DM_REMRD				0x00300
#define		APIC_DM_NMI					0x00400
#define		APIC_DM_INIT				0x00500
#define		APIC_DM_STARTUP				0x00600
#define		APIC_DM_EXTINT				0x00700
#define		APIC_VECTOR_MASK			0x000ff
#define APIC_ICR2						0x310
#define		GET_APIC_DEST_FIELD(x)		(((x)>>24)&0xff)
#define		SET_APIC_DEST_FIELD(x)		((x)<<24)
#define APIC_LVTT						0x320
#define APIC_LVTTHMR					0x330
#define APIC_LVTPC						0x340
#define APIC_LVT0						0x350
#define		APIC_LVT_TIMER_BASE_MASK	(0x3<<18)
#define		GET_APIC_TIMER_BASE(x)		(((x)>>18)&0x03)
#define		SET_APIC_TIMER_BASE(x)		((x)<<18)
#define		APIC_TIMER_BASE_CLKIN		0x0
#define		APIC_TIMER_BASE_TMBASE		0x1
#define		APIC_TIMER_BASE_DIV			0x2
#define		APIC_LVT_TIMER_PERIODIC		(1<<17)
#define		APIC_LVT_MASKED				(1<<16)
#define		APIC_LVT_LEVEL_TRIGGER		(1<<15)
#define		APIC_LVT_REMOTE_IRR			(1<<14)
#define		APIC_INPUT_POLARITY			(1<<13)
#define		APIC_SEND_PENDING			(1<<12)
#define		APIC_MODE_MASK				0x700
#define		GET_APIC_DELIVERY_MODE(x)	(((x)>>8)&0x7)
#define		SET_APIC_DELIVERY_MODE(x,y)	(((x)&~0x700)|((y)<<8))
#define			APIC_MODE_FIXED			0x0
#define			APIC_MODE_NMI			0x4
#define			APIC_MODE_EXTINT		0x7
#define APIC_LVT1						0x360
#define APIC_LVTERR						0x370
#define APIC_TMICT						0x380
#define APIC_TMCCT						0x390
#define APIC_TDCR						0x3e0
#define		APIC_TDR_DIV_TMBASE			(1<<2)
#define		APIC_TDR_DIV_1				0xb
#define		APIC_TDR_DIV_2				0x0
#define		APIC_TDR_DIV_4				0x1
#define		APIC_TDR_DIV_8				0x2
#define		APIC_TDR_DIV_16				0x3
#define		APIC_TDR_DIV_32				0x8
#define		APIC_TDR_DIV_64				0x9
#define		APIC_TDR_DIV_128			0xa

/* APIC access */
extern vuint32 g_nApicAddr;

static inline uint32 apic_read( uint32 nReg )
{
	return *( (vuint32 *)(g_nApicAddr + nReg) );
}

static inline void apic_write( uint32 nReg, uint32 nVal )
{
	*( (vuint32 *)(g_nApicAddr + nReg) ) = nVal;
}

/* Acknowlege an APIC request */
static inline void apic_eoi( void )
{
	apic_write( APIC_EOI, 0 );
}

static __inline__ void apic_wait_icr_idle( void )
{
	while( apic_read( APIC_ICR ) & APIC_ICR_BUSY )
		/* EMPTY */;
}

/* Apparently this needs to be non-volatile or GCC will generate incorrect code */
static inline int get_processor_id( void )
{
	return GET_APIC_LOGICAL_ID( *( (uint32 *)(g_nApicAddr + APIC_ID ) ) );
}

bool test_local_apic( void );
void setup_local_apic( void );

void apic_send_ipi( int nTarget, uint32 nDeliverMode, int nIntNum );

int init_apic( bool bScanACPI );

#endif	/* __F_APIC_H__ */

