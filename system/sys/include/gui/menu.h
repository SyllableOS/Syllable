/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef	_GUI_MENU_HPP
#define	_GUI_MENU_HPP

#include <gui/view.h>
#include <gui/window.h>
#include <util/invoker.h>
#include <util/locker.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class	Menu;
class	MenuWindow;

enum MenuLayout_e { ITEMS_IN_COLUMN, ITEMS_IN_ROW, ITEMS_CUSTOM_LAYOUT };


/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class MenuItem : public Invoker
{
public:
    MenuItem( const char* pzLabel, Message* pcMsg );
    MenuItem( Menu* pcMenu, Message* pcMsg );
    ~MenuItem();

    Menu*	  GetSubMenu() const;
    Menu*	  GetSuperMenu() const;
    Rect	  GetFrame() const;
    virtual Point GetContentSize();
    const char*	  GetLabel() const;
    virtual void  Draw();
    virtual void  DrawContent();
    virtual void  Highlight( bool bHighlight );
    Point	  GetContentLocation() const;
    virtual bool  Invoked( Message* pcMessage );
    
private:
    friend class Menu;
    MenuItem*	m_pcNext;
    Menu*		m_pcSuperMenu;
    Menu*		m_pcSubMenu;
    Rect		m_cFrame;

    char*		m_pzLabel;

    bool		m_bIsHighlighted;
};

/** Menu separator item.
 * \ingroup gui
 * \par Description:
 *	A os::MenuSeparator can be inserted to a menu to categorize other
 *	items. The separator will draw an etched line in the menu.
 * \since 0.3.7
 * \sa os::MenuItem, os::Menu
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class MenuSeparator : public MenuItem
{
public:
    MenuSeparator();
    ~MenuSeparator();

    virtual Point GetContentSize();
    virtual void  Draw();
    virtual void  DrawContent();
    virtual void  Highlight( bool bHighlight );
private:
};

/**
 * \ingroup gui
 * \par Description:
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class	Menu : public View
{
public:
    Menu( Rect cFrame, const char* pzName, MenuLayout_e eLayout,
	  uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP,
	  uint32 nFlags  = WID_WILL_DRAW | WID_CLEAR_BACKGROUND | WID_FULL_UPDATE_ON_RESIZE );

    ~Menu();
    int		Lock( void ) const;
    void	Unlock( void ) const;

//	From Handler:
    virtual void  TimerTick( int nID );

//	From View:
    void		AttachedToWindow( void );
    void		DetachedFromWindow( void );
    void		WindowActivated( bool bIsActive );

    Point		GetPreferredSize( bool bLargest ) const;
    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void	MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void	FrameSized( const Point& cDelta );

    virtual void	Paint( const Rect& cUpdateRect );

//	From Menu:
    std::string GetLabel() const;
    MenuLayout_e GetLayout() const;
    bool	AddItem( const char* pzLabel, Message* pcMessage );
    bool	AddItem( MenuItem* pcItem );
    bool	AddItem( MenuItem* pcItem, int nIndex );
    bool	AddItem( Menu* pcItem );
    bool	AddItem( Menu* pcItem, int nIndex );

    MenuItem*	RemoveItem( int nIndex );
    bool	RemoveItem( MenuItem* pcItem );
    bool	RemoveItem( Menu* pcMenu );

    MenuItem*	GetItemAt( int nIndex ) const;
    MenuItem*	GetItemAt( Point cPos ) const;
    Menu*	GetSubMenuAt( int nIndex ) const;
    int		GetItemCount( void ) const;

    int		GetIndexOf( MenuItem* pcItem ) const;
    int		GetIndexOf( Menu* pcMenu ) const;

    MenuItem*	FindItem( int nCode ) const;
    MenuItem*	FindItem( const char* pzName ) const;

    virtual	status_t SetTargetForItems( Handler* pcTarget );
    virtual	status_t SetTargetForItems( Messenger cMessenger );
    MenuItem*	FindMarked() const;

    Menu*	GetSuperMenu() const;
    MenuItem*	GetSuperItem();
    void	InvalidateLayout();

    void	SetCloseMessage( const Message& cMsg );
    void	SetCloseMsgTarget( const Messenger& cTarget );

    MenuItem*	Track( const Point& cScreenPos );
    void 	Open( Point cScrPos );

private:
    friend	class MenuItem;

    void		SetRoot( Menu* pcRoot );
    void		StartOpenTimer( bigtime_t nDelay );
    void		OpenSelection();
    void		SelectItem( MenuItem* pcItem );
    void		SelectPrev();
    void		SelectNext();
    void		EndSession( MenuItem* pcSelItem );
    void		Close( bool bCloseChilds, bool bCloseParent );
    void		SetCloseOnMouseUp( bool bFlag );
    static Locker s_cMutex;
    port_id	  m_hTrackPort;
    MenuWindow*	  m_pcWindow;
    Menu*	  m_pcRoot;
    MenuItem*	  m_pcFirstItem;
    int		  m_nItemCount;
    MenuLayout_e  m_eLayout;
    MenuItem*	  m_pcSuperItem;
    Point	  m_cSize;

    Rect	  m_cItemBorders;

    class Private;
    Private*	m;
    bool	  m_bIsTracking;
    bool	  m_bHasOpenChilds;
    bool	  m_bCloseOnMouseUp;
};

}
#endif	//	_GUI_MENU_HPP
