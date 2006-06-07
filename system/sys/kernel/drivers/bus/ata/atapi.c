/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2004 Kristian Van Der Vliet
 *  Copyright (C) 2004 Arno Klenke
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

/* atapi_check_sense()
 *
 * Check the sense data returned from atapi_request_sense() and return one of
 * three possible conditions:
 *
 * SENSE_OK		No error.  Continue the operation.
 * SENSE_RETRY  The drive is in a transitional state, retry the operation.
 * SENSE_FATAL  The drive is not ready.  Stop the operation.
*/
status_t atapi_check_sense( ATAPI_device_s* psDev, ATAPI_sense_s *psSense, bool bQuiet )
{
	int nError;

	kerndbg( KERN_DEBUG_LOW, "Key 0x%04x Asc 0x%04x Ascq 0x%04x\n",
							  psSense->sense_key,
							  psSense->asc,
							  psSense->ascq );

	switch( psSense->sense_key )
	{
		case ATAPI_NO_SENSE:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_NO_SENSE\n");

			/* No problem, carry on */
			nError = SENSE_OK;
			break;
		}

		case ATAPI_RECOVERED_ERROR:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_RECOVERED_ERROR\n");

			/* No problem, carry on */
			nError = SENSE_OK;
			break;
		}

		case ATAPI_NOT_READY:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_NOT_READY\n");

			/* Find out why the drive is not ready */
			nError = atapi_unit_not_ready( psDev, psSense );
			break;
		}

		case ATAPI_MEDIUM_ERROR:
		{
			if( !bQuiet )
				kerndbg(KERN_WARNING, "ATAPI drive has reported ATAPI_MEDIUM_ERROR\n");

			/* Medium errors may be transient, so we should at least try again */
			nError = SENSE_RETRY;
			break;
		}

		case ATAPI_HARDWARE_ERROR:
		{
			kerndbg(KERN_WARNING, "ATAPI drive has reported ATAPI_HARDWARE_ERROR\n");

			/* A hardware error is unlikly to be transient, so give up */
			nError = SENSE_FATAL;
			break;
		}

		case ATAPI_ILLEGAL_REQUEST:
		{
			if( !bQuiet )
				kerndbg(KERN_WARNING, "ATAPI drive has reported ATAPI_ILLEGAL_REQUEST\n");

			/* May be caused by a drive error, a CD-DA error, or an unknown or bad raw packet command */
			nError = SENSE_FATAL;
			break;
		}

		case ATAPI_UNIT_ATTENTION:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_UNIT_ATTENTION\n");

			/* Media changed, make a note of it and try again */
			psDev->bMediaChanged = true;
			psDev->bTocValid = false;
			nError = SENSE_RETRY;
			break;
		}

		case ATAPI_DATA_PROTECT:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_DATA_PROTECT\n");

			/* Not recoverable */
			nError = SENSE_FATAL;
			break;
		}

		case ATAPI_BLANK_CHECK:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_BLANK_CHECK\n");

			/* Not recoverable, unless this driver is updated to support WORM devices */
			nError = SENSE_FATAL;
			break;
		}

		case ATAPI_VENDOR_SPECIFIC:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_VENDOR_SPECIFIC\n");

			/* Not recoverable as we have no way to understand the error */
			nError = SENSE_FATAL;
			break;
		}

		case ATAPI_COPY_ABORTED:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_COPY_ABORTED\n");

			/* Never going to happen */
			nError = SENSE_FATAL;
			break;
		}

		case ATAPI_ABORTED_COMMAND:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_ABORTED_COMMAND\n");

			/* It might work again if we attempt to retry the command */
			nError = SENSE_RETRY;
			break;
		}

		case ATAPI_VOLUME_OVERFLOW:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_VOLUME_OVERFLOW\n");

			/* Currently unhandled.  Unlikely to happen. */
			nError = SENSE_FATAL;
			break;
		}

		case ATAPI_MISCOMPARE:
		{
			kerndbg(KERN_DEBUG_LOW, "ATAPI drive has reported ATAPI_MISCOMPARE\n");

			/* Never going to happen unless this driver is updated to support WORM devices */
			nError = SENSE_FATAL;
			break;
		}

		default:
		{
			kerndbg( KERN_WARNING, "ATAPI drive is reporting an undefined error\n");

			/* The drive did something screwy, possibly a deprecated sense condition, but still.. */
			nError = SENSE_FATAL;
		}
	}

	return nError;
}

