
/*
 *  The Syllable kernel
 *	Busmanager & Device handling
 *	Copyright (C) 2003 The Syllable Team
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
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
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/kdebug.h>
#include <atheos/image.h>
#include <atheos/fs_attribs.h>
#include <macros.h>


#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/global.h"
#include "inc/areas.h"
#include "inc/image.h"
#include "inc/mman.h"
#include "inc/ksyms.h"
#include "inc/smp.h"


/* Function definitions */
typedef status_t bus_init_t( void );
typedef void bus_uninit_t( void );
typedef void *bus_get_hooks_t( int nVersion );

/* One busmanager */

struct BusHandle_t;
struct BusHandle_t
{
	struct BusHandle_t *b_psNext;
	int b_nDeviceID;
	char b_zName[MAX_BUSMANAGER_NAME_LENGTH];
	bus_get_hooks_t *b_pGetHooks;
};

typedef struct BusHandle_t BusHandle_s;


/* One device */

struct DeviceHandle_t;
struct DeviceHandle_t
{
	struct DeviceHandle_t *d_psNext;
	int d_nHandle;
	int d_nDeviceID;
	char d_zOriginalName[MAX_DEVICE_NAME_LENGTH];
	char d_zName[MAX_DEVICE_NAME_LENGTH];
	char d_zBus[MAX_BUSMANAGER_NAME_LENGTH];
	bool d_bClaimed;
	enum device_type d_eType;
	void* d_pData;
};

typedef struct DeviceHandle_t DeviceHandle_s;


/* Global stuff */
extern ProcessorInfo_s g_asProcessorDescs[MAX_CPU_COUNT];

BusHandle_s *g_psFirstBus;
DeviceHandle_s *g_psFirstDevice;
uint32 g_nDriverHandle = 0;
sem_id g_hDeviceListLock;

//****************************************************************************/
/** Registers a device and returns a handle to it.
 * \ingroup DriverAPI
 * \param pzName the name of the device; will be overwritten after calling
 *     claim_device().
 * \param pzBus the name of the bus the device is attached to.
 * \return a handle to the device.
 * \sa claim_device(), unregister_device()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int register_device( const char *pzName, const char *pzBus )
{
	/* Create new handle */
	DeviceHandle_s *psDevice = ( DeviceHandle_s * ) kmalloc( sizeof( DeviceHandle_s ), MEMF_KERNEL );

	memset( psDevice, 0, sizeof( DeviceHandle_s ) );

	psDevice->d_nHandle = g_nDriverHandle++;
	if ( pzName )
		strcpy( psDevice->d_zName, pzName );
	else
		strcpy( psDevice->d_zName, "Unknown device" );
	strcpy( psDevice->d_zOriginalName, psDevice->d_zName );
	if ( pzBus )
		strcpy( psDevice->d_zBus, pzBus );
	else
		strcpy( psDevice->d_zBus, "Unknown bus" );
	psDevice->d_nDeviceID = -1;
	psDevice->d_bClaimed = false;
	psDevice->d_eType = DEVICE_UNKNOWN;

	LOCK( g_hDeviceListLock );

	psDevice->d_psNext = g_psFirstDevice;
	g_psFirstDevice = psDevice;

	//printk( "New device %s (Bus: %s)\n", psDevice->d_zName, psDevice->d_zBus );
	UNLOCK( g_hDeviceListLock );

	return ( psDevice->d_nHandle );
}

