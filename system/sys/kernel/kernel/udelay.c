
/*
 *	Precise Delay Loops for i386
 *
 *	Copyright (C) 1993 Linus Torvalds
 *	Copyright (C) 1997 Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 *
 *	The __delay function must _NOT_ be inlined as its execution time
 *	depends wildly on alignment on many x86 processors. The additional
 *	jump magic is needed to get the timing stable on all the CPU's
 *	we have to worry about.
 *
 *	Modified to fit into the AtheOS kernel by Kurt Skauen 08 Jan 2000
 */

#include <atheos/udelay.h>
#include <atheos/smp.h>

#include "inc/smp.h"


void __delay( unsigned long loops )
{
	int d0;
	__asm__ __volatile__( "\tjmp 1f\n" ".align 16\n" "1:\tjmp 2f\n" ".align 16\n" "2:\tdecl %0\n\tjns 2b":"=&a"( d0 ):"0"( loops ) );
}

inline void __const_udelay( unsigned long xloops )
{
	int d0;

      __asm__( "mull %0": "=d"( xloops ), "=&a"( d0 ):"1"( xloops ), "0"( g_asProcessorDescs[get_processor_id()].pi_nDelayCount ) );
	__delay( xloops );
}

void __udelay( unsigned long usecs )
{
	__const_udelay( usecs * 0x000010c6 );	/* 2**32 / 1000000 */
}
