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
#include <atheos/udelay.h>
#include <posix/errno.h>
#include <macros.h>

#include <scsi_common.h>
#include <scsi_generic.h>

//#undef DEBUG_LIMIT
//#define DEBUG_LIMIT   KERN_DEBUG_LOW

status_t scsi_cdrom_read_toc_entry( SCSI_device_s * psDevice, int trackno, int msf_flag, int format, char *buf, int buflen )
{
	SCSI_cmd_s sCmd;
	int nError;

retry:
	/* Create and queue command */
	scsi_init_cmd( &sCmd, psDevice );

	sCmd.nDirection = SCSI_DATA_READ;
	sCmd.pRequestBuffer = psDevice->pDataBuffer;
	sCmd.nRequestSize = buflen;

	sCmd.nCmd[0] = SCSI_READ_TOC;
	if( msf_flag )
		sCmd.nCmd[1] = 2;

	sCmd.nCmd[6] = trackno;
	sCmd.nCmd[7] = ( buflen >> 8 );
	sCmd.nCmd[8] = ( buflen & 0xff );
	sCmd.nCmd[9] = ( format << 6 );

	sCmd.nCmdLen = scsi_get_command_size( SCSI_READ_TOC );

	LOCK( psDevice->hLock );

	/* Send command */
	nError = psDevice->psHost->queue_command( &sCmd );

	if( sCmd.s.sSense.sense_key != SCSI_NO_SENSE || sCmd.nResult != 0 )
	{
		SCSI_sense_s sSense;

		nError = scsi_request_sense( psDevice, &sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from SCSI device, aborting.\n" );
			UNLOCK( psDevice->hLock );
			return -EIO;
		}

		nError = scsi_check_sense( psDevice, &sSense, false );
		UNLOCK( psDevice->hLock );

		switch ( nError )
		{
			case SENSE_OK:
				break;

			case SENSE_RETRY:
				goto retry;

			case SENSE_FATAL:
			{
				kerndbg( KERN_PANIC, "SCSI device reporting fatal error, aborting.\n" );
				return -EIO;
			}
		}
	}
	else
		UNLOCK( psDevice->hLock );

	memcpy( buf, psDevice->pDataBuffer, buflen );

	return 0;
}

