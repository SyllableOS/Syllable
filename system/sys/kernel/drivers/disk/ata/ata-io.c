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
#include "ata-io.h"
#include "ata-dma.h"

extern int g_nDevices[MAX_DRIVES];
extern ata_controllers_t g_nControllers[MAX_CONTROLLERS];

int wait_for_status( int controller, int mask, int value )
{
	uint8 status;
	ktimer_t timer;

	g_nControllers[controller].state = IDLE;

	timer = create_timer();
	start_timer( timer, timeout, (void*)&g_nControllers[controller].state, TIMEOUT * 20, true);

	kerndbg( KERN_DEBUG_LOW, "wait_for_status(), timer running\n");

	do{
		status = ata_inb( ATA_STATUS );	/* REG_ALT_STATUS */
		if( ( status & mask ) == value )
		{
			delete_timer( timer );
			kerndbg( KERN_DEBUG_LOW, "wait_for_status(), status O.K\n");
			return( 1 );
		}
	}while( g_nControllers[controller].state != DEV_TIMEOUT );

	delete_timer( timer );

	kerndbg( KERN_DEBUG_LOW, "wait_for_status(), timed out\n");

	return( 0 );
}

int ata_interrupt( int nIrq, void *pCtrl, SysCallRegs_s* psRegs )
{
	ata_controllers_t *controller = ( ata_controllers_t* )pCtrl;

	/* Check if a dma transfer is active */
	if( controller->dma_active )
		wakeup_sem( controller->irq_lock, true );

	return( 0 );
}

int get_data( int controller, int bytes, void *buffer )
{
	/* Poll for bytes of data from the controller controller */
	/* Assumes the data buffer is locked */
	uint8 status = 0, error = 0;
	int transfer_count = 0;
	uint16 *offset = (uint16*)buffer;
	ktimer_t timer = NULL;

	g_nControllers[controller].state = BUSY;

	timer = create_timer();
	start_timer( timer, timeout, (void*)&g_nControllers[controller].state, TIMEOUT, true);

	kerndbg( KERN_DEBUG, "get_data() timer running\n" );

	while( g_nControllers[controller].state == BUSY )
	{
		status = ata_inb( REG_ALT_STATUS );
		if( !( status & STATUS_DRQ ) && !( status & STATUS_ERR ) )
			continue;
		else
		{
			/* The drive has responded either with data or an error status */
			if( timer != NULL )
			{
				delete_timer( timer );
				timer = NULL;
			}

			if( status & STATUS_ERR )
			{
				error = ata_inb( ATA_ERROR );
				g_nControllers[controller].state = ERROR;
				break;
			}

			if( status & STATUS_DRQ )
			{
				int current_word;

				kerndbg( KERN_DEBUG, "Transferring data for PIO read operation\n");

				for( current_word = 0; current_word < 256; current_word++, offset++ )
					*offset = ata_inw( ATA_DATA );

				transfer_count += 512;	/* We don't include the ECC bytes as part of the transfer */

				/* Check to see if we have transfered all the data */
				if( bytes - transfer_count  == 0)
					break;

				status = ata_inb( ATA_STATUS );
				udelay(DELAY);
				continue;
			}
		}
	}

	if( g_nControllers[controller].state == DEV_TIMEOUT )
	{
		kerndbg( KERN_WARNING, "get_data() Drive timed out waiting for data.\n" );
		return( -2 );
	}
	
	if( g_nControllers[controller].state == ERROR )
	{
		kerndbg( KERN_WARNING, "get_data() Status = 0x%4x Error = 0x%4x\n", status, error );
		return( -1 );
	}

	return( bytes );
}

