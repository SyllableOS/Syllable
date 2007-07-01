/*
 ** Radeon graphics driver for Syllable application server
 *  Copyright (C) 2004 Michael Krueger <invenies@web.de>
 *  Copyright (C) 2003 Arno Klenke <arno_klenke@yahoo.com>
 *  Copyright (C) 1998-2001 Kurt Skauen <kurt@atheos.cx>
 *
 ** This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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
 ** For information about used sources and further copyright notices
 *  see radeon.cpp
 */

#include "radeon.h"

void ATIRadeon::EngineReset()
{
	uint32 clock_cntl_index, mclk_cntl, rbbm_soft_reset;
	uint32 host_path_cntl;
	uint32 tmp2;

	EngineFlush ();

	/* Some ASICs have bugs with dynamic-on feature, which are  
	 * ASIC-version dependent, so we force all blocks on for now
	 * -- from XFree86
	 */
	if (rinfo.has_CRTC2)
	{
		uint32 tmp;

		tmp = INPLL(SCLK_CNTL);
		OUTPLL(SCLK_CNTL, ((tmp & ~DYN_STOP_LAT_MASK) |
				   CP_MAX_DYN_STOP_LAT |
				   SCLK_FORCEON_MASK));

		if (rinfo.family == CHIP_FAMILY_RV200)
		{
			tmp = INPLL(SCLK_MORE_CNTL);
			OUTPLL(SCLK_MORE_CNTL, tmp | SCLK_MORE_FORCEON);
		}
	}

	/* against screen flicker through accesses to the text mode VGA memory*/
	tmp2 = INREG(CONFIG_CNTL);
	OUTREG(CONFIG_CNTL, tmp2 & ~CFG_VGA_RAM_EN);

	clock_cntl_index = INREG(CLOCK_CNTL_INDEX);
	mclk_cntl = INPLL(MCLK_CNTL);

	OUTPLL(MCLK_CNTL, (mclk_cntl |
			   FORCEON_MCLKA |
			   FORCEON_MCLKB |
			   FORCEON_YCLKA |
			   FORCEON_YCLKB |
			   FORCEON_MC |
			   FORCEON_AIC));

	host_path_cntl = INREG(HOST_PATH_CNTL);
	rbbm_soft_reset = INREG(RBBM_SOFT_RESET);

	if (rinfo.family == CHIP_FAMILY_R300 ||
	    rinfo.family == CHIP_FAMILY_R350 ||
	    rinfo.family == CHIP_FAMILY_RV350) {

		uint32 tmp;

		OUTREG(RBBM_SOFT_RESET, (rbbm_soft_reset |
					 SOFT_RESET_CP |
					 SOFT_RESET_HI |
					 SOFT_RESET_E2));
		INREG(RBBM_SOFT_RESET);
		OUTREG(RBBM_SOFT_RESET, 0);
		tmp = INREG(RB2D_DSTCACHE_MODE);
		OUTREG(RB2D_DSTCACHE_MODE, tmp | (1 << 17)); /* FIXME */
	} else {
		OUTREG(RBBM_SOFT_RESET, rbbm_soft_reset |
					SOFT_RESET_CP |
					SOFT_RESET_HI |
					SOFT_RESET_SE |
					SOFT_RESET_RE |
					SOFT_RESET_PP |
					SOFT_RESET_E2 |
					SOFT_RESET_RB);
		tmp2 = INREG(RBBM_SOFT_RESET);
		OUTREG(RBBM_SOFT_RESET, rbbm_soft_reset & (uint32)
					~(SOFT_RESET_CP |
					  SOFT_RESET_HI |
					  SOFT_RESET_SE |
					  SOFT_RESET_RE |
					  SOFT_RESET_PP |
					  SOFT_RESET_E2 |
					  SOFT_RESET_RB));
		tmp2 = INREG(RBBM_SOFT_RESET);
	}

	OUTREG(HOST_PATH_CNTL, host_path_cntl | HDP_SOFT_RESET);
	tmp2 = INREG(HOST_PATH_CNTL);
	OUTREG(HOST_PATH_CNTL, host_path_cntl);

	if (rinfo.family != CHIP_FAMILY_R300 ||
	    rinfo.family != CHIP_FAMILY_R350 ||
	    rinfo.family != CHIP_FAMILY_RV350)
	{
		OUTREG(RBBM_SOFT_RESET, rbbm_soft_reset);
	}

	OUTREG(CLOCK_CNTL_INDEX, clock_cntl_index);
	OUTPLL(MCLK_CNTL, mclk_cntl);
}

