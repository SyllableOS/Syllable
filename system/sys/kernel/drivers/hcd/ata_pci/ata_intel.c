/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Intel chipset support
 *
 *	Copyright (C) 2004 Arno Klenke
 *  Copyright (C) 1998-1999 Andrzej Krzysztofowicz, Author and Maintainer
 *  Copyright (C) 1998-2000 Andre Hedrick <andre@linux-ide.org>
 *  Copyright (C) 2003 Red Hat Inc <alan@redhat.com>
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

#define INTEL_UDMA_33 0x01
#define INTEL_UDMA_66 0x02
#define INTEL_UDMA_100 0x04
#define INTEL_UDMA_133 0x08
#define INTEL_UDMA 0x0f
#define INTEL_NO_MWDMA0 0x10
#define INTEL_INIT 0x20
#define INTEL_80W 0x40

struct Intel_ide_s {
	char* pzName;
	uint16 nDeviceID;
	uint16 nFlags;
};

static struct Intel_ide_s g_sIntelPATA[] = {
	{ "PIIXa", 0x122e, 0 },
	{ "PIIXb", 0x1230, 0 },
	{ "MPIIX", 0x1234, 0 },
	{ "PIIX3", 0x7010, 0 },
	{ "PIIX4", 0x7111, INTEL_UDMA_33 },
	{ "PIIX4", 0x7198, INTEL_UDMA_33 },
	{ "PIIX4", 0x7601, INTEL_UDMA_66 },
	{ "PIIX4", 0x84ca, INTEL_UDMA_33 },
	{ "ICH0", 0x2421, INTEL_UDMA_33 | INTEL_NO_MWDMA0 | INTEL_INIT },
	{ "ICH", 0x2411, INTEL_UDMA_66 | INTEL_NO_MWDMA0 | INTEL_INIT | INTEL_80W },
	{ "ICH2", 0x244b, INTEL_UDMA_100 | INTEL_NO_MWDMA0 | INTEL_INIT | INTEL_80W },
	{ "ICH2M", 0x244a, INTEL_UDMA_100 | INTEL_NO_MWDMA0 | INTEL_INIT | INTEL_80W },
	{ "ICH3M", 0x248a, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ICH3", 0x248b, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ICH4", 0x24cb, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ICH5", 0x24db, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "C-ICH", 0x245b, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ICH4", 0x24ca, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ESB2", 0x25a2, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ICH6", 0x266f, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ICH7", 0x27df, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ICH7", 0x24c1, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ESB2", 0x269e, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W },
	{ "ICH8", 0x2850, INTEL_UDMA_100 | INTEL_INIT | INTEL_80W }
};

static struct Intel_ide_s g_sIntelSATA[] = {
	{ "ICH5 SATA", 0x24d1, INTEL_UDMA_133 | INTEL_INIT },
	{ "ICH5 SATA", 0x24df, INTEL_UDMA_133 | INTEL_INIT },
	{ "ICH5 SATA", 0x25a3, INTEL_UDMA_133 | INTEL_INIT },
	{ "ICH5 SATA", 0x25b0, INTEL_UDMA_133 | INTEL_INIT },
	{ "ICH6 SATA", 0x2651, INTEL_UDMA_133 | INTEL_INIT }
};

struct Intel_private_s
{
	PCI_Info_s sDevice;
};

/* Set PIO speed ( even required in dma mode ) */
void intel_port_configure_pio( ATA_port_s* psPort )
{
	struct Intel_private_s* psPrivate = psPort->pPrivate;
	PCI_Info_s sDevice = psPrivate->sDevice;
	int i;
	int pio = -1;
	int master_port		= psPort->nChannel ? 0x42 : 0x40;
	int slave_port		= 0x44;
	int is_slave		= psPort->nPort == 1;
	uint16 master_data;
	uint8 slave_data = 0;
				 /* ISP  RTC */
	uint8 timings[][2]	= { { 0, 0 },
			    { 0, 0 },
			    { 1, 0 },
			    { 2, 1 },
			    { 2, 3 }, };
	
	for( i = ATA_SPEED_PIO_5; i >= 0; i-- )
	{
		if( ( psPort->nSupportedPortSpeed & ( 1 << i ) ) &&
			( psPort->nSupportedDriveSpeed & ( 1 << i ) ) )
		{
			pio = i + 2;
			break;
		}
	}
	if( pio < 0 )
		return;
	
	master_data = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										master_port, 2 );
	if (is_slave) {
		master_data = master_data | 0x4000;
		if (pio > 1)
			/* enable PPE, IE and TIME */
			master_data = master_data | 0x0070;
		slave_data = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										slave_port, 1 );
		slave_data = slave_data & (psPort->nChannel ? 0x0f : 0xf0);
		slave_data = slave_data | (((timings[pio][0] << 2) | timings[pio][1]) << (psPort->nChannel ? 4 : 0));
	} else {
		master_data = master_data & 0xccf8;
		if (pio > 1)
			/* enable PPE, IE and TIME */
			master_data = master_data | 0x0007;
		master_data = master_data | (timings[pio][0] << 12) | (timings[pio][1] << 8);
	}
	g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										master_port, 2, master_data );
	if (is_slave)
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										slave_port, 1, slave_data );
}

