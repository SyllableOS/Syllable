/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Generic PCI ATA controller driver
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

#include "ata.h"
#include <atheos/pci.h>
#include <atheos/time.h>
#include <posix/signal.h>

typedef void init_ata_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl );

typedef struct
{
	uint32 nVendorID;
	uint32 nDeviceID;
	init_ata_controller* psInit;
} ATA_PCI_dev_s;

void init_via_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl );
void init_via_sata_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl );
void init_intel_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl );
void init_intel_sata_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl );
void init_sis_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl );
void init_amd_controller( PCI_Info_s sDevice, ATA_controller_s* psCtrl );

/* List of supported chipsets */
ATA_PCI_dev_s g_sDevices[] = 
{
	/* VIA */
	{ 0x1106, 0x1571, init_via_controller },
	{ 0x1106, 0x0571, init_via_controller },
	{ 0x1106, 0x3164, init_via_controller },
	/* VIA SATA */
	{ 0x1106, 0x0591, init_via_sata_controller },	/* vt6420 */
	{ 0x1106, 0x3149, init_via_sata_controller },	/* vt6420 */
	{ 0x1106, 0x3249, init_via_sata_controller },	/* vt6421 */
	/* Intel */
	{ 0x8086, 0x122e, init_intel_controller },
	{ 0x8086, 0x1230, init_intel_controller },
	{ 0x8086, 0x1234, init_intel_controller },
	{ 0x8086, 0x7010, init_intel_controller },
	{ 0x8086, 0x7111, init_intel_controller },
	{ 0x8086, 0x7198, init_intel_controller },
	{ 0x8086, 0x7601, init_intel_controller },
	{ 0x8086, 0x84ca, init_intel_controller },
	{ 0x8086, 0x2421, init_intel_controller },
	{ 0x8086, 0x2411, init_intel_controller },
	{ 0x8086, 0x244b, init_intel_controller },
	{ 0x8086, 0x244a, init_intel_controller },
	{ 0x8086, 0x248a, init_intel_controller },
	{ 0x8086, 0x248b, init_intel_controller },
	{ 0x8086, 0x24cb, init_intel_controller },
	{ 0x8086, 0x24db, init_intel_controller },
	{ 0x8086, 0x245b, init_intel_controller },
	{ 0x8086, 0x24ca, init_intel_controller },
	{ 0x8086, 0x25a2, init_intel_controller },
	{ 0x8086, 0x266f, init_intel_controller },	/* ICH6 */
	{ 0x8086, 0x27df, init_intel_controller },
	{ 0x8086, 0x24c1, init_intel_controller },
	{ 0x8086, 0x269e, init_intel_controller },
	{ 0x8086, 0x2850, init_intel_controller },
	/* Intel SATA */
	{ 0x8086, 0x24d1, init_intel_sata_controller },	/* ICH5 SATA */
	{ 0x8086, 0x24df, init_intel_sata_controller },	/* ICH5 SATA */
	{ 0x8086, 0x25a3, init_intel_sata_controller },	/* ICH5 SATA */
	{ 0x8086, 0x25b0, init_intel_sata_controller },
	{ 0x8086, 0x2651, init_intel_sata_controller },	/* ICH6/ICH6W SATA */
	/* SIS */
	{ 0x1039, 0x5513, init_sis_controller },
	/* AMD */
	{ 0x1022, 0x7401, init_amd_controller },
	{ 0x1022, 0x7409, init_amd_controller },
	{ 0x1022, 0x7411, init_amd_controller },
	{ 0x1022, 0x7441, init_amd_controller },
	{ 0x1022, 0x7469, init_amd_controller },
	{ 0x1022, 0x209a, init_amd_controller },
	/* nVIDIA */
	{ 0x10de, 0x01bc, init_amd_controller },
	{ 0x10de, 0x0065, init_amd_controller },
	{ 0x10de, 0x0085, init_amd_controller },
	{ 0x10de, 0x008e, init_amd_controller },
	{ 0x10de, 0x00d5, init_amd_controller },
	{ 0x10de, 0x00e5, init_amd_controller },
	{ 0x10de, 0x00e3, init_amd_controller },
	{ 0x10de, 0x00ee, init_amd_controller },
	{ 0x10de, 0x0053, init_amd_controller },
	{ 0x10de, 0x0035, init_amd_controller },
	{ 0x10de, 0x0265, init_amd_controller },
	{ 0x10de, 0x036e, init_amd_controller },
	{ 0x10de, 0x03ec, init_amd_controller },
	{ 0x10de, 0x0448, init_amd_controller },	
	{ 0x10de, 0x0054, init_amd_controller },	
	{ 0x10de, 0x0055, init_amd_controller },
	{ 0x10de, 0x0036, init_amd_controller },
	{ 0x10de, 0x003e, init_amd_controller },	
	{ 0x10de, 0x0266, init_amd_controller },
	{ 0x10de, 0x037e, init_amd_controller },
	{ 0x10de, 0x037f, init_amd_controller },
	{ 0x10de, 0x03e7, init_amd_controller },
	{ 0x10de, 0x03f6, init_amd_controller },
	{ 0x10de, 0x03f7, init_amd_controller }
};

