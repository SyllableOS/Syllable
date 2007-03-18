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

#include "ata.h"
#include "ata_private.h"

extern bool g_bEnableProbeDebug;

/* Identify one device */
void ata_port_identify( ATA_port_s* psPort )
{
	uint8 nError, nStatus, nLbaHigh, nLbaMid;
	bool bMasterOnly = false;
	int i;
	ATA_controller_s* psCtrl = psPort->psCtrl;
	
	if( psPort->nPort > 0 )
		return;

	for( i = 0; i < psPort->psCtrl->nPortsPerChannel; i++ )
	{
		ATA_port_s* psIPort = psCtrl->psPort[psPort->nChannel*psPort->psCtrl->nPortsPerChannel + i];
		
		if( i > 0 && bMasterOnly )
		{
			psIPort->nDevice = ATA_DEV_NONE;
			psIPort->nCable = ATA_CABLE_NONE;
			continue;
		}
		
		/* Select port. The first port was already selected by the calling function */
		if( i > 0 )
			psIPort->sOps.select( psIPort, 0 );
		
		/* Read registers */
		ATA_READ_REG( psIPort, ATA_REG_LBA_HIGH, nLbaHigh )
		ATA_READ_REG( psIPort, ATA_REG_LBA_MID, nLbaMid )
		ATA_READ_REG( psIPort, ATA_REG_STATUS, nStatus )
		ATA_READ_REG( psIPort, ATA_REG_ERROR, nError )
		
		kerndbg( g_bEnableProbeDebug ? KERN_INFO : KERN_DEBUG, "ATA: High 0%x Low 0%x Status 0%x Error 0%x\n", (uint)nLbaHigh, (uint)nLbaMid, (uint)nStatus, (uint)nError );
	
		if( nError == 0x81 && i == 0 )
			bMasterOnly = true;
	
		if( nError == 1 || ( i == 0 && nError == 0x81 ) )
		{
			if( ( ( nLbaMid == 0x00 && nLbaHigh == 0x00 ) ||
			( nLbaMid == 0xc3 && nLbaHigh == 0xc3 ) ) && nStatus != 0 )
			{
				psIPort->nDevice = ATA_DEV_ATA;
			} else 
			{
				if( ( nLbaMid == 0x14 && nLbaHigh == 0xeb ) ||
				( nLbaMid == 0x69 && nLbaHigh == 0x96 ) )
				{
					psIPort->nDevice = ATA_DEV_ATAPI;
				} else {
					psIPort->nDevice = ATA_DEV_NONE;
					psIPort->nCable = ATA_CABLE_NONE;
				}
			}
		}
		else
		{
			psIPort->nDevice = ATA_DEV_NONE;
			psIPort->nCable = ATA_CABLE_NONE;
		}
	}
}

/* Default configure function */
status_t ata_port_configure( ATA_port_s* psPort )
{
	return( 0 );
}


/* Default select function */
void ata_port_select( ATA_port_s* psPort, uint8 nAdd )
{
	uint8 nControl;
	if( psPort->nPort == 0 )
		ATA_WRITE_REG( psPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT | nAdd )
	else
		ATA_WRITE_REG( psPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT | ATA_DEVICE_SLAVE | nAdd )
	
	ATA_READ_REG( psPort, ATA_REG_CONTROL, nControl )
	udelay( ATA_CMD_DELAY );
}

/* Default read status */
uint8 ata_port_status( ATA_port_s* psPort )
{
	uint8 nStatus;
	ATA_READ_REG( psPort, ATA_REG_STATUS, nStatus );
	return( nStatus );
}

/* Default read altstatus */
uint8 ata_port_altstatus( ATA_port_s* psPort )
{
	uint8 nStatus;
	ATA_READ_REG( psPort, ATA_REG_ALTSTATUS, nStatus );
	return( nStatus );
}

