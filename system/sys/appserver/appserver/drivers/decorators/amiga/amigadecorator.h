/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __F_AMIGADECORATOR_H__
#define __F_AMIGADECORATOR_H__

#include <windowdecorator.h>
#include <gui/font.h>

#include <string>

using namespace os;

class AmigaDecorator : public os::WindowDecorator
{
public:
    AmigaDecorator( Layer* pcLayer, uint32 nWndFlags );
    virtual ~AmigaDecorator();
  
    virtual hit_item HitTest( const Point& cPosition );
    virtual void FrameSized( const Rect& cNewFrame );
    virtual Rect GetBorderSize();
    virtual Point GetMinimumSize();
    virtual void SetFlags( uint32 nFlags );
    virtual void FontChanged();
    virtual void SetTitle( const char* pzTitle );
    virtual void SetWindowFlags( uint32 nFlags );
    virtual void SetFocusState( bool bHasFocus );
    virtual void SetButtonState( uint32 nButton, bool bPushed );
    virtual void Render( const Rect& cUpdateRect );
private:
    void CalculateBorderSizes();
    void Layout();
	void DrawButton( uint32 nButton, const os::Color32_s& sFillColor );
	uint32 CheckIndex( uint32 nButton );
  
    os::font_height m_sFontHeight;
    Rect	    m_cBounds;
    std::string	    m_cTitle;
    uint32	    m_nFlags;

	os::Rect	m_cObjectFrame[ HIT_DRAG+1 ];
  
    float  m_vLeftBorder;
    float  m_vTopBorder;
    float  m_vRightBorder;
    float  m_vBottomBorder;

    bool   m_bHasFocus;

    bool		m_bObjectState[ HIT_DRAG+1 ];
};

#endif // __F_AMIGADECORATOR_H__


