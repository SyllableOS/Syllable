/*
 * ACPI processor driver
 *
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *  Copyright (C) 2004, 2005 Dominik Brodowski <linux@brodo.de>
 *  Copyright (C) 2004  Anil S Keshavamurthy <anil.s.keshavamurthy@intel.com>
 *  			- Added processor hotplug support
 *  Copyright (C) 2005  Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>
 *  			- Added support for C3 on SMP
 *  Copyright (C) 2007 Arno Klenke
 *				- Syllable port
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <atheos/kernel.h>
#include <atheos/irq.h>
#include <atheos/types.h>
#include <atheos/list.h>
#include <atheos/device.h>
#include <atheos/acpi.h>
#include <atheos/tunables.h>
#include <atheos/resource.h>
#include <atheos/threads.h>
#include <posix/errno.h>
#include <macros.h>
#include "acpi_processor.h"


#define MSR_IA32_PERF_STATUS		0x198
#define MSR_IA32_PERF_CTL			0x199

#define rdmsr( nReg, nLow, nHigh ) \
	__asm__ __volatile__( "rdmsr" \
			  : "=a" ( nLow ), "=d" ( nHigh ) \
			  : "c" ( nReg ) )

#define wrmsr( nReg, nLow, nHigh ) \
	__asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" ( nReg ), "a" ( nLow ), "d" ( nHigh ) )
			  
extern ACPIProc_s* g_apsProcessors[MAX_CPU_COUNT];


void acpi_get_pstate_support( ACPIProc_s* psProc )
{
	acpi_status nStatus = 0;
	struct acpi_buffer sBuffer = { ACPI_ALLOCATE_BUFFER, NULL };
	struct acpi_buffer sPssBuffer = { ACPI_ALLOCATE_BUFFER, NULL };
	struct acpi_buffer sStateBuffer = { 0, NULL };
	union acpi_object *psPct;
	union acpi_object sObj = { 0 };
	union acpi_object *psPss;
	struct acpi_buffer sFormat = { sizeof( "NNNNNN" ), "NNNNNN" };
	int i;
	
	/* Try to evaluate _PCT object */
	nStatus = g_psBus->evaluate_object( psProc->hHandle, "_PCT", NULL, &sBuffer );
	if( ACPI_FAILURE( nStatus ) )
	{
		return;
	}
	
	psPct = (union acpi_object *)sBuffer.pointer;
	if( !psPct || ( psPct->type != ACPI_TYPE_PACKAGE) || psPct->package.count < 2 ) {
		printk( "ACPI: Error: not enough elements in _PCT\n" );
		return;
	}

	/* Control register */
	sObj = psPct->package.elements[0];

	if( ( sObj.type != ACPI_TYPE_BUFFER )
		|| ( sObj.buffer.length < sizeof( ACPIPctRegister_s ) )
		|| ( sObj.buffer.pointer == NULL)) {
		printk( "ACPI: Error: Invalid _PCT data\n");
		return;
	}
	memcpy( &psProc->sPCtrlReg, sObj.buffer.pointer,
		sizeof( ACPIPctRegister_s ) );
	
	/* Check that we support this */
	if( psProc->sPCtrlReg.nSpaceId != ACPI_ADR_SPACE_SYSTEM_IO
		&& !( psProc->sPCtrlReg.nSpaceId == ACPI_ADR_SPACE_FIXED_HARDWARE
		&& psProc->bSpeedStep ) )
	{
		printk( "ACPI: Warning: pstates not supported on this hardware\n" );
		return;
	}
	 
	
	/* Status register */
	sObj = psPct->package.elements[1];

	if( ( sObj.type != ACPI_TYPE_BUFFER )
		|| ( sObj.buffer.length < sizeof( ACPIPctRegister_s ) )
		|| ( sObj.buffer.pointer == NULL)) {
		printk( "ACPI: Error: Invalid _PCT data\n");
		return;
	}
	memcpy( &psProc->sPStsReg, sObj.buffer.pointer,
	       sizeof( ACPIPctRegister_s ) );
	kfree( sBuffer.pointer );
	       
	/* Performance states */
	nStatus = g_psBus->evaluate_object( psProc->hHandle, "_PSS", NULL, &sPssBuffer );
	if( ACPI_FAILURE( nStatus ) )
	{
		return;
	}
	
	psPss = (union acpi_object *)sPssBuffer.pointer;
	if( !psPss || ( psPss->type != ACPI_TYPE_PACKAGE) ) {
		printk( "ACPI: Error: Invalid _PSS object\n" );
		return;
	}
	
	psProc->nPStateCount = psPss->package.count;
	psProc->apsPStates = kmalloc( sizeof( ACPIPerfState_s ) * psProc->nPStateCount, MEMF_KERNEL | MEMF_CLEAR );
	for( i = 0; i < psProc->nPStateCount; i++ )
	{
		ACPIPerfState_s* psPState = &psProc->apsPStates[i];
		sStateBuffer.length = sizeof( ACPIPerfState_s );
		sStateBuffer.pointer = psPState;
		
		nStatus = g_psBus->extract_package( &( psPss->package.elements[i] ), &sFormat, &sStateBuffer );
		if( ACPI_FAILURE( nStatus ) )
		{
			printk( "Failed to extract pstate object\n" );
			psProc->nPStateCount = 0;
			kfree( psProc->apsPStates );
			return;
		}
		
		printk( "ACPI: State [%d]: core_frequency[%d] power[%d] transition_latency[%d] bus_master_latency[%d] control[0x%x] status[0x%x]\n",
				  i,
				  (uint32) psPState->nCoreFreq,
				  (uint32) psPState->nPower,
				  (uint32) psPState->nTransLatency,
				  (uint32) psPState->nBusMasterLatency,
				  (uint32) psPState->nControl, (u32)psPState->nStatus );
	}
	
	kfree( sPssBuffer.pointer );
	
	/* Check how to switch to different frequencies */
	if( psProc->nPStateCount <= 1 )
	{
		psProc->nPStateCount = 0;
		return;
	}
	
	if( psProc->sPCtrlReg.nSpaceId != psProc->sPStsReg.nSpaceId )
		return;
	
	if( psProc->sPCtrlReg.nSpaceId == ACPI_ADR_SPACE_SYSTEM_IO )
		printk( "ACPI: Using IO to switch between frequencies\n" );
	else if( psProc->sPCtrlReg.nSpaceId == ACPI_ADR_SPACE_FIXED_HARDWARE )
	{
		uint32 nHigh;
		uint32 nVal;
		
		if( !psProc->bSpeedStep )
			return;
		/* Get current frequency */
		rdmsr( MSR_IA32_PERF_STATUS, nVal, nHigh );
		nVal &= 0xffff;
		for( i = 0; i < psProc->nPStateCount; i++ )
		{
			ACPIPerfState_s* psPState = &psProc->apsPStates[i];
			if( psPState->nStatus == nVal )
			{
				psProc->nOrigCoreSpeed = psPState->nCoreFreq * 1000 * 1000;
				break;
			}
		}
		
		/* Update delay count */

		
		printk( "ACPI: Using MSR registers to switch between frequencies (current %iMhz)\n", (int)( psProc->nOrigCoreSpeed / 1000 / 1000 ) );
		
	}
}

