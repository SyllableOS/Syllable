/*
 * From AGPGART
 * Copyright (C) 2002-2003 Dave Jones
 * Copyright (C) 1999 Jeff Hartmann
 * Copyright (C) 1999 Precision Insight, Inc.
 * Copyright (C) 1999 Xi Graphics, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * JEFF HARTMANN, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <appserver/pci_graphics.h>
#include "driver.h"


struct gfx_device
{
	int nVendorID;
	int nDeviceID;
	char *zVendorName;
	char *zDeviceName;
};

struct gfx_node
{
	PCI_Info_s sGFXInfo;
	PCI_Info_s sAGPInfo;
	bool bIs9xx;
	area_id hGFXRegisters;
	uint8* pGFXRegisters;
	area_id hGTTRegisters; // i9xx
	uint32* pGTTRegisters; // i9xx
	uint32 nGATTAddress;
	uint32 nScratchPage;
	uint32 nPageEntries;
	uint32 nGTTEntries;
	uint32 nAGPEntries;
	uint32 nCursorAddress;
	
	char zName[255];
};

#define APPSERVER_DRIVER "i855"

struct gfx_device g_sDevices[] = {
	{0x8086, 0x3582, "Intel", "i855/852"},
	{0x8086, 0x3577, "Intel", "i830M"},
	{0x8086, 0x2562, "Intel", "i845G"},
	{0x8086, 0x2572, "Intel", "i865G"},
	{0x8086, 0x2582, "Intel", "i915G"},
	{0x8086, 0x258A, "Intel", "E7221G"},
	{0x8086, 0x2592, "Intel", "i915GM"},
	{0x8086, 0x2772, "Intel", "i945G"},
	{0x8086, 0x2792, "Intel", "i945GME"},
	{0x8086, 0x27A2, "Intel", "i945GM"}
};

#define flush_agp_cache() asm volatile("wbinvd":::"memory")

#define I855_GET_CURSOR_ADDRESS	100

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
		memcpy_to_user( pArgs, &psNode->sGFXInfo, sizeof( PCI_Info_s ) );
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
	case I855_GET_CURSOR_ADDRESS:
		{
			memcpy_to_user( pArgs, &psNode->nCursorAddress, sizeof( uint32 ) );
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

extern int32 get_free_page( int );
extern uint32 get_free_pages( int nPageCount, int nFlags );

bool i855_init_pagetable( struct gfx_node* psNode )
{
	PCI_bus_s *psBus;
	uint16 nGMCH;
	uint32 i;
	uint32 j;
	uint32 nContMem;
	uint8 rdct;
	static const int ddt[4] = { 0, 16, 32, 64 };
	
	/* Get size of preallocated memory */
	
	psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	
	nGMCH = psBus->read_pci_config( psNode->sAGPInfo.nBus, psNode->sAGPInfo.nDevice,
										psNode->sAGPInfo.nFunction, I855_GMCH_CTRL, 2 );

	if( psNode->sAGPInfo.nDeviceID == 0x3575 || psNode->sAGPInfo.nDeviceID == 0x2560 )
	{
		switch( nGMCH & I830_GMCH_GMS_MASK) {
			case I830_GMCH_GMS_STOLEN_512:
				psNode->nGTTEntries = KB(512) - KB(132);
				break;
			case I830_GMCH_GMS_STOLEN_1024:
				psNode->nGTTEntries = MB(1) - KB(132);
				break;
			case I830_GMCH_GMS_STOLEN_8192:
				psNode->nGTTEntries = MB(8) - KB(132);
				break;
			case I830_GMCH_GMS_LOCAL:
				rdct = INREG8(psNode->pGFXRegisters,
				      I830_RDRAM_CHANNEL_TYPE);
				psNode->nGTTEntries = (I830_RDRAM_ND(rdct) + 1) *
					MB(ddt[I830_RDRAM_DDT(rdct)]);			
				break;
			default:
				psNode->nGTTEntries = 0;
				break;
		}
		if( ( nGMCH & I830_GMCH_MEM_MASK ) == I830_GMCH_MEM_128M )
			psNode->nPageEntries = 32768;
		else
			psNode->nPageEntries = 16384;
		printk( "i855: %liMb aperture\n", psNode->nPageEntries * 4 / KB( 1 ) );
	}
	else {
		switch( nGMCH & I855_GMCH_GMS_MASK) {
			case I855_GMCH_GMS_STOLEN_1M:
				psNode->nGTTEntries = MB(1) - KB(132);
				break;
			case I855_GMCH_GMS_STOLEN_4M:
				psNode->nGTTEntries = MB(4) - KB(132);
				break;
			case I855_GMCH_GMS_STOLEN_8M:
				psNode->nGTTEntries = MB(8) - KB(132);
				break;
			case I855_GMCH_GMS_STOLEN_16M:
				psNode->nGTTEntries = MB(16) - KB(132);
				break;
			case I855_GMCH_GMS_STOLEN_32M:
				psNode->nGTTEntries = MB(32) - KB(132);
				break;
			case I915_GMCH_GMS_STOLEN_48M:
				if( psNode->bIs9xx )
				{
					psNode->nGTTEntries = MB(48) - KB(132);
					break;
				}
			case I915_GMCH_GMS_STOLEN_64M:
				if( psNode->bIs9xx )
				{
					psNode->nGTTEntries = MB(64) - KB(132);
					break;
				}
			default:
				psNode->nGTTEntries = 0;
				break;
		}
		if( psNode->bIs9xx && ( psNode->sGFXInfo.u.h0.nBase2 & 0x08000000 ) )
			psNode->nPageEntries = 65536;
		else
			psNode->nPageEntries = 32768;
		printk( "i855: %liMb aperture\n", psNode->nPageEntries * 4 / KB( 1 ) );
	}
	
	printk( "i855: Detected %liK preallocated memory\n", psNode->nGTTEntries / KB( 1 ) );
	psNode->nGTTEntries /= PAGE_SIZE;

	
	
	/* Enable controller */
		
	nGMCH |= I855_GMCH_ENABLED;
	
	nGMCH = psBus->write_pci_config( psNode->sAGPInfo.nBus, psNode->sAGPInfo.nDevice,
										psNode->sAGPInfo.nFunction, I855_GMCH_CTRL, 2, nGMCH );

	OUTREG32( psNode->pGFXRegisters, I855_PGETBL_CTL, psNode->nGATTAddress | I855_PGETBL_ENABLED );

	/* Allocate scratch page and put it in the unused pagetable entries */

	psNode->nScratchPage = get_free_page( MEMF_CLEAR );
	
	printk( "i855: Allocated scratch page @ 0x%x\n", (uint)psNode->nScratchPage );
	
	for( i = psNode->nGTTEntries; i < psNode->nPageEntries; i++)
	{
		if( psNode->bIs9xx ) {
			OUTREG32( psNode->pGTTRegisters, i, psNode->nScratchPage | I855_PTE_VALID );
			INREG32( psNode->pGTTRegisters, i );
		}
		else {
			OUTREG32( psNode->pGFXRegisters, I855_PTE_BASE + (i * 4), psNode->nScratchPage | I855_PTE_VALID );
			INREG32( psNode->pGFXRegisters, I855_PTE_BASE + (i * 4) );
		}
	}

	flush_agp_cache();
	/* Calculate the number of additional pages to get 32mb video memory */
	psNode->nAGPEntries = 8192 - psNode->nGTTEntries;
	
	/* We need at least some agp memory for the cursor and video overlay 
	   because we need to know the physical address of the memory */
	if( psNode->nAGPEntries < 10 )
		psNode->nAGPEntries = 10;

	printk( "i855: Allocating %liK additional memory\n", psNode->nAGPEntries * 4 );

	for( i = psNode->nGTTEntries; i < ( psNode->nGTTEntries + psNode->nAGPEntries - 10 ); i++)
	{
		if( psNode->bIs9xx ) {
			OUTREG32( psNode->pGTTRegisters, i, (uint32)get_free_page( MEMF_CLEAR ) | I855_PTE_VALID );
			INREG32( psNode->pGTTRegisters, i );
		} else {
			OUTREG32( psNode->pGFXRegisters, I855_PTE_BASE + (i * 4), (uint32)get_free_page( MEMF_CLEAR ) | I855_PTE_VALID );
			INREG32( psNode->pGFXRegisters, I855_PTE_BASE + (i * 4) );
		}
		
	}

	/* Allocate the last 10 pages of agp memory as linear memory */
	if( ( nContMem = get_free_pages( 10, MEMF_CLEAR ) ) == 0 )
	{
		printk( "i855: Could not allocate 10 pages as linear memory!\n" );
		return( false );
	}
	
	for( j = 0; i < ( psNode->nGTTEntries + psNode->nAGPEntries ); i++, j++)
	{
		if( psNode->bIs9xx ) {
			OUTREG32( psNode->pGTTRegisters, i, (((uint32)nContMem) + ( j * PAGE_SIZE )) | I855_PTE_VALID );
			INREG32( psNode->pGTTRegisters, i );
		} else {
			OUTREG32( psNode->pGFXRegisters, I855_PTE_BASE + (i * 4), (((uint32)nContMem) + ( j * PAGE_SIZE )) | I855_PTE_VALID );
			INREG32( psNode->pGFXRegisters, I855_PTE_BASE + (i * 4) );
		}
	}
	
	psNode->nCursorAddress = nContMem;
	
	flush_agp_cache();

	return( true );
}

