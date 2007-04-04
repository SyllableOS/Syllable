/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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


#include <stdio.h>
#include <assert.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <gui/menu.h>
#include <gui/font.h>
#include <gui/bitmap.h>
#include <util/message.h>
#include <atheos/kernel.h>
#include <typeinfo>
#include <iostream>

#define MIN_ICON_MARGIN			16		// Always leave 16 pixels, even if there are no icons
#define ICON_TO_TEXT_SPACE		4		// Space between icon and text
#define TITLE_TO_SHORTCUT_SPACE	16		// Space between text and shortcut
#define H_ROW_SPACE				2		// Space to the left/right of text in a menu bar
#define STICKY_THRESHOLD		5.0f	// If mouse button is released outside a STICKY_THRESHOLD
										// pixels radius from where it was pressed, the menu enters
										// "sticky mode".

#define SUB_MENU_ARROW_W 5
#define SUB_MENU_ARROW_H 8

uint8 nSubMenuArrowData[] = {
	#include "pixmaps/submenu.hex"
};

enum {
	IMG_STATE_NORMAL = 0,
	IMG_STATE_GRAY = 1,
	IMG_STATE_HIGHLIGHT = 2
};

namespace os
{
	class MenuWindow :public Window
	{
	   public:
		MenuWindow( const Rect& cFrame, Menu* pcMenu );
		 ~MenuWindow();

		void ShutDown()
		{
			if( m_pcMenu != NULL )
			{
				
				RemoveChild( m_pcMenu );
				m_pcMenu = NULL;
			}
			PostMessage( M_TERMINATE );
		}
	      private:
		  Menu* m_pcMenu;
	};
}

using namespace os;

Locker Menu::s_cMutex( "menu_mutex" );

/************************beginning of private classes************************/
class Menu::Private
{
public:
	Private():m_cMutex( "menu_mutex" )
	{
			m_bIsTracking = false;
			m_cItemBorders = Rect( 3, 2, 3, 2 );
			m_eLayout = os::ITEMS_IN_COLUMN;
			m_pcWindow = NULL;
			m_pcFirstItem = NULL;
			m_nItemCount = 0;
			m_bHasOpenChilds = false;
			m_bCloseOnMouseUp = false;
			m_hTrackPort = -1;
			m_bEnabled = true;
			m_bIsRootMenu = true;
			m_pcSubArrowImage[0] = NULL;
			m_pcSubArrowImage[1] = NULL;
	}
	
	~Private()
	{
		if( m_pcSubArrowImage[0] )
			delete m_pcSubArrowImage[0];
		if( m_pcSubArrowImage[1] )
			delete m_pcSubArrowImage[1];
	}
	
	String 			m_cTitle;
	Message 		m_cCloseMsg;
	Messenger 		m_cCloseMsgTarget;
	Locker 			m_cMutex;
    port_id	  		m_hTrackPort;
    MenuWindow*	  	m_pcWindow;
    bool			m_bIsRootMenu;
    Menu*	  		m_pcRoot;
    MenuItem*	  	m_pcFirstItem;
    int		  		m_nItemCount;
    MenuLayout_e  	m_eLayout;
    Point	  		m_cSize;
	Rect	  		m_cItemBorders;
	Point			m_cMouseHitPos;
    
    bool	  		m_bIsTracking;
    bool	  		m_bHasOpenChilds;
    bool	  		m_bCloseOnMouseUp;
    bool			m_bEnabled;

	Image*			m_pcSubArrowImage[2];
	
	float			m_vColumnWidths[5];	// with std::vector we could have many more columns
};

class MenuItem::Private
{
public:
	Private()
	{
		m_pcNext = NULL;
		m_pcSuperMenu = NULL;
		m_pcSubMenu = NULL;
		m_bIsHighlighted = false;
		m_bIsEnabled = true;
		m_pcMenuWindow = NULL;
		m_pcImage[ 0 ] = NULL;
		m_pcImage[ 1 ] = NULL;
		m_pcImage[ 2 ] = NULL;
		m_bIsSelectable = true;
	}
	
	~Private()
	{
		if( m_pcSubMenu )
			delete m_pcSubMenu;
		FreeImages();
	}

	void FreeImages()
	{
		int i;
		for( i = 0; i < 3; i++ ) {
			if( m_pcImage[ i ] ) {
				delete m_pcImage[ i ];
				m_pcImage[ i ] = NULL;
			}
		}
	}

	Image* GetImage( int nState )
	{
		if( !m_pcImage[ nState ] && nState > 0 ) {		
			/* Note: This is not a good method, since it will only work on Bitmaps. */
			/* We have to add some method to Image for copying Images of unknown type! */
			if( m_pcImage[ 0 ] && ( typeid( *m_pcImage[ 0 ] ) == typeid( BitmapImage ) ) )
			{
				m_pcImage[ nState ] = new BitmapImage( *( dynamic_cast < BitmapImage * >( m_pcImage[ 0 ] ) ) );
				
				if( m_pcImage[ nState ] ) {
					switch( nState )
					{
						case IMG_STATE_GRAY:
							m_pcImage[ nState ]->ApplyFilter( Image::F_GRAY );
							break;
						case IMG_STATE_HIGHLIGHT:		
							m_pcImage[ nState ]->ApplyFilter( Image::F_HIGHLIGHT );
							break;
					}
				}
			}
		}
		return m_pcImage[ nState ] ? m_pcImage[ nState ] : m_pcImage[ 0 ];
	}
	
	MenuItem *m_pcNext;
	Menu *m_pcSuperMenu;
	Menu *m_pcSubMenu;
	Rect m_cFrame;

	String m_cLabel;
	String m_cShortcut;

	bool m_bIsHighlighted;
	bool m_bIsEnabled;
	bool m_bIsSelectable;
	
	Image* m_pcImage[ 3 ];
	ShortcutKey m_cKey;
	Window* m_pcMenuWindow;
	Message* m_pcMessage;
};
/************************end of private classes*********************************/


/***********************beginning of menuitem***********************************/

/** Constructor.
 * \par Description:
 * 	This MenuItem constructor will create a menu item that holds a sub menu and attaches an image to the menuitem.
 * \param pcMenu	- The menu that will be a sub menu.
 * \param pcMsg		- The message that will be passed when invoked.
 * \param cShortcut	- Keyboard shortcut.
 * \param pcImage	- The image that will be attached to this menu item.
 * \par Example:
 * \code
 *	Menu* pcMenu = new Menu(Rect(),"main_menu",os::ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP ); //creates the main menu
 	Menu* pcSubMenu = new Menu(Rect(),"File", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP ); //creates the file menu
 	Menu* pcOptionsMenu = new Menu(Rect(),"Options", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP ); //creates the option menu
 	File cFile = File("./options.png"); //not the best way to do this, because then options.png will need to be in the same directory as the application at all times.
 	Image* pcImage = new BitmapImage(&cFile);
 	MenuItem* pcItem(pcMenu,NULL,pcImage) // creates the menu item with the sub menu and image.
 	pcSubMenu->AddItem(pcItem); //attaches the item to the file menu
 	pcMenu->AddItem(pcSubMenu) //attaches the file menu to the main menu
 * \endcode
 *
 * \sa os::Menu, os::MenuSeparator
 * \author Kurt Skauen(with modifications by the Syllable Team).
  *****************************************************************************/ 	
MenuItem::MenuItem(Menu* pcMenu, Message* pcMsg, const String& cShortcut, Image* pcImage) : Invoker(pcMsg, NULL,NULL)
{
	m = new Private;
	m->m_pcSubMenu = pcMenu;
	m->m_pcMessage = pcMsg;
	pcMenu->m_pcSuperItem = this;
	m->m_cLabel = pcMenu->GetLabel();
	m->m_cShortcut = cShortcut;

	SetImage( pcImage );
}

/** Constructor.
 * \par Description:
 * 	This MenuItem constructor will create a MenuItem with and Image attached to it.
 * \param cLabel	- The label that will be displayed.
 * \param pcMsg		- The message that will be passed when invoked.
 * \param cShortcut	- Keyboard shortcut.
 * \param pcImage	- The image that will be attached to the MenuItem.
 * \par Example:
 * \code
 *	Menu* pcMenu = new Menu(Rect(),"main_menu",os::ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP ); //creates the main menu
 	Menu* pcSubMenu = new Menu(Rect(),"File", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP ); //creates the file menu
 	File cFile = File("./options.png"); //not the best way to do this, because then options.png will need to be in the same directory as the application at all times.
 	Image* pcImage = new BitmapImage(&cFile);
 	MenuItem* pcItem("Options",NULL,pcImage) // creates the menu item with the sub menu and image.
 	pcSubMenu->AddItem(pcItem); //attaches the item to the file menu
 	pcMenu->AddItem(pcSubMenu) //attaches the file menu to the main menu
 * \endcode
 *
 * \sa os::Menu, os::MenuSeparator
 * \author Kurt Skauen(with modifications by the Syllable Team).
  *****************************************************************************/ 	
MenuItem::MenuItem( const String& cLabel, Message* pcMsg, const String& cShortcut, Image* pcImage ):Invoker( pcMsg, NULL, NULL )
{
	m = new Private;
	m->m_cLabel = cLabel;
	m->m_pcMessage = pcMsg;
	m->m_cShortcut = cShortcut;
	
	SetImage( pcImage );
}

