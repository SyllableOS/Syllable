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
 

#include <atheos/usb.h>
#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/time.h>
#include <atheos/device.h>
#include <posix/errno.h>
#include <macros.h>

//#undef DEBUG_LIMIT
//#define DEBUG_LIMIT KERN_DEBUG

/* Functions in usb.c */
int usb_control_msg( USB_device_s* psDevice, unsigned int nPipe, uint8 nRequest, 
					uint8 nRequestType, uint16 nValue, uint16 nIndex, void* pData, uint16 nSize,
					bigtime_t nTimeOut );
USB_packet_s* usb_alloc_packet( int nISOPackets );
void usb_free_packet( USB_packet_s* psPacket );
status_t usb_submit_packet( USB_packet_s* psPacket );
status_t usb_cancel_packet( USB_packet_s* psPacket );
void usb_connect( USB_device_s* psDevice );
void usb_disconnect( USB_device_s** psDevice );
int usb_new_device( USB_device_s* psDevice );
USB_device_s* usb_alloc_device( USB_device_s* psParent, USB_bus_driver_s* psBus );
void usb_free_device( USB_device_s* psDevice );
void usb_register_driver_force( USB_driver_s* psDriver );

/*
 * Hub request types
 */

#define USB_RT_HUB	(USB_TYPE_CLASS | USB_RECIP_DEVICE)
#define USB_RT_PORT	(USB_TYPE_CLASS | USB_RECIP_OTHER)

/*
 * Hub Class feature numbers
 * See USB 2.0 spec Table 11-17
 */
#define C_HUB_LOCAL_POWER	0
#define C_HUB_OVER_CURRENT	1

/*
 * Port feature numbers
 * See USB 2.0 spec Table 11-17
 */
#define USB_PORT_FEAT_CONNECTION	0
#define USB_PORT_FEAT_ENABLE		1
#define USB_PORT_FEAT_SUSPEND		2
#define USB_PORT_FEAT_OVER_CURRENT	3
#define USB_PORT_FEAT_RESET		4
#define USB_PORT_FEAT_POWER		8
#define USB_PORT_FEAT_LOWSPEED		9
#define USB_PORT_FEAT_HIGHSPEED		10
#define USB_PORT_FEAT_C_CONNECTION	16
#define USB_PORT_FEAT_C_ENABLE		17
#define USB_PORT_FEAT_C_SUSPEND		18
#define USB_PORT_FEAT_C_OVER_CURRENT	19
#define USB_PORT_FEAT_C_RESET		20
#define USB_PORT_FEAT_TEST              21
#define USB_PORT_FEAT_INDICATOR         22

/* 
 * Hub Status and Hub Change results
 * See USB 2.0 spec Table 11-19 and Table 11-20
 */
typedef struct {
	uint16 nPortStatus;
	uint16 nPortChange;	
} __attribute__ ((packed)) USB_port_status;

/* 
 * nPortStatus bit field
 * See USB 2.0 spec Table 11-21
 */
#define USB_PORT_STAT_CONNECTION	0x0001
#define USB_PORT_STAT_ENABLE		0x0002
#define USB_PORT_STAT_SUSPEND		0x0004
#define USB_PORT_STAT_OVERCURRENT	0x0008
#define USB_PORT_STAT_RESET		0x0010
/* bits 5 for 7 are reserved */
#define USB_PORT_STAT_POWER		0x0100
#define USB_PORT_STAT_LOW_SPEED		0x0200
#define USB_PORT_STAT_HIGH_SPEED        0x0400
#define USB_PORT_STAT_TEST              0x0800
#define USB_PORT_STAT_INDICATOR         0x1000
/* bits 13 to 15 are reserved */

/* 
 * nPortChange bit field
 * See USB 2.0 spec Table 11-22
 * Bits 0 to 4 shown, bits 5 to 15 are reserved
 */
#define USB_PORT_STAT_C_CONNECTION	0x0001
#define USB_PORT_STAT_C_ENABLE		0x0002
#define USB_PORT_STAT_C_SUSPEND		0x0004
#define USB_PORT_STAT_C_OVERCURRENT	0x0008
#define USB_PORT_STAT_C_RESET		0x0010

/*
 * nHubCharacteristics (masks) 
 * See USB 2.0 spec Table 11-13, offset 3
 */
#define HUB_CHAR_LPSM		0x0003 /* D1 .. D0 */
#define HUB_CHAR_COMPOUND	0x0004 /* D2       */
#define HUB_CHAR_OCPM		0x0018 /* D4 .. D3 */
#define HUB_CHAR_TTTT           0x0060 /* D6 .. D5 */
#define HUB_CHAR_PORTIND        0x0080 /* D7       */

typedef struct {
	uint16 nHubStatus;
	uint16 nHubChange;
} __attribute__ ((packed)) USB_hub_status;

/*
 * Hub Status & Hub Change bit masks
 * See USB 2.0 spec Table 11-19 and Table 11-20
 * Bits 0 and 1 for wHubStatus and wHubChange
 * Bits 2 to 15 are reserved for both
 */
#define HUB_STATUS_LOCAL_POWER	0x0001
#define HUB_STATUS_OVERCURRENT	0x0002
#define HUB_CHANGE_LOCAL_POWER	0x0001
#define HUB_CHANGE_OVERCURRENT	0x0002


