
/* Driver for USB Mass Storage compliant devices
 * SCSI layer glue code
 *
 * $Id$
 *
 * Current development and maintenance by:
 *   (c) 1999, 2000 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
 *
 * Developed with the assistance of:
 *   (c) 2000 David L. Brown, Jr. (usb-storage@davidb.org)
 *   (c) 2000 Stephen J. Gowdy (SGowdy@lbl.gov)
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
#include "scsi_convert.h"
#include "usb_disk.h"
#include "transport.h"

typedef uint8 __u8;

#define USB_STOR_SCSI_SENSE_HDRSZ 4
#define USB_STOR_SCSI_SENSE_10_HDRSZ 8

struct usb_stor_scsi_sense_hdr
{
	__u8 *dataLength;
	__u8 *mediumType;
	__u8 *devSpecParms;
	__u8 *blkDescLength;
};

typedef struct usb_stor_scsi_sense_hdr Usb_Stor_Scsi_Sense_Hdr;

union usb_stor_scsi_sense_hdr_u
{
	Usb_Stor_Scsi_Sense_Hdr hdr;
	__u8 *array[USB_STOR_SCSI_SENSE_HDRSZ];
};

typedef union usb_stor_scsi_sense_hdr_u Usb_Stor_Scsi_Sense_Hdr_u;

struct usb_stor_scsi_sense_hdr_10
{
	__u8 *dataLengthMSB;
	__u8 *dataLengthLSB;
	__u8 *mediumType;
	__u8 *devSpecParms;
	__u8 *reserved1;
	__u8 *reserved2;
	__u8 *blkDescLengthMSB;
	__u8 *blkDescLengthLSB;
};

typedef struct usb_stor_scsi_sense_hdr_10 Usb_Stor_Scsi_Sense_Hdr_10;

union usb_stor_scsi_sense_hdr_10_u
{
	Usb_Stor_Scsi_Sense_Hdr_10 hdr;
	__u8 *array[USB_STOR_SCSI_SENSE_10_HDRSZ];
};

typedef union usb_stor_scsi_sense_hdr_10_u Usb_Stor_Scsi_Sense_Hdr_10_u;

void usb_stor_scsiSenseParseBuffer( SCSI_cmd_s *, Usb_Stor_Scsi_Sense_Hdr_u *, Usb_Stor_Scsi_Sense_Hdr_10_u *, int * );

int usb_stor_scsiSense10to6( SCSI_cmd_s * the10 )
{
	__u8 *buffer = 0;
	int outputBufferSize = 0;
	int length = 0;
	Usb_Stor_Scsi_Sense_Hdr_u the6Locations;
	Usb_Stor_Scsi_Sense_Hdr_10_u the10Locations;

	//US_DEBUGP("-- converting 10 byte sense data to 6 byte\n");
	the10->nCmd[0] = the10->nCmd[0] & 0xBF;

	/* Determine buffer locations */
	usb_stor_scsiSenseParseBuffer( the10, &the6Locations, &the10Locations, &length );

	/* Work out minimum buffer to output */
	outputBufferSize = *the10Locations.hdr.dataLengthLSB;

	outputBufferSize += USB_STOR_SCSI_SENSE_HDRSZ;

	/* Check to see if we need to trucate the output */
	if ( outputBufferSize > length )
	{
		printk( "Had to truncate MODE_SENSE_10 buffer into MODE_SENSE.\n" );
		printk( "outputBufferSize is %d and length is %d.\n", outputBufferSize, length );
	}
	outputBufferSize = length;

	/* Data length */
	if ( *the10Locations.hdr.dataLengthMSB != 0 )	/* MSB must be zero */
	{
		printk( "Command will be truncated to fit in SENSE6 buffer.\n" );
		*the6Locations.hdr.dataLength = 0xff;
	}
	else
	{
		*the6Locations.hdr.dataLength = *the10Locations.hdr.dataLengthLSB;
	}

	/* Medium type and DevSpecific parms */
	*the6Locations.hdr.mediumType = *the10Locations.hdr.mediumType;
	*the6Locations.hdr.devSpecParms = *the10Locations.hdr.devSpecParms;

	/* Block descriptor length */
	if ( *the10Locations.hdr.blkDescLengthMSB != 0 )	/* MSB must be zero */
	{
		printk( "Command will be truncated to fit in SENSE6 buffer.\n" );
		*the6Locations.hdr.blkDescLength = 0xff;
	}
	else
	{
		*the6Locations.hdr.blkDescLength = *the10Locations.hdr.blkDescLengthLSB;
	}

	buffer = the10->pRequestBuffer;
	/* Copy the rest of the data */
	memmove( &( buffer[USB_STOR_SCSI_SENSE_HDRSZ] ), &( buffer[USB_STOR_SCSI_SENSE_10_HDRSZ] ), outputBufferSize - USB_STOR_SCSI_SENSE_HDRSZ );
	/* initialise last bytes left in buffer due to smaller header */
	memset( &( buffer[outputBufferSize - ( USB_STOR_SCSI_SENSE_10_HDRSZ - USB_STOR_SCSI_SENSE_HDRSZ )] ), 0, USB_STOR_SCSI_SENSE_10_HDRSZ - USB_STOR_SCSI_SENSE_HDRSZ );


	/* All done any everything was fine */
	return 0;
}

