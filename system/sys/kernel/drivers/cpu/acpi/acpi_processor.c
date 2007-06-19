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
#include <atheos/device.h>
#include <atheos/list.h>
#include <atheos/acpi.h>
#include <atheos/smp.h>
#include <atheos/tunables.h>
#include <atheos/resource.h>
#include <posix/errno.h>
#include <macros.h>
#include "pdc_intel.h"
#include "acpi_processor.h"

#define ACPI_PROCESSOR_COMPONENT	0x01000000
#define ACPI_PROCESSOR_CLASS		"processor"
#define ACPI_PROCESSOR_DRIVER_NAME	"ACPI Processor Driver"
#define ACPI_PROCESSOR_DEVICE_NAME	"Processor"
#define ACPI_PROCESSOR_FILE_INFO	"info"
#define ACPI_PROCESSOR_FILE_THROTTLING	"throttling"
#define ACPI_PROCESSOR_FILE_LIMIT	"limit"
#define ACPI_PROCESSOR_NOTIFY_PERFORMANCE 0x80
#define ACPI_PROCESSOR_NOTIFY_POWER	0x81

#define ACPI_PROCESSOR_LIMIT_USER	0
#define ACPI_PROCESSOR_LIMIT_THERMAL	1

#define ACPI_STA_PRESENT 0x00000001

#define _COMPONENT		ACPI_PROCESSOR_COMPONENT
ACPI_MODULE_NAME("acpi_processor")

static int acpi_processor_add( struct acpi_device *psDev );
static int acpi_processor_start( struct acpi_device *psDev );
static int acpi_processor_remove( struct acpi_device *psDev, int nType );

static struct acpi_driver acpi_processor_driver = {
	.name = ACPI_PROCESSOR_DRIVER_NAME,
	.class = ACPI_PROCESSOR_CLASS,
	.ids = ACPI_PROCESSOR_HID,
	.ops = {
		.add = acpi_processor_add,
		.remove = acpi_processor_remove,
		.start = acpi_processor_start,
		},
};



ACPIProc_s* g_apsProcessors[MAX_CPU_COUNT];
ACPI_bus_s* g_psBus;
static int g_nDeviceID;

static void read_cpu_id( unsigned int nReg, unsigned int *pData )
{
	/* Read CPU ID */
	__asm __volatile( "cpuid" : "=a"( pData[0] ), "=b"( pData[1] ), "=c"( pData[2] ), "=d"( pData[3] ):"0"( nReg ) );
}

static void acpi_processor_load_intel_pdc( ACPIProc_s* psProc, CPU_Extended_Info_s* psInfo )
{
	/* Check CPU vendor */
	unsigned int nRegs[4];
	char zVendor[17];
	struct acpi_object_list *psObjList;
	union acpi_object *psObj;
	uint32* pBuf;
	int nStatus = 0;
	
	read_cpu_id( 0x00000000, nRegs );
	sprintf( zVendor, "%.4s%.4s%.4s", ( char * )( nRegs + 1 ), ( char * )( nRegs + 3 ), ( char * )( nRegs + 2 ) );

	if( strcmp( "GenuineIntel", psInfo->anVendorID ) != 0 )
		return;
	
	
	/* BM check supported on all intel cpus */
	psProc->bBMCheck = true;
		
	read_cpu_id( 0x00000001, nRegs );
	
	/* Build pdc block */
	psObjList = kmalloc( sizeof( struct acpi_object_list ), MEMF_KERNEL | MEMF_CLEAR );
	psObj = kmalloc( sizeof( union acpi_object ), MEMF_KERNEL | MEMF_CLEAR );
	pBuf = kmalloc( 12, MEMF_KERNEL | MEMF_CLEAR );
	
	if( psObjList == NULL || psObj == NULL || pBuf == NULL )
		return;
		
	pBuf[0] = ACPI_PDC_REVISION_ID;
	pBuf[1] = 1;
	pBuf[2] = ACPI_PDC_C_CAPABILITY_SMP;

	if( nRegs[2] & ( 1 << 7 ) )
	{
		printk( "ACPI: Enhanced speed step detected\n" );
		pBuf[2] |= ACPI_PDC_EST_CAPABILITY_SWSMP;
		psProc->bSpeedStep = true;
	}

	psObj->type = ACPI_TYPE_BUFFER;
	psObj->buffer.length = 12;
	psObj->buffer.pointer = (uint8*)pBuf;
	psObjList->count = 1;
	psObjList->pointer = psObj;
	
	nStatus = g_psBus->evaluate_object( psProc->hHandle, "_PDC", psObjList, NULL);
	if( ACPI_FAILURE( nStatus ) )
		printk( "ACPI: Error: Failed to load PDC block\n" );
		
	printk( "ACPI: Intel PDC loaded\n" );
}