/* Set controller speed */
status_t intel_port_configure( ATA_port_s* psPort )
{
	struct Intel_private_s* psPrivate = psPort->pPrivate;
	PCI_Info_s sDevice = psPrivate->sDevice;
	uint8 maslave		= psPort->nChannel ? 0x42 : 0x40;
	uint8 speed		= psPort->nCurrentSpeed;
	int nDriveNum = psPort->nChannel * psPort->psCtrl->nPortsPerChannel + psPort->nPort;
	int a_speed		= 3 << (nDriveNum * 4);
	int u_flag		= 1 << nDriveNum;
	int v_flag		= 0x01 << nDriveNum;
	int w_flag		= 0x10 << nDriveNum;
	int u_speed		= 0;
	int			sitre;
	uint16			reg4042, reg44, reg48, reg4a, reg54;
	uint8			reg55;
	
	reg4042 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										maslave, 2 );

	sitre = (reg4042 & 0x4000) ? 1 : 0;
	reg44 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x44, 2 );
	reg48 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x48, 2 );
	reg4a = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x4a, 2 );
	reg54 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 2 );
	reg55 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x55, 1 );
	
	/* Enable DMA mode */
	#if 0
	if( speed >= ATA_SPEED_DMA )
	{
		uint8 nStatus;
		ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus );
		nStatus |= ( psPort->nPort == 1 ) ? ATA_DMA_STATUS_1_EN : ATA_DMA_STATUS_0_EN;
		ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus );
	}
	#endif
	
	switch(speed) {
		case ATA_SPEED_UDMA_4:
		case ATA_SPEED_UDMA_2:	u_speed = 2 << (nDriveNum * 4); break;
		case ATA_SPEED_UDMA_5:
		case ATA_SPEED_UDMA_3:
		case ATA_SPEED_UDMA_1:	u_speed = 1 << (nDriveNum * 4); break;
		case ATA_SPEED_UDMA_0:	u_speed = 0 << (nDriveNum * 4); break;
		case ATA_SPEED_MWDMA_0:
		case ATA_SPEED_MWDMA_1:
		case ATA_SPEED_MWDMA_2:	break;
		case ATA_SPEED_PIO_5:
		case ATA_SPEED_PIO_4:
		case ATA_SPEED_PIO_3:
		case ATA_SPEED_PIO:	break;
		default:		return -1;
	}

	if (speed >= ATA_SPEED_UDMA_0) {
		if (!(reg48 & u_flag))
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x48, 2, reg48|u_flag );
		if (speed == ATA_SPEED_UDMA_5) {
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x55, 1, (uint8) reg55|w_flag );
		} else {
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x55, 1, (uint8) reg55 & ~w_flag );
		}
		if (!(reg4a & u_speed)) {
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x4a, 2, reg4a & ~a_speed );
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x4a, 2, reg4a|u_speed );
		}
		if (speed > ATA_SPEED_UDMA_2) {
			if (!(reg54 & v_flag)) {
				g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 2, reg54|v_flag );
			}
		} else {
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 2, reg54 & ~v_flag );
		}
	} else {
		if (reg48 & u_flag)
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x48, 2, reg48 & ~u_flag );
		if (reg4a & a_speed)
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x4a, 2, reg4a & ~a_speed );
		if (reg54 & v_flag)
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 2, reg54 & ~v_flag );
		if (reg55 & w_flag)
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x55, 1, (uint8) reg55 & ~w_flag );
	}
	
	intel_port_configure_pio( psPort );
	
	
	return( 0 );
}