status_t scsi_cdrom_read_toc( SCSI_device_s * psDevice, cdrom_toc_s * toc )
{
	int stat, ntracks, i;

	struct
	{
		cdrom_toc_header_s hdr;
		cdrom_toc_entry_s ent;
	} ms_tmp;

	if( toc == NULL )
	{
		/* Try to allocate space. */
		toc = ( cdrom_toc_s * ) kmalloc( sizeof( cdrom_toc_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
		if( toc == NULL )
			return -ENOMEM;
	}

	/* First read just the header, so we know how long the TOC is. */
	stat = scsi_cdrom_read_toc_entry( psDevice, 0, 1, 0, ( char * )&toc->hdr, sizeof( cdrom_toc_header_s ) );
	if( stat < 0 )
		return stat;

	ntracks = toc->hdr.last_track - toc->hdr.first_track + 1;

	if( ntracks <= 0 )
		return -EIO;
	if( ntracks > MAX_TRACKS )
		ntracks = MAX_TRACKS;

	/* Now read the whole schmeer. */
	stat = scsi_cdrom_read_toc_entry( psDevice, toc->hdr.first_track, 1, 0, ( char * )&toc->hdr, sizeof( cdrom_toc_header_s ) + ( ntracks + 1 ) * sizeof( cdrom_toc_entry_s ) );

	if( stat < 0 && toc->hdr.first_track > 1 )
	{
		/* Cds with CDI tracks only don't have any TOC entries,
		   despite of this the returned values are
		   first_track == last_track = number of CDI tracks + 1,
		   so that this case is indistinguishable from the same
		   layout plus an additional audio track.
		   If we get an error for the regular case, we assume
		   a CDI without additional audio tracks. In this case
		   the readable TOC is empty (CDI tracks are not included)
		   and only holds the Leadout entry. Heiko Eißfeldt */

		ntracks = 0;
		stat = scsi_cdrom_read_toc_entry( psDevice, CDROM_LEADOUT, 1, 0, ( char * )&toc->hdr, sizeof( cdrom_toc_header_s ) + ( ntracks + 1 ) * sizeof( cdrom_toc_entry_s ) );

		if( stat < 0 )
			return stat;

		toc->hdr.first_track = CDROM_LEADOUT;
		toc->hdr.last_track = CDROM_LEADOUT;
	}

	if( stat < 0 )
		return stat;

	toc->hdr.toc_length = ( toc->hdr.toc_length << 8 | toc->hdr.toc_length >> 8 );

	for( i = 0; i <= ntracks; i++ )
		toc->ent[i].addr.lba = msf_to_lba( toc->ent[i].addr.msf.minute, toc->ent[i].addr.msf.second, toc->ent[i].addr.msf.frame );

	/* Read the multisession information. */
	if( toc->hdr.first_track != CDROM_LEADOUT )
	{
		/* Read the multisession information. */
		stat = scsi_cdrom_read_toc_entry( psDevice, 0, 1, 1, ( char * )&ms_tmp, sizeof( ms_tmp ) );
		if( stat < 0 )
			return stat;
	}
	else
	{
		ms_tmp.ent.addr.msf.minute = 0;
		ms_tmp.ent.addr.msf.second = 2;
		ms_tmp.ent.addr.msf.frame = 0;
		ms_tmp.hdr.first_track = ms_tmp.hdr.last_track = CDROM_LEADOUT;
	}

	toc->last_session_lba = msf_to_lba( ms_tmp.ent.addr.msf.minute, ms_tmp.ent.addr.msf.second, ms_tmp.ent.addr.msf.frame );
	toc->xa_flag = ( ms_tmp.hdr.first_track != ms_tmp.hdr.last_track );

	/* Now try to get the total cdrom capacity. */

	stat = scsi_read_capacity( psDevice, &toc->capacity );
	if( stat < 0 )
		toc->capacity = 0x1fffff;

	return 0;
}

static status_t scsi_cdrom_cdda_read( SCSI_device_s * psDevice, uint64 nBlock, int nSize )
{
	SCSI_cmd_s sCmd;
	uint32 nBlockCount = nSize / psDevice->nSectorSize;
	int nError;

	kerndbg( KERN_DEBUG_LOW, "Reading %i CD-DA blocks from block %Li\n", ( int )nBlockCount, nBlock );

retry:
	nError = 0;

	/* Prepare read command */
	scsi_init_cmd( &sCmd, psDevice );

	sCmd.nDirection = SCSI_DATA_READ;
	sCmd.pRequestBuffer = psDevice->pDataBuffer;
	sCmd.nRequestSize = nSize;

	sCmd.nCmd[0] = SCSI_READ_CD;
	sCmd.nCmd[1] = 0x04;	/* Expect CD-DA only, DAP cleared */
	sCmd.nCmd[2] = ( nBlock >> 24 ) & 0xff;
	sCmd.nCmd[3] = ( nBlock >> 16 ) & 0xff;
	sCmd.nCmd[4] = ( nBlock >> 8 ) & 0xff;
	sCmd.nCmd[5] = ( nBlock >> 0 ) & 0xff;
	sCmd.nCmd[6] = ( nBlockCount >> 16 ) & 0xff;
	sCmd.nCmd[7] = ( nBlockCount >> 8 ) & 0xff;
	sCmd.nCmd[8] = ( nBlockCount >> 0 ) & 0xff;
	sCmd.nCmd[9] = 0xf8;	/* Set all main-channel selection bits */

	sCmd.nCmdLen = scsi_get_command_size( SCSI_READ_CD );

	LOCK( psDevice->hLock );

	/* Send command */
	nError = psDevice->psHost->queue_command( &sCmd );

	if( sCmd.s.sSense.sense_key != SCSI_NO_SENSE || sCmd.nResult < 0 )
	{
		SCSI_sense_s sSense;

		nError = scsi_request_sense( psDevice, &sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from SCSI device, aborting.\n" );
			UNLOCK( psDevice->hLock );
			goto error;
		}

		nError = scsi_check_sense( psDevice, &sSense, false );
		UNLOCK( psDevice->hLock );

		switch ( nError )
		{
			case SENSE_OK:
				break;

			case SENSE_RETRY:
				goto retry;

			case SENSE_FATAL:
			{
				kerndbg( KERN_PANIC, "SCSI device reporting fatal error, aborting.\n" );
				nError = EIO;
				break;
			}
		}
	}
	else
		UNLOCK( psDevice->hLock );

error:
	return nError;
}

static status_t scsi_cdrom_packet_command( SCSI_device_s * psDevice, cdrom_packet_cmd_s * psRawCmd )
{
	SCSI_cmd_s sCmd;
	int nError;

retry:
	nError = 0;

	/* Prepare SCSI command */
	scsi_init_cmd( &sCmd, psDevice );

	sCmd.nDirection = psRawCmd->nDirection;
	sCmd.pRequestBuffer = psDevice->pDataBuffer;
	sCmd.nRequestSize = psRawCmd->nDataLength;

	memcpy_from_user( sCmd.nCmd, psRawCmd->nCommand, psRawCmd->nCommandLength );
	kerndbg( KERN_DEBUG_LOW, "Raw packet command: 0x%02x\n", sCmd.nCmd[0] );

	sCmd.nCmdLen = scsi_get_command_size( sCmd.nCmd[0] );

	if( psRawCmd->nDataLength > 0 )
	{
		if( psRawCmd->nDirection == WRITE )
		{
			kerndbg( KERN_DEBUG, "Transfer %i bytes TO device\n", psRawCmd->nDataLength );
			memcpy_from_user( psDevice->pDataBuffer, psRawCmd->pnData, psRawCmd->nDataLength );
		}
		else
			kerndbg( KERN_DEBUG, "Transfer %i bytes FROM device\n", psRawCmd->nDataLength );
	}
	else
		kerndbg( KERN_DEBUG, "No data to transfer\n" );

	LOCK( psDevice->hLock );

	/* Send command */
	nError = psDevice->psHost->queue_command( &sCmd );

	psRawCmd->nError = sCmd.nResult;
	psRawCmd->nSense = SENSE_OK;

	if( sCmd.s.sSense.sense_key != SCSI_NO_SENSE || sCmd.nResult < 0 )
	{
		SCSI_sense_s sSense;

		nError = scsi_request_sense( psDevice, &sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from SCSI device, aborting.\n" );
			goto error;
		}

		memcpy_to_user( &psRawCmd->pnSense, &sSense, psRawCmd->nSenseLength );

		psRawCmd->nSense = nError = scsi_check_sense( psDevice, &sSense, true );
		switch ( psRawCmd->nSense )
		{
			case SENSE_OK:
				break;

			case SENSE_RETRY:
			{
				UNLOCK( psDevice->hLock );
				goto retry;
			}

			case SENSE_FATAL:
			{
				kerndbg( KERN_DEBUG, "SCSI device reporting fatal error, aborting.\n" );

				/* Dump cmd bytes */
				kerndbg( KERN_DEBUG_LOW, "cmd data: 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n", sCmd.nCmd[0], sCmd.nCmd[1], sCmd.nCmd[2], sCmd.nCmd[3], sCmd.nCmd[4], sCmd.nCmd[5], sCmd.nCmd[6], sCmd.nCmd[7], sCmd.nCmd[8], sCmd.nCmd[9], sCmd.nCmd[10], sCmd.nCmd[11] );
				goto error;
			}
		}
	}

	if( psRawCmd->nSense == SENSE_OK && psRawCmd->nDirection == READ && psRawCmd->nDataLength > 0 )
		memcpy_to_user( psRawCmd->pnData, psDevice->pDataBuffer, psRawCmd->nDataLength );

	UNLOCK( psDevice->hLock );

	return 0;

error:
	UNLOCK( psDevice->hLock );
	return -EIO;
}

static status_t scsi_cdrom_ioctl( void *pNode, void *pCookie, uint32 nCommand, void *pArgs, bool bFromKernel )
{
	SCSI_device_s *psDevice = pNode;
	int nError = 0;

	switch ( nCommand )
	{
		case IOCTL_GET_DEVICE_GEOMETRY:
		{
			device_geometry sGeo;

			kerndbg( KERN_DEBUG, "scsi_cdrom_ioctl( IOCTL_GET_DEVICE_GEOMETRY )\n" );

			sGeo.sector_count = psDevice->nSectors;
			sGeo.cylinder_count = 1;
			sGeo.sectors_per_track = 1;
			sGeo.head_count = 1;
			sGeo.bytes_per_sector = psDevice->nSectorSize;
			sGeo.read_only = true;
			sGeo.removable = psDevice->bRemovable;

			if( bFromKernel )
				memcpy( pArgs, &sGeo, sizeof( sGeo ) );
			else
				nError = memcpy_to_user( pArgs, &sGeo, sizeof( sGeo ) );

			break;
		}

		case CD_READ_TOC:
		{
			cdrom_toc_s toc;

			kerndbg( KERN_DEBUG, "scsi_cdrom_ioctl( CD_READ_TOC )\n" );

			nError = scsi_cdrom_read_toc( psDevice, &toc );

			if( nError == 0 )
			{
				if( bFromKernel )
					memcpy( pArgs, &toc, sizeof( toc ) );
				else
					nError = memcpy_to_user( pArgs, &toc, sizeof( toc ) );
			}

			memcpy( &psDevice->sToc, &toc, sizeof( toc ) );
			psDevice->bMediaChanged = false;
			psDevice->bTocValid = true;

			kerndbg( KERN_DEBUG, "scsi_cdrom_ioctl( CD_READ_TOC ) done\n" );

			break;
		}

		case CD_READ_TOC_ENTRY:
		{
			kerndbg( KERN_DEBUG, "scsi_cdrom_ioctl( CD_READ_TOC_ENTRY )\n" );

			nError = -EINVAL;

			if( pArgs != NULL && psDevice->bTocValid )
			{
				cdrom_toc_entry_s *psEntry = ( cdrom_toc_entry_s * ) pArgs;
				uint8 nTrack = psEntry->track;

				if( nTrack > 0 && nTrack < psDevice->sToc.hdr.last_track )
				{
					if( bFromKernel )
						memcpy( pArgs, &psDevice->sToc.ent[nTrack], sizeof( cdrom_toc_entry_s ) );
					else
						nError = memcpy_to_user( pArgs, &psDevice->sToc.ent[nTrack], sizeof( cdrom_toc_entry_s ) );

					nError = 0;
				}
			}

			break;
		}

		case CD_EJECT:
		{
			nError = scsi_eject( psDevice );
			break;
		}

		case CD_READ_CDDA:
		{
			struct cdda_block *psBlock = ( struct cdda_block * )pArgs;

			kerndbg( KERN_DEBUG, "scsi_cdrom_ioctl( CD_READ_CDDA )\n" );

			if( psBlock != NULL )
			{
				nError = scsi_cdrom_cdda_read( psDevice, psBlock->nBlock, psBlock->nSize );
				if( nError == 0 )
					memcpy_to_user( psBlock->pBuf, psDevice->pDataBuffer, psBlock->nSize );
			}
			else
				nError = -EINVAL;

			kerndbg( KERN_DEBUG, "scsi_cdrom_ioctl( CD_READ_CDDA ) done\n" );

			break;
		}

		case CD_PACKET_COMMAND:
		{
			nError = scsi_cdrom_packet_command( psDevice, ( cdrom_packet_cmd_s * ) pArgs );
			break;
		}

		default:
		{
			kerndbg( KERN_WARNING, "scsi_cdrom_ioctl() unknown command %ld\n", nCommand );
			nError = -ENOSYS;
			break;
		}
	}

	return nError;
}

DeviceOperations_s g_sCDROMOperations = {
	scsi_generic_open,
	scsi_generic_close,
	scsi_cdrom_ioctl,
	scsi_generic_read,
	NULL,			// dop_write
	scsi_generic_readv,
	NULL,			// dop_writev
	NULL,			// dop_add_select_req
	NULL			// dop_rem_select_req
};

SCSI_device_s * scsi_add_cdrom( SCSI_host_s * psHost, int nChannel, int nDevice, int nLun, unsigned char nSCSIResult[] )
{
	SCSI_device_s *psDevice = NULL;
	char zTemp[255], zNodePath[255];
	int i, nError;

	/* Create device for CD-ROM */
	psDevice = kmalloc( sizeof( SCSI_device_s ), MEMF_KERNEL );
	memset( psDevice, 0, sizeof( SCSI_device_s ) );

	psDevice->nID = scsi_get_next_id();
	atomic_set( &psDevice->nOpenCount, 0 );
	psDevice->psHost = psHost;
	psDevice->psRawDevice = psDevice;
	psDevice->hLock = create_semaphore( "scsi_cdrom_lock", 1, 0 );
	psDevice->nChannel = nChannel;
	psDevice->nDevice = nDevice;
	psDevice->nLun = nLun;
	psDevice->nType = ( nSCSIResult[0] & 0x1f );
	psDevice->nSCSILevel = ( nSCSIResult[2] & 0x07 );
	psDevice->bRemovable = ( 0x80 & nSCSIResult[1] ) >> 7;
	psDevice->pDataBuffer = kmalloc( 0xffff, MEMF_KERNEL );
	psDevice->nStart = 0;
	psDevice->bMediaChanged = false;
	psDevice->bTocValid = false;

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
	sprintf( zNodePath, "disk/scsi/cd%c/raw", 'a' + psDevice->nID );
	sprintf( zTemp, "%s %s", psDevice->zVendor, psDevice->zModel );
	sprintf( psDevice->zName, "cd%c", 'a' + psDevice->nID );

	psDevice->nDeviceHandle = register_device( "", "scsi" );
	claim_device( psHost->get_device_id(), psDevice->nDeviceHandle, zTemp, DEVICE_DRIVE );

	nError = create_device_node( psHost->get_device_id(), psDevice->nDeviceHandle, zNodePath, &g_sCDROMOperations, psDevice );
	psDevice->nNodeHandle = nError;

	return psDevice;
}
