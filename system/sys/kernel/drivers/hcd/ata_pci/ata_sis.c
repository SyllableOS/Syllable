/*
 *  ATA/ATAPI driver for Syllable
 *
 *  SIS chipset support
 *
 *	Copyright (C) 2004 Arno Klenke
 *  Copyright (C) 1999-2000	Andre Hedrick <andre@linux-ide.org>
 *  Copyright (C) 2002		Lionel Bouton <Lionel.Bouton@inet6.fr>, Maintainer
 *  Copyright (C) 2003		Vojtech Pavlik <vojtech@suse.cz>
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

#define SIS_16 0x1
#define SIS_UDMA_33 0x02
#define SIS_UDMA_66 0x03
#define SIS_UDMA_100a 0x04
#define SIS_UDMA_100 0x05

#define SIS_UDMA_133a 0x06
#define SIS_UDMA_133 0x07
#define SIS_UDMA 0x07

static struct SIS_bridge_s {
	char* pzName;
	uint16 nDeviceID;
	uint16 nFlags;
} g_sSISBridges[] = {
	{ "SiS 745", 0x0745, SIS_UDMA_100 },
	{ "SiS 735", 0x0735, SIS_UDMA_100 },
	{ "SiS 733", 0x0733, SIS_UDMA_100 },
	{ "SiS 635", 0x0635, SIS_UDMA_100 },
	{ "SiS 633", 0x0633, SIS_UDMA_100 },
	
	{ "SiS 730", 0x0730, SIS_UDMA_100a },
	{ "SiS 550", 0x0550, SIS_UDMA_100a },
	
	{ "SiS 640", 0x0640, SIS_UDMA_66 },
	{ "SiS 630", 0x0630, SIS_UDMA_66 },
	{ "SiS 620", 0x0620, SIS_UDMA_66 },
	{ "SiS 540", 0x0540, SIS_UDMA_66 },
	{ "SiS 530", 0x0530, SIS_UDMA_66 },
	
	{ "SiS 5600", 0x5600, SIS_UDMA_33 },
	{ "SiS 5598", 0x5598, SIS_UDMA_33 },
	{ "SiS 5597", 0x5597, SIS_UDMA_33 },
	{ "SiS 5591/2", 0x5591, SIS_UDMA_33 },
	{ "SiS 5582", 0x5582, SIS_UDMA_33 },
	{ "SiS 5581", 0x5581, SIS_UDMA_33 },
	
	{ "SiS 5596", 0x5596, SIS_16 },
	{ "SiS 5571", 0x5571, SIS_16 },
	{ "SiS 551x", 0x5511, SIS_16 }
	
};

/* {0, ATA_16, ATA_33, ATA_66, ATA_100a, ATA_100, ATA_133} */
static uint8 cycle_time_offset[] = {0,0,5,4,4,0,0};
static uint8 cycle_time_range[] = {0,0,2,3,3,4,4};
static uint8 cycle_time_value[][ATA_SPEED_UDMA_6 - ATA_SPEED_UDMA_0 + 1] = {
	{0,0,0,0,0,0,0}, /* no udma */
	{0,0,0,0,0,0,0}, /* no udma */
	{3,2,1,0,0,0,0}, /* ATA_33 */
	{7,5,3,2,1,0,0}, /* ATA_66 */
	{7,5,3,2,1,0,0}, /* ATA_100a (730 specific), differences are on cycle_time range and offset */
	{11,7,5,4,2,1,0}, /* ATA_100 */
	{15,10,7,5,3,2,1}, /* ATA_133a (earliest 691 southbridges) */
	{15,10,7,5,3,2,1}, /* ATA_133 */
};

/* CRC Valid Setup Time vary across IDE clock setting 33/66/100/133
   See SiS962 data sheet for more detail */
static uint8 cvs_time_value[][ATA_SPEED_UDMA_6 - ATA_SPEED_UDMA_0 + 1] = {
	{0,0,0,0,0,0,0}, /* no udma */
	{0,0,0,0,0,0,0}, /* no udma */
	{2,1,1,0,0,0,0},
	{4,3,2,1,0,0,0},
	{4,3,2,1,0,0,0},
	{6,4,3,1,1,1,0},
	{9,6,4,2,2,2,2},
	{9,6,4,2,2,2,2},
};

struct SIS_private_s
{
	PCI_Info_s sDevice;
	int nSpeed;
};

