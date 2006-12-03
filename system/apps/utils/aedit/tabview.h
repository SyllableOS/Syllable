// AEdit -:-  (C)opyright 2006 Jonas Jarvoll
//		      
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __TABVIEW_H_
#define __TABVIEW_H_

#include <vector>
#include <string>

#include <gui/rect.h>
#include <gui/point.h>
#include <gui/view.h>
#include <gui/font.h>
#include <util/string.h>
#include <util/invoker.h>
#include <util/message.h>

class TabView;

class TabViewTab {
public:
	TabViewTab( const os::String& cTitle, os::View* pcView = NULL );
	virtual ~TabViewTab();
	
	void SetView( os::View* pcView );
	os::View* GetView() const;
	virtual void SetLabel( const os::String& cLabel );
	const os::String& GetLabel() const;
	virtual void SetOwner( TabView* pcTabView );
	TabView* GetOwner() const;
	
	virtual os::Point GetSize() const;
	
	virtual void Paint( os::View* pcView, const os::Rect& cBounds, int nIndex );

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

class TabView : public os::View, public os::Invoker
{
public:
	enum TabType { SHOW_TABS, HIDE_TABS, AUTOMATIC_TABS };

    TabView( const os::Rect& cFrame, const os::String& cTitle,
	     	  uint32 nResizeMask = os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP,
	     	  uint32 nFlags = os::WID_WILL_DRAW | os::WID_FULL_UPDATE_ON_RESIZE | os::WID_DRAW_ON_CHILDREN );

	~TabView();

    int			AppendTab( const os::String& cTitle, os::View* pcView = NULL );
    int			InsertTab( uint nIndex, const os::String& cTitle, os::View* pcView = NULL );
    int			AppendTab( TabViewTab* pcTab, os::View* pcView = NULL );
    int			InsertTab( uint nIndex, TabViewTab* pcTab, os::View* pcView = NULL );
    View*		DeleteTab( uint nIndex );
    View*		GetTabView( uint nIndex ) const;
    int			GetTabCount() const;

    int			SetTabTitle( uint nIndex, const os::String& cTitle );
    const os::String&	GetTabTitle( uint nIndex ) const;

    uint		GetSelection() const;
    void		SetSelection( uint nIndex, bool bNotify = true );

    virtual void	FrameSized( const os::Point& cDelta );
    virtual void	MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
    virtual void	MouseDown( const os::Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const os::Point& cPosition, uint32 nButtons, os::Message* pcData );
    virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );

    virtual os::Point	GetPreferredSize( bool bLargest ) const;
    virtual void	Paint( const os::Rect& cUpdateRect );
    virtual void	AllAttached();

	void SetTabType( enum TabType type );
	int GetTabType();

	bool DrawTabs();

private:
    virtual void	__TABV_reserved1__();
    virtual void	__TABV_reserved2__();
    virtual void	__TABV_reserved3__();
    virtual void	__TABV_reserved4__();
    virtual void	__TABV_reserved5__();

private:
    TabView& operator=( const TabView& );
    TabView( const TabView& );

	class ETopView : public os::View
	{
		public:
		ETopView( const os::Rect& cFrame, TabView* pcParent ) :
			os::View( cFrame, "top_view", os::CF_FOLLOW_NONE, os::WID_WILL_DRAW | os::WID_DRAW_ON_CHILDREN  ) {
			m_pcParent = pcParent;
		}
		virtual void Paint( const os::Rect& cUpdateRect );
		private:
		TabView* m_pcParent;	
	};

	friend class TabView::ETopView;
    
    class Private;
    
    Private *m;
};

#endif

