/*
 *  The Syllable kernel
 *  Simple SCSI layer
 *  Contains some linux kernel code
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 2006 Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU
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
 *
 */

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/scsi.h>
#include <atheos/cdrom.h>
#include <atheos/bootmodules.h>
#include <atheos/udelay.h>
#include <posix/errno.h>
#include <macros.h>

#include <scsi_common.h>
#include <scsi_generic.h>

//#undef DEBUG_LIMIT
//#define DEBUG_LIMIT   KERN_DEBUG_LOW

int scsi_generic_open( void *pNode, uint32 nFlags, void **ppCookie )
{
	SCSI_device_s *psDevice = pNode;
	int nError = 0;

	LOCK( psDevice->hLock );

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_open() called\n" );

	/* Discover if the drive is ready */
	nError = scsi_test_ready( psDevice );
	if( nError < 0 )
	{
		kerndbg( KERN_DEBUG, "An error occured waiting for the device, return code 0x%02x\n", nError );
	}
	else
	{
		/* No harm in allowing the drive firmware a little time */
		snooze( 10000 );

		nError = scsi_read_capacity( psDevice, NULL );
		if( nError < 0 )
		{
			kerndbg( KERN_DEBUG, "An error occured reading from the device, return code 0x%02x\n", nError );
		}
		else
			atomic_inc( &psDevice->nOpenCount );
	}

	UNLOCK( psDevice->hLock );

	return nError;
}

int scsi_generic_close( void *pNode, void *pCookie )
{
	SCSI_device_s *psDevice = pNode;

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_close() called\n" );

	LOCK( psDevice->hLock );
	atomic_dec( &psDevice->nOpenCount );
	UNLOCK( psDevice->hLock );
	return 0;
}

static int scsi_generic_do_read( SCSI_device_s * psDevice, off_t nPos, void *pBuffer, int nLen )
{
	uint64 nBlock;
	uint64 nBlockCount;
	SCSI_cmd sCmd;
	int nError;
	size_t nToRead = nLen;
	size_t nReadNow;

	/* The iso9660 fs requires this workaround... */
	if( nToRead < psDevice->nSectorSize )
		nToRead = psDevice->nSectorSize;

	while( nToRead > 0 )
	{
		if( nToRead > 0xffff )
			nReadNow = 0xffff;
		else
			nReadNow = nToRead;

		nBlock = nPos / psDevice->nSectorSize;
		nBlockCount = nReadNow / psDevice->nSectorSize;

		/* Build SCSI_READ_10 command */
		scsi_init_cmd( &sCmd, psDevice );

		sCmd.nDirection = SCSI_DATA_READ;
		sCmd.pRequestBuffer = psDevice->pDataBuffer;
		sCmd.nRequestSize = nReadNow;

		sCmd.nCmd[0] = SCSI_READ_10;
		if( psDevice->nSCSILevel <= SCSI_2 )
			sCmd.nCmd[1] = ( psDevice->nLun << 5 ) & 0xe0;
		else
			sCmd.nCmd[1] = 0;
		sCmd.nCmd[2] = ( unsigned char )( nBlock >> 24 ) & 0xff;
		sCmd.nCmd[3] = ( unsigned char )( nBlock >> 16 ) & 0xff;
		sCmd.nCmd[4] = ( unsigned char )( nBlock >> 8 ) & 0xff;
		sCmd.nCmd[5] = ( unsigned char )nBlock & 0xff;
		sCmd.nCmd[6] = sCmd.nCmd[9] = 0;
		sCmd.nCmd[7] = ( unsigned char )( nBlockCount >> 8 ) & 0xff;
		sCmd.nCmd[8] = ( unsigned char )nBlockCount & 0xff;

		sCmd.nCmdLen = scsi_get_command_size( SCSI_READ_10 );

		kerndbg( KERN_DEBUG, "Reading block %i (%i)\n", ( int )nBlock, ( int )nBlockCount );

		/* Send command */
		nError = psDevice->psHost->queue_command( &sCmd );

		kerndbg( KERN_DEBUG, "nError=%d\tsCmd.nResult=%d\n", nError, sCmd.nResult );

		if( nError != 0 || sCmd.nResult != 0 )
		{
			kerndbg( KERN_WARNING, "SCSI: Error while reading!\n" );
			return -EIO;
		}

		/* Copy data */
		if( nLen < psDevice->nSectorSize )
			memcpy( pBuffer, psDevice->pDataBuffer, nLen );
		else
			memcpy( pBuffer, psDevice->pDataBuffer, nReadNow );

		nPos += nReadNow;
		pBuffer += nReadNow;
		nToRead -= nReadNow;
	}

	return nLen;
}

int scsi_generic_read( void *pNode, void *pCookie, off_t nPos, void *pBuffer, size_t nLen )
{
	SCSI_device_s *psDevice = pNode;
	int nRet;

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_read() called\n" );

	LOCK( psDevice->hLock );

	nPos += psDevice->nStart;
	nRet = scsi_generic_do_read( psDevice, nPos, pBuffer, nLen );

	UNLOCK( psDevice->hLock );

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_read() completed: %d\n", nRet );

	return nRet;
}

