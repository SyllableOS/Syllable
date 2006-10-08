
/*
 *  The Syllable kernel
 *  PCI busmanager ( ACPI PCI IRQ router )
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *  Copyright (C) 2002       Dominik Brodowski <devel@brodo.de>
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  
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


#include <atheos/kernel.h>
#include <atheos/irq.h>
#include <atheos/types.h>
#include <atheos/list.h>
#include <atheos/msgport.h>
#include <atheos/device.h>
#include <atheos/acpi.h>
#include <posix/errno.h>
#include <macros.h>

#include "pci_internal.h"

#define ACPI_PCI_ROOT_CLASS		"pci_bridge"
#define ACPI_PCI_ROOT_HID		"PNP0A03"
#define ACPI_PCI_ROOT_DRIVER_NAME	"ACPI PCI Root Bridge Driver"
#define ACPI_PCI_ROOT_DEVICE_NAME	"PCI Root Bridge"

static int acpi_pci_root_add (struct acpi_device *device);
static int acpi_pci_root_remove (struct acpi_device *device, int type);

static struct acpi_driver g_sACPIRootDriver = {
	.name =		ACPI_PCI_ROOT_DRIVER_NAME,
	.class =	ACPI_PCI_ROOT_CLASS,
	.ids =		ACPI_PCI_ROOT_HID,
	.ops =		{
				.add =    acpi_pci_root_add,
				.remove = acpi_pci_root_remove,
			},
};

struct RoutingTableEntry_s
{
	acpi_handle	nHandle;
	int			nSegment;	
	int			nBus;
	struct acpi_pci_routing_table* psEntry;
};

static struct RoutingTableEntry_s g_sRoutingEntries[128];
static int g_nNumRoutingEntries = 0;
static ACPI_bus_s* g_psBus = NULL;

#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERN_DEBUG

static void eisa_set_edge_irq( unsigned int nIRQ )
{
	unsigned char nMask = 1 << ( nIRQ & 7 );
	unsigned int nPort = 0x4d0 + ( nIRQ >> 3 );
	unsigned char nVal = inb( nPort );
	if( ( nVal & nMask ) ) {
		outb( nVal & ~nMask, nPort );
	}
}


static void eisa_set_level_irq( unsigned int nIRQ )
{
	unsigned char nMask = 1 << ( nIRQ & 7 );
	unsigned int nPort = 0x4d0 + ( nIRQ >> 3 );
	unsigned char nVal = inb( nPort );
	if( !( nVal & nMask ) ) {
		outb( nVal | nMask, nPort );
	}
}

/* Add a bridge/device routing table */
static status_t acpi_pci_add_prt( acpi_handle nHandle, int nSegment, int nBus )
{
	struct acpi_buffer sBuffer = { 0, NULL };
	struct acpi_pci_routing_table* psPrt = NULL;
	struct acpi_pci_routing_table* psEntry = NULL;
	char *pzPathName = NULL;
	acpi_status nStatus;
	
	pzPathName = (char *)kmalloc( ACPI_PATHNAME_MAX, MEMF_KERNEL );
	if( !pzPathName )
		return_VALUE( -ENOMEM );
	memset(pzPathName , 0, ACPI_PATHNAME_MAX );
	
	/* 
	 * NOTE: We're given a 'handle' to the _PRT object's parent device
	 *       (either a PCI root bridge or PCI-PCI bridge).
	 */

	sBuffer.length = ACPI_PATHNAME_MAX;
	sBuffer.pointer = pzPathName;
	g_psBus->get_name( nHandle, ACPI_FULL_PATHNAME, &sBuffer );

	kerndbg( KERN_INFO, "PCI: PCI Interrupt Routing Table [%s._PRT]\n",
		pzPathName );

	/* 
	 * Evaluate this _PRT and call acpi_pci_scan_prt_entry() for every entry
	 */

	sBuffer.length = 0;
	sBuffer.pointer = NULL;
	kfree( pzPathName );
	nStatus = g_psBus->get_irq_routing_table( nHandle, &sBuffer );
	if( nStatus != AE_BUFFER_OVERFLOW ) {
		return_VALUE( -ENODEV );
	}

	psPrt = kmalloc( sBuffer.length, MEMF_KERNEL );
	if( !psPrt ) {
		return_VALUE( -ENOMEM );
	}
	memset( psPrt, 0, sBuffer.length );
	sBuffer.pointer = psPrt;

	nStatus =  g_psBus->get_irq_routing_table( nHandle, &sBuffer );
	if( ACPI_FAILURE( nStatus ) ) {
		kfree( sBuffer.pointer );
		return_VALUE( -ENODEV );
	}
	
	psEntry = psPrt;

	while( psEntry && ( psEntry->length > 0 ) ) {
		/* Add entry to the table */
		struct RoutingTableEntry_s sEntry;
		sEntry.nHandle = nHandle;
		sEntry.nSegment = nSegment;
		sEntry.nBus = nBus;
		sEntry.psEntry = psEntry;
		g_sRoutingEntries[g_nNumRoutingEntries++] = sEntry;
		
		kerndbg( KERN_DEBUG, "PCI: Routing Table Entry %u:%u Pin %i\n", (uint)nBus, (uint)( ( psEntry->address >> 16) & 0xFFFF ), 
				(int)psEntry->pin );
		if( psEntry->source[0] ) {
			kerndbg( KERN_DEBUG, "        Connected to link: %s:%i\n", psEntry->source, (int)psEntry->source_index );
		} else {
			kerndbg( KERN_DEBUG, "        Hardcoded IRQ %i\n", (int)psEntry->source_index );		
		}

		psEntry = ( struct acpi_pci_routing_table* )
			( ( unsigned long ) psEntry + psEntry->length );
	}
}

