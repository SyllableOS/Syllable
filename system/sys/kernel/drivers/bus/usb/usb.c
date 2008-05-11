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
#include <atheos/bootmodules.h>
#include <atheos/device.h>
#include <posix/errno.h>
#include <macros.h>

//#undef DEBUG_LIMIT
//#define DEBUG_LIMIT KERN_DEBUG

#define MAX_USB_DRIVERS 128

int g_nDeviceID;
bool g_bUSBBusMap[64]; /* bitmap of all buses */
USB_bus_driver_s* g_psUSBBus[64];
USB_driver_s* g_psFirstUSBDriver; /* Head of the list of all registered drivers */
SpinLock_s g_hUSBLock;

#define USB_LOCK spinlock( &g_hUSBLock )
#define USB_UNLOCK spinunlock( &g_hUSBLock );

#define mb() 	__asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory")
#define wmb()	__asm__ __volatile__ ("": : :"memory")
#define rmb()	mb()

status_t usb_find_driver( USB_device_s* psDevice );


/** 
 * \par Description: Registers one driver.
 * \par Note: The hLock semaphore does not have not be created before.
 * \par Warning:
 * \param psDriver - Pointer to the driver description.
 * \return 0 if sucessful or < 0 if the driver does not support any device.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t usb_register_driver( USB_driver_s* psDriver )
{
	uint32 i;
	bool bFound = false;
	
	USB_LOCK;
	
	/* Create semaphore */
	psDriver->hLock = create_semaphore( "usb_driver_lock", 1, 0 );
	/* Move driver at the first position */
	psDriver->psNext = g_psFirstUSBDriver;
	g_psFirstUSBDriver = psDriver;
	
	
	kerndbg( KERN_INFO, "USB: %s driver added\n", g_psFirstUSBDriver->zName );
	
	for( i = 0; i < 64; i++ )
	{
		if( g_bUSBBusMap[i] )
			if( usb_find_driver( g_psUSBBus[i]->psRootHUB ) == 0 )
				bFound = true; 
	}
	
	if( bFound == true ) {
		USB_UNLOCK;
		return( 0 );
	}
	/* No new driver for any device found -> driver is unnecessary */
	kerndbg( KERN_INFO, "USB: %s driver unnecessary\n", g_psFirstUSBDriver->zName );
	g_psFirstUSBDriver = psDriver->psNext;
	USB_UNLOCK;
	return( -1 );
}


/** 
 * \par Description: Same as usb_register_driver, but driver stays loaded.
 * \par Note: The hLock semaphore does not have not be created before.
 * \par Warning:
 * \param psDriver - Pointer to the driver description.
 * \return
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_register_driver_force( USB_driver_s* psDriver )
{
	uint32 i;
	USB_LOCK;
	
	/* Create semaphore */
	psDriver->hLock = create_semaphore( "usb_driver_lock", 1, 0 );
	/* Move driver at the first position */
	
	psDriver->psNext = g_psFirstUSBDriver;
	g_psFirstUSBDriver = psDriver;
	
	
	kerndbg( KERN_INFO, "USB: %s driver registered\n", g_psFirstUSBDriver->zName );
	
	
	for( i = 0; i < 64; i++ )
	{
		if( g_bUSBBusMap[i] )
			usb_find_driver( g_psUSBBus[i]->psRootHUB );
	}
	USB_UNLOCK;
}


/** 
 * \par Description: Remove driver from all devices and their children.
 * \param psDriver - Pointer to the driver description.
 * \param psDevice - Pointer to the first device.
 * \return
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_remove_driver( USB_driver_s* psDriver, USB_device_s* psDev )
{
	uint32 i;
	if( !psDev )
		return;
	
	/* Do recursive call */
	for( i = 0; i < 16; i++ )
		if( psDev->psChildren[i] )
			usb_remove_driver( psDriver, psDev->psChildren[i] );
	
	if( !psDev->psActConfig )
		return;
		
	/* Remove driver from all interfaces */
	for( i = 0; i < psDev->psActConfig->nNumInterfaces; i++ )
	{
		USB_interface_s* psIF = &psDev->psActConfig->psInterface[i];
		
		if( psIF->psDriver == psDriver )
		{
			LOCK( psDriver->hLock );
			psDriver->RemoveDevice( psDev, psIF->pPrivate );
			UNLOCK( psDriver->hLock );
			if( psIF->psDriver ) 
			{
				/* Buggy driver */
				psIF->psDriver = NULL;
				psIF->pPrivate = NULL;
			}
		}
	}
}

/** 
 * \par Description: Deregisters one driver.
 * \param psDriver - Pointer to the driver description.
 * \return
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_deregister_driver( USB_driver_s* psDriver )
{
	uint32 i;
	/* Remove driver from the list */
	USB_driver_s* psPrevDriver = NULL;
	USB_driver_s* psNextDriver = g_psFirstUSBDriver;
	USB_LOCK;
	while( psNextDriver != NULL )
	{
		if( psNextDriver == psDriver )
		{
			/* Remove it */
			delete_semaphore( psNextDriver->hLock );
			if( psPrevDriver )
				psPrevDriver->psNext = psNextDriver->psNext;
			else
				g_psFirstUSBDriver = psNextDriver->psNext;
			for( i = 0; i < 64; i++ )
			{
				if( g_bUSBBusMap[i] )
					usb_remove_driver( psDriver, g_psUSBBus[i]->psRootHUB ); 
			}
			kerndbg( KERN_INFO, "USB: %s driver deregistered\n", psDriver->zName );
			USB_UNLOCK;
			return;
		}
		psPrevDriver = psNextDriver;
		psNextDriver = psNextDriver->psNext;
	}
	USB_UNLOCK;
	kerndbg( KERN_WARNING, "USB: Could not deregister not registered %s driver\n", psDriver->zName );
}


/** 
 * \par Description: Allocates one USB bus.
 * \param
 * \return
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
USB_bus_driver_s* usb_alloc_bus()
{
	USB_bus_driver_s* psBus;
	
	psBus = ( USB_bus_driver_s* )kmalloc( sizeof( USB_bus_driver_s ), MEMF_KERNEL | MEMF_NOBLOCK );
	memset( psBus, 0, sizeof( USB_bus_driver_s ) );
	
	memset( psBus->bDevMap, 0, 128 );
	
	/* Set default values */
	psBus->nBusNum = -1;
	atomic_set( &psBus->nRefCount, 1 );
	
	psBus->psRootHUB = NULL;
	psBus->pHCPrivate = NULL;
	psBus->nBandWidthAllocated = 0;
	psBus->nBandWidthInt = 0;
	psBus->nBandWidthISO = 0;
	
	return( psBus );
}


/** 
 * \par Description: Frees one bus.
 * \param psBus - Pointer to the bus driver.
 * \return
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_free_bus( USB_bus_driver_s* psBus )
{
	if ( atomic_dec_and_test( &psBus->nRefCount ) )
		kfree( psBus );
}


/** 
 * \par Description: Registers one bus.
 * \param psBus - Pointer to the bus driver.
 * \return
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_register_bus( USB_bus_driver_s* psBus )
{
	uint32 i;
	USB_LOCK;
	/* Find busnumber */
	for( i = 0; i < 64; i++ )
		if( !g_bUSBBusMap[i] ) {
			break;
		}
	if( i == 64 ) {
		USB_UNLOCK;
		return;
	}
	
	g_bUSBBusMap[i] = true;
	g_psUSBBus[i] = psBus;
	psBus->nBusNum = i;
	atomic_inc( &psBus->nRefCount );
	USB_UNLOCK;
	
	kerndbg( KERN_INFO, "USB: Bus %i registered\n", (int)i );
}


/** 
 * \par Description: Deregister one bus.
 * \param psBus - Pointer to the bus driver.
 * \return
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_deregister_bus( USB_bus_driver_s* psBus )
{
	uint32 i;
	USB_LOCK;
	/* Find busnumber */
	for( i = 0; i < 64; i++ )
		if( g_bUSBBusMap[i] && g_psUSBBus[i] == psBus ) {
			g_bUSBBusMap[i] = false;
			g_psUSBBus[i] = NULL;
			
			if( atomic_dec_and_test( &psBus->nRefCount ) )
				kfree( psBus );
			USB_UNLOCK;
			return;
		}
	USB_UNLOCK;
}


