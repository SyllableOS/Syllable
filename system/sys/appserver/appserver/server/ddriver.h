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

//using namespace os;

class	SrvBitmap;
class	Glyph;
class	MousePtr;
class	SrvSprite;


extern SrvBitmap* g_pcScreenBitmap;
/*
inline uint32 ConvertColor32( Color32_s sColor, color_space eColorSpc )
{
    switch( eColorSpc )
    {
	case CS_RGBA32:
	    return( (sColor.red << 16) | (sColor.green << 5) | sColor.blue | (sColor.alpha << 24) );
	case CS_RGB32:
	case CS_RGB24:
	    return( (sColor.red << 16) | (sColor.green << 5) | sColor.blue );
	case CS_RGB16:
	    return( ((sColor.red >> 3) << 11) | ((sColor.green >> 2) << 5) | (sColor.blue >> 3) );
	case CS_RGB15:
	case CS_RGBA15:
	    return( ((sColor.red >> 3) << 10) | ((sColor.green >> 3) << 5) | (sColor.blue >> 3) );
	default:
	    dbprintf( "ConvertColor32() invalid color space %d\n", eColorSpc );
	    return( 0 );
    }
}
*/

struct ScreenMode
{
    ScreenMode() {}
    ScreenMode( int w, int h, int bbl, os::color_space cs ) { m_nWidth = w; m_nHeight = h; m_nBytesPerLine = bbl; m_eColorSpace = cs; }
    int		    m_nWidth;
    int		    m_nHeight;
    int		    m_nBytesPerLine;
    os::color_space m_eColorSpace;
};


class	DisplayDriver
{
public:
    DisplayDriver();
    virtual	    ~DisplayDriver();

    virtual area_id   	Open() = 0;
    virtual void	Close() = 0;

    virtual int	    	GetScreenModeCount() = 0;
    virtual bool    	GetScreenModeDesc( int nIndex, ScreenMode* psMode ) = 0;
    virtual int	    	SetScreenMode( int nWidth, int nHeight, os::color_space eColorSpc,
				       int nPosH, int nPosV, int nSizeH, int nSizeV, float vRefreshRate ) = 0;

    virtual int	        GetHorizontalRes() = 0;
    virtual int	        GetVerticalRes() = 0;
    virtual int	        GetBytesPerLine() = 0;
    virtual int		GetFramebufferOffset();
    virtual os::color_space GetColorSpace() = 0;
    virtual void	SetColor( int nIndex, const os::Color32_s& sColor ) = 0;

    virtual void	SetCursorBitmap( os::mouse_ptr_mode eMode, const os::IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight );
    
    virtual void	MouseOn();
    virtual void	MouseOff();
    virtual void	SetMousePos( os::IPoint cNewPos );
    virtual bool	IntersectWithMouse( const os::IRect& cRect ) = 0;

    virtual bool	WritePixel16( SrvBitmap* psBitMap, int X, int Y, uint32 nColor );
    virtual bool	DrawLine( SrvBitmap* psBitMap, const os::IRect& cClipRect,
				  const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode );
    virtual bool	FillRect( SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor );
    virtual bool	BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IPoint cDstPos, int nMode );
//    virtual bool	BltBitmapMask( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, const os::Color32_s& sHighColor,
//				       const os::Color32_s& sLowColor, os::IRect cSrcRect, os::IPoint cDstPos );
    virtual bool	RenderGlyph( SrvBitmap* pcBitmap, Glyph* pcGlyph, const os::IPoint& cPos,
				     const os::IRect& cClipRect, uint32* anPalette );
    virtual bool	RenderGlyph( SrvBitmap* pcBitmap, Glyph* pcGlyph, const os::IPoint& cPos,
				     const os::IRect& cClipRect, const os::Color32_s& sFgColor );

    static bool	ClipLine( const os::IRect& cRect, int* x1, int* y1, int* x2, int* y2 );
  
private:
    void	FillBlit8( uint8* pDst, int nMod, int W,int H, int nColor );
    void	FillBlit16( uint16 *pDst, int nMod, int W, int H, uint32 nColor );
    void	FillBlit24( uint8* pDst, int nMod, int W, int H, uint32 nColor );
    void	FillBlit32( uint32 *pDst, int nMod, int W, int H, uint32 nColor );

    SrvBitmap*   	  m_pcMouseImage;
    SrvSprite*		  m_pcMouseSprite;
    os::IPoint		  m_cMousePos;
    os::IPoint		  m_cCursorHotSpot;
};

typedef DisplayDriver* gfxdrv_init_func();


#endif	//	INTERFACE_DDRIVER_HPP
