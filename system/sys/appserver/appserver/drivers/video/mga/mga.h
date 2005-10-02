/*
 *  The Syllable application server
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *  Copyright (C) 2003  Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or
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
 */

#ifndef __F_MGA_MGA_H__
#define __F_MGA_MGA_H__

#include <atheos/kernel.h>
#include <atheos/pci.h>

#include "mga_crtc.h"
#include "mga_regs.h"
#include "mga_dac.h"
#include "mga_tvp3026.h"

#include "../../../server/vesadrv.h"

class Matrox : public VesaDriver
{
	public:
		Matrox( int nFd );
		~Matrox();

		virtual area_id Open();

		virtual int GetScreenModeCount();
		virtual bool GetScreenModeDesc( int nIndex, os::screen_mode* psMode );
		virtual int SetScreenMode( os::screen_mode );
		virtual os::screen_mode	GetCurrentScreenMode();

		virtual bool DrawLine( SrvBitmap* psBitMap, const os::IRect& cClipRect, const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode );
		virtual bool FillRect( SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor, int nMode );
		virtual bool BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IRect cDstRect, int nMode, int nAlpha );

		bool IsInitiated() const { return( m_bIsInitiated ); }
		virtual int GetFramebufferOffset() { return(0); }

	private:
		inline void   outl( uint32 nAddress, uint32 nValue ) const { *((vuint32*)(m_pRegisterBase + nAddress)) = nValue; }
		inline void   outb( uint32 nAddress, uint8 nValue ) const { *((vuint8*)(m_pRegisterBase + nAddress)) = nValue; }
		inline uint32 inl( uint32 nAddress ) const { return( *((vuint32*)(m_pRegisterBase + nAddress)) ); }
		inline uint8  inb( uint32 nAddress ) const { return( *((vuint8*)(m_pRegisterBase + nAddress)) ); }

		MgaCRTC *m_pcCrtc;
		MgaDac *m_pcDac;

		os::Locker m_cGELock;

		PCI_Info_s m_cPCIInfo;
		struct MGAChip_s m_sChip;

		bool m_bIsInitiated;

		vuint8 *m_pRegisterBase;
		area_id m_hRegisterArea;

		// These are only valid if m_nCRTCScheme==GX00
		vuint8* m_pFrameBufferBase;
		area_id m_hFrameBufferArea;

		enum CRTCScheme { CRTC_VESA, CRTC_GX00 };
		CRTCScheme m_nCRTCScheme;

		enum PointerScheme { POINTER_SPRITE, POINTER_MILLENIUM };
		PointerScheme m_nPointerScheme;
		os::IPoint m_cLastMousePosition;
		os::IPoint m_cCursorHotSpot;

		os::screen_mode m_sCurrentMode;
		std::vector<os::screen_mode> m_cScreenModeList;
};

#endif // __F_MGA_MGA_H__

