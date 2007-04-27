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

#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <gui/bitmap.h>

#include "sis3xx.h"

static uint32 dummybuf;
#define BITMASK(h,l)		(((unsigned)(1U << ((h) - (l) + 1)) - 1) << (l))
#define GENMASK(mask)		BITMASK(1 ? mask, 0 ? mask)

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

SiS3xx::SiS3xx( int nFd ) : m_cGELock("sis3xx_ge_lock")
{	
	m_bIsInitiated = false;
	m_bEngineDirty = false;
	
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
	                                 nMemSize, AREA_FULL_ACCESS | AREA_WRCOMB, AREA_NO_LOCK);
	remap_area(m_hFrameBufferArea, (void*)(m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK));
	
	/* Initialize shared info */
	m_pHw = &si;
	m_pHw->fd = nFd;
	m_pHw->pci_dev = m_cPCIInfo;
	m_pHw->device_id = m_cPCIInfo.nDeviceID;
	m_pHw->regs = ( vuint32* )m_pRegisterBase;
	m_pHw->framebuffer = (uint8*)m_pFrameBufferBase;
	m_pHw->io_base = (uint32)(m_cPCIInfo.u.h0.nBase2 & PCI_ADDRESS_IO_MASK);
	m_pHw->rom = ( void* )m_pRomBase;
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
		m_cScreenModeList.push_back(os::screen_mode(1280, 800, 1280*bpp[i], colspace[i], rf[j]));
		m_cScreenModeList.push_back(os::screen_mode(1280, 1024, 1280*bpp[i], colspace[i], rf[j]));
		m_cScreenModeList.push_back(os::screen_mode(1600, 1200, 1600*bpp[i], colspace[i], rf[j]));
		}
	}
	
	if( m_pHw->video_size > 1024 * 1024 * 8 )
		InitMemory( 1024 * 1024 * 8, m_pHw->video_size - 1024 * 1024 * 8, PAGE_SIZE - 1, 63 );
	
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
		    (sisbios_mode[mode_idx].chipset & m_pHw->sisvga_engine)) {
			mode_no = sisbios_mode[mode_idx].mode_no[m_pHw->sisvga_engine-1];
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
	si.mode_idx = mode_idx;
	si.rate_idx = rate_idx;
	si.video_width = xres;
	si.video_height = yres;
	
	sis_pre_setmode( rate_idx );
	if( SiSSetMode( &SiS_Pr, mode_no ) == 0 ) {
		dbprintf( "Setting mode[0x%x] failed\n", mode_no );
		return -1;
	}
	outSISIDXREG( SISSR, IND_SIS_PASSWORD, SIS_PASSWORD );
	sis_post_setmode();
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
	
	sis_set_pitch( m_sCurrentMode.m_nBytesPerLine );
	
	si.bpp = bpp;
	
	/* Initialize Video overlay */
	SISSetupImageVideo();
	m_bVideoOverlayUsed = false;
	m_bEngineDirty = true;
	
	/* Disable dual pipe */
	if( si.chip == XGI_40 ) {
		SiSIdle;
		SiSDualPipe(1);
		SiSIdle;
	}
	
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

void SiS3xx::LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrc, os::IRect cDst )
{
	if( ( pcDstBitmap->m_bVideoMem == false && ( pcSrcBitmap == NULL || pcSrcBitmap->m_bVideoMem == false ) ) || m_bEngineDirty == false )
		return;
	m_cGELock.Lock();
	if( m_pHw->sisvga_engine == SIS_315_VGA ) {
		SiSIdle
	} else {
		SiS300Idle
	}
	m_cGELock.Unlock();
	m_bEngineDirty = false;
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
	if (pcBitMap->m_bVideoMem == false || nMode != DM_COPY ) {
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
	
	if( m_pHw->sisvga_engine == SIS_315_VGA ) {
		SiSSetupDSTColorDepth(m_pHw->AccelDepth);
		SiSCheckQueue(16 * 3);
		SiSSetupLineCountPeriod(1, 1)
		SiSSetupPATFGDSTRect( nColor, pcBitMap->m_nBytesPerLine, -1 )
		SiSSetupROP( 0xF0 )
		SiSSetupCMDFlag(PATFG | LINE)
		SiSSyncWP
	} else {
		SiS300SetupLineCount( 1 )
		SiS300SetupPATFG( nColor )
		SiS300SetupDSTRect( pcBitMap->m_nBytesPerLine, -1 );
		SiS300SetupDSTColorDepth( m_pHw->DstColor );
		SiS300SetupROP( 0xF0 )
		SiS300SetupCMDFlag( PATFG | LINE )
	}
	
	long nDstBase = pcBitMap->m_nVideoMemOffset;
	int nMinY = ( y1 > y2 ) ? y2 : y1;
	int nMaxY = ( y1 > y2 ) ? y1 : y2;
	if( nMaxY >= 2048 ) {
		nDstBase += pcBitMap->m_nBytesPerLine * nMaxY;
		y1 -= nMinY;
		y2 -= nMinY;
	}
	
	if( m_pHw->sisvga_engine == SIS_315_VGA ) {
		SiSCheckQueue(16 * 2);
		SiSSetupX0Y0X1Y1(x1, y1, x2, y2)
		SiSSetupDSTBaseDoCMD( nDstBase )
	} else {
		SiS300SetupDSTBase( nDstBase )
		SiS300SetupX0Y0( x1, y1 )
		SiS300SetupX1Y1( x2, y2 )
		m_pHw->CmdReg &= ~( NO_LAST_PIXEL );
		SiS300DoCMD
	}
	
	m_bEngineDirty = true;
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool SiS3xx::FillRect(SrvBitmap *pcBitMap, const IRect& cRect, const Color32_s& sColor, int nMode)
{
	if ( pcBitMap->m_bVideoMem == false || nMode != DM_COPY ) {
		return DisplayDriver::FillRect(pcBitMap, cRect, sColor, nMode);
	}
	
	int nWidth = cRect.Width()+1;
	int nHeight = cRect.Height()+1;
	uint32 nColor;
	if (pcBitMap->m_eColorSpc == CS_RGB32 ) {
		nColor = COL_TO_RGB32(sColor);
	} else {
		// we only support CS_RGB32 and CS_RGB16
		nColor = COL_TO_RGB16(sColor);
	}
	
	m_cGELock.Lock();
	
	if( m_pHw->sisvga_engine == SIS_315_VGA ) {
		SiSSetupDSTColorDepth(m_pHw->AccelDepth);
		SiSCheckQueue(16 * 1);
		SiSSetupPATFGDSTRect( nColor, pcBitMap->m_nBytesPerLine, -1 )
		SiSSetupROP( 0xF0 )
		SiSSetupCMDFlag(PATFG)
		SiSSyncWP
	} else {
		SiS300SetupPATFG( nColor )
		SiS300SetupDSTRect( pcBitMap->m_nBytesPerLine, -1 );
		SiS300SetupDSTColorDepth( m_pHw->DstColor );
		SiS300SetupROP( 0xF0 )
		SiS300SetupCMDFlag( PATFG )
	}
	
	long nDstBase = pcBitMap->m_nVideoMemOffset;
	if( cRect.top >= 2048 ) {
		nDstBase = pcBitMap->m_nBytesPerLine * cRect.top;
	}
	if( m_pHw->sisvga_engine == SIS_315_VGA ) {
		SiSCheckQueue(16 * 2)
		SiSSetupDSTXYRect( cRect.left, ( cRect.top < 2048 ) ? cRect.top : 0, nWidth, nHeight )
		SiSSetupDSTBaseDoCMD( nDstBase )
	} else {
		SiS300SetupDSTBase( nDstBase )
		SiS300SetupDSTXY( cRect.left, ( cRect.top < 2048 ) ? cRect.top : 0 )
		SiS300SetupRect( nWidth, nHeight )
		SiS300SetupCMDFlag( BITBLT | X_INC | Y_INC )
		SiS300DoCMD
	}
	
	m_bEngineDirty = true;
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
                       IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha)
{
	if (pcDstBitMap->m_bVideoMem == false || pcSrcBitMap->m_bVideoMem == false || nMode != DM_COPY 
		|| cDstRect.Size() != cSrcRect.Size()) {
		return DisplayDriver::BltBitmap(pcDstBitMap, pcSrcBitMap, cSrcRect, cDstRect, nMode, nAlpha);
	}
		
	int nWidth = cSrcRect.Width()+1;
	int nHeight = cSrcRect.Height()+1;
	IPoint cDstPos = cDstRect.LeftTop();
	m_cGELock.Lock();
	
	if( m_pHw->sisvga_engine == SIS_315_VGA ) {
		SiSSetupDSTColorDepth(m_pHw->AccelDepth);
		SiSCheckQueue(16 * 2);
		SiSSetupSRCPitchDSTRect(pcSrcBitMap->m_nBytesPerLine, pcDstBitMap->m_nBytesPerLine, -1)
		SiSSetupROP( 0xCC );
		SiSSyncWP
	} else {
		SiS300SetupDSTColorDepth( m_pHw->DstColor );
		SiS300SetupSRCPitch( pcSrcBitMap->m_nBytesPerLine );
		SiS300SetupDSTRect( pcDstBitMap->m_nBytesPerLine, -1 );
		SiS300SetupROP( 0xCC );
		SiS300SetupClipLT( 0, 0 );
		SiS300SetupClipRB( (pcDstBitMap->m_nWidth - 1), (pcDstBitMap->m_nHeight - 1) );
	}
	
	long nSrcBase = pcSrcBitMap->m_nVideoMemOffset;
	if( cSrcRect.top >= 2048 ) {
		nSrcBase += pcSrcBitMap->m_nBytesPerLine * cSrcRect.top;
		cSrcRect.top = 0;
	}
	
	long nDstBase = pcDstBitMap->m_nVideoMemOffset;
	if( cDstPos.y >= 2048 ) {
		nDstBase = pcDstBitMap->m_nBytesPerLine * cDstPos.y;
		cDstPos.y = 0;
	}
	if( m_pHw->sisvga_engine == SIS_315_VGA ) {
		SiSCheckQueue(16 * 3);
		SiSSetupSRCDSTBase( nSrcBase, nDstBase )
		SiSSetupSRCDSTXY( cSrcRect.left, ( cSrcRect.top < 2048 ) ? cSrcRect.top : 0, cDstPos.x, 
							( cDstPos.y < 2048 ) ? cDstPos.y : 0 )
		SiSSetRectDoCMD( nWidth, nHeight )
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
	}
	m_bEngineDirty = true;
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
	if( ( eFormat == CS_YUV12 || eFormat == CS_YUV422 ) && !m_bVideoOverlayUsed ) {
		if( m_sCurrentMode.m_eColorSpace == os::CS_RGB16 )
			m_nColorKey = COL_TO_RGB16( sColorKey );
		else
			m_nColorKey = COL_TO_RGB32( sColorKey );
		*pBuffer = SISStartVideo( 0, 0, cDst.left, cDst.top, cSize.x, cSize.y, cDst.Width() + 1, 
							cDst.Height() + 1, eFormat == CS_YUV12 ? 0x01 : 0x02, cSize.x, cSize.y, m_nColorKey );
		if( *pBuffer < 0 )
			return( false );
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
	
	if( eFormat == CS_YUV12 || eFormat == CS_YUV422 ) {
		delete_area( *pBuffer );
		FreeMemory( m_pHw->video_port.bufAddr );
		*pBuffer = SISStartVideo( 0, 0, cDst.left, cDst.top, cSize.x, cSize.y, cDst.Width() + 1, 
							cDst.Height() + 1, eFormat == CS_YUV12 ? 0x01 : 0x02, cSize.x, cSize.y, m_nColorKey );
		if( *pBuffer < 0 )
			return( false );
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
