/**internal*/
MenuItem::~MenuItem()
{
	delete m;
}


 /** Gets the Sub Menu(Menu that is inside the MenuItem)...
 * \par Description:
 *	Gets the Sub Menu(Menu that is inside the MenuItem)...
 * \return This method returns NULL if there is no Sub Menu and returns the Sub Menu(os::Menu) if there is a Sub Menu.
 * \sa SetSubMenu()
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
Menu *MenuItem::GetSubMenu() const
{
	return ( m->m_pcSubMenu );
}

/** Gets the Super Menu(Menu that holds the MenuItem)...
 * \par Description:
 *	Gets the Super Menu(Menu that holds the MenuItem)...
 * \return This method returns NULL if there is no Super Menu and retuns the Super Menu(os::Menu) if there is a Super Menu.
 * \sa SetSuperMenu()
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
Menu *MenuItem::GetSuperMenu() const
{
	return ( m->m_pcSuperMenu );
}

/** Gets the frame of the MenuItem...
 * \par Description:
 *	Gets the frame of the MenuItem.  
 * \return The frame as an os::Rect
 * \sa SetFrame()
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
Rect MenuItem::GetFrame() const
{
	return ( m->m_cFrame );
}


/** Gets the size of the MenuItem...
 * \par Description:
 *	Gets the size of the MenuItem.  
 * \return The size of the MenuItem as an os::Point
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
Point MenuItem::GetContentSize()
{
	Menu *pcMenu = GetSuperMenu();
	Point cContentSize = pcMenu->GetTextExtent( GetLabel() );
	Point cShortcutSize = pcMenu->GetTextExtent( m->m_cShortcut );

	// Are we a "normal" menu, in a column of other menus?  If so, we need to allow for a gap on the left
	if( pcMenu != NULL && m->m_pcSuperMenu->GetLayout() != ITEMS_IN_ROW )
	{
		if( m->GetImage( 0 ) ) {
			float y = m->GetImage( 0 )->GetSize().y + 4;
			if( y > cContentSize.y )
				cContentSize.y = y;
			cContentSize.x += ICON_TO_TEXT_SPACE;
		}
		cContentSize.x = pcMenu->_GetColumnWidth(0) + pcMenu->_GetColumnWidth(1) + pcMenu->_GetColumnWidth(2);
		if( GetSubMenu() ) {
			cContentSize.x += 32;	// Space for submenu arrow
		}

		return cContentSize;
	}

	// We are a menu which is in a row of menus, I.e. a menu bar.  We don't need a gap, and we can't have a sub-menu
	if( pcMenu != NULL )
	{
		if( m->GetImage( 0 ) ) {
			cContentSize.y = std::max( cContentSize.y, m->GetImage( 0 )->GetSize().y );
			cContentSize.x += m->GetImage( 0 )->GetSize().x+1;
		}
		cContentSize.x += H_ROW_SPACE*2;
	}
	return ( cContentSize );
}

/** Gets the size of a column
 * \par Description:
 *	Gets the size of a column. Column 0 is the space used for icons, 1 is the
 *	space for text and 2 is the space normally used for shortcuts.
 * \return The width of the column
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
float MenuItem::GetColumnWidth( int nColumn ) const
{
	float width;
	Menu *pcMenu = GetSuperMenu();
	
	switch( nColumn ) {
		case 0:
			width = MIN_ICON_MARGIN;
			if( m->GetImage( 0 ) ) {
				width = std::max( m->GetImage( 0 )->GetSize().y + ICON_TO_TEXT_SPACE, width );
			}
			break;
		case 1:
			{
				Point cContentSize = pcMenu->GetTextExtent( GetLabel() );
				if( !(m->m_cShortcut == "") ) {
					cContentSize.x += TITLE_TO_SHORTCUT_SPACE;
				}
				width = cContentSize.x;
			}
			break;
		case 2:
			{
				Point cShortcutSize = pcMenu->GetTextExtent( m->m_cShortcut );
				width = cShortcutSize.x;
			}
			break;
	}
	return width;
}

/** Get number of columns
 * \par Description:
 *	Returns the number of columns. Normally there are three columns.
 *	Column 0 is the space used for icons, 1 is the
 *	space for text and 2 is the space normally used for shortcuts.
 * \return Number of columns
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int MenuItem::GetNumColumns( ) const
{
	return 3;
}

/** Gets the label of the MenuItem...
 * \par Description:
 *	Gets the label of the MenuItem.  
 * \return The label of the MenuItem as an os::String
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
const String& MenuItem::GetLabel() const
{
	return ( m->m_cLabel );
}


/** Sets the label of the MenuItem...
 * \par Description:
 *	Sets the label of the MenuItem.  
 * \par cLabel - The label of the MenuItem as an os::String
 * \author	Arno Klenke
 *****************************************************************************/
void MenuItem::SetLabel( const os::String& cLabel )
{
	m->m_cLabel = cLabel;
}

/** Returns the current location of this item...
 * \par Description:
 *	Returns the current location of this item.  It returns it as a os::Point
 * \sa
 * \author	Kurt Shauen (with modifications from the Syllable team)
 *****************************************************************************/
Point MenuItem::GetContentLocation() const
{
	return ( Point( m->m_cFrame.left, m->m_cFrame.top ) );
}

/** Gets the image that is attached to the MenuItem...
 * \par Description:
 *	Gets the image that is attached to the MenuItem.
 * \return This method will return an image pointer that is attached to the MenuItem.  It will return a null Image if the Image is not set. 
 * \sa SetImage()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
Image* MenuItem::GetImage() const
{
	return m->GetImage( 0 );
}

/** Gets the shortcut for the menu item...
 * \par Description:
 *	Gets the shortcut for the menu item... 
 * \return Right now this will return a default shortcut(until we implement shortcuts)
 * \sa SetShortcut()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
const String& MenuItem::GetShortcut() const
{
	return ( m->m_cShortcut );
}

/** Tells the programmer whether this element is highlighted or not highlighted...
 * \par Description:
 *	This method will tell programmer whether this element is highlighted or not highlighted.
 * \return This method returns true if the MenuItem is highlighted and false if it is not.
 * \sa SetHighlighed()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
bool MenuItem::IsHighlighted() const
{
	return m->m_bIsHighlighted;
}

/** Tells the programmer whether this element is enabled or disabled...
 * \par Description:
 *	This method will tell programmer whether or not this element is enabled.
 * \return This method returns true if this MenuItem is enabled and false if it is not.
 * \sa SetEnable()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
bool MenuItem::IsEnabled() const
{
	return m->m_bIsEnabled;
}

/** Tells the system to disable or enable this element...
 * \par Description:
 *	This method will tell the system to disable or enable this element.
 * \param bEnabled - To enable this element set this to true(default) and to disable set this to false.
 * \sa IsEnabled()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
void MenuItem::SetEnable( bool bEnabled )
{
	m->m_bIsEnabled = bEnabled;
	if (bEnabled)
		m->m_bIsSelectable = true;
	else
		m->m_bIsSelectable = false; 
	Draw();
}

/** Tells the system to highlight or unhighlight this element...
 * \par Description:
 *	This method will tell the system to highlight or unhighlight this element.
 * \param bHighlighted - To highlight this element set this to true(default) and to unhighlight set this to false.
 * \sa IsHighlighted()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
void MenuItem::SetHighlighted( bool bHighlighted )
{
	m->m_bIsHighlighted = bHighlighted;
	Draw();
}

/** Sets the image that you want to attach to the MenuItem...
 * \par Description:
 *	Sets the image that you want to attach to the MenuItem...
 * \param pcImage  - the image to be placed on the MenuItem.
 * \param bRefresh - tell the system whether to or not to redraw the MenuItem
 * \sa GetImage()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
void MenuItem::SetImage( Image* pcImage, bool bRefresh )
{
	m->FreeImages();
	m->m_pcImage[ 0 ] = pcImage;
	if (bRefresh)
		Draw();
}

/** Sets the shortcut for the MenuItem
 * \par Description:
 *	Sets the shortcut for the MenuItem.
 * \sa GetShortcut()
 * \param cShortcut - The shortcut key that will invoke with this menuitem.
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
void MenuItem::SetShortcut( const String& cShortcut )
{
	m->m_cShortcut = cShortcut;
}

/** Tells the system whether or not this item can be selected.
 * \par Description:
 *  Tells the system whether or not this item can be selected.
 * \author Rick Caudill(syllable@syllable-desk.tk)
 *****************************************************************************/
 bool MenuItem::IsSelectable() const
 {
 	return m->m_bIsSelectable;
 }
 
 
/*****************************inherited members*****************************/

/** Draws the os::MenuItem
 * \par Description:
 * This method draws the MenuItem, make sure you call Flush() before calling this method.
 * \sa os::View::Draw()
 * \author Kurt Skauen with modifications by the Syllable Team.
 **************************************************************************/
