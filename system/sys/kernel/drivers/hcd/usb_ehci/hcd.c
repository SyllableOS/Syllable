/*
 * Copyright (c) 2001-2002 by David Brownell
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <atheos/udelay.h>
#include <atheos/isa_io.h>
#include <atheos/pci.h>
#include <atheos/usb.h>
#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/time.h>
#include <atheos/bootmodules.h>
#include <atheos/device.h>
#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>
#include <posix/signal.h>
#include <macros.h>

#include "hcd.h"


extern USB_bus_s* g_psBus;
extern PCI_bus_s* g_psPCIBus;


/*
 * hcd_calc_bus_time - approximate periodic transaction time in nanoseconds
 * @speed: from dev->speed; USB_SPEED_{LOW,FULL,HIGH}
 * @is_input: true iff the transaction sends data to the host
 * @isoc: true for isochronous transactions, false for interrupt ones
 * @bytecount: how many bytes in the transaction.
 *
 * Returns approximate bus time in nanoseconds for a periodic transaction.
 * See USB 2.0 spec section 5.11.3; only periodic transfers need to be
 * scheduled in software, this function is only used for such scheduling.
 */
long hcd_calc_bus_time (int speed, int is_input, int isoc, int bytecount)
{
	unsigned long	tmp;

	switch (speed) {
	case USB_SPEED_LOW: 	/* INTR only */
		if (is_input) {
			tmp = (67667L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (64060L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
		} else {
			tmp = (66700L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (64107L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
		}
	case USB_SPEED_FULL:	/* ISOC or INTR */
		if (isoc) {
			tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (((is_input) ? 7268L : 6265L) + BW_HOST_DELAY + tmp);
		} else {
			tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (9107L + BW_HOST_DELAY + tmp);
		}
	case USB_SPEED_HIGH:	/* ISOC or INTR */
		// FIXME adjust for input vs output
		if (isoc)
			tmp = HS_USECS (bytecount);
		else
			tmp = HS_USECS_ISO (bytecount);
		return tmp;
	default:
		kerndbg( KERN_WARNING, "USB: Bogus device speed!\n" );
		return -1;
	}
}

/*-------------------------------------------------------------------------*/

/*
 * USB Host Controller Driver framework
 *
 * Plugs into usbcore (usb_bus) and lets HCDs share code, minimizing
 * HCD-specific behaviors/bugs.  Think of it as the "upper level" of
 * some drivers, where the "lower level" is hardware-specific.
 *
 * This does error checks, tracks devices and urbs, and delegates to a
 * "hc_driver" only for code (and data) that really needs to know about
 * hardware differences.  That includes root hub registers, i/o queues,
 * and so on ... but as little else as possible.
 *
 * Shared code includes most of the "root hub" code (these are emulated,
 * though each HC's hardware works differently) and PCI glue, plus request
 * tracking overhead.  The HCD code should only block on spinlocks or on
 * hardware handshaking; blocking on software events (such as other kernel
 * threads releasing resources, or completing actions) is all generic.
 *
 * Happens the USB 2.0 spec says this would be invisible inside the "USBD",
 * and includes mostly a "HCDI" (HCD Interface) along with some APIs used
 * only by the hub driver ... and that neither should be seen or used by
 * usb client device drivers.
 *
 * Contributors of ideas or unattributed patches include: David Brownell,
 * Roman Weissgaerber, Rory Bolt, ...
 *
 * HISTORY:
 * 2002-sept	Merge some 2.5 updates so we can share hardware level HCD
 * 	code between the 2.4.20+ and 2.5 trees.
 * 2002-feb	merge to 2.4.19
 * 2001-12-12	Initial patch version for Linux 2.5.1 kernel.
 */

/*-------------------------------------------------------------------------*/

/* host controllers we manage */
static LIST_HEAD (hcd_list);

/* used when updating list of hcds */
static SPIN_LOCK (hcd_list_lock, "hcd_list_lock");

/* used when updating hcd data */
static SpinLock_s hcd_data_lock = INIT_SPIN_LOCK( "hcd_data_lock" );

/*-------------------------------------------------------------------------*/

/*
 * Sharable chunks of root hub code.
 */

/*-------------------------------------------------------------------------*/

/* usb 2.0 root hub device descriptor */
static const u8 usb2_rh_dev_descriptor [18] = {
	0x12,       /*  __u8  bLength; */
	0x01,       /*  __u8  bDescriptorType; Device */
	0x00, 0x02, /*  __u16 bcdUSB; v2.0 */

	0x09,	    /*  __u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*  __u8  bDeviceSubClass; */
	0x01,       /*  __u8  bDeviceProtocol; [ usb 2.0 single TT ]*/
	0x08,       /*  __u8  bMaxPacketSize0; 8 Bytes */

	0x00, 0x00, /*  __u16 idVendor; */
 	0x00, 0x00, /*  __u16 idProduct; */
	0x00, 0x05, /*  __u16 bcdDevice */

	0x03,       /*  __u8  iManufacturer; */
	0x02,       /*  __u8  iProduct; */
	0x01,       /*  __u8  iSerialNumber; */
	0x01        /*  __u8  bNumConfigurations; */
};

/* no usb 2.0 root hub "device qualifier" descriptor: one speed only */

/* usb 1.1 root hub device descriptor */
static const u8 usb11_rh_dev_descriptor [18] = {
	0x12,       /*  __u8  bLength; */
	0x01,       /*  __u8  bDescriptorType; Device */
	0x10, 0x01, /*  __u16 bcdUSB; v1.1 */

	0x09,	    /*  __u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*  __u8  bDeviceSubClass; */
	0x00,       /*  __u8  bDeviceProtocol; [ low/full speeds only ] */
	0x08,       /*  __u8  bMaxPacketSize0; 8 Bytes */

	0x00, 0x00, /*  __u16 idVendor; */
 	0x00, 0x00, /*  __u16 idProduct; */
	0x00, 0x05, /*  __u16 bcdDevice */

	0x03,       /*  __u8  iManufacturer; */
	0x02,       /*  __u8  iProduct; */
	0x01,       /*  __u8  iSerialNumber; */
	0x01        /*  __u8  bNumConfigurations; */
};


/*-------------------------------------------------------------------------*/

/* Configuration descriptors for our root hubs */

static const u8 fs_rh_config_descriptor [] = {

	/* one configuration */
	0x09,       /*  __u8  bLength; */
	0x02,       /*  __u8  bDescriptorType; Configuration */
	0x19, 0x00, /*  __u16 wTotalLength; */
	0x01,       /*  __u8  bNumInterfaces; (1) */
	0x01,       /*  __u8  bConfigurationValue; */
	0x00,       /*  __u8  iConfiguration; */
	0x40,       /*  __u8  bmAttributes; 
				 Bit 7: Bus-powered,
				     6: Self-powered,
				     5 Remote-wakwup,
				     4..0: resvd */
	0x00,       /*  __u8  MaxPower; */
      
	/* USB 1.1:
	 * USB 2.0, single TT organization (mandatory):
	 *	one interface, protocol 0
	 *
	 * USB 2.0, multiple TT organization (optional):
	 *	two interfaces, protocols 1 (like single TT)
	 *	and 2 (multiple TT mode) ... config is
	 *	sometimes settable
	 *	NOT IMPLEMENTED
	 */

	/* one interface */
	0x09,       /*  __u8  if_bLength; */
	0x04,       /*  __u8  if_bDescriptorType; Interface */
	0x00,       /*  __u8  if_bInterfaceNumber; */
	0x00,       /*  __u8  if_bAlternateSetting; */
	0x01,       /*  __u8  if_bNumEndpoints; */
	0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,       /*  __u8  if_bInterfaceSubClass; */
	0x00,       /*  __u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x00,       /*  __u8  if_iInterface; */
     
	/* one endpoint (status change endpoint) */
	0x07,       /*  __u8  ep_bLength; */
	0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  __u8  ep_bmAttributes; Interrupt */
 	0x02, 0x00, /*  __u16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8) */
	0xff        /*  __u8  ep_bInterval; (255ms -- usb 2.0 spec) */
};

static const u8 hs_rh_config_descriptor [] = {

	/* one configuration */
	0x09,       /*  __u8  bLength; */
	0x02,       /*  __u8  bDescriptorType; Configuration */
	0x19, 0x00, /*  __u16 wTotalLength; */
	0x01,       /*  __u8  bNumInterfaces; (1) */
	0x01,       /*  __u8  bConfigurationValue; */
	0x00,       /*  __u8  iConfiguration; */
	0x40,       /*  __u8  bmAttributes; 
				 Bit 7: Bus-powered,
				     6: Self-powered,
				     5 Remote-wakwup,
				     4..0: resvd */
	0x00,       /*  __u8  MaxPower; */
      
	/* USB 1.1:
	 * USB 2.0, single TT organization (mandatory):
	 *	one interface, protocol 0
	 *
	 * USB 2.0, multiple TT organization (optional):
	 *	two interfaces, protocols 1 (like single TT)
	 *	and 2 (multiple TT mode) ... config is
	 *	sometimes settable
	 *	NOT IMPLEMENTED
	 */

	/* one interface */
	0x09,       /*  __u8  if_bLength; */
	0x04,       /*  __u8  if_bDescriptorType; Interface */
	0x00,       /*  __u8  if_bInterfaceNumber; */
	0x00,       /*  __u8  if_bAlternateSetting; */
	0x01,       /*  __u8  if_bNumEndpoints; */
	0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,       /*  __u8  if_bInterfaceSubClass; */
	0x00,       /*  __u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x00,       /*  __u8  if_iInterface; */
     
	/* one endpoint (status change endpoint) */
	0x07,       /*  __u8  ep_bLength; */
	0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  __u8  ep_bmAttributes; Interrupt */
 	0x02, 0x00, /*  __u16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8) */
	0x0c        /*  __u8  ep_bInterval; (256ms -- usb 2.0 spec) */
};

/*-------------------------------------------------------------------------*/

/*
 * helper routine for returning string descriptors in UTF-16LE
 * input can actually be ISO-8859-1; ASCII is its 7-bit subset
 */
static int ascii2utf (char *s, u8 *utf, int utfmax)
{
	int retval;

	for (retval = 0; *s && utfmax > 1; utfmax -= 2, retval += 2) {
		*utf++ = *s++;
		*utf++ = 0;
	}
	return retval;
}

/*
 * rh_string - provides manufacturer, product and serial strings for root hub
 * @id: the string ID number (1: serial number, 2: product, 3: vendor)
 * @pci_desc: PCI device descriptor for the relevant HC
 * @type: string describing our driver 
 * @data: return packet in UTF-16 LE
 * @len: length of the return packet
 *
 * Produces either a manufacturer, product or serial number string for the
 * virtual root hub device.
 */
static int rh_string (
	int		id,
	struct usb_hcd	*hcd,
	u8		*data,
	int		len
) {
	char buf [100];

	// language ids
	if (id == 0) {
		*data++ = 4; *data++ = 3;	/* 4 bytes string data */
		*data++ = 0; *data++ = 0;	/* some language id */
		return 4;

	// serial number
	} else if (id == 1) {
		sprintf(buf, "%i", hcd->bus->nBusNum);

	// product description
	} else if (id == 2) {
                strcpy (buf, hcd->product_desc);

 	// id 3 == vendor description
	} else if (id == 3) {
                sprintf (buf, "%s %s %s", "Syllable", "0.6",
			hcd->description);

	// unsupported IDs --> "protocol stall"
	} else
	    return 0;

	data [0] = 2 * (strlen (buf) + 1);
	data [1] = 3;	/* type == string */
	return 2 + ascii2utf (buf, data + 2, len - 2);
}


/* Root hub control transfers execute synchronously */
static int rh_call_control (struct usb_hcd *hcd, USB_packet_s *urb)
{
	USB_request_s *cmd = (USB_request_s *) urb->pSetupPacket;
 	u16		typeReq, wValue, wIndex, wLength;
	const u8	*bufp = 0;
	u8		*ubuf = urb->pBuffer;
	int		len = 0;

	typeReq  = (cmd->nRequestType << 8) | cmd->nRequest;
	wValue   = le16_to_cpu (cmd->nValue);
	wIndex   = le16_to_cpu (cmd->nIndex);
	wLength  = le16_to_cpu (cmd->nLength);

	if (wLength > urb->nBufferLength)
		goto error;

	/* set up for success */
	urb->nStatus = 0;
	urb->nActualLength = wLength;
	switch (typeReq) {

	/* DEVICE REQUESTS */

	case DeviceRequest | USB_REQ_GET_STATUS:
		// DEVICE_REMOTE_WAKEUP
		ubuf [0] = 1; // selfpowered
		ubuf [1] = 0;
			/* FALLTHROUGH */
	case DeviceOutRequest | USB_REQ_CLEAR_FEATURE:
	case DeviceOutRequest | USB_REQ_SET_FEATURE:
		kerndbg( KERN_DEBUG, "no device features yet yet\n");
		break;
	case DeviceRequest | USB_REQ_GET_CONFIGURATION:
		ubuf [0] = 1;
			/* FALLTHROUGH */
	case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
		break;
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		switch (wValue & 0xff00) {
		case USB_DT_DEVICE << 8:
			if (hcd->driver->flags & HCD_USB2)
				bufp = usb2_rh_dev_descriptor;
			else if (hcd->driver->flags & HCD_USB11)
				bufp = usb11_rh_dev_descriptor;
			else
				goto error;
			len = 18;
			break;
		case USB_DT_CONFIG << 8:
			if (hcd->driver->flags & HCD_USB2) {
				bufp = hs_rh_config_descriptor;
				len = sizeof hs_rh_config_descriptor;
			} else {
				bufp = fs_rh_config_descriptor;
				len = sizeof fs_rh_config_descriptor;
			}
			break;
		case USB_DT_STRING << 8:
			urb->nActualLength = rh_string (
				wValue & 0xff, hcd,
				ubuf, wLength);
			break;
		default:
			goto error;
		}
		break;
	case DeviceRequest | USB_REQ_GET_INTERFACE:
		ubuf [0] = 0;
			/* FALLTHROUGH */
	case DeviceOutRequest | USB_REQ_SET_INTERFACE:
		break;
	case DeviceOutRequest | USB_REQ_SET_ADDRESS:
		// wValue == urb->dev->devaddr
		kerndbg( KERN_DEBUG_LOW, "%i root hub device address %d\n",
			hcd->bus->nBusNum, wValue);
		break;

	/* INTERFACE REQUESTS (no defined feature/status flags) */

	/* ENDPOINT REQUESTS */

	case EndpointRequest | USB_REQ_GET_STATUS:
		// ENDPOINT_HALT flag
		ubuf [0] = 0;
		ubuf [1] = 0;
			/* FALLTHROUGH */
	case EndpointOutRequest | USB_REQ_CLEAR_FEATURE:
	case EndpointOutRequest | USB_REQ_SET_FEATURE:
		kerndbg( KERN_DEBUG, "no endpoint features yet\n");
		break;

	/* CLASS REQUESTS (and errors) */

	default:
		/* non-generic request */
		urb->nStatus = hcd->driver->hub_control (hcd,
			typeReq, wValue, wIndex,
			ubuf, wLength);
		break;
error:
		/* "protocol stall" on error */
		urb->nStatus = -EPIPE;
		kerndbg(KERN_DEBUG, "unsupported hub control message (maxchild %d)\n",
				urb->psDevice->nMaxChild);
	}
	if (urb->nStatus) {
		urb->nActualLength = 0;
		kerndbg(KERN_WARNING, "CTRL: TypeReq=0x%x val=0x%x idx=0x%x len=%d ==> %d\n",
			typeReq, wValue, wIndex, wLength, urb->nStatus);
	}
	if (bufp) {
		if (urb->nBufferLength < len)
			len = urb->nBufferLength;
		urb->nActualLength = len;
		// always USB_DIR_IN, toward host
		memcpy (ubuf, bufp, len);
	}

	/* any errors get returned through the urb completion */
	usb_hcd_giveback_urb (hcd, urb, 0);
	return 0;
}

/*-------------------------------------------------------------------------*/

/*
 * Root Hub interrupt transfers are synthesized with a timer.
 * Completions are called in_interrupt() but not in_irq().
 */

static void rh_report_status (void* ptr);

static int rh_status_urb (struct usb_hcd *hcd, USB_packet_s *urb) 
{
	int	len = 1 + (urb->psDevice->nMaxChild / 8);

	/* rh_timer protected by hcd_data_lock */
	if (/*timer_pending (&hcd->rh_timer)
			|| */urb->nStatus != -EINPROGRESS
			|| !HCD_IS_RUNNING (hcd->state)
			|| urb->nBufferLength < len) {
		kerndbg(KERN_WARNING, "not queuing status urb, stat %d\n", urb->nStatus);
		return -EINVAL;
	}

	urb->pHCPrivate = hcd;	/* nonzero to indicate it's queued */
	
	
	//hcd->rh_timer.function = rh_report_status;
	//hcd->rh_timer.data = (unsigned long) urb;
	hcd->rh_timer_data = urb;
	/* USB 2.0 spec says 256msec; this is close enough */
	start_timer( hcd->rh_timer, rh_report_status, urb, 1000 / 4 * 1000, true );
	//hcd->rh_timer.expires = jiffies + HZ/4;
	//add_timer (&hcd->rh_timer);
	return 0;
}

/* timer callback */

static void rh_report_status (void* ptr)
{
	USB_packet_s	*urb;
	struct usb_hcd	*hcd;
	int		length;
	unsigned long	flags;

	urb = (USB_packet_s *) ptr;
	spin_lock_irqsave (&urb->hLock, flags);
	if (!urb->psDevice) {
		spin_unlock_irqrestore (&urb->hLock, flags);
		return;
	}

	hcd = urb->psDevice->psBus->pHCPrivate;
	if (urb->nStatus == -EINPROGRESS) {
		if (HCD_IS_RUNNING (hcd->state)) {
			length = hcd->driver->hub_status_data (hcd,
					urb->pBuffer);
			spin_unlock_irqrestore (&urb->hLock, flags);
			if (length > 0) {
				urb->nActualLength = length;
				urb->nStatus = 0;
				urb->pComplete (urb);
			}
			spin_lock_irqsave (&hcd_data_lock, flags);
			urb->nStatus = -EINPROGRESS;
			if (HCD_IS_RUNNING (hcd->state)
					&& rh_status_urb (hcd, urb) != 0) {
				/* another driver snuck in? */
				kerndbg(KERN_WARNING, "%i, can't resubmit roothub status urb?\n",
					hcd->bus->nBusNum);
				spin_unlock_irqrestore (&hcd_data_lock, flags);
				//BUG ();
			}
			spin_unlock_irqrestore (&hcd_data_lock, flags);
		} else
			spin_unlock_irqrestore (&urb->hLock, flags);
	} else {
		/* this urb's been unlinked */
		urb->pHCPrivate = 0;
		spin_unlock_irqrestore (&urb->hLock, flags);

		usb_hcd_giveback_urb (hcd, urb, 0);
	}
}

/*-------------------------------------------------------------------------*/

static int rh_urb_enqueue (struct usb_hcd *hcd, USB_packet_s *urb)
{
	//printk( "rh_urb_enqueue()\n" );
	if (usb_pipeint (urb->nPipe)) {
		int		retval;
		unsigned long	flags;

		spin_lock_irqsave (&hcd_data_lock, flags);
		hcd->rh_timer = create_timer();
		retval = rh_status_urb (hcd, urb);
		spin_unlock_irqrestore (&hcd_data_lock, flags);
		return retval;
	}
	if (usb_pipecontrol (urb->nPipe))
		return rh_call_control (hcd, urb);
	else
		return -EINVAL;
}

/*-------------------------------------------------------------------------*/

static void rh_status_dequeue (struct usb_hcd *hcd, USB_packet_s *urb)
{
	unsigned long	flags;

	spin_lock_irqsave (&hcd_data_lock, flags);
	delete_timer(hcd->rh_timer);
	spin_unlock_irqrestore (&hcd_data_lock, flags);

	/* we rely on RH callback code not unlinking its URB! */
	usb_hcd_giveback_urb (hcd, urb, 0);
}

/*-------------------------------------------------------------------------*/

/*
 * Generic HC operations.
 */

/*-------------------------------------------------------------------------*/

/* called from khubd, or root hub init threads for hcd-private init */
static int hcd_alloc_dev (USB_device_s *udev)
{
	struct hcd_dev		*dev;
	struct usb_hcd		*hcd;
	unsigned long		flags;

	if (!udev || udev->pHCPrivate)
		return -EINVAL;
	if (!udev->psBus || !udev->psBus->pHCPrivate)
		return -ENODEV;
	hcd = udev->psBus->pHCPrivate;
	if (hcd->state == USB_STATE_QUIESCING)
		return -ENOLINK;

	dev = (struct hcd_dev *) kmalloc (sizeof *dev, MEMF_KERNEL);
	if (dev == NULL)
		return -ENOMEM;
	memset (dev, 0, sizeof *dev);

	INIT_LIST_HEAD (&dev->dev_list);
	INIT_LIST_HEAD (&dev->urb_list);

	spin_lock_irqsave (&hcd_data_lock, flags);
	list_add (&dev->dev_list, &hcd->dev_list);
	// refcount is implicit
	udev->pHCPrivate = dev;
	spin_unlock_irqrestore (&hcd_data_lock, flags);

	return 0;
}

/*-------------------------------------------------------------------------*/

#if 0
static void hcd_panic (void *_hcd)
{
	struct usb_hcd *hcd = _hcd;
	hcd->driver->stop (hcd);
}
#endif

static void hc_died (struct usb_hcd *hcd)
{
	struct list_head	*devlist, *urblist;
	struct hcd_dev		*dev;
	USB_packet_s		*urb;
	unsigned long		flags;
	
	/* flag every pending urb as done */
	spin_lock_irqsave (&hcd_data_lock, flags);
	list_for_each (devlist, &hcd->dev_list) {
		dev = list_entry (devlist, struct hcd_dev, dev_list);
		list_for_each (urblist, &dev->urb_list) {
			urb = list_entry (urblist, USB_packet_s, lPacketList);
			kerndbg( KERN_WARNING, "shutdown %i is urb %p pipe %x, current status %d\n",
				hcd->bus->nBusNum,
				urb, urb->nPipe, urb->nStatus);
			if (urb->nStatus == -EINPROGRESS)
				urb->nStatus = -ESHUTDOWN;
		}
	}
	
	kerndbg( KERN_FATAL, "hc_died() called\n" );
	
	
	urb = (USB_packet_s *) hcd->rh_timer_data;
	if (urb)
		urb->nStatus = -ESHUTDOWN;
	
	spin_unlock_irqrestore (&hcd_data_lock, flags);

	if (urb)
		rh_status_dequeue (hcd, urb);
	#if 0
	/* hcd->stop() needs a task context */
	INIT_TQUEUE (&hcd->work, hcd_panic, hcd);
	(void) schedule_task (&hcd->work);
	#endif
}

/*-------------------------------------------------------------------------*/

static void urb_unlink (USB_packet_s *urb)
{
	unsigned long		flags;
	USB_device_s	*dev;

	/* Release any periodic transfer bandwidth */
	if (urb->nBandWidth)
		g_psBus->release_bandwidth (urb->psDevice, urb,
			usb_pipeisoc (urb->nPipe));

	/* clear all state linking urb to this dev (and hcd) */

	spin_lock_irqsave (&hcd_data_lock, flags);
	list_del_init (&urb->lPacketList);
	dev = urb->psDevice;
	urb->psDevice = NULL;
	atomic_dec( &dev->nRefCount );
	spin_unlock_irqrestore (&hcd_data_lock, flags);
}


/* may be called in any context with a valid urb->dev usecount */
/* caller surrenders "ownership" of urb */

static int hcd_submit_urb (USB_packet_s *urb)
{
	int			status;
	struct usb_hcd		*hcd;
	struct hcd_dev		*dev;
	unsigned long		flags;
	int			pipe, temp, max;
	int			mem_flags;
	
	//printk( "hcd_submit_urb()\n" );

	if (!urb || urb->pHCPrivate || !urb->pComplete)
		return -EINVAL;
		
	//printk( "1\n" );

	urb->nStatus = -EINPROGRESS;
	urb->nActualLength = 0;
	urb->nBandWidth = 0;
	INIT_LIST_HEAD (&urb->lPacketList);

	if (!urb->psDevice || !urb->psDevice->psBus || urb->psDevice->nDeviceNum < 0)
		return -ENODEV;
		
	//printk("2\n" );
	hcd = urb->psDevice->psBus->pHCPrivate;
	dev = urb->psDevice->pHCPrivate;
	if (!hcd || !dev)
		return -ENODEV;
		
	//printk( "3\n" );

	/* can't submit new urbs when quiescing, halted, ... */
	if (hcd->state == USB_STATE_QUIESCING || !HCD_IS_RUNNING (hcd->state))
		return -ESHUTDOWN;
		
	//printk( "4\n" );
	pipe = urb->nPipe;
	temp = usb_pipetype (urb->nPipe);
	if (usb_endpoint_halted (urb->psDevice, usb_pipeendpoint (pipe),
			usb_pipeout (pipe)))
		return -EPIPE;
		
	//printk( "5\n" );

	/* NOTE: 2.5 passes this value explicitly in submit() */
	mem_flags = MEMF_KERNEL;

	/* FIXME there should be a sharable lock protecting us against
	 * config/altsetting changes and disconnects, kicking in here.
	 */

	/* Sanity check, so HCDs can rely on clean data */
	max = usb_maxpacket (urb->psDevice, pipe, usb_pipeout (pipe));
	if (max <= 0) {
		kerndbg(KERN_FATAL, "bogus endpoint (bad maxpacket)");
		return -EINVAL;
	}

	/* "high bandwidth" mode, 1-3 packets/uframe? */
	if (urb->psDevice->eSpeed == USB_SPEED_HIGH) {
		int	mult;
		switch (temp) {
		case USB_PIPE_ISOCHRONOUS:
		case USB_PIPE_INTERRUPT:
			mult = 1 + ((max >> 11) & 0x03);
			max &= 0x03ff;
			max *= mult;
		}
	}

	/* periodic transfers limit size per frame/uframe */
	switch (temp) {
	case USB_PIPE_ISOCHRONOUS: {
		int	n, len;

		if (urb->nPacketNum <= 0)		    
			return -EINVAL;
		for (n = 0; n < urb->nPacketNum; n++) {
			len = urb->sISOFrame[n].nLength;
			if (len < 0 || len > max) 
				return -EINVAL;
		}

		}
		break;
	case USB_PIPE_INTERRUPT:
		if (urb->nBufferLength > max)
			return -EINVAL;
	}
	
	//printk( "6\n" );

	/* the I/O buffer must usually be mapped/unmapped */
	if (urb->nBufferLength < 0)
		return -EINVAL;

	if (urb->psNext) {
		kerndbg(KERN_WARNING, "use explicit queuing not urb->next\n");
		return -EINVAL;
	}

#ifdef DEBUG
	/* stuff that drivers shouldn't do, but which shouldn't
	 * cause problems in HCDs if they get it wrong.
	 */
	{
	unsigned int	orig_flags = urb->nTransferFlags;
	unsigned int	allowed;

	/* enforce simple/standard policy */
	allowed = USB_ASYNC_UNLINK;	// affects later unlinks
	allowed |= USB_NO_FSBR;		// only affects UHCI
	switch (temp) {
	case USB_PIPE_CONTROL:
		allowed |= USB_DISABLE_SPD;
		break;
	case USB_PIPE_BULK:
		allowed |= USB_DISABLE_SPD | USB_QUEUE_BULK
				| USB_ZERO_PACKET | USB_NO_INTERRUPT;
		break;
	case USB_PIPE_INTERRUPT:
		allowed |= USB_DISABLE_SPD;
		break;
	case USB_PIPE_ISOCHRONOUS:
		allowed |= USB_ISO_ASAP;
		break;
	}
	urb->nTransferFlags &= allowed;

	/* fail if submitter gave bogus flags */
	if (urb->nTransferFlags != orig_flags) {
		kerndbg(KERN_FATAL, "BOGUS urb flags, %x --> %x\n",
			orig_flags, urb->nTransferFlags);
		return -EINVAL;
	}
	}
#endif
	/*
	 * Force periodic transfer intervals to be legal values that are
	 * a power of two (so HCDs don't need to).
	 *
	 * FIXME want bus->{intr,iso}_sched_horizon values here.  Each HC
	 * supports different values... this uses EHCI/UHCI defaults (and
	 * EHCI can use smaller non-default values).
	 */
	switch (temp) {
	case USB_PIPE_ISOCHRONOUS:
	case USB_PIPE_INTERRUPT:
		/* too small? */
		if (urb->nInterval <= 0)
			return -EINVAL;
		/* too big? */
		switch (urb->psDevice->eSpeed) {
		case USB_SPEED_HIGH:	/* units are microframes */
			// NOTE usb handles 2^15
			if (urb->nInterval > (1024 * 8))
				urb->nInterval = 1024 * 8;
			temp = 1024 * 8;
			break;
		case USB_SPEED_FULL:	/* units are frames/msec */
		case USB_SPEED_LOW:
			if (temp == USB_PIPE_INTERRUPT) {
				if (urb->nInterval > 255)
					return -EINVAL;
				// NOTE ohci only handles up to 32
				temp = 128;
			} else {
				if (urb->nInterval > 1024)
					urb->nInterval = 1024;
				// NOTE usb and ohci handle up to 2^15
				temp = 1024;
			}
			break;
		default:
			return -EINVAL;
		}
		/* power of two? */
		while (temp > urb->nInterval)
			temp >>= 1;
		urb->nInterval = temp;
	}

	//printk( "7\n" );

	/*
	 * FIXME:  make urb timeouts be generic, keeping the HCD cores
	 * as simple as possible.
	 */

	// NOTE:  a generic device/urb monitoring hook would go here.
	// hcd_monitor_hook(MONITOR_URB_SUBMIT, urb)
	// It would catch submission paths for all urbs.

	/*
	 * Atomically queue the urb,  first to our records, then to the HCD.
	 * Access to urb->status is controlled by urb->lock ... changes on
	 * i/o completion (normal or fault) or unlinking.
	 */

	// FIXME:  verify that quiescing hc works right (RH cleans up)

	spin_lock_irqsave (&hcd_data_lock, flags);
	if (HCD_IS_RUNNING (hcd->state) && hcd->state != USB_STATE_QUIESCING) {
		atomic_inc( &urb->psDevice->nRefCount);
		list_add (&urb->lPacketList, &dev->urb_list);
		status = 0;
	} else {
		INIT_LIST_HEAD (&urb->lPacketList);
		status = -ESHUTDOWN;
	}
	spin_unlock_irqrestore (&hcd_data_lock, flags);
	if (status)
		return status;

	// NOTE:  2.5 does this if !URB_NO_DMA_MAP transfer flag
	#if 0
	if (usb_pipecontrol (urb->nPipe))
		urb->setup_dma = pci_map_single (
				hcd->pdev,
				urb->setup_packet,
				sizeof (struct usb_ctrlrequest),
				PCI_DMA_TODEVICE);
	if (urb->transfer_buffer_length != 0)
		urb->transfer_dma = pci_map_single (
				hcd->pdev,
				urb->transfer_buffer,
				urb->transfer_buffer_length,
				usb_pipein (urb->pipe)
				    ? PCI_DMA_FROMDEVICE
				    : PCI_DMA_TODEVICE);
	#endif
	if (urb->psDevice == hcd->bus->psRootHUB)
		status = rh_urb_enqueue (hcd, urb);
	else
		status = hcd->driver->urb_enqueue (hcd, urb, mem_flags);
	return status;
}

/*-------------------------------------------------------------------------*/

/* called in any context */
static int hcd_get_frame_number (USB_device_s *udev)
{
	struct usb_hcd	*hcd = (struct usb_hcd *)udev->psBus->pHCPrivate;
	return hcd->driver->get_frame_number (hcd);
}

/*-------------------------------------------------------------------------*/

struct completion_splice {		// modified urb context:
	/* did we complete? */
	//struct completion	done;
	sem_id done;

	/* original urb data */
	void			(*complete)(USB_packet_s *);
	void			*context;
};

static void unlink_complete (USB_packet_s *urb)
{
	struct completion_splice	*splice;

	splice = (struct completion_splice *) urb->pCompleteData;

	/* issue original completion call */
	urb->pComplete = splice->complete;
	urb->pCompleteData = splice->context;
	urb->pComplete (urb);

	/* then let the synchronous unlink call complete */
	
	UNLOCK( splice->done );
	//complete (&splice->done);
}

/*
 * called in any context; note ASYNC_UNLINK restrictions
 *
 * caller guarantees urb won't be recycled till both unlink()
 * and the urb's completion function return
 */
static int hcd_unlink_urb (USB_packet_s *urb)
{
	struct hcd_dev			*dev;
	struct usb_hcd			*hcd = 0;
	unsigned long			flags;
	struct completion_splice	splice;
	int				retval;

	if (!urb)
		return -EINVAL;

	/*
	 * we contend for urb->status with the hcd core,
	 * which changes it while returning the urb.
	 *
	 * Caller guaranteed that the urb pointer hasn't been freed, and
	 * that it was submitted.  But as a rule it can't know whether or
	 * not it's already been unlinked ... so we respect the reversed
	 * lock sequence needed for the usb_hcd_giveback_urb() code paths
	 * (urb lock, then hcd_data_lock) in case some other CPU is now
	 * unlinking it.
	 */
	spin_lock_irqsave (&urb->hLock, flags);
	spinlock (&hcd_data_lock);
	if (!urb->pHCPrivate || urb->nTransferFlags & USB_TIMEOUT_KILLED) {
		retval = -EINVAL;
		goto done;
	}

	if (!urb->psDevice || !urb->psDevice->psBus) {
		retval = -ENODEV;
		goto done;
	}

	/* giveback clears dev; non-null means it's linked at this level */
	dev = urb->psDevice->pHCPrivate;
	hcd = urb->psDevice->psBus->pHCPrivate;
	if (!dev || !hcd) {
		retval = -ENODEV;
		goto done;
	}

	/* Any status except -EINPROGRESS means the HCD has already started
	 * to return this URB to the driver.  In that case, there's no
	 * more work for us to do.
	 *
	 * There's much magic because of "automagic resubmit" of interrupt
	 * transfers, stopped only by explicit unlinking.  We won't issue
	 * an "it's unlinked" callback more than once, but device drivers
	 * can need to retry (SMP, -EAGAIN) an unlink request as well as
	 * fake out the "not yet completed" state (set -EINPROGRESS) if
	 * unlinking from complete().  Automagic eventually vanishes.
	 *
	 * FIXME use an URB_UNLINKED flag to match URB_TIMEOUT_KILLED
	 */
	if (urb->nStatus != -EINPROGRESS) {
		if (usb_pipetype (urb->nPipe) == USB_PIPE_INTERRUPT)
			retval = -EAGAIN;
		else
			retval = -EBUSY;
		goto done;
	}

	/* maybe set up to block on completion notification */
	if ((urb->nTransferFlags & USB_TIMEOUT_KILLED))
		urb->nStatus = -ETIMEDOUT;
	else if (!(urb->nTransferFlags & USB_ASYNC_UNLINK)) {
		/*if (in_interrupt ()) {
			dbg ("non-async unlink in_interrupt");
			retval = -EWOULDBLOCK;
			goto done;
		}*/
		/* synchronous unlink: block till we see the completion */
		splice.done = create_semaphore( "usb_hcd_complete", 0, 0 );
//		init_completion (&splice.done);
		splice.complete = urb->pComplete;
		splice.context = urb->pCompleteData;
		urb->pComplete = unlink_complete;
		urb->pCompleteData = &splice;
		urb->nStatus = -ENOENT;
	} else {
		/* asynchronous unlink */
		urb->nStatus = -ECONNRESET;
	}
	spinunlock (&hcd_data_lock);
	spin_unlock_irqrestore (&urb->hLock, flags);

	if (urb == (USB_packet_s *) hcd->rh_timer_data) {
		rh_status_dequeue (hcd, urb);
		retval = 0;
	} else {
		retval = hcd->driver->urb_dequeue (hcd, urb);
		// FIXME:  if retval and we tried to splice, whoa!!
		if (retval && urb->nStatus == -ENOENT) printk("whoa! retval %d\n", retval);
	}

    	/* block till giveback, if needed */
		if (!(urb->nTransferFlags & (USB_ASYNC_UNLINK|USB_TIMEOUT_KILLED))
			&& HCD_IS_RUNNING (hcd->state)
			&& !retval) {
		LOCK( splice.done );
		delete_semaphore( splice.done );
		//wait_for_completion (&splice.done);
	} else if ((urb->nTransferFlags & USB_ASYNC_UNLINK) && retval == 0) {
		return -EINPROGRESS;
	}
	goto bye;
done:
	spinunlock (&hcd_data_lock);
	spin_unlock_irqrestore (&urb->hLock, flags);
bye:
	if (retval)
		printk("%i: hcd_unlink_urb fail %d\n",
		    hcd ? hcd->bus->nBusNum : -1,
		    retval);
	return retval;
}

/*-------------------------------------------------------------------------*/

/* called by khubd, rmmod, apmd, or other thread for hcd-private cleanup */

// FIXME:  likely best to have explicit per-setting (config+alt)
// setup primitives in the usbcore-to-hcd driver API, so nothing
// is implicit.  kernel 2.5 needs a bunch of config cleanup...

static int hcd_free_dev (USB_device_s *udev)
{
	struct hcd_dev		*dev;
	struct usb_hcd		*hcd;
	unsigned long		flags;

	if (!udev || !udev->pHCPrivate)
		return -EINVAL;

	if (!udev->psBus || !udev->psBus->pHCPrivate)
		return -ENODEV;

	// should udev->devnum == -1 ??

	dev = udev->pHCPrivate;
	hcd = udev->psBus->pHCPrivate;

	/* device driver problem with refcounts? */
	if (!list_empty (&dev->urb_list)) {
		printk("free busy dev, %i devnum %d (bug!)\n",
			hcd->bus->nBusNum, udev->nDeviceNum);
		return -EINVAL;
	}

	hcd->driver->free_config (hcd, udev);

	spin_lock_irqsave (&hcd_data_lock, flags);
	list_del (&dev->dev_list);
	udev->pHCPrivate = NULL;
	spin_unlock_irqrestore (&hcd_data_lock, flags);

	kfree (dev);
	return 0;
}

#if 0
static struct usb_operations hcd_operations = {
	allocate:		hcd_alloc_dev,
	get_frame_number:	hcd_get_frame_number,
	submit_urb:		hcd_submit_urb,
	unlink_urb:		hcd_unlink_urb,
	deallocate:		hcd_free_dev,
};
#endif

/*-------------------------------------------------------------------------*/

static int hcd_irq (int irq, void *__hcd, SysCallRegs_s * r)
{
	struct usb_hcd		*hcd = __hcd;
	int			start = hcd->state;

	if (hcd->state == USB_STATE_HALT)	/* irq sharing? */
		return( 0 );

	hcd->driver->irq (hcd, r);
	if (hcd->state != start && hcd->state == USB_STATE_HALT)
		hc_died (hcd);
		
	return( 0 );
}

/*-------------------------------------------------------------------------*/

/**
 * usb_hcd_giveback_urb - return URB from HCD to device driver
 * @hcd: host controller returning the URB
 * @urb: urb being returned to the USB device driver.
 * @regs: saved hardware registers (ignored on 2.4 kernels)
 * Context: in_interrupt()
 *
 * This hands the URB from HCD to its USB device driver, using its
 * completion function.  The HCD has freed all per-urb resources
 * (and is done using urb->hcpriv).  It also released all HCD locks;
 * the device driver won't cause deadlocks if it resubmits this URB,
 * and won't confuse things by modifying and resubmitting this one.
 * Bandwidth and other resources will be deallocated.
 *
 * HCDs must not use this for periodic URBs that are still scheduled
 * and will be reissued.  They should just call their completion handlers
 * until the urb is returned to the device driver by unlinking.
 *
 * NOTE that no urb->next processing is done, even for isochronous URBs.
 * ISO streaming functionality can be achieved by having completion handlers
 * re-queue URBs.  Such explicit queuing doesn't discard error reports.
 */
void usb_hcd_giveback_urb (struct usb_hcd *hcd, USB_packet_s *urb, SysCallRegs_s *regs)
{
	urb_unlink (urb);

	// NOTE:  a generic device/urb monitoring hook would go here.
	// hcd_monitor_hook(MONITOR_URB_FINISH, urb, dev)
	// It would catch exit/unlink paths for all urbs, but non-exit
	// completions for periodic urbs need hooks inside the HCD.
	// hcd_monitor_hook(MONITOR_URB_UPDATE, urb, dev)

	#if 0
	// NOTE:  2.5 does this if !URB_NO_DMA_MAP transfer flag
	if (usb_pipecontrol (urb->nPipe))
		pci_unmap_single (hcd->pdev, urb->pSetupPacket,
				sizeof (USB_request_s),
				PCI_DMA_TODEVICE);
	if (urb->transfer_buffer_length != 0)
		pci_unmap_single (hcd->pdev, urb->transfer_dma,
				urb->transfer_buffer_length,
				usb_pipein (urb->pipe)
				    ? PCI_DMA_FROMDEVICE
				    : PCI_DMA_TODEVICE);
	#endif
	/* pass ownership to the completion handler */
	urb->pComplete (urb);
}


/* PCI-based HCs are normal, but custom bus glue should be ok */

static int hcd_irq (int irq, void* __hcd, SysCallRegs_s* regs );
static void hc_died (struct usb_hcd *hcd);

/*-------------------------------------------------------------------------*/

/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */


inline uint32 pci_size(uint32 base, uint32 mask)
{
	uint32 size = base & mask;
	return size & ~(size-1);
}

static uint32 get_pci_memory_size(const PCI_Info_s *pcPCIInfo, int nResource)
{
	uint32 nSize, nBase;
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource*4;
	nBase = g_psPCIBus->read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	g_psPCIBus->write_pci_config(nBus, nDev, nFnc, nOffset, 4, ~0);
	nSize = g_psPCIBus->read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	g_psPCIBus->write_pci_config(nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	if (!(nSize & PCI_ADDRESS_SPACE)) {
		return pci_size(nSize, PCI_ADDRESS_MEMORY_32_MASK);
	} else {
		return pci_size(nSize, PCI_ADDRESS_IO_MASK & 0xffff);
	}
}

/**
 * usb_hcd_pci_probe - initialize PCI-based HCDs
 * @dev: USB Host Controller being probed
 * @id: pci hotplug id connecting controller to HCD framework
 * Context: !in_interrupt()
 *
 * Allocates basic PCI resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 * Store this function in the HCD's struct pci_driver as probe().
 */
int usb_hcd_pci_probe (int nDeviceID, PCI_Info_s dev, void* driver_data)
{
	struct hc_driver	*driver;
	unsigned long		resource, len;
	void			*base;
	USB_bus_driver_s		*bus;
	struct usb_hcd		*hcd;
	int			retval, region;
	char			buf [8], *bufp = buf;
	int irqhandle;

	if ( !(driver = (struct hc_driver *)driver_data))
		return -EINVAL;

	g_psPCIBus->write_pci_config( dev.nBus, dev.nDevice, dev.nFunction, PCI_COMMAND, 2, g_psPCIBus->read_pci_config( dev.nBus, dev.nDevice, dev.nFunction, PCI_COMMAND, 2 )
					| PCI_COMMAND_IO | PCI_COMMAND_MASTER );
	
    if (!dev.u.h0.nInterruptLine) {
      	kerndbg( KERN_FATAL, "Found HC with no IRQ.  Check BIOS/PCI %i setup!\n",
		dev.nDevice);
   	       return -ENODEV;
    }
	
	if (driver->flags & HCD_MEMORY) {	// EHCI, OHCI
		uint32 temp;
		region = 0;
		base = NULL;
		resource = dev.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
		len = get_pci_memory_size( &dev, 0 );
		len = PAGE_ALIGN( len );
		
		remap_area( create_area( driver->description, &base, len + PAGE_SIZE, len + PAGE_SIZE, AREA_ANY_ADDRESS|AREA_KERNEL,
								AREA_FULL_LOCK ), (void*)( resource & PAGE_MASK ) ); 
		temp = (uint32)base + ( resource - ( resource & PAGE_MASK ) );
		base = (void*)temp;
		
	} else { 				// UHCI
		resource = len = 0;
		for (region = 0; region < 6; region++) {
			unsigned int io_addr = *( &dev.u.h0.nBase0 + region );
			len = get_pci_memory_size( &dev, region );
		
			if( !( io_addr & PCI_ADDRESS_SPACE ) ) {
				continue;
			}

			resource = io_addr & PCI_ADDRESS_IO_MASK;
			break;
		}
		
		if (region == 6 ) {
			printk("no i/o regions available\n");
			return -EBUSY;
		}
		base = (void *) resource;
	}

	// driver->start(), later on, will transfer device from
	// control by SMM/BIOS to control by Linux (if needed)

	hcd = driver->hcd_alloc ();
	if (hcd == NULL){
		kerndbg( KERN_FATAL, "hcd alloc fail\n");
		retval = -ENOMEM;
clean_2:
		if (driver->flags & HCD_MEMORY) {
//			goto clean_1;
		} else {
			return retval;
		}
	}
	hcd->driver = driver;
	hcd->description = driver->description;
	hcd->pdev = dev;
	kerndbg(KERN_DEBUG, "%s %i\n",
			hcd->description,  dev.nDevice);

	sprintf (buf, "%d", dev.u.h0.nInterruptLine);
	if ( ( irqhandle = request_irq (dev.u.h0.nInterruptLine, hcd_irq, NULL, SA_SHIRQ, hcd->description, hcd) )
			< 0) {
		kerndbg( KERN_FATAL, "request interrupt %s failed\n", bufp);
		retval = -EBUSY;
clean_3:
		driver->hcd_free (hcd);
		goto clean_2;
	}
	hcd->irq = dev.u.h0.nInterruptLine;

	hcd->regs = base;
	hcd->region = region;
	kerndbg( KERN_DEBUG, "%s %i: irq %s, %s %p\n",
		hcd->description, dev.nDevice, bufp,
		(driver->flags & HCD_MEMORY) ? "pci mem" : "io base",
		base);

// FIXME simpler: make "bus" be that data, not pointer to it.
// (fixed in 2.5)
	bus = g_psBus->alloc_bus ();
	if (bus == NULL) {
		kerndbg( KERN_FATAL, "usb_alloc_bus fail\n");
		retval = -ENOMEM;
		release_irq (dev.u.h0.nInterruptLine, irqhandle);
		goto clean_3;
	}
	bus->AddDevice = hcd_alloc_dev;
	bus->RemoveDevice = hcd_free_dev;
	bus->GetFrameNumber = hcd_get_frame_number;
	bus->SubmitPacket = hcd_submit_urb;
	bus->CancelPacket = hcd_unlink_urb;
	bus->pHCPrivate = hcd;
	hcd->bus = bus;
	hcd->product_desc = "USB EHCI Root Hub";
	hcd->device_id = nDeviceID;

	INIT_LIST_HEAD (&hcd->dev_list);
	INIT_LIST_HEAD (&hcd->hcd_list);

	spinlock(&hcd_list_lock);
	list_add (&hcd->hcd_list, &hcd_list);
	spinunlock(&hcd_list_lock);

	g_psBus->add_bus( bus );

	if ((retval = driver->start (hcd)) < 0)
		usb_hcd_pci_remove (dev, hcd);

	return retval;
} 


/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */

/**
 * usb_hcd_pci_remove - shutdown processing for PCI-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_pci_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 * Store this function in the HCD's struct pci_driver as remove().
 */
void usb_hcd_pci_remove (PCI_Info_s dev, void* __hcd)
{
	struct usb_hcd		*hcd;
	USB_device_s	*hub;

	hcd = __hcd;
	if (!hcd)
		return;
	kerndbg( KERN_INFO, "%s %i: remove state %x\n",
		hcd->description,  dev.nDevice, hcd->state);

	hub = hcd->bus->psRootHUB;
	hcd->state = USB_STATE_QUIESCING;

	kerndbg( KERN_DEBUG, "%i: roothub graceful disconnect", hcd->bus->nBusNum);
	g_psBus->disconnect(&hub);
	// usb_disconnect (&hcd->bus->root_hub);

	hcd->driver->stop (hcd);
	hcd->state = USB_STATE_HALT;

//	free_irq (hcd->irq);
	if (hcd->driver->flags & HCD_MEMORY) {
		//iounmap (hcd->regs);
		//release_mem_region (pci_resource_start (dev, 0),
			//pci_resource_len (dev, 0));
	} else {
		//release_region (pci_resource_start (dev, hcd->region),
			//pci_resource_len (dev, hcd->region));
	}

	spinlock(&hcd_list_lock);
	list_del (&hcd->hcd_list);
	spinunlock(&hcd_list_lock);

	g_psBus->remove_bus(hcd->bus);
	g_psBus->free_bus(hcd->bus);
	hcd->bus = NULL;

	hcd->driver->hcd_free (hcd);
}



int hcd_bus_suspend (struct usb_hcd *hcd)
{
	if (!hcd->driver->suspend)
		return -ENOENT;
	
	/* Stop root hub timer */
	hcd->state = USB_STATE_SUSPENDED;
	delete_timer( hcd->rh_timer );
	
	/* Suspend */
	hcd->driver->suspend( hcd, 0 );
	
	return( 0 );
}

int hcd_bus_resume (struct usb_hcd *hcd)
{
	if (!hcd->driver->resume)
		return -ENOENT;
	
	/* Resume controller */
	hcd->state = USB_STATE_RESUMING;
	hcd->driver->resume( hcd, 0 );
	hcd->state = USB_STATE_RUNNING;	

	/* Start timer */
	hcd->rh_timer = create_timer();
	
	start_timer( hcd->rh_timer, rh_report_status, hcd->rh_timer_data, 1000 / 4 * 1000, true );
	
	
	return( 0 );
}

/*-------------------------------------------------------------------------*/


















