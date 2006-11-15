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
#include <atheos/bootmodules.h>
#include <posix/errno.h>
#include <macros.h>

#include <scsi_common.h>
#include <scsi_generic.h>

//#undef DEBUG_LIMIT
//#define DEBUG_LIMIT   KERN_DEBUG_LOW

static int scsi_disk_decode_partitions( SCSI_device_s * psInode );

static size_t scsi_disk_read_partition_data( void *pCookie, off_t nOffset, void *pBuffer, size_t nSize )
{
	return scsi_generic_read( pCookie, NULL, nOffset, pBuffer, nSize );
}

static status_t scsi_disk_ioctl( void *pNode, void *pCookie, uint32 nCommand, void *pArgs, bool bFromKernel )
{
	int nError = 0;
	SCSI_device_s *psDevice = pNode;

	switch ( nCommand )
	{
		case IOCTL_GET_DEVICE_GEOMETRY:
		{
			device_geometry sGeo;

			sGeo.sector_count = psDevice->nSize / psDevice->nSectorSize;
			sGeo.cylinder_count = 1;
			sGeo.sectors_per_track = 1;
			sGeo.head_count = 1;
			sGeo.bytes_per_sector = psDevice->nSectorSize;
			sGeo.read_only = false;
			sGeo.removable = psDevice->bRemovable;

			if( bFromKernel )
				memcpy( pArgs, &sGeo, sizeof( sGeo ) );
			else
				nError = memcpy_to_user( pArgs, &sGeo, sizeof( sGeo ) );

			break;
		}

		case IOCTL_REREAD_PTABLE:
			nError = scsi_disk_decode_partitions( psDevice );
			break;

		default:
		{
			kerndbg( KERN_WARNING, "scsi_disk_ioctl() unknown command %ld\n", nCommand );
			nError = -ENOSYS;
			break;
		}
	}
	return nError;
}

DeviceOperations_s g_sDiskOperations = {
	scsi_generic_open,
	scsi_generic_close,
	scsi_disk_ioctl,
	scsi_generic_read,
	scsi_generic_write,
	scsi_generic_readv,
	scsi_generic_writev,
	NULL,			// dop_add_select_req
	NULL			// dop_rem_select_req
};