int put_data( int controller, int bytes, void *buffer )
{
	/* Poll for a data request and transfer bytes data to the controller "controller" */
	/* Assumes the data buffer is locked */
	uint8 status, error = 0;
	int transfer_count = 0;
	uint16 *offset = (uint16*)buffer;

	g_nControllers[controller].state = BUSY;

	while( g_nControllers[controller].state == BUSY )
	{
		status = ata_inb( REG_ALT_STATUS );
		if( !( status & STATUS_DRQ ) && !( status & STATUS_ERR ) )
			continue;
		else
		{
			if( status & STATUS_ERR )
			{
				error = ata_inb( ATA_ERROR );
				g_nControllers[controller].state = ERROR;
				break;
			}

			if( status & STATUS_DRQ )
			{
				int current_word;

				kerndbg( KERN_DEBUG, "Transferring data for PIO write operation\n");

				for( current_word = 0; current_word < 256; current_word++, offset++ )
					ata_outw( ATA_DATA, *offset );

				transfer_count += 512;	/* We don't include the ECC bytes as part of the transfer */

				status = ata_inb( ATA_STATUS );
				udelay(DELAY);

				/* Check to see if we have transfered all the data */
				if( bytes - transfer_count  == 0)
					break;

				continue;
			}
		}
	}

	if( g_nControllers[controller].state == ERROR )
	{
		kerndbg( KERN_WARNING, "put_data() Status = 0x%.4x Error = 0x%.4x\n", status, error );
		return( -1 );
	}

	return( bytes );
}

int read_sectors( AtaInode_s* psInode, void* pBuffer, int64 nSector, int nSectorCount )
{
	int drive = psInode->bi_nDriveNum;
	int controller = get_controller( drive );

	uint8 sector;					/* Sector # */
	uint8 cyl_lo;					/* Least significant byte of the cylinder # */
	uint8 cyl_hi;					/* Most significant byte of the cylinder # */
	uint8 ldh;						/* LBA, disk #, head # */
	uint8 ctl;						/* Control register */

	uint64 transfer_total = nSectorCount * 512;	/* Number of bytes to transfer */

	int  nRetryCount = 0;

	if ( psInode->bi_bTruncateToCyl && psInode->bi_nSectors > 1 )
		if ( nSector / psInode->bi_nSectors != (nSector+nSectorCount-1) / psInode->bi_nSectors )
			nSectorCount = psInode->bi_nSectors - nSector % psInode->bi_nSectors;

	if ( nSectorCount > 128 )
		nSectorCount = 128;

	/* Calculate the sector address */
	ldh = LDH_DEFAULT | (drive << 4);	/* Note that we do not set the head bits here */

#if __ENABLE_CSH_
	if( psInode->bi_bCSHAddressing )
	{
		int64 csh_sector;
		int64 csh_cyl;
		int64 csh_head;
		int64 temp;

		csh_cyl    = nSector / ((int64)psInode->bi_nSectors * psInode->bi_nHeads);
		temp    = nSector % ((int64)psInode->bi_nSectors * psInode->bi_nHeads);
		csh_head   = temp / (int64)psInode->bi_nSectors;
		csh_sector = (temp % (int64)psInode->bi_nSectors) + 1;

		if ( nSectorCount > ((int64)psInode->bi_nSectors) - csh_sector + 1LL  )
			nSectorCount = ((int64)psInode->bi_nSectors) - csh_sector + 1LL;

		if ( (csh_sector + nSectorCount - 1) / (psInode->bi_nSectors * psInode->bi_nHeads) != csh_cyl )
		{
			printk( "read_sectors() : Failed to read CSH sectors at %Ld:%Ld:%Ld (%d)\n", csh_cyl, csh_sector, csh_head, nSectorCount );
			nSectorCount = 1;
		}

		kassertw( nSectorCount > 0 );
		kassertw( csh_sector > 0 && csh_sector < 64 );

		/* Scatter the CSH values into the CSH registers */
		sector = (uint8)csh_sector;
		cyl_lo = csh_cyl & 0xff;
		cyl_hi = ( csh_cyl >> 8 ) & 0xff;
		ldh |= csh_head & 0x0f;
	}
	else
	{
#endif
		ldh |= LDH_LBA;				/* Enable LBA addressing (Otherwise none of this will work!) */

		sector = nSector & 0xff;
		nSector >>= 8;
		cyl_lo = nSector & 0xff;
		nSector >>= 8;
		cyl_hi = nSector & 0xff;
		nSector >>= 8;
		ldh |= nSector & 0x0f;
#if __ENABLE_CSH_
	}
#endif

retry:
	/* Write the command to the ATA registers */
	if( !wait_for_status( controller, STATUS_BSY, 0 ) )
		goto error;

	ata_outb( ATA_LDH, ldh );
	udelay(DELAY);

	if( !wait_for_status( controller, STATUS_BSY, 0 ) )
		goto error;

	ctl = psInode->bi_nHeads > 7 ? CTL_EIGHTHEADS : 0;
	if( !psInode->bi_bDMA )
		ctl |= CTL_INTDISABLE;

	ata_outb( ATA_CTL, ctl );
	ata_outb( ATA_PRECOMP, 0 );
	ata_outb( ATA_COUNT, nSectorCount );
	ata_outb( ATA_SECTOR, sector );
	ata_outb( ATA_CYL_LO, cyl_lo );
	ata_outb( ATA_CYL_HI, cyl_hi );

	if( psInode->bi_bDMA )
	{
		ata_dma_table_prepare( psInode->bi_nDriveNum, pBuffer, transfer_total );
		ata_dma_read( psInode->bi_nDriveNum );
		ata_outb( ATA_COMMAND, CMD_READ_DMA );
		ata_dma_start( psInode->bi_nDriveNum );

		if( sleep_on_sem( g_nControllers[controller].irq_lock, TIMEOUT * 50 ) < 0 )
		{
			kerndbg( KERN_WARNING, "DMA transfer timeout -> falling back to PIO mode!");
			psInode->bi_bDMA = false;
			return( -EIO );
		}

		kerndbg( KERN_DEBUG, "DMA transfer Finished!\n" );

		if( !ata_dma_stop( psInode->bi_nDriveNum ) )
		{
			kerndbg( KERN_DEBUG, "DMA transfer successful!\n");
		}
		else
		{
			/* DMA Transfers always fail when aterm is opened, but why ? */
			if( nRetryCount++ < 3 )
				goto retry;
			else
			{
				kerndbg( KERN_WARNING, "DMA transfer failed -> falling back to PIO mode!\n");
				psInode->bi_bDMA = false;
				return( -EIO );
			}
		}
	}
	else
	{
		ata_outb( ATA_COMMAND, CMD_READ );

		if( get_data( controller, transfer_total, pBuffer ) < 0 )
		{
			if( nRetryCount++ < 3 )
				goto retry;
			else
				goto error;
		}
	}

	return( nSectorCount );

error:
	kerndbg( KERN_WARNING, "read_sectors() : Failed to read sectors for drive %u\n", drive );
	return( -EIO );

}