void MenuItem::Draw()
{
	Menu *pcMenu = GetSuperMenu();

	if( pcMenu == NULL )
	{
		return;
	}

	const os::String& cLabel = GetLabel();  //the label of the menuitem
	os::Rect cFrame = GetFrame();  //the frame of the menuitem
	os::Rect cMFrame = pcMenu->GetBounds(); //the frame of the menu
	os::Image *pcImage = NULL;  //used if we have an image for this item
	os::Image *pcArrowImage = NULL; //used for the arrow image
	
	os::font_height sHeight; //the height of the menu
	pcMenu->GetFontHeight(&sHeight);
	

	//lets load the images
	pcImage = m->GetImage( IsEnabled() ? ( m->m_bIsHighlighted ? IMG_STATE_HIGHLIGHT : IMG_STATE_NORMAL ) : IMG_STATE_GRAY );
	pcArrowImage = pcMenu->_GetSubMenuArrow( IsEnabled() );
	
	/*highlighting part*/
	if( IsEnabled() )
	{
		if( m->m_bIsHighlighted )  //color goes to selected
		{
			pcMenu->SetFgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
		}
		else //color goes to regular background
		{
			pcMenu->SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
		}

		pcMenu->FillRect( GetFrame() ); //fill the menu with color

		//if highlighted, set colors for highlight
		if( m->m_bIsHighlighted )
		{
			pcMenu->SetFgColor( get_default_color( COL_SEL_MENU_TEXT ) );
			pcMenu->SetBgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
		}
		else //else set regular colors
		{
			pcMenu->SetFgColor( get_default_color( COL_MENU_TEXT ) );
			pcMenu->SetBgColor( get_default_color( COL_MENU_BACKGROUND ) );
		}
	}

	else //set normal colors
	{
		pcMenu->SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
		pcMenu->FillRect( GetFrame() );
		pcMenu->SetFgColor( 255, 255, 255 );
		pcMenu->SetBgColor( get_default_color( COL_MENU_BACKGROUND ) );
	}
	
	float x;

	if (m->m_pcSuperMenu->GetLayout() == ITEMS_IN_COLUMN) {
		x = ( GetFrame().left + m->m_pcSuperMenu->_GetColumnWidth(0) );
	} else {
		if( GetImage() ) {
			x = ( GetFrame().left + H_ROW_SPACE + ICON_TO_TEXT_SPACE + GetImage()->GetSize().x );
		} else {
			x = ( GetFrame().left + H_ROW_SPACE );
		}
	}

	Rect cTextRect( x, cFrame.top, x + m->m_pcSuperMenu->_GetColumnWidth(1), cFrame.bottom );

	
	if( m->m_cShortcut != "" )  //if we have a shortcut 
	{
		Rect cShortcutRect( cTextRect.right, cFrame.top, cFrame.right, cFrame.bottom );
	
		if( IsEnabled() == false )
		{
			pcMenu->SetFgColor( 100, 100, 100 );
			pcMenu->SetDrawingMode( DM_OVER );
			cShortcutRect.MoveTo( -1, -1 );
			pcMenu->SetDrawingMode( DM_COPY );
			pcMenu->DrawText( cShortcutRect, m->m_cShortcut, DTF_ALIGN_LEFT | DTF_ALIGN_MIDDLE | DTF_UNDERLINES );
		}	
		
		else //we don't have a shortcut
		{
			pcMenu->SetDrawingMode(DM_COPY);
			pcMenu->DrawText( cShortcutRect, m->m_cShortcut, DTF_ALIGN_LEFT | DTF_ALIGN_MIDDLE | DTF_UNDERLINES );
		}
	}


	//time to draw the text
	if (IsEnabled() == false)
	{
		pcMenu->SetFgColor( 100, 100, 100 );
		pcMenu->SetDrawingMode( DM_OVER );
		cTextRect.MoveTo( -1, -1 );
		pcMenu->DrawText( cTextRect, cLabel, DTF_ALIGN_LEFT | DTF_ALIGN_MIDDLE | DTF_UNDERLINES );
		pcMenu->SetDrawingMode( DM_COPY );
	}

	else
	{
		pcMenu->SetDrawingMode(DM_COPY);
		pcMenu->DrawText( cTextRect, cLabel, DTF_ALIGN_LEFT | DTF_ALIGN_MIDDLE | DTF_UNDERLINES );
	}

	if (pcImage) //if we have an image
	{
		pcMenu->SetDrawingMode(DM_BLEND);
		pcImage->Draw( Point( cFrame.left, cFrame.top + ( cFrame.Height() / 2 - pcImage->GetBounds().Height() / 2 ) + 1 ), pcMenu );
		pcMenu->SetDrawingMode(DM_COPY);
	}

	// If we are the super-menu, draw the sub-menu triangle on the right of the label
	if( GetSubMenu() != NULL && m->m_pcSuperMenu->GetLayout(  ) == ITEMS_IN_COLUMN )
	{
		float y;
		
		y = m->m_cFrame.top + ( m->m_cFrame.Height() / 2 ) - SUB_MENU_ARROW_W/2 ;
		
		pcMenu->SetDrawingMode( DM_BLEND );
		pcArrowImage->Draw( Point( m->m_cFrame.right - SUB_MENU_ARROW_W, y ), pcMenu );
		pcMenu->SetDrawingMode( DM_COPY );
	}
}

/***************************************************************************
 * NAME: Invoked()
 * DESC: Inherited from os::Invoker::Invoked().
 * NOTE: 
 * SEE ALSO: 
 **************************************************************************/
bool MenuItem::Invoked( Message * pcMessage )
{
	return ( true );
}

/***************************private members********************************/

/***************************************************************************
 * NAME: _SetNext()
 * DESC: Sets the next menuitem down the menu.
 * NOTE: Private
 * SEE ALSO: _GetNext()
 **************************************************************************/
void MenuItem::_SetNext( MenuItem* pcItem )
{
	m->m_pcNext = pcItem;
}

/***************************************************************************
 * NAME: _GetNext()
 * DESC: gets the next menuitem down the menu.
 * NOTE: Private
 * SEE ALSO: _SetNext()
 **************************************************************************/
MenuItem *MenuItem::_GetNext()
{
	return m->m_pcNext;
}

/** Sets the frame of the MenuItem...
 * \par Description:
 *	Sets the frame of the MenuItem.
 * \param cFrame - The os::Rect that will become the frame of this object.
 * \sa GetFrame()
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
void MenuItem::_SetFrame( const Rect & cFrame )
{
	m->m_cFrame = cFrame;
}

/** Sets the Super Menu (Menu that holds this MenuItem)...
 * \par Description:
 *	Sets the Super Menu(Menu that holds this MenuItem)...
 * \param pcMenu - The Menu that will become the Super Menu.
 * \sa GetSuperMenu()
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
void MenuItem::_SetSuperMenu( Menu * pcMenu )
{
	m->m_pcSuperMenu = pcMenu;
}

/** Sets the Sub Menu(Menu that is inside the MenuItem)...
 * \par Description:
 *	Sets the Sub Menu(Menu that is inside the MenuItem)...
 * \sa GetSubMenu()
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
void MenuItem::_SetSubMenu( Menu* pcMenu )
{
	m->m_pcSubMenu = pcMenu;
}

/************************end of os::MenuItem***********************************/


/************************beginning of os::MenuSeparator************************/


MenuSeparator::MenuSeparator():MenuItem("", NULL )
{
}

/**internal*/
MenuSeparator::~MenuSeparator()
{
}

/** Gets the size of the separator...
 * \par Description:
 *	Gets the size of the separator.
 * \return This method returns the size as an os::Point.
 * \sa
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
Point MenuSeparator::GetContentSize()
{
	Menu *pcMenu = GetSuperMenu();

	if( pcMenu != NULL )
	{
		font_height sHeight;

		pcMenu->GetFontHeight( &sHeight );

		if( pcMenu->GetLayout() == ITEMS_IN_ROW )
		{
			return ( Point( 6.0f, 0.0f ) );
		}
		else
		{
			return ( Point( 0.0f, 6.0f ) );
		}
	}
	return ( Point( 0.0f, 0.0f ) );
}
/******************************end specific members********************************/


/******************************inherited members***********************************/

/** Draws the os::MenuSeparator
 * \par Description:
 * This method draws the Separator, make sure you call Flush() before calling this method.
 * \sa os::View::Draw()
 * \author Kurt Skauen with modifications by the Syllable Team.
 **************************************************************************/
void MenuSeparator::Draw()
{
	Menu *pcMenu = GetSuperMenu();

	if( pcMenu == NULL )
	{
		return;
	}
	Rect cFrame = GetFrame();
	Rect cMFrame = pcMenu->GetBounds();

	if( pcMenu->GetLayout() == ITEMS_IN_ROW )
	{
		cFrame.top = cMFrame.top;
		cFrame.bottom = cMFrame.bottom;
	}
	else
	{
		cFrame.left = cMFrame.left;
		cFrame.right = cMFrame.right;
	}
	pcMenu->SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
	pcMenu->FillRect( cFrame );

	if( pcMenu->GetLayout() == ITEMS_IN_ROW )
	{
		float x = floor( cFrame.left + ( cFrame.Width() + 1.0f ) * 0.5f );

		pcMenu->SetFgColor( get_default_color( COL_SHADOW ) );
		pcMenu->DrawLine( Point( x, cFrame.top + 2.0f ), Point( x, cFrame.bottom - 2.0f ) );
		x += 1.0f;
		pcMenu->SetFgColor( get_default_color( COL_SHINE ) );
		pcMenu->DrawLine( Point( x, cFrame.top + 2.0f ), Point( x, cFrame.bottom - 2.0f ) );
	}
	else
	{
		float y = floor( cFrame.top + ( cFrame.Height() + 1.0f ) * 0.5f );

		pcMenu->SetFgColor( get_default_color( COL_SHADOW ) );
		pcMenu->DrawLine( Point( cFrame.left + 4.0f, y ), Point( cFrame.right - 4.0f, y ) );
		y += 1.0f;
		pcMenu->SetFgColor( get_default_color( COL_SHINE ) );
		pcMenu->DrawLine( Point( cFrame.left + 4.0f, y ), Point( cFrame.right - 4.0f, y ) );
	}

}

/***************************************************************************
 * NAME: DrawContent()
 * DESC: Doesn't do anything, but had to be inherited
 * NOTE: 
 * SEE ALSO:
 **************************************************************************/
void MenuSeparator::DrawContent()
{
}

/***************************************************************************
 * NAME: SetHighlighted()
 * DESC: Doesn't do anything, but had to be inherited
 * NOTE: 
 * SEE ALSO:
 **************************************************************************/
void MenuSeparator::SetHighlighted( bool bHighlighted )
{
}
/************************end of os::MenuSeparator*******************************/

/************************beginning of os::Menu**********************************/

