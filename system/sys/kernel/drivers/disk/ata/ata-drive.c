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
#include "ata-drive.h"
#include "ata-io.h"
#include "ata-probe.h"
#include "ata-dma.h"

extern int g_nDevices[MAX_DRIVES];
extern ata_controllers_t g_nControllers[MAX_CONTROLLERS];

DeviceOperations_s g_sAtaOperations = {
	ata_open,
	ata_close,
	ata_ioctl,
	ata_read,
	ata_write,
	ata_readv,
	ata_writev,
	NULL,	// dop_add_select_req
	NULL	// dop_rem_select_req
};

int ata_open( void* pNode, uint32 nFlags, void **ppCookie )
{
	AtaInode_s* psInode = pNode;
	int controller = psInode->bi_nController;

	LOCK( g_nControllers[controller].buf_lock );
	atomic_add( &psInode->bi_nOpenCount, 1 );
	UNLOCK( g_nControllers[controller].buf_lock );

	return( 0 );
}

int ata_close( void* pNode, void* pCookie )
{
	AtaInode_s* psInode = pNode;
	int controller = psInode->bi_nController;

	LOCK( g_nControllers[controller].buf_lock );
	atomic_add( &psInode->bi_nOpenCount, -1 );
	UNLOCK( g_nControllers[controller].buf_lock );

	return( 0 );
}

