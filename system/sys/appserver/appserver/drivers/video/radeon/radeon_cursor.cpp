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

/* Indexed ARGB palette for mono cursor (transp, transp, black, white) */
static uint32 anPalette[] = { 0x00000000, 0x00000000, 0xff000000, 0xffffffff };

void ATIRadeon::SetCursorBitmap( mouse_ptr_mode eMode, const IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight )
{	
	if( m_hFramebufferArea < 0 )
		return;

	if( ( (eMode != MPTR_MONO && eMode != MPTR_RGB32) ||
			nWidth > 64 || nHeight > 64 ) && HWcursor == true ) {
		MouseOff();
		HWcursor = false;
	}
	
	if( eMode == MPTR_MONO && HWcursor == true ) {
		const uint8 *pnSrc = static_cast<const uint8*>(pRaster);
		uint32 *pnSaved = ( uint32 * )rinfo.cursor.ram;
		uint32 save1;
		rinfo.cursor.size.x = nWidth;
		rinfo.cursor.size.y = nHeight;
		rinfo.cursor.hot.x = cHotSpot.x;
		rinfo.cursor.hot.y = cHotSpot.y;

		m_cLock.Lock();
		FifoWait(1);
		save1 = INREG(CRTC_GEN_CNTL) & ~(uint32) (3 << 20);
		save1 |= (uint32) (2 << 20);
		OUTREG(CRTC_GEN_CNTL, save1 & ~CRTC_CUR_EN);
		m_cLock.Unlock();

		for ( int y = 0; y < 64; y++ )
		{
			for ( int x = 0; x < 64; x++, pnSaved++ )
			{
				if ( y >= nHeight || x >= nWidth )
				{
					*pnSaved = 0x00000000;
				}
				else
				{
					*pnSaved = anPalette[*pnSrc++];
				}
			}
		}

		MouseOn();

  	} else if( eMode == MPTR_RGB32 && HWcursor == true ) {
		const uint32 *pnSrc = static_cast<const uint32*>(pRaster);
		uint32 *pnSaved = ( uint32 * )rinfo.cursor.ram;
		uint32 save1;
		rinfo.cursor.size.x = nWidth;
		rinfo.cursor.size.y = nHeight;
		rinfo.cursor.hot.x = cHotSpot.x;
		rinfo.cursor.hot.y = cHotSpot.y;

		m_cLock.Lock();
		FifoWait(1);
		save1 = INREG(CRTC_GEN_CNTL) & ~(uint32) (3 << 20);
		save1 |= (uint32) (2 << 20);
		OUTREG(CRTC_GEN_CNTL, save1 & ~CRTC_CUR_EN);
		m_cLock.Unlock();

		for ( int y = 0; y < 64; y++ )
		{
			for ( int x = 0; x < 64; x++, pnSaved++ )
			{
				if ( y >= nHeight || x >= nWidth )
				{
					*pnSaved = 0x00000000;
				}
				else
				{
					*pnSaved = *pnSrc++;
				}
			}
		}

		MouseOn();
	} else
		return DisplayDriver::SetCursorBitmap(eMode, cHotSpot, pRaster, nWidth, nHeight);
}

bool ATIRadeon::IntersectWithMouse(const IRect &cRect) { 
	return false;
}

void ATIRadeon::MouseOn( void )
{
	if( m_hFramebufferArea < 0 )
		return;

	if( HWcursor == true ) {
		uint16 xoff = 0, yoff = 0;
		int x, y;
		
		x = rinfo.cursor.pos.x - rinfo.cursor.hot.x;
		if (x < 0)
			xoff = -x;

		y = rinfo.cursor.pos.y - rinfo.cursor.hot.y;
		if (y < 0)
			yoff = -y;

		m_cLock.Lock();
		FifoWait(4);
		OUTREG(CUR_HORZ_VERT_OFF, CUR_LOCK |
		    ((uint32)(xoff) << 16) | yoff);
		OUTREG(CUR_HORZ_VERT_POSN, CUR_LOCK | ((uint32)(xoff ? 0 : x) << 16)
		    | (yoff ? 0 : y));
		OUTREG(CUR_OFFSET, (rinfo.cursor.offset + xoff + yoff * 16));
		OUTREG(CRTC_GEN_CNTL, INREG(CRTC_GEN_CNTL) | CRTC_CUR_EN);
		EngineIdle();
		m_cLock.Unlock();
	} else 
		return DisplayDriver::MouseOn();
}

void ATIRadeon::MouseOff( void )
{
	if( HWcursor == true ) {
		m_cLock.Lock();
		FifoWait(1);
		OUTREG(CRTC_GEN_CNTL, INREG(CRTC_GEN_CNTL) & ~CRTC_CUR_EN);
		EngineIdle();
		m_cLock.Unlock();
	} else
		return DisplayDriver::MouseOn();
}

void ATIRadeon::SetMousePos( IPoint cNewPos )
{
	if( m_hFramebufferArea < 0 )
		return;

	if( HWcursor == true ) {
		rinfo.cursor.pos.x = cNewPos.x;
		rinfo.cursor.pos.y = cNewPos.y;
		MouseOn();	
	} else
		return DisplayDriver::SetMousePos(cNewPos);
}

void ATIRadeon::HWCursorInit()
{
	rinfo.video_ram -= PAGE_SIZE * 5;
	rinfo.cursor.offset = rinfo.video_ram;
	rinfo.cursor.ram = (uint8 *)rinfo.fb_base + rinfo.cursor.offset;

	if (!rinfo.cursor.ram) {
		return;
	}
	
	rinfo.cursor.hot.x = 0;
	rinfo.cursor.hot.y = 0;
	rinfo.cursor.size.x = 0;
	rinfo.cursor.size.y = 0;
	
	HWcursor = true; 
	dbprintf("Radeon :: Hardware cursor enabled\n");
	
}

