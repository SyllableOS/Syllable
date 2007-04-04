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

#include <assert.h>
#include <stdio.h>

#include <atheos/types.h>

#include <appserver/protocol.h>
#include <util/application.h>
#include <util/message.h>
#include <gui/window.h>
#include <gui/bitmap.h>
#include <gui/menu.h>
#include <gui/exceptions.h>
#include <macros.h>
#include <gui/desktop.h>
#include <util/shortcutkey.h>


using namespace os;

enum
{ FOCUS_STACK_SIZE = 8 };

class TopView:public View
{
	public:
	TopView( const Rect & cFrame, Window * pcWindow );

	virtual void FrameMoved( const Point & cDelta );
	virtual void FrameSized( const Point & cDelta );

	virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
	
	bool m_bKeyDownRejected;

	private:
	Window * m_pcWindow;
};

enum ShortcutDataFlags {
	SDF_DELETE = 1,
	SDF_MESSAGE = 2,
	SDF_VIEW = 4
};

struct ShortcutData {
	uint32		m_nFlags;
	union {
		Message*	m_pcMessage;
		View*		m_pcView;
	};

	ShortcutData( Message* pcMessage ) {
		m_pcMessage = pcMessage;
		m_nFlags = SDF_DELETE | SDF_MESSAGE;
	}
	ShortcutData( View* pcView ) {
		m_pcView = pcView;
		m_nFlags = SDF_VIEW;
	}
	ShortcutData() {
		m_pcMessage = NULL;
		m_nFlags = 0;
	}
};

typedef std::map<ShortcutKey, ShortcutData> shortcut_map;

class Window::Private
{
      public:
	Private( const String & cTitle ):m_cTitle( cTitle ),m_cRenderLock( "render_lock" )
	{
		m_psRenderPkt = NULL;
		m_nRndBufSize = RENDER_BUFFER_SIZE;
		m_nFlags = 0;
		m_hLayer = -1;
		m_hLayerPort = -1;
		m_hReplyPort = -1;
		m_bIsActive = false;
		m_nButtons = 0;
		m_pcTopView = NULL;
		m_pcLastMouseView = NULL;
		m_pcMenuBar = NULL;
		m_bMenuOpen = false;
		m_nMouseMoveRun = 1;
		m_nNextTabOrder = 0;
		m_pcDefaultButton = NULL;
		m_pcDefaultWheelView = NULL;
		m_bDidScrollRect = false;
		memset( m_apcFocusStack, 0, FOCUS_STACK_SIZE * sizeof( View * ) );
	}

	String m_cTitle;
	WR_Render_s *m_psRenderPkt;
	uint32 m_nRndBufSize;

	uint32 m_nFlags;
	int m_nMouseMoveRun;
	int32 m_nMouseTransition;
	int m_hLayer;
	int m_nNextTabOrder;
	port_id m_hLayerPort;
	port_id m_hReplyPort;	// We direct replies to us to this port

	bool m_bIsActive;
	uint32 m_nButtons;
	Point m_cMousePos;

	TopView *m_pcTopView;
	View *m_pcLastMouseView;
	View *m_apcFocusStack[FOCUS_STACK_SIZE];
	View *m_pcDefaultButton;
	View *m_pcDefaultWheelView;
	Menu *m_pcMenuBar;
	bool m_bMenuOpen;
	bool m_bIsRunning;
	bool m_bDidScrollRect;
	
	os::Locker m_cRenderLock;

	shortcut_map	m_cShortcuts;

};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Window::_Cleanup()
{
	shortcut_map::iterator i;
	for( i = m->m_cShortcuts.begin(); i != m->m_cShortcuts.end(); i++ ) {
		if( ( (*i).second.m_nFlags & SDF_DELETE ) && ( (*i).second.m_pcMessage ) ) {
			delete (*i).second.m_pcMessage;
		}
		m->m_cShortcuts.erase(i);
	}
	if( m->m_pcTopView != NULL )
	{
		Message cReq( AR_CLOSE_WINDOW );
		Message cReply;

		cReq.AddInt32( "top_view", m->m_pcTopView->_GetHandle() );

		m->m_pcTopView->_Detached( true, 0 );
		delete m->m_pcTopView;

		if( Messenger( m->m_hLayerPort ).SendMessage( &cReq, &cReply ) < 0 )
		{
			dbprintf( "Error: Window::_Cleanup() failed to send AR_CLOSE_WINDOW request to server\n" );
		}
	}
	delete m->m_psRenderPkt;
	
	
	if( m->m_hReplyPort >= 0 )
	{
		delete_port( m->m_hReplyPort );
	}
	Application::GetInstance()->RemoveWindow( this );

	delete m;
}

