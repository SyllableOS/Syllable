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
 *
 * Changes
 *
 * 05/04/03 - Implemented CD_READ_TOC_ENTRY (Untested).  Added
 *			  cdrom_stop_audio(), cdrom_get_plaback_time() and
 *		      CD_STOP & CD_GET_TIME ioctl()'s
 *
 * 02/04/03 - Removed cdrom_start_stop(), which was not required and
 *		      causing problems with some CD/DVD drives
 *
 */

#include "ata.h"
#include "ata-io.h"
#include "atapi-drive.h"

#include <atheos/cdrom.h>

extern int g_nDevices[MAX_DRIVES];
extern ata_controllers_t g_nControllers[MAX_CONTROLLERS];

DeviceOperations_s g_sAtapiOperations = {
	atapi_open,
	atapi_close,
	atapi_ioctl,
	atapi_read,
	NULL,	/* dop_write */
	atapi_readv,
	NULL,	/* dop_writev */
	NULL,	/* dop_add_select_req */
	NULL	/* dop_rem_select_req */
};

int atapi_open( void* pNode, uint32 nFlags, void **ppCookie )
{
	AtapiInode_s* psInode = pNode;
	int controller = psInode->bi_nController;
	int nError = 0;

	LOCK( g_nControllers[controller].buf_lock );
	atomic_add( &psInode->bi_nOpenCount, 1 );
	UNLOCK( g_nControllers[controller].buf_lock );

	nError = cdrom_test_unit_ready( psInode );
	if( nError == 0 )
	{
		unsigned long capacity;

		kerndbg( KERN_DEBUG, "Unit ready\n");

		nError = cdrom_read_capacity( psInode, &capacity );
		if( nError == 0 )
		{
			psInode->bi_nSize = capacity;
			psInode->bi_nSize *= CD_FRAMESIZE;

			kerndbg( KERN_DEBUG, "Capacity %Ld\n", psInode->bi_nSize );
		}
		else
			kerndbg( KERN_WARNING, "Could not read capacity");

		udelay( 200 );	/* FIXME: Should send another TEST UNIT READY to ensure the disc has spun up */
		kerndbg( KERN_DEBUG, "Unit started\n");
	}
	else
	{
		kerndbg( KERN_DEBUG, "Unit not ready\n");
		/* FIXME: We should REQUEST SENSE now to find out why the unit isn't ready; it could be in the process of becoming ready, for example */
	}

	return( nError );
}

int atapi_close( void* pNode, void* pCookie )
{
	AtapiInode_s* psInode = pNode;
	int controller = psInode->bi_nController;

	LOCK( g_nControllers[controller].buf_lock );
	atomic_add( &psInode->bi_nOpenCount, -1 );
	UNLOCK( g_nControllers[controller].buf_lock );

	return( 0 );
}

int atapi_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen )
{
	AtapiInode_s *psInode = (AtapiInode_s*)pNode;
	atapi_packet_s command;
	int drive = psInode->bi_nDriveNum;
	int controller = get_controller( drive );
	int nError = 0;
	uint64 block_lba, block_count;
	size_t nLenSave = nLen;

	kerndbg( KERN_DEBUG_LOW, "atapi_read() nPos = %Li, nLen = %i\n", nPos, nLen);

	/* Ensure sensible values */
	if( nPos > psInode->bi_nSize || nPos + nLen > psInode->bi_nSize )
	{
		kerndbg( KERN_DEBUG_LOW, "atapi_read() nPos out of range\n");
		return( -EINVAL );
	}

	if( nLen < CD_FRAMESIZE )
		nLen = CD_FRAMESIZE;

	block_lba = nPos / CD_FRAMESIZE;
	block_count = nLen / CD_FRAMESIZE;

	/* Data comes in as words, so we have to enforce even byte counts. */
	if ( nLen & 1)
	{
		kerndbg( KERN_DEBUG_LOW, "atapi_read() nLen is not an even value\n");
		return( -EINVAL );
	}

	kerndbg( KERN_DEBUG, "Reading %Li blocks from %Li\n", block_count, block_lba );

	memset(&command, 0, sizeof(command));

	command.count = nLen;
	command.packet[0] = GPCMD_READ_10;
	command.packet[2] = ( block_lba >> 24 ) & 0xff;
	command.packet[3] = ( block_lba >> 16 ) & 0xff;
	command.packet[4] = ( block_lba >> 8 ) & 0xff;
	command.packet[5] = ( block_lba >> 0 ) & 0xff;
	command.packet[6] = ( block_count >> 16 ) & 0xff;
	command.packet[7] = ( block_count >> 8 ) & 0xff;
	command.packet[8] = ( block_count >> 0 ) & 0xff;

	LOCK( g_nControllers[controller].buf_lock );

	nError = atapi_packet_command( psInode, &command );

	kerndbg( KERN_DEBUG_LOW, "nError = %i\n", nError );

	if( nError == 0 )
	{
		if( pBuf != NULL )	/* If called from atapi_readv() then pBuf is NULL */
			memcpy( pBuf, g_nControllers[controller].data_buffer, nLenSave );

		nError = nLenSave;
	}

	UNLOCK( g_nControllers[controller].buf_lock );

	return( nError );
}