/* 
 * Hub descriptor 
 * See USB 2.0 spec Table 11-13
 */
typedef struct {
	uint8  nDescLength;
	uint8  nDescriptorType;
	uint8  nNbrPorts;
	uint16 nHubCharacteristics;
	uint8  nPwrOn2PwrGood;
	uint8  nHubContrCurrent;
	    	/* add 1 bit for hub status change; round to bytes */
	uint8  bDeviceRemovable[(16 + 1 + 7) / 8];
	uint8  bPortPwrCtrlMask[(16 + 1 + 7) / 8];
} __attribute__ ((packed)) USB_desc_hub;

struct USB_hub_t
{
	struct USB_hub_t* psNext;
	sem_id hLock;
	bool bNeedsAttention;
	USB_device_s* psDevice;
	USB_desc_hub* psDesc;
	char nBuffer[(16 + 1 + 7) / 8]; /* add 1 bit for hub status change */
	USB_packet_s* psPacket;
	int nError;
	int nErrors;
	USB_device_s* psTT;
	int nTTMulti;
};

typedef struct USB_hub_t USB_hub;

static USB_hub* g_psFirstHub;
static sem_id g_hHubWait; 
static SpinLock_s g_hHubLock;
static sem_id g_hHubAddressLock;
static sem_id g_hHubThreadLock;

/* USB 2.0 spec Section 11.24.4.5 */
int usb_get_hub_descriptor( USB_device_s* psDevice, void *pData, int nSize )
{
	return( usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
		USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
		USB_DT_HUB << 8, 0, pData, nSize, 1000 * 1000 ) );
}

/*
 * USB 2.0 spec Section 11.24.2.1
 */
int usb_clear_hub_feature( USB_device_s* psDevice, int nFeature)
{
	return( usb_control_msg( psDevice, usb_sndctrlpipe( psDevice, 0 ),
		USB_REQ_CLEAR_FEATURE, USB_RT_HUB, nFeature, 0, NULL, 0, 1000 * 1000 ) );
}

/*
 * USB 2.0 spec Section 11.24.2.2
 * BUG: doesn't handle port indicator selector in high byte of wIndex
 */
int usb_clear_port_feature( USB_device_s* psDevice, int nPort, int nFeature)
{
	return( usb_control_msg( psDevice, usb_sndctrlpipe( psDevice, 0 ),
		USB_REQ_CLEAR_FEATURE, USB_RT_PORT, nFeature, nPort, NULL, 0, 1000 * 1000 ) );
}

/*
 * USB 2.0 spec Section 11.24.2.13
 * BUG: doesn't handle port indicator selector in high byte of wIndex
 */
int usb_set_port_feature( USB_device_s* psDevice, int nPort, int nFeature )
{
	return( usb_control_msg( psDevice, usb_sndctrlpipe( psDevice, 0 ),
		USB_REQ_SET_FEATURE, USB_RT_PORT, nFeature, nPort, NULL, 0, 1000 * 1000 ) );
}

/*
 * USB 2.0 spec Section 11.24.2.6
 */
int usb_get_hub_status( USB_device_s* psDevice, void *pData )
{
	return( usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
		USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB, 0, 0,
		pData, sizeof( USB_hub_status ), 1000 * 1000 ) );
}

/*
 * USB 2.0 spec Section 11.24.2.7
 */
int usb_get_port_status( USB_device_s* psDevice, int nPort, void *pData )
{
	return( usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
		USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT, 0, nPort,
		pData, sizeof( USB_hub_status ), 1000 * 1000 ) );
}

void usb_hub_irq( USB_packet_s* psPacket )
{
	USB_hub* psHub = ( USB_hub* )psPacket->pCompleteData;
	unsigned long nFlags;
	
	if( psPacket->nStatus ) {
		if( psPacket->nStatus == -ENOENT )
			return;
		kerndbg( KERN_WARNING, "USB: usb_hub_irq() error\n" );
		
		if( ( ++psHub->nErrors < 10 ) || psHub->nError )
			return;
			
		psHub->nError = psPacket->nStatus;
	}
	psHub->nErrors = 0;
	spinlock_cli( &g_hHubLock, nFlags );
	
	/* We need attention */
	psHub->bNeedsAttention = true;
	wakeup_sem( g_hHubWait, false );
	
	spinunlock_restore( &g_hHubLock, nFlags );
}


void usb_hub_power_on( USB_hub* psHub )
{
	int i;

	/* Enable power to the ports */
	kerndbg( KERN_DEBUG, "Enabling power on all ports\n" );
	for( i = 0; i < psHub->psDesc->nNbrPorts; i++ )
		usb_set_port_feature( psHub->psDevice, i + 1, USB_PORT_FEAT_POWER );

	/* Wait for power to be enabled */
	snooze( psHub->psDesc->nPwrOn2PwrGood * 2 * 1000 );
}

