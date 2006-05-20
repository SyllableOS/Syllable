/*
 *  nVIDIA Riva driver for Syllable
 *  Copyright (C) 2001  Joseph Artsimovich (joseph_a@mail.ru)
 *
 *  Based on the rivafb linux kernel driver and the xfree86 nv driver.
 *  Video overlay code is from the rivatv project.
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
#include <atheos/vesa_gfx.h>
#include <atheos/udelay.h>
#include <appserver/pci_graphics.h>

#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <gui/bitmap.h>

#include "riva.h"
#include "riva_hw.h"

extern "C"
{
	extern int RivaGetConfig( RIVA_HW_INST* chip );
};

using namespace os;

static const struct chip_info asChipInfos[] = {
  { 0x12D20018, NV_ARCH_03, "RIVA 128" },
  { 0x10DE0020, NV_ARCH_04, "RIVA TNT" },
  { 0x10DE0028, NV_ARCH_04, "RIVA TNT2" },
  { 0x10DE002A, NV_ARCH_04, "Unknown TNT2" },
  { 0x10DE002C, NV_ARCH_04, "Vanta" },
  { 0x10DE0029, NV_ARCH_04, "RIVA TNT2 Ultra" },
  { 0x10DE002D, NV_ARCH_04, "RIVA TNT2 Model 64" },
  { 0x10DE00A0, NV_ARCH_04, "Aladdin TNT2" },
};


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

Riva::Riva( int nFd )
:	m_cGELock("riva_ge_lock"),
	m_hRegisterArea(-1),
	m_hFrameBufferArea(-1)
{	
	m_bIsInitiated = false;
	m_bPaletteEnabled = false;
	m_bEngineDirty = false;
	m_pRegisterBase = m_pFrameBufferBase = NULL;
	int j;
	
	bool bFound = false;
	int nNrDevs = sizeof(asChipInfos)/sizeof(chip_info);
	
	/* Get Info */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_cPCIInfo ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}
	
	
	uint32 ChipsetID = (m_cPCIInfo.nVendorID << 16) | m_cPCIInfo.nDeviceID;
	for ( j = 0; j < nNrDevs; j++) {
		if (ChipsetID == asChipInfos[j].nDeviceId) {
			bFound = true;
			break;
		}
	}
		

	if ( !bFound )
	{
		dbprintf( "Riva :: No supported cards found\n" );
		return;
	}
	
	dbprintf( "Riva :: Found %s\n", asChipInfos[j].pzName );
	
	int nIoSize = get_pci_memory_size(nFd, &m_cPCIInfo, 0); 
	m_hRegisterArea = create_area("riva_regs", (void**)&m_pRegisterBase, nIoSize,
	                              AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hRegisterArea, (void*)(m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK));
	
	int nMemSize = get_pci_memory_size(nFd, &m_cPCIInfo, 1);
	m_hFrameBufferArea = create_area("riva_framebuffer", (void**)&m_pFrameBufferBase,
	                                 nMemSize, AREA_FULL_ACCESS | AREA_WRCOMB, AREA_NO_LOCK);
	remap_area(m_hFrameBufferArea, (void*)(m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK));

	memset( &m_sHW, 0, sizeof( m_sHW ) );
	
	
	m_sHW.Chipset = asChipInfos[j].nDeviceId;
	m_sHW.Architecture = asChipInfos[j].nArchRev;

	switch (m_sHW.Architecture) {
		case NV_ARCH_03:
            NV3Setup();
            break;
		case NV_ARCH_04:
			NV4Setup();
			break;
		default:
			dbprintf("Riva :: No chipset has been set - this is BAD. Exiting.\n");
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
	
	
	CRTCout(0x11, CRTCin(0x11) | 0x80);
	m_sHW.LockUnlock(&m_sHW, 0);
	
	m_bIsInitiated = true;
	m_bVideoOverlayUsed = false;
}

Riva::~Riva()
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