/** 
 * \par Description: Tries to find a driver for one device.
 * \param psDev - Pointer to the device.
 * \param nIFNum - Number of the interface.
 * \return 0 if a driver was found.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t usb_find_interface_driver( USB_device_s* psDev, int nIFNum )
{
	USB_interface_s* psIF;
	USB_driver_s* psDriver;
	bool bSuccess;
	void* pPrivate;
	
	LOCK( psDev->hLock );

	psIF = psDev->psActConfig->psInterface + nIFNum;
	
	if( psIF->psDriver != NULL ) {
		/* there is already a driver for this interface */
		UNLOCK( psDev->hLock );
		return( -1 );
	}
	
	/* Go through all registered driver */
	psDriver = g_psFirstUSBDriver;
	
	while( psDriver != NULL )
	{
		LOCK( psDriver->hLock );
		bSuccess = psDriver->AddDevice( psDev, nIFNum, &pPrivate );
		UNLOCK( psDriver->hLock );
		
		psIF = psDev->psActConfig->psInterface + nIFNum;
		if( bSuccess ) {
			UNLOCK( psDev->hLock );
			psIF->psDriver = psDriver;
			psIF->pPrivate = pPrivate;
			kerndbg( KERN_INFO, "USB: Device claimed by %s driver\n", psDriver->zName );
			return( 0 );
		}
		psDriver = psDriver->psNext;
	}
	UNLOCK( psDev->hLock );
	kerndbg( KERN_INFO, "USB: No driver found\n");
	return( -1 );
}


/** 
 * \par Description: Tries to find a driver for one device.
 * \param psBus - Pointer to the bus driver.
 * \return 0 if successful.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t usb_find_driver( USB_device_s* psDev )
{
	int i;
	status_t nRet = -1;
	if( !psDev )
		return( -1 );
	
	/* Do recursive call */
	for( i = 0; i < 16; i++ )
		if( psDev->psChildren[i] )
			if( usb_find_driver( psDev->psChildren[i] ) == 0 )
				nRet = 0;
	
	if( !psDev->psActConfig )
		return( -1 );

	if( psDev->nDeviceNum > 0 )
		for( i = 0; i < psDev->psActConfig->nNumInterfaces; i++ )
			if( usb_find_interface_driver( psDev, i ) == 0 )
				nRet = 0;
	return( nRet );
}


/** 
 * \par Description: Goes through the list of registered drivers and tries to find one for
 *					a new device.
 * \param psDev - Pointer to the device.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_find_drivers_for_new_device( USB_device_s* psDev )
{
	uint32 i;
	
	for( i = 0; i < psDev->psActConfig->nNumInterfaces; i++ )
	{
		if( ( psDev->psActConfig->psInterface + i )->psDriver == NULL )
		{
			usb_find_interface_driver( psDev, i );
		}
	}
}

/*
 * Some USB bandwidth allocation constants.
 */
#define BW_HOST_DELAY	1000L		/* nanoseconds */
#define BW_HUB_LS_SETUP	333L		/* nanoseconds */
                        /* 4 full-speed bit times (est.) */

#define FRAME_TIME_BITS         12000L		/* frame = 1 millisecond */
#define FRAME_TIME_MAX_BITS_ALLOC	(90L * FRAME_TIME_BITS / 100L)
#define FRAME_TIME_USECS	1000L
#define FRAME_TIME_MAX_USECS_ALLOC	(90L * FRAME_TIME_USECS / 100L)

#define BitTime(bytecount)  (7 * 8 * bytecount / 6)  /* with integer truncation */
		/* Trying not to use worst-case bit-stuffing
                   of (7/6 * 8 * bytecount) = 9.33 * bytecount */
		/* bytecount = data payload byte count */

#define NS_TO_US(ns)	((ns + 500L) / 1000L)
			/* convert & round nanoseconds to microseconds */

/*
 * Ceiling microseconds (typical) for that many bytes at high speed
 * ISO is a bit less, no ACK ... from USB 2.0 spec, 5.11.3 (and needed
 * to preallocate bandwidth)
 */
#define USB2_HOST_DELAY	5	/* nsec, guess */
#define HS_USECS(bytes) NS_TO_US ( ((55 * 8 * 2083)/1000) \
	+ ((2083UL * (3167 + BitTime (bytes)))/1000) \
	+ USB2_HOST_DELAY)
#define HS_USECS_ISO(bytes) NS_TO_US ( ((long)(38 * 8 * 2.083)) \
	+ ((2083UL * (3167 + BitTime (bytes)))/1000) \
	+ USB2_HOST_DELAY)	
			
/*
 * usb_calc_bus_time - approximate periodic transaction time in nanoseconds
 * @speed: from dev->speed; USB_SPEED_{LOW,FULL,HIGH}
 * @is_input: true iff the transaction sends data to the host
 * @isoc: true for isochronous transactions, false for interrupt ones
 * @bytecount: how many bytes in the transaction.
 *
 * Returns approximate bus time in nanoseconds for a periodic transaction.
 * See USB 2.0 spec section 5.11.3; only periodic transfers need to be
 * scheduled in software, this function is only used for such scheduling.
 */
long usb_calc_bus_time (int speed, int is_input, int isoc, int bytecount)
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

/** 
 * \par Description: Checks if there is enough bandwidth for the packet.
 * \par Note:
 * \par Warning:
 * \param psDevice - Pointer to the device.
 * \param psPacket - Pointer to the packet.
 * \return Bustime or -ENOSPC.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int usb_check_bandwidth( USB_device_s* psDevice, USB_packet_s* psPacket )
{
	int		nNewAlloc;
	int		nOldAlloc = psDevice->psBus->nBandWidthAllocated;
	unsigned int nPipe = psPacket->nPipe;
	long		nBusTime;

	nBusTime = usb_calc_bus_time( psDevice->eSpeed, usb_pipein( nPipe ),
			usb_pipeisoc( nPipe ), usb_maxpacket( psDevice, nPipe, usb_pipeout( nPipe ) ) );
	if( usb_pipeisoc( nPipe ) )
		nBusTime = NS_TO_US( nBusTime ) / psPacket->nPacketNum;
	else
		nBusTime = NS_TO_US( nBusTime );

	nNewAlloc = nOldAlloc + (int)nBusTime;
		/* what new total allocated bus time would be */

	if( nNewAlloc > FRAME_TIME_MAX_USECS_ALLOC )
		kerndbg( KERN_WARNING, "USB: usb-check-bandwidth %sFAILED: was %u, would be %u, bustime = %ld us\n",
			"would have ",
			nOldAlloc, nNewAlloc, nBusTime );

	
	return( ( nNewAlloc <= FRAME_TIME_MAX_USECS_ALLOC ) ? nBusTime : -ENOSPC );
}


