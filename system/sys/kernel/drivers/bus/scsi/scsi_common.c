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

//#undef DEBUG_LIMIT
//#define DEBUG_LIMIT   KERN_DEBUG_LOW

const unsigned char nSCSICmdSize[8] = {
	6, 10, 10, 12,
	16, 12, 10, 10
};

unsigned char scsi_get_command_size( int nOpcode )
{
	return ( nSCSICmdSize[( ( nOpcode ) >> 5 ) & 7] );
}

/* Send a REQUEST_SENSE packet and read the response in psSense */
status_t scsi_request_sense( SCSI_device_s * psDevice, SCSI_sense_s * psSense )
{
	SCSI_cmd_s sCmd;

	kerndbg( KERN_DEBUG_LOW, "Requesting sense data...\n" );

	if( psSense == NULL )
	{
		kerndbg( KERN_PANIC, "scsi_request_sense() called with psSense = NULL\n" );
		return -EINVAL;
	}

	/* Create and queue request sense command */
	scsi_init_cmd( &sCmd, psDevice );

	sCmd.nDirection = SCSI_DATA_READ;
	sCmd.pRequestBuffer = (void*)psSense;
	sCmd.nRequestSize = sizeof( psSense );

	sCmd.nCmd[0] = SCSI_REQUEST_SENSE;
	sCmd.nCmd[4] = sCmd.nRequestSize;

	sCmd.nCmdLen = scsi_get_command_size( SCSI_REQUEST_SENSE );

	psDevice->psHost->queue_command( &sCmd );

	if( sCmd.s.sSense.sense_key != SCSI_NO_SENSE || sCmd.nResult < 0 )
	{
		kerndbg( KERN_WARNING, "Could not get SCSI sense data. Sense key is 0x%04x\n", ( sCmd.s.nSense[2] & 0xf0 ) );
		return -EIO;
	}

	return 0;
}

/* scsi_check_sense()
 *
 * Check the sense data returned from scsi_request_sense() and return one of
 * three possible conditions:
 *
 * SENSE_OK		No error.  Continue the operation.
 * SENSE_RETRY  The drive is in a transitional state, retry the operation.
 * SENSE_FATAL  The drive is not ready.  Stop the operation.
*/
status_t scsi_check_sense( SCSI_device_s * psDevice, SCSI_sense_s * psSense, bool bQuiet )
{
	int nError;

	kerndbg( KERN_DEBUG_LOW, "Key 0x%04x Asc 0x%04x Ascq 0x%04x\n", psSense->sense_key, psSense->asc, psSense->ascq );

	switch ( psSense->sense_key )
	{
		case SCSI_NO_SENSE:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_NO_SENSE\n" );

			/* No problem, carry on */
			nError = SENSE_OK;
			break;
		}

		case SCSI_RECOVERED_ERROR:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_RECOVERED_ERROR\n" );

			/* No problem, carry on */
			nError = SENSE_OK;
			break;
		}

		case SCSI_NOT_READY:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_NOT_READY\n" );

			/* Find out why the drive is not ready */
			nError = scsi_unit_not_ready( psDevice, psSense );
			break;
		}

		case SCSI_MEDIUM_ERROR:
		{
			if( !bQuiet )
				kerndbg( KERN_WARNING, "SCSI drive has reported SCSI_MEDIUM_ERROR\n" );

			/* Medium errors may be transient, so we should at least try again */
			nError = SENSE_RETRY;
			break;
		}

		case SCSI_HARDWARE_ERROR:
		{
			kerndbg( KERN_WARNING, "SCSI drive has reported SCSI_HARDWARE_ERROR\n" );

			/* A hardware error is unlikly to be transient, so give up */
			nError = SENSE_FATAL;
			break;
		}

		case SCSI_ILLEGAL_REQUEST:
		{
			if( !bQuiet )
				kerndbg( KERN_WARNING, "SCSI drive has reported SCSI_ILLEGAL_REQUEST\n" );

			/* May be caused by a drive error, a CD-DA error, or an unknown or bad raw packet command */
			nError = SENSE_FATAL;
			break;
		}

		case SCSI_UNIT_ATTENTION:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_UNIT_ATTENTION\n" );

			/* Media changed, make a note of it and try again */
			psDevice->bMediaChanged = true;
			psDevice->bTocValid = false;
			nError = SENSE_RETRY;
			break;
		}

		case SCSI_DATA_PROTECT:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_DATA_PROTECT\n" );

			/* Not recoverable */
			nError = SENSE_FATAL;
			break;
		}

		case SCSI_BLANK_CHECK:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_BLANK_CHECK\n" );

			/* Not recoverable, unless this driver is updated to support WORM devices */
			nError = SENSE_FATAL;
			break;
		}

		case SCSI_VENDOR_SPECIFIC:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_VENDOR_SPECIFIC\n" );

			/* Not recoverable as we have no way to understand the error */
			nError = SENSE_FATAL;
			break;
		}

		case SCSI_COPY_ABORTED:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_COPY_ABORTED\n" );

			/* Never going to happen */
			nError = SENSE_FATAL;
			break;
		}

		case SCSI_ABORTED_COMMAND:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_ABORTED_COMMAND\n" );

			/* It might work again if we attempt to retry the command */
			nError = SENSE_RETRY;
			break;
		}

		case SCSI_VOLUME_OVERFLOW:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_VOLUME_OVERFLOW\n" );

			/* Currently unhandled.  Unlikely to happen. */
			nError = SENSE_FATAL;
			break;
		}

		case SCSI_MISCOMPARE:
		{
			kerndbg( KERN_DEBUG_LOW, "SCSI drive has reported SCSI_MISCOMPARE\n" );

			/* Never going to happen unless this driver is updated to support WORM devices */
			nError = SENSE_FATAL;
			break;
		}

		default:
		{
			kerndbg( KERN_WARNING, "SCSI drive is reporting an undefined error\n" );

			/* The drive did something screwy, possibly a deprecated sense condition, but still.. */
			nError = SENSE_FATAL;
		}
	}

	return nError;
}

