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


#include <atheos/areas.h>
#include <atheos/kernel.h>
#include <atheos/pci.h>
#include <atheos/types.h>

#include <gui/rect.h>

#include "../../../server/ddriver.h"
#include "../../../server/sprite.h"

#include "ChipInfo.h"
#include "CursorInfo.h"


using namespace os;

#ifndef __VMWARE_H__
#define __VMWARE_H__

class VMware : public DisplayDriver
{
public:
	VMware();
	~VMware();

	virtual area_id Open();
	virtual void Close();

	virtual int GetScreenModeCount();
	virtual bool GetScreenModeDesc(int nIndex, os::screen_mode *psMode);
	virtual int SetScreenMode(os::screen_mode);
	virtual os::screen_mode GetCurrentScreenMode();

	virtual int GetFramebufferOffset();
	
	virtual void UnlockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrc, os::IRect cDst );

	virtual bool DrawLine(SrvBitmap* psBitMap, const os::IRect& cClipRect, const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode);
	virtual bool FillRect(SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor, int nMode);
	virtual bool BltBitmap(SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IRect cDstRect, int nMode, int nAlpha);

	bool IsInitiated() const
	{
		return(m_bIsInitiated);
	}


private:

	bool m_bIsInitiated;
	os::Locker m_cGELock;

	std::vector<os::screen_mode> m_cScreenModeList;
	os::screen_mode m_sCurrentMode;

	PCI_Info_s m_cPciInfo;
	ChipInfo m_vmChipInfo;

	/* IO ports */
	uint16 m_IndexPort;		/* VMware Index Port */
	uint16 m_ValuePort;		/* VMware Value Port */

	/* Frame Buffer */
	vuint8* m_pFbBase;		/* Indexed by bytes */
	area_id m_hFbArea;

	/* Command FIFO */
	bool m_bFifoCmds;		/* There are FB commands in the FIFO */
	vuint32* m_pFifoBase;	/* Indexed by dwords */
	area_id m_hFifoArea;

	/* SVGA Registers */
	uint32 m_regId;				/* SVGA_REG_ID 0 */
	uint32 m_regEnable;			/* SVGA_REG_ENABLE 1 */
	uint32 m_regWidth;			/* SVGA_REG_WIDTH 2 */
	uint32 m_regHeight;			/* SVGA_REG_HEIGHT 3 */
	uint32 m_regMaxWidth;		/* SVGA_REG_MAX_WIDTH 4 */
	uint32 m_regMaxHeight;		/* SVGA_REG_MAX_HEIGHT 5 */
	uint32 m_regDepth;			/* SVGA_REG_DEPTH 6 */
	uint32 m_regBitsPerPixel;	/* SVGA_REG_DEPTH 7 Current bpp in the guest*/
	uint32 m_regPseudoColor;	/* SVGA_REG_PSEUDOCOLOR 8 */
	uint32 m_regRedMask;		/* SVGA_REG_RED_MASK 9 */
	uint32 m_regGreenMask;		/* SVGA_REG_GREEN_MASK 10 */
	uint32 m_regBlueMask;		/* SVGA_REG_BLUE_MASK 11 */
	uint32 m_regBytesPerLine;	/* SVGA_REG_BYTES_PER_LINE 12 */
	uint32 m_regFbStart;		/* SVGA_REG_FB_START 13 */
	uint32 m_regFbOffset;		/* SVGA_REG_FB_OFFSET 14 */
	uint32 m_regVramSize;		/* SVGA_REG_VRAM_SIZE 15 */
	uint32 m_regFbSize;			/* SVGA_REG_FB_SIZE 16 */
	uint32 m_regCapabilities;	/* SVGA_REG_CAPABILITIES 17 */
	uint32 m_regMemStart;		/* SVGA_REG_MEM_START 18 Memory for command FIFO and bitmaps */
	uint32 m_regMemSize;		/* SVGA_REG_MEM_SIZE 19 */
	uint32 m_regConfigDone;		/* SVGA_REG_CONFIG_DONE 20 Set when memory area configured */
	uint32 m_regSync;			/* SVGA_REG_SYNC 21 Write to force synchronization */
	uint32 m_regBusy;			/* SVGA_REG_BUSY 22 Read to check if sync is done */
	uint32 m_regGuestId;		/* SVGA_REG_GUEST_ID 23 Set guest OS identifier */
	uint32 m_regCursorId;		/* SVGA_REG_CURSOR_ID 24 ID of cursor */
	uint32 m_regCursorX;		/* SVGA_REG_CURSOR_X 25 Set cursor X position */
	uint32 m_regCursorY;		/* SVGA_REG_CURSOR_Y 26 Set cursor Y position */
	uint32 m_regCursorOn;		/* SVGA_REG_CURSOR_ON 27 Turn cursor on/off */
	uint32 m_regHostBPP;		/* SVGA_REG_HOST_BITS_PER_PIXEL 28 Current bpp in the host */

	bool m_bCursorIsOn;
	bool m_bUsingHwCursor;
	os::IPoint m_CursorPos;
	os::IPoint m_CursorHotSpot;

	/* These methods are implemented in vmware_fb.cpp */
	/**************************************************/
	bool CreateFb(void);
	bool InitFb(void);
	bool SetMode(uint32 width, uint32 height, uint32 bpp);
	/**************************************************/
	/**************************************************/


	/* These methods are implemented in vmware_fifo.cpp */
	/****************************************************/
	bool CreateFifo(void);
	bool InitFifo(void);
	void FifoSync(void);
	void Fifo_WriteWord(uint32 value);
	void Fifo_UpdateRect(uint32 dstX, uint32 dstY, uint32 width, uint32 height);

	void Fifo_RectFill(uint32 dstX, uint32 dstY, uint32 width, uint32 height,
						uint32 color);
	void Fifo_RectCopy(uint32 srcX, uint32 srcY, uint32 dstX, uint32 dstY,
						uint32 width, uint32 height);
	/****************************************************/
	/****************************************************/


	/* These methods are implemented in vmware_cursor.cpp */
	/******************************************************/
	bool Fifo_DefineCursor(uint32 cursorId, CursorInfo *cursInfo);
	bool Fifo_DefineAlphaCursor(uint32 cursorId, CursorInfo *cursInfo);
	/******************************************************/
	/******************************************************/


	/* These methods are implemented in vmware_io.cpp */
	/**************************************************/
	/* Returns true/false if successfully calculated the
	 * correct values for m_IndexPort and m_ValuePort */
	bool setupIoPorts(void);


	/*** Read / Write to IO ports ***/
	/********************************/
	uint32 vmwareReadIndexPort(void);
	void vmwareWriteIndexPort(uint32 newIndex);
	uint32 vmwareReadValuePort(void);
	void vmwareWriteValuePort(uint32 newValue);
	/********************************/


	/*** Read / Write SVGA registers ***/
	/***********************************/
	/* Returns the highest SVGA Id supported by the video card */
	uint32 vmwareGetSvgaId(void);

	void vmwareWriteReg(int32 index, uint32 value);
	uint32 vmwareReadReg(int32 index);
	/***********************************/

	void InitMembers(void);
	bool InitHardware(void);
};

#endif // __VMWARE_H__