/* atapi_unit_not_ready()
 *
 * If the sense key is ATAPI_NOT_READY then this function checks the additional
 * sense key & its qualifier to determine the final sense outcome.  Return:
 *
 * SENSE_OK		No error.  Continue the operation.
 * SENSE_RETRY  The drive is in a transitional state, retry the operation.
 * SENSE_FATAL  The drive is not ready.  Stop the operation.
*/
status_t atapi_unit_not_ready( ATAPI_device_s* psDev, ATAPI_sense_s *psSense )
{
	int nError;

	/* Find out why the unit is not ready */
	switch( psSense->asc )
	{
		case ATAPI_NO_ASC_DATA:
		{
			kerndbg(KERN_DEBUG_LOW, "With additional code ATAPI_NO_ASC_DATA\n");

			/* Probably worth trying again */
			nError = SENSE_RETRY;
			break;
		}

		case ATAPI_LOGICAL_UNIT_NOT_READY:
		{
			kerndbg(KERN_DEBUG_LOW, "With additional code ATAPI_LOGICAL_UNIT_NOT_READY\n");

			/* Find out why it isn't ready */
			switch( psSense->ascq )
			{
				case ATAPI_NOT_REPORTABLE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_NOT_REPORTABLE\n");

					/* We don't know what this error is, may as well try again */
					nError = SENSE_RETRY;
					break;
				}

				case ATAPI_BECOMING_READY:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_BECOMING_READY\n");

					/* Wait for the drive to become ready.  Delay a little to give
					   the drive a chance to spin up */
					udelay(1000);
					nError = SENSE_RETRY;
					break;
				}

				case ATAPI_MUST_INITIALIZE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_MUST_INITIALIZE\n");

					/* Need to send a START/STOP to the drive */
					if( atapi_start( psDev ) < 0 )
					{
						nError = SENSE_FATAL;
					}
					else
						nError = SENSE_RETRY;

					break;
				}

				case ATAPI_MANUAL_INTERVENTION:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_MANUAL_INTERVENTION\n");

					/* Nothing we can do */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_FORMAT_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_FORMAT_IN_PROGRESS\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_REBUILD_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_REBUILD_IN_PROGRESS\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_RECALC_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_RECALC_IN_PROGRESS\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_OP_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_OP_IN_PROGRESS\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_LONG_WRITE_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_LONG_WRITE_IN_PROGRESS\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_SELF_TEST_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_SELF_TEST_IN_PROGRESS\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_ASSYM_ACCESS_STATE_TRANS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_ASSYM_ACCESS_STATE_TRANS\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_TARGET_PORT_STANDBY:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_TARGET_PORT_STANDBY\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_TARGET_PORT_UNAVAILABLE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_TARGET_PORT_UNAVAILABLE\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_AUX_MEM_UNAVAILABLE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_AUX_MEM_UNAVAILABLE\n");

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_NOTIFY_REQUIRED:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_NOTIFY_REQUIRED\n");

					/* Send notify? */
					nError = SENSE_FATAL;
					break;
				}

				default:
				{
					kerndbg( KERN_DEBUG_LOW, "and an unknown qualifier 0x%04x\n", psSense->ascq);

					/* We don't know what this error is */
					nError = SENSE_FATAL;
					break;
				}
			}
			break;
		}

		case ATAPI_NOT_RESPONDING:
		{
			kerndbg(KERN_DEBUG_LOW, "With additional code ATAPI_NOT_RESPONDING\n");

			/* Give up */
			nError = SENSE_FATAL;
			break;
		}

		case ATAPI_MEDIUM:
		{
			kerndbg(KERN_DEBUG_LOW, "With additional code ATAPI_MEDIUM\n");

			/* It helps if the disk is in the drive.  Is it? */
			switch( psSense->ascq )
			{
				case ATAPI_MEDIUM_TRAY_OPEN:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_MEDIUM_TRAY_OPEN\n");

					/* We have the option of sending a START/STOP to close the drive
					  but that could be evil, so we won't */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_MEDIUM_NOT_PRESENT:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_MEDIUM_NOT_PRESENT\n");

					/* Put the disk in the drive, dum dum! */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_MEDIUM_TRAY_CLOSED:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_MEDIUM_TRAY_CLOSED\n");

					/* This is a bit of a silly error really; why is it an error if the tray is closed? */
					nError = SENSE_FATAL;
					break;
				}

				case ATAPI_MEDIUM_LOADABLE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of ATAPI_MEDIUM_LOADABLE\n");

					/* We can't load it ourselves */
					nError = SENSE_FATAL;
					break;
				}

				default:
				{
					kerndbg( KERN_DEBUG_LOW, "and an unknown qualifier 0x%04x\n", psSense->ascq);

					/* We don't know what this error is */
					nError = SENSE_FATAL;
					break;
				}
			}
			break;
		}

		default:
		{
			kerndbg( KERN_WARNING, "With an unknown or unsupported additional code 0x%04x\n", psSense->asc);

			/* The drive did something screwy, possibly a deprecated sense condition, but still.. */
			nError = SENSE_FATAL;
			break;
		}
	}

	return nError;
}