static void acpi_processor_notify( acpi_handle hHandle, uint32 nEvent, void *pData )
{
	ACPIProc_s* psProc = ( ACPIProc_s* )pData;
	struct acpi_device* psDevice;

	if( !psProc )
		return;

	if( g_psBus->get_device( psProc->hHandle, &psDevice ) )
		return;

	switch( nEvent ) {
	case ACPI_PROCESSOR_NOTIFY_PERFORMANCE:
		printk( "Error: ACPI processor ppc change!\n" );
		break;
	case ACPI_PROCESSOR_NOTIFY_POWER:
		printk( "Warning: ACPI processor cstate change!\n" );
		acpi_get_cstate_support( psProc );
		break;
	default:
		break;
	}

	return;
}


static int acpi_processor_start( struct acpi_device *psDev )
{
	acpi_status nStatus = 0;
	int nId = -1;
	ACPIProc_s* psProc = ( ACPIProc_s* )acpi_driver_data( psDev );
	int i;
	unsigned int nRegs[4];
	CPU_Extended_Info_s sInfo;
	
	union acpi_object sObject = { 0 };
	struct acpi_buffer sBuffer = { sizeof( union acpi_object ), &sObject };
	
	/* Evaluate object */
	nStatus = g_psBus->evaluate_object( psProc->hHandle, NULL, NULL, &sBuffer );
	if( ACPI_FAILURE( nStatus ) )
	{
		printk( "Error: Could not evaluate processor object!\n" );
		return( -ENODEV );
	}
	
	/* Get ACPI id */
	psProc->nAcpiId = sObject.processor.proc_id;
	
	/* Convert it to the APIC id */
	for( i = 0; i < MAX_CPU_COUNT; ++i )
	{
		int nAcpiId = get_processor_acpi_id( i );

		if( nAcpiId >= 0 && nAcpiId == psProc->nAcpiId )
		{
			nId = i;
			break;
		}
	}
	
	if( get_active_cpu_count() > 1 && nId == -1 )
	{
		printk( "Error: Could not find physical processor id\n" );
		return( -ENODEV );
	}
	
	/* BM check supported on uniprocessor machines */
	if( get_active_cpu_count() < 2 && g_psBus->get_fadt()->pm2_control_block && g_psBus->get_fadt()->pm2_control_length )
		psProc->bBMCheck = true;	

	if( nId == -1 )
		nId = get_processor_id();
		
	if( g_apsProcessors[nId] != NULL )
		return( 0 );

	/* Get CPU speed */
	if( get_cpu_extended_info( nId, &sInfo, CPU_EXTENDED_INFO_VERSION ) != 0 )
	{
		printk( "Error: Could not get processor info\n" );
		return( -ENODEV );
	}
	
	psProc->nId = nId;	
	psProc->nOrigCoreSpeed = sInfo.nCoreSpeed;
	psProc->nOrigDelayCount = sInfo.nDelayCount;
	psProc->nCurrentFreq = (int)( sInfo.nCoreSpeed / 1000000 );
	
	/* Get CPUID */
	psProc->nCPU = sInfo.nFamily;
	psProc->nCPUModel = sInfo.nModel;
		
	/* Check for constant TSC */
	if( strcmp( sInfo.anVendorID, "GenuineIntel" ) == 0 )
	{
		if( ( psProc->nCPU == 0xf && psProc->nCPUModel >= 0x03 ) ||
			( psProc->nCPU == 0x6 && psProc->nCPUModel >= 0x0e ) )
		{
			psProc->bConstantTSC = true;
		}
	}
	else if( strcmp( sInfo.anVendorID, "AuthenticAMD" ) == 0 )
	{
		read_cpu_id( 0x80000000, nRegs );
		if( nRegs[0] >= 0x80000007 )
		{
			read_cpu_id( 0x80000007, nRegs );
			if( nRegs[3] & ( 1 << 8 ) )
			{
				psProc->bConstantTSC = true;
			}
		}
	}

	printk( "ACPI: Processor [CPU ID: 0x%x%x APIC ID:%i ACPI ID:%i%s] found\n", psProc->nCPU, psProc->nCPUModel, psProc->nId, psProc->nAcpiId,
		psProc->bConstantTSC ? " CONSTANT TSC" : "" );
	
	/* Get PBLK address */
	if( !sObject.processor.pblk_address || sObject.processor.pblk_length != 6 )
	{
		printk( "ACPI: Processor has no valid PBLK\n" );
	}
	else
	{
		psProc->nPblkAddr = sObject.processor.pblk_address;
		printk( "ACPI: Processor PBLK @ 0x%x\n", (uint)psProc->nPblkAddr );
		
		request_region( psProc->nPblkAddr, 6, "ACPI PBLK" );
	}
	
	/* Add the processor to the array */
	g_apsProcessors[psProc->nId] = psProc;

				
	/* Load intel pdc */
	acpi_processor_load_intel_pdc( psProc, &sInfo );
	
	/* Check cstate abilities */
	acpi_get_cstate_support( psProc );
	
	/* Check pstate abilities */
	acpi_get_pstate_support( psProc );
	
	/* Install notify handler */
	g_psBus->install_notify_handler( psProc->hHandle, ACPI_DEVICE_NOTIFY,
							acpi_processor_notify, psProc );
	
	return( 0 );
}

