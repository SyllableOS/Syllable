/*
 *  The Syllable kernel
 *  Simple SCSI layer
 *  Contains some linux kernel code
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 2006 Kristian Van Der Vliet
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/scsi.h>
#include <atheos/bootmodules.h>
#include <posix/errno.h>
#include <macros.h>

#include <scsi_common.h>

SCSI_device_s *g_psFirstDevice;
static bool g_nIDTable[255];

const char *const nSCSIDeviceTypes[14] = {
	"Direct-Access    ",
	"Sequential-Access",
	"Printer          ",
	"Processor        ",
	"WORM             ",
	"CD-ROM           ",
	"Scanner          ",
	"Optical Device   ",
	"Medium Changer   ",
	"Communications   ",
	"Unknown          ",
	"Unknown          ",
	"Unknown          ",
	"Enclosure        ",
};

static void scsi_print_inquiry( unsigned char *data )
{
	int i;
	char buf[256];
	char buf2[256];

	strcpy( buf, "SCSI: Vendor: " );
	for( i = 8; i < 16; i++ )
	{
		if( data[i] >= 0x20 && i < data[4] + 5 )
		{
			sprintf( buf2, "%c", data[i] );
			strcat( buf, buf2 );
		}
		else
			strcat( buf, " " );
	}
	strcat( buf, "\n" );
	printk( buf );

	strcpy( buf, "SCSI: Model: " );
	for( i = 16; i < 32; i++ )
	{
		if( data[i] >= 0x20 && i < data[4] + 5 )
		{
			sprintf( buf2, "%c", data[i] );
			strcat( buf, buf2 );
		}
		else
			strcat( buf, " " );
	}
	strcat( buf, "\n" );
	printk( buf );

	strcpy( buf, "SCSI: Revision: " );
	for( i = 32; i < 36; i++ )
	{
		if( data[i] >= 0x20 && i < data[4] + 5 )
		{
			sprintf( buf2, "%c", data[i] );
			strcat( buf, buf2 );
		}
		else
			strcat( buf, " " );
	}
	strcat( buf, "\n" );
	printk( buf );

	i = data[0] & 0x1f;

	printk( "SCSI: Type: %s\n", i < 14 ? nSCSIDeviceTypes[i] : "Unknown" );
	printk( "SCSI: ANSI SCSI revision: %02x\n", data[2] & 0x07 );
}

int scsi_get_next_id()
{
	int i;

	for( i = 0; i < 255; i++ )
	{
		if( g_nIDTable[i] == false )
		{
			g_nIDTable[i] = true;
			return ( i );
		}
	}
	return ( -1 );
}

SCSI_device_s *scsi_scan_device( SCSI_host_s * psHost, int nChannel, int nDevice, int nLun, int *pnSCSILevel )
{
	unsigned char nSCSIResult[256];
	SCSI_cmd sCmd;
	int nError;
	SCSI_device_s *psDevice = NULL;

	printk( "Scanning device %i:%i:%i...\n", nChannel, nDevice, nLun );

	/* Build SCSI_INQUIRY command */
	memset( &sCmd, 0, sizeof( sCmd ) );

	sCmd.psHost = psHost;
	sCmd.nChannel = nChannel;
	sCmd.nDevice = nDevice;
	sCmd.nLun = nLun;
	sCmd.nDirection = SCSI_DATA_READ;

	sCmd.nCmd[0] = SCSI_INQUIRY;
	if( *pnSCSILevel <= SCSI_2 )
		sCmd.nCmd[1] = ( nLun << 5 ) & 0xe0;
	sCmd.nCmd[4] = 255;
	sCmd.nCmdLen = scsi_get_command_size( SCSI_INQUIRY );

	sCmd.pRequestBuffer = nSCSIResult;
	sCmd.nRequestSize = 256;

	/* Send command */
	nError = psHost->queue_command( &sCmd );

	kerndbg( KERN_DEBUG, "Result: %i\n", sCmd.nResult );

	/* Check result */
	if( nError != 0 || sCmd.nResult != 0 )
	{
		return NULL;
	}

	scsi_print_inquiry( nSCSIResult );

	switch( ( nSCSIResult[0] & 0x1f ) )
	{
		case SCSI_TYPE_DISK:
		{
			kerndbg( KERN_DEBUG, "SCSI_TYPE_DISK\n" );

			psDevice = scsi_add_disk( psHost, nChannel, nDevice, nLun, nSCSIResult );
			break;
		}

		case SCSI_TYPE_ROM:
		{
			kerndbg( KERN_DEBUG, "SCSI_TYPE_ROM\n" );

			psDevice = scsi_add_cdrom( psHost, nChannel, nDevice, nLun, nSCSIResult );
			break;
		}

		default:
		{
			printk( "Unsupported SCSI device (Type 0x%.2x)\n", ( nSCSIResult[0] & 0x1f ) );
			return NULL;
		}
	}

	if( psDevice )
		*pnSCSILevel = psDevice->nSCSILevel;

	return psDevice;
}

