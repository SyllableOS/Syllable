
/* Driver for USB Mass Storage compliant devices
 * Main Header File
 *
 * $Id$
 *
 * Current development and maintenance by:
 *   (c) 1999, 2000 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
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

#ifndef _USB_DISK_H_
#define _USB_DISK_H_

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/usb.h>
#include <atheos/semaphore.h>
#include <atheos/threads.h>
#include <atheos/scsi.h>

struct USB_disk_t;

typedef int ( *trans_cmnd ) ( SCSI_cmd_s *, struct USB_disk_t * );
typedef int ( *trans_reset ) ( struct USB_disk_t * );
typedef void ( *proto_cmnd ) ( SCSI_cmd_s *, struct USB_disk_t * );
typedef void ( *extra_data_destructor ) ( void * );	/* extra data destructor   */


/* Flag definitions */
#define US_FL_SINGLE_LUN      0x00000001	/* allow access to only LUN 0      */
#define US_FL_MODE_XLATE      0x00000002	/* translate _6 to _10 commands for
						   Win/MacOS compatibility */
#define US_FL_START_STOP      0x00000004	/* ignore START_STOP commands      */
#define US_FL_IGNORE_SER      0x00000010	/* Ignore the serial number given  */
#define US_FL_SCM_MULT_TARG   0x00000020	/* supports multiple targets */
#define US_FL_FIX_INQUIRY     0x00000040	/* INQUIRY response needs fixing */

#define USB_STOR_STRING_LEN 32

/* we allocate one of these for every device that we remember */
struct USB_disk_t
{
	struct USB_disk_t *next;	/* next device */

	/* the device we're working with */
	sem_id dev_semaphore;	/* protect pusb_dev */
	USB_device_s *pusb_dev;	/* this usb_device */

	unsigned int flags;	/* from filter initially */

	/* information about the device -- always good */
	char vendor[USB_STOR_STRING_LEN];
	char product[USB_STOR_STRING_LEN];
	char serial[USB_STOR_STRING_LEN];
	char *transport_name;
	char *protocol_name;
	uint8 subclass;
	uint8 protocol;
	uint8 max_lun;

	/* information about the device -- only good if device is attached */
	uint8 ifnum;		/* interface number   */
	uint8 ep_in;		/* bulk in endpoint   */
	uint8 ep_out;		/* bulk out endpoint  */
	USB_desc_endpoint_s *ep_int;	/* interrupt endpoint */

	/* function pointers for this device */
	trans_cmnd transport;	/* transport function     */
	trans_reset transport_reset;	/* transport device reset */
	proto_cmnd proto_handler;	/* protocol handler       */

	/* SCSI interfaces */
	int host_number;
	SCSI_host_s *host;	/* our dummy host data */
	SCSI_cmd_s *srb;	/* current srb         */

	/* interrupt info for CBI devices -- only good if attached */
	sem_id ip_waitq;	/* for CBI interrupts   */
	atomic_t ip_wanted;	/* is an IRQ expected?  */

	/* interrupt communications data */
	sem_id irq_urb_sem;	/* to protect irq_urb   */
	USB_packet_s *irq_urb;	/* for USB int requests */
	unsigned char irqbuf[2];	/* buffer for USB IRQ   */
	unsigned char irqdata[2];	/* data from USB IRQ    */

	/* control and bulk communications data */
	sem_id current_urb_sem;	/* to protect irq_urb   */
	USB_packet_s *current_urb;	/* non-int USB requests */

	/* the semaphore for sleeping the control thread */
	sem_id sema;		/* to sleep thread on   */

	/* mutual exclusion structures */
	usb_complete notify;	/* thread begin/end        */
	sem_id queue_exclusion;	/* to protect data structs */
	void *extra;		/* Any extra data          */
	extra_data_destructor extra_destructor;	/* extra data destructor   */
};

typedef struct USB_disk_t USB_disk_s;

/* The list of structures and the protective lock for them */
extern USB_disk_s *us_list;
extern sem_id us_list_semaphore;

/* The structure which defines our driver */
extern USB_driver_s usb_storage_driver;

#endif


