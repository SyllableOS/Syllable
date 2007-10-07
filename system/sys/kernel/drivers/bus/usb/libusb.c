/*
 *  The Syllable kernel
 *  USB user-space (libusb) driver
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
#include <atheos/libusb.h>

#include <posix/errno.h>

/* XXXKV: Enable debugging */
#if 0
 #undef DEBUG_LIMIT
 #define DEBUG_LIMIT KERN_DEBUG_LOW
#endif

USB_packet_s* usb_alloc_packet( int nISOPackets );
status_t usb_submit_packet( USB_packet_s* psPacket );
int usb_control_msg( USB_device_s* psDevice, unsigned int nPipe, uint8 nRequest, 
					uint8 nRequestType, uint16 nValue, uint16 nIndex, void* pData, uint16 nSize,
					bigtime_t nTimeOut );
int usb_set_configuration( USB_device_s* psDevice, int nConfig );
int usb_set_interface( USB_device_s* psDevice, int nIFNum, int nAlternate );
int usb_clear_halt( USB_device_s* psDevice, int nPipe );

extern int g_nDeviceID;

status_t libusb_dev_open( void* pNode, uint32 nFlags, void **pCookie )
{
	USB_libusb_node_s *psUSBNode = (USB_libusb_node_s*)pNode;
	if( TRY_LOCK( psUSBNode->hOpen ) != EOK )
		return EAGAIN;

	reset_semaphore( psUSBNode->hWait, 0 );
	return EOK;
}

status_t libusb_dev_close( void* pNode, void* pCookie )
{
	USB_libusb_node_s *psUSBNode = (USB_libusb_node_s*)pNode;
	UNLOCK( psUSBNode->hOpen );
	return EOK;
}

/* URB completion callback */
static void do_io_complete( USB_packet_s *psPacket )
{
	USB_libusb_node_s *psUSBNode = (USB_libusb_node_s*)psPacket->pCompleteData;
	UNLOCK( psUSBNode->hWait );
}

static status_t do_io_request( USB_libusb_node_s *psUSBNode, struct usb_io_request *psReq, int nCmd )
{
	USB_packet_s *psPacket;
	size_t nSize;
	void *pData;
	int nPipe;
	status_t nError = EOK;

	nSize = min( psReq->nSize, 65535 );	/* Max. reqeuest size for USB 2.0 */
	pData = kmalloc( nSize, MEMF_KERNEL|MEMF_OKTOFAIL );
	if( NULL == pData )
	{
		kerndbg( KERN_FATAL, "libusb: Failed to allocate I/O buffer\n" );
		nError = ENOMEM;
		goto err;
	}
	if( ( nCmd == IOCNR_BULK_WRITE ) || ( nCmd == IOCNR_INT_WRITE ) )
		memcpy_from_user( pData, psReq->pData, nSize );

	psPacket = usb_alloc_packet( 0 );
	if( NULL == psPacket )
	{
		kerndbg( KERN_FATAL, "libusb: Failed to allocate URB\n" );
		nError = ENOMEM;
		goto err_free;
	}

	if( ( nCmd == IOCNR_BULK_READ ) || ( nCmd == IOCNR_BULK_WRITE ) )
	{
		if( nCmd == IOCNR_BULK_READ )
			nPipe = usb_rcvbulkpipe( psUSBNode->psDevice, psReq->nEndpoint );
		else
			nPipe = usb_sndbulkpipe( psUSBNode->psDevice, psReq->nEndpoint );
		USB_FILL_BULK( psPacket, psUSBNode->psDevice, nPipe, pData, nSize, do_io_complete, psUSBNode );
	}
	else
	{
		/* XXXKV: I have no idea if this is correct */
		uint8 nIFNum = psUSBNode->nInterfaceNum;
		int nEPNum = psReq->nEndpoint & USB_PIPE_DEVEP_MASK;
		uint8 nInterval = psUSBNode->psDevice->psActConfig->psInterface[nIFNum].psSetting[0].psEndpoint[nEPNum].nInterval;

		if( nCmd == IOCNR_INT_READ )
			nPipe = usb_rcvintpipe( psUSBNode->psDevice, psReq->nEndpoint );
		else
			nPipe = usb_sndintpipe( psUSBNode->psDevice, psReq->nEndpoint );
		USB_FILL_INT( psPacket, psUSBNode->psDevice, nPipe, pData, nSize, do_io_complete, psUSBNode, nInterval );
	}

	/* Submit the URB and wait for completion */
	nError = usb_submit_packet( psPacket );
	kerndbg( KERN_DEBUG, "do_io_request(): usb_submit_packet() returned %d\n", nError );
	if( nError == EOK )
		nError = lock_semaphore( psUSBNode->hWait, SEM_NOSIG, psReq->nTimeout );

	kerndbg( KERN_DEBUG_LOW, "psUSBNode->hWait unlocked with nError=%d\n", nError );

	if( ( nError == EOK) && ( ( nCmd == IOCNR_BULK_READ ) || ( nCmd == IOCNR_INT_READ ) ) )
		memcpy_to_user( psReq->pData, pData, nSize );

err_free:
	kfree( pData );
err:
	return nError;
}

