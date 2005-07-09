
/*
 *  The Syllable kernel
 *  PCI busmanager
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 1999 - 2000 Matrin Mares
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

#include <posix/errno.h>
#include <atheos/device.h>
#include <atheos/kernel.h>
#include <atheos/isa_io.h>
#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <atheos/pci.h>
#include <atheos/bootmodules.h>
#include <atheos/config.h>

#include <macros.h>

struct RoutingIRQEntry_t {

	uint8 nLink;
	uint16 nBitmap;
} __attribute__((packed));

struct RoutingEntry_t {
	uint8 nBus;
	uint8 nFunction : 3;
	uint8 nDevice : 5;
	struct RoutingIRQEntry_t asIrq[4];
	uint8 nSlot;
	uint8 nRFU;
} __attribute__((packed));

typedef struct RoutingEntry_t RoutingEntry_s;

struct RoutingTable_t {
	uint32 nSignature;
	uint16 nVersion;
	uint16 nSize;
	uint8 nRouterBus;
	uint8 nRouterFnc : 3;
	uint8 nRouterDev : 5;
	uint16 nExclusiveIRQs;
	uint16 nVendorID;
	uint16 nDeviceID;
	uint32 nMiniportData;
	uint8 nRFU[11];
	uint8 nChecksum;
	RoutingEntry_s asCards[0];
} __attribute__((packed));

typedef struct RoutingTable_t RoutingTable_s;

typedef struct {
	char *pzName;
	uint16 nVendorID;
	uint16 nDeviceID;
	uint8 nBus;
	uint8 nDevice;
	uint8 nFunction;
	int (*get)( PCI_Entry_s* psDev, int nReg);
	int (*set)( PCI_Entry_s* psDev, int nReg, int nValue);
} Router_s;


#define RT_SIGNATURE	(('$' << 0) + ('P' << 8) + ('I' << 16) + ('R' << 24))
#define RT_VERSION 0x0100

unsigned int g_nPCIIrqMask = 0xfff8;
static int g_nIRQPenalty[16] = {
	1000000, 1000000, 1000000, 1000, 1000, 0, 1000, 1000,
	0, 0, 0, 0, 1000, 100000, 100000, 100000
};
static RoutingTable_s* g_psRoutingTable;
static Router_s g_sRouter;

extern uint32 read_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize );
extern status_t write_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize, uint32 nValue );

/* Chipset specific functions */

static int intel_get( PCI_Entry_s* psDev, int nReg )
{
	uint8 nValue = read_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nReg, 1 );

	return ( nValue < 16 ) ? nValue : 0;
}

static int intel_set( PCI_Entry_s* psDev, int nReg, int nValue )
{
	write_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nReg, 1, nValue );
	return 1;
}


static int sis_get( PCI_Entry_s* psDev, int nReg )
{
	uint nValue;
	if( nReg >= 1 && nReg <= 4 )
		nReg += 0x40;
		
	nValue = read_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nReg, 1 );

	return ( nValue & 0x80 ) ? 0 : ( nValue & 0x0f );
}

static int sis_set( PCI_Entry_s* psDev, int nReg, int nValue )
{
	uint8 nTemp;
	if( nReg >= 1 && nReg <= 4 )
		nReg += 0x40;
		
	nTemp = read_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nReg, 1 );
	nTemp &= ~( 0x0f | 0x80 );
	nTemp |= nValue ? nValue : 0x80;
	write_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nReg, 1, nValue );
	return 1;
}


static int via_get( PCI_Entry_s* psDev, int nReg )
{
	uint nValue;
	int nOffset = 0x55 + ( ( nReg == 4 ? 5 : nReg ) >> 1 );
	
	nValue = read_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nOffset, 1 );
	
	return( ( nReg & 1 ) ? ( nValue >> 4 ) : ( nValue & 0xf ) );
}

static int via_set( PCI_Entry_s* psDev, int nReg, int nValue )
{
	uint nRead;
	int nOffset = 0x55 + ( ( nReg == 4 ? 5 : nReg ) >> 1 );
	
	nRead = read_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nOffset, 1 );	
	nRead = ( nReg & 1 ) ? ( ( nRead & 0x0f ) | ( nValue << 4 ) ) : ( ( nRead & 0xf0 ) | nValue );
	write_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nOffset, 1, nRead );
	return 1;
}

static unsigned char anALIIrqMap[16] = { 0, 9, 3, 10, 4, 5, 7, 6, 1, 11, 0, 12, 0, 14, 0, 15 };

static int ali_get( PCI_Entry_s* psDev, int nReg )
{
	uint nValue;
	int nOffset = 0x48 + ( ( nReg - 1 ) >> 1 );
	
	nValue = read_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nOffset, 1 );
	
	nValue = ( nReg & 1 ) ? ( nValue >> 4 ) : ( nValue & 0xf );
	return( anALIIrqMap[nValue] );
}

