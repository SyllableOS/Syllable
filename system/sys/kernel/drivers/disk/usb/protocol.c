
/* Driver for USB Mass Storage compliant devices
 *
 * $Id$
 *
 * Current development and maintenance by:
 *   (c) 1999, 2000 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
 *
 * Developed with the assistance of:
 *   (c) 2000 David L. Brown, Jr. (usb-storage@davidb.org)
 *
 * Initial work by:
 *   (c) 1999 Michael Gee (michael@linuxspecific.com)
 *
 * This driver is based on the 'USB Mass Storage Class' document. This
 * describes in detail the protocol used to communicate with such
 * devices.  Clearly, the designers had SCSI and ATAPI commands in
 * mind when they created this document.  The commands are all very
 * similar to commands in the SCSI-II and ATAPI specifications.
 *
 * It is important to note that in a number of cases this class
 * exhibits class-specific exemptions from the USB specification.
 * Notably the usage of NAK, STALL and ACK differs from the norm, in
 * that they are used to communicate wait, failed and OK on commands.
 *
 * Also, for certain devices, the interrupt endpoint is used to convey
 * status of a command.
 *
 * Please see http://www.one-eyed-alien.net/~mdharm/linux-usb for more
 * information about this driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "protocol.h"
#include "usb_disk.h"
#include "transport.h"
#include "scsi_convert.h"

/***********************************************************************
 * Helper routines
 ***********************************************************************/

/* Fix-up the return data from an INQUIRY command to show 
 * ANSI SCSI rev 2 so we don't confuse the SCSI layers above us
 */
void fix_inquiry_data( SCSI_cmd_s * srb )
{
	unsigned char *data_ptr;

	/* verify that it's an INQUIRY command */
	if ( srb->nCmd[0] != SCSI_INQUIRY )
		return;

	//printk("Fixing INQUIRY data to show SCSI rev 2\n");

	/* find the location of the data */
	data_ptr = ( unsigned char * )srb->pRequestBuffer;

	/* Change the SCSI revision number */
	data_ptr[2] = ( data_ptr[2] & ~7 ) | 2;
}

/***********************************************************************
 * Protocol routines
 ***********************************************************************/

void usb_stor_qic157_command( SCSI_cmd_s * srb, USB_disk_s * psDisk )
{
	/* Pad the ATAPI command with zeros 
	 * NOTE: This only works because a Scsi_Cmnd struct field contains
	 * a unsigned char cmnd[12], so we know we have storage available
	 */
	for ( ; srb->nCmdLen < 12; srb->nCmdLen++ )
		srb->nCmd[srb->nCmdLen] = 0;

	/* set command length to 12 bytes */
	srb->nCmdLen = 12;

	/* send the command to the transport layer */
	usb_stor_invoke_transport( srb, psDisk );

	/* fix the INQUIRY data if necessary */
	fix_inquiry_data( srb );
}

void usb_stor_ATAPI_command( SCSI_cmd_s * srb, USB_disk_s * psDisk )
{
	int old_cmnd = 0;

	/* Fix some commands -- this is a form of mode translation
	 * ATAPI devices only accept 12 byte long commands 
	 *
	 * NOTE: This only works because a Scsi_Cmnd struct field contains
	 * a unsigned char cmnd[12], so we know we have storage available
	 */

	/* Pad the ATAPI command with zeros */
	for ( ; srb->nCmdLen < 12; srb->nCmdLen++ )
		srb->nCmd[srb->nCmdLen] = 0;

	/* set command length to 12 bytes */
	srb->nCmdLen = 12;

	/* determine the correct (or minimum) data length for these commands */
	switch ( srb->nCmd[0] )
	{

		/* change MODE_SENSE/MODE_SELECT from 6 to 10 byte commands */
	case SCSI_MODE_SENSE:
	case SCSI_MODE_SELECT:
		/* save the command so we can tell what it was */
		old_cmnd = srb->nCmd[0];

		srb->nCmd[11] = 0;
		srb->nCmd[10] = 0;
		srb->nCmd[9] = 0;
		srb->nCmd[8] = srb->nCmd[4];
		srb->nCmd[7] = 0;
		srb->nCmd[6] = 0;
		srb->nCmd[5] = 0;
		srb->nCmd[4] = 0;
		srb->nCmd[3] = 0;
		srb->nCmd[2] = srb->nCmd[2];
		srb->nCmd[1] = srb->nCmd[1];
		srb->nCmd[0] = srb->nCmd[0] | 0x40;
		break;

		/* change READ_6/WRITE_6 to READ_10/WRITE_10, which 
		 * are ATAPI commands */
	case SCSI_WRITE_6:
	case SCSI_READ_6:
		srb->nCmd[11] = 0;
		srb->nCmd[10] = 0;
		srb->nCmd[9] = 0;
		srb->nCmd[8] = srb->nCmd[4];
		srb->nCmd[7] = 0;
		srb->nCmd[6] = 0;
		srb->nCmd[5] = srb->nCmd[3];
		srb->nCmd[4] = srb->nCmd[2];
		srb->nCmd[3] = srb->nCmd[1] & 0x1F;
		srb->nCmd[2] = 0;
		srb->nCmd[1] = srb->nCmd[1] & 0xE0;
		srb->nCmd[0] = srb->nCmd[0] | 0x20;
		break;
	}			/* end switch on cmnd[0] */

	/* convert MODE_SELECT data here */
	if ( old_cmnd == SCSI_MODE_SELECT )
		usb_stor_scsiSense6to10( srb );

	/* send the command to the transport layer */
	usb_stor_invoke_transport( srb, psDisk );

	/* Fix the MODE_SENSE data if we translated the command */
	if ( ( old_cmnd == SCSI_MODE_SENSE ) && ( SCSI_STATUS( srb->nResult ) == SCSI_GOOD ) )
		usb_stor_scsiSense10to6( srb );

	/* fix the INQUIRY data if necessary */
	fix_inquiry_data( srb );
}


