/*
 *  The Syllable kernel
 *  USB printer & printer cable driver
 *  Copyright (C) 2006 Kristian Van Der Vliet
 *  Copyright (C) 2003 Arno Klenke
 *
 *  Based on the Linux usblp driver
 *  Copyright (c) 1999 Michael Gee	<michael@linuxspecific.com>
 *  Copyright (c) 1999 Pavel Machek	<pavel@suse.cz>
 *  Copyright (c) 2000 Randy Dunlap	<rddunlap@osdl.org>
 *  Copyright (c) 2000 Vojtech Pavlik	<vojtech@suse.cz>
 *  Copyright (c) 2001 Pete Zaitcev	<zaitcev@redhat.com>
 *  Copyright (c) 2001 David Paschal	<paschal@rcsis.com>
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

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/string.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/smp.h>
#include <atheos/usb.h>
#include <atheos/lp.h>

#include <posix/errno.h>

#define USB_PRINTER_WRITE_TIMEOUT	(50000)			/* 5 seconds */

#define USB_PRINTER_FIRST_PROTOCOL	1
#define USB_PRINTER_LAST_PROTOCOL	3
#define USB_PRINTER_MAX_PROTOCOLS	(USB_PRINTER_LAST_PROTOCOL+1)

#define USB_PRINTER_REQ_GET_ID						0x00
#define USB_PRINTER_REQ_GET_STATUS					0x01
#define USB_PRINTER_REQ_RESET						0x02
#define USB_PRINTER_REQ_HP_CHANNEL_CHANGE_REQUEST	0x00	/* HP Vendor-specific */

#define USB_PRINTER_QUIRK_BIDIR		0x1		/* reports bidir but requires unidirectional mode (no INs/reads) */
#define USB_PRINTER_QUIRK_USB_INIT	0x2		/* needs vendor USB init string */

#define USB_PRINTER_BUF_SIZE		8192
#define USB_PRINTER_STATUS_BUF_SIZE	8
#define USB_PRINTER_DEVICE_ID_LEN	1024
#define USB_PRINTER_NAME_LEN		128

#define IOCNR_GET_DEVICE_ID			1
#define IOCNR_GET_PROTOCOLS			2
#define IOCNR_SET_PROTOCOL			3
#define IOCNR_HP_SET_CHANNEL		4
#define IOCNR_GET_BUS_ADDRESS		5
#define IOCNR_GET_VID_PID			6
#define IOCNR_SOFT_RESET			7

struct quirks
{
	uint16 nVendorID;
	uint16 nDeviceID;
	unsigned int nQuirks;
};

#define USB_PRINTER_QUIRK_BIDIR		0x1	/* reports bidir but requires unidirectional mode (no INs/reads) */
#define USB_PRINTER_QUIRK_USB_INIT	0x2	/* needs vendor USB init string */

static struct quirks quirk_printers[] = {
	{ 0x03f0, 0x0004, USB_PRINTER_QUIRK_BIDIR }, /* HP DeskJet 895C */
	{ 0x03f0, 0x0104, USB_PRINTER_QUIRK_BIDIR }, /* HP DeskJet 880C */
	{ 0x03f0, 0x0204, USB_PRINTER_QUIRK_BIDIR }, /* HP DeskJet 815C */
	{ 0x03f0, 0x0304, USB_PRINTER_QUIRK_BIDIR }, /* HP DeskJet 810C/812C */
	{ 0x03f0, 0x0404, USB_PRINTER_QUIRK_BIDIR }, /* HP DeskJet 830C */
	{ 0x03f0, 0x0504, USB_PRINTER_QUIRK_BIDIR }, /* HP DeskJet 885C */
	{ 0x03f0, 0x0604, USB_PRINTER_QUIRK_BIDIR }, /* HP DeskJet 840C */   
	{ 0x03f0, 0x0804, USB_PRINTER_QUIRK_BIDIR }, /* HP DeskJet 816C */   
	{ 0x03f0, 0x1104, USB_PRINTER_QUIRK_BIDIR }, /* HP Deskjet 959C */
	{ 0x0409, 0xefbe, USB_PRINTER_QUIRK_BIDIR }, /* NEC Picty900 (HP OEM) */
	{ 0x0409, 0xbef4, USB_PRINTER_QUIRK_BIDIR }, /* NEC Picty760 (HP OEM) */
	{ 0x0409, 0xf0be, USB_PRINTER_QUIRK_BIDIR }, /* NEC Picty920 (HP OEM) */
	{ 0x0409, 0xf1be, USB_PRINTER_QUIRK_BIDIR }, /* NEC Picty800 (HP OEM) */
	{ 0, 0 }
};

struct usb_printer {
	char zName[USB_PRINTER_NAME_LEN];

	USB_device_s *psDev;
	int hNode;
	sem_id hDeviceLock;

	/* Alternate-setting numbers and endpoints for each protocol
	 * (7/1/{index=1,2,3}) that the device supports: */
	struct
	{
		int nAltSetting;
		USB_desc_endpoint_s *psReadEndpoint;
		USB_desc_endpoint_s *psWriteEndpoint;
	} sProtocol[USB_PRINTER_MAX_PROTOCOLS];
	uint8 nCurrentProtocol;
	uint8 nInterface;
	uint8 nQuirks;
	bool bBidir;

