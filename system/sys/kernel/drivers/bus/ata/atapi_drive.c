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

#include <atheos/time.h>
#include "ata.h"
#include "ata_private.h"
#include "atapi.h"

int atapi_drive_open( void* pNode, uint32 nFlags, void **ppCookie )
{
	ATAPI_device_s* psDev = pNode;
	int nError = 0;

	/* Discover if the drive is ready */
	nError = atapi_test_ready( psDev );
	if( nError < 0 )
	{
		kerndbg( KERN_DEBUG, "An error occured waiting for the device, return code 0x%02x\n", nError );
		return( nError );
	}

	/* No harm in allowing the drive firmware a little time */
	udelay(200);

	nError = atapi_read_capacity( psDev, NULL );
	if( nError < 0 )
	{
		kerndbg( KERN_DEBUG, "An error occured reading from the device, return code 0x%02x\n", nError );
		return nError;
	}

	kerndbg( KERN_DEBUG, "Capacity %Ld\n", psDev->nSize );
	atomic_inc( &psDev->nOpenCount );

	return( 0 );
}

int atapi_drive_close( void* pNode, void* pCookie )
{
	ATAPI_device_s* psDev = pNode;
	atomic_dec( &psDev->nOpenCount );

	return( 0 );
}


int atapi_drive_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen )
{
	ATAPI_device_s* psDev = pNode;
	int nBytesLeft;
	int nError;

	kerndbg( KERN_DEBUG, "atapi_drive_read() enter\n");

	if ( nLen & 1 )
	{
		kerndbg( KERN_FATAL, "Error: atapi_drive_read() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}
	
	if ( nPos >= psDev->nSize )
	{
		kerndbg( KERN_FATAL, "Warning: atapi_drive_read() Request outside partiton %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psDev->nSize )
	{
		kerndbg( KERN_WARNING, "Warning: atapi_drive_read() Request truncated from %d to %Ld\n", nLen, (psDev->nSize - nPos) );
		nLen = psDev->nSize - nPos;
	}
	
	nBytesLeft = nLen;
	nPos += psDev->nStart;
	
	LOCK( psDev->hLock );

	/* The iso9660 fs requires this workaround... */
	if( nLen < psDev->nSectorSize )
		nBytesLeft = psDev->nSectorSize;
	
	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( 16 * PAGE_SIZE, nBytesLeft );
		uint64 nBlock = nPos / psDev->nSectorSize;

		nError = atapi_do_read( psDev, nBlock, nCurSize );

		if( nError < 0 )
		{
			kerndbg( KERN_WARNING, "A fatal error occured reading data from device, aborting.\n");
			goto error;
		}

		if( nLen < psDev->nSectorSize )
			memcpy( pBuf, psDev->psPort->pDataBuf, nLen );
		else
			memcpy( pBuf, psDev->psPort->pDataBuf, nCurSize );
			
		pBuf = ((uint8*)pBuf) + nCurSize;
		nPos += nCurSize;
		nBytesLeft -= nCurSize;
	}
	nError = nLen;
	
	UNLOCK( psDev->hLock );

	kerndbg( KERN_DEBUG, "atapi_drive_read() completed\n");

error:
	return( nError );
}


int atapi_drive_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount )
{
	ATAPI_device_s* psDev = pNode;
	int nBytesLeft;
	int nLen = 0;
	int i;
	int nError;
	int nCurVec;
	char* pCurVecPtr;
	int   nCurVecLen;
	
	kerndbg( KERN_DEBUG, "atapi_drive_readv() enter\n");

	for ( i = 0 ; i < nCount ; ++i )
		nLen += psVector[i].iov_len;


	if ( nLen <= 0 )
		return( 0 );

	
	if ( nLen & ( psDev->nSectorSize - 1 ) )
	{
		kerndbg( KERN_FATAL, "Error: atapi_drive_readv() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psDev->nSize )
	{
		kerndbg( KERN_FATAL, "Warning: atapi_drive_readv() Request outside partiton : %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psDev->nSize )
	{
		kerndbg( KERN_FATAL, "Warning: atapi_drive_readv() Request truncated from %d to %Ld\n", nLen, (psDev->nSize - nPos) );
		nLen = psDev->nSize - nPos;
	}

	nBytesLeft = nLen;
	nPos += psDev->nStart;

	pCurVecPtr = psVector[0].iov_base;
	nCurVecLen = psVector[0].iov_len;
	nCurVec = 1;
	
	LOCK( psDev->hLock );

	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( 16 * PAGE_SIZE, nBytesLeft );
		char* pSrcPtr = psDev->psPort->pDataBuf;
		uint64 nBlock = nPos / psDev->nSectorSize;
	
		if ( nPos < 0 )
		{
			kerndbg( KERN_PANIC, "atapi_drive_readv() wierd pos = %Ld\n", nPos );
			kassertw(0);
		}

		nError = atapi_do_read( psDev, nBlock, nCurSize );
		
		if( nError < 0 )
		{
			kerndbg( KERN_WARNING, "A fatal error occured reading data from device, aborting.\n");
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
	nError = nLen;
	
	UNLOCK( psDev->hLock );

	kerndbg( KERN_DEBUG, "atapi_drive_readv() completed\n");

error:
    return( nError );
}

status_t atapi_drive_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	ATAPI_device_s* psDev = pNode;
	int nError = 0;

	switch( nCommand )
	{
		case IOCTL_GET_DEVICE_GEOMETRY:
		{
			device_geometry sGeo;

			sGeo.sector_count		= psDev->nSize / psDev->nSectorSize;
			sGeo.cylinder_count		= 1;
			sGeo.sectors_per_track	= 1;
			sGeo.head_count			= 1;
			sGeo.bytes_per_sector	= psDev->nSectorSize;
			sGeo.read_only			= true;
			sGeo.removable			= psDev->bRemovable;

			if ( bFromKernel )
				memcpy( pArgs, &sGeo, sizeof(sGeo) );
			else
				nError = memcpy_to_user( pArgs, &sGeo, sizeof(sGeo) );

			break;
		}

		case CD_EJECT:
		{
			nError = atapi_eject( psDev );
			break;
		}

		case CD_READ_TOC:
		{
			cdrom_toc_s toc;

			nError = atapi_read_toc( psDev, &toc );

			if( nError == 0 )
			{
				if ( bFromKernel )
					memcpy( pArgs, &toc, sizeof(toc) );
				else
					nError = memcpy_to_user( pArgs, &toc, sizeof(toc) );
			}

			memcpy( &psDev->sToc, &toc, sizeof(toc) );
			psDev->bMediaChanged = false;
			psDev->bTocValid = true;
			break;
		}

		case CD_READ_TOC_ENTRY:
		{
			nError = -EINVAL;

			if( pArgs != NULL && psDev->bTocValid )
			{
				cdrom_toc_entry_s *psEntry = (cdrom_toc_entry_s*)pArgs;
				uint8 nTrack = psEntry->track;

				if( nTrack > 0 && nTrack < psDev->sToc.hdr.last_track )
				{
					if ( bFromKernel )
						memcpy( pArgs, &psDev->sToc.ent[nTrack], sizeof(cdrom_toc_entry_s) );
					else
						nError = memcpy_to_user( pArgs, &psDev->sToc.ent[nTrack], sizeof(cdrom_toc_entry_s) );

					nError = 0;
				}
			}

			break;
		}

		case CD_PLAY_MSF:
		{
			cdrom_audio_track_s *track = (cdrom_audio_track_s*)pArgs;

			nError = atapi_play_audio( psDev, msf_to_lba(track->msf_start.minute,
															   track->msf_start.second,
															   track->msf_start.frame),
													msf_to_lba(track->msf_stop.minute,
															   track->msf_stop.second,
															   track->msf_stop.frame) );
			break;
		}

		case CD_PLAY_LBA:
		{
			cdrom_audio_track_s *track = (cdrom_audio_track_s*)pArgs;

			nError = atapi_play_audio( psDev, track->lba_start, track->lba_stop );
			break;
		}

		case CD_PAUSE:
		{
			nError = atapi_pause_resume_audio( psDev, true );
			break;
		}

		case CD_RESUME:
		{
			nError = atapi_pause_resume_audio( psDev, false );
			break;
		}

		case CD_STOP:
		{
			nError = atapi_stop_audio( psDev );
			break;
		}

		case CD_GET_TIME:
		{
			nError = atapi_get_playback_time( psDev, (cdrom_msf_s*)pArgs );
			break;
		}

		case CD_READ_CDDA:
		{
			struct cdda_block *psBlock = (struct cdda_block*)pArgs;

			if( psBlock != NULL )
			{
				nError = atapi_do_cdda_read( psDev, psBlock->nBlock, psBlock->nSize );
				if( nError == 0 )
					memcpy_to_user( psBlock->pBuf, psDev->psPort->pDataBuf, psBlock->nSize );
			}
			else
				nError = -EINVAL;

			break;
		}

		case CD_PACKET_COMMAND:
		{
			nError = atapi_do_packet_command( psDev, (cdrom_packet_cmd_s*)pArgs );
			break;
		}

		default:
		{
			kerndbg( KERN_WARNING, "atapi_drive_ioctl() unknown command %ld\n", nCommand );
			nError = -ENOSYS;
			break;
		}
	}

	return( nError );
}

static DeviceOperations_s g_sOperations = {
	atapi_drive_open,
	atapi_drive_close,
	atapi_drive_ioctl,
	atapi_drive_read,
	NULL,	/* dop_write */
	atapi_drive_readv,
	NULL,	/* dop_writev */
	NULL,	/* dop_add_select_req */
	NULL	/* dop_rem_select_req */
};

/* Add one atapi drive */
void atapi_drive_add( ATA_port_s* psPort )
{
	char zNodePath[PATH_MAX];
	char zName[10];
	/* Create device */
	ATAPI_device_s* psDev = kmalloc( sizeof( ATAPI_device_s ), MEMF_KERNEL | MEMF_CLEAR );
	
	sprintf( zName, "cd%c", 'a' + psPort->nID );
	
	strcpy( psDev->zName, zName );
	psDev->psPort = psPort;
	psDev->hLock = create_semaphore( "atapi_drive_lock", 1, SEM_RECURSIVE );
	psDev->nStart = 0;
	psDev->nSize = 0;
	psDev->nSectorSize = 0;
	psDev->bRemovable = psPort->sID.configuration & 0x80;
	psDev->bMediaChanged = false;
	psDev->bTocValid = false;
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
}