void usb_stor_ufi_command( SCSI_cmd_s * srb, USB_disk_s * psDisk )
{
	int old_cmnd = 0;

	/* fix some commands -- this is a form of mode translation
	 * UFI devices only accept 12 byte long commands 
	 *
	 * NOTE: This only works because a Scsi_Cmnd struct field contains
	 * a unsigned char cmnd[12], so we know we have storage available
	 */

	/* set command length to 12 bytes (this affects the transport layer) */
	srb->nCmdLen = 12;

	/* determine the correct (or minimum) data length for these commands */
	switch ( srb->nCmd[0] )
	{

		/* for INQUIRY, UFI devices only ever return 36 bytes */
	case SCSI_INQUIRY:
		srb->nCmd[4] = 36;
		break;

		/* change MODE_SENSE/MODE_SELECT from 6 to 10 byte commands */
	case SCSI_MODE_SENSE:
	case SCSI_MODE_SELECT:
		/* save the command so we can tell what it was */
		old_cmnd = srb->nCmd[0];

		srb->nCmd[11] = 0;
		srb->nCmd[10] = 0;
		srb->nCmd[9] = 0;

		/* if we're sending data, we send all.  If getting data, 
		 * get the minimum */
		if ( srb->nCmd[0] == SCSI_MODE_SELECT )
			srb->nCmd[8] = srb->nCmd[4];
		else
			srb->nCmd[8] = 8;

		srb->nCmd[7] = 0;
		srb->nCmd[6] = 0;
		srb->nCmd[5] = 0;
		srb->nCmd[4] = 0;
		srb->nCmd[3] = 0;
		srb->nCmd[2] = srb->nCmd[2];
		srb->nCmd[1] = srb->nCmd[1];
		srb->nCmd[0] = srb->nCmd[0] | 0x40;
		break;

		/* again, for MODE_SENSE_10, we get the minimum (8) */
	case SCSI_MODE_SENSE_10:
		srb->nCmd[7] = 0;
		srb->nCmd[8] = 8;
		break;

		/* for REQUEST_SENSE, UFI devices only ever return 18 bytes */
	case SCSI_REQUEST_SENSE:
		srb->nCmd[4] = 18;
		break;

		/* change READ_6/WRITE_6 to READ_10/WRITE_10, which 
		 * are UFI commands */
	case SCSI_WRITE_6:
	case SCSI_READ_6:
		srb->nCmd[11] = 0;
		srb->nCmd[10] = 0;
		srb->nCmd[9] = 0;
		srb->nCmd[8] = srb->nCmd[4];
		srb->nCmd[7] = 0;
		srb->nCmd[6] = 0;
		srb->nCmd[5] = srb->nCmd[3];
		srb->nCmd[4] = srb->nCmd[2];
		srb->nCmd[3] = srb->nCmd[1] & 0x1F;
		srb->nCmd[2] = 0;
		srb->nCmd[1] = srb->nCmd[1] & 0xE0;
		srb->nCmd[0] = srb->nCmd[0] | 0x20;
		break;
	}			/* end switch on cmnd[0] */

	/* convert MODE_SELECT data here */
	if ( old_cmnd == SCSI_MODE_SELECT )
		usb_stor_scsiSense6to10( srb );

	/* send the command to the transport layer */
	usb_stor_invoke_transport( srb, psDisk );

	/* Fix the MODE_SENSE data if we translated the command */
	if ( ( old_cmnd == SCSI_MODE_SENSE ) && ( SCSI_STATUS( srb->nResult ) == SCSI_GOOD ) )
		usb_stor_scsiSense10to6( srb );

	/* Fix the data for an INQUIRY, if necessary */
	fix_inquiry_data( srb );
}

void usb_stor_transparent_scsi_command( SCSI_cmd_s * srb, USB_disk_s * psDisk )
{
	/* send the command to the transport layer */
	usb_stor_invoke_transport( srb, psDisk );


	/* fix the INQUIRY data if necessary */
	fix_inquiry_data( srb );
}