/* Send a REQUEST_SENSE packet and read the response in psSense */
int atapi_request_sense( ATAPI_device_s* psDev, ATAPI_sense_s *psSense )
{
	ATA_cmd_s sCmd;

	kerndbg( KERN_DEBUG_LOW, "Requesting sense data...\n" );

	if( psSense == NULL )
	{
		kerndbg( KERN_PANIC, "atapi_request_sense() called with psSense = NULL\n");
		return( -EINVAL );
	}
	
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );

	sCmd.nDirection = READ;
	sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
	sCmd.nTransferLength = sizeof( ATAPI_sense_s );
		
	sCmd.nCmd[0] = ATAPI_REQUEST_SENSE;
	sCmd.nCmd[4] = sCmd.nTransferLength;

	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 )
	{
		kerndbg( KERN_WARNING, "Could not get ATAPI sense data. Sense key is 0x%04x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	memcpy( psSense, psDev->psPort->pDataBuf, sizeof( ATAPI_sense_s ) );
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}

/* Read capacity of the drive */
status_t atapi_read_capacity( ATAPI_device_s* psDev, unsigned long* pnCapacity )
{
	int nError;
	struct {
		uint32 lba;
		uint32 blocklen;
	} sCap;
	ATA_cmd_s sCmd;
	
retry:
	nError = 0;
	LOCK( psDev->hLock );

	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );

	sCmd.nDirection = READ;
	sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
	sCmd.nTransferLength = sizeof( sCap );
		
	sCmd.nCmd[0] = ATAPI_READ_CDVD_CAPACITY;

	kerndbg( KERN_DEBUG_LOW, "Queuing ATAPI_READ_CDVD_CAPACITY\n");

	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	UNLOCK( psDev->hLock );

	if( sCmd.sSense.sense_key != ATAPI_NO_SENSE || sCmd.nStatus < 0 )
	{
		nError = atapi_request_sense( psDev, &sCmd.sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from ATAPI device, aborting.\n" );
			goto error;
		}

		nError = atapi_check_sense( psDev, &sCmd.sSense, false );
		switch( nError )
		{
			case SENSE_OK:
				break;
			
			case SENSE_RETRY:
				goto retry;

			case SENSE_FATAL:
			{
				kerndbg( KERN_PANIC, "ATAPI device reporting fatal error, aborting.\n");
				goto error;
			}
		}
	}
	
	memcpy( &sCap, psDev->psPort->pDataBuf, sizeof( sCap ) );
	
	psDev->nSize = 1 + be32_to_cpu(sCap.lba);
	psDev->nSectorSize = 2048;
	psDev->nSize *= psDev->nSectorSize;
	
	if( pnCapacity != NULL )
		*pnCapacity = psDev->nSize;
	
	return( 0 );

error:
	psDev->nSize = 0;
	psDev->nSectorSize = 0;

	if( pnCapacity != NULL )
		*pnCapacity = psDev->nSize;

	return( -EIO );
}