//****************************************************************************/
/** Unregisters a previously registered device.
 * \ingroup DriverAPI
 * \par Note:
 * An error message will be written to the kernel log if the device is still claimed.
 * \param nHandle the handle returned by the register_device() call.
 * \sa register_device(), release_device()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void unregister_device( int nHandle )
{
	DeviceHandle_s *psPrev = NULL;
	DeviceHandle_s *psDevice = g_psFirstDevice;
	
	LOCK( g_hDeviceListLock );

	while ( psDevice != NULL )
	{
		if ( psDevice->d_nHandle == nHandle )
		{
			if ( psDevice->d_bClaimed )
				printk( "Warning: trying to unregister claimed device %s\n", psDevice->d_zName );
			if ( psPrev )
				psPrev->d_psNext = psDevice->d_psNext;
			else
				g_psFirstDevice = psDevice->d_psNext;

			//printk( "Device %s removed\n", psDevice->d_zName );
			kfree( psDevice );
			UNLOCK( g_hDeviceListLock );
			return;
		}
		psPrev = psDevice;
		psDevice = psDevice->d_psNext;
	}
	UNLOCK( g_hDeviceListLock );
	printk( "Error: unregister_device() called with invalid device\n" );
}

//****************************************************************************/
/** Claims a registered device.
 * \ingroup DriverAPI
 * \param nDeviceID the device id of the driver.
 * \param nHandle the handle returned by the register_device() call.
 * \param pzName the new name of the device.
 * \param eType the type of the device.
 * \return <code>0</code> if successful or another value if the device is already
 *     claimed.
 * \sa register_device(), release_device()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t claim_device( int nDeviceID, int nHandle, const char *pzName, enum device_type eType )
{
	DeviceHandle_s *psDevice = g_psFirstDevice;
	
	LOCK( g_hDeviceListLock );

	while ( psDevice != NULL )
	{
		if ( psDevice->d_nHandle == nHandle )
		{
			if ( psDevice->d_bClaimed )
			{
				printk( "Error: device %s already claimed\n", psDevice->d_zName );
				UNLOCK( g_hDeviceListLock );
				return ( -1 );
			}

			/* Overwrite name and type */
			//printk( "New Device %s (Bus: %s)\n", pzName, psDevice->d_zBus );

			if ( pzName )
				strcpy( psDevice->d_zName, pzName );
			psDevice->d_nDeviceID = nDeviceID;
			psDevice->d_eType = eType;
			psDevice->d_bClaimed = true;
			UNLOCK( g_hDeviceListLock );
			return ( 0 );
		}
		psDevice = psDevice->d_psNext;
	}
	UNLOCK( g_hDeviceListLock );
	printk( "Error: claim_device() called with invalid device\n" );
	return ( -1 );
}

//****************************************************************************/
/** Releases a device.
 * \ingroup DriverAPI
 * \param nHandle the handle returned by the register_device() call.
 * \sa claim_device(), unregister_device()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void release_device( int nHandle )
{
	DeviceHandle_s *psDevice = g_psFirstDevice;
	
	LOCK( g_hDeviceListLock );

	while ( psDevice != NULL )
	{
		if ( psDevice->d_nHandle == nHandle )
		{
			if ( !psDevice->d_bClaimed )
			{
				printk( "Error: device %s not claimed, but release_device() called\n", psDevice->d_zName );
				UNLOCK( g_hDeviceListLock );
				return;
			}

			/* Overwrite name and type */
			//printk( "Device %s removed\n", psDevice->d_zName );
			psDevice->d_nDeviceID = -1;
			psDevice->d_bClaimed = false;
			strcpy( psDevice->d_zName, psDevice->d_zOriginalName );
			UNLOCK( g_hDeviceListLock );
			return;
		}
		psDevice = psDevice->d_psNext;
	}
	UNLOCK( g_hDeviceListLock );
	printk( "Error: release_device() called with invalid device\n" );
	return;
}


//****************************************************************************/
/** Release all devices of one driver.
 * \param nDeviceID - The device id of the driver.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void release_devices( int nDeviceID )
{
	DeviceHandle_s *psDevice = g_psFirstDevice;
	
	extern void do_release_device( int nDeviceID, int nDeviceHandle, void* pPrivateData );
	
	LOCK( g_hDeviceListLock );

	while ( psDevice != NULL )
	{
		DeviceHandle_s *psNext = psDevice->d_psNext;
		if ( psDevice->d_nDeviceID == nDeviceID )
		{
			UNLOCK( g_hDeviceListLock ); // The driver will probably call release_device() or unregister_device()
			
			/* Tell the devfs to call device_release() */
			do_release_device( nDeviceID, psDevice->d_nHandle, psDevice->d_pData );
		
			LOCK( g_hDeviceListLock );
		}
		psDevice = psNext;
	}
	UNLOCK( g_hDeviceListLock );
	return;
}

