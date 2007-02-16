/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
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
#include <atheos/types.h>

#include <gui/window.h>
#include <gui/view.h>
#include <gui/scrollbar.h>
#include <gui/menu.h>
#include <util/application.h>
#include <util/message.h>
#include <util/string.h>

#include <gui/font.h>
#include <gui/bitmap.h>

#include <appserver/protocol.h>

#include <macros.h>

using namespace os;

int View::s_nNextTabOrder = 0;


static Color32_s g_asDefaultColors[] = {
	Color32_s( 0xaa, 0xaa, 0xaa, 0xff ),	// COL_NORMAL
	Color32_s( 0xff, 0xff, 0xff, 0xff ),	// COL_SHINE
	Color32_s( 0x00, 0x00, 0x00, 0xff ),	// COL_SHADOW
	Color32_s( 0x66, 0x88, 0xbb, 0xff ),	// COL_SEL_WND_BORDER
	Color32_s( 0x78, 0x78, 0x78, 0xff ),	// COL_NORMAL_WND_BORDER
	Color32_s( 0x00, 0x00, 0x00, 0xff ),	// COL_MENU_TEXT
	Color32_s( 0x00, 0x00, 0x00, 0xff ),	// COL_SEL_MENU_TEXT
	Color32_s( 0xcc, 0xcc, 0xcc, 0xff ),	// COL_MENU_BACKGROUND
	Color32_s( 0x66, 0x88, 0xbb, 0xff ),	// COL_SEL_MENU_BACKGROUND
	Color32_s( 0x78, 0x78, 0x78, 0xff ),	// COL_SCROLLBAR_BG
	Color32_s( 0xaa, 0xaa, 0xaa, 0xff ),	// COL_SCROLLBAR_KNOB
	Color32_s( 0x78, 0x78, 0x78, 0xff ),	// COL_LISTVIEW_TAB
	Color32_s( 0xff, 0xff, 0xff, 0xff ),	// COL_LISTVIEW_TAB_TEXT
	Color32_s( 0x00, 0x00, 0x00, 0xff ),    // COL_ICON_TEXT
	Color32_s( 0xBA, 0xC7, 0xE3, 0xff ),    // COL_ICON_SELECTED
	Color32_s( 0xff, 0xff, 0xff, 0xff ),	// COL_ICON_BACKGROUND
	Color32_s( 0x00, 0xCC, 0x00, 0xff ),    // COL_FOCUS
};

/** Get the value of one of the standard system colors.
 * \par Description:
 *	Call this function to obtain one of the user-configurable
 *	system colors. This should be used whenever possible instead
 *	of hardcoding colors to make it possible for the user to
 *	customize the look.
 * \par Note:
 * \par Warning:
 * \param
 *	nColor - One of the COL_xxx enums from default_color_t
 * \return
 *	The current color for the given system pen.
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Color32_s os::get_default_color( default_color_t nColor )
{
	return ( g_asDefaultColors[nColor] );
}

/** \internal */

void os::__set_default_color( default_color_t nColor, const Color32_s & sColor )
{
	g_asDefaultColors[nColor] = sColor;
}

void os::set_default_color( default_color_t nColor, const Color32_s & sColor )
{
	g_asDefaultColors[nColor] = sColor;
}