/** 
 * \par Description: Claims bandwidth.
 * \param psDevice - Pointer to the device.
 * \param psPacket - Pointer to the packet.
 * \param nBusTime - Bustime.
 * \param nISOc - Number of ISO packets.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_claim_bandwidth( USB_device_s* psDevice, USB_packet_s* psPacket, int nBusTime, int nISOc )
{
	psDevice->psBus->nBandWidthAllocated += nBusTime;
	if ( nISOc )
		psDevice->psBus->nBandWidthISO++;
	else
		psDevice->psBus->nBandWidthInt++;
	
	psPacket->nBandWidth = nBusTime;
}

/** 
 * \par Description: Releases bandwidth.
 * \param psDevice - Pointer to the device.
 * \param psPacket - Pointer to the packet.
 * \param nISOc - Number of ISO packets.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_release_bandwidth( USB_device_s* psDevice, USB_packet_s* psPacket, int nISOc )
{
	psDevice->psBus->nBandWidthAllocated -= psPacket->nBandWidth;
	
	if ( nISOc )
		psDevice->psBus->nBandWidthISO--;
	else
		psDevice->psBus->nBandWidthInt--;
	
	psPacket->nBandWidth = 0;
}



/** 
 * \par Description: Allocates one USB device.
 * \param psParent - Pointer to the parent device.
 * \param psPacket - Pointer to the bus.
 * \return Pointer to the device.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
USB_device_s* usb_alloc_device( USB_device_s* psParent, USB_bus_driver_s* psBus )
{
	USB_device_s* psDevice;
	
	psDevice = ( USB_device_s* )kmalloc( sizeof( USB_device_s ), MEMF_KERNEL | MEMF_NOBLOCK );
	memset( psDevice, 0, sizeof( USB_device_s ) );
	
	atomic_inc( &psBus->nRefCount );
	
	psDevice->psBus = psBus;
	psDevice->psParent = psParent;
	atomic_set( &psDevice->nRefCount, 1 );
	
	psDevice->hLock = create_semaphore( "usb_device_lock", 1, 0 );
	
	psDevice->psBus->AddDevice( psDevice );
	
	return( psDevice );
}


/** 
 * \par Description: Frees one USB device.
 * \param psDevice - Pointer to the device.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_free_device( USB_device_s* psDevice )
{
	if( atomic_dec_and_test( &psDevice->nRefCount ) ) {
		psDevice->psBus->RemoveDevice( psDevice );
		
		atomic_dec( &psDevice->psBus->nRefCount );
		kfree( psDevice );
		
		//printk( "USB: TODO: usb_free_device()\n" );
		// TODO !!!
	}
}


/** 
 * \par Description: Allocates one USB packet.
 * \param nISOPackets - Number of requested ISO packets.
 * \return Pointer to the packet.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
USB_packet_s* usb_alloc_packet( int nISOPackets )
{
	USB_packet_s* psPacket;
	
	psPacket = ( USB_packet_s* )kmalloc( sizeof( USB_packet_s ) + nISOPackets * sizeof( USB_ISO_frame_s ), MEMF_KERNEL | MEMF_NOBLOCK );
	memset( psPacket, 0, sizeof( USB_packet_s ) + nISOPackets * sizeof( USB_ISO_frame_s ) );
	
	spinlock_init( &psPacket->hLock, "usb_packet_lock" );
	
	return( psPacket );
}


/** 
 * \par Description: Frees one USB packet.
 * \param psPacket - Pointer to the packet.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_free_packet( USB_packet_s* psPacket )
{
	if( psPacket )
		kfree( psPacket );
}


/** 
 * \par Description: Submits one USB packet.
 * \param psPacket - Pointer to the packet.
 * \sa
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t usb_submit_packet( USB_packet_s* psPacket )
{
	if( psPacket && psPacket->psDevice && psPacket->psDevice->psBus )
		return( psPacket->psDevice->psBus->SubmitPacket( psPacket ) );
	return( -ENODEV );
}


/** 
 * \par Description: Cancels one USB packet.
 * \param psPacket - Pointer to the packet.
 * \sa
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t usb_cancel_packet( USB_packet_s* psPacket )
{
	if( psPacket && psPacket->psDevice && psPacket->psDevice->psBus )
		return( psPacket->psDevice->psBus->CancelPacket( psPacket ) );
	return( -ENODEV );
}

void usb_blocked_complete( USB_packet_s* psPacket )
{
	psPacket->bDone = true;
	wmb();
	UNLOCK( psPacket->hWait );
	
}

int usb_submit_packet_blocked( USB_packet_s* psPacket, bigtime_t nTimeOut, int* pnLength )
{
	int nStatus;
	bigtime_t nTime = get_system_time() + nTimeOut;
	
	kerndbg( KERN_DEBUG, "usb_submit_packet_blocked(): Submitting usb packet...\n" );
	
	psPacket->bDone = false;
	psPacket->hWait = create_semaphore( "usb_block_packet", 0, 0 );
	
	nStatus = usb_submit_packet( psPacket );
	if( nStatus != 0 )
	{
		kerndbg( KERN_DEBUG, "usb_submit_packet_blocked(): Failed!\n" );
		usb_free_packet( psPacket );
		delete_semaphore( psPacket->hWait );
		return( nStatus );
	}
	kerndbg( KERN_DEBUG, "usb_submit_packet_blocked(): Waiting...\n" );
	while( nTime > get_system_time() && !psPacket->bDone )
	{
		lock_semaphore( psPacket->hWait, 0, nTimeOut );
		rmb();
	}
	kerndbg( KERN_DEBUG, "usb_submit_packet_blocked(): Done!\n" );
	
	delete_semaphore( psPacket->hWait );
	
	kerndbg( KERN_DEBUG, "usb_submit_packet_blocked(): Checking status...\n" );
	
	if( !psPacket->bDone ) {
		if( psPacket->nStatus != -EINPROGRESS ) {
			kerndbg( KERN_WARNING, "USB: usb_submit_block_packet() -> timeout\n" );
			nStatus = psPacket->nStatus;
		} else {
			kerndbg( KERN_WARNING, "USB: usb_submit_block_packet() -> timeout (urb still pending)\n" );
			usb_cancel_packet( psPacket );
			nStatus = -ETIMEDOUT;
		} 
	} else
		nStatus = psPacket->nStatus;
		
	if( pnLength )
		*pnLength = psPacket->nActualLength;
		
	usb_free_packet( psPacket );
	return( nStatus );
}

int usb_internal_control_msg( USB_device_s* psDevice, unsigned int nPipe,
						USB_request_s* psCmd, void* pData, int nLen, bigtime_t nTimeOut )
{
	int nLength;
	USB_packet_s* psPacket = usb_alloc_packet( 0 );
	
	if( !psPacket )
		return( -ENOMEM );
	
	/* Fill packet */
	USB_FILL_CONTROL( psPacket, psDevice, nPipe, ( uint8* )psCmd, pData, nLen,
					usb_blocked_complete, NULL );
	if( usb_submit_packet_blocked( psPacket, nTimeOut, &nLength ) < 0 )
		return( -1 );
	else
		return nLength;
}


/** 
 * \par Description: Sends one USB control message.
 * \param psDevice - Pointer to the device.
 * \param nPipe - Pipe.
 * \param nRequest - Request number.
 * \param nRequestType - Type of the request.
 * \param nValue - Value.
 * \param nIndex - Index.
 * \param pData - Pointer to the data buffer.
 * \param nSize - Size of the data buffer.
 * \param nTimeOut - Timeout time.
 * \sa
 * \return 0 if successful.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int usb_control_msg( USB_device_s* psDevice, unsigned int nPipe, uint8 nRequest, 
					uint8 nRequestType, uint16 nValue, uint16 nIndex, void* pData, uint16 nSize,
					bigtime_t nTimeOut )
{
	int nResult;
	USB_request_s* psReq = ( USB_request_s* )kmalloc( sizeof( USB_request_s ), MEMF_KERNEL | MEMF_NOBLOCK );
	if( !psReq )
		return( -ENOMEM );
	
	/* Fill request */
	psReq->nRequestType = nRequestType;
	psReq->nRequest = nRequest;
	psReq->nValue = nValue;
	psReq->nIndex = nIndex;
	psReq->nLength = nSize;
	
	nResult = usb_internal_control_msg( psDevice, nPipe, psReq, pData, nSize, nTimeOut );
	kfree( psReq );
	
	return( nResult );
}

int usb_bulk_msg( USB_device_s* psDevice, unsigned int nPipe,
						void* pData, int nLen, int* pnActLength, bigtime_t nTimeOut )
{
	USB_packet_s* psPacket = usb_alloc_packet( 0 );
	
	if( !psPacket )
		return( -ENOMEM );
	
	if( nLen < 0 ) {
		usb_free_packet( psPacket );
		return( -EINVAL );
	}
	
	
	/* Fill packet */
	USB_FILL_BULK( psPacket, psDevice, nPipe, pData, nLen,
					usb_blocked_complete, NULL );
	return( usb_submit_packet_blocked( psPacket, nTimeOut, pnActLength ) );
}

