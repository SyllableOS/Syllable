#ifndef	INTERFACE_DDRIVER_HPP
#define	INTERFACE_DDRIVER_HPP

/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <gui/region.h>
#include <gui/guidefines.h>
#include <util/locker.h>
#include <atheos/vesa_gfx.h>

#include "desktop.h"

#include <vector>

class	SrvBitmap;
class	Glyph;
class	MousePtr;
class	SrvSprite;


extern SrvBitmap* g_pcScreenBitmap;

class	DisplayDriver
{
public:
    DisplayDriver();
    virtual	    ~DisplayDriver();

    virtual area_id   	Open() = 0;
    virtual void	Close() = 0;
    
    virtual int	    	GetScreenModeCount() = 0;
    virtual bool    	GetScreenModeDesc( int nIndex, os::screen_mode* psMode ) = 0;
    virtual int	    	SetScreenMode( os::screen_mode sMode ) = 0;
    virtual os::screen_mode GetCurrentScreenMode() = 0; 

    virtual int		GetFramebufferOffset();
    
    virtual void	SetCursorBitmap( os::mouse_ptr_mode eMode, const os::IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight );
    
    virtual void	MouseOn();
    virtual void	MouseOff();
    virtual void	SetMousePos( os::IPoint cNewPos );
    virtual bool	IntersectWithMouse( const os::IRect& cRect ) = 0;
    
    virtual bool	DrawLine( SrvBitmap* psBitMap, const os::IRect& cClipRect,
				  const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode );
    virtual bool	FillRect( SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor );
    virtual bool	BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IPoint cDstPos, int nMode );

    static bool	ClipLine( const os::IRect& cRect, int* x1, int* y1, int* x2, int* y2 );

	virtual void 	RenderGlyph( SrvBitmap *pcBitmap, Glyph* pcGlyph,
			 const os::IPoint& cPos, const os::IRect& cClipRect, const os::Color32_s& sFgColor ); 
	virtual void	RenderGlyphBlend( SrvBitmap *pcBitmap, Glyph* pcGlyph,
			      const os::IPoint& cPos, const os::IRect& cClipRect, const os::Color32_s& sFgColor );      
	virtual void	RenderGlyph( SrvBitmap *pcBitmap, Glyph* pcGlyph,
			 const os::IPoint& cPos, const os::IRect& cClipRect, const uint32* anPallette );

    virtual bool	CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *phArea );
    virtual bool	RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, area_id *phArea );
    virtual void	UpdateVideoOverlay( area_id *phArea );
    virtual void	DeleteVideoOverlay( area_id *phArea );

private:
    void	FillBlit8( uint8* pDst, int nMod, int W,int H, int nColor );
    void	FillBlit16( uint16 *pDst, int nMod, int W, int H, uint32 nColor );
    void	FillBlit24( uint8* pDst, int nMod, int W, int H, uint32 nColor );
    void	FillBlit32( uint32 *pDst, int nMod, int W, int H, uint32 nColor );
private:
	virtual void	__VW_reserved1__();
    virtual void	__VW_reserved2__();
    virtual void	__VW_reserved3__();
    virtual void	__VW_reserved4__();
    virtual void	__VW_reserved5__();
    virtual void	__VW_reserved6__();
    virtual void	__VW_reserved7__();
    virtual void	__VW_reserved8__();
    virtual void	__VW_reserved9__();

    SrvBitmap*   	  m_pcMouseImage;
    SrvSprite*		  m_pcMouseSprite;
    os::IPoint		  m_cMousePos;
    os::IPoint		  m_cCursorHotSpot;

    SrvBitmap*		  m_pcOverlayImage;
    SrvBitmap*		  m_pcOverlayConvertedImage;
    os::IRect		  m_pcOverlayRect;
};

typedef DisplayDriver* gfxdrv_init_func( int nFd );


#endif	//	INTERFACE_DDRIVER_HPP

