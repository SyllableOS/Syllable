
/*
 *  The Syllable kernel
 *  Simple SCSI layer
 *  Contains some linux kernel code
 *  Copyright (C) 2003 Arno Klenke
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
#include <atheos/bootmodules.h>
#include <posix/errno.h>
#include <macros.h>

static SCSI_device_s *g_psFirstDevice;
static bool g_nIDTable[255];

const unsigned char nSCSICmdSize[8] = {
	6, 10, 10, 12,
	16, 12, 10, 10
};


const char *const nSCSIDeviceTypes[14] = {
	"Direct-Access    ",
	"Sequential-Access",
	"Printer          ",
	"Processor        ",
	"WORM             ",
	"CD-ROM           ",
	"Scanner          ",
	"Optical Device   ",
	"Medium Changer   ",
	"Communications   ",
	"Unknown          ",
	"Unknown          ",
	"Unknown          ",
	"Enclosure        ",
};

static int scsi_decode_partitions( SCSI_device_s * psInode );

unsigned char scsi_get_command_size( int nOpcode )
{
	return( nSCSICmdSize[((nOpcode) >> 5) & 7] );
}

static int scsi_open( void *pNode, uint32 nFlags, void **ppCookie )
{
	SCSI_cmd_s sCmd;
	SCSI_device_s *psDevice = pNode;
	SCSI_host_s *psHost = psDevice->psHost;
	int nError;
	uint32 nCapacity = 0;
	uint32 nSectorSize = 0;
	uint8 *pBuffer;
	uint32 nTries = 0;

	LOCK( psDevice->hLock );

	printk( "Opening SCSI disk...\n" );
	
	while( nTries < 3 )
	{

		/* Try to test the state of the unit */
		sCmd.psHost = psDevice->psHost;
		sCmd.nChannel = psDevice->nChannel;
		sCmd.nDevice = psDevice->nDevice;
		sCmd.nLun = psDevice->nLun;
		sCmd.nDirection = SCSI_DATA_NONE;

		sCmd.nCmd[0] = SCSI_TEST_UNIT_READY;
		if ( psDevice->nSCSILevel <= SCSI_2 )
			sCmd.nCmd[1] = ( psDevice->nLun << 5 ) & 0xe0;
		else
			sCmd.nCmd[1] = 0;
		memset( ( void * )&sCmd.nCmd[2], 0, 8 );
		sCmd.nCmdLen = scsi_get_command_size( SCSI_TEST_UNIT_READY );
		sCmd.nSense[0] = 0;
		sCmd.nSense[2] = 0;

		/* Send command */
		nError = psHost->queue_command( &sCmd );

		//printk( "Result: %i\n", sCmd.nResult );

		if ( sCmd.nResult != 0 && !( sCmd.nSense[2] == SCSI_UNIT_ATTENTION &&
			sCmd.nSense[12] == 0x3A ) ) {
		} else {
			break;
		}
		
		snooze( 10 * 1000 );
		nTries++;
	}
	
	if( nTries == 3 ) {
		printk( "SCSI: Error while sending SCSI_TEST_UNIT_READY\n" );
		UNLOCK( psDevice->hLock );
		return( -EIO );
	}

	/* Read capacity */
	pBuffer = kmalloc( 512, MEMF_KERNEL );

	printk( "SCSI: Reading capacity...\n" );
	
	nTries = 0;
	while( nTries < 3 )
	{
		sCmd.nCmd[0] = SCSI_READ_CAPACITY;
		sCmd.nCmd[1] = ( psDevice->nLun << 5 ) & 0xe0;

		memset( ( void * )&sCmd.nCmd[2], 0, 8 );
		memset( ( void * )pBuffer, 0, 8 );

		sCmd.nCmdLen = scsi_get_command_size( SCSI_READ_CAPACITY );

		sCmd.nSense[0] = 0;
		sCmd.nSense[2] = 0;
		sCmd.nDirection = SCSI_DATA_READ;
		sCmd.pRequestBuffer = pBuffer;
		sCmd.nRequestSize = 8;

		/* Send command */
		nError = psHost->queue_command( &sCmd );

		//printk( "Result: %i\n", sCmd.nResult );

		if ( nError != 0 || sCmd.nResult != 0 ) {
			
			
		} else {
			/* Calculate capacity and sectorsize */
			nCapacity = 1 + ( ( pBuffer[0] << 24 ) | ( pBuffer[1] << 16 ) | ( pBuffer[2] << 8 ) | pBuffer[3] );
			nSectorSize = ( pBuffer[4] << 24 ) | ( pBuffer[5] << 16 ) | ( pBuffer[6] << 8 ) | pBuffer[7];
			break;
		}
		snooze( 10 * 1000 );
		nTries++;
	}
	
	if( nTries == 3 )
	{
		printk( "SCSI: Could not read capacity!\n" );
		UNLOCK( psDevice->hLock );
		return ( -EIO );
	}

	/* Save values */
	psDevice->nSectorSize = nSectorSize;
	psDevice->nSectors = nCapacity;

	kfree( pBuffer );

	printk( "SCSI: %i sectors of %i bytes\n", ( int )psDevice->nSectors, ( int )psDevice->nSectorSize );

	atomic_inc( &psDevice->nOpenCount );
	UNLOCK( psDevice->hLock );

	return ( 0 );
}