int atapi_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount )
{
	AtapiInode_s*  psInode  = pNode;
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

	if ( nPos & (CD_FRAMESIZE - 1) )
	{
		kerndbg( KERN_WARNING, "atapi_readv() position has bad alignment %Ld\n", nPos );
		return( -EINVAL );
	}

	if ( nLen & (CD_FRAMESIZE - 1) )
	{
		kerndbg( KERN_WARNING, "atapi_readv() length has bad alignment %d\n", nLen );
		return( -EINVAL );
	}

	if ( nPos >= psInode->bi_nSize )
	{
		kerndbg( KERN_WARNING, "atapi_readv() Request outside disc : %Ld\n", nPos );
		return( 0 );
	}

	if ( nPos + nLen > psInode->bi_nSize )
	{
		kerndbg( KERN_WARNING, "atapi_readv() Request truncated from %d to %Ld\n", nLen, (psInode->bi_nSize - nPos) );
		nLen = psInode->bi_nSize - nPos;
	}

	nBytesLeft = nLen;
	nPos += psInode->bi_nStart;

	pCurVecPtr = psVector[0].iov_base;
	nCurVecLen = psVector[0].iov_len;
	nCurVec = 1;

	while ( nBytesLeft > 0 )
	{
		int nCurSize = min( ATA_BUFFER_SIZE, nBytesLeft );
		char* pSrcPtr = g_nControllers[controller].data_buffer;
	
		if ( nPos < 0 )
		{
			kerndbg( KERN_DEBUG,"atapi_readv() wierd pos = %Ld\n", nPos );
			kassertw(0);
		}

		nError = atapi_read( psInode, NULL, nPos, NULL, nCurSize );

		LOCK( g_nControllers[controller].buf_lock );

		if ( nError < 0 )
		{
			kerndbg( KERN_WARNING, "ata_readv() failed to read %d sectors from drive %x\n", (nCurSize / CD_FRAMESIZE), psInode->bi_nDriveNum );
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

		UNLOCK( g_nControllers[controller].buf_lock );
	}

error:
	return( nLen );
}

int atapi_request_sense( AtapiInode_s *psInode, request_sense_s *psSense )
{
	/* Send a REQUEST_SENSE packet and read the response in psSense */

	atapi_packet_s command;
	int nError, controller = psInode->bi_nController;

	if( psSense == NULL )
	{
		kerndbg( KERN_PANIC, "psSense is NULL\n");
		return( -EINVAL );
	}

	memset(&command, 0, sizeof (command));

	command.packet[0] = GPCMD_REQUEST_SENSE;
	command.packet[4] = sizeof( request_sense_s );

	command.count = sizeof( request_sense_s );

	kerndbg( KERN_DEBUG, "Getting sense data\n");

	LOCK( g_nControllers[controller].buf_lock );
	nError = atapi_packet_command( psInode, &command );
	UNLOCK( g_nControllers[controller].buf_lock );

	if( nError != 0 )
	{
		kerndbg(KERN_PANIC,"Error getting sense data.  Things are looking bad.\n");
	}
	else
		memcpy( psSense, g_nControllers[controller].data_buffer, sizeof( request_sense_s ));

	return( nError );
}

status_t cdrom_test_unit_ready( AtapiInode_s *psInode )
{
	atapi_packet_s command;
	int nError, controller = psInode->bi_nController;

	memset(&command, 0, sizeof(command));
	command.packet[0] = GPCMD_TEST_UNIT_READY;

	LOCK( g_nControllers[controller].buf_lock );
	nError = atapi_packet_command( psInode, &command );
	UNLOCK( g_nControllers[controller].buf_lock );

	if( nError == 0 )
	{
		switch( command.sense_key )
		{
			case NO_SENSE:
			{
				kerndbg( KERN_DEBUG, "Unit responded GOOD\n");
				break;
			}

			case RECOVERED_ERROR:
			{
				kerndbg( KERN_DEBUG, "Error is recoverable, ignoring\n");
				break;
			}

			case NOT_READY:
			{
				/* See if the unit is going to become ready */
				if( atapi_request_sense( psInode, &command.sense ) == 0 )
				{
					if( command.sense.sense_key == NO_SENSE )
					{
						/* The unit became ready, carry on */
						break;
					}
					else if( command.sense.asc == 4 && command.sense.ascq != 4 )
					{
						/* Unit is becoming ready */
						kerndbg( KERN_INFO, "Unit is becoming READY...\n");

						nError = cdrom_wait_for_unit_ready( psInode );
						if(  nError != 0 )
							kerndbg( KERN_DEBUG, "Unit has not become READY\n");

						break;
					}
					else
					{
						kerndbg( KERN_DEBUG, "Unit NOT READY\n");
						nError = -EIO;
						break;
					}
				}
				else
				{
					/* Something is really up */
					kerndbg( KERN_WARNING, "Unit NOT READY and unable to get additional sense data.\n");
					nError = -EIO;
				}

				break;
			}

			case UNIT_ATTENTION:
			{
				kerndbg( KERN_DEBUG, "UNIT ATTENTION, continuing\n");
				cdrom_saw_media_change( psInode );
				break;
			}

			default:
			{
				kerndbg( KERN_DEBUG, "Non-recoverable error 0x%.2x\n", command.sense_key );
				nError = -EIO;
				break;
			}
		}
	}
	else
		kerndbg( KERN_DEBUG, "An ATAPI packet error occured, unable to get unit status\n"); 

	return( nError );
}

status_t cdrom_wait_for_unit_ready( AtapiInode_s *psInode )
{
	ktimer_t timer;
	int nState, nError, controller = psInode->bi_nController;
	atapi_packet_s command;

	memset(&command, 0, sizeof(command));

	timer = create_timer();
	start_timer( timer, timeout, (void*)&nState, ATAPI_SPIN_TIMEOUT, true);

	nState = BUSY;
	while( nState == BUSY )
	{
		kerndbg( KERN_DEBUG, "Waiting for UNIT READY\n");

		udelay( 200 );

		command.packet[0] = GPCMD_TEST_UNIT_READY;

		LOCK( g_nControllers[controller].buf_lock );
		nError = atapi_packet_command( psInode, &command );
		UNLOCK( g_nControllers[controller].buf_lock );

		if( nError == 0 )
		{
			if( command.sense_key == NO_SENSE )
			{
				/* The unit became ready, carry on */
				break;
			}
			else
			{
				nError = atapi_request_sense( psInode, &command.sense );
				if( nError == 0 )
				{
					if( command.sense.sense_key == NO_SENSE )
					{
						/* The unit became ready, carry on */
						break;
					}
					else if( command.sense.asc == 4 && command.sense.ascq != 4 )
					{
						/* Unit is becoming ready */
						continue;
					}
				}
			}
		}
		else
		{
			kerndbg( KERN_PANIC, "cdrom_wait_for_unit_ready() could not get device status\n");
			break;
		}
	}

	delete_timer( timer );

	if( nState == DEV_TIMEOUT )
	{
		kerndbg( KERN_DEBUG, "Timedout waiting for device to become ready\n");
		nError = -EIO;
	}

	return( nError );
}

status_t cdrom_eject_media( AtapiInode_s *psInode )
{
	atapi_packet_s command;
	int nError, controller = psInode->bi_nController;

	memset(&command, 0, sizeof(command));
	command.packet[0] = GPCMD_START_STOP_UNIT;
	command.packet[4] = 0x02;

	LOCK( g_nControllers[controller].buf_lock );
	nError = atapi_packet_command( psInode, &command );
	UNLOCK( g_nControllers[controller].buf_lock );

	return( nError );
}

status_t cdrom_read_toc_entry( AtapiInode_s *psInode, int trackno, int msf_flag, int format, char *buf, int buflen)
{
	atapi_packet_s command;
	int error, controller = psInode->bi_nController;

	memset(&command, 0, sizeof(command));

	command.count = buflen;
	command.packet[0] = GPCMD_READ_TOC_PMA_ATIP;
	command.packet[6] = trackno;
	command.packet[7] = (buflen >> 8);
	command.packet[8] = (buflen & 0xff);
	command.packet[9] = (format << 6);

	if(msf_flag)
		command.packet[1] = 2;

	LOCK( g_nControllers[controller].buf_lock );

	error = atapi_packet_command( psInode, &command);

	if( error == 0 )
		memcpy( buf, g_nControllers[psInode->bi_nController].data_buffer, buflen );

	UNLOCK( g_nControllers[controller].buf_lock );

	return( error );
}

status_t cdrom_read_toc( AtapiInode_s *psInode, cdrom_toc_s *toc )
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
	stat = cdrom_read_toc_entry( psInode, 0, 1, 0, (char *) &toc->hdr,  sizeof(cdrom_toc_header_s));
	if(stat)
		return( stat );

	ntracks = toc->hdr.last_track - toc->hdr.first_track + 1;
	if (ntracks <= 0)
		return -EIO;
	if (ntracks > MAX_TRACKS)
		ntracks = MAX_TRACKS;

	/* Now read the whole schmeer. */
	stat = cdrom_read_toc_entry( psInode, toc->hdr.first_track, 1, 0, (char *)&toc->hdr, sizeof(cdrom_toc_header_s) + (ntracks + 1) *  sizeof(cdrom_toc_entry_s));

	if (stat && toc->hdr.first_track > 1)
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
		stat = cdrom_read_toc_entry(psInode, CDROM_LEADOUT, 1, 0, (char *)&toc->hdr, sizeof(cdrom_toc_header_s) + (ntracks + 1) * sizeof(cdrom_toc_entry_s));
		if (stat)
			return( stat );

		toc->hdr.first_track = CDROM_LEADOUT;
		toc->hdr.last_track = CDROM_LEADOUT;
	}

	if(stat)
		return( stat );

	toc->hdr.toc_length = ( toc->hdr.toc_length << 8 | toc->hdr.toc_length >> 8 );

	for (i=0; i<=ntracks; i++)
		toc->ent[i].addr.lba = msf_to_lba (toc->ent[i].addr.msf.minute, toc->ent[i].addr.msf.second, toc->ent[i].addr.msf.frame);

	/* Read the multisession information. */
	if (toc->hdr.first_track != CDROM_LEADOUT)
	{
		/* Read the multisession information. */
		stat = cdrom_read_toc_entry(psInode, 0, 1, 1, (char *)&ms_tmp, sizeof(ms_tmp));
		if(stat)
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
#if 0
	stat = cdrom_get_last_written(dev, &toc->capacity);
	if (stat)
#endif
		stat = cdrom_read_capacity(psInode, &toc->capacity);
	if (stat)
		toc->capacity = 0x1fffff;

	psInode->bi_nSize = toc->capacity * CD_FRAMESIZE;

	return( 0 );
}