int ata_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen )
{
	AtaInode_s*  psInode  = pNode;
  	int controller = psInode->bi_nController;
	int nError;
	int nBytesLeft;

	if ( nPos & (512 - 1) )
	{
		printk( "Error: ata_read() position has bad alignment %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nLen & (512 - 1) )
	{
		printk( "Error: ata_read() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psInode->bi_nSize )
	{
		printk( "Warning: ata_read() Request outside partiton %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psInode->bi_nSize )
	{
		printk( "Warning: ata_read() Request truncated from %d to %Ld\n", nLen, (psInode->bi_nSize - nPos) );
		nLen = psInode->bi_nSize - nPos;
	}

	nBytesLeft = nLen;
	nPos += psInode->bi_nStart;

	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( ATA_BUFFER_SIZE, nBytesLeft );

		if ( nPos < 0 )
		{
			printk( "ata_read() vierd pos = %Ld\n", nPos );
			kassertw(0);
		}

		LOCK( g_nControllers[controller].buf_lock );
		nError = read_sectors( psInode, g_nControllers[controller].data_buffer, (nPos / 512), nCurSize / 512 );
		memcpy( pBuf, g_nControllers[controller].data_buffer, nCurSize );
		UNLOCK( g_nControllers[controller].buf_lock );

		if ( nError < 0 )
		{
			printk( "ata_read() failed to read %d sectors from drive %x\n", (nCurSize / 512), psInode->bi_nDriveNum );
			return( nError );
		}

		nCurSize = nError * 512;
		pBuf = ((uint8*)pBuf) + nCurSize;
		nPos       += nCurSize;
		nBytesLeft -= nCurSize;
	}

	return( nLen );
}

int ata_write( void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLen )
{
	AtaInode_s*  psInode  = pNode;
  	int controller = psInode->bi_nController;
	int nBytesLeft;
	int nError;

	if ( nPos & (512 - 1) )
	{
		printk( "Error: ata_write() position has bad alignment %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nLen & (512 - 1) )
	{
		printk( "Error: ata_write() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psInode->bi_nSize )
	{
		printk( "Warning: ata_write() Request outside partiton : %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psInode->bi_nSize )
	{
		printk( "Warning: ata_write() Request truncated from %d to %Ld\n", nLen, (psInode->bi_nSize - nPos) );
		nLen = psInode->bi_nSize - nPos;
	}

	nBytesLeft = nLen;
	nPos += psInode->bi_nStart;

	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( ATA_BUFFER_SIZE, nBytesLeft );

		LOCK( g_nControllers[controller].buf_lock );
		memcpy( g_nControllers[controller].data_buffer, pBuf, nCurSize );
		nError = write_sectors( psInode, g_nControllers[controller].data_buffer, nPos / 512, nCurSize / 512 );
		UNLOCK( g_nControllers[controller].buf_lock );

		if ( nError < 0 )
		{
			printk( "ata_write() failed to write %d sectors to drive %x\n", (nCurSize / 512), psInode->bi_nDriveNum );
			return( nError );
		}

		nCurSize = nError * 512;
		pBuf = ((uint8*)pBuf) + nCurSize;
		nPos       += nCurSize;
		nBytesLeft -= nCurSize;
	}

	return( nLen );
}

int ata_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount )
{
	AtaInode_s*  psInode  = pNode;
  	int controller = psInode->bi_nController;
	int nError;
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
		printk( "Error: ata_readv() position has bad alignment %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nLen & (512 - 1) )
	{
		printk( "Error: ata_readv() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psInode->bi_nSize )
	{
		printk( "Warning: ata_readv() Request outside partiton : %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psInode->bi_nSize )
	{
		printk( "Warning: ata_readv() Request truncated from %d to %Ld\n", nLen, (psInode->bi_nSize - nPos) );
		nLen = psInode->bi_nSize - nPos;
	}

	nBytesLeft = nLen;
	nPos += psInode->bi_nStart;

	pCurVecPtr = psVector[0].iov_base;
	nCurVecLen = psVector[0].iov_len;
	nCurVec = 1;

	LOCK( g_nControllers[controller].buf_lock );
	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( ATA_BUFFER_SIZE, nBytesLeft );
		char* pSrcPtr = g_nControllers[controller].data_buffer;
	
		if ( nPos < 0 )
		{
			printk( "ata_readv() wierd pos = %Ld\n", nPos );
			kassertw(0);
		}

		nError = read_sectors( psInode, g_nControllers[controller].data_buffer, (nPos / 512), nCurSize / 512 );
		nCurSize = nError * 512;

		if ( nError < 0 )
		{
			printk( "ata_readv() failed to read %d sectors from drive %x\n", (nCurSize / 512), psInode->bi_nDriveNum );
			nLen = nError;
			goto error;
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

error:
    UNLOCK( g_nControllers[controller].buf_lock );
    return( nLen );
}

static int ata_do_write( AtaInode_s* psInode, off_t nPos, void* pBuffer, int nLen )
{
	int nBytesLeft;
	int nError;

	nBytesLeft = nLen;

	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( ATA_BUFFER_SIZE, nBytesLeft );

		nError = write_sectors( psInode, pBuffer, nPos / 512, nCurSize / 512 );

		if ( nError < 0 )
		{
			printk( "ata_do_write() failed to write %d sectors to drive %x\n", nCurSize / 512, psInode->bi_nDriveNum );
			return( nError );
		}

		nCurSize = nError * 512;
		pBuffer = ((uint8*)pBuffer) + nCurSize;
		nPos       += nCurSize;
		nBytesLeft -= nCurSize;
	}

	return( (nLen < 0) ? nLen : (nLen * 512) );
}

int ata_writev( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount )
{
	AtaInode_s*  psInode  = pNode;
  	int controller = psInode->bi_nController;
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
			printk( "Error: ata_writev() negative size (%d) in vector %d\n", psVector[i].iov_len, i );
			return( -EINVAL );
		}

		nLen += psVector[i].iov_len;
	}

	if ( nLen <= 0 )
	{
		printk( "Warning: ata_writev() length to small: %d\n", nLen );
		return( 0 );
	}

	if ( nPos < 0 )
	{
		printk( "Error: ata_writev() negative position %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nPos & (512 - 1) )
	{
		printk( "Error: ata_writev() position has bad alignment %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nLen & (512 - 1) )
	{
		printk( "Error: ata_writev() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psInode->bi_nSize )
	{
		printk( "Warning: ata_writev() Request outside partiton : %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psInode->bi_nSize )
	{
		printk( "Warning: ata_writev() Request truncated from %d to %Ld\n", nLen, (psInode->bi_nSize - nPos) );
		nLen = psInode->bi_nSize - nPos;
	}

	nBytesLeft = nLen;
	nPos += psInode->bi_nStart;

	pCurVecPtr = psVector[0].iov_base;
	nCurVecLen = psVector[0].iov_len;
	nCurVec = 1;

	LOCK( g_nControllers[controller].buf_lock );
	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( ATA_BUFFER_SIZE, nBytesLeft );
		char* pDstPtr = g_nControllers[controller].data_buffer;
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

		nError = ata_do_write( psInode, nPos, g_nControllers[controller].data_buffer, nBytesInBuf );

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
    UNLOCK( g_nControllers[controller].buf_lock );
    return( nLen );
}


status_t ata_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	AtaInode_s*  psInode  = pNode;
	int nError = 0;

	switch( nCommand )
	{
		case IOCTL_GET_DEVICE_GEOMETRY:
		{
			device_geometry sGeo;

			sGeo.sector_count      = psInode->bi_nSize / 512;
			sGeo.cylinder_count    = psInode->bi_nCylinders;
			sGeo.sectors_per_track = psInode->bi_nSectors;
			sGeo.head_count	   = psInode->bi_nHeads;
			sGeo.bytes_per_sector  = 512;
			sGeo.read_only 	   = false;
			sGeo.removable 	   = psInode->bi_bRemovable;

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
			nError = ata_decode_partitions( psInode );
			break;
		}

		default:
		{
			printk( "Error: ata_ioctl() unknown command %ld\n", nCommand );
			nError = -ENOSYS;
			break;
		}
	}

	return( nError );
}