static int scsi_disk_decode_partitions( SCSI_device_s * psInode )
{
	int nNumPartitions;
	device_geometry sDiskGeom;
	Partition_s asPartitions[16];
	SCSI_device_s *psPartition;
	SCSI_device_s **ppsTmp;
	int nError;
	int i;

	sDiskGeom.sector_count = psInode->nSectors;
	sDiskGeom.cylinder_count = 1;
	sDiskGeom.sectors_per_track = 1;
	sDiskGeom.head_count = 1;
	sDiskGeom.bytes_per_sector = psInode->nSectorSize;
	sDiskGeom.read_only = false;
	sDiskGeom.removable = psInode->bRemovable;

	printk( "Decode partition table for %s\n", psInode->zName );

	nNumPartitions = decode_disk_partitions( &sDiskGeom, asPartitions, 16, psInode, scsi_disk_read_partition_data );

	if( nNumPartitions < 0 )
	{
		printk( "   Invalid partition table\n" );
		return nNumPartitions;
	}
	for( i = 0; i < nNumPartitions; ++i )
	{
		if( asPartitions[i].p_nType != 0 && asPartitions[i].p_nSize != 0 )
		{
			printk( "   Partition %d : %10Ld -> %10Ld %02x (%Ld)\n", i, asPartitions[i].p_nStart, asPartitions[i].p_nStart + asPartitions[i].p_nSize - 1LL, asPartitions[i].p_nType, asPartitions[i].p_nSize );
		}
	}
	nError = 0;

	for( psPartition = psInode->psFirstPartition; psPartition != NULL; psPartition = psPartition->psNext )
	{
		bool bFound = false;

		for( i = 0; i < nNumPartitions; ++i )
		{
			if( asPartitions[i].p_nStart == psPartition->nStart && asPartitions[i].p_nSize == psPartition->nSize )
			{
				bFound = true;
				break;
			}
		}
		if( bFound == false && atomic_read( &psPartition->nOpenCount ) > 0 )
		{
			printk( "scsi_disk_decode_partitions() Error: Open partition %s on %s has changed\n", psPartition->zName, psInode->zName );
			nError = -EBUSY;
			goto error;
		}
	}

	// Remove deleted partitions from /dev/disk/scsi/*/*
	for( ppsTmp = &psInode->psFirstPartition; *ppsTmp != NULL; )
	{
		bool bFound = false;

		psPartition = *ppsTmp;
		for( i = 0; i < nNumPartitions; ++i )
		{
			if( asPartitions[i].p_nStart == psPartition->nStart && asPartitions[i].p_nSize == psPartition->nSize )
			{
				asPartitions[i].p_nSize = 0;
				psPartition->nPartitionType = asPartitions[i].p_nType;
				sprintf( psPartition->zName, "%d", i );
				bFound = true;
				break;
			}
		}
		if( bFound == false )
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
	for( i = 0; i < nNumPartitions; ++i )
	{
		char zNodePath[64];

		if( asPartitions[i].p_nType == 0 || asPartitions[i].p_nSize == 0 )
		{
			continue;
		}

		psPartition = kmalloc( sizeof( SCSI_device_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
		memset( psPartition, 0, sizeof( SCSI_device_s ) );

		if( psPartition == NULL )
		{
			printk( "Error: scsi_disk_decode_partitions() no memory for partition inode\n" );
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

		psPartition->nNodeHandle = create_device_node( psInode->psHost->get_device_id(), psPartition->nDeviceHandle, zNodePath, &g_sDiskOperations, psPartition );

	}

	/* We now have to rename nodes that might have moved around in the table and
	 * got new names. To avoid name-clashes while renaming we first give all
	 * nodes a unique temporary name before looping over again giving them their
	 * final names
	 */

	for( psPartition = psInode->psFirstPartition; psPartition != NULL; psPartition = psPartition->psNext )
	{
		char zNodePath[64];

		strcpy( zNodePath, "disk/scsi/" );
		strcat( zNodePath, psInode->zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->zName );
		strcat( zNodePath, "_tmp" );
		rename_device_node( psPartition->nNodeHandle, zNodePath );
	}
	for( psPartition = psInode->psFirstPartition; psPartition != NULL; psPartition = psPartition->psNext )
	{
		char zNodePath[64];

		strcpy( zNodePath, "disk/scsi/" );
		strcat( zNodePath, psInode->zName );
		strcat( zNodePath, "/" );
		strcat( zNodePath, psPartition->zName );
		rename_device_node( psPartition->nNodeHandle, zNodePath );
	}

error:
	return nError;
}

SCSI_device_s * scsi_add_disk( SCSI_host_s * psHost, int nChannel, int nDevice, int nLun, unsigned char nSCSIResult[] )
{
	SCSI_device_s *psDevice = NULL;
	char zTemp[255], zNodePath[255];
	int i, nError;

	/* Create device for harddisk */
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
	psDevice->nSCSILevel = ( nSCSIResult[2] & 0x07 );
	psDevice->bRemovable = ( 0x80 & nSCSIResult[1] ) >> 7;
	psDevice->pDataBuffer = kmalloc( 0xffff, MEMF_KERNEL );

	if( psDevice->bRemovable )
		printk( "SCSI: Removable device\n" );

	strcpy( psDevice->zVendor, "" );
	for( i = 8; i < 16; i++ )
	{
		if( nSCSIResult[i] >= 0x20 && i < nSCSIResult[4] + 5 )
		{
			sprintf( zTemp, "%c", nSCSIResult[i] );
			strcat( psDevice->zVendor, zTemp );
		}
		else
			strcat( psDevice->zVendor, " " );
	}

	strcpy( psDevice->zModel, "" );
	for( i = 16; i < 32; i++ )
	{
		if( nSCSIResult[i] >= 0x20 && i < nSCSIResult[4] + 5 )
		{
			sprintf( zTemp, "%c", nSCSIResult[i] );
			strcat( psDevice->zModel, zTemp );
		}
		else
			strcat( psDevice->zModel, " " );
	}

	strcpy( psDevice->zRev, "" );
	for( i = 32; i < 36; i++ )
	{
		if( nSCSIResult[i] >= 0x20 && i < nSCSIResult[4] + 5 )
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

	nError = create_device_node( psHost->get_device_id(), psDevice->nDeviceHandle, zNodePath, &g_sDiskOperations, psDevice );
	psDevice->nNodeHandle = nError;

	/* Try to open the device to read the size information and then decode the partition table */
	if( scsi_generic_open( psDevice, 0, NULL ) == 0 )
	{
		scsi_generic_close( psDevice, NULL );
		if( nError >= 0 )
		{
			scsi_disk_decode_partitions( psDevice );
		}
	}

	return psDevice;
}