/* scsi_unit_not_ready()
 *
 * If the sense key is SCSI_NOT_READY then this function checks the additional
 * sense key & its qualifier to determine the final sense outcome.  Return:
 *
 * SENSE_OK		No error.  Continue the operation.
 * SENSE_RETRY  The drive is in a transitional state, retry the operation.
 * SENSE_FATAL  The drive is not ready.  Stop the operation.
*/
status_t scsi_unit_not_ready( SCSI_device_s * psDevice, SCSI_sense_s * psSense )
{
	int nError;

	/* Find out why the unit is not ready */
	switch ( psSense->asc )
	{
		case SCSI_NO_ASC_DATA:
		{
			kerndbg( KERN_DEBUG_LOW, "With additional code SCSI_NO_ASC_DATA\n" );

			/* Probably worth trying again */
			nError = SENSE_RETRY;
			break;
		}

		case SCSI_LOGICAL_UNIT_NOT_READY:
		{
			kerndbg( KERN_DEBUG_LOW, "With additional code SCSI_LOGICAL_UNIT_NOT_READY\n" );

			/* Find out why it isn't ready */
			switch ( psSense->ascq )
			{
				case SCSI_NOT_REPORTABLE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_NOT_REPORTABLE\n" );

					/* We don't know what this error is, may as well try again */
					nError = SENSE_RETRY;
					break;
				}

				case SCSI_BECOMING_READY:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_BECOMING_READY\n" );

					/* Wait for the drive to become ready.  Delay a little to give
					   the drive a chance to spin up */
					udelay( 1000 );
					nError = SENSE_RETRY;
					break;
				}

				case SCSI_MUST_INITIALIZE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_MUST_INITIALIZE\n" );
					/* Need to send a START/STOP to the drive */
					if( scsi_start( psDevice ) < 0 )
					{
						nError = SENSE_FATAL;
					}
					else
						nError = SENSE_RETRY;

					break;
				}

				case SCSI_MANUAL_INTERVENTION:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_MANUAL_INTERVENTION\n" );

					/* Nothing we can do */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_FORMAT_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_FORMAT_IN_PROGRESS\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_REBUILD_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_REBUILD_IN_PROGRESS\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_RECALC_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_RECALC_IN_PROGRESS\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_OP_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_OP_IN_PROGRESS\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_LONG_WRITE_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_LONG_WRITE_IN_PROGRESS\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_SELF_TEST_IN_PROGRESS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_SELF_TEST_IN_PROGRESS\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_ASSYM_ACCESS_STATE_TRANS:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_ASSYM_ACCESS_STATE_TRANS\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_TARGET_PORT_STANDBY:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_TARGET_PORT_STANDBY\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_TARGET_PORT_UNAVAILABLE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_TARGET_PORT_UNAVAILABLE\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_AUX_MEM_UNAVAILABLE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_AUX_MEM_UNAVAILABLE\n" );

					/* Should never happen */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_NOTIFY_REQUIRED:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_NOTIFY_REQUIRED\n" );

					/* Send notify? */
					nError = SENSE_FATAL;
					break;
				}

				default:
				{
					kerndbg( KERN_DEBUG_LOW, "and an unknown qualifier 0x%04x\n", psSense->ascq );

					/* We don't know what this error is */
					nError = SENSE_FATAL;
					break;
				}
			}
			break;
		}

		case SCSI_NOT_RESPONDING:
		{
			kerndbg( KERN_DEBUG_LOW, "With additional code SCSI_NOT_RESPONDING\n" );

			/* Give up */
			nError = SENSE_FATAL;
			break;
		}

		case SCSI_MEDIUM:
		{
			kerndbg( KERN_DEBUG_LOW, "With additional code SCSI_MEDIUM\n" );

			/* It helps if the disk is in the drive.  Is it? */
			switch ( psSense->ascq )
			{
				case SCSI_MEDIUM_TRAY_OPEN:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_MEDIUM_TRAY_OPEN\n" );

					/* We have the option of sending a START/STOP to close the drive
					   but that could be evil, so we won't */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_MEDIUM_NOT_PRESENT:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_MEDIUM_NOT_PRESENT\n" );

					/* Put the disk in the drive, dum dum! */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_MEDIUM_TRAY_CLOSED:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_MEDIUM_TRAY_CLOSED\n" );

					/* This is a bit of a silly error really; why is it an error if the tray is closed? */
					nError = SENSE_FATAL;
					break;
				}

				case SCSI_MEDIUM_LOADABLE:
				{
					kerndbg( KERN_DEBUG_LOW, "and a qualifier of SCSI_MEDIUM_LOADABLE\n" );

					/* We can't load it ourselves */
					nError = SENSE_FATAL;
					break;
				}

				default:
				{
					kerndbg( KERN_DEBUG_LOW, "and an unknown qualifier 0x%04x\n", psSense->ascq );

					/* We don't know what this error is */
					nError = SENSE_FATAL;
					break;
				}
			}
			break;
		}

		default:
		{
			kerndbg( KERN_WARNING, "With an unknown or unsupported additional code 0x%04x\n", psSense->asc );

			/* The drive did something screwy, possibly a deprecated sense condition, but still.. */
			nError = SENSE_FATAL;
			break;
		}
	}

	return nError;
}