/* Scan one bus */
static void acpi_pci_parse_bus( struct acpi_device *psDevice, int nSegment, int nBus, int nDev, int nFunc )
{
	struct list_head *psNode;
	acpi_handle nHandle;
	int i;
	
	/* Check if a routing table is present */
	int nStatus = g_psBus->get_handle( psDevice->handle, METHOD_NAME__PRT, &nHandle );
	
	if( ACPI_SUCCESS( nStatus ) )
	{
		acpi_pci_add_prt( psDevice->handle, nSegment, nBus );
	}
	
	/* Iterate through the children */
	list_for_each( psNode, &psDevice->children ) {
		struct acpi_device *psChild = list_entry( psNode, struct acpi_device, node );
		uint32 nChildDev = psChild->pnp.bus_address >> 16;
		uint32 nChildFunc = psChild->pnp.bus_address & 0xFFFF;
		

		bool bIsBridge = false;
		/* Check if this child is a bridge */
		for( i = 0; i < g_nPCINumBusses; i++ )
		{
			PCI_Bus_s* psBus = g_apsPCIBus[i];
		
			if( psBus == NULL || psBus->nPCIDeviceNumber == -1 )
				continue;
			
			PCI_Entry_s* psDevice = g_apsPCIDevice[psBus->nPCIDeviceNumber];
			
			if( psDevice->nBus == nBus && psDevice->nDevice == nChildDev && 
				psDevice->nFunction == nChildFunc )
			{
				/* Recursive parsing */
				acpi_pci_parse_bus( psChild, nSegment, psBus->nSecondaryBus, nChildDev, nChildFunc );
				bIsBridge = true;
				break;
			}
		}
		/* It seems that there are also prt tables attached to devices */
		if( !bIsBridge )
		{
			int nStatus = g_psBus->get_handle( psChild->handle, METHOD_NAME__PRT, &nHandle );
	
			if( ACPI_SUCCESS( nStatus ) )
			{
				acpi_pci_add_prt( psChild->handle, nSegment, nBus );
			}
		}
	}
}

/* Add a pci root bridge */
static int acpi_pci_root_add( struct acpi_device *psDevice )
{
	int nSegment = 0;
	int nBus = 0;
	int nDevice = 0;
	int nFunc = 0;
	acpi_status	nStatus = AE_OK;
	unsigned long nValue = 0;
	acpi_handle	nHandle = NULL;

	if( !psDevice )
		return_VALUE(-EINVAL);

	/* 
	 * Segment
	 * -------
	 * Obtained via _SEG, if exists, otherwise assumed to be zero (0).
	 */
	nStatus = g_psBus->evaluate_integer( psDevice->handle, METHOD_NAME__SEG, NULL, 
		&nValue );
	switch( nStatus ) {
	case AE_OK:
		nSegment = nValue;
		break;
	case AE_NOT_FOUND:
		nSegment = 0;
		break;
	default:
		return( -ENODEV );
	}

	/* 
	 * Bus
	 * ---
	 * Obtained via _BBN, if exists, otherwise assumed to be zero (0).
	 */
	nStatus = g_psBus->evaluate_integer( psDevice->handle, METHOD_NAME__BBN, NULL, 
		&nValue );
	switch( nStatus ) {
	case AE_OK:
		nBus = nValue;
		break;
	case AE_NOT_FOUND:
		nBus = 0;
		break;
	default:
		return( -ENODEV );
	}

	/*
	 * Device & Function
	 * -----------------
	 * Obtained from _ADR (which has already been evaluated for us).
	 */
	nDevice = psDevice->pnp.bus_address >> 16;
	nFunc = psDevice->pnp.bus_address & 0xFFFF;

	kerndbg( KERN_INFO, "PCI: %s [%s] (%02x:%02x)\n", 
		acpi_device_name( psDevice ), acpi_device_bid( psDevice ),
		nSegment, nBus);
		
	/* Check if this bus is already registered */
	PCI_Bus_s* psBus = g_apsPCIBus[nBus];
		
	if( psBus == NULL )
	{
		/* Add bus */
		pci_scan_bus( nBus, -1, -1 );
	}
	
	/*
	 * PCI Routing Table
	 * -----------------
	 * Parse _PRT
	 */
	acpi_pci_parse_bus( psDevice, nSegment, nBus, nDevice, nFunc );
	

	return( 0 );
}