int usb_get_current_frame_number( USB_device_s* psDevice )
{
	return( psDevice->psBus->GetFrameNumber( psDevice ) );
}

int usb_parse_endpoint( USB_desc_endpoint_s* psEndpoint, uint8* pBuffer, int nSize )
{
	USB_desc_header_s* psHeader;
	uint8* pBegin;
	int nParsed = 0, nLen, nNumSkipped;
	
	psHeader = ( USB_desc_header_s* )pBuffer;
	
	if( psHeader->nLength > nSize ) {
		kerndbg( KERN_WARNING, "USB: Error while parsing endpoint\n" );
		return( -1 );
	}
	
	if( psHeader->nDescriptorType != USB_DT_ENDPOINT ) {
		kerndbg( KERN_WARNING, "USB: Error while parsing endpoint\n" );
		return( nParsed );
	}
	
	if( psHeader->nLength == USB_DT_ENDPOINT_AUDIO_SIZE )
		memcpy( psEndpoint, pBuffer, USB_DT_ENDPOINT_AUDIO_SIZE );
	else
		memcpy( psEndpoint, pBuffer, USB_DT_ENDPOINT_SIZE );
	
	pBuffer += psHeader->nLength;
	nSize -= psHeader->nLength;
	nParsed += psHeader->nLength;
	
	/* Skip vendor specific data */
	pBegin = pBuffer;
	nNumSkipped = 0;
	
	while( nSize >= sizeof( USB_desc_header_s ) ) {
		psHeader = ( USB_desc_header_s* )pBuffer;
		
		if( psHeader->nLength < 2 ) {
			kerndbg( KERN_WARNING, "USB: Error while parsing endpoint\n" );
			return( -1 );
		}
		
		/* Look if we have found the next descriptors */
		if( ( psHeader->nDescriptorType == USB_DT_ENDPOINT ) ||
			( psHeader->nDescriptorType == USB_DT_INTERFACE ) ||
			( psHeader->nDescriptorType == USB_DT_CONFIG ) ||
			( psHeader->nDescriptorType == USB_DT_DEVICE ) )
			break;
			
		/* Skip data */
		kerndbg( KERN_DEBUG, "Skipping %i\n", psHeader->nDescriptorType );
		nNumSkipped++;
		
		pBuffer += psHeader->nLength;
		nSize -= psHeader->nLength;
		nParsed += psHeader->nLength;
	}
	if( nNumSkipped )
		kerndbg( KERN_DEBUG, "USB: %i of vendor specific descriptors skipped\n", nNumSkipped );
	
	nLen = (int)( pBuffer - pBegin );
	if( !nLen ) {
		psEndpoint->pExtra = NULL;
		psEndpoint->nExtraLen = 0;
		return( nParsed );
	}
	
	/* Allocate extra memory */
	psEndpoint->pExtra = kmalloc( nLen, MEMF_KERNEL | MEMF_NOBLOCK );
	if( !psEndpoint->pExtra ) {
		kerndbg( KERN_WARNING, "USB: Out of memory!\n" );
		return( nParsed );
	}
	memcpy( psEndpoint->pExtra, pBegin, nLen );
	psEndpoint->nExtraLen = nLen;
	
	return( nParsed );
}


int usb_parse_interface( USB_interface_s* psIF, uint8* pBuffer, int nSize )
{
	USB_desc_header_s* psHeader;
	USB_desc_interface_s* psIFDesc;
	uint8* pBegin;
	int nParsed = 0, nLen, nNumSkipped, i, nResult;
	
	psIF->nActSetting = 0;
	psIF->nNumSetting = 0;
	psIF->nMaxSetting = 16;
	
	psIF->psSetting = kmalloc( sizeof( USB_desc_interface_s ) * psIF->nMaxSetting, MEMF_KERNEL | MEMF_NOBLOCK );
	if( !psIF->psSetting ) {
		kerndbg( KERN_WARNING, "USB: Out of memory!\n" );
		return( -1 );
	}
	memset( psIF->psSetting, 0, sizeof( USB_desc_interface_s ) * psIF->nMaxSetting );
	
	while( nSize > 0 ) {
		if( psIF->nNumSetting >= psIF->nMaxSetting )
		{
			kerndbg( KERN_WARNING, "USB: Too many alternate settings -> increase number in usb_parse_interface()\n" );
			return( -1 );
		}
		
		psIFDesc = psIF->psSetting + psIF->nNumSetting;
		psIF->nNumSetting++;
		
		memcpy( psIFDesc, pBuffer, USB_DT_INTERFACE_SIZE );
		
		pBuffer += psIFDesc->nLength;
		nSize -= psIFDesc->nLength;
		nParsed += psIFDesc->nLength;
		
		pBegin = pBuffer;
		nNumSkipped = 0;
		
		while( nSize >= sizeof( USB_desc_header_s ) ) {
			psHeader = ( USB_desc_header_s* )pBuffer;
		
			if( psHeader->nLength < 2 ) {
				kerndbg( KERN_WARNING, "USB: Error while parsing interface\n" );
				return( -1 );
			}
		
			/* Look if we have found the next descriptors */
			if( ( psHeader->nDescriptorType == USB_DT_ENDPOINT ) ||
				( psHeader->nDescriptorType == USB_DT_INTERFACE ) ||
				( psHeader->nDescriptorType == USB_DT_CONFIG ) ||
				( psHeader->nDescriptorType == USB_DT_DEVICE ) )
				break;
			
			/* Skip data */
			kerndbg( KERN_DEBUG, "Skipping %i\n", psHeader->nDescriptorType );
			nNumSkipped++;
		
			pBuffer += psHeader->nLength;
			nSize -= psHeader->nLength;
			nParsed += psHeader->nLength;
		}
		if( nNumSkipped )
			kerndbg( KERN_DEBUG, "%i of vendor specific descriptors skipped\n", nNumSkipped );
	
		nLen = (int)( pBuffer - pBegin );
		if( !nLen ) {
			psIFDesc->pExtra = NULL;
			psIFDesc->nExtraLen = 0;
			
		} else {
	
			/* Allocate extra memory */
			psIFDesc->pExtra = kmalloc( nLen, MEMF_KERNEL | MEMF_NOBLOCK );
			if( !psIFDesc->pExtra ) {
				kerndbg( KERN_WARNING, "USB: Out of memory!\n" );
				return( -1 );
			}
			memset( psIFDesc->pExtra, 0, nLen );
			memcpy( psIFDesc->pExtra, pBegin, nLen );
			psIFDesc->nExtraLen = nLen;
		}
		
		psHeader = ( USB_desc_header_s* )pBuffer;
		if( ( nSize >= sizeof( USB_desc_header_s ) ) &&
			( ( psHeader->nDescriptorType == USB_DT_CONFIG ) ||
			  ( psHeader->nDescriptorType == USB_DT_DEVICE ) ) )
			  return( nParsed );
		
		kerndbg( KERN_DEBUG, "Interface has %i endpoints\n", psIFDesc->nNumEndpoints );
		
		/* Allocate endpoints */
		psIFDesc->psEndpoint = ( USB_desc_endpoint_s* )kmalloc( sizeof( USB_desc_endpoint_s ) *
															psIFDesc->nNumEndpoints, MEMF_KERNEL | MEMF_NOBLOCK );
		if( !psIFDesc->psEndpoint ) {
			kerndbg( KERN_WARNING, "USB: Out of memory!\n" );
			return( -1 );
		}
		memset( psIFDesc->psEndpoint, 0, sizeof( USB_desc_endpoint_s ) * psIFDesc->nNumEndpoints );
		
		/* Now parse the endpoints */
		for( i = 0; i < psIFDesc->nNumEndpoints; i++ )
		{
			psHeader = ( USB_desc_header_s* )pBuffer;
			
			if( psHeader->nLength > nSize ) {
				kerndbg( KERN_WARNING, "USB: Invalid endpoint header\n" );
				return( -1 );
			}
			nResult = usb_parse_endpoint( psIFDesc->psEndpoint + i, pBuffer, nSize );
			if( nResult < 0)
				return( nResult );
			pBuffer += nResult;
			nSize -= nResult;
			nParsed += nResult;
		}
		psIFDesc = ( USB_desc_interface_s* )pBuffer;
		if( nSize < USB_DT_INTERFACE_SIZE || psIFDesc->nDescriptorType != USB_DT_INTERFACE ||
			!psIFDesc->nAlternateSettings )
			return( nParsed );
	}
	return( nParsed );
}

