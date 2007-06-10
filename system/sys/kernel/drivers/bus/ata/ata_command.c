/*
 *  ATA/ATAPI driver for Syllable
 *
 *	Copyright (C) 2004 Arno Klenke
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

#include <atheos/time.h>
#include "ata.h"
#include "ata_private.h"
#include "atapi.h"

extern bool g_bEnableDirect;

/* Initialize command buffer */
void ata_cmd_init_buffer( ATA_controller_s* psCtrl, int nChannel )
{
	int i;
	ATA_cmd_buf_s* psBuf = kmalloc( sizeof( ATA_cmd_buf_s ), MEMF_KERNEL | MEMF_CLEAR );
	
	psBuf->nChannel = nChannel;
	psBuf->hCount = create_semaphore( "ata_cmd_buffer_count", 0, 0 );
	psBuf->hLock = create_semaphore( "ata_cmd_buffer_lock", 1, 0 );
	psBuf->psHead = psBuf->psTail = NULL;
	psBuf->hThread = spawn_kernel_thread( "ata_cmd_thread", ata_cmd_thread, DISPLAY_PRIORITY, 4096, psBuf );
	
	for( i = 0; i < psCtrl->nPortsPerChannel; i++ )
	{
		psCtrl->psPort[nChannel*psCtrl->nPortsPerChannel+i]->psCmdBuf = psBuf;
	}

	wakeup_thread( psBuf->hThread, true );
}

/* Allocate command buffer */
void ata_cmd_init( ATA_cmd_s* psCmd, ATA_port_s* psPort )
{
	memset( psCmd, 0, sizeof( ATA_cmd_s ) );
	psCmd->psPort = psPort;
	psCmd->hWait = create_semaphore( "ata_cmd_wait", 0, 0 );
	psCmd->bCanDMA = true;
}

/* Free command buffer */
void ata_cmd_free( ATA_cmd_s* psCmd )
{
	delete_semaphore( psCmd->hWait );
}

/* Queue command in the command buffer */
void ata_cmd_queue( ATA_port_s* psPort, ATA_cmd_s* psCmd )
{
	ATA_cmd_buf_s* psBuf = psPort->psCmdBuf;
	
	if( g_bEnableDirect )
	{
		/* Direct mode */
		if( psCmd->psPort->nDevice == ATA_DEV_ATAPI )
			ata_cmd_atapi( psCmd );
		else
			ata_cmd_ata( psCmd );
		return;
	}
	
	LOCK( psBuf->hLock );
	
	if( psBuf->psTail != NULL )
	{
		psBuf->psTail->psNext = psCmd;
	}
	
	if( psBuf->psHead == NULL )
	{
		psBuf->psHead = psCmd;
	}
	
	psCmd->psNext = NULL;
	psCmd->psPrev = psBuf->psTail;
	psBuf->psTail = psCmd;
	
	UNLOCK( psBuf->hCount );
	UNLOCK( psBuf->hLock );
}

/* Grab the next packet from the command queue */
ATA_cmd_s* ata_cmd_next( ATA_cmd_buf_s* psBuf )
{
	ATA_cmd_s* psCmd;
	LOCK( psBuf->hLock );
	
	psCmd = psBuf->psHead;
	
	if( psCmd != NULL )
	{
		if( psCmd->psNext != NULL )
		{
			psCmd->psNext->psPrev = psCmd->psPrev;
		}
		if( psCmd->psPrev != NULL )
		{
			psCmd->psPrev->psNext = psCmd->psNext;
		}
		if( psBuf->psHead == psCmd )
		{
			psBuf->psHead = psCmd->psNext;
		}
		if( psBuf->psTail == psCmd )
		{
			psBuf->psTail = psCmd->psPrev;
		}
		psCmd->psNext = NULL;
		psCmd->psPrev = NULL;
	}
	
	UNLOCK( psBuf->hLock );
	
	return( psCmd );
}