/** Constructor.
 * \par Description:
 * 	The Menu constructor initializes the local object.
 * \param cFrame		-	The frame that holds the size of the Menu.
 * \param cTitle		-	The label that will be displayed.
 * \param eLayout		-	The layout of the Menu.  OS::ITEMS_IN_ROW for displaying items horizontally and OS::ITEMS_IN_COLUMN for displaying items vertically.
 * \param nResizeMask 	-   The parameters of how you want the Menu to resize when resizing the Application it is attached to.  Look at os::View for more documentatinon. 
 * \param nFlags		-	The view flags that will be passed to the Menu.
 * \par Example:
 * \code
 *	Menu* pcMenu = new Menu(Rect(),"main_menu",os::ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP ); //creates a menu and this menu would be attached to another menu(main menu)
 	Menu* pcFileMenu = new Menu(Rect(),"File",os::ITEMS_IN_COLUMN,CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP);
 	pcFileMenu->AddItem("Open",new Message());
 	pcMenu->AddItem(pcFileMenu); //add the file menu to the main_menu
 	
 	//this code will create a new frame for the menu so that the Menu will fit the Application better
 	Rect cMenuFrame;
 	cMenuFrame.left = 0;
	cMenuFrame.top = 0;
	cMenuFrame.right = GetBounds().Width();
	cMenuFrame.bottom = m_pcMenu->GetPreferredSize(true).y;
 	pcMenu->SetFrame(cMenuFrame);
 * \endcode
 *
 * \sa os::Menu, os::MenuSeparator
 * \author Kurt Skauen(with modifications by Rick Caudill).
  *****************************************************************************/
Menu::Menu( Rect cFrame, const String& cTitle, MenuLayout_e eLayout, uint32 nResizeMask, uint32 nFlags ):View( cFrame, "_menu_", nResizeMask, nFlags & ~WID_FULL_UPDATE_ON_RESIZE )
{
	m = new Private;
	
	m->m_cTitle = cTitle;
	m->m_pcRoot = this;
	m->m_eLayout = eLayout;
	m_pcSuperItem = NULL;
}

/**internal*/
Menu::~Menu()
{
	while( m->m_pcFirstItem != NULL )
	{
		MenuItem *pcItem = m->m_pcFirstItem;
		m->m_pcFirstItem = pcItem->_GetNext();
		delete pcItem;
	}
	delete m;
}

/** Locks the Menu...
 * \par Description:
 *	Locks the Menu and all of it items so that the user/programmer cannot modify them.  
 * \note If you call the method make sure you unlock it before you try and modify the Menu or any of the items in the Menu.
 * \sa Unlock
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
int Menu::Lock() const
{
	return ( m->m_pcRoot->m->m_cMutex.Lock() );
}

/** Unlocks the Menu...
 * \par Description:
 *	Unlocks the Menu.  Call this is you called Lock() so that you can modify the Menu again.
 * \sa Lock()
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
void Menu::Unlock() const
{
	m->m_pcRoot->m->m_cMutex.Unlock();
}

/** Gets the label of the Menu...
 * \par Description:
 *	Gets the label that the Menu displays on the Appliction.  
 * \return The label as an os::String
 * \sa
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
String Menu::GetLabel() const
{
	return ( m->m_cTitle );
}

/** Sets the label of the Menu...
 * \par Description:
 *	Sets the label that the Menu displays on the Appliction.  
 * \par cLabel - The Label.
 * \sa
 * \author	Arno Klenke
 *****************************************************************************/
void Menu::SetLabel( const os::String& cTitle )
{
	m->m_cTitle = cTitle;
}


/** Gets the layout of the Menu...
 * \par Description:
 *	Gets the layout of the Menu.
 * \return It will return it as a MenuLayout_e.
 * \sa
 * \author	Kurt Skauen(with modifications from the Syllable team)
 *****************************************************************************/
MenuLayout_e Menu::GetLayout() const
{
	return ( m->m_eLayout );
}