static Color32_s Tint( const Color32_s & sColor, float vTint )
{
	int r = int ( ( float ( sColor.red ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int g = int ( ( float ( sColor.green ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int b = int ( ( float ( sColor.blue ) * vTint + 127.0f * ( 1.0f - vTint ) ) );

	if( r < 0 )
		r = 0;
	else if( r > 255 )
		r = 255;
	if( g < 0 )
		g = 0;
	else if( g > 255 )
		g = 255;
	if( b < 0 )
		b = 0;
	else if( b > 255 )
		b = 255;
	return ( Color32_s( r, g, b, sColor.alpha ) );
}


class View::Private
{
      public:
	int m_hViewHandle;	// Our bridge to the server
	port_id m_hReplyPort;	// Used as reply address when talking to server
	View *m_pcTopChild;
	View *m_pcBottomChild;

	View *m_pcLowerSibling;
	View *m_pcHigherSibling;

	View *m_pcParent;

	View *m_pcPrevFocus;

	ScrollBar *m_pcHScrollBar;
	ScrollBar *m_pcVScrollBar;
	Rect m_cFrame;
	Point m_cScrollOffset;
	String m_cTitle;

	drawing_mode m_eDrawingMode;
	Color32_s m_sFgColor;
	Color32_s m_sBgColor;
	Color32_s m_sEraseColor;

	Font *m_pcFont;

	int m_nBeginPaintCount;
	int __unused;

	uint32 m_nResizeMask;
	uint32 m_nFlags;
	int m_nHideCount;
	int m_nMouseMoveRun;
	int m_nMouseMode;	/* Record wether the mouse was inside or outside
				 * the bounding box during the last mouse event */
	int m_nTabOrder;	// Sorting order for keyboard manouvering

	ShortcutKey	m_cKey;	// Keyboard shortcut
	Menu* m_pcContextMenu; // Popup menu for this view.
};


/**
 * View constructor
 *  \param	cFrame The frame rectangle in the parents coordinate system.
 *  \param	pzTitle The logical name of the view. This parameter is newer
 *	   	rendered anywhere, but is passed to the Handler::Handler()
 *		constructor to identify the view.
 *  \param	nResizeMask Flags defining how the views frame rectangle is
 *		affected if the parent view is resized.
 *  \param	nFlags Various flags to control the views behavour.
 *
 *  \sa AttachedToWindow(), AllAttached(), os::view_flags, os::view_resize_flags, Handler::Handler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

View::View( const Rect & cFrame, const String & cTitle, uint32 nResizeMask, uint32 nFlags ):Handler( cTitle )	//, m_cFrame( cFrame ), m_cTitle( cTitle ), m_nResizeMask( nResizeMask ), m_nFlags( nFlags )
{
	m = new Private;

	m->m_hReplyPort = create_port( "view_reply", DEFAULT_PORT_SIZE );

	m->m_pcContextMenu = NULL;

	m->m_cFrame = cFrame;
	m->m_cTitle = cTitle;
	m->m_nResizeMask = nResizeMask;
	m->m_nFlags = nFlags;

	m->m_pcParent = NULL;
	m->m_pcPrevFocus = NULL;

	m->m_pcTopChild = NULL;
	m->m_pcBottomChild = NULL;

	m->m_pcLowerSibling = NULL;
	m->m_pcHigherSibling = NULL;

	m->m_pcFont = NULL;
	m->m_hViewHandle = -1;
	m->m_nBeginPaintCount = 0;

	m->m_sFgColor.red = 255;
	m->m_sFgColor.green = 255;
	m->m_sFgColor.blue = 255;

	m->m_eDrawingMode = DM_COPY;
	m->m_nMouseMode = MOUSE_OUTSIDE;
	m->m_nMouseMoveRun = 0;
	m->m_nHideCount = 0;

	m->m_pcHScrollBar = NULL;
	m->m_pcVScrollBar = NULL;

	m->m_nTabOrder = -1;
	m->m_sBgColor = get_default_color( COL_NORMAL );
	m->m_sEraseColor = get_default_color( COL_NORMAL );

	try
	{
		Font *pcFont = new Font( DEFAULT_FONT_REGULAR );

		SetFont( pcFont );
		pcFont->Release();
	} catch( ... )
	{
		dbprintf( "Error : View::View() unable to create font\n" );
	}

	Flush();
}

/** View destructor
 * \par Description:
 *	The destructor will release all resources used by the view
 *	itseld, and then recursively delete all child views.
 * \par
 *	To prevent a children to be deleted you must unlink it with
 *	View::RemoveChild() before deleting it's parent.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

View::~View()
{
	View *pcChild;

	if( m->m_pcVScrollBar != NULL )
	{
		m->m_pcVScrollBar->SetScrollTarget( NULL );
	}
	if( m->m_pcHScrollBar != NULL )
	{
		m->m_pcHScrollBar->SetScrollTarget( NULL );
	}

	Window *pcWnd = GetWindow();

	if( pcWnd != NULL )
	{
		pcWnd->_ViewDeleted( this );
	}

	while( ( pcChild = m->m_pcTopChild ) /*  GetChildAt( 0 ) */  )
	{
		RemoveChild( pcChild );
		delete pcChild;
	}

	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->RemoveChild( this );
	}
	_ReleaseFont();
	delete_port( m->m_hReplyPort );
	delete m;
}

/** Get the keybord manouvering order.
 * \par Description:
 *	This member is called by the system to decide which view to select next
 *	when the <TAB> key is pressed. The focus is given to the next view with
 *	higher or equal tab-order as the current. You can overload this member
 *	to decide the order whenever it is called, or rely on the default
 *	implementation that will return whatever was set by SetTabOrder().
 *	A negative return value means that the view should not be skipped when
 *	searching for the next view to activate.
 *
 * \return The views sorting order for keyboard manouvering.
 * \sa SetTabOrder()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int View::GetTabOrder() const
{
	return ( m->m_nTabOrder );
}

/** Set the keyboard manouvering sorting order.
 * \par Description:
 *	Set the value that will be returned by GetTabOrder().
 * \param
 *	nOrder - The sorting order.
 * \sa GetTabOrder()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::SetTabOrder( int nOrder )
{
	if( nOrder == NEXT_TAB_ORDER ) {
		nOrder = s_nNextTabOrder++;
	}

	s_nNextTabOrder = nOrder + 1;
	m->m_nTabOrder = nOrder;
}

/** Set popup menu for a View.
 * \par Description:
 *	Set a popup menu to be used for this View. The popup menu is opened when
 * the user right-clicks inside the View, unless the right-click event is
 * consumed by the subclass.
 * \param
 *	pcMenu - popup menu for this View. Deleted automatically.
 * \sa GetContextMenu()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void View::SetContextMenu( Menu* pcMenu )
{
	if( m->m_pcContextMenu ) delete m->m_pcContextMenu;
	m->m_pcContextMenu = pcMenu;
}

/** Get popup menu.
 * \par Description:
 *	Returns the popup menu assigned to this View, or null if no popup menu is
 * assigned.
 * \sa SetContextMenu()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
Menu* View::GetContextMenu() const
{
	return m->m_pcContextMenu;
}

const ShortcutKey& View::GetShortcut() const
{
	return ( m->m_cKey );
}

/** Set keyboard shortcut.
 * \par Description:
 *	Sets a shortcut that activates the View. The exact behaviour of shortcuts
 * is defined in the various subclasses. For a Button, for instance, pressing
 * the associated key will "push" the button, releasing the associated key will
 * "release" the button and send the associated message. Pressing any other key
 * while the shortcut key is still held, will cancel the operation. Please note
 * that most widgets (including Button) will automatically extract keyboard
 * shortcuts from their labels.
 * \param
 *	cShortcut - key combination to activate View.
 * \sa Control::SetLabel(), GetShortcut(), SetShortcutFromLabel(), ShortcutKey
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void View::SetShortcut( const ShortcutKey& cShortcut )
{
	if( m->m_cKey.IsValid() && GetWindow() ) {
		GetWindow()->RemoveShortcut( m->m_cKey );
	}
	m->m_cKey = cShortcut;
	if( m->m_cKey.IsValid() && GetWindow() ) {
		GetWindow()->AddShortcut( m->m_cKey, this );
	}
}

/** Set keyboard shortcut from Label.
 * \par Description:
 *	Like SetShortcut(), but the key is extracted from a text label. The
 * character following the first unescaped underscore character (_) is used.
 * \param
 *	cLabel - text string containing shortcut.
 * \sa GetShortcut(), SetShortcutFromLabel(), ShortcutKey
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void View::SetShortcutFromLabel( const String& cLabel )
{
	if( m->m_cKey.IsValid() && GetWindow() ) {
		GetWindow()->RemoveShortcut( m->m_cKey );
	}
	m->m_cKey.SetFromLabel( cLabel );
	if( m->m_cKey.IsValid() && GetWindow() ) {
		GetWindow()->AddShortcut( m->m_cKey, this );
	}
}

/** \internal
 *****************************************************************************/

void View::_LinkChild( View * pcChild, bool bTopmost )
{
	if( NULL == pcChild->m->m_pcParent )
	{
		pcChild->m->m_pcParent = this;
		if( NULL == m->m_pcBottomChild && NULL == m->m_pcTopChild )
		{
			m->m_pcBottomChild = pcChild;
			m->m_pcTopChild = pcChild;
			pcChild->m->m_pcLowerSibling = NULL;
			pcChild->m->m_pcHigherSibling = NULL;
		}
		else
		{
			if( bTopmost )
			{
				if( NULL != m->m_pcTopChild )
				{
					m->m_pcTopChild->m->m_pcHigherSibling = pcChild;
				}

				pcChild->m->m_pcLowerSibling = m->m_pcTopChild;
				m->m_pcTopChild = pcChild;
				pcChild->m->m_pcHigherSibling = NULL;
			}
			else
			{
				if( NULL != m->m_pcBottomChild )
				{
					m->m_pcBottomChild->m->m_pcLowerSibling = pcChild;
				}
				pcChild->m->m_pcHigherSibling = m->m_pcBottomChild;
				m->m_pcBottomChild = pcChild;
				pcChild->m->m_pcLowerSibling = NULL;
			}
		}
	}
	else
	{
		dbprintf( "ERROR : Attempt to add an view already belonging to a window\n" );
	}
}

/** \internal
 *****************************************************************************/

void View::_UnlinkChild( View * pcChild )
{
	if( pcChild->m->m_pcParent == this )
	{
		pcChild->m->m_pcParent = NULL;

		if( pcChild == m->m_pcTopChild )
		{
			m->m_pcTopChild = pcChild->m->m_pcLowerSibling;
		}
		if( pcChild == m->m_pcBottomChild )
		{
			m->m_pcBottomChild = pcChild->m->m_pcHigherSibling;
		}

		if( NULL != pcChild->m->m_pcLowerSibling )
		{
			pcChild->m->m_pcLowerSibling->m->m_pcHigherSibling = pcChild->m->m_pcHigherSibling;
		}

		if( NULL != pcChild->m->m_pcHigherSibling )
		{
			pcChild->m->m_pcHigherSibling->m->m_pcLowerSibling = pcChild->m->m_pcLowerSibling;
		}

		pcChild->m->m_pcLowerSibling = NULL;
		pcChild->m->m_pcHigherSibling = NULL;
	}
	else
	{
		dbprintf( "ERROR : Attempt to remove a view (%s) not belonging to this window\n", pcChild->GetName().c_str() );
	}
}


/** Start a drag and drop operation.
 * \par Description:
 *
 *	This member is normally called from the MouseMove() member to
 *	initiate a drag and drop operation. The function takes a
 *	os::Message containing the data to drag, and information about
 *	how to visually represent the dragged data to the user. The
 *	visual representation can be eighter a bitmap (possibly with
 *	an alpha-channel to make it half-transparent) or a simple
 *	rectangle.
 * \par
 *	When a drag and drop operation is in progress the application
 *	server will include the message given here in \p pcData in
 *	M_MOUSE_MOVED (os::View::MouseMove()) messages sendt to views
 *	as the user moves the mouse and also in the M_MOUSE_UP
 *	(os::View::MouseUp()) message that terminate the operation.
 * 
 * \param pcData
 *	A os::Message object containing the data to be dragged.
 * \param cHotSpot
 *	Mouse position relative to the visible representation
 *	of the dragged object.
 * \param pcBitmap
 *	A bitmap that will be used as the visible representation
 *	of the dragged data. The bitmap can have an alpha channel.
 * \param cBounds
 *	And alternate way to visually represent the dragged data.
 *	If a rectangle is used instead of an bitmap the appserver
 *	will render a rectangle instead of a bitmap.
 * \param pcReplyHandler
 *	The handler that should receive replies sendt by the receiver
 *	of the dragged data.
 * \sa MouseMove(), MouseUp(), os::Message, os::Handler
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::BeginDrag( Message * pcData, const Point & cHotSpot, const Bitmap * pcBitmap, Handler * pcReplyHandler )
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		dbprintf( "Error: View::BeginDrag() called on view not attached to a window\n" );
		return;
	}

	Message cReq( WR_BEGIN_DRAG );

	pcData->_SetReplyHandler( ( pcReplyHandler == NULL ) ? this : pcReplyHandler );

	cReq.AddInt32( "bitmap", pcBitmap->m_hHandle );
	cReq.AddPoint( "hot_spot", cHotSpot );
	cReq.AddMessage( "data", pcData );

	Message cReply;

	if( Messenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply ) < 0 )
	{
		dbprintf( "View::BeginDrag() failed to send request to server\n" );
	}
}

/** Start a drag and drop operation.
 * \par Description:
 *
 *	Same as void View::BeginDrag( Message* pcData, const Point& cHotSpot,
 *	const Bitmap* pcBitmap, Handler* pcReplyHandler ) except that this
 *	version accept a rectangle instead of a bitmap for the "drag gismo".
 * \par
 *	When starting a drag operation with this member the appserver will
 *	render a x-or'ed rectangle according to \a cBounds to indicate the
 *	drag. This is useful when the selection image would be to large to
 *	be rendered efficiently or when there is no good graphical representation
 *	of the dragged data.
 * \par
 *	See void View::BeginDrag( Message* pcData, const Point& cHotSpot, const Bitmap* pcBitmap, Handler* pcReplyHandler )
 *	for a more detailed description of the drag and drop system.
 * \sa void View::BeginDrag( Message* pcData, const Point& cHotSpot, const Bitmap* pcBitmap, Handler* pcReplyHandler )
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::BeginDrag( Message * pcData, const Point & cHotSpot, const Rect & cBounds, Handler * pcReplyHandler )
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		dbprintf( "Error: View::BeginDrag() called on view not attached to a window\n" );
		return;
	}

	Message cReq( WR_BEGIN_DRAG );

	pcData->_SetReplyHandler( ( pcReplyHandler == NULL ) ? this : pcReplyHandler );

	cReq.AddInt32( "bitmap", -1 );
	cReq.AddRect( "bounds", cBounds );
	cReq.AddPoint( "hot_spot", cHotSpot );
	cReq.AddMessage( "data", pcData );

	Message cReply;

	if( Messenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply ) < 0 )
	{
		dbprintf( "View::BeginDrag() failed to send request to server\n" );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SetFlags( uint32 nFlags )
{
	m->m_nFlags = nFlags;
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndSetFlags_s *psCmd = static_cast < GRndSetFlags_s * >( pcWindow->_AllocRenderCmd( DRC_SET_FLAGS, this, sizeof( GRndSetFlags_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->nFlags = m->m_nFlags;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

uint32 View::GetFlags( uint32 nMask ) const
{
	return ( m->m_nFlags & nMask );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SetResizeMask( uint32 nFlags )
{
	m->m_nResizeMask = nFlags;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

uint32 View::GetResizeMask() const
{
	return ( m->m_nResizeMask );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::GetPreferredSize( bool bLargest ) const
{
	if( bLargest ) {
		return Point( COORD_MAX, COORD_MAX );
	} else {
		return Point( 10.0f, 10.0f );
	}
	/*
	return ( ( bLargest ) ? Point( COORD_MAX, COORD_MAX ) : Point( 50.0f, 20.0f ) );*/
}

Point View::GetContentSize() const
{
	Rect cBounds = GetFrame();

	return ( Point( cBounds.Width(), cBounds.Height(  ) ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::GetScrollOffset( void ) const
{
	return ( m->m_cScrollOffset );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

View *View::GetParent( void ) const
{
	return ( m->m_pcParent );
}

/** \internal
 *****************************************************************************/

void View::_Attached( Window * pcWindow, View * pcParent, int hHandle, int nHideCount )
{
	m->m_nHideCount += nHideCount;
	if( pcWindow != NULL )
	{
		pcWindow->Flush();
		pcWindow->AddHandler( this );

		if( hHandle != -1 )
		{
			m->m_hViewHandle = hHandle;
		}
		else
		{
			Message cReq( WR_CREATE_VIEW );

			cReq.AddInt32( "top_view", pcWindow->_GetTopView()->m->m_hViewHandle );
			cReq.AddInt32( "flags", m->m_nFlags );
			cReq.AddPointer( "user_object", this );
			cReq.AddString( "name", GetName().c_str(  ) );
			cReq.AddRect( "frame", GetFrame() );
			cReq.AddInt32( "parent_view", ( pcParent ) ? pcParent->m->m_hViewHandle : -1 );
			cReq.AddColor32( "fg_color", m->m_sFgColor );
			cReq.AddColor32( "bg_color", m->m_sBgColor );
			cReq.AddColor32( "er_color", m->m_sEraseColor );
			cReq.AddInt32( "hide_count", ( hHandle == -1 ) ? m->m_nHideCount : 0 );
			cReq.AddPoint( "scroll_offset", m->m_cScrollOffset );

			Message cReply;

			Messenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply );

			cReply.FindInt( "handle", &m->m_hViewHandle );
		}
		SetNextHandler( pcParent );

		Font *pcFont;

		if( NULL != m->m_pcFont )
		{
			GRndSetFont_s *psCmd = static_cast < GRndSetFont_s * >( pcWindow->_AllocRenderCmd( DRC_SET_FONT, this, sizeof( GRndSetFont_s ) ) );

			if( psCmd != NULL )
			{
				psCmd->hFontID = m->m_pcFont->GetFontID();
			}
			pcWindow->_PutRenderCmd();
		}
		else
		{
			pcFont = new Font();
			pcFont->SetFamilyAndStyle( SYS_FONT_FAMILY, SYS_FONT_PLAIN );
			pcFont->SetProperties( 8.0f, 0.0f, 0.0f );
			SetFont( pcFont );
			pcFont->Release();
		}

		Flush();
		AttachedToWindow();
	}
	View *pcChild;

	for( pcChild = m->m_pcBottomChild; NULL != pcChild; pcChild = pcChild->m->m_pcHigherSibling )
	{
		pcChild->_Attached( pcWindow, this, -1, nHideCount );
	}
	if( pcWindow != NULL )
	{
		AllAttached();
	}
}

/** \internal
 *****************************************************************************/

void View::_Detached( bool bFirst, int nHideCount )
{
	View *pcChild;

	m->m_nHideCount -= nHideCount;

	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		DetachedFromWindow();
	}

	for( int i = 0; ( pcChild = GetChildAt( i ) ); ++i )
	{
		pcChild->_Detached( false, nHideCount );
	}

	if( pcWindow != NULL )
	{
		AllDetached();
	}

	if( pcWindow != NULL && m->m_hViewHandle != -1 )
	{
		pcWindow->Flush();
		pcWindow->_DeleteViewFromServer( this );
		m->m_hViewHandle = -1;
		if( pcWindow->RemoveHandler( this ) == false )
		{
			dbprintf( "View::_Detached() failed to remove view from Looper\n" );
		}
		pcWindow->_ViewDeleted( this );
	}

	if( bFirst )
	{
		m->m_pcParent = NULL;
	}
}

/** \internal
 *****************************************************************************/

void View::_WindowActivated( bool bIsActive )
{
	Window *pcWnd = GetWindow();
	View *pcChild;

	if( pcWnd != NULL ) {
		WindowActivated( bIsActive );
		if( pcWnd->GetFocusChild() == this )
		{
			Activated( bIsActive );
		}
	
		for( pcChild = m->m_pcBottomChild; NULL != pcChild; pcChild = pcChild->m->m_pcHigherSibling )
		{
			pcChild->_WindowActivated( bIsActive );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::RemoveChild( View * pcChild )
{
	Window *pcWindow = GetWindow();

	_UnlinkChild( pcChild );

	if( NULL != pcWindow && -1 != m->m_hViewHandle )
	{
		pcChild->_Detached( true, m->m_nHideCount );
		pcWindow->Sync();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::RemoveThis( void )
{
	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->RemoveChild( this );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::AddChild( View * pcChild, bool bAssignTabOrder )
{
	_LinkChild( pcChild, true );

	if( bAssignTabOrder )
	{
		pcChild->SetTabOrder( NEXT_TAB_ORDER );
	}

	pcChild->_Attached( GetWindow(), this, -1, m->m_nHideCount );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

View *View::GetChildAt( const Point & cPos ) const
{
	View *pcChild;

	for( pcChild = m->m_pcTopChild; NULL != pcChild; pcChild = pcChild->m->m_pcLowerSibling )
	{
		if( pcChild->m->m_nHideCount > 0 )
		{
			continue;
		}
		if( !( ( cPos.x < pcChild->m->m_cFrame.left ) || ( cPos.x > pcChild->m->m_cFrame.right ) || ( cPos.y < pcChild->m->m_cFrame.top ) || ( cPos.y > pcChild->m->m_cFrame.bottom ) ) )
		{
			return ( pcChild );
		}
	}
	return ( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

View *View::GetChildAt( int nIndex ) const
{
	View *pcChild;

	if( nIndex >= 0 )
	{
		for( pcChild = m->m_pcTopChild; NULL != pcChild; pcChild = pcChild->m->m_pcLowerSibling )
		{
			if( 0 == nIndex )
			{
				return ( pcChild );
			}
			nIndex--;
		}
	}
	return ( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ScrollBar *View::GetVScrollBar( void ) const
{
	return ( m->m_pcVScrollBar );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ScrollBar *View::GetHScrollBar( void ) const
{
	return ( m->m_pcHScrollBar );
}

/** \internal
 *****************************************************************************/

void View::_IncHideCount( bool bVisible )
{
	if( bVisible )
	{
		m->m_nHideCount--;
	}
	else
	{
		m->m_nHideCount++;
	}
	
	Window *pcWindow = GetWindow();
	if( pcWindow != NULL )
	{
		GRndShowView_s *psCmd;

		psCmd = ( GRndShowView_s * ) pcWindow->_AllocRenderCmd( DRC_SHOW_VIEW, this, sizeof( GRndShowView_s ) );

		if( psCmd != NULL )
		{
			psCmd->bVisible = bVisible;
		}
		pcWindow->_PutRenderCmd();
	}
	for( View * pcChild = m->m_pcBottomChild; pcChild != NULL; pcChild = pcChild->m->m_pcHigherSibling )
	{
		pcChild->_IncHideCount( bVisible );
	}
}

/** Show/hide a view and all it's children.
 * \par Description:
 *	Calling show with bVisible == false on a view have the same effect as
 *	unlinking it from it's parent, except that the view will remember it's
 *	entire state like the font, colors, position, depth, etc etc. A subsequent
 *	call to Show() with bVisible == true will restore the view compleatly.
 * \par
 *	Calls to Show() can be nested. If it is called twice with bVisible == false
 *	it must be called twice with bVisible == true before the view is made
 *	visible. It is safe to call Show(false) on a view before it is attached to a
 *	window. The view will then remember it's state and will not be visible
 *	when attached to a window later on.
 * \par
 *	A hidden view will not receive paint messages or input events from keybord
 *	or mouse. It will still receive messages targeted directly at the view.
 * \par Note:
 *	It is NOT leagal to show a visible view! Calling Show(true) before
 *	Show(false) is an error.
 * \param
 *	bVisible - A boolean telling if the view should be hidden or viewed.
 * \sa IsVisible(), Window::Show()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::Show( bool bVisible )
{
	if( bVisible )
	{
		m->m_nHideCount--;
	}
	else
	{
		m->m_nHideCount++;
	}
	
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndShowView_s *psCmd;

		psCmd = ( GRndShowView_s * ) pcWindow->_AllocRenderCmd( DRC_SHOW_VIEW, this, sizeof( GRndShowView_s ) );
		if( psCmd != NULL )
		{
			psCmd->bVisible = bVisible;
		}
		pcWindow->_PutRenderCmd();
	}
	for( View * pcChild = m->m_pcBottomChild; pcChild != NULL; pcChild = pcChild->m->m_pcHigherSibling )
	{
		pcChild->_IncHideCount( bVisible );
	}
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool View::IsVisible() const
{
	return ( m->m_nHideCount == 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::MakeFocus( bool bFocus )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		if( false == bFocus )
		{
			if( pcWindow->GetFocusChild() == this )
			{
				pcWindow->SetFocusChild( NULL );
			}
		}
		else
		{
			pcWindow->SetFocusChild( this );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool View::HasFocus( void ) const
{
	Window *pcWindow = GetWindow();

	if( NULL != pcWindow )
	{
		if( pcWindow->IsActive() == false )
		{
			return ( false );
		}
		View *pcFocus = pcWindow->GetFocusChild();

		return ( pcFocus == this );
	}
	else
	{
		return ( false );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::GetMouse( Point * pcPosition, uint32 *pnButtons ) const
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		dbprintf( "Error: View::GetMouse() called on view not attached to a window\n" );
		return;
	}

	Message cReq( WR_GET_MOUSE );

	Message cReply;

	if( Messenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply ) < 0 )
	{
		dbprintf( "View::GetMouse() failed to send request to server\n" );
	}
	if( pcPosition != NULL )
	{
		cReply.FindPoint( "position", pcPosition );
		ConvertFromScreen( pcPosition );
	}
	if( pnButtons != NULL )
	{
		cReply.FindInt( "buttons", pnButtons );
	}
}

void View::SetMousePos( const Point & cPosition )
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		dbprintf( "Error: View::GetMouse() called on view not attached to a window\n" );
		return;
	}

	Message cReq( WR_SET_MOUSE_POS );

	cReq.AddPoint( "position", ConvertToScreen( cPosition ) );

	if( Messenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq ) < 0 )
	{
		dbprintf( "View::SetMousePos() failed to send request to server\n" );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

uint32 View::GetQualifiers() const
{
	if( Application::GetInstance() == NULL )
	{
		return ( 0 );
	}
	return ( Application::GetInstance()->GetQualifiers(  ) );
}

/** Hook called by the system when the mouse is moved.
 * \par Description:
 *	This member is called from the window thread whenever the mouse is
 *	moved above the view or if the view has focus.
 * \par
 *	Oveload this member if your view need to handle mouse-move events.
 * \par
 *	The default implementation of this member will call MouseMove()
 *	on it's parent view if one exists.
 *
 * \param cNewPos
 *	New mouse position given in the views coordinate system.
 * \param nCode
 *	Enter/exit code. This is MOUSE_ENTERED when the mouse first
 *	enter the view, MOUSE_EXITED when the mouse leaves the view,
 *	MOUSE_INSIDE whenever the mouse move withing the boundary of
 *	the view and MOUSE_OUTSIDE when the mouse move outside the
 *	view (will only happen if the view has focus).
 * \param nButtons
 *	Bitmask telling which buttons that are currently pressed.
 *	Bit 0 is button 1 (left), bit 1 is button 2 (right), and
 *	bit 2 is button 3 (middle), and so on.
 * \param pcData
 *	Pointer to a Message object containing the dragged data
 *	if the user is in the middle of a drag and drop operation.
 *	Otherwise this pointer is NULL. Look at BeginDrag() for
 *	a more detailed description of the drag and drop system.
 *
 * \sa MouseDown(), MouseUp(), WheelMoved(), BeginDrag()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	Window *pcWnd = GetWindow();

	if( pcWnd != NULL )
	{
		pcWnd->_MouseEvent( ConvertToScreen( cNewPos ), nButtons, pcData, true );
	}
}

/** Hook called by the system when a mouse button is pressed.
 * \par Description:
 *	This member is called from the window thread whenever
 *	a mouse button is clicked above the view. You can
 *	overload this member if your view need to know about
 *	mouse-down events.
 * \par
 *	A view will not automatically take focus when clicked
 *	so if you want that behaviour you must call MakeFocus()
 *	from an overloaded version of this member.
 * \par
 *	The default implementation of this member will call MouseDown()
 *	on it's parent view if one exists.
 * 
 * \param cPosition
 *	Mouse position in the views coordinate system at the time the mouse
 *	was pressed.
 * \param nButtons
 *	Index of the pressed button. Buttons start at 1 for the left button,
 *	2 for the right button, 3 for the middle button. Additional buttons
 *	might be supported by the mouse driver and will then be assigned numbers
 *	from 4 and up.
 *
 * \sa MouseUp(), MouseMove(), WheelMoved()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::MouseDown( const Point & cPosition, uint32 nButtons )
{
	if( nButtons == 2 && m->m_pcContextMenu != NULL ) {
		m->m_pcContextMenu->Open( ConvertToScreen( cPosition ) );
	} else if( m->m_pcParent != NULL )
	{
		m->m_pcParent->MouseDown( ConvertToParent( cPosition ), nButtons );
	}
}

/** Hook called by the system when a mouse button is release.
 * \par Description:
 *	This member is called from the window thread whenever
 *	a mouse button is released above the view. You can
 *	overload this member if your view need to know about
 *	mouse-up events or if your view support drag and drop.
 * \par
 *	If mouse-up was the result of ending a drag and drop
 *	operation the \p pcData member will point to a Message
 *	containing the dragged data. Look at BeginDrag() for
 *	a more detailed description of the drag and drop system.
 * \par
 *	The default implementation of this member will call MouseDown()
 *	on it's parent view if one exists.
 * 
 * \param cPosition
 *	Mouse position in the views coordinate system at the time the mouse
 *	was pressed.
 * \param nButtons
 *	Index of the pressed button. Buttons start at 1 for the left button,
 *	2 for the right button, 3 for the middle button. Additional buttons
 *	might be supported by the mouse driver and will then be assigned numbers
 *	from 4 and up.
 * \param pcData
 *	Pointer to a Message object containing the dragged data if this
 *	mouse-up was the end of a drag and drop operation. If no data
 *	was dragged it will be NULL.
 *
 * \sa MouseDown(), MouseMove(), WheelMoved()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->MouseUp( ConvertToParent( cPosition ), nButtons, pcData );
	}
}

/** Hook called by the system when the scroll-wheel is rotated.
 * \par Description:
 *	This member is called from the window thread whenever the user
 *	rotates the scroll wheel with the mouse pointer above this
 *	view.
 * \par
 *	The default implementation of this member will call WheelMoved()
 *	on it's parent view if one exists.
 *
 * \since V0.3.7
 * \param cDelta
 *	Delta movement. Normally only the y value is used but it is possible
 *	for the mouse driver to also support horizontal scroll wheel functionality.
 *	The delta value is normally +/- 1.0 for each "click" on the wheel.
 * \sa MouseMove(), MouseDown(), MouseUp()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::WheelMoved( const Point & cDelta )
{
	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->WheelMoved( cDelta );
	}
}

/** Hook called by the system when a key is pressed while the view has focus.
 * \par Description:
 *	Overload this member if your view need to handle keyboard input.
 *	This member is called to allow the view to handle M_KEY_DOWN messages.
 *	The most common members are exctracted from the message and passed
 *	as parameters but you might need to obtain the raw message with
 *	os::Looper::GetCurrentMessage() and find some members yourself if you
 *	the data you needed are not passed in.
 * \par
 *	For example if you need the raw key-code for the pressed key you will
 *	have to lookup the "_raw_key" member from the message.
 *
 * \param pzString
 *	String containing a single UTF-8 encoded character. This is the character
 *	generated by the pressed key according to the current keymap accounting
 *	for any qualifiers that might be pressed.
 * \param pzRawString
 *	Same as \p pzString except that the key is converted without accounting
 *	for qualifiers. Ie. if 'A' is pressed while pressing <SHIFT> pzString
 *	will contain 'A' and pzRawString will contain 'a'.
 * \param nQualifiers
 *	Bitmask describing which \ref os_gui_qualifiers "qualifiers" that was
 *	active when the key was
 *	pressed. 
 *
 * \sa KeyUp(), os::Looper::GetCurrentMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->KeyDown( pzString, pzRawString, nQualifiers );
	}
}

/** Hook called by the system when a key is released while the view has focus.
 * \par Description:
 *	This is the counterpart to KeyDown().
 * \sa KeyDown()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::KeyUp( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->KeyUp( pzString, pzRawString, nQualifiers );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int View::ToggleDepth( void )
{
	Window *pcWindow = GetWindow();

	if( NULL != pcWindow )
	{
		Message cReq( WR_TOGGLE_VIEW_DEPTH );

		cReq.AddInt32( "top_view", pcWindow->_GetTopView()->m->m_hViewHandle );
		cReq.AddInt32( "view", m->m_hViewHandle );

		Message cReply;

		if( Messenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply ) < 0 )
		{
			dbprintf( "Error: Window::ToggleDepth() failed to send WR_TOGGLE_VIEW_DEPTH request to the server\n" );
		}
	}

	if( NULL != m->m_pcParent )
	{
		if( m->m_pcParent->m->m_pcTopChild != this )
		{
			m->m_pcParent->_UnlinkChild( this );
			m->m_pcParent->_LinkChild( this, true );
		}
		else
		{
			m->m_pcParent->_UnlinkChild( this );
			m->m_pcParent->_LinkChild( this, false );
		}
	}
	return ( false );
}

/** Set the size and position relative to the parent view.
 * \par Description:
 *	Set the frame-rectangle of the view relative to it's parent.
 *	If this cause new areas of this view or any of it's
 *	siblings/childs the affected views will receive a paint
 *	message to update the newly exposed areas.
 * \par
 *	If the upper-left corner of the view is moved the
 *	View::FrameMoved() hook will be called and if the lower-right
 *	corner move relative to the upper-left corner the
 *	View::FrameSized() hook will be called.
 *
 * \param cRect
 *	The new frame rectangle.
 * \sa ResizeTo(), ResizeBy(), MoveTo(), MoveBy()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::SetFrame( const Rect & cRect, bool bNotifyServer )
{
	Point cMove = Point( cRect.left, cRect.top ) - Point( m->m_cFrame.left, m->m_cFrame.top );
	Point cSize = Point( cRect.Width(), cRect.Height(  ) ) - Point( m->m_cFrame.Width(  ), m->m_cFrame.Height(  ) );

	m->m_cFrame = cRect;


	if( cSize.x || cSize.y || cMove.x || cMove.y )
	{
		Window *pcWindow = GetWindow();

		if( bNotifyServer && pcWindow != NULL )
		{
			GRndSetFrame_s *psCmd = static_cast < GRndSetFrame_s * >( pcWindow->_AllocRenderCmd( DRC_SET_FRAME, this, sizeof( GRndSetFrame_s ) ) );

			if( psCmd != NULL )
			{
				psCmd->cFrame = cRect;
			}
			pcWindow->_PutRenderCmd();
		}

		if( cSize.x || cSize.y )
		{
			View *pcChild;

			for( pcChild = m->m_pcBottomChild; NULL != pcChild; pcChild = pcChild->m->m_pcHigherSibling )
			{
				pcChild->_ParentSized( cSize );
			}
		}
	}

	if( cSize.x || cSize.y )
	{
		FrameSized( cSize );
	}
	if( cMove.x || cMove.y )
	{
		FrameMoved( cMove );
	}
}

/** Move the view within the parent coordinate system
 * \par Description:
 *	Move the view relative to it's current position.
 * \param cDelta
 *	The distance to move the view.
 * \sa MoveTo()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::MoveBy( const Point & cDelta )
{
	SetFrame( m->m_cFrame + cDelta );
}

/** \sa MoveBy( const Point& cDelta )
 *****************************************************************************/

void View::MoveBy( float vDeltaX, float vDeltaY )
{
	SetFrame( m->m_cFrame + Point( vDeltaX, vDeltaY ) );
}

/** Set the views position within the parent coordinate system.
 * \par Description:
 *	Move the view to a new absolute position within
 *	the parents coordinate system.
 * \param cPos
 *	The new position of the upper/left corner of the view
 * \sa MoveBy()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


void View::MoveTo( const Point & cPos )
{
	SetFrame( m->m_cFrame - GetLeftTop() + cPos );
}

/** \sa MoveTo( const Point& cPos )
 *****************************************************************************/

void View::MoveTo( float x, float y )
{
	SetFrame( m->m_cFrame - GetLeftTop() + Point( x, y ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ResizeBy( const Point & cDelta )
{
	SetFrame( Rect( m->m_cFrame.left, m->m_cFrame.top, m->m_cFrame.right + cDelta.x, m->m_cFrame.bottom + cDelta.y ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ResizeBy( float vDeltaW, float vDeltaH )
{
	SetFrame( Rect( m->m_cFrame.left, m->m_cFrame.top, m->m_cFrame.right + vDeltaW, m->m_cFrame.bottom + vDeltaH ) );
}

/** Set a new absolute size for the view.
 * \par Description:
 *	See ResizeTo(float,float)
 * \param cSize
 *	New size (os::Point::x is width, os::Point::y is height).
 * \sa ResizeTo(float,float)
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::ResizeTo( const Point & cSize )
{
	SetFrame( Rect( m->m_cFrame.left, m->m_cFrame.top, m->m_cFrame.left + cSize.x, m->m_cFrame.top + cSize.y ) );
}

/** Set a new absolute size for the view.
 * \par Description:
 *	Move the bottom/right corner of the view to give it
 *	the new size. The top/left corner will not be affected.
 *	This will cause FrameSized() to be called and any child
 *	views to be resized according to their resize flags if
 *	the new size differ from the previous.
 * \note
 *	Coordinates start in the middle of a pixel and all rectangles
 *	are inclusive so the real width and height in pixels will be
 *	1 larger than the value given in \p w and \p h.
 * \param w
 *	New width
 * \param h
 *	New height.
 * \return
 * \par Error codes:
 * \sa ResizeTo(const Point&), ResizeBy(), SetFrame()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::ResizeTo( float w, float h )
{
	SetFrame( Rect( m->m_cFrame.left, m->m_cFrame.top, m->m_cFrame.left + w, m->m_cFrame.top + h ) );
}

/** Restrict rendering using a clipping region.
 * \par Description:
 *	Restrict rendering using a clipping region. By assigning a drawing
 *	region to a view you can restrict what areas of the view that will
 *	be affected by subsequent rendering commands. Only areas represented
 *	by the region (and are not obscured by other views or resides outside
 *	the view frame) will be rendered to.
 * \par
 *	Unlike SetShapeRegion() the drawing region is not inherited by children
 *	views and it will not affect the shape of the view (ie. it will not make
 *	any areas of the view transparent) and it will not affect hit testing.
 * \bug
 *	The region is sendt using the fixed size buffer which can only hold
 *	about 680 rectangles. Trying to set a region with to many rectangles
 *	will cause this member to silently fail. This limit will be removed
 *	in a later version.
 * \param cReg
 *	A region defining drawable areas. The rectangles in the
 *	region should be in the views own coordinate system.
 * \return
 * \par Error codes:
 * \since 0.3.7
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::SetDrawingRegion( const Region & cReg )
{
	int nCount = cReg.GetClipCount();
	int nPkgSize = sizeof( GRndRegion_s ) + sizeof( IRect ) * nCount;

	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		return;
	}
	GRndRegion_s *psCmd = static_cast < GRndRegion_s * >( pcWindow->_AllocRenderCmd( DRC_SET_DRAW_REGION, this, nPkgSize ) );

	if( psCmd == NULL )
	{
		pcWindow->_PutRenderCmd();
		return;
	}
	psCmd->m_nClipCount = nCount;
	IRect *pasRects = reinterpret_cast < IRect * >( psCmd + 1 );
	ClipRect *pcClip = cReg.m_cRects.m_pcFirst;

	for( int i = 0; i < nCount; ++i )
	{
		pasRects[i] = pcClip->m_cBounds;
		pcClip = pcClip->m_pcNext;
	}
	pcWindow->_PutRenderCmd();
}

/** Remove any previously assigned drawing region.
 * \par Description:
 *	Remove any previously assigned drawing region and allow subsequent
 *	rendering commands to alter any visible areas of the view (rendering
 *	will still be restricted by the damage-list during a Paint() update).
 * \since 0.3.7
 * \sa SetDrawingRegion(), SetShapeRegion(), ClearShapeRegion()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::ClearDrawingRegion()
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		return;
	}
	GRndRegion_s *psCmd = static_cast < GRndRegion_s * >( pcWindow->_AllocRenderCmd( DRC_SET_DRAW_REGION, this, sizeof( GRndRegion_s ) ) );

	if( psCmd == NULL )
	{
		pcWindow->_PutRenderCmd();
		return;
	}
	psCmd->m_nClipCount = -1;
	pcWindow->_PutRenderCmd();
}

/** Define a non-square shape for the view.
 * \par Description:
 *	Define a non-square shape for the view.
 * \par
 *	Normally a view's shape is defined by a simple rectangle but it is
 *	possible to define views with complex shapes and even with holes by
 *	restricting the shape using a clipping region. A clipping region
 *	(os::Region) is basically a list of non-overlapping visible rectangles.
 *	When adding a shape-defining clipping region to a view all areas not
 *	covered by a rectangle in the region will be transparent.
 * \par
 *	Note that defining complex shapes with many rectangles can quickly
 *	become expensive, espesially if multiple complexly shaped views
 *	are siblings.
 * \bug
 *	The region is sendt using the fixed size buffer which can only hold
 *	about 680 rectangles. Trying to set a region with to many rectangles
 *	will cause this member to silently fail. This limit will be removed
 *	in a later version.
 * \param cReg
 *	The region defining the new shape of the view. The rectangles in the
 *	region should be in the views own coordinate system.
 * \since 0.3.7
 * \sa ClearShapeRegion(), SetDrawingRegion(), os::Window::SetShapeRegion()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::SetShapeRegion( const Region & cReg )
{
	int nCount = cReg.GetClipCount();
	int nPkgSize = sizeof( GRndRegion_s ) + sizeof( IRect ) * nCount;

	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		return;
	}
	GRndRegion_s *psCmd = static_cast < GRndRegion_s * >( pcWindow->_AllocRenderCmd( DRC_SET_SHAPE_REGION, this, nPkgSize ) );

	if( psCmd == NULL )
	{
		pcWindow->_PutRenderCmd();
		return;
	}
	psCmd->m_nClipCount = nCount;
	IRect *pasRects = reinterpret_cast < IRect * >( psCmd + 1 );
	ClipRect *pcClip = cReg.m_cRects.m_pcFirst;

	for( int i = 0; i < nCount; ++i )
	{
		pasRects[i] = pcClip->m_cBounds;
		pcClip = pcClip->m_pcNext;
	}
	pcWindow->_PutRenderCmd();
	Flush();
}

/** Remove any previously assigned shape region.
 * \par Description:
 *	Restore the view to it's normal rectangular shape as defined by
 *	it's frame rectangle.
 * \note
 *	Clearing the shape rectangle will automatically flush the render
 *	queue.
 * \sa SetShapeRegion(), os::Window::ClearShapeRegion()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::ClearShapeRegion()
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		return;
	}
	GRndRegion_s *psCmd = static_cast < GRndRegion_s * >( pcWindow->_AllocRenderCmd( DRC_SET_SHAPE_REGION, this, sizeof( GRndRegion_s ) ) );

	if( psCmd == NULL )
	{
		pcWindow->_PutRenderCmd();
		return;
	}
	psCmd->m_nClipCount = -1;
	pcWindow->_PutRenderCmd();
	Flush();
}

/** Virtual hook called by the system when the view is moved within it's parent.
 * \par Description:
 *	Overload this member if you need to know when the view is moved
 *	within the coordinate system of the parent.
 * \par Note:
 *	This member is called after the view is moved. If you need the
 *	old position you can subtract the \p cDelta value from the
 *	current position.
 * \param cDelta
 *	The distance the view was moved.
 * \sa FrameSized(), SetFrame(), MoveBy(), MoveTo()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::FrameMoved( const Point & cDelta )
{
}

/** Virtual hook called by the system when the view is resized.
 * \par Description:
 *	Overload this member if you need to know when the view is resized.
 * \par Note:
 *	This member is called after the view is resized. If you need the
 *	old size you can subtract the \p cDelta calue from the current
 *	size.
 * \param cDelta
 *	The distance the bottom/right corner was moved relative to the
 *	upper/left corner.
 * \sa FrameMoved(), SetFrame, ResizeBy(), ResizeTo()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::FrameSized( const Point & cDelta )
{
}

/** Virtual hook called by the system when the view content is scrolled
 * \par Description:
 *	Overload this member if you need to know when the views content
 *	has been scrolled with the ScrollBy() or ScrollTo() members.
 * \par
 *	See the ScrollBy() documentation for more details on scrolling.
 * \par Note:
 *	This member is called after the view has been scrolled. If you
 *	need the old scroll offset you can subtract the \p cDelta value
 *	from the current scroll offset.
 * \param cDelta
 *	The distance the view was scrolled.
 * \sa ScrollBy(), ScrollTo(), GetScrollOffset()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


void View::ViewScrolled( const Point & cDelta )
{
}

/** Add a rectangle to the damage list.
 * \par Description:
 *	To avoid rendering the whole view when a new area is made
 *	visible, the appserver maintain a damage list containing
 *	areas made visible since the last paint message was sendt
 *	to the view. When the View respond to the paint message
 *	the appserver will clip the rendering to the damage list
 *	to avoid rendering any pixels that are still intact.
 *	By calling Invalidate() you can force an area
 *	into the damage list, and make the appserver send a
 *	paint message to the view. This will again make the
 *	Paint() member be called to revalidate the damaged
 *	areas.
 * \param cRect
 *	The rectangle to invalidate.
 * \param bRecurse
 *	If true cRect will also be converted into each children's
 *	coordinate system and added to their damage list.
 * 
 * \sa Invalidate( bool ), Paint()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::Invalidate( const Rect & cRect, bool bRecurse )
{
	Window *pcWnd = GetWindow();

	if( pcWnd == NULL )
	{
		return;
	}
	GRndInvalidateRect_s *psCmd = static_cast < GRndInvalidateRect_s * >( pcWnd->_AllocRenderCmd( DRC_INVALIDATE_RECT, this, sizeof( GRndInvalidateRect_s ) ) );

	if( psCmd != NULL )
	{
		psCmd->m_cRect = cRect;
		psCmd->m_bRecurse = bRecurse;
	}
	pcWnd->_PutRenderCmd();
}

/** Invalidate the whole view.
 * \par Description:
 *	Same as calling Invalidate( GetBounds() ), but more efficient since
 *	the appserver know that the whole view is to be invalidated, and
 *	does not have to merge the rectangle into the clipping region.
 * \param bRecurse - True if all childs should be invalidated reqursivly aswell.
 * \sa Invalidate( const Rect&, bool ), Paint()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::Invalidate( bool bRecurse )
{
	Window *pcWnd = GetWindow();

	if( pcWnd == NULL )
	{
		return;
	}
	GRndInvalidateView_s *psCmd = static_cast < GRndInvalidateView_s * >( pcWnd->_AllocRenderCmd( DRC_INVALIDATE_VIEW, this, sizeof( GRndInvalidateView_s ) ) );

	if( psCmd != NULL )
	{
		psCmd->m_bRecurse = bRecurse;
	}
	pcWnd->_PutRenderCmd();
}

/** \internal
 *****************************************************************************/

void View::_ParentSized( const Point & cDelta )
{
	Rect cNewFrame = m->m_cFrame;

	if( m->m_nResizeMask & CF_FOLLOW_H_MIDDLE )
	{
		if( m->m_nResizeMask & CF_FOLLOW_LEFT )
		{
			cNewFrame.right = ( cNewFrame.right * 2 + cDelta.x + 1 ) / 2;
		}
		if( m->m_nResizeMask & CF_FOLLOW_RIGHT )
		{
			cNewFrame.left = ( cNewFrame.left * 2 + cDelta.x + 1 ) / 2;

			cNewFrame.right += cDelta.x;
		}
	}
	else
	{
		if( m->m_nResizeMask & CF_FOLLOW_RIGHT )
		{
			if( ( m->m_nResizeMask & CF_FOLLOW_LEFT ) == 0 )
			{
				cNewFrame.left += cDelta.x;
			}
			cNewFrame.right += cDelta.x;
		}
	}
	if( m->m_nResizeMask & CF_FOLLOW_BOTTOM )
	{
		if( ( m->m_nResizeMask & CF_FOLLOW_TOP ) == 0 )
		{
			cNewFrame.top += cDelta.y;
		}
		cNewFrame.bottom += cDelta.y;
	}

	SetFrame( cNewFrame );
}

/** \internal
 *****************************************************************************/

void View::_BeginUpdate()
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		if( m->m_hViewHandle != -1 )
		{
			if( m->m_nBeginPaintCount++ == 0 )
			{
				pcWindow->_AllocRenderCmd( DRC_BEGIN_UPDATE, this, sizeof( GRndHeader_s ) );
				pcWindow->_PutRenderCmd();
			}
		}
	}
}

/** \internal
 *****************************************************************************/

void View::_EndUpdate( void )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		if( --m->m_nBeginPaintCount == 0 )
		{
			pcWindow->_AllocRenderCmd( DRC_END_UPDATE, this, sizeof( GRndHeader_s ) );
			pcWindow->_PutRenderCmd();
		}
	}
}

/** Called by the system update "damaged" areas of the view.
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param cUpdateRect
 *	A rectangle enclosing all damaged areas. This is just a rough
 *	"worst-case", further fine-grained clipping will be performed
 *	by the Application Server to avoid updating non-damaged pixels
 *	and make the update as fast and flicker-free as possible.
 *
 * \sa Invalidate(), Flush()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


void View::Paint( const Rect & cUpdateRect )
{
	SetEraseColor( get_default_color( COL_NORMAL ) );
	EraseRect( cUpdateRect );
}

/** Flush the render queue.
 * \par Description:
 *	Call the window's Flush() member to flush the render queue.
 * \par
 *	If the view is not attached to a window, this member does nothing.
 * \par
 *	To get more info take a look at Window::Flush()
 * \sa Sync(), Window::Flush()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::Flush( void )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		pcWindow->Flush();
	}
}

/** Flush the render queue.
 * \par Description:
 *	Call the window's Flush() member to flush the render queue and
 *	syncronize the view with the appserver.
 * \par
 *	If the view is not attached to a window, this member does nothing.
 * \par
 *	To get more info take a look at Window::Flush()
 * \sa Sync(), Window::Flush()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::Sync( void )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		pcWindow->Sync();
	}
}

/** Get the current pen position.
 * \par Note:
 *	This member will do a syncronous request to the application server
 *	which is a rather expensive operation. Since some operations like
 *	text-rendering move the cursor in a fashion that is not predictable
 *	by the View class it does not cache the pen position. You should avoid
 *	using this member unless you have to know how much the pen was moved
 *	by a text rendering operation.
 * \return The current pen position in the views coordinate system.
 * \sa MovePenTo(), MovePenBy()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


Point View::GetPenPosition() const
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		return ( pcWindow->_GetPenPosition( m->m_hViewHandle ) );
	}
	else
	{
		return ( Point( 0, 0 ) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::MovePenTo( const Point & cPos )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndSetPenPos_s *psCmd = static_cast < GRndSetPenPos_s * >( pcWindow->_AllocRenderCmd( DRC_SET_PEN_POS, this, sizeof( GRndSetPenPos_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->bRelative = false;
			psCmd->sPosition = cPos;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::MovePenBy( const Point & cPos )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndSetPenPos_s *psCmd = static_cast < GRndSetPenPos_s * >( pcWindow->_AllocRenderCmd( DRC_SET_PEN_POS, this, sizeof( GRndSetPenPos_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->bRelative = true;
			psCmd->sPosition = cPos;
		}
		pcWindow->_PutRenderCmd();
	}
}

/** Render a text-string at the current pen position.
 * \par Description:
 *	DrawString() render an UTF-8 encoded character string at the current
 *	pen position using the current font, pen color and drawing mode.
 * \par
 *	There is a few issues you should be aware of when rendering text.
 *	Since AtheOS antialiaze the glyphs to improve the quality and
 *	readability of the text it need a bit more information than just
 *	a pen color. If the drawing mode is DM_COPY the text will be
 *	antialiazed against color of the current background pen (as
 *	set by SetBgColor()). If it is DM_OVER the text will be antialiazed
 *	against the existing graphics under the text. If the drawing mode
 *	is DM_BLEND the antialiazing is written to the alpha channel of the
 *	destination bitmap. This is useful when you render text into a
 *	transparent bitmap that will later be render on top of a background
 *	with unknown color. One examaple of this is when you create a bitmap
 *	that will be used as a drag and drop image. This mode only work when
 *	rendering into 32-bit bitmaps.
 * \par
 *	You should always use the DM_COPY mode when rendering against a static
 *	background where you know the background color. When using DM_OVER it
 *	is necessary to read the old pixels from the frame-buffer and this is
 *	a very slow operations.
 *
 * \param cString
 *	UTF-8 encoded string to render.
 * \param nLength
 *	Number of bytes to render from pString. If the string is NULL
 *	terminated a length of -1 can be used to render the entire
 *	string.
 * \sa SetFont(), os::Font, SetDrawingMode(), SetFgColor(), SetBgColor()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::DrawString( const String& cString )
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		return;
	}
	GRndDrawString_s *psCmd = static_cast < GRndDrawString_s * >( pcWindow->_AllocRenderCmd( DRC_DRAW_STRING, this,
			sizeof( GRndDrawString_s ) + cString.size() - 1 ) );

	if( psCmd != NULL )
	{
		psCmd->nLength = cString.size();
		memcpy( psCmd->zString, cString.c_str(), psCmd->nLength );
	}
	pcWindow->_PutRenderCmd();
}

void View::DrawString( const char *pzString, int nLength )
{
	Window *pcWindow = GetWindow();

	if( nLength == -1 ) {
		nLength = strlen( pzString );
	}

	if( pcWindow == NULL )
	{
		return;
	}
	GRndDrawString_s *psCmd = static_cast < GRndDrawString_s * >( pcWindow->_AllocRenderCmd( DRC_DRAW_STRING, this,
			sizeof( GRndDrawString_s ) + nLength - 1 ) );

	if( psCmd != NULL )
	{
		psCmd->nLength = nLength;
		memcpy( psCmd->zString, pzString, psCmd->nLength );
	}
	pcWindow->_PutRenderCmd();
}

/** Render a text-string in a specified rectangle.
 * \par Description:
 *	DrawText() renders a UTF-8 encoded character string inside the
 *	rectangle specified, using the current font, pen color and drawing
 *	mode.
 * \par
 *	DrawText() parses the specified text string for the following
 *	formatting codes:
 * \par
 *	\\n = newline
 * \par
 *	_ = underscore next character
 * \par
 *	ESCc = center text
 * \par
 *	ESCl = left align text
 * \par
 *	ESCr = right align text
 * \par
 *	There is a few issues you should be aware of when rendering text.
 *	Since Syllable antialiases the glyphs to improve the quality and
 *	readability of the text, it needs a bit more information than just
 *	a pen color. If the drawing mode is DM_COPY the text will be
 *	antialiased against the colour of the current background pen (as
 *	set by SetBgColor()). If it is DM_OVER the text will be antialiased
 *	against the existing graphics under the text. If the drawing mode
 *	is DM_BLEND the antialiazing is written to the alpha channel of the
 *	destination bitmap. This is useful when you render text into a
 *	transparent bitmap that will later be rendered on top of a background
 *	with unknown color. One examaple of this is when you create a bitmap
 *	that will be used as a drag and drop image. This mode only works when
 *	rendering into 32-bit bitmaps.
 * \par
 *	You should always use the DM_COPY mode when rendering against a static
 *	background where you know the background color. When using DM_OVER it
 *	is necessary to read the old pixels from the frame-buffer and this is
 *	a very slow operations.
 *
 * \param cPos
 *	Rectangle to render the text in, the text may be centred, left or right aligned
 *	inside this rectangle.
 * \param cString
 *	UTF-8 encoded string to render.
 * \param nLength
 *	Number of bytes to render from pString. If the string is NULL
 *	terminated a length of -1 can be used to render the entire
 *	string.
 * \param nFlags
 *	Flags that control how the text is rendered.
 * \sa SetFont(), os::Font, SetDrawingMode(), SetFgColor(), SetBgColor()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void View::DrawText( const Rect& cPos, const String& cString, uint32 nFlags )
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		return;
	}
	GRndDrawText_s *psCmd = static_cast < GRndDrawText_s * >( pcWindow->_AllocRenderCmd( DRC_DRAW_TEXT, this,
			sizeof( GRndDrawText_s ) + cString.size() - 1 ) );

	if( psCmd != NULL )
	{
		psCmd->nLength = cString.size();
		psCmd->nFlags = nFlags;
		psCmd->cPos = cPos;
		memcpy( psCmd->zString, cString.c_str(), psCmd->nLength );
	}
	pcWindow->_PutRenderCmd();	
}

void View::DrawSelectedText( const Rect& cPos, const String& cString, const IPoint& cSel1, const IPoint& cSel2, uint32 nMode, uint32 nFlags )
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
	{
		return;
	}
	GRndDrawSelectedText_s *psCmd = static_cast < GRndDrawSelectedText_s * >( pcWindow->_AllocRenderCmd( DRC_DRAW_SELECTED_TEXT, this,
			sizeof( GRndDrawSelectedText_s ) + cString.size() - 1 ) );

	if( psCmd != NULL )
	{
		psCmd->nLength = cString.size();
		psCmd->nFlags = nFlags;
		psCmd->cPos = cPos;
		psCmd->cSel1 = cSel1;
		psCmd->cSel2 = cSel2;
		psCmd->nMode = nMode;
		memcpy( psCmd->zString, cString.c_str(), psCmd->nLength );
	}
	pcWindow->_PutRenderCmd();	
}

void View::GetSelection( const String &cClipboard )
{
	Window *pcWindow = GetWindow();

	if( pcWindow == NULL )
		return;

	GRndGetSelection_s *psCmd = static_cast < GRndGetSelection_s * >( pcWindow->_AllocRenderCmd( DRC_GET_SELECTION, this, sizeof( GRndGetSelection_s ) ) );
	if( psCmd != NULL )
		strncpy( psCmd->m_zName, cClipboard.c_str(), 64 );

	pcWindow->_PutRenderCmd();	
}

void View::DrawString( const Point & cPos, const String& cString )
{
	MovePenTo( cPos );
	DrawString( cString );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SetDrawingMode( drawing_mode eMode )
{
	if( eMode != m->m_eDrawingMode )
	{
		Window *pcWnd = GetWindow();

		m->m_eDrawingMode = eMode;

		if( pcWnd == NULL )
		{
			return;
		}
		GRndSetDrawingMode_s *psMsg;

		psMsg = static_cast < GRndSetDrawingMode_s * >( pcWnd->_AllocRenderCmd( DRC_SET_DRAWING_MODE, this, sizeof( GRndSetDrawingMode_s ) ) );
		if( psMsg != NULL )
		{
			psMsg->nDrawingMode = eMode;
		}
		pcWnd->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

drawing_mode View::GetDrawingMode() const
{
	return ( m->m_eDrawingMode );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SetFgColor( int nRed, int nGreen, int nBlue, int nAlpha )
{
	SetFgColor( Color32_s( nRed, nGreen, nBlue, nAlpha ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SetFgColor( Color32_s sColor )
{
	Window *pcWindow = GetWindow();

	m->m_sFgColor = sColor;

	if( pcWindow != NULL )
	{
		GRndSetColor32_s *psCmd = static_cast < GRndSetColor32_s * >( pcWindow->_AllocRenderCmd( DRC_SET_COLOR32, this, sizeof( GRndSetColor32_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->nWhichPen = PEN_HIGH;
			psCmd->sColor = sColor;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SetBgColor( int nRed, int nGreen, int nBlue, int nAlpha )
{
	SetBgColor( Color32_s( nRed, nGreen, nBlue, nAlpha ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SetBgColor( Color32_s sColor )
{
	Window *pcWindow = GetWindow();

	m->m_sBgColor = sColor;

	if( pcWindow != NULL )
	{
		GRndSetColor32_s *psCmd = static_cast < GRndSetColor32_s * >( pcWindow->_AllocRenderCmd( DRC_SET_COLOR32, this, sizeof( GRndSetColor32_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->nWhichPen = PEN_LOW;
			psCmd->sColor = sColor;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SetEraseColor( int nRed, int nGreen, int nBlue, int nAlpha )
{
	SetEraseColor( Color32_s( nRed, nGreen, nBlue, nAlpha ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SetEraseColor( Color32_s sColor )
{
	Window *pcWindow = GetWindow();

	m->m_sEraseColor = sColor;


	if( pcWindow != NULL )
	{
		GRndSetColor32_s *psCmd = static_cast < GRndSetColor32_s * >( pcWindow->_AllocRenderCmd( DRC_SET_COLOR32, this, sizeof( GRndSetColor32_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->nWhichPen = PEN_ERASE;
			psCmd->sColor = sColor;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Color32_s View::GetFgColor() const
{
	return ( m->m_sFgColor );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Color32_s View::GetBgColor() const
{
	return ( m->m_sBgColor );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Color32_s View::GetEraseColor() const
{
	return ( m->m_sEraseColor );
}

void View::_ReleaseFont()
{
	m->m_pcFont->Release();
}

/** Change the views text font.
 * \par Description:
 *	Replace the font used when rendering text and notify the view
 *	itself about the change by calling the View::FontChanged()
 *	hook function.
 * \par
 *	Font's are reference counted and SetFont() will call
 *	Font::AddRef() on the new font and Font::Release() on the old
 *	font. This means that you retain ownership on the font even
 *	after View::SetFont() returns. You must therefor normally call
 *	Font::Release() on the font after setting it.
 * \par
 *	The view will be affected by changes applied to the font after
 *	it is set. If someone change the properties of the font later
 *	the view will again be notified through the View::FontChanged()
 *	hook
 *
 * \param pcFont
 *	Pointer to the new font. It's reference count will be
 *	increased by one.
 * 
 * \sa DrawString(), os::Font
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::SetFont( Font * pcFont )
{
	Window *pcWindow = GetWindow();

	if( pcFont == m->m_pcFont )
	{
		return;
	}
	if( m->m_pcFont != NULL )
	{
		_ReleaseFont();
	}
	if( NULL != pcFont )
	{
		m->m_pcFont = pcFont;
		m->m_pcFont->AddRef();
	}
	else
	{
		m->m_pcFont = new Font();
		if( m->m_pcFont->SetFamilyAndStyle( SYS_FONT_FAMILY, SYS_FONT_PLAIN ) == 0 )
		{
			m->m_pcFont->SetProperties( 8.0f, 0.0f, 0.0f );
		}
	}

	if( pcWindow != NULL )
	{
		GRndSetFont_s *psCmd = static_cast < GRndSetFont_s * >( pcWindow->_AllocRenderCmd( DRC_SET_FONT, this, sizeof( GRndSetFont_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->hFontID = m->m_pcFont->GetFontID();
		}
		pcWindow->_PutRenderCmd();
	}
	FontChanged( m->m_pcFont );
}

Font *View::GetFont() const
{
	return ( m->m_pcFont );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::DrawLine( const Point & cFromPnt, const Point & cToPnt )
{
	Window *pcWindow = GetWindow();

	MovePenTo( cFromPnt );

	if( pcWindow != NULL )
	{
		GRndLine32_s *psCmd = static_cast < GRndLine32_s * >( pcWindow->_AllocRenderCmd( DRC_LINE32, this, sizeof( GRndLine32_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->sToPos = cToPnt;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::DrawLine( const Point & cToPnt )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndLine32_s *psCmd = static_cast < GRndLine32_s * >( pcWindow->_AllocRenderCmd( DRC_LINE32, this, sizeof( GRndLine32_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->sToPos = cToPnt;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::FillRect( const Rect & cRect )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndFillRect32_s *psCmd = static_cast < GRndFillRect32_s * >( pcWindow->_AllocRenderCmd( DRC_FILL_RECT32, this, sizeof( GRndFillRect32_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->sRect = cRect;
			psCmd->sColor = m->m_sFgColor;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::FillRect( const Rect & cRect, Color32_s sColor )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndFillRect32_s *psCmd = static_cast < GRndFillRect32_s * >( pcWindow->_AllocRenderCmd( DRC_FILL_RECT32, this, sizeof( GRndFillRect32_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->sRect = cRect;
			psCmd->sColor = sColor;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::EraseRect( const Rect & cRect )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndFillRect32_s *psCmd = static_cast < GRndFillRect32_s * >( pcWindow->_AllocRenderCmd( DRC_FILL_RECT32, this, sizeof( GRndFillRect32_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->sRect = cRect;
			psCmd->sColor = m->m_sEraseColor;
		}
		pcWindow->_PutRenderCmd();
	}
}

/** Render a bitmap into the view.
 * \par Description:
 *	DrawBitmap() copy a region of a bitmap into the view performing
 *	any required color space conversion.
 * \par
 *	How the bitmap is rendered depends on the drawing mode and the
 *	format of the source bitmap.
 * \par
 *	The following drawing modes are suppoted by DrawBitmap():
 * \par
 *	DM_COPY:<dl><dd>
 *		Copy the content of the bitmap directly into the view.
 *		Only color space conversion is performed (when needed). </dl>
 * 	DM_OVER:<dl><dd>
 *		Almost like DM_COPY but pixels with the value TRANSPARENT_CMAP8
 *		in CS_CMAP8 bitmaps or TRANSPARENT_RGB32 in CS_RGB32 bitmaps
 *		will not be copyed.</dl>
 * DM_BLEND:<dl><dd>
 *		Blend the bitmap into the original content of the view based
 *		on the bitmaps alpha channel. This mode only works for CS_RGB32
 *		bitmaps.</dl>
 *
 * \param pcBitmap
 *	The bitmap to render. Only CS_CMAP8, CS_RGB15, CS_RGB16, and CS_RGB32
 *	type bitmaps are currently supported.
 * \param cSrcRect
 *	The source rectangle. Only the area described by this rectangle will
 *	be copyed into the view.
 * \param cDstRect
 *	Destination rectangle.
 * \sa Bitmap
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::DrawBitmap( const Bitmap * pcBitmap, const Rect & cSrcRect, const Rect & cDstRect )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndDrawBitmap_s *psCmd = static_cast < GRndDrawBitmap_s * >( pcWindow->_AllocRenderCmd( DRC_DRAW_BITMAP, this, sizeof( GRndDrawBitmap_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->hBitmapToken = pcBitmap->m_hHandle;
			psCmd->cSrcRect = cSrcRect;
			psCmd->cDstRect = cDstRect;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::DrawFrame( const Rect & a_cRect, uint32 nStyle )
{
	Rect cRect( a_cRect );

	Color32_s fg_save = GetFgColor();
	Color32_s bg_save = GetBgColor();
	
	cRect.Floor();
	bool bSunken = false;

	if( ( ( nStyle & FRAME_RAISED ) == 0 ) && ( nStyle & ( FRAME_RECESSED ) ) )
	{
		bSunken = true;
	}

	Color32_s sFgCol = get_default_color( COL_SHINE );
	Color32_s sBgCol = get_default_color( COL_SHADOW );

	if( nStyle & FRAME_DISABLED )
	{
		sFgCol = Tint( sFgCol, 0.6f );
		sBgCol = Tint( sBgCol, 0.4f );
	}
	Color32_s sFgShadowCol = Tint( sFgCol, 0.6f );
	Color32_s sBgShadowCol = Tint( sBgCol, 0.5f );

	if( nStyle & FRAME_FLAT )
	{
		SetFgColor( ( bSunken ) ? sBgCol : sFgCol );
		MovePenTo( cRect.left, cRect.bottom );
		DrawLine( Point( cRect.left, cRect.top ) );
		DrawLine( Point( cRect.right, cRect.top ) );
		DrawLine( Point( cRect.right, cRect.bottom ) );
		DrawLine( Point( cRect.left, cRect.bottom ) );
	}
	else
	{
		if( nStyle & FRAME_THIN )
		{
			SetFgColor( ( bSunken ) ? sBgCol : sFgCol );
		}
		else
		{
			SetFgColor( ( bSunken ) ? sBgCol : sBgShadowCol );
		}

		MovePenTo( cRect.left, cRect.bottom );
		DrawLine( Point( cRect.left, cRect.top ) );
		DrawLine( Point( cRect.right, cRect.top ) );

		if( nStyle & FRAME_THIN )
		{
			SetFgColor( ( bSunken ) ? sFgCol : sBgCol );
		}
		else
		{
			SetFgColor( ( bSunken ) ? sBgCol : sBgShadowCol );
		}
		DrawLine( Point( cRect.right, cRect.bottom ) );
		DrawLine( Point( cRect.left, cRect.bottom ) );


		if( ( nStyle & FRAME_THIN ) == 0 )
		{
			if( nStyle & FRAME_ETCHED )
			{
				SetFgColor( ( bSunken ) ? sBgCol : sFgCol );

				MovePenTo( cRect.left + 1.0f, cRect.bottom - 1.0f );

				DrawLine( Point( cRect.left + 1.0f, cRect.top + 1.0f ) );
				DrawLine( Point( cRect.right - 1.0f, cRect.top + 1.0f ) );

				SetFgColor( ( bSunken ) ? sFgCol : sBgCol );

				DrawLine( Point( cRect.right - 1.0f, cRect.bottom - 1.0f ) );
				DrawLine( Point( cRect.left + 1.0f, cRect.bottom - 1.0f ) );
			}
			else
			{
				SetFgColor( ( bSunken ) ? sBgShadowCol : sFgCol );

				MovePenTo( cRect.left + 1.0f, cRect.bottom - 1.0f );

				DrawLine( Point( cRect.left + 1.0f, cRect.top + 1.0f ) );
				DrawLine( Point( cRect.right - 1.0f, cRect.top + 1.0f ) );

				SetFgColor( ( bSunken ) ? sFgShadowCol : sBgCol );

				MovePenTo( Point( cRect.right - 1.0f, cRect.top + 2.0f ) );
				DrawLine( Point( cRect.right - 1.0f, cRect.bottom - 1.0f ) );
				DrawLine( Point( cRect.left + 2.0f, cRect.bottom - 1.0f ) );
			}
			if( ( nStyle & FRAME_TRANSPARENT ) == 0 )
			{

				EraseRect( Rect( cRect.left + 2.0f, cRect.top + 2.0f, cRect.right - 2.0f, cRect.bottom - 2.0f ) );
			}
		}
		else
		{
			if( ( nStyle & FRAME_TRANSPARENT ) == 0 )
			{
				EraseRect( Rect( cRect.left + 1.0f, cRect.top + 1.0f, cRect.right - 1.0f, cRect.bottom - 1.0f ) );
			}
		}
	}

	SetFgColor( fg_save );
	SetBgColor( bg_save );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ScrollBy( const Point & cDelta )
{
	if( cDelta.x || cDelta.y )
	{
		m->m_cScrollOffset += cDelta;
		Window *pcWindow = GetWindow();

		if( pcWindow != NULL )
		{
			GRndScrollView_s *psCmd = ( GRndScrollView_s * ) pcWindow->_AllocRenderCmd( DRC_SCROLL_VIEW, this,
				sizeof( GRndScrollView_s ) );

			if( psCmd != NULL )
			{
				psCmd->cDelta = cDelta;
			}
			pcWindow->_PutRenderCmd();
		}
		if( cDelta.x != 0 && m->m_pcHScrollBar != NULL )
		{
			m->m_pcHScrollBar->SetValue( -m->m_cScrollOffset.x );
		}
		if( cDelta.y != 0 && m->m_pcVScrollBar != NULL )
		{
			m->m_pcVScrollBar->SetValue( -m->m_cScrollOffset.y );
		}
		ViewScrolled( cDelta );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ScrollTo( Point cTopLeft )
{
	ScrollBy( cTopLeft - m->m_cScrollOffset );
}

/** Copy a rectangle from one location to another within the view.
 * \par Description:
 *	ScrollRect() will copy the source rectangle to the destination
 *	rectangle using the blitter. If parts of the source rectangle
 *	is obscured (by another window/view or the screen edge) and
 *	the same area in the destination area is not that area in the
 *	destination rectangle will be invalidated (see Invalidate()).
 *
 * \par Note:
 *	Scrolling an area within the view might affect the current
 *	"damage-list" of the view and cause a repaint message to be
 *	sendt to the view. If ScrollRect() is called repeatadly and
 *	the application for some reason can't handle the generated
 *	repaint messages fast enough more and more damage-rectangles
 *	will accumulate in the views damage list slowing things
 *	further down until the appserver is broght to a grinding halt.
 *	To avoid this situation the next paint message received after
 *	a call to ScrollRect() will cause Sync() instead of Flush() to
 *	be called when Paint() returns to syncronize the application
 *	with the appserver. So even though the ScrollRect() member
 *	itself is asyncronous it might cause Sync() to be called and
 *	will have the performance implications mentioned in the
 *	documentation of Sync()
 * 
 * \param cSrcRect
 *	The source rectangle in the views coordinate system.
 * \param cSrcRect
 *	The destination rectangle in the views coordinate system.
 *	This rectangle should have the same size but a difference
 *	position than the \p cSrcRect. In a future version it might be
 *	possible to scale the rectangle by using a different size so
 *	make sure they don't differ or you might get a surprice
 *	some day.
 *
 * \sa ScrollTo(), ScrollBy(), Sync()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


void View::ScrollRect( const Rect & cSrcRect, const Rect & cDstRect )
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		GRndCopyRect_s *psCmd = static_cast < GRndCopyRect_s * >( pcWindow->_AllocRenderCmd( DRC_COPY_RECT, this, sizeof( GRndCopyRect_s ) ) );

		if( psCmd != NULL )
		{
			psCmd->cSrcRect = cSrcRect;
			psCmd->cDstRect = cDstRect;
		}
		pcWindow->_PutRenderCmd();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float View::GetStringWidth( const String & cString ) const
{
	if( m->m_pcFont == NULL )
	{
		dbprintf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return ( 0 );
	}
	return ( m->m_pcFont->GetStringWidth( cString ) );
}

float View::GetStringWidth( const char* pzString, int nLen ) const
{
	if( m->m_pcFont == NULL )
	{
		dbprintf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return ( 0 );
	}
	return ( m->m_pcFont->GetStringWidth( pzString, nLen ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::GetStringWidths( const char *apzStringArray[], const int *anLengthArray, int nStringCount, float *avWidthArray ) const
{
	if( m->m_pcFont == NULL )
	{
		dbprintf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return;
	}
	m->m_pcFont->GetStringWidths( apzStringArray, anLengthArray, nStringCount, avWidthArray );
}

Point View::GetTextExtent( const String & cString, uint32 nFlags, int nTargetWidth ) const
{
	if( m->m_pcFont == NULL )
	{
		dbprintf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return Point();
	}
	return ( m->m_pcFont->GetTextExtent( cString, nFlags, nTargetWidth ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int View::GetStringLength( const String & cString, float vWidth, bool bIncludeLast ) const
{
	if( m->m_pcFont == NULL )
	{
		dbprintf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return ( 0 );
	}
	return ( m->m_pcFont->GetStringLength( cString, vWidth, bIncludeLast ) );
}

int View::GetStringLength( const char* pzString, int nLen, float vWidth, bool bIncludeLast ) const
{
	if( m->m_pcFont == NULL )
	{
		dbprintf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return ( 0 );
	}
	return ( m->m_pcFont->GetStringLength( pzString, nLen, vWidth, bIncludeLast ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::GetStringLengths( const char **apzStringArray, const int *anLengthArray, int nStringCount, float vWidth, int *anMaxLengthArray, bool bIncludeLast ) const
{
	if( m->m_pcFont == NULL )
	{
		dbprintf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return;
	}
	m->m_pcFont->GetStringLengths( apzStringArray, anLengthArray, nStringCount, vWidth, anMaxLengthArray, bIncludeLast );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::GetFontHeight( font_height * psHeight ) const
{
	if( m->m_pcFont == NULL )
	{
		dbprintf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return;
	}
	m->m_pcFont->GetHeight( psHeight );
}

/** @name ConvertToParent
 * \brief Convert a point or rectangle from local to parent coordinate system
 * \par Description:
 *	Convert a point or rectangle from local to parent coordinate system
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 */
//@{

/** Translate a os::Point into our parents coordinate system. Returning the the result */

Point View::ConvertToParent( const Point & cPoint ) const
{
	return ( cPoint + GetLeftTop() + m->m_cScrollOffset );
}

/** Translate a os::Point into our parents coordinate system. Modifying the passet Point object */

void View::ConvertToParent( Point * pcPoint ) const
{
	*pcPoint += GetLeftTop() + m->m_cScrollOffset;
}

Rect View::ConvertToParent( const Rect & cRect ) const
{
	return ( cRect + GetLeftTop() + m->m_cScrollOffset );
}


void View::ConvertToParent( Rect * pcRect ) const
{
	*pcRect += GetLeftTop() + m->m_cScrollOffset;
}

//@}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::ConvertFromParent( const Point & cPoint ) const
{
	return ( cPoint - GetLeftTop() - m->m_cScrollOffset );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromParent( Point * pcPoint ) const
{
	*pcPoint -= GetLeftTop() + m->m_cScrollOffset;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::ConvertFromParent( const Rect & cRect ) const
{
	return ( cRect - GetLeftTop() - m->m_cScrollOffset );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromParent( Rect * pcRect ) const
{
	*pcRect -= GetLeftTop() + m->m_cScrollOffset;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::ConvertToWindow( const Point & cPoint ) const
{
	if( m->m_pcParent != NULL )
	{
		return ( m->m_pcParent->ConvertToWindow( cPoint + GetLeftTop() + m->m_cScrollOffset ) );
	}
	else
	{
		return ( cPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertToWindow( Point * pcPoint ) const
{
	if( m->m_pcParent != NULL )
	{
		*pcPoint += GetLeftTop() + m->m_cScrollOffset;
		m->m_pcParent->ConvertToWindow( pcPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::ConvertToWindow( const Rect & cRect ) const
{
	if( m->m_pcParent != NULL )
	{
		return ( m->m_pcParent->ConvertToWindow( cRect + GetLeftTop() + m->m_cScrollOffset ) );
	}
	else
	{
		return ( cRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertToWindow( Rect * pcRect ) const
{
	if( m->m_pcParent != NULL )
	{
		*pcRect += GetLeftTop() + m->m_cScrollOffset;
		m->m_pcParent->ConvertToWindow( pcRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::ConvertFromWindow( const Point & cPoint ) const
{
	if( m->m_pcParent != NULL )
	{
		return ( m->m_pcParent->ConvertFromWindow( cPoint - GetLeftTop() - m->m_cScrollOffset ) );
	}
	else
	{
		return ( cPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromWindow( Point * pcPoint ) const
{
	if( m->m_pcParent != NULL )
	{
		*pcPoint -= GetLeftTop() + m->m_cScrollOffset;
		m->m_pcParent->ConvertFromWindow( pcPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::ConvertFromWindow( const Rect & cRect ) const
{
	if( m->m_pcParent != NULL )
	{
		return ( m->m_pcParent->ConvertFromWindow( cRect - GetLeftTop() - m->m_cScrollOffset ) );
	}
	else
	{
		return ( cRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromWindow( Rect * pcRect ) const
{
	if( m->m_pcParent != NULL )
	{
		*pcRect -= GetLeftTop() + m->m_cScrollOffset;
		m->m_pcParent->ConvertFromWindow( pcRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::ConvertToScreen( const Point & cPoint ) const
{
	if( m->m_pcParent != NULL )
	{
		return ( m->m_pcParent->ConvertToScreen( cPoint + GetLeftTop() + m->m_cScrollOffset ) );
	}
	else
	{
		return ( cPoint + GetLeftTop() + m->m_cScrollOffset );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertToScreen( Point * pcPoint ) const
{
	*pcPoint += GetLeftTop() + m->m_cScrollOffset;

	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->ConvertToScreen( pcPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::ConvertToScreen( const Rect & cRect ) const
{
	if( m->m_pcParent != NULL )
	{
		return ( m->m_pcParent->ConvertToScreen( cRect + GetLeftTop() + m->m_cScrollOffset ) );
	}
	else
	{
		return ( cRect + GetLeftTop() );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertToScreen( Rect * pcRect ) const
{
	*pcRect += GetLeftTop() + m->m_cScrollOffset;

	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->ConvertToScreen( pcRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::ConvertFromScreen( const Point & cPoint ) const
{
	if( m->m_pcParent != NULL )
	{
		return ( m->m_pcParent->ConvertFromScreen( cPoint - GetLeftTop() - m->m_cScrollOffset ) );
	}
	else
	{
		return ( cPoint - GetLeftTop() );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromScreen( Point * pcPoint ) const
{
	*pcPoint -= GetLeftTop() + m->m_cScrollOffset;

	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->ConvertFromScreen( pcPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::ConvertFromScreen( const Rect & cRect ) const
{
	if( m->m_pcParent != NULL )
	{
		return ( m->m_pcParent->ConvertFromScreen( cRect - GetLeftTop() - m->m_cScrollOffset ) );
	}
	else
	{
		return ( cRect - GetLeftTop() );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromScreen( Rect * pcRect ) const
{
	*pcRect -= GetLeftTop() + m->m_cScrollOffset;

	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->ConvertFromScreen( pcRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::GetFrame() const
{
	return ( m->m_cFrame );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::GetBounds() const
{
	return ( m->m_cFrame - Point( m->m_cFrame.left, m->m_cFrame.top ) - m->m_cScrollOffset );
}

Rect View::GetNormalizedBounds() const
{
	return ( m->m_cFrame.Bounds() );
}

void View::_ConstrictRectangle( Rect * pcRect, const Point & cOffset )
{
	Point cOff = cOffset - Point( m->m_cFrame.left, m->m_cFrame.top ) - m->m_cScrollOffset;

	*pcRect &= m->m_cFrame + cOff;
	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->_ConstrictRectangle( pcRect, cOff );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float View::Width() const
{
	return ( m->m_cFrame.Width() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float View::Height() const
{
	return ( m->m_cFrame.Height() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::GetLeftTop() const
{
	return ( Point( m->m_cFrame.left, m->m_cFrame.top ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

String View::GetTitle() const
{
	return ( m->m_cTitle );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::AttachedToWindow()
{
	if( m->m_cKey.IsValid() ) {
		GetWindow()->AddShortcut( m->m_cKey, this );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::AllAttached()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::DetachedFromWindow()
{
	if( m->m_cKey.IsValid() ) {
		GetWindow()->RemoveShortcut( m->m_cKey );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::AllDetached()
{
}

/** Hook called when the view gain or loose focus.
 * \par Description:
 *	This is a callback member that can be overloaded by derived classes to
 *	learn when the view is activated and when it is deactivated. The
 *	bIsActive parameter tell whether the focus was lost or gained.
 * \par
 *	The view has focus when it is the active view in the active window.
 * \par Note:
 *	This is a hook function that is called by the system to notify about
 *	an event. You should never call this member yourself.
 *
 *	The window is locked when this member is called.
 * \param bIsActive - true if the view gain and false if it loose focus.
 * \sa MakeFocus(), WindowActivated()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


void View::Activated( bool bIsActive )
{
}

/** Hook called when the window hosting this view gain or loose focus.
 * \par Description:
 *	This is a callback member that can be overloaded by derived classes to
 *	learn when the window that this view is hosted by is activated and when
 *	it is deactivated. The bIsActive parameter tell whether the focus was
 *	lost or gained. This member is called whenever the window changes focus
 *	independent of whether the view is active or not.
 *	
 * \par Note:
 *	This is a hook function that is called by the system to notify about
 *	an event. You should never call this member yourself.
 *
 *	The window is locked when this member is called.
 * \param bIsActive - true if the window gain and false if it loose focus.
 * \sa MakeFocus(), Activated()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::WindowActivated( bool bIsActive )
{
}

/** Called to notify the view that the font has changed
 * \par Description:
 *	FontChanged() is a virtual hook function that can be
 *	overloaded by inheriting classes to track changes to the
 *	view's font.
 * \par
 *	This hook function is called whenver the font is replaced
 *	through the View::SetFont() member or if the currently
 *	assigned font is modified in a way that whould alter the
 *	graphical appearance of the font.
 *
 * \par Note:
 *	View::SetFont() will call FontChanged() syncronously and will
 *	cause FontChanged() to be called even if the view is not yet
 *	added to a window. Changes done to the font-object cause a
 *	message to be sendt to the window thread and FontChanged()
 *	will then be called asyncronously from the window thread when
 *	the message arrive. For this reason it is only possible to
 *	track changes done to the font object itself when the view is
 *	added to a window.
 *	
 * \param pcNewFont
 *	Pointer to the affected font (same as returned by GetFont()).
 *
 * \sa SetFont(), os::Font
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void View::FontChanged( Font * pcNewFont )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::Ping( int nSize ) const
{
	Window *pcWindow = GetWindow();

	if( pcWindow != NULL )
	{
		pcWindow->_AllocRenderCmd( DRC_PING, this, sizeof( GRndHeader_s ) + nSize );
		pcWindow->_PutRenderCmd();
	}
}

/** \internal
 *****************************************************************************/

int View::_GetHandle() const
{
	return ( m->m_hViewHandle );
}

/** \internal
 *****************************************************************************/

void View::_SetMouseMoveRun( int nValue )
{
	m->m_nMouseMoveRun = nValue;
}

/** \internal
 *****************************************************************************/

int View::_GetMouseMoveRun() const
{
	return ( m->m_nMouseMoveRun );
}

/** \internal
 *****************************************************************************/

void View::_SetMouseMode( int nMode )
{
	m->m_nMouseMode = nMode;
}

/** \internal
 *****************************************************************************/

int View::_GetMouseMode() const
{
	return ( m->m_nMouseMode );
}

/** \internal
 *****************************************************************************/

void View::_SetHScrollBar( ScrollBar * pcScrollBar )
{
	m->m_pcHScrollBar = pcScrollBar;
}

/** \internal
 *****************************************************************************/

void View::_SetVScrollBar( ScrollBar * pcScrollBar )
{
	m->m_pcVScrollBar = pcScrollBar;
}

void View::__VW_reserved2__()
{
}
void View::__VW_reserved3__()
{
}
void View::__VW_reserved4__()
{
}
void View::__VW_reserved5__()
{
}
void View::__VW_reserved6__()
{
}
void View::__VW_reserved7__()
{
}
void View::__VW_reserved8__()
{
}
void View::__VW_reserved9__()
{
}
void View::__VW_reserved10__()
{
}
void View::__VW_reserved11__()
{
}
void View::__VW_reserved12__()
{
}
void View::__VW_reserved13__()
{
}
void View::__VW_reserved14__()
{
}
void View::__VW_reserved15__()
{
}
void View::__VW_reserved16__()
{
}
void View::__VW_reserved17__()
{
}
void View::__VW_reserved18__()
{
}
void View::__VW_reserved19__()
{
}
void View::__VW_reserved20__()
{
}