/* Test if the drive is ready */
status_t atapi_test_ready( ATAPI_device_s* psDev )
{
	ATA_cmd_s sCmd;
	int nError;

retry:
	nError = 0;
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.nCmd[0] = ATAPI_TEST_UNIT_READY;
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	UNLOCK( psDev->hLock );

	if( sCmd.sSense.sense_key != ATAPI_NO_SENSE || sCmd.nStatus < 0 )
	{
		nError = atapi_request_sense( psDev, &sCmd.sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from ATAPI device, aborting.\n" );
			return( -EIO );
		}

		nError = atapi_check_sense( psDev, &sCmd.sSense, false );
		switch( nError )
		{
			case SENSE_OK:
				break;
			
			case SENSE_RETRY:
				goto retry;

			case SENSE_FATAL:
			{
				kerndbg( KERN_PANIC, "ATAPI device reporting fatal error, aborting.\n");
				return( -EIO );
			}
		}
	}

	return( nError );
}

status_t atapi_do_start_stop( ATAPI_device_s* psDev, int nFlags )
{
	ATA_cmd_s sCmd;
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );

	sCmd.nCmd[0] = ATAPI_START_STOP_UNIT;
	sCmd.nCmd[4] = nFlags;
	
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

status_t atapi_start( ATAPI_device_s* psDev )
{
	return atapi_do_start_stop( psDev, 0x01 );
}

status_t atapi_eject( ATAPI_device_s* psDev )
{
	return atapi_do_start_stop( psDev, 0x02 );
}

status_t atapi_read_toc_entry( ATAPI_device_s* psDev, int trackno, int msf_flag, int format, char *buf, int buflen)
{
	ATA_cmd_s sCmd;
	int nError;

retry:
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );

	sCmd.nDirection = READ;
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

	UNLOCK( psDev->hLock );

	if( sCmd.sSense.sense_key != ATAPI_NO_SENSE || sCmd.nStatus < 0 )
	{
		nError = atapi_request_sense( psDev, &sCmd.sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from ATAPI device, aborting.\n" );
			return( -EIO );
		}

		nError = atapi_check_sense( psDev, &sCmd.sSense, false );
		switch( nError )
		{
			case SENSE_OK:
				break;
			
			case SENSE_RETRY:
				goto retry;

			case SENSE_FATAL:
			{
				kerndbg( KERN_PANIC, "ATAPI device reporting fatal error, aborting.\n");
				return( -EIO );
			}
		}
	}

	memcpy( buf, psDev->psPort->pDataBuf, buflen );
	
	return( 0 );
}

status_t atapi_read_toc( ATAPI_device_s* psDev, cdrom_toc_s *toc )
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
	stat = atapi_read_toc_entry( psDev, 0, 1, 0, (char *) &toc->hdr,  sizeof(cdrom_toc_header_s));
	if( stat < 0 )
		return( stat );

	ntracks = toc->hdr.last_track - toc->hdr.first_track + 1;
	if (ntracks <= 0)
		return -EIO;
	if (ntracks > MAX_TRACKS)
		ntracks = MAX_TRACKS;
		
	/* Now read the whole schmeer. */
	stat = atapi_read_toc_entry( psDev, toc->hdr.first_track, 1, 0, (char *)&toc->hdr, sizeof(cdrom_toc_header_s) + (ntracks + 1) *  sizeof(cdrom_toc_entry_s));

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
		stat = atapi_read_toc_entry( psDev, CDROM_LEADOUT, 1, 0, (char *)&toc->hdr, sizeof(cdrom_toc_header_s) + (ntracks + 1) * sizeof(cdrom_toc_entry_s));
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
		stat = atapi_read_toc_entry(psDev, 0, 1, 1, (char *)&ms_tmp, sizeof(ms_tmp));
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

	stat = atapi_read_capacity(psDev, &toc->capacity);
	if ( stat < 0 )
		toc->capacity = 0x1fffff;
	
	return( 0 );
}