/** Initialize the window.
 * \par Description:
 *	The window contructor initialize the local object, and
 *	initiate the windows connection to the appserver. The window
 *	is invisible and locked when the contructor returns. The first
 *	call to Show() will unlock the widow, start the looper thread
 *	and make it visible. If you want the windows looper to start
 *	handling messages before the window is made visible, you can
 *	call Start(). This will unlock the window, and start the
 *	looper thread without making the window visible. Simply
 *	calling Run()/Unlock() will not work, since the internal state
 *	of the Window will not be updated.
 * \param cFrame - The size and position of the window.
 * \sa Show(), Start(), Looper::Lock(), Looper::Unlock()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Window::Window( const Rect & cFrame, const String & cName, const String & cTitle, uint32 nFlags, uint32 nDesktopMask ):Looper( cName, DISPLAY_PRIORITY )	//, m_cTitle( cTitle )
{
	assert( Application::GetInstance() != NULL );
//    Init();
	m = new Private( cTitle );

	m->m_nFlags = nFlags;
	m->m_bIsRunning = false;

	try
	{
		m->m_hReplyPort = create_port( "window_reply", DEFAULT_PORT_SIZE );
		if( m->m_hReplyPort < 0 )
		{
			throw( GeneralFailure( "Failed to create message port", ENOMEM ) );
		}

		Rect cWndBounds = cFrame.Bounds();

		m->m_pcTopView = new TopView( cFrame, this );


		int hTopView;

		m->m_hLayerPort = Application::GetInstance()->CreateWindow( m->m_pcTopView, cFrame, cTitle, nFlags, nDesktopMask, GetMsgPort(  ), &hTopView );

		if( m->m_hLayerPort < 0 )
		{
			throw( GeneralFailure( "Failed to open window", ENOMEM ) );
		}
		m->m_psRenderPkt = new WR_Render_s;
		m->m_psRenderPkt->m_hTopView = hTopView;
		m->m_psRenderPkt->nCount = 0;
		m->m_nRndBufSize = 0;
		m->m_pcMenuBar = NULL;

		m->m_pcTopView->_Attached( this, NULL, hTopView, 1 );
		Application::GetInstance()->AddWindow( this );
//      Lock();
	}
	catch( ... )
	{
		_Cleanup();
		throw;
	}
}

/** Construct a window attached to a Bitmap
 * \par Description:
 *	This constructor is used internaly by the GUI tool-kit to create
 *	a window to handle rendering in a bitmap.
 * \param pcBitmap - The bitmap to which the window will be atached.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Window::Window( Bitmap * pcBitmap ):Looper( "bitmap", DISPLAY_PRIORITY )
{
	assert( Application::GetInstance() != NULL );

//    Init();
	m = new Private( "" );

	m->m_nFlags = WND_NO_BORDER;

	try
	{
		m->m_hReplyPort = create_port( "bwindow_reply", DEFAULT_PORT_SIZE );

		Rect cWndBounds = pcBitmap->GetBounds();

		m->m_pcTopView = new TopView( cWndBounds, this );

		int hTopView;

		m->m_hLayerPort = Application::GetInstance()->CreateWindow( m->m_pcTopView, pcBitmap->m_hHandle, &hTopView );

		m->m_psRenderPkt = new WR_Render_s;
		m->m_psRenderPkt->m_hTopView = hTopView;
		m->m_psRenderPkt->nCount = 0;
		m->m_nRndBufSize = 0;

		m->m_pcTopView->_Attached( this, NULL, hTopView, 0 );
		Application::GetInstance()->AddWindow( this );
	}
	catch( ... )
	{
		_Cleanup();
		throw;
	}
}

/** Destruct a window.
 * \par Description:
 *	The window destructor will delete all View's still atached to
 *	it, and shut down the connection to the appserver.
 * \sa Window::Window()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Window::~Window()
{
	_Cleanup();
}

void Window::SetFlags( uint32 nFlags )
{
	Flush();
	if( m->m_hLayerPort >= 0 )
	{
		Message cReq( WR_SET_FLAGS );

		cReq.AddInt32( "flags", nFlags );
		if( Messenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			dbprintf( "Error: Window::SetFlags() failed to send request to server\n" );
		}
		else
		{
			m->m_nFlags = nFlags;
		}
	}
}

uint32 Window::GetFlags() const
{
	return ( m->m_nFlags );
}

/** Assign a default button.
 * \par Description:
 *	Set the default button. The default button will receive KeyDown()/KeyUp()
 *	events generated by the <ENTER> key even when not having focus. Since the
 *	os::Button class will invoke itself when the <ENTER> key is hit, it will
 *	allow the user to activate the default button by simply hitting the
 *	<ENTER> key without making the button active first.
 * \param pcView - Pointer to the View that should receive all events generated
 *		   by the <ENTER> key. This will typically be an instance of
 *		   the os::Button class.
 * \sa GetDefaultButton(), os::Button, View::KeyUp(), View::KeyDown()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::SetDefaultButton( View * pcView )
{
	m->m_pcDefaultButton = pcView;
}

/** Return the view assigned as the default button.
 * \par Description:
 *	Return the pointer last set through SetDefaultButton() or NULL
 *	if not default button is assigned.
 *	Note that this is a os::View pointer, and if you want to
 *	call any os::Button spesific member you must static_cast it
 *	to a os::Button. If you are not 100% sure that the View is in fact
 *	a os::Button, you should use dynamic_cast, and test the result
 *	before touching it.
 * \return Pointer to the default button, or NULL if no default button is assigned.
 * \sa SetDefaultButton()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

View *Window::GetDefaultButton() const
{
	return ( m->m_pcDefaultButton );
}

void Window::SetDefaultWheelView( View * pcView )
{
	m->m_pcDefaultWheelView = pcView;
}

View *Window::GetDefaultWheelView() const
{
	return ( m->m_pcDefaultWheelView );
}

/** Activate a view.
 * \par Description:
 *	Give a view focus, or remove the focus from the current active
 *	view by passing in a NULL pointer. This has the same effect as
 *	calling pcView->MakeFocus(true).
 * \param pcView - The View that should receive focus, or a NULL pointer to
 *		   remove focus from the currently active view.
 * \return Pointer to the previously active view, or NULL if no view
 *	   had focus.
 * \sa View::MakeFocus()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
View *Window::SetFocusChild( View * pcView )
{
	View *pcPrevView = GetFocusChild();

	if( pcPrevView == pcView )
	{
		return ( pcPrevView );
	}

	if( pcView == NULL )
	{
		pcView = m->m_apcFocusStack[0];
		memmove( &m->m_apcFocusStack[0], &m->m_apcFocusStack[1], sizeof( View * ) * ( FOCUS_STACK_SIZE - 1 ) );
		m->m_apcFocusStack[FOCUS_STACK_SIZE - 1] = NULL;
	}
	else
	{
		if( pcPrevView != NULL )
		{
			memmove( &m->m_apcFocusStack[1], &m->m_apcFocusStack[0], sizeof( View * ) * ( FOCUS_STACK_SIZE - 1 ) );
			m->m_apcFocusStack[0] = pcPrevView;
		}
	}

	SetDefaultHandler( pcView );

	if( pcPrevView != NULL )
	{
		pcPrevView->Activated( false );
	}
	if( pcView != NULL )
	{
		pcView->Activated( true );
	}
	return ( pcPrevView );
}

/** Change the window title.
 * \par Description:
 *	SetTitle() will change the title rendered in the window border.
 * \param pzTitle
 *	The new window title.
 * \sa GetTitle()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::SetTitle( const String & cTitle )
{
	m->m_cTitle = cTitle;

	if( m->m_hLayerPort >= 0 )
	{
		Message cReq( WR_SET_TITLE );

		cReq.AddString( "title", m->m_cTitle.c_str() );

		if( Messenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			dbprintf( "Error: Window::SetTitle() failed to send request to server\n" );
		}
	}
}


/** Change the window icon.
 * \par Description:
 *	SetIcon() will change the icon assigned to the window.
 * \par Note:
 *	The Bitmap will be copied, so you can delete the bitmap
 *  afterwards,
 * \param pcIcon
 *	The new window icon.
 * \sa GetIcon(), SetTitle()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

void Window::SetIcon( Bitmap* pcIcon )
{
	if( pcIcon == NULL )
	{
		dbprintf( "Error: Window::SetIcon() called with empty icon\n" );
		return;
	}

	if( m->m_hLayerPort >= 0 )
	{
		Message cReq( WR_SET_ICON );
		Message cReply;
		cReq.AddInt32( "handle", pcIcon->m_hHandle );

		if( Messenger( m->m_hLayerPort ).SendMessage( &cReq, &cReply ) < 0 )
		{
			dbprintf( "Error: Window::SetIcon() failed to send request to server\n" );
		}
	}
}

/** Limit the minimum and maximum window size.
 * \par Description:
 *	SetSizeLimits() sets the maximum and minimum size the window
 *	can be resized to by the user. The limits are not enforced by
 *	ResizeBy(), ResizeTo(), and SetFrame()
 * \par Note:
 *	If you want to prevent the user from resizing the window at
 *	all it is normally better to set the \b WND_NOT_RESIZABLE flag
 *	in the constructor or with the SetFlags() member than to set
 *	the same size as minimum and maximum. Setting the \b
 *	WND_NOT_RESIZABLE flag will cause visual changes to notify the
 *	user that the window can not be resized.
 *
 *	Also think twice before making a window non-resizable it is
 *	normally better to make the window layout dynamic through
 *	the builtin layout engine.
 *
 * \param cMinSize
 *	Minimum size in pixels.
 * \param cMaxSize
 *	Maximum size in pixels.
 *	
 * \sa SetFlags()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::SetSizeLimits( const Point & cMinSize, const Point & cMaxSize )
{
	Flush();
	if( m->m_hLayerPort >= 0 )
	{
		Message cReq( WR_SET_SIZE_LIMITS );

		cReq.AddPoint( "min_size", cMinSize );
		cReq.AddPoint( "max_size", cMaxSize );

		if( Messenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			dbprintf( "Error: Window::SetSizeLimits() failed to send request to server\n" );
		}
	}
}

void Window::SetAlignment( const IPoint & cSize, const IPoint & cSizeOffset, const IPoint & cPos, const IPoint & cPosOffset )
{
	Flush();
	if( m->m_hLayerPort >= 0 )
	{
		Message cReq( WR_SET_ALIGNMENT );

		cReq.AddIPoint( "size", cSize );
		cReq.AddIPoint( "size_off", cSizeOffset );
		cReq.AddIPoint( "pos", cPos );
		cReq.AddIPoint( "pos_off", cPosOffset );

		if( Messenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			dbprintf( "Error: Window::SetAlignment() failed to send request to server\n" );
		}
	}
}

/** Obtain the current window title.
 * \return A const pointer to the internal window title.
 * \sa SetTitle()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

String Window::GetTitle( void ) const
{
	return ( m->m_cTitle );
}


/** Not longer supported
 * \return Not longer supported.
 * \sa SetIcon(), GetTitle()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
/* REMOVE */
Bitmap* Window::GetIcon( void ) const
{
	
	return( NULL );
}

