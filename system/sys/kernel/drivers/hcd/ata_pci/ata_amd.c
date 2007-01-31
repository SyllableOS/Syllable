/*
 *  ATA/ATAPI driver for Syllable
 *
 *  AMD/nVIDIA chipset support
 *
 *	Copyright (C) 2004 Arno Klenke
 *  Copyright (c) 2000-2002 Vojtech Pavlik
 *
 *  Based on the work of:
 *      Andre Hedrick
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

#define AMD_IDE_ENABLE		( 0x00 + psBridge->nBase )
#define AMD_IDE_CONFIG		( 0x01 + psBridge->nBase )
#define AMD_CABLE_DETECT	( 0x02 + psBridge->nBase )
#define AMD_DRIVE_TIMING	( 0x08 + psBridge->nBase )
#define AMD_8BIT_TIMING		( 0x0e + psBridge->nBase )
#define AMD_ADDRESS_SETUP	( 0x0c + psBridge->nBase )
#define AMD_UDMA_TIMING		( 0x10 + psBridge->nBase )

#define AMD_CLOCK			33333
#define AMD_UDMA			0x00f
#define AMD_UDMA_NONE		0x000
#define AMD_UDMA_33			0x001
#define AMD_UDMA_66			0x002
#define AMD_UDMA_100		0x004
#define AMD_UDMA_133		0x008
#define AMD_BAD_FIFO		0x010
#define AMD_SATA			0x080

static struct AMD_bridge_s {
	char* pzName;
	uint16 nVendorID;
	uint16 nDeviceID;
	uint32 nBase;
	uint16 nFlags;
} g_sAMDBridges[] = {
	{ "AMD Cobra", 0x1022, 0x7401, 0x40, AMD_UDMA_33 },
	{ "AMD Viper", 0x1022, 0x7409, 0x40, AMD_UDMA_66 },
	{ "AMD Viper", 0x1022, 0x7411, 0x40, AMD_UDMA_100 | AMD_BAD_FIFO },
	{ "AMD Opus", 0x1022, 0x7441, 0x40, AMD_UDMA_100 },
	{ "AMD 8111", 0x1022, 0x7469, 0x40, AMD_UDMA_100 },
	{ "AMD CS5536", 0x1022, 0x209a, 0x40, AMD_UDMA_100 },
	
	{ "nVIDIA nForce", 0x10de, 0x01bc, 0x50, AMD_UDMA_100 },
	{ "nVIDIA nForce 2", 0x10de, 0x0065, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce 2S", 0x10de, 0x0085, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce 2S SATA", 0x10de, 0x008e, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce 3", 0x10de, 0x00d5, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce 3S", 0x10de, 0x00e5, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce 3S SATA", 0x10de, 0x00e3, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce 3S SATA", 0x10de, 0x00ee, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce CK804", 0x10de, 0x0053, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce MCP04", 0x10de, 0x0035, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce MCP51", 0x10de, 0x0265, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce MCP55", 0x10de, 0x036e, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce MCP61", 0x10de, 0x03ec, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce MCP65", 0x10de, 0x0448, 0x50, AMD_UDMA_133 },
	{ "nVIDIA nForce CK804 SATA", 0x10de, 0x0054, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce CK804 SATA2", 0x10de, 0x0055, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce MCP04 SATA", 0x10de, 0x0036, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce MCP04 SATA2", 0x10de, 0x003e, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce MCP51 SATA", 0x10de, 0x0266, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce MCP55 SATA", 0x10de, 0x037e, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce MCP55 SATA", 0x10de, 0x037f, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce MCP61 SATA", 0x10de, 0x03e7, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce MCP61 SATA2", 0x10de, 0x03f6, 0x50, AMD_UDMA_133 | AMD_SATA },
	{ "nVIDIA nForce MCP61 SATA3", 0x10de, 0x03f7, 0x50, AMD_UDMA_133 | AMD_SATA }
};

struct AMD_private_s
{
	PCI_Info_s sDevice;
	struct AMD_bridge_s* psBridge;
};

static unsigned char amd_cyc2udma[] = { 6, 6, 5, 4, 0, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 7 };

/* Set speed on one drive */
void amd_port_set_speed( struct AMD_bridge_s* psBridge, PCI_Info_s sDevice, ATA_port_s* psPort, ATA_timing_s* psTiming )
{
	uint8 nT;
	int nDriveNum = psPort->nChannel * psPort->psCtrl->nPortsPerChannel + psPort->nPort;
	
	kerndbg( KERN_DEBUG, "%s: Set %i Act8 %i Rec8 %i Cyc8 %i Act %i Rec %i Cyc %i DMA %i\n",
			psPort->nPort ? "Slave" : "Master", psTiming->nSetup, psTiming->nAct8b, psTiming->nRec8b, psTiming->nCyc8b,
			psTiming->nAct, psTiming->nRec, psTiming->nCyc, psTiming->nUDMA );
	
	
	nT = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_ADDRESS_SETUP, 1 );
	nT = (nT & ~(3 << ((3 - nDriveNum) << 1))) | ((FIT(psTiming->nSetup, 1, 4) - 1) << ((3 - nDriveNum) << 1));
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_ADDRESS_SETUP, 1, nT );

	
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_8BIT_TIMING + (1 - (nDriveNum >> 1)), 1, 
										((FIT(psTiming->nAct8b, 1, 16) - 1) << 4) | (FIT(psTiming->nRec8b, 1, 16) - 1));
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_DRIVE_TIMING + (3 - nDriveNum), 1, 
										((FIT(psTiming->nAct, 1, 16) - 1) << 4) | (FIT(psTiming->nRec, 1, 16) - 1));
	
	switch( psBridge->nFlags & AMD_UDMA )
	{
		case AMD_UDMA_33:  nT = psTiming->nUDMA ? (0xc0 | (FIT(psTiming->nUDMA, 2, 5) - 2)) : 0x03; break;
		case AMD_UDMA_66:  nT = psTiming->nUDMA ? (0xc0 | amd_cyc2udma[FIT(psTiming->nUDMA, 2, 10)]) : 0x03; break;
		case AMD_UDMA_100: nT = psTiming->nUDMA ? (0xc0 | amd_cyc2udma[FIT(psTiming->nUDMA, 1, 10)]) : 0x03; break;
		case AMD_UDMA_133: nT = psTiming->nUDMA ? (0xc0 | amd_cyc2udma[FIT(psTiming->nUDMA, 1, 15)]) : 0x03; break;
		default: return;
	}
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_UDMA_TIMING + (3 - nDriveNum), 1, nT );
}