static int ali_set( PCI_Entry_s* psDev, int nReg, int nValue )
{
	uint nRead;
	int nOffset = 0x48 + ( ( nReg - 1 ) >> 1 );
	
	if( nValue == 0 )
		return 0;
	
	nValue = anALIIrqMap[nValue];
	
	nRead = read_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nOffset, 1 );	
	nRead = ( nReg & 1 ) ? ( ( nRead & 0x0f ) | ( nValue << 4 ) ) : ( ( nRead & 0xf0 ) | nValue );
	write_pci_config( g_sRouter.nBus, g_sRouter.nDevice, g_sRouter.nFunction, nOffset, 1, nRead );
	return 1;
}

static bool find_pci_router( void )
{	
	g_sRouter.nBus = g_psRoutingTable->nRouterBus;
	g_sRouter.nDevice = g_psRoutingTable->nRouterDev;
	g_sRouter.nFunction = g_psRoutingTable->nRouterFnc;
	g_sRouter.nVendorID = g_psRoutingTable->nVendorID;
	g_sRouter.nDeviceID = g_psRoutingTable->nDeviceID;

	
	/* Intel IRQ router */
	if( g_psRoutingTable->nVendorID == 0x8086 )
	{
		switch( g_psRoutingTable->nDeviceID )
		{
			case 0x122e:
			case 0x7000:
			case 0x7110:
			case 0x1234:
			case 0x1235:
			case 0x2410:
			case 0x2420:
			case 0x2440:
			case 0x244c:
			case 0x2480:
			case 0x248c:
			case 0x24c0:
			case 0x2450:
			case 0x24d0:
			case 0x25a1:
			case 0x24cc:
			case 0x2640:
			case 0x2641:
			case 0x27b8:
			case 0x27b1:
				g_sRouter.pzName = "Intel PIIX/ICH IRQ router";
				g_sRouter.get = intel_get;
				g_sRouter.set = intel_set;
				return( true );
		}
    }
    /* SiS IRQ router */
    if( g_psRoutingTable->nVendorID == 0x1039 && g_psRoutingTable->nDeviceID == 0x0008 )
    {
    	g_sRouter.pzName = "SiS IRQ router";
		g_sRouter.get = sis_get;
		g_sRouter.set = sis_set;
		return( true );
    }
	/* VIA IRQ router */
	if( g_psRoutingTable->nVendorID == 0x1106 )
	{
		switch( g_psRoutingTable->nDeviceID )
		{
			case 0x0586:
			case 0x0596:
			case 0x0686:
			case 0x8231:
				g_sRouter.pzName = "VIA IRQ router";
				g_sRouter.get = via_get;
				g_sRouter.set = via_set;
				return( true );
		}
    }
     /* ALI IRQ router */
    if( g_psRoutingTable->nVendorID == 0x10b9 && g_psRoutingTable->nDeviceID == 0x1533 )
    {
    	g_sRouter.pzName = "ALI IRQ router";
		g_sRouter.get = ali_get;
		g_sRouter.set = ali_set;
		return( true );
    }
    g_psRoutingTable = NULL;
    return( false );
}

/* Common functions */

static void eisa_set_level_irq( unsigned int nIRQ )
{
	unsigned char nMask = 1 << ( nIRQ & 7 );
	unsigned int nPort = 0x4d0 + ( nIRQ >> 3 );
	unsigned char nVal = inb( nPort );
	if( !( nVal & nMask ) ) {
		outb( nVal | nMask, nPort );
	}
}


static RoutingEntry_s *get_routing_entry( PCI_Entry_s* psDevice )
{
	int nEntries = ( g_psRoutingTable->nSize - sizeof( RoutingTable_s ) ) / sizeof( RoutingEntry_s );
	RoutingEntry_s* psEntry;

	for( psEntry = g_psRoutingTable->asCards; nEntries--; psEntry++ )
		if( psEntry->nBus == psDevice->nBus && psEntry->nDevice == psDevice->nDevice )
			return psEntry;
	return NULL;
}