int usb_hub_configure( USB_hub* psHub, USB_desc_endpoint_s* psEndpoint )
{
	USB_device_s *psDevice = psHub->psDevice;
	USB_hub_status sStatus;
	char portstr[16 + 1];
	unsigned int nPipe;
	//unsigned long nFlags;
	int i, nMaxp, nRet;

	psHub->psDesc = kmalloc( sizeof( *psHub->psDesc ), MEMF_KERNEL | MEMF_NOBLOCK );
	if( !psHub->psDesc ) {
		kerndbg( KERN_WARNING, "USB: Unable to kmalloc %Zd bytes for hub descriptor\n", sizeof( *psHub->psDesc ) );
		return( -1 );
	}
	memset( psHub->psDesc, 0, sizeof( *psHub->psDesc ) );

	/* Request the entire hub descriptor. */
	nRet = usb_get_hub_descriptor( psDevice, psHub->psDesc, sizeof( *psHub->psDesc ) );
		/* <hub->descriptor> is large enough for a hub with 127 ports;
		 * the hub can/will return fewer bytes here. */
	if( nRet < 0 ) {
		kerndbg( KERN_WARNING, "USB: Unable to get hub descriptor (err = %d)\n", nRet );
		kfree( psHub->psDesc );
		return( -1 );
	}

	psDevice->nMaxChild = psHub->psDesc->nNbrPorts;

	kerndbg( KERN_INFO,  "%d port%s detected\n", psHub->psDesc->nNbrPorts, (psHub->psDesc->nNbrPorts == 1) ? "" : "s" );

#ifdef __ENABLE_DEBUG__
	if( KERN_DEBUG >= DEBUG_LIMIT )
	{
	if( psHub->psDesc->nHubCharacteristics & HUB_CHAR_COMPOUND )
		printk( "Part of a compound device\n" );
	else
		printk( "Standalone hub\n" );

	switch( psHub->psDesc->nHubCharacteristics & HUB_CHAR_LPSM ) {
		case 0x00:
			printk("Ganged power switching\n");
		break;
		case 0x01:
			printk("Individual port power switching\n");
		break;
		case 0x02:
		case 0x03:
			printk("Unknown reserved power switching mode\n");
		break;
	}

	switch( psHub->psDesc->nHubCharacteristics & HUB_CHAR_OCPM ) {
		case 0x00:
			printk("Global over-current protection\n");
		break;
		case 0x08:
			printk("Individual port over-current protection\n");
		break;
		case 0x10:
		case 0x18:
			printk("No over-current protection\n");
		break;
	}

	printk( "Port indicators are %s supported\n", 
	    (psHub->psDesc->nHubCharacteristics & HUB_CHAR_PORTIND) ? "" : "not" );

	printk( "Power on to power good time: %dms\n", psHub->psDesc->nPwrOn2PwrGood * 2);
	printk( "Hub controller current requirement: %dmA\n", psHub->psDesc->nHubContrCurrent );
	}
	#endif
	
	switch ( psDevice->sDeviceDesc.nDeviceProtocol ) {
		case 0:
			break;
		case 1:
			kerndbg( KERN_DEBUG, "Single TT\n" );
			psHub->psTT = psDevice;
			break;
		case 2:
			kerndbg( KERN_DEBUG, "TT per port\n" );
			psHub->psTT = psDevice;
			psHub->nTTMulti = 1;
			break;
		default:
			kerndbg( KERN_WARNING, "Unrecognized hub protocol %d\n",
				psDevice->sDeviceDesc.nDeviceProtocol );
			break;
	}
	
	for( i = 0; i < psDevice->nMaxChild; i++ )
		portstr[i] = psHub->psDesc->bDeviceRemovable[((i + 1) / 8)] & (1 << ((i + 1) % 8)) ? 'F' : 'R';
	portstr[psDevice->nMaxChild] = 0;

	kerndbg( KERN_DEBUG, "Port removable status: %s (HUB %x)\n", portstr, psHub );

	nRet = usb_get_hub_status( psDevice, &sStatus );
	if( nRet < 0) {
		kerndbg( KERN_WARNING, "USB: Unable to get hub status (err = %d)\n", nRet );
		kfree( psHub->psDesc );
		return( -1 );
	}

	
	/* Start the interrupt endpoint */
	nPipe = usb_rcvintpipe( psDevice, psEndpoint->nEndpointAddress );
	nMaxp = usb_maxpacket( psDevice, nPipe, usb_pipeout( nPipe ) );

	if( nMaxp > sizeof( psHub->nBuffer ) )
		nMaxp = sizeof( psHub->nBuffer );

	psHub->psPacket = usb_alloc_packet( 0 );
	if( !psHub->psPacket ) {
		kerndbg( KERN_WARNING, "USB: Out of memory\n" );
		kfree( psHub->psDesc );
		return( -1 );
	}

	USB_FILL_INT( psHub->psPacket, psDevice, nPipe, psHub->nBuffer, nMaxp, usb_hub_irq,
		psHub, ( psDevice->eSpeed == USB_SPEED_HIGH )
			? 1 << ( psEndpoint->nInterval - 1 )
			: psEndpoint->nInterval );
	nRet = usb_submit_packet( psHub->psPacket );
	if( nRet ) {
		kerndbg( KERN_WARNING, "USB: usb_submit_packet failed (%d)", nRet );
		kfree( psHub->psDesc );
		return( -1 );
	}
	
	wakeup_sem( g_hHubWait, false );
	
	usb_hub_power_on( psHub );

	return( 0 );
}

