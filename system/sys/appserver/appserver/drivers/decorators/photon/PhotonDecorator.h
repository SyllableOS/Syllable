/*
 *  "Photon" Window-Decorator
 *  Copyright (C) 2002 Iain Hutchison
 *
 *  Based on "Amiga" Window-Decorator
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

#ifndef __F_PHOTONDECORATOR_H__
#define __F_PHOTONDECORATOR_H__

#include <windowdecorator.h>
#include <gui/font.h>

#include <string>

using namespace os;

class PhotonDecorator : public os::WindowDecorator
{
public:
    PhotonDecorator( Layer* pcLayer, uint32 nWndFlags );
    virtual ~PhotonDecorator();
  
    virtual hit_item HitTest( const Point& cPosition );
    virtual void FrameSized( const Rect& cNewFrame );
    virtual Rect GetBorderSize();
    virtual Point GetMinimumSize();
    virtual void SetFlags( uint32 nFlags );
    virtual void FontChanged();
    virtual void SetTitle( const char* pzTitle );
    virtual void SetWindowFlags( uint32 nFlags );
    virtual void SetFocusState( bool bHasFocus );
    virtual void SetCloseButtonState( bool bPushed );
    virtual void SetZoomButtonState( bool bPushed );
    virtual void SetMinimizeButtonState( bool bPushed );
    virtual void SetButtonState( uint32 nButton, bool bPushed );
    virtual void Render( const Rect& cUpdateRect );
private:
    void CalculateBorderSizes();
    void Layout();
    void DrawMinimize  ( const Rect& cRect, bool bActive, bool bRecessed );
    void DrawZoom   ( const Rect& cRect, bool bActive, bool bRecessed );
    void DrawClose  ( const Rect& cRect, bool bActive, bool bRecessed );
    void DrawPanel  ( const Rect& cRect, bool bActive, bool bRecessed );
    void DrawFrame  ( const Rect& cRect, bool bSolid,  bool bRecessed );
    void DrawTitleBG( const Rect& cRect, bool bActive );

	os::font_height m_sFontHeight;
	uint32			m_nTitleWidth;
	Rect			m_cBounds;
	std::string		m_cTitle;
	uint32			m_nFlags;

    Rect   m_cCloseRect;
    Rect   m_cZoomRect;
    Rect   m_cMinimizeRect;
    Rect   m_cDragRect;
  
    float  m_vLeftBorder;
    float  m_vTopBorder;
    float  m_vRightBorder;
    float  m_vBottomBorder;

    bool   m_bHasFocus;
    bool   m_bCloseState;
    bool   m_bZoomState;
    bool   m_bMinimizeState;
};

#endif // __F_PHOTONDECORATOR_H__




