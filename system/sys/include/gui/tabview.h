/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef	__F_GUI_TABVIEW_H__
#define	__F_GUI_TABVIEW_H__

#include <atheos/types.h>

#include <gui/view.h>
#include <gui/font.h>
#include <util/invoker.h>

#include <vector>
#include <string>

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class TabView : public View, public Invoker
{
public:
    TabView( const Rect& cFrame, const char* pzTitle,
	     uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
	     uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );

    int			AppendTab( const char* pzTitle, View* pcView = NULL );
    int			InsertTab( uint nIndex, const char* pzTitle, View* pcView = NULL );
    View*		DeleteTab( uint nIndex );
    View*		GetTabView( uint nIndex ) const;
    int			GetTabCount() const;

    int			SetTabTitle( uint nIndex, const std::string& cTitle );
    const std::string&	GetTabTitle( uint nIndex ) const;

    uint		GetSelection();
    void		SetSelection( uint nIndex, bool bNotify = true );

    virtual void	FrameSized( const Point& cDelta );
    virtual void	MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );

    virtual Point	GetPreferredSize( bool bLargest ) const;
    virtual void	Paint( const Rect& cUpdateRect );
    virtual void	AllAttached();

private:
    struct Tab {
	Tab( const char* pzTitle, View* pcView ) : m_cTitle( pzTitle ) { m_pcView = pcView; }
	View* m_pcView;
	std::string m_cTitle;
	float	    m_vWidth;
    };

    class TopView : public View
    {
    public:
	TopView( const Rect& cFrame, TabView* pcParent ) :
		View( cFrame, "top_view", CF_FOLLOW_NONE, WID_WILL_DRAW ) {
		    m_pcParent = pcParent;
	}
	
	virtual void Paint( const Rect& cUpdateRect );
    private:
	TabView* m_pcParent;
    };

    friend class TabView::TopView;
  
    uint	     m_nSelectedTab;
    float	     m_vScrollOffset;
    Point	     m_cHitPos;
    bool	     m_bMouseClicked;
    float	     m_vTabHeight;
    float	     m_vGlyphHeight;
    font_height      m_sFontHeight;
    float	     m_vTotalWidth;
    std::vector<Tab> m_cTabList;
    TopView*	     m_pcTopView;
};


}


#endif //__F_GUI_TABVIEW_H__

