/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2004 Arno Klenke
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2002 William Rose
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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


/* Wait for status */
status_t ata_io_wait( ATA_port_s* psPort, int nMask, int nValue )
{
	bigtime_t nTime = get_system_time();
	uint8 nStatus = 0;
	
	while( get_system_time() - nTime < ATA_CMD_TIMEOUT * 20 )
	{
		nStatus = psPort->sOps.status( psPort );
		if( ( nStatus & nMask ) == nValue )
			return( 0 );
	}
	
	kerndbg( KERN_WARNING,  "ata_io_wait() %x %x timed out (current %x)\n", (uint)nMask, (uint)nValue, nStatus );
	return( -1 );
}

/* Wait for status on alternative status register */
status_t ata_io_wait_alt( ATA_port_s* psPort, int nMask, int nValue )
{
	bigtime_t nTime = get_system_time();
	uint8 nStatus = 0;
	
	while( get_system_time() - nTime < ATA_CMD_TIMEOUT * 20 )
	{
		nStatus = psPort->sOps.altstatus( psPort );
		if( ( nStatus & nMask ) == nValue )
			return( 0 );
	}
	
	kerndbg( KERN_WARNING,  "ata_io_wait_alt() %x %x timed out (current %x)\n", (uint)nMask, (uint)nValue, nStatus );
	return( -1 );
}

/* Read data from the data registers */
int ata_io_read( ATA_port_s* psPort, void *pBuffer, int nBytes )
{
	bigtime_t nTime = get_system_time();
	uint8 nStatus = 0;
	uint8 nError = 0;
	uint16 *pData = pBuffer;
	int nTransfered = 0;

	while( get_system_time() - nTime < ATA_CMD_TIMEOUT )
	{
		ATA_READ_REG( psPort, ATA_REG_ALTSTATUS, nStatus )
		if( !( nStatus & ATA_STATUS_DRQ ) && !( nStatus & ATA_STATUS_ERROR ) )
			continue;
		else
		{
			
			if( nStatus & ATA_STATUS_ERROR )
			{
				ATA_READ_REG( psPort, ATA_REG_ERROR, nError )
				break;
			}

			if( nStatus & ATA_STATUS_DRQ )
			{
				int i;
				

				kerndbg( KERN_DEBUG_LOW, "Transferring data for PIO read operation\n" );

				if( psPort->bPIO32bit )
				{
					uint32* pData32 = (uint32*)pData;
					for( i = 0; i < 128; i++, pData32++ )
						ATA_READ_REG32( psPort, ATA_REG_DATA, *pData32 )
					pData += 256;
				} else
					for( i = 0; i < 256; i++, pData++ )
						ATA_READ_REG16( psPort, ATA_REG_DATA, *pData )

				nTransfered += 512;	/* We don't include the ECC bytes as part of the transfer */
				
				/* Check to see if we have transfered all the data */
				if( nBytes - nTransfered  == 0)
					return( nBytes );

				udelay( ATA_CMD_DELAY );
				continue;
			}
		}
	}

	
	kerndbg( KERN_WARNING, "ata_pio_read() Drive timed out waiting for data.\n" );
	kerndbg( KERN_WARNING, "ata_pio_read() Status = 0x%4x Error = 0x%4x\n", nStatus, nError );
	
	
	return( nTransfered );
}

/* Write data to the data registers */
int ata_io_write( ATA_port_s* psPort, void *pBuffer, int nBytes )
{
	bigtime_t nTime = get_system_time();
	uint8 nStatus = 0;
	uint8 nError = 0;
	uint16 *pData = pBuffer;
	int nTransfered = 0;

	while( get_system_time() - nTime < ATA_CMD_TIMEOUT )
	{
		ATA_READ_REG( psPort, ATA_REG_ALTSTATUS, nStatus )
		if( !( nStatus & ATA_STATUS_DRQ ) && !( nStatus & ATA_STATUS_ERROR ) )
			continue;
		else
		{
			
			if( nStatus & ATA_STATUS_ERROR )
			{
				ATA_READ_REG( psPort, ATA_REG_ERROR, nError )
				break;
			}

			if( nStatus & ATA_STATUS_DRQ )
			{
				int i;

				kerndbg( KERN_DEBUG_LOW, "Transferring data for PIO read operation\n" );

				if( psPort->bPIO32bit )
				{
					uint32* pData32 = (uint32*)pData;
					for( i = 0; i < 128; i++, pData32++ )
						ATA_WRITE_REG32( psPort, ATA_REG_DATA, *pData32 )
					pData += 256;
				} else
					for( i = 0; i < 256; i++, pData++ )
						ATA_WRITE_REG16( psPort, ATA_REG_DATA, *pData )

				nTransfered += 512;

				/* Check to see if we have transfered all the data */
				if( nBytes - nTransfered  == 0)
					return( nBytes );

				udelay( ATA_CMD_DELAY );
				continue;
			}
		}
	}

	
	kerndbg( KERN_WARNING, "ata_pio_write() Drive timed out waiting for data.\n" );
	kerndbg( KERN_WARNING, "ata_pio_write() Status = 0x%4x Error = 0x%4x\n", nStatus, nError );
		
	return( nTransfered );
}

