area_id Riva::Open()
{
	return m_hFrameBufferArea;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void Riva::Close()
{
}


int Riva::GetScreenModeCount()
{
	return m_cScreenModeList.size();
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool Riva::GetScreenModeDesc(int nIndex, os::screen_mode* psMode)
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

int Riva::SetScreenMode(os::screen_mode sMode)
{
	struct riva_regs newmode;
	memset(&newmode, 0, sizeof(struct riva_regs));
	
	float vHPeriodEst = ( ( ( 1.0 / sMode.m_vRefreshRate ) - ( 550.0 / 1000000.0 ) ) / ( ( float )sMode.m_nHeight + 1 ) * 1000000.0 );
	float vVSyncPlusBp = rint( 550.0 / vHPeriodEst );
	float vVTotal = ( float )sMode.m_nHeight + vVSyncPlusBp + 1;
	float vVFieldRateEst = 1.0 / vHPeriodEst / vVTotal * 1000000.0;
	float vHPeriod = vHPeriodEst / ( sMode.m_vRefreshRate / vVFieldRateEst );

	//float vVFieldRate = 1.0 / vHPeriod / vVTotal * 1000000.0;
	//float vVFrameRate = vVFieldRate;
	float vIdealDutyCycle = ( ( ( 40.0 - 20.0 ) * 128.0 / 256.0 ) + 20.0 ) - ( ( 128.0 / 256.0 * 600.0 ) * vHPeriod / 1000.0 );
	float vHBlank = rint( ( float )sMode.m_nWidth * vIdealDutyCycle / ( 100.0 - vIdealDutyCycle ) / ( 2.0 * 8 ) ) * ( 2.0 * 8 );
	float vHTotal = ( float )sMode.m_nWidth + vHBlank;
	float vPixClock = vHTotal / vHPeriod;
	float vHSync = rint( 8.0 / 100.0 * vHTotal / 8 ) * 8;
	float vHFrontPorch = ( vHBlank / 2.0 ) - vHSync;
	float vVOddFrontPorchLines = 1;

	int nBpp = ( sMode.m_eColorSpace == CS_RGB32 ) ? 4 : 2;

	int nHTotal = ( int )rint( vHTotal );
	int nVTotal = ( int )rint( vVTotal );
	int nPixClock = ( int )rint( vPixClock ) * 1000;
	int nHSyncStart = sMode.m_nWidth + ( int )rint( vHFrontPorch );
	int nHSyncEnd = nHSyncStart + ( int )rint( vHSync );
	int nVSyncStart = sMode.m_nHeight + ( int )rint( vVOddFrontPorchLines );
	int nVSyncEnd = ( int )nVSyncStart + 3;

	int horizTotal = nHTotal / 8 - 5;
	int horizDisplay = sMode.m_nWidth / 8 - 1;
	int horizStart = nHSyncStart / 8 - 1;
	int horizEnd = nHSyncEnd / 8 - 1;
	int horizBlankStart = sMode.m_nWidth / 8 - 1;
	int horizBlankEnd = nHTotal / 8 - 1;
	int vertTotal = nVTotal - 2;
	int vertDisplay = sMode.m_nHeight - 1;
	int vertStart = nVSyncStart - 1;
	int vertEnd = nVSyncEnd - 1;
	int vertBlankStart = sMode.m_nHeight - 1;
	int vertBlankEnd = nVTotal - 1;
	
	newmode.crtc[0x00] = Set8Bits(horizTotal);
	newmode.crtc[0x01] = Set8Bits(horizDisplay);
	newmode.crtc[0x02] = Set8Bits(horizBlankStart);
	newmode.crtc[0x03] = SetBitField(horizBlankEnd, 4:0, 4:0)
						 | SetBit(7);
	newmode.crtc[0x04] = Set8Bits(horizStart);
	newmode.crtc[0x05] = SetBitField(horizBlankEnd, 5:5, 7:7)
						 | SetBitField(horizEnd, 4:0, 4:0);
	newmode.crtc[0x06] = SetBitField(vertTotal, 7:0, 7:0);
	newmode.crtc[0x07] = SetBitField(vertTotal, 8:8, 0:0)
	                     | SetBitField(vertDisplay, 8:8, 1:1)
						 | SetBitField(vertStart, 8:8, 2:2)
						 | SetBitField(vertBlankStart, 8:8, 3:3)
						 | SetBit(4)
						 | SetBitField(vertTotal, 9:9, 5:5)
						 | SetBitField(vertDisplay, 9:9, 6:6)
						 | SetBitField(vertStart, 9:9, 7:7);
	newmode.crtc[0x09] = SetBitField(vertBlankStart, 9:9, 5:5)
						 | SetBit(6);
	newmode.crtc[0x10] = Set8Bits(vertStart);
	newmode.crtc[0x11] = SetBitField(vertEnd, 3:0, 3:0) | SetBit(5);
	newmode.crtc[0x12] = Set8Bits(vertDisplay);
	newmode.crtc[0x13] = Set8Bits((sMode.m_nWidth/8)*nBpp);
	newmode.crtc[0x15] = Set8Bits(vertBlankStart);
	newmode.crtc[0x16] = Set8Bits(vertBlankEnd);
	newmode.crtc[0x17] = 0xc3;
	newmode.crtc[0x18] = 0xff;
	
	newmode.gra[0x05] = 0x40;
	newmode.gra[0x06] = 0x05;
	newmode.gra[0x07] = 0x0f;
	newmode.gra[0x08] = 0xff;
	
	for (int i = 0; i < 16; i++) {
		newmode.attr[i] = (uint8)i;
	}
	newmode.attr[0x10] = 0x01;
	newmode.attr[0x11] = 0xff;
	newmode.attr[0x12] = 0x0f;
	newmode.attr[0x13] = 0x00;
	newmode.attr[0x14] = 0x00;
	
	newmode.seq[0x01] = 0x01;
	newmode.seq[0x02] = 0x0f;
	newmode.seq[0x04] = 0x0e;
	
	newmode.ext.bpp = nBpp*8;
	newmode.ext.width = sMode.m_nWidth;
	newmode.ext.height = sMode.m_nHeight;

    newmode.ext.screen = SetBitField(horizBlankEnd,6:6,4:4)
						 | SetBitField(vertBlankStart,10:10,3:3)
						 | SetBitField(vertStart,10:10,2:2)
						 | SetBitField(vertDisplay,10:10,1:1)
						 | SetBitField(vertTotal,10:10,0:0);

    newmode.ext.horiz  = SetBitField(horizTotal,8:8,0:0) 
						 | SetBitField(horizDisplay,8:8,1:1)
						 | SetBitField(horizBlankStart,8:8,2:2)
						 | SetBitField(horizStart,8:8,3:3);

    newmode.ext.extra  = SetBitField(vertTotal,11:11,0:0)
						 | SetBitField(vertDisplay,11:11,2:2)
						 | SetBitField(vertStart,11:11,4:4)
						 | SetBitField(vertBlankStart,11:11,6:6);

	newmode.ext.interlace = 0xff;  /* interlace off */
	
	newmode.misc_output = 0x2f;
	
	m_sHW.CalcStateExt( &m_sHW, &newmode.ext, nBpp*8, sMode.m_nWidth, sMode.m_nWidth, sMode.m_nHeight, nPixClock, 0);
	newmode.ext.cursorConfig = 0x02000100;
			     
	LoadState(&newmode);
	
	m_sHW.LockUnlock(&m_sHW, 0);

	SetupAccel();

	m_sCurrentMode = sMode;
	m_sCurrentMode.m_nBytesPerLine = sMode.m_nWidth * nBpp;
	
	return 0;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

os::screen_mode Riva::GetCurrentScreenMode()
{

	return m_sCurrentMode;
}

void Riva::LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrc, os::IRect cDst )
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


bool Riva::DrawLine(SrvBitmap* pcBitMap, const IRect& cClipRect,
                      const IPoint& cPnt1, const IPoint& cPnt2, const Color32_s& sColor, int nMode)
{
	if (pcBitMap->m_bVideoMem == false || nMode != DM_COPY) {
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
	
	RIVA_FIFO_FREE(m_sHW, Line, 5);
	m_sHW.Line->Color = nColor;
	m_sHW.Line->Lin[0].point0 = (y1 << 16) | (x1 & 0xffff);
	m_sHW.Line->Lin[0].point1 = (y2 << 16) | (x2 & 0xffff);
	
	// The graphic engine won't draw the last pixel of the line
	// so we have to draw it separately
	m_sHW.Line->Lin[1].point0 = (y2 << 16) | (x2 & 0xffff);
	m_sHW.Line->Lin[1].point1 = ((y2+1)<<16) | (x2 & 0xffff);
	
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

bool Riva::FillRect(SrvBitmap *pcBitMap, const IRect& cRect, const Color32_s& sColor, int nMode)
{
	if (pcBitMap->m_bVideoMem == false || nMode != DM_COPY ) {
		return DisplayDriver::FillRect(pcBitMap, cRect, sColor, nMode);
	}
	
	int nWidth = cRect.Width()+1;
	int nHeight = cRect.Height()+1;
	uint32 nColor;
	if (pcBitMap->m_eColorSpc == CS_RGB32) {
		nColor = COL_TO_RGB32(sColor);
	} else {
		// we only support CS_RGB32 and CS_RGB16
		nColor = COL_TO_RGB16(sColor);
	}
	
	m_cGELock.Lock();
	
	RIVA_FIFO_FREE(m_sHW, Bitmap, 1);
	m_sHW.Bitmap->Color1A = nColor;
	RIVA_FIFO_FREE(m_sHW, Bitmap, 2);
	m_sHW.Bitmap->UnclippedRectangle[0].TopLeft = (cRect.left << 16) | cRect.top;
	m_sHW.Bitmap->UnclippedRectangle[0].WidthHeight = (nWidth << 16) | nHeight;
	
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

bool Riva::BltBitmap(SrvBitmap *pcDstBitMap, SrvBitmap *pcSrcBitMap,
                       IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha)
{
	if (pcDstBitMap->m_bVideoMem == false || pcSrcBitMap->m_bVideoMem == false || nMode != DM_COPY ) {
		return DisplayDriver::BltBitmap(pcDstBitMap, pcSrcBitMap, cSrcRect, cDstRect, nMode, nAlpha);
	}
		
	IPoint cDstPos = cDstRect.LeftTop();
	int nWidth = cSrcRect.Width()+1;
	int nHeight = cSrcRect.Height()+1;
	m_cGELock.Lock();
	
	RIVA_FIFO_FREE(m_sHW, Blt, 3);
	m_sHW.Blt->TopLeftSrc = (cSrcRect.top << 16) | cSrcRect.left;
	m_sHW.Blt->TopLeftDst = (cDstPos.y << 16) | cDstPos.x;
	m_sHW.Blt->WidthHeight = (nHeight << 16) | nWidth;
	
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

bool Riva::CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer )
{
	if( eFormat == CS_YUV422 && !m_bVideoOverlayUsed ) {
		/* Calculate offset */
    	uint32 pitch = 0; 
    	uint32 totalSize = 0;
    	
    	pitch = ( ( cSize.x << 1 ) + 0xff )  & ~0xff;
    	totalSize = pitch * cSize.y;
    	
    	uint32 offset = PAGE_ALIGN( ( m_sHW.RamAmountKBytes - 128 ) * 1024 - totalSize - PAGE_SIZE );
		
		*pBuffer = create_area( "riva_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		remap_area( *pBuffer, (void*)((m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK) + offset) );
		
		
		/* Start video */
		
		switch( BitsPerPixel( m_sCurrentMode.m_eColorSpace ) )
		{
			case 16:
				m_nColorKey = COL_TO_RGB16( sColorKey );
			break;
			case 32:
				m_nColorKey = COL_TO_RGB32( sColorKey );
			break;
			
			default:
				delete_area( *pBuffer );
				return( false );
		}
	
		/* NV3/NV4 Overlay */
		m_sHW.PMC[0x00680224/4] = 0;
		m_sHW.PMC[0x00680228/4] = 0;
		m_sHW.PMC[0x0068022C/4] = 0;
		m_sHW.PMC[0x0068020C/4] = offset;
		m_sHW.PMC[0x00680210/4] = offset;
		m_sHW.PMC[0x00680214/4] = pitch;
		m_sHW.PMC[0x00680218/4] = pitch;
		m_sHW.PMC[0x00680230/4] = (cDst.top << 16) | cDst.left;
		m_sHW.PMC[0x00680234/4] = (cDst.Height() << 16) | cDst.Width();
		m_sHW.PMC[0x00680200/4] = ( (uint16)(((cSize.y) << 11) / cDst.Height() ) << 16 ) | (uint16)( ((cSize.x) << 11) / cDst.Width() );
		m_sHW.PMC[0x00680280/4] = 0x69;
		m_sHW.PMC[0x00680284/4] = 0x3e;
		m_sHW.PMC[0x00680288/4] = 0x89;
		m_sHW.PMC[0x0068028C/4] = 0x00000;
		m_sHW.PMC[0x00680204/4] = 0x001;
		m_sHW.PMC[0x00680208/4] = 0x111;
		m_sHW.PMC[0x0068023C/4] = 0x03;
		m_sHW.PMC[0x00680238/4] = 0x38;
		m_sHW.PMC[0x0068021C/4] = 0;
		m_sHW.PMC[0x00680220/4] = 0;
		m_sHW.PMC[0x00680240/4] = m_nColorKey & 0xffffff;
		m_sHW.PMC[0x00680244/4] = 0x11;
		m_sHW.PMC[0x00680228/4] = 1 << 16;
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

bool Riva::RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, area_id *pBuffer )
{
	
	if( eFormat == CS_YUV422 ) {
		delete_area( *pBuffer );
		/* Calculate offset */
    	uint32 pitch = 0; 
    	uint32 totalSize = 0;
    	
    	pitch = ( ( cSize.x << 1 ) + 0xff )  & ~0xff;
    	totalSize = pitch * cSize.y;
    	
    	uint32 offset = PAGE_ALIGN( ( m_sHW.RamAmountKBytes - 128 ) * 1024 - totalSize - PAGE_SIZE );
		
		*pBuffer = create_area( "riva_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		remap_area( *pBuffer, (void*)((m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK) + offset) );
		
		
		/* NV3/NV4 Overlay */
		m_sHW.PMC[0x00680224/4] = 0;
		m_sHW.PMC[0x00680228/4] = 0;
		m_sHW.PMC[0x0068022C/4] = 0;
		m_sHW.PMC[0x0068020C/4] = offset;
		m_sHW.PMC[0x00680210/4] = offset;
		m_sHW.PMC[0x00680214/4] = pitch;
		m_sHW.PMC[0x00680218/4] = pitch;
		m_sHW.PMC[0x00680230/4] = (cDst.top << 16) | cDst.left;
		m_sHW.PMC[0x00680234/4] = (cDst.Height() << 16) | cDst.Width();
		m_sHW.PMC[0x00680200/4] = ( (uint16)(((cSize.y) << 11) / cDst.Height() ) << 16 ) | (uint16)( ((cSize.x) << 11) / cDst.Width() );
		m_sHW.PMC[0x00680280/4] = 0x69;
		m_sHW.PMC[0x00680284/4] = 0x3e;
		m_sHW.PMC[0x00680288/4] = 0x89;
		m_sHW.PMC[0x0068028C/4] = 0x00000;
		m_sHW.PMC[0x00680204/4] = 0x001;
		m_sHW.PMC[0x00680208/4] = 0x111;
		m_sHW.PMC[0x0068023C/4] = 0x03;
		m_sHW.PMC[0x00680238/4] = 0x38;
		m_sHW.PMC[0x0068021C/4] = 0;
		m_sHW.PMC[0x00680220/4] = 0;
		m_sHW.PMC[0x00680240/4] = m_nColorKey & 0xffffff;
		m_sHW.PMC[0x00680244/4] = 0x11;
		m_sHW.PMC[0x00680228/4] = 1 << 16;
		
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

void Riva::DeleteVideoOverlay( area_id *pBuffer )
{
	if( m_bVideoOverlayUsed ) {
		delete_area( *pBuffer );
	
		m_sHW.PMC[0x00680244/4] = m_sHW.PMC[0x00680244/4] & ~1;
		m_sHW.PMC[0x00680224/4] = 0;
		m_sHW.PMC[0x00680228/4] = 0;
		m_sHW.PMC[0x0068022C/4] = 0;
		
	}
	m_bVideoOverlayUsed = false;
}


//-----------------------------------------------------------------------------
//                          Private Functions
//-----------------------------------------------------------------------------

void Riva::LoadState(struct riva_regs *regs)
{
	RIVA_HW_STATE *state = &regs->ext;
	
	MISCout(regs->misc_output);
	
	for (int i = 1; i < NUM_SEQ_REGS; i++) {
		SEQout(i, regs->seq[i]);
	}
	
	CRTCout(17, regs->crtc[17] & ~0x80);
	
	for (int i = 0; i < 25; i++) {
		CRTCout(i, regs->crtc[i]);
	}
	
	for (int i = 0; i < NUM_GRC_REGS; i++) {
		GRAout(i, regs->gra[i]);
	}
	
//	DisablePalette();	
	for (int i = 0; i < NUM_ATC_REGS; i++) {
		ATTRout(i, regs->attr[i]);
	}
	
	m_sHW.LockUnlock(&m_sHW, 0);
	m_sHW.LoadStateExt(&m_sHW, state);
}

//-----------------------------------------------------------------------------

void Riva::SetupAccel()
{
	m_cGELock.Lock();
	RIVA_FIFO_FREE(m_sHW, Clip, 2);
	m_sHW.Clip->TopLeft = 0x0;
	m_sHW.Clip->WidthHeight = 0x7fff7fff;
	SetupROP();
	WaitForIdle();
	m_cGELock.Unlock();
}

//-----------------------------------------------------------------------------

void Riva::SetupROP()
{
	RIVA_FIFO_FREE(m_sHW, Patt, 5);
	m_sHW.Patt->Shape = 0;
	m_sHW.Patt->Color0 = 0xffffffff;
	m_sHW.Patt->Color1 = 0xffffffff;
	m_sHW.Patt->Monochrome[0] = 0xffffffff;
	m_sHW.Patt->Monochrome[1] = 0xffffffff;
	
	RIVA_FIFO_FREE(m_sHW, Rop, 1);
	m_sHW.Rop->Rop3 = 0xCC;
}

void Riva::NVCommonSetup()
{
//	m_sRiva.Dac.LoadPalette = NVDACLoadPalette;

    /*
     * No IRQ in use.
     */
    m_sHW.EnableIRQ = 0;
    /*
     * Map remaining registers. This MUST be done in the OS specific driver code.
     */
//	m_sHW.IO      = VGA_IOBASE_COLOR;

    m_sHW.PRAMDAC	= (unsigned	*)(m_pRegisterBase+0x00680000);
    m_sHW.PFB		= (unsigned	*)(m_pRegisterBase+0x00100000);
    m_sHW.PFIFO		= (unsigned	*)(m_pRegisterBase+0x00002000);
    m_sHW.PGRAPH		= (unsigned	*)(m_pRegisterBase+0x00400000);
    m_sHW.PEXTDEV	= (unsigned	*)(m_pRegisterBase+0x00101000);
    m_sHW.PTIMER		= (unsigned	*)(m_pRegisterBase+0x00009000);
    m_sHW.PMC		= (unsigned	*)(m_pRegisterBase+0x00000000);
    m_sHW.FIFO		= (unsigned	*)(m_pRegisterBase+0x00800000);
    m_sHW.PCIO		= (U008		*)(m_pRegisterBase+0x00601000);
    m_sHW.PDIO		= (U008		*)(m_pRegisterBase+0x00681000);
    m_sHW.PVIO		= (U008		*)(m_pRegisterBase+0x000C0000);

    
    RivaGetConfig(&m_sHW);

//	m_sRiva.Dac.maxPixelClock = m_sHW.MaxVClockFreqKHz;

    m_sHW.LockUnlock(&m_sHW, 0);

}

void Riva::NV3Setup()
{
	dbprintf("Riva :: NV3Setup\n");
	m_sHW.PRAMIN = (unsigned	*)(m_pRegisterBase+0x00C00000);
	NVCommonSetup();
	m_sHW.PCRTC = m_sHW.PGRAPH;
}

void Riva::NV4Setup()
{
	dbprintf("Riva :: NV4Setup\n");
	m_sHW.PRAMIN = (unsigned	*)(m_pRegisterBase+0x00710000);
	m_sHW.PCRTC = (unsigned	*)(m_pRegisterBase+0x00600000);
	NVCommonSetup();
}


//-----------------------------------------------------------------------------

extern "C" DisplayDriver* init_gfx_driver( int nFd )
{
    dbprintf("riva attempts to initialize\n");
    
    try {
	    Riva *pcDriver = new Riva( nFd );
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
