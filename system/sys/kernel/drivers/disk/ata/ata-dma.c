/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2003 Arno Klenke ( DMA code )
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

/* Check if dma is enabled for one drive */
int ata_dma_check( int drive )
{
	int controller = get_controller( drive );
	if( g_nControllers[controller].dma_base == 0x0 )
		return( -1 );

	if( get_drive( drive ) == 0 )
		return( !( ata_dma_inb( ATA_DMA_STATUS ) & DMA_STATUS_DRIVE0_ENABLED ) );

	return( !( ata_dma_inb( ATA_DMA_STATUS ) & DMA_STATUS_DRIVE1_ENABLED ) );
}

/* Prepare DMA table */
bool ata_dma_table_prepare( int drive, void *buffer, uint32 len )
{
	int controller = get_controller( drive );
	uint32 nLeft = len;
	uint32 nAddress = (uint32)buffer;
	uint32* pnTable = (uint32*)g_nControllers[controller].dma_buffer;
	uint32 nEntryCount = 0;
	uint32 nWriteBytes;
	uint32 nLen;

	/* Check if the number of bytes is too big */
	if( len > 0x10000 * 255 )
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
		nAddress += nLen;
		nLeft -= nLen;
	}

	/* Terminate Table */
	*--pnTable |= 0x80000000;
	return( true );
}

/* Initialize DMA read */
void ata_dma_read( int drive )
{
	int controller = get_controller( drive );

	ata_dma_outl( ATA_DMA_TABLE, (uint32)g_nControllers[controller].dma_buffer );
	ata_dma_outb( ATA_DMA_CONTROL, DMA_CONTROL_READ );
	ata_dma_outb( ATA_DMA_STATUS, ata_inb( ATA_DMA_STATUS ) | DMA_STATUS_ERROR | DMA_STATUS_IRQ );
}

/* Initialize DMA write */
void ata_dma_write( int drive )
{
	int controller = get_controller( drive );

	ata_dma_outl( ATA_DMA_TABLE, (uint32)g_nControllers[controller].dma_buffer );
	ata_dma_outb( ATA_DMA_CONTROL, 0 );
	ata_dma_outb( ATA_DMA_STATUS, ata_inb( ATA_DMA_STATUS ) | DMA_STATUS_ERROR | DMA_STATUS_IRQ );
}

/* Start DMA transfer */
void ata_dma_start( int drive )
{
	int controller = get_controller( drive );

	g_nControllers[controller].dma_active = 1;
	ata_dma_outb( ATA_DMA_CONTROL, ata_dma_inb( ATA_DMA_CONTROL ) | DMA_CONTROL_START );
}

/* Stop DMA transfer */
int ata_dma_stop( int drive )
{
	uint8 nStat;
	int controller = get_controller( drive );

	ata_dma_outb( ATA_DMA_CONTROL, ata_dma_inb( ATA_DMA_CONTROL ) &~DMA_CONTROL_START );
	nStat = ata_dma_inb( ATA_DMA_STATUS );
	ata_dma_outb( ATA_DMA_STATUS, nStat | DMA_STATUS_ERROR | DMA_STATUS_IRQ );
	g_nControllers[controller].dma_active = 0;

	return( ( nStat & ( DMA_STATUS_ERROR | DMA_STATUS_IRQ ) ) == DMA_STATUS_IRQ ? 0 : -1 );
}

