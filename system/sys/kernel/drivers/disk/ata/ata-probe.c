/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2002 William Rose
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  Code from the Linux ide driver is
 *  Copyright (C) 1994, 1995, 1996  scott snyder  <snyder@fnald0.fnal.gov>
 *  Copyright (C) 1996-1998  Erik Andersen <andersee@debian.org>
 *  Copyright (C) 1998-2000  Jens Axboe <axboe@suse.de>
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
#include "ata-probe.h"
#include "ata-io.h"
#include "ata-drive.h"
#include "atapi-drive.h"
#include "ata-dma.h"

extern int g_nDevices[MAX_DRIVES];
extern ata_controllers_t g_nControllers[MAX_CONTROLLERS];
extern int g_nDevID;

extern DeviceOperations_s g_sAtaOperations;
extern DeviceOperations_s g_sAtapiOperations;

void ata_detect_pci_controllers( void )
{
	/* Scan the PCI bus for controllers.  Fall back to the EIDE */
	/* base addresses if no PCI controllers found */
	int currentdev, api, i;
	PCI_Info_s pcidevice;
	char zArg[32];
	bool bPrimaryEnabled = true, bSecondaryEnabled = true;
	bool bPrimaryDmaEnabled = true, bSecondaryDmaEnabled = true;
	bool bPciFound = false;
	const char* const *argv;
	int argc;
	PCI_bus_s* psBus;
	
	get_kernel_arguments( &argc, &argv );

	/* See if any of controllers are disabled by the user */
	for( i = 0; i < argc; ++i )
	{
		if( get_str_arg( zArg, "ata0=", argv[i], strlen( argv[i] ) ) )
		{
			if( strcmp( zArg, "disabled" ) == 0 )
			{
				kerndbg( KERN_INFO, "ata0 disabled by user\n" );
				bPrimaryEnabled = false;
				continue;
			}
			else if( strcmp( zArg, "nodma" ) == 0 )
			{
				kerndbg( KERN_INFO, "ata0 DMA disabled by user\n" );
				bPrimaryDmaEnabled = false;
				continue;
			}
			else	/* Assume that the argument given is the base address */
			{
				kerndbg( KERN_INFO, "ata0 set to %s\n", zArg );

				/* FIXME: Translate the base address & program the PCI device with it! */
				continue;
			}
		}

		if( get_str_arg( zArg, "ata1=", argv[i], strlen( argv[i] ) ) )
		{
			if( strcmp( zArg, "disabled" ) == 0 )
			{
				kerndbg( KERN_INFO, "ata1 disabled by user\n" );
				bSecondaryEnabled = false;
				continue;
			}
			else if( strcmp( zArg, "nodma" ) == 0 )
			{
				kerndbg( KERN_INFO, "ata1 DMA disabled by user\n" );
				bSecondaryDmaEnabled = false;
				continue;
			}
			else	/* Assume that the argument given is the base address */
			{
				kerndbg( KERN_INFO, "ata1 set to %s\n", zArg );

				/* FIXME: Translate the base address & program the PCI device with it! */
				continue;
			}
		}
	}
	
	/* Get PCI busmanager */
	psBus = ( PCI_bus_s* )get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL ) {
		printk( "Error: Could not get PCI busmanager!\n" );
		goto nopci;
	}

	for(currentdev = 0; psBus->get_pci_info(&pcidevice,currentdev)==0;currentdev++)
	{
		if( pcidevice.nClassBase == PCI_MASS_STORAGE && pcidevice.nClassSub == PCI_IDE )
		{
			
			/* Ensure that the controller is enabled */
			if( ( psBus->read_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, PCI_COMMAND, 2 ) & PCI_COMMAND_IO ) != PCI_COMMAND_IO )
			{
				kerndbg( KERN_INFO, "ATA controller at bus %u, device %u, function %u is disabled by BIOS\n", pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction );
				continue;
			}
			
			/* Claim */
			if( claim_device( g_nDevID, pcidevice.nHandle, "PCI IDE controller", DEVICE_CONTROLLER ) != 0 )
				continue;

			bPciFound = true;

			if( bPrimaryEnabled )
			{
				/* Is the Primary channel in compatability or PCI native mode? */
				if( ( psBus->read_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, 0x09, 1 ) & 0x01 ) != 0x00 )
				{
					/* The Primary channel is not in compatability mode; see if we can change it */
					api = psBus->read_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, 0x09, 1 );
					if( ( api & 0x02 ) != 0x02 )
					{
						kerndbg( KERN_INFO, "ATA controller at bus %u, device %u, function %u has a Primary channel that is native PCI only\n", pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction );

						/* FIXME: Program PCI native controllers */
						g_nControllers[0].io_base = 0x00;
						g_nControllers[0].irq = 0;
					}
					else
					{
						/* Switch the channel to compatability mode */
						psBus->write_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, 0x09, 1, ( api & 0xFE ) );

						g_nControllers[0].io_base = ATA_DEFAULT_PRIMARY_BASE;
						g_nControllers[0].irq = ATA_DEFAULT_PRIMARY_IRQ;
					}
				}
				else
				{
					/* The Primary channel is in compatability mode */
					g_nControllers[0].io_base = ATA_DEFAULT_PRIMARY_BASE;
					g_nControllers[0].irq = ATA_DEFAULT_PRIMARY_IRQ;
				}

				/* Enable Busmastering , save dma port and request irq */
				if( g_nControllers[0].io_base != 0x00 && bPrimaryDmaEnabled )
				{
					psBus->write_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, PCI_COMMAND, 2, psBus->read_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, PCI_COMMAND , 2 ) | PCI_COMMAND_MASTER | PCI_COMMAND_IO );
					g_nControllers[0].dma_base = pcidevice.u.h0.nBase4 & PCI_ADDRESS_IO_MASK;

					/* Disable DMA transfers if the interrupt could not be requested */
					if( request_irq( g_nControllers[0].irq, ata_interrupt, NULL, 0, "ata_controller0_irq", &g_nControllers[0] ) < 0 )
					{
						g_nControllers[0].dma_base = 0x0;
						kerndbg( KERN_INFO, "ATA IRQ could not be requested. DMA transfers will be disabled\n" );
					}
					if( g_nControllers[0].dma_base != 0x0 )
						kerndbg( KERN_INFO, "DMA busmaster at 0x%x\n", g_nControllers[0].dma_base );
				}
				else
					g_nControllers[0].dma_base = 0x0;
			}

			if( bSecondaryEnabled )
			{
				/* Is the Secondary channel in compatability or PCI native mode? */
				if( ( psBus->read_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, 0x09, 1 ) & 0x04 ) != 0x00 )
				{
					/* The Primary channel is not in compatability mode; see if we can change it */
					api = psBus->read_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, 0x09, 1 );
					if( ( api & 0x08 ) != 0x08 )
					{
						kerndbg( KERN_INFO, "ATA controller at bus %u, device %u, function %u has a Secondary channel that is native PCI only\n", pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction );

						/* FIXME: Program PCI native controllers */
						g_nControllers[1].io_base = 0x00;
						g_nControllers[1].irq = 0;
					}
					else
					{
						/* Switch the channel to compatability mode */
						psBus->write_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, 0x09, 1, ( api & 0xFB ) );

						g_nControllers[1].io_base = ATA_DEFAULT_SECONDARY_BASE;
						g_nControllers[1].irq = ATA_DEFAULT_SECONDARY_IRQ;
					}
				}
				else
				{
					/* The Secondary channel is in compatability mode */
					g_nControllers[1].io_base = ATA_DEFAULT_SECONDARY_BASE;
					g_nControllers[1].irq = ATA_DEFAULT_SECONDARY_IRQ;
				}

				/* Enable Busmastering, save dma port and request irq */
				if( g_nControllers[1].io_base != 0x00 && bSecondaryDmaEnabled )
				{
					psBus->write_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, PCI_COMMAND, 2, psBus->read_pci_config( pcidevice.nBus, pcidevice.nDevice, pcidevice.nFunction, PCI_COMMAND , 2 ) | PCI_COMMAND_MASTER | PCI_COMMAND_IO );
					g_nControllers[1].dma_base = ( pcidevice.u.h0.nBase4 & PCI_ADDRESS_IO_MASK ) + 0x8;

					/* Disable DMA transfers if the interrupt could not be requested */
					if( request_irq( g_nControllers[1].irq, ata_interrupt, NULL, 0, "ata_controller1_irq", &g_nControllers[1] ) < 0 )
					{
						g_nControllers[1].dma_base = 0x0;
						kerndbg( KERN_INFO, "ATA IRQ could not be requested. DMA transfers will be disabled\n" );
					}
					if( g_nControllers[1].dma_base != 0x0 )
						kerndbg( KERN_INFO, "DMA busmaster at 0x%x\n", g_nControllers[1].dma_base );
				}
			}
		}
	}
