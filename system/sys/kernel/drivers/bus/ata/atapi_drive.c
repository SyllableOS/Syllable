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

/* Send a REQUEST_SENSE packet and read the response in psSense */
int atapi_drive_request_sense( ATAPI_device_s* psDev, ATAPI_sense_s *psSense )
{
	ATA_cmd_s sCmd;
	
	kerndbg( KERN_DEBUG, "Requesting sense...\n" );

	if( psSense == NULL )
	{
		kerndbg( KERN_PANIC, "atapi_drive_request_sense() called with psSense = NULL\n");
		return( -EINVAL );
	}
	
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
	sCmd.nTransferLength = sizeof( ATAPI_sense_s );
		
	sCmd.nCmd[0] = ATAPI_REQUEST_SENSE;
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not get sense data. Sense key is %x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	memcpy( psSense, psDev->psPort->pDataBuf, sizeof( ATAPI_sense_s ) );
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}

/* Read capacity of the drive */
status_t atapi_drive_read_capacity( ATAPI_device_s* psDev, unsigned long* pnCapacity )
{
	struct {
		uint32 lba;
		uint32 blocklen;
	} sCap;
	ATA_cmd_s sCmd;
	
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
	sCmd.nTransferLength = sizeof( sCap );
		
	sCmd.nCmd[0] = ATAPI_READ_CDVD_CAPACITY;
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not read capacity. Sense key is %x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	memcpy( &sCap, psDev->psPort->pDataBuf, sizeof( sCap ) );
	
	UNLOCK( psDev->hLock );
	
	psDev->nSize = 1 + be32_to_cpu(sCap.lba);
	psDev->nSectorSize = be32_to_cpu(sCap.blocklen);
	psDev->nSize *= be32_to_cpu(sCap.blocklen);
	
	if( pnCapacity != NULL )
		*pnCapacity = psDev->nSize;
	
	return( 0 );
}

/* Test if the drive is ready */
status_t atapi_drive_test_ready( ATAPI_device_s* psDev )
{
	ATA_cmd_s sCmd;
	bigtime_t nTimeout = get_system_time();
	bool bAgain;
	int nError ;

start:
	nError = 0;
	bAgain = false;
	
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.nCmd[0] = ATAPI_TEST_UNIT_READY;
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not get unit status. Sense key is %x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	UNLOCK( psDev->hLock );
	
	switch( sCmd.sSense.sense_key )
	{
		case ATAPI_NO_SENSE:
		{
			kerndbg( KERN_DEBUG_LOW, "Drive is ready\n");
			break;
		}

		case ATAPI_RECOVERED_ERROR:
		{
			kerndbg( KERN_DEBUG, "Drive reports a recovered error\n");
			break;
		}

		case ATAPI_NOT_READY:
		{
			kerndbg( KERN_DEBUG_LOW, "Drive is not ready\n" );
			
			/* See if the unit is going to become ready */
			if( atapi_drive_request_sense( psDev, &sCmd.sSense ) == 0 )
			{
				if( sCmd.sSense.sense_key == ATAPI_NO_SENSE )
				{
					/* The unit became ready, carry on */
					break;
				}
				else if( sCmd.sSense.asc == 4 && sCmd.sSense.ascq != 4 )
				{
					/* Unit is becoming ready */
					kerndbg( KERN_DEBUG_LOW, "Drive is becoming READY...\n");

					snooze( 500000 );
					if( get_system_time() - nTimeout > 31000000 )
					{
						kerndbg( KERN_FATAL, "Error: Timeout while waiting for the drive to become ready\n" );
						bAgain = false;
					} else
						bAgain = true;

					break;
				}
				else
				{
					kerndbg( KERN_WARNING, "Drive is NOT READY\n");
					nError = -EIO;
					break;
				}
			}
			else
			{
				/* Something is really up */
				kerndbg( KERN_WARNING, "Drive is not ready and unable to provide additional sense data.\n");
				nError = -EIO;
			}

			break;
		}

		case ATAPI_UNIT_ATTENTION:
		{
			kerndbg( KERN_DEBUG_LOW, "Drive reports attention\n");
			psDev->bMediaChanged = true;
			psDev->bTocValid = false;
			break;
		}
		default:
		{
			kerndbg( KERN_FATAL, "Non-recoverable error 0x%.2x\n", sCmd.sSense.sense_key );
			nError = -EIO;
			break;
		}
	}

	if( bAgain )
		goto start;

	return( nError );
}