static bool g_bLegacyController = false;
static bool g_bForceLegacy = false;
static bool g_bEnableSpeeds = false;
PCI_bus_s* g_psPCIBus;
static ATA_bus_s* g_psBus;


/* Initialize the controllers first and put them into this list and then add
   them to the ata busmanager. This is necessary because some mainboards seem to
   map their pata and sata controllers at the same port to support older ata drivers. */
#define ATA_PCI_CONTROLLERS 8
static ATA_controller_s* g_apsControllers[ATA_PCI_CONTROLLERS];
static int g_nControllers = 0;

#define ATA_CMD_DELAY 500
#define ATA_CMD_TIMEOUT 315000

status_t ata_pci_wait( ATA_port_s* psPort, int nMask, int nValue )
{
	bigtime_t nTime = get_system_time();
	
	while( get_system_time() - nTime < ATA_CMD_TIMEOUT * 20 )
	{
		uint8 nStatus;
		ATA_READ_REG( psPort, ATA_REG_STATUS, nStatus )
		if( ( nStatus & nMask ) == nValue )
			return( 0 );
	}
	
	kerndbg( KERN_WARNING, "ata_pci_wait() %x %x timed out\n", (uint)nMask, (uint)nValue );
	return( -1 );
}

int ata_pci_interrupt( int nIrq, void *pPort, SysCallRegs_s* psRegs )
{
	ATA_port_s* psPort = pPort;
	uint8 nStatus = 0;
	uint8 nControl = 0;
	uint8 nDevControl;
	uint8 nDevStatus;
	
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	
	//printk( "Got ATA interrupt status %x\n", (uint)nStatus );
	
	if( !( nStatus & ATA_DMA_STATUS_IRQ ) )
	{
		/* Not our IRQ */
		//printk( "Got shared interrupt\n" );
		return( 0 );
	}
	

	/* Stop transfer */
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	nControl &= ~ATA_DMA_CONTROL_START;
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	ATA_READ_REG( psPort, ATA_REG_CONTROL, nDevControl )
	
	/* Check control reg */
	ATA_READ_REG( psPort, ATA_REG_CONTROL, nDevControl )
	if( nDevControl & ATA_STATUS_BUSY )
		kerndbg( KERN_WARNING, "Warning: Drive altstatus still busy after DMA transfer\n" );
		
	/* Check status reg. This clears the irq */
	ATA_READ_REG( psPort, ATA_REG_STATUS, nDevStatus )
	if( nDevStatus & ATA_STATUS_BUSY )
		kerndbg( KERN_WARNING, "Warning: Drive status still busy after DMA transfer\n" );

	/* Clear status */	
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	if( nStatus & ATA_DMA_STATUS_ERROR )
	{
		kerndbg( KERN_FATAL, "Error: ATA/ATAPI DMA transfer failed\n" );
		psPort->bIRQError = true;
	}
	unlock_semaphore( psPort->hIRQWait );
	
	return( 0 );
}