nopci:
	
	if( bPciFound == false )
	{
		kerndbg( KERN_INFO, "No PCI ATA controllers found.  Assuming EISA controller defaults.\n");

		g_nControllers[0].io_base = ATA_DEFAULT_PRIMARY_BASE;
		g_nControllers[0].irq = ATA_DEFAULT_PRIMARY_IRQ;
		g_nControllers[0].dma_base = 0x0;

		g_nControllers[1].io_base = ATA_DEFAULT_SECONDARY_BASE;
		g_nControllers[1].irq = ATA_DEFAULT_SECONDARY_IRQ;
		g_nControllers[1].dma_base = 0x0;
		
		/* Show some nice information */
		claim_device( g_nDevID, register_device( "", "isa" ), "ISA IDE controller", DEVICE_CONTROLLER );
	}

	return;
}

void ata_init_controllers( void )
{
	/* Probe for & reset ATA controller(s) */
	int controller;
	uint8 count, sector, cyl_lo, cyl_hi, status;
	char master_type[6];
	char slave_type[6];
	char info_string[48];

	kerndbg( KERN_DEBUG, "Checking for ATA / ATAPI devices\n");

	/* Reset the controller and find out which drives are connected */
	for( controller = 0; controller < MAX_CONTROLLERS; controller++ )
	{
		if( g_nControllers[controller].io_base == 0x0 )
			continue;

		kerndbg( KERN_DEBUG_LOW, "Checking for ata%i Master\n", controller );

		/* See if a master device is present */
		ata_outb( ATA_CTL, CTL_EIGHTHEADS);
		ata_outb( ATA_LDH, 0xa0 );	/* Select the master drive */
		udelay( DELAY );
		ata_outb( ATA_COUNT, 0x55 );
		ata_outb( ATA_SECTOR, 0xaa );
		ata_outb( ATA_COUNT, 0x55 );
		ata_outb( ATA_SECTOR, 0xaa );
		ata_outb( ATA_COUNT, 0x55 );
		ata_outb( ATA_SECTOR, 0xaa );

		count = ata_inb( ATA_COUNT);
		sector = ata_inb( ATA_SECTOR );

		if( ( count == 0x55 ) && ( sector == 0xaa ) )
		{
			g_nDevices[controller*2] = DEVICE_UNKNOWN;
			kerndbg( KERN_DEBUG, "ata%i Master device found\n", controller );
		}
		else
		{
			g_nDevices[controller*2] = DEVICE_NONE;
			kerndbg( KERN_DEBUG, "ata%i Master device not found\n", controller );
		}

		/* Probe for a slave device */
		kerndbg( KERN_DEBUG_LOW, "Checking for ata%i Slave\n", controller );

		ata_outb( ATA_CTL, CTL_EIGHTHEADS);
		ata_outb( ATA_LDH, 0xb0 );	/* Select the slave drive */
		udelay( DELAY );
		ata_outb( ATA_COUNT, 0x55 );
		ata_outb( ATA_SECTOR, 0xaa );
		ata_outb( ATA_COUNT, 0x55 );
		ata_outb( ATA_SECTOR, 0xaa );
		ata_outb( ATA_COUNT, 0x55 );
		ata_outb( ATA_SECTOR, 0xaa );

		count = ata_inb( ATA_COUNT);
		sector = ata_inb( ATA_SECTOR );

		if( ( count == 0x55 ) && ( sector == 0xaa ) )
		{
			g_nDevices[(controller*2)+1] = DEVICE_UNKNOWN;
			kerndbg( KERN_DEBUG, "ata%i Slave device found\n", controller );
		}
		else
		{
			g_nDevices[(controller*2)+1] = DEVICE_NONE;
			kerndbg( KERN_DEBUG, "ata%i Slave device not found\n", controller );
		}

		/* Soft reset */
		ata_outb( ATA_LDH, 0xa0 );	/* Select the master drive */
		udelay( DELAY );

		ata_drive_reset( controller );

		/* Check for the presence of the master device again, and find out if it is an ATA or an ATAPI device */
		ata_outb( ATA_LDH, 0xa0 );
		udelay( DELAY );

		count = ata_inb( ATA_COUNT );
		sector = ata_inb( ATA_SECTOR );

		if( ( count == 0x01 ) && ( sector == 0x01) )
		{
			/* Device exists.  Find out which type of device it is */
			cyl_lo = ata_inb( ATA_CYL_LO );
			cyl_hi = ata_inb( ATA_CYL_HI );
			status = ata_inb( ATA_STATUS );

			if( ( cyl_lo == 0x14 ) && ( cyl_hi == 0xeb ) )
			{
				/* Found an ATAPI device */
				g_nDevices[controller*2] = DEVICE_ATAPI;
				kerndbg( KERN_DEBUG, "Found an ATAPI device on ata%i Master\n", controller );
			}
			else
			{
				if( ( cyl_lo == 0x00 ) && ( cyl_hi == 0x00 ) && ( status != 0x00 ) )
				{
					/* Found an ATA device */
					g_nDevices[controller*2] = DEVICE_ATA;
					kerndbg( KERN_DEBUG, "Found an ATA device on ata%i Master\n", controller );
				}
				else
				{
					g_nDevices[controller*2] = DEVICE_NONE;
					kerndbg( KERN_DEBUG, "Un-recognised response from ata%i Master\n", controller );
				}
			}
		}

		/* Check & identify the slave */
		ata_outb( ATA_LDH, 0xb0 );
		udelay( DELAY );

		count = ata_inb( ATA_COUNT );
		sector = ata_inb( ATA_SECTOR );

		if( ( count == 0x01 ) && ( sector == 0x01) )
		{
	 		/* Device exists.  Find out which type of device it is */
			cyl_lo = ata_inb( ATA_CYL_LO );
			cyl_hi = ata_inb( ATA_CYL_HI );
			status = ata_inb( ATA_STATUS );

			if( ( cyl_lo == 0x14 ) && ( cyl_hi == 0xeb ) )
			{
				/* Found an ATAPI device */
				g_nDevices[(controller*2)+1] = DEVICE_ATAPI;
				kerndbg( KERN_DEBUG, "Found an ATAPI device on ata%i Slave\n", controller );
			}
			else
			{
				if( ( cyl_lo == 0x00 ) && ( cyl_hi == 0x00 ) && ( status != 0x00 ) )
				{
					/* Found an ATA device */
					g_nDevices[(controller*2)+1] = DEVICE_ATA;
					kerndbg( KERN_DEBUG, "Found an ATA device on ata%i Slave\n", controller );
				}
				else
				{
					g_nDevices[(controller*2)+1] = DEVICE_NONE;
					kerndbg( KERN_DEBUG, "Un-recognised response from ata%i Slave\n", controller );
				}
			}
		}

		/* Select a connected device */
		if( g_nDevices[controller*2] != DEVICE_NONE )
		{
			ata_outb( ATA_LDH, 0xa0 );	/* Select the master */
			udelay( DELAY );
		}
		else if( g_nDevices[(controller*2)+1] != DEVICE_NONE )
		{
			ata_outb( ATA_LDH, 0xb0 );	/* Select the slave */
			udelay( DELAY );
		}
	}

	/* Tell the user what we have found */
	for( controller = 0; controller < MAX_CONTROLLERS; controller++)
	{
		switch( g_nDevices[controller*2] )
		{
			case DEVICE_ATA:
				sprintf( master_type, "%s", "ATA   " );
				break;
			case DEVICE_ATAPI:
				sprintf( master_type, "%s", "ATAPI " );
				break;
			default:
				sprintf( master_type, "%s", "None  " );
				break;
		}

		switch( g_nDevices[(controller*2)+1] )
		{
			case DEVICE_ATA:
				sprintf( slave_type, "%s", "ATA   " );
				break;
			case DEVICE_ATAPI:
				sprintf( slave_type, "%s", "ATAPI " );
				break;
			default:
				sprintf( slave_type, "%s", "None  " );
				break;
		}

		sprintf( info_string, "ata%u at 0x%.4X  Master:  %s  Slave:  %s\n", controller, g_nControllers[controller].io_base, master_type, slave_type );
		kerndbg( KERN_INFO, info_string );
	}
}