bool usb_hub_add( USB_device_s* psDevice, unsigned int nIF,
		       void **pPrivate )
{
	USB_desc_interface_s* psIFDesc = NULL;
	USB_desc_endpoint_s* psEndpoint;
	USB_hub* psHub;
	USB_interface_s* psIF;
	int i;
	bool bFound = false;
	uint32 nFlags;
	
	psIF = psDevice->psActConfig->psInterface + nIF;
	
	for( i = 0; i < psIF->nNumSetting; i++ ) {
		psIF->nActSetting = i;
		psIFDesc = &psIF->psSetting[psIF->nActSetting];
		
		if( psIFDesc->nInterfaceClass == USB_CLASS_HUB ) {
			bFound = true;
			break;
		}
	}
	
	if( !bFound ) {
		return( false );
	}
	
	psIFDesc = &psDevice->psActConfig->psInterface[i].psSetting[0];
	

	/* Some hubs have a subclass of 1, which AFAICT according to the */
	/*  specs is not defined, but it works */
	if( ( psIFDesc->nInterfaceSubClass != 0 ) &&
	    ( psIFDesc->nInterfaceSubClass != 1 ) ) {
		kerndbg( KERN_WARNING, "USB: Invalid subclass (%d) for USB hub device #%d",
			psIFDesc->nInterfaceSubClass, psDevice->nDeviceNum );
		return( false );
	}

	/* Multiple endpoints? What kind of mutant ninja-hub is this? */
	if( psIFDesc->nNumEndpoints != 1 ) {
		kerndbg( KERN_WARNING, "USB: Invalid bNumEndpoints (%d) for USB hub device #%d",
			psIFDesc->nInterfaceSubClass, psDevice->nDeviceNum );
		return( false );
	}

	psEndpoint = &psIFDesc->psEndpoint[0];

	/* Output endpoint? Curiousier and curiousier.. */
	if( !( psEndpoint->nEndpointAddress & USB_DIR_IN ) ) {
		kerndbg( KERN_WARNING, "USB: Device #%d is hub class, but has output endpoint?",
			psDevice->nDeviceNum );
		return( false );
	}

	/* If it's not an interrupt endpoint, we'd better punt! */
	if( ( psEndpoint->nMAttributes & USB_ENDPOINT_XFERTYPE_MASK ) != USB_ENDPOINT_XFER_INT ) {
		kerndbg( KERN_WARNING, "USB: Device #%d is hub class, but has endpoint other than interrupt?\n",
			psDevice->nDeviceNum );
		return( false );
	}


	psHub = kmalloc( sizeof( *psHub ), MEMF_KERNEL | MEMF_NOBLOCK );
	if( !psHub ) {
		kerndbg( KERN_WARNING, "USB: Couldn't kmalloc hub struct" );
		return( false );
	}

	memset( psHub, 0, sizeof( *psHub ) );
	psHub->psDevice = psDevice;
	psHub->hLock = create_semaphore( "usb_hub_thread_lock", 1, SEM_RECURSIVE );
	
	/* Now we know that we had success -> add ourself to the list */
	
	nFlags = spinlock_disable( &g_hHubLock );
	psHub->psNext = g_psFirstHub;
	g_psFirstHub = psHub;
	spinunlock_enable( &g_hHubLock, nFlags );

	if( usb_hub_configure( psHub, psEndpoint ) >= 0 ) {
		/* We found a hub -> claim the device */
		char zTemp[255];
		sprintf( zTemp, "%i ports USB hub", psHub->psDesc->nNbrPorts );
		claim_device( -1, psDevice->nHandle, zTemp, DEVICE_PORT );
		*pPrivate = psHub;
		return( true );
	}
		
	kerndbg( KERN_WARNING, "USB: Hub configuration failed for device #%d", psDevice->nDeviceNum );

	
	kfree( psHub );

	return( false );
}

void usb_hub_remove( USB_device_s* psDevice, void* pPrivate )
{
	USB_hub* psHub = ( USB_hub* )pPrivate;
	
	/* Delete hub */
	USB_hub* psPrevHub = NULL;
	USB_hub* psNextHub = g_psFirstHub;
	uint32 nFlags = spinlock_disable( &g_hHubLock );
	
	release_device( psDevice->nHandle );
	
	LOCK( psHub->hLock );
	UNLOCK( psHub->hLock );
	
	while( psNextHub != NULL )
	{
		if( psNextHub == psHub )
		{
			/* Remove it */
			if( psPrevHub )
				psPrevHub->psNext = psNextHub->psNext;
			else
				g_psFirstHub = psNextHub->psNext;
				
			if( psHub->psPacket ) {
				usb_cancel_packet( psHub->psPacket );
				usb_free_packet( psHub->psPacket );
				psHub->psPacket = NULL;
			}
			if( psHub->psDesc ) {
				kfree( psHub->psDesc );
				psHub->psDesc = NULL;
			}
			delete_semaphore( psHub->hLock );
			kfree( psHub );
			spinunlock_enable( &g_hHubLock, nFlags );
			return;
		}
		psPrevHub = psNextHub;
		psNextHub = psNextHub->psNext;
	}
	spinunlock_enable( &g_hHubLock, nFlags );
}

inline char *portspeed (int portstatus)
{
	if (portstatus & (1 << USB_PORT_FEAT_HIGHSPEED))
    		return "480 Mb/s";
	else if (portstatus & (1 << USB_PORT_FEAT_LOWSPEED))
		return "1.5 Mb/s";
	else
		return "12 Mb/s";
}