status_t atapi_drive_eject( ATAPI_device_s* psDev )
{
	ATA_cmd_s sCmd;
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	
	sCmd.nCmd[0] = ATAPI_START_STOP_UNIT;
	sCmd.nCmd[4] = 0x02;
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not eject drive. Sense key is %x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}


status_t atapi_drive_read_toc_entry( ATAPI_device_s* psDev, int trackno, int msf_flag, int format, char *buf, int buflen)
{
	ATA_cmd_s sCmd;
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
	sCmd.nTransferLength = buflen;
	sCmd.nCmd[0] = ATAPI_READ_TOC_PMA_ATIP;
	sCmd.nCmd[6] = trackno;
	sCmd.nCmd[7] = (buflen >> 8);
	sCmd.nCmd[8] = (buflen & 0xff);
	sCmd.nCmd[9] = (format << 6);
	
	if(msf_flag)
		sCmd.nCmd[1] = 2;
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not read toc entry. Sense key is %x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	memcpy( buf, psDev->psPort->pDataBuf, buflen );
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}

status_t atapi_drive_read_toc( ATAPI_device_s* psDev, cdrom_toc_s *toc )
{
	int stat, ntracks, i;

	struct
	{
		cdrom_toc_header_s hdr;
		cdrom_toc_entry_s  ent;
	} ms_tmp;

	if (toc == NULL)
	{
		/* Try to allocate space. */
		toc = ( cdrom_toc_s*) kmalloc (sizeof (cdrom_toc_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
		if (toc == NULL)
			return( -ENOMEM );
	}

	/* First read just the header, so we know how long the TOC is. */
	stat = atapi_drive_read_toc_entry( psDev, 0, 1, 0, (char *) &toc->hdr,  sizeof(cdrom_toc_header_s));
	if( stat < 0 )
		return( stat );

	ntracks = toc->hdr.last_track - toc->hdr.first_track + 1;
	if (ntracks <= 0)
		return -EIO;
	if (ntracks > MAX_TRACKS)
		ntracks = MAX_TRACKS;
		
	/* Now read the whole schmeer. */
	stat = atapi_drive_read_toc_entry( psDev, toc->hdr.first_track, 1, 0, (char *)&toc->hdr, sizeof(cdrom_toc_header_s) + (ntracks + 1) *  sizeof(cdrom_toc_entry_s));

	if ( stat < 0 && toc->hdr.first_track > 1 )
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
		stat = atapi_drive_read_toc_entry( psDev, CDROM_LEADOUT, 1, 0, (char *)&toc->hdr, sizeof(cdrom_toc_header_s) + (ntracks + 1) * sizeof(cdrom_toc_entry_s));
		if (stat < 0)
			return( stat );

		toc->hdr.first_track = CDROM_LEADOUT;
		toc->hdr.last_track = CDROM_LEADOUT;
	}

	if(stat < 0)
		return( stat );

	toc->hdr.toc_length = ( toc->hdr.toc_length << 8 | toc->hdr.toc_length >> 8 );

	for (i=0; i<=ntracks; i++)
		toc->ent[i].addr.lba = msf_to_lba (toc->ent[i].addr.msf.minute, toc->ent[i].addr.msf.second, toc->ent[i].addr.msf.frame);

	/* Read the multisession information. */
	if (toc->hdr.first_track != CDROM_LEADOUT)
	{
		/* Read the multisession information. */
		stat = atapi_drive_read_toc_entry(psDev, 0, 1, 1, (char *)&ms_tmp, sizeof(ms_tmp));
		if(stat<0)
			return( stat );
	}
	else
	{
		ms_tmp.ent.addr.msf.minute = 0;
		ms_tmp.ent.addr.msf.second = 2;
		ms_tmp.ent.addr.msf.frame  = 0;
		ms_tmp.hdr.first_track = ms_tmp.hdr.last_track = CDROM_LEADOUT;
	}

	toc->last_session_lba = msf_to_lba (ms_tmp.ent.addr.msf.minute, ms_tmp.ent.addr.msf.second, ms_tmp.ent.addr.msf.frame);
	toc->xa_flag = (ms_tmp.hdr.first_track != ms_tmp.hdr.last_track);

	/* Now try to get the total cdrom capacity. */

	stat = atapi_drive_read_capacity(psDev, &toc->capacity);
	if ( stat < 0 )
		toc->capacity = 0x1fffff;
	
	return( 0 );
}


status_t atapi_drive_play_audio( ATAPI_device_s* psDev, int lba_start, int lba_end)
{
	ATA_cmd_s sCmd;
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.nCmd[0] = ATAPI_PLAY_AUDIO_MSF;
	lba_to_msf(lba_start, &sCmd.nCmd[3], &sCmd.nCmd[4], &sCmd.nCmd[5]);
	lba_to_msf(lba_end-1, &sCmd.nCmd[6], &sCmd.nCmd[7], &sCmd.nCmd[8]);
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not play audio. Sense key is %x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}

status_t atapi_drive_pause_resume_audio( ATAPI_device_s* psDev, bool bPause )
{
	ATA_cmd_s sCmd;
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.nCmd[0] = ATAPI_PAUSE_RESUME;
	if( !bPause )
		sCmd.nCmd[8] = 0x01;	/* Set the RESUME bit */
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not pause audio. Sense key is %x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}

status_t atapi_drive_stop_audio( ATAPI_device_s *psDev )
{
	ATA_cmd_s sCmd;
	int nError;
	
	nError = atapi_drive_pause_resume_audio( psDev, true );
	
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.nCmd[0] = ATAPI_STOP_AUDIO;
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not stop audio. Sense key is %x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}

status_t atapi_drive_get_playback_time( ATAPI_device_s *psDev, cdrom_msf_s *pnTime )
{
	struct {
		uint8 format;
		uint8 adr_ctl;
		uint8 track;
		uint8 index;
		uint32 abs_addr;
		uint32 rel_addr;
	} posbuf;
	
	ATA_cmd_s sCmd;
	
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
	sCmd.nTransferLength = sizeof(posbuf);
	
	sCmd.nCmd[0] = ATAPI_READ_SUB_CHANNEL;
	sCmd.nCmd[2] = 0x40;	/* Return SUBQ data */
	sCmd.nCmd[3] = 0x01;	/* CD Current Position */
	sCmd.nCmd[7] = ( sizeof(posbuf) >> 8);
	sCmd.nCmd[8] = ( sizeof(posbuf) & 0xff);
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not get time. Sense key is %x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	memcpy( &posbuf, psDev->psPort->pDataBuf, sizeof( posbuf ) );
	
	UNLOCK( psDev->hLock );
	
	lba_to_msf( be32_to_cpu( posbuf.rel_addr ), &pnTime->minute, &pnTime->second, &pnTime->frame );

	return( 0 );
}

int atapi_drive_open( void* pNode, uint32 nFlags, void **ppCookie )
{
	ATAPI_device_s* psDev = pNode;
	int nError = 0;

	atomic_add( &psDev->nOpenCount, 1 );

	nError = atapi_drive_test_ready( psDev );
	if( nError == 0 )
	{
		nError = atapi_drive_read_capacity( psDev, NULL );
		if( nError == 0 )
		{
			
			kerndbg( KERN_DEBUG, "Capacity %Ld\n", psDev->nSize );
		}
		else
			kerndbg( KERN_WARNING, "Could not read capacity");

		udelay( 200 );	/* FIXME: Should send another TEST UNIT READY to ensure the disc has spun up */
	}
	else
	{
		kerndbg( KERN_DEBUG, "Could not open drive\n");
		/* FIXME: We should REQUEST SENSE now to find out why the unit isn't ready; it could be in the process of becoming ready, for example */
	}

	return( nError );
}

int atapi_drive_close( void* pNode, void* pCookie )
{
	ATAPI_device_s* psDev = pNode;
	atomic_add( &psDev->nOpenCount, -1 );

	return( 0 );
}


int atapi_drive_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen )
{
	ATAPI_device_s* psDev = pNode;
	ATA_cmd_s sCmd;
	int nBytesLeft;
	
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
	
	
	LOCK( psDev->hLock );
	
	nBytesLeft = nLen;
	nPos += psDev->nStart;
	
	/* The iso9660 fs requires this workaround... */
	if( nLen < psDev->nSectorSize )
		nBytesLeft = psDev->nSectorSize;
	
	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( 16 * PAGE_SIZE, nBytesLeft );
		uint64 nBlock = nPos / psDev->nSectorSize;
		uint32 nBlockCount = nCurSize / psDev->nSectorSize;
		
		kerndbg( KERN_DEBUG_LOW, "Reading %i blocks from block %Lin", (int)nBlockCount, nBlock );
		
		/* Prepare read command */
		ata_cmd_init( &sCmd, psDev->psPort );
	
		sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
		sCmd.nTransferLength = nCurSize;
	
		sCmd.nCmd[0] = ATAPI_READ_10;
		sCmd.nCmd[2] = ( nBlock >> 24 ) & 0xff;
		sCmd.nCmd[3] = ( nBlock >> 16 ) & 0xff;
		sCmd.nCmd[4] = ( nBlock >> 8 ) & 0xff;
		sCmd.nCmd[5] = ( nBlock >> 0 ) & 0xff;
		sCmd.nCmd[6] = ( nBlockCount >> 16 ) & 0xff;
		sCmd.nCmd[7] = ( nBlockCount >> 8 ) & 0xff;
		sCmd.nCmd[8] = ( nBlockCount >> 0 ) & 0xff;
	
		/* Queue and wait */
		ata_cmd_queue( psDev->psPort, &sCmd );
		LOCK( sCmd.hWait );
		//ata_cmd_ata( &sCmd );
		ata_cmd_free( &sCmd );
		
		if( sCmd.nStatus < 0 ) {
			kerndbg( KERN_WARNING, "Error: Packet finished with status %i\n", sCmd.nStatus );
			UNLOCK( psDev->hLock );
			return( -EIO );
		}
		
		if( nLen < psDev->nSectorSize )
			memcpy( pBuf, psDev->psPort->pDataBuf, nLen );
		else
			memcpy( pBuf, psDev->psPort->pDataBuf, nCurSize );
			
		pBuf = ((uint8*)pBuf) + nCurSize;
		nPos += nCurSize;
		nBytesLeft -= nCurSize;
	}
	UNLOCK( psDev->hLock );
	return( nLen );	
}