static status_t ata_check_port( ATA_port_s* psPort )
{
	/* Select drive */
	uint8 nCount, nLbaLow, nControl;
	ATA_WRITE_REG( psPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT )
	if( psPort->nPort == 0 )
		ATA_WRITE_REG( psPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT )
	else
		ATA_WRITE_REG( psPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT | ATA_DEVICE_SLAVE )
	ATA_READ_REG( psPort, ATA_REG_CONTROL, nControl )
	udelay( ATA_CMD_DELAY );
	
	/* Reset */
	ATA_WRITE_REG( psPort, ATA_REG_COUNT, 0x55 )
	ATA_WRITE_REG( psPort, ATA_REG_LBA_LOW, 0xaa )
	ATA_WRITE_REG( psPort, ATA_REG_COUNT, 0x55 )
	ATA_WRITE_REG( psPort, ATA_REG_LBA_LOW, 0xaa )
	ATA_WRITE_REG( psPort, ATA_REG_COUNT, 0x55 )
	ATA_WRITE_REG( psPort, ATA_REG_LBA_LOW, 0xaa )
	
	/* Check registers */
	ATA_READ_REG( psPort, ATA_REG_COUNT, nCount )
	ATA_READ_REG( psPort, ATA_REG_LBA_LOW, nLbaLow )
	
	/* Some drives (E.g. NEC, Toshiba) drives can't get this right and set nCount to 0x01. */
	if( !( ( nCount == 0x55 || nCount == 0x01 ) && nLbaLow == 0xaa ) )
		return( -1 );

	return( 0 );
}

status_t ata_pci_reset( ATA_port_s* psMaster, ATA_port_s* psSlave )
{
	bool bMaster = false;
	bool bSlave = false;
	uint8 nCount, nLbaLow, nControl, nStatus;
	

	if( ata_check_port( psMaster ) == 0 )
		bMaster = true;
	if( psSlave != NULL && ata_check_port( psSlave ) == 0 )
		bSlave = true;
	
	/* Select master */
	ATA_WRITE_REG( psMaster, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT )
	ATA_READ_REG( psMaster, ATA_REG_CONTROL, nControl )
	udelay( ATA_CMD_DELAY );
	
	/* Bus reset */
	ATA_WRITE_REG( psMaster, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT )
	udelay( 10 );
	ATA_WRITE_REG( psMaster, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT | ATA_CONTROL_RESET )
	udelay( 10 );
	ATA_WRITE_REG( psMaster, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT )
	udelay( 10 );
	
	snooze( 15000 );
	
	ATA_READ_REG( psMaster, ATA_REG_STATUS, nStatus )
	if( nStatus == 0xff )
	{
		if( bMaster )
			psMaster->nDevice = ATA_DEV_UNKNOWN;
		if( bSlave )
			psSlave->nDevice = ATA_DEV_UNKNOWN;
		return( 0 );
	}
	
	if( bMaster ) {
		if( ata_pci_wait( psMaster, ATA_STATUS_BUSY, 0 ) < 0 )
		{
			ATA_READ_REG( psMaster, ATA_REG_STATUS, nStatus )
			kerndbg( KERN_FATAL, "Timeout while resetting master with status %x\n", nStatus );
		}
	}
	
	/* Check slave */
	bigtime_t nTimeout = get_system_time() + ATA_CMD_TIMEOUT;
	while( bSlave )
	{
		/* Select */
		ATA_WRITE_REG( psSlave, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT | ATA_DEVICE_SLAVE )
		ATA_READ_REG( psSlave, ATA_REG_CONTROL, nControl )
		udelay( ATA_CMD_DELAY );
		ATA_READ_REG( psSlave, ATA_REG_COUNT, nCount )
		ATA_READ_REG( psSlave, ATA_REG_LBA_LOW, nLbaLow )
		
		if( ( nCount == 0x01 && nLbaLow == 0x01 ) )
			break;
		if( get_system_time() > nTimeout )
		{
			printk( "Timeout while waiting for slave!\n" );
			bSlave = false;
			break;
		}
		snooze( 50000 );
	}
	
	if( bSlave ) {
		if( ata_pci_wait( psSlave, ATA_STATUS_BUSY, 0 ) < 0 )
		{
			ATA_READ_REG( psSlave, ATA_REG_STATUS, nStatus )
			kerndbg( KERN_FATAL, "Timeout while resetting slave with status %x\n", nStatus );
		}
	}
		

	if( bMaster )
		psMaster->nDevice = ATA_DEV_UNKNOWN;
	if( bSlave )
		psSlave->nDevice = ATA_DEV_UNKNOWN;
		
	return( 0 );
}

