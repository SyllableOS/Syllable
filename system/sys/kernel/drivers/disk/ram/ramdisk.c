/*  RAM disk device driver
 *  Copyright (C) 2004 Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#define __ENABLE_DEBUG__
#undef DEBUG_LIMIT
#define DEBUG_LIMIT	KERN_DEBUG_LOW

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/device.h>
#include <atheos/areas.h>
#include <atheos/semaphore.h>
#include <errno.h>
#include <macros.h>

struct RamDisk_s{
	int hDeviceId;
	int hNodeHandle;

	uint32 nBlockCount;	
	uint32 nBlockSize;
	
	uint8 *pDiskMem;

	int nOpenCount;
	sem_id hLock;
};

#define MAX_RAMDISKS		64

static struct RamDisk_s *g_psDisks[MAX_RAMDISKS];
static int g_nDiskCount = 0;

static int ramdisk_open( void* pNode, uint32 nFlags, void** ppCookie )
{
	struct RamDisk_s *psDisk = (struct RamDisk_s*)pNode;

	kerndbg( KERN_DEBUG, "%s on disk %p\n", __FUNCTION__, psDisk );

	if( 0 != psDisk->nOpenCount )
		return -1;
	psDisk->nOpenCount++;

	return( 0 );
}

static int ramdisk_close( void* pNode, void* pCookie )
{
	struct RamDisk_s *psDisk = (struct RamDisk_s*)pNode;

	kerndbg( KERN_DEBUG, "%s on disk %p\n", __FUNCTION__, psDisk );

	if( 1 != psDisk->nOpenCount )
		return -1;
	psDisk->nOpenCount--;

    return( 0 );
}

static status_t ramdisk_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct RamDisk_s *psDisk = (struct RamDisk_s*)pNode;
	int nError = 0;

	kerndbg( KERN_DEBUG, "%s on disk %p\n", __FUNCTION__, psDisk );

	switch( nCommand )
	{
		case IOCTL_GET_DEVICE_GEOMETRY:
		{
			device_geometry sGeo;

			sGeo.sector_count		= psDisk->nBlockCount;
			sGeo.cylinder_count		= 1;
			sGeo.sectors_per_track	= 1;
			sGeo.head_count			= 1;
			sGeo.bytes_per_sector	= psDisk->nBlockSize;
			sGeo.read_only			= false;
			sGeo.removable			= false;

			if ( bFromKernel )
				memcpy( pArgs, &sGeo, sizeof(sGeo) );
			else
				nError = memcpy_to_user( pArgs, &sGeo, sizeof(sGeo) );

			break;
		}
		
		default:
			nError = -ENOSYS;
	}

	return( nError );
}

static int ramdisk_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLength )
{
	struct RamDisk_s *psDisk = (struct RamDisk_s*)pNode;

	if( nPos + nLength > psDisk->nBlockCount * 1024 )
	{
		kerndbg( KERN_DEBUG, "%s: request for %i bytes at position %Li outside of disk\n", __FUNCTION__, nLength, nPos );
		return 0;
	}
	
	kerndbg( KERN_DEBUG, "%s: read %i bytes from position %Li on disk %p\n", __FUNCTION__, nLength, nPos, psDisk );
	
	LOCK( psDisk->hLock );
	memcpy( (uint8*)pBuf, psDisk->pDiskMem + nPos, nLength );
	UNLOCK( psDisk->hLock );

	return( nLength );
}

static int ramdisk_write( void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLength )
{
	struct RamDisk_s *psDisk = (struct RamDisk_s*)pNode;

	if( nPos + nLength > psDisk->nBlockCount * 1024 )
	{
		kerndbg( KERN_DEBUG, "%s: request for %i bytes at position %Li outside of disk\n",__FUNCTION__, nLength, nPos );
		return 0;
	}
	
	kerndbg( KERN_DEBUG, "%s: write %i bytes to position %Li on disk %p\n", __FUNCTION__, nLength, nPos, psDisk );
	
	LOCK( psDisk->hLock );
	memcpy( psDisk->pDiskMem + nPos, (uint8*)pBuf, nLength );
	UNLOCK( psDisk->hLock );

	return( nLength );
}

static int ramdisk_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector,
		size_t nCount )
{
	struct RamDisk_s *psDisk = (struct RamDisk_s*)pNode;
	int nLen = 0;
	int i;
	off_t nCurPos;

	for ( i = 0 ; i < nCount ; ++i )
		nLen += psVector[i].iov_len;

	if ( nLen <= 0 )
		{
		kerndbg( KERN_DEBUG, "%s: bad total length %d on disk %p\n", __FUNCTION__, nLen, psDisk );
		return ( 0 );
		}

	if( nPos + nLen > psDisk->nBlockCount * psDisk->nBlockSize )
	{
		kerndbg( KERN_DEBUG, "%s: request for %i bytes at position %Li outside of disk\n", __FUNCTION__, nLen, nPos );
		return 0;
	}

	kerndbg( KERN_DEBUG, "%s: read %i bytes from position %Li on disk %p\n", __FUNCTION__, nLen, nPos, psDisk );

	nCurPos = nPos;

	LOCK( psDisk->hLock );
	for ( i = 0 ; i < nCount ; ++i )
		{
		memcpy( (uint8*)psVector[i].iov_base, psDisk->pDiskMem + nCurPos, psVector[i].iov_len );
		nCurPos += psVector[i].iov_len;
		}
	UNLOCK( psDisk->hLock );

	return( nLen );
}

DeviceOperations_s ramdisk_ops = 
{
	ramdisk_open,
	ramdisk_close,
	ramdisk_ioctl,
	ramdisk_read,
	ramdisk_write,
	ramdisk_readv,
	NULL,
	NULL,
	NULL
};

status_t device_init( int nDeviceId )
{
	struct RamDisk_s *psDisk;
	char zDeviceName[MAX_DEVICE_NAME_LENGTH];

	kerndbg( KERN_DEBUG, "Initialising RAM disk driver\n" );

	/* FIXME: The number & size of each RAM disk should be read from a config file */	
	psDisk = kmalloc( sizeof( struct RamDisk_s ), MEMF_KERNEL | MEMF_OKTOFAIL );
	if( NULL == psDisk )
		return -1;

	psDisk->hDeviceId = nDeviceId;
	psDisk->nBlockSize = 1024;
	psDisk->nBlockCount = 8192;	/* 8Mb RAM disk */

	psDisk->pDiskMem = kmalloc( psDisk->nBlockCount * 1024, MEMF_KERNEL | MEMF_OKTOFAIL );
	if( NULL == psDisk->pDiskMem )
	{
		kfree( psDisk );
		return -1;
	}
	psDisk->nOpenCount = 0;
	psDisk->hLock = create_semaphore( "ramdisk_lock", 1, 0 );

	sprintf( zDeviceName, "disk/ram/%i", g_nDiskCount );
	g_psDisks[g_nDiskCount++] = psDisk;

	psDisk->hNodeHandle = create_device_node( psDisk->hDeviceId, -1, zDeviceName, &ramdisk_ops, psDisk );
	if( psDisk->hNodeHandle < 0 )
	{
		kfree( psDisk->pDiskMem );
		delete_semaphore( psDisk->hLock );
		kfree( psDisk );
		return -1;
	}	

	return psDisk->hNodeHandle;
}

status_t device_uninit( int nDeviceId )
{
	int nCurrent, nError = -1;
	
	kerndbg( KERN_DEBUG, "Un-initialising RAM disk driver\n" );

	for( nCurrent = 0; nCurrent < g_nDiskCount; nCurrent++ )
	{
		if( NULL == g_psDisks[nCurrent] )
			break;
			
		if( nDeviceId == g_psDisks[nCurrent]->hDeviceId )
		{
			nError = delete_device_node( g_psDisks[nCurrent]->hNodeHandle );
			kfree( g_psDisks[nCurrent]->pDiskMem );
			delete_semaphore( g_psDisks[nCurrent]->hLock );
			kfree( g_psDisks[nCurrent] );
			
			nError = 0;
		}
	}
		
	return( nError );
}