int atapi_drive_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount )
{
	ATAPI_device_s* psDev = pNode;
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
		uint64 nBlock = nPos / psDev->nSectorSize;
		uint32 nBlockCount = nCurSize / psDev->nSectorSize;
	
		if ( nPos < 0 )
		{
			kerndbg( KERN_PANIC, "atapi_drive_readv() wierd pos = %Ld\n", nPos );
			kassertw(0);
		}
		
		kerndbg( KERN_DEBUG_LOW, "Reading %i blocks from block %Lin", (int)nBlockCount, nBlock );
		
		/* Prepare read command */
		ata_cmd_init( &sCmd, psDev->psPort );
	
		sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
		sCmd.nTransferLength = nCurSize;
	
		sCmd.nCmd[0] = ATAPI_READ_10;
		sCmd.nCmd[2] = ( nBlock >> 24 ) & 0xff;
		sCmd.nCmd[3] = ( nBlock >> 16 ) & 0xff;
		sCmd.nCmd[4] = ( nBlock >> 8 ) & 0xff;
		sCmd.nCmd[5] = ( nBlock >> 0 ) & 0xff;
		sCmd.nCmd[6] = ( nBlockCount >> 16 ) & 0xff;
		sCmd.nCmd[7] = ( nBlockCount >> 8 ) & 0xff;
		sCmd.nCmd[8] = ( nBlockCount >> 0 ) & 0xff;
	
		/* Queue and wait */
		ata_cmd_queue( psDev->psPort, &sCmd );
		LOCK( sCmd.hWait );
		//ata_cmd_ata( &sCmd );
		ata_cmd_free( &sCmd );
		
		if( sCmd.nStatus < 0 ) {
			kerndbg( KERN_WARNING, "Error: Packet finished with status %i\n", sCmd.nStatus );
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
			nError = atapi_drive_eject( psDev );
			break;
		}

		case CD_READ_TOC:
		{
			cdrom_toc_s toc;

			nError = atapi_drive_read_toc( psDev, &toc );

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

			nError = atapi_drive_play_audio( psDev, msf_to_lba(track->msf_start.minute, track->msf_start.second, track->msf_start.frame), msf_to_lba(track->msf_stop.minute, track->msf_stop.second, track->msf_stop.frame) );
			break;
		}

		case CD_PLAY_LBA:
		{
			cdrom_audio_track_s *track = (cdrom_audio_track_s*)pArgs;

			nError = atapi_drive_play_audio( psDev, track->lba_start, track->lba_stop );
			break;
		}

		case CD_PAUSE:
		{
			nError = atapi_drive_pause_resume_audio( psDev, true );
			break;
		}

		case CD_RESUME:
		{
			nError = atapi_drive_pause_resume_audio( psDev, false );
			break;
		}

		case CD_STOP:
		{
			nError = atapi_drive_stop_audio( psDev );
			break;
		}

		case CD_GET_TIME:
		{
			nError = atapi_drive_get_playback_time( psDev, (cdrom_msf_s*)pArgs );
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
	psDev->hLock = create_semaphore( "atapi_drive_lock", 1, 0 );
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


