status_t ata_pci_reset_dummy( ATA_port_s* psPort )
{
	return( 0 );
}

status_t ata_pci_add_controller( int nDeviceID, PCI_Info_s sDevice, init_ata_controller* psInit )
{
	bool bMMIO = false;
	bool bNativeMode = false;
	bool bDMAPossible = true;
	int i;
	uint32 nCommand1, nCommand2, nControl1, nControl2, nDMA1, nDMA2;
	int nIrq1, nIrq2;
	sem_id hLock1, hLock2;
	sem_id hIRQWait1, hIRQWait2;
	ATA_controller_s* psCtrl;
	const char* const *argv;
	int argc;
	
	printk( "ATA PCI controller detected on %i:%i:%i\n", sDevice.nBus, sDevice.nDevice, sDevice.nFunction );
			
	/* Enable a disabled controller */
	if( ( g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_COMMAND, 2 ) & ( PCI_COMMAND_IO | PCI_COMMAND_MASTER ) ) 
			!= ( PCI_COMMAND_IO | PCI_COMMAND_MASTER ) )
	{
		g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_COMMAND, 2, 
			g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_COMMAND, 2 ) | PCI_COMMAND_IO | PCI_COMMAND_MASTER );
		if( ( g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_COMMAND, 2 ) & ( PCI_COMMAND_IO | PCI_COMMAND_MASTER ) ) 
			!= ( PCI_COMMAND_IO | PCI_COMMAND_MASTER ) )
		{
			kerndbg( KERN_FATAL, "Controller could not be enabled\n" );
			return( -1 );
		}
		kerndbg( KERN_FATAL, "Controller enabled by driver\n" );
	}
	
	/* Check controller mode */
	if( ( ( g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, 0x09, 1 ) & 0x05 ) == 0x0 )
		&& psInit != init_via_sata_controller )
	{
		kerndbg( KERN_INFO, "Controller in legacy mode\n" );
		if( g_bLegacyController )
		{
			/* Try to switch to PCI native mode if another controller is already in legacy mode */
			kerndbg( KERN_WARNING, "Another controller is already in legacy mode -> Switching to PCI native mode\n" );
			if( ( g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, 0x09, 1 ) & 0x0a ) != 0x0a )
			{
				kerndbg( KERN_FATAL, "Switch to PCI native mode not possible\n" );
				return( -1 );
			}
			g_psPCIBus->write_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, 0x09, 1, 
				g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, 0x09, 1 ) | 0x05 );
			bNativeMode = true;
		} else {
			/* Default legacy ATA registers */
			nCommand1 = 0x1f0;
			nCommand2 = 0x170;
			nControl1 = 0x3f6;
			nControl2 = 0x376;
			nIrq1 = 14;
			nIrq2 = 15;
			
			nDMA1 = nDMA2 = 0;
			
			get_kernel_arguments( &argc, &argv );

			/* Let the user override the base address */
			for( i = 0; i < argc; ++i )
			{
				if( get_num_arg( &nCommand1, "ata_pci_legacy_cmd1=", argv[i], strlen( argv[i] ) ) )
					kerndbg( KERN_WARNING, "User specified command register base for channel 0\n" );
				if( get_num_arg( &nCommand2, "ata_pci_legacy_cmd2=", argv[i], strlen( argv[i] ) ) )
					kerndbg( KERN_WARNING, "User specified command register base for channel 1\n" );
				if( get_num_arg( &nControl1, "ata_pci_legacy_ctrl1=", argv[i], strlen( argv[i] ) ) )
					kerndbg( KERN_WARNING, "User specified control register base for channel 0\n" );
				if( get_num_arg( &nControl2, "ata_pci_legacy_ctrl2=", argv[i], strlen( argv[i] ) ) )
					kerndbg( KERN_WARNING, "User specified control register base for channel 1\n" );
			}
			
			
			nDMA1 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_BASE_REGISTERS + 16, 4 );
			if( !( nDMA1 & 0x01 ) || !( nDMA1 & PCI_ADDRESS_IO_MASK ) )
			{
				kerndbg( KERN_WARNING, "DMA registers in MMIO mode or not present\n" );
				kerndbg( KERN_WARNING, "Disabling DMA\n" );
				bDMAPossible = false;
				nDMA1 = 0;
			} else {
				nDMA1 = nDMA1 & PCI_ADDRESS_IO_MASK;
				nDMA2 = nDMA1 + 0x08;
			}
			g_bLegacyController = true;
		}
	} else {
		if( g_bForceLegacy ) {
			printk( "Ignoring PCI native controller\n" );
			return( -1 );
		}
		bNativeMode = true;
	}
		
	/* Read PCI native registers */
	if( bNativeMode )
	{
		uint32 nMask;
		
		nCommand1 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_BASE_REGISTERS + 0, 4 );
		nCommand2 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_BASE_REGISTERS + 8, 4 );
		nControl1 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_BASE_REGISTERS + 4, 4 );
		nControl2 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_BASE_REGISTERS + 12, 4 );
		nIrq1 = nIrq2 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_INTERRUPT_LINE, 1 );
		
		if( !( nCommand1 & 0x1 ) ) {
			kerndbg( KERN_INFO, "Controller uses MMIO registers\n" );
			bMMIO = true;
			nMask = PCI_ADDRESS_MEMORY_32_MASK;
		} else {
			kerndbg( KERN_INFO, "Controller uses PIO registers\n" );
			nMask = PCI_ADDRESS_IO_MASK;
		}
		
		if( nIrq1 < 5 || nIrq1 > 24 || nIrq2 < 5 || nIrq2 > 24 )
		{
			kerndbg( KERN_WARNING, "Invalid IRQ values %i and/or %i. Disabling DMA\n", nIrq1, nIrq2 );
			bDMAPossible = false;
		}
			
		nCommand1 = nCommand1 & nMask;
		nCommand2 = nCommand2 & nMask;
		nControl1 = ( nControl1 & nMask ) | 2;
		nControl2 = ( nControl2 & nMask ) | 2;
		
		nDMA1 = nDMA2 = 0;
			
		nDMA1 = g_psPCIBus->read_pci_config( sDevice.nBus, sDevice.nDevice, sDevice.nFunction, PCI_BASE_REGISTERS + 16, 4 );
		if( ( !( nDMA1 & 0x01 ) != bMMIO ) || !( nDMA1 & nMask ) )
		{
			kerndbg( KERN_WARNING, "DMA registers in different mode than base registers or not present\n" );
			kerndbg( KERN_WARNING, "Disabling DMA\n" );
			bDMAPossible = false;
			nDMA1 = 0;
		} else {
			nDMA1 = nDMA1 & nMask;
			nDMA2 = nDMA1 + 0x08;
		}
	}
	
	/* Print out data */
	kerndbg( KERN_INFO, "Channel 0: %s Cmd @ %x Ctrl @ %x DMA @ %x IRQ @ %i\n", bMMIO ? "MMIO" : "PIO",
					(uint)nCommand1, (uint)nControl1, (uint)nDMA1, nIrq1 );
	kerndbg( KERN_INFO, "Channel 1: %s Cmd @ %x Ctrl @ %x DMA @ %x IRQ @ %i\n", bMMIO ? "MMIO" : "PIO",
					(uint)nCommand2, (uint)nControl2, (uint)nDMA2, nIrq2 );			
	
	
	/* Create locks */
	if( nIrq1 != nIrq2 ) {
		hLock1 = create_semaphore( "ata_pci_port_lock", 1, 0 );
		hLock2 = create_semaphore( "ata_pci_port_lock", 1, 0 );
		hIRQWait1 = create_semaphore( "ata_pci_irq_wait", 0, 0 );
		hIRQWait2 = create_semaphore( "ata_pci_irq_wait", 0, 0 );
	} else {
		hLock1 = hLock2 = create_semaphore( "ata_pci_port_lock", 1, 0 );
		hIRQWait1 = hIRQWait2 = create_semaphore( "ata_pci_irq_wait", 0, 0 );
	}
	
	/* Allocate controller */
	psCtrl = g_psBus->alloc_controller( nDeviceID, 2, 2 );
	strcpy( psCtrl->zName, "PCI ATA controller" );
	
	/* Allocate and register port */
	for( i = 0; i < 4; i++ )
	{
		int j;
		ATA_port_s* psPort = g_psBus->alloc_port( psCtrl );
		uint32 nRegBase = ( i & 2 ) ? nCommand2 : nCommand1;
		uint32 nDmaRegBase = ( i & 2 ) ? nDMA2 : nDMA1;
		uint8 nDmaMask = ( i & 1 ) ? ATA_DMA_STATUS_1_EN : ATA_DMA_STATUS_0_EN;
		sem_id hLock = ( i & 2 ) ? hLock2 : hLock1;
		sem_id hIRQWait = ( i & 2 ) ? hIRQWait2 : hIRQWait1;
		uint32 nDmaSpeedMask = g_bEnableSpeeds ? 0xffffffff : ( ( 1 << ATA_SPEED_PIO ) | ( 1 << ATA_SPEED_DMA ) );
		
		psPort->hPortLock = hLock;
		psPort->hIRQWait = hIRQWait;
		psPort->sOps.reset = ata_pci_reset_dummy;
		psPort->bMMIO = bMMIO;
		if( bDMAPossible && bMMIO && ( *((uint8*)nDmaRegBase + 2) & nDmaMask ) )
			psPort->nSupportedPortSpeed = nDmaSpeedMask;
		else if( bDMAPossible && !bMMIO && ( inb( nDmaRegBase + 2 ) & nDmaMask ) )
			psPort->nSupportedPortSpeed = nDmaSpeedMask;
		else
			psPort->nSupportedPortSpeed = ( 1 << ATA_SPEED_PIO );
		psPort->nType = ATA_PATA;
		psPort->nCable = ATA_CABLE_UNKNOWN;
		psPort->pDMATable = alloc_real( PAGE_SIZE / 2 + 4, 0 );
		if( !psPort->pDMATable )
		{
			kerndbg( KERN_FATAL, "Failed to allocate DMA buffer\n" );
			return( -1 );
		}
		psPort->pDMATable = ( uint8* )( ( (uint32)psPort->pDMATable + 4 ) & ~3 );
		
		
		/* Fill registers */
		for( j = 0; j < 8; j++ )
			psPort->nRegs[j] = nRegBase + j;
		psPort->nRegs[ATA_REG_CONTROL] = ( i & 2 ) ? nControl2 : nControl1;
		
		psPort->nDmaRegs[ATA_REG_DMA_CONTROL] = nDmaRegBase;
		psPort->nDmaRegs[ATA_REG_DMA_STATUS] = nDmaRegBase + 2;
		psPort->nDmaRegs[ATA_REG_DMA_TABLE] = nDmaRegBase + 4;
		
		psCtrl->psPort[i] = psPort;
		
	}
	
	/* Interrupts */
	if( bDMAPossible ) {
		if( request_irq( nIrq1, ata_pci_interrupt, NULL, SA_SHIRQ, "ata_pci_irq", psCtrl->psPort[0] ) < 0 ) {
			kerndbg( KERN_FATAL, "Could not request interrupt %i\n", nIrq1 );
		} 
		if( request_irq( nIrq2, ata_pci_interrupt, NULL, SA_SHIRQ, "ata_pci_irq", psCtrl->psPort[2] ) < 0 ) {
			kerndbg( KERN_FATAL, "Could not request interrupt %i\n", nIrq2 );
		}
	}
	
	/* Chipset specific initialization */
	if( psInit && bDMAPossible )
		psInit( sDevice, psCtrl );
		
	/* Reset */
	for( i = 0; i < psCtrl->nChannels; i++ )
	{
		ATA_port_s* psMaster = psCtrl->psPort[psCtrl->nPortsPerChannel*i];
		if( psCtrl->nPortsPerChannel == 1 )
			ata_pci_reset( psMaster, NULL );
		else
			ata_pci_reset( psMaster, psCtrl->psPort[psCtrl->nPortsPerChannel*i+1] );
	}
	
	/* Put the controller into the list */
	g_apsControllers[g_nControllers++] = psCtrl;
	
	
	return( 0 );
}