int get_drive_params( int nDrive, DriveParams_s* psParams )
{
	/* Get drive size etc. */
	int ret, controller = get_controller( nDrive );
	ata_identify_info_t *info = NULL; 
	char name[40];

	/* Get the ID data direct from the drive */
	if( !wait_for_status( controller, STATUS_BSY, 0 ) )
		goto error;

	select_drive( nDrive );

	if( !wait_for_status( controller, STATUS_BSY, 0 ) )
		goto error;

	if( g_nDevices[nDrive] == DEVICE_ATA )
		ata_outb( ATA_COMMAND, CMD_IDENTIFY );
	else
		ata_outb( ATA_COMMAND, CMD_ATAPI_IDENTIFY );

	LOCK( g_nControllers[controller].buf_lock );
	ret = get_data( controller, 512, g_nControllers[controller].data_buffer );
	if( ret < 0 )
	{
		if( ret == -2 )
		{
			/* The drive failed to send any data.  This may be a "phantom" slave
			device which we probed earlier.  The device should be disabled to stop
			further problems */
			kerndbg( KERN_WARNING, "ata%i %s did not respond to an identify command.  Disabling drive.\n", controller, get_drive(nDrive) ? "Slave" : "Master" );
			g_nDevices[nDrive] = DEVICE_NONE;
		}
		goto error1;
	}
	
	info = (ata_identify_info_t*)g_nControllers[controller].data_buffer;

	psParams->nStructSize = sizeof(DriveParams_s);
	psParams->nFlags      = 0;
	if( info->configuration & 0x80 )
		psParams->nFlags |= DIF_REMOVABLE;

#if __ENABLE_CSH_
	/* Should this drive use CSH? */
	if( info->capabilities & 0x02 )
		psParams->nFlags |= DIF_USE_CSH;
#endif

	if( g_nDevices[nDrive] == DEVICE_ATA )
	{
		/* Extract geometry if we did not already have one for the drive */
		if (!psParams->nCylinders || !psParams->nHeads || !psParams->nSectors )
		{
			psParams->nCylinders  = info->cylinders;
			psParams->nHeads      = info->heads;
			psParams->nSectors    = info->sectors;
		}

		/* Handle logical geometry translation by the drive */
		if ((info->valid & 0x01) && info->current_cylinders && info->current_heads && (info->current_heads <= 16) && info->current_sectors)
		{
			psParams->nCylinders  = info->cylinders;
			psParams->nHeads      = info->heads;
			psParams->nSectors    = info->sectors;
		}

		/* Use physical geometry if what we have still makes no sense */
		if ( psParams->nHeads > 16 && info->heads && info->heads <= 16)
		{
			psParams->nCylinders  = info->cylinders;
			psParams->nHeads      = info->heads;
			psParams->nSectors    = info->sectors;
		}

		if( !psParams->nTotSectors )
			psParams->nTotSectors = info->lba_sectors;

		psParams->nBytesPerSector  = 512;
	}
	else
	{
		/* We don't know the size of an ATAPI device until we try to open it */
		psParams->nCylinders  = 0;
		psParams->nHeads      = 0;
		psParams->nSectors    = 0;
		psParams->nBytesPerSector  = 2048;
	}

	kerndbg( KERN_DEBUG, "%li Cylinders, %li Sectors, %li Heads, %Li Bytes\n", psParams->nCylinders, psParams->nSectors, psParams->nHeads, ( psParams->nTotSectors * psParams->nBytesPerSector ) );

	/* Tell the user something about the device */
	extract_model_id( name, info->model_id );
	kerndbg( KERN_INFO, "ata%i %s : %.40s (%Li MB)\n", controller, get_drive(nDrive) ? "Slave" : "Master ", name, ( psParams->nTotSectors * psParams->nBytesPerSector )/ MiB );

	UNLOCK( g_nControllers[controller].buf_lock );
	
	/* Register device */
	psParams->nDeviceHandle = register_device( "", "ide" );
	claim_device( g_nDevID, psParams->nDeviceHandle, name, DEVICE_DRIVE );

	return( 0 );

error1:
	UNLOCK( g_nControllers[controller].buf_lock );

error:
	kerndbg( KERN_WARNING, "Unable to get info for drive %u\n", nDrive );
	return( -1 );
}

