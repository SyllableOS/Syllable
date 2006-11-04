/*
 *  ATA/ATAPI driver for Syllable
 *
 *  VIA chipset support
 *
 *  Copyright (C) 2006 Kristian Van Der Vliet
 *  Copyright (C) 2004 Arno Klenke
 *  Copyright (c) 2000-2002 Vojtech Pavlik
 *  Based on the work of:
 *   Michel Aubry
 *   Jeff Garzik
 *   Andre Hedrick
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
#include "ata_timing.h"
#include <atheos/pci.h> 
extern PCI_bus_s* g_psPCIBus;

#define VIA_IDE_ENABLE		0x40
#define VIA_IDE_CONFIG		0x41
#define VIA_FIFO_CONFIG		0x43
#define VIA_MISC_1			0x44
#define VIA_MISC_2			0x45
#define VIA_MISC_3			0x46
#define VIA_DRIVE_TIMING	0x48
#define VIA_8BIT_TIMING		0x4e
#define VIA_ADDRESS_SETUP	0x4c
#define VIA_UDMA_TIMING		0x50

#define VIA_CLOCK			33333
#define VIA_UDMA			0x00f
#define VIA_UDMA_NONE		0x000
#define VIA_UDMA_33			0x001
#define VIA_UDMA_66			0x002
#define VIA_UDMA_100		0x004
#define VIA_UDMA_133		0x008
#define VIA_BAD_PREQ		0x010	/* Crashes if PREQ# till DDACK# set */
#define VIA_BAD_CLK66		0x020	/* 66 MHz clock doesn't work correctly */
#define VIA_SET_FIFO		0x040	/* Needs to have FIFO split set */
#define VIA_NO_UNMASK		0x080	/* Doesn't work with IRQ unmasking on */
#define VIA_BAD_ID			0x100	/* Has wrong vendor ID (0x1107) */
#define VIA_BAD_AST			0x200	/* Don't touch Address Setup Timing */

static struct VIA_bridge_s {
	char* pzName;
	uint16 nDeviceID;
	uint8 nMinRev;
	uint8 nMaxRev;
	uint16 nFlags;
} g_sVIABridges[] = {
	{ "VT6410", 0x3164, 0x00, 0x2f, VIA_UDMA_133 | VIA_BAD_AST },
	{ "VT8251", 0x3287, 0x00, 0x2f, VIA_UDMA_133 | VIA_BAD_AST },
	{ "VT8237", 0x3227, 0x00, 0x2f, VIA_UDMA_133 | VIA_BAD_AST },
	{ "VT8237a", 0x3337, 0x00, 0x2f, VIA_UDMA_133 | VIA_BAD_AST },
	{ "VT8235", 0x3177, 0x00, 0x2f, VIA_UDMA_133 | VIA_BAD_AST },
	{ "VT8233a", 0x3147, 0x00, 0x2f, VIA_UDMA_133 | VIA_BAD_AST },
	{ "VT8233c", 0x3109, 0x00, 0x2f, VIA_UDMA_100 },
	{ "VT8233", 0x3074, 0x00, 0x2f, VIA_UDMA_100 },
	{ "VT8231", 0x8231, 0x00, 0x2f, VIA_UDMA_100 },
	{ "VT82C686b", 0x0686, 0x40, 0x4f, VIA_UDMA_100 },
	{ "VT82C686a", 0x0686, 0x10, 0x2f, VIA_UDMA_66 },
	{ "VT82C686",  0x0686, 0x00, 0x0f, VIA_UDMA_33 | VIA_BAD_CLK66 },
	{ "VT82C596b", 0x0596, 0x10, 0x2f, VIA_UDMA_66 },
	{ "VT82C596a", 0x0596, 0x00, 0x0f, VIA_UDMA_33 | VIA_BAD_CLK66 },
	{ "VT82C586b", 0x0586, 0x47, 0x4f, VIA_UDMA_33 | VIA_SET_FIFO },
	{ "VT82C586b", 0x0586, 0x40, 0x46, VIA_UDMA_33 | VIA_SET_FIFO | VIA_BAD_PREQ },
	{ "VT82C586b", 0x0586, 0x30, 0x3f, VIA_UDMA_33 | VIA_SET_FIFO },
	{ "VT82C586a", 0x0586, 0x20, 0x2f, VIA_UDMA_33 | VIA_SET_FIFO },
	{ "VT82C586",  0x0586, 0x00, 0x0f, VIA_UDMA_NONE | VIA_SET_FIFO },
	{ "VT82C576b", 0x0576, 0x00, 0x2f, VIA_UDMA_NONE | VIA_SET_FIFO | VIA_NO_UNMASK }
};

