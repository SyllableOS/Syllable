/*
 * Syllable Driver for SiS chipsets
 *
 * Syllable specific code is
 * Copyright (c) 2003, Arno Klenke
 *
 * Other code is
 * Copyright (c) Thomas Winischhofer <thomas@winischhofer.net>:
 * Copyright (c) SiS (www.sis.com.tw)
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holder not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holder makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <appserver/pci_graphics.h>

#undef HAVE_MTRR
#ifdef HAVE_MTRR
#include <atheos/mtrr.h>
#endif
#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <gui/bitmap.h>

#include "sis3xx.h"
#include "sis_cursor.h"

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

SiS3xx::SiS3xx( int nFd ) : m_cGELock("sis3xx_ge_lock"), m_cCursorHotSpot(0,0)
{	
	m_bIsInitiated = false;
	m_bCursorIsOn = false;
	
	/* Get Info */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_cPCIInfo ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}
	
	dbprintf("A supported card seems to be installed!\n");
	
	/* Map ROM */
	m_pRomBase = NULL;
	m_hRomArea = create_area("sis3xx_rom", (void**)&m_pRomBase, 0x30000,
	                              AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hRomArea, (void*)(0x000c0000));
	
	
	/* Map MMIO and IO ports */
	m_pRegisterBase = NULL;
	int nIoSize = get_pci_memory_size(nFd, &m_cPCIInfo, 1); 
	m_hRegisterArea = create_area("sis3xx_regs", (void**)&m_pRegisterBase, nIoSize,
	                              AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hRegisterArea, (void*)(m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK));
	
	/* Map Framebuffer */
	m_pFrameBufferBase = NULL;
	int nMemSize = get_pci_memory_size(nFd, &m_cPCIInfo, 0); 
	m_hFrameBufferArea = create_area("sis3xx_framebuffer", (void**)&m_pFrameBufferBase,
	                                 nMemSize, AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hFrameBufferArea, (void*)(m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK));
	
	/* Initialize shared info */
	m_pHw = &si;
	m_pHw->nFd = nFd;
	m_pHw->pci_dev = m_cPCIInfo;
	m_pHw->device_id = m_cPCIInfo.nDeviceID;
	m_pHw->regs = ( vuint32* )m_pRegisterBase;
	m_pHw->framebuffer = (uint8*)m_pFrameBufferBase;
	m_pHw->io_base = (uint32)(m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_IO_MASK);
	m_pHw->rom = ( void* )m_pRomBase;
	m_pHw->filter = 0;
	dbprintf("MMIO @ 0x%x IO @ 0x%x\n", (uint)(m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK), 
										(uint)(m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_IO_MASK) ); 
	dbprintf("Framebuffer @ 0x%x\n", (uint)(m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK) ); 
	
	if( sis_init() != 0 ) {
		/* We can not do anything here */
		dbprintf( "Fatal error : Could not initialize card!!!" );
		return;
	}
	
	os::color_space colspace[] = {CS_RGB16, CS_RGB32};
	int bpp[] = {2, 4};
	float rf[] = { 60.0f, 75.0f, 85.0f, 100.0f };
	for (int i=0; i<int(sizeof(bpp)/sizeof(bpp[0])); i++) {
		for( int j = 0; j < 4; j++ ) {
		m_cScreenModeList.push_back(os::screen_mode(640, 480, 640*bpp[i], colspace[i], rf[j]));
		m_cScreenModeList.push_back(os::screen_mode(800, 600, 800*bpp[i], colspace[i], rf[j]));
		m_cScreenModeList.push_back(os::screen_mode(1024, 768, 1024*bpp[i], colspace[i], rf[j]));
		m_cScreenModeList.push_back(os::screen_mode(1280, 1024, 1280*bpp[i], colspace[i], rf[j]));
		m_cScreenModeList.push_back(os::screen_mode(1600, 1200, 1600*bpp[i], colspace[i], rf[j]));
		}
	}
	/* Initialize HW cursor */
	m_bUsingHWCursor = true;
	m_pHw->mem_size -= 64 * 64 * 4;
	SetMousePos( os::IPoint( 0, 0 ) );
	
	memset(m_anCursorShape, 0, sizeof(m_anCursorShape));
	
	#ifdef HAVE_MTRR
	/* Initialize MTRRs if possible */
	MTRR_ioctl_s mtrr = { 1, m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK, nMemSize,
							MTRR_WRITE_COMBINE };
	int mtrr_fd = open( "/dev/misc/mtrr", O_RDWR );
	if( mtrr_fd >= 0 ) {
		ioctl( mtrr_fd, MTRR_SET_NEXT, &mtrr );
		close( mtrr_fd );
	}
	#endif
	

	m_bIsInitiated = true;
}

