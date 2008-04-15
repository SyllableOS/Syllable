/*

The Syllable application server
Intel i855 graphics driver  
Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright 2002 by David Dawes.
Copyright 2004 Arno Klenke

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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

#include "i855.h"



using namespace os;

#define	MAX_MODEINFO_NAME			79
#define RING_BUFFER_SIZE ( 8 * 1024 )

#define I855_GET_CURSOR_ADDRESS 100

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

i855::i855(  int nFd ) :
	m_cGELock( "i855_engine_lock" ),
	m_hRegisterArea(-1),
	m_hFrameBufferArea(-1)
{
	m_bIsInitiated = false;
	m_pRegisterBase = m_pFrameBufferBase = m_pRingBase = m_pCursorBase = NULL;
	char nBiosVersion[5];
	m_nRingSize = RING_BUFFER_SIZE;
	m_nRingMask = m_nRingSize - 1;
	m_nFrameBufferOffset = ( m_nRingSize + 0xffff ) & ~( 0xffff );
	m_bTwoPipes = false;
	m_bEngineDirty = false;
	bool bSwapBases = false;
	bool bTweakBios = false;
	
	/* Get Info */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_cPCIInfo ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}
	
	/* Print gpu type */
	char zGPUName[30];
	switch( m_cPCIInfo.nDeviceID )
	{
		case 0x3577:
			m_bTwoPipes = true;
			strcpy( zGPUName, "i830M" );
		break;
		case 0x2562:
			strcpy( zGPUName, "i845G" );
		break;
		case 0x3582:
		{
			m_bTwoPipes = true;
			bTweakBios = true;
			uint32 nVariant = pci_gfx_read_config(nFd, m_cPCIInfo.nBus, m_cPCIInfo.nDevice,
												m_cPCIInfo.nFunction, I85X_CAPID, 4);
			nVariant = ( nVariant >> I85X_VARIANT_SHIFT) & I85X_VARIANT_MASK;
			switch( nVariant )
			{
				case I855_GM:
					strcpy( zGPUName, "i855GM" );
				break;
				case I855_GME:
					strcpy( zGPUName, "i855GME" );
				break;
				case I852_GM:
					strcpy( zGPUName, "i852GM" );
				break;
				case I852_GME:
					strcpy( zGPUName, "i852GME" );
				break;
				default:
					strcpy( zGPUName, "i852GM/i855GM (unknown variant)" );
				break;
			}
			break;
		}
		case 0x2572:
			strcpy( zGPUName, "i865G" );
		break;
		case 0x2582:
			m_bTwoPipes = true;
			bTweakBios = true;
			bSwapBases = true;
			strcpy( zGPUName, "i915G" );
		break;
		case 0x258A:
			bTweakBios = true;
			bSwapBases = true;
			strcpy( zGPUName, "E7221G" );
		break;
		case 0x2592:
			m_bTwoPipes = true;
			bTweakBios = true;
			bSwapBases = true;
			strcpy( zGPUName, "i915GM" );
		break;
		case 0x2772:
			m_bTwoPipes = true;
			bTweakBios = true;
			bSwapBases = true;
			strcpy( zGPUName, "i945G" );
		break;
		case 0x2792:
			m_bTwoPipes = true;
			bTweakBios = true;
			bSwapBases = true;
			strcpy( zGPUName, "i945GME" );
		break;
		case 0x27A2:
			m_bTwoPipes = true;
			bTweakBios = true;
			bSwapBases = true;
			strcpy( zGPUName, "i945GM" );
		break;
		default:
			strcpy( zGPUName, "Unknown" );
	}
	
	dbprintf( "i855:: Intel %s detected\n", zGPUName );
	
	/* Map ROM */
	m_pRomBase = NULL;
	m_hRomArea = create_area("i855_rom", (void**)&m_pRomBase, 0x30000,
	                              AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hRomArea, (void*)(0x000c0000));
	
	/* Create mmio and framebuffer area */
	uint32 nIoSize = 0;
	uint32 nIoBase = 0;
	if( bSwapBases ) {
		nIoSize = get_pci_memory_size(nFd, &m_cPCIInfo, 0);
		nIoBase = m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	} else {
		nIoSize = get_pci_memory_size(nFd, &m_cPCIInfo, 1);
		nIoBase = m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;
	}
		
	m_hRegisterArea = create_area("i855_regs", (void**)&m_pRegisterBase, nIoSize,
	                              AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hRegisterArea, (void*)(nIoBase));
	
	dbprintf( "i855:: MMIO @ 0x%x mapped to 0x%x size 0x%x\n", (uint)nIoBase,
													(uint)m_pRegisterBase, (uint)nIoSize );
	
	uint32 nMemSize = 0;
	uint32 nMemBase = 0;
	if( bSwapBases ) {
		nMemSize = get_pci_memory_size(nFd, &m_cPCIInfo, 2);
		nMemBase = m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK;
	} else {
		nMemSize = get_pci_memory_size(nFd, &m_cPCIInfo, 0);
		nMemBase = m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	}
	m_hFrameBufferArea = create_area("i855_framebuffer", (void**)&m_pFrameBufferBase,
	                                 nMemSize, AREA_FULL_ACCESS | AREA_WRCOMB, AREA_NO_LOCK);
	
	remap_area(m_hFrameBufferArea, (void*)( ( nMemBase ) + m_nFrameBufferOffset ));
	dbprintf( "i855:: Framebuffer @ 0x%x mapped to 0x%x size 0x%x\n", (uint)((nMemBase) + m_nFrameBufferOffset),
													(uint)m_pFrameBufferBase, (uint)nMemSize );
	
	/* Create command ring area */
	m_hRingArea = create_area("i855_cmd_ring", (void**)&m_pRingBase,
	                                 m_nRingSize, AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hRingArea, (void*)(nMemBase));
	
	dbprintf( "i855:: Ringbuffer @ 0x%x mapped to 0x%x size 0x%x\n", (uint)(nMemBase),
													(uint)m_pRingBase, (uint)m_nRingSize );
	/* Initialize the cursor */
	/* The last 10 pages of the video memory are allocated as linear memory */
	m_nCursorOffset = ( 8192 - 10 ) * PAGE_SIZE;
	m_pCursorBase = m_pFrameBufferBase + m_nCursorOffset - m_nFrameBufferOffset;
	if( ioctl( nFd, I855_GET_CURSOR_ADDRESS, &m_nCursorAddress ) != 0 )
	{
		dbprintf( "i855:: Could not get cursor address from kernel driver\n" );
		return;
	}
	dbprintf( "i855:: Cursor @ 0x%x mapped to offset 0x%x\n", (uint)m_nCursorAddress,
															(uint)m_nCursorOffset );
	

	/* Initialize the video overlay */
	m_bVideoOverlayUsed = m_bVideoIsOn = false;
	m_psVidRegs = (i855VideoRegs_s*)(m_pCursorBase + PAGE_SIZE * 4);
	m_nVideoAddress = m_nCursorAddress + PAGE_SIZE * 4;
	m_nVideoEnd = m_nCursorOffset;
	dbprintf( "i855:: Video @ 0x%x end 0x%x\n", (uint)m_nVideoAddress,
															(uint)m_nVideoEnd );
	

	/* Get BIOS version */
	m_nBIOSVersion = 0;
	struct RMREGS rm;
	memset( &rm, 0, sizeof( struct RMREGS ) );
	rm.EAX = 0x5f01;
	realint( 0x10, &rm );
	
	if( rm.EAX == 0x005f )
	{
		uint32 nVer = rm.EBX;
		memset( nBiosVersion, 0, 5 );
		nBiosVersion[0] = (nVer & 0xff000000) >> 24;
		nBiosVersion[1] = (nVer & 0x00ff0000) >> 16;
		nBiosVersion[2] = (nVer & 0x0000ff00) >> 8;
		nBiosVersion[3] = (nVer & 0x000000ff) >> 0;
		m_nBIOSVersion = atoi( nBiosVersion );
		dbprintf( "i855:: BIOS version %d\n", (uint)m_nBIOSVersion );
	}
		
	/* Get the active pipe */
	m_nDisplayPipe = 0;
	if( m_bTwoPipes )
	{
		memset( &rm, 0, sizeof( struct RMREGS ) );
		rm.EAX = 0x5f1c;
		rm.EBX = 0x100;
		realint( 0x10, &rm );
	
		if( rm.EAX == 0x005f )
		{
			if( m_nBIOSVersion >= 3062 )
				m_nDisplayPipe = rm.EBX & 0x1;
			else
				m_nDisplayPipe = ( rm.ECX & 0100 ) >> 8;
		}
	}
	dbprintf( "i855:: Active display pipe %i\n", m_nDisplayPipe + 1 );
	
	/* Get the connected devices */
	m_nDisplayInfo = 0;
	memset( &rm, 0, sizeof( struct RMREGS ) );
	rm.EAX = 0x5f64;
	rm.EBX = 0x100;
	realint( 0x10, &rm );
	
	if( rm.EAX == 0x005f )
	{
		m_nDisplayInfo = rm.ECX & 0xffff;
	}
	
	/* Patch BIOS */
	if( bTweakBios )
		TweakMemorySize( nFd );
	
	
	/* Initialize the screenmodes */
	
	if( !InitModes() )
	{
		dbprintf( "i855:: Could not find vesa modes!\n" );
		return;
	}
	
	/* Initialize command ringbuffer */
	OUTREG( LP_RING + RING_LEN, 0 );
	OUTREG( LP_RING + RING_TAIL, 0 );
	OUTREG( LP_RING + RING_HEAD, 0 );
	OUTREG( LP_RING + RING_START, 0 );
	m_nRingPos = 0;
	m_nRingSpace = m_nRingSize - 8;
	
	/* Initialize memory manager */
	InitMemory( 8 * 1024 * 1024, m_nVideoEnd - 8 * 1024 * 1024 - m_nFrameBufferOffset, PAGE_SIZE - 1, 63 );
	
	m_bIsInitiated = true;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