/* Test if the drive is ready */
status_t scsi_test_ready( SCSI_device_s * psDevice )
{
	SCSI_cmd_s sCmd;
	int nError;

retry:
	nError = 0;

	/* Try to test the state of the unit */
	scsi_init_cmd( &sCmd, psDevice );

	sCmd.nCmd[0] = SCSI_TEST_UNIT_READY;
	if( psDevice->nSCSILevel <= SCSI_2 )
		sCmd.nCmd[1] = ( psDevice->nLun << 5 ) & 0xe0;

	sCmd.nCmdLen = scsi_get_command_size( SCSI_TEST_UNIT_READY );

	/* Send command */
	nError = psDevice->psHost->queue_command( &sCmd );

	if( sCmd.s.sSense.sense_key != SCSI_NO_SENSE || sCmd.nResult != 0 )
	{
		SCSI_sense_s sSense;

		nError = scsi_request_sense( psDevice, &sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from SCSI device, aborting.\n" );
			return -EIO;
		}

		nError = scsi_check_sense( psDevice, &sSense, false );
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

	kerndbg( KERN_DEBUG, "scsi_test_ready(): Device ready\n" );

	return 0;
}

/* Read capacity of the drive */
status_t scsi_read_capacity( SCSI_device_s * psDevice, unsigned long *pnCapacity )
{
	int nError;
	struct
	{
		uint32 lba;
		uint32 blocklen;
	} sCap;
	SCSI_cmd_s sCmd;

      retry:
	nError = 0;

	scsi_init_cmd( &sCmd, psDevice );

	sCmd.nDirection = SCSI_DATA_READ;
	sCmd.pRequestBuffer = (void*)&sCap;
	sCmd.nRequestSize = sizeof( sCap );

	sCmd.nCmd[0] = SCSI_READ_CAPACITY;
	sCmd.nCmd[1] = ( psDevice->nLun << 5 ) & 0xe0;

	sCmd.nCmdLen = scsi_get_command_size( SCSI_READ_CAPACITY );

	/* Send command */
	nError = psDevice->psHost->queue_command( &sCmd );

	if( sCmd.s.sSense.sense_key != SCSI_NO_SENSE || sCmd.nResult != 0 )
	{
		SCSI_sense_s sSense;

		nError = scsi_request_sense( psDevice, &sSense );
		if( nError < 0 )
		{
			kerndbg( KERN_PANIC, "Unable to request sense data from SCSI device, aborting.\n" );
			goto error;
		}

		nError = scsi_check_sense( psDevice, &sSense, false );
		switch ( nError )
		{
			case SENSE_OK:
				break;

			case SENSE_RETRY:
				goto retry;

			case SENSE_FATAL:
			{
				kerndbg( KERN_PANIC, "SCSI device reporting fatal error, aborting.\n" );
				goto error;
			}
		}
	}

	psDevice->nSectorSize = be32_to_cpu( sCap.blocklen );
	if( psDevice->nSectorSize > 2048 )
		psDevice->nSectorSize = 2048;

	psDevice->nSectors = 1 + be32_to_cpu( sCap.lba );
	psDevice->nSize = psDevice->nSectors * psDevice->nSectorSize;

	if( pnCapacity != NULL )
		*pnCapacity = psDevice->nSize;

	kerndbg( KERN_DEBUG, "scsi_read_capacity(): %d bytes\n", psDevice->nSize );

	return 0;

error:
	psDevice->nSectors = 0;
	psDevice->nSectorSize = 0;
	psDevice->nSize = 0;

	if( pnCapacity != NULL )
		*pnCapacity = psDevice->nSize;

	return -EIO;
}

static status_t scsi_do_start_stop( SCSI_device_s * psDevice, int nFlags )
{
	SCSI_cmd_s sCmd;
	status_t nError = 0;

	/* Create and queue request sense command */
	scsi_init_cmd( &sCmd, psDevice );

	sCmd.nCmd[0] = SCSI_START_STOP;
	sCmd.nCmd[4] = nFlags;

	sCmd.nCmdLen = scsi_get_command_size( SCSI_START_STOP );

	/* Send command */
	nError = psDevice->psHost->queue_command( &sCmd );

	if( sCmd.nResult < 0 || nError != 0 )
	{
		kerndbg( KERN_WARNING, "Error: Could not eject drive. Sense key is %x\n", sCmd.s.sSense.sense_key );
		nError = -EIO;
	}

	return nError;
}

status_t scsi_start( SCSI_device_s * psDevice )
{
	return scsi_do_start_stop( psDevice, 0x01 );
}

status_t scsi_eject( SCSI_device_s * psDevice )
{
	return scsi_do_start_stop( psDevice, 0x02 );
}