static struct VIA_sata_hcd_s {
	char *pzName;
	uint16 nDeviceID;
} g_sVIAHcds[] = {
	{ "VT6420", 0x0591 },
	{ "VT6420", 0x3149 },
	{ "VT6421", 0x3249 }
};

struct VIA_private_s
{
	PCI_Info_s sDevice;
	struct VIA_bridge_s* psBridge;
};

enum {
	SATA_CHAN_ENAB		= 0x40, /* SATA channel enable */
	SATA_INT_GATE		= 0x41, /* SATA interrupt gating */
	SATA_NATIVE_MODE	= 0x42, /* Native mode enable */
	SATA_PATA_SHARING	= 0x49, /* PATA/SATA sharing func ctrl */

	PORT0			= (1 << 1),
	PORT1			= (1 << 0),
	ALL_PORTS		= PORT0 | PORT1,

	NATIVE_MODE_ALL		= (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4)
};

/* Set speed on one drive */
void via_port_set_speed( struct VIA_bridge_s* psBridge, PCI_Info_s sDevice, ATA_port_s* psPort, ATA_timing_s* psTiming )
{
	uint8 nT;
	int nDriveNum = psPort->nChannel * psPort->psCtrl->nPortsPerChannel + psPort->nPort;
	
	kerndbg( KERN_DEBUG, "%s: Set %i Act8 %i Rec8 %i Cyc8 %i Act %i Rec %i Cyc %i DMA %i\n",
			psPort->nPort ? "Slave" : "Master", psTiming->nSetup, psTiming->nAct8b, psTiming->nRec8b, psTiming->nCyc8b,
			psTiming->nAct, psTiming->nRec, psTiming->nCyc, psTiming->nUDMA );
	
	if( !( psBridge->nFlags & VIA_BAD_AST ) )
	{
		nT = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_ADDRESS_SETUP, 1 );
		nT = (nT & ~(3 << ((3 - nDriveNum) << 1))) | ((FIT(psTiming->nSetup, 1, 4) - 1) << ((3 - nDriveNum) << 1));
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_ADDRESS_SETUP, 1, nT );
	}
	
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_8BIT_TIMING + (1 - (nDriveNum >> 1)), 1, 
										((FIT(psTiming->nAct8b, 1, 16) - 1) << 4) | (FIT(psTiming->nRec8b, 1, 16) - 1));
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_DRIVE_TIMING + (3 - nDriveNum), 1, 
										((FIT(psTiming->nAct, 1, 16) - 1) << 4) | (FIT(psTiming->nRec, 1, 16) - 1));
	
	switch( psBridge->nFlags & VIA_UDMA )
	{
		case VIA_UDMA_33:  nT = psTiming->nUDMA ? (0xe0 | (FIT(psTiming->nUDMA, 2, 5) - 2)) : 0x03; break;
		case VIA_UDMA_66:  nT = psTiming->nUDMA ? (0xe8 | (FIT(psTiming->nUDMA, 2, 9) - 2)) : 0x0f; break;
		case VIA_UDMA_100: nT = psTiming->nUDMA ? (0xe0 | (FIT(psTiming->nUDMA, 2, 9) - 2)) : 0x07; break;
		case VIA_UDMA_133: nT = psTiming->nUDMA ? (0xe0 | (FIT(psTiming->nUDMA, 2, 9) - 2)) : 0x07; break;
		default: return;
	}
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_UDMA_TIMING + (3 - nDriveNum), 1, nT );
}