/** Activate/Deactivate the window.
 * \par Description:
 *	If bFocus is true the window will gain focus, and start receiving
 *	input events generated by the mouse and keyboard. If bFocus is false
 *	the window will lose focus to the window that previously had it. 
 * \par Note:
 *	The appserver keeps a small stack of previously active
 *	windows. When a window give up focus volunterly it will then
 *	pop one window off the stack and if the stack was not empty
 *	that window will receive focus.
 * \param bFocus - True to activate the window, false to deactivate it.
 * \sa View::MakeFocus(), SetFocusChild()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::MakeFocus( bool bFocus )
{
	Message cReq( WR_MAKE_FOCUS );

	cReq.AddBool( "focus", bFocus );
	if( Messenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
	{
		dbprintf( "Error: Window::MakeFocus() failed to send WR_MAKE_FOCUS to server\n" );
	}
}

/** Hide/Unhide the window.
 * \par Description:
 
 *	When a window is first constructed, it is not made visible on
 *	the screen. You must first call Show(true), The first time
 *	Show() is called it will also unlock the window, and start the
 *	looper thread if it is not done already by calling Start().
 *
 *	You can nest calls to Show(false). It will then require the same
 *	numbers of Show(true) calls to make the window visible.
 * \par Note:
 *	The Window visibility state is global across all desktops.
 * \param bMakeVisible - Set to true to make the window visible, and false to hide it.
 * \sa Start(), Window::Window()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::Show( bool bMakeVisible )
{
	m->m_pcTopView->Show( bMakeVisible );
	if( m->m_bIsRunning == false )
	{
		m->m_bIsRunning = true;
		Run();
//      Unlock();
	}
}

bool Window::IsVisible() const
{
	return m->m_pcTopView->IsVisible();
}

/** Unlock the window and start the looper thread.
 * \par Description:
 *	If you whould like to unlock the window and start the looper thread
 *	without making the window visible, you can call Start() instead of
 *	Show(). This will invert the locking done by the contructor and
 *	make the looper run. It will also prevent Show() from doing the
 *	same when it is called later to make the window visible.
 *
 * \sa Show(), Window::Window()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::Start()
{
	if( m->m_bIsRunning == false )
	{
		m->m_bIsRunning = true;
		Run();
		Unlock();
	}
}

/** Set the window's position and size.
 * \par Description:
 *	SetFrame() will set the windows client position and size on the
 *	current desktop.
 * \param cRect - The new frame rectangle of the window's client area.
 * \sa GetFrame(), GetBounds(), MoveBy(), MoveTo(), ResizeBy(), ResizeTo()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::SetFrame( const Rect & cRect, bool bNotifyServer )
{
	m->m_pcTopView->SetFrame( cRect, bNotifyServer );
	Flush();
}

/** Move the window relative to it's current position.
 * \param cDelta - The distance to move the window.
 * \sa MoveTo(), ResizeBy(), ResizeTo(), SetFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::MoveBy( const Point & cDelta )
{
	m->m_pcTopView->MoveBy( cDelta );
}

/** Move the window relative to it's current position.
 * \param nDeltaX - The horizontal distance to move the window.
 * \param nDeltaY - The vertical distance to move the window.
 * \sa MoveTo(), ResizeBy(), ResizeTo(), SetFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::MoveBy( float vDeltaX, float vDeltaY )
{
	m->m_pcTopView->MoveBy( vDeltaX, vDeltaY );
}

/** Move the window to an absolute position
 * \par Description:
 *	Move the window so the upper left corner of the client area is
 *	at the given position
 * \param cPos - The new position
 * \sa MoveBy(), ResizeBy(), ResizeTo(), SetFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::MoveTo( const Point & cPos )
{
	m->m_pcTopView->MoveTo( cPos );
}

/** Move the window to an absolute position
 * \par Description:
 *	Move the window so the upper left corner of the client area is
 *	at the given position.
 * \param x - The new horizontal position.
 * \param y - The new vertical position.
 * \sa MoveBy(), ResizeBy(), ResizeTo(), SetFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::MoveTo( float x, float y )
{
	m->m_pcTopView->MoveTo( x, y );
}

/** Move the window to the centre of another window
 * \par Description:
 *	Move the window so the centre of the window is in the centre of the
 *	the given window.
 * \param pcWin - The Window to center in.
 * \sa MoveTo(), MoveBy(), ResizeBy(), ResizeTo(), SetFrame()
 * \author	Rick Caudill
 *****************************************************************************/
void Window::CenterInWindow( Window * pcWin )
{
	Rect cBounds = GetBounds();
	Rect cR = pcWin->GetFrame();

	MoveTo( cR.left + ( cR.Width() - cBounds.Width(  ) ) / 2, cR.top + ( cR.Height(  ) - cBounds.Height(  ) ) / 2 );
}

/** Move the window to the center of the screen
 * \par Description:
 * This method moves the window to the center of the screen.
 * \sa MoveTo(), MoveBy(), ResizeBy(), ResizeTo(), SetFrame()
 * \author	Rick Caudill
  *****************************************************************************/
void Window::CenterInScreen()
{
	Rect cBounds = GetBounds();
	Desktop cDesktop;
	IPoint cPoint = cDesktop.GetResolution();

	MoveTo( cPoint.x / 2 - cBounds.Width() / 2, cPoint.y / 2 - cBounds.Height(  ) / 2 );
}