static int acpi_processor_add( struct acpi_device *psDev )
{
	/* Create processor structure */
	ACPIProc_s* psProc;	
	if( psDev == NULL )
		return( -EINVAL );
	
	psProc = kmalloc( sizeof( ACPIProc_s ), MEMF_KERNEL | MEMF_CLEAR );
	if( psProc == NULL )
		return( -ENOMEM );
		
	psProc->hHandle = psDev->handle;
	strcpy( acpi_device_name( psDev ), ACPI_PROCESSOR_DEVICE_NAME );
	strcpy( acpi_device_class( psDev ), ACPI_PROCESSOR_CLASS );
	acpi_driver_data( psDev ) = psProc;
	
	return( 0 );
}


static int acpi_processor_remove( struct acpi_device* psDev, int nType )
{
	ACPIProc_s* psProc;	
	if( psDev == NULL || acpi_driver_data( psDev ) == NULL )
		return( -EINVAL );

	psProc = ( ACPIProc_s* )acpi_driver_data( psDev );
	
	if( psProc->nId >= MAX_CPU_COUNT )
	{
		kfree( psProc );
		return( 0 );
	}
	
	g_psBus->remove_notify_handler( psProc->hHandle, ACPI_DEVICE_NOTIFY,
								acpi_processor_notify );
	
	g_apsProcessors[psProc->nId] = NULL;
	kfree( psProc );
	
	return( 0 );
}


status_t acpi_processor_open( void* pNode, uint32 nFlags, void **pCookie )
{
    
    return( 0 );
}