/* Calculate controller speed */
status_t via_port_configure( ATA_port_s* psPort )
{
	ATA_controller_s* psCtrl = psPort->psCtrl;
	ATA_port_s* psFirst = NULL;
	ATA_port_s* psSecond = NULL;
	ATA_timing_s sResult, sSecond;
	unsigned int T, UT;
	struct VIA_private_s* psPrivate = psPort->pPrivate;
	struct VIA_bridge_s* psBridge = psPrivate->psBridge;
	
	/* Check configuration */
	if( psPort->nPort == 0 && !( psPort->nDevice >= ATA_DEV_ATA 
		&& psCtrl->psPort[psPort->nChannel*psCtrl->nPortsPerChannel+1]->nDevice < ATA_DEV_ATA ) )
		return( 0 ); 

	if( psPort->nPort == 0 ) {
		psFirst = psPort;
	}
		
	if( psPort->nPort == 1 ) {
		if( psPort->nDevice < ATA_DEV_ATA )
			return( 0 );
		if( psCtrl->psPort[psPort->nChannel*psCtrl->nPortsPerChannel]->nDevice < ATA_DEV_ATA ) {
			psFirst = psPort;
		} else {
			psFirst = psCtrl->psPort[psPort->nChannel*psCtrl->nPortsPerChannel];
			psSecond = psPort;
		}
	}
	
	/* Calculate timing */
	T = 1000000000 / VIA_CLOCK;
	
	switch( psBridge->nFlags & VIA_UDMA ) {
		case VIA_UDMA_33:   UT = T;   break;
		case VIA_UDMA_66:   UT = T/2; break;
		case VIA_UDMA_100:  UT = T/3; break;
		case VIA_UDMA_133:  UT = T/4; break;
		default: UT = T;
	}
	
	if( ata_timing_compute( psFirst, psFirst->nCurrentSpeed, &sResult, T, UT ) < 0 )
		return( -1 );
	
	/* Calculate timing of second port and merge both */
	if( psSecond )
	{
		if( ata_timing_compute( psSecond, psSecond->nCurrentSpeed, &sSecond, T, UT ) < 0 )
			return( -1 );
		ata_timing_merge( &sSecond, &sResult, &sResult, ATA_TIMING_8BIT );
	}
	
	/* Set speed */
	if( psSecond )
		via_port_set_speed( psBridge, psPrivate->sDevice, psSecond, &sSecond );
	if( psFirst )
		via_port_set_speed( psBridge, psPrivate->sDevice, psFirst, &sResult );
	
	return( 0 );
}

