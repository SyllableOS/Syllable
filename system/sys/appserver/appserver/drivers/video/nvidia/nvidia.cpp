/*
 *  nVidia driver for AtheOS application server
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

#define DISABLE_HW_CURSOR

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

#include "nvidia.h"
#include "riva_hw.h"
#include "nvreg.h"
#include "nv_pci_ids.h"
#include "nv_type.h"

static const struct chip_info asChipInfos[] = {
  { 0x12D20018, NV_ARCH_03,"RIVA 128" },
  { 0x10DE0020, NV_ARCH_04,"RIVA TNT" },
  { 0x10DE0028, NV_ARCH_04,"RIVA TNT2" },
  { 0x10DE002A, NV_ARCH_04,"Unknown TNT2" },
  { 0x10DE002C, NV_ARCH_04,"Vanta" },
  { 0x10DE0029, NV_ARCH_04,"RIVA TNT2 Ultra" },
  { 0x10DE002D, NV_ARCH_04,"RIVA TNT2 Model 64" },
  { 0x10DE00A0, NV_ARCH_04,"Aladdin TNT2" },
  { 0x10DE0100, NV_ARCH_10, "GeForce 256" },
  { 0x10DE0101, NV_ARCH_10, "GeForce DDR" },
  { 0x10DE0103, NV_ARCH_10, "Quadro" },
  { 0x10DE0110, NV_ARCH_10, "GeForce2 MX/MX 400" },
  { 0x10DE0111, NV_ARCH_10, "GeForce2 MX 100/200" },
  { 0x10DE0112, NV_ARCH_10, "GeForce2 Go" },
  { 0x10DE0113, NV_ARCH_10, "Quadro2 MXR/EX/Go" },
  { 0x10DE01A0, NV_ARCH_10, "GeForce2 Integrated GPU" },
  { 0x10DE0150, NV_ARCH_10, "GeForce2 GTS" },
  { 0x10DE0151, NV_ARCH_10, "GeForce2 Ti" },
  { 0x10DE0152, NV_ARCH_10, "GeForce2 Ultra" },
  { 0x10DE0153, NV_ARCH_10, "Quadro2 Pro" },
  { 0x10DE0170, NV_ARCH_10, "GeForce4 MX 460" },
  { 0x10DE0171, NV_ARCH_10, "GeForce4 MX 440" },
  { 0x10DE0172, NV_ARCH_10, "GeForce4 MX 420" },
  { 0x10DE0173, NV_ARCH_10, "GeForce4 MX 440-SE" },
  { 0x10DE0174, NV_ARCH_10, "GeForce4 440 Go" },
  { 0x10DE0175, NV_ARCH_10, "GeForce4 420 Go" },
  { 0x10DE0176, NV_ARCH_10, "GeForce4 420 Go 32M" },
  { 0x10DE0177, NV_ARCH_10, "GeForce4 460 Go" },
  { 0x10DE0179, NV_ARCH_10, "GeForce4 440 Go 64M" },
  { 0x10DE017D, NV_ARCH_10, "GeForce4 410 Go 16M" },
  { 0x10DE017C,NV_ARCH_10,  "Quadro4 500 GoGL" },
  { 0x10DE0178, NV_ARCH_10, "Quadro4 550 XGL" },
  { 0x10DE017A, NV_ARCH_10, "Quadro4 NVS" },
  { 0x10DE0181, NV_ARCH_10, "GeForce4 MX 440 with AGP8X" },
  { 0x10DE0182, NV_ARCH_10, "GeForce4 MX 440SE with AGP8X" },
  { 0x10DE0183, NV_ARCH_10, "GeForce4 MX 420 with AGP8X" },
  { 0x10DE0186, NV_ARCH_10, "GeForce4 448 Go" },
  { 0x10DE0187, NV_ARCH_10, "GeForce4 488 Go" },
  { 0x10DE0188, NV_ARCH_10, "Quadro4 580 XGL" },
  { 0x10DE018A, NV_ARCH_10, "Quadro4 280 NVS" },
  { 0x10DE018B, NV_ARCH_10, "Quadro4 380 XGL" },
  { 0x10DE01F0, NV_ARCH_10, "GeForce4 MX Integrated GPU" },
  { 0x10DE0200, NV_ARCH_20,"GeForce3" },
  { 0x10DE0201, NV_ARCH_20,"GeForce3 Ti 200" },
  { 0x10DE0202, NV_ARCH_20,"GeForce3 Ti 500" },
  { 0x10DE0203,NV_ARCH_20, "Quadro DCC" },
  { 0x10DE0250, NV_ARCH_20,"GeForce4 Ti 4600" },
  { 0x10DE0251, NV_ARCH_20,"GeForce4 Ti 4400" },
  { 0x10DE0252,NV_ARCH_20, "0x0252" },
  { 0x10DE0253, NV_ARCH_20,"GeForce4 Ti 4200" },
  { 0x10DE0258, NV_ARCH_20,"Quadro4 900 XGL" },
  { 0x10DE0259, NV_ARCH_20,"Quadro4 750 XGL" },
  { 0x10DE025B, NV_ARCH_20,"Quadro4 700 XGL" },
  { 0x10DE0280, NV_ARCH_20,"GeForce4 Ti 4800" },
  { 0x10DE0281, NV_ARCH_20,"GeForce4 Ti 4200 with AGP8X" },
  { 0x10DE0282, NV_ARCH_20,"GeForce4 Ti 4800 SE" },
  { 0x10DE0286, NV_ARCH_20,"GeForce4 4200 Go" },
  { 0x10DE028C, NV_ARCH_20,"Quadro4 700 GoGL" },
  { 0x10DE0288,NV_ARCH_20, "Quadro4 980 XGL" },
  { 0x10DE0289,NV_ARCH_20, "Quadro4 780 XGL" },
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

NVidia::NVidia( int nFd )
:	m_cGELock("nvidia_ge_lock"),
	m_hRegisterArea(-1),
	m_hFrameBufferArea(-1),
	m_cCursorPos(0,0),
	m_cCursorHotSpot(0,0)
{	
	m_bIsInitiated = false;
	m_bPaletteEnabled = false;
	m_bUsingHWCursor = false;
	m_bCursorIsOn = false;
	
	std::vector<PCI_Info_s> cCandidates;
	std::vector<struct chip_info> cCandidatesInfo;
	int nNrDevs = sizeof(asChipInfos)/sizeof(chip_info);
	
	/* Get Info */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &m_cPCIInfo ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}
	
	
	uint32 ChipsetID = (m_cPCIInfo.nVendorID << 16) | m_cPCIInfo.nDeviceID;
	for (int j = 0; j < nNrDevs; j++) {
		if (ChipsetID == asChipInfos[j].nDeviceId) {
			cCandidates.push_back(m_cPCIInfo);
			cCandidatesInfo.push_back(asChipInfos[j]);
			break;
		}
	}
		

	int nCards = cCandidates.size();
	if (!nCards) {
 		dbprintf("nVidia :: No supported cards found\n");
 		return;
	}
	
	dbprintf("nVidia :: Found supported cards:\n");
	for (int i = 0; i < nCards; i++) {
		dbprintf("  %s\n", cCandidatesInfo[i].pzName);
	}
	
	int nPrimaryCardIndex = -1;
	
	Vesa_Info_s sVesaInfo;
	VESA_Mode_Info_s sModeInfo;
	uint16 anModes[1024];
	uint32 nFBPhysAddr = 0;
	
	strcpy(sVesaInfo.VesaSignature, "VBE2");
	int nModeCount = get_vesa_info(&sVesaInfo, anModes, 1024);
	
	for (int i = 0; i < nModeCount; i++) {
		get_vesa_mode_info(&sModeInfo, anModes[i]);
		if (!sModeInfo.PhysBasePtr) continue;
		if (sModeInfo.BitsPerPixel < 8 ) continue;
		if (sModeInfo.NumberOfPlanes != 1) continue;
		nFBPhysAddr = (uint32)sModeInfo.PhysBasePtr;
		break;
	}
	
	if (!nFBPhysAddr) {
		dbprintf("nVidia :: Could not get linear framebuffer address from vesa. Exiting.\n");
		return;
	}
	
	for (int i = 0; i < nCards; i++) {
		if (nFBPhysAddr == (cCandidates[i].u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK)) {
			nPrimaryCardIndex = i;
			break;
		}
	}
	
	if (nPrimaryCardIndex < 0) {
		dbprintf("nVidia :: None of the found cards is primary. Exiting.\n");
		return;
	}
	
	dbprintf("nVidia :: Using %s\n", cCandidatesInfo[nPrimaryCardIndex].pzName);
	
	PCI_Info_s *pcPrimaryCardPCIInfo = &cCandidates[nPrimaryCardIndex];
	m_sRiva.Chipset				= cCandidatesInfo[nPrimaryCardIndex].nDeviceId;
	m_sRiva.riva.Architecture	= cCandidatesInfo[nPrimaryCardIndex].nArchRev;
	m_cPCIInfo = cCandidates[nPrimaryCardIndex];
	
	int nIoSize = get_pci_memory_size(nFd, &m_cPCIInfo, 0); 
	m_hRegisterArea = create_area("nvidia_regs", (void**)&m_pRegisterBase, nIoSize,
	                              AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hRegisterArea, (void*)(pcPrimaryCardPCIInfo->u.h0.nBase0 & PCI_ADDRESS_IO_MASK));
	
	int nMemSize = get_pci_memory_size(nFd, &m_cPCIInfo, 1);
	m_hFrameBufferArea = create_area("nvidia_framebuffer", (void**)&m_pFrameBufferBase,
	                                 nMemSize, AREA_FULL_ACCESS, AREA_NO_LOCK);
	remap_area(m_hFrameBufferArea, (void*)(pcPrimaryCardPCIInfo->u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK));

	m_sRiva.FlatPanel = -1;   /* autodetect later */
	m_sRiva.forceCRTC = -1;

     switch (m_sRiva.Chipset & 0x0ff0) {
        case 0x0010:
            NV3Setup();
            break;
        case 0x0020:
        case 0x00A0:
            NV4Setup();
            break;
        case 0x0100:
        case 0x0110:
        case 0x0150:
        case 0x0170:
        case 0x01A0:
        case 0x01F0:
            NV10Setup();
			break;
		case 0x0200:
		case 0x0250:
		case 0x0280:
            NV20Setup();
            break;
        case 0x0300:
		case 0x0310:
		case 0x0320:
		case 0x0330:
		case 0x0340:
			NV30Setup();
            break;
        default:
        	dbprintf("nVidia :: No chipset has been set - this is BAD. Exiting.\n");
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
	
	memset(m_anCursorShape, 0, sizeof(m_anCursorShape));
	
	CRTCout(0x11, CRTCin(0x11) | 0x80);
	m_sRiva.riva.LockUnlock(&m_sRiva.riva, 0);
	
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

bool NVidia::GetScreenModeDesc(int nIndex, os::screen_mode* psMode)
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

int NVidia::SetScreenMode(os::screen_mode sMode)
{
	struct riva_regs newmode;
	memset(&newmode, 0, sizeof(struct riva_regs));
	
	int nBpp = (sMode.m_eColorSpace == CS_RGB32) ? 4 : 2;
	int nHTotal = int(sMode.m_nWidth*1.3);
	int nVTotal = int(sMode.m_nHeight*1.1);
	int nHFreq = int((sMode.m_vRefreshRate+0.2)*nVTotal);
	int nPixClock = nHFreq*nHTotal/1000;
	int nHSyncLength = (nHTotal-sMode.m_nWidth)/2;
	int nRightBorder = (nHTotal-sMode.m_nWidth-nHSyncLength)/3;
	int nVSyncLength = 2;
	int nBottomBorder = (nVTotal-sMode.m_nHeight-nVSyncLength)/3;	

	int nHSyncStart = sMode.m_nWidth+nRightBorder;
	int nVSyncStart = sMode.m_nHeight+nBottomBorder;
	int nHSyncEnd = nHSyncStart+nHSyncLength;
	int nVSyncEnd = nVSyncStart+nVSyncLength;
	
//	int nHTot = nHTotal/8-1;
//	int nHDisplay = nWidth/8-1;
//	int nHStart = nHSyncStart/8-1;
//	int nHEnd = nHSyncEnd/8-1;
//	int nVTot = nVTotal-2;
//	int nVDisplay = nHeight-1;
//	int nVStart = nVSyncStart-1;
//	int nVEnd = nVSyncEnd-1;
	
	int horizTotal		= (nHTotal/8-1) - 4;
	int horizDisplay	= sMode.m_nWidth/8-1;
	int horizStart		= nHSyncStart/8-1;
	int horizEnd		= nHSyncEnd/8-1;
	int horizBlankStart	= sMode.m_nWidth/8-1;
	int horizBlankEnd	= nHTotal/8-1;
	int vertTotal		= nVTotal-2;
	int vertDisplay		= sMode.m_nHeight-1;
	int vertStart		= nVSyncStart-1;
	int vertEnd			= nVSyncEnd-1;
	int vertBlankStart	= sMode.m_nHeight-1;
	int vertBlankEnd	= (nVTotal-2) + 1;
	
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
	newmode.attr[0x10] = 0x41;
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
	
	m_sRiva.riva.CalcStateExt(&m_sRiva.riva, &newmode.ext, nBpp*8, sMode.m_nWidth, sMode.m_nWidth, sMode.m_nHeight, nPixClock, 0);
			     
	LoadState(&newmode);
	
//	m_sRiva.riva.SetStartAddress(&m_sRiva, 0);
	
	m_sRiva.riva.LockUnlock(&m_sRiva.riva, 0);

	SetupAccel();

#ifndef DISABLE_HW_CURSOR	
	// restore the hardware cursor
	if (m_bUsingHWCursor) {
		uint32 *pnSrc = (uint32 *)m_anCursorShape;
		volatile uint32 *pnDst = (uint32 *)m_sRiva.riva.CURSOR;
		for (int i = 0; i < MAX_CURS*MAX_CURS/2; i++) {
			*pnDst++ = *pnSrc++;
		}
		
		SetMousePos(m_cCursorPos);
		if (m_bCursorIsOn) {
			MouseOn();
		}
	}
#endif

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

os::screen_mode NVidia::GetCurrentScreenMode()
{

	return m_sCurrentMode;
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
	volatile uint32 *pnDst = (uint32 *)m_sRiva.riva.CURSOR;
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
	*(m_sRiva.riva.CURSORPOS) = (y << 16) | (x & 0xffff);
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
	m_sRiva.riva.ShowHideCursor(&m_sRiva.riva, 1);
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
		return DisplayDriver::MouseOff();
#ifndef DISABLE_HW_CURSOR
	}
	m_sRiva.riva.ShowHideCursor(&m_sRiva.riva, 0);
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
	
	RIVA_FIFO_FREE(m_sRiva.riva, Line, 5);
	m_sRiva.riva.Line->Color = nColor;
	m_sRiva.riva.Line->Lin[0].point0 = (y1 << 16) | (x1 & 0xffff);
	m_sRiva.riva.Line->Lin[0].point1 = (y2 << 16) | (x2 & 0xffff);
	
	// The graphic engine won't draw the last pixel of the line
	// so we have to draw it separately
	m_sRiva.riva.Line->Lin[1].point0 = (y2 << 16) | (x2 & 0xffff);
	m_sRiva.riva.Line->Lin[1].point1 = ((y2+1)<<16) | (x2 & 0xffff);
	
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
	if (pcBitMap->m_bVideoMem == false ) {
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
	
	RIVA_FIFO_FREE(m_sRiva.riva, Bitmap, 1);
	m_sRiva.riva.Bitmap->Color1A = nColor;
	RIVA_FIFO_FREE(m_sRiva.riva, Bitmap, 2);
	m_sRiva.riva.Bitmap->UnclippedRectangle[0].TopLeft = (cRect.left << 16) | cRect.top;
	m_sRiva.riva.Bitmap->UnclippedRectangle[0].WidthHeight = (nWidth << 16) | nHeight;
	
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
	if (pcDstBitMap->m_bVideoMem == false || pcSrcBitMap->m_bVideoMem == false || nMode != DM_COPY ) {
		WaitForIdle();
		return DisplayDriver::BltBitmap(pcDstBitMap, pcSrcBitMap, cSrcRect, cDstPos, nMode);
	}
		
	int nWidth = cSrcRect.Width()+1;
	int nHeight = cSrcRect.Height()+1;
	m_cGELock.Lock();
	
	RIVA_FIFO_FREE(m_sRiva.riva, Blt, 3);
	m_sRiva.riva.Blt->TopLeftSrc = (cSrcRect.top << 16) | cSrcRect.left;
	m_sRiva.riva.Blt->TopLeftDst = (cDstPos.y << 16) | cDstPos.x;
	m_sRiva.riva.Blt->WidthHeight = (nHeight << 16) | nWidth;
	
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

bool NVidia::CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer )
{
	if( eFormat == CS_YUV422 && !m_bVideoOverlayUsed ) {
		/* Calculate offset */
    	uint32 pitch = 0; 
    	uint32 totalSize = 0;
    	
    	pitch = ( ( cSize.x << 1 ) + 3 )  & ~3;
    	totalSize = pitch * cSize.y;
    	
    	uint32 offset = PAGE_ALIGN( ( m_sRiva.riva.RamAmountKBytes - 128 ) * 1024 - totalSize - PAGE_SIZE );
		
		dbprintf("Offset %i\n", (int)offset );
		
		*pBuffer = create_area( "nvidia_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
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
		if( m_sRiva.riva.Architecture >= NV_ARCH_10 ) {
			/* NV10/NV20 Overlay */
			m_sRiva.riva.PMC[0x00008910/4] = 4096;
			m_sRiva.riva.PMC[0x00008914/4] = 4096;
			m_sRiva.riva.PMC[0x00008918/4] = 4096;
			m_sRiva.riva.PMC[0x0000891C/4] = 4096;
			m_sRiva.riva.PMC[0x00008b00/4] = m_nColorKey & 0xffffff;
		
			m_sRiva.riva.PMC[0x8900/4] = offset;
			m_sRiva.riva.PMC[0x8908/4] = offset + totalSize;
		
			if( m_sRiva.riva.Architecture >= NV_ARCH_20 ) {
				m_sRiva.riva.PMC[0x8800/4] = offset;
				m_sRiva.riva.PMC[0x8808/4] = offset + totalSize;
			}
			m_sRiva.riva.PMC[0x8920/4] = 0;
		
			m_sRiva.riva.PMC[0x8928/4] = (cSize.y << 16) | cSize.x;
			m_sRiva.riva.PMC[0x8930/4] = 0;
			m_sRiva.riva.PMC[0x8938/4] = (cSize.x << 20) / cDst.Width();
			m_sRiva.riva.PMC[0x8940/4] = (cSize.y << 20) / cDst.Height();
			m_sRiva.riva.PMC[0x8948/4] = (cDst.top << 16) | cDst.left;
			m_sRiva.riva.PMC[0x8950/4] = (cDst.Height() << 16) | cDst.Width();
		
			uint32 dstPitch = ( pitch ) | 1 << 20;
			m_sRiva.riva.PMC[0x8958/4] = dstPitch;	
			m_sRiva.riva.PMC[0x8704/4] = 0;
			m_sRiva.riva.PMC[0x8700/4] = 1;
		} else {
			/* NV3/NV4 Overlay */
			m_sRiva.riva.PMC[0x00680224/4] = 0;
			m_sRiva.riva.PMC[0x00680228/4] = 0;
			m_sRiva.riva.PMC[0x0068022C/4] = 0;
			m_sRiva.riva.PMC[0x0068020C/4] = offset;
			m_sRiva.riva.PMC[0x00680210/4] = offset;
			m_sRiva.riva.PMC[0x00680214/4] = pitch;
			m_sRiva.riva.PMC[0x00680218/4] = pitch;
			m_sRiva.riva.PMC[0x00680230/4] = (cDst.top << 16) | cDst.left;
			m_sRiva.riva.PMC[0x00680234/4] = (cDst.Height() << 16) | cDst.Width();
			m_sRiva.riva.PMC[0x00680200/4] = ( (uint16)(((cSize.y) << 11) / cDst.Height() ) << 16 ) | (uint16)( ((cSize.x) << 11) / cDst.Width() );
			m_sRiva.riva.PMC[0x00680280/4] = 0x69;
			m_sRiva.riva.PMC[0x00680284/4] = 0x3e;
			m_sRiva.riva.PMC[0x00680288/4] = 0x89;
			m_sRiva.riva.PMC[0x0068028C/4] = 0x00000;
			m_sRiva.riva.PMC[0x00680204/4] = 0x001;
			m_sRiva.riva.PMC[0x00680208/4] = 0x111;
			m_sRiva.riva.PMC[0x0068023C/4] = 0x03;
			m_sRiva.riva.PMC[0x00680238/4] = 0x38;
			m_sRiva.riva.PMC[0x0068021C/4] = 0;
			m_sRiva.riva.PMC[0x00680220/4] = 0;
			m_sRiva.riva.PMC[0x00680240/4] = m_nColorKey & 0xffffff;
			m_sRiva.riva.PMC[0x00680244/4] = 0x11;
			m_sRiva.riva.PMC[0x00680228/4] = 1 << 16;
		}
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

bool NVidia::RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, area_id *pBuffer )
{
	
	if( eFormat == CS_YUV422 ) {
		delete_area( *pBuffer );
		/* Calculate offset */
    	uint32 pitch = 0; 
    	uint32 totalSize = 0;
    	
    	pitch = ( ( cSize.x << 1 ) + 3 )  & ~3;
    	totalSize = pitch * cSize.y;
    	
    	uint32 offset = PAGE_ALIGN( ( m_sRiva.riva.RamAmountKBytes - 128 ) * 1024 - totalSize - PAGE_SIZE );
		
		*pBuffer = create_area( "nvidia_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		remap_area( *pBuffer, (void*)((m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK) + offset) );
		
		if( m_sRiva.riva.Architecture >= NV_ARCH_10 ) {
			m_sRiva.riva.PMC[0x00008910/4] = 4096;
			m_sRiva.riva.PMC[0x00008914/4] = 4096;
			m_sRiva.riva.PMC[0x00008918/4] = 4096;
			m_sRiva.riva.PMC[0x0000891C/4] = 4096;
			m_sRiva.riva.PMC[0x00008b00/4] = m_nColorKey & 0xffffff;
		
			m_sRiva.riva.PMC[0x8900/4] = offset;
			m_sRiva.riva.PMC[0x8908/4] = offset + totalSize;
		
			if( m_sRiva.riva.Architecture >= NV_ARCH_20 ) {
				m_sRiva.riva.PMC[0x8800/4] = offset;
				m_sRiva.riva.PMC[0x8808/4] = offset + totalSize;
			}
			m_sRiva.riva.PMC[0x8920/4] = 0;
		
			m_sRiva.riva.PMC[0x8928/4] = (cSize.y << 16) | cSize.x;
			m_sRiva.riva.PMC[0x8930/4] = 0;
			m_sRiva.riva.PMC[0x8938/4] = (cSize.x << 20) / cDst.Width();
			m_sRiva.riva.PMC[0x8940/4] = (cSize.y << 20) / cDst.Height();
			m_sRiva.riva.PMC[0x8948/4] = (cDst.top << 16) | cDst.left;
			m_sRiva.riva.PMC[0x8950/4] = (cDst.Height() << 16) | cDst.Width();
		
			uint32 dstPitch = ( ( ( cSize.x << 1 ) + 63 )  & ~63 ) | 1 << 20;
			m_sRiva.riva.PMC[0x8958/4] = dstPitch;	
			m_sRiva.riva.PMC[0x00008704/4] = 0;
			m_sRiva.riva.PMC[0x8704/4] = 0xfffffffe;
			m_sRiva.riva.PMC[0x8700/4] = 1;
		} else {
			/* NV3/NV4 Overlay */
			m_sRiva.riva.PMC[0x00680224/4] = 0;
			m_sRiva.riva.PMC[0x00680228/4] = 0;
			m_sRiva.riva.PMC[0x0068022C/4] = 0;
			m_sRiva.riva.PMC[0x0068020C/4] = offset;
			m_sRiva.riva.PMC[0x00680210/4] = offset;
			m_sRiva.riva.PMC[0x00680214/4] = pitch;
			m_sRiva.riva.PMC[0x00680218/4] = pitch;
			m_sRiva.riva.PMC[0x00680230/4] = (cDst.top << 16) | cDst.left;
			m_sRiva.riva.PMC[0x00680234/4] = (cDst.Height() << 16) | cDst.Width();
			m_sRiva.riva.PMC[0x00680200/4] = ( (uint16)(((cSize.y) << 11) / cDst.Height() ) << 16 ) | (uint16)( ((cSize.x) << 11) / cDst.Width() );
			m_sRiva.riva.PMC[0x00680280/4] = 0x69;
			m_sRiva.riva.PMC[0x00680284/4] = 0x3e;
			m_sRiva.riva.PMC[0x00680288/4] = 0x89;
			m_sRiva.riva.PMC[0x0068028C/4] = 0x00000;
			m_sRiva.riva.PMC[0x00680204/4] = 0x001;
			m_sRiva.riva.PMC[0x00680208/4] = 0x111;
			m_sRiva.riva.PMC[0x0068023C/4] = 0x03;
			m_sRiva.riva.PMC[0x00680238/4] = 0x38;
			m_sRiva.riva.PMC[0x0068021C/4] = 0;
			m_sRiva.riva.PMC[0x00680220/4] = 0;
			m_sRiva.riva.PMC[0x00680240/4] = m_nColorKey & 0xffffff;
			m_sRiva.riva.PMC[0x00680244/4] = 0x11;
			m_sRiva.riva.PMC[0x00680228/4] = 1 << 16;
		}
		
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

void NVidia::UpdateVideoOverlay( area_id *pBuffer )
{
}

//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void NVidia::DeleteVideoOverlay( area_id *pBuffer )
{
	if( m_bVideoOverlayUsed ) {
		delete_area( *pBuffer );
		
		if( m_sRiva.riva.Architecture >= NV_ARCH_10 ) {
			m_sRiva.riva.PMC[0x00008704/4] = 1;
		} else {
			m_sRiva.riva.PMC[0x00680244/4] = m_sRiva.riva.PMC[0x00680244/4] & ~1;
			m_sRiva.riva.PMC[0x00680224/4] = 0;
			m_sRiva.riva.PMC[0x00680228/4] = 0;
			m_sRiva.riva.PMC[0x0068022C/4] = 0;
		}
	}
	m_bVideoOverlayUsed = false;
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
	
	m_sRiva.riva.LockUnlock(&m_sRiva.riva, 0);
	m_sRiva.riva.LoadStateExt(&m_sRiva.riva, state);
}

//-----------------------------------------------------------------------------

void NVidia::SetupAccel()
{
	m_cGELock.Lock();
	RIVA_FIFO_FREE(m_sRiva.riva, Clip, 2);
	m_sRiva.riva.Clip->TopLeft = 0x0;
	m_sRiva.riva.Clip->WidthHeight = 0x7fff7fff;
	SetupROP();
	WaitForIdle();
	m_cGELock.Unlock();
}

//-----------------------------------------------------------------------------

void NVidia::SetupROP()
{
	RIVA_FIFO_FREE(m_sRiva.riva, Patt, 5);
	m_sRiva.riva.Patt->Shape = 0;
	m_sRiva.riva.Patt->Color0 = 0xffffffff;
	m_sRiva.riva.Patt->Color1 = 0xffffffff;
	m_sRiva.riva.Patt->Monochrome[0] = 0xffffffff;
	m_sRiva.riva.Patt->Monochrome[1] = 0xffffffff;
	
	RIVA_FIFO_FREE(m_sRiva.riva, Rop, 1);
	m_sRiva.riva.Rop->Rop3 = 0xCC;
}

//-----------------------------------------------------------------------------

Bool NVidia::NVIsConnected(Bool second)
{
#if 0
	if (second)
		return 1;
	else
		return 0;
#else
    volatile U032 *PRAMDAC = m_sRiva.riva.PRAMDAC0;
    U032 reg52C, reg608;
    Bool present;

    if(second) PRAMDAC += 0x800;

    reg52C = PRAMDAC[0x052C/4];
    reg608 = PRAMDAC[0x0608/4];

    PRAMDAC[0x0608/4] = reg608 & ~0x00010000;

    PRAMDAC[0x052C/4] = reg52C & 0x0000FEEE;
    snooze(1000);
    PRAMDAC[0x052C/4] |= 1;

    m_sRiva.riva.PRAMDAC0[0x0610/4] = 0x94050140;
    m_sRiva.riva.PRAMDAC0[0x0608/4] |= 0x00001000;

    snooze(1000);

    present = (PRAMDAC[0x0608/4] & (1 << 28)) ? TRUE : FALSE;

    m_sRiva.riva.PRAMDAC0[0x0608/4] &= 0x0000EFFF;

    PRAMDAC[0x052C/4] = reg52C;
    PRAMDAC[0x0608/4] = reg608;

    return present;
#endif
}

void NVidia::NVOverrideCRTC()
{
    dbprintf("nVidia :: Detected CRTC controller %i being used\n", m_sRiva.SecondCRTC ? 1 : 0);
    if(m_sRiva.forceCRTC != -1) {
        dbprintf("nVidia :: Forcing usage of CRTC %i\n", m_sRiva.forceCRTC);
        m_sRiva.SecondCRTC = m_sRiva.forceCRTC;
    }
}

void NVidia::NVIsSecond()
{
    if(m_sRiva.FlatPanel == 1) {
		switch(m_sRiva.Chipset) {
			case 0x0112:
			case 0x0113:
			case 0x0174:
			case 0x0175:
			case 0x0176:
			case 0x0177:
			case 0x0178:
			case 0x0179:
			case 0x017D:
			case 0x017C:
			case 0x0186:
			case 0x0187:
			case 0x0286:
			case 0x028C:
				m_sRiva.SecondCRTC = TRUE;
				break;
			default:
				m_sRiva.SecondCRTC = FALSE;
				break;
		}
    } else {
       if(NVIsConnected(0)) {
          if(m_sRiva.riva.PRAMDAC0[0x0000052C/4] & 0x100)
             m_sRiva.SecondCRTC = TRUE;
          else
             m_sRiva.SecondCRTC = FALSE;
       } else 
       if (NVIsConnected(1)) {
          if(m_sRiva.riva.PRAMDAC0[0x0000252C/4] & 0x100)
             m_sRiva.SecondCRTC = TRUE;
          else
             m_sRiva.SecondCRTC = FALSE;
       } else /* default */
          m_sRiva.SecondCRTC = FALSE;
    }

    NVOverrideCRTC();
}

void NVidia::NVCommonSetup()
{
//	m_sRiva.Dac.LoadPalette = NVDACLoadPalette;

    /*
     * No IRQ in use.
     */
    m_sRiva.riva.EnableIRQ = 0;
    /*
     * Map remaining registers. This MUST be done in the OS specific driver code.
     */
//	m_sRiva.riva.IO      = VGA_IOBASE_COLOR;

    m_sRiva.riva.PRAMDAC0	= (unsigned	*)(m_pRegisterBase+0x00680000);
    m_sRiva.riva.PFB		= (unsigned	*)(m_pRegisterBase+0x00100000);
    m_sRiva.riva.PFIFO		= (unsigned	*)(m_pRegisterBase+0x00002000);
    m_sRiva.riva.PGRAPH		= (unsigned	*)(m_pRegisterBase+0x00400000);
    m_sRiva.riva.PEXTDEV	= (unsigned	*)(m_pRegisterBase+0x00101000);
    m_sRiva.riva.PTIMER		= (unsigned	*)(m_pRegisterBase+0x00009000);
    m_sRiva.riva.PMC		= (unsigned	*)(m_pRegisterBase+0x00000000);
    m_sRiva.riva.FIFO		= (unsigned	*)(m_pRegisterBase+0x00800000);
    m_sRiva.riva.PCIO0		= (U008		*)(m_pRegisterBase+0x00601000);
    m_sRiva.riva.PDIO0		= (U008		*)(m_pRegisterBase+0x00681000);
    m_sRiva.riva.PVIO		= (U008		*)(m_pRegisterBase+0x000C0000);

    if(m_sRiva.FlatPanel == -1) {
       switch(m_sRiva.Chipset) {
			case 0x0112:
			case 0x0113:
			case 0x0174:
			case 0x0175:
			case 0x0176:
			case 0x0177:
			case 0x0178:
			case 0x0179:
			case 0x017D:
			case 0x017C:
			case 0x0186:
			case 0x0187:
			case 0x0286:
			case 0x028C:
			case 0x0316:
    		case 0x0317:
    		case 0x031A:
    		case 0x031B:
    		case 0x031C:
    		case 0x031D:
    		case 0x031E:
   			case 0x031F:
    		case 0x0324:
    		case 0x0325:
    		case 0x0328:
    		case 0x0329:
    		case 0x032C:
    		case 0x032D:
		    case 0x0347:
    		case 0x0348:
    		case 0x0349:
    		case 0x034B:
    		case 0x034C:
				dbprintf("nVidia :: On a laptop - assuming Digital Flat Panel\n");
				m_sRiva.FlatPanel = 1;
				break;
			default:
				break;
       }
    }

    switch(m_sRiva.Chipset & 0x0ff0) {
    case 0x0110:
        if((m_sRiva.Chipset&0xFFFF) == 0x112)
            m_sRiva.SecondCRTC = TRUE;
        NVOverrideCRTC();
        break;
    case 0x0170:
    case 0x0250:
	case 0x0280:
        NVIsSecond();
        break;
    default:
        break;
    }

    if(m_sRiva.SecondCRTC) {
       m_sRiva.riva.PCIO	= m_sRiva.riva.PCIO0	+ 0x2000;
       m_sRiva.riva.PCRTC	= m_sRiva.riva.PCRTC0	+ 0x0800;
       m_sRiva.riva.PRAMDAC	= m_sRiva.riva.PRAMDAC0	+ 0x0800;
       m_sRiva.riva.PDIO	= m_sRiva.riva.PDIO0	+ 0x2000;
    } else {
       m_sRiva.riva.PCIO	= m_sRiva.riva.PCIO0;
       m_sRiva.riva.PCRTC	= m_sRiva.riva.PCRTC0;
       m_sRiva.riva.PRAMDAC	= m_sRiva.riva.PRAMDAC0;
       m_sRiva.riva.PDIO	= m_sRiva.riva.PDIO0;
    }

    RivaGetConfig(&m_sRiva);

//	m_sRiva.Dac.maxPixelClock = m_sRiva.riva.MaxVClockFreqKHz;

    m_sRiva.riva.LockUnlock(&m_sRiva.riva, 0);

    if(m_sRiva.FlatPanel == -1) {
        m_sRiva.FlatPanel = 0;
    }
    m_sRiva.riva.flatPanel = (m_sRiva.FlatPanel > 0) ? TRUE : FALSE;
}

void NVidia::NV1Setup()
{
}

void NVidia::NV3Setup()
{
	dbprintf("nVidia :: NV3Setup\n");
	m_sRiva.riva.Architecture = 3;
	m_sRiva.riva.PRAMIN = (unsigned	*)(m_pRegisterBase+0x00C00000);
	NVCommonSetup();
	m_sRiva.riva.PCRTC = m_sRiva.riva.PCRTC0 = m_sRiva.riva.PGRAPH;
}

void NVidia::NV4Setup()
{
	dbprintf("nVidia :: NV4Setup\n");
	m_sRiva.riva.Architecture = NV_ARCH_04;
	m_sRiva.riva.PRAMIN = (unsigned	*)(m_pRegisterBase+0x00710000);
	m_sRiva.riva.PCRTC0 = (unsigned	*)(m_pRegisterBase+0x00600000);
	NVCommonSetup();
}

void NVidia::NV10Setup()
{
	dbprintf("nVidia :: NV10Setup\n");
	m_sRiva.riva.Architecture = NV_ARCH_10;
	m_sRiva.riva.PRAMIN = (unsigned	*)(m_pRegisterBase+0x00710000);
	m_sRiva.riva.PCRTC0 = (unsigned	*)(m_pRegisterBase+0x00600000);
	NVCommonSetup();
}

void NVidia::NV20Setup()
{
	dbprintf("nVidia :: NV20Setup\n");
	m_sRiva.riva.Architecture = NV_ARCH_20;
	m_sRiva.riva.PRAMIN = (unsigned	*)(m_pRegisterBase+0x00710000);
	m_sRiva.riva.PCRTC0 = (unsigned	*)(m_pRegisterBase+0x00600000);
	NVCommonSetup();
}


void NVidia::NV30Setup()
{
	dbprintf("nVidia :: NV30Setup\n");
	m_sRiva.riva.Architecture = NV_ARCH_30;
	m_sRiva.riva.PRAMIN = (unsigned	*)(m_pRegisterBase+0x00710000);
	m_sRiva.riva.PCRTC0 = (unsigned	*)(m_pRegisterBase+0x00600000);
	NVCommonSetup();
}

//-----------------------------------------------------------------------------

extern "C" DisplayDriver* init_gfx_driver( int nFd )
{
    dbprintf("nvidia attempts to initialize\n");
    
    try {
	    NVidia *pcDriver = new NVidia( nFd );
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




