status_t acpi_processor_close( void* pNode, void* pCookie )
{
   
    return( 0 );
}


status_t acpi_processor_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	/* TODO: Use processor id */
	switch( nCommand )
	{
		case CPU_GET_CURRENT_PSTATE:
		{
			CPUPerformanceState_s* psState = (CPUPerformanceState_s*)pArgs;
			ACPIProc_s* psProc = g_apsProcessors[get_processor_id()];
			if( psProc == NULL )
				return( -EINVAL );
			psState->cps_nFrequency = psProc->nCurrentFreq;
			break;
		}
		case CPU_GET_PSTATE:
		{
			CPUPerformanceState_s* psState = (CPUPerformanceState_s*)pArgs;
			ACPIProc_s* psProc = g_apsProcessors[get_processor_id()];
			if( psProc == NULL )
				return( -EINVAL );
			if( psState->cps_nIndex < 0 || psState->cps_nIndex >= psProc->nPStateCount )
				return( -EINVAL );
			
			psState->cps_nFrequency = psProc->apsPStates[psState->cps_nIndex].nCoreFreq;
			psState->cps_nPower = psProc->apsPStates[psState->cps_nIndex].nPower;
			break;
		}
		case CPU_SET_PSTATE:
		{
			CPUPerformanceState_s* psState = (CPUPerformanceState_s*)pArgs;
			ACPIProc_s* psProc = g_apsProcessors[get_processor_id()];
			if( psProc == NULL )
				return( -EINVAL );
			if( psState->cps_nIndex < 0 || psState->cps_nIndex >= psProc->nPStateCount )
				return( -EINVAL );
			
			return( acpi_set_global_pstate( psState->cps_nIndex ) );			
		}
		case CPU_GET_ISTATE:
		{
			CPUIdleState_s* psState = (CPUIdleState_s*)pArgs;
			ACPIProc_s* psProc = g_apsProcessors[get_processor_id()];
			if( psProc == NULL )
				return( -EINVAL );
			if( psState->cis_nIndex < 0 || psState->cis_nIndex >= psProc->nCStateCount )
				return( -EINVAL );
			
			psState->cis_nType = psProc->asCStates[psState->cis_nIndex].nType;
			psState->cis_nUsage = psProc->asCStates[psState->cis_nIndex].nUsage;
			break;
		}
		
		default:
			return( -EINVAL );
	}
    return( 0 );
}


static DeviceOperations_s g_sOperations = {
    acpi_processor_open,
    acpi_processor_close,
    acpi_processor_ioctl,
    NULL,
    NULL
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
    int nError;
    
    g_psBus = get_busmanager( ACPI_BUS_NAME, ACPI_BUS_VERSION );
    if( g_psBus == NULL )
    	return( -1 );
    
    /* Create device node */
    nError = create_device_node( nDeviceID, -1, "cpu/acpi", &g_sOperations, NULL );
    if( nError < 0 ) {
		return( nError );
	}

    
    g_nDeviceID = nDeviceID;
    memset( g_apsProcessors, 0, sizeof( g_apsProcessors ) );
    
    /* Notify the bios that we support cstates and pstates */
    if( g_psBus->get_fadt()->cst_control )
    {
    	outb( g_psBus->get_fadt()->cst_control, g_psBus->get_fadt()->smi_command );
    }
     if( g_psBus->get_fadt()->pstate_control )
    {
    	outb( g_psBus->get_fadt()->pstate_control, g_psBus->get_fadt()->smi_command );
    }
    
	nError = g_psBus->register_driver(&acpi_processor_driver);
	if( nError <= 0 ) {
		disable_device_on_bus( nDeviceID, ACPI_BUS_NAME );
		return( -1 );
	}
    
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
	set_idle_loop_handler( NULL );
	set_cpu_time_handler( NULL );
	g_psBus->unregister_driver(&acpi_processor_driver);
    return( 0 );
}


























