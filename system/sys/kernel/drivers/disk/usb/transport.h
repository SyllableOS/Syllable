
/* Driver for USB Mass Storage compliant devices
 * Transport Functions Header File
 *
 * $Id$
 *
 * Current development and maintenance by:
 *   (c) 1999, 2000 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
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

#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

#include <atheos/types.h>
#include <atheos/usb.h>
#include "usb_disk.h"

/* Protocols */

#define US_PR_CBI	0x00	/* Control/Bulk/Interrupt */
#define US_PR_CB	0x01	/* Control/Bulk w/o interrupt */
#define US_PR_BULK	0x50	/* bulk only */

#define US_PR_DPCM_USB 0xf0	/* Combination CB/SDDR09 */
#define US_PR_JUMPSHOT 0xf3 /* Lexar Jumpshot */


/*
 * Bulk only data structures
 */

/* command block wrapper */
struct bulk_cb_wrap
{
	uint32 Signature;	/* contains 'USBC' */
	uint32 Tag;		/* unique per command id */
	uint32 DataTransferLength;	/* size of data */
	uint8 Flags;		/* direction in bit 0 */
	uint8 Lun;		/* LUN normally 0 */
	uint8 Length;		/* of of the CDB */
	uint8 CDB[16];		/* max command */
};

#define US_BULK_CB_WRAP_LEN	31
#define US_BULK_CB_SIGN		0x43425355	/*spells out USBC */
#define US_BULK_FLAG_IN		1
#define US_BULK_FLAG_OUT	0

/* command status wrapper */
struct bulk_cs_wrap
{
	uint32 Signature;	/* should = 'USBS' */
	uint32 Tag;		/* same as original command */
	uint32 Residue;		/* amount not transferred */
	uint8 Status;		/* see below */
	uint8 Filler[18];
};

#define US_BULK_CS_WRAP_LEN	13
#define US_BULK_CS_SIGN		0x53425355	/* spells out 'USBS' */
#define US_BULK_STAT_OK		0
#define US_BULK_STAT_FAIL	1
#define US_BULK_STAT_PHASE	2

/* bulk-only class specific requests */
#define US_BULK_RESET_REQUEST	0xff
#define US_BULK_GET_MAX_LUN	0xfe

/*
 * us_bulk_transfer() return codes
 */
#define US_BULK_TRANSFER_GOOD		0	/* good transfer                 */
#define US_BULK_TRANSFER_SHORT		1	/* transfered less than expected */
#define US_BULK_TRANSFER_FAILED		2	/* transfer died in the middle   */
#define US_BULK_TRANSFER_ABORTED	3	/* transfer canceled             */

/*
 * Transport return codes
 */

#define USB_STOR_TRANSPORT_GOOD	   0	/* Transport good, command good     */
#define USB_STOR_TRANSPORT_FAILED  1	/* Transport good, command failed   */
#define USB_STOR_TRANSPORT_ERROR   2	/* Transport bad (i.e. device dead) */
#define USB_STOR_TRANSPORT_ABORTED 3	/* Transport aborted                */

/*
 * CBI accept device specific command
 */

#define US_CBI_ADSC		0

extern void usb_stor_CBI_irq( USB_packet_s * );
extern int usb_stor_CBI_transport( SCSI_cmd_s *, USB_disk_s * );

extern int usb_stor_CB_transport( SCSI_cmd_s *, USB_disk_s * );
extern int usb_stor_CB_reset( USB_disk_s * );

extern int usb_stor_Bulk_transport( SCSI_cmd_s *, USB_disk_s * );
extern int usb_stor_Bulk_max_lun( USB_disk_s * );
extern int usb_stor_Bulk_reset( USB_disk_s * );

extern unsigned int usb_stor_transfer_length( SCSI_cmd_s * );
extern void usb_stor_invoke_transport( SCSI_cmd_s *, USB_disk_s * );
extern int usb_stor_transfer_partial( USB_disk_s *, char *, int );
extern int usb_stor_bulk_msg( USB_disk_s *, void *, int, unsigned int, unsigned int * );
extern int usb_stor_control_msg( USB_disk_s *, unsigned int, uint8, uint8, uint16, uint16, void *, uint16 );
extern void usb_stor_transfer( SCSI_cmd_s *, USB_disk_s * );
extern int usb_stor_clear_halt( USB_device_s *, int );
#endif