int usb_parse_config( USB_desc_config_s* psConfig, uint8* pBuffer )
{
	int i, nResult, nSize;
	USB_desc_header_s* psHeader;
	
	memcpy( psConfig, pBuffer, USB_DT_CONFIG_SIZE );
	nSize = psConfig->nTotalLength;
	
	kerndbg( KERN_DEBUG, "Config has %i interfaces\n", psConfig->nNumInterfaces );
	
	/* Allocate memory for the interfaces */
	psConfig->psInterface = ( USB_interface_s* )kmalloc( sizeof( USB_interface_s ) * psConfig->nNumInterfaces,
													MEMF_KERNEL | MEMF_NOBLOCK );
	if( !psConfig->psInterface ) {
		kerndbg( KERN_WARNING, "USB: Out of memory!\n" );
		return( -1 );
	}
	
	memset( psConfig->psInterface, 0, sizeof( USB_interface_s ) * psConfig->nNumInterfaces );
	
	pBuffer += psConfig->nLength;
	nSize -= psConfig->nLength;
	
	psConfig->pExtra = NULL;
	psConfig->nExtraLen = 0;
	
	/* Parse interfaces */
	for( i = 0; i < psConfig->nNumInterfaces; i++ )
	{
		int nNumSkipped, nLen;
		uint8* pBegin;
		
		/* Skip vendor specific data */
		pBegin = pBuffer;
		nNumSkipped = 0;
		
		while( nSize >= sizeof( USB_desc_header_s ) )
		{
			psHeader = ( USB_desc_header_s* )pBuffer;
			
			if( ( psHeader->nLength > nSize ) || ( psHeader->nLength < 2 ) ) {
				kerndbg( KERN_WARNING, "USB: Error while parsing config\n" );
				return( -1 );
			}
			/* Look if we have found the next descriptors */
			if( ( psHeader->nDescriptorType == USB_DT_ENDPOINT ) ||
				( psHeader->nDescriptorType == USB_DT_INTERFACE ) ||
				( psHeader->nDescriptorType == USB_DT_CONFIG ) ||
				( psHeader->nDescriptorType == USB_DT_DEVICE ) )
				break;
				
			kerndbg( KERN_DEBUG, "Skipping %i\n", psHeader->nDescriptorType );
			nNumSkipped++; 
			
			pBuffer += psHeader->nLength;
			nSize -= psHeader->nLength;
		}
		if( nNumSkipped )
			kerndbg( KERN_DEBUG, "Skipped %i of vendor specific data\n", nNumSkipped );
		
		/* Save vendor specific data */
		nLen = (int)( pBuffer - pBegin );
		if( nLen ) {
			if( !psConfig->nExtraLen ) {
				psConfig->pExtra = kmalloc( nLen, MEMF_KERNEL | MEMF_NOBLOCK );
				if( !psConfig->pExtra ) {
					kerndbg( KERN_WARNING, "USB: Out of memory!\n" );
					return( -1 );
				}
				memset( psConfig->pExtra, 0, nLen );
				memcpy( psConfig->pExtra, pBegin, nLen );
				psConfig->nExtraLen = nLen;
			}
		}
		
		/* And now parse the interface */
		nResult = usb_parse_interface( psConfig->psInterface + i, pBuffer, nSize );
		if( nResult < 0 )
			return( nResult );
			
		pBuffer += nResult;
		nSize -= nResult;
	}						
	return( nSize );
} 

void usb_disconnect( USB_device_s** psDevice )
{
	USB_device_s* psDev = *psDevice;
	int i;
	
	if( !psDev )
		return;
		
	*psDevice = NULL;
	
	kerndbg( KERN_INFO, "USB: Device %i disconnected\n", psDev->nDeviceNum );
	
	
	if( psDev->psActConfig ) 
	{
		for( i = 0; i < psDev->psActConfig->nNumInterfaces; i++ ) {
			USB_interface_s* psIF = &psDev->psActConfig->psInterface[i];
			USB_driver_s* psDriver = psIF->psDriver;
			
			/* Tell the driver about the disconnect */
			if( psDriver ) {
				LOCK( psDriver->hLock );
				psDriver->RemoveDevice( psDev, psIF->pPrivate );
				UNLOCK( psDriver->hLock );
				
				psIF->psDriver = NULL;
				psIF->pPrivate = NULL;
			}
		}
	}
	
	unregister_device( psDev->nHandle );

	/* Remove libusb node */
	libusb_remove( psDev );

	/* Free all children */
	for( i = 0; i < 16; i++ ) {
		USB_device_s** psChild = psDev->psChildren + i;
		if( *psChild )
			usb_disconnect( psChild );
	}
	if( psDev->nDeviceNum > 0 ) {
		psDev->psBus->bDevMap[psDev->nDeviceNum] = false;
	}
	usb_free_device( psDev );
}


/** 
 * \par Description: Connects one USB device.
 * \par Note: You have to call usb_new_device afterwards.
 * \param psDevice - Pointer to the device.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void usb_connect( USB_device_s* psDevice )
{
	int i;
	psDevice->sDeviceDesc.nMaxPacketSize = 8;
	
	/* Look for a free slot */
	for( i = 0; i < 128; i++ ) 
	{
		if( !psDevice->psBus->bDevMap[i] )
			break; 
	}
	if( i == 128 ) {
		kerndbg( KERN_WARNING, "USB: Too many devices\n" );
		return;
	}
	psDevice->psBus->bDevMap[i] = true;
	psDevice->nDeviceNum = i;
}

int usb_set_address( USB_device_s* psDevice )
{
	kerndbg( KERN_DEBUG, "Sending address setting message ( device %i on bus %i )...\n", psDevice->nDeviceNum, psDevice->psBus->nBusNum );
	return( usb_control_msg( psDevice, usb_snddefctrl( psDevice ), USB_REQ_SET_ADDRESS,
			0, psDevice->nDeviceNum, 0, NULL, 0, 3 * 1000 * 1000 ) );
}

int usb_get_descriptor( USB_device_s* psDevice, uint8 nType, uint8 nIndex, uint8* pBuffer,
							int nSize )
{
	int nRetry = 5;
	int nResult;
	
	memset( pBuffer, 0, nSize );
	while( nRetry-- ) {
		if( ( nResult = usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ), 
			USB_REQ_GET_DESCRIPTOR, USB_DIR_IN, ( nType << 8 ) + nIndex, 0, pBuffer, nSize, 
			3 * 1000 * 1000 ) ) > 0 || nResult == -EPIPE )
			break;
	}
	return( nResult );
}

int usb_get_class_descriptor( USB_device_s* psDevice, int nIFNum, uint8 nType, uint8 nID, uint8*
							pBuffer, int nSize )
{
	return( usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
		USB_REQ_GET_DESCRIPTOR, USB_RECIP_INTERFACE | USB_DIR_IN,
		( nType << 8 ) + nID, nIFNum, pBuffer, nSize, 3 * 1000 * 1000 ) );
}


int usb_get_string( USB_device_s* psDevice, unsigned short nLangID, uint8 nIndex, uint8* pBuffer, 
					int nSize )
{
	return( usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
		USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
		( USB_DT_STRING << 8 ) + nIndex, nLangID, pBuffer, nSize, 3 * 1000 * 1000 ) );
}

int usb_get_device_descriptor( USB_device_s* psDevice )
{
	int nRet = usb_get_descriptor( psDevice, USB_DT_DEVICE, 0, ( uint8* )&psDevice->sDeviceDesc,
				     sizeof( psDevice->sDeviceDesc ) );
	return( nRet );
}