/* TODO: Tune pio modes */
status_t sis_port_configure( ATA_port_s* psPort )
{
	struct SIS_private_s* psPrivate = psPort->pPrivate;
	PCI_Info_s sDevice = psPrivate->sDevice;
	uint8 nDrivePCI = 0x40;
	int nSpeed = psPrivate->nSpeed;
	int nDriveNum = psPort->nChannel * psPort->psCtrl->nPortsPerChannel + psPort->nPort;
	uint32 nRegDW = 0;
	uint8 nReg = 0;
	int nNewSpeed = psPort->nCurrentSpeed;
	
	if( nSpeed >= SIS_UDMA_133 ) 
	{
		uint32 n54;
		
		n54 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 4 );
		if( n54 & 0x40000000 ) nDrivePCI = 0x70;
		nDrivePCI += nDriveNum * 0x4;
		nRegDW = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										nDrivePCI, 4 );
		if( nNewSpeed < ATA_SPEED_UDMA_0 )
		{
			nRegDW &= 0xfffffffb;
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										nDrivePCI, 4, nRegDW );
		}
	} else {
		nDrivePCI += nDriveNum * 0x2;
		nReg = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										nDrivePCI + 1, 1 );
		if( ( nNewSpeed < ATA_SPEED_UDMA_0 ) && ( nSpeed > SIS_16 ) ) {
			nReg &= 0x7f;
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										nDrivePCI + 1, 1, nReg );
		}
	}
	switch( nNewSpeed )
	{
		case ATA_SPEED_UDMA_6:
		case ATA_SPEED_UDMA_5:
		case ATA_SPEED_UDMA_4:
		case ATA_SPEED_UDMA_3:
		case ATA_SPEED_UDMA_2:
		case ATA_SPEED_UDMA_1:
		case ATA_SPEED_UDMA_0:
			if( nSpeed >= SIS_UDMA_133 ) 
			{
				nRegDW |= 0x04;
				nRegDW &= 0xfffff00f;
				if( nRegDW & 0x08 ) {
					nRegDW |= (unsigned long)cycle_time_value[SIS_UDMA_133][nNewSpeed-ATA_SPEED_UDMA_0] << 4;
					nRegDW |= (unsigned long)cvs_time_value[SIS_UDMA_133][nNewSpeed-ATA_SPEED_UDMA_0] << 8;
				} else {
					if( nNewSpeed > ATA_SPEED_UDMA_5 )
						nNewSpeed = ATA_SPEED_UDMA_5;
					nRegDW |= (unsigned long)cycle_time_value[SIS_UDMA_100][nNewSpeed-ATA_SPEED_UDMA_0] << 4;
					nRegDW |= (unsigned long)cvs_time_value[SIS_UDMA_100][nNewSpeed-ATA_SPEED_UDMA_0] << 8;
				}
				g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										nDrivePCI, 4, nRegDW );
			} else {
				nReg |= 0x80;
				nReg &= ~((0xFF >> (8 - cycle_time_range[nSpeed]))
					 << cycle_time_offset[nSpeed]);
				/* set reg cycle time bits */
				nReg |= cycle_time_value[nSpeed][nNewSpeed-ATA_SPEED_UDMA_0]
					<< cycle_time_offset[nSpeed];
				g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										nDrivePCI + 1, 1, nReg );
			}
			break;
		
	}
	return( 0 );
}

void init_sis_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl )
{
	PCI_Info_s sBridgeDev;
	int i;
	int j;
	bool bFound = false;
	int n80W = 0;
	int nSpeed = 0;
	struct SIS_private_s* psPrivate = kmalloc( sizeof( struct SIS_private_s ), MEMF_KERNEL | MEMF_CLEAR );
	
	/* Scan bridges */
	for( i = 0; g_psPCIBus->get_pci_info( &sBridgeDev, i ) == 0; ++i )
	{
		for( j = 0; j < ( sizeof( g_sSISBridges ) / sizeof( struct SIS_bridge_s ) ); j++ )
		{
			if( sBridgeDev.nVendorID == 0x1039 && sBridgeDev.nDeviceID == g_sSISBridges[j].nDeviceID  )
			{
				nSpeed = g_sSISBridges[j].nFlags;
				bFound = true;
				
				/* Special case for SiS630 : 630S/ET is ATA_100a */
				if( bFound && sBridgeDev.nDeviceID == 0x0630 )
				{
					if( sBridgeDev.nRevisionID >= 0x30 )
						nSpeed = SIS_UDMA_100a;
				}
				kerndbg( KERN_INFO, "%s bridge detected\n", g_sSISBridges[j].pzName );
				
				break;
			}
			
		}
	}
	
	if( !bFound )
	{
		uint32 nMisc;
		uint16 nTrueID;
		uint8 nPrefCtl;
		uint8 nIDECfg;
		uint8 nSBRev;
		
		nMisc = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 4 );
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 4, ( nMisc & 0x7fffffff ) );
		nTrueID = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										PCI_DEVICE_ID, 2 );
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 4, nMisc );
										
		if( nTrueID == 0x5518 )
		{
			nSpeed = SIS_UDMA_133;
			kerndbg( KERN_INFO, "SiS 962/963 MuTIOL IDE UDMA133 controller detected\n" );
			goto found;
		}
		
		nIDECfg = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x4a, 1 );
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x4a, 1, nIDECfg | 0x10 );
		nTrueID = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										PCI_DEVICE_ID, 2 );
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x4a, 1, nIDECfg );
		
		if( nTrueID == 0x5517 ) 
		{
			nSBRev =  g_psPCIBus->read_pci_config( 0, 2, 0,	PCI_REVISION, 1 );
			nPrefCtl = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x49, 1 );
			if( nSBRev == 0x10 && ( nPrefCtl & 0x80 ) ) {
				nSpeed = SIS_UDMA_133a;
				kerndbg( KERN_INFO, "SiS 961B MuTIOL IDE UDMA133 controller detected\n" );
				goto found;
			} else {
				nSpeed = SIS_UDMA_100;
				kerndbg( KERN_INFO, "SiS 961 MuTIOL IDE UDMA100 controller detected\n" );
				goto found;
			}
		}
										
		
		kerndbg( KERN_WARNING, "Unknown SIS ATA controller detected\n" );
		return;
	}