status_t libusb_dev_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	status_t nError = EOK;
	USB_libusb_node_s *psUSBNode = (USB_libusb_node_s*)pNode;

	if( bFromKernel )
	{
		kerndbg( KERN_WARNING, "Someone attempted to use the USB uspace driver from the kernel!\n" );
		return EINVAL;
	}

	LOCK( psUSBNode->hLock );

	switch( IOCTL_CMD( nCommand ) )
	{
		case IOCNR_GET_DEVICE_DESC:
		{
			kerndbg( KERN_DEBUG_LOW, "Got command IOCNR_GET_DEVICE_DESC\n" );

			size_t nSize = min( IOCTL_SIZE( nCommand ), sizeof( USB_desc_device_s ) );
			memcpy_to_user( pArgs, &psUSBNode->psDevice->sDeviceDesc, nSize );
			break;
		}

		case IOCNR_DO_REQUEST:
		{
			kerndbg( KERN_DEBUG_LOW, "Got command IOCNR_DO_REQUEST\n" );

			struct usb_ctl_request sReq;
			void *pData = NULL;
			size_t nDataSize = 0;
			int nPipe;

			size_t nSize = sizeof( struct usb_ctl_request );
			memcpy_from_user( &sReq, pArgs, nSize );

			nDataSize = sReq.nSize;
			if( nDataSize > 0 && sReq.pData )
			{
				pData = kmalloc( sReq.nSize, MEMF_KERNEL );
				memcpy_from_user( pData, sReq.pData, nDataSize );
			}

			if( sReq.nRequestType & USB_DIR_IN )
				nPipe = usb_rcvctrlpipe( psUSBNode->psDevice, 0 );
			else
				nPipe = usb_sndctrlpipe( psUSBNode->psDevice, 0 );

			nError = usb_control_msg( psUSBNode->psDevice, nPipe,
											   sReq.nRequest, sReq.nRequestType,
											   sReq.nValue, sReq.nIndex,
											   pData, nDataSize,
								  			   sReq.nTimeout );

			kerndbg( KERN_DEBUG_LOW, "usb_control_msg() returned %d\n", nError );
			if( nError != EOK )
				sReq.nSize = nError;

			memcpy_to_user( pArgs, &sReq, nSize );
			if( nDataSize > 0 && pData && sReq.pData )
			{
				memcpy_to_user( sReq.pData, pData, nDataSize );
				kfree( pData );
			}

			break;
		}

		case IOCNR_SET_CONFIGURATION:
		{
			kerndbg( KERN_DEBUG_LOW, "Got command IOCNR_SET_CONFIGURATION\n" );

			int nConfiguration;
			memcpy_from_user( &nConfiguration, pArgs, sizeof( nConfiguration ) );

			kerndbg( KERN_DEBUG_LOW, "nConfiguration=%d\n", nConfiguration );

			nError = usb_set_configuration( psUSBNode->psDevice, nConfiguration );
			break;
		}

		case IOCNR_SET_ALTINTERFACE:
		{
			kerndbg( KERN_DEBUG_LOW, "Got command IOCNR_SET_ALTINTERFACE\n" );

			struct usb_alt_interface sAlt;
			size_t nSize = sizeof( struct usb_alt_interface );
			memcpy_from_user( &sAlt, pArgs, nSize );

			nError = usb_set_interface( psUSBNode->psDevice, sAlt.nIFNum, sAlt.nAlternate );
			break;
		}

		case IOCNR_BULK_READ:
		case IOCNR_BULK_WRITE:
		case IOCNR_INT_READ:
		case IOCNR_INT_WRITE:
		{
			kerndbg( KERN_DEBUG_LOW, "Got I/O command %d\n", IOCTL_CMD( nCommand ) );

			struct usb_io_request sReq;
			memcpy_from_user( &sReq, pArgs, sizeof( struct usb_io_request ) );

			kerndbg( KERN_DEBUG_LOW, "I/O cmd req: nEndpoint=%d (0x%.2x), pData=%p, nSize=%d, nTimeout=%ld\n",
						sReq.nEndpoint,
						sReq.nEndpoint,
						sReq.pData,
						sReq.nSize,
						sReq.nTimeout );

			nError = do_io_request( psUSBNode, &sReq, IOCTL_CMD( nCommand ) );
			if( nError < 0 )
				kerndbg( KERN_DEBUG, "%d: do_io_request() failed with error %d\n", IOCTL_CMD( nCommand ), nError );

			break;
		}

		case IOCNR_CLEAR_HALT:
		{
			kerndbg( KERN_DEBUG_LOW, "Got command IOCNR_CLEAR_HALT\n" );

			int nEndpoint, nPipe;
			memcpy_from_user( &nEndpoint, pArgs, sizeof( nEndpoint ) );

			kerndbg( KERN_DEBUG, "nEndpoint=%d (0x%.2x)\n", nEndpoint, nEndpoint );

			if( nEndpoint & USB_DIR_IN )
				nPipe = usb_rcvctrlpipe( psUSBNode->psDevice, 0 );
			else
				nPipe = usb_sndctrlpipe( psUSBNode->psDevice, 0 );

			nError = usb_clear_halt( psUSBNode->psDevice, nPipe );
			break;
		}

		default:
		{
			kerndbg( KERN_FATAL, "libusb: Unknown command %d\n", IOCTL_CMD( nCommand ) );
			nError = ENOSYS;
			break;
		}
	}

	UNLOCK( psUSBNode->hLock );

	return nError;
}