static int scsi_close( void *pNode, void *pCookie )
{
	SCSI_device_s *psDevice = pNode;

	LOCK( psDevice->hLock );
	atomic_dec( &psDevice->nOpenCount );
	UNLOCK( psDevice->hLock );
	//printk( "scsi_close() called\n" );
	return ( 0 );
}

static int scsi_read( void *pNode, void *pCookie, off_t nPos, void *pBuf, size_t nLen )
{
	SCSI_device_s *psDevice = pNode;
	SCSI_host_s *psHost = psDevice->psHost;
	uint64 nBlock;
	uint64 nBlockCount;
	SCSI_cmd sCmd;
	int nError;
	size_t nToRead = nLen;
	size_t nReadNow;

	//printk( "scsi_read() called\n" );

	if ( ( nPos & ( psDevice->nSectorSize - 1 ) ) || ( nLen & ( psDevice->nSectorSize - 1 ) ) )
	{
		printk( "SCSI: Invalid position requested\n" );
		return ( -EIO );
	}

	nPos += psDevice->nStart;

	while ( nToRead > 0 )
	{
		if ( nToRead > 0xffff )
		{
			nReadNow = 0xffff;
		}
		else
		{
			nReadNow = nToRead;
		}
		nBlock = nPos / psDevice->nSectorSize;
		nBlockCount = nReadNow / psDevice->nSectorSize;

		/* Build SCSI_READ_10 command */
		sCmd.psHost = psHost;
		sCmd.nChannel = psDevice->nChannel;
		sCmd.nDevice = psDevice->nDevice;
		sCmd.nLun = psDevice->nLun;
		sCmd.nDirection = SCSI_DATA_READ;

		sCmd.nCmd[0] = SCSI_READ_10;
		if ( psDevice->nSCSILevel <= SCSI_2 )
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

		sCmd.pRequestBuffer = psDevice->pDataBuffer;
		sCmd.nRequestSize = nReadNow;

		//printk( "Reading block %i (%i)\n", (int)nBlock, (int)nBlockCount );

		LOCK( psDevice->hLock );

		/* Send command */
		nError = psHost->queue_command( &sCmd );

		//printk( "Result: %i\n", sCmd.nResult );

		if ( nError != 0 || sCmd.nResult != 0 )
		{
			UNLOCK( psDevice->hLock );
			printk( "SCSI: Error while reading!\n" );
			return ( -EIO );
		}

		/* Copy data */
		memcpy( pBuf, psDevice->pDataBuffer, nReadNow );

		UNLOCK( psDevice->hLock );

		nPos += nReadNow;
		pBuf += nReadNow;
		nToRead -= nReadNow;
	}

	return ( nLen );
}


static size_t scsi_read_partition_data( void *pCookie, off_t nOffset, void *pBuffer, size_t nSize )
{
	return ( scsi_read( pCookie, NULL, nOffset, pBuffer, nSize ) );
}

