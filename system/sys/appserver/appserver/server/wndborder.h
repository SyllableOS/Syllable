#ifndef __F_WNDBORDER_H__
#define __F_WNDBORDER_H__

/*
 *  The AtheOS application server
 *  Copyright (C) 1999  Kurt Skauen
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

#include "layer.h"
#include "windowdecorator.h"

using namespace os;

namespace os {
  class WindowDecorator;
}

class WndBorder : public Layer
{
public:
    WndBorder( SrvWindow* pcWindow, Layer* pcParent, const char* pzName, bool bBackdrop );

    void		 SetDecorator( os::WindowDecorator* pcDecorator );
    os::WindowDecorator* GetDecorator() const;
    void		 SetFlags( uint32 nFlags );
    Layer*	       	 GetClient() const { return( m_pcClient ); }
    void	     	 SetSizeLimits( const Point& cMinSize, const Point& cMaxSize );
    void	     	 GetSizeLimits( Point* pcMinSize, Point* pcMaxSize );
    void		 SetAlignment( const IPoint& cSize, const IPoint& cSizeOffset, const IPoint& cPos, const IPoint& cPosOffset );
    
    virtual void	 SetFrame( const Rect& cRect );
    virtual void	 Paint( const IRect& cUpdateRect, bool bUpdate = false );

    bool		MouseMoved( Messenger* pcAppTarget, const Point& cNewPos, int nTransit );
    bool		MouseDown( Messenger* pcAppTarget, const Point& cPos, int nButton );
    void		MouseUp( Messenger* pcAppTarget, const Point& cPos, int nButton );
    void		WndMoveReply( Messenger* pcAppTarget );
    bool 		HasPendingSizeEvents() const;
private:
    IPoint	GetHitPointBase() const;
    IRect	AlignRect( const IRect& cRect, const IRect& cBorders );
    void	DoSetFrame( const Rect& cRect );
    Layer* 		    m_pcClient;
    os::WindowDecorator*    m_pcDecorator;
    IPoint     		    m_cHitOffset;
    WindowDecorator::hit_item m_eHitItem; // Item hit by the intital mouse-down event.
    WindowDecorator::hit_item m_eCursorHitItem;
    int	     		    m_nCloseDown;
    int	     		    m_nZoomDown;
    int	     		    m_nToggleDown;
    bool	    	    m_bWndMovePending;
    bool		    m_bFrameUpdated;
    IPoint  		    m_cMinSize;
    IPoint		    m_cMaxSize;

    IRect		    m_cRawFrame;
    IPoint		    m_cAlignSize;
    IPoint		    m_cAlignSizeOff;
    IPoint		    m_cAlignPos;
    IPoint		    m_cAlignPosOff;
    
    IPoint     		    m_cDeltaMove;
    IPoint     		    m_cDeltaSize;
};

#endif // __F_WNDBORDER_H__