/* Note that all I/O is done via. ioctl() */
DeviceOperations_s g_sOperations = {
    libusb_dev_open,
    libusb_dev_close,
    libusb_dev_ioctl
};

static USB_libusb_node_s * alloc_usb_node( USB_device_s* psDevice, unsigned int nIfnum )
{
	USB_libusb_node_s *psUSBNode;
	char zNode[PATH_MAX+1];	

	psUSBNode = kmalloc( sizeof(USB_libusb_node_s), MEMF_KERNEL|MEMF_NOBLOCK|MEMF_CLEAR );

	if( NULL == psUSBNode )
	{
		kerndbg( KERN_FATAL, "libusb: Failed to allocate new USB_libusb_node\n" );
	}
	else
	{
		psUSBNode->psDevice = psDevice;
		psUSBNode->nInterfaceNum = nIfnum;

		psUSBNode->hLock = create_semaphore( "libusb_node_lock", 1, 0 );
		psUSBNode->hWait = create_semaphore( "libusb_node_wait", 0, 0 );
		psUSBNode->hOpen = create_semaphore( "libusb_node_open", 1, 0 );
	}
	
	sprintf( zNode, "usb/%d/%d-%i", psDevice->psBus->nBusNum, psDevice->nDeviceNum, nIfnum );
	psUSBNode->hNode = create_device_node( g_nDeviceID, -1, zNode, &g_sOperations, psUSBNode );	

	return psUSBNode;
}

/* Create node for the device */
void libusb_add( USB_device_s* psDevice )
{
	USB_interface_s *psInterface;
	USB_desc_interface_s *psDescInterface;
	USB_desc_endpoint_s *psDescEndpoint;
	USB_libusb_node_s *psUSBNode;
	int i, nEndpoint;
	
	for( i=0; i < psDevice->psActConfig->nNumInterfaces; i++ )
	{
		psInterface = psDevice->psActConfig->psInterface + i;

		/* Create a device node */
		psUSBNode = alloc_usb_node( psDevice, i );
		if( NULL == psUSBNode )
			return;

		psInterface->psUSBNode = psUSBNode;
	}
}

/* Remove node for the device */
void libusb_remove( USB_device_s* psDevice )
{
	int i;
	for( i = 0; i < psDevice->psActConfig->nNumInterfaces; i++ )
	{
		USB_interface_s* psInterface = psDevice->psActConfig->psInterface + i;
	
		if( psInterface->psUSBNode == NULL )
			continue;
			
		delete_device_node( psInterface->psUSBNode->hNode );
		delete_semaphore( psInterface->psUSBNode->hLock );
		delete_semaphore( psInterface->psUSBNode->hWait );
		delete_semaphore( psInterface->psUSBNode->hOpen );
		kfree( psInterface->psUSBNode );
	}
}


