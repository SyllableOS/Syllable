/*
 *  The Syllable kernel
 *	USB busmanager
 *
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 2007 Kristian Van Der Vliet
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

#ifndef _SYLLABLE_LIBUSB_H_
#define _SYLLABLE_LIBUSB_H_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#include <atheos/types.h>
#include <posix/ioctl.h>

struct usb_ctl_request
{
	uint8 nRequest;
	uint8 nRequestType;
	uint16 nValue;
	uint16 nIndex;
	void* pData;
	uint16 nSize;
	bigtime_t nTimeout;
};

struct usb_alt_interface
{
	int nIFNum;
	int nAlternate;
};

struct usb_io_request
{
	int nEndpoint;
	void* pData;
	size_t nSize;
	bigtime_t nTimeout;
};

#define IOCNR_GET_DEVICE_DESC	1
#define IOCNR_DO_REQUEST		2
#define IOCNR_SET_CONFIGURATION	3
#define IOCNR_SET_ALTINTERFACE	4
#define IOCNR_BULK_READ			5
#define IOCNR_BULK_WRITE		6
#define IOCNR_INT_READ			7
#define IOCNR_INT_WRITE			8
#define IOCNR_CLEAR_HALT		9

#define USB_GET_DEVICE_DESC		MK_IOCTLR( IOCTL_MOD_BLOCK, IOCNR_GET_DEVICE_DESC, 18 )
#define USB_DO_REQUEST			MK_IOCTLRW( IOCTL_MOD_BLOCK, IOCNR_DO_REQUEST, sizeof( struct usb_ctl_request ) )
#define USB_SET_CONFIGURATION	MK_IOCTLW( IOCTL_MOD_BLOCK, IOCNR_SET_CONFIGURATION, sizeof( int ) )
#define USB_SET_ALTINTERFACE	MK_IOCTLW( IOCTL_MOD_BLOCK, IOCNR_SET_ALTINTERFACE, sizeof( struct usb_alt_interface ) )
#define USB_BULK_READ			MK_IOCTLR( IOCTL_MOD_BLOCK, IOCNR_BULK_READ, sizeof( struct usb_io_request ) )
#define USB_BULK_WRITE			MK_IOCTLW( IOCTL_MOD_BLOCK, IOCNR_BULK_WRITE, sizeof( struct usb_io_request ) )
#define USB_INT_READ			MK_IOCTLR( IOCTL_MOD_BLOCK, IOCNR_INT_READ, sizeof( struct usb_io_request ) )
#define USB_INT_WRITE			MK_IOCTLW( IOCTL_MOD_BLOCK, IOCNR_INT_WRITE, sizeof( struct usb_io_request ) )
#define USB_CLEAR_HALT			MK_IOCTLN( IOCTL_MOD_BLOCK, IOCNR_CLEAR_HALT, sizeof( int ) )

#ifdef __cplusplus
}
#endif

#endif