int usb_stor_scsiSense6to10( SCSI_cmd_s * the6 )
{
	/* will be used to store part of buffer */
	__u8 tempBuffer[USB_STOR_SCSI_SENSE_10_HDRSZ - USB_STOR_SCSI_SENSE_HDRSZ], *buffer = 0;
	int outputBufferSize = 0;
	int length = 0;
	Usb_Stor_Scsi_Sense_Hdr_u the6Locations;
	Usb_Stor_Scsi_Sense_Hdr_10_u the10Locations;

	//US_DEBUGP("-- converting 6 byte sense data to 10 byte\n");
	the6->nCmd[0] = the6->nCmd[0] | 0x40;

	/* Determine buffer locations */
	usb_stor_scsiSenseParseBuffer( the6, &the6Locations, &the10Locations, &length );

	/* Work out minimum buffer to output */
	outputBufferSize = *the6Locations.hdr.dataLength;

	outputBufferSize += USB_STOR_SCSI_SENSE_10_HDRSZ;

	/* Check to see if we need to trucate the output */
	if ( outputBufferSize > length )
	{
		printk( "Had to truncate MODE_SENSE into MODE_SENSE_10 buffer.\n" );
		printk( "outputBufferSize is %d and length is %d.\n", outputBufferSize, length );
	}
	outputBufferSize = length;

	/* Block descriptor length - save these before overwriting */
	tempBuffer[2] = *the10Locations.hdr.blkDescLengthMSB;
	tempBuffer[3] = *the10Locations.hdr.blkDescLengthLSB;
	*the10Locations.hdr.blkDescLengthLSB = *the6Locations.hdr.blkDescLength;
	*the10Locations.hdr.blkDescLengthMSB = 0;

	/* reserved - save these before overwriting */
	tempBuffer[0] = *the10Locations.hdr.reserved1;
	tempBuffer[1] = *the10Locations.hdr.reserved2;
	*the10Locations.hdr.reserved1 = *the10Locations.hdr.reserved2 = 0;

	/* Medium type and DevSpecific parms */
	*the10Locations.hdr.devSpecParms = *the6Locations.hdr.devSpecParms;
	*the10Locations.hdr.mediumType = *the6Locations.hdr.mediumType;

	/* Data length */
	*the10Locations.hdr.dataLengthLSB = *the6Locations.hdr.dataLength;
	*the10Locations.hdr.dataLengthMSB = 0;


	buffer = the6->pRequestBuffer;
	/* Copy the rest of the data */
	memmove( &( buffer[USB_STOR_SCSI_SENSE_10_HDRSZ] ), &( buffer[USB_STOR_SCSI_SENSE_HDRSZ] ), outputBufferSize - USB_STOR_SCSI_SENSE_10_HDRSZ );
	/* Put the first four bytes (after header) in place */
	memcpy( &( buffer[USB_STOR_SCSI_SENSE_10_HDRSZ] ), tempBuffer, USB_STOR_SCSI_SENSE_10_HDRSZ - USB_STOR_SCSI_SENSE_HDRSZ );

	/* All done and everything was fine */
	return 0;
}

void usb_stor_scsiSenseParseBuffer( SCSI_cmd_s * srb, Usb_Stor_Scsi_Sense_Hdr_u * the6, Usb_Stor_Scsi_Sense_Hdr_10_u * the10, int *length_p )
{
	int i = 0;
	int length = 0;
	__u8 *buffer = 0;


	length = srb->nRequestSize;
	buffer = srb->pRequestBuffer;
	if ( length < USB_STOR_SCSI_SENSE_10_HDRSZ )
		printk( "Buffer length smaller than header!!" );
	for ( i = 0; i < USB_STOR_SCSI_SENSE_10_HDRSZ; i++ )
	{
		if ( i < USB_STOR_SCSI_SENSE_HDRSZ )
		{
			the6->array[i] = &( buffer[i] );
			the10->array[i] = &( buffer[i] );
		}
		else
		{
			the10->array[i] = &( buffer[i] );
		}
	}

	/* Set value of length passed in */
	*length_p = length;
}