static int scsi_write( void *pNode, void *pCookie, off_t nPos, const void *pBuf, size_t nLen )
{
	SCSI_device_s *psDevice = pNode;
	SCSI_host_s *psHost = psDevice->psHost;
	uint64 nBlock;
	uint64 nBlockCount;
	SCSI_cmd sCmd;
	int nError;
	size_t nToWrite = nLen;
	size_t nWriteNow;

	//printk( "scsi_write() called\n" );

	if ( ( nPos & ( psDevice->nSectorSize - 1 ) ) || ( nLen & ( psDevice->nSectorSize - 1 ) ) )
	{
		printk( "SCSI: Invalid position requested\n" );
		return ( -EIO );
	}

	nPos += psDevice->nStart;

	while ( nToWrite > 0 )
	{
		if ( nToWrite > 0xffff )
		{
			nWriteNow = 0xffff;
		}
		else
		{
			nWriteNow = nToWrite;
		}
		nBlock = nPos / psDevice->nSectorSize;
		nBlockCount = nWriteNow / psDevice->nSectorSize;

		/* Build SCSI_WRITE_10 command */
		sCmd.psHost = psHost;
		sCmd.nChannel = psDevice->nChannel;
		sCmd.nDevice = psDevice->nDevice;
		sCmd.nLun = psDevice->nLun;
		sCmd.nDirection = SCSI_DATA_WRITE;

		sCmd.nCmd[0] = SCSI_WRITE_10;
		if ( psDevice->nSCSILevel <= SCSI_2 )
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

		sCmd.pRequestBuffer = psDevice->pDataBuffer;
		sCmd.nRequestSize = nWriteNow;

		//printk( "Writing block %i (%i)\n", (int)nBlock, (int)nBlockCount );

		LOCK( psDevice->hLock );

		/* Copy data */
		memcpy( psDevice->pDataBuffer, pBuf, nWriteNow );

		/* Send command */
		nError = psHost->queue_command( &sCmd );

		//printk( "Result: %i\n", sCmd.nResult );

		if ( nError != 0 || sCmd.nResult != 0 )
		{
			UNLOCK( psDevice->hLock );
			printk( "SCSI: Error while writing!\n" );
			return ( -EIO );
		}

		UNLOCK( psDevice->hLock );

		nPos += nWriteNow;
		pBuf += nWriteNow;
		nToWrite -= nWriteNow;
	}

	return ( nLen );
}


static int scsi_readv( void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount )
{
	int i;
	int nError = 0;

	//printk( "scsi_readv() called!\n" );
	for ( i = 0; i < nCount; ++i )
	{
		ssize_t nCurSize = scsi_read( pNode, pCookie, nPos + nError,
			psVector[i].iov_base, psVector[i].iov_len );

		if ( nCurSize < 0 )
		{
			nError = nCurSize;
			goto error;
		}
		nError += nCurSize;
		if ( nCurSize != psVector[i].iov_len )
		{
			break;
		}
	}
      error:
	return ( nError );
}



static int scsi_writev( void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount )
{
	int i;
	int nError = 0;

	//printk( "scsi_writev() called!\n" );
	for ( i = 0; i < nCount; ++i )
	{
		ssize_t nCurSize = scsi_write( pNode, pCookie, nPos + nError,
			psVector[i].iov_base, psVector[i].iov_len );

		if ( nCurSize < 0 )
		{
			nError = nCurSize;
			goto error;
		}
		nError += nCurSize;
		if ( nCurSize != psVector[i].iov_len )
		{
			break;
		}
	}
      error:
	return ( nError );
}

static status_t scsi_ioctl( void *pNode, void *pCookie, uint32 nCommand, void *pArgs, bool bFromKernel )
{
	int nError = 0;
	SCSI_device_s *psDevice = pNode;

	//printk( "scsi_ioctl() called!\n" );

	switch ( nCommand )
	{
	case IOCTL_GET_DEVICE_GEOMETRY:
		{
			device_geometry sGeo;

			sGeo.sector_count = psDevice->nSectors;
			sGeo.cylinder_count = psDevice->nSectorSize;
			sGeo.sectors_per_track = 1;
			sGeo.head_count = 1;
			sGeo.bytes_per_sector = psDevice->nSectorSize;
			sGeo.read_only = false;
			sGeo.removable = psDevice->bRemovable;

			if ( bFromKernel )
				memcpy( pArgs, &sGeo, sizeof( sGeo ) );
			else
				nError = memcpy_to_user( pArgs, &sGeo, sizeof( sGeo ) );

			break;
		}
	case IOCTL_REREAD_PTABLE:
		nError = scsi_decode_partitions( psDevice );
		break;
	default:
		{
			kerndbg( KERN_WARNING, "scsi_ioctl() unknown command %ld\n", nCommand );
			nError = -ENOSYS;
			break;
		}
	}
	return ( nError );
}