/** Opens the Menu at a specific position.
 * \par Description:
 *  Opens the Menu at a specific position.
 * \param cScrPos - The Point on the screen where you want the Menu to open 
 * \note This method is mainly used for when you want to add a context menu to you application.
 * \sa 
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
void Menu::Open( Point cScrPos )
{
	//now the menu will not open if there is no menuitems
	if (GetItemCount() == 0)
		return;
		
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );

	if( m->m_pcWindow != NULL )
	{
		return;
	}

	InvalidateLayout();

	Point cSize = GetPreferredSize( false );
	Rect cBounds( 0, 0, cSize.x - 1, cSize.y - 1 );

	Desktop cDesktop;
	IPoint cRes = cDesktop.GetResolution();

	if( cBounds.bottom + cScrPos.y > cRes.y )
		cScrPos.y = cRes.y - cSize.y;
	if( cBounds.top + cScrPos.y < 0 )
		cScrPos.y = 0;
	if( cBounds.right + cScrPos.x > cRes.x )
		cScrPos.x = cRes.x - cSize.x;
	if( cBounds.left + cScrPos.x < 0 )
		cScrPos.x = 0;

	m->m_bIsRootMenu = false;	// To avoid adding shortcuts to the MenuWindow.
								// (It doesn't hurt if we do, but it's a waste of time)
	m->m_pcWindow = new MenuWindow( cBounds + cScrPos, this );
	m->m_pcRoot->m->m_cMutex.Lock();
	m->m_pcWindow->SetMutex( &m->m_pcRoot->m->m_cMutex );
	SetFrame( cBounds );

	m->m_bIsTracking = true;
	MakeFocus( true );

	_SelectItem( m->m_pcFirstItem );
	m->m_pcWindow->Show();

	Flush();
}


/** Adds a MenuItem.
 * \par Description:
 *  Adds a MenuItem to the Menu.
 * \param cLabel - the label of the MenuItem.
 * \param pcMessage - the Message of that is passed when the MenuItem is clicked.
 * \param cShortcut - Keyboard shortcut.
 * \param pcImage - pointer to Image to use as icon. The image is deleted automatically
 *					by the Menu destructor!
 * \return It returns true if MenuItem was added and false if it could not add the MenuItem.
 * \sa 
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
bool Menu::AddItem( const String& cLabel, Message* pcMessage, const String& cShortcut, Image* pcImage )
{
	return ( AddItem( new MenuItem( cLabel, pcMessage, cShortcut, pcImage ) ) );
}

/** Adds a MenuItem.
 * \par Description:
 *  Adds a MenuItem to the Menu.
 * \param pcItem - the MenuItem to add
 * \return It returns true if MenuItem was added and false if it could not add the MenuItem.
 * \sa AddItem(MenuItem* pcItem, int nIndex)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
bool Menu::AddItem( MenuItem* pcItem )
{
	return ( AddItem( pcItem, m->m_nItemCount ) );
}

/** Adds a MenuItem.
 * \par Description:
 *  Adds a MenuItem to the Menu.
 * \param pcItem - the MenuItem to add
 * \param nIndex - the place where the MenuItem will be put.
 * \return It returns true if MenuItem was added and false if it could not add the MenuItem.
 * \sa AddItem(MenuItem* pcItem)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
bool Menu::AddItem( MenuItem* pcItem, int nIndex )
{
	bool bResult = false;

	pcItem->_SetSuperMenu( this );

	if( nIndex == 0 )
	{
		pcItem->_SetNext( m->m_pcFirstItem );
		m->m_pcFirstItem = pcItem;
		bResult = true;
	}
	else
	{
		MenuItem *pcPrev;
		int i;

		for( i = 1, pcPrev = m->m_pcFirstItem; i < nIndex && NULL != pcPrev; ++i )
		{
			pcPrev = pcPrev->_GetNext();
		}
		if( pcPrev )
		{
			pcItem->_SetNext( pcPrev->_GetNext() );
			pcPrev->_SetNext( pcItem );
			bResult = true;
		}
	}

	if( bResult )
	{
		if( pcItem->GetSubMenu() != NULL )
		{
			pcItem->GetSubMenu()->_SetRoot( m->m_pcRoot );
		}
		m->m_nItemCount++;
		InvalidateLayout();
	}
	return ( bResult );
}

/** Adds a MenuItem.
 * \par Description:
 *  Adds a MenuItem with the passing Menu* as its SubMenu.
 * \param pcMenu - the SubMenu that will be added to the Menu.
 * \return It returns true if MenuItem was added and false if it could not add the MenuItem.
 * \sa AddItem(Menu* pcMenu, int nIndex)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
bool Menu::AddItem( Menu* pcMenu )
{
	pcMenu->_SetRoot( m->m_pcRoot );
	return ( AddItem( pcMenu, m->m_nItemCount ) );
}

/** Adds a MenuItem.
 * \par Description:
 *  Adds a MenuItem with the passing Menu* as its SubMenu.
 * \param pcMenu - the SubMenu that will be added to the Menu.
 * \param nIndex - the place where the MenuItem will be put.
 * \return It returns true if MenuItem was added and false if it could not add the MenuItem.
 * \sa AddItem(Menu* pcMenu)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
bool Menu::AddItem( Menu* pcMenu, int nIndex )
{
	pcMenu->_SetRoot( m->m_pcRoot );
	return ( AddItem( new MenuItem( pcMenu, NULL ), nIndex ) );
}

/** Removes an MenuItem.
 * \par Description:
 *  Removes an MenuItem that is at the passed index.
 * \param nIndex - the index number of the MenuItem you wish to remove.
 * \return It returns true if the MenuItem was found and removed and false if the MenuItem was not found and not removed.
 * \sa RemoveItem(MenuItem* pcItem). RemoveItem(Menu* pcMenu)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
MenuItem *Menu::RemoveItem( int nIndex )
{
	MenuItem *pcTmp;
	MenuItem *pcPrev = NULL;
	MenuItem *pcItem = NULL;

	for( pcTmp = m->m_pcFirstItem; pcTmp != NULL; pcPrev = pcTmp, pcTmp = pcTmp->_GetNext() )
	{
		if( nIndex == 0 )
		{
			pcItem = pcTmp;
			pcTmp = pcItem->_GetNext();

			if( pcPrev )
				pcPrev->_SetNext( pcTmp );
			else
				m->m_pcFirstItem = pcTmp;

			if( pcItem->GetSubMenu() != NULL )
			{
				pcItem->GetSubMenu()->_SetRoot( pcItem->GetSubMenu(  ) );
			}

			m->m_nItemCount--;
			InvalidateLayout();
			break;
		}
		nIndex--;
	}
	return ( pcItem );
}

/** Removes an MenuItem.
 * \par Description:
 *  Removes the MenuItem that is passed to the method.
 * \param pcItem - the MenuItem to remove.
 * \return It returns true if the MenuItem was found and removed and false if the MenuItem was not found and not removed.
 * \sa RemoveItem(Menu* pcMenu). RemoveItem(int nIndex)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
bool Menu::RemoveItem( MenuItem* pcItem )
{
	MenuItem *pcTmp;
	MenuItem *pcPrev = NULL;

	for( pcTmp = m->m_pcFirstItem; pcTmp != NULL; pcPrev = pcTmp, pcTmp = pcTmp->_GetNext() )
	{
		if( pcItem == pcTmp )
		{
			pcItem = pcTmp;
			pcTmp = pcItem->_GetNext();

			if( pcPrev )
				pcPrev->_SetNext( pcTmp );
			else
				m->m_pcFirstItem = pcTmp;

			if( pcItem->GetSubMenu() != NULL )
			{
				pcItem->GetSubMenu()->_SetRoot( pcItem->GetSubMenu(  ) );
			}

			delete pcItem;

			m->m_nItemCount--;
			InvalidateLayout();
			return true;
		}
	}
	return false;
}

/** Removes an MenuItem.
 * \par Description:
 *  Removes an MenuItem with the passing Menu* as its SubMenu.
 * \param pcMenu - the SubMenu that will be used to remove a certain item.
 * \return It returns true if the MenuItem was found and removed and false if the MenuItem was not found and not removed.
 * \sa RemoveItem(MenuItem* pcItem). RemoveItem(int nIndex)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
bool Menu::RemoveItem( Menu* pcMenu )
{
	MenuItem *pcTmp;
	MenuItem *pcPrev = NULL;
	MenuItem *pcItem = NULL;

	for( pcTmp = m->m_pcFirstItem; pcTmp != NULL; pcPrev = pcTmp, pcTmp = pcTmp->_GetNext() )
	{
		if( pcTmp->GetSubMenu() == pcMenu )
		{
			pcItem = pcTmp;
			pcTmp = pcItem->_GetNext();

			if( pcPrev )
				pcPrev->_SetNext( pcTmp );
			else
				m->m_pcFirstItem = pcTmp;

			pcMenu->_SetRoot( pcMenu );

			delete pcItem;

			m->m_nItemCount--;
			InvalidateLayout();
			return true;
		}
	}
	return false;
}

/** Gets the MenuItem at a certain point.
 * \par Description:
 *	Gets the MenuItem at a certain point.
 * \param cPosition - The position that you want get the MenuItem from
 * \return The MenuItem that is at the certain point.  If there is no MenuItem at that Point then it will return a Null MenuItem.
 * \sa GetItemAt(int nIndex)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
MenuItem *Menu::GetItemAt( Point cPosition ) const
{
	MenuItem *pcItem=NULL;

	for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
	{
		if( pcItem->GetFrame().DoIntersect( cPosition ) )
		{
			break;
		}
	}
	return ( pcItem );
}

/** Gets the MenuItem at a certain index number.
 * \par Description:
 *	Gets the MenuItem at a certain index number.
 * \param nIndex - The index number you want to get the MenuItem from.
 * \return The MenuItem that is at the certain index number.  If there is no MenuItem at that number then it will return a Null MenuItem.
 * \sa GetItemAt(Point cPosition)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
MenuItem *Menu::GetItemAt( int nIndex ) const
{
	MenuItem *pcItem = NULL;

	if( nIndex >= 0 && nIndex < m->m_nItemCount )
	{
		for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
		{
			if( 0 == nIndex )
			{
				return ( pcItem );
			}
			nIndex--;
		}
	}
	return ( NULL );
}

/** Gets the SubMenu determined by the passing index number.
 * \par Description:
 *	Gets the SubMenu that is at the index(how many MenuItems down) number
 * \param nIndex - The index number you want to get the SubMenu from
 * \return The SubMenu.  If there is no SubMenu, it will return NULL
 * \sa
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
Menu *Menu::GetSubMenuAt( int nIndex ) const
{
	MenuItem *pcItem = GetItemAt( nIndex );

	if( NULL != pcItem )
	{
		return ( pcItem->GetSubMenu() );
	}
	else
	{
		return ( NULL );
	}
}

/** Returns the number of items in the Menu.
 * \par Description:
 *	Returns the number of items in the Menu.
 * \return An interger giving the number of MenuItems there are attached.
 * \sa
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
int Menu::GetItemCount() const
{
	return ( m->m_nItemCount );
}

/** Returns the number of the MenuItem's position.
 * \par Description:
 *	Returns the number of the MenuItems's position
 * \param pcItem - The MenuItem that you w you want to get the index of.
 * \return An interger giving the number of places down the line that this MenuItem is attached.
 * \sa
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
int Menu::GetIndexOf(MenuItem* pcItem ) const
{
	MenuItem *pcPtr;
	int nIndex = 0;

	for( pcPtr = m->m_pcFirstItem; NULL != pcPtr; pcPtr = pcPtr->_GetNext() )
	{
		if( pcPtr == pcItem )
		{
			return ( nIndex );
		}
		nIndex++;
	}
	return ( -1 );
}

/** Returns the number of the Menu's position.
 * \par Description:
 *	Returns the number of the Menu's position
 * \param pcMenu - The Menu you want to get the index of.
 * \return An interger giving the number of places down the line that this menu is attached.
 * \sa
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
int Menu::GetIndexOf( Menu* pcMenu ) const
{
	MenuItem *pcPtr;
	int nIndex = 0;

	for( pcPtr = m->m_pcFirstItem; NULL != pcPtr; pcPtr = pcPtr->_GetNext() )
	{
		if( pcPtr->GetSubMenu() == pcMenu )
		{
			return ( nIndex );
		}
		nIndex++;
	}
	return ( -1 );
}

/** Method that finds an item by a certain Message code.
 * \par Description:
 *	This method will find a MenuItem by a certain Message code.
 * \param nCode - Code of the MenuItem you wish to find.
 * \sa FindItem(const String& cName)
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
MenuItem *Menu::FindItem( int nCode ) const
{
	MenuItem *pcItem = NULL;

	for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
	{
		if( pcItem->GetSubMenu() != NULL )
		{
			MenuItem *pcTmp = pcItem->GetSubMenu()->FindItem( nCode );

			if( pcTmp != NULL )
			{
				return ( pcTmp );
			}
		}
		else
		{
			Message *pcMsg = pcItem->GetMessage();

			if( pcMsg != NULL && pcMsg->GetCode() == nCode )
			{
				return ( pcItem );
			}
		}
	}
	return ( NULL );
}

/** Method that finds an item by a certain name.
 * \par Description:
 *	This method will find a MenuItem by a certain name.
 * \param cName - Name of the MenuItem you wish to find.
 * \sa
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
MenuItem *Menu::FindItem( const String& cName ) const
{
	MenuItem *pcItem;

	for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
	{
		if( pcItem->GetSubMenu() != NULL )
		{
			MenuItem *pcTmp = pcItem->GetSubMenu()->FindItem( cName );

			if( pcTmp != NULL )
			{
				return ( pcTmp );
			}
		}
		else
		{
			if( pcItem->GetLabel() == cName )
			{
				break;
			}
		}
	}
	return ( pcItem );
}

/** Sets the object that the MenuItems will send messages to.
 * \par Description:
 *	This method will set the place of where your MenuItems will pass there message to.
 * \sa
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
status_t Menu::SetTargetForItems( Handler* pcTarget )
{
	MenuItem *pcItem;
	for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
	{
		if( NULL == pcItem->GetSubMenu() )
		{
			pcItem->SetTarget( pcTarget );
		}
		else
		{
			pcItem->GetSubMenu()->SetTargetForItems( pcTarget );
		}
	}
	return ( 0 );
}

/** Sets the object that the MenuItems will send messages to.
 * \par Description:
 *	This method will set the place of where your MenuItems will pass there message to.
 * \sa
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
status_t Menu::SetTargetForItems( Messenger cMessenger )
{
	MenuItem *pcItem;
	for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
	{
		if( NULL == pcItem->GetSubMenu() )
		{
			pcItem->SetTarget( cMessenger );
		}
		else
		{
			pcItem->GetSubMenu()->SetTargetForItems( cMessenger );
		}
	}
	return ( 0 );
}

/** Sets the message.
 * \par Description:
 *	This method will set the message that will be passed to the target
 * \sa SetCloseMsgTarget()
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
void Menu::SetCloseMessage( const Message& cMsg )
{
	m->m_cCloseMsg = cMsg;
}

/** Sets Target for Menu.
 * \par Description:
 *	This method will set target of where the message will be pushed to when you close the Menu.
 * \sa SetCloseMessage()
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
void Menu::SetCloseMsgTarget(const Messenger& cTarget )
{
	m->m_cCloseMsgTarget = cTarget;
}


/** Gets the MenuItem that is Hightlighted.
 * \par Description:
 *	This method will give you the MenuItem is highlighted.
 * \return This will return NULL if there is no item highlighted and if there is item then it will return that MenuItem.
 * \sa FindItem()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
MenuItem *Menu::FindMarked() const
{
	MenuItem *pcItem;

	for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
	{
		if( pcItem->IsHighlighted() )
		{
			break;
		}
	}
	return ( pcItem );
}

/** Gets the Menu that the Menu is attached to.
 * \par Description:
 *	This method will give you the Menu that the Menu is attached to.
 * \return This will return NULL if there is not any Super Menu and if there is an Super Menu then it will return that Menu.
 * \sa GetSuperItem()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
Menu *Menu::GetSuperMenu() const
{
	if( NULL != m_pcSuperItem )
	{
		return ( m_pcSuperItem->GetSuperMenu() );
	}
	else
	{
		return ( NULL );
	}
}

/** Gets the MenuItem that the Menu is attached to.
 * \par Description:
 *	This method will give you the MenuItem that the Menu is attached to.
 * \return This will return NULL if there is not any Super MenuItem and if there is an Super MenuItem then it will return that MenuItem.
 * \sa GetSuperMenu()
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
MenuItem *Menu::GetSuperItem()
{
	return (m_pcSuperItem );
}

/** Returns the exact size of the Menu
 * \par Description:
 * This method returns the exact size of the menu
 * \return Based on bLargest.  If bLargest is true then it will return the maximum size of the Menu, otherwise it will return the size of the Menu plus two more pixels high and two more pixels wide.
 * \author Kurt Skauen with modifications by the Syllable team.
 **************************************************************************/
