/*
 *  Syllable Driver for VMware virtual video card
 *
 *  Based on the the xfree86 VMware driver.
 *
 *  Syllable specific code is
 *  Copyright (C) 2004, James Hayhurst (james_hayhurst@hotmail.com)
 *
 *  Other code is
 *  Copyright (C) 1998-2001 VMware, Inc.
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
 *
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/vesa_gfx.h>
#include <atheos/udelay.h>
#include <atheos/areas.h>

#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <gui/bitmap.h>

#include "ChipInfo.h"
#include "CursorInfo.h"
#include "guest_os.h"
#include "svga_reg.h"
#include "vmware.h"
#include "vmware_pci_ids.h"


static const struct ChipInfo supportedChips[] =
{
	{VMWARE_CHIP_SVGA	, 0,	"VMware SVGA"},
	{VMWARE_CHIP_SVGA2	, 0,	"VMware SVGA2"}
};

static int numSupportedChips = sizeof(supportedChips)
									/ sizeof(ChipInfo);

/******************** Constructor and Destructor ********************/
/********************************************************************/
VMware::VMware()
:	m_cGELock("VMware_ge_lock")
{
	int pciItor, chipItor;
	bool vidCardFound = false;

	/* Give safe values to all the class data members */
	InitMembers();

	for(pciItor = 0; get_pci_info(&m_cPciInfo, pciItor) == 0; pciItor++)
	{
		m_vmChipInfo.nChipId = (m_cPciInfo.nVendorID << 16) | (m_cPciInfo.nDeviceID);

		for(chipItor = 0; chipItor < numSupportedChips; chipItor++)
		{
			if(m_vmChipInfo.nChipId == supportedChips[chipItor].nChipId)
			{
				vidCardFound = true;
				m_vmChipInfo.nArchRev = supportedChips[chipItor].nArchRev;
				m_vmChipInfo.pzName = supportedChips[chipItor].pzName;
				break;
			}
		}

		if(vidCardFound)
			break;
	}

	if(!vidCardFound)
	{
		printf("VMware - Did not find any VMware video cards.\n");
		return;
	}

	dbprintf("VMware - Found match - PCI %d: Vendor 0x%04x "
				"Device 0x%04x - Rev 0x%04x\n", pciItor,
				m_cPciInfo.nVendorID, m_cPciInfo.nDeviceID,
				m_cPciInfo.nRevisionID);

	/* Initialize the hardware.  This will setup the
	 * IO ports and write to m_regId */
	if(!InitHardware())
	{
		dbprintf("VMware - Unable to initialize hardware.\n");
		vmwareWriteReg(SVGA_REG_CONFIG_DONE, 0);
		vmwareWriteReg(SVGA_REG_ENABLE, 0);
		return;
	}

	dbprintf("VMware - Host bpp is %d\n", (int) m_regHostBPP);
	dbprintf("VMware - Current Guest bpp is set to %d\n", (int) m_regBitsPerPixel);
	dbprintf("VMware - Only allowing %d bpp\n", (int) m_regBitsPerPixel);

	int bpp;
	color_space colspace;
	float rf[] = {60.0f};
	if(m_regBitsPerPixel == 16)
	{
		bpp = 2;
		colspace = CS_RGB16;
	}
	else if(m_regBitsPerPixel == 32)
	{
		bpp = 4;
		colspace = CS_RGB32;
	}
	else
	{
		dbprintf("VMware - m_regBitsPerPixel is unexpected value of %d\n",
					(int) m_regBitsPerPixel);
		return;
	}

	for(int j = 0; j < 4; j++)
	{
		m_cScreenModeList.push_back(os::screen_mode(640, 480, 640 * bpp, colspace, rf[j]));
		m_cScreenModeList.push_back(os::screen_mode(800, 600, 800 * bpp, colspace, rf[j]));
		m_cScreenModeList.push_back(os::screen_mode(1024, 768, 1024 * bpp, colspace, rf[j]));
	}

	m_bIsInitiated = true;
	return;
}


