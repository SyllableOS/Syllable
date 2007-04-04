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

#ifndef	__F_GUI_MENU_H__
#define	__F_GUI_MENU_H__

#include <gui/view.h>
#include <gui/window.h>
#include <util/invoker.h>
#include <util/locker.h>
#include <gui/bitmap.h>
#include <gui/image.h>
#include <gui/exceptions.h>
#include <util/shortcutkey.h>
namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class	Menu;
class	MenuWindow;

enum MenuLayout_e { ITEMS_IN_COLUMN, ITEMS_IN_ROW, ITEMS_CUSTOM_LAYOUT };


/** Menu item class.
 * \ingroup gui
 * \par Description:
 *	The MenuItem class is what you use to insert into a menu
 * \author	Kurt Skauen (kurt@atheos.cx) with modifications by the Syllable team
 *****************************************************************************/

class MenuItem : public Invoker
{
public:
    MenuItem( const String& cLabel, Message* pcMsg, const String& cShortcut = "", Image* pcImage = NULL);
    MenuItem( Menu* pcMenu, Message* pcMsg, const String& cShortcut = "", Image* pcImage = NULL );
    ~MenuItem();

    Menu*				GetSubMenu() const;
    Menu*				GetSuperMenu() const;
    Rect				GetFrame() const;
    
    virtual Point 		GetContentSize();
    virtual float		GetColumnWidth( int nColumn ) const;
    virtual int			GetNumColumns() const;
    const String&  		GetLabel() const;
    void				SetLabel( const os::String& cTitle );
    
    virtual void  		Draw();

    Point	  			GetContentLocation() const;
    
    virtual bool  		Invoked( Message* pcMessage );
    void 				SetEnable( bool bEnabled);
    bool 				IsEnabled() const;
    
    virtual void 		SetHighlighted( bool bHighlighted );
	bool 				IsHighlighted() const;
	
	Image* 				GetImage() const;
	void 				SetImage( Image* pcImage, bool bRefresh = false );
	
	void 				SetShortcut( const String& cShortcut );
	const 				String& GetShortcut() const;

	virtual	bool		IsSelectable() const;

private:
	virtual void		__MI_reserved_1__();
	virtual void		__MI_reserved_2__();
	virtual void		__MI_reserved_3__();
	virtual void		__MI_reserved_4__();
	virtual void		__MI_reserved_5__();
private:
    MenuItem& operator=( const MenuItem& );
    MenuItem( const MenuItem& );

    friend class Menu;
    class Private;
    
    MenuItem*			_GetNext();
	void				_SetNext( MenuItem* pcItem );
	void				_SetSubMenu( Menu* pcMenu );
	void				_SetSuperMenu( Menu* pcMenu );
    void				_SetFrame( const Rect& cFrame );
	Private* 			m;
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
    virtual void  SetHighlighted( bool bHighlight );
	virtual bool  IsSelectable() const {return false;}

private:
	virtual void		__MS_reserved_1__();
	virtual void		__MS_reserved_2__();

private:
    MenuSeparator& operator=( const MenuSeparator& );
    MenuSeparator( const MenuSeparator& );
};

/** The menuing system for Syllable
 * \ingroup gui
 * \par Description:
 *  The menuing system for Syllable.	
 * \sa os::MenuItem, os::MenuSeparator()
 * \author Kurt Skauen (kurt@atheos.cx) with modifications and improvements by the Syllable team.
 *****************************************************************************/

class	Menu : public View
{
public:
    Menu( Rect cFrame, const String& cName, MenuLayout_e eLayout,
	  uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP,
	  uint32 nFlags  = WID_WILL_DRAW | WID_CLEAR_BACKGROUND | WID_FULL_UPDATE_ON_RESIZE );
	~Menu();
    
    int					Lock( void ) const;
    void				Unlock( void ) const;

//	From Handler:
    virtual void  		TimerTick( int nID );

//	From View:
    void				AttachedToWindow( void );
    void				DetachedFromWindow( void );
    void				WindowActivated( bool bIsActive );

	Point				GetPreferredSize( bool bLargest ) const;
    
    virtual void		MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void		MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void		MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void		KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void		FrameSized( const Point& cDelta );
	virtual void		Paint( const Rect& cUpdateRect );

//	From Menu:
    String 				GetLabel() const;
    void				SetLabel( const os::String& cLabel );
    MenuLayout_e 		GetLayout() const;
    
    bool				AddItem( const String& cLabel, Message* pcMessage, const String& cShortcut = "", Image* pcImage = NULL );
    bool				AddItem( MenuItem* pcItem );
    bool				AddItem( MenuItem* pcItem, int nIndex );
    bool				AddItem( Menu* pcItem );
    bool				AddItem( Menu* pcItem, int nIndex );

    MenuItem*			RemoveItem( int nIndex );
    bool				RemoveItem( MenuItem* pcItem );
    bool				RemoveItem( Menu* pcMenu );

    MenuItem*			GetItemAt( int nIndex ) const;
    MenuItem*			GetItemAt( Point cPos ) const;
    Menu*				GetSubMenuAt( int nIndex ) const;
    int					GetItemCount( void ) const;

    int					GetIndexOf( MenuItem* pcItem ) const;
    int					GetIndexOf( Menu* pcMenu ) const;

    MenuItem*			FindItem( int nCode ) const;
    MenuItem*			FindItem( const String& cName ) const;
    MenuItem*			FindMarked() const;
    
    virtual	status_t 	SetTargetForItems( Handler* pcTarget );
    virtual	status_t 	SetTargetForItems( Messenger cMessenger );

    Menu*				GetSuperMenu() const;
    MenuItem*			GetSuperItem();
    void				InvalidateLayout();

    void				SetCloseMessage( const Message& cMsg );
    void				SetCloseMsgTarget( const Messenger& cTarget );

    MenuItem*			Track( const Point& cScreenPos );
    void 				Open( Point cScrPos );
    
    void 				SetEnable(bool);
    bool 				IsEnabled();

private:
	virtual void		__ME_reserved_1__();
	virtual void		__ME_reserved_2__();
	virtual void		__ME_reserved_3__();
	virtual void		__ME_reserved_4__();
	virtual void		__ME_reserved_5__();
private:
    Menu& operator=( const Menu& );
    Menu( const Menu& );

    friend	class MenuItem;

    void				_SetRoot( Menu* pcRoot );
    
    void				_StartOpenTimer( bigtime_t nDelay );
    void				_OpenSelection();
    
    void				_SelectItem( MenuItem* pcItem);
    void				_SelectPrev();
    void				_SelectNext();
    
    void				_EndSession( MenuItem* pcSelItem );
    void				_Close( bool bCloseChilds, bool bCloseParent );
    void				_SetCloseOnMouseUp( bool bFlag );

	void				_SetOrClearShortcut( MenuItem* pcStart, bool bSet = true );
	Image*				_GetSubMenuArrow( int nState );
	float				_GetColumnWidth( int nColumn );

    class Private;
    Private*	m;
    
    MenuItem*	  	m_pcSuperItem;
    static 			Locker s_cMutex;
};

}
#endif	//	__F_GUI_MENU_H__