/* Prepare DMA table ( Helper function for ata_port_prepare_dma_read/write ) */
bool ata_port_dma_table( ATA_port_s* psPort, void *pBuffer, uint32 nTotal )
{
	uint32 nLeft = nTotal;
	uint32 nAddress = (uint32)pBuffer;
	uint32* pnTable = (uint32*)psPort->pDMATable;
	uint32 nEntryCount = 0;
	uint32 nWriteBytes;
	uint32 nLen;

	/* Check if the number of bytes is too big */
	if( nTotal > 0xfffff * 255 )
		return( false );

	/* Fill table with entries ( all entries cannot cross a 64k boundary ) */
	while( nLeft )
	{
		if( nEntryCount++ >= 256 )
		{
			kerndbg( KERN_WARNING, "IDE DMA Table too small!\n" );
			return( false );
		}

		nWriteBytes = 0x10000 - ( nAddress & 0xffff );
		if( nWriteBytes > nLeft )
			nWriteBytes = nLeft;

		nLen = nWriteBytes & 0xffff;
		*pnTable++ = nAddress;
		

		if( nLen == 0x0000 )
		{
			if( nEntryCount++ >= 256 )
			{
				kerndbg( KERN_WARNING, "IDE DMA Table too small!\n" );
				return( false );
			}

			*pnTable++ = 0x8000;
			*pnTable++ = nAddress + 0x8000;
			nAddress += 0x8000;
			nLen = 0x8000;
		}

		*pnTable++ = nLen;
		
		//printk( "%x %x\n", (uint)nAddress, (uint)nLen );
		
		nAddress += nLen;
		nLeft -= nLen;
	}

	/* Terminate Table */
	*--pnTable |= 0x80000000;
	return( true );
}

/* Default prepare dma read function */
status_t ata_port_prepare_dma_read( ATA_port_s* psPort, uint8* pBuffer, uint32 nLen )
{
	/* Prepare dma table */
	uint8 nStatus;
	uint8 nControl;
	if( ata_port_dma_table( psPort, pBuffer, nLen ) == false )
		return( -EIO );
	
	/* Write registers */
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	ATA_WRITE_DMA_REG32( psPort, ATA_REG_DMA_TABLE, (uint32)psPort->pDMATable )	
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	nControl &= ~( ATA_DMA_CONTROL_START );
	nControl |= ATA_DMA_CONTROL_READ;
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	
	return( 0 );
}

/* Default prepare dma write function */
status_t ata_port_prepare_dma_write( ATA_port_s* psPort, uint8* pBuffer, uint32 nLen )
{
	/* Prepare dma table */
	uint8 nStatus;
	uint8 nControl;
	if( ata_port_dma_table( psPort, pBuffer, nLen ) == false )
		return( -EIO );
	
	/* Write registers */
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	ATA_WRITE_DMA_REG32( psPort, ATA_REG_DMA_TABLE, (uint32)psPort->pDMATable )		
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	nControl &= ~( ATA_DMA_CONTROL_START | ATA_DMA_CONTROL_READ );
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	
	return( 0 );
}

/* Default start dma function */
status_t ata_port_start_dma( ATA_port_s* psPort )
{
	/* Start transfer */
	uint8 nStatus;
	uint8 nControl;
	uint8 nDevStatus;
	
#if 0	
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	ATA_READ_REG( psPort, ATA_REG_CONTROL, nDevControl )
#endif	
//	printk( "Starting DMA DMA Control %x DMA status %x Dev %x!\n", (uint)nControl, (uint)nStatus, (uint)nDevControl );
	
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	psPort->bIRQError = false;
	reset_semaphore( psPort->hIRQWait, 0 );
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl | ATA_DMA_CONTROL_START )
	
	nStatus = 0;
	if( lock_semaphore( psPort->hIRQWait, SEM_NOSIG, 5 * 1000 * 1000 ) == -ETIME )
	{
		ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
		ATA_READ_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
		ATA_READ_REG( psPort, ATA_REG_STATUS, nDevStatus )
		/* Stop transfer */
		ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl & ~ATA_DMA_CONTROL_START )
		kerndbg( KERN_FATAL, "Error: DMA transfer timeout Control 0x%x Status 0x%x Devstatus 0x%x!\n", (uint)nControl, (uint)nStatus, (uint)nDevStatus );
		return( -1 );
	}
	
	/* Check that the transfer was stopped */
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	if( nControl & ATA_DMA_CONTROL_START )
	{
		ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
		ATA_READ_REG( psPort, ATA_REG_STATUS, nDevStatus )
		nControl &= ~ATA_DMA_CONTROL_START;
		ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
		kerndbg( KERN_FATAL, "Error: DMA transfer was still pending! Control 0x%x Status 0x%x Devstatus 0x%x!\n", (uint)nControl, (uint)nStatus, (uint)nDevStatus );
		return( -1 );
	}

#if 0	
	nStatus = 0;
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	ATA_READ_REG( psPort, ATA_REG_CONTROL, nDevControl )
	
	printk( "Finished DMA Control %x DMA Status %x Dev %x\n", (uint)nControl, (uint)nStatus, (uint)nDevControl );	
#endif	
	return( psPort->bIRQError ? -1 : 0 );
}






