Point Menu::GetPreferredSize( bool bLargest ) const
{
	if( !bLargest ) {
		return ( m->m_cSize + Point( 2.0f, 1.0f ) );
	} else {
		return Point( COORD_MAX, m->m_cSize.y + 1.0f );
	}
}

/** Refreshes the layout of the Menu...
 * \par Description:
 *	This method will reset the layout of the Menu.
 * \sa
 * \author	Kurt Skauen with modifications by the Syllable team.
 *****************************************************************************/
void Menu::InvalidateLayout()
{
	MenuItem *pcItem;

	m->m_cSize = Point( 0.0f, 1.0f );
	m->m_vColumnWidths[ 0 ] = 0.0f;
	m->m_vColumnWidths[ 1 ] = 0.0f;
	m->m_vColumnWidths[ 2 ] = 0.0f;

	switch ( m->m_eLayout )
	{
		case ITEMS_IN_ROW:
			for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
			{
				Point cSize = pcItem->GetContentSize();
	
				pcItem->_SetFrame( Rect( m->m_cSize.x + m->m_cItemBorders.left, m->m_cItemBorders.top, m->m_cSize.x + cSize.x + m->m_cItemBorders.left, cSize.y + m->m_cItemBorders.bottom ) );
	
				m->m_cSize.x += cSize.x + m->m_cItemBorders.left + m->m_cItemBorders.right;
	
				if( cSize.y > m->m_cSize.y )
					m->m_cSize.y = cSize.y;
			}
			m->m_cSize.y += m->m_cItemBorders.top + m->m_cItemBorders.bottom - 1.0f;
	
			break;
	
		case ITEMS_IN_COLUMN:
			for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
			{
				for( int i = 0; i < pcItem->GetNumColumns(); i++ )
				{

					m->m_vColumnWidths[ i ] = std::max( pcItem->GetColumnWidth(i), m->m_vColumnWidths[ i ] );
				}
			}

			for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
			{
				Point cSize = pcItem->GetContentSize();
	
				pcItem->_SetFrame( Rect( m->m_cItemBorders.left, m->m_cSize.y + m->m_cItemBorders.top, cSize.x + m->m_cItemBorders.right - 1, m->m_cSize.y + cSize.y + m->m_cItemBorders.top - 1 ) );
	
				m->m_cSize.y += cSize.y + m->m_cItemBorders.top + m->m_cItemBorders.bottom;
	
				if( cSize.x + m->m_cItemBorders.left + m->m_cItemBorders.right > m->m_cSize.x )
					m->m_cSize.x = cSize.x + m->m_cItemBorders.left + m->m_cItemBorders.right;
			}
	
			for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
			{
				Rect frame = pcItem->GetFrame();
	
				frame.right = m->m_cSize.x - 2;
				pcItem->_SetFrame( frame );
			}
			break;
	
		case ITEMS_CUSTOM_LAYOUT:
			dbprintf( "Error: ITEMS_CUSTOM_LAYOUT not supported\n" );
			break;
	}
}

/** Go to a certain MenuItem by screen position...
 * \par Description:
 *	This method will open a menu by it's position.
 * \param cScreenPos - The screen position.
 * \note   This method is not to be used outside this class and it will probably freeze the system if used.
 * \sa
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
MenuItem *Menu::Track( const Point & cScreenPos )
{
	m->m_hTrackPort = create_port( "menu_track_port", 1 );
	if( m->m_hTrackPort < 0 )
	{
		return ( NULL );
	}
	Point cSize = GetPreferredSize( false );
	Rect cBounds( 0, 0, cSize.x - 1, cSize.y - 1 );

	m->m_pcWindow = new MenuWindow( cBounds + cScreenPos, this );
	m->m_pcRoot->m->m_cMutex.Lock();
	m->m_pcWindow->SetMutex( &m->m_pcRoot->m->m_cMutex );

	SetFrame( cBounds );

	m->m_pcWindow->Show();
	MakeFocus( true );

	m->m_bIsTracking = true;

	uint32 nCode;
	MenuItem *pcSelItem;

	get_msg( m->m_hTrackPort, &nCode, &pcSelItem, sizeof( &pcSelItem ) );

	assert( m->m_pcWindow == NULL );
	delete_port( m->m_hTrackPort );
	m->m_hTrackPort = -1;
	return ( pcSelItem );
}

/** Tells the system to disable or enable this element...
 * \par Description:
 *	This method will tell the system to disable or enable this element.
 * \param bEnabled - To enable this element set this to true(default) and to disable set this to false.
 * \sa IsEnabled()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
void Menu::SetEnable( bool bEnabled )
{
	Menu* pcSubMenu;
	MenuItem* pcSubItem;
	m->m_bEnabled = bEnabled;

	for( int i = 0; i < GetItemCount(); i++ )
	{
		pcSubItem = GetItemAt(i);
		
		/*added submenu enabling*/
		if (pcSubItem->GetSubMenu())
		{
			pcSubMenu = pcSubItem->GetSubMenu();
			pcSubMenu->SetEnable(bEnabled);
		}
		GetItemAt( i )->SetEnable( bEnabled );
	}
	Flush();
}

/** Tells the programmer whether this element is enabled or disabled...
 * \par Description:
 *	This method will tell programmer whether or not this element is enabled.
 * \return This method returns true if this Menu is enabled and false if it is not.
 * \sa SetEnabled()
 * \author	Rick Caudill (syllable_desktop@hotpop.com)
 *****************************************************************************/
bool Menu::IsEnabled()
{
	return m->m_bEnabled;
}

/**************************inherited menbers*********************************/

/***************************************************************************
 * NAME: AttachedToWindow()
 * DESC: Inherited from os::View 
 * NOTE: Look at os::View documentation for this function
 * SEE ALSO:
 **************************************************************************/
void Menu::AttachedToWindow()
{
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );
	InvalidateLayout();
	if( m->m_bIsRootMenu ) {
		_SetOrClearShortcut( m->m_pcFirstItem, true );
	}
}

/***************************************************************************
 * NAME: TimerTick()
 * DESC: Inherited/Reimplemented from os::Handler
 * NOTE: Look at os::Handler documentation
 * SEE ALSO:
 **************************************************************************/
void Menu::TimerTick( int nID )
{
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );
	_OpenSelection();
}

/***************************************************************************
 * NAME: DetachedFromWindow()
 * DESC: Inherited from os::View 
 * NOTE: Look at os::View documentation for this function
 * SEE ALSO:
 **************************************************************************/
void Menu::DetachedFromWindow()
{
	
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );
	_Close( true, false );
	if( m->m_bIsRootMenu ) {
		_SetOrClearShortcut( m->m_pcFirstItem, false );
	}
}

/***************************************************************************
 * NAME: WindowActivated()
 * DESC: Inherited from os::View 
 * NOTE: Look at os::View documentation for this function
 * SEE ALSO:
 **************************************************************************/
void Menu::WindowActivated( bool bIsActive )
{
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );

	if( false == bIsActive && m->m_bHasOpenChilds == false )
	{
		_Close( false, true );

		if( m->m_hTrackPort != -1 )
		{
			MenuItem *pcItem = NULL;

			if( send_msg( m->m_hTrackPort, 1, &pcItem, sizeof( pcItem ) ) < 0 )
			{
				dbprintf( "Error: Menu::WindowActivated() failed to send message to m_hTrackPort\n" );
			}
		}
	}
}

/***************************************************************************
 * NAME: MouseDown()
 * DESC: Inherited from os::View 
 * NOTE: Look at os::View documentation for this function
 * SEE ALSO:
 **************************************************************************/
void Menu::MouseDown( const Point & cPosition, uint32 nButtons )
{
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );

	m->m_cMouseHitPos = cPosition;

	if( GetBounds().DoIntersect( cPosition ) )
	{
		MenuItem *pcItem = GetItemAt( cPosition );

		_SetCloseOnMouseUp( false );

		if( pcItem != NULL )
		{
			MakeFocus( true );
			m->m_bIsTracking = true;
			_SelectItem( pcItem );

			if( m->m_eLayout == ITEMS_IN_ROW )
			{
				_OpenSelection();
			}
			else
			{
				_StartOpenTimer( 200000 );
			}
			Flush();
		}
		_OpenSelection();
	}
	else
	{
		if( m->m_bHasOpenChilds == false )
		{
			Menu *pcParent = GetSuperMenu();
			os::Point cParentPos;
			if( pcParent != NULL )
				cParentPos = pcParent->ConvertFromScreen( ConvertToScreen( cPosition ) );
			
			_Close( false, false );
			
			pcParent = GetSuperMenu();
			if( pcParent != NULL )
				pcParent->MouseDown( cParentPos, nButtons );

			if( m->m_hTrackPort != -1 )
			{
				MenuItem *pcItem = NULL;

				if( send_msg( m->m_hTrackPort, 1, &pcItem, sizeof( pcItem ) ) < 0 )
				{
					dbprintf( "Error: Menu::MouseDown() failed to send message to m_hTrackPort\n" );
				}
			}
		}
	}
}

/***************************************************************************
 * NAME: MouseUp()
 * DESC: Inherited from os::View 
 * NOTE: Look at os::View documentation for this function
 * SEE ALSO:
 **************************************************************************/