status_t cdrom_read_capacity(AtapiInode_s *psInode, unsigned long *capacity )
{
	struct {
		uint32 lba;
		uint32 blocklen;
	} capbuf;

	int controller = get_controller( psInode->bi_nDriveNum );
	int stat;
	atapi_packet_s command;

	memset(&command, 0, sizeof(command));

	command.count = sizeof(capbuf);
	command.packet[0] = GPCMD_READ_CDVD_CAPACITY;

	LOCK( g_nControllers[controller].buf_lock );

	stat = atapi_packet_command(psInode, &command);
	if (stat == 0)
	{
		memcpy( &capbuf, g_nControllers[controller].data_buffer, sizeof(capbuf));
		*capacity = 1 + be32_to_cpu(capbuf.lba);
	}

	UNLOCK( g_nControllers[controller].buf_lock );

	return( stat );
}

status_t cdrom_play_audio(AtapiInode_s *psInode, int lba_start, int lba_end)
{
	atapi_packet_s command;
	int nError, controller = psInode->bi_nController;

	memset(&command, 0, sizeof (command));

	command.packet[0] = GPCMD_PLAY_AUDIO_MSF;
	lba_to_msf(lba_start, &command.packet[3], &command.packet[4], &command.packet[5]);
	lba_to_msf(lba_end-1, &command.packet[6], &command.packet[7], &command.packet[8]);

	LOCK( g_nControllers[controller].buf_lock );
	nError = atapi_packet_command( psInode, &command);
	UNLOCK( g_nControllers[controller].buf_lock );

	return( nError );
}