int scsi_generic_readv( void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount )
{
	SCSI_device_s *psDevice = pNode;
	uint64 nBlock;
	uint64 nBlockCount;
	SCSI_cmd sCmd;
	int nError;
	size_t nToRead = 0;
	size_t nReadNow;
	int nLen = 0;
	int i;
	int nCurVec;
	char *pCurVecPtr;
	int nCurVecLen;

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_readv() called\n" );

	for( i = 0; i < nCount; ++i )
		nLen += psVector[i].iov_len;

	nToRead = nLen;

	if( nToRead <= 0 )
		return 0;

	if( ( nPos & ( psDevice->nSectorSize - 1 ) ) || ( nToRead & ( psDevice->nSectorSize - 1 ) ) )
	{
		printk( "SCSI: Invalid position requested\n" );
		return -EIO;
	}

	if( nPos >= psDevice->nSize )
	{
		kerndbg( KERN_FATAL, "Warning: scsi_generic_readv() Request outside partiton : %Ld\n", nPos );
		return 0;
	}

	if( nPos + nToRead > psDevice->nSize )
	{
		kerndbg( KERN_WARNING, "Warning: scsi_generic_readv() Request truncated from %d to %Ld\n", nLen, ( psDevice->nSize - nPos ) );
		nLen = nToRead = psDevice->nSize - nPos;
	}

	nPos += psDevice->nStart;

	pCurVecPtr = psVector[0].iov_base;
	nCurVecLen = psVector[0].iov_len;
	nCurVec = 1;

	LOCK( psDevice->hLock );

	while( nToRead > 0 )
	{
		char *pSrcPtr = psDevice->pDataBuffer;

		if( nToRead > 0xffff )
			nReadNow = 0xffff;
		else
			nReadNow = nToRead;

		if( nPos < 0 )
		{
			kerndbg( KERN_PANIC, "scsi_generic_readv() wierd pos = %Ld\n", nPos );
			kassertw( 0 );
		}

		nError = scsi_generic_do_read( psDevice, nPos, psDevice->pDataBuffer, nReadNow );
		if( nError < 0 )
		{
			nLen = nError;
			goto error;
		}

		nReadNow = nError;
		nPos += nReadNow;
		nToRead -= nReadNow;

		while( nReadNow > 0 )
		{
			int nSegSize = min( nReadNow, nCurVecLen );

			memcpy( pCurVecPtr, pSrcPtr, nSegSize );
			pSrcPtr += nSegSize;
			pCurVecPtr += nSegSize;
			nCurVecLen -= nSegSize;
			nReadNow -= nSegSize;

			if( nCurVecLen == 0 && nCurVec < nCount )
			{
				pCurVecPtr = psVector[nCurVec].iov_base;
				nCurVecLen = psVector[nCurVec].iov_len;
				nCurVec++;
			}
		}
	}

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_readv() completed: %d\n", nLen );

error:
	UNLOCK( psDevice->hLock );
	return nLen;
}

static int scsi_generic_do_write( SCSI_device_s * psDevice, off_t nPos, const void *pBuffer, int nLen )
{
	int nToWrite = nLen;
	int nWriteNow = 0;
	uint64 nBlock;
	uint64 nBlockCount;
	SCSI_cmd sCmd;
	int nError = 0;

	while( nToWrite > 0 )
	{
		if( nToWrite > 0xffff )
			nWriteNow = 0xffff;
		else
			nWriteNow = nToWrite;

		nBlock = nPos / psDevice->nSectorSize;
		nBlockCount = nWriteNow / psDevice->nSectorSize;

		/* Build SCSI_WRITE_10 command */
		scsi_init_cmd( &sCmd, psDevice );

		sCmd.nDirection = SCSI_DATA_WRITE;
		sCmd.pRequestBuffer = (void*)pBuffer;
		sCmd.nRequestSize = nWriteNow;

		sCmd.nCmd[0] = SCSI_WRITE_10;
		if( psDevice->nSCSILevel <= SCSI_2 )
			sCmd.nCmd[1] = ( psDevice->nLun << 5 ) & 0xe0;
		else
			sCmd.nCmd[1] = 0;
		sCmd.nCmd[2] = ( unsigned char )( nBlock >> 24 ) & 0xff;
		sCmd.nCmd[3] = ( unsigned char )( nBlock >> 16 ) & 0xff;
		sCmd.nCmd[4] = ( unsigned char )( nBlock >> 8 ) & 0xff;
		sCmd.nCmd[5] = ( unsigned char )nBlock & 0xff;
		sCmd.nCmd[6] = sCmd.nCmd[9] = 0;
		sCmd.nCmd[7] = ( unsigned char )( nBlockCount >> 8 ) & 0xff;
		sCmd.nCmd[8] = ( unsigned char )nBlockCount & 0xff;

		sCmd.nCmdLen = scsi_get_command_size( SCSI_WRITE_10 );

		kerndbg( KERN_DEBUG, "Writing block %i (%i)\n", ( int )nBlock, ( int )nBlockCount );

		/* Send command */
		nError = psDevice->psHost->queue_command( &sCmd );

		kerndbg( KERN_DEBUG, "Result: %i\n", sCmd.nResult );

		if( nError != 0 || sCmd.nResult != 0 )
		{
			printk( "SCSI: Error while writing!\n" );
			return -EIO;
		}

		pBuffer = ( ( uint8 * )pBuffer ) + nWriteNow;
		nPos += nWriteNow;
		nToWrite -= nWriteNow;
	}
	return nLen;
}