static int usb_hub_reset( USB_hub *psHub )
{
	USB_device_s *psDev = psHub->psDevice;
	int i;

	/* Disconnect any attached devices */
	for (i = 0; i < psHub->psDesc->nNbrPorts; i++ ) {
		if ( psDev->psChildren[i] )
			usb_disconnect( &psDev->psChildren[i] );
	}

	/* Attempt to reset the hub */
	if( psHub->psPacket )
		usb_cancel_packet( psHub->psPacket );
	else
		return -1;

	if( usb_reset_device( psDev ) )
		return -1;

	psHub->psPacket->psDevice = psDev;
	psHub->psPacket->nStatus = 0;
	if( usb_submit_packet( psHub->psPacket ) )
		return -1;

	usb_hub_power_on( psHub );

	return 0;
}

#define HUB_RESET_TRIES		5
#define HUB_PROBE_TRIES		2
#define HUB_SHORT_RESET_TIME	10
#define HUB_LONG_RESET_TIME	200
#define HUB_RESET_TIMEOUT	500

/* return: -1 on error, 0 on success, 1 on disconnect.  */
int usb_hub_port_wait_reset( USB_device_s* psHub, int nPort,
				USB_device_s* psDevice, unsigned int nDelay )
{
	int delay_time, ret;
	USB_port_status portsts;
	unsigned short portchange, portstatus;

	for( delay_time = 0; delay_time < HUB_RESET_TIMEOUT; delay_time += nDelay ) {
		/* wait to give the device a chance to reset */
		snooze( nDelay * 1000 );

		/* read and decode port status */
		ret = usb_get_port_status( psHub, nPort + 1, &portsts );
		if( ret < 0 ) {
			kerndbg( KERN_WARNING, "USB: get_port_status(%d) failed (err = %d)\n", nPort + 1, ret );
			return( -1 );
		}

		portstatus = portsts.nPortStatus;
		portchange = portsts.nPortChange;
		kerndbg( KERN_DEBUG, "Port %d, portstatus %x, change %x, %s\n", nPort + 1,
			portstatus, portchange, portspeed (portstatus));
		
		/* Device went away? */
		if( !( portstatus & USB_PORT_STAT_CONNECTION ) )
			return 1;

		/* bomb out completely if something weird happened */
		if( ( portchange & USB_PORT_STAT_C_CONNECTION ) )
			return -1;

		/* if we`ve finished resetting, then break out of the loop */
		if( !(portstatus & USB_PORT_STAT_RESET) &&
		    (portstatus & USB_PORT_STAT_ENABLE) ) {
			if( portstatus & USB_PORT_STAT_HIGH_SPEED )
				psDevice->eSpeed = USB_SPEED_HIGH;
			else if( portstatus & USB_PORT_STAT_LOW_SPEED )
				psDevice->eSpeed = USB_SPEED_LOW;
			else
				psDevice->eSpeed = USB_SPEED_FULL;
			return( 0 );
		}

		/* switch to the long delay after two short delay failures */
		if( delay_time >= 2 * HUB_SHORT_RESET_TIME )
			nDelay = HUB_LONG_RESET_TIME;

		kerndbg( KERN_WARNING, "USB: Port %d of hub %d not reset yet, waiting %dms\n", nPort + 1,
			psHub->nDeviceNum, nDelay );
	}

	return( -1 );
}

/* return: -1 on error, 0 on success, 1 on disconnect.  */
int usb_hub_port_reset( USB_device_s* psHub, int nPort,
				USB_device_s* psDevice, unsigned int nDelay )
{
	int i, status;

	/* Reset the port */
	for( i = 0; i < HUB_RESET_TRIES; i++ ) {
		usb_set_port_feature( psHub, nPort + 1, USB_PORT_FEAT_RESET );

		/* return on disconnect or reset */
		status = usb_hub_port_wait_reset( psHub, nPort, psDevice, nDelay );
		if( status != -1 ) {
			usb_clear_port_feature( psHub, nPort + 1, USB_PORT_FEAT_C_RESET);
			return( status );
		}

		kerndbg( KERN_WARNING, "USB: Port %d of hub %d not enabled, trying reset again...\n",
			nPort + 1, psHub->nDeviceNum );
		nDelay = HUB_LONG_RESET_TIME;
	}

	kerndbg( KERN_WARNING, "USB: Cannot enable port %i of hub %d, disabling port.\n",
		nPort + 1, psHub->nDeviceNum );
	kerndbg( KERN_WARNING, "USB: Maybe the USB cable is bad?\n");

	return( -1 );
}



void usb_hub_port_disable( USB_device_s* psHub, int nPort )
{
	int nRet;

	nRet = usb_clear_port_feature( psHub, nPort + 1, USB_PORT_FEAT_ENABLE);
	if( nRet )
		kerndbg( KERN_WARNING, "USB: Cannot disable port %d of hub %d (err = %d)\n",
			nPort + 1, psHub->nDeviceNum, nRet );
}