void init_via_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl )
{
	PCI_Info_s sBridgeDev;
	int i, j;
	bool bFound = false;
	struct VIA_bridge_s* psBridge = NULL;
	uint32 nU;
	uint8 nV, nT;
	int n80W = 0;
	struct VIA_private_s* psPrivate = kmalloc( sizeof( struct VIA_private_s ), MEMF_KERNEL | MEMF_CLEAR );

	/* Scan bridges */
	for( i = 0; g_psPCIBus->get_pci_info( &sBridgeDev, i ) == 0; ++i )
	{
		for( j = 0; j < ( sizeof( g_sVIABridges ) / sizeof( struct VIA_bridge_s ) ); j++ )
		{
			if( sBridgeDev.nVendorID == 0x1106 && sBridgeDev.nDeviceID == g_sVIABridges[j].nDeviceID 
			&& sBridgeDev.nRevisionID >= g_sVIABridges[j].nMinRev && sBridgeDev.nRevisionID <= g_sVIABridges[j].nMaxRev )
			{
				psBridge = &g_sVIABridges[j];
				bFound = true;
				break;
			}
			
		}
	}
	
	if( !bFound )
	{
		kerndbg( KERN_WARNING, "Unknown VIA ATA controller detected\n" );
		return;
	}
	
	kerndbg( KERN_INFO, "VIA %s bridge detected\n", psBridge->pzName );
	
	/* Check cable type */
	switch( psBridge->nFlags & VIA_UDMA ) {
		case VIA_UDMA_66:
			nU = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_UDMA_TIMING, 4 );
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_UDMA_TIMING, 4, nU | 0x80008 );
			for (i = 24; i >= 0; i -= 8)
				if (((nU >> (i & 16)) & 8) &&
				    ((nU >> i) & 0x20) &&
				     (((nU >> i) & 7) < 2)) {
					/* BIOS 80-wire bit or
					 * UDMA w/ < 60ns/cycle
					 */
					n80W |= (1 << (1 - (i >> 4)));
				}
			break;
		case VIA_UDMA_100:
			nU = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_UDMA_TIMING, 4 );
			for (i = 24; i >= 0; i -= 8)
				if (((nU >> i) & 0x10) ||
				    (((nU >> i) & 0x20) &&
				     (((nU >> i) & 7) < 4))) {
					/* BIOS 80-wire bit or
					 * UDMA w/ < 60ns/cycle
					 */
					n80W |= (1 << (1 - (i >> 4)));
				}
			break;
		case VIA_UDMA_133:
			nU = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_UDMA_TIMING, 4 );
			for (i = 24; i >= 0; i -= 8)
				if (((nU >> i) & 0x10) ||
				    (((nU >> i) & 0x20) &&
				     (((nU >> i) & 7) < 6))) {
					/* BIOS 80-wire bit or
					 * UDMA w/ < 60ns/cycle
					 */
					n80W |= (1 << (1 - (i >> 4)));
				}
			break;
	}
	
	psCtrl->psPort[0]->nCable = psCtrl->psPort[1]->nCable = ( n80W & 0x1 ) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40;
	psCtrl->psPort[2]->nCable = psCtrl->psPort[3]->nCable = ( n80W & 0x2 ) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40;
	
	if( psBridge->nFlags & VIA_BAD_CLK66 )
	{
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_UDMA_TIMING, 4,  g_psPCIBus->read_pci_config( sDevice.nBus, 
										sDevice.nDevice, sDevice.nFunction,
										VIA_UDMA_TIMING, 4 ) & ~0x80008 );
	}
	
	/* Check if interfaces are enabled */
	nV = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_IDE_ENABLE, 1 );
	nT = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_FIFO_CONFIG, 1 );
	if( psBridge->nFlags & VIA_BAD_PREQ )
		nT &= 0x7f;
	
	/* Set fifo sizes */
	if( psBridge->nFlags & VIA_SET_FIFO ) {
		nT &= (nT & 0x9f);
		switch (nV & 3) {
			case 2: nT |= 0x00; break;	/* 16 on primary */
			case 1: nT |= 0x60; break;	/* 16 on secondary */
			case 3: nT |= 0x20; break;	/* 8 pri 8 sec */
		}
	}
	
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										VIA_FIFO_CONFIG, 1, nT );
										
	/* Add speeds */
	psPrivate->sDevice = sDevice;
	psPrivate->psBridge = psBridge;
	
	for( i = 0; i < 4; i++ )
	{
		ATA_port_s* psPort = psCtrl->psPort[i];
		bool b80bit = ( i & 0x02 ) ? ( n80W & 0x2 ) : ( n80W & 0x1 );
		
		/* Add all multiword dma and pio modes (right???) */
		psPort->nSupportedPortSpeed = 0xff;
		
		
		if( psBridge->nFlags & VIA_UDMA ) psPort->nSupportedPortSpeed |= 0x700;
		if( ( psBridge->nFlags & VIA_UDMA_66 ) && b80bit ) psPort->nSupportedPortSpeed |= 0x1f00;
		if( ( psBridge->nFlags & VIA_UDMA_100 ) && b80bit ) psPort->nSupportedPortSpeed |= 0x3fff;
		if( ( psBridge->nFlags & VIA_UDMA_133 ) && b80bit ) psPort->nSupportedPortSpeed |= 0x7fff;
	
		psPort->sOps.configure = via_port_configure;
		psPort->pPrivate = psPrivate;
	}
	
	strcpy( psCtrl->zName, "VIA ATA controller" );
	claim_device( psCtrl->nDeviceID, sDevice.nHandle, "VIA ATA controller", DEVICE_CONTROLLER );
}

static uint32 vt6420_scr_addr( uint32 nAddr, uint8 nPort )
{
	return nAddr + ( nPort * 128 );
}

static uint32 vt6421_scr_addr( uint32 nAddr, uint8 nPort )
{
	return nAddr + ( nPort * 64 );
}

static void vt6421_set_addrs( ATA_port_s *psPort, uint32 nCommand, uint32 nDMA, uint32 nSCR, uint8 nPort )
{
	int i;

	for( i = 0; i < 8; i++ )
		psPort->nRegs[i] = nCommand + i;

	psPort->nDmaRegs[ATA_REG_DMA_CONTROL] = nDMA;
	psPort->nDmaRegs[ATA_REG_DMA_STATUS] = nDMA + 2;
	psPort->nDmaRegs[ATA_REG_DMA_TABLE] = nDMA + 4;

	uint32 nSCRAddr = vt6421_scr_addr( nSCR, nPort );
	for( i = 0; i < SATA_TOTAL_REGS; i++ )
		psPort->nSATARegisters[i] = nSCRAddr + i;
}

