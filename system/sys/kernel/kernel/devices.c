
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
typedef status_t bus_init_t();
typedef void bus_uninit_t();
typedef void *bus_get_hooks_t( int nVersion );

/* One busmanager */

struct BusHandle_t;
struct BusHandle_t
{
	struct BusHandle_t *b_psNext;
	int b_nImage;
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
};

typedef struct DeviceHandle_t DeviceHandle_s;


/* Global stuff */
extern ProcessorInfo_s g_asProcessorDescs[MAX_CPU_COUNT];

BusHandle_s *g_psFirstBus;
DeviceHandle_s *g_psFirstDevice;
uint32 g_nDriverHandle = 0;
sem_id g_hDeviceListLock;

/** Register a device.
 * \ingroup DriverAPI
 * \par Description:
 * Registers a device and returns a handle to it.
 * \param pzName - Name of the device, will be overwritten after calling claim_device().
 * \param pzBus  - Name of the bus the device is attached to.
 * \return Handle to the device.
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
	psDevice->d_psNext = g_psFirstDevice;
	
	LOCK( g_hDeviceListLock );

	g_psFirstDevice = psDevice;

	//printk( "New device %s (Bus: %s)\n", psDevice->d_zName, psDevice->d_zBus );
	UNLOCK( g_hDeviceListLock );

	return ( psDevice->d_nHandle );
}

/** Unregister a device.
 * \ingroup DriverAPI
 * \par Description:
 * Unregisters a previously registered device.
 * \par Note:
 * An error message will be written to the kernel log if the device is still claimed.
 * \param nHandle - Handle returned by the register_device() call.
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

/** Claim one device.
 * \ingroup DriverAPI
 * \par Description:
 * Claims a registered device.
 * \param nDeviceID - device id of the driver.
 * \param nHandle - Handle returned by the register_device() call.
 * \param pzName  - New name of the device.
 * \param eType - Type of the device.
 * \return 0 if successful or another value if the device is already claimed.
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

/** Release one device.
 * \ingroup DriverAPI
 * \par Description:
 * Releases a device.
 * \param nHandle - Handle returned by the register_device() call.
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

/** Return device informations.
 * \ingroup DriverAPI
 * \ingroup SysCalls
 * \par Description:
 * Returns some informations about one registered device.
 * \param psInfo - Pointer to a DeviceInfo_s structure, which will be filled
 *                with the information.
 * \param nIndex - Index of the device.
 * \return 0 if successful.
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

/** Get one busmanager.
 * \ingroup DriverAPI
 * \par Description:
 * Returns a pointer to a structure, which will probably contain pointers
 * to the busmanager's functions.
 * \param pzName - Name of the bus.
 * \param nVersion - Version of the busmanager.
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

/** Load one busmanager.
 * \param pzPath - Path of the busmanager.
 * \param pzName - Name of the busmanager.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void load_busmanager( const char *pzPath, char *pzName )
{
	bus_init_t *pInitFunc;
	bus_get_hooks_t *pHookFunc;
	BusHandle_s *psBus;
	int nError;
	int nImage;


	/* Try to load */
	nImage = load_kernel_driver( pzPath );
	if ( nImage < 0 )
	{
		printk( "Error: %s is not a valid busmanager\n", pzPath );
		return;
	}

	/* Get bus_init() symbol */
	nError = get_symbol_address( nImage, "bus_init", -1, ( void ** )&pInitFunc );

	if ( nError < 0 )
	{
		printk( "Error: busmanager %s does not export bus_init()\n", pzPath );
		unload_kernel_driver( nImage );
		return;
	}

	/* Now get hooks */
	nError = get_symbol_address( nImage, "bus_get_hooks", -1, ( void ** )&pHookFunc );

	if ( nError < 0 )
	{
		printk( "Error: busmanager %s does not export bus_get_hooks()\n", pzPath );
		unload_kernel_driver( nImage );
		return;
	}



	/* Create a new entry */
	psBus = ( BusHandle_s * ) kmalloc( sizeof( BusHandle_s ), MEMF_KERNEL );
	memset( psBus, 0, sizeof( BusHandle_s ) );

	strcpy( psBus->b_zName, pzName );
	psBus->b_nImage = nImage;
	psBus->b_pGetHooks = pHookFunc;
	psBus->b_psNext = g_psFirstBus;
	g_psFirstBus = psBus;


	/* Initialize busmanager */

	nError = pInitFunc( false );

	/* Look if we had success */
	if ( nError != 0 )
	{
		printk( "Error: busmanager %s failed to initialize\n", pzPath );
		g_psFirstBus = psBus->b_psNext;
		kfree( psBus );
		unload_kernel_driver( nImage );
		return;
	}
}

