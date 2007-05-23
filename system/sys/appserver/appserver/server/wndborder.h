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

namespace os {
  class WindowDecorator;
}

/** Represents the border around a window
 * \par Description:
 *	This class is used to add the border around a window.  It does the drawing and mouse
 * 	handling for decorators
 *
 * \sa os::Layer, os::WindowDecorator, os::Gate, os::Rect, os::IRect, os::IPoint, os::Point, SrvWindow, os::Bitmap
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
class WndBorder : public Layer
{
public:
	
    WndBorder( SrvWindow* pcWindow, Layer* pcParent, const char* pzName, bool bBackdrop );
	~WndBorder();

    void									SetDecorator( os::WindowDecorator* pcDecorator );
    os::WindowDecorator* 					GetDecorator() const;
    void		 							SetFlags( uint32 nFlags );
    Layer*	       	 						GetClient() const { return( m_pcClient ); }
    void	     	 						SetSizeLimits( const os::Point& cMinSize, const os::Point& cMaxSize );
    void	     	 						GetSizeLimits( os::Point* pcMinSize, os::Point* pcMaxSize );
    void		 							SetAlignment( const os::IPoint& cSize, const os::IPoint& cSizeOffset, const os::IPoint& cPos, const os::IPoint& cPosOffset );
    os::Rect	 							AlignFrame( const os::Rect& cFrame );
    
    virtual void	 						SetFrame( const os::Rect& cRect );
    virtual void	 						Paint( const os::IRect& cUpdateRect, bool bUpdate = false );

    bool									MouseMoved( os::Messenger* pcAppTarget, const os::Point& cNewPos, int nTransit );
    bool									MouseDown( os::Messenger* pcAppTarget, const os::Point& cPos, int nButton );
    void									MouseUp( os::Messenger* pcAppTarget, const os::Point& cPos, int nButton );
    void									WndMoveReply( os::Messenger* pcAppTarget );
    bool 									HasPendingSizeEvents() const;
private:
    os::IPoint								GetHitPointBase() const;
    os::IRect								AlignRect( const os::IRect& cRect, const os::IRect& cBorders );
    void									DoSetFrame( const os::Rect& cRect );

private:
    Layer* 		    						m_pcClient;
    os::WindowDecorator*    				m_pcDecorator;
    os::IPoint     		    				m_cHitOffset;
    os::WindowDecorator::hit_item 			m_eHitItem; // Item hit by the intital mouse-down event.
    os::WindowDecorator::hit_item 			m_eCursorHitItem;
	int										m_nButtonDown[ os::WindowDecorator::HIT_MAX_ITEMS ];
    
	
	bool	    	    					m_bWndMovePending;
    bool		    						m_bFrameUpdated;
    bool									m_bFrameMaxHit;


    os::IRect		    					m_cRawFrame;
    os::IPoint		    					m_cAlignSize;
    os::IPoint		    					m_cAlignSizeOff;
    os::IPoint		    					m_cAlignPos;
    os::IPoint		    					m_cAlignPosOff;
    os::IRect								m_cStoredFrame;
    
    
    os::IPoint  		    				m_cMinSize;
    os::IPoint		    					m_cMaxSize;    
    os::IPoint     		    				m_cDeltaMove;
    os::IPoint     		    				m_cDeltaSize;
};

#endif // __F_WNDBORDER_H__