void Menu::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );

	if( GetBounds().DoIntersect( cPosition ) )
	{
		MenuItem *pcItem = FindMarked();

		if( NULL != pcItem && pcItem == GetItemAt( cPosition ) && NULL == pcItem->GetSubMenu() )
		{
			if( pcItem->IsEnabled() )	//if enabled it will invoke the message and then close
			{	//else it will not do anything
				pcItem->Invoke();
				_Close( false, true );
			}
		}

	}

	else
	{
		if( m->m_bCloseOnMouseUp )
		{
			_Close( true, true );
			if( m->m_hTrackPort != -1 )
			{
				MenuItem *pcItem = NULL;

				if( send_msg( m->m_hTrackPort, 1, &pcItem, sizeof( pcItem ) ) < 0 )
				{
					dbprintf( "Error: Menu::WindowActivated() failed to send message to m_hTrackPort\n" );
				}
			}
		}
	}
	_SetCloseOnMouseUp( false );
}

/***************************************************************************
 * NAME: MouseMove()
 * DESC: Inherited from os::View 
 * NOTE: Look at os::View documentation for this function
 * SEE ALSO:
 **************************************************************************/
void Menu::MouseMove( const Point & cPosition, int nCode, uint32 nButtons, Message * pcData )
{
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );
	
	if( nButtons & 0x01 )
	{
		if( !m->m_bCloseOnMouseUp ) {
			Point cDistance;
			cDistance.x = cPosition.x - m->m_cMouseHitPos.x;
			cDistance.y = cPosition.y - m->m_cMouseHitPos.y;
			float vDist = sqrt( cDistance.x * cDistance.x + cDistance.y * cDistance.y );
			if( vDist > STICKY_THRESHOLD ) {
				_SetCloseOnMouseUp( true );
			}
		}
	}
	if( GetBounds().DoIntersect( cPosition ) == false )
	{
		Menu *pcParent = GetSuperMenu();

		if( pcParent != NULL )
		{
			pcParent->MouseMove( pcParent->ConvertFromScreen( ConvertToScreen( cPosition ) ), nCode, nButtons, pcData );
		}
	}
	else
	{
		if( m->m_bIsTracking )
		{
			MenuItem *pcItem = GetItemAt( cPosition );

			if( pcItem != NULL )
			{
				_SelectItem( pcItem );
				if( m->m_eLayout == ITEMS_IN_ROW )
				{
					_OpenSelection();
				}
				else
				{
					_StartOpenTimer( 200000 );
				}
				Flush();
			}
		}
	}
}

/***************************************************************************
 * NAME: KeyDown()
 * DESC: Inherited from os::View 
 * NOTE: Look at os::View documentation for this function
 * SEE ALSO:
 **************************************************************************/
void Menu::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );

	switch ( pzString[0] )
	{
		case VK_UP_ARROW:
			if( m->m_eLayout == ITEMS_IN_ROW )
			{
			}
			else
			{
				_SelectPrev();
				_StartOpenTimer( 1000000 );
			}
			break;
		case VK_DOWN_ARROW:
			if( m->m_eLayout == ITEMS_IN_ROW )
			{
				_OpenSelection();
			}
			else
			{
				_SelectNext();
				_StartOpenTimer( 1000000 );
			}
			break;
		case VK_LEFT_ARROW:
			if( m->m_eLayout == ITEMS_IN_ROW )
			{
				_SelectPrev();
				_OpenSelection();
			}
			else
			{
				Menu *pcSuper = GetSuperMenu();
	
				if( pcSuper != NULL )
				{
					if( pcSuper->m->m_eLayout == ITEMS_IN_COLUMN )
					{
						_Close( false, false );
					}
					else
					{
						pcSuper->_SelectPrev();
						pcSuper->_OpenSelection();
					}
				}
			}
			break;
		case VK_RIGHT_ARROW:
			if( m->m_eLayout == ITEMS_IN_ROW )
			{
				_SelectNext();
				_OpenSelection();
			}
			else
			{
				Menu *pcSuper = GetSuperMenu();
	
				if( pcSuper != NULL )
				{
					MenuItem *pcItem = FindMarked();
	
					if( pcItem != NULL && pcItem->GetSubMenu() != NULL )
					{
						_OpenSelection();
					}
					else if( pcSuper->m->m_eLayout == ITEMS_IN_ROW )
					{
						pcSuper->_SelectNext();
						pcSuper->_OpenSelection();
					}
				}
			}
			break;
		case VK_ENTER:
			{
				MenuItem *pcItem = FindMarked();
	
				if( pcItem != NULL )
				{
					if( pcItem->GetSubMenu() != NULL )
					{
						_OpenSelection();
					}
					else
					{
						pcItem->Invoke();
						_Close( false, true );
					}
				}
				break;
			}
		case 27:		// ESC
			_Close( false, false );
			Flush();
			break;
		default:
			View::KeyDown( pzString, pzRawString, nQualifiers );
	}
}

/***************************************************************************
 * NAME: Paint()
 * DESC: Inherited from os::View 
 * NOTE: Look at os::View documentation for this function
 * SEE ALSO:
 **************************************************************************/
void Menu::Paint( const Rect & cUpdateRect )
{
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );
	MenuItem *pcItem;

	SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
	FillRect( GetBounds() );


	for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
	{
		//pcItem->SetEnable(IsEnabled());
		pcItem->Draw();
	}	
	DrawFrame( GetBounds(), FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
}

/***************************************************************************
 * NAME: FrameSized()
 * DESC: Inherited/Reimplemented from os::View 
 * NOTE: Look at os::View documentation for this function
 * SEE ALSO:
 **************************************************************************/
void Menu::FrameSized( const Point & cDelta )
{
	if( cDelta.x > 0.0f )
	{
		Rect cBounds = GetBounds();

		cBounds.left = cBounds.right - cDelta.x;
		Invalidate( cBounds );
		Flush();
	}
	else if( cDelta.x < 0.0f )
	{
		Rect cBounds = GetBounds();

		cBounds.left = cBounds.right - 2.0f;
		Invalidate( cBounds );
		Flush();
	}
}
/*******************************private members*****************************/

/***************************************************************************
 * NAME: _EndSession()
 * DESC: When a menuitem is clicked this function gets called to send information to the window
 * NOTE: Private
 * SEE ALSO:
 **************************************************************************/
void Menu::_EndSession( MenuItem* pcSelItem )
{
	Menu *pcSuper = GetSuperMenu();

	if( NULL != pcSuper )
	{
		Window *pcWindow = pcSuper->GetWindow();

		if( NULL != pcWindow )
		{
			Message cMsg( M_MENU_EVENT );

			cMsg.AddPointer( "_item", pcSelItem /* GetSuperItem() */  );

			pcWindow->PostMessage( &cMsg, pcSuper );
		}
	}
	if( m->m_hTrackPort != -1 )
	{
		if( send_msg( m->m_hTrackPort, 1, &pcSelItem, sizeof( &pcSelItem ) ) < 0 )
		{
			dbprintf( "Error: Menu::EndSession() failed to send message to m_hTrackPort\n" );
		}
	}
}

/***************************************************************************
 * NAME: _SetCloseOnMouseUp()
 * DESC: Called from MouseUp().
 * NOTE: Private
 * SEE ALSO:
 **************************************************************************/
void Menu::_SetCloseOnMouseUp( bool bFlag )
{
	MenuItem *pcItem;

	m->m_bCloseOnMouseUp = bFlag;

	for( pcItem = m->m_pcFirstItem; NULL != pcItem; pcItem = pcItem->_GetNext() )
	{
		if( pcItem->GetSubMenu() != NULL )
		{
			pcItem->GetSubMenu()->_SetCloseOnMouseUp( bFlag );
		}
	}
}

/***************************************************************************
 * NAME: _SetRoot()
 * DESC: Sets the root Menu fot this menu
 * NOTE: Private
 * SEE ALSO:
 **************************************************************************/
void Menu::_SetRoot( Menu* pcRoot )
{
	m->m_pcRoot = pcRoot;

	MenuItem *pcTmp;

	for( pcTmp = m->m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->_GetNext() )
	{
		if( pcTmp->GetSubMenu() != NULL )
		{
			pcTmp->GetSubMenu()->_SetRoot( pcRoot );
		}
	}
}

/***************************************************************************
 * NAME: _StartOpenTimer()
 * DESC: Starts the timer if set by TimerTick()
 * NOTE: Private
 * SEE ALSO: _TimerTick()
 **************************************************************************/
void Menu::_StartOpenTimer( bigtime_t nDelay )
{
	Looper *pcLooper = GetLooper();

	if( pcLooper != NULL )
	{
		pcLooper->Lock();
		pcLooper->AddTimer( this, 123, nDelay, true );
		pcLooper->Unlock();
	}
}

/***************************************************************************
 * NAME: _OpenSelection()
 * DESC: Opens selected item
 * NOTE: Private
 * SEE ALSO:
 **************************************************************************/