status_t ata_legacy_add_controller( int nDeviceID )
{
	int i;
	uint32 nCommand1, nCommand2, nControl1, nControl2;
	int nIrq1, nIrq2;
	sem_id hLock1, hLock2;
	sem_id hIRQWait1, hIRQWait2;
	ATA_controller_s* psCtrl;
	const char* const *argv;
	int argc;
	
	printk( "Assuming generic legacy ata controller\n" );
			
	
	/* Default legacy ATA registers */
	nCommand1 = 0x1f0;
	nCommand2 = 0x170;
	nControl1 = 0x3f6;
	nControl2 = 0x376;
	nIrq1 = 14;
	nIrq2 = 15;
			
			
	get_kernel_arguments( &argc, &argv );

	/* Let the user override the base address */
	for( i = 0; i < argc; ++i )
	{
		if( get_num_arg( &nCommand1, "ata_pci_legacy_cmd1=", argv[i], strlen( argv[i] ) ) )
			kerndbg( KERN_WARNING, "User specified command register base for channel 0\n" );
		if( get_num_arg( &nCommand2, "ata_pci_legacy_cmd2=", argv[i], strlen( argv[i] ) ) )
			kerndbg( KERN_WARNING, "User specified command register base for channel 1\n" );
		if( get_num_arg( &nControl1, "ata_pci_legacy_ctrl1=", argv[i], strlen( argv[i] ) ) )
			kerndbg( KERN_WARNING, "User specified control register base for channel 0\n" );
		if( get_num_arg( &nControl2, "ata_pci_legacy_ctrl2=", argv[i], strlen( argv[i] ) ) )
			kerndbg( KERN_WARNING, "User specified control register base for channel 1\n" );
	}
	g_bLegacyController = true;
	
	/* Print out data */
	kerndbg( KERN_INFO, "Channel 0: %s Cmd @ %x Ctrl @ %x IRQ @ %i\n", "PIO",
					(uint)nCommand1, (uint)nControl1, nIrq1 );
	kerndbg( KERN_INFO, "Channel 1: %s Cmd @ %x Ctrl @ %x IRQ @ %i\n", "PIO",
					(uint)nCommand2, (uint)nControl2, nIrq2 );			
	
	
	/* Create locks */
	hLock1 = create_semaphore( "ata_pci_port_lock", 1, 0 );
	hLock2 = create_semaphore( "ata_pci_port_lock", 1, 0 );
	hIRQWait1 = create_semaphore( "ata_pci_irq_wait", 0, 0 );
	hIRQWait2 = create_semaphore( "ata_pci_irq_wait", 0, 0 );
	
	
	/* Allocate controller */
	psCtrl = g_psBus->alloc_controller( nDeviceID, 2, 2 );
	strcpy( psCtrl->zName, "Legacy ATA controller" );
	
	/* Allocate and register port */
	for( i = 0; i < 4; i++ )
	{
		int j;
		ATA_port_s* psPort = g_psBus->alloc_port( psCtrl );
		uint32 nRegBase = ( i & 2 ) ? nCommand2 : nCommand1;
		sem_id hLock = ( i & 2 ) ? hLock2 : hLock1;
		sem_id hIRQWait = ( i & 2 ) ? hIRQWait2 : hIRQWait1;
		
		psPort->hPortLock = hLock;
		psPort->hIRQWait = hIRQWait;
		psPort->sOps.reset = ata_pci_reset_dummy;
		psPort->bMMIO = false;
		psPort->nSupportedPortSpeed = ( 1 << ATA_SPEED_PIO );
		
		psPort->nType = ATA_PATA;
		psPort->nCable = ATA_CABLE_UNKNOWN;
		psPort->pDMATable = alloc_real( PAGE_SIZE / 2 + 4, 0 );
		if( !psPort->pDMATable )
		{
			kerndbg( KERN_FATAL, "Failed to allocate DMA buffer\n" );
			return( -1 );
		}
		psPort->pDMATable = ( uint8* )( ( (uint32)psPort->pDMATable + 4 ) & ~3 );
		
		/* Fill registers */
		for( j = 0; j < 8; j++ )
			psPort->nRegs[j] = nRegBase + j;
		psPort->nRegs[ATA_REG_CONTROL] = ( i & 2 ) ? nControl2 : nControl1;
		psCtrl->psPort[i] = psPort;
		
	}
	
	/* Reset */
	for( i = 0; i < psCtrl->nChannels; i++ )
	{
		ATA_port_s* psMaster = psCtrl->psPort[psCtrl->nPortsPerChannel*i];
		ata_pci_reset( psMaster, psCtrl->psPort[psCtrl->nPortsPerChannel*i+1] );
	}
	
	g_psBus->add_controller( psCtrl );
	
	return( 0 );
}