VMware::~VMware()
{
	vmwareWriteReg(SVGA_REG_CONFIG_DONE, 0);
	vmwareWriteReg(SVGA_REG_ENABLE, 0);

	if(m_hFifoArea != -1)
	{
		delete_area(m_hFifoArea);
	}

	if(m_hFbArea != -1)
	{
		delete_area(m_hFbArea);
	}
}
/********************************************************************/
/********************************************************************/



/************************** Public Methods **************************/
/********************************************************************/
area_id VMware::Open()
{
	return m_hFbArea;
}


void VMware::Close()
{
	vmwareWriteReg(SVGA_REG_CONFIG_DONE, 0);
	vmwareWriteReg(SVGA_REG_ENABLE, 0);
}


int VMware::GetScreenModeCount()
{
	return m_cScreenModeList.size();
}


bool VMware::GetScreenModeDesc(int nIndex, os::screen_mode* psMode)
{
	if(nIndex<0 || uint(nIndex) >= m_cScreenModeList.size())
	{
		return false;
	}

	*psMode = m_cScreenModeList[nIndex];
	return true;
}


os::screen_mode VMware::GetCurrentScreenMode()
{
	return m_sCurrentMode;
}


int VMware::SetScreenMode(os::screen_mode sMode)
{
	uint32 newBpp = 0;

	/* FIXME - check the values before setting mode */
	if(sMode.m_eColorSpace == CS_RGB32)
		newBpp = 32;
	else if(sMode.m_eColorSpace == CS_RGB16)
		newBpp = 16;
	else
	{
		dbprintf("VMware::SetScreenMode() - Unsupported color space %d\n",
					sMode.m_eColorSpace);
		return -1;
	}

	if((SVGA_CAP_8BIT_EMULATION & m_regCapabilities) == 0)
	{
		dbprintf("VMware::SetScreenMode() - SVGA_CAP_8BIT_EMULATION not detected! SVGA_REG_BITS_PER_PIXEL is read only!\n");
	}

	if(newBpp != m_regHostBPP)
	{
		dbprintf("VMware::SetScreenMode() - Trying to set bits per pixel to an invalid value(%d bpp).\n",
					(int) newBpp);
		dbprintf("\tMust either be the same as HOST_BITS_PER_PIXEL(%d bpp)\n",
					(int) m_regHostBPP);
		return -1;
	}

	/* FIXME - check return value from this */
	SetMode(sMode.m_nWidth, sMode.m_nHeight, newBpp);

	m_sCurrentMode = sMode;
	m_sCurrentMode.m_nBytesPerLine = m_regBytesPerLine;

	return 0;
}


int VMware::GetFramebufferOffset()
{
	return m_regFbOffset;
}


static int makeAnd1(uint8* cursRaster, CursorInfo *cursInfo)
{
	int i, andIndex, bitMask, cursByteLen;
	uint8 *andMask;

	if((cursRaster == NULL) || (cursInfo == NULL))
		return 1;

	andMask = (uint8 *)cursInfo->andMask;

	/* Zero out each word so we don't have to set 0's */
	// for(i = 0; i < cursInfo->andSize; i++)
	//	andMask[i] = 0x00000000;

	/* Length in bytes of the cursor (does not include padding bytes) */
	cursByteLen = cursInfo->cursWidth * cursInfo->cursHeight;
	for(i = 0; i < cursByteLen; i++)
	{
		andIndex = ((i / cursInfo->cursWidth) * 4) + ((i % cursInfo->cursWidth) / 8);
		bitMask = 0x80 >> ((i % cursInfo->cursWidth) % 8);

		if(cursRaster[i] == 0)
		{
			// Transparent - Set bit to 1
			andMask[andIndex] |= bitMask;
		}
		else if((cursRaster[i] == 1) || (cursRaster[i] == 2))
		{
			// Black - Set bit to 0
			andMask[andIndex] &= ~(bitMask);
		}
		else if(cursRaster[i] == 3)
		{
			// White - Set bit to 0
			andMask[andIndex] &= ~(bitMask);
		}
	}

	return 0;
}


