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

typedef struct
{
	uint8 p_nStatus;
	uint8 p_nFirstHead;
	uint16 p_nFirstCyl;
	uint8 p_nType;
	uint8 p_nLastHead;
	uint16 p_nLastCyl;
	uint32 p_nStartLBA;
	uint32 p_nSize;
} PartitionRecord_s;


int ata_decode_disk_partitions( device_geometry *psDiskGeom, Partition_s * pasPartitions, int nMaxPartitions, void *pCookie, disk_read_op *pfReadCallback )
{
	char anBuffer[512];
	PartitionRecord_s *pasTable = ( PartitionRecord_s * ) ( anBuffer + 0x1be );
	int i;
	int nCount = 0;
	int nRealCount = 0;
	off_t nDiskSize = (off_t)psDiskGeom->sector_count * (off_t)psDiskGeom->bytes_per_sector;
	off_t nTablePos = 0;
	off_t nExtStart = 0;
	off_t nFirstExtended = 0;
	int nNumExtended;
	int nNumActive;
	int nError;
	
	//printk( "%Li sectors %i bytes per sectors\n", psDiskGeom->sector_count, (int)psDiskGeom->bytes_per_sector );
	
	//printk( "%Li disk size\n", nDiskSize );

      repeat:
	if ( pfReadCallback( pCookie, nTablePos, anBuffer, 512 ) != 512 )
	{
		printk( "Error: decode_disk_partitions() failed to read MBR\n" );
		nError = 0;	//-EINVAL;
		goto error;
	}
	if ( *( ( uint16 * )( anBuffer + 0x1fe ) ) != 0xaa55 )
	{
		printk( "Error: decode_disk_partitions() Invalid partition table signature %04x\n", *( ( uint16 * )( anBuffer + 0x1fe ) ) );
		nError = 0;	//-EINVAL;
		goto error;
	}
	
	//printk( "%x signature\n", *( ( uint16 * )( anBuffer + 0x1fe ) ) );

	nNumActive = 0;
	nNumExtended = 0;

	for ( i = 0; i < 4; ++i )
	{
		
		if ( pasTable[i].p_nStatus & 0x80 )
		{
			nNumActive++;
		}
		if ( pasTable[i].p_nType == 0x05 || pasTable[i].p_nType == 0x0f || pasTable[i].p_nType == 0x85 )
		{
			nNumExtended++;
		}
		if ( nNumActive > 1 )
		{
			printk( "Warning: decode_disk_partitions() more than one active partitions\n" );
		}
		if ( nNumExtended > 1 )
		{
			printk( "Error: decode_disk_partitions() more than one extended partitions\n" );
			nError = -EINVAL;
			goto error;
		}
	}
	for ( i = 0; i < 4; ++i )
	{
		//printk( "Partition %i Status %i Type %i Start %x Size %x\n", i, pasTable[i].p_nStatus,
			//	pasTable[i].p_nType, (uint)pasTable[i].p_nStartLBA, (uint)pasTable[i].p_nSize );
		int j;

		if ( nCount >= nMaxPartitions )
		{
			break;
		}
		if ( pasTable[i].p_nType == 0 )
		{
			continue;
		}
		if ( pasTable[i].p_nType == 0x05 || pasTable[i].p_nType == 0x0f || pasTable[i].p_nType == 0x85 )
		{
			nExtStart = ( ( uint64 )pasTable[i].p_nStartLBA ) * ( ( uint64 )psDiskGeom->bytes_per_sector );	// + nTablePos;
			if ( nFirstExtended == 0 )
			{
				memset( &pasPartitions[nCount], 0, sizeof( pasPartitions[nCount] ) );
				nCount++;
			}
			continue;
		}

		pasPartitions[nCount].p_nType = pasTable[i].p_nType;
		pasPartitions[nCount].p_nStatus = pasTable[i].p_nStatus;
		pasPartitions[nCount].p_nStart = ( ( off_t )pasTable[i].p_nStartLBA ) * ( ( off_t )psDiskGeom->bytes_per_sector ) + nTablePos;
		pasPartitions[nCount].p_nSize = ( ( off_t )pasTable[i].p_nSize ) * ( ( off_t )psDiskGeom->bytes_per_sector );

		
		//printk( "Calculated start %Li size %Li\n", (pasPartitions[nCount].p_nStart), (pasPartitions[nCount].p_nSize) );

		if ( pasPartitions[nCount].p_nStart + pasPartitions[nCount].p_nSize > nDiskSize )
		{
			printk( "Error: Partition %d extend outside the disk/extended partition\n", nCount );
			nError = -EINVAL;
			goto error;
		}

		for ( j = 0; j < nCount; ++j )
		{
			if ( pasPartitions[nCount].p_nType == 0 )
			{
				continue;
			}
			if ( pasPartitions[j].p_nStart + pasPartitions[j].p_nSize > pasPartitions[nCount].p_nStart && pasPartitions[j].p_nStart < pasPartitions[nCount].p_nStart + pasPartitions[nCount].p_nSize )
			{
				printk( "Error: decode_disk_partitions() partition %d overlap partition %d\n", j, nCount );
				nError = -EINVAL;
				goto error;
			}
			if ( ( pasPartitions[nCount].p_nStatus & 0x80 ) != 0 && ( pasPartitions[j].p_nStatus & 0x80 ) != 0 )
			{
				printk( "Error: decode_disk_partitions() more than one active partitions\n" );
				nError = -EINVAL;
				goto error;
			}
			if ( pasPartitions[nCount].p_nType == 0x05 && pasPartitions[j].p_nType == 0x05 )
			{
				printk( "Error: decode_disk_partitions() more than one extended partitions\n" );
				nError = -EINVAL;
				goto error;
			}
		}
		nCount++;
		nRealCount++;
	}
	if ( nExtStart != 0 )
	{
		nTablePos = nFirstExtended + nExtStart;
		if ( nFirstExtended == 0 )
		{
			nFirstExtended = nExtStart;
		}
		nExtStart = 0;
		if ( nCount < 4 )
		{
			while ( nCount & 0x03 )
			{
				memset( &pasPartitions[nCount++], 0, sizeof( Partition_s ) );
			}
		}
		goto repeat;
	}
	return ( nCount );
      error:
	if ( nCount < 4 || nRealCount == 0 )
	{
		return ( nError );
	}
	else
	{
		return ( nCount & ~3 );
	}
}


