
/*
 *  The Syllable kernel
 *	nVIDIA graphics kernel driver
 *	Copyright (C) 2003 Arno Klenke
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
 *
 */


#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <appserver/pci_graphics.h>


struct gfx_device
{
	int nVendorID;
	int nDeviceID;
	char *zVendorName;
	char *zDeviceName;
};

struct gfx_node
{
	PCI_Info_s sInfo;
	char zName[255];
};

#define APPSERVER_DRIVER "nvidia"

struct gfx_device g_sDevices[] = {
	{0x12d2, 0x0018, "nVidia/SGS", "Riva 128"},
	{0x10de, 0x0020, "nVidia", "Riva TNT"},
	{0x10de, 0x0028, "nVidia", "Riva TNT2"},
	{0x10de, 0x002a, "nVidia", "Riva TNT2"},
	{0x10de, 0x002c, "nVidia", "Riva Vanta"},
	{0x10de, 0x0029, "nVidia", "Riva TNT2 Ultra"},
	{0x10de, 0x002d, "nVidia", "Riva TNT2 M64"},
	{0x10de, 0x00a0, "nVidia", "Aladdin TNT2"},
	{0x10de, 0x0100, "nVidia", "GeForce 256"},
	{0x10de, 0x0101, "nVidia", "GeForce DDR"},
	{0x10de, 0x0103, "nVidia", "Quadro"},
	{0x10de, 0x0110, "nVidia", "GeForce2 MX/MX 400"},
	{0x10de, 0x0111, "nVidia", "GeForce2 MX 100/200"},
	{0x10de, 0x0112, "nVidia", "GeForce2 Go"},
	{0x10de, 0x0113, "nVidia", "Quadro2 MXR/EX/Go"},
	{0x10de, 0x01a0, "nVidia", "GeForce2 Integrated"},
	{0x10de, 0x0150, "nVidia", "GeForce2 GTS"},
	{0x10de, 0x0151, "nVidia", "GeForce2 Ti"},
	{0x10de, 0x0152, "nVidia", "GeForce2 Ultra"},
	{0x10de, 0x0153, "nVidia", "Quadro2 Pro"},
	{0x10de, 0x0170, "nVidia", "GeForce4 MX460"},
	{0x10de, 0x0171, "nVidia", "GeForce4 MX440"},
	{0x10de, 0x0172, "nVidia", "GeForce4 MX420"},
	{0x10de, 0x0173, "nVidia", "GeForce4 MX440SE"},
	{0x10de, 0x0174, "nVidia", "GeForce4 440 Go"},
	{0x10de, 0x0175, "nVidia", "GeForce4 420 Go"},
	{0x10de, 0x0176, "nVidia", "GeForce4 420 Go 32M"},
	{0x10de, 0x0177, "nVidia", "GeForce4 460 Go"},
	{0x10de, 0x0179, "nVidia", "GeForce4 440 Go 64M"},
	{0x10de, 0x017D, "nVidia", "GeForce4 410 Go 16M"},
	{0x10de, 0x017C, "nVidia", "Quadro4 550 GoGL"},
	{0x10de, 0x0178, "nVidia", "Quadro4 550 XGL"},
	{0x10de, 0x017a, "nVidia", "Quadro4 NVS"},
	{0x10de, 0x0181, "nVidia", "GeForce4 MX 440 AGP8X"},
	{0x10de, 0x0182, "nVidia", "GeForce4 MX 440SE AGP8X"},
	{0x10de, 0x0183, "nVidia", "GeForce4 MX 420 AGP8X"},
	{0x10de, 0x0186, "nVidia", "GeForce4 448 Go"},
	{0x10de, 0x0187, "nVidia", "GeForce4 488 Go"},
	{0x10de, 0x0188, "nVidia", "Quadro4 580 XGL"},
	{0x10de, 0x018a, "nVidia", "Quadro4 280 NVS"},
	{0x10de, 0x018b, "nVidia", "Quadro4 380 XGL"},
	{0x10de, 0x01f0, "nVidia", "GeForce4 MX Integrated"},
	{0x10de, 0x0200, "nVidia", "GeForce3"},
	{0x10de, 0x0201, "nVidia", "GeForce3 Ti 200"},
	{0x10de, 0x0202, "nVidia", "GeForce3 Ti 500"},
	{0x10de, 0x0203, "nVidia", "Quadro DCC"},
	{0x10de, 0x0250, "nVidia", "GeForce4 Ti 4600"},
	{0x10de, 0x0251, "nVidia", "GeForce4 Ti 4400"},
	{0x10de, 0x0252, "nVidia", "NV25"},
	{0x10de, 0x0253, "nVidia", "GeForce4 Ti 4200"},
	{0x10de, 0x0258, "nVidia", "Quadro4 900 XGL"},
	{0x10de, 0x0259, "nVidia", "Quadro4 750 XGL"},
	{0x10de, 0x025b, "nVidia", "Quadro4 700 XGL"},
	{0x10de, 0x0280, "nVidia", "GeForce4 Ti 4800"},
	{0x10de, 0x0281, "nVidia", "GeForce4 Ti 4200 AGP8X"},
	{0x10de, 0x0282, "nVidia", "GeForce4 Ti 4800 SE"},
	{0x10de, 0x0286, "nVidia", "GeForce4 Ti 4200 Go"},
	{0x10de, 0x028C, "nVidia", "Quadro4 700 GoGL"},
	{0x10de, 0x0288, "nVidia", "Quadro4 980 XGL"},
	{0x10de, 0x0289, "nVidia", "Quadro4 780 XGL"},
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t gfx_open( void *pNode, uint32 nFlags, void **pCookie )
{
	struct gfx_node *psNode = pNode;

	printk( "%s opened\n", psNode->zName );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t gfx_close( void *pNode, void *pCookie )
{
	struct gfx_node *psNode = pNode;

	printk( "%s closed\n", psNode->zName );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t gfx_ioctl( void *pNode, void *pCookie, uint32 nCommand, void *pArgs, bool bFromKernel )
{
	struct gfx_node *psNode = pNode;
	int nError = 0;

	switch ( nCommand )
	{
	case IOCTL_GET_APPSERVER_DRIVER:
		memcpy_to_user( pArgs, APPSERVER_DRIVER, strlen( APPSERVER_DRIVER ) );
		break;
	case PCI_GFX_GET_PCI_INFO:
		memcpy_to_user( pArgs, &psNode->sInfo, sizeof( PCI_Info_s ) );
		break;
	case PCI_GFX_READ_PCI_CONFIG:
		{
			struct gfx_pci_config sConfig;
			PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
			memcpy_from_user( &sConfig, pArgs, sizeof( struct gfx_pci_config ) );

			if ( psBus == NULL )
			{
				nError = -ENODEV;
			}
			else
			{
				sConfig.m_nValue = psBus->read_pci_config( sConfig.m_nBus, sConfig.m_nDevice, sConfig.m_nFunction, sConfig.m_nOffset, sConfig.m_nSize );
				memcpy_to_user( pArgs, &sConfig, sizeof( struct gfx_pci_config ) );
			}

		}
		break;
	case PCI_GFX_WRITE_PCI_CONFIG:
		{
			struct gfx_pci_config sConfig;
			PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
			memcpy_from_user( &sConfig, pArgs, sizeof( struct gfx_pci_config ) );

			if ( psBus == NULL )
			{
				nError = -ENODEV;
			}
			else
			{
				int nVal = psBus->write_pci_config( sConfig.m_nBus, sConfig.m_nDevice,
					sConfig.m_nFunction, sConfig.m_nOffset, sConfig.m_nSize, sConfig.m_nValue );

				sConfig.m_nValue = nVal;
				memcpy_to_user( pArgs, &sConfig, sizeof( struct gfx_pci_config ) );
			}
		}
		break;
	default:
		nError = -ENOIOCTLCMD;
	}
	return ( nError );
}


DeviceOperations_s g_sOperations = {
	gfx_open,
	gfx_close,
	gfx_ioctl,
	NULL,
	NULL
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
	PCI_bus_s *psBus;
	int nNumDevices = sizeof( g_sDevices ) / sizeof( struct gfx_device );
	int nDeviceNum;
	PCI_Info_s sInfo;
	int nPCINum;
	char zTemp[255];
	char zNodePath[255];
	struct gfx_node *psNode;
	bool bDevFound = false;

	/* Get PCI busmanager */
	psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if ( psBus == NULL )
		return ( -ENODEV );

	/* Look for the device */
	for ( nPCINum = 0; psBus->get_pci_info( &sInfo, nPCINum ) == 0; ++nPCINum )
	{
		for ( nDeviceNum = 0; nDeviceNum < nNumDevices; ++nDeviceNum )
		{
			/* Compare vendor and device id */
			if ( sInfo.nVendorID == g_sDevices[nDeviceNum].nVendorID && sInfo.nDeviceID == g_sDevices[nDeviceNum].nDeviceID )
			{
				sprintf( zTemp, "%s %s", g_sDevices[nDeviceNum].zVendorName, g_sDevices[nDeviceNum].zDeviceName );
				if ( claim_device( nDeviceID, sInfo.nHandle, zTemp, DEVICE_VIDEO ) != 0 )
				{
					continue;
				}

				printk( "%s found\n", zTemp );

				/* Create private node */
				psNode = kmalloc( sizeof( struct gfx_node ), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );
				if ( psNode == NULL )
				{
					printk( "Error: Out of memory\n" );
					continue;
				}
				memcpy( &psNode->sInfo, &sInfo, sizeof( PCI_Info_s ) );
				strcpy( psNode->zName, zTemp );

				/* Create node path */
				sprintf( zNodePath, "graphics/nvidia_%i_0x%x_0x%x", nPCINum, ( uint )sInfo.nVendorID, ( uint )sInfo.nDeviceID );

				if ( create_device_node( nDeviceID, sInfo.nHandle, zNodePath, &g_sOperations, psNode ) < 0 )
				{
					printk( "Error: Failed to create device node %s\n", zNodePath );
					continue;
				}
				
				bDevFound = true;
			}
		}
	}

	if ( !bDevFound )
	{
		disable_device( nDeviceID );
		return ( -ENODEV );
	}

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
	return ( 0 );
}