int lookup_irq( PCI_Entry_s* psDevice, bool bAssign )
{
	uint8 nPin;
	RoutingEntry_s* psEntry;
	int i, nNewirq;
	int nLink;
	uint32 nMask;
	char* pzMsg = NULL;
	int nIRQ = 0;
	
	if( !g_psRoutingTable )
		return( 0 );

	/* Find IRQ pin */
	nPin = psDevice->u.h0.nInterruptPin;
	if( !nPin ) {
		kerndbg( KERN_DEBUG_LOW, "PCI: No interrupt pin found\n" );
		return 0;
	}
	nPin = nPin - 1;

	/* Find IRQ routing entry */

	if( !g_psRoutingTable )
		return 0;
	
	psEntry = get_routing_entry( psDevice );
	if( !psEntry ) {
		kerndbg( KERN_DEBUG_LOW, "PCI: Not found in routing table\n" );
		return 0;
	}
	nLink = psEntry->asIrq[nPin].nLink;
	nMask = psEntry->asIrq[nPin].nBitmap;
	if( !nLink ) {
		kerndbg( KERN_DEBUG_LOW, "PCI: This device is not routed\n" );
		return 0;
	}
	
	nMask &= g_nPCIIrqMask;

	kerndbg( KERN_DEBUG_LOW, "PCI: PIRQ %02x, mask %04x, excl %04x\n", nLink, (uint)nMask, g_psRoutingTable->nExclusiveIRQs );
	
	
	/*
	 * Find the best IRQ to assign: use the one
	 * reported by the device if possible.
	 */
	nNewirq = psDevice->u.h0.nInterruptLine;
	kerndbg( KERN_DEBUG_LOW, "PCI: Current IRQ %d\n", nNewirq );
	if( !nNewirq && bAssign ) {
		for( i = 0; i < 16; i++ ) {
			if( !( nMask & ( 1 << i ) ) )
				continue;
			if( g_nIRQPenalty[i] < g_nIRQPenalty[nNewirq] ) {
				
				nNewirq = i;
			}
		}
	}
	kerndbg( KERN_DEBUG_LOW, "PCI: New IRQ IRQ %d\n", nNewirq );

	/* Use: 1. BIOS mapping, 2. Hardcored IRQ, 3. IRQ router table, 4. Assign IRQ */
	if( psDevice->u.h0.nInterruptLine )
	{
		nIRQ = psDevice->u.h0.nInterruptLine;
		pzMsg = "BIOS assigned";
	} else if( ( nLink & 0xf0 ) == 0xf0 ) {
		nIRQ = nLink & 0xf;
		pzMsg = "Hardcoded";
	} else if( g_sRouter.get && ( nIRQ = g_sRouter.get( psDevice, nLink ) ) ) {
		pzMsg = "Found";
	} else if( nNewirq && g_sRouter.set && ( psDevice->nClassBase >> 8) != PCI_DISPLAY ) {
		if( g_sRouter.set( psDevice, nLink, nNewirq ) ) {
			eisa_set_level_irq( nNewirq );
			nIRQ = nNewirq;
			pzMsg = "Assigned";
		}
	}

	if( !nIRQ ) {
		kerndbg( KERN_WARNING, "PCI: Could not assign an interrupt to this device\n" );
		return( 0 );
	}
	
	g_nIRQPenalty[nIRQ]++;
	psDevice->u.h0.nInterruptLine = nIRQ;
	kerndbg( KERN_DEBUG, "PCI: %s IRQ %d for device %i:%i:%i\n", pzMsg, nIRQ, psDevice->nBus, psDevice->nDevice, psDevice->nFunction );
	

	return 1;
}

static bool find_pci_routing_table( void )
{
	/* Try to find the PCI routing table */
	area_id hArea;
	uint8* pnAddress = NULL;
	unsigned int i;
	int j;
	RoutingTable_s* psTable;
	uint8 nSum = 0;
	
	hArea = create_area( "pci_routing_table", (void**)&pnAddress, 0x10000, 0x10000, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
	if( hArea < 0 )
	{
		return( false );
	}
	remap_area( hArea, (void*)0xf0000 );
	for( i = 0; i < 0x10000; i += 16 )
	{
		psTable = (RoutingTable_s*)( pnAddress + i );
		if( psTable->nSignature != RT_SIGNATURE ||
			psTable->nVersion != RT_VERSION ||
			psTable->nSize % 16 ||
			psTable->nSize < sizeof( RoutingTable_s ) )
			continue;
		nSum = 0;
		for( j = 0; j < psTable->nSize; j++ )
			nSum += pnAddress[i+j];
		if( !nSum )
		{
			kerndbg( KERN_INFO, "PCI: Found IRQ Routing Table at 0x%x\n", (uint)0xf0000 + i );
			g_psRoutingTable = psTable;
			return( true );
		}
	}
	return( false );
}

void init_pci_irq_routing( void )
{
	/* Try to find the irq routing device and the routing table */	
	if( find_pci_routing_table() ) {
		if( find_pci_router() ) {
			/* Increase IRQ penalty for non-exclusive PCI IRQs */
			if( g_psRoutingTable->nExclusiveIRQs ) {
				int i;
				for( i = 0; i < 16; i++ )
					if( !( g_psRoutingTable->nExclusiveIRQs & ( 1 << i ) ) )
						g_nIRQPenalty[i] += 100;
			}
			
			kerndbg( KERN_INFO, "PCI: Using %s\n", g_sRouter.pzName );
			return;
		}
	}
	
	kerndbg( KERN_INFO, "PCI: Using standard BIOS IRQ routing\n" );
}