void Menu::_OpenSelection()
{
	MenuItem *pcItem = FindMarked();

	if( pcItem == NULL )
	{
		return;
	}

	MenuItem *pcTmp;

	for( pcTmp = m->m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->_GetNext() )
	{
		if( pcTmp == pcItem )
		{
			continue;
		}
		if( pcTmp->GetSubMenu() != NULL && pcTmp->GetSubMenu(  )->m->m_pcWindow != NULL )
		{
			pcTmp->GetSubMenu()->_Close( true, false );
			break;
		}
	}

	if( pcItem->GetSubMenu() == NULL || pcItem->GetSubMenu(  )->m->m_pcWindow != NULL )
	{
		return;
	}

	Rect cFrame = pcItem->GetFrame();
	Point cScrPos;

	Point cScreenRes( Desktop().GetResolution(  ) );

	Point cLeftTop( 0, 0 );

	ConvertToScreen( &cLeftTop );

	if( m->m_eLayout == ITEMS_IN_ROW )
	{
		cScrPos.x = cLeftTop.x + cFrame.left - m->m_cItemBorders.left;

		float nHeight = pcItem->GetSubMenu()->m->m_cSize.y;

		if( cLeftTop.y + GetBounds().Height(  ) + 1.0f + nHeight <= cScreenRes.y )
		{
			cScrPos.y = cLeftTop.y + GetBounds().Height(  ) + 1.0f;
		}
		else
		{
			cScrPos.y = cLeftTop.y - nHeight;
		}
	}
	else
	{
		float nHeight = pcItem->GetSubMenu()->m->m_cSize.y;

		cScrPos.x = cLeftTop.x + GetBounds().Width(  ) + 1.0f - 4.0f;
		if( cLeftTop.y + cFrame.top - m->m_cItemBorders.top + nHeight <= cScreenRes.y )
		{
			cScrPos.y = cLeftTop.y + cFrame.top - m->m_cItemBorders.top;
		}
		else
		{
			cScrPos.y = cLeftTop.y + cFrame.bottom - nHeight + m->m_cItemBorders.top + pcItem->GetSubMenu()->m->m_cItemBorders.bottom;
		}
	}
	m->m_bHasOpenChilds = true;

	pcItem->GetSubMenu()->Open( cScrPos );
}

/***************************************************************************
 * NAME: _Close()
 * DESC: Closes the menu based on what menu is highlighted
 * NOTE: Private
 * SEE ALSO:
 **************************************************************************/
void Menu::_Close( bool bCloseChilds, bool bCloseParent )
{
	AutoLocker __lock__( &m->m_pcRoot->m->m_cMutex );
	MenuItem *pcTmp;
	
	for( pcTmp = m->m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->_GetNext() )
	{
		if( pcTmp->IsHighlighted() )
		{
			pcTmp->SetHighlighted( false );
		}
	}

	if( bCloseChilds )
	{
		for( pcTmp = m->m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->_GetNext() )
		{
			if( pcTmp->GetSubMenu() != NULL )
			{
				pcTmp->GetSubMenu()->_Close( bCloseChilds, bCloseParent );
			}
		}
	}
	
	if( m->m_pcWindow != NULL )
	{
		MenuWindow *pcWnd = m->m_pcWindow;

		m->m_pcWindow = NULL;	// Must clear this before we shutdown.
		pcWnd->ShutDown();

		Menu *pcParent = GetSuperMenu();

		if( pcParent != NULL )
		{
			Window *pcWnd = pcParent->m->m_pcWindow;
			
			if( pcWnd != NULL )
			{
				pcParent->MakeFocus( true );
			}
			pcParent->m->m_bHasOpenChilds = false;
			pcParent->m->m_bIsTracking = true;
		}
		
		if( m->m_cCloseMsgTarget.IsValid() )
		{
			m->m_cCloseMsgTarget.SendMessage( &m->m_cCloseMsg );
		}
	}
	#if 0
	/* This causes a freeze */
	if( m->m_bIsRootMenu && GetWindow() != NULL )
	{
		MakeFocus( false );
	}
	#endif


	

	if( bCloseParent )
	{
		Menu *pcParent = GetSuperMenu();

		if( pcParent != NULL )
		{
			pcParent->_Close( false, true );
		}
	}
	Flush();
	m->m_bIsTracking = false;
}

/***************************************************************************
 * NAME: _SelectItem()
 * DESC: Selects the menuitem down the menu and then highlights it.
 * NOTE: Private
 * SEE ALSO: _SelectNext(), _SelectPrev()
 **************************************************************************/
void Menu::_SelectItem( MenuItem* pcItem)
{
	MenuItem *pcPrev = FindMarked();

	if( pcItem == pcPrev )
	{
		return;
	}
	
	if (pcPrev != NULL)
	{
		pcPrev->SetHighlighted( false );
	}

	if (pcItem != NULL && pcItem->IsSelectable())
	{
		pcItem->SetHighlighted( true );
	}
}

/***************************************************************************
 * NAME: _SelectPrev()
 * DESC: Selects the previous menuitem down the menu and then highlights it.
 * NOTE: Private
 * SEE ALSO: _SelectNext(), _SelectItem()
 **************************************************************************/
void Menu::_SelectPrev()
{
	MenuItem *pcTmp;
	MenuItem *pcPrev = NULL;

	for( pcTmp = m->m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->_GetNext() )
	{
		if( pcTmp->IsHighlighted() )
		{
			if( pcPrev != NULL)
			{
				if (pcPrev->IsSelectable())
				{
					_SelectItem(pcPrev);
					Flush();
					return;
				}
				else  // now we are going to go up menuitems until we find a selectable item
				{
					int nIndex = GetIndexOf(pcPrev);  //get the index of previous
					do
					{
						pcPrev = GetItemAt(--nIndex); //get the item at next less index
					}	while (pcPrev != NULL && !pcPrev->IsSelectable());
					_SelectItem(pcPrev);
					Flush();
					return;
				}
			}
		}
		pcPrev = pcTmp;
	}

	if( m->m_eLayout == ITEMS_IN_COLUMN )
	{
		Menu *pcSuper = GetSuperMenu();

		if( pcSuper != NULL && pcSuper->m->m_eLayout == ITEMS_IN_ROW )
		{
			_Close( false, false );
		}
	}
	else
	{
		_SelectItem( m->m_pcFirstItem );
		Flush();
	}
}

/***************************************************************************
 * NAME: _SelectNext()
 * DESC: Selects the next menuitem down the menu and then highlights it.
 * NOTE: Private
 * SEE ALSO: _SelectPrev(), _SelectItem()
 **************************************************************************/
void Menu::_SelectNext()
{
	MenuItem *pcTmp;

	for( pcTmp = m->m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->_GetNext() )
	{
		if( pcTmp->IsHighlighted()  || pcTmp->_GetNext(  ) == NULL )
		{
			if( pcTmp->_GetNext() != NULL)
			{
				if (pcTmp->_GetNext()->IsSelectable())  //so the next item is selectable
				{
					_SelectItem( pcTmp->_GetNext());
				}
				else  //we must traverse down until we find a selectable item
				{
					do
					{
						pcTmp = pcTmp->_GetNext();
					} while (pcTmp->_GetNext() != NULL && !pcTmp->IsSelectable());
					_SelectItem(pcTmp); //found the item now select it
				}
			}
			else
			{
				_SelectItem(pcTmp);
			}
			Flush();
			return;
		}
	}
}


void Menu::_SetOrClearShortcut( MenuItem* pcStart, bool bSet )
{
	Window* pcWnd = GetWindow();

	if( !pcWnd ) return;

	for( ; pcStart; pcStart = pcStart->_GetNext() )
	{
		if( pcStart->GetSubMenu() ) {
			_SetOrClearShortcut( pcStart->GetSubMenu()->m->m_pcFirstItem, bSet );
		}
		if( !( pcStart->GetShortcut() == "" ) ) {
			if( bSet ) {
				if( pcStart->m->m_pcMessage ) {
//					printf("SET   Shortcut( %s )\n",pcStart->GetShortcut().c_str() );
					pcWnd->AddShortcut( ShortcutKey( pcStart->GetShortcut() ), new Message( *pcStart->m->m_pcMessage ) );
				}
			} else {
				pcWnd->RemoveShortcut( ShortcutKey( pcStart->GetShortcut() ) );
//				printf("CLEAR Shortcut( %s )\n",pcStart->GetShortcut().c_str() );
			}
		}
	}
}

Image* Menu::_GetSubMenuArrow( int nState )
{
	if( nState > 1 ) nState = 1;
	if( nState < 0 ) nState = 0;
	if( !m->m_pcSubArrowImage[ nState ] ) {
		m->m_pcSubArrowImage[ nState ] = new BitmapImage(nSubMenuArrowData,IPoint(SUB_MENU_ARROW_W,SUB_MENU_ARROW_H),CS_RGB32);
		if( nState == 0 ) {
			m->m_pcSubArrowImage[ nState ]->ApplyFilter( Image::F_GRAY );
		}
	}
	
	return m->m_pcSubArrowImage[ nState ];
}

float Menu::_GetColumnWidth( int nColumn )
{
	return m->m_vColumnWidths[ nColumn ];
}

/***************************end of os::Menu*************************************/


/***************************beginning of MenuWindow*****************************/

/***************************************************************************
 * NAME: MenuWindow()
 * DESC: The window that is opened when you open a <enu.
 * NOTE: Private
 * SEE ALSO:
 **************************************************************************/
MenuWindow::MenuWindow( const Rect & cFrame, Menu * pcMenu ):Window( cFrame, "menu", "menu", WND_NO_BORDER | WND_SYSTEM )
{
	// If the machine is very loaded the previous window hosting the menu
	// might still own it, so we must wait til it terminates.
	while( pcMenu->GetWindow() != NULL )
	{
		snooze( 50000 );
	}
	m_pcMenu = pcMenu;
	AddChild( pcMenu );
}

/**internal*/
MenuWindow::~MenuWindow()
{
	if( m_pcMenu != NULL )
	{
		RemoveChild( m_pcMenu );
	}
}
/************************end of MenuWindow**************************************/


void Menu::__ME_reserved_1__()
{
}

void Menu::__ME_reserved_2__()
{
}

void Menu::__ME_reserved_3__()
{
}

void Menu::__ME_reserved_4__()
{
}

void Menu::__ME_reserved_5__()
{
}

void MenuSeparator::__MS_reserved_1__()
{
}

void MenuSeparator::__MS_reserved_2__()
{
}

void MenuItem::__MI_reserved_1__()
{
}

void MenuItem::__MI_reserved_2__()
{
}

void MenuItem::__MI_reserved_3__()
{
}

void MenuItem::__MI_reserved_4__()
{
}

void MenuItem::__MI_reserved_5__()
{
}