	char *zPrinterID;		/* IEEE 1284 DEVICE ID */
	uint8 *pStatusBuffer;	/* Printer status data */

	/* For writing */
	USB_packet_s sWrite;
	int nWritePipe;
	uint8 *pWriteBuffer;
	sem_id hWriteLock;
	bool bWriteComplete;

	/* For reading */
	USB_packet_s sRead;
	int nReadPipe;
	uint8 *pReadBuffer;
	sem_id hReadLock;
	bool bReadComplete;
	size_t nReadCount;

	bool bConnected;
	bool bOpen;
};

USB_bus_s* g_psBus = NULL;
USB_driver_s* g_pcDriver;
int g_nDeviceID;

void usbprinter_write_irq( USB_packet_s *psPacket );
void usbprinter_read_irq( USB_packet_s *psPacket );
static void usbprinter_cancel( struct usb_printer *psPrinter );
static void usbprinter_cleanup( struct usb_printer *psPrinter );
static int usbprinter_get_device_id( struct usb_printer *psPrinter );
static int usbprinter_set_protocol( struct usb_printer *psPrinter, int nProtocol );

/* XXXKV: This should be somewhere else; see also atapi.h */
static inline uint16 be16_to_cpu(uint16 be_val)
{
	return( ( be_val & 0xff00 ) >> 8 |
		    ( be_val & 0x00ff ) << 8 );
}

static int usbprinter_ctrl_msg( struct usb_printer *psPrinter, int nReq, int nType, int nDir, int nRecip, int nVal, void *pBuf, int nLen )
{
	int nError;
	int nIndex = psPrinter->nInterface;

	/* High byte has the interface index.
	   Low byte has the alternate setting.
	 */
	if( ( nReq == USB_PRINTER_REQ_GET_ID ) && ( nType == USB_TYPE_CLASS ) )
		nIndex = (psPrinter->nInterface<<8)|psPrinter->sProtocol[psPrinter->nCurrentProtocol].nAltSetting;

	nError = g_psBus->control_msg(psPrinter->psDev,
		nDir ? usb_rcvctrlpipe(psPrinter->psDev, 0) : usb_sndctrlpipe(psPrinter->psDev, 0),
		nReq, nType | nDir | nRecip, nVal, nIndex, pBuf, nLen, USB_PRINTER_WRITE_TIMEOUT);
	kerndbg( KERN_DEBUG, "usbprinter_ctrl_msg: rq: 0x%02x dir: %d recip: %d value: %d idx: %d len: %#x result: %d\n",
		nReq, !!nDir, nRecip, nVal, nIndex, nLen, nError);
	return nError < 0 ? nError : 0;
}

#define usbprinter_read_status(usbprinter, status)\
	usbprinter_ctrl_msg(usbprinter, USB_PRINTER_REQ_GET_STATUS, USB_TYPE_CLASS, USB_DIR_IN, USB_RECIP_INTERFACE, 0, status, 1)
#define usbprinter_get_id(usbprinter, config, id, maxlen)\
	usbprinter_ctrl_msg(usbprinter, USB_PRINTER_REQ_GET_ID, USB_TYPE_CLASS, USB_DIR_IN, USB_RECIP_INTERFACE, config, id, maxlen)
#define usbprinter_reset(usbprinter)\
	usbprinter_ctrl_msg(usbprinter, USB_PRINTER_REQ_RESET, USB_TYPE_CLASS, USB_DIR_OUT, USB_RECIP_OTHER, 0, NULL, 0)

#define usbprinter_hp_channel_change_request(usbprinter, channel, buffer) \
	usbprinter_ctrl_msgg(usbprinter, USB_PRINTER_REQ_HP_CHANNEL_CHANGE_REQUEST, USB_TYPE_VENDOR, USB_DIR_IN, USB_RECIP_INTERFACE, channel, buffer, 1)

static char *usb_printer_messages[] = { "O.K", "Out of paper", "Off-line", "Dead?" };