/* TODO: Need to read the ACPI tables to find out if we need to switch
			freqs for every CPU */

status_t acpi_set_pstate( int nProc, int nIndex )
{
	ACPIProc_s* psProc = g_apsProcessors[nProc];
	ACPIPerfState_s* psState;
	uint32 nFlags;
	uint64 nNewDelayCount;
	if( psProc == NULL )
		return( -EINVAL );
	
	if( nIndex < 0 || nIndex >= psProc->nPStateCount )
		return( -EINVAL );
	
	psState = &psProc->apsPStates[nIndex];
	

	nFlags = cli();
	set_thread_target_cpu( sys_get_thread_id( NULL ), nProc );
	Schedule();
	if( get_processor_id() != nProc )
	{
		printk( "ACPI: Could not switch to the target CPU!" );
		put_cpu_flags( nFlags );
		return( -EINVAL );		
	}
	
	printk( "Switching to %i MHz on CPU %i\n", (int)psState->nCoreFreq, nProc );
	
	/* Calculate new speed */
	nNewDelayCount = psState->nCoreFreq * 1000 * psProc->nOrigDelayCount;
	nNewDelayCount /= ( psProc->nOrigCoreSpeed / 1000 );
	psProc->nCurrentFreq = psState->nCoreFreq;
	
	//printk( "Delay %i %i\n", (int)psProc->nOrigDelayCount, (int)nNewDelayCount );
			
	
	if( psProc->sPCtrlReg.nSpaceId == ACPI_ADR_SPACE_SYSTEM_IO )
	{
		switch( psProc->sPCtrlReg.nBitWidth )
		{
		case 8:
			outb(psState->nControl, psProc->sPCtrlReg.nAddress);
		break;
		case 16:
			outw(psState->nControl, psProc->sPCtrlReg.nAddress);
			break;
		case 32:
			outl(psState->nControl, psProc->sPCtrlReg.nAddress);
			break;
		default:
			break;
		}
	}
	else
	{
		uint32 nVal;
		uint32 nHigh;
		rdmsr( MSR_IA32_PERF_STATUS, nVal, nHigh );
		nVal &= ~0xffff;
		nVal |= ( psState->nControl & 0xffff );
		wrmsr( MSR_IA32_PERF_CTL, nVal, nHigh );
	}

	update_cpu_speed( nProc, psProc->bConstantTSC ? psProc->nOrigCoreSpeed : (uint64)psState->nCoreFreq * 1000 * 1000, nNewDelayCount );

	set_thread_target_cpu( sys_get_thread_id( NULL ), -1 );
	
	put_cpu_flags( nFlags );
	return( 0 );
}

status_t acpi_set_global_pstate( int nIndex )
{
	ACPIProc_s* psProc = g_apsProcessors[get_processor_id()];
	int i;
	if( psProc == NULL )
		return( -EINVAL );
	
	for( i = 0; i < MAX_CPU_COUNT; i++ )
	{
		if( g_apsProcessors[i] != NULL )
		{
			status_t nResult = acpi_set_pstate( i, nIndex );
			if( nResult != 0 )
				return( nResult );
		}
	}
	return( 0 );	
}












