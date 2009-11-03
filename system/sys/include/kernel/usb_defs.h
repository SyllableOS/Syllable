/*
 *  The Syllable kernel
 *	USB busmanager
 *
 *	Ported from the linux USB code:
 *	(C) Copyright Linus Torvalds 1999
 *	(C) Copyright Johannes Erdfelt 1999-2001
 *	(C) Copyright Andreas Gal 1999
 *	(C) Copyright Gregory P. Smith 1999
 *	(C) Copyright Deti Fliegl 1999 (new USB architecture)
 *	(C) Copyright Randy Dunlap 2000
 *	(C) Copyright David Brownell 2000 (kernel hotplug, usb_device_id)
 *	(C) Copyright Yggdrasil Computing, Inc. 2000
 *     (usb_device_id matching changes by Adam J. Richter)
 *  
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
 */

#ifndef __F_KERNEL_USB_DEFS_H_
#define __F_KERNEL_USB_DEFS_H_

#include <kernel/types.h>

/* Standard descriptor header */
typedef struct 
{
	uint8 	nLength;
	uint8	nDescriptorType;
} USB_desc_header_s __attribute__ ((packed));

/* Device descriptor */
typedef struct
{
	uint8 	nLength;
	uint8	nDescriptorType;
	uint16	nCdUSB;
	uint8	nDeviceClass;
	uint8	nDeviceSubClass;
	uint8	nDeviceProtocol;
	uint8	nMaxPacketSize;
	uint16	nVendorID;
	uint16	nDeviceID;
	uint16	bCdDevice;
	uint8	nManufacturer;
	uint8	nProduct;
	uint8	nSerialNumber;
	uint8	nNumConfigurations;
} USB_desc_device_s __attribute__ ((packed));

/* Endpoint descriptor */
typedef struct
{
	uint8 	nLength __attribute__ ((packed));
	uint8	nDescriptorType	__attribute__ ((packed));
	uint8	nEndpointAddress __attribute__ ((packed));
	uint8	nMAttributes __attribute__ ((packed));
	uint16	nMaxPacketSize __attribute__ ((packed));
	uint8	nInterval __attribute__ ((packed));
	uint8	nRefresh __attribute__ ((packed));
	uint8	nSynchAddress __attribute__ ((packed));
	
	uint8*	pExtra;
	int		nExtraLen;
} USB_desc_endpoint_s;


/* Interface descriptor */
typedef struct
{
	uint8 	nLength __attribute__ ((packed));;
	uint8	nDescriptorType __attribute__ ((packed));;
	uint8	nInterfaceNumber __attribute__ ((packed));;
	uint8	nAlternateSettings __attribute__ ((packed));;
	uint8	nNumEndpoints __attribute__ ((packed));;
	uint8	nInterfaceClass __attribute__ ((packed));;
	uint8	nInterfaceSubClass __attribute__ ((packed));;
	uint8	nInterfaceProtocol __attribute__ ((packed));;
	uint8	nInterface __attribute__ ((packed));;
	
	USB_desc_endpoint_s* psEndpoint;
	
	uint8*	pExtra;
	int		nExtraLen;
} USB_desc_interface_s __attribute__ ((packed));

/* Structure which describes one interface */

struct USB_driver_t;

typedef struct
{
	USB_desc_interface_s* psSetting;
	
	int	nActSetting;
	int nNumSetting;
	int nMaxSetting;
	
	struct USB_driver_t* psDriver;
	
	void* pPrivate;
	
	struct USB_libusb_node_t* psUSBNode;
} USB_interface_s;

/* Config descriptor */
typedef struct
{
	uint8 	nLength __attribute__ ((packed));;
	uint8	nDescriptorType __attribute__ ((packed));;
	uint16	nTotalLength __attribute__ ((packed));;
	uint8	nNumInterfaces __attribute__ ((packed));;
	uint8	nConfigValue __attribute__ ((packed));;
	uint8	nConfig __attribute__ ((packed));;
	uint8	nMAttributes __attribute__ ((packed));;
	uint8	nMaxPower __attribute__ ((packed));;
	
	USB_interface_s* psInterface;
	
	uint8*	pExtra;
	int		nExtraLen;
} USB_desc_config_s;

/* String descriptor */
typedef struct
{
	uint8 	nLength;
	uint8	nDescriptorType;
	uint16	nData[1];
} USB_desc_string_s __attribute__ ((packed));

/* Request */
typedef struct {
	uint8	nRequestType;
	uint8	nRequest;
	uint16	nValue;
	uint16	nIndex;
	uint16	nLength;
} USB_request_s __attribute__ ((packed));



/* USB constants */

/* 
 * Speeds 
 */
 
enum {
	USB_SPEED_UNKNOWN = 0,
	USB_SPEED_LOW, USB_SPEED_FULL,	/* USB 1 */
	USB_SPEED_HIGH					/* USB 2 */
};