int write_sectors( AtaInode_s* psInode, const void* pBuffer, off_t nSector, int nSectorCount )
{
	int drive = psInode->bi_nDriveNum;
	int controller = get_controller( drive );

	uint8 sector;					/* Sector # */
	uint8 cyl_lo;					/* Least significant byte of the cylinder # */
	uint8 cyl_hi;					/* Most significant byte of the cylinder # */
	uint8 ldh;						/* LBA, disk #, head # */
	uint8 ctl;						/* Control register */

	uint64 transfer_total = nSectorCount * 512;	/* Number of bytes to transfer */

	int  nRetryCount = 0;

	if ( psInode->bi_bTruncateToCyl && psInode->bi_nSectors > 1 )
		if ( nSector / psInode->bi_nSectors != (nSector+nSectorCount-1) / psInode->bi_nSectors )
			nSectorCount = psInode->bi_nSectors - nSector % psInode->bi_nSectors;

	if ( nSectorCount > 128 )
		nSectorCount = 128;

	/* Calculate the sector address */
	ldh = LDH_DEFAULT | (drive << 4);	/* Note that we do not set the head bits here */

#if __ENABLE_CSH_
	if( psInode->bi_bCSHAddressing )
	{
		int64 csh_sector;
		int64 csh_cyl;
		int64 csh_head;
		int64 temp;

		csh_cyl    = nSector / ((int64)psInode->bi_nSectors * psInode->bi_nHeads);
		temp    = nSector % ((int64)psInode->bi_nSectors * psInode->bi_nHeads);
		csh_head   = temp / (int64)psInode->bi_nSectors;
		csh_sector = (temp % (int64)psInode->bi_nSectors) + 1;

		if ( nSectorCount > ((int64)psInode->bi_nSectors) - csh_sector + 1LL  )
			nSectorCount = ((int64)psInode->bi_nSectors) - csh_sector + 1LL;

		if ( (csh_sector + nSectorCount - 1) / (psInode->bi_nSectors * psInode->bi_nHeads) != csh_cyl )
		{
			printk( "write_sectors() : Failed to write CSH sectors at %Ld:%Ld:%Ld (%d)\n", csh_cyl, csh_sector, csh_head, nSectorCount );
			nSectorCount = 1;
		}

		kassertw( nSectorCount > 0 );
		kassertw( csh_sector > 0 && csh_sector < 64 );

		/* Scatter the CSH values into the CSH registers */
		sector = (uint8)csh_sector;
		cyl_lo = csh_cyl & 0xff;
		cyl_hi = ( csh_cyl >> 8 ) & 0xff;
		ldh |= csh_head & 0x0f;
	}
	else
	{
#endif
		ldh |= LDH_LBA;				/* Enable LBA addressing (Otherwise none of this will work!) */

		sector = nSector & 0xff;
		nSector >>= 8;
		cyl_lo = nSector & 0xff;
		nSector >>= 8;
		cyl_hi = nSector & 0xff;
		nSector >>= 8;
		ldh |= nSector & 0x0f;
#if __ENABLE_CSH_
	}
#endif

retry:
	/* Write the command to the ATA registers */
	if( !wait_for_status( controller, STATUS_BSY, 0 ) )
		goto error;

	ata_outb( ATA_LDH, ldh );
	udelay(DELAY);

	if( !wait_for_status( controller, STATUS_BSY, 0 ) )
		goto error;

	ctl = psInode->bi_nHeads > 7 ? CTL_EIGHTHEADS : 0;
	if( !psInode->bi_bDMA )
		ctl |= CTL_INTDISABLE;

	ata_outb( ATA_CTL, ctl );
	ata_outb( ATA_PRECOMP, 0 );
	ata_outb( ATA_COUNT, nSectorCount );
	ata_outb( ATA_SECTOR, sector );
	ata_outb( ATA_CYL_LO, cyl_lo );
	ata_outb( ATA_CYL_HI, cyl_hi );

	if( psInode->bi_bDMA )
	{
		ata_dma_table_prepare( psInode->bi_nDriveNum, (void*)pBuffer, transfer_total );
		ata_dma_write( psInode->bi_nDriveNum );
		ata_outb( ATA_COMMAND, CMD_WRITE_DMA );
		ata_dma_start( psInode->bi_nDriveNum );

		if( sleep_on_sem( g_nControllers[controller].irq_lock, TIMEOUT * 50 ) < 0 )
		{
			kerndbg( KERN_WARNING, "DMA transfer timeout -> falling back to PIO mode!");
			psInode->bi_bDMA = false;
			return( -EIO );
		}

		kerndbg( KERN_DEBUG, "DMA transfer Finished!\n" );

		if( !ata_dma_stop( psInode->bi_nDriveNum ) )
		{
			kerndbg( KERN_DEBUG, "DMA transfer successful!\n");
		}
		else
		{
			if( nRetryCount++ < 3 )
				goto retry;
			else
			{
				kerndbg( KERN_WARNING, "DMA transfer failed -> falling back to PIO mode!\n");
				psInode->bi_bDMA = false;
				return( -EIO );
			}
				
		}
	}
	else
	{
		ata_outb( ATA_COMMAND, CMD_WRITE );

		/* Send the first sector of data to the drive */
		put_data( controller, 512, (void*)pBuffer );

		transfer_total -= 512;
		pBuffer += 512;

		/* If more sectors are pending, transfer them to the drive */
		if( transfer_total > 0 )
		{
			if( put_data( controller, transfer_total, (void*)pBuffer ) < 0 )
			{
				if( nRetryCount++ < 3 )
					goto retry;
				else
					goto error;
			}
		}
	}
	return( nSectorCount );

error:
	kerndbg( KERN_WARNING, "write_sectors() : Failed to write sector for drive %u\n", drive );
	return( -EIO );
}