SiS3xx::~SiS3xx()
{
	if (m_hRegisterArea != -1) {
		delete_area(m_hRegisterArea);
	}
	if (m_hFrameBufferArea != -1) {
		delete_area(m_hFrameBufferArea);
	}
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

area_id SiS3xx::Open()
{
	return m_hFrameBufferArea;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void SiS3xx::Close()
{
}


int SiS3xx::GetScreenModeCount()
{
	return m_cScreenModeList.size();
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool SiS3xx::GetScreenModeDesc(int nIndex, os::screen_mode* psMode)
{
	if (nIndex<0 || uint(nIndex)>=m_cScreenModeList.size()) {
		return false;
	}
	*psMode = m_cScreenModeList[nIndex];
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

int SiS3xx::SetScreenMode( os::screen_mode sMode )
{
	int bpp = (sMode.m_eColorSpace == CS_RGB32) ? 4 : 2;
	int mode_idx = 0;
	int rate_idx = 0;
	uint8 mode_no = 0;
	int found_mode = 0;
	int refresh_rate;
	uint16 xres, yres;
	int i = 0;
	
	/* Search mode */
	while( (sisbios_mode[mode_idx].mode_no != 0) &&
	       (sisbios_mode[mode_idx].xres <= sMode.m_nWidth) ) {
		if( (sisbios_mode[mode_idx].xres == sMode.m_nWidth) &&
		    (sisbios_mode[mode_idx].yres == sMode.m_nHeight) &&
		    (sisbios_mode[mode_idx].bpp == bpp * 8) &&
		    (sisbios_mode[mode_idx].chipset & m_pHw->vga_engine)) {
			mode_no = sisbios_mode[mode_idx].mode_no;
			found_mode = 1;
			break;
		}
		mode_idx++;
	}
	if( !found_mode ) {
		dbprintf("Error setting video mode!\n");
		return -1;
	}
	refresh_rate = (int)(sMode.m_vRefreshRate);
	
	/* Search refresh rate */
	xres = sisbios_mode[mode_idx].xres;
	yres = sisbios_mode[mode_idx].yres;

	while ((sis_vrate[i].idx != 0) && (sis_vrate[i].xres <= xres)) {
		if ((sis_vrate[i].xres == xres) && (sis_vrate[i].yres == yres)) {
			if (sis_vrate[i].refresh == refresh_rate) {
				rate_idx = sis_vrate[i].idx;
				break;
			} else if (sis_vrate[i].refresh > refresh_rate) {
				if ((sis_vrate[i].refresh - refresh_rate) <= 2) {
					rate_idx = sis_vrate[i].idx;
				} else if (((refresh_rate - sis_vrate[i-1].refresh) <= 2)
						&& (sis_vrate[i].idx != 1)) {
					rate_idx = sis_vrate[i-1].idx;
				}
				break;
			}
		}
		i++;
	}
	if( rate_idx <= 0 ) {
		dbprintf("Error setting video mode ( could not find refesh rate )!\n");
		return -1;
	}
	sis_pre_setmode( rate_idx );
	if( SiSSetMode( &SiS_Pr, &sishw_ext, mode_no ) == 0 ) {
		dbprintf( "Setting mode[0x%x] failed\n", mode_no );
		return -1;
	}
	outSISIDXREG( SISSR, IND_SIS_PASSWORD, SIS_PASSWORD );
	sis_post_setmode( bpp * 8, sMode.m_nWidth );
	outSISIDXREG( SISSR, IND_SIS_PASSWORD, SIS_PASSWORD );
	
	switch( bpp ) {
		case 2:
			m_pHw->AccelDepth = 0x00010000;
			m_pHw->DstColor = 0x8000;
		break;
		case 4:
			m_pHw->AccelDepth = 0x00020000;
			m_pHw->DstColor = 0xC000;
		break;
		default:
			dbprintf( "Unsupported color depth!\n" );
		break;
	}
	m_sCurrentMode = sMode;
	m_sCurrentMode.m_nBytesPerLine = m_sCurrentMode.m_nWidth * bpp;
	
	si.width = m_sCurrentMode.m_nWidth;
	si.height = m_sCurrentMode.m_nHeight;
	si.bpp = bpp;
	
	/* Initialize Video overlay */
	SISSetupImageVideo();
	m_bVideoOverlayUsed = false;
	
	return 0;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

os::screen_mode SiS3xx::GetCurrentScreenMode()
{
	return m_sCurrentMode;
}
//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool SiS3xx::IntersectWithMouse(const IRect& cRect)
{
	return false;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void SiS3xx::SetCursorBitmap(os::mouse_ptr_mode eMode, const os::IPoint& cHotSpot,
                             const void *pRaster, int nWidth, int nHeight)
{
	m_cCursorHotSpot = cHotSpot;
	int maxcurs = ( m_pHw->vga_engine == SIS_315_VGA ) ? 64 : 32;
	if (eMode != MPTR_MONO || nWidth > maxcurs || nHeight > maxcurs) {
		if( m_bUsingHWCursor ) {
			MouseOff();
			m_bCursorIsOn = true;
		}
		m_bUsingHWCursor = false;
		return DisplayDriver::SetCursorBitmap(eMode, cHotSpot, pRaster, nWidth, nHeight);
	}
	
	if( !m_bUsingHWCursor ) {
		DisplayDriver::MouseOff();
		m_bUsingHWCursor = true;
		MouseOn();
	}
	
	int nCurAddr = m_pHw->mem_size / 1024;
	const uint8 *pnSrc = (const uint8*)pRaster;
	uint8 *nTemp = m_pHw->framebuffer + m_pHw->mem_size;
	uint32 *pnDst = (uint32*)nTemp;
	uint32 *pnSaved = (uint32*)&m_anCursorShape[0];
	static uint32 anPalette310[] =
	{ 0x00000000, 0xFF000000, 0xFF000000, 0xFFFFFFFF };
	static uint32 anPalette300[] =
	{ 0xff000000, 0x00000000, 0x00000000, 0x00FFFFFF };
	
	
	for (int y = 0; y < maxcurs; y++) {
		for (int x = 0; x < maxcurs; x++, pnSaved++) {
			if (y >= nHeight || x >= nWidth) {
				*pnSaved = ( m_pHw->vga_engine == SIS_315_VGA ) ? 
							anPalette310[0] : anPalette300[0];
			} else {
				*pnSaved = ( m_pHw->vga_engine == SIS_315_VGA ) ? 
							anPalette310[*pnSrc++] : anPalette300[*pnSrc++];
			}
		}
	}
	
	pnSaved = (uint32*)&m_anCursorShape[0];
	for (int i = 0; i < maxcurs * maxcurs; i++) {
		*pnDst++ = *pnSaved++;
	}
	
	if( m_pHw->vga_engine == SIS_315_VGA ) {
		sis310SwitchToRGBCursor()
		sis310SetCursorBGColor( 0x00000000 )
		sis310SetCursorFGColor( 0xFFFFFFFF )
		sis310SetCursorAddress( nCurAddr )
		sis310SetCursorPatternSelect( 0 )
		sis310EnableHWARGBCursor()
		if( m_pHw->disp_state & DISPTYPE_DISP2 ) {
    		sis301SwitchToRGBCursor310()
			sis301SetCursorBGColor310( 0x00000000 )
			sis301SetCursorFGColor310( 0xFFFFFFFF )
			sis301SetCursorAddress310( nCurAddr )
			sis301SetCursorPatternSelect310( 0 )
			sis301EnableHWARGBCursor310()
    	}
	} else {
		sis300SwitchToRGBCursor()
		sis300SetCursorBGColor( 0x00000000 )
		sis300SetCursorFGColor( 0xFFFFFFFF )
		sis300SetCursorAddress( nCurAddr )
		sis300SetCursorPatternSelect( 0 )
		sis300EnableHWARGBCursor()
		if( m_pHw->disp_state & DISPTYPE_DISP2 ) {
    		sis301SwitchToRGBCursor();
			sis301SetCursorBGColor( 0x00000000 )
			sis301SetCursorFGColor( 0xFFFFFFFF )
			sis301SetCursorAddress( nCurAddr )
			sis301SetCursorPatternSelect( 0 )
			sis301EnableHWARGBCursor()
    	}
	}
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


void SiS3xx::SetMousePos(os::IPoint cNewPos)
{
	if( !m_bUsingHWCursor )
		return DisplayDriver::SetMousePos( cNewPos );
	m_cCursorPos = cNewPos;
	unsigned char xpre = 0;
	unsigned char ypre = 0;
	
	int xp = cNewPos.x - m_cCursorHotSpot.x;
	int yp = cNewPos.y - m_cCursorHotSpot.y;
	/* clamp cursor to virtual display */
	if( xp >= m_sCurrentMode.m_nWidth ) xp =m_sCurrentMode.m_nWidth - 1;
	if( yp >= m_sCurrentMode.m_nHeight ) yp = m_sCurrentMode.m_nHeight - 1;
	if( xp < 0 ) {
		xpre = (-xp);
		xp = 0;
	}
	if( yp < 0 ) {
		ypre = (-yp);
		yp = 0;
	}

	if( m_pHw->vga_engine == SIS_315_VGA ) {
		sis310SetCursorPositionX( xp, xpre )
    	sis310SetCursorPositionY( yp, ypre )
    	if( m_pHw->disp_state & DISPTYPE_DISP2 ) {
    		sis301SetCursorPositionX310( xp + 17, xpre )
    		sis301SetCursorPositionY310( yp, ypre )
    	}
    } else {
    	sis300SetCursorPositionX( xp, xpre )
    	sis300SetCursorPositionY( yp, ypre )
    	if( m_pHw->disp_state & DISPTYPE_DISP2 ) {
    		sis301SetCursorPositionX( xp + 13, xpre )
    		sis301SetCursorPositionY( yp, ypre )
    	}
    }
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


void SiS3xx::MouseOn()
{
	if( !m_bUsingHWCursor )
		return DisplayDriver::MouseOn();
	if( m_pHw->vga_engine == SIS_315_VGA ) {
		sis310EnableHWARGBCursor()
		if( m_pHw->disp_state & DISPTYPE_DISP2 ) {
			sis301EnableHWARGBCursor310()
		}
	} else {
		sis300EnableHWARGBCursor()
		if( m_pHw->disp_state & DISPTYPE_DISP2 ) {
			sis301EnableHWARGBCursor()
		}
	}
	m_bCursorIsOn = true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


void SiS3xx::MouseOff()
{
	if( !m_bUsingHWCursor )
		return DisplayDriver::MouseOff();
	if( m_pHw->vga_engine == SIS_315_VGA ) {
		sis310DisableHWCursor()
		if( m_pHw->disp_state & DISPTYPE_DISP2 ) {
			sis301DisableHWCursor310()
		}
	} else {
		sis300DisableHWCursor()
		if( m_pHw->disp_state & DISPTYPE_DISP2 ) {
			sis301DisableHWCursor()
		}
	}
	m_bCursorIsOn = false;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


bool SiS3xx::DrawLine(SrvBitmap* pcBitMap, const IRect& cClipRect,
                      const IPoint& cPnt1, const IPoint& cPnt2, const Color32_s& sColor, int nMode)
{
	if (pcBitMap->m_bVideoMem == false || nMode != DM_COPY  ) {
		return DisplayDriver::DrawLine(pcBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode);
	}
	
	int x1 = cPnt1.x;
	int y1 = cPnt1.y;
	int x2 = cPnt2.x;
	int y2 = cPnt2.y;
	
	if (DisplayDriver::ClipLine(cClipRect, &x1, &y1, &x2, &y2) == false) {
		return false;
	}

	uint32 nColor;
	if (pcBitMap->m_eColorSpc == CS_RGB32) {
		nColor = COL_TO_RGB32(sColor);
	} else {
		// we only support CS_RGB32 and CS_RGB16
		nColor = COL_TO_RGB16(sColor);
	}
	
	m_cGELock.Lock();
	
	if( m_pHw->vga_engine == SIS_315_VGA ) {
		SiS310SetupLineCount( 1 )
		SiS310SetupPATFG( nColor )
		SiS310SetupDSTRect( m_sCurrentMode.m_nBytesPerLine, -1 );
		SiS310SetupDSTColorDepth( m_pHw->DstColor );
		SiS310SetupROP( 0xF0 )
		SiS310SetupCMDFlag( PATFG | LINE | m_pHw->AccelDepth )
	} else {
		SiS300SetupLineCount( 1 )
		SiS300SetupPATFG( nColor )
		SiS300SetupDSTRect( m_sCurrentMode.m_nBytesPerLine, -1 );
		SiS300SetupDSTColorDepth( m_pHw->DstColor );
		SiS300SetupROP( 0xF0 )
		SiS300SetupCMDFlag( PATFG | LINE )
	}
	
	long nDstBase = 0;
	int nMinY = ( y1 > y2 ) ? y2 : y1;
	int nMaxY = ( y1 > y2 ) ? y1 : y2;
	if( nMaxY >= 2048 ) {
		nDstBase = m_sCurrentMode.m_nBytesPerLine * nMaxY;
		y1 -= nMinY;
		y2 -= nMinY;
	}
	
	if( m_pHw->vga_engine == SIS_315_VGA ) {
		SiS310SetupDSTBase( nDstBase )
		SiS310SetupX0Y0( x1, y1 )
		SiS310SetupX1Y1( x2, y2 )
		m_pHw->CmdReg &= ~( NO_LAST_PIXEL );
		SiS310DoCMD
		SiS310Idle
	} else {
		SiS300SetupDSTBase( nDstBase )
		SiS300SetupX0Y0( x1, y1 )
		SiS300SetupX1Y1( x2, y2 )
		m_pHw->CmdReg &= ~( NO_LAST_PIXEL );
		SiS300DoCMD
		SiS300Idle
	}
	
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool SiS3xx::FillRect(SrvBitmap *pcBitMap, const IRect& cRect, const Color32_s& sColor)
{
	if (pcBitMap->m_bVideoMem == false) {
		return DisplayDriver::FillRect(pcBitMap, cRect, sColor);
	}
	
	int nWidth = cRect.Width()+1;
	int nHeight = cRect.Height()+1;
	uint32 nColor;
	if (pcBitMap->m_eColorSpc == CS_RGB32 || 
		( ( COL_TO_RGB32(sColor) & 0xffffff ) ==  ( m_nColorKey & 0xffffff ) ) ) {
		nColor = COL_TO_RGB32(sColor);
	} else {
		// we only support CS_RGB32 and CS_RGB16
		nColor = COL_TO_RGB16(sColor);
	}
	
	m_cGELock.Lock();
	
	if( m_pHw->vga_engine == SIS_315_VGA ) {
		SiS310SetupPATFG( nColor )
		SiS310SetupDSTRect( m_sCurrentMode.m_nBytesPerLine, -1 );
		SiS310SetupDSTColorDepth( m_pHw->DstColor );
		SiS310SetupROP( 0xF0 )
		SiS310SetupCMDFlag( PATFG | m_pHw->AccelDepth )
	} else {
		SiS300SetupPATFG( nColor )
		SiS300SetupDSTRect( m_sCurrentMode.m_nBytesPerLine, -1 );
		SiS300SetupDSTColorDepth( m_pHw->DstColor );
		SiS300SetupROP( 0xF0 )
		SiS300SetupCMDFlag( PATFG )
	}
	
	long nDstBase = 0;
	if( cRect.top >= 2048 ) {
		nDstBase = m_sCurrentMode.m_nBytesPerLine * cRect.top;
	}
	if( m_pHw->vga_engine == SIS_315_VGA ) {
		SiS310SetupDSTBase( nDstBase )
		SiS310SetupDSTXY( cRect.left, ( cRect.top < 2048 ) ? cRect.top : 0 )
		SiS310SetupRect( nWidth, nHeight )
		SiS310SetupCMDFlag( BITBLT )
		SiS310DoCMD
		SiS310Idle
	} else {
		SiS300SetupDSTBase( nDstBase )
		SiS300SetupDSTXY( cRect.left, ( cRect.top < 2048 ) ? cRect.top : 0 )
		SiS300SetupRect( nWidth, nHeight )
		SiS300SetupCMDFlag( BITBLT | X_INC | Y_INC )
		SiS300DoCMD
		SiS300Idle
	}
	
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool SiS3xx::BltBitmap(SrvBitmap *pcDstBitMap, SrvBitmap *pcSrcBitMap,
                       IRect cSrcRect, IPoint cDstPos, int nMode)
{
	if (pcDstBitMap->m_bVideoMem == false || pcSrcBitMap->m_bVideoMem == false || nMode != DM_COPY) {
		return DisplayDriver::BltBitmap(pcDstBitMap, pcSrcBitMap, cSrcRect, cDstPos, nMode);
	}
		
	int nWidth = cSrcRect.Width()+1;
	int nHeight = cSrcRect.Height()+1;
	m_cGELock.Lock();
	
	if( m_pHw->vga_engine == SIS_315_VGA ) {
		SiS310SetupDSTColorDepth( m_pHw->DstColor );
		SiS310SetupSRCPitch( m_sCurrentMode.m_nBytesPerLine );
		SiS310SetupDSTRect( m_sCurrentMode.m_nBytesPerLine, -1 );
		SiS310SetupROP( 0xCC );
		SiS310SetupClipLT( 0, 0 );
		SiS310SetupClipRB( (m_sCurrentMode.m_nWidth - 1), (m_sCurrentMode.m_nHeight - 1) );
		SiS310SetupCMDFlag( m_pHw->AccelDepth )
		
	} else {
		SiS300SetupDSTColorDepth( m_pHw->DstColor );
		SiS300SetupSRCPitch( m_sCurrentMode.m_nBytesPerLine );
		SiS300SetupDSTRect( m_sCurrentMode.m_nBytesPerLine, -1 );
		SiS300SetupROP( 0xCC );
		SiS300SetupClipLT( 0, 0 );
		SiS300SetupClipRB( (m_sCurrentMode.m_nWidth - 1), (m_sCurrentMode.m_nHeight - 1) );
	}
	
	long nSrcBase = 0;
	if( cSrcRect.top >= 2048 ) {
		nSrcBase = m_sCurrentMode.m_nBytesPerLine * cSrcRect.top;
	}
	
	long nDstBase = 0;
	if( cDstPos.y >= 2048 ) {
		nDstBase = m_sCurrentMode.m_nBytesPerLine * cDstPos.y;
	}
	if( m_pHw->vga_engine == SIS_315_VGA ) {
		SiS310SetupSRCBase( nSrcBase )
		SiS310SetupDSTBase( nDstBase )
		SiS310SetupRect( nWidth, nHeight )
		SiS310SetupSRCXY( cSrcRect.left, ( cSrcRect.top < 2048 ) ? cSrcRect.top : 0 )
		SiS310SetupDSTXY( cDstPos.x, ( cDstPos.y < 2048 ) ? cDstPos.y : 0 )
		SiS310SetupRect( nWidth, nHeight )
		SiS310DoCMD
		SiS310Idle
	} else {
		int src_left = cSrcRect.left;
		int src_top = ( cSrcRect.top < 2048 ) ? cSrcRect.top : 0;
		int dest_left = cDstPos.x;
		int dest_top = ( cDstPos.y < 2048 ) ? cDstPos.y : 0;
		
		if( src_left > dest_left )
			SiS300SetupCMDFlag( X_INC )
		else {
			src_left += (nWidth - 1);
			dest_left += (nWidth - 1);
		}
		if( src_top > dest_top )
			SiS300SetupCMDFlag( Y_INC )
		else {
			src_top += (nHeight - 1);
			dest_top += (nHeight - 1);
		}
		
		SiS300SetupSRCBase( nSrcBase );
		SiS300SetupDSTBase( nDstBase );
		SiS300SetupRect( nWidth, nHeight )
		SiS300SetupSRCXY( src_left, src_top )
		SiS300SetupDSTXY( dest_left, dest_top )
		SiS300DoCMD
		SiS300Idle
	}
	
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool SiS3xx::CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer )
{
	if( eFormat == CS_YUV12 && !m_bVideoOverlayUsed ) {
		m_nColorKey = COL_TO_RGB32( sColorKey );
		*pBuffer = SISStartVideo( 0, 0, cDst.left, cDst.top, cSize.x, cSize.y, cDst.Width(), 
							cDst.Height(), 0x01, cSize.x, cSize.y, m_nColorKey );
		m_bVideoOverlayUsed = true;
		return( true );
	} else if ( eFormat == CS_YUV422 && !m_bVideoOverlayUsed ) {
		m_nColorKey = COL_TO_RGB32( sColorKey );
		*pBuffer = SISStartVideo( 0, 0, cDst.left, cDst.top, cSize.x, cSize.y, cDst.Width(), 
							cDst.Height(), 0x02, cSize.x, cSize.y, m_nColorKey );
		m_bVideoOverlayUsed = true;
		return( true );
	}
	return( false );
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool SiS3xx::RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, area_id *pBuffer )
{
	
	if( eFormat == CS_YUV12 ) {
		delete_area( *pBuffer );
		*pBuffer = SISStartVideo( 0, 0, cDst.left, cDst.top, cSize.x, cSize.y, cDst.Width(), 
							cDst.Height(), 0x01, cSize.x, cSize.y, m_nColorKey );
		m_bVideoOverlayUsed = true;
		return( true );
	} else if( eFormat == CS_YUV422 ) {
		delete_area( *pBuffer );
		*pBuffer = SISStartVideo( 0, 0, cDst.left, cDst.top, cSize.x, cSize.y, cDst.Width(), 
							cDst.Height(), 0x02, cSize.x, cSize.y, m_nColorKey );
		m_bVideoOverlayUsed = true;
		return( true );
	}
	return( false );
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void SiS3xx::UpdateVideoOverlay( area_id *pBuffer )
{
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void SiS3xx::DeleteVideoOverlay( area_id *pBuffer )
{
	if( m_bVideoOverlayUsed ) {
		delete_area( *pBuffer );
		SISStopVideo();
	}
	m_bVideoOverlayUsed = false;
}


//-----------------------------------------------------------------------------

extern "C" DisplayDriver* init_gfx_driver( int nFd )
{
    dbprintf("sis3xx attempts to initialize\n");
    
    try {
	    SiS3xx *pcDriver = new SiS3xx( nFd );
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
