//****************************************************************************/
/** Suspend all devices.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t suspend_devices()
{
	DeviceHandle_s *psDevice = g_psFirstDevice;
	int nError = 0;
	
	extern status_t do_suspend_device( int nDeviceID, int nDeviceHandle, void* pPrivateData );
	
	LOCK( g_hDeviceListLock );

	while ( psDevice != NULL )
	{
		DeviceHandle_s *psNext = psDevice->d_psNext;
		if ( psDevice->d_nDeviceID != -1 )
			nError = do_suspend_device( psDevice->d_nDeviceID, psDevice->d_nHandle, psDevice->d_pData );
		if( nError != 0 )
		{
			UNLOCK( g_hDeviceListLock );
			return( nError );
		}
		psDevice = psNext;
	}
	UNLOCK( g_hDeviceListLock );
	return( 0 );
}

//****************************************************************************/
/** Resume all devices.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t resume_devices()
{
	DeviceHandle_s *psDevice = g_psFirstDevice;
	int nError = 0;
	
	extern status_t do_resume_device( int nDeviceID, int nDeviceHandle, void* pPrivateData );
	
	LOCK( g_hDeviceListLock );

	while ( psDevice != NULL )
	{
		DeviceHandle_s *psNext = psDevice->d_psNext;
		if ( psDevice->d_nDeviceID != -1 )
			nError = do_resume_device( psDevice->d_nDeviceID, psDevice->d_nHandle, psDevice->d_pData );
		if( nError != 0 )
		{
			UNLOCK( g_hDeviceListLock );
			return( nError );
		}
		psDevice = psNext;
	}
	UNLOCK( g_hDeviceListLock );
	return( 0 );
}


//****************************************************************************/
/** Returns a <code>DeviceInfo_s</code> structure for a registered device.
 * \ingroup DriverAPI
 * \param psInfo a pointer to the <code>DeviceInfo_s</code> structure to fill.
 * \param nIndex the index of the device.
 * \return <code>0</code> if successful.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t get_device_info( DeviceInfo_s * psInfo, int nIndex )
{
	DeviceHandle_s *psDevice = g_psFirstDevice;
	int nCount = 0;

	if ( nIndex < 0 )
		return ( -1 );
		
	LOCK( g_hDeviceListLock );

	while ( psDevice != NULL )
	{
		if ( nCount == nIndex )
		{
			psInfo->di_nHandle = psDevice->d_nHandle;
			strcpy( psInfo->di_zOriginalName, psDevice->d_zOriginalName );
			strcpy( psInfo->di_zName, psDevice->d_zName );
			strcpy( psInfo->di_zBus, psDevice->d_zBus );
			psInfo->di_eType = psDevice->d_eType;
			UNLOCK( g_hDeviceListLock );
			return ( 0 );
		}
		nCount++;
		psDevice = psDevice->d_psNext;
	}
	UNLOCK( g_hDeviceListLock );
	return ( -1 );
}

//****************************************************************************/
/** Sets private device driver data of one device.
 * \ingroup DriverAPI
 * \param nHandle the handle returned by the register_device() call.
 * \param pData the data.
 * \sa get_device_data()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void set_device_data( int nHandle, void* pData )
{
	DeviceHandle_s *psDevice = g_psFirstDevice;
	
	LOCK( g_hDeviceListLock );

	while ( psDevice != NULL )
	{
		if ( psDevice->d_nHandle == nHandle )
		{
			if ( !psDevice->d_bClaimed )
			{
				printk( "Error: device %s not claimed, but set_device_data() called\n", psDevice->d_zName );
				UNLOCK( g_hDeviceListLock );
				return;
			}
			
			psDevice->d_pData = pData;
			UNLOCK( g_hDeviceListLock );
			return;
		}
		psDevice = psDevice->d_psNext;
	}
	UNLOCK( g_hDeviceListLock );
	printk( "Error: set_device_data() called with invalid device\n" );
	return;
}

//****************************************************************************/
/** Returns the private device driver data of one device.
 * \ingroup DriverAPI
 * \param nHandle the handle of the device.
 * \sa set_device_data()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void* get_device_data( int nHandle )
{
	DeviceHandle_s *psDevice = g_psFirstDevice;
	
	LOCK( g_hDeviceListLock );

	while ( psDevice != NULL )
	{
		if ( psDevice->d_nHandle == nHandle )
		{
			void* pData = psDevice->d_pData;
			UNLOCK( g_hDeviceListLock );
			return( pData );
		}
		psDevice = psDevice->d_psNext;
	}
	UNLOCK( g_hDeviceListLock );
	printk( "Error: set_device_data() called with invalid device\n" );
	return( NULL );
}


//****************************************************************************/
/** Registers a busmanager.
 * \ingroup DriverAPI
 * \param nDeviceID device id of the driver registering this busmanager
 * \param pzName the name of the busmanager.
 * \param pfHooks pointer to the function table of the busmanager
 * \sa get_busmanager()
 * \return 0 if successful.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
 status_t register_busmanager( int nDeviceID, const char* pzName, busmanager_get_hooks* pfHooks )
 {
 	/* Check if this busmanager is already registered */
 	BusHandle_s *psBus = g_psFirstBus;
 	while ( psBus != NULL )
	{
		if ( !strcmp( psBus->b_zName, pzName ) )
			return ( -EEXIST );
		psBus = psBus->b_psNext;
	}
 	
 	/* Create a new entry */
	psBus = ( BusHandle_s * ) kmalloc( sizeof( BusHandle_s ), MEMF_KERNEL );
	memset( psBus, 0, sizeof( BusHandle_s ) );

	strcpy( psBus->b_zName, pzName );
	psBus->b_pGetHooks = pfHooks;
	psBus->b_psNext = g_psFirstBus;
	psBus->b_nDeviceID = nDeviceID;
	g_psFirstBus = psBus;
	
	/* Mark this driver as a busmanager (implementation in vfs/dev.c) */
	set_device_as_busmanager( nDeviceID );
	
	return( 0 );
}