DeviceOperations_s g_sOperations = {
	scsi_open,
	scsi_close,
	scsi_ioctl,
	scsi_read,
	scsi_write,
	scsi_readv,
	scsi_writev,
	NULL,			// dop_add_select_req
	NULL			// dop_rem_select_req
};

static void scsi_print_inquiry( unsigned char *data )
{
	int i;
	char buf[256];
	char buf2[256];

	strcpy( buf, "SCSI: Vendor: " );
	for ( i = 8; i < 16; i++ )
	{
		if ( data[i] >= 0x20 && i < data[4] + 5 )
		{
			sprintf( buf2, "%c", data[i] );
			strcat( buf, buf2 );
		}
		else
			strcat( buf, " " );
	}
	strcat( buf, "\n" );
	printk( buf );

	strcpy( buf, "SCSI: Model: " );
	for ( i = 16; i < 32; i++ )
	{
		if ( data[i] >= 0x20 && i < data[4] + 5 )
		{
			sprintf( buf2, "%c", data[i] );
			strcat( buf, buf2 );
		}
		else
			strcat( buf, " " );
	}
	strcat( buf, "\n" );
	printk( buf );

	strcpy( buf, "SCSI: Revision: " );
	for ( i = 32; i < 36; i++ )
	{
		if ( data[i] >= 0x20 && i < data[4] + 5 )
		{
			sprintf( buf2, "%c", data[i] );
			strcat( buf, buf2 );
		}
		else
			strcat( buf, " " );
	}
	strcat( buf, "\n" );
	printk( buf );

	i = data[0] & 0x1f;

	printk( "SCSI: Type: %s\n", i < 14 ? nSCSIDeviceTypes[i] : "Unknown" );
	printk( "SCSI: ANSI SCSI revision: %02x\n", data[2] & 0x07 );
}


