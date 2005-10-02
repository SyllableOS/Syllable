/*
 *  Mach64 driver for AtheOS application server
 *  Copyright (C) 2001  Arno Klenke
 *
 *  Based on the atyfb linux kernel driver and the xfree86 ati driver.
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

#ifndef _ATI_DRIVER_H_
#define _ATI_DRIVER_H_

#include <atheos/kernel.h>
#include <atheos/pci.h>
#include <atheos/pci_vendors.h>
#include <gui/bitmap.h>
#include "../../../server/ddriver.h"
#include "../../../server/sprite.h"
#include "../../../server/bitmap.h"

#include "mach64_regs.h"
#include "mach64_info.h"
#include "mach64_vesa.h"

using namespace os;

#define dprintf dbprintf
#define mach64_delay(arg) snooze( arg * 1000 )


// Video mode descriptor
#define V_PHSYNC		0x00000001 // + HSYNC
#define V_NHSYNC		0x00000002 // - HSYNC
#define V_PVSYNC		0x00000004 // + VSYNC
#define V_NVSYNC		0x00000008 // - VSYNC
#define V_INTERLACE	0x00000010
#define V_DBLSCAN	0x00000020
#define V_CLKDIV2		0x00004000

#define UnitOf(___Value)                                \
        (((((___Value) ^ ((___Value) - 1)) + 1) >> 1) | \
         ((((___Value) ^ ((___Value) - 1)) >> 1) + 1))

#define GetBits(__Value, _Mask) (((__Value) & (_Mask)) / UnitOf(_Mask))
#define SetBits(__Value, _Mask) (((__Value) * UnitOf(_Mask)) & (_Mask))
#define MaxBits(__Mask)         GetBits(__Mask, __Mask)


struct VideoMode {
	int		Width;			/* width 						*/
	int		Height;			/* height						*/
	int		Refresh;			/* refresh rate 				*/

	uint32	Clock;      			/* pixel clock					*/
	uint32	left_margin;		/* time from sync to picture	*/
	uint32	right_margin;		/* time from picture to sync	*/
	uint32	upper_margin;		/* time from sync to picture	*/
	uint32	lower_margin;
	uint32	hsync_len;		/* length of horizontal sync	*/
	uint32	vsync_len;		/* length of vertical sync		*/
	uint32	Flags;
};


// ****************************************************************************
class ATImach64 : public M64VesaDriver {
public:

	 // *** constructor and destructor ***
	ATImach64 ( int nFd );
	virtual ~ATImach64 ();

	uint32 ProbeHardware (PCI_Info_s * PCIInfo);
	bool InitHardware ( int nFd );

	area_id Open();
	virtual void Close();

	virtual int SetScreenMode( os::screen_mode sMode );

	virtual int GetScreenModeCount();
	virtual bool GetScreenModeDesc(int nIndex, os::screen_mode *psMode);
	virtual os::screen_mode GetCurrentScreenMode();

	int BytesPerPixel(color_space cs);
	int BitsPerPixel(color_space cs);

	virtual void SetColor(int nIndex, const Color32_s &sColor);
	virtual bool FillRect(SrvBitmap *psBitMap, const IRect &cRect, const Color32_s &sColor, int nMode);
	virtual bool DrawLine(SrvBitmap *psBitMap, const IRect &cClipRect,
		const IPoint &cPnt1, const IPoint &cPnt2, const Color32_s &sColor, int nMode);
	virtual bool BltBitmap(SrvBitmap *pcDstBitMap, SrvBitmap *pcSrcBitMap,
		IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha);
	virtual bool	CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer );
	virtual bool	RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, area_id *pBuffer );
	virtual void	DeleteVideoOverlay( area_id *pBuffer );
	bool IsInitialized();

	//--------------------------------------------------------------------

private:

	// internal members
	bool m_bIsInitialized; // set to true, if driver initialized
	Locker m_cLock; // hardware access lock
	bool m_bVESA; // Use vesa

	// hardware resources
	PCI_Info_s m_cPCIInfo;      // card descriptor
	vuint8 * m_pFramebufferBase; // base of the framebuffer
	area_id m_hFramebufferArea; // area_id of the framebuffer
	vuint8 * m_pRegisterBase; // base of the registers
	area_id m_hRegisterArea; // area_id of the registers
	ati_info info;


	// *** screen modes ***
	std::vector<os::screen_mode> m_cScreenModeList; // list of the screen modes

	// current mode
	os::screen_mode     m_sCurrentMode;        // current video mode
	
	// Video Overlay
	bool				m_bSupportsYUV;
	uint32				m_nColorKey;
	bool				m_bVideoOverlayUsed;

	// I / O

	inline void   outl( uint32 nValue, uint32 nAddress ) { *((vuint32*)(info.ati_regbase + nAddress)) = nValue; }
	inline void   outb( uint8 nValue, uint32 nAddress ) { *((vuint8*)(info.ati_regbase + nAddress)) = nValue; }
	inline uint32 inl( uint32 nAddress )  { return( *((vuint32*)(info.ati_regbase + nAddress)) ); }
	inline uint8  inb( uint32 nAddress ) { return( *((vuint8*)(info.ati_regbase + nAddress)) ); }

	uint32 aty_ld_le32(int regindex);
	void aty_st_le32(int regindex, uint32 val);
	uint8 aty_ld_8(int regindex);
	void aty_st_8(int regindex, uint8 val);
	uint8 aty_ld_lcd( int regindex );
	void aty_st_lcd(int regindex, uint8 val);
	
	void wait_for_fifo(uint16 entries);
	void wait_for_idle();
	void wait_for_vblank();
	uint8 aty_ld_pll(int offset);
	void reset_GTC_3D_engine();
	void reset_engine();
	void init_engine(const struct ati_par *par);

	// CRTC

	int aty_var_to_crtc(const struct VideoMode *var, int var_bpp, struct crtc *crtc);
	void aty_set_crtc(const struct crtc *crtc);

	// Mach64 CT support

	void aty_st_pll(int offset, uint8 val);
	int aty_dsp_gt(uint8 bpp, struct pll_ct *pll);
	int aty_valid_pll_ct(uint32 vclk_per, struct pll_ct *pll);
	void aty_calc_pll_ct(struct pll_ct *pll);
	int aty_var_to_pll_ct(uint32 vclk_per, uint8 bpp, union ati_pll *pll);
	void aty_set_pll_ct(const union ati_pll *pll);

};

#endif
// *** end of file ***



