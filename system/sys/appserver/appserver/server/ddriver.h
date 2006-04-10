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

struct DisplayMemObj
{
	struct DisplayMemObj* pcPrev;
	struct DisplayMemObj* pcNext;
	bool bFree;
	uint32 nOffset;
	uint32 nObjStart;
	uint32 nSize;
};

class DisplayDriver
{
public:
	/* Video driver API */
	
	/* Construction/Destruction */
    DisplayDriver();
    virtual	    ~DisplayDriver();

    virtual area_id  Open() = 0;
    virtual void	 Close() = 0;
 
	/* Screenmodes */
    virtual int	    GetScreenModeCount() = 0;   
    virtual bool    GetScreenModeDesc( int nIndex, os::screen_mode* psMode ) = 0;
    virtual int		SetScreenMode( os::screen_mode sMode ) = 0;
    virtual os::screen_mode GetCurrentScreenMode() = 0;
	virtual int		GetFramebufferOffset();
   
	/* Acceleration */
    virtual void	LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrcRect, os::IRect cDstRect );
    virtual void	UnlockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrcRect, os::IRect cDstRect );
    
    virtual bool	DrawLine( SrvBitmap* psBitMap, const os::IRect& cClipRect,
				  const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode );
    virtual bool	FillRect( SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor, int nMode );
    virtual bool	BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IRect cDstRect, int nMode, int nAlpha );
    
    /* Video overlays */
    virtual bool	CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *phArea );
    virtual bool	RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, area_id *phArea );
    virtual void	DeleteVideoOverlay( area_id *phArea );

	virtual status_t Control( int nCommand, void* pData, uint32 nDataSize );
	
	// --------------------------------------------------------------------------------

	/* Internal methods */
	
	/* Lines */
	static bool	ClipLine( const os::IRect& cRect, int* x1, int* y1, int* x2, int* y2 );
	
	/* Memory management */
	void		InitMemory( uint32 nOffset, uint32 nSize, uint32 nMemObjAlign, uint32 nRowAlign );
	status_t	AllocateMemory( uint32 nSize, uint32* pnOffset );
	SrvBitmap*	AllocateBitmap( int nWidth, int nHeight, os::color_space eColorSpc );
	void		FreeMemory( uint32 nOffset );
	
	/* Cursor management */
	void		SetCursorBitmap( os::mouse_ptr_mode eMode, const os::IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight );
    void		MouseOn();
    void		MouseOff();
    void		SetMousePos( os::IPoint cNewPos );
    
    /* Text rendering */
	void 		RenderGlyph( SrvBitmap *pcBitmap, Glyph* pcGlyph,
				 const os::IPoint& cPos, const os::IRect& cClipRect, const os::Color32_s& sFgColor ); 
	void		RenderGlyphBlend( SrvBitmap *pcBitmap, Glyph* pcGlyph,
				 const os::IPoint& cPos, const os::IRect& cClipRect, const os::Color32_s& sFgColor );      
	void		RenderGlyph( SrvBitmap *pcBitmap, Glyph* pcGlyph,
				 const os::IPoint& cPos, const os::IRect& cClipRect, const uint32* anPallette );
private:
	void		FillBlit16( uint16 *pDst, int nMod, int W, int H, uint16 nColor );
	void		FillBlit32( uint32 *pDst, int nMod, int W, int H, uint32 nColor );
	void		FillAlpha16( uint16 *pDst, int nMod, int W, int H, uint32 nColor );
	void		FillAlpha32( uint32 *pDst, int nMod, int W, int H, uint32 nColor, uint32 nFlags );
	void		DrawLine16( SrvBitmap * pcBitmap, const os::IRect & cClip, int x1, int y1, int x2, int y2, uint16 nColor, uint32 nColor32, int nMode );
	void 		DrawLine32( SrvBitmap * pcBitmap, const os::IRect & cClip, int x1, int y1, int x2, int y2, uint32 nColor, int nMode );
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
    
    SrvBitmap*   	m_pcMouseImage;
    SrvSprite*		m_pcMouseSprite;
    os::IPoint		m_cMousePos;
    os::IPoint		m_cCursorHotSpot;
    
    uint8*			m_pVideoMem;
    DisplayMemObj*	m_psFirstMemObj;
    uint32			m_nMemObjAlign;
    uint32			m_nRowAlign;
};

typedef DisplayDriver* gfxdrv_init_func( int nFd );


#endif	//	INTERFACE_DDRIVER_HPP