status_t atapi_play_audio( ATAPI_device_s* psDev, int lba_start, int lba_end)
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
		kerndbg( KERN_WARNING, "Error: Could not play audio. Sense key is 0x%04x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}

status_t atapi_pause_resume_audio( ATAPI_device_s* psDev, bool bPause )
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
		kerndbg( KERN_WARNING, "Error: Could not pause audio. Sense key is 0x%04x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}

status_t atapi_stop_audio( ATAPI_device_s *psDev )
{
	ATA_cmd_s sCmd;
	int nError;
	
	nError = atapi_pause_resume_audio( psDev, true );
	
	LOCK( psDev->hLock );
	
	/* Create and queue request sense command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	sCmd.nCmd[0] = ATAPI_STOP_AUDIO;
	
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	
	ata_cmd_free( &sCmd );
	
	if( sCmd.nStatus < 0 ) {
		kerndbg( KERN_WARNING, "Error: Could not stop audio. Sense key is 0x%04x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	UNLOCK( psDev->hLock );
	
	return( 0 );
}

status_t atapi_get_playback_time( ATAPI_device_s *psDev, cdrom_msf_s *pnTime )
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

	sCmd.nDirection = READ;
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
		kerndbg( KERN_WARNING, "Error: Could not get time. Sense key is 0x%04x\n", sCmd.sSense.sense_key );
		UNLOCK( psDev->hLock );
		return( -EIO );
	}
	
	memcpy( &posbuf, psDev->psPort->pDataBuf, sizeof( posbuf ) );
	
	UNLOCK( psDev->hLock );
	
	lba_to_msf( be32_to_cpu( posbuf.rel_addr ), &pnTime->minute, &pnTime->second, &pnTime->frame );

	return( 0 );
}

status_t atapi_do_read( ATAPI_device_s *psDev, uint64 nBlock, int nSize )
{
	ATA_cmd_s sCmd;
	uint32 nBlockCount = nSize / psDev->nSectorSize;
	int nError;

	kerndbg( KERN_DEBUG_LOW, "Reading %i data blocks from block %Li\n",(int)nBlockCount, nBlock );

#if 0
	if( atapi_test_ready( psDev ) < 0 )
	{
		kerndbg( KERN_DEBUG, "%s failed, device not ready.\n", __FUNCTION__ );
		goto error;
	}
#endif

retry:
	nError = 0;
	LOCK( psDev->hLock );

	/* Prepare read command */
	ata_cmd_init( &sCmd, psDev->psPort );

	sCmd.nDirection = READ;
	sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
	sCmd.nTransferLength = nSize;
	
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
	ata_cmd_free( &sCmd );

	UNLOCK( psDev->hLock );
		
	if( sCmd.sSense.sense_key != ATAPI_NO_SENSE || sCmd.nStatus < 0 )
	{
		nError = atapi_request_sense( psDev, &sCmd.sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from ATAPI device, aborting.\n" );
			goto error;
		}

		nError = atapi_check_sense( psDev, &sCmd.sSense, false );
		switch( nError )
		{
			case SENSE_OK:
				break;
			
			case SENSE_RETRY:
				goto retry;

			case SENSE_FATAL:
			{
				kerndbg( KERN_PANIC, "ATAPI device reporting fatal error, aborting.\n");
				goto error;
			}
		}
	}

	return 0;

error:
	return( -EIO );
}

status_t atapi_do_cdda_read( ATAPI_device_s *psDev, uint64 nBlock, int nSize )
{
	ATA_cmd_s sCmd;
	uint32 nBlockCount = nSize / psDev->nSectorSize;
	int nError;

	kerndbg( KERN_DEBUG_LOW, "Reading %i CD-DA blocks from block %Li\n", (int)nBlockCount, nBlock );

retry:
	nError = 0;
	LOCK( psDev->hLock );

	/* Prepare read command */
	ata_cmd_init( &sCmd, psDev->psPort );

	sCmd.nDirection = READ;
	sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
	sCmd.nTransferLength = nSize;
	
	sCmd.nCmd[0] = ATAPI_READ_CD;
	sCmd.nCmd[1] = 0x04;	/* Expect CD-DA only, DAP cleared */
	sCmd.nCmd[2] = ( nBlock >> 24 ) & 0xff;
	sCmd.nCmd[3] = ( nBlock >> 16 ) & 0xff;
	sCmd.nCmd[4] = ( nBlock >> 8 ) & 0xff;
	sCmd.nCmd[5] = ( nBlock >> 0 ) & 0xff;
	sCmd.nCmd[6] = ( nBlockCount >> 16 ) & 0xff;
	sCmd.nCmd[7] = ( nBlockCount >> 8 ) & 0xff;
	sCmd.nCmd[8] = ( nBlockCount >> 0 ) & 0xff;
	sCmd.nCmd[9] = 0xf8;	/* Set all main-channel selection bits */

	/* Queue and wait */
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	ata_cmd_free( &sCmd );

	UNLOCK( psDev->hLock );

	if( sCmd.sSense.sense_key != ATAPI_NO_SENSE || sCmd.nStatus < 0 )
	{
		nError = atapi_request_sense( psDev, &sCmd.sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from ATAPI device, aborting.\n" );
			goto error;
		}

		nError = atapi_check_sense( psDev, &sCmd.sSense, false );
		switch( nError )
		{
			case SENSE_OK:
				break;
			
			case SENSE_RETRY:
				goto retry;

			case SENSE_FATAL:
			{
				kerndbg( KERN_PANIC, "ATAPI device reporting fatal error, aborting.\n");
				goto error;
			}
		}
	}

	return 0;

error:
	return( -EIO );
}

status_t atapi_do_packet_command( ATAPI_device_s *psDev, cdrom_packet_cmd_s *psRawCmd )
{
	ATA_cmd_s sCmd;
	int nError;

retry:
	nError = 0;
	LOCK( psDev->hLock );

	/* Prepare ATAPI command */
	ata_cmd_init( &sCmd, psDev->psPort );
	
	memcpy_from_user( sCmd.nCmd, psRawCmd->nCommand, psRawCmd->nCommandLength );
	kerndbg( KERN_DEBUG_LOW, "Raw packet command: 0x%02x\n", sCmd.nCmd[0] );

	if( psRawCmd->nDataLength > 0 )
	{
		if( psRawCmd->nDirection == WRITE )
		{
			kerndbg( KERN_DEBUG, "Transfer %i bytes TO device\n", psRawCmd->nDataLength );
			memcpy_from_user( psDev->psPort->pDataBuf, psRawCmd->pnData, psRawCmd->nDataLength );
		}
		else
			kerndbg( KERN_DEBUG, "Transfer %i bytes FROM device\n", psRawCmd->nDataLength );
	}
	else
		kerndbg( KERN_DEBUG, "No data to transfer\n" );

	sCmd.nDirection = psRawCmd->nDirection;
	sCmd.pTransferBuffer = psDev->psPort->pDataBuf;
	sCmd.nTransferLength = psRawCmd->nDataLength;
	
	/* Disable DMA on commands that are known to be bad */
 	switch( sCmd.nCmd[0] )
 	{
		case ATAPI_READ_CD_12:
 			sCmd.bCanDMA = false;
	}

	/* Queue and wait */
	ata_cmd_queue( psDev->psPort, &sCmd );
	LOCK( sCmd.hWait );
	ata_cmd_free( &sCmd );

	

	psRawCmd->nError = sCmd.nStatus;
	psRawCmd->nSense = SENSE_OK;

	if( sCmd.sSense.sense_key != ATAPI_NO_SENSE || sCmd.nStatus < 0 )
	{
		nError = atapi_request_sense( psDev, &sCmd.sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from ATAPI device, aborting.\n" );
			goto error;
		}

		memcpy_to_user( &psRawCmd->pnSense, &sCmd.sSense, psRawCmd->nSenseLength );

		psRawCmd->nSense = nError = atapi_check_sense( psDev, &sCmd.sSense, true );
		switch( psRawCmd->nSense )
		{
			case SENSE_OK:
				break;
			
			case SENSE_RETRY:
				goto retry;

			case SENSE_FATAL:
			{
				kerndbg( KERN_DEBUG, "ATAPI device reporting fatal error, aborting.\n");

				/* Dump cmd bytes */
				kerndbg( KERN_DEBUG_LOW, "cmd data: 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n",
						sCmd.nCmd[0], sCmd.nCmd[1], sCmd.nCmd[2], sCmd.nCmd[3], 
						sCmd.nCmd[4], sCmd.nCmd[5], sCmd.nCmd[6], sCmd.nCmd[7], 
						sCmd.nCmd[8], sCmd.nCmd[9], sCmd.nCmd[10], sCmd.nCmd[11] );
				goto error;
			}
		}
	}

	if( psRawCmd->nSense == SENSE_OK &&	psRawCmd->nDirection == READ && psRawCmd->nDataLength > 0 )
		memcpy_to_user( psRawCmd->pnData, psDev->psPort->pDataBuf, psRawCmd->nDataLength );

	UNLOCK( psDev->hLock );

	return 0;

error:
	UNLOCK( psDev->hLock );
	return( -EIO );
}