/* Handle ATAPI commands */
void ata_cmd_atapi( ATA_cmd_s* psCmd )
{
	bool bDMA = false;
	int nRetry = 0;
	int i;
	ATA_port_s* psPort = psCmd->psPort;
	uint16* pBuffer = (uint16*)psCmd->pTransferBuffer;
	uint32 nLen = psCmd->nTransferLength;
	uint8 nControl;
	uint8 nStatus;
	uint16* pCmd = (uint16*)&psCmd->nCmd[0];
	int nDirection = psCmd->nDirection;

	LOCK( psPort->hPortLock );
	
again:
	psCmd->nStatus = -1;
	psCmd->nError = -EIO;

	/* Change DMA flag if necessary */
	if( psPort->nCurrentSpeed >= ATA_SPEED_DMA &&
		psCmd->bCanDMA &&
		nDirection != NO_DATA )
	{
		kerndbg(KERN_DEBUG,"Enabling DMA\n");
		bDMA = true;
	}

	/* Select port */
	psPort->sOps.select( psPort, 0 );
	
	/* Wait */
	if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) != 0 )
	{
		kerndbg( KERN_FATAL, "Error: Device still busy after selecting ATAPI drive\n" );
		goto err;
	}
	
	/* Write control register */
	if( bDMA )
		ATA_WRITE_REG( psPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT )
	else
		ATA_WRITE_REG( psPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT | ATA_CONTROL_INTDISABLE )
		

	/* Write atapi command registers */
	ATA_WRITE_REG( psPort, ATA_REG_FEATURE, bDMA ? ATAPI_FEATURE_DMA : 0 )
	ATA_WRITE_REG( psPort, ATAPI_REG_IRR, 0 )
	ATA_WRITE_REG( psPort, ATAPI_REG_SAMTAG, 0 )
	ATA_WRITE_REG( psPort, ATAPI_REG_COUNT_LOW, psCmd->nTransferLength & 0xff )
	ATA_WRITE_REG( psPort, ATAPI_REG_COUNT_HIGH, ( psCmd->nTransferLength >> 8 ) & 0xff )

	/* Wait */
	if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) != 0 )
	{
		kerndbg( KERN_FATAL, "Error: Device still busy after writing ATAPI command registers\n" );
		goto err;
	}
	
	/* Prepare DMA transfer */
	if( bDMA )
	{
		status_t nReturn = -1;
dma_again:
		//printk( "Starting DMA command...\n" );
		if( READ == nDirection )
		{
			nReturn = psPort->sOps.prepare_dma_read( psPort, (uint8*)pBuffer, nLen );
		}
		else
			nReturn = psPort->sOps.prepare_dma_write( psPort, (uint8*)pBuffer, nLen );
	
		if( nReturn != 0 && nRetry++ >= 3 ) {
			kerndbg( KERN_FATAL, "Failed to prepare the dma table\n" );
			goto pio;
		}
		if( nReturn != 0 )
			goto dma_again;
	}
	
	/* Put command */
	ATA_WRITE_REG( psPort, ATA_REG_COMMAND, ATAPI_CMD_PACKET )
	ATA_READ_REG( psPort, ATA_REG_CONTROL, nControl )
	udelay( ATA_CMD_DELAY );
	
	/* Wait */
	if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) != 0 ) {
		kerndbg( KERN_FATAL, "Error: Device still busy after writing ATAPI command\n" );
		goto err;
	}
		
	ATA_READ_REG( psPort, ATA_REG_STATUS, nStatus )
	
	if( nStatus & ATA_STATUS_ERROR )
	{
		ATA_READ_REG( psPort, ATA_REG_ERROR, psCmd->nError );
		psCmd->sSense.sense_key = psCmd->nError >> 4;
		kerndbg( KERN_FATAL, "Error: Device reported an error after writing ATAPI command\n" );	
		goto err;
	}
	
	if( ata_io_wait( psPort, ATA_STATUS_DRQ, ATA_STATUS_DRQ ) != 0 ){
		kerndbg( KERN_FATAL, "Error: Device did not set DRQ after writing ATAPI command\n" );
		goto err;
	}
	
	if( nStatus & ATA_STATUS_DRQ )
	{
		bigtime_t nTime;
		bool bTimedout;

		/* Transfer command */
		if( psPort->bPIO32bit )
		{
			uint32* pCmd32 = (uint32*)pCmd;
			for( i = 0; i < 3; i++, pCmd32++ )
				ATA_WRITE_REG32( psPort, ATA_REG_DATA, *pCmd32 )		
		} else
			for( i = 0; i < 6; i++, pCmd++ )
				ATA_WRITE_REG16( psPort, ATA_REG_DATA, *pCmd )
				
		kerndbg( KERN_DEBUG_LOW, "ATAPI command 0x%04x written. Length: %i\n", psCmd->nCmd[0], (int)psCmd->nTransferLength );
		ATA_READ_REG( psPort, ATA_REG_CONTROL, nControl )
		udelay( ATA_CMD_DELAY );

		/* Transfer data */
		if( psCmd->nTransferLength > 0 && nDirection != NO_DATA )
		{
			if( bDMA )
			{
				status_t nReturn = -1;
				/* Start transfer */
				nReturn = psPort->sOps.start_dma( psPort );
				
				if( nReturn != 0 )
					goto pio;
					
				kerndbg( KERN_DEBUG_LOW, "DMA transfer finished\n" );
					
				psCmd->nStatus = psCmd->nError = 0;
				goto end;
			}

			/* Wait for BUSY low, DRQ or ERR high.
			 *
			 * This is an infinite loop; if the drive doesn't do what we expect
			 * we will hang forever here.  In previous versions of this driver
			 * this loop could timeout:
			 *
			 * while( get_system_time() - nTime < ATA_CMD_TIMEOUT )
			 *
			 * The only problem with this is that some drives take a very long
			 * time to clear BUSY.  This is especially a problem with E.g. cdrecord
			 * where some commands will not drop BUSY for nearly half a second.
			 * Making this (and a few others) infinite loops ensures that we wait
			 * however long is required and the operation completes as normal,
			 * but we run the slim risk of a drive not doing what we expect and
			 * getting stuck here.  If that happens to anyone we'll have to
			 * re-evalute using a timeout for these loops.
			 */
			while( true )
			{
				ATA_READ_REG( psPort, ATA_REG_STATUS, nStatus )

				/* Wait for BUSY to clear, DRQ or ERR to be set */
				if( !( nStatus & ATA_STATUS_BUSY ) && 
				   ( ( nStatus & ATA_STATUS_DRQ ) || 
				     ( nStatus & ATA_STATUS_ERROR ) ) )
					break;
			}

			kerndbg( KERN_DEBUG_LOW, "ATA_REG_STATUS is 0x%04x\n", nStatus );

			if( nStatus & ATA_STATUS_ERROR )
			{
				uint8 nLow, nHigh;
				int nCurrent;

				kerndbg( KERN_FATAL, "Error: Device reported an error after sending ATAPI packet (status %x)\n", nStatus );

				ATA_READ_REG( psPort, ATA_REG_ERROR, psCmd->nError );
				psCmd->sSense.sense_key = psCmd->nError >> 4;

				/* KV: Let's see if the drive still wants to transfer data.. */
				ATA_READ_REG( psPort, ATAPI_REG_COUNT_LOW, nLow )
				ATA_READ_REG( psPort, ATAPI_REG_COUNT_HIGH, nHigh )
				nCurrent = nLow + (nHigh * 256);

				kerndbg( KERN_DEBUG_LOW, "%i bytes available (Low %i High %i)\n", nCurrent, nLow, nHigh );

				goto err;
			}
			else
			{
				int nBytesLeft = nLen;

				while( nBytesLeft > 0 )
				{
					uint8 nLow, nHigh;
					int nCurrent;

					/* Wait for DRQ */
					if( ata_io_wait_alt( psPort, ATA_STATUS_DRQ, ATA_STATUS_DRQ ) != 0 )
					{
						kerndbg( KERN_FATAL, "Error: Device did not set DRQ after sending ATAPI packet\n" );
						goto err;
					}

					if( READ == nDirection )
					{
						/* Read position */
						ATA_READ_REG( psPort, ATAPI_REG_COUNT_LOW, nLow )
						ATA_READ_REG( psPort, ATAPI_REG_COUNT_HIGH, nHigh )
						nCurrent = nLow + (nHigh * 256);

						kerndbg( KERN_DEBUG_LOW, "%i bytes available (Low %i High %i)\n", nCurrent, nLow, nHigh );

						if( nCurrent > 0 )
						{
							if( nCurrent > nLen )
								nCurrent = nLen;

							/* Only perform a 32bit PIO transfer if the drive supports it *and*
							   the data is an even muliple */
							if( psPort->bPIO32bit && ( nCurrent % 4 == 0 ) )
							{
								uint32* pBuffer32 = (uint32*)pBuffer;
								for( i = 0; i < nCurrent / 4; i++, pBuffer32++ )
									ATA_READ_REG32( psPort, ATA_REG_DATA, *pBuffer32 )		
								pBuffer += nCurrent / 2;

								kerndbg( KERN_DEBUG_LOW, "PIO32 data transfer finished, %i bytes transfered\n", i * 4 );
							}
							else
							{
								for( i = 0; i < nCurrent / 2; i++, pBuffer++ )
									ATA_READ_REG16( psPort, ATA_REG_DATA, *pBuffer )

								kerndbg( KERN_DEBUG_LOW, "PIO16 data transfer finished, %i bytes transfered\n", i * 2 );
							}

							nBytesLeft -= nCurrent;
							if( nBytesLeft > 0 )
							{
								
								
								/* The length we pass to the device is only the *maximum*
								   possible transfer; the device might want to send less.
								   If we're expecting more data then we have to check if
								   DRQ is asserted; if it isn't then the device has finished
								   and we should stop.
								   
								   We first wait until the drive is idle
								*/
								nTime = get_system_time();
								while( true )
								{
									if( get_system_time() - nTime > 20 * 1000 * 1000 )
									{
										kerndbg( KERN_WARNING, "ATAPI drive still busy (left %i status 0x%x transfer %i)", nBytesLeft, (uint)nStatus, (uint)nLen );
										goto err;
									}
									
									ATA_READ_REG( psPort, ATA_REG_STATUS, nStatus )

									/* Wait for BUSY to clear */
									if( !( nStatus & ATA_STATUS_BUSY ) )
										break;
								}
								
								nTime = get_system_time();
								bTimedout = true;
								while( get_system_time() - nTime < ATA_CMD_TIMEOUT )
								{
									ATA_READ_REG( psPort, ATA_REG_STATUS, nStatus )

									if( ( nStatus & ATA_STATUS_DRQ ) )
									{
										bTimedout = false;
										break;
									}

								}
								if( bTimedout )
								{
									kerndbg( KERN_WARNING, "DRQ is low, (left %i status 0x%x transfer %i)\n", nBytesLeft, (uint)nStatus, (uint)nLen );
									break;
								}
							}
						}
						else
						{
							/* This is an exception catch that I ran into during testing, but which
							   I think I have now fixed using the ata_io_wait_alt() at the top of
							   the loop.  I've left this in just in case. */
							kerndbg( KERN_WARNING, "DRQ is high but the drive reports 0 bytes available.\n");

							ATA_READ_REG( psPort, ATA_REG_ERROR, psCmd->nError );
							psCmd->sSense.sense_key = psCmd->nError >> 4;
							goto err;
						}
					}
					else	/* WRITE == nDirection */
					{
						/* Get transfer chunk size */
						ATA_READ_REG( psPort, ATAPI_REG_COUNT_LOW, nLow )
						ATA_READ_REG( psPort, ATAPI_REG_COUNT_HIGH, nHigh )
						nCurrent = nLow + (nHigh * 256);

						kerndbg( KERN_DEBUG_LOW, "Device requested %i bytes (Low %i High %i)\n", nCurrent, nLow, nHigh );

						if( nCurrent > 0 )
						{
							if( nCurrent > nLen )
								nCurrent = nLen;

							/* Only perform a 32bit PIO transfer if the drive supports it *and*
							   the data is an even muliple */
							if( psPort->bPIO32bit && ( nCurrent % 4 == 0 ) )
							{
								uint32* pBuffer32 = (uint32*)pBuffer;
								for( i = 0; i < nCurrent / 4; i++, pBuffer32++ )
									ATA_WRITE_REG32( psPort, ATA_REG_DATA, *pBuffer32 )		
								pBuffer += nCurrent / 2;

								kerndbg( KERN_DEBUG_LOW, "PIO32 data transfer finished, %i bytes transfered\n", i * 4 );
							}
							else
							{
								for( i = 0; i < nCurrent / 2; i++, pBuffer++ )
									ATA_WRITE_REG16( psPort, ATA_REG_DATA, *pBuffer )

								kerndbg( KERN_DEBUG_LOW, "PIO16 data transfer finished, %i bytes transfered\n", i * 2 );
							}

							nBytesLeft -= nCurrent;
						}	/* nCurrent > 0 */
					}	/* nDirection */
				}

				/* Wait for command completion; BUSY & DRQ low, DRDY high */
				bool bTimedout = true;
				nTime = get_system_time();
				while( get_system_time() - nTime < ATA_CMD_TIMEOUT * 20 )
				{
					ATA_READ_REG( psPort, ATA_REG_STATUS, nStatus )
					/* Wait for BUSY & DRQ to clear, DRDY or ERROR to be set */
					if( ( !( nStatus & ATA_STATUS_BUSY ) && !( nStatus & ATA_STATUS_DRQ ) ) && 
					    ( nStatus & ATA_STATUS_DRDY ) )
					{
						bTimedout = false;
						break;
					}
				}
				if( bTimedout )
				{
					uint8 nLow, nHigh;
					ATA_READ_REG( psPort, ATAPI_REG_COUNT_LOW, nLow )
					ATA_READ_REG( psPort, ATAPI_REG_COUNT_HIGH, nHigh )
					kerndbg( KERN_WARNING, "ATAPI command failed to execute (status 0x%x high 0x%x low 0x%x transfer %i)\n", (uint)nStatus, (uint)nHigh, (uint)nLow, (uint)nLen );
					goto err;
				}
			}

			psCmd->nStatus = psCmd->nError = 0;

		} else {
			/* Command without data transfer.
			 * Another infinite loop.  See the comment above */
			while( true )
			{
				ATA_READ_REG( psPort, ATA_REG_ALTSTATUS, nStatus )

				/* Wait for BUSY to clear */
				if( !( nStatus & ATA_STATUS_BUSY ) )
					break;
			}

			ATA_READ_REG( psPort, ATA_REG_ERROR, psCmd->nError );
			psCmd->sSense.sense_key = psCmd->nError >> 4;
			
			psCmd->nStatus = 0;
			
			kerndbg( KERN_DEBUG_LOW, "No data to transfer.  Sense key is 0x%04x\n", psCmd->sSense.sense_key );
		}
	}
	goto end;