void timeout( void* data )
{
	int *state = (int*)data;
	*state = DEV_TIMEOUT;
	return;
}

void select_drive( int nDrive )
{
	int controller = get_controller( nDrive );
	int drive = get_drive( nDrive );

	ata_outb( ATA_LDH,  LDH_DEFAULT | (drive << 4) );
	udelay( DELAY );
}

void ata_drive_reset( int controller )
{
	ktimer_t timer;

	/* Set and then reset the soft reset bit in the Device Control register.  This causes device 0 be selected. */
	ata_outb( ATA_CTL, CTL_EIGHTHEADS | CTL_RESET);
	udelay( DELAY );
	ata_outb( ATA_CTL, CTL_EIGHTHEADS );
	udelay( DELAY );

	if( g_nDevices[controller*2] != DEVICE_NONE )		/* Wait for the Master drive to clear BSY */
	{
		if( !wait_for_status( controller, STATUS_BSY, 0 ) )
		{
			kerndbg( KERN_WARNING, "Reset of ata%i Master timedout\n", controller );
			return;
		}

		kerndbg( KERN_DEBUG_LOW, "Reset ata%i Master O.K\n", controller );
	}

	if( g_nDevices[(controller*2)+1] != DEVICE_NONE )		/* Wait for the slave device to allow register I/O */
	{
		timer = create_timer();
		start_timer( timer, timeout, (void*)&g_nControllers[controller].state, TIMEOUT, true);

		kerndbg( KERN_DEBUG_LOW, "Slave reset timer running\n");

		g_nControllers[controller].state = BUSY;
		while( g_nControllers[controller].state == BUSY )
		{
			udelay( DELAY );

			ata_outb( ATA_LDH, 0xb0 );
			if( ata_inb( ATA_COUNT ) == 0x01 && ata_inb( ATA_SECTOR ) == 0x01 )
				break;
		}

		if( g_nControllers[controller].state == DEV_TIMEOUT )
		{
			kerndbg( KERN_DEBUG_LOW, "Slave reset timer timedout\n");
		}
		else
			kerndbg( KERN_DEBUG_LOW, "Slave responded before reset timer timedout\n");

		delete_timer( timer );

		/* Check that the Master device cleared BSY */
		if( ata_inb( ATA_STATUS ) & STATUS_BSY )
		{
			kerndbg( KERN_WARNING, "Reset of ata%i Slave failed.  Master did not clear BSY\n", controller );
			return;
		}

		kerndbg( KERN_DEBUG_LOW, "Reset ata%i Slave O.K\n", controller );
	}

	return;
}

