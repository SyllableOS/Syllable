/*
 *  The Syllable kernel
 *  USB mouse driver
 *  Copyright (C) 2003 Arno Klenke
 *	Based on linux usbmouse driver :
 *  Copyright (c) 1999-2000 Vojtech Pavlik
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


#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/smp.h>
#include <atheos/usb.h>


struct usb_mouse {
	signed char anData[8];
	char zName[128];
	USB_device_s *psDev;
	USB_packet_s sIRQ;
	int nOpen;
	int nMaxpacket;
	
};

sem_id hBufLock;
char anReadBuffer[4];
USB_bus_s* g_psBus = NULL;
int g_nDeviceID;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbmouse_open( void* pNode, uint32 nFlags, void **pCookie )
{
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbmouse_close( void* pNode, void* pCookie )
{
    
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t usbmouse_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int usbmouse_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
	if( nSize != 4 )
		return( -1 );
	sleep_on_sem( hBufLock, INFINITE_TIMEOUT );
	memcpy_to_user( pBuffer, &anReadBuffer, 4 );
	memset( &anReadBuffer[1], 0, 3 );
	return( 4 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int usbmouse_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    return( -1 );
}

DeviceOperations_s g_sOperations = {
    usbmouse_open,
    usbmouse_close,
    usbmouse_ioctl,
    usbmouse_read,
    usbmouse_write
};


void usb_mouse_irq(USB_packet_s *psPacket)
{
	struct usb_mouse *psMouse = psPacket->pCompleteData;
	signed char *pData = psMouse->anData;
	int nOffset = 0;
	
	if ( psPacket->nStatus ) return;
	
	/* Hack for Microsoft Wireless Explorer mouse */
	if( psMouse->nMaxpacket == 5 )
		nOffset = 1;
	
	anReadBuffer[0] = pData[nOffset];
	anReadBuffer[1] = pData[nOffset+1];
	anReadBuffer[2] = pData[nOffset+2];
	anReadBuffer[3] = pData[nOffset+3];
	
	wakeup_sem( hBufLock, false );
}

bool usbmouse_add( USB_device_s* psDevice, unsigned int nIfnum,
		       void **pPrivate )
{
	USB_interface_s *psIface;
	USB_desc_interface_s *psInterface;
	USB_desc_endpoint_s *psEndpoint;
	struct usb_mouse *psMouse;
	int nPipe, nMaxp;
	char *pBuf;
	bool bFound = false;
	int i;
	
	psIface = psDevice->psActConfig->psInterface + nIfnum;
	
	for( i = 0; i < psIface->nNumSetting; i++ ) {
		psIface->nActSetting = i;
		psInterface = &psIface->psSetting[psIface->nActSetting];
		
		if( psInterface->nInterfaceClass == USB_CLASS_HID && psInterface->nInterfaceSubClass == 1
			&& psInterface->nInterfaceProtocol == 2 ) {
			bFound = true;
			break;
		}
	}
	if( !bFound )
		return( false );

	nIfnum = i;
	psIface = &psDevice->psActConfig->psInterface[nIfnum];
	psInterface = &psIface->psSetting[psIface->nActSetting];

	if( psInterface->nNumEndpoints != 1 ) return( false );

	psEndpoint = psInterface->psEndpoint + 0;
	if( !( psEndpoint->nEndpointAddress & 0x80 ) ) return( false );
	if( ( psEndpoint->nMAttributes & 3) != 3 ) return( false );

	nPipe = usb_rcvintpipe( psDevice, psEndpoint->nEndpointAddress );
	nMaxp = usb_maxpacket( psDevice, nPipe, usb_pipeout( nPipe ) );
	
	g_psBus->set_idle( psDevice, psInterface->nInterfaceNumber, 0, 0 );

	if( !( psMouse = kmalloc( sizeof(struct usb_mouse), MEMF_KERNEL|MEMF_NOBLOCK ) ) ) return( false );
	memset( psMouse, 0, sizeof( struct usb_mouse ) );

	psMouse->psDev = psDevice;
	psMouse->nMaxpacket = nMaxp;

	if (!(pBuf = kmalloc(63, MEMF_KERNEL|MEMF_NOBLOCK))) {
		kfree(psMouse);
		return( false );
	}

	if (psDevice->sDeviceDesc.nManufacturer &&
		g_psBus->string(psDevice, psDevice->sDeviceDesc.nManufacturer, pBuf, 63) > 0)
			strcat(psMouse->zName, pBuf);
	if (psDevice->sDeviceDesc.nProduct &&
		g_psBus->string(psDevice, psDevice->sDeviceDesc.nProduct, pBuf, 63) > 0)
			sprintf(psMouse->zName, "%s %s", psMouse->zName, pBuf);

	if (!strlen(psMouse->zName))
		sprintf(psMouse->zName, "USB HIDBP Mouse %04x:%04x",
			psMouse->psDev->sDeviceDesc.nVendorID, psMouse->psDev->sDeviceDesc.nDeviceID);
	kfree(pBuf);

	USB_FILL_INT(&psMouse->sIRQ, psDevice, nPipe, psMouse->anData, nMaxp > 8 ? 8 : nMaxp,
		usb_mouse_irq, psMouse, psEndpoint->nInterval);

	claim_device( g_nDeviceID, psDevice->nHandle, psMouse->zName, DEVICE_INPUT );
	printk("%s on usb%d:%d.%d\n",
		 psMouse->zName, psDevice->psBus->nBusNum, psDevice->nDeviceNum, nIfnum );
		 
	psMouse->sIRQ.psDevice = psMouse->psDev;
	if( g_psBus->submit_packet( &psMouse->sIRQ ) )
		return -EIO;

	*pPrivate = psMouse;
	return( true );
}


void usbmouse_remove( USB_device_s* psDevice, void* pPrivate )
{
	
	struct usb_mouse *psMouse = pPrivate;
	release_device( psDevice->nHandle );
	g_psBus->cancel_packet( &psMouse->sIRQ );
	kfree( psMouse );
}

USB_driver_s* g_pcDriver;

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
	
	strcpy( g_pcDriver->zName, "USB mouse" );
	g_pcDriver->AddDevice = usbmouse_add;
	g_pcDriver->RemoveDevice = usbmouse_remove;
	
	g_psBus->add_driver_resistant( g_pcDriver );
	
	
	hBufLock = create_semaphore( "usbmouse_buf_lock", 0, 0 );
	memset( anReadBuffer, 0, 4 );
	
	create_device_node( nDeviceID, -1, "input/usb_mouse", &g_sOperations, NULL );
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
	delete_semaphore( hBufLock );
    return( 0 );
}


