pio:
	kerndbg( KERN_WARNING, "DMA transfer failed -> falling back to PIO mode\n" );
	psPort->nCurrentSpeed = ATA_SPEED_PIO;
	bDMA = false;
	nRetry = 0;
	goto again;
err:
	kerndbg( KERN_DEBUG, "Failed to execute packet with command 0x%04x\n", psCmd->nCmd[0] );
	kerndbg( KERN_DEBUG, "Sense key 0x%04x\n", psCmd->sSense.sense_key );
end:
	UNLOCK( psCmd->hWait );
	UNLOCK( psPort->hPortLock );
}

/* Handle ATA command */
void ata_cmd_ata( ATA_cmd_s* psCmd )
{
	bool bWrite = false;
	bool bDMA = false;
	bool bChangeCmd = false;
	int nRetry = 0;
	uint8 nCommand;
	uint8 nControl;
	uint8* pBuffer = psCmd->pTransferBuffer;
	uint32 nLen = psCmd->nTransferLength;
	
	ATA_port_s* psPort = psCmd->psPort;
	
	LOCK( psPort->hPortLock );
	
again:
	
	psCmd->nStatus = -1;
	psCmd->nError = -EIO;
	
	/* Change DMA and WRITE flag if necessary */
	if( psCmd->nCmd[ATA_REG_COMMAND] == ATA_CMD_READ_PIO )
		bChangeCmd = true;
	if( psCmd->nCmd[ATA_REG_COMMAND] == ATA_CMD_WRITE_PIO )
	{
		bChangeCmd = true;
		bWrite = true;
	}
	if( psPort->nCurrentSpeed >= ATA_SPEED_DMA &&
		psCmd->bCanDMA ) {
		bDMA = true;
	}
	
	/* Select port */
	psPort->sOps.select( psPort, psCmd->nCmd[ATA_REG_DEVICE] );

	/* Wait */
	if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) != 0 )
		goto err;
	
	/* Extra wait for ATAPI devices */
	if( psPort->nDevice == ATA_DEV_ATAPI && psCmd->nCmd[ATA_REG_COMMAND] == ATAPI_CMD_IDENTIFY )
	{
		snooze( 150000 );
		if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) != 0 )
		{
			kerndbg( KERN_FATAL, "Error: ATAPI device does not clear busy flag after selecting!\n" );
			goto err;
		}
	}
	
	/* Write control register */
	if( bDMA )
		ATA_WRITE_REG( psPort, ATA_REG_CONTROL, psCmd->nCmd[ATA_REG_CONTROL] )
	else
		ATA_WRITE_REG( psPort, ATA_REG_CONTROL, psCmd->nCmd[ATA_REG_CONTROL] | ATA_CONTROL_INTDISABLE )

	if( psPort->bLBA48bit )
	{
		/* Write 48bit registers */
		ATA_WRITE_REG( psPort, ATA_REG_FEATURE, psCmd->nCmd[9] )
		ATA_WRITE_REG( psPort, ATA_REG_COUNT, psCmd->nCmd[10] )
		ATA_WRITE_REG( psPort, ATA_REG_LBA_LOW, psCmd->nCmd[11] )
		ATA_WRITE_REG( psPort, ATA_REG_LBA_MID, psCmd->nCmd[12] )
		ATA_WRITE_REG( psPort, ATA_REG_LBA_HIGH, psCmd->nCmd[13] )
	}
	
	/* Write standard registers */
	ATA_WRITE_REG( psPort, ATA_REG_FEATURE, psCmd->nCmd[ATA_REG_FEATURE] )
	ATA_WRITE_REG( psPort, ATA_REG_COUNT, psCmd->nCmd[ATA_REG_COUNT] )
	ATA_WRITE_REG( psPort, ATA_REG_LBA_LOW, psCmd->nCmd[ATA_REG_LBA_LOW] )
	ATA_WRITE_REG( psPort, ATA_REG_LBA_MID, psCmd->nCmd[ATA_REG_LBA_MID] )
	ATA_WRITE_REG( psPort, ATA_REG_LBA_HIGH, psCmd->nCmd[ATA_REG_LBA_HIGH] )
	
	
	
	/* Wait */
	if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) != 0 )
		goto err;
		

	/* Select command */
	if( bChangeCmd )
	{
		if( bDMA ) {
			if( bWrite ) {
				if( psPort->bLBA48bit ) {
					nCommand = ATA_CMD_WRITE_DMA_48;
				} else {
					nCommand = ATA_CMD_WRITE_DMA;
				}
			} else {
				if( psPort->bLBA48bit ) {
					nCommand = ATA_CMD_READ_DMA_48;
				} else {
					nCommand = ATA_CMD_READ_DMA;
				}
			}
		} else {
			if( bWrite ) {
				if( psPort->bLBA48bit ) {
					nCommand = ATA_CMD_WRITE_PIO_48;
				} else {
					nCommand = ATA_CMD_WRITE_PIO;
				}
			} else {
				if( psPort->bLBA48bit ) {
					nCommand = ATA_CMD_READ_PIO_48;
				} else {
					nCommand = ATA_CMD_READ_PIO;
				}
			}
		}
	} else
		nCommand = psCmd->nCmd[ATA_REG_COMMAND];
	
	if( bDMA )
	{
		/* Initialize DMA transfer */
		status_t nReturn = -1;
		
dma_again:
		//printk( "Starting DMA command...\n" );	
		if( bWrite )
			nReturn = psPort->sOps.prepare_dma_write( psPort, pBuffer, nLen );
		else
			nReturn = psPort->sOps.prepare_dma_read( psPort, pBuffer, nLen );
	
		if( nReturn != 0 && nRetry++ >= 3 ) {
			kerndbg( KERN_FATAL, "Failed to prepare the dma table\n" );
			goto pio;
		}
			
		if( nReturn != 0 )
			goto dma_again;
		
		/* Put command */
		ATA_WRITE_REG( psPort, ATA_REG_COMMAND, nCommand )
		
		ATA_READ_REG( psPort, ATA_REG_CONTROL, nControl )
		udelay( ATA_CMD_DELAY );
		
		/* Start transfer */
		nReturn = psPort->sOps.start_dma( psPort );
		
		if( nReturn != 0 && nRetry++ >= 3 ) {
			kerndbg( KERN_FATAL, "Failed to execute the dma command\n" );
			goto pio;
		}
		
		//printk( "Returned %i\n", nReturn );
			
		if( nReturn != 0 )
			goto dma_again;
		
		psCmd->nStatus = psCmd->nError = 0;
		goto end;
		
	} else {
		
		/* Put command */
		ATA_WRITE_REG( psPort, ATA_REG_COMMAND, nCommand )
		ATA_READ_REG( psPort, ATA_REG_CONTROL, nControl )
		udelay( ATA_CMD_DELAY );
		
		if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) != 0 )
		{
			kerndbg( KERN_FATAL, "Error: Device still busy after sending ATA command\n" );
			goto err;
		}
		
	
		while( nLen > 0 && nRetry < 3 )
		{
			int nTransferred = -1;
			if( bWrite )
				nTransferred = ata_io_write( psPort, pBuffer, 512 );
			else
				nTransferred = ata_io_read( psPort, pBuffer, 512 );
				
			if( nTransferred <= 0 )
			{
				nRetry++;
				continue;
			}
			
			pBuffer += nTransferred;
			nLen -= nTransferred;
		}
		
		if( !bWrite )
		{
			uint8 nStatus;
	
			ATA_READ_REG( psPort, ATA_REG_ALTSTATUS, nStatus )
			if( ata_io_wait( psPort, ATA_STATUS_BUSY, 0 ) != 0 )
			{
				printk( "Error: Device still busy after read request\n" );
				goto err;
			}
		}
		
		if( nRetry >= 3 ) {
			kerndbg( KERN_FATAL, "Packet with command %i timed out\n", psCmd->nCmd[ATA_REG_COMMAND] );
			goto err;
		}
		psCmd->nStatus = psCmd->nError = 0;
		goto end;
	}
	