status_t cdrom_pause_resume_audio(AtapiInode_s *psInode, bool bPause )
{
	atapi_packet_s command;
	int nError, controller = psInode->bi_nController;

	memset(&command, 0, sizeof (command));

	command.packet[0] = GPCMD_PAUSE_RESUME;
	if( !bPause )
		command.packet[8] = 0x01;	/* Set the RESUME bit */

	LOCK( g_nControllers[controller].buf_lock );
	nError = atapi_packet_command( psInode, &command);
	UNLOCK( g_nControllers[controller].buf_lock );

	return( nError );
}

status_t cdrom_stop_audio( AtapiInode_s *psInode )
{
	atapi_packet_s command;
	int nError, controller = psInode->bi_nController;

	/* The drive must be paused before it will stop */
	nError = cdrom_pause_resume_audio( psInode, true );
	if( !nError )
	{
		memset(&command, 0, sizeof (command));

		command.packet[0] = GPCMD_STOP_AUDIO;

		LOCK( g_nControllers[controller].buf_lock );
		nError = atapi_packet_command( psInode, &command);
		UNLOCK( g_nControllers[controller].buf_lock );
	}

	return( nError );
}

status_t cdrom_get_playback_time( AtapiInode_s *psInode, cdrom_msf_s *pnTime )
{
	struct {
		uint8 format;
		uint8 adr_ctl;
		uint8 track;
		uint8 index;
		uint32 abs_addr;
		uint32 rel_addr;
	} posbuf;

	atapi_packet_s command;
	int nError, controller = psInode->bi_nController;

	memset(&command, 0, sizeof (command));

	command.packet[0] = GPCMD_READ_SUB_CHANNEL;
	command.packet[2] = 0x40;	/* Return SUBQ data */
	command.packet[3] = 0x01;	/* CD Current Position */
	command.packet[7] = ( sizeof(posbuf) >> 8);
	command.packet[8] = ( sizeof(posbuf) & 0xff);

	command.count = sizeof(posbuf);

	LOCK( g_nControllers[controller].buf_lock );

	nError = atapi_packet_command(psInode, &command);
	if(nError == 0)
		memcpy( &posbuf, g_nControllers[controller].data_buffer, sizeof(posbuf));

	UNLOCK( g_nControllers[controller].buf_lock );

	/* posbuf.track *should* contain the # of the current track, but does not appear
	   to be valid on my CD-RW drive?
	   rel_addr & abs_addr *are* valid (Big Endian), though */
 
	lba_to_msf( be32_to_cpu( posbuf.rel_addr ), &pnTime->minute, &pnTime->second, &pnTime->frame );

	kerndbg( KERN_DEBUG_LOW, "Playback time : %.2i:%.2i\n", pnTime->minute, pnTime->second );

	return( nError );
}