void init_intel_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl )
{
	int i;
	bool bFound = false;
	struct Intel_ide_s* psIDE = NULL;
	uint8 n54, n55;
	int n80W = 0;
	struct Intel_private_s* psPrivate = kmalloc( sizeof( struct Intel_private_s ), MEMF_KERNEL | MEMF_CLEAR );

	/* Find controller */
	for( i = 0; i < ( sizeof( g_sIntelPATA ) / sizeof( struct Intel_ide_s ) ); i++ )
		if( sDevice.nDeviceID == g_sIntelPATA[i].nDeviceID )
		{
			psIDE = &g_sIntelPATA[i];
			bFound = true;
			break;
		}
	
	if( !bFound )
	{
		kerndbg( KERN_WARNING, "Unknown Intel ATA controller detected\n" );
		return;
	}
	
	kerndbg( KERN_INFO, "Intel %s controller detected\n", psIDE->pzName );
	
	/* Initialize */
	if( psIDE->nFlags & INTEL_INIT )
	{
		unsigned int nExtra = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 4 );
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 4, nExtra | 0x400 );
	}
	
	/* Check cable type */
	if( psIDE->nFlags & INTEL_80W )
	{
		n54 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 1 );
		n55 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x55, 1 );
		if( n54 & 0x30 )
			n80W |= 0x1;
		if( n54 & 0xc0 )
			n80W |= 0x2;
	} 
	
	
	psCtrl->psPort[0]->nCable = psCtrl->psPort[1]->nCable = ( n80W & 0x1 ) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40;
	psCtrl->psPort[2]->nCable = psCtrl->psPort[3]->nCable = ( n80W & 0x2 ) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40;
	
	psPrivate->sDevice = sDevice;
	
	for( i = 0; i < 4; i++ )
	{
		ATA_port_s* psPort = psCtrl->psPort[i];
		
		/* Add all multiword dma and pio modes (right???) */
		psPort->nSupportedPortSpeed = 0xff;
		bool b80bit = ( i & 0x02 ) ? ( n80W & 0x2 ) : ( n80W & 0x1 );
		
		if( psIDE->nFlags & INTEL_NO_MWDMA0 ) {
			kerndbg( KERN_DEBUG, "Disabling Multiword DMA mode 0\n" );
			psPort->nSupportedPortSpeed = psPort->nSupportedPortSpeed & ~0x20;
		}
		
		if( psIDE->nFlags & INTEL_UDMA ) psPort->nSupportedPortSpeed |= 0x700;
		if( ( psIDE->nFlags & INTEL_UDMA_66 ) && b80bit ) psPort->nSupportedPortSpeed |= 0x1f00;
		if( ( psIDE->nFlags & INTEL_UDMA_100 ) && b80bit ) psPort->nSupportedPortSpeed |= 0x3fff;
		if( ( psIDE->nFlags & INTEL_UDMA_133 ) && b80bit ) psPort->nSupportedPortSpeed |= 0x7fff;
		
		
		psPort->sOps.configure = intel_port_configure;
		psPort->pPrivate = psPrivate;
	}
	
	strcpy( psCtrl->zName, "Intel ATA controller" );
	claim_device( psCtrl->nDeviceID, sDevice.nHandle, "Intel ATA controller", DEVICE_CONTROLLER );
}

void init_intel_sata_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl )
{
	int i;
	struct Intel_ide_s* psIDE = NULL;
	struct Intel_private_s* psPrivate = kmalloc( sizeof( struct Intel_private_s ), MEMF_KERNEL | MEMF_CLEAR );

	/* Only one port per channel (right?) */
	kfree( psCtrl->psPort[1] );
	kfree( psCtrl->psPort[3] );
	psCtrl->psPort[1] = psCtrl->psPort[2];
	psCtrl->psPort[2] = NULL;
	psCtrl->nPortsPerChannel = 1;

	psPrivate->sDevice = sDevice;

	for( i = 0; i < 2; i++ )
	{
		ATA_port_s *psPort = psCtrl->psPort[i];

		psPort->nCable = ATA_CABLE_SATA;
		psPort->nType = ATA_SATA;
		psPort->nSupportedPortSpeed |= 0x7fff;	/* UDMA133 */
		psPort->sOps.configure = intel_port_configure;
		psPort->pPrivate = psPrivate;
	}

	/* Find controller */
	for( i = 0; i < ( sizeof( g_sIntelSATA ) / sizeof( struct Intel_ide_s ) ); i++ )
		if( sDevice.nDeviceID == g_sIntelSATA[i].nDeviceID )
		{
			psIDE = &g_sIntelSATA[i];
			break;
		}

	kerndbg( KERN_INFO, "Intel %s controller detected\n", psIDE->pzName );

	/* Initialize */
	if( psIDE->nFlags & INTEL_INIT )
	{
		unsigned int nExtra = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 4 );
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction,
										0x54, 4, nExtra | 0x400 );
	}

	strcpy( psCtrl->zName, "Intel Serial ATA controller" );
	claim_device( psCtrl->nDeviceID, sDevice.nHandle, "Intel Serial ATA controller", DEVICE_CONTROLLER );

	return;
}