static int scsi_decode_partitions( SCSI_device_s * psInode )
{
	int nNumPartitions;
	device_geometry sDiskGeom;
	Partition_s asPartitions[16];
	SCSI_device_s *psPartition;
	SCSI_device_s **ppsTmp;
	int nError;
	int i;

	sDiskGeom.sector_count = psInode->nSectors;
	sDiskGeom.cylinder_count = 0;
	sDiskGeom.sectors_per_track = 0;
	sDiskGeom.head_count = 0;
	sDiskGeom.bytes_per_sector = psInode->nSectorSize;
	sDiskGeom.read_only = false;
	sDiskGeom.removable = psInode->bRemovable;

	printk( "Decode partition table for %s\n", psInode->zName );

	nNumPartitions = decode_disk_partitions( &sDiskGeom, asPartitions, 16, psInode, scsi_read_partition_data );

	if ( nNumPartitions < 0 )
	{
		printk( "   Invalid partition table\n" );
		return ( nNumPartitions );
	}
	for ( i = 0; i < nNumPartitions; ++i )
	{
		if ( asPartitions[i].p_nType != 0 && asPartitions[i].p_nSize != 0 )
		{
			printk( "   Partition %d : %10Ld -> %10Ld %02x (%Ld)\n", i, asPartitions[i].p_nStart, asPartitions[i].p_nStart + asPartitions[i].p_nSize - 1LL, asPartitions[i].p_nType, asPartitions[i].p_nSize );
		}
	}
	nError = 0;

	for ( psPartition = psInode->psFirstPartition; psPartition != NULL; psPartition = psPartition->psNext )
	{
		bool bFound = false;

		for ( i = 0; i < nNumPartitions; ++i )
		{
			if ( asPartitions[i].p_nStart == psPartition->nStart && asPartitions[i].p_nSize == psPartition->nSize )
			{
				bFound = true;
				break;
			}
		}
		if ( bFound == false && atomic_read( &psPartition->nOpenCount ) > 0 )
		{
			printk( "scsi_decode_partitions() Error: Open partition %s on %s has changed\n", psPartition->zName, psInode->zName );
			nError = -EBUSY;
			goto error;
		}
	}

	// Remove deleted partitions from /dev/disk/bios/*/*
	for ( ppsTmp = &psInode->psFirstPartition; *ppsTmp != NULL; )
	{
		bool bFound = false;

		psPartition = *ppsTmp;
		for ( i = 0; i < nNumPartitions; ++i )
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
			ppsTmp = &( *ppsTmp )->psNext;
		}
	}

	// Create nodes for any new partitions.
	for ( i = 0; i < nNumPartitions; ++i )
	{
		char zNodePath[64];

		if ( asPartitions[i].p_nType == 0 || asPartitions[i].p_nSize == 0 )
		{
			continue;
		}

		psPartition = kmalloc( sizeof( SCSI_device_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
		memset( psPartition, 0, sizeof( SCSI_device_s ) );

		if ( psPartition == NULL )
		{
			printk( "Error: scsi_decode_partitions() no memory for partition inode\n" );
			break;
		}

		sprintf( psPartition->zName, "%d", i );
		psPartition->psRawDevice = psInode;
		psPartition->psHost = psInode->psHost;
		psPartition->hLock = psInode->hLock;
		psPartition->nID = psInode->nID;
		psPartition->nDeviceHandle = psInode->nDeviceHandle;
		psPartition->nChannel = psInode->nChannel;
		psPartition->nDevice = psInode->nDevice;
		psPartition->nLun = psInode->nLun;
		psPartition->nType = psInode->nType;
		psPartition->nSCSILevel = psInode->nSCSILevel;
		psPartition->bRemovable = psInode->bRemovable;
		psPartition->pDataBuffer = psInode->pDataBuffer;

		memcpy( &psPartition->zVendor, &psInode->zVendor, 8 );
		memcpy( &psPartition->zModel, &psInode->zModel, 16 );
		memcpy( &psPartition->zRev, &psInode->zRev, 4 );

		psPartition->nSectorSize = psInode->nSectorSize;
		psPartition->nSectors = psInode->nSectors;

		psPartition->nStart = asPartitions[i].p_nStart;
		psPartition->nSize = asPartitions[i].p_nSize;

		psPartition->psNext = psInode->psFirstPartition;
		psInode->psFirstPartition = psPartition;

		strcpy( zNodePath, "disk/scsi/" );
		strcat( zNodePath, psInode->zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->zName );
		strcat( zNodePath, "_new" );

		psPartition->nNodeHandle = create_device_node( psInode->psHost->get_device_id(), psPartition->nDeviceHandle, zNodePath, &g_sOperations, psPartition );

	}

	/* We now have to rename nodes that might have moved around in the table and
	 * got new names. To avoid name-clashes while renaming we first give all
	 * nodes a unique temporary name before looping over again giving them their
	 * final names
	 */

	for ( psPartition = psInode->psFirstPartition; psPartition != NULL; psPartition = psPartition->psNext )
	{
		char zNodePath[64];

		strcpy( zNodePath, "disk/scsi/" );
		strcat( zNodePath, psInode->zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->zName );
		strcat( zNodePath, "_tmp" );
		rename_device_node( psPartition->nNodeHandle, zNodePath );
	}
	for ( psPartition = psInode->psFirstPartition; psPartition != NULL; psPartition = psPartition->psNext )
	{
		char zNodePath[64];

		strcpy( zNodePath, "disk/scsi/" );
		strcat( zNodePath, psInode->zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->zName );
		rename_device_node( psPartition->nNodeHandle, zNodePath );
	}

      error:
	return ( nError );
}

int scsi_get_next_id()
{
	int i;

	for ( i = 0; i < 255; i++ )
	{
		if ( g_nIDTable[i] == false )
		{
			g_nIDTable[i] = true;
			return ( i );
		}
	}
	return ( -1 );
}

SCSI_device_s *scsi_scan_device( SCSI_host_s * psHost, int nChannel, int nDevice, int nLun, int *pnSCSILevel )
{
	unsigned char nSCSIResult[256];
	char zTemp[255];
	char zNodePath[255];
	SCSI_cmd sCmd;
	int nError;
	SCSI_device_s *psDevice;
	int i;

	printk( "Scanning device %i:%i:%i...\n", nChannel, nDevice, nLun );

	/* Build SCSI_INQUIRY command */
	sCmd.psHost = psHost;
	sCmd.nChannel = nChannel;
	sCmd.nDevice = nDevice;
	sCmd.nLun = nLun;
	sCmd.nDirection = SCSI_DATA_READ;

	sCmd.nCmd[0] = SCSI_INQUIRY;
	if ( *pnSCSILevel <= SCSI_2 )
		sCmd.nCmd[1] = ( nLun << 5 ) & 0xe0;
	else
		sCmd.nCmd[1] = 0;
	sCmd.nCmd[2] = 0;
	sCmd.nCmd[3] = 0;
	sCmd.nCmd[4] = 255;
	sCmd.nCmd[5] = 0;
	sCmd.nCmdLen = scsi_get_command_size( SCSI_INQUIRY );

	sCmd.pRequestBuffer = nSCSIResult;
	sCmd.nRequestSize = 256;

	/* Send command */
	nError = psHost->queue_command( &sCmd );

	//printk( "Result: %i\n", sCmd.nResult );

	/* Check result */
	if ( nError != 0 || sCmd.nResult != 0 )
	{
		return ( NULL );
	}

	scsi_print_inquiry( nSCSIResult );

	/* Check if the device is a harddisk */
	if ( ( nSCSIResult[0] & 0x1f ) != SCSI_TYPE_DISK )
	{
		return ( NULL );
	}

	*pnSCSILevel = nSCSIResult[2] & 0x07;



	/* Create device if it is a harddisk */
	psDevice = kmalloc( sizeof( SCSI_device_s ), MEMF_KERNEL );
	memset( psDevice, 0, sizeof( SCSI_device_s ) );

	psDevice->nID = scsi_get_next_id();
	atomic_set( &psDevice->nOpenCount, 0 );
	psDevice->psHost = psHost;
	psDevice->psRawDevice = psDevice;
	psDevice->hLock = create_semaphore( "scsi_disk_lock", 1, 0 );
	psDevice->nChannel = nChannel;
	psDevice->nDevice = nDevice;
	psDevice->nLun = nLun;
	psDevice->nType = ( nSCSIResult[0] & 0x1f );
	psDevice->nSCSILevel = *pnSCSILevel;
	psDevice->bRemovable = ( 0x80 & nSCSIResult[1] ) >> 7;
	psDevice->pDataBuffer = kmalloc( 0xffff, MEMF_KERNEL );

	if ( psDevice->bRemovable )
		printk( "SCSI: Removable device\n" );

	strcpy( psDevice->zVendor, "" );
	for ( i = 8; i < 16; i++ )
	{
		if ( nSCSIResult[i] >= 0x20 && i < nSCSIResult[4] + 5 )
		{
			sprintf( zTemp, "%c", nSCSIResult[i] );
			strcat( psDevice->zVendor, zTemp );
		}
		else
			strcat( psDevice->zVendor, " " );
	}

	strcpy( psDevice->zModel, "" );
	for ( i = 16; i < 32; i++ )
	{
		if ( nSCSIResult[i] >= 0x20 && i < nSCSIResult[4] + 5 )
		{
			sprintf( zTemp, "%c", nSCSIResult[i] );
			strcat( psDevice->zModel, zTemp );
		}
		else
			strcat( psDevice->zModel, " " );
	}

	strcpy( psDevice->zRev, "" );
	for ( i = 32; i < 36; i++ )
	{
		if ( nSCSIResult[i] >= 0x20 && i < nSCSIResult[4] + 5 )
		{
			sprintf( zTemp, "%c", nSCSIResult[i] );
			strcat( psDevice->zRev, zTemp );
		}
		else
			strcat( psDevice->zRev, " " );
	}

	/* Add device to the list */
	psDevice->psNext = g_psFirstDevice;
	g_psFirstDevice = psDevice;

	/* Create device node */
	sprintf( zNodePath, "disk/scsi/hd%c/raw", 'a' + psDevice->nID );
	sprintf( zTemp, "%s %s", psDevice->zVendor, psDevice->zModel );
	sprintf( psDevice->zName, "hd%c", 'a' + psDevice->nID );

	psDevice->nDeviceHandle = register_device( "", "scsi" );
	claim_device( psHost->get_device_id(), psDevice->nDeviceHandle, zTemp, DEVICE_DRIVE );

	nError = create_device_node( psHost->get_device_id(), psDevice->nDeviceHandle, zNodePath, &g_sOperations, psDevice );
	psDevice->nNodeHandle = nError;

	/* Try to open the device to read the size information and then decode the partition table */
	if ( scsi_open( psDevice, 0, NULL ) == 0 )
	{
		scsi_close( psDevice, NULL );
		if ( nError >= 0 )
		{
			scsi_decode_partitions( psDevice );
		}
	}




	return ( psDevice );
}

void scsi_scan_host( SCSI_host_s * psHost )
{
	int nChannel;
	int nDevice;
	int nLun;
	int nSCSILevel = SCSI_2;

	/* Scan for devices */
	printk( "SCSI: Scanning for devices on host %s\n", psHost->get_name() );

	for ( nChannel = 0; nChannel <= psHost->get_max_channel(); nChannel++ )
	{
		for ( nDevice = 0; nDevice <= psHost->get_max_device(); nDevice++ )
		{
			nSCSILevel = SCSI_2;
			for ( nLun = 0; nLun < 8; nLun++ )
			{
				if ( scsi_scan_device( psHost, nChannel, nDevice, nLun, &nSCSILevel ) )
				{
				}
				else
				{
					nLun = 9;
				}
			}
		}
	}
}

void scsi_remove_host( SCSI_host_s * psHost )
{
	/* Look for all devices that belong to this host */
	SCSI_device_s *psDevice = g_psFirstDevice;
	SCSI_device_s *psPrev = g_psFirstDevice;
	SCSI_device_s *psPartition;

      restart:
	psDevice = g_psFirstDevice;
	psPrev = g_psFirstDevice;

	while ( psDevice )
	{
		if ( psDevice->psHost == psHost )
		{
			/* First remove all partitions */
			for ( psPartition = psDevice->psFirstPartition; psPartition != NULL; psPartition = psPartition->psNext )
			{
				printk( "SCSI: Removing partition %s\n", psPartition->zName );
				if ( atomic_read( &psPartition->nOpenCount ) > 0 )
				{
					printk( "SCSI: Warning: Device still opened\n" );
				}
				delete_device_node( psPartition->nNodeHandle );
			}
			/* Then the raw device */
			printk( "SCSI: Removing device %s\n", psDevice->zName );
			if ( atomic_read( &psDevice->nOpenCount ) > 0 )
			{
				printk( "SCSI: Warning: Device still opened\n" );
			}
			delete_device_node( psDevice->nNodeHandle );

			g_nIDTable[psDevice->nID] = false;

			release_device( psDevice->nDeviceHandle );
			unregister_device( psDevice->nDeviceHandle );
			kfree( psDevice->pDataBuffer );
			if ( psPrev == psDevice )
			{
				g_psFirstDevice = psDevice->psNext;
			}
			else
			{
				psPrev->psNext = psDevice->psNext;
			}
			kfree( psDevice );
			goto restart;
		}
		psPrev = psDevice;
		psDevice = psDevice->psNext;
	}
}


bool get_bool_arg( bool *pbValue, const char *pzName, const char *pzArg, int nArgLen )
{
	char zBuffer[256];

	if ( get_str_arg( zBuffer, pzName, pzArg, nArgLen ) == false )
	{
		return ( false );
	}
	if ( stricmp( zBuffer, "false" ) == 0 )
	{
		*pbValue = false;
		return ( true );
	}
	else if ( stricmp( zBuffer, "true" ) == 0 )
	{
		*pbValue = true;
		return ( true );
	}
	return ( false );
}

status_t bus_init()
{
	/* Check if the use of the bus is disabled */
	int i;
	int argc;
	const char *const *argv;
	bool bDisablePCI = false;

	get_kernel_arguments( &argc, &argv );

	for ( i = 0; i < argc; ++i )
	{
		if ( get_bool_arg( &bDisablePCI, "disable_scsi=", argv[i], strlen( argv[i] ) ) )
			if ( bDisablePCI )
			{
				printk( "SCSI bus disabled by user\n" );
				return ( -1 );
			}
	}
	/* Set default values */

	g_psFirstDevice = NULL;
	for ( i = 0; i < 255; i++ )
	{
		g_nIDTable[i] = false;
	}
	
	printk( "SCSI: Busmanager initialized\n" );
	
	return( 0 );
}


void bus_uninit()
{
}

SCSI_bus_s sBus = {
	scsi_get_command_size,
	scsi_scan_host,
	scsi_remove_host
};

void *bus_get_hooks( int nVersion )
{
	if ( nVersion != SCSI_BUS_VERSION )
		return ( NULL );
	return ( ( void * )&sBus );
}