/* USB 2.0 spec, 7.1.7.3 / fig 7-29:
 *
 * Between connect detection and reset signaling there must be a delay
 * of 100ms at least for debounce and power-settling. The corresponding
 * timer shall restart whenever the downstream port detects a disconnect.
 * 
 * Apparently there are some bluetooth and irda-dongles and a number
 * of low-speed devices which require longer delays of about 200-400ms.
 * Not covered by the spec - but easy to deal with.
 *
 * This implementation uses 400ms minimum debounce timeout and checks
 * every 100ms for transient disconnects to restart the delay.
 */

#define USB_HUB_DEBOUNCE_TIMEOUT	400
#define USB_HUB_DEBOUNCE_STEP	100

/* return: -1 on error, 0 on success, 1 on disconnect.  */
static int usb_hub_port_debounce( USB_device_s* psDevice, int nPort )
{
	int nRet;
	unsigned nDelayTime;
	USB_port_status portsts;
	uint16 portchange, portstatus;

	for( nDelayTime = 0; nDelayTime < USB_HUB_DEBOUNCE_TIMEOUT; /* empty */ ) {

		/* wait debounce step increment */
		snooze( USB_HUB_DEBOUNCE_STEP * 1000 );

		nRet = usb_get_port_status( psDevice, nPort + 1, &portsts );
		if( nRet < 0 )
			return -1;
			
		portstatus = portsts.nPortStatus;
		portchange = portsts.nPortChange;

		if( ( portchange & USB_PORT_STAT_C_CONNECTION ) ) {
			usb_clear_port_feature( psDevice, nPort + 1, USB_PORT_FEAT_C_CONNECTION );
			nDelayTime = 0;
		}
		else
			nDelayTime += USB_HUB_DEBOUNCE_STEP;
	}
	return ( ( portstatus & USB_PORT_STAT_CONNECTION ) ) ? 0 : 1;
}

void usb_hub_port_connect_change( USB_hub* psH, int nPort,
					USB_port_status *psPortsts )
{
	USB_device_s* psHub = psH->psDevice;
	USB_device_s* psDevice;
	unsigned short portstatus, portchange;
	unsigned int delay = 10;
	int i;
	char *portstr, *tempstr;

	portstatus = psPortsts->nPortStatus;
	portchange = psPortsts->nPortChange;
	kerndbg( KERN_DEBUG, "Port %d, portstatus %x, change %x, %s\n",
		nPort + 1, portstatus, portchange, portspeed ( portstatus ) );

	/* Clear the connection change status */
	usb_clear_port_feature( psHub, nPort + 1, USB_PORT_FEAT_C_CONNECTION );
	
	/* Disconnect any existing devices under this port */
	if( psHub->psChildren[nPort] )
		usb_disconnect( &psHub->psChildren[nPort] );
		
	/* Return now if nothing is connected */
	if( !(portstatus & USB_PORT_STAT_CONNECTION)) {
		if (portstatus & USB_PORT_STAT_ENABLE)
			usb_hub_port_disable( psHub, nPort );

		return;
	}

	if ( usb_hub_port_debounce( psHub, nPort ) ) {
		kerndbg( KERN_FATAL, "USB: connect-debounce failed, port %d disabled\n", nPort + 1 );
		usb_hub_port_disable( psHub, nPort);
		return;
	}
	
	LOCK( g_hHubAddressLock );

	tempstr = kmalloc( 1024, MEMF_KERNEL | MEMF_NOBLOCK | MEMF_CLEAR );
	portstr = kmalloc( 1024, MEMF_KERNEL | MEMF_NOBLOCK | MEMF_CLEAR );
	
	for( i = 0; i < 2; i++ ) {
		USB_device_s* psPDev;
		USB_device_s* psCDev;
		
		/* Allocate a new device struct */
		psDevice = usb_alloc_device( psHub, psHub->psBus );
		if( !psDevice ) {
			kerndbg( KERN_WARNING, "USB: Couldn't allocate usb_device\n");
			break;
		}
		
		psHub->psChildren[nPort] = psDevice;

		/* Reset the device */
		if( usb_hub_port_reset( psHub, nPort, psDevice, delay ) ) {
			usb_free_device( psDevice );
			break;
		}
		
		/* Find a new device ID for it */
		usb_connect( psDevice );
		
		/* Set up TT records, if needed  */
		if( psHub->psTT ) {
			psDevice->psTT = psHub->psTT;
			psDevice->nTTPort = psHub->nTTPort;
		} else if( psDevice->eSpeed != USB_SPEED_HIGH
					&& psHub->eSpeed == USB_SPEED_HIGH ) {
			psDevice->psTT = psH->psTT;
			psDevice->nTTPort = nPort + 1;
		}
		
		/* Create a readable topology string */
		psCDev = psDevice;
		psPDev = psDevice->psParent;
		if (portstr && tempstr) {
			portstr[0] = 0;
			while( psPDev ) {
				int port;

				for( port = 0; port < psPDev->nMaxChild; port++ )
					if( psPDev->psChildren[nPort] == psCDev )
						break;

				strcpy(tempstr, portstr);
				if (!strlen(tempstr))
					sprintf(portstr, "%d", port + 1);
				else
					sprintf(portstr, "%d/%s", port + 1, tempstr);

				psCDev = psPDev;
				psPDev = psPDev->psParent;
			}
			kerndbg( KERN_DEBUG, "USB new device connect on bus%d/%s, assigned device number %d\n",
				psDevice->psBus->nBusNum, portstr, psDevice->nDeviceNum );
		} 
		else if( KERN_DEBUG >= DEBUG_LIMIT )
		{
			printk("USB new device connect on bus%d, assigned device number %d\n",
				psDevice->psBus->nBusNum, psDevice->nDeviceNum );
		}

		/* Run it through the hoops (find a driver, etc) */
		if ( !usb_new_device( psDevice ) ) {
			goto done;
		}
			
		/* Free the configuration if there was an error */
		usb_free_device( psDevice );
		
		/* Switch to a long reset time */
		delay = 200;
	}
	psHub->psChildren[nPort] = NULL;
	usb_hub_port_disable( psHub, nPort );
done:
	UNLOCK( g_hHubAddressLock );
	if (portstr)
		kfree(portstr);
	if (tempstr)
		kfree(tempstr);
}