int scsi_generic_write( void *pNode, void *pCookie, off_t nPos, const void *pBuffer, size_t nLen )
{
	SCSI_device_s *psDevice = pNode;
	int nRet;

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_write() called\n" );

	if( ( nPos & ( psDevice->nSectorSize - 1 ) ) || ( nLen & ( psDevice->nSectorSize - 1 ) ) )
	{
		printk( "SCSI: Invalid position requested\n" );
		return -EIO;
	}

	LOCK( psDevice->hLock );

	nPos += psDevice->nStart;
	nRet = scsi_generic_do_write( psDevice, nPos, pBuffer, nLen );

	UNLOCK( psDevice->hLock );

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_write() completed: %d\n", nRet );

	return nRet;
}

int scsi_generic_writev( void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount )
{
	SCSI_device_s *psDev = pNode;
	int nBytesLeft;
	int nError;
	int nLen = 0;
	int i;
	int nCurVec;
	char *pCurVecPtr;
	int nCurVecLen;

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_writev() called\n" );

	for( i = 0; i < nCount; ++i )
	{
		if( psVector[i].iov_len < 0 )
		{
			kerndbg( KERN_FATAL, "Error: scsi_generic_writev() negative size (%d) in vector %d\n", psVector[i].iov_len, i );
			return ( -EINVAL );
		}

		nLen += psVector[i].iov_len;
	}

	if( nLen <= 0 )
	{
		kerndbg( KERN_FATAL, "Warning: scsi_generic_writev() length to small: %d\n", nLen );
		return 0;
	}

	if( nPos < 0 )
	{
		kerndbg( KERN_FATAL, "Error: scsi_generic_writev() negative position %Ld\n", nPos );
		return -EINVAL;
	}

	if( nPos & ( psDev->nSectorSize - 1 ) )
	{
		kerndbg( KERN_FATAL, "Error: scsi_generic_writev() position has bad alignment %Ld\n", nPos );
		return -EINVAL;
	}

	if( nLen & ( psDev->nSectorSize - 1 ) )
	{
		kerndbg( KERN_FATAL, "Error: scsi_generic_writev() length has bad alignment %d\n", nLen );
		return -EINVAL;
	}

	if( nPos >= psDev->nSize )
	{
		kerndbg( KERN_FATAL, "Warning: scsi_generic_writev() Request outside partiton : %Ld\n", nPos );
		return 0;
	}

	if( nPos + nLen > psDev->nSize )
	{
		kerndbg( KERN_WARNING, "Warning: scsi_generic_writev() Request truncated from %d to %Ld\n", nLen, ( psDev->nSize - nPos ) );
		nLen = psDev->nSize - nPos;
	}

	LOCK( psDev->hLock );

	nBytesLeft = nLen;
	nPos += psDev->nStart;

	pCurVecPtr = psVector[0].iov_base;
	nCurVecLen = psVector[0].iov_len;
	nCurVec = 1;

	while( nBytesLeft > 0 )
	{
		int nCurSize = min( 0xffff, nBytesLeft );
		char *pDstPtr = psDev->pDataBuffer;
		int nBytesInBuf = 0;

		while( nCurSize > 0 )
		{
			int nSegSize = min( nCurSize, nCurVecLen );

			memcpy( pDstPtr, pCurVecPtr, nSegSize );
			pDstPtr += nSegSize;
			pCurVecPtr += nSegSize;
			nCurVecLen -= nSegSize;
			nCurSize -= nSegSize;
			nBytesInBuf += nSegSize;

			if( nCurVecLen == 0 && nCurVec < nCount )
			{
				pCurVecPtr = psVector[nCurVec].iov_base;
				nCurVecLen = psVector[nCurVec].iov_len;
				nCurVec++;
			}
		}

		nError = scsi_generic_do_write( psDev, nPos, psDev->pDataBuffer, nBytesInBuf );

		if( nError < 0 )
		{
			nLen = nError;
			goto error;
		}

		nCurSize = nError;
		nPos += nCurSize;
		nBytesLeft -= nCurSize;
	}

	kerndbg( KERN_DEBUG_LOW, "scsi_generic_writev() completed: %d\n", nLen );

error:
	UNLOCK( psDev->hLock );
	return nLen;
}