/* ATAPI command and I/O */

int atapi_reset( int drive )
{
	int controller = get_controller( drive );

	/* Select Master/Slave drive */
	select_drive( drive );

	if (!wait_for_status(controller, STATUS_BSY | STATUS_DRQ, 0))
	{
		kerndbg(KERN_INFO, "ATAPI drive not ready\n");
		return(-EIO);
	}

	ata_outb( ATAPI_FEAT, 0);
	ata_outb( ATAPI_IRR, 0);
	ata_outb( ATAPI_SAMTAG, 0);
	ata_outb( ATAPI_CNT_LO, 0);
	ata_outb( ATAPI_CNT_HI, 0);
	ata_outb( ATA_COMMAND, CMD_ATAPI_RESET );

	if (!wait_for_status(controller, STATUS_BSY, 0))
	{
		kerndbg(KERN_INFO, "Reset failed, status is 0x%.4x\n", ata_inb( ATA_STATUS ));
		return(-EIO);
	}

	return( 0 );
}

int atapi_packet_command( AtapiInode_s *psInode, atapi_packet_s *command )
{
	/* Send an Atapi Packet Command */
	ktimer_t timer;
	int len;
	int total;

	int drive = psInode->bi_nDriveNum;
	int controller = get_controller( drive );

	uint16 *offset = (uint16*)command->packet;

	/* Select Master/Slave drive */
	select_drive( drive );

	/* Wait for the controller to become idle */
	while( true )
	{
		command->status = ata_inb( ATA_STATUS );
		if( ( command->status & STATUS_BSY ) == 0 )
			break;
	}

	timer = create_timer();
	start_timer( timer, timeout, (void*)&g_nControllers[controller].state, ATAPI_CMD_TIMEOUT, true);

	if(command->count > 0xFFFE)
		command->count = 0xFFFE;	/* Max data per interrupt. */

	/* Prepare DMA */
	if( command->packet[0] == GPCMD_READ_10 && psInode->bi_bDMA )
	{
		ata_dma_table_prepare( drive, (void*)g_nControllers[controller].data_buffer, command->count );
		ata_dma_read( drive );
	}

	ata_outb( ATAPI_FEAT, command->packet[0] == GPCMD_READ_10 && psInode->bi_bDMA );
	ata_outb( ATAPI_IRR, 0);
	ata_outb( ATAPI_SAMTAG, 0);
	ata_outb( ATAPI_CNT_LO, (command->count >> 0) & 0xFF);
	ata_outb( ATAPI_CNT_HI, (command->count >> 8) & 0xFF);

	if( command->packet[0] == GPCMD_READ_10 && psInode->bi_bDMA )
		ata_outb( ATA_CTL, CTL_EIGHTHEADS );
	else
		ata_outb( ATA_CTL, CTL_EIGHTHEADS | CTL_INTDISABLE );

	ata_outb( ATA_COMMAND, CMD_ATAPI_PACKET);

	/* This function assumes that the higher calling function has already locked the buffer */
	g_nControllers[controller].state = BUSY;

	while( g_nControllers[controller].state == BUSY )
	{
		command->status = ata_inb( REG_ALT_STATUS );
		if( !( command->status & STATUS_DRQ ) && !( command->status & STATUS_ERR ) )
			continue;
		else
			break;
	}

	delete_timer( timer );

	if( command->status & STATUS_ERR )
	{
		/* Store the error data for later use */
		command->error = ata_inb( ATA_ERROR );
		command->sense_key = command->error >> 4;

		kerndbg( KERN_DEBUG, "ATAPI packet command got error writing packet, sense key 0x%.2x\n", command->sense_key );

		/* Don't retry. */
		g_nControllers[controller].state = ERROR;
	}

	if( command->status & STATUS_DRQ )
	{
		int current_word;

		kerndbg( KERN_DEBUG, "Writing ATAPI packet\n");

		for( current_word = 0; current_word < 6; current_word++, offset++ )
			ata_outw( ATA_DATA, *offset );

		/* Read in any data that is expected by the command */
		if( command->count > 0 )
		{
			kerndbg( KERN_DEBUG, "Waiting for drive response...\n");

			/* Start DMA transfer if possible */
			if( command->packet[0] == GPCMD_READ_10 && psInode->bi_bDMA )
			{
				kerndbg( KERN_DEBUG, "Starting DMA transfer...\n" );
				ata_dma_start( drive );

				if( sleep_on_sem( g_nControllers[controller].irq_lock, ATAPI_CMD_TIMEOUT * 50 ) < 0 )
				{
					kerndbg( KERN_WARNING, "DMA transfer timeout -> falling back to PIO mode!");
					psInode->bi_bDMA = false;
					g_nControllers[controller].state = IDLE;
					return( -EIO );
				}

				kerndbg( KERN_DEBUG, "DMA transfer Finished!\n" );
				g_nControllers[controller].state = IDLE;

				if( !ata_dma_stop( drive ) )
				{
					kerndbg( KERN_DEBUG, "DMA transfer successful!\n");
					return( 0 );
				}
				else
				{
					kerndbg( KERN_WARNING, "DMA transfer failed -> falling back to PIO mode!");
					psInode->bi_bDMA = false;
					g_nControllers[controller].state = IDLE;
					return( -EIO );
				}
			}

			/* Create a new timer, as this is a seperate operation */
			timer = create_timer();

			/* Give read packets more time to finish */
			if( command->packet[0] == GPCMD_READ_10 ) 
				start_timer( timer, timeout, (void*)&g_nControllers[controller].state, ATAPI_CMD_TIMEOUT * 50, true);
			else
				start_timer( timer, timeout, (void*)&g_nControllers[controller].state, ATAPI_CMD_TIMEOUT, true);

			offset = (uint16*)g_nControllers[controller].data_buffer;
			total = command->count;

			while( g_nControllers[controller].state == BUSY )
			{
				command->status = ata_inb( REG_ALT_STATUS );
				if( !( command->status & STATUS_DRQ ) && !( command->status & STATUS_ERR ) )
					continue;

				if( command->status & STATUS_DRQ )
				{
					/* FIXME: We can overflow the data buffer here with a sufficently
					large drive buffer.  This should probably be re-written as a "gather"
					style function that can handle buffer fragmentation */
					int current_word;

					kerndbg( KERN_DEBUG, "Reading ATAPI response data\n");

					len = ata_inb( ATAPI_CNT_LO) + 256 * ata_inb( ATAPI_CNT_HI );

					if( len > total )
						len = total;

					for( current_word = 0; current_word < ( len / 2 ); current_word++, offset++ )
						*offset = ata_inw( ATA_DATA );

					total -= len;
					if( total <= 0 )
						break;
				}

				if( command->status & STATUS_ERR )
				{
					/* Store the error data for later use */
					command->error = ata_inb( ATA_ERROR );
					command->sense_key = command->error >> 4;

					kerndbg( KERN_DEBUG, "ATAPI packet command got error reading response, sense key 0x%.2x\n", command->sense_key );

					/* Don't retry. */
					g_nControllers[controller].state = ERROR;
					break;
				}
			}

			delete_timer(timer);
		}
		else
		{
			/* Wait to see if drive returns CHECK CONDITION (By setting the STATUS_ERR bit in ATA_ERROR) */
			kerndbg( KERN_DEBUG, "Waiting for drive response...\n");

			/* Create a new timer, as this is a seperate operation */
			timer = create_timer();
			start_timer( timer, timeout, (void*)&g_nControllers[controller].state, ATAPI_CMD_TIMEOUT, true);

			while( g_nControllers[controller].state == BUSY )
			{
				command->status = ata_inb( REG_ALT_STATUS );
				if( command->status & STATUS_BSY )
					continue;

				command->error = ata_inb( ATA_ERROR );
				command->sense_key = command->error >> 4;
			}

			delete_timer( timer );

			/* Reset the controller status.  This is a little bit of a crock, but
			   for this instance we don't care if the operation has timed out; all
			   that means is that no error has occured.  We really need a better
			   way of dealing with this however, as "No Error" is currently treated
			   as the exceptional case, and that slows everything down... */

			g_nControllers[controller].state = IDLE;

			kerndbg( KERN_DEBUG, "ATAPI drive responded 0x%.2x\n", command->sense_key );
		}
	}

	if( g_nControllers[controller].state == DEV_TIMEOUT )
	{
		kerndbg( KERN_DEBUG, "Operation timedout\n");
		return( -EIO );
	}

	if( g_nControllers[controller].state == ERROR )
	{
		kerndbg( KERN_DEBUG, "Error occured\n");
		return( -EIO );
	}

	return(0);
}