static int usbprinter_check_status( struct usb_printer *psPrinter, int nOldError )
{
	uint8 nStatus, nNewError = 0;
	int nError;

	nError = usbprinter_read_status( psPrinter, psPrinter->pStatusBuffer );
	if( nError < 0 )
	{
		kerndbg( KERN_WARNING, "Error %d reading printer status", nError );
		return 0;
	}

	nStatus = *( psPrinter->pStatusBuffer );
	if( ~nStatus & LP_PERRORP )
		nNewError = 3;
	if( nStatus & LP_POUTPA )
		nNewError = 1;
	if( ~nStatus & LP_PSELECD )
		nNewError = 2;

	if( nNewError != nOldError )
		kerndbg( KERN_DEBUG, "Printer is: %s", usb_printer_messages[nNewError] );

	return nNewError;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbprinter_open( void* pNode, uint32 nFlags, void **pCookie )
{
	struct usb_printer *psPrinter = (struct usb_printer*)pNode;

	if( NULL == psPrinter || NULL == psPrinter->psDev || psPrinter->bConnected == false )
		return ENODEV;

	if( psPrinter->bOpen )
		return EBUSY;

	LOCK( psPrinter->hDeviceLock );

	psPrinter->sWrite.nBufferLength = 0;
	psPrinter->sWrite.nStatus = psPrinter->sRead.nStatus = 0;
	psPrinter->bWriteComplete = true;

	if( psPrinter->bBidir )
	{
		/* Refill the URB and re-submit it ready for an attempt to read from the device */
		USB_FILL_BULK(&psPrinter->sRead, psPrinter->psDev, psPrinter->nReadPipe, psPrinter->pReadBuffer, USB_PRINTER_BUF_SIZE, usbprinter_read_irq, psPrinter);
		psPrinter->bReadComplete = false;
		psPrinter->nReadCount = 0;

		if( g_psBus->submit_packet( &psPrinter->sRead ) < 0 )
		{
			UNLOCK( psPrinter->hDeviceLock );
			return EIO;
		}
	}

	psPrinter->bOpen = true;

	UNLOCK( psPrinter->hDeviceLock );
	return EOK;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbprinter_close( void* pNode, void* pCookie )
{
	struct usb_printer *psPrinter = (struct usb_printer*)pNode;

	if( NULL == psPrinter || NULL == psPrinter->psDev )
		return ENODEV;

	LOCK( psPrinter->hDeviceLock );
	psPrinter->bOpen = false;

	usbprinter_cancel( psPrinter );
	UNLOCK( psPrinter->hDeviceLock );

	/* Check to see if the device was unplugged while we were open */
	if( psPrinter->bConnected == false )
		usbprinter_cleanup( psPrinter );

	return EOK;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbprinter_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct usb_printer *psPrinter = (struct usb_printer*)pNode;
	status_t nError;
	int i, nStatus, nLength;
	int anTwoints[2];

	if( psPrinter == NULL )
		return ENODEV;

	LOCK( psPrinter->hDeviceLock );
	if( psPrinter->bConnected == false )
	{
		nError = ENODEV;
		goto out;
	}

	kerndbg( KERN_DEBUG_LOW, "ioctl() nCommand=0x%x (%c nr=%d len=%d dir=%d)\n", nCommand, _IOC_TYPE(nCommand),
		_IOC_NR(nCommand), _IOC_SIZE(nCommand), _IOC_DIR(nCommand) );

	if (_IOC_TYPE(nCommand) == 'P')	/* new-style ioctl number */
	{
		switch (_IOC_NR(nCommand))
		{
			case IOCNR_GET_DEVICE_ID: /* get the DEVICE_ID string */
			{
				if(_IOC_DIR(nCommand) != _IOC_READ)
				{
					nError = EINVAL;
					goto out;
				}

				nLength = usbprinter_get_device_id( psPrinter );
				if( nLength < 0 )
				{
					nError = EIO;
					goto out;
				}
				if( nLength > _IOC_SIZE(nCommand) )
					nLength = _IOC_SIZE(nCommand); /* truncate */

				if( memcpy_to_user( pArgs, psPrinter->zPrinterID, (unsigned long)nLength ) )
				{
					nError = EFAULT;
					goto out;
				}

				break;
			}

			case IOCNR_GET_PROTOCOLS:
			{
				if(_IOC_DIR(nCommand) != _IOC_READ || _IOC_SIZE(nCommand) < sizeof(anTwoints))
				{
					nError = EINVAL;
					goto out;
				}

				anTwoints[0] = psPrinter->nCurrentProtocol;
				anTwoints[1] = 0;
				for( i = USB_PRINTER_FIRST_PROTOCOL; i <= USB_PRINTER_LAST_PROTOCOL; i++ )
					if( psPrinter->sProtocol[i].nAltSetting >= 0 )
						anTwoints[1] |= (1<<i);

				if( memcpy_to_user(pArgs, (unsigned char *)anTwoints, sizeof(anTwoints)))
				{
					nError = EFAULT;
					goto out;
				}

				break;
			}

			case IOCNR_SET_PROTOCOL:
			{
				int nProtocol;

				if(_IOC_DIR(nCommand) != _IOC_WRITE)
				{
					nError = EINVAL;
					goto out;
				}

				if( memcpy_from_user( &nProtocol, pArgs, sizeof(int) ))
				{
					nError = EFAULT;
					goto out;
				}

				usbprinter_cancel( psPrinter );
				nError = usbprinter_set_protocol( psPrinter, nProtocol );
				if( nError < 0 )
					usbprinter_set_protocol( psPrinter, psPrinter->nCurrentProtocol );
				break;
			}

			case IOCNR_HP_SET_CHANNEL:
			{
				/* XXXKV: Implement */
				break;
			}

			case IOCNR_GET_BUS_ADDRESS:
			{
				if(_IOC_DIR(nCommand) != _IOC_READ || _IOC_SIZE(nCommand) < sizeof(anTwoints)) {
					nError = EINVAL;
					goto out;
				}

				anTwoints[0] = psPrinter->psDev->psBus->nBusNum;
				anTwoints[1] = psPrinter->psDev->nDeviceNum;
				if( memcpy_to_user(pArgs, (unsigned char *)anTwoints, sizeof(anTwoints)))
				{
					nError = EFAULT;
					goto out;
				}

				kerndbg( KERN_DEBUG, "usb_printer: bus=%d, device=%d", anTwoints[0], anTwoints[1] );
				break;
			}

			case IOCNR_GET_VID_PID:
			{
				if(_IOC_DIR(nCommand) != _IOC_READ || _IOC_SIZE(nCommand) < sizeof(anTwoints))
				{
					nError = EINVAL;
					goto out;
				}

				anTwoints[0] = psPrinter->psDev->sDeviceDesc.nVendorID;
				anTwoints[1] = psPrinter->psDev->sDeviceDesc.nDeviceID;	/* idProduct */
				if( memcpy_to_user(pArgs, (unsigned char *)anTwoints, sizeof(anTwoints)))
				{
					nError = EFAULT;
					goto out;
				}

				kerndbg( KERN_DEBUG, "usb_printer: VID=0x%4.4X, PID=0x%4.4X", anTwoints[0], anTwoints[1] );
				break;
			}

			case IOCNR_SOFT_RESET:
			{
				if(_IOC_DIR(nCommand) != _IOC_NONE)
				{
					nError = EINVAL;
					goto out;
				}
				nError = usbprinter_reset( psPrinter );
				break;
			}

			default:
				nError = -ENOTTY;
		}
	}
	else	/* old-style ioctl value */
	{
		switch(nCommand)
		{
			case LPGETSTATUS:
			{
				if( usbprinter_read_status(psPrinter, psPrinter->pStatusBuffer) )
				{
					kerndbg( KERN_WARNING, "usb_printer: failed to read printer status" );
					nError = EIO;
					goto out;
				}
				nStatus = *(psPrinter->pStatusBuffer);
				if(memcpy_to_user(pArgs, &nStatus, sizeof(int)))
					nError = EFAULT;
				break;
			}

			default:
				nError = ENOTTY;
		}
	}

out:
	UNLOCK( psPrinter->hDeviceLock );
    return nError;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void usbprinter_read_irq( USB_packet_s *psPacket )
{
	struct usb_printer *psPrinter = psPacket->pCompleteData;

	if ( psPacket->nStatus )
		return;

	psPrinter->bReadComplete = true;
	wakeup_sem( psPrinter->hReadLock, false );
}

int usbprinter_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
	struct usb_printer *psPrinter = (struct usb_printer*)pNode;
	bigtime_t nTimeout;
	status_t nError;
	size_t nReadCount = 0;

	if( psPrinter->bBidir == false || psPrinter->bConnected == false )
		return -ENODEV;

	/* Wait for data from the device */
	if( psPrinter->bReadComplete == false )
	{
		nTimeout = INFINITE_TIMEOUT;
		sleep_on_sem( psPrinter->hReadLock, nTimeout );
		if( is_signal_pending() )
			return nReadCount ? nReadCount : -EINTR;
		if( psPrinter->bConnected == false )
			return -ENODEV;
	}

	LOCK( psPrinter->hDeviceLock );

	if( psPrinter->sRead.nStatus != 0 )
	{
		kerndbg( KERN_WARNING, "Error %d reading from printer.", psPrinter->sRead.nStatus );

		/* Read failed.  Refill the URB and re-submit it ready for the next attempt to read from the device */
		USB_FILL_BULK(&psPrinter->sRead, psPrinter->psDev, psPrinter->nReadPipe, psPrinter->pReadBuffer, USB_PRINTER_BUF_SIZE, usbprinter_read_irq, psPrinter);
		psPrinter->sRead.nBufferLength = 0;
		psPrinter->bReadComplete = false;
		psPrinter->nReadCount = 0;

		nError = g_psBus->submit_packet( &psPrinter->sRead );
		if( nError != EOK )
		{
			kerndbg( KERN_WARNING, "submit_packet() failed with error %d\n", nError );
			nReadCount = -EIO;
			goto out;
		}
	}

	nReadCount = nReadCount < ( psPrinter->sRead.nActualLength - psPrinter->nReadCount ) ?
		nReadCount : ( psPrinter->sRead.nActualLength - psPrinter->nReadCount );

	if( memcpy_to_user( pBuffer, psPrinter->pReadBuffer + psPrinter->nReadCount, nReadCount ) != EOK )
	{
		nReadCount = -EFAULT;
		goto out;
	}

	psPrinter->nReadCount += nReadCount;
	if( psPrinter->nReadCount == psPrinter->sRead.nActualLength )
	{
		/* Read complete.  Refill the URB and re-submit it ready for the next attempt to read from the device */
		USB_FILL_BULK(&psPrinter->sRead, psPrinter->psDev, psPrinter->nReadPipe, psPrinter->pReadBuffer, USB_PRINTER_BUF_SIZE, usbprinter_read_irq, psPrinter);
		psPrinter->sRead.nBufferLength = 0;
		psPrinter->bReadComplete = false;
		psPrinter->nReadCount = 0;

		nError = g_psBus->submit_packet( &psPrinter->sRead );
		if( nError != EOK )
		{
			kerndbg( KERN_WARNING, "submit_packet() failed with error %d\n", nError );
			nReadCount = -EIO;
			goto out;
		}
	}

out:
	UNLOCK( psPrinter->hDeviceLock );

	return nReadCount;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void usbprinter_write_irq( USB_packet_s *psPacket )
{
	struct usb_printer *psPrinter = psPacket->pCompleteData;

	if ( psPacket->nStatus )
		return;

	psPrinter->bWriteComplete = true;
	wakeup_sem( psPrinter->hWriteLock, false );
}

int usbprinter_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	struct usb_printer *psPrinter = (struct usb_printer*)pNode;
	bigtime_t nTimeout;
	int nError = 0, nTransferSize = 0;
	size_t nWriteCount = 0;

	while( nWriteCount < nSize )
	{
		/* Wait for the previous packet to complete */
		if( psPrinter->bWriteComplete == false )
		{
			nTimeout = USB_PRINTER_WRITE_TIMEOUT;
			sleep_on_sem( psPrinter->hWriteLock, nTimeout );

			if( is_signal_pending() )
				return nWriteCount ? nWriteCount : -EINTR;
		}

		LOCK( psPrinter->hDeviceLock );
		if( psPrinter->bConnected == false )
		{
			UNLOCK( psPrinter->hDeviceLock );
			return -ENODEV;
		}

		if( psPrinter->sWrite.nStatus != 0 )
		{
			if( psPrinter->nQuirks & USB_PRINTER_QUIRK_BIDIR )
			{
				if( psPrinter->bWriteComplete == false )
				{
					kerndbg( KERN_WARNING, "Error %d writing to printer", psPrinter->sWrite.nStatus );
					nError = psPrinter->sWrite.nStatus;
				}
			}
			else
				nError = usbprinter_check_status( psPrinter, nError );

			UNLOCK( psPrinter->hDeviceLock );
			continue;
		}

		/* We must increment writecount here, and not at the end of the loop. Otherwise, the final loop iteration may
		 * be skipped, leading to incomplete printer output. */
		nWriteCount += nTransferSize;
		if( nWriteCount == nSize )
		{
			UNLOCK( psPrinter->hDeviceLock );
			break;
		}

		/* Refill URB */
		USB_FILL_BULK(&psPrinter->sWrite, psPrinter->psDev, psPrinter->nWritePipe, psPrinter->pWriteBuffer, USB_PRINTER_BUF_SIZE, usbprinter_write_irq, psPrinter);

		nTransferSize = ( nSize - nWriteCount );
		if( nTransferSize > USB_PRINTER_BUF_SIZE )
			nTransferSize = USB_PRINTER_BUF_SIZE;

		psPrinter->sWrite.nBufferLength = nTransferSize;
		if( memcpy_from_user( psPrinter->pWriteBuffer, pBuffer + nWriteCount, nTransferSize ) != EOK )
		{
			UNLOCK( psPrinter->hDeviceLock );
			return nWriteCount ? nWriteCount : -EFAULT;
		}

		psPrinter->bWriteComplete = false;
		nError = g_psBus->submit_packet( &psPrinter->sWrite );
		if( nError != EOK )
		{
			kerndbg( KERN_WARNING, "submit_packet() failed with error %d\n", nError );

			if( nError != ENOMEM )
				nSize = -EIO;
			else
				nSize = nWriteCount ? nWriteCount : -ENOMEM;

			UNLOCK( psPrinter->hDeviceLock );
			break;		
		}

		UNLOCK( psPrinter->hDeviceLock );
	}

    return nSize;
}

DeviceOperations_s g_sOperations = {
    usbprinter_open,
    usbprinter_close,
    usbprinter_ioctl,
    usbprinter_read,
    usbprinter_write
};

/* A DEVICE_ID string may include the printer's serial number.
   It should end with a semi-colon (';').  An example from an HP 970C DeskJet printer is

	MFG:HEWLETT-PACKARD;MDL:DESKJET 970C;CMD:MLC,PCL,PML;CLASS:PRINTER;DESCRIPTION:Hewlett-Packard DeskJet 970C;SERN:US970CSEPROF;VSTATUS:$HB0$NC0,ff,DN,IDLE,CUT,K1,C0,DP,NR,KP000,CP027;VP:0800,FL,B0;VJ:                    ;
 */

static int usbprinter_get_device_id( struct usb_printer *psPrinter )
{
	const char *zDescLong="DESCRIPTION:", *zDescShort="DES:";
	char *pStart, *pEnd;
	int nDescLen, nError, nLength;

	psPrinter->zPrinterID[0] = psPrinter->zPrinterID[1] = '\0';
	psPrinter->zName[0] = '\0';

	nError = usbprinter_get_id(psPrinter, 0, psPrinter->zPrinterID, USB_PRINTER_DEVICE_ID_LEN - 1);
	if( nError < 0 )
	{
		kerndbg( KERN_WARNING, "Error %d reading IEEE-1284 Device ID string\n", nError);
		return -EIO;
	}

	/* First two bytes are length in big-endian. They count themselves, and we copy them
	   into the user's buffer. */
	nLength = be16_to_cpu(*((uint16 *)psPrinter->zPrinterID));
	if (nLength < 2)
		nLength = 2;
	else if(nLength >= USB_PRINTER_DEVICE_ID_LEN )
		nLength = USB_PRINTER_DEVICE_ID_LEN - 1;
	psPrinter->zPrinterID[nLength] = '\0';

	kerndbg( KERN_DEBUG, "Device ID string [len=%d]=\"%s\"\n", nLength, &psPrinter->zPrinterID[2]);

	/* Extract the description */
	pStart = strstr( &psPrinter->zPrinterID[2], zDescShort );
	if( pStart != NULL )
	{
		kerndbg( KERN_DEBUG_LOW, "Found \"%s\" in IEEE string\n", zDescShort );
		pStart += strlen( zDescShort );
	}
	else
	{
		pStart = strstr( &psPrinter->zPrinterID[2], zDescLong );
		if( pStart != NULL )
		{
			kerndbg( KERN_DEBUG_LOW, "Found \"%s\" in IEEE string\n", zDescLong );
			pStart += strlen( zDescLong );
		}
	}

	if( pStart != NULL )
	{
		pEnd = strchr( pStart, ';' );
		if( pEnd != NULL )
		{
			nDescLen = pEnd - pStart;
			if (nDescLen < 1)
				nDescLen = 1;
			else if(nDescLen > USB_PRINTER_NAME_LEN )
				nDescLen = USB_PRINTER_NAME_LEN - 1;
			strncpy( psPrinter->zName, pStart, nDescLen );
			psPrinter->zName[nDescLen]='\0';		

			kerndbg( KERN_DEBUG, "Printer description is \"%s\" (%d)\n", psPrinter->zName, nDescLen );
		}
	}

	return nLength;
}

static int usbprinter_set_protocol( struct usb_printer *psPrinter, int nProtocol )
{
	int nError, nAlt;

	if( nProtocol < USB_PRINTER_FIRST_PROTOCOL || nProtocol > USB_PRINTER_LAST_PROTOCOL)
		return -EINVAL;

	nAlt = psPrinter->sProtocol[nProtocol].nAltSetting;
	if( nAlt < 0 )
		return -EINVAL;

	nError = g_psBus->set_interface( psPrinter->psDev, psPrinter->nInterface, nAlt );
	if( nError != EOK )
	{
		kerndbg( KERN_WARNING, "Failed to set desired protocol %d on interface %d, error was %d", nAlt, psPrinter->nInterface, nError );
		return nError;
	}

	/* Rebuild packets */
	int nWritePipe = usb_sndbulkpipe( psPrinter->psDev, psPrinter->sProtocol[nProtocol].psWriteEndpoint->nEndpointAddress );
	USB_FILL_BULK(&psPrinter->sWrite, psPrinter->psDev, nWritePipe, psPrinter->pWriteBuffer, USB_PRINTER_BUF_SIZE,
		usbprinter_write_irq, psPrinter);
	psPrinter->nWritePipe = nWritePipe;

	if( psPrinter->bBidir )
	{
		int nReadPipe = usb_rcvbulkpipe( psPrinter->psDev, psPrinter->sProtocol[nProtocol].psReadEndpoint->nEndpointAddress );
		USB_FILL_BULK(&psPrinter->sRead, psPrinter->psDev, nReadPipe, psPrinter->pReadBuffer, USB_PRINTER_BUF_SIZE,
			usbprinter_read_irq, psPrinter);
		psPrinter->nReadPipe = nReadPipe;
	}

	psPrinter->nCurrentProtocol = nProtocol;
	kerndbg( KERN_DEBUG, "Set protocol to %d\n", nProtocol);
	return 0;
}

bool usbprinter_add( USB_device_s* psDevice, unsigned int nIfnum, void **pPrivate )
{
	USB_interface_s *psIface;
	USB_desc_interface_s *psInterface;
	USB_desc_endpoint_s *psEndpoint, *psReadEndpoint, *psWriteEndpoint;
	struct usb_printer *psPrinter;
	int nReadPipe, nWritePipe, nMaxRead, nMaxWrite;
	char *pBuf;
	bool bFound = false;
	int i, j, k, nEndpoint;

	if( !( psPrinter = kmalloc( sizeof(struct usb_printer), MEMF_KERNEL|MEMF_NOBLOCK ) ) )
		return( false );
	memset( psPrinter, 0, sizeof( struct usb_printer ) );

	if( !( psPrinter->zPrinterID = kmalloc( USB_PRINTER_DEVICE_ID_LEN, MEMF_KERNEL|MEMF_NOBLOCK ) ) )
		goto out;
	if( !( psPrinter->pStatusBuffer = kmalloc( USB_PRINTER_STATUS_BUF_SIZE, MEMF_KERNEL|MEMF_NOBLOCK ) ) )
		goto out;

	psPrinter->psDev = psDevice;
	psPrinter->nInterface = nIfnum;
	psPrinter->bBidir = true;
	psPrinter->nQuirks = 0;

	psIface = psDevice->psActConfig->psInterface + nIfnum;

	for (j = 0; j < USB_PRINTER_MAX_PROTOCOLS; j++)
		psPrinter->sProtocol[j].nAltSetting = -1;

	for( i = 0; i < psIface->nNumSetting; i++ )
	{
		psIface->nActSetting = i;
		psInterface = &psIface->psSetting[psIface->nActSetting];
		
		if( psInterface->nInterfaceClass != USB_CLASS_PRINTER || psInterface->nInterfaceSubClass != 1
	   || ( psInterface->nInterfaceProtocol < USB_PRINTER_FIRST_PROTOCOL || psInterface->nInterfaceProtocol > USB_PRINTER_LAST_PROTOCOL ) )
			continue;

		/* Try to find bulk OUT & IN endpoints */
		psReadEndpoint = psWriteEndpoint = NULL;
		for( nEndpoint = 0; nEndpoint < psInterface->nNumEndpoints; nEndpoint++ )
		{
			psEndpoint = psInterface->psEndpoint + nEndpoint;
			if( ( psEndpoint->nMAttributes & USB_ENDPOINT_XFERTYPE_MASK ) != USB_ENDPOINT_XFER_BULK )
				continue;

			if( !( psEndpoint->nEndpointAddress & USB_ENDPOINT_DIR_MASK ) )
			{
				if( psWriteEndpoint == NULL )
					psWriteEndpoint = psEndpoint;
			}
			else
			{
				if( psReadEndpoint == NULL )
					psReadEndpoint = psEndpoint;
			}
		}

		/* Ignore buggy hardware without the right endpoints. */
		if( psWriteEndpoint == NULL || ( psInterface->nInterfaceProtocol > 1 && psReadEndpoint == NULL ) )
			continue;

		/* Check quirks list for buggy bi-directional devices */
		uint16 nVendorID, nDeviceID;
		nVendorID = psPrinter->psDev->sDeviceDesc.nVendorID;
		nDeviceID = psPrinter->psDev->sDeviceDesc.nDeviceID;

		for( k = 0; quirk_printers[k].nVendorID; k++ )
		{
			if( nVendorID == quirk_printers[k].nVendorID && nDeviceID == quirk_printers[k].nDeviceID )
			{
				psPrinter->nQuirks = quirk_printers[k].nQuirks;
				break;
			}
	 	}

		/* Turn off reads for 7/1/1 (unidirectional) interfaces and buggy bidirectional printers. */
		if( psInterface->nInterfaceProtocol == 1 || ( psPrinter->nQuirks & USB_PRINTER_QUIRK_BIDIR ) )
		{
			kerndbg( KERN_INFO, "Bi-directional transfers are not available for this printer.\n" );
			psReadEndpoint = NULL;
		}

		psPrinter->sProtocol[psInterface->nInterfaceProtocol].nAltSetting = psInterface->nAlternateSettings;
		psPrinter->sProtocol[psInterface->nInterfaceProtocol].psWriteEndpoint = psWriteEndpoint;
		psPrinter->sProtocol[psInterface->nInterfaceProtocol].psReadEndpoint = psReadEndpoint;

		/* We have at least one interface */
		bFound = true;
	}
	if( !bFound )
		goto out;

	/* Select our preferred interface.  We prefer 7/1/2, then 7/1/3, then the unidirectional 7/1/1 */
	int nProtocol = -1;

	if( psPrinter->sProtocol[2].nAltSetting != -1 )
		nProtocol = 2;
	else if( psPrinter->sProtocol[1].nAltSetting != -1 )
		nProtocol = 1;
	else if(  psPrinter->sProtocol[3].nAltSetting != -1 )
		nProtocol = 3;

	/* Check that we have a usable interface */
	if( nProtocol == -1 )
		goto out;

	if( psPrinter->sProtocol[nProtocol].psReadEndpoint == NULL )
		psPrinter->bBidir = false;

	/* Prepare read/write buffers */
	psPrinter->sRead.psDevice = psPrinter->sWrite.psDevice = psPrinter->psDev;
	if( !( psPrinter->pWriteBuffer = kmalloc( USB_PRINTER_BUF_SIZE, MEMF_KERNEL|MEMF_NOBLOCK ) ))
		goto out;

	psPrinter->hWriteLock = create_semaphore( "usbprinter_write_lock", 0, 0 );

	if( psPrinter->bBidir )
	{
		if( !( psPrinter->pReadBuffer = kmalloc( USB_PRINTER_BUF_SIZE, MEMF_KERNEL|MEMF_NOBLOCK ) ))
			goto out;

		psPrinter->hReadLock = create_semaphore( "usbprinter_read_lock", 0, 0 );
	}

	psPrinter->hDeviceLock = create_semaphore( "usbprinter_lock", 1, 0 );

	USB_FILL_BULK(&psPrinter->sWrite, psDevice, nWritePipe, psPrinter->pWriteBuffer, USB_PRINTER_BUF_SIZE,
		usbprinter_write_irq, psPrinter);

	if( psPrinter->bBidir )
		USB_FILL_BULK(&psPrinter->sRead, psDevice, nReadPipe, psPrinter->pReadBuffer, USB_PRINTER_BUF_SIZE,
			usbprinter_read_irq, psPrinter);

	usbprinter_set_protocol( psPrinter, nProtocol );

	/* Get the printer ID string */
	usbprinter_get_device_id( psPrinter );

	if( psPrinter->zName[0] == '\0' )
	{
		if (!(pBuf = kmalloc(63, MEMF_KERNEL|MEMF_NOBLOCK)))
			goto out;

		if (psDevice->sDeviceDesc.nManufacturer &&
			g_psBus->string(psDevice, psDevice->sDeviceDesc.nManufacturer, pBuf, 63) > 0)
				strcat(psPrinter->zName, pBuf);
		if (psDevice->sDeviceDesc.nProduct &&
			g_psBus->string(psDevice, psDevice->sDeviceDesc.nProduct, pBuf, 63) > 0)
				sprintf(psPrinter->zName, "%s %s", psPrinter->zName, pBuf);

		if (!strlen(psPrinter->zName))
			sprintf(psPrinter->zName, "USB Printer %04x:%04x",
				psPrinter->psDev->sDeviceDesc.nVendorID, psPrinter->psDev->sDeviceDesc.nDeviceID);
		kfree(pBuf);
	}

	/* XXXKV: Need a new DEVICE_ type for printers */
	kerndbg( KERN_DEBUG_LOW, "claiming device\n" );

	claim_device( g_nDeviceID, psDevice->nHandle, psPrinter->zName, DEVICE_UNKNOWN );
	printk("%s on usb%d:%d.%d\n", psPrinter->zName, psDevice->psBus->nBusNum, psDevice->nDeviceNum, nIfnum );

	psPrinter->bConnected = true;
	psPrinter->bOpen = false;

	/* XXXKV: Handle multiple devices */
	*pPrivate = psPrinter;
	psPrinter->hNode = create_device_node( g_nDeviceID, -1, "printer/usb/0", &g_sOperations, psPrinter );

	return( true );

out:
	if( psPrinter )
	{
		kfree( psPrinter->pReadBuffer );
		kfree( psPrinter->pWriteBuffer );
		kfree( psPrinter->pStatusBuffer );
		kfree( psPrinter->zPrinterID );
		kfree( psPrinter );
	}
	return( false );
}

void usbprinter_cancel( struct usb_printer *psPrinter )
{
	g_psBus->cancel_packet( &psPrinter->sWrite );
	if( psPrinter->bBidir )
		g_psBus->cancel_packet( &psPrinter->sRead );
}

void usbprinter_cleanup( struct usb_printer *psPrinter )
{
	kassertw( psPrinter->bOpen == false );

	delete_device_node( psPrinter->hNode );

	kfree(psPrinter->pWriteBuffer);
	delete_semaphore( psPrinter->hWriteLock );

	if( psPrinter->bBidir )
	{
		kfree(psPrinter->pReadBuffer);
		delete_semaphore( psPrinter->hReadLock );
	}

	delete_semaphore( psPrinter->hDeviceLock );
	kfree( psPrinter->pStatusBuffer );
	kfree( psPrinter->zPrinterID );
	kfree( psPrinter );
}

void usbprinter_remove( USB_device_s* psDevice, void* pPrivate )
{
	struct usb_printer *psPrinter = pPrivate;

	LOCK( psPrinter->hDeviceLock );

	/* Dave's not here man */
	psPrinter->bConnected = false;

	release_device( psDevice->nHandle );
	usbprinter_cancel( psPrinter );

	/* Wakeup any read or write sleepers so they exit early */
	wakeup_sem( psPrinter->hWriteLock, false );

	if( psPrinter->bBidir )
		wakeup_sem( psPrinter->hReadLock, false );

	UNLOCK( psPrinter->hDeviceLock );

	/* Don't destroy anything until the device is closed */
	if( psPrinter->bOpen == false )
		usbprinter_cleanup( psPrinter );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
	/* Get USB bus */
	g_psBus = get_busmanager( USB_BUS_NAME, USB_BUS_VERSION );
	if( g_psBus == NULL ) {
		return( -1 );
	}
	
	g_nDeviceID = nDeviceID;
	
	/* Register */
	g_pcDriver = ( USB_driver_s* )kmalloc( sizeof( USB_driver_s ), MEMF_KERNEL | MEMF_NOBLOCK );
	
	strcpy( g_pcDriver->zName, "USB printer" );
	g_pcDriver->AddDevice = usbprinter_add;
	g_pcDriver->RemoveDevice = usbprinter_remove;
	
	g_psBus->add_driver( g_pcDriver );
	
    return( 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
	if( g_psBus )
		g_psBus->remove_driver( g_pcDriver );
    return( 0 );
}