/** Load busmanager of one directory.
 * \param pzPath - Path of the directory.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void load_busmanagers( char *pzPath )
{
	struct kernel_dirent sDirEnt;
	int nDir;
	int nPathLen = strlen( pzPath );
	int nError;


	/* Open directory */
	nDir = open( pzPath, O_RDONLY );
	if ( nDir < 0 )
	{
		printk( "Error: load_busmanagers() failed to open dir %s!\n", pzPath );
		return;
	}

	strcat( pzPath, "/" );
	nPathLen++;

	/* Look through the directory */
	while ( getdents( nDir, &sDirEnt, 1 ) == 1 )
	{
		struct stat sStat;

		if ( strcmp( sDirEnt.d_name, "." ) == 0 || strcmp( sDirEnt.d_name, ".." ) == 0 )
		{
			continue;
		}
		strcat( pzPath, sDirEnt.d_name );
		nError = stat( pzPath, &sStat );
		if ( nError < 0 )
		{
			printk( "Error: load_busmanagers() failed to stat %s\n", pzPath );
			continue;
		}
		if ( S_ISDIR( sStat.st_mode ) )
		{
			/* Do recursive calls for directories */
			load_busmanagers( pzPath );
		}
		else
		{
			/* Look if the busmanager is already loaded */
			BusHandle_s *psBus = g_psFirstBus;

			bool bLoaded = false;

			while ( psBus != NULL )
			{
				if ( !strcmp( psBus->b_zName, sDirEnt.d_name ) )
					bLoaded = true;
				psBus = psBus->b_psNext;
			}
			if ( !bLoaded )
				load_busmanager( pzPath, sDirEnt.d_name );
		}
		pzPath[nPathLen] = '\0';	// Remove leaf name
	}

	/* Close directory */
	close( nDir );
}


/** Called by write_kernel_config() ( config.c ) to save the configuration.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void write_devices_config()
{
	int nError;
	BusHandle_s *psBus = g_psFirstBus;
	bus_uninit_t *pUninitFunc;

	/* Go through all busses */
	while ( psBus != NULL )
	{
		/* Get bus_uninit symbol */
		nError = get_symbol_address( psBus->b_nImage, "bus_uninit", -1, ( void ** )&pUninitFunc );

		if ( nError >= 0 )
		{
			/* Call it */
			pUninitFunc();
		}
		psBus = psBus->b_psNext;
	}
}

/** Called by init_boot_modules() ( elf.c ) to add one bootmodule.
 * \param pzPath - Path of the busmanager.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void add_devices_bootmodule( const char *pzPath )
{
	if ( strstr( pzPath, "/drivers/bus/" ) != NULL )
	{
		/* Build name */
		char zName[MAX_BUSMANAGER_NAME_LENGTH];
		int i;

		strcpy( zName, pzPath );

		for ( i = strlen( pzPath ) - 1; i >= 0; i-- )
		{
			if ( pzPath[i] == '/' )
			{
				memcpy( zName, &pzPath[i] + 1, strlen( pzPath ) - i + 1 );
				break;
			}
		}

		/* Load busmanager */
		load_busmanager( pzPath, zName );
	}

}

/** Set defaults.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void init_devices_boot()
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
			sprintf( zTemp, "%s %iMHz", g_asProcessorDescs[i].pi_zName, ( int )( ( g_asProcessorDescs[i].pi_nCoreSpeed + 500000 ) / 1000000 ) );
			claim_device( -1, register_device( "", "system" ), zTemp, DEVICE_PROCESSOR );
		}
	}
}


static int dummy ( const char *pzPath, struct stat *psStat, void *pArg )
{
	return ( 0 );
}

/** Initialize all devices after the root volume has been mounted.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void init_devices()
{
	char zPath[255];	/* /atheos/sys/drivers/kernel */

	/* Load busmanagers, which are not already loaded */
	sys_get_system_path( zPath, 256 );
	strcat( zPath, "sys/drivers/bus" );
	load_busmanagers( zPath );

	/* Iterate over /dev/hcd to load the hostcontroller drivers */
	iterate_directory( "/dev/hcd", false, dummy, NULL );
}