int ata_drive_open( void* pNode, uint32 nFlags, void **ppCookie )
{
	ATA_device_s* psDev = pNode;
	
	atomic_inc( &psDev->nOpenCount );
	
	return( 0 );
}


int ata_drive_close( void* pNode, void* pCookie )
{
	ATA_device_s* psDev = pNode;
	
	atomic_dec( &psDev->nOpenCount );
	
	return( 0 );	
}


int ata_drive_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen )
{
	ATA_device_s* psDev = pNode;
	ATA_cmd_s sCmd;
	int nBytesLeft;
	
	if ( nPos & (512 - 1) )
	{
		kerndbg( KERN_FATAL, "Error: ata_drive_read() position has bad alignment %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nLen & (512 - 1) )
	{
		kerndbg( KERN_FATAL, "Error: ata_drive_read() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psDev->nSize )
	{
		kerndbg( KERN_FATAL, "Warning: ata_drive_read() Request outside partiton %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psDev->nSize )
	{
		kerndbg( KERN_WARNING, "Warning: ata_drive_read() Request truncated from %d to %Ld\n", nLen, (psDev->nSize - nPos) );
		nLen = psDev->nSize - nPos;
	}
	
	LOCK( psDev->hLock );
	
	nBytesLeft = nLen;
	nPos += psDev->nStart;
	
	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( 16 * PAGE_SIZE, nBytesLeft );
		uint64 nBlock = nPos / 512;
		
		/* Prepare read command */
		ata_cmd_init( &sCmd, psDev->psPort );
	
		sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
		sCmd.nTransferLength = nCurSize;
	
		sCmd.nCmd[ATA_REG_CONTROL] = ATA_CONTROL_DEFAULT;
		sCmd.nCmd[ATA_REG_COMMAND] = ATA_CMD_READ_PIO;
		sCmd.nCmd[ATA_REG_DEVICE] = ATA_DEVICE_LBA;
		sCmd.nCmd[ATA_REG_COUNT] = nCurSize / 512;
		sCmd.nCmd[ATA_REG_LBA_LOW] = nBlock & 0xff;
		nBlock >>= 8;
		sCmd.nCmd[ATA_REG_LBA_MID] = nBlock & 0xff;
		nBlock >>= 8;
		sCmd.nCmd[ATA_REG_LBA_HIGH] = nBlock & 0xff;
		nBlock >>= 8;
		if( psDev->psPort->bLBA48bit )
			sCmd.nCmd[11] = nBlock & 0xff;
		else
			sCmd.nCmd[ATA_REG_DEVICE] |= nBlock & 0x0f;
			
		
		/* Queue and wait */
		ata_cmd_queue( psDev->psPort, &sCmd );
		LOCK( sCmd.hWait );
		//ata_cmd_ata( &sCmd );
		ata_cmd_free( &sCmd );
		
		if( sCmd.nStatus < 0 ) {
			kerndbg( KERN_FATAL, "Error: Packet finished with status %i\n", sCmd.nStatus );
			UNLOCK( psDev->hLock );
			return( -EIO );
		}
		
		memcpy( pBuf, psDev->psPort->pDataBuf, nCurSize );
			
		pBuf = ((uint8*)pBuf) + nCurSize;
		nPos += nCurSize;
		nBytesLeft -= nCurSize;
	}
	UNLOCK( psDev->hLock );
	return( nLen );	
}


int ata_drive_write( void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLen )
{
	ATA_device_s* psDev = pNode;
	ATA_cmd_s sCmd;
	int nBytesLeft;
	
	if ( nPos & (512 - 1) )
	{
		kerndbg( KERN_FATAL, "Error: ata_drive_write() position has bad alignment %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nLen & (512 - 1) )
	{
		kerndbg( KERN_FATAL, "Error: ata_drive_write() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psDev->nSize )
	{
		kerndbg( KERN_FATAL, "Warning: ata_drive_write() Request outside partiton %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psDev->nSize )
	{
		kerndbg( KERN_WARNING, "Warning: ata_drive_write() Request truncated from %d to %Ld\n", nLen, (psDev->nSize - nPos) );
		nLen = psDev->nSize - nPos;
	}
	
	LOCK( psDev->hLock );
	
	nBytesLeft = nLen;
	nPos += psDev->nStart;
	
	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( 16 * PAGE_SIZE, nBytesLeft );
		uint64 nBlock = nPos / 512;
		
		/* Prepare read command */
		ata_cmd_init( &sCmd, psDev->psPort );
	
		sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
		sCmd.nTransferLength = nCurSize;
	
		sCmd.nCmd[ATA_REG_COMMAND] = ATA_CMD_WRITE_PIO;
		sCmd.nCmd[ATA_REG_DEVICE] = ATA_DEVICE_LBA;
		sCmd.nCmd[ATA_REG_COUNT] = nCurSize / 512;
		sCmd.nCmd[ATA_REG_LBA_LOW] = nBlock & 0xff;
		nBlock >>= 8;
		sCmd.nCmd[ATA_REG_LBA_MID] = nBlock & 0xff;
		nBlock >>= 8;
		sCmd.nCmd[ATA_REG_LBA_HIGH] = nBlock & 0xff;
		nBlock >>= 8;
		if( psDev->psPort->bLBA48bit )
			sCmd.nCmd[11] = nBlock & 0xff;
		else
			sCmd.nCmd[ATA_REG_DEVICE] |= nBlock & 0x0f;
			
		memcpy( psDev->psPort->pDataBuf, pBuf, nCurSize );
		
		/* Queue and wait */
		ata_cmd_queue( psDev->psPort, &sCmd );
		//printk( "Waiting...\n" );
		LOCK( sCmd.hWait );
		//ata_cmd_ata( &sCmd );
		
		ata_cmd_free( &sCmd );
		
		if( sCmd.nStatus < 0 ) {
			kerndbg( KERN_FATAL, "Error: Packet finished with status %i\n", sCmd.nStatus );
			UNLOCK( psDev->hLock );
			return( -EIO );
		}
		
		pBuf = ((uint8*)pBuf) + nCurSize;
		nPos += nCurSize;
		nBytesLeft -= nCurSize;
	}
	
	UNLOCK( psDev->hLock );
	
	return( nLen );	
}


int ata_drive_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount )
{
	ATA_device_s* psDev = pNode;
	ATA_cmd_s sCmd;
	int nBytesLeft;
	int nLen = 0;
	int i;
	int nCurVec;
	char* pCurVecPtr;
	int   nCurVecLen;
	

	for ( i = 0 ; i < nCount ; ++i )
		nLen += psVector[i].iov_len;


	if ( nLen <= 0 )
		return( 0 );

	if ( nPos & (512 - 1) )
	{
		kerndbg( KERN_FATAL, "Error: ata_drive_readv() position has bad alignment %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nLen & (512 - 1) )
	{
		kerndbg( KERN_FATAL, "Error: ata_drive_readv() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psDev->nSize )
	{
		kerndbg( KERN_FATAL, "Warning: ata_drive_readv() Request outside partiton : %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psDev->nSize )
	{
		kerndbg( KERN_WARNING, "Warning: ata_drive_readv() Request truncated from %d to %Ld\n", nLen, (psDev->nSize - nPos) );
		nLen = psDev->nSize - nPos;
	}
	
	LOCK( psDev->hLock );

	nBytesLeft = nLen;
	nPos += psDev->nStart;

	pCurVecPtr = psVector[0].iov_base;
	nCurVecLen = psVector[0].iov_len;
	nCurVec = 1;

	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( 16 * PAGE_SIZE, nBytesLeft );
		char* pSrcPtr = psDev->psPort->pDataBuf;
		uint64 nBlock = nPos / 512;
	
		if ( nPos < 0 )
		{
			kerndbg( KERN_PANIC, "ata_drive_readv() wierd pos = %Ld\n", nPos );
			kassertw(0);
		}
		
		/* Prepare read command */
		ata_cmd_init( &sCmd, psDev->psPort );
	
		sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
		sCmd.nTransferLength = nCurSize;
	
		sCmd.nCmd[ATA_REG_CONTROL] = ATA_CONTROL_DEFAULT;
		sCmd.nCmd[ATA_REG_COMMAND] = ATA_CMD_READ_PIO;
		sCmd.nCmd[ATA_REG_DEVICE] = ATA_DEVICE_LBA;
		sCmd.nCmd[ATA_REG_COUNT] = nCurSize / 512;
		sCmd.nCmd[ATA_REG_LBA_LOW] = nBlock & 0xff;
		nBlock >>= 8;
		sCmd.nCmd[ATA_REG_LBA_MID] = nBlock & 0xff;
		nBlock >>= 8;
		sCmd.nCmd[ATA_REG_LBA_HIGH] = nBlock & 0xff;
		nBlock >>= 8;
		if( psDev->psPort->bLBA48bit )
			sCmd.nCmd[11] = nBlock & 0xff;
		else
			sCmd.nCmd[ATA_REG_DEVICE] |= nBlock & 0x0f;
			
		
		/* Queue and wait */
		ata_cmd_queue( psDev->psPort, &sCmd );
		LOCK( sCmd.hWait );
		//ata_cmd_ata( &sCmd );
		ata_cmd_free( &sCmd );
		
		if( sCmd.nStatus < 0 ) {
			kerndbg( KERN_FATAL, "Error: Packet finished with status %i\n", sCmd.nStatus );
			UNLOCK( psDev->hLock );
			return( -EIO );
		}

		
		nPos       += nCurSize;
		nBytesLeft -= nCurSize;

		while ( nCurSize > 0 )
		{
			int nSegSize = min( nCurSize, nCurVecLen );

			memcpy( pCurVecPtr, pSrcPtr, nSegSize );
			pSrcPtr += nSegSize;
			pCurVecPtr += nSegSize;
			nCurVecLen -= nSegSize;
			nCurSize   -= nSegSize;

			if ( nCurVecLen == 0 && nCurVec < nCount  )
			{
				pCurVecPtr = psVector[nCurVec].iov_base;
				nCurVecLen = psVector[nCurVec].iov_len;
				nCurVec++;
			}
		}
	}

    UNLOCK( psDev->hLock );
    return( nLen );
}

static int ata_drive_do_write( ATA_device_s* psDev, off_t nPos, void* pBuffer, int nLen )
{
	int nBytesLeft = nLen;
	ATA_cmd_s sCmd;
	
	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( 16 * PAGE_SIZE, nBytesLeft );
		uint64 nBlock = nPos / 512;
		
		/* Prepare read command */
		ata_cmd_init( &sCmd, psDev->psPort );
	
		sCmd.pTransferBuffer = pBuffer;
		sCmd.nTransferLength = nCurSize;
	
		sCmd.nCmd[ATA_REG_COMMAND] = ATA_CMD_WRITE_PIO;
		sCmd.nCmd[ATA_REG_DEVICE] = ATA_DEVICE_LBA;
		sCmd.nCmd[ATA_REG_COUNT] = nCurSize / 512;
		sCmd.nCmd[ATA_REG_LBA_LOW] = nBlock & 0xff;
		nBlock >>= 8;
		sCmd.nCmd[ATA_REG_LBA_MID] = nBlock & 0xff;
		nBlock >>= 8;
		sCmd.nCmd[ATA_REG_LBA_HIGH] = nBlock & 0xff;
		nBlock >>= 8;
		if( psDev->psPort->bLBA48bit )
			sCmd.nCmd[11] = nBlock & 0xff;
		else
			sCmd.nCmd[ATA_REG_DEVICE] |= nBlock & 0x0f;
			
		/* Queue and wait */
		ata_cmd_queue( psDev->psPort, &sCmd );
		//printk( "Waiting...\n" );
		LOCK( sCmd.hWait );
		//ata_cmd_ata( &sCmd );
		
		ata_cmd_free( &sCmd );
		
		if( sCmd.nStatus < 0 ) {
			kerndbg( KERN_FATAL, "Error: Packet finished with status %i\n", sCmd.nStatus );
			return( -EIO );
		}
		
		pBuffer = ((uint8*)pBuffer) + nCurSize;
		nPos += nCurSize;
		nBytesLeft -= nCurSize;
	}

	return( nLen );
}

int ata_drive_writev( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount )
{
	ATA_device_s* psDev = pNode;
	int nBytesLeft;
	int nError;
	int nLen = 0;
	int i;
	int nCurVec;
	char* pCurVecPtr;
	int   nCurVecLen;

	for ( i = 0 ; i < nCount ; ++i )
	{
		if ( psVector[i].iov_len < 0 )
		{
			kerndbg( KERN_FATAL, "Error: ata_drive_writev() negative size (%d) in vector %d\n", psVector[i].iov_len, i );
			return( -EINVAL );
		}

		nLen += psVector[i].iov_len;
	}

	if ( nLen <= 0 )
	{
		kerndbg( KERN_FATAL, "Warning: ata_drive_writev() length to small: %d\n", nLen );
		return( 0 );
	}

	if ( nPos < 0 )
	{
		kerndbg( KERN_FATAL, "Error: ata_drive_writev() negative position %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nPos & (512 - 1) )
	{
		kerndbg( KERN_FATAL, "Error: ata_drive_writev() position has bad alignment %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nLen & (512 - 1) )
	{
		kerndbg( KERN_FATAL,  "Error: ata_drive_writev() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psDev->nSize )
	{
		kerndbg( KERN_FATAL, "Warning: ata_drive_writev() Request outside partiton : %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psDev->nSize )
	{
		kerndbg( KERN_WARNING, "Warning: ata_drive_writev() Request truncated from %d to %Ld\n", nLen, (psDev->nSize - nPos) );
		nLen = psDev->nSize - nPos;
	}
	
	LOCK( psDev->hLock );

	nBytesLeft = nLen;
	nPos += psDev->nStart;

	pCurVecPtr = psVector[0].iov_base;
	nCurVecLen = psVector[0].iov_len;
	nCurVec = 1;

	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( PAGE_SIZE * 16, nBytesLeft );
		char* pDstPtr = psDev->psPort->pDataBuf;
		int   nBytesInBuf = 0;

		while ( nCurSize > 0 )
		{
			int nSegSize = min( nCurSize, nCurVecLen );

			memcpy( pDstPtr, pCurVecPtr, nSegSize );
			pDstPtr += nSegSize;
			pCurVecPtr += nSegSize;
			nCurVecLen -= nSegSize;
			nCurSize   -= nSegSize;
			nBytesInBuf += nSegSize;

			if ( nCurVecLen == 0 && nCurVec < nCount  )
			{
				pCurVecPtr = psVector[nCurVec].iov_base;
				nCurVecLen = psVector[nCurVec].iov_len;
				nCurVec++;
			}
		}

		nError = ata_drive_do_write( psDev, nPos, psDev->psPort->pDataBuf, nBytesInBuf );

		if ( nError < 0 )
		{
			nLen = nError;
			goto error;
		}

		nCurSize    = nError;
		nPos       += nCurSize;
		nBytesLeft -= nCurSize;
	}

error:
	UNLOCK( psDev->hLock );
	return( nLen );
}

int ata_drive_partitions( ATA_device_s* psDev );

status_t ata_drive_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	ATA_device_s*  psInode  = pNode;
	int nError = 0;

	switch( nCommand )
	{
		case IOCTL_GET_DEVICE_GEOMETRY:
		{
			device_geometry sGeo;

			sGeo.sector_count      = psInode->nSize / 512;
			sGeo.cylinder_count    = 1;
			sGeo.sectors_per_track = 1;
			sGeo.head_count	   = 1;
			sGeo.bytes_per_sector  = 512;
			sGeo.read_only 	   = false;
			sGeo.removable 	   = psInode->bRemovable;

			if ( bFromKernel )
			{
				memcpy( pArgs, &sGeo, sizeof(sGeo) );
			}
			else
			{
				nError = memcpy_to_user( pArgs, &sGeo, sizeof(sGeo) );
			}

			break;
		}

		case IOCTL_REREAD_PTABLE:
		{
			nError = ata_drive_partitions( psInode );
			break;
		}

		default:
		{
			kerndbg( KERN_WARNING, "Error: ata_drive_ioctl() unknown command %ld\n", nCommand );
			nError = -ENOSYS;
			break;
		}
	}

	return( nError );
}

static DeviceOperations_s g_sOperations = {
	ata_drive_open,
	ata_drive_close,
	ata_drive_ioctl,
	ata_drive_read,
	ata_drive_write,
	ata_drive_readv,
	ata_drive_writev,
	NULL,	// dop_add_select_req
	NULL	// dop_rem_select_req
};

/* Helper function for ata_drive_partitions */
size_t ata_drive_read_partition_data( void* pCookie, off_t nOffset, void* pBuffer, size_t nSize )
{
    return( ata_drive_read( pCookie, NULL, nOffset, pBuffer, nSize ) );
}

/* Decode partitions */
int ata_drive_partitions( ATA_device_s* psDev )
{
	int		    nNumPartitions;
	device_geometry sDiskGeom;
	Partition_s     asPartitions[16];
	ATA_device_s*     psPartition;
	ATA_device_s**    ppsTmp;
	int		    nError;
	int		    i;

	sDiskGeom.sector_count      = psDev->nSectors;
	sDiskGeom.cylinder_count    = 1;
	sDiskGeom.sectors_per_track = 1;
	sDiskGeom.head_count	= 1;
	sDiskGeom.bytes_per_sector  = 512;
	sDiskGeom.read_only 	= false;
	sDiskGeom.removable 	= psDev->bRemovable;

	kerndbg( KERN_INFO, "Decode partition table for %s\n", psDev->zName );

	nNumPartitions = ata_decode_disk_partitions( &sDiskGeom, asPartitions, 32, psDev, ata_drive_read_partition_data );

	if ( nNumPartitions < 0 )
	{
		kerndbg( KERN_INFO,  "   Invalid partition table\n" );
		return( nNumPartitions );
	}

	for ( i = 0 ; i < nNumPartitions ; ++i )
	{
		if ( asPartitions[i].p_nType != 0 && asPartitions[i].p_nSize != 0 )
			kerndbg( KERN_INFO,  "   Partition %d : %10Ld -> %10Ld %02x (%Ld)\n", i, asPartitions[i].p_nStart, asPartitions[i].p_nStart + asPartitions[i].p_nSize - 1LL, asPartitions[i].p_nType, asPartitions[i].p_nSize );
	}

	nError = 0;

	for ( psPartition = psDev->psFirstPartition ; psPartition != NULL ; psPartition = psPartition->psNext )
	{
		bool bFound = false;
		for ( i = 0 ; i < nNumPartitions ; ++i )
		{
			if ( asPartitions[i].p_nStart == psPartition->nStart && asPartitions[i].p_nSize == psPartition->nSize )
			{
				bFound = true;
				break;
			}
		}

		if ( bFound == false && atomic_read( &psPartition->nOpenCount ) > 0 )
		{
			kerndbg( KERN_FATAL, "ata_decode_partitions() Error: Open partition %s on %s has changed\n", psPartition->zName, psDev->zName );
			nError = -EBUSY;
			goto error;
		}
	}

	/* Remove deleted partitions from /dev/disk/ide/?/? */
	for ( ppsTmp = &psDev->psFirstPartition ; *ppsTmp != NULL ; )
	{
		bool bFound = false;
		psPartition = *ppsTmp;

		for ( i = 0 ; i < nNumPartitions ; ++i )
		{
			if ( asPartitions[i].p_nStart == psPartition->nStart && asPartitions[i].p_nSize == psPartition->nSize )
			{
				asPartitions[i].p_nSize = 0;
				psPartition->nPartitionType = asPartitions[i].p_nType;
				sprintf( psPartition->zName, "%d", i );
				bFound = true;
				break;
			}
		}

		if ( bFound == false )
		{
			*ppsTmp = psPartition->psNext;
			delete_device_node( psPartition->nNodeHandle );
			kfree( psPartition );
		}
		else
		{
			ppsTmp = &(*ppsTmp)->psNext;
		}
	}

	/* Create nodes for any new partitions. */
	for ( i = 0 ; i < nNumPartitions ; ++i )
	{
		char zNodePath[64];

		if ( asPartitions[i].p_nType == 0 || asPartitions[i].p_nSize == 0 )
			continue;

		psPartition = kmalloc( sizeof( ATA_device_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

		if ( psPartition == NULL )
		{
			kerndbg( KERN_FATAL, "Error: ata_drive_decode_partitions() no memory for partition inode\n" );
			break;
		}

		sprintf( psPartition->zName, "%d", i );
		psPartition->psPort			= psDev->psPort;
		psPartition->hLock			= psDev->hLock;
		psPartition->nDeviceHandle  = psDev->nDeviceHandle;
		psPartition->nSectors       = psDev->nSectors;
		psPartition->nSectorSize    = psDev->nSectorSize;
		psPartition->bRemovable    = psDev->bRemovable;

		psPartition->nPartitionType = asPartitions[i].p_nType;
		psPartition->nStart = asPartitions[i].p_nStart;
		psPartition->nSize  = asPartitions[i].p_nSize;

		psPartition->psNext = psDev->psFirstPartition;
		psDev->psFirstPartition = psPartition;
	
		strcpy( zNodePath, "disk/ata/" );
		strcat( zNodePath, psDev->zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->zName );
		strcat( zNodePath, "_new" );

		psPartition->nNodeHandle = create_device_node( psDev->psPort->nDeviceID, psPartition->nDeviceHandle, zNodePath, &g_sOperations, psPartition );
	}

	/* We now have to rename nodes that might have moved around in the table and
	 * got new names. To avoid name-clashes while renaming we first give all
	 * nodes a unique temporary name before looping over again giving them their
	 * final names
	*/

	for ( psPartition = psDev->psFirstPartition ; psPartition != NULL ; psPartition = psPartition->psNext )
	{
		char zNodePath[64];

		strcpy( zNodePath, "disk/ata/" );
		strcat( zNodePath, psDev->zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->zName );
		strcat( zNodePath, "_tmp" );

		rename_device_node( psPartition->nNodeHandle, zNodePath );
	}

	for ( psPartition = psDev->psFirstPartition ; psPartition != NULL ; psPartition = psPartition->psNext )
	{
		char zNodePath[64];

		strcpy( zNodePath, "disk/ata/" );
		strcat( zNodePath, psDev->zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->zName );

		rename_device_node( psPartition->nNodeHandle, zNodePath );
	}
    
error:
    return( nError );
}
 
/* Add one ata drive */
void ata_drive_add( ATA_port_s* psPort )
{
	char zNodePath[PATH_MAX];
	char zName[10];
	/* Create device */
	ATA_device_s* psDev = kmalloc( sizeof( ATA_device_s ), MEMF_KERNEL | MEMF_CLEAR );
	
	sprintf( zName, "hd%c", 'a' + psPort->nID );
	
	strcpy( psDev->zName, zName );
	psDev->psPort = psPort;
	psDev->hLock = create_semaphore( "ata_drive_lock", 1, 0 );
	if( psPort->bLBA48bit )
		psDev->nSectors = psPort->sID.lba_capacity_48;
	else
		psDev->nSectors = psPort->sID.lba_sectors;
	psDev->nSectorSize = 512;
	psDev->nStart = 0;
	psDev->nSize = psDev->nSectors * psDev->nSectorSize;
	psDev->bRemovable = psPort->sID.configuration & 0x80;
	psDev->nDeviceHandle = register_device( "", "ata" );
	
	strcpy( zNodePath, "disk/ata/" );
	strcat( zNodePath, zName );
	strcat( zNodePath, "/raw" );
	
	claim_device( psPort->nDeviceID, psDev->nDeviceHandle, psPort->zDeviceName, DEVICE_DRIVE );
	
	kerndbg( KERN_DEBUG_LOW, "Creating node %s\n", zNodePath );
	
	psDev->nNodeHandle = create_device_node( psPort->nDeviceID, psDev->nDeviceHandle, zNodePath, &g_sOperations, psDev );
	if( psDev->nNodeHandle < 0 ) {
		kerndbg( KERN_WARNING, "Could not create device node %s\n", zNodePath );
		return;
	}
	
	ata_drive_partitions( psDev );
}


















































