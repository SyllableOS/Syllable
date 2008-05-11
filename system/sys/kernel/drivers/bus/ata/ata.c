/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2004 Arno Klenke
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2002 William Rose
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *	Copyright (C) 2003 Red Hat, Inc.  All rights reserved.
 *	Copyright (C) 2003 Jeff Garzik
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

#include <atheos/threads.h>
#include <posix/signal.h>
#include "ata.h"
#include "ata_private.h"


bool g_bEnableLBA48bit = true;
bool g_bEnablePIO32bit = true;
bool g_bEnableDMA = true;
bool g_bEnableDirect = false;
bool g_bEnableProbeDebug = false;

ATA_controller_s* g_psFirstController;
static bool g_nIDTable[255];
static sem_id g_hATAListLock;

/* Return next port id */
int ata_get_next_id()
{
	int i;

	for ( i = 0; i < 255; i++ )
	{
		if ( g_nIDTable[i] == false )
		{
			g_nIDTable[i] = true;
			return ( i );
		}
	}
	return ( -1 );
}

/* Allocate one controller */
ATA_controller_s* ata_alloc_controller( int nDeviceID, int nChannels, int nPortsPerChannel )
{
	ATA_controller_s* psCtrl = kmalloc( sizeof( ATA_controller_s ), MEMF_KERNEL | MEMF_CLEAR );
	
	/* Set default values */
	psCtrl->nDeviceID = nDeviceID;
	psCtrl->nChannels = nChannels;
	psCtrl->nPortsPerChannel = nPortsPerChannel;
	strcpy( psCtrl->zName, "ATA controller" );
	
	return( psCtrl );
}

/* Allocate one port */
ATA_port_s* ata_alloc_port( ATA_controller_s* psCtrl )
{
	ATA_port_s* psPort = kmalloc( sizeof( ATA_port_s ), MEMF_KERNEL | MEMF_CLEAR );
	
	/* Set default values */
	psPort->psCtrl = psCtrl;
	psPort->nDeviceID = psCtrl->nDeviceID;
	
	
	psPort->sOps.reset = NULL;
	psPort->sOps.identify = ata_port_identify;
	psPort->sOps.select = ata_port_select;
	psPort->sOps.status = ata_port_status;
	psPort->sOps.altstatus = ata_port_altstatus;
	psPort->sOps.configure = ata_port_configure;
	psPort->sOps.prepare_dma_read = ata_port_prepare_dma_read;
	psPort->sOps.prepare_dma_write = ata_port_prepare_dma_write;
	psPort->sOps.start_dma = ata_port_start_dma;
	psPort->sOps.ata_cmd_ata = ata_cmd_ata;
	psPort->sOps.ata_cmd_atapi = ata_cmd_atapi;
	
	psPort->hPortLock = -1;
	psPort->hIRQWait = -1;
	
	return( psPort );
}

/* Add one controller to the busmanager */
status_t ata_add_controller( ATA_controller_s* psCtrl )
{
	int i, j;
	
	/* Check values and create missing items */
	for( i = 0; i < psCtrl->nChannels; i++ )
	{
		for( j = 0; j < psCtrl->nPortsPerChannel; j++ )
		{
			ATA_port_s* psPort = psCtrl->psPort[i*psCtrl->nPortsPerChannel+j];
		
			/* Check values */
			if( psPort->sOps.reset == NULL )
			{
				kerndbg( KERN_FATAL, "ata_add_controller(): Reset function not present\n" );
				return( -1 );
			}
			if( psPort->hPortLock < 0 || psPort->hIRQWait < 0 )
			{
				kerndbg( KERN_FATAL, "ata_add_controller(): Locks not created\n" );
			}
			
			/* Allocate port id */
			psPort->nID = ata_get_next_id();
			psPort->nChannel = i;
			psPort->nPort = j;
			psPort->pDataBuf = kmalloc( 16 * PAGE_SIZE + 4, MEMF_KERNEL );
			psPort->pDataBuf = ( uint8* )( ( (uint32)psPort->pDataBuf + 4 ) & ~3 );
		}
	}
	
	/* Probe ports */
	for( i = 0; i < psCtrl->nChannels * psCtrl->nPortsPerChannel; i++ )
	{
		/* Probe port */
		ata_probe_port( psCtrl->psPort[i] );
	}
	
	/* Configure ports */
	for( i = 0; i < psCtrl->nChannels * psCtrl->nPortsPerChannel; i++ )
	{
		ata_probe_configure_drive( psCtrl->psPort[i] );
		psCtrl->psPort[i]->sOps.configure( psCtrl->psPort[i] );
	}
		
	/* Create command thread  */
	for( i = 0; i < psCtrl->nChannels; i++ )
	{
		ata_cmd_init_buffer( psCtrl, i );
	}
	
	/* Add drives */
	for( i = 0; i < psCtrl->nChannels * psCtrl->nPortsPerChannel; i++ )
	{
		if( psCtrl->psPort[i]->nDevice == ATA_DEV_ATA )
			ata_drive_add( psCtrl->psPort[i] );
		if( psCtrl->psPort[i]->nDevice == ATA_DEV_ATAPI )
			atapi_drive_add( psCtrl->psPort[i] );
	}
	
	/* Add controller to list */
	LOCK( g_hATAListLock );
	psCtrl->psNext = g_psFirstController;
	g_psFirstController = psCtrl;
	UNLOCK( g_hATAListLock );
	
	return( 0 );
}

