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
#include <atheos/time.h>
#include <atheos/tunables.h>
#include <atheos/resource.h>
#include <posix/errno.h>
#include <macros.h>
#include "acpi_processor.h"

extern ACPIProc_s* g_apsProcessors[MAX_CPU_COUNT];
static struct acpi_table_fadt* g_psFadt = NULL;

#define C2_OVERHEAD	4
#define C3_OVERHEAD	4


static inline uint32 ticks_elapsed( u32 t1, u32 t2 )
{
	if( t2 >= t1 )
		return( t2 - t1 );
	else if (!(g_psFadt->flags & ACPI_FADT_32BIT_TIMER))
		return (((0x00FFFFFF - t1) + t2) & 0x00FFFFFF);
	else
		return ((0xFFFFFFFF - t1) + t2);
}


void acpi_cstate_idle( int nProcessor )
{
	ACPIProc_s* psProc = g_apsProcessors[nProcessor];
	int nSleepTicks = 0xFFFFFFFF;
	static bigtime_t nTime = 0;
	uint32 nThreshold = 1;
	
	if( psProc == NULL || g_psFadt == NULL )
	{
		printk( "ACPI: Error: Processor not detected but idle handler called!\n" );
		set_idle_loop_handler( NULL );
		return;
	}
	
	if( get_system_time() - nTime > 5000000 )
	{
		//printk( "%i %i %i\n", psProc->asCStates[0].nUsage, psProc->asCStates[1].nUsage, psProc->asCStates[2].nUsage );
		nTime = get_system_time();
	}
	
	uint32 nFlags = cli();
	
	ACPICState_s* psCState = &psProc->asCStates[psProc->nCurrentCState];
	
	/* Check for busmaster activity */

	if( psProc->bBMCheck )
	{
		uint32 nBMStatus;
		bigtime_t nDiff = ( get_system_time() - psProc->nLastBMCheck ) / 1000;
		if( nDiff > 31 )
			nDiff = 31;
		psProc->nBMHistory <<= nDiff;
		
		g_psBus->get_register( ACPI_BITREG_BUS_MASTER_STATUS, &nBMStatus );
				  
		psProc->nLastBMCheck = get_system_time();
		if( nBMStatus ) {
			psProc->nBMHistory |= 1;
			g_psBus->set_register( ACPI_BITREG_BUS_MASTER_STATUS, 1 );
			if( psCState->nType == ACPI_STATE_C3 )
			{
				g_psBus->set_register( ACPI_BITREG_BUS_MASTER_RLD, 0 );
	 			//printk( "Busmaster activity %i!\n", psCState->nType );
			
				if( psProc->nCurrentCState > 0 )
					psProc->nCurrentCState--;
				psProc->nPromotionCount = 0;
				put_cpu_flags( nFlags );
				return;
			}
		}
		
	}

	switch( psCState->nType )
	{
		case ACPI_STATE_C1:
		{
			put_cpu_flags( nFlags );
			__asm__ volatile ( "hlt" );
			nThreshold = 5;
			break;			
		}
		case ACPI_STATE_C2:
		{
			uint32 nT1 = inl( g_psFadt->xpm_timer_block.address );
			uint32 nTemp, nT2;
			inb( psCState->nAddr );
			nTemp = inl( g_psFadt->xpm_timer_block.address );
			nT2 = inl( g_psFadt->xpm_timer_block.address );
			
			nThreshold = 15;
			nSleepTicks = ticks_elapsed( nT1, nT2 ) - psCState->nLatency - C2_OVERHEAD;
			break;
		}
		case ACPI_STATE_C3:
		{
			if( !psProc->bBMCheck )
			{
				printk( "ACPI: Error: Tried to enter C3 mode on unsupported platform!\n" );
				set_idle_loop_handler( NULL );
				goto end;
			}

			
			if( atomic_inc_and_read( &psProc->nC3CpuCount ) == get_active_cpu_count() - 1 )
			{
				g_psBus->set_register( ACPI_BITREG_ARB_DISABLE, 1 );
			}
			uint32 nTemp, nT2;
			uint32 nT1 = inl( g_psFadt->xpm_timer_block.address );
			inb( psCState->nAddr );
			nTemp = inl( g_psFadt->xpm_timer_block.address );
			nT2 = inl( g_psFadt->xpm_timer_block.address );

			nSleepTicks = ticks_elapsed( nT1, nT2 ) - psCState->nLatency - C3_OVERHEAD;

			atomic_dec( &psProc->nC3CpuCount );
			g_psBus->set_register( ACPI_BITREG_ARB_DISABLE, 0 );
			

			break;
		}
	}
	
	psCState->nUsage++;
	
	/* Promotion */
	if( psProc->nCurrentCState + 1 < psProc->nCStateCount )
	{
		ACPICState_s* psNextCState = &psProc->asCStates[psProc->nCurrentCState + 1];
		
		if( nSleepTicks > psNextCState->nLatency )
		{
			if( psProc->nPromotionCount++ > nThreshold )
			{
				if( psNextCState->nType == ACPI_STATE_C3
					&& psProc->nBMHistory )
					goto end;
				psProc->nCurrentCState++;
				psProc->nPromotionCount = 0;
				if( psNextCState->nType == ACPI_STATE_C3 )
				{
					g_psBus->set_register( ACPI_BITREG_BUS_MASTER_RLD, 1 );
				}
			}
			goto end;
		}
	}
	
	/* Demotion */
	if( psProc->nCurrentCState > 0 )
	{
		ACPICState_s* psNextCState = &psProc->asCStates[psProc->nCurrentCState - 1];
		
		if( nSleepTicks < psNextCState->nLatency )
		{
			psProc->nCurrentCState--;
			psProc->nPromotionCount = 0;
			if( psCState->nType == ACPI_STATE_C3 )
			{
				g_psBus->set_register( ACPI_BITREG_BUS_MASTER_RLD, 0 );
			}
			goto end;
		}
	}
	
end:	
	
	

	if( psCState->nType != ACPI_STATE_C1 )
		put_cpu_flags( nFlags );
	//printk( "Next state %i %i\n", nSleepTicks, psProc->nCurrentCState );
}