void ATIRadeon::EngineInit ()
{
	unsigned long temp;

	/* disable 3D engine */
	OUTREG(RB3D_CNTL, 0);

	EngineReset();

	FifoWait(1);
	if ((rinfo.family != CHIP_FAMILY_R300) &&
	    (rinfo.family != CHIP_FAMILY_R350) &&
	    (rinfo.family != CHIP_FAMILY_RV350))
	{
		OUTREG(RB2D_DSTCACHE_MODE, 0);
	}

	FifoWait(3);
	/* We re-read MC_FB_LOCATION from card as it can have been
	 * modified by XFree drivers (ouch !)
	 */
	rinfo.fb_local_base = INREG(MC_FB_LOCATION) << 16;

	OUTREG(DEFAULT_PITCH_OFFSET, (rinfo.pitch << 0x16) |
				     (rinfo.fb_local_base >> 10));
	OUTREG(DST_PITCH_OFFSET, (rinfo.pitch << 0x16) | (rinfo.fb_local_base >> 10));
	OUTREG(SRC_PITCH_OFFSET, (rinfo.pitch << 0x16) | (rinfo.fb_local_base >> 10));

	FifoWait(1);
	OUTREGP(DP_DATATYPE, 0, ~HOST_BIG_ENDIAN_EN);

	FifoWait(2);
	OUTREG(DEFAULT_SC_TOP_LEFT, 0);
	OUTREG(DEFAULT_SC_BOTTOM_RIGHT, (DEFAULT_SC_RIGHT_MAX |
					 DEFAULT_SC_BOTTOM_MAX));

	temp = GetDstBpp(rinfo.depth);
	rinfo.dp_gui_master_cntl = ((temp << 8) | GMC_CLR_CMP_CNTL_DIS);

	FifoWait (1);
	OUTREG(DP_GUI_MASTER_CNTL, (rinfo.dp_gui_master_cntl |
				    GMC_BRUSH_SOLID_COLOR |
				    GMC_SRC_DATATYPE_COLOR));

	FifoWait(7);

	/* clear line drawing regs */
	OUTREG(DST_LINE_START, 0);
	OUTREG(DST_LINE_END, 0);

	/* set brush color regs */
	OUTREG(DP_BRUSH_FRGD_CLR, 0xffffffff);
	OUTREG(DP_BRUSH_BKGD_CLR, 0x00000000);

	/* set source color regs */
	OUTREG(DP_SRC_FRGD_CLR, 0xffffffff);
	OUTREG(DP_SRC_BKGD_CLR, 0x00000000);

	/* default write mask */
	OUTREG(DP_WRITE_MSK, 0xffffffff);

	EngineIdle();
}

bool ATIRadeon::FillRect(SrvBitmap *pcBitmap, const IRect &cRect, 
		      const Color32_s &sColor, int nMode)
{
	if( m_hFramebufferArea < 0 )
		return false;

	if( pcBitmap->m_bVideoMem == false || nMode != DM_COPY ) {
		return( DisplayDriver::FillRect(pcBitmap, cRect, sColor, nMode) );
	}

	uint32 nColor;
	switch( pcBitmap->m_eColorSpc ) {
		case CS_RGB16:
			nColor = COL_TO_RGB16 (sColor);
			break;
		case CS_RGB32:
		case CS_RGBA32:
			nColor = COL_TO_RGB32 (sColor);
			break;
		default:
			return( DisplayDriver::FillRect(pcBitmap, cRect, sColor, nMode) );
	}

	if (rinfo.asleep)
	{
		return( DisplayDriver::FillRect(pcBitmap, cRect, sColor, nMode) );
	}

	m_cLock.Lock();

	FifoWait(4);  
  	OUTREG(DP_GUI_MASTER_CNTL,  
		rinfo.dp_gui_master_cntl  /* contains, like GMC_DST_32BPP */
                | GMC_BRUSH_SOLID_COLOR
                | ROP3_P);
	OUTREG(DP_BRUSH_FRGD_CLR, nColor);
	OUTREG(DP_WRITE_MSK, 0xffffffff);
	OUTREG(DP_CNTL, (DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM));

	FifoWait(2);  
	OUTREG(DST_Y_X, (cRect.top << 16) | cRect.left);
	OUTREG(DST_WIDTH_HEIGHT, ((cRect.Width() + 1) << 16) | cRect.Height() + 1);

	EngineIdle();

	m_cLock.Unlock();

	return( true );
}