/* Calculate controller speed */
status_t amd_port_configure( ATA_port_s* psPort )
{
	ATA_controller_s* psCtrl = psPort->psCtrl;
	ATA_port_s* psFirst = NULL;
	ATA_port_s* psSecond = NULL;
	ATA_timing_s sResult, sSecond;
	unsigned int T, UT;
	struct AMD_private_s* psPrivate = psPort->pPrivate;
	struct AMD_bridge_s* psBridge = psPrivate->psBridge;
	
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
	T = 1000000000 / AMD_CLOCK;
	
	switch( psBridge->nFlags & AMD_UDMA ) {
		case AMD_UDMA_33:   UT = T;   break;
		case AMD_UDMA_66:   UT = T/2; break;
		case AMD_UDMA_100:  UT = T/3; break;
		case AMD_UDMA_133:  UT = T/4; break;
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
	
	if( psFirst && psFirst->nCurrentSpeed == ATA_SPEED_UDMA_5 ) sResult.nUDMA = 1;
	if( psFirst && psFirst->nCurrentSpeed == ATA_SPEED_UDMA_6 ) sResult.nUDMA = 15;
	
	if( psSecond && psSecond->nCurrentSpeed == ATA_SPEED_UDMA_5 ) sSecond.nUDMA = 1;
	if( psSecond && psSecond->nCurrentSpeed == ATA_SPEED_UDMA_6 ) sSecond.nUDMA = 15;
	
	/* Set speed */
	if( psSecond )
		amd_port_set_speed( psBridge, psPrivate->sDevice, psSecond, &sSecond );
	if( psFirst )
		amd_port_set_speed( psBridge, psPrivate->sDevice, psFirst, &sResult );
	
	return( 0 );
}

void init_amd_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl )
{
	int i;
	int j;
	bool bFound = false;
	struct AMD_bridge_s* psBridge = NULL;
	uint32 nU;
	uint8 nT;
	int n80W = 0;
	struct AMD_private_s* psPrivate = kmalloc( sizeof( struct AMD_private_s ), MEMF_KERNEL | MEMF_CLEAR );
	
	/* Scan controllers */
	for( j = 0; j < ( sizeof( g_sAMDBridges ) / sizeof( struct AMD_bridge_s ) ); j++ )
	{
		if( sDevice.nVendorID == g_sAMDBridges[j].nVendorID && sDevice.nDeviceID == g_sAMDBridges[j].nDeviceID )
		{
			psBridge = &g_sAMDBridges[j];
			bFound = true;
			break;
		}	
	}
	
	if( !bFound )
	{
		kerndbg( KERN_WARNING, "Unknown AMD/nVIDIA ATA controller detected\n" );
		return;
	}
	
	if( psBridge->nFlags & AMD_SATA )
	{
		kerndbg( KERN_INFO, "nVIDIA Serial ATA controller detected\n" );
		strcpy( psCtrl->zName, "nVIDIA Serial ATA controller" );
		
		/* Only one port per channel (right?) */
		kfree( psCtrl->psPort[1] );
		kfree( psCtrl->psPort[3] );
		psCtrl->psPort[1] = psCtrl->psPort[2];
		psCtrl->psPort[2] = NULL;
		psCtrl->nPortsPerChannel = 1;
		psCtrl->psPort[0]->nCable = psCtrl->psPort[1]->nCable = ATA_CABLE_SATA;
		psCtrl->psPort[0]->nSupportedPortSpeed |= 0x7fff;
		psCtrl->psPort[1]->nSupportedPortSpeed |= 0x7fff;
		return;
	}
	
	kerndbg( KERN_INFO, "%s controller detected\n", psBridge->pzName );
	
	/* Check cable type */
	switch( psBridge->nFlags & AMD_UDMA ) {
		case AMD_UDMA_66:
			nU = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_UDMA_TIMING, 4 );
			for (i = 24; i >= 0; i -= 8)
				if ((nU >> i) & 4) {
					n80W |= (1 << (1 - (i >> 4)));
				}
			break;
		case AMD_UDMA_100:
		case AMD_UDMA_133:
			nT = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_CABLE_DETECT, 1 );
			nU = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_UDMA_TIMING, 4 );
			n80W = ((nT & 0x3) ? 1 : 0) | ((nT & 0xc) ? 2 : 0);
			for (i = 24; i >= 0; i -= 8)
				if (((nU >> i) & 4) && !(n80W & (1 << (1 - (i >> 4))))) {
					kerndbg( KERN_WARNING, "BIOS didn't set cable bits correctly. Enabling workaround.\n" );
					n80W |= (1 << (1 - (i >> 4)));
				}
			break;
	}
	
	psCtrl->psPort[0]->nCable = psCtrl->psPort[1]->nCable = ( n80W & 0x1 ) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40;
	psCtrl->psPort[2]->nCable = psCtrl->psPort[3]->nCable = ( n80W & 0x2 ) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40;
	
	
	/* Take care of prefetch & postwrite. */
	nT = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_IDE_CONFIG, 1 );
	
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										AMD_IDE_CONFIG, 1, ( ( psBridge->nFlags & AMD_BAD_FIFO )
										? ( nT & 0x0f ) : ( nT | 0xf0 ) ) );
		
	/* Add speeds */
	psPrivate->sDevice = sDevice;
	psPrivate->psBridge = psBridge;
	
	for( i = 0; i < 4; i++ )
	{
		ATA_port_s* psPort = psCtrl->psPort[i];
		bool b80bit = ( i & 0x02 ) ? ( n80W & 0x2 ) : ( n80W & 0x1 );
		
		/* Add all multiword dma and pio modes (right???) */
		psPort->nSupportedPortSpeed = 0xff;
		
		if( psBridge->nFlags & AMD_UDMA ) psPort->nSupportedPortSpeed |= 0x700;
		if( ( psBridge->nFlags & AMD_UDMA_66 ) && b80bit ) psPort->nSupportedPortSpeed |= 0x1f00;
		if( ( psBridge->nFlags & AMD_UDMA_100 ) && b80bit ) psPort->nSupportedPortSpeed |= 0x3fff;
		if( ( psBridge->nFlags & AMD_UDMA_133 ) && b80bit ) psPort->nSupportedPortSpeed |= 0x7fff;
		
		psPort->sOps.configure = amd_port_configure;
		psPort->pPrivate = psPrivate;
	}
	
	strcpy( psCtrl->zName, "AMD/nVIDIA ATA controller" );
	claim_device( psCtrl->nDeviceID, sDevice.nHandle, "AMD/nVIDIA ATA controller", DEVICE_CONTROLLER );
}

























