/*
 *  The Syllable kernel
 *  Intel cpuid support
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

#include "inc/cputable.h"

bool g_bHasFXSR = false;
bool g_bHasXMM = false;

extern uint32 g_anKernelStackEnd[];


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
static void read_cpu_id( unsigned int nReg, unsigned int *pData )
{
	/* Read CPU ID */
	__asm __volatile( "cpuid" : "=a"( pData[0] ), "=b"( pData[1] ), "=c"( pData[2] ), "=d"( pData[3] ):"0"( nReg ) );
}


//****************************************************************************/
/** Reads the cpuid and enables features like SSE.
 * Called by init_kernel() (init.c).
 * \internal
 * \ingroup CPU
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void init_cpuid( void )
{
	/* Save cpu name and features ( from mplayer ) */
	unsigned int nRegs[4];
	unsigned int nRegs2[4];
	char zVendor[17];
	int i;
	uint nCPUid = 0;
	
	for ( i = 0; i < MAX_CPU_COUNT; i++ )
	{
		strcpy( g_asProcessorDescs[i].pi_zName, "Unknown" );
		g_asProcessorDescs[i].pi_nFeatures = CPU_FEATURE_NONE;
	}


	/* Warning : We do not check if the CPU supports CPUID
	   ( Not a problem because all CPUs > 486 support it ) */
	read_cpu_id( 0x00000000, nRegs );
	if ( nRegs[0] >= 0x00000001 )
	{
		read_cpu_id( 0x00000001, nRegs2 );
		if ( nRegs2[3] & ( 1 << 23 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_MMX;
		if ( nRegs2[3] & ( 1 << 24 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_FXSAVE;
		if ( nRegs2[3] & ( 1 << 25 ) ) {
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_SSE;
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_MMX2;
		}
		if ( nRegs2[3] & ( 1 << 26 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_SSE2;
		
		if ( nRegs2[3] & ( 1 << 9 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_APIC;
		
#define CPUID_FAMILY	( ( nRegs2[0] >> 8 ) & 0x0F )
#define CPUID_MODEL		( ( nRegs2[0] >> 4 ) & 0x0F )

		/* Find out CPU name */
		strcpy( g_asProcessorDescs[g_nBootCPU].pi_zName, "Unknown" );
		sprintf( zVendor, "%.4s%.4s%.4s", ( char * )( nRegs + 1 ), ( char * )( nRegs + 3 ), ( char * )( nRegs + 2 ) );
		for ( i = 0; i < MAX_VENDORS; i++ )
		{
			if ( !strcmp( cpuvendors[i].string, zVendor ) )
			{
				if ( cpuname[i][CPUID_FAMILY][CPUID_MODEL] )
				{
					nCPUid = ( CPUID_FAMILY << 4 ) | CPUID_MODEL;
					sprintf( g_asProcessorDescs[g_nBootCPU].pi_zName, "%s %s", cpuvendors[i].name, cpuname[i][CPUID_FAMILY][CPUID_MODEL] );
				}
			}
		}
	}
	read_cpu_id( 0x80000000, nRegs );
	if ( nRegs[0] >= 0x80000001 )
	{
		read_cpu_id( 0x80000001, nRegs2 );
		if ( nRegs2[3] & ( 1 << 23 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_MMX;
		if ( nRegs2[3] & ( 1 << 22 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_MMX2;
		if ( nRegs2[3] & ( 1 << 31 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_3DNOW;
		if ( nRegs2[3] & ( 1 << 30 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_3DNOWEX;
	}
	/* Copy this information to all other CPUs */
	for ( i = 0; i < MAX_CPU_COUNT; i++ )
	{
		if ( i == g_nBootCPU )
			continue;
		strcpy( g_asProcessorDescs[i].pi_zName, g_asProcessorDescs[g_nBootCPU].pi_zName );
		g_asProcessorDescs[i].pi_nFeatures = g_asProcessorDescs[g_nBootCPU].pi_nFeatures;
	}
	kerndbg( KERN_INFO, "CPU: %s (0x%x)\n", g_asProcessorDescs[g_nBootCPU].pi_zName, nCPUid );
	if ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_MMX )
		kerndbg( KERN_DEBUG, "MMX supported\n" );
	if ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_MMX2 )
		kerndbg( KERN_DEBUG, "MMX2 supported\n" );
	if ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_3DNOW )
		kerndbg( KERN_DEBUG, "3DNOW supported\n" );
	if ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_3DNOWEX )
		kerndbg( KERN_DEBUG, "3DNOWEX supported\n" );
	if ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_FXSAVE )
	{
		// enable fast FPU save and restore
		g_bHasFXSR = g_asProcessorDescs[g_nBootCPU].pi_bHaveFXSR = true;
		set_in_cr4( X86_CR4_OSFXSR );
		kerndbg( KERN_DEBUG, "FXSAVE supported\n" );

		if ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_SSE )
		{
			// enable SSE support
			g_bHasXMM = g_asProcessorDescs[g_nBootCPU].pi_bHaveXMM = true;
			set_in_cr4( X86_CR4_OSXMMEXCPT );
			kerndbg( KERN_DEBUG, "SSE supported\n" );
		}
		if ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_SSE2 )
			kerndbg( KERN_DEBUG, "SSE2 supported\n" );
	}
	if ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_APIC )
		kerndbg( KERN_DEBUG, "APIC present\n" );
}