void usb_hub_thread_worker()
{
	USB_hub* psHub;
	USB_hub_status hubsts;
	USB_device_s* psDevice;
	bool bFound;
	int i;
	int nRet;
	unsigned short hubstatus, hubchange;
	unsigned long nFlags;
	
	while( 1 )
	{
		
		kerndbg( KERN_DEBUG, "Hub needs attention\n" );
		
		spinlock_cli( &g_hHubLock, nFlags );
		/* Look if any of our hubs needs attention */
		bFound = false;
		
		psHub = g_psFirstHub;
		while( psHub != NULL && !bFound )
		{
			if( psHub->bNeedsAttention ) {
				bFound = true;
			}
			if( !bFound )
				psHub = psHub->psNext;
		}
		if( !bFound ) {
			break;
		}
		
		psHub->bNeedsAttention = false;
		psDevice = psHub->psDevice;
		
		LOCK( psHub->hLock );
		spinunlock_restore( &g_hHubLock, nFlags );
	
		kerndbg( KERN_DEBUG, "Working... (HUB %x DEVICE %x DESC %x)\n", psHub,
		( psHub != NULL ) ? psHub->psDevice : NULL, ( psHub != NULL ) ? psHub->psDesc : NULL );
		
		if( psHub->nError ) {
			kerndbg( KERN_WARNING, "USB: Hub reports an error, trying to reset...\n" );
			psHub->nError = 0;
			psHub->nErrors = 0;
			
			if( usb_hub_reset( psHub ) ) {
				kerndbg( KERN_WARNING, "USB: Hub reset failed!\n" );
				UNLOCK( psHub->hLock );
				continue;
			}
		}
		kerndbg( KERN_DEBUG, "Check ports...\n" );
		for( i = 0; i < psHub->psDesc->nNbrPorts; i++ ) {
			USB_port_status sPortsts;
			unsigned short portstatus, portchange;

			nRet = usb_get_port_status( psDevice, i + 1, &sPortsts );
			if( nRet < 0 ) {
				kerndbg( KERN_WARNING, "USB: get_port_status failed (err = %d)\n", nRet);
				continue;
			}

			portstatus = sPortsts.nPortStatus;
			portchange = sPortsts.nPortChange;

			if( portchange & USB_PORT_STAT_C_CONNECTION ) {
				kerndbg( KERN_DEBUG, "Port %d connection change\n", i + 1);

				usb_hub_port_connect_change( psHub, i, &sPortsts );
			} else if( portchange & USB_PORT_STAT_C_ENABLE ) {
				kerndbg( KERN_DEBUG, "Port %d enable change, status %x\n", i + 1, portstatus);
				usb_clear_port_feature( psDevice, i + 1, USB_PORT_FEAT_C_ENABLE );

				/*
				 * EM interference sometimes causes bad shielded USB devices to 
				 * be shutdown by the hub, this hack enables them again.
				 * Works at least with mouse driver. 
				 */
				if (!(portstatus & USB_PORT_STAT_ENABLE) && 
				    (portstatus & USB_PORT_STAT_CONNECTION) && (psDevice->psChildren[i])) {
					kerndbg( KERN_WARNING, "USB: Already running port %i disabled by hub (EMI?), re-enabling...",
						i + 1);
					usb_hub_port_connect_change( psHub, i, &sPortsts );
				}
			}

			if( portchange & USB_PORT_STAT_C_SUSPEND ) {
				kerndbg( KERN_DEBUG, "Port %d suspend change\n", i + 1);
				usb_clear_port_feature( psDevice, i + 1,  USB_PORT_FEAT_C_SUSPEND );
			}
			
			if( portchange & USB_PORT_STAT_C_OVERCURRENT ) {
				kerndbg( KERN_DEBUG, "Port %d over-current change\n", i + 1);
				usb_clear_port_feature( psDevice, i + 1, USB_PORT_FEAT_C_OVER_CURRENT );
				usb_hub_power_on( psHub );
			}

			if (portchange & USB_PORT_STAT_C_RESET) {
				kerndbg( KERN_DEBUG, "Port %d reset change", i + 1);
				usb_clear_port_feature( psDevice, i + 1, USB_PORT_FEAT_C_RESET );
			}
		} /* end for i */
		
		
		kerndbg( KERN_DEBUG, "Check hub...\n" );

		/* deal with hub status changes */
		if( usb_get_hub_status( psDevice, &hubsts ) < 0 )
			printk( "USB: get_hub_status failed\n");
		else {
			hubstatus = hubsts.nHubStatus;
			hubchange = hubsts.nHubChange;
			if( hubchange & HUB_CHANGE_LOCAL_POWER ) {
				kerndbg( KERN_DEBUG, "hub power change\n" );
				usb_clear_hub_feature( psDevice, C_HUB_LOCAL_POWER );
			}
			if( hubchange & HUB_CHANGE_OVERCURRENT ) {
				kerndbg( KERN_DEBUG, "Hub overcurrent change\n");
				snooze( 500 * 1000 ); /* Cool down */
				usb_clear_hub_feature( psDevice, C_HUB_OVER_CURRENT );
				usb_hub_power_on( psHub );
			}
		}
		kerndbg( KERN_DEBUG, "Finished...\n" );
		UNLOCK( psHub->hLock );
	}
	spinunlock_restore( &g_hHubLock, nFlags );
}

