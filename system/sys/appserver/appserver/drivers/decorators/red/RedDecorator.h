/*
 *  "Red" Window-Decorator
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

#ifndef __F_REDDECORATOR_H__
#define __F_REDDECORATOR_H__

#include <windowdecorator.h>
#include <gui/font.h>

#include <string>

using namespace os;

class RedDecorator : public os::WindowDecorator
{
public:
    RedDecorator( Layer* pcLayer, uint32 nWndFlags );
    virtual ~RedDecorator();
  
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
    virtual void SetDepthButtonState( bool bPushed );
    virtual void SetButtonState( uint32 nButton, bool bPushed );
    virtual void Render( const Rect& cUpdateRect );
private:
    void CalculateBorderSizes();
    void Layout();
    void DrawDepth( const Rect& cRect, bool bActive, bool bRecessed );
    void DrawZoom(  const Rect& cRect, bool bActive, bool bRecessed );
    void DrawClose( const Rect& cRect, bool bActive, bool bRecessed );
    void DrawPanel( const Rect& cRect, bool bActive, bool bRecessed );
    
    os::font_height m_sFontHeight;
    Rect	    m_cBounds;
    std::string	    m_cTitle;
    uint32	    m_nFlags;

    Rect   m_cCloseRect;
    Rect   m_cCloseRect2;
    Rect   m_cZoomRect;
    Rect   m_cToggleRect;
    Rect   m_cDragRect;
  
    float  m_vLeftBorder;
    float  m_vTopBorder;
    float  m_vRightBorder;
    float  m_vBottomBorder;

    bool   m_bHasFocus;
    bool   m_bCloseState;
    bool   m_bZoomState;
    bool   m_bDepthState;
};

#endif // __F_NEXTDECORATOR_H__

