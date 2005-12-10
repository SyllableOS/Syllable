
/*
 *  The Syllable kernel
 *	ATi Radeon graphics kernel driver
 *	Copyright (C) 2004 Michael Krueger
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

#define APPSERVER_DRIVER "radeon"

struct gfx_device g_sDevices[] = {
	/* Radeon R100 */
	{0x1002, 0x5144, "ATi", "Radeon 7200 (R100 QD)"},
	{0x1002, 0x5145, "ATi", "Radeon 7200 (R100 QE)"},
	{0x1002, 0x5146, "ATi", "Radeon 7200 (R100 QF)"},
	{0x1002, 0x5147, "ATi", "Radeon 7200 (R100 QG)"},
	/* Radeon RS100 */
	{0x1002, 0x4136, "ATi", "Radeon A3 (RS100 IGP320)"},
	{0x1002, 0x4336, "ATi", "Mobility Radeon U1 (RS100 IGP320M)"},
	/* Radeon RV100 */
	{0x1002, 0x5159, "ATi", "Radeon 7000/VE (RV100 QY)"},
	{0x1002, 0x515a, "ATi", "Radeon 7000/VE (RV100 QZ)"},
	{0x1002, 0x4c59, "ATi", "Radeon Mobility M6 (RV100 LY)"},
	{0x1002, 0x4c5a, "ATi", "Radeon Mobility M6 (RV100 LZ)"},
	/* Radeon R200 */
	{0x1002, 0x5148, "ATi", "Radeon 8700/8800 (R200 QH)"},
	{0x1002, 0x5149, "ATi", "Radeon 8500 (R200 QI)"},
	{0x1002, 0x514a, "ATi", "Radeon 8500 (R200 QJ)"},
	{0x1002, 0x514b, "ATi", "Radeon 8500 (R200 QK)"},
	{0x1002, 0x514c, "ATi", "Radeon 8500LE (R200 QL)"},
	{0x1002, 0x514d, "ATi", "Radeon 9100 (R200 QM)"},
	{0x1002, 0x4242, "ATi", "Radeon 8500 AIW (R200 BB)"},
	{0x1002, 0x4243, "ATi", "Radeon 8500 AIW (R200 BC)"},
	/* Radeon RS200 */
	{0x1002, 0x4137, "ATi", "Radeon A4 (RS200 IGP330)"},
	{0x1002, 0x4337, "ATi", "Radeon Mobility U2 (RS200 IGP330M)"},
	/* Radeon RV200 */
	{0x1002, 0x5157, "ATi", "Radeon 7500 (RV200 QW)"},
	{0x1002, 0x5158, "ATi", "Radeon 7500 (RV200 QX)"},
	{0x1002, 0x4c57, "ATi", "Radeon Mobility 7500 M7 (RV200 LW)"},
	{0x1002, 0x4c58, "ATi", "Radeon Mobility 7800 M7 (RV200 LX)"},
	/* Radeon RS250 */
	{0x1002, 0x4237, "ATi", "Radeon 7000 A4+ (RS250 IGP)"},
	{0x1002, 0x4437, "ATi", "Radeon Mobility 7000 (RS250 IGP)"},
	/* Radeon RV250 */
	{0x1002, 0x4964, "ATi", "Radeon 9000/Pro (RV250 ID)"},
	{0x1002, 0x4965, "ATi", "Radeon 9000/Pro (RV250 IE)"},
	{0x1002, 0x4966, "ATi", "Radeon 9000/Pro (RV250 IF)"},
	{0x1002, 0x4967, "ATi", "Radeon 9000/Pro (RV250 IG)"},
	{0x1002, 0x4c64, "ATi", "Radeon Mobility 9000 M9 (RV250 LD)"},
	{0x1002, 0x4c65, "ATi", "Radeon Mobility 9000 M9 (RV250 LE)"},
	{0x1002, 0x4c66, "ATi", "Radeon Mobility 9000 M9 (RV250 LF)"},
	{0x1002, 0x4c67, "ATi", "Radeon Mobility 9000 M9 (RV250 LG)"},
	/* Radeon RV280 */
	{0x1002, 0x5960, "ATi", "Radeon 9200 (RV280)"},
	{0x1002, 0x5961, "ATi", "Radeon 9200 (RV280)"},
	{0x1002, 0x5962, "ATi", "Radeon 9200 (RV280)"},
	{0x1002, 0x5963, "ATi", "Radeon 9200SE (RV280)"},
	{0x1002, 0x5964, "ATi", "Radeon 9200SE (RV280)"},
	{0x1002, 0x5c61, "ATi", "Radeon Mobility 9200 M9+ (RV280)"},
	{0x1002, 0x5c63, "ATi", "Radeon Mobility 9200 M9+ (RV280)"},
	/* Radeon R300 */
	{0x1002, 0x4144, "ATi", "Radeon 9500 (R300 AD)"},
	{0x1002, 0x4145, "ATi", "Radeon 9500 (R300 AE)"},
	{0x1002, 0x4146, "ATi", "Radeon 9600TX/FireGL Z1 (R300 AF)"},
	{0x1002, 0x4147, "ATi", "Radeon 9600TX/FireGL Z1 (R300 AG)"},
	{0x1002, 0x4e44, "ATi", "Radeon 9500/9700/Pro/FireGL X1 (R300 ND)"},
	{0x1002, 0x4e45, "ATi", "Radeon 9500/9700/Pro/FireGL X1 (R300 NE)"},
	{0x1002, 0x4e46, "ATi", "Radeon 9500/9700/Pro/FireGL X1 (R300 NF)"},
	{0x1002, 0x4e47, "ATi", "Radeon 9500/9700/Pro/FireGL X1 (R300 NG)"},
	/* Radeon RS300 */
	{0x1002, 0x5835, "ATi", "Radeon 9100 A5 (RS300 IGP)"},
	{0x1002, 0x5834, "ATi", "Radeon Mobility 9100 U3 (RS300 IGP)"},
	/* Radeon R350 */
	{0x1002, 0x4148, "ATi", "Radeon 9800/Pro/FireGL X2 (R350 AH)"},
	{0x1002, 0x4149, "ATi", "Radeon 9800/Pro/FireGL X2 (R350 AI)"},
	{0x1002, 0x414a, "ATi", "Radeon 9800/Pro/FireGL X2 (R350 AJ)"},
	{0x1002, 0x414b, "ATi", "Radeon 9800/Pro/FireGL X2 (R350 AK)"},
	{0x1002, 0x4e48, "ATi", "Radeon 9800/Pro/FireGL X2 (R350 NH)"},
	{0x1002, 0x4e49, "ATi", "Radeon 9800/Pro/FireGL X2 (R350 NI)"},
	{0x1002, 0x4e4a, "ATi", "Radeon 9800/Pro/FireGL X2 (R350 NJ)"},
	{0x1002, 0x4e4b, "ATi", "Radeon 9800/Pro/FireGL X2 (R350 NK)"},
	/* Radeon RV350 */
	{0x1002, 0x4150, "ATi", "Radeon 9600/FireGL T2 (RV350 AP)"},
	{0x1002, 0x4151, "ATi", "Radeon 9600/FireGL T2 (RV350 AQ)"},
	{0x1002, 0x4152, "ATi", "Radeon 9600/FireGL T2 (RV350 AR)"},
	{0x1002, 0x4153, "ATi", "Radeon 9600/FireGL T2 (RV350 AS)"},
	{0x1002, 0x4154, "ATi", "Radeon 9600/FireGL T2 (RV350 AT)"},
	{0x1002, 0x4156, "ATi", "Radeon 9600/FireGL T2 (RV350 AV)"},
	{0x1002, 0x4e50, "ATi", "Radeon Mobility 9600 M10 (RV350 NP)"},
	{0x1002, 0x4e51, "ATi", "Radeon Mobility 9600 M10 (RV350 NQ)"},
	{0x1002, 0x4e52, "ATi", "Radeon Mobility 9600 M10 (RV350 NR)"},
	{0x1002, 0x4e53, "ATi", "Radeon Mobility 9700 M11 (RV350 NS)"},
	{0x1002, 0x4e54, "ATi", "Radeon Mobility 9700 M11 (RV350 NT)"},
	{0x1002, 0x4e56, "ATi", "Radeon Mobility 9700 M11 (RV350 NV)"},
	/* Radeon RV370 */
	{0x1002, 0x5460, "ATi", "Radeon Mobility M300 (RV370)"},
	{0x1002, 0x5464, "ATi", "Radeon FireGL M22 GL (RV370)"},
	{0x1002, 0x5B64, "ATi", "Radeon FireGL V3100 (RV370)"},
	{0x1002, 0x5B65, "ATi", "Radeon FireGL D1100 (RV370)"},
	{0x1002, 0x5B60, "ATi", "Radeon X300 (RV370)"},
	{0x1002, 0x5B62, "ATi", "Radeon X600 (RV370)"},
	/* Radeon XPRESS */
	{0x1002, 0x5A42, "ATi", "Radeon XPRESS 200M (RS400)"},
	{0x1002, 0x5A62, "ATi", "Radeon XPRESS 200M (RC410)"},
	{0x1002, 0x5955, "ATi", "Radeon XPRESS 200M (RS480)"},
	{0x1002, 0x5975, "ATi", "Radeon XPRESS 200M (RS482)"},
	{0x1002, 0x5A41, "ATi", "Radeon XPRESS 200 (RS400)"},
	{0x1002, 0x5A61, "ATi", "Radeon XPRESS 200 (RC410)"},
	{0x1002, 0x5954, "ATi", "Radeon XPRESS 200 (RS480)"},
	{0x1002, 0x5974, "ATi", "Radeon XPRESS 200 (RS482)"},
	/* Radeon RV380 */
	{0x1002, 0x3150, "ATi", "Radeon Mobility X600 (RV380)"},
	{0x1002, 0x3E50, "ATi", "Radeon X600 (RV380)"},
	{0x1002, 0x3154, "ATi", "Radeon FireGL M24 GL (RV380)"},
	{0x1002, 0x3E54, "ATi", "Radeon FireGL V3200 (RV380)"},
	/* Radeon RV410 */
	{0x1002, 0x564A, "ATi", "Radeon Mobility FireGL V5000 (RV410)"},
	{0x1002, 0x564B, "ATi", "Radeon Mobility FireGL V5000 (RV410)"},
	{0x1002, 0x5652, "ATi", "Radeon Mobility Radeon X700 (RV410)"},
	{0x1002, 0x5653, "ATi", "Radeon Mobility Radeon X700 (RV410)"},
	{0x1002, 0x5E48, "ATi", "Radeon FireGL V5000 (RV410)"},
	{0x1002, 0x5E4D, "ATi", "Radeon Radeon X700 (RV410)"},
	{0x1002, 0x5E4B, "ATi", "Radeon Radeon X700 PRO (RV410)"},
	{0x1002, 0x5E4A, "ATi", "Radeon Radeon X700 XT (RV410)"},
	{0x1002, 0x5E4C, "ATi", "Radeon Radeon X700 SE (RV410)"},
	{0x1002, 0x5E4F, "ATi", "Radeon Radeon X700 SE (RV410)"},
	/* Radeon R420 */
	{0x1002, 0x4A4E, "ATi", "Radeon Mobility 9800 (R420 JN)"},
	{0x1002, 0x4A4D, "ATi", "Radeon FireGL X3 (R420 JM)"},
	{0x1002, 0x4A48, "ATi", "Radeon X800 (R420 JH)"},
	{0x1002, 0x4A4B, "ATi", "Radeon X800 (R420 JK)"},
	{0x1002, 0x4A4C, "ATi", "Radeon X800 (R420 JL)"},
	{0x1002, 0x4A49, "ATi", "Radeon X800 PRO (R420 JI)"},
	{0x1002, 0x4A50, "ATi", "Radeon X800 XT (R420 JP)"},
	{0x1002, 0x4A4A, "ATi", "Radeon X800 SE (R420 JJ)"},
	{0x1002, 0x4A4F, "ATi", "Radeon X800 SE (R420)"},
	/* Radeon R423 */
	{0x1002, 0x5548, "ATi", "Radeon X800 (R423 UH)"},
	{0x1002, 0x5549, "ATi", "Radeon X800 PRO (R423 UI)"},
	{0x1002, 0x5D57, "ATi", "Radeon X800 XT (R423)"},
	{0x1002, 0x554A, "ATi", "Radeon X800 LE (R423 UJ)"},
	{0x1002, 0x554B, "ATi", "Radeon X800 SE (R423 UK)"},
	{0x1002, 0x5551, "ATi", "Radeon FireGL V7200 (R423 UQ)"},
	{0x1002, 0x5552, "ATi", "Radeon FireGL V5100 (R423 UR)"},
	{0x1002, 0x5554, "ATi", "Radeon FireGL V7100 (R423 UT)"},
	{0x1002, 0x5550, "ATi", "Radeon FireGL V7100 (R423)"},
	/* Radeon R430 */
	{0x1002, 0x5D4A, "ATi", "Radeon Mobility X800 (R430)"},
	{0x1002, 0x5D48, "ATi", "Radeon Mobility X800 XT (R430)"},
	{0x1002, 0x5D49, "ATi", "Radeon Mobility FireGL V5100 (R430)"},
	{0x1002, 0x554F, "ATi", "Radeon X800 (R430)"},
	{0x1002, 0x554D, "ATi", "Radeon X800 XL (R430)"},
	{0x1002, 0x554E, "ATi", "Radeon X800 SE (R430)"},
	{0x1002, 0x554C, "ATi", "Radeon X800 XTP (R430)"},
	/* Radeon R480 */
	{0x1002, 0x5D50, "ATi", "Radeon FireGL GL 5D50 (R480)"},
	{0x1002, 0x5D4C, "ATi", "Radeon X850 (R480)"},
	{0x1002, 0x5D4F, "ATi", "Radeon X850 PRO (R480)"},
	{0x1002, 0x4B4B, "ATi", "Radeon X850 PRO (R480)"},
	{0x1002, 0x5D52, "ATi", "Radeon X850 XT (R480)"},
	{0x1002, 0x4B49, "ATi", "Radeon X850 XT (R480)"},
	{0x1002, 0x5D4D, "ATi", "Radeon X850 XT PE (R480)"},
	{0x1002, 0x4B4C, "ATi", "Radeon X850 XT PE (R480)"},
	{0x1002, 0x5D4E, "ATi", "Radeon X850 SE (R480)"},
	{0x1002, 0x4B4A, "ATi", "Radeon X850 SE (R480)"}
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
				sprintf( zNodePath, "graphics/radeon_%i_0x%x_0x%x", nPCINum, ( uint )sInfo.nVendorID, ( uint )sInfo.nDeviceID );

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