//****************************************************************************/
/** Returns a pointer to a structure containing function pointers for the
 *  the specified busmanager.
 * \ingroup Devices
 * \param pzName the name of the bus.
 * \param nVersion the version of the busmanager.
 * \return a pointer to the busmanager's function pointer structure.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void *get_busmanager( const char *pzName, int nVersion )
{
	BusHandle_s *psBus = g_psFirstBus;

	while ( psBus != NULL )
	{
		if ( !strcmp( psBus->b_zName, pzName ) )
			return ( psBus->b_pGetHooks( nVersion ) );
		psBus = psBus->b_psNext;
	}
	return ( NULL );
}
//****************************************************************************/
/** Set defaults.
 * \internal
 * \ingroup Devices
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void init_devices_boot( void )
{
	int i;

	/* First set everything to the default state */
	g_psFirstBus = NULL;
	g_psFirstDevice = NULL;
	
	g_hDeviceListLock = create_semaphore( "device_list_lock", 1, 0 );

	/* Register CPUs */
	for ( i = 0; i < MAX_CPU_COUNT; i++ )
	{
		char zTemp[255];

		if ( g_asProcessorDescs[i].pi_bIsRunning == true )
		{
			sprintf( zTemp, "%s", g_asProcessorDescs[i].pi_zName );
			claim_device( -1, register_device( "", "system" ), zTemp, DEVICE_PROCESSOR );
		}
	}
}


static int dummy ( const char *pzPath, struct stat *psStat, void *pArg )
{
	return ( 0 );
}

//****************************************************************************/
/** Initialize all devices after the root volume has been mounted.
 * \internal
 * \ingroup Devices
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void init_devices( void )
{
	/* Iterate over /dev/bus to load the busmanager drivers */
	iterate_directory( "/dev/bus", false, dummy, NULL );

	/* Iterate over /dev/hcd to load the hostcontroller drivers */
	iterate_directory( "/dev/hcd", false, dummy, NULL );
}