/** Resize the window relative to it's current size
 * \param cDelta - The distance to move the lower-right corner of the window.
 * \sa ResizeTo(), MoveBy(), MoveTo(), SetFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::ResizeBy( const Point & cDelta )
{
	m->m_pcTopView->ResizeBy( cDelta );
}

/** Resize the window relative to it's current size
 * \param nDeltaW - The horizontal distance to move the lower-right corner of the window.
 * \param nDeltaH - The vertical distance to move the lower-right corner of the window.
 * \sa ResizeTo(), MoveBy(), MoveTo(), SetFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::ResizeBy( float vDeltaW, float vDeltaH )
{
	m->m_pcTopView->ResizeBy( vDeltaW, vDeltaH );
}

/** Resize the window to a new absolute size.
 * \param cSize - The new size of the windows client area.
 * \sa ResizeBy(), MoveBy(), MoveTo(), SetFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::ResizeTo( const Point & cSize )
{
	m->m_pcTopView->ResizeTo( cSize );
}

/** Resize the window to a new absolute size.
 * \param W - The new width of the windows client area.
 * \param W - The new heigth of the windows client area.
 * \sa ResizeBy(), MoveBy(), MoveTo(), SetFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::ResizeTo( float w, float h )
{
	m->m_pcTopView->ResizeTo( w, h );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \since 0.3.7
 * \sa ClearShapeRegion(), os::View::SetShapeRegion()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::SetShapeRegion( const Region & cReg )
{
	m->m_pcTopView->SetShapeRegion( cReg );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \since 0.3.7
 * \sa SetShapeRegion(), os::View::ClearShapeRegion()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::ClearShapeRegion()
{
	m->m_pcTopView->ClearShapeRegion();
}

/** Add a view to the window.
 * \par Description:
 *	AddChild() add a view to the windows top-view.
 *	The view's View::AttachedToWindow() and View::AllAttached()
 *	will be called to let the view know about it's new status.
 *
 *	If bAssignTabOrder is true, the view will be assigned
 *	a tab order making it possible to activate the view with
 *	the keyboard.
 * \par Note:
 *	The window share the tab-order counter with the View class.
 *	
 *	When the window is later closed, it will automatically delete
 *	all remaining child views.
 * \param pcChild - The View to add to the window.
 * \param bAssignTabOrder - If true the view will be assigned a tab-order
 *			    one higher than the previously added view.
 * \return
 * \sa RemoveChild(), View::AddChild(), View::AttachedToWindow(), View::SetTabOrder()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::AddChild( View * pcChild, bool bAssignTabOrder )
{
	m->m_pcTopView->AddChild( pcChild, bAssignTabOrder );
}

/** Remove a view from the window.
 * \par Description:
 *	Unlink the view from the windows top-view and call the view's
 *	View::DetachedFromWindow() and View::AllDetached()
 * \param pcChild - The view to remove.
 * \sa AddChild(), View::RemoveChild()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::RemoveChild( View * pcChild )
{
	m->m_pcTopView->RemoveChild( pcChild );
}

void Window::_ViewDeleted( View * pcView )
{
	if( pcView == m->m_pcLastMouseView )
	{
		m->m_pcLastMouseView = NULL;
	}
	if( pcView == m->m_pcDefaultButton )
	{
		m->m_pcDefaultButton = NULL;
	}
	if( pcView == m->m_pcDefaultWheelView )
	{
		m->m_pcDefaultWheelView = NULL;
	}
	for( int i = 0; i < FOCUS_STACK_SIZE; ++i )
	{
		if( m->m_apcFocusStack[i] == pcView )
		{
			for( int j = i; j < FOCUS_STACK_SIZE - 1; ++j )
			{
				m->m_apcFocusStack[j] = m->m_apcFocusStack[j + 1];
			}
			m->m_apcFocusStack[FOCUS_STACK_SIZE - 1] = NULL;
			i = i - 1;
		}
	}
}

/** Add a keyboard shortcut
 * \par Description:
 *	Adds a keyboard shortcut to this window. When the keyboard event occurs,
 *	a message is sent to the window's message handler. Normally you should not
 *	need to use this method directly, menus and widgets will automatically
 *	register their shortcuts.
 * \param cKey - The keyboard event that triggers the shortcut.
 * \param pcMsg - Message to send to this window (automatically deleted).
 * \sa RemoveShortcut()
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void Window::AddShortcut( const ShortcutKey& cKey, Message* pcMsg )
{
	if( cKey.IsValid() ) {
		RemoveShortcut( cKey );
		m->m_cShortcuts[ cKey ] = ShortcutData( pcMsg );
	}
}

/** Add a keyboard shortcut
 * \par Description:
 *	Adds a keyboard shortcut to this window. When the keyboard event occurs,
 *	the assigned View's KeyDown event handler is called. Normally you should not
 *	need to use this method directly, menus and widgets will automatically
 *	register their shortcuts.
 * \param cKey - The keyboard event that triggers the shortcut.
 * \param pcView - View to send the shortcut to.
 * \sa RemoveShortcut()
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void Window::AddShortcut( const ShortcutKey& cKey, View* pcView )
{
	if( cKey.IsValid() ) {
		RemoveShortcut( cKey );
		m->m_cShortcuts[ cKey ] = ShortcutData( pcView );
	}
}

/** Remove a keyboard shortcut
 * \par Description:
 *	Remove a keyboard shortcut previously added by one of the AddShortcut()
 *	methods. Normally you should not
 *	need to use this method directly, menus and widgets will automatically
 *	unregister their shortcuts.
 * \param cKey - The keyboard event that triggers the shortcut.
 * \sa AddShortcut()
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void Window::RemoveShortcut( const ShortcutKey& cKey )
{
	if( cKey.IsValid() ) {
//				dbprintf("valid\n");
		shortcut_map::iterator cItem = m->m_cShortcuts.find( cKey );
		if( cItem != m->m_cShortcuts.end() ) {
			if( ( (*cItem).second.m_nFlags & SDF_DELETE ) && ( (*cItem).second.m_pcMessage ) ) {
//				dbprintf("del: %p\n",(*cItem).second.m_pcMessage);
				delete (*cItem).second.m_pcMessage;
			}
//				dbprintf("erase\n");
			m->m_cShortcuts.erase( cItem );
		}
	}
}

/** Get the windows position and size on the current desktop.
 * \return The windows client rectangle.
 * \sa SetFrame(), GetBounds()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Rect Window::GetFrame() const
{
	return ( m->m_pcTopView->GetFrame() );
}

/** Get the window boundary.
 * \par Description:
 *	Same as GetFrame() except that the rectangle is moved so
 *	the left/top corner is located at 0,0.
 * \return The windows boundary.
 * \sa GetFrame(), SetFrame()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Rect Window::GetBounds() const
{
	return ( m->m_pcTopView->GetBounds() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Window::_HandleActivate( bool bIsActive, const Point & cMousePos )
{
	m->m_bIsActive = bIsActive;
	
	if( NULL == m->m_pcTopView )
	{
		dbprintf( "Error: Window::_HandleActivate() m->m_pcTopView == NULL\n" );
	}

	m->m_pcTopView->_WindowActivated( m->m_bIsActive );

	m->m_nMouseTransition = MOUSE_INSIDE;
	_MouseEvent( cMousePos, m->m_nButtons, NULL, false );	// FIXME: Get buttons from message
}

/** Flush the windows render queue.
 * \par Description:
 *	When a view is rendering graphics, the render commands is buffered
 *	in the window, and sendt to the appserver in batches to reduce
 *	the overhead of message passing. When the View::Paint() member is
 *	called by the system, the render queue will be automatically flushed
 *	when View::Paint() returnes. If the view however desides to render
 *	anything between paint messages, the render queue must be flushed
 *	manually to execute the rendering.
 * \par Note:
 *	Flush() do not guarantee that the pixels are indead on-screen when it
 *	returns, it only guarantee that the rendering will take place
 *	as soon as possible. If you want to make sure all the pixels is
 *	rendered, you must use Sync() instead.
 * \sa Sync(), Bitmap::Sync(), Bitmap::Flush(), View::Flush(), View::Sync()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::Flush()
{
	m->m_cRenderLock.Lock();
	if( m->m_psRenderPkt->nCount == 0 )
	{
		m->m_cRenderLock.Unlock();
		return;
	}
	m->m_psRenderPkt->hReply = -1;
	if( send_msg( m->m_hLayerPort, WR_RENDER, m->m_psRenderPkt, sizeof( WR_Render_s ) + m->m_nRndBufSize - RENDER_BUFFER_SIZE ) )
	{
		dbprintf( "Error: Window::Flush() failed to send WR_RENDER request to server\n" );
	}
	m->m_psRenderPkt->nCount = 0;
	m->m_nRndBufSize = 0;
	m->m_cRenderLock.Unlock();
}

/** Flush the render queue, and wait til the rendering is done.
 * \par Description:
 *	Sync() will like the Flush() member flush the render queue.
 *	The difference is that Sync() will wait for the appserver to
 *	finnish the rendering and send a reply informing that the
 *	rendering is done. This means that when Sync() returnes you
 *	have a guarante that all the previously issued rendering operations
 *	are indead executed.
 * \par Note:
 *	Sync() is normally not used on normal windows, since it have a higher
 *	overhead than Flush(), and you normally dont need to wait for the
 *	appserver to finnish. It is only useful when the window is attached
 *	to a bitmap, and you want to assure that the rendering indead have
 *	taken place before you start using the bitmap.
 * \sa Flush(), Bitmap::Sync(), Bitmap::Flush(), View::Sync(), View::Flush()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Window::Sync()
{
	m->m_cRenderLock.Lock();
	m->m_psRenderPkt->hReply = m->m_hReplyPort;
	if( send_msg( m->m_hLayerPort, WR_RENDER, m->m_psRenderPkt, sizeof( WR_Render_s ) + m->m_nRndBufSize - RENDER_BUFFER_SIZE ) == 0 )
	{
		if( get_msg( m->m_hReplyPort, NULL, NULL, 0 ) < 0 )
		{
			dbprintf( "Error: Window::Sync() failed to get reply\n" );
		}
	}
	m->m_psRenderPkt->nCount = 0;
	m->m_nRndBufSize = 0;
	m->m_cRenderLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void *Window::_AllocRenderCmd( uint32 nCmd, const View * pcView, uint32 nSize )
{
	void *pObj = NULL;
	
	m->m_cRenderLock.Lock();
	
	if( nSize <= RENDER_BUFFER_SIZE )
	{
		if( m->m_nRndBufSize + nSize > RENDER_BUFFER_SIZE )
		{
			Flush();
		}
		pObj = ( void * )&m->m_psRenderPkt->aBuffer[m->m_nRndBufSize];

		GRndHeader_s *psHdr = ( GRndHeader_s * ) pObj;

		psHdr->nSize = nSize;
		psHdr->nCmd = nCmd;
		psHdr->hViewToken = pcView->_GetHandle();

		m->m_psRenderPkt->nCount++;
		m->m_nRndBufSize += nSize;

		if( nCmd == DRC_COPY_RECT )
		{
			m->m_bDidScrollRect = true;
		}
	}
	else
	{
		dbprintf( "Error: View::_AllocRenderCmd() packet to big!\n" );
	}
	return ( pObj );
}

void Window::_PutRenderCmd()
{
	m->m_cRenderLock.Unlock();
}

/** Find the view covering a given position on the window.
 * \par Description:
 *	FindView( const Point& cPos ) will do a reqursive search through
 *	all attached view's to find the topmost view covering the given
 *	point. If no view is intersecting the point NULL is returned.
 * \return Pointer to the View, or NULL.
 * \sa FindView( const char* pzName )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

View *Window::FindView( const Point & cPos ) const
{
	Point cViewPos = cPos;
	View *pcView = NULL;

	for( View * pcTmpWid = m->m_pcTopView->GetChildAt( cViewPos ); pcTmpWid != NULL; pcTmpWid = pcTmpWid->GetChildAt( cViewPos ) )
	{
		Rect cChildFrame = pcTmpWid->GetFrame();

		pcView = pcTmpWid;

		cViewPos -= cChildFrame.LeftTop();
		cViewPos -= pcTmpWid->GetScrollOffset();
	}
	return ( pcView );
}

void Window::_CallMouseMoved( View * pcView, uint32 nButtons, int nWndTransit, Message * pcData, bool bUnderMouse )
{
	while( pcView != NULL && pcView->_GetMouseMoveRun() == m->m_nMouseMoveRun )
	{
		pcView = pcView->GetParent();
	}
	if( pcView == NULL )
	{
		return;
	}
	Point cPos = pcView->ConvertFromScreen( m->m_cMousePos );
	int nCode;

	if( ( nWndTransit == MOUSE_ENTERED || nWndTransit == MOUSE_INSIDE ) && bUnderMouse )
	{
		if( pcView->_GetMouseMode() == MOUSE_OUTSIDE )
		{
			nCode = MOUSE_ENTERED;
			pcView->_SetMouseMode( MOUSE_INSIDE );
		}
		else
		{
			nCode = MOUSE_INSIDE;
		}
	}
	else
	{
		if( pcView->_GetMouseMode() == MOUSE_INSIDE )
		{
			nCode = MOUSE_EXITED;
			pcView->_SetMouseMode( MOUSE_OUTSIDE );
		}
		else
		{
			nCode = MOUSE_OUTSIDE;
		}
	}
	pcView->_SetMouseMoveRun( m->m_nMouseMoveRun );
	pcView->MouseMove( cPos, nCode, nButtons, pcData );
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Window::_MouseEvent( const Point & cNewPos, uint32 nButtons, Message * pcData, bool bReEntered )
{
	View *pcFocusChild;
	View *pcMouseView;
	
	pcMouseView = FindView( m->m_pcTopView->ConvertFromScreen( m->m_cMousePos ) );
	pcFocusChild = GetFocusChild();

	if( m->m_pcLastMouseView != pcMouseView )
	{
		if( m->m_pcLastMouseView != NULL )
		{
			_CallMouseMoved( m->m_pcLastMouseView, nButtons, m->m_nMouseTransition, pcData, false );
		}
		m->m_pcLastMouseView = pcMouseView;
	}
	if( pcMouseView != NULL )
	{
		_CallMouseMoved( pcMouseView, nButtons, m->m_nMouseTransition, pcData, true );
	}
	if( pcFocusChild != NULL )
	{
		_CallMouseMoved( pcFocusChild, nButtons, m->m_nMouseTransition, pcData, false );
	}
}

View *Window::_GetPrevTabView( View * pcCurrent )
{
	int nFocusOrder = pcCurrent->GetTabOrder();
	const Looper::handler_map &cMap = GetHandlerMap();
	Looper::handler_map::const_iterator i;

	int nHighestOrder = INT_MIN;
	int nClosest = -1;
	View *pcClosest = NULL;
	View *pcHighestOrder = NULL;

	for( i = cMap.begin(); i != cMap.end(  ); ++i )
	{
		View *pcView = dynamic_cast < View * >( ( *i ).second );

		if( pcView == NULL || pcView == pcCurrent )
		{
			continue;
		}
		int nOrder = pcView->GetTabOrder();

		if( nOrder < 0 )
		{
			continue;
		}
		if( nOrder <= nFocusOrder )
		{
			if( nFocusOrder - nOrder < nFocusOrder - nClosest )
			{
				pcClosest = pcView;
				nClosest = nOrder;
			}
		}
		if( nOrder > nHighestOrder )
		{
			nHighestOrder = nOrder;
			pcHighestOrder = pcView;
		}
	}
	return ( ( pcClosest != NULL ) ? pcClosest : pcHighestOrder );

}

View *Window::_GetNextTabView( View * pcCurrent )
{
	int nFocusOrder = pcCurrent->GetTabOrder();
	const Looper::handler_map &cMap = GetHandlerMap();
	Looper::handler_map::const_iterator i;
//	int x = 0;	// debug

	int nLowestOrder = INT_MAX;
	int nClosest = INT_MAX;
	View *pcClosest = NULL;
	View *pcLowestOrder = NULL;

//	dbprintf( "Window::_GetNextTabView( %ld ) : %s\n", nFocusOrder, pcCurrent->GetName().c_str() );

	for( i = cMap.begin(); i != cMap.end(  ); ++i )
	{
//		x++;
		View *pcView = dynamic_cast < View * >( ( *i ).second );

		if( pcView == NULL || pcView == pcCurrent )
		{
//			dbprintf( "- %ld = pcCurrent or NULL\n", x );
			continue;
		}
		int nOrder = pcView->GetTabOrder();

		if( nOrder < 0 )
		{
			continue;
		}
		if( nOrder >= nFocusOrder )
		{
//			dbprintf( "- %ld = nOrder (%ld) >= nFocusOrder (%ld)\n", x, nOrder, nFocusOrder );
			if( nOrder - nFocusOrder < nClosest - nFocusOrder )
			{
				pcClosest = pcView;
				nClosest = nOrder;
			}
		}
		if( nOrder < nLowestOrder )
		{
//			dbprintf( "- %ld = nOrder (%ld) < nLowestOrder (%ld)\n", x, nOrder, nLowestOrder );
			nLowestOrder = nOrder;
			pcLowestOrder = pcView;
		}
	}
	return ( ( pcClosest != NULL ) ? pcClosest : pcLowestOrder );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Window::DispatchMessage( Message * pcMsg, Handler * pcHandler )
{
	View *pcView;

	if( pcMsg->FindPointer( "_widget", ( void ** )&pcView ) == 0 && pcView != NULL && _FindHandler( pcView->m_nToken ) == pcView )
	{
		switch ( pcMsg->GetCode() )
		{
		case M_MOUSE_DOWN:
			{
				int32 nButton;
				
				pcMsg->FindPoint( "_scr_pos", &m->m_cMousePos );

				if( pcMsg->FindInt32( "_button", &nButton ) == 0 )
				{
					View *pcChild;

					m->m_nButtons |= 1 << ( nButton - 1 );
					/*
					   if ( pcChild = GetFocusChild() ) {
					   pcChild->MouseDown( pcChild->ConvertFromScreen( m->m_cMousePos ), m->m_nButtons );
					   }
					 */
					pcChild = FindView( m->m_pcTopView->ConvertFromScreen( m->m_cMousePos ) );
					if( pcChild != NULL /*  && pcChild != GetFocusChild() */  )
					{
						pcChild->MouseDown( pcChild->ConvertFromScreen( m->m_cMousePos ), nButton );
					}
					else
					{
						View *pcFocusView = GetFocusChild();

						if( pcFocusView != NULL )
						{
							pcFocusView->MouseDown( pcFocusView->ConvertFromScreen( m->m_cMousePos ), nButton );
						}
					}
				}
				else
				{
					dbprintf( "Error: _buttons not found\n" );
				}
				break;
			}
		case M_MOUSE_UP:
			{
				int32 nButton;
				
				pcMsg->FindPoint( "_scr_pos", &m->m_cMousePos );				

				if( pcMsg->FindInt32( "_button", &nButton ) == 0 )
				{
					Message *pcData = NULL;
					Message cData( 0 );

					View *pcChild;

					if( pcMsg->FindMessage( "_drag_message", &cData ) == 0 )
					{
						pcData = &cData;
					}

					m->m_nButtons &= ~( 1 << ( nButton - 1 ) );
					Point cPos = m->m_pcTopView->ConvertFromScreen( m->m_cMousePos );

					pcChild = FindView( cPos );
					if( pcChild != NULL )
					{
						pcChild->MouseUp( pcChild->ConvertFromScreen( m->m_cMousePos ), nButton, pcData );
					}
					if( pcChild != GetFocusChild() )
					{
						pcChild = GetFocusChild();
						if( pcChild != NULL )
						{
							pcChild->MouseUp( pcChild->ConvertFromScreen( m->m_cMousePos ), nButton, NULL );
						}
					}
				}
				break;
			}
		case M_MOUSE_MOVED:
			{
				MessageQueue *pcQueue = GetMessageQueue();

				int32 nButtons;
				pcMsg->FindInt32( "_buttons", &nButtons );
				m->m_nButtons = nButtons;
				if( pcMsg->FindInt32( "_transit", &m->m_nMouseTransition ) != 0 )
				{
					dbprintf( "Warning: Window::DispatchMessage() could not find '_transit' in M_MOUSE_MOVED message\n" );
				}
				bool bDoProcess = true;
				
				pcQueue->Lock();
				Message *pcNextMsg = pcQueue->FindMessage( M_MOUSE_MOVED, 0 );

				if( pcNextMsg != NULL )
				{
					int32 nTransition;

					if( pcMsg->FindInt32( "_transit", &nTransition ) != 0 )
					{
						dbprintf( "Warning: Window::DispatchMessage() could not find '_transit' in M_MOUSE_MOVED message\n" );
					}
					if( nTransition == m->m_nMouseTransition )
					{
						bDoProcess = false;
					}
				}
				pcQueue->Unlock();
				if( bDoProcess )
				{	// Wait for the last mouse move message
					if( pcMsg->FindPoint( "_scr_pos", &m->m_cMousePos ) == 0 )
					{

						Message *pcData = NULL;
						Message cData( 0 );

						if( pcMsg->FindMessage( "_drag_message", &cData ) == 0 )
						{
							pcData = &cData;
						}
						m->m_nMouseMoveRun++;
						_MouseEvent( m->m_cMousePos, m->m_nButtons, pcData, false );
					}
					else
					{
						dbprintf( "ERROR : Could not find Point in mouse move message\n" );
					}
				}
				break;
			}
		case M_WHEEL_MOVED:
			{
				View *pcView = m->m_pcDefaultWheelView;

				if( pcView == NULL )
				{
					pcView = m->m_pcLastMouseView;
				}
				if( pcView != NULL )
				{
					Point cDelta;

					pcMsg->FindPoint( "delta_move", &cDelta );
					pcView->WheelMoved( cDelta );
				}
				break;
			}
		case M_KEY_DOWN:
			{
				int32 nQualifier;
				const char *pzString;
				const char *pzRawString;

				if( pcMsg->FindInt32( "_qualifiers", &nQualifier ) != 0 )
				{
					dbprintf( "Error: Window::DispatchMessage() could not find '_qualifiers' member in M_KEY_DOWN message\n" );
					break;
				}
				if( pcMsg->FindString( "_string", &pzString ) != 0 )
				{
					dbprintf( "Error: Window::DispatchMessage() could not find '_string' member in M_KEY_DOWN message\n" );
					break;
				}
				if( pcMsg->FindString( "_raw_string", &pzRawString ) != 0 )
				{
					dbprintf( "Error: Window::DispatchMessage() could not find '_raw_string' member in M_KEY_DOWN message\n" );
					break;
				}

				View *pcFocusChild = GetFocusChild();

				if( pcFocusChild != NULL )
				{
					if( m->m_pcDefaultButton != NULL && pzString[0] == VK_ENTER && pzString[1] == '\0' )
					{
						m->m_pcDefaultButton->KeyDown( pzString, pzRawString, nQualifier );
						break;
					}
					int nFocusOrder = pcFocusChild->GetTabOrder();

/*					if( pzString[0] == '\t' ) {	// debug
						dbprintf( "TAB! Focus order = %d\n", nFocusOrder );
					}*/

					if( nFocusOrder >= 0 && pzString[0] == '\t' )
					{
						if( nFocusOrder != -1 )
						{
							View *pcNext;

							if( nQualifier & QUAL_SHIFT )
							{
								pcNext = _GetPrevTabView( pcFocusChild );
							}
							else
							{
								pcNext = _GetNextTabView( pcFocusChild );
							}
							if( pcNext != NULL )
							{
								pcNext->MakeFocus();
							}
						}
					}
					else
					{
						// This flag will let us know if the event was rejected by the focus
						// child and all it's ancestors.
						m->m_pcTopView->m_bKeyDownRejected = false;
						pcFocusChild->KeyDown( pzString, pzRawString, nQualifier );
						if( m->m_pcTopView->m_bKeyDownRejected ) {
							_HandleShortcuts( pzString, pzRawString, nQualifier );
						}
					}
				} else {
					_HandleShortcuts( pzString, pzRawString, nQualifier );
				}

				break;
			}

		case M_KEY_UP:
			{
				View *pcFocusChild = GetFocusChild();

				if( pcFocusChild != NULL )
				{
					int32 nQualifier;
					const char *pzString;
					const char *pzRawString;

					if( pcMsg->FindInt32( "_qualifiers", &nQualifier ) != 0 )
					{
						dbprintf( "Error: Window::DispatchMessage() could not find '_qualifiers' member in M_KEY_UP message\n" );
						break;
					}
					if( pcMsg->FindString( "_string", &pzString ) != 0 )
					{
						dbprintf( "Error: Window::DispatchMessage() could not find '_string' member in M_KEY_UP message\n" );
						break;
					}
					if( pcMsg->FindString( "_raw_string", &pzRawString ) != 0 )
					{
						dbprintf( "Error: Window::DispatchMessage() could not find '_raw_string' member in M_KEY_UP message\n" );
						break;
					}
					if( m->m_pcDefaultButton != NULL && pzString[0] == VK_ENTER && pzString[1] == '\0' )
					{
						m->m_pcDefaultButton->KeyUp( pzString, pzRawString, nQualifier );
					}
					else
					{
						pcFocusChild->KeyUp( pzString, pzRawString, nQualifier );
					}
				}
				break;
			}
		case M_PAINT:
			{
				Rect cRect;

				if( pcMsg->FindRect( "_rect", &cRect ) == 0 )
				{
					pcView->_BeginUpdate();
					pcView->_ConstrictRectangle( &cRect, Point( 0, 0 ) );
					if( cRect.IsValid() )			
						pcView->Paint( cRect );
					pcView->_EndUpdate();
					if( m->m_bDidScrollRect )
					{
						m->m_bDidScrollRect = false;
						Sync();
						SpoolMessages();
					}
					else
					{
						MessageQueue *pcQueue = GetMessageQueue();

						pcQueue->Lock();
						Message *pcNextMsg = pcQueue->FindMessage( M_PAINT, 0 );
						if( pcNextMsg == NULL )
							Flush();
						pcQueue->Unlock();
					}
				}
				else
				{
					dbprintf( "Error: Could not find rectangle in paint message\n" );
				}
				send_msg( m->m_hLayerPort, WR_PAINT_FINISHED, NULL, 0 );
				break;
			}
		case M_FONT_CHANGED:
			pcView->FontChanged( pcView->GetFont() );
			break;
		default:
			Looper::DispatchMessage( pcMsg, pcHandler );
			break;
		}
	}
	else
	{
		switch ( pcMsg->GetCode() )
		{
		case M_WINDOW_FRAME_CHANGED:
			{
				Rect cNewFrame;

				if( pcMsg->FindRect( "_new_frame", &cNewFrame ) != 0 )
				{
					dbprintf( "Error: Could not find '_new_frame' in M_WINDOW_FRAME_CHANGED message\n" );
					break;
				}
				SetFrame( cNewFrame, false );
				if( send_msg( m->m_hLayerPort, WR_WND_MOVE_REPLY, NULL, 0 ) < 0 )
				{
					dbprintf( "Error: Window::DispatchMessage() failed to send WR_WND_MOVE_REPLY to server\n" );
				}
				break;
			}
		case M_WINDOW_ACTIVATED:
			{
				bool bIsActive;

				if( pcMsg->FindBool( "_is_active", &bIsActive ) != 0 )
				{
					dbprintf( "Error: Failed to find '_is_active' member in M_WINDOW_ACTIVATED message\n" );
				}

				if( pcMsg->FindPoint( "_scr_pos", &m->m_cMousePos ) != 0 )
				{
					dbprintf( "Error: Failed to find '_scr_pos' member in M_WINDOW_ACTIVATED message\n" );
				}
				_HandleActivate( bIsActive, m->m_cMousePos );
				break;
			}
		case M_DESKTOP_ACTIVATED:
			{
				int32 nDesktop;
				bool bActivated;

				if( pcMsg->FindInt32( "_new_desktop", &nDesktop ) != 0  )
				{
					dbprintf( "Error: Failed to find '_new_desktop' member in M_DESKTOP_ACTIVATED message\n" );
				} 
				if( pcMsg->FindBool( "_activated", &bActivated ) != 0  )
				{
					dbprintf( "Error: Failed to find '_activated' member in M_DESKTOP_ACTIVATED message\n" );
				} 
				
				DesktopActivated( nDesktop, bActivated );
				break;
			}
		case M_SCREENMODE_CHANGED:
			{
				IPoint cPoint;
				int32 nColorSpace;

				if( pcMsg->FindIPoint( "_new_resolution", &cPoint ) != 0 )
				{
					dbprintf( "Error: Failed to find '_new_resolution' member in M_SCREENMODE_CHANGED message\n" );
				} 
				
				if( pcMsg->FindInt32( "_new_color_space", &nColorSpace ) != 0 )
				{
					dbprintf( "Error: Failed to find '_new_color_space' member in M_SCREENMODE_CHANGED message\n" );
				} 
				enum color_space eSpace = ( enum color_space )nColorSpace;
				ScreenModeChanged( cPoint, eSpace );
				break;
			}
		case M_WINDOWS_CHANGED:
			{
				WindowsChanged();
				break;
			}
		default:
			Looper::DispatchMessage( pcMsg, pcHandler );
			break;
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
//      Send a delete request to the appserver to get rid of the server
//      part of a view object. When the object is deleted on the server
//      we ask the Looper to copy all unread messages into the internal
//      message queue and then filter out all messages that was targeted
//      to the delete view.
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Window::_DeleteViewFromServer( View * pcView )
{
	Sync();
	if( pcView != m->m_pcTopView )
	{
		Message cReq( WR_DELETE_VIEW );

		cReq.AddInt32( "top_view", m->m_pcTopView->_GetHandle() );
		cReq.AddInt32( "handle", pcView->_GetHandle() );

		if( Messenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			dbprintf( "Error: Window::_DeleteViewFromServer() failed to send message to server\n" );
		}
	}

	SpoolMessages();	// Copy all messages from the message port to the message queue.
	MessageQueue *pcQueue = GetMessageQueue();
	MessageQueue cTmp;
	Message *pcMsg;

	pcQueue->Lock();

	while( ( pcMsg = pcQueue->NextMessage() ) != NULL )
	{
		View *pcTmpView;

		if( pcMsg->FindPointer( "_widget", ( void ** )&pcTmpView ) == 0 && pcTmpView == pcView )
		{
			if( pcMsg->GetCode() == M_PAINT )
				send_msg( m->m_hLayerPort, WR_PAINT_FINISHED, NULL, 0 );
			delete pcMsg;
		}
		else
		{
			cTmp.AddMessage( pcMsg );
		}
	}
	while( ( pcMsg = cTmp.NextMessage() ) != NULL )
	{
		pcQueue->AddMessage( pcMsg );
	}
	pcQueue->Unlock();
}

void Window::_HandleShortcuts( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
	ShortcutKey cKey( pzRawString, nQualifiers );

	if( !cKey.IsValid() ) return;
	
//	shortcut_map::iterator cItem = m->m_cShortcuts.find( cKey );
	shortcut_map::iterator cItem = m->m_cShortcuts.begin();
	while( cItem != m->m_cShortcuts.end() ) {
		if( (*cItem).first == cKey ) break;
/*		if( cKey < (*cItem).first ) {
			cItem = m->m_cShortcuts.end();
			break;
		}*/
		cItem++;
	}
	if( cItem != m->m_cShortcuts.end() ) {
		if( (*cItem).second.m_pcMessage ) {
			if( (*cItem).second.m_nFlags & SDF_MESSAGE ) {
				PostMessage( (*cItem).second.m_pcMessage, this );
			} else if( (*cItem).second.m_nFlags & SDF_VIEW ) {
				(*cItem).second.m_pcView->KeyDown( pzString, pzRawString, nQualifiers );
			}
		}
	}
}

bool Window::IsActive() const
{
	return ( m->m_bIsActive || m->m_bMenuOpen );
}

int Window::ToggleDepth()
{
	return ( m->m_pcTopView->ToggleDepth() );
}

Point Window::_GetPenPosition( int nViewHandle )
{
	WR_GetPenPosition_s sReq;
	WR_GetPenPositionReply_s sReply;

	sReq.m_hTopView = m->m_pcTopView->_GetHandle();
	sReq.m_hReply = m->m_hReplyPort;
	sReq.m_hViewHandle = nViewHandle;

	if( send_msg( m->m_hLayerPort, WR_GET_PEN_POSITION, &sReq, sizeof( sReq ) ) == 0 )
	{
		if( get_msg( m->m_hReplyPort, NULL, &sReply, sizeof( sReply ) ) < 0 )
		{
			dbprintf( "Error: Window::_GetPenPosition() failed to get reply\n" );
		}
	}
	else
	{
		dbprintf( "Error: Window::_GetPenPosition() failed to send request to server\n" );
	}
	return ( sReply.m_cPos );
}


void Window::FrameMoved( const Point & cDelta )
{
}

void Window::FrameSized( const Point & cDelta )
{
}


/** Called whenever the screenmode changes.
 * \par Description:
 * ScreenModeChanged( const IPoint & cNewRes, color_space eColorSpace ) will
 * be called when the screenmode changes. This will also happen during a 
 * desktop change to the desktop displaying the window if both desktops use
 * different screenmodes.
 * \param cNewRes - New resolution.
 * \param eColorSpace - New colorspace.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void Window::ScreenModeChanged( const IPoint & cNewRes, color_space eColorSpace )
{
}


/** Called whenever the desktop displaying the window will be activated or deactivated.
 * \par Description:
 * DesktopActivated( int nDesktop, bool bActive ) will
 * be called when the desktop displaying the window will be activated or deactivated.
 * \param nDesktop - Now activated desktop if bActive is true. Otherwise the old desktop.
 * \param bActive - True if the desktop is activated.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void Window::DesktopActivated( int nDesktop, bool bActive )
{
}

void Window::DesktopsChanged( uint32 nOldDesktops, uint32 nNewDesktops )
{
}

TopView::TopView( const Rect & cFrame, Window * pcWindow ):View( cFrame, "window_top_view", CF_FOLLOW_ALL, WID_CLEAR_BACKGROUND )
{
	m_pcWindow = pcWindow;
}

void TopView::FrameMoved( const Point & cDelta )
{
	m_pcWindow->FrameMoved( cDelta );
}

void TopView::FrameSized( const Point & cDelta )
{
	m_pcWindow->FrameSized( cDelta );
}

void TopView::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
	/* It would be very risky to handle keyboard shortcuts here, so we'll use a flag */
	/* to tell DispatchMessage() to do it. */
	m_bKeyDownRejected = true;
}


void Window::_DefaultColorsChanged()
{
}

void Window::_SetMenuOpen( bool bOpen )
{
	m->m_bMenuOpen = bOpen;
}

View *Window::_GetTopView() const
{
	return ( m->m_pcTopView );
}

port_id Window::_GetAppserverPort() const
{
	return ( m->m_hLayerPort );
}
/** Called whenever the currently shown windows change.
 * \par Description:
 * THIS METHOD WILL BE REMOVED IN ONE OF THE FUTURE SYLLABLE RELEASES.
 * USE THE NEW EVENT INTERFACE INSTEAD. 	
 * \par Note:
 * This will only work if you have set the WND_SEND_WINDOWS_CHANGED flag.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void Window::WindowsChanged()
{
}

void Window::__WI_reserved2__()
{
}
void Window::__WI_reserved3__()
{
}
void Window::__WI_reserved4__()
{
}
void Window::__WI_reserved5__()
{
}
void Window::__WI_reserved6__()
{
}