/* 
 * Packet transfer flags
 */

#define USB_DISABLE_SPD			0x0001
#define USB_SHORT_NOT_OK		0x0001
#define USB_ISO_ASAP			0x0002
#define USB_ASYNC_UNLINK		0x0008
#define USB_QUEUE_BULK			0x0010
#define USB_NO_FSBR				0x0020
#define USB_ZERO_PACKET			0x0040  
#define USB_NO_INTERRUPT		0x0080
#define USB_TIMEOUT_KILLED		0x1000

/*
 * Device and/or Interface Class codes
 */
#define USB_CLASS_PER_INTERFACE		0	/* for DeviceClass */
#define USB_CLASS_AUDIO				1
#define USB_CLASS_COMM				2
#define USB_CLASS_HID				3
#define USB_CLASS_PHYSICAL			5
#define USB_CLASS_STILL_IMAGE		6
#define USB_CLASS_PRINTER			7
#define USB_CLASS_MASS_STORAGE		8
#define USB_CLASS_HUB				9
#define USB_CLASS_CDC_DATA			0x0a
#define USB_CLASS_CSCID				0x0b /* chip+ smart card */
#define USB_CLASS_CONTENT_SEC		0x0d /* content security */
#define USB_CLASS_APP_SPEC			0xfe
#define USB_CLASS_VENDOR_SPEC		0xff

/*
 * USB types
 */
#define USB_TYPE_MASK			(0x03 << 5)
#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_TYPE_RESERVED		(0x03 << 5)

/*
 * USB recipients
 */
#define USB_RECIP_MASK			0x1f
#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE		0x01
#define USB_RECIP_ENDPOINT		0x02
#define USB_RECIP_OTHER			0x03

/*
 * USB directions
 */
#define USB_DIR_OUT			0
#define USB_DIR_IN			0x80

/*
 * Descriptor types
 */
#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE		0x04
#define USB_DT_ENDPOINT			0x05

#define USB_DT_HID			(USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT			(USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL			(USB_TYPE_CLASS | 0x03)
#define USB_DT_HUB			(USB_TYPE_CLASS | 0x09)

/*
 * Descriptor sizes per descriptor type
 */
#define USB_DT_DEVICE_SIZE		18
#define USB_DT_CONFIG_SIZE		9
#define USB_DT_INTERFACE_SIZE		9
#define USB_DT_ENDPOINT_SIZE		7
#define USB_DT_ENDPOINT_AUDIO_SIZE	9	/* Audio extension */
#define USB_DT_HUB_NONVAR_SIZE		7
#define USB_DT_HID_SIZE			9

/*
 * Endpoints
 */
#define USB_ENDPOINT_NUMBER_MASK	0x0f	/* in nEndpointAddress */
#define USB_ENDPOINT_DIR_MASK		0x80

#define USB_ENDPOINT_XFERTYPE_MASK	0x03	/* in MAttributes */
#define USB_ENDPOINT_XFER_CONTROL	0
#define USB_ENDPOINT_XFER_ISOC		1
#define USB_ENDPOINT_XFER_BULK		2
#define USB_ENDPOINT_XFER_INT		3

/*
 * USB Packet IDs (PIDs)
 */
#define USB_PID_UNDEF_0                        0xf0
#define USB_PID_OUT                            0xe1
#define USB_PID_ACK                            0xd2
#define USB_PID_DATA0                          0xc3
#define USB_PID_PING                           0xb4	/* USB 2.0 */
#define USB_PID_SOF                            0xa5
#define USB_PID_NYET                           0x96	/* USB 2.0 */
#define USB_PID_DATA2                          0x87	/* USB 2.0 */
#define USB_PID_SPLIT                          0x78	/* USB 2.0 */
#define USB_PID_IN                             0x69
#define USB_PID_NAK                            0x5a
#define USB_PID_DATA1                          0x4b
#define USB_PID_PREAMBLE                       0x3c	/* Token mode */
#define USB_PID_ERR                            0x3c	/* USB 2.0: handshake mode */
#define USB_PID_SETUP                          0x2d
#define USB_PID_STALL                          0x1e
#define USB_PID_MDATA                          0x0f	/* USB 2.0 */

/*
 * Standard requests
 */
#define USB_REQ_GET_STATUS			0x00
#define USB_REQ_CLEAR_FEATURE		0x01
#define USB_REQ_SET_FEATURE			0x03
#define USB_REQ_SET_ADDRESS			0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME			0x0C

/*
 * HID requests
 */
#define USB_REQ_GET_REPORT			0x01
#define USB_REQ_GET_IDLE			0x02
#define USB_REQ_GET_PROTOCOL		0x03
#define USB_REQ_SET_REPORT			0x09
#define USB_REQ_SET_IDLE			0x0A
#define USB_REQ_SET_PROTOCOL		0x0B

#endif	/* __F_KERNEL_USB_DEFS_H_ */