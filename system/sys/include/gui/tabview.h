/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Syllable Team
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

class TabView;

class TabViewTab {
public:
	TabViewTab( const String& cTitle, View* pcView = NULL );
	virtual ~TabViewTab();
	
	void SetView( View* pcView );
	View* GetView() const;
	virtual void SetLabel( const String& cLabel );
	const String& GetLabel() const;
	virtual void SetOwner( TabView* pcTabView );
	TabView* GetOwner() const;
	
	virtual Point GetSize() const;
	
	virtual void Paint( View* pcView, const Rect& cBounds, int nIndex );

private:
    virtual void	__TABT_reserved1__();
    virtual void	__TABT_reserved2__();
    virtual void	__TABT_reserved3__();
    virtual void	__TABT_reserved4__();
    virtual void	__TABT_reserved5__();

private:
    TabViewTab& operator=( const TabViewTab& );
    TabViewTab( const TabViewTab& );

	class Private;
	Private* m;
};

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
    TabView( const Rect& cFrame, const String& cTitle,
	     uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
	     uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );

	~TabView();

    int			AppendTab( const String& cTitle, View* pcView = NULL );
    int			InsertTab( uint nIndex, const String& cTitle, View* pcView = NULL );
    int			AppendTab( TabViewTab* pcTab, View* pcView = NULL );
    int			InsertTab( uint nIndex, TabViewTab* pcTab, View* pcView = NULL );
    View*		DeleteTab( uint nIndex );
    View*		GetTabView( uint nIndex ) const;
    int			GetTabCount() const;

    int			SetTabTitle( uint nIndex, const String& cTitle );
    const String&	GetTabTitle( uint nIndex ) const;

    uint		GetSelection() const;
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
    virtual void	__TABV_reserved1__();
    virtual void	__TABV_reserved2__();
    virtual void	__TABV_reserved3__();
    virtual void	__TABV_reserved4__();
    virtual void	__TABV_reserved5__();

private:
    TabView& operator=( const TabView& );
    TabView( const TabView& );

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
    
    class Private;
    
    Private *m;
};

}

#endif //__F_GUI_TABVIEW_H__