void init_via_sata_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl )
{
	int i, j;
	bool bFound = false;
	struct VIA_sata_hcd_s* psHcd = NULL;
	uint8 nT;

	for( i = 0; i < ( sizeof( g_sVIAHcds ) / sizeof( struct VIA_sata_hcd_s ) ); i++ )
	{
		if( sDevice.nVendorID == 0x1106 && sDevice.nDeviceID == g_sVIAHcds[i].nDeviceID )
		{
			psHcd = &g_sVIAHcds[i];
			bFound = true;
			break;
		}
	}

	if( !bFound )
	{
		kerndbg( KERN_WARNING, "Unknown VIA SATA controller detected\n" );
		return;
	}

	kerndbg( KERN_INFO, "VIA %s HCD detected\n", psHcd->pzName );

	/* Only one port per channel (right?) */
	kfree( psCtrl->psPort[1] );
	kfree( psCtrl->psPort[3] );
	psCtrl->psPort[1] = psCtrl->psPort[2];
	psCtrl->psPort[2] = NULL;
	psCtrl->nPortsPerChannel = 1;
	psCtrl->psPort[0]->nCable = psCtrl->psPort[1]->nCable = ATA_CABLE_SATA;
	psCtrl->psPort[0]->nType = psCtrl->psPort[1]->nType = ATA_SATA;
	psCtrl->psPort[0]->nSupportedPortSpeed = psCtrl->psPort[1]->nSupportedPortSpeed |= 0x7fff;	/* UDMA133 */

	uint32 nSCR = sDevice.u.h0.nBase4 & PCI_ADDRESS_MEMORY_32_MASK;

	if( sDevice.nDeviceID == 0x3249 )
	{
		/* vt6421 */
		uint32 nCommand, nDMA;

		nCommand = sDevice.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
		nDMA = sDevice.u.h0.nBase3 & PCI_ADDRESS_MEMORY_32_MASK;
		vt6421_set_addrs( psCtrl->psPort[0], nCommand, nDMA, nSCR, 0 );

		nCommand = sDevice.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;
		nDMA = (sDevice.u.h0.nBase3 & PCI_ADDRESS_MEMORY_32_MASK) + 8;
		vt6421_set_addrs( psCtrl->psPort[1], nCommand, nDMA, nSCR, 1 );
	}
	else
	{
		/* vt6420 */
		for( i = 0; i < 2; i++ )
		{
			uint32 nSCRAddr = vt6420_scr_addr( nSCR, i );
			for( j = 0; j < SATA_TOTAL_REGS; j++ )
				psCtrl->psPort[i]->nSATARegisters[j] = nSCRAddr + j;
		}
	}

	/* Make sure SATA channels are enabled */
	nT = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, SATA_CHAN_ENAB, 1 );
	if( (nT & ALL_PORTS) != ALL_PORTS)
	{
		kerndbg( KERN_DEBUG, "Enabling SATA channels (0x%x)\n", (int)nT );
		nT |= ALL_PORTS;
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, SATA_CHAN_ENAB, 1, nT );
	}

	/* Make sure interrupts for each channel sent to us */
	nT = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, SATA_INT_GATE, 1 );
	if( (nT & ALL_PORTS) != ALL_PORTS)
	{
		kerndbg( KERN_DEBUG, "Enabling SATA channel interrupts (0x%x)\n", (int)nT );
		nT |= ALL_PORTS;
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, SATA_INT_GATE, 1, nT );
	}

	/* Make sure native mode is enabled */
	nT = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, SATA_NATIVE_MODE, 1 );
	if( (nT & NATIVE_MODE_ALL) != NATIVE_MODE_ALL)
	{
		kerndbg( KERN_DEBUG, "Enabling SATA channel native mode (0x%x)\n", (int)nT );
		nT |= NATIVE_MODE_ALL;
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, SATA_NATIVE_MODE, 1, nT );
	}

	strcpy( psCtrl->zName, "VIA Serial ATA controller" );
	claim_device( psCtrl->nDeviceID, sDevice.nHandle, "VIA Serial ATA controller", DEVICE_CONTROLLER );

	return;
}

