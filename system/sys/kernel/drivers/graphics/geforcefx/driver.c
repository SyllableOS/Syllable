
/*
 *  The Syllable kernel
 *	nVIDIA GeForce FX graphics kernel driver
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

enum {
	FX_GET_DMA_ADDRESS = PCI_GFX_LAST_IOCTL
};

#define APPSERVER_DRIVER "geforcefx"

struct gfx_device g_sDevices[] = {
	{0x10de, 0xffff, "nVidia", "GeForce PCI express" },
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
	{0x10de, 0x0185, "nVidia", "GeForce4 MX 4000"},
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
	{0x10de, 0x0300, "nVidia", "NV30"},
	{0x10de, 0x0301, "nVidia", "GeForce FX 5800 Ultra"},
	{0x10de, 0x0302, "nVidia", "GeForce FX 5800"},
	{0x10de, 0x0308, "nVidia", "Quadro FX 2000"},
	{0x10de, 0x0309, "nVidia", "Quadro FX 1000"},
	{0x10de, 0x0311, "nVidia", "GeForce FX 5600 Ultra"},
	{0x10de, 0x0312, "nVidia", "GeForce FX 5600"},
	{0x10de, 0x0314, "nVidia", "GeForce FX 5600SE"},
	{0x10de, 0x031a, "nVidia", "GeForce FX Go5600"},
	{0x10de, 0x031b, "nVidia", "GeForce FX Go5650"},
	{0x10de, 0x031c, "nVidia", "Quadro FX Go700"},
	{0x10de, 0x0320, "nVidia", "GeForce FX 5200"},
	{0x10de, 0x0321, "nVidia", "GeForce FX 5200 Ultra"},
	{0x10de, 0x0322, "nVidia", "GeForce FX 5200"},
	{0x10de, 0x0323, "nVidia", "GeForce FX 5200SE"},
	{0x10de, 0x0324, "nVidia", "GeForce FX Go5200"},
	{0x10de, 0x0325, "nVidia", "GeForce FX Go5250"},
	{0x10de, 0x0326, "nVidia", "GeForce FX 5500"},
	{0x10de, 0x0327, "nVidia", "GeForce FX 5100"},
	{0x10de, 0x0328, "nVidia", "GeForce FX Go5200 32M/64M"},
	{0x10de, 0x032a, "nVidia", "Quadro NVS 280 PCI"},
	{0x10de, 0x032b, "nVidia", "Quadro FX 500/600 PCI"},
	{0x10de, 0x032c, "nVidia", "Quadro FX Go53xx Series"},
	{0x10de, 0x032d, "nVidia", "Quadro FX Go5100"},
	{0x10de, 0x0330, "nVidia", "GeForce FX 5900 Ultra"},
	{0x10de, 0x0331, "nVidia", "GeForce FX 5900"},
	{0x10de, 0x0332, "nVidia", "GeForce FX 5900"},
	{0x10de, 0x0333, "nVidia", "GeForce FX 5950 Ultra"},
	{0x10de, 0x033F, "nVidia", "Quadro FX 700"},
	{0x10de, 0x0334, "nVidia", "GeForce FX 5950ZT"},
	{0x10de, 0x0338, "nVidia", "Quadro FX 3000"},
	{0x10de, 0x0341, "nVidia", "GeForce FX 5700 Ultra"},
	{0x10de, 0x0342, "nVidia", "GeForce FX 5700"},
	{0x10de, 0x0343, "nVidia", "GeForce FX 5700LE"},
	{0x10de, 0x0344, "nVidia", "GeForce FX 5700VE"},
	{0x10de, 0x0347, "nVidia", "GeForce FX Go5700"},
	{0x10de, 0x0348, "nVidia", "GeForce FX Go5700"},
	{0x10de, 0x034C, "nVidia", "Quadro FX Go1000"},
	{0x10de, 0x034E, "nVidia", "Quadro FX 1100"},
	{0x10de, 0x0040, "nVidia", "GeForce FX 6800 Ultra"},
	{0x10de, 0x0041, "nVidia", "GeForce FX 6800"},
	{0x10de, 0x0042, "nVidia", "GeForce FX 6800 LE"},
	{0x10de, 0x0043, "nVidia", "GeForce FX 6800 XE"},
	{0x10de, 0x0045, "nVidia", "GeForce FX 6800 GT"},
	{0x10de, 0x0046, "nVidia", "GeForce FX 6800 GT"},
	{0x10de, 0x0047, "nVidia", "GeForce FX 6800 GS"},
	{0x10de, 0x004E, "nVidia", "Quadro FX 4000"},
	{0x10de, 0x00c0, "nVidia", "GeForce FX 6800 GS" },
	{0x10de, 0x00c1, "nVidia", "GeForce FX 6800" },
	{0x10de, 0x00c2, "nVidia", "GeForce FX 6800 LE" },
	{0x10de, 0x00c3, "nVidia", "GeForce FX 6800 XT" },
	{0x10de, 0x00c8, "nVidia", "GeForce FX Go6800" },
	{0x10de, 0x00c9, "nVidia", "GeForce FX Go6800 Ultra" },
	{0x10de, 0x00cc, "nVidia", "Quadro FX Go1400" },
	{0x10de, 0x00cd, "nVidia", "Quadro FX 3450/4000 SDI" },
	{0x10de, 0x00ce, "nVidia", "Quadro FX Go1400" },
	{0x10de, 0x0140, "nVidia", "GeForce FX 6600 GT"},
	{0x10de, 0x0141, "nVidia", "GeForce FX 6600"},
	{0x10de, 0x0142, "nVidia", "GeForce FX 6600 LE"},
	{0x10de, 0x0143, "nVidia", "GeForce FX 6600 VE"},
	{0x10de, 0x0144, "nVidia", "GeForce FX Go6600"},
	{0x10de, 0x0145, "nVidia", "GeForce FX 6610 XL"},
	{0x10de, 0x0146, "nVidia", "GeForce FX 6600 TE/ 6200 TE"},
	{0x10de, 0x0147, "nVidia", "GeForce FX 6700 XL"},
	{0x10de, 0x0148, "nVidia", "GeForce FX Go6600"},
	{0x10de, 0x0149, "nVidia", "GeForce FX Go6600 GT"},
	{0x10de, 0x014E, "nVidia", "Quadro FX 540"},
	{0x10de, 0x014f, "nVidia", "GeForce FX 6200"},
	{0x10de, 0x0160, "nVidia", "GeForce FX 6500"},
	{0x10de, 0x0161, "nVidia", "GeForce FX 6200 TurboCache(TM)"},
	{0x10de, 0x0162, "nVidia", "GeForce FX 6200 SE TurboCache(TM)"},
	{0x10de, 0x0163, "nVidia", "GeForce FX 6200 LE"},
	{0x10de, 0x0164, "nVidia", "GeForce FX Go6200"},
	{0x10de, 0x0165, "nVidia", "Quadro NVS 285"},
	{0x10de, 0x0166, "nVidia", "GeForce FX Go6400"},
	{0x10de, 0x0167, "nVidia", "GeForce FX Go6200"},
	{0x10de, 0x0168, "nVidia", "GeForce FX Go6400"},
	{0x10de, 0x0169, "nVidia", "GeForce FX 6250"},
	{0x10de, 0x0211, "nVidia", "GeForce FX 6800"},
	{0x10de, 0x0212, "nVidia", "GeForce FX 6800 LE"},
	{0x10de, 0x0215, "nVidia", "GeForce FX 6800 GT"},
	{0x10de, 0x0218, "nVidia", "GeForce FX 6800 XT"},
	{0x10de, 0x0221, "nVidia", "GeForce FX 6200"},
	{0x10de, 0x0090, "nVidia", "GeForce FX 7800 GTX" },
	{0x10de, 0x0091, "nVidia", "GeForce FX 7800 GTX" },
	{0x10de, 0x0092, "nVidia", "GeForce FX 7800 GT" },
	{0x10de, 0x0093, "nVidia", "GeForce FX 7800 GS" },
	{0x10de, 0x0095, "nVidia", "GeForce FX 7800 SLI" },
	{0x10de, 0x0098, "nVidia", "GeForce FX Go7800" },
	{0x10de, 0x0099, "nVidia", "GeForce FX Go7800 GTX" },
	{0x10de, 0x009D, "nVidia", "Quadro FX 4500" },
	{0x10de, 0x01d1, "nVidia", "GeForce FX 7300 LE" },
	{0x10de, 0x01d6, "nVidia", "GeForce Go 7200" },
	{0x10de, 0x01d7, "nVidia", "GeForce Go 7300" },
	{0x10de, 0x01d8, "nVidia", "GeForce Go 7400" },
	{0x10de, 0x01da, "nVidia", "Quadro NVS 110M" },
	{0x10de, 0x01db, "nVidia", "Quadro NVS 120M" },
	{0x10de, 0x01dc, "nVidia", "Quadro FX 350M" },
	{0x10de, 0x01de, "nVidia", "Quadro FX 350" },
	{0x10de, 0x01df, "nVidia", "GeForce 7300 GS" },
	{0x10de, 0x0398, "nVidia", "GeForce Go 7600" },
	{0x10de, 0x0399, "nVidia", "GeForce Go 7600 GT"},
	{0x10de, 0x039A, "nVidia", "Quadro NVS 300M" },
	{0x10de, 0x039C, "nVidia", "Quadro FX 550M" },

	{0x10de, 0x0298, "nVidia", "GeForce Go 7900 GS" },
	{0x10de, 0x0299, "nVidia", "GeForce Go 7900 GTX" },
	{0x10de, 0x029A, "nVidia", "Quadro FX 2500M" },
	{0x10de, 0x029B, "nVidia", "Quadro FX 1500M" },

	{0x10de, 0x0240, "nVidia", "GeForce 6150" },
	{0x10de, 0x0241, "nVidia", "GeForce 6150 LE" },
	{0x10de, 0x0242, "nVidia", "GeForce 6100" },
	{0x10de, 0x0244, "nVidia", "GeForce Go 6150" },
	{0x10de, 0x0247, "nVidia", "GeForce Go 6100" },
	{0x10de, 0x0247, "nVidia", "GeForce Go 6100" },
	{0x10de, 0x03d1, "nVidia", "GeForce 6100" },
	
	{ 0x10de, 0x016A, "nVidia", "GeForce 7100 GS" },
	{ 0x10de, 0x01D3, "nVidia", "GeForce 7300 SE/7200 GS" },
	
	{ 0x10DE, 0x0191, "nVidia", "GeForce 8800 GTX" },
	{ 0x10DE, 0x0193, "nVidia", "GeForce 8800 GTS" },
	{ 0x10DE, 0x0194, "nVidia", "GeForce 8800 Ultra" },
	{ 0x10DE, 0x019D, "nVidia", "Quadro FX 5600" },
	{ 0x10DE, 0x019E, "nVidia", "Quadro FX 4600" },
	{ 0x10DE, 0x0400, "nVidia", "GeForce 8600 GTS" },
	{ 0x10DE, 0x0402, "nVidia", "GeForce 8600 GT" },
	{ 0x10DE, 0x0404, "nVidia", "GeForce 8400 GS" },
	{ 0x10DE, 0x0407, "nVidia", "GeForce 8600M GT" },
	{ 0x10DE, 0x0409, "nVidia", "GeForce 8700M GT" },
	{ 0x10DE, 0x040A, "nVidia", "Quadro FX 370" },
	{ 0x10DE, 0x040B, "nVidia", "Quadro NVS 320M" },
	{ 0x10DE, 0x040C, "nVidia", "Quadro FX 570M" },
	{ 0x10DE, 0x040D, "nVidia", "Quadro FX 1600M" },
	{ 0x10DE, 0x040E, "nVidia", "Quadro FX 570" },
	{ 0x10DE, 0x040F, "nVidia", "Quadro FX 1700" },
	{ 0x10DE, 0x0421, "nVidia", "GeForce 8500 GT" },
	{ 0x10DE, 0x0422, "nVidia", "GeForce 8400 GS" },
	{ 0x10DE, 0x0423, "nVidia", "GeForce 8300 GS" },
	{ 0x10DE, 0x0425, "nVidia", "GeForce 8600M GS" },
	{ 0x10DE, 0x0426, "nVidia", "GeForce 8400M GT" },
	{ 0x10DE, 0x0427, "nVidia", "GeForce 8400M GS" },
	{ 0x10DE, 0x0428, "nVidia", "GeForce 8400M G" },
	{ 0x10DE, 0x0429, "nVidia", "Quadro NVS 140M" },
	{ 0x10DE, 0x042A, "nVidia", "Quadro NVS 130M" },
	{ 0x10DE, 0x042B, "nVidia", "Quadro NVS 135M" },
	{ 0x10DE, 0x042D, "nVidia", "Quadro FX 360M" },
	{ 0x10DE, 0x042F, "nVidia", "Quadro NVS 290" },
	{ 0x10DE, 0x042B, "nVidia", "Quadro NVS 135M" }, 
	{ 0x10DE, 0x042D, "nVidia", "Quadro FX 360M" }, 
	{ 0x10DE, 0x042F, "nVidia", "Quadro NVS 290" }, 
	{ 0x10DE, 0x0602, "nVidia", "GeForce 8800 GT" }, 
	{ 0x10DE, 0x0606, "nVidia", "GeForce 8800 GS" }, 
	{ 0x10DE, 0x060D, "nVidia", "GeForce 8800 GS" }, 
	{ 0x10DE, 0x0611, "nVidia", "GeForce 8800 GT" }, 
	{ 0x10DE, 0x061A, "nVidia", "Quadro FX 3700" },  
	{ 0x10DE, 0x06E4, "nVidia", "GeForce 8400 GS" }, 
	{ 0x10DE, 0x0622, "nVidia", "GeForce 9600 GT" }
	
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
	case 100:
		{
			area_id hArea;
			uintptr_t nAddress;
			
			memcpy_from_user( &hArea, pArgs, sizeof( hArea ) );
			if( get_area_physical_address( hArea, &nAddress ) != 0 )
				nError = -EINVAL;
				
			memcpy_to_user( pArgs, &nAddress, sizeof( nAddress ) );
			
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
			if ( sInfo.nVendorID == g_sDevices[nDeviceNum].nVendorID && 
			( sInfo.nDeviceID == g_sDevices[nDeviceNum].nDeviceID )
			|| ( ( sInfo.nDeviceID & 0xFFF0 ) == 0x00F0 ) || ( ( sInfo.nDeviceID & 0xFFF0 ) == 0x02E0 ) )
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
				sprintf( zNodePath, "graphics/geforcefx_%i_0x%x_0x%x", nPCINum, ( uint )sInfo.nVendorID, ( uint )sInfo.nDeviceID );

				if ( create_device_node( nDeviceID, sInfo.nHandle, zNodePath, &g_sOperations, psNode ) < 0 )
				{
					printk( "Error: Failed to create device node %s\n", zNodePath );
					continue;
				}
				
				nDeviceNum = nNumDevices;
				
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









