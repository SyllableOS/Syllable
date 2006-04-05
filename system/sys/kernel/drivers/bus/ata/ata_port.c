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
	if( ata_port_dma_table( psPort, pBuffer, nLen ) == false )
		return( -EIO );
	
	/* Write registers */
	ATA_WRITE_DMA_REG32( psPort, ATA_REG_DMA_TABLE, (uint32)psPort->pDMATable )
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, ATA_DMA_CONTROL_READ )
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus | ATA_DMA_STATUS_IRQ | ATA_DMA_STATUS_ERROR )
	
	return( 0 );
}

/* Default prepare dma write function */
status_t ata_port_prepare_dma_write( ATA_port_s* psPort, uint8* pBuffer, uint32 nLen )
{
	/* Prepare dma table */
	uint8 nStatus;
	if( ata_port_dma_table( psPort, pBuffer, nLen ) == false )
		return( -EIO );
	
	/* Write registers */
	ATA_WRITE_DMA_REG32( psPort, ATA_REG_DMA_TABLE, (uint32)psPort->pDMATable )
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, 0 )
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus | ATA_DMA_STATUS_IRQ | ATA_DMA_STATUS_ERROR )
	
	return( 0 );
}

/* Default start dma function */
status_t ata_port_start_dma( ATA_port_s* psPort )
{
	/* Start transfer */
	uint8 nControl;
	uint8 nStatus;
	
	//printk( "Starting DMA!\n" );
	
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	reset_semaphore( psPort->hIRQWait, 0 );
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl | ATA_DMA_CONTROL_START )
	
	nStatus = 0;
	lock_semaphore( psPort->hIRQWait, SEM_NOSIG, INFINITE_TIMEOUT );
	
	/* Stop transfer */
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl )
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_CONTROL, nControl & ~ATA_DMA_CONTROL_START )
	ATA_READ_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus )
	
	ATA_WRITE_DMA_REG( psPort, ATA_REG_DMA_STATUS, nStatus | ATA_DMA_STATUS_IRQ | ATA_DMA_STATUS_ERROR )
	
	//printk( "Finished Control %i Status %i\n", nControl, nStatus );
	
	ATA_READ_REG( psPort, ATA_REG_CONTROL, nControl )
	
	return( ( nStatus & ( ATA_DMA_STATUS_ERROR | ATA_DMA_STATUS_IRQ ) ) == ATA_DMA_STATUS_IRQ ? 0 : -1 );
}






