status_t device_init( int nDeviceID )
{
	PCI_Info_s sDevice;
	
	const char* const *argv;
	int argc;
	int i;
	int j;
	char zArg[32];
	bool bControllerFound = false;
	bool bForceGeneric = false;
	
	/* Parse command line */
	get_kernel_arguments( &argc, &argv );

	for( i = 0; i < argc; ++i )
	{
		if( get_str_arg( zArg, "ata_pci=", argv[i], strlen( argv[i] ) ) )
		{
			if( strcmp( zArg, "disabled" ) == 0 )
			{
				kerndbg( KERN_INFO, "ATA PCI driver disabled by user\n" );
				return( -ENODEV );
			}
		}
		if( get_str_arg( zArg, "ata_pci_force_native=", argv[i], strlen( argv[i] ) ) )
		{
			if( strcmp( zArg, "true" ) == 0 )
			{
				kerndbg( KERN_INFO, "PCI native mode forced\n" );
				g_bLegacyController = true;
			}
		}
		if( get_str_arg( zArg, "ata_pci_force_legacy=", argv[i], strlen( argv[i] ) ) )
		{
			if( strcmp( zArg, "true" ) == 0 )
			{
				kerndbg( KERN_INFO, "Legacy mode forced\n" );
				g_bForceLegacy = true;
			}
		}
		if( get_str_arg( zArg, "ata_pci_enable_speed=", argv[i], strlen( argv[i] ) ) )
		{
			if( strcmp( zArg, "true" ) == 0 )
			{
				kerndbg( KERN_INFO, "All speeds enabled\n" );
				g_bEnableSpeeds = true;
			}
		}
		if( get_str_arg( zArg, "ata_pci_force_generic=", argv[i], strlen( argv[i] ) ) )
		{
			if( strcmp( zArg, "true" ) == 0 )
			{
				kerndbg( KERN_INFO, "Chipset specific code disabled\n" );
				bForceGeneric = true;
			}
		}
	}
	
	/* Get ATA busmanager */
	g_psBus = ( ATA_bus_s* )get_busmanager( ATA_BUS_NAME, ATA_BUS_VERSION );
	if( g_psBus == NULL ) {
		printk( "Error: Could not get ATA busmanager!\n" );
		return( -ENODEV );
	}
	
	/* Get PCI busmanager */
	g_psPCIBus = ( PCI_bus_s* )get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( g_psPCIBus == NULL ) {
		printk( "Error: Could not get PCI busmanager!\n" );
		return( ata_legacy_add_controller( nDeviceID ) );
	}
	
	for( i = 0; g_psPCIBus->get_pci_info( &sDevice, i ) == 0; ++i )
	{
		bool bFound = false;
		for( j = 0; j < ( sizeof( g_sDevices ) / sizeof( ATA_PCI_dev_s ) ); j++ )
		{
			if( !bForceGeneric && sDevice.nVendorID == g_sDevices[j].nVendorID &&
				sDevice.nDeviceID == g_sDevices[j].nDeviceID )
			{
				bFound = true;
				
				/* Leave claiming to the chipset specific code */
				if( ata_pci_add_controller( nDeviceID, sDevice, g_sDevices[j].psInit ) == 0 )
					bControllerFound = true;
			}
		}
		if( bFound == false && sDevice.nClassBase == PCI_MASS_STORAGE && sDevice.nClassSub == PCI_IDE )
		{
			if( claim_device( nDeviceID, sDevice.nHandle, "ATA PCI controller", DEVICE_CONTROLLER ) != 0 )
				continue;
			if( ata_pci_add_controller( nDeviceID, sDevice, NULL ) == 0 )
				bControllerFound = true;
		}
	}
	
	printk( "Detected %i ATA PCI controllers\n", g_nControllers );
	
	/* Register the controllers */
	for( i = 0; i < g_nControllers; i++ )
		g_psBus->add_controller( g_apsControllers[i] );
	
	return( bControllerFound ? 0 : -ENODEV );
}

status_t device_uninit( int nDeviceID )
{
	return( 0 );
}