bool i855_init( struct gfx_node* psNode )
{
	PCI_bus_s *psBus;
	PCI_Info_s sAGPBridge;
	int i;
	bool bAGPFound = false;
	uint32 nGFXRegsPhys;
	uint32 nGTTRegsPhys;
	
	/* Use the first device */
	if( psNode->sGFXInfo.nFunction != 0 )
	{
		printk( "i855: Ignoring secondary device\n" );
		return( false );
	}
	
	psNode->bIs9xx = psNode->sGFXInfo.nDeviceID >= 0x2582 && psNode->sGFXInfo.nDeviceID < 0x3500;
		
	/* Search for the AGP bridge */
	psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	for ( i = 0; psBus->get_pci_info( &sAGPBridge, i ) == 0; ++i )
	{
		if( ( sAGPBridge.nVendorID == 0x8086 && ( sAGPBridge.nDeviceID == 0x3580 ||
			sAGPBridge.nDeviceID == 0x3575 || sAGPBridge.nDeviceID == 0x2560 ||	sAGPBridge.nDeviceID == 0x2570 
			|| sAGPBridge.nDeviceID == 0x2580 || sAGPBridge.nDeviceID == 0x2590 || sAGPBridge.nDeviceID == 0x2770
			|| sAGPBridge.nDeviceID == 0x27A0 ) ) )
		{
			bAGPFound = true;
			break;
		}
	}
	
	if( !bAGPFound )
	{
		printk( "i855: Could not find the AGP bridge!\n" );
		return( false );
	}
	
	/* Map GFX registers */
	memcpy( &psNode->sAGPInfo, &sAGPBridge, sizeof( PCI_Info_s ) );
	
	printk( "i855: Found AGP bridge at %i:%i:%i\n", sAGPBridge.nBus, sAGPBridge.nDevice,
													sAGPBridge.nFunction );
	
	nGFXRegsPhys = psBus->read_pci_config( psNode->sGFXInfo.nBus, psNode->sGFXInfo.nDevice,
										psNode->sGFXInfo.nFunction, psNode->bIs9xx ? I915_MMADDR : I855_MMADDR, 4 );
	nGFXRegsPhys &= 0xfff80000;
	
	psNode->hGFXRegisters = create_area( "i855_gfx_regs", ( void ** )&psNode->pGFXRegisters, 
							128 * PAGE_SIZE, 128 * PAGE_SIZE, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
	remap_area( psNode->hGFXRegisters, (void*)nGFXRegsPhys );
	
	printk( "i855: GFX memory address @ 0x%x mapped to 0x%x\n", (uint)nGFXRegsPhys, (uint)psNode->pGFXRegisters );
	
	if( psNode->bIs9xx ) {
		nGTTRegsPhys = psBus->read_pci_config( psNode->sGFXInfo.nBus, psNode->sGFXInfo.nDevice,
										psNode->sGFXInfo.nFunction, I915_PTEADDR, 4 );
		psNode->hGTTRegisters = create_area( "i855_gtt_regs", ( void ** )&psNode->pGTTRegisters, 
							256 * PAGE_SIZE, 256 * PAGE_SIZE, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
		remap_area( psNode->hGTTRegisters, (void*)nGTTRegsPhys );
		printk( "i855: GTT memory address @ 0x%x mapped to 0x%x\n", (uint)nGTTRegsPhys, (uint)psNode->pGTTRegisters );
	}

	
	/* Get GATT table address */
	psNode->nGATTAddress = INREG32( psNode->pGFXRegisters, I855_PGETBL_CTL ) & 0xfffff000;
	
	printk( "i855: GATT address @ 0x%x\n", (uint)psNode->nGATTAddress );
	
	flush_agp_cache();
	
	/* Initialize pagetable entries */
	return( i855_init_pagetable( psNode ) );
}

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
				memcpy( &psNode->sGFXInfo, &sInfo, sizeof( PCI_Info_s ) );
				strcpy( psNode->zName, zTemp );
				
				/* Initialize device */
				if( !i855_init( psNode ) )
					continue;

				/* Create node path */
				sprintf( zNodePath, "graphics/i855_%i_0x%x_0x%x", nPCINum, ( uint )sInfo.nVendorID, ( uint )sInfo.nDeviceID );

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