int usb_get_status( USB_device_s* psDevice, int nType, int nTarget, void *pData )
{
	return( usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
		USB_REQ_GET_STATUS, USB_DIR_IN | nType, 0, nTarget, pData, 2, 3 * 1000 * 1000 ) );
}

int usb_get_protocol( USB_device_s* psDevice, int nIFNum )
{
	uint8 nType;
	int nRet;

	if( ( nRet = usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
	    USB_REQ_GET_PROTOCOL, USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
	    0, nIFNum, &nType, 1, 3 * 1000 * 1000 ) ) < 0 )
		return( nRet );

	return( nType );
}

int usb_set_protocol( USB_device_s* psDevice, int nIFNum, int nProtocol )
{
	return( usb_control_msg( psDevice, usb_sndctrlpipe( psDevice, 0 ) ,
		USB_REQ_SET_PROTOCOL, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		nProtocol, nIFNum, NULL, 0, 4 * 1000 * 1000 ) );
}


/** 
 * \par Description: Moves a device into idle state.
 * \param psDevice - Pointer to the device.
 * \param nIFNum - Number of the interface.
 * \param nDuration - Duration time.
 * \param nReportID - ???
 * \return 0 if successful.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int usb_set_idle( USB_device_s* psDevice, int nIFNum, int nDuration, int nReportID )
{
	return( usb_control_msg( psDevice, usb_sndctrlpipe( psDevice, 0 ),
		USB_REQ_SET_IDLE, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		( nDuration << 8 ) | nReportID, nIFNum, NULL, 0, 4 * 1000 * 1000 ) );
}

void usb_set_maxpacket( USB_device_s* psDevice )
{
	int i, b;

	for( i=0; i< psDevice->psActConfig->nNumInterfaces; i++ ) {
		USB_interface_s* psIF = psDevice->psActConfig->psInterface + i;
		USB_desc_interface_s* psIFDesc = psIF->psSetting + psIF->nActSetting;
		USB_desc_endpoint_s* psEndpoint = psIFDesc->psEndpoint;
		int e;

		for( e = 0; e < psIFDesc->nNumEndpoints; e++ ) {
			b = psEndpoint[e].nEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
			if( ( psEndpoint[e].nMAttributes & USB_ENDPOINT_XFERTYPE_MASK ) ==
				USB_ENDPOINT_XFER_CONTROL ) {	/* Control => bidirectional */
				psDevice->nMaxPacketOut[b] = psEndpoint[e].nMaxPacketSize;
				psDevice->nMaxPacketIn[b] = psEndpoint[e].nMaxPacketSize;
			}
			else if( usb_endpoint_out( psEndpoint[e].nEndpointAddress ) ) {
				if( psEndpoint[e].nMaxPacketSize > psDevice->nMaxPacketOut[b] )
					psDevice->nMaxPacketOut[b] = psEndpoint[e].nMaxPacketSize;
			}
			else {
				if( psEndpoint[e].nMaxPacketSize > psDevice->nMaxPacketIn[b] )
					psDevice->nMaxPacketIn[b] = psEndpoint[e].nMaxPacketSize;
			}
		}
	}
}

int usb_clear_halt( USB_device_s* psDevice, int nPipe )
{
	int nResult;
	uint16 nStatus;
	uint8* pBuffer;
	int nEndp = usb_pipeendpoint( nPipe) | ( usb_pipein( nPipe ) << 7 );

	nResult = usb_control_msg( psDevice, usb_sndctrlpipe( psDevice, 0 ),
		USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT, 0, nEndp, NULL, 0, 4 * 1000 * 1000 );

	/* don't clear if failed */
	if( nResult < 0 )
		return( nResult );

	pBuffer = kmalloc( sizeof( nStatus ), MEMF_KERNEL | MEMF_NOBLOCK );
	if( !pBuffer ) {
		kerndbg( KERN_WARNING, "USB: unable to allocate memory for configuration descriptors\n" );
		return( -ENOMEM );
	}
	memset( pBuffer, 0, sizeof( nStatus ) );

	nResult = usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
		USB_REQ_GET_STATUS, USB_DIR_IN | USB_RECIP_ENDPOINT, 0, nEndp,
		pBuffer, sizeof( nStatus ), 4 * 1000 * 1000 );

	memcpy( &nStatus, pBuffer, sizeof( nStatus ) );
	kfree( pBuffer );

	if( nResult < 0 )
		return( nResult );

	if( nStatus & 1 )
		return -EPIPE;		/* still halted */

	usb_endpoint_running( psDevice, usb_pipeendpoint( nPipe ), usb_pipeout( nPipe ) );

	/* toggle is reset on clear */

	usb_settoggle( psDevice, usb_pipeendpoint( nPipe ), usb_pipeout( nPipe), 0 );

	return( 0 );
}


/* for returning string descriptors in UTF-16LE */
int ascii2utf( char *ascii, uint8 *utf, int utfmax )
{
	int retval;

	for (retval = 0; *ascii && utfmax > 1; utfmax -= 2, retval += 2) {
		*utf++ = *ascii++ & 0x7f;
		*utf++ = 0;
	}
	return retval;
}


/** 
 * \par Description: Sets USB root hub string.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
/*
 * root_hub_string is used by each host controller's root hub code,
 * so that they're identified consistently throughout the system.
 */
int usb_root_hub_string( int id, int serial, char *type, uint8 *data, int len )
{
	char buf [30];

	// assert (len > (2 * (sizeof (buf) + 1)));
	// assert (strlen (type) <= 8);

	// language ids
	if (id == 0) {
		*data++ = 4; *data++ = 3;	/* 4 bytes data */
		*data++ = 0; *data++ = 0;	/* some language id */
		return 4;

	// serial number
	} else if (id == 1) {
		sprintf (buf, "%x", serial);

	// product description
	} else if (id == 2) {
		sprintf (buf, "USB %s Root Hub", type);

	// id 3 == vendor description

	// unsupported IDs --> "stall"
	} else
	    return 0;

	data [0] = 2 + ascii2utf (buf, data + 2, len - 2);
	data [1] = 3;
	return data [0];
}


USB_interface_s* usb_ifnum_to_if( USB_device_s*psDevice, unsigned nIFNum )
{
	int i;

	for( i = 0; i < psDevice->psActConfig->nNumInterfaces; i++ )
		if( psDevice->psActConfig->psInterface[i].psSetting[0].nInterfaceNumber == nIFNum )
			return( &psDevice->psActConfig->psInterface[i] );

	return( NULL );
}

int usb_set_interface( USB_device_s* psDevice, int nIFNum, int nAlternate )
{
	USB_interface_s* psIF;
	int nRet;

	psIF = usb_ifnum_to_if( psDevice, nIFNum );
	if( !psIF ) {
		kerndbg( KERN_WARNING, "USB: selecting invalid interface %d\n", nIFNum );
		return( -EINVAL );
	}

	/* 9.4.10 says devices don't need this, if the interface
	   only has one alternate setting */
	if( psIF->nNumSetting == 1 ) {
		kerndbg( KERN_DEBUG, "USB: ignoring set_interface for dev %d, iface %d, alt %d",
			psDevice->nDeviceNum, nIFNum, nAlternate );
		return( 0 );
	}

	if( ( nRet = usb_control_msg( psDevice, usb_sndctrlpipe( psDevice, 0 ),
	    USB_REQ_SET_INTERFACE, USB_RECIP_INTERFACE, nAlternate,
	    nIFNum, NULL, 0, 5 * 1000 * 1000 ) ) < 0 )
		return( nRet );

	psIF->nActSetting = nAlternate;
	psDevice->nToggle[0] = 0;	/* 9.1.1.5 says to do this */
	psDevice->nToggle[1] = 0;
	usb_set_maxpacket( psDevice );
	return( 0 );
}