pio:
	kerndbg( KERN_WARNING, "DMA transfer failed -> falling back to PIO mode\n" );
	psPort->nCurrentSpeed = ATA_SPEED_PIO;
	bDMA = false;
	nRetry = 0;
	goto again;
err:
	ATA_READ_REG( psPort, ATA_REG_ERROR, psCmd->nError );
	kerndbg( KERN_DEBUG, "Failed to execute packet with command %i\n", psCmd->nCmd[ATA_REG_COMMAND] );
end:
	UNLOCK( psCmd->hWait );
	UNLOCK( psPort->hPortLock );
}

/* Command thread */
int32 ata_cmd_thread( void* pData )
{
	ATA_cmd_buf_s* psBuf = pData;
	ATA_cmd_s* psCmd;
	
	kerndbg( KERN_DEBUG_LOW, "Command thread for channel %i started\n", psBuf->nChannel );
	
	while( 1 )
	{
		psCmd = ata_cmd_next( psBuf );
		if( psCmd )
		{
			if( psCmd->psPort->nDevice == ATA_DEV_ATAPI )
				psCmd->psPort->sOps.ata_cmd_atapi( psCmd );
			else
				psCmd->psPort->sOps.ata_cmd_ata( psCmd );
		}

		LOCK( psBuf->hCount );
	}
	
	return( 0 );
}











































