static bigtime_t g_nLastTime = 0;
static uint32 g_nLastAPMTime = 0;

bigtime_t acpi_get_cpu_time( int nProc )
{
	uint64 nElapsed;
	uint32 nCurrentAPMTime = inl( g_psFadt->xpm_timer_block.address );
	nElapsed = ticks_elapsed( g_nLastAPMTime, nCurrentAPMTime );
	g_nLastAPMTime = nCurrentAPMTime;
	g_nLastTime += nElapsed * 1000000 / PM_TIMER_FREQUENCY;
	
	return( g_nLastTime );
}

void acpi_get_cstate_support( ACPIProc_s* psProc )
{
	acpi_status nStatus = 0;
	struct acpi_buffer sBuffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *psCst;
	acpi_integer nCount;
	int i;
	bool bWarnAboutC3 = false;
	
	g_psFadt = g_psBus->get_fadt();
	
	set_cpu_time_handler( acpi_get_cpu_time );
	
	/* Add C1 state */
	psProc->asCStates[psProc->nCStateCount].nType = ACPI_STATE_C1;
	psProc->asCStates[psProc->nCStateCount].nLatency = 1;
	psProc->asCStates[psProc->nCStateCount].nAddr = 0;
	psProc->nCStateCount++;
	
	/* Try to evaluate _CST object */
	nStatus = g_psBus->evaluate_object( psProc->hHandle, "_CST", NULL, &sBuffer );
	if( ACPI_FAILURE( nStatus ) )
	{
		/* Try to use PBLK address */
		if( !psProc->nPblkAddr )
			return;
			
			
		/* Check latency of C2 mode */
		if( g_psFadt->C2latency > ACPI_PROCESSOR_MAX_C2_LATENCY )
			return;
		
		psProc->asCStates[psProc->nCStateCount].nType = ACPI_STATE_C2;
		psProc->asCStates[psProc->nCStateCount].nLatency = g_psFadt->C2latency;
		psProc->asCStates[psProc->nCStateCount].nAddr = psProc->nPblkAddr + 4;
		psProc->nCStateCount++;
		
		/* Check latency of C3 mode */
		if( g_psFadt->C3latency > ACPI_PROCESSOR_MAX_C3_LATENCY )
			goto end;
			

		if( !psProc->bBMCheck ) {
			bWarnAboutC3 = true;
			goto end;
		}

		
		psProc->asCStates[psProc->nCStateCount].nType = ACPI_STATE_C3;
		psProc->asCStates[psProc->nCStateCount].nLatency = g_psFadt->C3latency;
		psProc->asCStates[psProc->nCStateCount].nAddr = psProc->nPblkAddr + 5;
		psProc->nCStateCount++;
		
		g_psBus->set_register( ACPI_BITREG_BUS_MASTER_RLD, 0 );
		
		goto end;
	}
	
	psCst = (union acpi_object *)sBuffer.pointer;

	/* There must be at least 2 elements */
	if( !psCst || ( psCst->type != ACPI_TYPE_PACKAGE) || psCst->package.count < 2 ) {
		printk( "ACPI: Error: not enough elements in _CST\n" );
		return;
	}
	

	nCount = psCst->package.elements[0].integer.value;
	
	if( nCount < 1 || nCount != psCst->package.count ) {
		printk( "ACPI: Error: _CST invalid\n" );
		return;
	}
	
	for( i = 1; i <= nCount; i++ )
	{
		union acpi_object* psElement;
		union acpi_object* psObj;
		ACPIPowerRegister_s* psReg;
		acpi_io_address nAddr;
		
		psElement = (union acpi_object *)&( psCst->package.elements[i] );
		if( psElement->type != ACPI_TYPE_PACKAGE )
			continue;

		if( psElement->package.count != 4 )
			continue;

		psObj = ( union acpi_object* )&( psElement->package.elements[0] );

		if( psObj->type != ACPI_TYPE_BUFFER )
			continue;

		psReg = ( ACPIPowerRegister_s* )psObj->buffer.pointer;

		if( psReg->nSpaceId != ACPI_ADR_SPACE_SYSTEM_IO )
			continue;
			
		nAddr = psReg->nAddress;
				
		psObj = ( union acpi_object* )&( psElement->package.elements[1] );
		if( psObj->type != ACPI_TYPE_INTEGER )
			continue;

		psProc->asCStates[psProc->nCStateCount].nType = psObj->integer.value;
		if( psObj->integer.value != ACPI_STATE_C2 && psObj->integer.value != ACPI_STATE_C3 )
			continue;
			
		psObj = ( union acpi_object* )&( psElement->package.elements[2] );
		if( psObj->type != ACPI_TYPE_INTEGER )
			continue;
			
		if( psProc->asCStates[psProc->nCStateCount].nType == ACPI_STATE_C2 &&
			psObj->integer.value > ACPI_PROCESSOR_MAX_C2_LATENCY )
			continue;
		
		if( psProc->asCStates[psProc->nCStateCount].nType == ACPI_STATE_C3 &&
			( psObj->integer.value > ACPI_PROCESSOR_MAX_C3_LATENCY ) )
			continue;
		
		if( psProc->asCStates[psProc->nCStateCount].nType == ACPI_STATE_C3 &&
			!psProc->bBMCheck ) {
			bWarnAboutC3 = true;
			continue;
		}

		
		psProc->asCStates[psProc->nCStateCount].nIndex = i;
		psProc->asCStates[psProc->nCStateCount].nLatency = psObj->integer.value;
		psProc->asCStates[psProc->nCStateCount].nAddr = psReg->nAddress;
		psProc->nCStateCount++;
		
		if( psProc->nCStateCount == ACPI_MAX_C_STATES )
			break;
	}
	
	kfree( sBuffer.pointer );
	
	if( psProc->nCStateCount == 1 )
	{
		printk( "ACPI: No support for ACPI C states detected\n" );
		psProc->nCStateCount = 0;
		return;
	}
	
end:
	for( i = 0; i < psProc->nCStateCount; i++ )
	{
		printk( "ACPI: C%i state latency[%03d] address[0x%x]\n",
				psProc->asCStates[i].nType,
				psProc->asCStates[i].nLatency, psProc->asCStates[i].nAddr );
		if( psProc->asCStates[i].nType == ACPI_STATE_C3 )
		{

			g_psBus->set_register( ACPI_BITREG_BUS_MASTER_RLD, 0 );

		}
	}
	
	if( bWarnAboutC3 )
	{
		printk( "ACPI: Warning: C3 states detected but cannot be used\n" );
	}
	set_idle_loop_handler( acpi_cstate_idle );
}

















































