void scsi_scan_host( SCSI_host_s * psHost )
{
	int nChannel;
	int nDevice;
	int nLun;
	int nSCSILevel = SCSI_2;

	/* Scan for devices */
	printk( "SCSI: Scanning for devices on host %s\n", psHost->get_name() );

	for( nChannel = 0; nChannel <= psHost->get_max_channel(); nChannel++ )
	{
		for( nDevice = 0; nDevice <= psHost->get_max_device(); nDevice++ )
		{
			nSCSILevel = SCSI_2;
			for( nLun = 0; nLun < 8; nLun++ )
			{
				if( scsi_scan_device( psHost, nChannel, nDevice, nLun, &nSCSILevel ) )
				{
				}
				else
				{
					nLun = 9;
				}
			}
		}
	}
}

void scsi_remove_host( SCSI_host_s * psHost )
{
	/* Look for all devices that belong to this host */
	SCSI_device_s *psDevice = g_psFirstDevice;
	SCSI_device_s *psPrev = g_psFirstDevice;
	SCSI_device_s *psPartition;

restart:
	psDevice = g_psFirstDevice;
	psPrev = g_psFirstDevice;

	while( psDevice )
	{
		if( psDevice->psHost == psHost )
		{
			/* First remove all partitions */
			for( psPartition = psDevice->psFirstPartition; psPartition != NULL; psPartition = psPartition->psNext )
			{
				printk( "SCSI: Removing partition %s\n", psPartition->zName );
				if( atomic_read( &psPartition->nOpenCount ) > 0 )
				{
					printk( "SCSI: Warning: Device still opened\n" );
				}
				delete_device_node( psPartition->nNodeHandle );
			}
			/* Then the raw device */
			printk( "SCSI: Removing device %s\n", psDevice->zName );
			if( atomic_read( &psDevice->nOpenCount ) > 0 )
			{
				printk( "SCSI: Warning: Device still opened\n" );
			}
			delete_device_node( psDevice->nNodeHandle );

			g_nIDTable[psDevice->nID] = false;

			release_device( psDevice->nDeviceHandle );
			unregister_device( psDevice->nDeviceHandle );
			kfree( psDevice->pDataBuffer );
			if( psPrev == psDevice )
			{
				g_psFirstDevice = psDevice->psNext;
			}
			else
			{
				psPrev->psNext = psDevice->psNext;
			}
			kfree( psDevice );
			goto restart;
		}
		psPrev = psDevice;
		psDevice = psDevice->psNext;
	}
}

SCSI_bus_s sBus = {
	scsi_get_command_size,
	scsi_scan_host,
	scsi_remove_host
};

void *scsi_bus_get_hooks( int nVersion )
{
	if( nVersion != SCSI_BUS_VERSION )
		return NULL;
	return ( void * )&sBus;
}

status_t device_init( int nDeviceID )
{
	/* Check if the use of the bus is disabled */
	int i;
	int argc;
	const char *const *argv;
	bool bDisableSCSI = false;

	get_kernel_arguments( &argc, &argv );

	for( i = 0; i < argc; ++i )
	{
		if( get_bool_arg( &bDisableSCSI, "disable_scsi=", argv[i], strlen( argv[i] ) ) )
			if( bDisableSCSI )
			{
				printk( "SCSI bus disabled by user\n" );
				return -1;
			}
	}
	/* Set default values */

	g_psFirstDevice = NULL;
	for( i = 0; i < 255; i++ )
	{
		g_nIDTable[i] = false;
	}

	register_busmanager( nDeviceID, "scsi", scsi_bus_get_hooks );

	printk( "SCSI: Busmanager initialized\n" );

	return 0;
}

status_t device_uninit( int nDeviceID )
{
}