i855::~i855()
{
	if (m_hRegisterArea != -1) {
		delete_area(m_hRegisterArea);
	}
	if (m_hRingArea != -1) {
		delete_area(m_hRingArea);
	}
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

void i855::TweakMemorySize( int nFd )
{
	struct RMREGS rm;
	memset( &rm, 0, sizeof( struct RMREGS ) );
	rm.EAX = 0x5f11;
	rm.ECX = 8 * 1024 * 1024 / PAGE_SIZE;
	realint( 0x10, &rm );
	
	if( rm.EAX != 0x005f )
	{
		dbprintf( "i855:: Warning: Failed to set bios memory size\n" );
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool i855::InitModes( void )
{
	Vesa_Info_s sVesaInfo;
	VESA_Mode_Info_s sModeInfo;
	uint16 anModes[1024];
	int nModeCount;

	int i = 0;

	strcpy( sVesaInfo.VesaSignature, "VBE2" );

	nModeCount = get_vesa_info( &sVesaInfo, anModes, 1024 );

	if( nModeCount <= 0 )
	{
		dbprintf( "Error: i855::InitModes() no VESA20 modes found\n" );
		return ( false );
	}
	
	dbprintf( "i855:: Detected %i kbytes video memory\n", sVesaInfo.TotalMemory * 1024 / 16 );

//    dbprintf( "Found %d vesa modes\n", nModeCount );

	int nPagedCount = 0;
	int nPlanarCount = 0;
	int nBadCount = 0;

	for( i = 0; i < nModeCount; ++i )
	{
		get_vesa_mode_info( &sModeInfo, anModes[i] );

		if( sModeInfo.PhysBasePtr == 0 )
		{		// We must have linear frame buffer
			nPagedCount++;
			continue;
		}
		if( sModeInfo.BitsPerPixel < 8 )
		{
			nPlanarCount++;
			continue;
		}
		if( sModeInfo.NumberOfPlanes != 1 )
		{
			nPlanarCount++;
			continue;
		}

		if( sModeInfo.BitsPerPixel != 15 && sModeInfo.BitsPerPixel != 16 && sModeInfo.BitsPerPixel != 32 )
		{
			nBadCount++;
			continue;
		}

		if( sModeInfo.RedMaskSize == 0 && sModeInfo.GreenMaskSize == 0 && sModeInfo.BlueMaskSize == 0 && sModeInfo.RedFieldPosition == 0 && sModeInfo.GreenFieldPosition == 0 && sModeInfo.BlueFieldPosition == 0 )
		{
			m_cModeList.push_back( i855Mode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine, CS_CMAP8, 60.0f, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
		}
		else if( sModeInfo.RedMaskSize == 5 && sModeInfo.GreenMaskSize == 6 && sModeInfo.BlueMaskSize == 5 && sModeInfo.RedFieldPosition == 11 && sModeInfo.GreenFieldPosition == 5 && sModeInfo.BlueFieldPosition == 0 )
		{
			m_cModeList.push_back( i855Mode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine, CS_RGB16, 60.0f, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
		}
		else if( sModeInfo.BitsPerPixel == 32 && sModeInfo.RedMaskSize == 8 && sModeInfo.GreenMaskSize == 8 && sModeInfo.BlueMaskSize == 8 && sModeInfo.RedFieldPosition == 16 && sModeInfo.GreenFieldPosition == 8 && sModeInfo.BlueFieldPosition == 0 )
		{
			m_cModeList.push_back( i855Mode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine, CS_RGB32, 60.0f, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
		}
		else
		{
			dbprintf( "i855:: Found unsupported video mode: %dx%d %d BPP %d BPL - %d:%d:%d, %d:%d:%d\n", sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BitsPerPixel, sModeInfo.BytesPerScanLine, sModeInfo.RedMaskSize, sModeInfo.GreenMaskSize, sModeInfo.BlueMaskSize, sModeInfo.RedFieldPosition, sModeInfo.GreenFieldPosition, sModeInfo.BlueFieldPosition );
		}
//#if 0
		dbprintf( "i855:: Mode %04x: %dx%d %d BPP %d BPL - %d:%d:%d, %d:%d:%d (%p)\n", anModes[i], sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BitsPerPixel, sModeInfo.BytesPerScanLine, sModeInfo.RedMaskSize, sModeInfo.GreenMaskSize, sModeInfo.BlueMaskSize, sModeInfo.RedFieldPosition, sModeInfo.GreenFieldPosition, sModeInfo.BlueFieldPosition, ( void * )sModeInfo.PhysBasePtr );
//#endif
	}
	dbprintf( "i855:: Found total of %d VESA modes. Valid: %d, Paged: %d, Planar: %d, Bad: %d\n", nModeCount, m_cModeList.size(), nPagedCount, nPlanarCount, nBadCount );
	return ( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

area_id i855::Open( void )
{
	return( m_hFrameBufferArea );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void i855::Close( void )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int i855::GetScreenModeCount()
{
	return ( m_cModeList.size() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool i855::GetScreenModeDesc( int nIndex, screen_mode * psMode )
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

int i855::SetScreenMode( screen_mode sMode )
{
	m_nCurrentMode = -1;

	for( int i = GetScreenModeCount() - 1; i >= 0; --i )
	{
		if( m_cModeList[i].m_nWidth == sMode.m_nWidth && m_cModeList[i].m_nHeight == sMode.m_nHeight && m_cModeList[i].m_eColorSpace == sMode.m_eColorSpace )
		{
			m_nCurrentMode = i;
			break;
		}

	}

	if( m_nCurrentMode >= 0 )
	{
		/* Stop video overlay */
		if ( m_bVideoOverlayUsed )
		{
			m_psVidRegs->OCMD &= ~OVERLAY_ENABLE;
			UpdateVideo();
			VideoOff();
			WaitForIdle();
		}
			
		/* Set screenmode */
		if( SetVesaMode( m_cModeList[m_nCurrentMode].m_nVesaMode, m_cModeList[m_nCurrentMode].m_nWidth ) )
		{
			
			/* Reinitialize command ringbuffer */
			OUTREG( LP_RING + RING_LEN, 0 );
			OUTREG( LP_RING + RING_TAIL, 0 );
			OUTREG( LP_RING + RING_HEAD, 0 );
			OUTREG( LP_RING + RING_START, 0 );
			OUTREG( LP_RING + RING_LEN, ( ( m_nRingSize - 4096 ) & I830_RING_NR_PAGES ) | RING_NO_REPORT | RING_VALID );
			m_nRingPos = 0;
			
			/* Put the framebuffer behind the ringbuffer */
			
			OUTREG( DSPASTRIDE, m_cModeList[m_nCurrentMode].m_nBytesPerLine );
			OUTREG( DSPASIZE, ( ( m_cModeList[m_nCurrentMode].m_nHeight - 1 ) << 16 ) | 
								 ( m_cModeList[m_nCurrentMode].m_nWidth - 1 ) );			
			OUTREG( DSPABASE, m_nFrameBufferOffset );
			uint32 nTemp = INREG( DSPABASE );
			OUTREG( DSPABASE, nTemp );
			if( m_bTwoPipes ) {
				OUTREG( DSPBSTRIDE, m_cModeList[m_nCurrentMode].m_nBytesPerLine );
				OUTREG( DSPBSIZE, ( ( m_cModeList[m_nCurrentMode].m_nHeight - 1 ) << 16 ) | 
								 ( m_cModeList[m_nCurrentMode].m_nWidth - 1 ) );				
				OUTREG( DSPBBASE, m_nFrameBufferOffset );
				uint32 nTemp = INREG( DSPBBASE );
				OUTREG( DSPBBASE, nTemp );
			}
			
			WaitForIdle();
			
			/* Reenable video overlay */
			if ( m_bVideoOverlayUsed )
			{
				SetupVideoOneLine();
				UpdateVideo();
			}
			m_bEngineDirty = true;
			
			#if 0
			dbprintf( "%x %x\n", (uint)INREG( DSPACNTR ), (uint)INREG( DSPBCNTR ) );
			dbprintf( "%x %x\n", (uint)INREG( PIPEACONF ), (uint)INREG( PIPEBCONF ) );
			dbprintf( "%x\n", (uint)INREG( LP_RING + RING_LEN ) );
			dbprintf( "BASE %x %x %x %x\n", (uint)INREG( DSPABASE ), (uint)( DSPASTRIDE ),
											(uint)INREG( DSPBBASE ), (uint)( DSPBSTRIDE ) );
			#endif
			return ( 0 );
		}
		
		
	}
	return ( -1 );
}

screen_mode i855::GetCurrentScreenMode()
{
	return ( m_cModeList[m_nCurrentMode] );
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void i855::LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrc, os::IRect cDst )
{
	if( ( pcDstBitmap->m_bVideoMem == false && ( pcSrcBitmap == NULL || pcSrcBitmap->m_bVideoMem == false ) ) || m_bEngineDirty == false )
		return;
	m_cGELock.Lock();
	WaitForIdle();	
	m_cGELock.Unlock();
	m_bEngineDirty = false;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


bool i855::DrawLine(SrvBitmap* pcBitMap, const IRect& cClipRect,
                      const IPoint& cPnt1, const IPoint& cPnt2, const Color32_s& sColor, int nMode)
{
	return DisplayDriver::DrawLine(pcBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode);
	
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool i855::FillRect(SrvBitmap *pcBitMap, const IRect& cRect, const Color32_s& sColor, int nMode)
{
	if ( pcBitMap->m_bVideoMem == false || nMode != DM_COPY ) {
		return DisplayDriver::FillRect(pcBitMap, cRect, sColor, nMode);
	}
	
	m_cGELock.Lock();
	
	os::screen_mode sMode = GetCurrentScreenMode();
	
	if( !cRect.IsValid() || cRect.Width() > 2000 || cRect.Height() > 2000
		|| cRect.left < 0 || cRect.top < 0 )
	{
		dbprintf( "Error: Invalid FILL!\n" );
	}

	
	uint32 nBR13 = pcBitMap->m_nBytesPerLine | ( 0xF0 << 16 );
	int nSize = cRect.Width() + 1;
	int nBpp;
	uint32 nColor;
	
	BEGIN_RING( 6 );
	if( GetCurrentScreenMode().m_eColorSpace == os::CS_RGB32 ) {
		nBR13 |= ((1 << 25) | (1 << 24));
		OUTRING( COLOR_BLT_CMD | COLOR_BLT_WRITE_ALPHA |
		  COLOR_BLT_WRITE_RGB );
		 nSize *= 4;
		 nBpp = 4;
		 nColor = COL_TO_RGB32(sColor);
	} else {
		nBR13 |= (1 << 24);
		OUTRING(COLOR_BLT_CMD);
		nSize *= 2;
		nBpp = 2;
		nColor = COL_TO_RGB16(sColor);
	}
	OUTRING( nBR13 );
	OUTRING(((cRect.Height()+1) << 16) | (nSize & 0xffff));
	OUTRING(m_nFrameBufferOffset + pcBitMap->m_nVideoMemOffset + cRect.top * pcBitMap->m_nBytesPerLine + cRect.left * nBpp );
	OUTRING(nColor);
	OUTRING(0);
	FLUSH_RING();
	
	m_bEngineDirty = true;
	m_cGELock.Unlock();
	


	return( true );
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool i855::BltBitmap(SrvBitmap *pcDstBitMap, SrvBitmap *pcSrcBitMap,
                       IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha)
{
	if (pcDstBitMap->m_bVideoMem == false || pcSrcBitMap->m_bVideoMem == false || 
			nMode != DM_COPY || cSrcRect.Size() != cDstRect.Size() ) {
		return DisplayDriver::BltBitmap(pcDstBitMap, pcSrcBitMap, cSrcRect, cDstRect, nMode, nAlpha);
	}
	
	IPoint cDstPos = cDstRect.LeftTop();
	
	if( !cSrcRect.IsValid() || !cDstRect.IsValid() || cDstPos.x < 0 || cDstPos.y < 0 || cSrcRect.Width() > 2000 || cSrcRect.Height() > 2000
		|| cSrcRect.left < 0 || cSrcRect.top < 0 )
	{
		dbprintf( "Error: Invalid BLIT!\n" );
	}
	
	m_cGELock.Lock();
	
	os::screen_mode sMode = GetCurrentScreenMode();
	
	uint32 nBR13 = pcDstBitMap->m_nBytesPerLine | ( 0xCC << 16 );
	int nDstX2 = cDstPos.x + cSrcRect.Width() + 1;
	int nDstY2 = cDstPos.y + cSrcRect.Height() + 1;
	
	BEGIN_RING( 8 );
	if( GetCurrentScreenMode().m_eColorSpace == os::CS_RGB32 ) {
		nBR13 |= ((1 << 25) | (1 << 24));
		OUTRING( XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
		  XY_SRC_COPY_BLT_WRITE_RGB );
	} else {
		nBR13 |= (1 << 24);
		OUTRING(XY_SRC_COPY_BLT_CMD);
	}
	OUTRING( nBR13 );
	OUTRING((cDstPos.y << 16) | (cDstPos.x & 0xffff));
	OUTRING((nDstY2 << 16) | (nDstX2 & 0xffff));
	OUTRING(m_nFrameBufferOffset + pcDstBitMap->m_nVideoMemOffset);
	OUTRING((cSrcRect.top << 16) | (cSrcRect.left & 0xffff));
	OUTRING( pcSrcBitMap->m_nBytesPerLine & 0xFFFF );
	OUTRING(m_nFrameBufferOffset + pcSrcBitMap->m_nVideoMemOffset);
	FLUSH_RING();
	
	m_bEngineDirty = true;
	m_cGELock.Unlock();
	
	return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool i855::SetVesaMode( uint32 nMode, int nWidth )
{
	struct RMREGS rm;

	memset( &rm, 0, sizeof( struct RMREGS ) );

	rm.EAX = 0x4f02;
	rm.EBX = nMode;

	realint( 0x10, &rm );
	int nResult = rm.EAX & 0xffff;

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
	rm.ECX = nWidth;
	rm.EAX = 0x4f06;
	realint( 0x10, &rm );

	return ( nResult );
}

void i855::WaitForIdle()
{
	/* Put flush commands */
	BEGIN_RING( 2 );
	OUTRING( MI_FLUSH | MI_WRITE_DIRTY_STATE | MI_INVALIDATE_MAP_CACHE );
	OUTRING( MI_NOOP );
    FLUSH_RING();
	
	/* Wait for the commands to be processed */
	bool bTimeout = false;
	bigtime_t nLastTime = get_system_time();
	uint32 nLastHead = INREG( LP_RING + RING_HEAD ) & I830_HEAD_MASK;
	do {
		nHead = INREG( LP_RING + RING_HEAD ) & I830_HEAD_MASK;
		nTail = INREG( LP_RING + RING_TAIL ) & I830_TAIL_MASK;
		
		if( nHead != nLastHead )
		{
			nLastHead = nHead;
			nLastTime = get_system_time();
		}
		if( get_system_time() - nLastTime > 2000000 ) {
			if( nHead == nTail )
				return;
			dbprintf( "i855:: Engine timed out! %i %i\n", nHead, nTail );	
			bTimeout = true;
			OUTREG( LP_RING + RING_HEAD, 0 );
			OUTREG( LP_RING + RING_TAIL, 0 );
			m_nRingPos = 0;
			dbprintf( "Dump buffer...\n" );
			for( uint i = 0; i < m_nRingSize / 4; i++ )
			{
				dbprintf( "Value %x %x\n", i * 4, ((uint32*)m_pRingBase)[i] );
			}
		}
	} while( ( nHead != nTail ) && !bTimeout );
}

//-----------------------------------------------------------------------------

extern "C" DisplayDriver* init_gfx_driver( int nFd )
{
    dbprintf("i855 attempts to initialize\n");
    
    try {
	    i855 *pcDriver = new i855( nFd );
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
