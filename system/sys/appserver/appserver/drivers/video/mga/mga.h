/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

#include "../../../server/vesadrv.h"

class Millenium2 : public VesaDriver
{
public:
    Millenium2();
    ~Millenium2();
    virtual area_id	Open();

    virtual int         GetScreenModeCount();
    virtual bool        GetScreenModeDesc( int nIndex, ScreenMode* psMode );
    virtual int		SetScreenMode( int nWidth, int nHeight, os::color_space eColorSpc,
				       int nPosH, int nPosV, int nSizeH, int nSizeV, float vRefreshRate );
    virtual int         GetHorizontalRes();
    virtual int         GetVerticalRes();
    virtual int         GetBytesPerLine();
    virtual os::color_space GetColorSpace();  

    virtual void	SetCursorBitmap( os::mouse_ptr_mode eMode, const os::IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight );
    virtual void        MouseOn( void );
    virtual void        MouseOff( void );
    virtual void        SetMousePos( os::IPoint cNewPos );

    virtual bool	DrawLine( SrvBitmap* psBitMap, const os::IRect& cClipRect,
				  const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode );
  
    virtual bool	FillRect( SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor );
    virtual bool	BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IPoint cDstPos, int nMode );

    bool	IsInitiated() const { return( m_bIsInitiated ); }
private:
    inline void   outl( uint32 nAddress, uint32 nValue ) const { *((vuint32*)(m_pRegisterBase + nAddress)) = nValue; }
    inline void   outb( uint32 nAddress, uint8 nValue ) const { *((vuint8*)(m_pRegisterBase + nAddress)) = nValue; }
    inline uint32 inl( uint32 nAddress ) const { return( *((vuint32*)(m_pRegisterBase + nAddress)) ); }
    inline uint8  inb( uint32 nAddress ) const { return( *((vuint8*)(m_pRegisterBase + nAddress)) ); }

    // Used by the gx00_* code.
    friend class CGx00CRTC; // 
//    uint8         ReadConfigBYTE( uint8 nReg );
//    uint32        ReadConfigDWORD( uint8 nReg );
    void          WriteConfigBYTE( uint8 nReg, uint8 nValue );
    void          WriteConfigDWORD( uint8 nReg, uint32 nValue );

//    void        SetMouseBitmap( const uint8 *bits, int width, int height );

    bool	m_bIsInitiated;
    PCI_Info_s  m_cPCIInfo;
  
    os::Locker      m_cGELock;
    vuint8*     m_pRegisterBase;
    area_id     m_hRegisterArea;

    // These are only valid if m_nCRTCScheme==GX00
    vuint8*     m_pFrameBufferBase;
    area_id     m_hFrameBufferArea;

    enum PointerScheme { POINTER_SPRITE, POINTER_MILLENIUM };
    PointerScheme m_nPointerScheme;
    os::IPoint       m_cLastMousePosition;
    os::IPoint	 m_cCursorHotSpot;

    enum CRTCScheme { CRTC_VESA, CRTC_GX00 };
    CRTCScheme  m_nCRTCScheme;
    int         m_nCurrentMode;
    std::vector<ScreenMode> m_cScreenModeList;
};

#endif // __F_MGA_MGA_H__