static int makeXor1(uint8* cursRaster, CursorInfo *cursInfo)
{
	int i, xorIndex, bitMask, cursByteLen;
	uint8 *xorMask;

	if((cursRaster == NULL) || (cursInfo == NULL))
		return 1;

	xorMask = (uint8 *)cursInfo->xorMask;

	/* Zero out each word so we don't have to set 0's */
	// for(i = 0; i < cursInfo->andSize; i++)
	//	xorMask[i] = 0x00000000;

	/* Length in bytes of the cursor (does not include padding bytes) */
	cursByteLen = cursInfo->cursWidth * cursInfo->cursHeight;
	for(i = 0; i < cursByteLen; i++)
	{
		xorIndex = ((i / cursInfo->cursWidth) * 4) + ((i % cursInfo->cursWidth) / 8);
		bitMask = 0x80 >> ((i % cursInfo->cursWidth) % 8);

		if(cursRaster[i] == 0)
		{
			// Transparent - Set bit to 0
			xorMask[xorIndex] &= ~(bitMask);
		}
		else if((cursRaster[i] == 1) || (cursRaster[i] == 2))
		{
			// Black - Set bit to 0
			xorMask[xorIndex] &= ~(bitMask);
		}
		else if(cursRaster[i] == 3)
		{
			// White - Set bit to 1
			xorMask[xorIndex] |= bitMask;
		}
	}

	return 0;
}


bool VMware::DrawLine(SrvBitmap* pcBitmap, const os::IRect& cClipRect,
	const os::IPoint& cPnt1, const os::IPoint& cPnt2,
	const os::Color32_s& sColor, int nMode)
{
	if(!pcBitmap->m_bVideoMem)
	{
		/* Off-screen */
		return DisplayDriver::DrawLine(pcBitmap, cClipRect, cPnt1, cPnt2,
										sColor, nMode);
	}

	int x1 = cPnt1.x;
	int y1 = cPnt1.y;
	int x2 = cPnt2.x;
	int y2 = cPnt2.y;
	int dstX, dstY;
	int width, height;

	if(!DisplayDriver::ClipLine(cClipRect, &x1, &y1, &x2, &y2))
	{
		return false;
	}

	if(x1 < x2)
	{
		dstX = x1;
		width = x2 - x1 + 1;
	}
	else
	{
		dstX = x2;
		width = x1 - x2 + 1;
	}

	if(y1 < y2)
	{
		dstY = y1;
		height = y2 - y1 + 1;
	}
	else
	{
		dstY = y2;
		height = y1 - y2 + 1;
	}


	// ACQUIRE lock
	m_cGELock.Lock();

	if(m_bFifoCmds)
		FifoSync();

	DisplayDriver::DrawLine(pcBitmap, cClipRect, cPnt1, cPnt2, sColor, nMode);
	Fifo_UpdateRect(dstX, dstY, width, height);

	// RELEASE lock
	m_cGELock.Unlock();
	return true;
}


bool VMware::FillRect(SrvBitmap *pcBitmap, const IRect& cRect,
	const Color32_s& sColor, int nMode)
{
	if(!pcBitmap->m_bVideoMem || nMode != DM_COPY)
	{
		/* Off-screen */
		return DisplayDriver::FillRect(pcBitmap, cRect, sColor, nMode);
	}

	int dstX = cRect.left;
	int dstY = cRect.top;
	int width = cRect.Width() + 1;
	int height = cRect.Height() + 1;
	int nColor;

	if (pcBitmap->m_eColorSpc == CS_RGB32)
		nColor = COL_TO_RGB32(sColor);
	else
		nColor = COL_TO_RGB16(sColor);

	bool accelOp = false;
	// accelOp = m_regCapabilities & SVGA_CAP_RECT_FILL;

	// ACQUIRE lock
	m_cGELock.Lock();

	if(accelOp)
	{
		/* Accelerated on-screen RectFill */
		Fifo_RectFill(dstX, dstY, width, height, nColor);
		m_bFifoCmds = true;
	}
	else
	{
		if(m_bFifoCmds)
			FifoSync();

		DisplayDriver::FillRect(pcBitmap, cRect, sColor, nMode);
		Fifo_UpdateRect(dstX, dstY, width, height);
	}

	// RELEASE lock
	m_cGELock.Unlock();
	return true;
}