/* Unregister one controller */
void ata_remove_controller( ATA_controller_s* psCtrl )
{
	ATA_controller_s* psCurrent;
	ATA_controller_s* psPrev;
	int i, j;
	
	/* Search controller */
	LOCK( g_hATAListLock );
	psPrev = g_psFirstController;
	psCurrent = g_psFirstController;
	
	while( psCurrent != NULL )
	{
		if( psCurrent == psCtrl )
		{
			/* Remove port from list */
			if( psPrev == psCurrent )
				g_psFirstController = psCurrent->psNext;
			else
				psPrev->psNext = psCurrent->psNext;
				
				
			UNLOCK( g_hATAListLock );
			
			kerndbg( KERN_INFO, "%s removed\n", psCurrent->zName );
			
			/* Clean up ports */
			for( i = 0; i < psCurrent->nChannels; i++ )
			{
				sys_kill( psCurrent->psPort[i*psCurrent->nPortsPerChannel]->psCmdBuf->hThread, SIGKILL );
				delete_semaphore( psCurrent->psPort[i*psCurrent->nPortsPerChannel]->psCmdBuf->hLock );
				kfree( psCurrent->psPort[i*psCurrent->nPortsPerChannel]->psCmdBuf );
				for( j = 0; j < psCurrent->nPortsPerChannel; j++ )
				{
					kfree( psCurrent->psPort[i*psCurrent->nPortsPerChannel+j]->pDataBuf );
					g_nIDTable[psCurrent->psPort[i*psCurrent->nPortsPerChannel+j]->nID] = false;
				}
			}
			/* TODO: Remove drives */
			return;
		}
		
		psPrev = psCurrent;
		psCurrent = psCurrent->psNext;
	}
	kerndbg( KERN_FATAL, "ata_remove_controller(): Controller not found in ata_remove_controller()\n" );
	
	UNLOCK( g_hATAListLock );
}


void ata_free_controller( ATA_controller_s* psController )
{
	kfree( psController );
}

void ata_free_port( ATA_port_s* psPort )
{
	if( psPort->pPrivate || psPort->pDMATable )
	{
		kerndbg( KERN_FATAL, "ata_free_port(): Controller driver forgot to delete private data\n" );
	}
	kfree( psPort );
}



uint32 ata_get_controller_count()
{
	int nCount = 0;
	ATA_controller_s* psCurrent;
	
	LOCK( g_hATAListLock );
	
	psCurrent = g_psFirstController;
	
	while( psCurrent != NULL )
	{
		nCount++;
		psCurrent = psCurrent->psNext;
	}
	
	UNLOCK( g_hATAListLock );
	
	return( nCount );
}

ATA_controller_s* ata_get_controller( uint32 nController )
{
	int nCount = 0;
	ATA_controller_s* psCurrent;
	
	LOCK( g_hATAListLock );
	
	psCurrent = g_psFirstController;
	
	while( psCurrent != NULL )
	{
		if( nCount == nController )
		{
			UNLOCK( g_hATAListLock );
			return( psCurrent );
		}
		nCount++;
		psCurrent = psCurrent->psNext;
	}
	
	UNLOCK( g_hATAListLock );
	
	return( NULL );
}


ATA_bus_s sBus = {
	ata_alloc_controller,
	ata_free_controller,
	ata_add_controller,
	ata_remove_controller,
	ata_alloc_port,
	ata_free_port,
	ata_get_controller_count,
	ata_get_controller
};

void *ata_bus_get_hooks( int nVersion )
{
	if ( nVersion != ATA_BUS_VERSION )
		return ( NULL );
	return ( ( void * )&sBus );
}



status_t device_init( int nDeviceID )
{
	/* Check commandline parameters */
	int i;
	int argc;
	const char *const *argv;
	bool bDisableATA = false;

	get_kernel_arguments( &argc, &argv );

	for ( i = 0; i < argc; ++i )
	{
		if ( get_bool_arg( &bDisableATA, "disable_ata=", argv[i], strlen( argv[i] ) ) )
			if ( bDisableATA )
			{
				printk( "ATA bus disabled by user\n" );
				return ( -1 );
			}
		if ( get_bool_arg( &g_bEnableLBA48bit, "enable_ata_lba48=", argv[i], strlen( argv[i] ) ) )
			if ( !g_bEnableLBA48bit )
			{
				printk( "48bit LBA support disabled\n" );
			}
		if ( get_bool_arg( &g_bEnablePIO32bit, "enable_ata_pio32=", argv[i], strlen( argv[i] ) ) )
			if ( !g_bEnablePIO32bit )
			{
				printk( "32bit PIO mode disabled\n" );
			}
		if ( get_bool_arg( &g_bEnableDMA, "enable_ata_dma=", argv[i], strlen( argv[i] ) ) )
			if ( !g_bEnableDMA )
			{
				printk( "DMA transfers disabled\n" );
			}
		if ( get_bool_arg( &g_bEnableDirect, "enable_ata_direct=", argv[i], strlen( argv[i] ) ) )
			if ( g_bEnableDirect )
			{
				printk( "Command queueing disabled\n" );
			}
		if ( get_bool_arg( &g_bEnableProbeDebug, "enable_ata_probe_debug=", argv[i], strlen( argv[i] ) ) )
			if ( g_bEnableProbeDebug )
			{
				printk( "Probe debug output enabled\n" );
			}
	}
	
	/* Set default values */
	g_psFirstController = NULL;
	for ( i = 0; i < 255; i++ )
	{
		g_nIDTable[i] = false;
	}
	
	g_hATAListLock = create_semaphore( "ata_list_lock", 1, 0 );
	
	register_busmanager( nDeviceID, "ata", ata_bus_get_hooks );
	
	printk( "ATA busmanager initialized\n" );
	
	return( 0 );
}


status_t device_uninit( int nDeviceID )
{
	return( -1 );
}























