int usb_hub_thread( void* pData )
{
	kerndbg( KERN_DEBUG,  "USB hub thread running\n" );
	while( 1 ) {
		usb_hub_thread_worker();
		sleep_on_sem( g_hHubWait, INFINITE_TIMEOUT );
		kerndbg( KERN_DEBUG,  "usb_hub_thread() waking up\n" );
	}
	return( 0 );
}

void usb_hub_init()
{
	/* Register */
	USB_driver_s* pcDriver = ( USB_driver_s* )kmalloc( sizeof( USB_driver_s ), MEMF_KERNEL | MEMF_NOBLOCK );
	
	g_psFirstHub = NULL;
	g_hHubWait = create_semaphore( "usb_hub_wait", 0, 0 );
	spinlock_init( &g_hHubLock, "usb_hub_lock" );
	g_hHubAddressLock = create_semaphore( "usb_hub_address_lock", 1, 0 );
	
	strcpy( pcDriver->zName, "USB HUB" );
	pcDriver->AddDevice = usb_hub_add;
	pcDriver->RemoveDevice = usb_hub_remove;
	
	usb_register_driver_force( pcDriver );
	
	/* Start thread */
	wakeup_thread( spawn_kernel_thread( "usb_hub_thread", usb_hub_thread, 0, 4096, NULL ), true );
}

int usb_reset_device( USB_device_s* dev )
{
	USB_device_s *parent = dev->psParent;
	USB_desc_device_s *descriptor;
	int i, ret, port = -1;
	
	kerndbg( KERN_DEBUG, "Resetting device...\n");

	if( !parent ) {
		kerndbg( KERN_WARNING, "USB: attempting to reset root hub!\n" );
		return -EINVAL;
	}

	for( i = 0; i < parent->nMaxChild; i++ )
		if( parent->psChildren[i] == dev ) {
			port = i;
			break;
		}

	if( port < 0 )
		return -ENOENT;

	LOCK( g_hHubAddressLock );

	/* Send a reset to the device */
	if( usb_hub_port_reset( parent, port, dev, HUB_SHORT_RESET_TIME ) ) {
		usb_hub_port_disable( parent, port );
		UNLOCK( g_hHubAddressLock );
		return(-ENODEV);
	}

	/* Reprogram the Address */
	ret = usb_set_address( dev );
	if( ret < 0 ) {
		kerndbg( KERN_FATAL, "USB: USB device not accepting new address (error=%d)", ret );
		usb_hub_port_disable( parent, port );
		UNLOCK( g_hHubAddressLock );
		return ret;
	}

	/* Let the SET_ADDRESS settle */
	snooze( 10000 );

	UNLOCK( g_hHubAddressLock );

	/*
	 * Now we fetch the configuration descriptors for the device and
	 * see if anything has changed. If it has, we dump the current
	 * parsed descriptors and reparse from scratch. Then we leave
	 * the device alone for the caller to finish setting up.
	 *
	 * If nothing changed, we reprogram the configuration and then
	 * the alternate settings.
	 */
	descriptor = kmalloc( sizeof *descriptor, MEMF_KERNEL );
	if( !descriptor ) {
		return -ENOMEM;
	}
	ret = usb_get_descriptor(dev, USB_DT_DEVICE, 0, descriptor,
			sizeof( *descriptor ) );
	if( ret < 0 ) {
		kfree(descriptor);
		return ret;
	}

	if( memcmp( &dev->sDeviceDesc, descriptor, sizeof( *descriptor ) ) ) {
		kfree( descriptor );
		kerndbg( KERN_FATAL, "USB: Device descriptor has changed during reset!\n" );
		return 1;
	}

	kfree( descriptor );

	ret = usb_set_configuration( dev, dev->psActConfig->nConfigValue );
	if( ret < 0 ) {
		kerndbg( KERN_FATAL, "USB: failed to set active configuration (error=%d)\n", ret );
		return ret;
	}

	for (i = 0; i < dev->psActConfig->nNumInterfaces; i++ ) {
		USB_interface_s *intf = &dev->psActConfig->psInterface[i];
		USB_desc_interface_s *as = &intf->psSetting[intf->nActSetting];

		ret = usb_set_interface( dev, as->nInterfaceNumber, as->nAlternateSettings );
		if( ret < 0 ) {
			kerndbg( KERN_FATAL, "USB: failed to set active alternate setting for interface %d (error=%d)\n", i, ret );
			return ret;
		}
	}

	return 0;
}