bool VMware::BltBitmap(SrvBitmap *pcDstBitmap, SrvBitmap *pcSrcBitmap,
	IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha)
{
	if(((!pcDstBitmap->m_bVideoMem) && (!pcSrcBitmap->m_bVideoMem)) || nMode != DM_COPY || cSrcRect.Size() != cDstRect.Size())
	{
		// Off-screen to off-screen
		return DisplayDriver::BltBitmap(pcDstBitmap, pcSrcBitmap, cSrcRect,
										cDstRect, nMode, nAlpha);
	}

	IPoint cDstPos = cDstRect.LeftTop();
	int srcX = cSrcRect.left;
	int srcY = cSrcRect.top;
	int dstX = cDstPos.x;
	int dstY = cDstPos.y;
	int width = cSrcRect.Width() + 1;
	int height = cSrcRect.Height() + 1;

	bool accelDmCopy = false;
	if((pcDstBitmap->m_bVideoMem) && (pcSrcBitmap->m_bVideoMem))
	{
		if(nMode == DM_COPY)
			accelDmCopy = m_regCapabilities & SVGA_CAP_RECT_COPY;
		else if(nMode == DM_OVER)
			dbprintf("VMware::BltBitmap() - Screen to screen DM_OVER\n");
		else if(nMode == DM_BLEND)
			dbprintf("VMware::BltBitmap() - Screen to screen DM_BLEND\n");
		else
			dbprintf("VMware::BltBitmap() - Unknown nMode = %d\n", nMode);
	}

	// ACQUIRE lock
	m_cGELock.Lock();

	if(accelDmCopy)
	{
		// Accelerated screen to screen DM_COPY
		Fifo_RectCopy(srcX, srcY, dstX, dstY, width, height);
		m_bFifoCmds = true;
	}
	else
	{
		if(m_bFifoCmds)
			FifoSync();

		DisplayDriver::BltBitmap(pcDstBitmap, pcSrcBitmap, cSrcRect,
									cDstRect, nMode, nAlpha);

		if(pcDstBitmap->m_bVideoMem)
			Fifo_UpdateRect(dstX, dstY, width, height);
	}

	// RELEASE lock
	m_cGELock.Unlock();
	return true;
}

void VMware::UnlockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrc, os::IRect cDst )
{
	if( pcDstBitmap->m_bVideoMem == false )
		return;
		
	// ACQUIRE lock
	m_cGELock.Lock();

	if(m_bFifoCmds)
		FifoSync();
	
	Fifo_UpdateRect(cDst.left, cDst.top, cDst.Width()+1,
					cDst.Height()+1);
		
	// RELEASE lock
	m_cGELock.Unlock();
}

/********************************************************************/
/********************************************************************/



