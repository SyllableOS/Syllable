/*
 *  nVidia driver for AtheOS application server
 *  Copyright (C) 2001  Joseph Artsimovich (joseph_a@mail.ru)
 *
 *  Based on the rivafb linux kernel driver and the xfree86 nv driver.
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

#define DISABLE_HW_CURSOR

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <gui/bitmap.h>

#include "nvidia.h"
#include "riva_hw.h"
#include "nvreg.h"

static video_mode g_sCurrentMode;

static const struct chip_info asChipInfos[] = {
	{0x0020, NV_ARCH_04, "Riva TNT"},
	{0x0028, NV_ARCH_04, "Riva TNT2"},
	{0x0029, NV_ARCH_04, "Riva TNT2 Ultra"},
	{0x002C, NV_ARCH_04, "Riva Vanta"},
	{0x002D, NV_ARCH_04, "Riva Ultra 64"},
	{0x002E, NV_ARCH_04, "Riva TNT2 (A)"},
	{0x002F, NV_ARCH_04, "Riva TNT2 (B)"},
	{0x00A0, NV_ARCH_04, "Riva Integrated"},
	{0x0100, NV_ARCH_10, "GeForce 256"},
	{0x0101, NV_ARCH_10, "GeForce DDR"},
	{0x0103, NV_ARCH_10, "Quadro"},
	{0x0110, NV_ARCH_10, "GeForce2 MX"},
	{0x0111, NV_ARCH_10, "GeForce2 MX DDR"},
	{0x0112, NV_ARCH_10, "GeForce2 GO"},
	{0x0113, NV_ARCH_10, "Quadro2 MXR"},
	{0x0150, NV_ARCH_10, "GeForce2 GTS"},
	{0x0151, NV_ARCH_10, "GeForce2 GTS (rev 1)"},
	{0x0152, NV_ARCH_10, "GeForce2 Ultra"},
	{0x0153, NV_ARCH_10, "Quadro2 PRO"},
	{0x0200, NV_ARCH_20, "GeForce3"},
	{0x0201, NV_ARCH_20, "GeForce3 (rev 1)"},
	{0x0202, NV_ARCH_20, "GeForce3 (rev 2)"},
	{0x0203, NV_ARCH_20, "GeForce3 (rev 3)"}
};

inline uint32 pci_size(uint32 base, uint32 mask)
{
	uint32 size = base & mask;
	return size & ~(size-1);
}

static uint32 get_pci_memory_size(const PCI_Info_s *pcPCIInfo, int nResource)
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource*4;
	uint32 nBase = read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	write_pci_config(nBus, nDev, nFnc, nOffset, 4, ~0);
	uint32 nSize = read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	write_pci_config(nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	if (!(nSize & PCI_ADDRESS_SPACE)) {
		return pci_size(nSize, PCI_ADDRESS_MEMORY_32_MASK);
	} else {
		return pci_size(nSize, PCI_ADDRESS_IO_MASK & 0xffff);
	}
}

NVidia::NVidia() : m_cGELock("nvidia_ge_lock"), m_cCursorHotSpot(0,0)
{	
	bool bFound = false;
	m_bIsInitiated = false;
	m_bPaletteEnabled = false;
	m_bUsingHWCursor = false;
	m_bCursorIsOn = false;
	
	char *zCardName = "";
	int nNrDevs = sizeof(asChipInfos)/sizeof(chip_info);
	for (int i = 0; get_pci_info(&m_cPCIInfo, i)==0; i++) {
		if (m_cPCIInfo.nVendorID == PCI_VENDOR_ID_NVIDIA) {
			for (int j = 0; j < nNrDevs; j++) {
				if (m_cPCIInfo.nDeviceID==asChipInfos[j].nDeviceId) {
					bFound = true;
					zCardName = asChipInfos[j].zName;
					m_sRiva.Architecture = asChipInfos[j].nArchRev;
					break;
				}
			}
			if (bFound) break;
		}
	}
	
	if (!bFound) {
		dbprintf("No supported cards found\n");
		return;
	}
	
	dbprintf("Found %s\n", zCardName);
	
	int nIoSize = get_pci_memory_size(&m_cPCIInfo, 0); 
	m_hRegisterArea = create_area("nvidia_regs", (void**)&m_pRegisterBase, nIoSize,
	                              AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hRegisterArea, (void*)(m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK));
	
	int nMemSize = get_pci_memory_size(&m_cPCIInfo, 1);
	m_hFrameBufferArea = create_area("nvidia_framebuffer", (void**)&m_pFrameBufferBase,
	                                 nMemSize, AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hFrameBufferArea, (void*)(m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK));
	
	m_sRiva.EnableIRQ = 0;
	m_sRiva.PRAMDAC = (unsigned *)(m_pRegisterBase+0x00680000);
	m_sRiva.PFB = (unsigned *)(m_pRegisterBase+0x00100000);
	m_sRiva.PFIFO = (unsigned *)(m_pRegisterBase+0x00002000);
	m_sRiva.PGRAPH = (unsigned *)(m_pRegisterBase+0x00400000);
	m_sRiva.PEXTDEV = (unsigned *)(m_pRegisterBase+0x00101000);
	m_sRiva.PTIMER = (unsigned *)(m_pRegisterBase+0x00009000);
	m_sRiva.PMC = (unsigned *)(m_pRegisterBase+0x00000000);
	m_sRiva.FIFO = (unsigned *)(m_pRegisterBase+0x00800000);
	m_sRiva.PCIO = (U008 *)(m_pRegisterBase+0x00601000);
	m_sRiva.PDIO = (U008 *)(m_pRegisterBase+0x00681000);
	m_sRiva.PVIO = (U008 *)(m_pRegisterBase+0x000C0000);
	
	m_sRiva.IO = 0x3d0;
	
	m_sRiva.PCRTC = (unsigned *)(m_pRegisterBase+0x00600000);
	m_sRiva.PRAMIN = (unsigned *)(m_pRegisterBase+0x00710000);
	
	RivaGetConfig(&m_sRiva);
	
	os::color_space colspace[] = {CS_RGB16, CS_RGB32};
	int bpp[] = {2, 4};
	for (int i=0; i<int(sizeof(bpp)/sizeof(bpp[0])); i++) {
		m_cScreenModeList.push_back(ScreenMode(640, 480, 640*bpp[i], colspace[i]));
		m_cScreenModeList.push_back(ScreenMode(800, 600, 800*bpp[i], colspace[i]));
		m_cScreenModeList.push_back(ScreenMode(1024, 768, 1024*bpp[i], colspace[i]));
		m_cScreenModeList.push_back(ScreenMode(1280, 1024, 1280*bpp[i], colspace[i]));
		m_cScreenModeList.push_back(ScreenMode(1600, 1200, 1600*bpp[i], colspace[i]));
	}
	
	memset(m_anCursorShape, 0, sizeof(m_anCursorShape));
	
	CRTCout(0x11, CRTCin(0x11) | 0x80);
	m_sRiva.LockUnlock(&m_sRiva, 0);
	
	m_bIsInitiated = true;
}

NVidia::~NVidia()
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

area_id NVidia::Open()
{
	return m_hFrameBufferArea;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void NVidia::Close()
{
}


int NVidia::GetScreenModeCount()
{
	return m_cScreenModeList.size();
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool NVidia::GetScreenModeDesc(int nIndex, ScreenMode* psMode)
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

int NVidia::SetScreenMode(int nWidth, int nHeight, color_space eColorSpc,
                          int nPosH, int nPosV, int nSizeH, int nSizeV, float vRefreshRate)
{
	struct riva_regs newmode;
	memset(&newmode, 0, sizeof(struct riva_regs));
	
	int nBpp = (eColorSpc == CS_RGB32) ? 4 : 2;
	int nHTotal = int(nWidth*1.3);
	int nVTotal = int(nHeight*1.1);
	int nHFreq = int((vRefreshRate+0.2)*nVTotal);
	int nPixClock = nHFreq*nHTotal/1000;
	int nHSyncLength = (nHTotal-nWidth)/2;
	int nRightBorder = (nHTotal-nWidth-nHSyncLength)/3;
	int nVSyncLength = 2;
	int nBottomBorder = (nVTotal-nHeight-nVSyncLength)/3;
	
	int nHSyncStart = nWidth+nRightBorder;
	int nVSyncStart = nHeight+nBottomBorder;
	int nHSyncEnd = nHSyncStart+nHSyncLength;
	int nVSyncEnd = nVSyncStart+nVSyncLength;
	
	int nHTot = nHTotal/8-1;
	int nHDisplay = nWidth/8-1;
	int nHStart = nHSyncStart/8-1;
	int nHEnd = nHSyncEnd/8-1;
	int nVTot = nVTotal-2;
	int nVDisplay = nHeight-1;
	int nVStart = nVSyncStart-1;
	int nVEnd = nVSyncEnd-1;
	
	newmode.crtc[0x00] = Set8Bits(nHTot-4);
	newmode.crtc[0x01] = Set8Bits(nHDisplay);
	newmode.crtc[0x02] = Set8Bits(nHDisplay);
	newmode.crtc[0x03] = SetBitField(nHTot, 4:0, 4:0) | SetBit(7);
	newmode.crtc[0x04] = Set8Bits(nHStart);
	newmode.crtc[0x05] = SetBitField(nHTot, 5:5, 7:7) |
	                     SetBitField(nHEnd, 4:0, 4:0);
	newmode.crtc[0x06] = SetBitField(nVTot, 7:0, 7:0);
	newmode.crtc[0x07] = SetBitField(nVTot, 8:8, 0:0) |
	                     SetBitField(nVDisplay, 8:8, 1:1) |
			     SetBitField(nVStart, 8: 8, 2:2) |
			     SetBitField(nVDisplay, 8:8, 3:3) |
			     SetBit(4) |
			     SetBitField(nVTot, 9:9, 5:5) |
			     SetBitField(nVDisplay, 9:9, 6:6) |
			     SetBitField(nVStart, 9:9, 7:7);
	newmode.crtc[0x09] = SetBitField(nVDisplay, 9:9, 5:5) |
	                     SetBit(6);
	newmode.crtc[0x10] = Set8Bits(nVStart);
	newmode.crtc[0x11] = SetBitField(nVEnd, 3:0, 3:0) |
	                     SetBit(5);
	newmode.crtc[0x12] = Set8Bits(nVDisplay);
	newmode.crtc[0x13] = Set8Bits((nWidth/8)*nBpp);
	newmode.crtc[0x15] = Set8Bits(nVDisplay);
	newmode.crtc[0x16] = Set8Bits(nVTot+1);
	newmode.crtc[0x17] = 0xc3;
	newmode.crtc[0x18] = 0xff;
	
	newmode.gra[0x05] = 0x40;
	newmode.gra[0x06] = 0x05;
	newmode.gra[0x07] = 0x0f;
	newmode.gra[0x08] = 0xff;
	
	for (int i = 0; i < 16; i++) {
		newmode.attr[i] = (uint8)i;
	}
	newmode.attr[0x10] = 0x41;
	newmode.attr[0x11] = 0xff;
	newmode.attr[0x12] = 0x0f;
	newmode.attr[0x13] = 0x00;
	newmode.attr[0x14] = 0x00;
	
	newmode.seq[0x01] = 0x01;
	newmode.seq[0x02] = 0x0f;
	newmode.seq[0x04] = 0x0e;
	
	newmode.ext.bpp = nBpp*8;
	newmode.ext.width = nWidth;
	newmode.ext.height = nHeight;
	
	newmode.misc_output = 0x2f;
	
	m_sRiva.CalcStateExt(&m_sRiva, &newmode.ext, nBpp*8, nWidth,
	                     nWidth, nHDisplay, nHStart, nHEnd,
			     nHTot, nHeight, nVDisplay, nVStart, nVEnd,
			     nVTot, nPixClock, 0);
	
			     
	LoadState(&newmode);
	
	//m_sRiva.SetStartAddress(&m_sRiva, 0);
	
	m_sRiva.LockUnlock(&m_sRiva, 0);

	SetupAccel();

#ifndef DISABLE_HW_CURSOR	
	// restore the hardware cursor
	if (m_bUsingHWCursor) {
		uint32 *pnSrc = (uint32 *)m_anCursorShape;
		volatile uint32 *pnDst = (uint32 *)m_sRiva.CURSOR;
		for (int i = 0; i < MAX_CURS*MAX_CURS/2; i++) {
			*pnDst++ = *pnSrc++;
		}
		
		SetMousePos(m_cCursorPos);
		if (m_bCursorIsOn) {
			MouseOn();
		}
	}
#endif

	g_sCurrentMode.nXres = nWidth;
	g_sCurrentMode.nYres = nHeight;
	g_sCurrentMode.nBpp = nBpp;
	g_sCurrentMode.eColorSpace = eColorSpc;
	
	return 0;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

int NVidia::GetHorizontalRes()
{
	return g_sCurrentMode.nXres;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

int NVidia::GetVerticalRes()
{
	return g_sCurrentMode.nYres;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

int NVidia::GetBytesPerLine()
{
	return g_sCurrentMode.nXres * g_sCurrentMode.nBpp;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

os::color_space NVidia::GetColorSpace()
{
	return g_sCurrentMode.eColorSpace;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void NVidia::SetColor(int nIndex, const Color32_s& sColor)
{

}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool NVidia::IntersectWithMouse(const IRect& cRect)
{
	return false;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void NVidia::SetCursorBitmap(os::mouse_ptr_mode eMode, const os::IPoint& cHotSpot,
                             const void *pRaster, int nWidth, int nHeight)
{
#ifndef DISABLE_HW_CURSOR
	m_cCursorHotSpot = cHotSpot;
	if (eMode != MPTR_MONO || nWidth > MAX_CURS || nHeight > MAX_CURS) {
#endif
		m_bUsingHWCursor = false;
		return DisplayDriver::SetCursorBitmap(eMode, cHotSpot, pRaster, nWidth, nHeight);
#ifndef DISABLE_HW_CURSOR
	}
	
	const uint8 *pnSrc = (const uint8*)pRaster;
	volatile uint32 *pnDst = (uint32 *)m_sRiva.CURSOR;
	uint16 *pnSaved = m_anCursorShape;
	uint32 *pnSaved32 = (uint32 *)m_anCursorShape;
	static uint16 anPalette[] =
	{CURS_TRANSPARENT, CURS_BLACK, CURS_BLACK, CURS_WHITE};
	
	for (int y = 0; y < MAX_CURS; y++) {
		for (int x = 0; x < MAX_CURS; x++, pnSaved++) {
			if (y >= nHeight || x >= nWidth) {
				*pnSaved = CURS_TRANSPARENT;
			} else {
				*pnSaved = anPalette[*pnSrc++];
			}
		}
	}
	
	for (int i = 0; i < MAX_CURS*MAX_CURS/2; i++) {
		*pnDst++ = *pnSaved32++;
	}
	
	m_bUsingHWCursor = true;
#endif
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


void NVidia::SetMousePos(os::IPoint cNewPos)
{
#ifndef DISABLE_HW_CURSOR
	m_cCursorPos = cNewPos;
	if (!m_bUsingHWCursor) {
#endif
		return DisplayDriver::SetMousePos(cNewPos);
#ifndef DISABLE_HW_CURSOR
	}
	int x = cNewPos.x - m_cCursorHotSpot.x;
	int y = cNewPos.y - m_cCursorHotSpot.y;
	*(m_sRiva.CURSORPOS) = (y << 16) | (x & 0xffff);
#endif
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


void NVidia::MouseOn()
{
#ifndef DISABLE_HW_CURSOR
	m_bCursorIsOn = true;
	if (!m_bUsingHWCursor) {
#endif
		return DisplayDriver::MouseOn();
#ifndef DISABLE_HW_CURSOR
	}
	m_sRiva.ShowHideCursor(&m_sRiva, 1);
#endif
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


void NVidia::MouseOff()
{
#ifndef DISABLE_HW_CURSOR
	m_bCursorIsOn = false;
	if (!m_bUsingHWCursor) {
#endif
		return DisplayDriver::MouseOn();
#ifndef DISABLE_HW_CURSOR
	}
	m_sRiva.ShowHideCursor(&m_sRiva, 0);
#endif
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------


bool NVidia::DrawLine(SrvBitmap* pcBitMap, const IRect& cClipRect,
                      const IPoint& cPnt1, const IPoint& cPnt2, const Color32_s& sColor, int nMode)
{
	if (pcBitMap->m_bVideoMem == false || nMode != DM_COPY) {
		WaitForIdle();
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
	
	RIVA_FIFO_FREE(m_sRiva, Line, 5);
	m_sRiva.Line->Color = nColor;
	m_sRiva.Line->Lin[0].point0 = (y1 << 16) | (x1 & 0xffff);
	m_sRiva.Line->Lin[0].point1 = (y2 << 16) | (x2 & 0xffff);
	
	// The graphic engine won't draw the last pixel of the line
	// so we have to draw it separately
	m_sRiva.Line->Lin[1].point0 = (y2 << 16) | (x2 & 0xffff);
	m_sRiva.Line->Lin[1].point1 = ((y2+1)<<16) | (x2 & 0xffff);
	
	WaitForIdle();
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool NVidia::FillRect(SrvBitmap *pcBitMap, const IRect& cRect, const Color32_s& sColor)
{
	if (pcBitMap->m_bVideoMem == false) {
		WaitForIdle();
		return DisplayDriver::FillRect(pcBitMap, cRect, sColor);
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
	
	RIVA_FIFO_FREE(m_sRiva, Bitmap, 1);
	m_sRiva.Bitmap->Color1A = nColor;
	RIVA_FIFO_FREE(m_sRiva, Bitmap, 2);
	m_sRiva.Bitmap->UnclippedRectangle[0].TopLeft = (cRect.left << 16) | cRect.top;
	m_sRiva.Bitmap->UnclippedRectangle[0].WidthHeight = (nWidth << 16) | nHeight;
	
	WaitForIdle();
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool NVidia::BltBitmap(SrvBitmap *pcDstBitMap, SrvBitmap *pcSrcBitMap,
                       IRect cSrcRect, IPoint cDstPos, int nMode)
{
	if (pcDstBitMap->m_bVideoMem == false || pcSrcBitMap->m_bVideoMem == false || nMode != DM_COPY) {
		WaitForIdle();
		return DisplayDriver::BltBitmap(pcDstBitMap, pcSrcBitMap, cSrcRect, cDstPos, nMode);
	}
		
	int nWidth = cSrcRect.Width()+1;
	int nHeight = cSrcRect.Height()+1;
	m_cGELock.Lock();
	
	RIVA_FIFO_FREE(m_sRiva, Blt, 3);
	m_sRiva.Blt->TopLeftSrc = (cSrcRect.top << 16) | cSrcRect.left;
	m_sRiva.Blt->TopLeftDst = (cDstPos.y << 16) | cDstPos.x;
	m_sRiva.Blt->WidthHeight = (nHeight << 16) | nWidth;
	
	WaitForIdle();
	m_cGELock.Unlock();
	return true;
}

//-----------------------------------------------------------------------------
//                          Private Functions
//-----------------------------------------------------------------------------

void NVidia::LoadState(struct riva_regs *regs)
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
	
	m_sRiva.LockUnlock(&m_sRiva, 0);
	m_sRiva.LoadStateExt(&m_sRiva, state);
}

//-----------------------------------------------------------------------------

void NVidia::SetupAccel()
{
	m_cGELock.Lock();
	RIVA_FIFO_FREE(m_sRiva, Clip, 2);
	m_sRiva.Clip->TopLeft = 0x0;
	m_sRiva.Clip->WidthHeight = 0x7fff7fff;
	SetupROP();
	WaitForIdle();
	m_cGELock.Unlock();
}

//-----------------------------------------------------------------------------

void NVidia::SetupROP()
{
	RIVA_FIFO_FREE(m_sRiva, Patt, 5);
	m_sRiva.Patt->Shape = 0;
	m_sRiva.Patt->Color0 = 0xffffffff;
	m_sRiva.Patt->Color1 = 0xffffffff;
	m_sRiva.Patt->Monochrome[0] = 0xffffffff;
	m_sRiva.Patt->Monochrome[1] = 0xffffffff;
	
	RIVA_FIFO_FREE(m_sRiva, Rop, 1);
	m_sRiva.Rop->Rop3 = 0xCC;
}

//-----------------------------------------------------------------------------

extern "C" DisplayDriver* init_gfx_driver()
{
    dbprintf("nvidia attempts to initialize\n");
    
    try {
	    NVidia *pcDriver = new NVidia();
	    if (pcDriver->IsInitiated()) {
		    return pcDriver;
	    }
	    return NULL;
    }
    catch (exception& cExc) {
	    dbprintf("Got exception\n");
	    return NULL;
    }
}