status_t cdrom_saw_media_change(AtapiInode_s *psInode)
{
	psInode->bi_bMediaChanged = true;
	psInode->bi_bTocValid = false;
	return( 0 );
}

status_t atapi_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	AtapiInode_s*  psInode  = pNode;
	int nError = 0;

	switch( nCommand )
	{
		case IOCTL_GET_DEVICE_GEOMETRY:
		{
			device_geometry sGeo;

			sGeo.sector_count		= psInode->bi_nSize / 512;
			sGeo.cylinder_count		= 0;
			sGeo.sectors_per_track	= 0;
			sGeo.head_count			= 0;
			sGeo.bytes_per_sector	= CD_FRAMESIZE;
			sGeo.read_only			= true;
			sGeo.removable			= psInode->bi_bRemovable;

			if ( bFromKernel )
				memcpy( pArgs, &sGeo, sizeof(sGeo) );
			else
				nError = memcpy_to_user( pArgs, &sGeo, sizeof(sGeo) );

			break;
		}

		case CD_EJECT:
		{
			nError = cdrom_eject_media( psInode );
			break;
		}

		case CD_READ_TOC:
		{
			cdrom_toc_s toc;

			nError = cdrom_read_toc( psInode, &toc );

			if( nError == 0 )
			{
				if ( bFromKernel )
					memcpy( pArgs, &toc, sizeof(toc) );
				else
					nError = memcpy_to_user( pArgs, &toc, sizeof(toc) );
			}

			memcpy( &psInode->toc, &toc, sizeof(toc) );
			psInode->bi_bMediaChanged = false;
			psInode->bi_bTocValid = true;
			break;
		}

		case CD_READ_TOC_ENTRY:
		{
			nError = -EINVAL;

			if( pArgs != NULL && psInode->bi_bTocValid )
			{
				cdrom_toc_entry_s *psEntry = (cdrom_toc_entry_s*)pArgs;
				uint8 nTrack = psEntry->track;

				if( nTrack > 0 && nTrack < psInode->toc.hdr.last_track )
				{
					if ( bFromKernel )
						memcpy( pArgs, &psInode->toc.ent[nTrack], sizeof(cdrom_toc_entry_s) );
					else
						nError = memcpy_to_user( pArgs, &psInode->toc.ent[nTrack], sizeof(cdrom_toc_entry_s) );

					nError = 0;
				}
			}

			break;
		}

		case CD_PLAY_MSF:
		{
			cdrom_audio_track_s *track = (cdrom_audio_track_s*)pArgs;

			nError = cdrom_play_audio( psInode, msf_to_lba(track->msf_start.minute, track->msf_start.second, track->msf_start.frame), msf_to_lba(track->msf_stop.minute, track->msf_stop.second, track->msf_stop.frame) );
			break;
		}

		case CD_PLAY_LBA:
		{
			cdrom_audio_track_s *track = (cdrom_audio_track_s*)pArgs;

			nError = cdrom_play_audio( psInode, track->lba_start, track->lba_stop );
			break;
		}

		case CD_PAUSE:
		{
			nError = cdrom_pause_resume_audio( psInode, true );
			break;
		}

		case CD_RESUME:
		{
			nError = cdrom_pause_resume_audio( psInode, false );
			break;
		}

		case CD_STOP:
		{
			nError = cdrom_stop_audio( psInode );
			break;
		}

		case CD_GET_TIME:
		{
			nError = cdrom_get_playback_time( psInode, (cdrom_msf_s*)pArgs );
			break;
		}

		default:
		{
			kerndbg( KERN_WARNING, "atapi_ioctl() unknown command %ld\n", nCommand );
			nError = -ENOSYS;
			break;
		}
	}

	return( nError );
}