size_t ata_read_partition_data( void* pCookie, off_t nOffset, void* pBuffer, size_t nSize )
{
    return( ata_read( pCookie, NULL, nOffset, pBuffer, nSize ) );
}

int ata_decode_partitions( AtaInode_s* psInode )
{
	int		    nNumPartitions;
	device_geometry sDiskGeom;
	Partition_s     asPartitions[16];
	AtaInode_s*     psPartition;
	AtaInode_s**    ppsTmp;
	int		    nError;
	int		    i;
  	int controller = psInode->bi_nController;

	sDiskGeom.sector_count      = psInode->bi_nSize / 512;
	sDiskGeom.cylinder_count    = psInode->bi_nCylinders;
	sDiskGeom.sectors_per_track = psInode->bi_nSectors;
	sDiskGeom.head_count	= psInode->bi_nHeads;
	sDiskGeom.bytes_per_sector  = 512;
	sDiskGeom.read_only 	= false;
	sDiskGeom.removable 	= psInode->bi_bRemovable;

	printk( "Decode partition table for %s\n", psInode->bi_zName );

	nNumPartitions = decode_disk_partitions( &sDiskGeom, asPartitions, 16, psInode, ata_read_partition_data );

	if ( nNumPartitions < 0 )
	{
		printk( "   Invalid partition table\n" );
		return( nNumPartitions );
	}

	for ( i = 0 ; i < nNumPartitions ; ++i )
	{
		if ( asPartitions[i].p_nType != 0 && asPartitions[i].p_nSize != 0 )
			printk( "   Partition %d : %10Ld -> %10Ld %02x (%Ld)\n", i, asPartitions[i].p_nStart, asPartitions[i].p_nStart + asPartitions[i].p_nSize - 1LL, asPartitions[i].p_nType, asPartitions[i].p_nSize );
	}

	LOCK( g_nControllers[controller].buf_lock );
	nError = 0;

	for ( psPartition = psInode->bi_psFirstPartition ; psPartition != NULL ; psPartition = psPartition->bi_psNext )
	{
		bool bFound = false;
		for ( i = 0 ; i < nNumPartitions ; ++i )
		{
			if ( asPartitions[i].p_nStart == psPartition->bi_nStart && asPartitions[i].p_nSize == psPartition->bi_nSize )
			{
				bFound = true;
				break;
			}
		}

		if ( bFound == false && psPartition->bi_nOpenCount > 0 )
		{
			printk( "ata_decode_partitions() Error: Open partition %s on %s has changed\n", psPartition->bi_zName, psInode->bi_zName );
			nError = -EBUSY;
			goto error;
		}
	}

	/* Remove deleted partitions from /dev/disk/ide/?/? */
	for ( ppsTmp = &psInode->bi_psFirstPartition ; *ppsTmp != NULL ; )
	{
		bool bFound = false;
		psPartition = *ppsTmp;

		for ( i = 0 ; i < nNumPartitions ; ++i )
		{
			if ( asPartitions[i].p_nStart == psPartition->bi_nStart && asPartitions[i].p_nSize == psPartition->bi_nSize )
			{
				asPartitions[i].p_nSize = 0;
				psPartition->bi_nPartitionType = asPartitions[i].p_nType;
				sprintf( psPartition->bi_zName, "%d", i );
				bFound = true;
				break;
			}
		}

		if ( bFound == false )
		{
			*ppsTmp = psPartition->bi_psNext;
			delete_device_node( psPartition->bi_nNodeHandle );
			kfree( psPartition );
		}
		else
		{
			ppsTmp = &(*ppsTmp)->bi_psNext;
		}
	}

	/* Create nodes for any new partitions. */
	for ( i = 0 ; i < nNumPartitions ; ++i )
	{
		char zNodePath[64];

		if ( asPartitions[i].p_nType == 0 || asPartitions[i].p_nSize == 0 )
			continue;

		psPartition = kmalloc( sizeof( AtaInode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

		if ( psPartition == NULL )
		{
			printk( "Error: ata_decode_partitions() no memory for partition inode\n" );
			break;
		}

		sprintf( psPartition->bi_zName, "%d", i );
		psPartition->bi_nDeviceHandle  = psInode->bi_nDeviceHandle;
		psPartition->bi_nDriveNum      = psInode->bi_nDriveNum;
		psPartition->bi_bDMA		= psInode->bi_bDMA;
		psPartition->bi_nSectors       = psInode->bi_nSectors;
		psPartition->bi_nCylinders     = psInode->bi_nCylinders;
		psPartition->bi_nHeads	       = psInode->bi_nHeads;
		psPartition->bi_nSectorSize    = psInode->bi_nSectorSize;
		psPartition->bi_bRemovable     = psInode->bi_bRemovable;
		psPartition->bi_bLockable      = psInode->bi_bLockable;
		psPartition->bi_bHasChangeLine = psInode->bi_bHasChangeLine;

		psPartition->bi_nStart = asPartitions[i].p_nStart;
		psPartition->bi_nSize  = asPartitions[i].p_nSize;

		psPartition->bi_psNext = psInode->bi_psFirstPartition;
		psInode->bi_psFirstPartition = psPartition;
	
		strcpy( zNodePath, "disk/ide/" );
		strcat( zNodePath, psInode->bi_zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->bi_zName );
		strcat( zNodePath, "_new" );

		psPartition->bi_nNodeHandle = create_device_node( g_nDevID, psPartition->bi_nDeviceHandle, zNodePath, &g_sAtaOperations, psPartition );
	}

	/* We now have to rename nodes that might have moved around in the table and
	 * got new names. To avoid name-clashes while renaming we first give all
	 * nodes a unique temporary name before looping over again giving them their
	 * final names
	*/

	for ( psPartition = psInode->bi_psFirstPartition ; psPartition != NULL ; psPartition = psPartition->bi_psNext )
	{
		char zNodePath[64];

		strcpy( zNodePath, "disk/ide/" );
		strcat( zNodePath, psInode->bi_zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->bi_zName );
		strcat( zNodePath, "_tmp" );

		rename_device_node( psPartition->bi_nNodeHandle, zNodePath );
	}

	for ( psPartition = psInode->bi_psFirstPartition ; psPartition != NULL ; psPartition = psPartition->bi_psNext )
	{
		char zNodePath[64];

		strcpy( zNodePath, "disk/ide/" );
		strcat( zNodePath, psInode->bi_zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->bi_zName );

		rename_device_node( psPartition->bi_nNodeHandle, zNodePath );
	}
    
error:
    UNLOCK( g_nControllers[controller].buf_lock );
    return( nError );
}

int ata_create_node( int nDevID, int nDeviceHandle, const char* pzPath, const char* pzHDName, int nDriveNum, int nSec, int nCyl, int nHead, int nSecSize, off_t nStart, off_t nSize, bool bCSH )
{
	AtaInode_s* psInode = kmalloc( sizeof( AtaInode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	int		nError;

	if ( psInode == NULL )
		return( -ENOMEM );

	strcpy( psInode->bi_zName, pzHDName );
	psInode->bi_nDeviceHandle = nDeviceHandle;
	psInode->bi_nDriveNum   = nDriveNum;
	psInode->bi_nSectors    = nSec;
	psInode->bi_nCylinders  = nCyl;
	psInode->bi_nHeads	    = nHead;
	psInode->bi_nSectorSize = nSecSize;
	psInode->bi_nStart	    = nStart;
	psInode->bi_nSize	    = nSize;
#if __ENABLE_CSH_
	psInode->bi_bCSHAddressing = bCSH;
#endif

	nError = create_device_node( nDevID, nDeviceHandle, pzPath, &g_sAtaOperations, psInode );

	psInode->bi_nNodeHandle = nError;

	if ( nError < 0 )
		kfree( psInode );

	return( nError );
}

int ata_scan_for_disks( int nDeviceID )
{
	AtaInode_s *psAtaInode;
	AtapiInode_s *psAtapiInode;
	int nError, i;
	char zNodePath[128];

	for ( i = 0 ; i < MAX_DRIVES ; ++i )
	{
		DriveParams_s sDriveParams;
		char zName[16];
		char zArgName[32];

		memset( &sDriveParams, 0, sizeof( DriveParams_s ) );

		if( ( g_nDevices[i] == DEVICE_ATA ) || ( g_nDevices[i] == DEVICE_ATAPI ) )
		{
			if( g_nDevices[i] == DEVICE_ATA )
			{
				kerndbg( KERN_DEBUG, "Processing ATA device %u\n", i);
				sprintf( zName, "hd%c", 'a' + i );
			}
			else
			{
				kerndbg( KERN_DEBUG, "Processing ATAPI device %u\n", i);
				sprintf( zName, "cd%c", 'a' + i );
			}

			strcpy( zArgName, "ata_" );
			strcat( zArgName, zName );
			strcat( zArgName, "_mode=" );

			get_drive_params( i, &sDriveParams );

			if( g_nDevices[i] == DEVICE_ATA )
			{
				psAtaInode = kmalloc( sizeof( AtaInode_s ), MEMF_KERNEL | MEMF_CLEAR );

				if ( psAtaInode == NULL )
				{
				    nError = -ENOMEM;
				    goto error;
				}

				strcpy( psAtaInode->bi_zName, zName );
				psAtaInode->bi_nDeviceHandle	= sDriveParams.nDeviceHandle;
				psAtaInode->bi_nDriveNum		= i;
				psAtaInode->bi_nSectors		= sDriveParams.nSectors;
				psAtaInode->bi_nCylinders		= sDriveParams.nCylinders;
				psAtaInode->bi_nHeads			= sDriveParams.nHeads;
				psAtaInode->bi_nSectorSize		= sDriveParams.nBytesPerSector;
				psAtaInode->bi_nStart			= 0;
				psAtaInode->bi_nSize			= sDriveParams.nTotSectors * sDriveParams.nBytesPerSector;
				psAtaInode->bi_nController		= get_controller( i );
				psAtaInode->bi_bRemovable		= (sDriveParams.nFlags & DIF_REMOVABLE) != 0;
				psAtaInode->bi_bRemoved		= false;
				psAtaInode->bi_bLockable		= (sDriveParams.nFlags & DIF_CAN_LOCK) != 0;
				psAtaInode->bi_bLocked			= false;
				psAtaInode->bi_bHasChangeLine	= (sDriveParams.nFlags & DIF_HAS_CHANGE_LINE) != 0;
#if __ENABLE_CSH_
				psAtaInode->bi_bCSHAddressing	= (sDriveParams.nFlags & DIF_USE_CSH) != 0;
#endif

				kerndbg( KERN_INFO, "/dev/disk/ide/%s : (%u) %Ld sectors of %d bytes (%d:%d:%d)\n", zName, psAtaInode->bi_nDriveNum, sDriveParams.nTotSectors, sDriveParams.nBytesPerSector, psAtaInode->bi_nHeads, psAtaInode->bi_nCylinders, psAtaInode->bi_nSectors );

				if( ata_dma_check( i ) == 0 )
				{
					psAtaInode->bi_bDMA = true;
					kerndbg( KERN_INFO, "DMA transfers enabled\n");
				}
				else 
					psAtaInode->bi_bDMA = false;


				strcpy( zNodePath, "disk/ide/" );
				strcat( zNodePath, zName );
				strcat( zNodePath, "/raw" );

				if ( psAtaInode->bi_bRemovable )
				    kerndbg( KERN_INFO, "Drive %s is removable\n", zNodePath );

				if ( psAtaInode->bi_bLockable )
				    kerndbg( KERN_INFO, "Drive %s is lockable\n", zNodePath );

				nError = create_device_node( nDeviceID, psAtaInode->bi_nDeviceHandle, zNodePath, &g_sAtaOperations, psAtaInode );

				psAtaInode->bi_nNodeHandle = nError;

				if ( ( nError >= 0 ) && ( g_nDevices[i] == DEVICE_ATA ) )
					ata_decode_partitions( psAtaInode );
			}
			else
			{
				/* Create an ATAPI device node */
				psAtapiInode = kmalloc( sizeof( AtapiInode_s ), MEMF_KERNEL | MEMF_CLEAR );

				if ( psAtapiInode == NULL )
				{
				    nError = -ENOMEM;
				    goto error;
				}

				strcpy( psAtapiInode->bi_zName, zName );
				psAtapiInode->bi_nDeviceHandle	= sDriveParams.nDeviceHandle;
				psAtapiInode->bi_nDriveNum		= i;
				psAtapiInode->bi_nStart			= 0;
				psAtapiInode->bi_nSize			= 0;
				psAtapiInode->bi_nController	= get_controller( i );
				psAtapiInode->bi_bRemovable		= (sDriveParams.nFlags & DIF_REMOVABLE) != 0;
				psAtapiInode->bi_bLockable		= (sDriveParams.nFlags & DIF_CAN_LOCK) != 0;
				psAtapiInode->bi_bMediaChanged	= false;
				psAtapiInode->bi_bTocValid		= false;

				if( ata_dma_check( i ) == 0 )
				{
					psAtapiInode->bi_bDMA = true;
					kerndbg( KERN_INFO, "DMA transfers enabled\n");
				}
				else 
					psAtapiInode->bi_bDMA = false;
 
				strcpy( zNodePath, "disk/ide/" );
				strcat( zNodePath, zName );
				strcat( zNodePath, "/raw" );

				if ( psAtapiInode->bi_bRemovable )
				    kerndbg( KERN_INFO, "Drive %s is removable\n", zNodePath );

				if ( psAtapiInode->bi_bLockable )
				    kerndbg( KERN_INFO, "Drive %s is lockable\n", zNodePath );

				nError = create_device_node( nDeviceID, psAtapiInode->bi_nDeviceHandle, zNodePath, &g_sAtapiOperations, psAtapiInode );
				psAtapiInode->bi_nNodeHandle = nError;
				atapi_reset( i );
			}
		}
		else	/* Unknown device, not ATA or ATAPI */
		{
			kerndbg( KERN_DEBUG, "Device %u does not exist\n", i);
			continue;
		}
	}

	return( 0 );

error:
	kerndbg( KERN_WARNING, "An error occured in ata_scan_for_disks()\n");
	return( nError );
}