/************************** Private Methods *************************/
/********************************************************************/
void VMware::InitMembers(void)
{
	m_bIsInitiated = false;
	/* 	m_cGELock; */ // Already initialized by constructor

	/* m_cScreenModeList; */
	/* os::screen_mode m_sCurrentMode; */

	/* m_cPciInfo; */
	memset(&m_vmChipInfo, 0, sizeof(struct ChipInfo));

	/* IO ports */
	m_IndexPort = 0;
	m_ValuePort = 0;

	/* Frame Buffer */
	m_pFbBase = NULL;
	m_hFbArea = -1;

	/* Command FIFO */
	m_bFifoCmds = false;
	m_pFifoBase = NULL;
	m_hFifoArea = -1;

	m_regId = SVGA_ID_INVALID;
	m_regEnable = 0;
	m_regWidth = 0;
	m_regHeight = 0;
	m_regMaxWidth = 0;
	m_regMaxHeight = 0;
	m_regDepth = 0;
	m_regBitsPerPixel = 0;
	m_regPseudoColor = 0;
	m_regRedMask = 0;
	m_regGreenMask = 0;
	m_regBlueMask = 0;
	m_regBytesPerLine = 0;
	m_regFbStart = 0;
	m_regFbOffset = 0;
	m_regVramSize = 0;
	m_regFbSize = 0;
	m_regCapabilities = 0;
	m_regMemStart = 0;
	m_regMemSize = 0;
	m_regConfigDone = 0;
	m_regSync = 0;
	m_regBusy = 0;
	m_regGuestId = 0;
	m_regCursorId = 0;
	m_regCursorX = 0;
	m_regCursorY = 0;
	m_regCursorOn = 0;
	m_regHostBPP = 0;

	m_bCursorIsOn = false;
	m_bUsingHwCursor = false;
}


/* Returns true/false if initialization was successful */
bool VMware::InitHardware(void)
{
	/* This has to be done first, otherwise we can't
	 * access the SVGA registers */
	if(!setupIoPorts())
	{
		dbprintf("VMware - Failed to setup the IO ports.\n");
		return false;
	}


	/*** Grab the SVGA Id ***/
	m_regId = vmwareGetSvgaId();
	if(m_regId == (uint32)SVGA_ID_2)
	{
		dbprintf("VMware - SVGA Id is SVGA_ID_2\n");
	}
	else if(m_regId == (uint32)SVGA_ID_1)
	{
		dbprintf("VMware - SVGA Id is SVGA_ID_1\n");
		return false;
	}
	else if(m_regId == (uint32)SVGA_ID_0)
	{
		dbprintf("VMware - SVGA Id is SVGA_ID_0\n");
		return false;
	}
	else
	{
		dbprintf("VMware - SVGA Id is SVGA_ID_INVALID\n");
		return false;
	}

	/*** Map the frame buffer and the command FIFO ***/
	if(!CreateFb())
	{
		dbprintf("VMware -Failed to map the frame buffer.\n");
		return false;
	}
	dbprintf("VMware - Successfully mapped the frame buffer.\n");


	if(!CreateFifo())
	{
		dbprintf("VMware - Failed to map the command FIFO.\n");
		return false;
	}
	dbprintf("VMware - Successfully mapped the command FIFO.\n");


	/*** Initialize the FB ***/
	if(!InitFb())
	{
		dbprintf("VMware - Failed to initialized the frame buffer.\n");
		return false;
	}
	dbprintf("VMware - Successfully initialized the frame buffer.\n");


	/*** Initialize the command FIFO ***/
	if(!InitFifo())
	{
		dbprintf("VMware - Failed to initialized the command FIFO.\n");
		return false;
	}
	dbprintf("VMware - Successfully initialized the command FIFO.\n");


	/*** Initially disable hardware cursor ***/
	vmwareWriteReg(SVGA_REG_CURSOR_X, 0);
	vmwareWriteReg(SVGA_REG_CURSOR_Y, 0);
	vmwareWriteReg(SVGA_REG_CURSOR_ON, SVGA_CURSOR_ON_HIDE);

	return true;
}
/********************************************************************/
/********************************************************************/


extern "C" DisplayDriver* init_gfx_driver()
{
	try
	{
		VMware *pcDriver = new VMware();

		if(pcDriver->IsInitiated())
		{
			return pcDriver;
		}

		return NULL;
	}
	catch(std::exception& cExc)
	{
		dbprintf("VMware - Got exception\n");
		return NULL;
	}
}