static int acpi_pci_root_remove( struct acpi_device* psDevice, int	nType )
{

	return_VALUE(0);
}

static bool acpi_pci_find_prt_entry( PCI_Entry_s* psEntryDev, PCI_Entry_s* psRealDev, int nPin )
{
	int j;
	int nIrq = -1;
	
	for( j = 0; j < g_nNumRoutingEntries; j++ )
	{
		struct RoutingTableEntry_s* psRTEntry = &g_sRoutingEntries[j];
		struct acpi_pci_routing_table* psEntry = psRTEntry->psEntry;
		
		/* Compare bus, device and pin. The function doesn’t matter here */
		if( psEntryDev->nBus == psRTEntry->nBus && psEntryDev->nDevice == ( ( psEntry->address >> 16) & 0xFFFF )
			&& nPin == psEntry->pin )
		{
			int nEdgeLevel = ACPI_LEVEL_SENSITIVE;
			int nActiveHighLow = ACPI_ACTIVE_LOW;
			if( psEntry->source[0] )
			{
				/* Get IRQ from Link */
				acpi_handle nLinkHandle;
					g_psBus->get_handle( psRTEntry->nHandle, psEntry->source, &nLinkHandle );
				nIrq = acpi_pci_link_get_irq( nLinkHandle, psEntry->source_index, &nEdgeLevel, &nActiveHighLow );
			} else {
				/* Hardcoded IRQ */
				nIrq = psEntry->source_index;
			}
		
			if( nIrq < 0 )
				return( true ); // ???
		
			kerndbg( KERN_DEBUG, "PCI: Device 0x%x:0x%x IRQ transform %i -> %i\n", (uint)psRealDev->nVendorID, (uint)psRealDev->nDeviceID, (int)psRealDev->u.h0.nInterruptLine, (int)nIrq );
			psRealDev->u.h0.nInterruptLine = nIrq;
		
			/* Necessary for via chipsets */
			write_pci_config( psRealDev->nBus, psRealDev->nDevice, psRealDev->nFunction, PCI_INTERRUPT_LINE, 1, nIrq & 15 );
		
			/* Set interrupt trigger */
			if( nEdgeLevel == ACPI_LEVEL_SENSITIVE )
				eisa_set_level_irq( nIrq );
			else
				eisa_set_edge_irq( nIrq );
					
			return( true );
		}
	}
	return( false );
}

/* Assign IRQs to the PCI devices */
static void acpi_pci_lookup_irqs( void )
{
	int i, j;
	int nIrq = 0;
	
	for( i = 0; i < g_nPCINumDevices; i++ )
	{
		PCI_Entry_s* psDev = g_apsPCIDevice[i];
		bool bEntryFound = false;
		int nPin = psDev->u.h0.nInterruptPin;
		
		if( nPin == 0 )
		{
			kerndbg( KERN_DEBUG, "PCI: Device 0x%x:0x%x has no interrupt pin\n", (uint)psDev->nVendorID, (uint)psDev->nDeviceID );
			continue;
		}
		
		nPin--;
		
		if( acpi_pci_find_prt_entry( psDev, psDev, nPin ) == false )
		{
			/* Try to find an irq using the bus device */
			if( g_apsPCIBus[psDev->nBus] == NULL )
			{
				kerndbg( KERN_WARNING, "PCI: Namespace corrupted (No PCI bus %i)!\n", psDev->nBus );
			}
			
			if( g_apsPCIBus[psDev->nBus]->nPCIDeviceNumber < 0 )
				goto notfound;
							
			PCI_Entry_s* psBridge = g_apsPCIDevice[g_apsPCIBus[psDev->nBus]->nPCIDeviceNumber];
			
			if( psBridge == NULL )
				goto notfound;
				
			if( acpi_pci_find_prt_entry( psBridge, psDev, ( nPin + psDev->nDevice ) % 4 ) == true )
				continue;
				
			notfound:
				kerndbg( KERN_DEBUG, "PCI: No interrupt found for device 0x%x:0x%x Pin %i\n", (uint)psDev->nVendorID, (uint)psDev->nDeviceID, (int)psDev->u.h0.nInterruptPin - 1 );
		}
	}
}


void init_acpi_pci_router(void)
{
	/* Register driver */
	g_psBus = get_busmanager( ACPI_BUS_NAME, ACPI_BUS_VERSION );
    if( g_psBus == NULL )
    	return;

	g_psBus->register_driver( &g_sACPIRootDriver );
	acpi_pci_lookup_irqs();
}