int usb_set_configuration( USB_device_s* psDevice, int nConfig )
{
	int i, nRet;
	USB_desc_config_s* psConfig = NULL;
	
	for( i=0; i < psDevice->sDeviceDesc.nNumConfigurations; i++ ) {
		if( psDevice->psConfig[i].nConfigValue == nConfig ) {
			psConfig = &psDevice->psConfig[i];
			break;
		}
	}
	if( !psConfig ) {
		kerndbg( KERN_WARNING, "USB: selecting invalid configuration %d\n", nConfig );
		return( -EINVAL );
	}

	if( ( nRet = usb_control_msg( psDevice, usb_sndctrlpipe( psDevice, 0 ),
	    USB_REQ_SET_CONFIGURATION, 0, nConfig, 0, NULL, 0, 4 * 1000 * 1000 ) ) < 0 )
		return( nRet );

	psDevice->psActConfig = psConfig;
	psDevice->nToggle[0] = 0;
	psDevice->nToggle[1] = 0;
	usb_set_maxpacket( psDevice );

	return( 0 );
}

int usb_get_report( USB_device_s* psDevice, int nIFNum, uint8 nType, uint8 nID, uint8 *pBuf, int nSize )
{
	return( usb_control_msg( psDevice, usb_rcvctrlpipe( psDevice, 0 ),
		USB_REQ_GET_REPORT, USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		( nType << 8 ) + nID, nIFNum, pBuf, nSize, 4 * 1000 * 1000 ) );
}

int usb_set_report( USB_device_s* psDevice, int nIFNum, uint8 nType, uint8 nID, uint8* pBuffer, int nSize)
{
	return( usb_control_msg( psDevice, usb_sndctrlpipe( psDevice, 0 ),
		USB_REQ_SET_REPORT, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		( nType << 8 ) + nID, nIFNum, pBuffer, nSize, 1000 * 1000 ) );
}

int usb_get_configuration( USB_device_s* psDevice )
{
	int nResult;
	unsigned int nCfgNo, nLength;
	uint8 *pBuffer;
	uint8 *pBigbuffer;
	USB_desc_config_s* psConfigDesc;
	
	kerndbg( KERN_DEBUG, "Device has %i configurations\n", psDevice->sDeviceDesc.nNumConfigurations );
 	

	if( psDevice->sDeviceDesc.nNumConfigurations < 1 ) {
		kerndbg( KERN_WARNING, "USB: Not enough configurations\n" );
		return( -EINVAL );
	}

	psDevice->psConfig = ( USB_desc_config_s* )
		kmalloc( psDevice->sDeviceDesc.nNumConfigurations *
		sizeof( USB_desc_config_s ), MEMF_KERNEL | MEMF_NOBLOCK );
	if( !psDevice->psConfig ) {
		kerndbg( KERN_WARNING, "USB: Out of memory\n" );
		return( -ENOMEM );	
	}
	memset( psDevice->psConfig, 0, psDevice->sDeviceDesc.nNumConfigurations *
		sizeof( USB_desc_config_s ) );

	psDevice->pRawDescs = ( char ** )kmalloc( sizeof( char* ) *
		psDevice->sDeviceDesc.nNumConfigurations, MEMF_KERNEL | MEMF_NOBLOCK );
	if( !psDevice->pRawDescs ) {
		kerndbg( KERN_WARNING, "USB: Out of memory\n" );
		return( -ENOMEM );
	}
	memset( psDevice->pRawDescs, 0, sizeof( char* ) *
		psDevice->sDeviceDesc.nNumConfigurations );

	pBuffer = kmalloc( 8, MEMF_KERNEL | MEMF_NOBLOCK );
	if( !pBuffer ) {
		kerndbg( KERN_WARNING, "USB: Unable to allocate memory for configuration descriptors\n" );
		return( -ENOMEM );
	}
	memset( pBuffer, 0, 8 );
	psConfigDesc = ( USB_desc_config_s* )pBuffer;

	for( nCfgNo = 0; nCfgNo < psDevice->sDeviceDesc.nNumConfigurations; nCfgNo++ ) {
		/* We grab the first 8 bytes so we know how long the whole */
		/*  configuration is */
		nResult = usb_get_descriptor( psDevice, USB_DT_CONFIG, nCfgNo, pBuffer, 8 );
		if( nResult < 8 ) {
			if( nResult < 0) {
				kerndbg( KERN_WARNING, "USB: unable to get descriptor\n" );
			} else {
				kerndbg( KERN_WARNING, "USB: config descriptor too short (expected %i, got %i)\n", 8, nResult );
				nResult = -EINVAL;
			}
			goto err;
		}

  	  	/* Get the full buffer */
		nLength = psConfigDesc->nTotalLength;

		pBigbuffer = kmalloc( nLength, MEMF_KERNEL | MEMF_NOBLOCK );
		if( !pBigbuffer ) {
			kerndbg( KERN_WARNING, "USB: Unable to allocate memory for configuration descriptors\n" );
			nResult = -ENOMEM;
			goto err;
		}
		memset( pBigbuffer, 0, nLength );

		/* Now that we know the length, get the whole thing */
		nResult = usb_get_descriptor( psDevice, USB_DT_CONFIG, nCfgNo, pBigbuffer, nLength );
		if( nResult < 0 ) {
			kerndbg( KERN_WARNING, "USB: Couldn't get all of config descriptors\n" );
			kfree( pBigbuffer );
			goto err;
		}	
	
		if( nResult < nLength ) {
			kerndbg( KERN_WARNING, "USB: Config descriptor too short (expected %i, got %i)\n", nLength, nResult );
			nResult = -EINVAL;
			kfree( pBigbuffer );
			goto err;
		}

		psDevice->pRawDescs[nCfgNo] = pBigbuffer;

		nResult = usb_parse_config( &psDevice->psConfig[nCfgNo], pBigbuffer );
		if( nResult > 0 ) {
			kerndbg( KERN_WARNING, "USB: Descriptor data left\n" );
		} else if( nResult < 0 ) {
			nResult = -EINVAL;
			goto err;
		}
	}

	kfree( pBuffer );
	return( 0 );
err:
	kfree( pBuffer );
	psDevice->sDeviceDesc.nNumConfigurations = nCfgNo;
	return( nResult );
}


/** 
 * \par Description: Reads one string.
 * \param psDevice - Pointer to the device.
 * \param nIndex - Index of the string.
 * \param pBuf - Data buffer.
 * \param nSize - Size of the buffer.
 * \return Index.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int usb_string( USB_device_s* psDevice, int nIndex, char* pBuf, size_t nSize )
{
	uint8 *pTbuf;
	int nErr;
	unsigned int nU, nIdx;

	if( nSize <= 0 || !pBuf || !nIndex )
		return( -EINVAL );
	pBuf[0] = 0;
	pTbuf = kmalloc( 256, MEMF_KERNEL | MEMF_NOBLOCK );
	if( !pTbuf )
		return( -ENOMEM );

	/* get langid for strings if it's not yet known */
	if( !psDevice->bLangID ) {
		nErr = usb_get_string( psDevice, 0, 0, pTbuf, 4 );
		if( nErr < 0 ) {
			kerndbg( KERN_WARNING, "USB: Error getting string descriptor 0 (error=%d)\n", nErr);
			goto errout;
		} else if( pTbuf[0] < 4 ) {
			kerndbg( KERN_WARNING, "USB: String descriptor 0 too short\n" );
			nErr = -EINVAL;
			goto errout;
		} else {
			psDevice->bLangID = true;
			psDevice->nStringLangID = pTbuf[2] | ( pTbuf[3]<< 8 );
				/* always use the first langid listed */
			kerndbg( KERN_DEBUG, "USB device number %d default language ID 0x%x\n",
				psDevice->nDeviceNum, psDevice->nStringLangID );
		}
	}

	/*
	 * Just ask for a maximum length string and then take the length
	 * that was returned.
	 */
	nErr = usb_get_string( psDevice, psDevice->nStringLangID, nIndex, pTbuf, 255 );
	if( nErr < 0 )
		goto errout;

	nSize--;		/* leave room for trailing NULL char in output buffer */
	for( nIdx = 0, nU = 2; nU < nErr; nU += 2 ) {
		if( nIdx >= nSize )
			break;
		if ( pTbuf[nU+1] )			/* high byte */
			pBuf[nIdx++] = '?';  /* non-ASCII character */
		else
			pBuf[nIdx++] = pTbuf[nU];
	}
	pBuf[nIdx] = 0;
	nErr = nIdx;

 errout:
	kfree( pTbuf );
	return( nErr );
}

