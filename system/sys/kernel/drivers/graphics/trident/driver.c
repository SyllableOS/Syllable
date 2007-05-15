
/*
 *  The Syllable kernel
 *	Trident graphics kernel driver
 *	Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  2002-05-12 Damien Danneels
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
#include <atheos/isa_io.h>
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
	bool bIsISA;
	PCI_Info_s sInfo;
	char zName[255];
};

#define APPSERVER_DRIVER "trident"

struct gfx_device g_sDevices[] = {
	{0x1023, 0x8400, "Trident", "CyberBlade i7"},
	{0x1023, 0x8420, "Trident", "CyberBlade i7D"},
	{0x1023, 0x8500, "Trident", "CyberBlade i1"},
	{0x1023, 0x9320, "Trident", "Cyber 9320"},
	{0x1023, 0x9388, "Trident", "Cyber9388"},
	{0x1023, 0x9397, "Trident", "Cyber 9397"},
	{0x1023, 0x939a, "Trident", "Cyber 9397DVD"},
	{0x1023, 0x9420, "Trident", "TGUI 9420"},
	{0x1023, 0x9440, "Trident", "TGUI 9440"},
	{0x1023, 0x9520, "Trident", "Cyber 9520"},
	{0x1023, 0x9525, "Trident", "Cyber 9525"},
	{0x1023, 0x9540, "Trident", "Cyber 9540"},
	{0x1023, 0x9660, "Trident", "TGUI/Providia 9660"},
	{0x1023, 0x9680, "Trident", "TGUI/Providia 9680"},
	{0x1023, 0x9750, "Trident", "Image 975"},
	{0x1023, 0x9850, "Trident", "Image 985"},
	{0x1023, 0x9880, "Trident", "Blade 3D"}
};


bool IdentifyChip()
{
  uint8 temp;
  uint8 origVal;
  uint8 newVal;

  // Check first that we have a Trident card.
  outb(0x0B, 0x3C4);
  temp = inb(0x3C5);  // Save old value
  outb(0x0B, 0x3C4);  // Switch to Old Mode
  outb(0x00, 0x3C5);
  inb(0x3C5);         // Now to New Mode
  outb(0x0E, 0x3C4 );
  origVal = inb(0x3C5);
  outb(0x00, 0x3C5 );
  newVal = inb(0x3C5) & 0x0F;
  outb((origVal ^ 0x02), 0x3C5 );

  // If it is not a Trident card, quit.
  if (newVal != 2) {
    outb(0x0B, 0x3C4 );  // Restore value of 0x0B
    outb(temp, 0x3C5 );
    outb(0x0E, 0x3C4 );
    outb(origVal, 0x3C5 );
  } else {
    // At this point, we are sure to have a trident card.
    outb(0x0B, 0x3C4 );
    temp = inb(0x3C5);
    switch (temp) {
      // Unsupported chipsets :
      // case 0x01:
      //   found = TVGA8800BR;
      //   break;
      // case 0x02:
      //   found = TVGA8800CS;
      //   break;
      // case 0x03:
      //   found = TVGA8900B;
      //   break;
      // case 0x04:
      // case 0x13:
      //   found = TVGA8900C;
      //   break;
      // case 0x23:
      //   found = TVGA9000;
      //   break;
      // case 0x33:
      //   found = TVGA8900D;
      //   break;
      // case 0x43:
      //   found = TVGA9000I;
      //   break;
      // case 0x53:
      //   found = TVGA9200CXR;
      //   break;
      // case 0x63:
      //   found = TVGA9100B;
      //   break;
      // case 0x83:
      //   found = TVGA8200LX;
      //   break;
      // case 0x93:
      //   found = TGUI9400CXI;
      //   break;
      //
      // List of supported adapters (they provide linear framebuffer.):
      case 0xA3:
        return( true );
        break;
      case 0x73:
      case 0xC3:
        return( true );
        break;
      case 0xD3:
       return( true );
        break;
      case 0xE3:
        return( true );
        break;
      case 0xF3:
        return( true );
        break;
      default:
        printk("Unrecognised Trident chipset : 0x%x.\n", temp );
        break;
    }
  }
  return( false );
}

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
		if( psNode->bIsISA )
			nError = -ENODEV;
		else
			memcpy_to_user( pArgs, &psNode->sInfo, sizeof( PCI_Info_s ) );
		break;
	case PCI_GFX_READ_PCI_CONFIG:
		if( psNode->bIsISA )
			nError = -ENODEV;
		else
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
		if( psNode->bIsISA )
			nError = -ENODEV;
		else
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
		goto nopci;

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
				sprintf( zNodePath, "graphics/trident_%i_0x%x_0x%x", nPCINum, ( uint )sInfo.nVendorID, ( uint )sInfo.nDeviceID );

				if ( create_device_node( nDeviceID, sInfo.nHandle, zNodePath, &g_sOperations, psNode ) < 0 )
				{
					printk( "Error: Failed to create device node %s\n", zNodePath );
					continue;
				}
				
				bDevFound = true;
			}
		}
	}
nopci:
	if( IdentifyChip() == true )
	{
		
		int nDeviceHandle = register_device( "Trident ISA", "isa" );
		
		printk( "Trident ISA card found\n" );
		claim_device( nDeviceID, nDeviceHandle, "Trident ISA", DEVICE_VIDEO );
		
		/* Create private node */
		psNode = kmalloc( sizeof( struct gfx_node ), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );
		if ( psNode == NULL )
		{
			printk( "Error: Out of memory\n" );
			return( -ENODEV );
		}
		psNode->bIsISA = true;
		strcpy( psNode->zName, "Trident ISA" );

		/* Create node path */
		strcpy( zNodePath, "graphics/trident_isa" );

		if ( create_device_node( nDeviceID, nDeviceID, zNodePath, &g_sOperations, psNode ) < 0 )
		{
			printk( "Error: Failed to create device node %s\n", zNodePath );
			return( -ENODEV );
		}
				
		bDevFound = true;
	}
	
	
	if ( !bDevFound )
	{
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