bool ATIRadeon::BltBitmap(SrvBitmap *pcDstBitMap, SrvBitmap *pcSrcBitMap, 
                           IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha)
{
	if( m_hFramebufferArea < 0 )
		return false;

	if( pcDstBitMap->m_bVideoMem == false ||
		pcSrcBitMap->m_bVideoMem == false || rinfo.asleep || cSrcRect.Size() != cDstRect.Size() ) {
		return( DisplayDriver::BltBitmap( pcDstBitMap, pcSrcBitMap,
                                         cSrcRect, cDstRect, nMode, nAlpha ) );
	}
  
	if (nMode != DM_COPY) {
		return( DisplayDriver::BltBitmap( pcDstBitMap, pcSrcBitMap,
                                         cSrcRect, cDstRect, nMode, nAlpha ) );
	}

	IPoint cDstPos = cDstRect.LeftTop();
	int srcx = cSrcRect.left;
	int srcy = cSrcRect.top;
	int dstx = cDstPos.x;
	int dsty = cDstPos.y;
	uint width = cSrcRect.Width();
	uint height = cSrcRect.Height();
	uint direction = 0;

	if( srcy < dsty ) {
		dsty += height;
		srcy += height;
	} else
		direction |= DST_Y_TOP_TO_BOTTOM;

	if (srcx < dstx) {
		dstx += width;
		srcx += width;
	} else
		direction |= DST_X_LEFT_TO_RIGHT;

	m_cLock.Lock();

	FifoWait(3);
	OUTREG(DP_GUI_MASTER_CNTL,
		rinfo.dp_gui_master_cntl /* i.e. GMC_DST_32BPP */
		| GMC_SRC_DATATYPE_COLOR
		| ROP3_S 
		| DP_SRC_SOURCE_MEMORY);
	OUTREG(DP_WRITE_MSK, 0xffffffff);
	OUTREG(DP_CNTL, direction);

	FifoWait(3);
	OUTREG(SRC_Y_X, (srcy << 16) | srcx);
	OUTREG(DST_Y_X, (dsty << 16) | dstx);
	OUTREG(DST_HEIGHT_WIDTH, ((height + 1) << 16) | width + 1);

	EngineIdle();

	m_cLock.Unlock();

	return( true );
}

bool ATIRadeon::DrawLine(SrvBitmap *psBitMap, const IRect &cClipRect, 
                          const IPoint &cPnt1, const IPoint &cPnt2, 
                          const Color32_s &sColor, int nMode) { 

	if( m_hFramebufferArea < 0 )
		return false;

	if( psBitMap->m_bVideoMem == false || nMode != DM_COPY ) {
		return( DisplayDriver::DrawLine(psBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode) );
	}

	uint32 nColor;

	int x1 = cPnt1.x;
	int y1 = cPnt1.y;
	int x2 = cPnt2.x;
	int y2 = cPnt2.y;

	/* Currently, this driver implements only horizontal and vertical lines */
	if(!(x1 == x2 || y1 == y2))
	{
		return( DisplayDriver::DrawLine(psBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode) );
	}

	switch( psBitMap->m_eColorSpc ) {
		case CS_RGB16:
			nColor = COL_TO_RGB16 (sColor);
			break;
		case CS_RGB32:
		case CS_RGBA32:
			nColor = COL_TO_RGB32 (sColor);
			break;
		default:
			return( DisplayDriver::DrawLine(psBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode) );
	}

	if( DisplayDriver::ClipLine( cClipRect, &x1, &y1, &x2, &y2 ) == false ) {
		return( false );
	}

	/*
	 * The implemenation uses the same instructions as FillRect()
	 * There are registers for line drawing but they don't work as advertised :(
 	 */
	m_cLock.Lock();
	FifoWait(4);
	OUTREG(DP_GUI_MASTER_CNTL,
		rinfo.dp_gui_master_cntl /* i.e. GMC_DST_32BPP */
		| GMC_BRUSH_SOLID_COLOR
		| ROP3_P);
	OUTREG(DP_BRUSH_FRGD_CLR, nColor);
	OUTREG(DP_WRITE_MSK, 0xffffffff);
	OUTREG(DP_CNTL, (DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM));

	FifoWait(2);
	if(x1 == x2) {
		if(y1 < y2)
		{
			OUTREG(DST_Y_X, (y1 << 16) | x1);
			OUTREG(DST_WIDTH_HEIGHT, 1 << 16 | y2 - y1 + 1);
		} else {
			OUTREG(DST_Y_X, (y2 << 16) | x1);
			OUTREG(DST_WIDTH_HEIGHT, 1 << 16 | y1 - y2 + 1);
		}
	} else {
		if(x1 < x2)
		{
			OUTREG(DST_Y_X, (y1 << 16) | x1);
			OUTREG(DST_WIDTH_HEIGHT, (x2 - x1 + 1) << 16 | 1);
		} else {
			OUTREG(DST_Y_X, (y1 << 16) | x2);
			OUTREG(DST_WIDTH_HEIGHT, (x1 - x2 + 1) << 16 | 1);
		}
	}
	EngineIdle();
	m_cLock.Unlock();
	return( true );
}