found:
	
	{
		uint8 nReg;
		uint16 nRegW;
		
		switch( nSpeed )
		{
			case SIS_UDMA_133:
				nRegW = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x50, 2 );
				if( nRegW & 0x08 )
					g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x50, 2, nRegW & 0xfff7 );
				nRegW = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x52, 2 );
				if( nRegW & 0x08 )
					g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x52, 2, nRegW & 0xfff7 );
				break;
			case SIS_UDMA_133a:
			case SIS_UDMA_100:
				g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										PCI_LATENCY, 1, 0x80 );
				nReg = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x49, 1 );
				if( !( nReg & 0x01 ) )
					g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x49, 1, nReg | 0x01 );
				break;
			case SIS_UDMA_100a:
			case SIS_UDMA_66:
				g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										PCI_LATENCY, 1, 0x10 );
				nReg = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x52, 1 );
				if( !( nReg & 0x04 ) )
					g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x52, 1, nReg | 0x04 );
				break;
			case SIS_UDMA_33:
				nReg = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x09, 1 );
				if( ( nReg & 0x0f ) != 0x00 )
					g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x9, 1, nReg & 0xf0 );
				break;
			case SIS_16:
				nReg = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x52, 1 );
				if( !( nReg & 0x08 ) )
					g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x52, 1, nReg | 0x08 );
				break;
		}
	}
	
	if( nSpeed >= SIS_UDMA_133 )
	{
		for( i = 0; i < 2; i++ )
		{
			uint16 nRegW;
			uint16 nRegAddr = i ? 0x52 : 0x50;
			nRegW = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										nRegAddr, 2 );
			n80W |= ( nRegW & 0x8000 ) ? 0 : ( 1 << i );
			
		}
	} else if( nSpeed >= SIS_UDMA_66 )
	{
		for( i = 0; i < 2; i++ )
		{
			uint8 nReg;
			uint8 nMask = i ? 0x20 : 0x10;
			nReg = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x48, 1 );
			n80W |= ( nReg & nMask ) ? 0 : ( 1 << i );
			
		}
	} 
	
	
	psCtrl->psPort[0]->nCable = psCtrl->psPort[1]->nCable = ( n80W & 0x1 ) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40;
	psCtrl->psPort[2]->nCable = psCtrl->psPort[3]->nCable = ( n80W & 0x2 ) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40;
	
	
	/* Add speeds */
	psPrivate->sDevice = sDevice;
	psPrivate->nSpeed = nSpeed;
	
	for( i = 0; i < 4; i++ )
	{
		ATA_port_s* psPort = psCtrl->psPort[i];
		bool b80bit = ( i & 0x02 ) ? ( n80W & 0x2 ) : ( n80W & 0x1 );
		
		/* Add all multiword dma and pio modes (right???) */
		psPort->nSupportedPortSpeed = 0xff;
		
		if( nSpeed >= SIS_UDMA_33 ) psPort->nSupportedPortSpeed |= 0x700;
		if( nSpeed >= SIS_UDMA_66 && b80bit ) psPort->nSupportedPortSpeed |= 0x1f00;
		if( nSpeed >= SIS_UDMA_100a && b80bit ) psPort->nSupportedPortSpeed |= 0x3fff;
		if( nSpeed >= SIS_UDMA_133a && b80bit ) psPort->nSupportedPortSpeed |= 0x7fff;
		
		psPort->sOps.configure = sis_port_configure;
		psPort->pPrivate = psPrivate;
	}
	
	strcpy( psCtrl->zName, "SiS ATA controller" );
	claim_device( psCtrl->nDeviceID, sDevice.nHandle, "SiS ATA controller", DEVICE_CONTROLLER );
}



























