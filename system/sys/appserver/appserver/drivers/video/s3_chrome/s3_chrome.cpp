/*
 *  S3 Chrome driver for Syllable
 *
 *
 *  Copyright 2007 Arno Klenke
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/isa_io.h>
#include <atheos/vesa_gfx.h>
#include <atheos/udelay.h>
#include <atheos/time.h>
#include <appserver/pci_graphics.h>

#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <gui/bitmap.h>

#include "s3_chrome.h"



using namespace os;

inline uint32 pci_size(uint32 base, uint32 mask)
{
	uint32 size = base & mask;
	return size & ~(size-1);
}

static uint32 get_pci_memory_size(int nFd, const PCI_Info_s *pcPCIInfo, int nResource)
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource*4;
	uint32 nBase = pci_gfx_read_config(nFd, nBus, nDev, nFnc, nOffset, 4);
	
	pci_gfx_write_config(nFd, nBus, nDev, nFnc, nOffset, 4, ~0);
	uint32 nSize = pci_gfx_read_config(nFd, nBus, nDev, nFnc, nOffset, 4);
	pci_gfx_write_config(nFd, nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	if (!(nSize & PCI_ADDRESS_SPACE)) {
		return pci_size(nSize, PCI_ADDRESS_MEMORY_32_MASK);
	} else {
		return pci_size(nSize, PCI_ADDRESS_IO_MASK & 0xffff);
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Chrome::Chrome(  int nFd ) :
	m_hFrameBufferArea(-1)
{
	m_bIsInitiated = false;
	m_pFrameBufferBase = NULL;
	
	/* Get Info */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_cPCIInfo ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}
	
	uint32 nMemSize = 0;
	uint32 nMemBase = 0;
	nMemSize = get_pci_memory_size(nFd, &m_cPCIInfo, 1);
	nMemBase = m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;
	
	m_hFrameBufferArea = create_area("chrome_framebuffer", (void**)&m_pFrameBufferBase,
	                                 nMemSize, AREA_FULL_ACCESS | AREA_WRCOMB, AREA_NO_LOCK);
	
	remap_area(m_hFrameBufferArea, (void*)( ( nMemBase ) ));
	dbprintf( "Chrome:: Framebuffer @ 0x%x mapped to 0x%x size 0x%x\n", (uint)(nMemBase),
													(uint)m_pFrameBufferBase, (uint)nMemSize );
	

	/* Save active devices */
	struct RMREGS rm;
	memset( &rm, 0, sizeof( struct RMREGS ) );
	rm.EAX = 0x4f14;
	rm.EBX = 0x0103;
	realint( 0x10, &rm );
	m_nActiveDevices = rm.ECX;

	/* Initialize the screenmodes */
	if( !InitModes() )
	{
		dbprintf( "Chrome:: Could not find any screen modes!\n" );
		return;
	}

	m_bIsInitiated = true;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Chrome::~Chrome()
{
	if (m_hFrameBufferArea != -1) {
		delete_area(m_hFrameBufferArea);
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Chrome::InitModes( void )
{
	/* Parse BIOS modes */
	struct RMREGS rm;
	uint32 nModeNr = 0;
	uint32 nVesaMode = 0;
	uint32 nRefreshNr = 0;
	VESA_Mode_Info_s sModeInfo;	
	
	do
	{
		memset( &rm, 0, sizeof( struct RMREGS ) );
		rm.EAX = 0x4f14;
		rm.EBX = 0x0202;
		rm.ECX = 0;
		rm.EDX = nModeNr;
		realint( 0x10, &rm );
		
		if( rm.EAX != 0x4f )
			break;
			
		nVesaMode = rm.ECX;
		nModeNr = (uint32)rm.EDX;

		get_vesa_mode_info( &sModeInfo, nVesaMode );

		if( sModeInfo.PhysBasePtr == 0 )
		{		// We must have linear frame buffer
			continue;
		}
		if( sModeInfo.BitsPerPixel < 8 )
		{
			continue;
		}
		if( sModeInfo.NumberOfPlanes != 1 )
		{
			continue;
		}

		if( sModeInfo.BitsPerPixel != 16 && sModeInfo.BitsPerPixel != 32 )
		{
			continue;
		}

		dbprintf( "Chrome:: Mode %04x: %dx%d %d BPP %d BPL - %d:%d:%d, %d:%d:%d (%p)\n", nVesaMode, sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BitsPerPixel, sModeInfo.BytesPerScanLine, sModeInfo.RedMaskSize, sModeInfo.GreenMaskSize, sModeInfo.BlueMaskSize, sModeInfo.RedFieldPosition, sModeInfo.GreenFieldPosition, sModeInfo.BlueFieldPosition, ( void * )sModeInfo.PhysBasePtr );

		nRefreshNr = 0;
		do
		{
			memset( &rm, 0, sizeof( struct RMREGS ) );
			rm.EAX = 0x4f14;
			rm.EBX = 0x0201;
			rm.ECX = nVesaMode;
			rm.EDX = nRefreshNr;
			realint( 0x10, &rm );

			if( rm.EAX != 0x4f )
				break;

			if( sModeInfo.RedMaskSize == 5 && sModeInfo.GreenMaskSize == 6 && sModeInfo.BlueMaskSize == 5 && sModeInfo.RedFieldPosition == 11 && sModeInfo.GreenFieldPosition == 5 && sModeInfo.BlueFieldPosition == 0 )
			{
				m_cModeList.push_back( ChromeMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine, CS_RGB16, (int)rm.EDI, nVesaMode | 0x4000/*, sModeInfo.PhysBasePtr*/ ) );
			}
			else if( sModeInfo.BitsPerPixel == 32 && sModeInfo.RedMaskSize == 8 && sModeInfo.GreenMaskSize == 8 && sModeInfo.BlueMaskSize == 8 && sModeInfo.RedFieldPosition == 16 && sModeInfo.GreenFieldPosition == 8 && sModeInfo.BlueFieldPosition == 0 )
			{
				m_cModeList.push_back( ChromeMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine, CS_RGB32, (int)rm.EDI, nVesaMode | 0x4000/*, sModeInfo.PhysBasePtr*/ ) );
			}

			nRefreshNr = rm.EDX;		
		} while( nRefreshNr != 0 );

	} while( nModeNr != 0 );
	
	if( m_cModeList.size() == 0 )
		return( false );
	return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

area_id Chrome::Open( void )
{
	return( m_hFrameBufferArea );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Chrome::Close( void )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Chrome::GetScreenModeCount()
{
	return ( m_cModeList.size() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Chrome::GetScreenModeDesc( int nIndex, screen_mode * psMode )
{
	if( nIndex >= 0 && nIndex < int ( m_cModeList.size() ) )
	{
		*psMode = m_cModeList[nIndex];
		return ( true );
	}
	else
	{
		return ( false );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Chrome::SetScreenMode( screen_mode sMode )
{
	m_nCurrentMode = -1;

	for( int i = GetScreenModeCount() - 1; i >= 0; --i )
	{
		if( m_cModeList[i].m_nWidth == sMode.m_nWidth && m_cModeList[i].m_nHeight == sMode.m_nHeight && m_cModeList[i].m_eColorSpace == sMode.m_eColorSpace && m_cModeList[i].m_vRefreshRate == sMode.m_vRefreshRate )
		{
			m_nCurrentMode = i;
			break;
		}

	}

	if( m_nCurrentMode >= 0 )
	{
		struct RMREGS rm;

		memset( &rm, 0, sizeof( struct RMREGS ) );
		rm.EAX = 0x4f14;
		rm.EBX = 0x01;
		rm.ECX = m_nActiveDevices;
		rm.EDI = (int)sMode.m_vRefreshRate;
		realint( 0x10, &rm );		
	
		memset( &rm, 0, sizeof( struct RMREGS ) );
		rm.EAX = 0x4f02;
		rm.EBX = m_cModeList[m_nCurrentMode].m_nVesaMode;;
		realint( 0x10, &rm );

		memset( &rm, 0, sizeof( struct RMREGS ) );
		rm.EBX = 0x01;		// Get display offset.
		rm.EAX = 0x4f07;
		realint( 0x10, &rm );

		memset( &rm, 0, sizeof( struct RMREGS ) );
		rm.EBX = 0x00;
		rm.EAX = 0x4f07;
		realint( 0x10, &rm );

		memset( &rm, 0, sizeof( struct RMREGS ) );
		rm.EBX = 0x01;		// Get display offset.
		rm.EAX = 0x4f07;
		realint( 0x10, &rm );
	
		memset( &rm, 0, sizeof( struct RMREGS ) );
		rm.EBX = 0x00;		// Set logical scan line length
		rm.ECX = sMode.m_nWidth;
		rm.EAX = 0x4f06;
		realint( 0x10, &rm );

		return( 0 );
	}
	return ( -1 );
}

screen_mode Chrome::GetCurrentScreenMode()
{
	return ( m_cModeList[m_nCurrentMode] );
}

//-----------------------------------------------------------------------------

extern "C" DisplayDriver* init_gfx_driver( int nFd )
{
    dbprintf("s3_chrome attempts to initialize\n");
    
    try {
	    Chrome *pcDriver = new Chrome( nFd );
	    if (pcDriver->IsInitiated()) {
		    return pcDriver;
	    }
	    return NULL;
    }
    catch (std::exception& cExc) {
	    dbprintf("Got exception\n");
	    return NULL;
    }
}