void usb_show_string( USB_device_s* psDevice, char *pID, int nIndex )
{
	char *pBuf;

	if( !nIndex )
		return;
	if( !( pBuf = kmalloc( 256, MEMF_KERNEL | MEMF_NOBLOCK ) ) )
		return;
	if( usb_string( psDevice, nIndex, pBuf, 256 ) > 0 )
		kerndbg( KERN_INFO, "%s: %s\n", pID, pBuf );
	kfree( pBuf );
}


/** 
 * \par Description: Adds a new device to the system after calling usb_connect()
 * \param psDevice - Pointer to the device.
 * \return 0 if successful.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int usb_new_device( USB_device_s* psDevice )
{
	int nErr;
	char zTemp[255];

	/* USB v1.1 5.5.3 */
	/* We read the first 8 bytes from the device descriptor to get to */
	/*  the nMaxPacketSize field. Then we set the maximum packet size */
	/*  for the control pipe, and retrieve the rest */
	psDevice->nMaxPacketIn[0] = 8;
	psDevice->nMaxPacketOut[0] = 8;
	
	kerndbg( KERN_DEBUG, "USB: Setting address...\n" );

	nErr = usb_set_address( psDevice );
	if( nErr < 0 ) {
		kerndbg( KERN_WARNING, "USB: Device not accepting new address=%d (error=%d)\n",
			psDevice->nDeviceNum, nErr );
		psDevice->psBus->bDevMap[psDevice->nDeviceNum] = false;
		psDevice->nDeviceNum = -1;
		return( 1 );
	}

	snooze( 10 * 1000 );
	
	kerndbg( KERN_DEBUG, "USB: Getting descriptor...\n" );

	nErr = usb_get_descriptor( psDevice, USB_DT_DEVICE, 0, (uint8*)&psDevice->sDeviceDesc, 8 );
	if( nErr < 8) {
		if( nErr < 0 ) {
			kerndbg( KERN_WARNING, "USB: Device not responding, giving up (error=%d)\n", nErr );
		} else {
			kerndbg( KERN_WARNING, "USB: Device descriptor short read (expected %i, got %i)\n", 8, nErr );
		}
		psDevice->psBus->bDevMap[psDevice->nDeviceNum] = false;
		psDevice->nDeviceNum = -1;
		return( 1 );
	}
	
	psDevice->nMaxPacketIn[0] = psDevice->sDeviceDesc.nMaxPacketSize;
	psDevice->nMaxPacketOut[0] = psDevice->sDeviceDesc.nMaxPacketSize;
	
	kerndbg( KERN_DEBUG, "USB: Getting device descriptor...\n" );

	nErr = usb_get_device_descriptor( psDevice );
	if( nErr < (signed)sizeof( psDevice->sDeviceDesc ) ) {
		if( nErr < 0 ) {
			kerndbg( KERN_WARNING, "USB: Unable to get device descriptor (error=%d)\n", nErr );
		} else {
			kerndbg( KERN_WARNING, "USB: Device descriptor short read (expected %Zi, got %i)\n",
				sizeof( psDevice->sDeviceDesc ), nErr );
		}
		psDevice->psBus->bDevMap[psDevice->nDeviceNum] = false;
		psDevice->nDeviceNum = -1;
		return( 1 );
	}
	
	kerndbg( KERN_DEBUG, "USB: Getting config...\n" );

	nErr = usb_get_configuration( psDevice );
	if( nErr < 0 ) {
		kerndbg( KERN_WARNING, "USB: Unable to get device %d configuration (error=%d)\n",
			psDevice->nDeviceNum, nErr );
		psDevice->psBus->bDevMap[psDevice->nDeviceNum] = false;
		psDevice->nDeviceNum = -1;
		return( 1 );
	}
	
	kerndbg( KERN_DEBUG, "USB: Setting config...\n" );

	/* we set the default configuration here */
	nErr = usb_set_configuration( psDevice, psDevice->psConfig[0].nConfigValue );
	if( nErr ) {
		kerndbg( KERN_WARNING, "USB: Failed to set device %d default configuration (error=%d)\n",
			psDevice->nDeviceNum, nErr );
		psDevice->psBus->bDevMap[psDevice->nDeviceNum] = false;
		psDevice->nDeviceNum = -1;
		return( 1 );
	}
	
	/* Register device in the device manager*/
	strcpy( zTemp, "USB device" );
	usb_string( psDevice, psDevice->sDeviceDesc.nProduct, zTemp, 256 );
	psDevice->nHandle = register_device( zTemp, "usb" );

	kerndbg( KERN_INFO, "USB: New device strings: Mfr=%d, Product=%d, SerialNumber=%d\n",
		psDevice->sDeviceDesc.nManufacturer, psDevice->sDeviceDesc.nProduct, psDevice->sDeviceDesc.nSerialNumber );

	if( psDevice->sDeviceDesc.nManufacturer )
		usb_show_string( psDevice, "Manufacturer", psDevice->sDeviceDesc.nManufacturer );
	if( psDevice->sDeviceDesc.nProduct )
		usb_show_string( psDevice, "Product", psDevice->sDeviceDesc.nProduct );
	if( psDevice->sDeviceDesc.nSerialNumber )
		usb_show_string( psDevice, "SerialNumber", psDevice->sDeviceDesc.nSerialNumber );

	/* Enable all usb device drivers */
	enable_devices_on_bus( USB_BUS_NAME );

	/* find drivers willing to handle this device */
	usb_find_drivers_for_new_device( psDevice );
	
	/* Create libusb device node */
	libusb_add( psDevice );


	
	return( 0 );
}

void usb_hub_init();



USB_bus_s sBus = {
	usb_register_driver,
	usb_register_driver_force,
	usb_deregister_driver,
	usb_connect,
	usb_disconnect,
	usb_new_device,
	usb_alloc_device,
	usb_free_device,
	usb_set_idle,
	usb_string,
	usb_alloc_bus,
	usb_free_bus,
	usb_register_bus,
	usb_deregister_bus,
	usb_root_hub_string,
	usb_check_bandwidth,
	usb_claim_bandwidth,
	usb_release_bandwidth,
	usb_alloc_packet,
	usb_free_packet,
	usb_submit_packet,
	usb_cancel_packet,
	usb_control_msg,
	usb_set_interface,
	usb_set_configuration
};


/** 
 * \par Description: Returns hooks of the usb busmanager
 * \param
 * \return
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void* usb_bus_get_hooks( int nVersion )
{
	if( nVersion != USB_BUS_VERSION )
		return( NULL );
	return( &sBus );
}



/** 
 * \par Description: Initializes the USB busmanager.
 * \param
 * \return
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t device_init( int nDeviceID )
{
	/* Check if the use of the bus is disabled */
	int i;
	int argc;
	const char* const *argv;
	bool bDisableUSB = false;
	get_kernel_arguments( &argc, &argv );
	
	for( i = 0; i < argc; ++i )
	{
		if( get_bool_arg( &bDisableUSB, "disable_usb=", argv[i], strlen( argv[i] ) ) )
			if( bDisableUSB ) {
				kerndbg( KERN_WARNING, "USB bus disabled by user\n" );
				return( -1 );
			}
	}

	
	/* Clear everything */
	for( i = 0; i < 64; i++ ) {
		g_bUSBBusMap[i] = false;
		g_psUSBBus[i] = NULL;
	}
	g_psFirstUSBDriver = NULL;
	
	spinlock_init( &g_hUSBLock, "usb_lock" );
	
	kerndbg( KERN_INFO, "USB: Busmanager initialized\n" );
	
	usb_hub_init();

	g_nDeviceID = nDeviceID;
	register_busmanager( g_nDeviceID, "usb", usb_bus_get_hooks );
	
	return( 0 );
}

status_t device_uninit( int nDeviceID )
{
	// TODO:
}






