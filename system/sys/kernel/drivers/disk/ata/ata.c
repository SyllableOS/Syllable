/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2003 Kristian Van Der Vliet
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

#include "ata.h"
#include "ata-drive.h"
#include "ata-io.h"
#include "ata-probe.h"

int g_nDevID = -1;
int g_nDevices[MAX_DRIVES];
ata_controllers_t g_nControllers[MAX_CONTROLLERS];

status_t device_init( int nDeviceID )
{
	int controller;

	g_nDevID = nDeviceID;

	for( controller=0; controller < MAX_CONTROLLERS; controller++)
	{
		g_nControllers[controller].raw_data_buffer = (uint8*) alloc_real( ATA_BUFFER_SIZE + 65535, 0 );

		if ( g_nControllers[controller].raw_data_buffer == NULL )
		{
			kerndbg( KERN_PANIC, "device_init() : Failed to alloc IO buffer\n" );
			return( -ENOMEM );
		}

		g_nControllers[controller].data_buffer = (uint8*) (((uint32)g_nControllers[controller].raw_data_buffer + 65535) & ~65535);

		g_nControllers[controller].raw_dma_buffer = (uint8*) alloc_real( PAGE_SIZE / 2 + 4, 0 );
		if ( g_nControllers[controller].raw_dma_buffer == NULL )
		{
			kerndbg( KERN_PANIC, "device_init() : Failed to alloc DMA buffer\n" );
			free_real( g_nControllers[controller].raw_data_buffer );
			return( -ENOMEM );
		}

		g_nControllers[controller].dma_buffer = (uint8*) (((uint32)g_nControllers[controller].raw_dma_buffer + 4) & ~4);
 
 		g_nControllers[controller].buf_lock = create_semaphore( "ata_buffer_lock", 1, 0 );
		g_nControllers[controller].irq_lock = create_semaphore( "ata_irq_lock", 0, 0 );
		g_nControllers[controller].dma_active = 0;
 
		g_nControllers[controller].buf_lock = create_semaphore( "ata_buffer_lock", 1, 0 );

		if ( g_nControllers[controller].buf_lock < 0 )
		{
			kerndbg( KERN_PANIC, "device_init() : Failed to create buffer semaphore\n" );
			return( g_nControllers[0].buf_lock );
		}
	}

	ata_detect_pci_controllers();
	ata_init_controllers();
	ata_scan_for_disks( nDeviceID );

	return( 0 );
}

status_t device_uninit( int nDeviceID )
{
	int controller;

	for( controller = 0; controller < MAX_CONTROLLERS; controller++ )
	{
		free_real( g_nControllers[controller].raw_data_buffer );
		free_real( g_nControllers[controller].raw_dma_buffer );
		delete_semaphore( g_nControllers[controller].buf_lock );
	}

	return( 0 );
}

