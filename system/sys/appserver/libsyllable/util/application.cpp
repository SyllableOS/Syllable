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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <atheos/types.h>
#include <atheos/threads.h>
#include <atheos/kernel.h>
#include <util/application.h>
#include <util/message.h>
#include <util/messenger.h>
#include <appserver/protocol.h>
#include <util/keymap.h>
#include <algorithm>

using namespace os;

Application *Application::s_pcInstance = NULL;

/** \internal */

class Application::Private
{
	public:
	port_id m_hServerPort;
	port_id m_hSrvAppPort;
	port_id m_hReplyPort;
	std::vector < Window * >m_cWindows;
	std::vector <os::KeyboardEvent> m_cKeyEvents;
	Locale m_cLocale;
	Catalog* m_pcCatalog;
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Application::Application( const char *pzName ):Looper( pzName )
{
	m = new Private;
	Message cReq( DR_CREATE_APP );

	m->m_pcCatalog = NULL;
	
	assert( NULL == s_pcInstance );

	m->m_hReplyPort = create_port( "app_reply", DEFAULT_PORT_SIZE );

	assert( m->m_hReplyPort >= 0 );

	m->m_hServerPort = get_app_server_port();

	cReq.AddInt32( "process_id", get_process_id( NULL ) );
	cReq.AddInt32( "event_port", GetMsgPort() );
	cReq.AddString( "app_name", pzName );

	Message cReply;

	if( Messenger( m->m_hServerPort ).SendMessage( &cReq, &cReply ) >= 0 )
	{
		cReply.FindInt( "app_cmd_port", &m->m_hSrvAppPort );
	}
	else
	{
		dbprintf( "Application::Application() failed to send message to server: %s\n", strerror( errno ) );
	}
	for( int i = 0; i < COL_COUNT; ++i )
	{
		Color32_s sColor;

		if( cReply.FindColor32( "cfg_colors", &sColor, i ) != 0 )
		{
			break;
		}
		__set_default_color( static_cast < default_color_t > ( i ), sColor );
	}

	assert( m->m_hSrvAppPort >= 0 );

	s_pcInstance = this;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Application::~Application()
{
	int timeout = 1000;
	Lock();
	
	while( m->m_cWindows.size() > 0 && --timeout > 0 )
	{
		Window* w = m->m_cWindows[0];
		Unlock();
		w->Terminate();
		// Give the windows some time to get rid of themselves.
		snooze( 10000 );	// Usually a context switch is enough
		Lock();
	}


	for (uint i=0; i<m->m_cKeyEvents.size(); i++)
	{
			Message cReq(DR_UNREGISTER_KEY_EVNT);
			cReq.AddString("app",GetName());
			cReq.AddObject("key",m->m_cKeyEvents[i].GetShortcutKey());
			Messenger( m->m_hServerPort ).SendMessage( &cReq );
			snooze( 10000 );
	}
	
	m->m_cKeyEvents.clear();
		
	// If all windows aren't gone in 10 secs, we'll simply let the AppServer take care of them
	
	if( send_msg( m->m_hSrvAppPort, M_QUIT, NULL, 0 ) < 0 )
	{
		dbprintf( "Error: Application::~Application() failed to send M_QUIT request to server\n" );
	}

	delete_port( m->m_hReplyPort );

	s_pcInstance = NULL;

	if( m->m_pcCatalog )
		m->m_pcCatalog->Release();

	Unlock();
	delete m;
}

/** \brief Get the (one and only) instance of the Application class.
 *
 *	When the Application class is instantiated it will assign a pointer to
 *	the instance to a static member that can be obtained through this function.
 *	Many other classes expect this function to return a valid pointer, so it
 *	is importen that you instantiate the Application class once (and only once)
 *	before calling any other functions in the native AtheOS API.
 *  \return
 *	Pointer to the global instance of the Application drived class.
 *  \sa
 *  \author	Kurt Skauen (kurt.skauen@c2i.net)
    *//////////////////////////////////////////////////////////////////////////////

Application *Application::GetInstance()
{
	return ( s_pcInstance );
}

/** \brief Get the default string catalog
 *
 *	This function returns the string catalog that was set with SetCatalog().
 *  \return
 *	Pointer to the application's default string catalog
 *  \sa SetCatalog()
 *  \author	Henrik Isaksson (syllable@isaksson.tk)
 */
const Catalog* Application::GetCatalog() const
{
	return m->m_pcCatalog;
}

/** \brief Set the default string catalog
 *
 *	This function sets the string catalog that is returned by GetCatalog().
 *  \sa GetCatalog()
 *  \author	Henrik Isaksson (syllable@isaksson.tk)
 */
void Application::SetCatalog( Catalog* pcCatalog )
{
	Lock();

	if( m->m_pcCatalog )
		m->m_pcCatalog->Release();

	m->m_pcCatalog = pcCatalog;

	if( m->m_pcCatalog )
		m->m_pcCatalog->AddRef();

	Unlock();
}

/** \brief Set the default string catalog
 *
 *	This function sets the string catalog that is returned by GetCatalog().
 *  \sa GetCatalog()
 *  \author	Henrik Isaksson (syllable@isaksson.tk)
 */
bool Application::SetCatalog( const String& cCatalogName )
{
	Catalog* pcCat = m->m_cLocale.GetLocalizedCatalog( cCatalogName );
	if( pcCat ) {
		SetCatalog( pcCat );
		return true;
	} else {
		dbprintf( "Catalog '%s' not found\n", cCatalogName.c_str() );
		return false;
	}
}

/** \brief Get current locale
 *
 *	This function returns the default locale that is used by this application.
 *	Unless you change it using SetApplicationLocale, the application locale will
 *	be the same as the system's default locale (as set by Locale prefs).
 *	\sa SetApplicationLocale()
 *  \author	Henrik Isaksson (syllable@isaksson.tk)
 */
Locale* Application::GetApplicationLocale() const
{
	return &m->m_cLocale;
}

/** \brief Change current locale
 *
 *	This function changes the default locale that is used by this application.
 *	Normally, you shouldn't use this function, as the locale will be set
 *	automatically to the system's default locale (as set by Locale prefs).
 *	\sa GetApplicationLocale()
 *  \author	Henrik Isaksson (syllable@isaksson.tk)
 */
void Application::SetApplicationLocale( const Locale& cLocale )
{
	m->m_cLocale = cLocale;
}

/** \brief Return the appserver message port.
 *
 *	When contructed the Application class will establish a connection to the
 *	application server. The appserver will create an message port and spawn
 *	a thread that will handle application-level operation on behave of the
 *	Application class. This function returnes the message port that connect
 *	the Application object with the corresponding thread in the application
 *	server
 *  \return
 *	The message port id for the server side message port connecting this
 *	object to the application server.
 *
 *  \sa
 *  \author	Kurt Skauen (kurt.skauen@c2i.net)
    *//////////////////////////////////////////////////////////////////////////////

port_id Application::GetAppPort() const
{
	return ( m->m_hSrvAppPort );
}

port_id Application::GetServerPort() const
{
	return ( m->m_hServerPort );
}

bigtime_t Application::GetIdleTime()
{
	Message cReq( AR_GET_IDLE_TIME );
	Message cReply;

	Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply );

	int64 nTime;

	cReply.FindInt64( "idle_time", &nTime );

	return ( nTime );
}

void Application::PushCursor( mouse_ptr_mode eMode, void *pImage, int nWidth, int nHeight, const IPoint & cHotSpot )
{
	Message cReq( WR_PUSH_CURSOR );
	
	/* Calculate size of the cursor bitmap */
	switch( eMode )
	{
		case MPTR_MONO:
		case MPTR_CMAP8:
			cReq.AddData( "image", T_ANY_TYPE, pImage, nWidth * nHeight );
			break;
		case MPTR_RGB32:
			cReq.AddData( "image", T_ANY_TYPE, pImage, nWidth * nHeight * 4 );
			break;
		default:
			dbprintf( "Application::PushCursor() got cursor with invalid mode: %i\n", (int)eMode );
			return;
	}	

	cReq.AddIPoint( "size", IPoint( nWidth, nHeight ) );
	cReq.AddInt32( "mode", eMode );
	cReq.AddIPoint( "hot_spot", cHotSpot );

	Messenger( m->m_hSrvAppPort ).SendMessage( &cReq );
}

void Application::PopCursor()
{
	Message cReq( WR_POP_CURSOR );

	Messenger( m->m_hSrvAppPort ).SendMessage( &cReq );
}

int Application::GetScreenModeCount()
{
	Message cReq( AR_GET_SCREENMODE_COUNT );
	Message cReply;

	Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply );

	int nCount;

	cReply.FindInt( "count", &nCount );

	return ( nCount );
}

int Application::GetScreenModeInfo( int nIndex, screen_mode * psMode )
{
	Message cReq( AR_GET_SCREENMODE_INFO );
	Message cReply;

	cReq.AddInt32( "index", nIndex );

	Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply );

	IPoint cScrRes;
	int nColorSpace;
	int nError;
	float vRf;

	cReply.FindIPoint( "resolution", &cScrRes );
	cReply.FindInt( "bytes_per_line", &psMode->m_nBytesPerLine );
	cReply.FindInt( "color_space", &nColorSpace );
	cReply.FindFloat( "refresh_rate", &vRf );
	cReply.FindInt( "error", &nError );

	psMode->m_nWidth = cScrRes.x;
	psMode->m_nHeight = cScrRes.y;
	psMode->m_eColorSpace = ( color_space ) nColorSpace;
	psMode->m_vRefreshRate = vRf;

	if( nError != 0 )
	{
		errno = nError;
		return ( -1 );
	}
	else
	{
		return ( 0 );
	}
}

int Application::GetFontConfigNames( std::vector < String > *pcTable ) const
{
	Message cReq( DR_GET_DEFAULT_FONT_NAMES );
	Message cReply;

	if( Messenger( m->m_hServerPort ).SendMessage( &cReq, &cReply ) >= 0 )
	{
		const char *pzName;

		for( int i = 0; cReply.FindString( "config_names", &pzName, i ) == 0; ++i )
		{
			pcTable->push_back( pzName );
		}
		return ( 0 );
	}
	else
	{
		dbprintf( "Application::GetFontConfigNames() failed to send message to server: %s\n", strerror( errno ) );
		return ( -1 );
	}

}


int Application::GetDefaultFont( const String & cName, font_properties * psProps ) const
{
	Message cReq( DR_GET_DEFAULT_FONT );
	Message cReply;

	cReq.AddString( "config_name", cName );
	if( Messenger( m->m_hServerPort ).SendMessage( &cReq, &cReply ) >= 0 )
	{
		int nError = -EINVAL;

		cReply.FindInt( "error", &nError );
		if( nError < 0 )
		{
			errno = -nError;
			return ( -1 );
		}
		if( psProps != NULL )
		{
			cReply.FindString( "family", &psProps->m_cFamily );
			cReply.FindString( "style", &psProps->m_cStyle );
			cReply.FindFloat( "size", &psProps->m_vSize );
			cReply.FindFloat( "shear", &psProps->m_vShear );
			cReply.FindFloat( "rotation", &psProps->m_vRotation );
			cReply.FindInt32( "flags", ( int32 * )&psProps->m_nFlags );
		}
		return ( 0 );
	}
	else
	{
		dbprintf( "Application::GetFontConfigNames() failed to send message to server: %s\n", strerror( errno ) );
		return ( -1 );
	}

}

int Application::SetDefaultFont( const String & cName, const font_properties & sProps )
{
	Message cReq( DR_SET_DEFAULT_FONT );
	Message cReply;

	cReq.AddString( "config_name", cName );
	cReq.AddString( "family", sProps.m_cFamily );
	cReq.AddString( "style", sProps.m_cStyle );
	cReq.AddFloat( "size", sProps.m_vSize );
	cReq.AddFloat( "shear", sProps.m_vShear );
	cReq.AddFloat( "rotation", sProps.m_vRotation );
	cReq.AddInt32( "flags", sProps.m_nFlags );

	if( Messenger( m->m_hServerPort ).SendMessage( &cReq, &cReply ) >= 0 )
	{
		int nError = -EINVAL;

		cReply.FindInt( "error", &nError );
		if( nError < 0 )
		{
			errno = -nError;
			return ( -1 );
		}
		else
		{
			return ( 0 );
		}
	}
	else
	{
		dbprintf( "Application::GetDefaultFont() failed to send message to server: %s\n", strerror( errno ) );
		return ( -1 );
	}
}

int Application::AddDefaultFont( const String & cName, const font_properties & sProps )
{
	Message cReq( DR_ADD_DEFAULT_FONT );
	Message cReply;

	cReq.AddString( "config_name", cName );
	cReq.AddString( "family", sProps.m_cFamily );
	cReq.AddString( "style", sProps.m_cStyle );
	cReq.AddFloat( "size", sProps.m_vSize );
	cReq.AddFloat( "shear", sProps.m_vShear );
	cReq.AddFloat( "rotation", sProps.m_vRotation );
	cReq.AddInt32( "flags", sProps.m_nFlags );

	if( Messenger( m->m_hServerPort ).SendMessage( &cReq, &cReply ) >= 0 )
	{
		int nError = -EINVAL;

		cReply.FindInt( "error", &nError );
		if( nError < 0 )
		{
			errno = -nError;
			return ( -1 );
		}
		else
		{
			return ( 0 );
		}
	}
	else
	{
		dbprintf( "Application::GetDefaultFont() failed to send message to server: %s\n", strerror( errno ) );
		return ( -1 );
	}
}

/** \brief Get the number of font families installed on the system.
 *
 *  \return
 *	Number of font families that can be obtained through Application::GetFontFamily()
 *  \sa Application::GetFontFamily()
 *  \author	Kurt Skauen (kurt.skauen@c2i.net)
    *//////////////////////////////////////////////////////////////////////////////

int Application::GetFontFamilyCount()
{
	Message cReq( AR_GET_FONT_FAMILY_COUNT );

	Message cReply;

	if( Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply ) < 0 )
	{
		dbprintf( "Error: Application::GetFontFamilyCount() failed to send message: %s\n", strerror( errno ) );
		return ( -1 );
	}
	int nCount = -1;

	cReply.FindInt( "count", &nCount );
	return ( nCount );
}

/** \brief Obtaine the name of a font family.
 *
 *	Call this function to learn the name of the nIndex'th font family.
 *  \param nIndex - An integer between 0 and Application::GetFontFamilyCount() - 1
 *	   	    of the font familiy you are interrested in.
 *  \param pzFamily - Pointer to a buffer of at least FONT_FAMILY_LENGTH bytes
 *	   	      that will receive the family name
 *  \return
 *	If everyting when well 0 will be returned. Otherwise a negative number
 *	is returned, and the <errno> variable is set.
 *  \sa Application::GetFontFamilyCount(), Application::GetFontStyleCount(), Application::GetFontStyle()
 *  \author	Kurt Skauen (kurt.skauen@c2i.net)
    *//////////////////////////////////////////////////////////////////////////////

status_t Application::GetFontFamily( int nIndex, char *pzFamily )
{
	Message cReq( AR_GET_FONT_FAMILY );

	cReq.AddInt32( "index", nIndex );

	Message cReply;

	if( Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply ) < 0 )
	{
		dbprintf( "Error: Application::GetFontFamily() failed to send message: %s\n", strerror( errno ) );
		return ( -1 );
	}

	int nError = -1;

	cReply.FindInt( "error", &nError );

	if( nError >= 0 )
	{
		const char *pzName;

		cReply.FindString( "family", &pzName );
		strcpy( pzFamily, pzName );
	}
	else
	{
		errno = -nError;
	}
	return ( nError );
}

/** \brief Obtaine the number of styles belonging to a font family
 *
 *	Call this function to learn how many styles are embedded in a font family.
 *  \param pzFamily - Name of the family to investigate.
 *  \return The style count
 *  \sa Application::GetFontStyleCount(), GetFontFamilyCount(), GetFontFamily()
 *  \author	Kurt Skauen (kurt.skauen@c2i.net)
    *//////////////////////////////////////////////////////////////////////////////

int Application::GetFontStyleCount( const char *pzFamily )
{
	Message cReq( AR_GET_FONT_STYLE_COUNT );
	Message cReply;

	cReq.AddString( "family", pzFamily );

	if( Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply ) < 0 )
	{
		dbprintf( "Error: Application::GetFontStyleCount() failed to send message: %s\n", strerror( errno ) );
		return ( -1 );
	}
	int nCount = 0;

	cReply.FindInt( "count", &nCount );

	if( nCount < 0 )
	{
		errno = -nCount;
		nCount = -1;
	}
	return ( nCount );
}

/** \brief Get a font style.
 *
 *	Call this function to get the name and properties of a font style based
 *	on the family name, and an index into the family's list of styles.
 *
 *  \param pzFamily - Name of the family containing the style.
 *  \param nIndex - The index into the family's list of styles.
 *  \param pnFlags - Pointer to an uint32 that if not NULL will receive a
 *		     bitmask describing the font (see font_style_flags)
 *  \return
 *	If everyting when well 0 will be returned. Otherwise a negative number
 *	is returned, and the <errno> variable is set.
 *  \sa Application::GetFontStyleCount(), Application::GetFontFamilyCount()
 *  \sa Application::GetFontFamily()
 *  \author	Kurt Skauen (kurt.skauen@c2i.net)
    *//////////////////////////////////////////////////////////////////////////////

status_t Application::GetFontStyle( const char *pzFamily, int nIndex, char *pzStyle, uint32 *pnFlags )
{
	Message cReq( AR_GET_FONT_STYLE );
	Message cReply;


	cReq.AddString( "family", pzFamily );
	cReq.AddInt32( "index", nIndex );

	if( Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply ) < 0 )
	{
		dbprintf( "Error: Application::GetFontStyle() failed to send message: %s\n", strerror( errno ) );
		return ( -1 );
	}

	int nError = -EINVAL;

	cReply.FindInt( "error", &nError );

	if( nError >= 0 )
	{
		const char *pzStyleName = "";

		cReply.FindString( "style", &pzStyleName );
		cReply.FindInt( "flags", pnFlags );
		strcpy( pzStyle, pzStyleName );
		return ( 0 );
	}
	else
	{
		errno = -nError;
		return ( -1 );
	}
}

/** \brief Returnes the current state of keyboards qualifiers.
 *
 *	Sends a requester to the appserver to obtain the current
 *	state of keyboard qualifiers.
 *  \return
 *	A bit mask composed from os::qualifiers decribing which qualifiers
 *	currently pressed
 *  \sa
 *  \author	Kurt Skauen (kurt.skauen@c2i.net)
    *//////////////////////////////////////////////////////////////////////////////

uint32 Application::GetQualifiers()
{
	AR_GetQualifiers_s sReq( m->m_hReplyPort );
	AR_GetQualifiersReply_s sReply;

	Lock();

	if( send_msg( m->m_hSrvAppPort, AR_GET_QUALIFIERS, &sReq, sizeof( sReq ) ) != 0 )
	{
		Unlock();
		dbprintf( "Error: Application::GetQualifiers() failed to send request to server\n" );
		return ( 0 );
	}

	if( get_msg( m->m_hReplyPort, NULL, &sReply, sizeof( sReply ) ) < 0 )
	{
		Unlock();
		dbprintf( "Error: Application::GetQualifiers() failed to read reply from server\n" );
		return ( 0 );
	}
	Unlock();
	return ( sReply.m_nQualifiers );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

port_id Application::CreateWindow( View * pcTopView, const Rect & cFrame, const String & cTitle, uint32 nFlags, uint32 nDesktopMask, port_id hEventPort, int *phTopView )
{
	Message cReq( AR_OPEN_WINDOW );

	cReq.AddString( "title", cTitle.c_str() );
	cReq.AddInt32( "event_port", hEventPort );
	cReq.AddInt32( "flags", nFlags );
	cReq.AddInt32( "desktop_mask", nDesktopMask );
	cReq.AddRect( "frame", cFrame );
	cReq.AddPointer( "user_obj", pcTopView );

	Message cReply;

	Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply );

	port_id hCmdPort;

	cReply.FindInt( "top_view", phTopView );
	cReply.FindInt( "cmd_port", &hCmdPort );
	return ( hCmdPort );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

port_id Application::CreateWindow( View * pcTopView, int hBitmapHandle, int *phTopView )
{
	Message cReq( AR_OPEN_BITMAP_WINDOW );

	cReq.AddInt32( "bitmap_handle", hBitmapHandle );
	cReq.AddPointer( "user_obj", pcTopView );

	Message cReply;

	Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply );

	port_id hCmdPort;

	cReply.FindInt( "top_view", phTopView );
	cReply.FindInt( "cmd_port", &hCmdPort );

	return ( hCmdPort );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Application::CreateBitmap( int nWidth, int nHeight, color_space eColorSpc, uint32 nFlags, int *phHandle, int *phArea )
{
	AR_CreateBitmap_s sReq;
	AR_CreateBitmapReply_s sReply;

	sReq.hReply = m->m_hReplyPort;
	sReq.nFlags = nFlags;
	sReq.nWidth = nWidth;
	sReq.nHeight = nHeight;
	sReq.eColorSpc = eColorSpc;
	
	Lock();

	if( send_msg( m->m_hSrvAppPort, AR_CREATE_BITMAP, &sReq, sizeof( sReq ) ) != 0 )
	{
		Unlock();
		dbprintf( "Error: Application::CreateBitmap() failed to send request to server\n" );
		return ( -1 );
	}

	if( get_msg( m->m_hReplyPort, NULL, &sReply, sizeof( sReply ) ) < 0 )
	{
		Unlock();
		dbprintf( "Error: Application::CreateBitmap() failed to read reply from server\n" );
		return ( -1 );
	}
	Unlock();
	if( sReply.m_hHandle < 0 )
	{
		return ( sReply.m_hHandle );
	}
	*phHandle = sReply.m_hHandle;
	*phArea = sReply.m_hArea;
	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Application::CloneBitmap( int hHandle, int* phHandle, area_id* phArea, uint32 *pnFlags, int* pnWidth,
				int* pnHeight, color_space* peColorSpc )
{
	AR_CloneBitmap_s sReq;
	AR_CloneBitmapReply_s sReply;

	sReq.m_hReply = m->m_hReplyPort;
	sReq.m_hHandle = hHandle;

	Lock();

	if( send_msg( m->m_hSrvAppPort, AR_CLONE_BITMAP, &sReq, sizeof( sReq ) ) != 0 )
	{
		Unlock();
		dbprintf( "Error: Application::CloneBitmap() failed to send request to server\n" );
		return ( -1 );
	}

	if( get_msg( m->m_hReplyPort, NULL, &sReply, sizeof( sReply ) ) < 0 )
	{
		Unlock();
		dbprintf( "Error: Application::CloneBitmap() failed to read reply from server\n" );
		return ( -1 );
	}
	Unlock();
	if( sReply.m_hHandle < 0 )
	{
		return ( sReply.m_hHandle );
	}
	*phHandle = sReply.m_hHandle;
	*phArea = sReply.m_hArea;
	*pnFlags = sReply.m_nFlags;
	*pnWidth = sReply.m_nWidth;
	*pnHeight = sReply.m_nHeight;
	*peColorSpc = sReply.m_eColorSpc;
	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Application::DeleteBitmap( int nBitmap )
{
	AR_DeleteBitmap_s sReq( nBitmap );

	if( send_msg( m->m_hSrvAppPort, AR_DELETE_BITMAP, &sReq, sizeof( sReq ) ) != 0 )
	{
		dbprintf( "Error: Application::DeleteBitmap() failed to send request to server\n" );
		return ( -1 );
	}
	else
	{
		return ( 0 );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Application::CreateSprite( const Rect & cFrame, int nBitmapHandle, uint32 *pnHandle )
{
	AR_CreateSprite_s sReq( m->m_hReplyPort, cFrame, nBitmapHandle );

	Lock();

	if( send_msg( m->m_hSrvAppPort, AR_CREATE_SPRITE, &sReq, sizeof( sReq ) ) != 0 )
	{
		Unlock();
		dbprintf( "Error: Application::CreateSprite() failed to send request to server\n" );
		return ( -1 );
	}

	AR_CreateSpriteReply_s sReply;

	if( get_msg( m->m_hReplyPort, NULL, &sReply, sizeof( sReply ) ) < 0 )
	{
		Unlock();
		dbprintf( "Error: Application::CreateSprite() failed to read reply from server\n" );
		return ( -1 );
	}
	Unlock();
	
	*pnHandle = sReply.m_nHandle;
	return ( sReply.m_nError );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Application::DeleteSprite( uint32 nSprite )
{
	AR_DeleteSprite_s sReq( nSprite );

	if( send_msg( m->m_hSrvAppPort, AR_DELETE_SPRITE, &sReq, sizeof( sReq ) ) != 0 )
	{
		dbprintf( "Error: Application::DeleteSprite() failed to send request to server\n" );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Application::MoveSprite( uint32 nSprite, const Point & cNewPos )
{
	AR_MoveSprite_s sReq( nSprite, cNewPos );

	if( send_msg( m->m_hSrvAppPort, AR_MOVE_SPRITE, &sReq, sizeof( sReq ) ) != 0 )
	{
		dbprintf( "Error: Application::MoveSprite() failed to send request to server\n" );
	}
}

int Application::LockDesktop( int nDesktop, Message * pcDesktopParams )
{
	Message cReq( AR_LOCK_DESKTOP );

	cReq.AddInt32( "desktop", nDesktop );
	return ( Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, pcDesktopParams ) );
}

int Application::UnlockDesktop( int nCookie )
{
	return ( 0 );
}

int Application::SwitchDesktop( int nDesktop )
{
	Message cReq( DR_SET_DESKTOP );

	cReq.AddInt32( "desktop", nDesktop );
	return ( Messenger( m->m_hServerPort ).SendMessage( &cReq ) );
}

void Application::SetScreenMode( int nDesktop, uint32 nValidityMask, screen_mode * psMode )
{
	Message cReq( AR_SET_SCREEN_MODE );

	cReq.AddInt32( "desktop", nDesktop );
	cReq.AddIPoint( "resolution", IPoint( psMode->m_nWidth, psMode->m_nHeight ) );
	cReq.AddInt32( "color_space", psMode->m_eColorSpace );
	cReq.AddFloat( "refresh_rate", psMode->m_vRefreshRate );
	cReq.AddFloat( "h_pos", psMode->m_vHPos );
	cReq.AddFloat( "v_pos", psMode->m_vVPos );
	cReq.AddFloat( "h_size", psMode->m_vHSize );
	cReq.AddFloat( "v_size", psMode->m_vVSize );

	Message cReply;

	Messenger( m->m_hSrvAppPort ).SendMessage( &cReq, &cReply );
}

void Application::SetWindowDecorator( const char *pzPath )
{
	DR_SetWindowDecorator_s sReq;

	strcpy( sReq.m_zDecoratorPath, pzPath );
	if( send_msg( m->m_hServerPort, DR_SET_WINDOW_DECORATOR, &sReq, sizeof( sReq ) ) != 0 )
	{
		dbprintf( "Error: Application::SetWindowDecorator() failed to send request to server\n" );
	}
}

void Application::CommitColorConfig()
{
	Message cMsg( DR_SET_COLOR_CONFIG );

	for( int i = 0; i < COL_COUNT; ++i )
	{
		cMsg.AddColor32( "_colors", get_default_color( static_cast < default_color_t > ( i ) ) );
	}
	Messenger( m->m_hServerPort ).SendMessage( &cMsg );
}

void Application::GetKeyboardConfig( String * pcKeymapName, int *pnKeyDelay, int *pnKeyRepeat )
{
	Message cReq( DR_GET_KEYBOARD_CFG );
	Message cReply;

	Messenger( m->m_hServerPort ).SendMessage( &cReq, &cReply );

	const char *pzKeymap;

	if( pcKeymapName != NULL && cReply.FindString( "keymap", &pzKeymap ) == 0 )
	{
		*pcKeymapName = pzKeymap;
	}
	if( pnKeyDelay != NULL )
	{
		cReply.FindInt( "delay", pnKeyDelay );
	}
	if( pnKeyRepeat != NULL )
	{
		cReply.FindInt( "repeat", pnKeyRepeat );
	}
}

status_t Application::SetKeymap( const char *pzName )
{
	Message cReq( DR_SET_KEYBOARD_CFG );
	
	os::String cPath = os::Keymap::GetKeymapDirectory();
	cPath += pzName;
	cReq.AddString( "keymap", cPath );
	Messenger( m->m_hServerPort ).SendMessage( &cReq );
	return ( 0 );
}

status_t Application::SetKeyboardTimings( int nDelay, int nRepeat )
{
	Message cReq( DR_SET_KEYBOARD_CFG );

	cReq.AddInt32( "delay", nDelay );
	cReq.AddInt32( "repeat", nRepeat );
	Messenger( m->m_hServerPort ).SendMessage( &cReq );
	return ( 0 );
}

void Application::RegisterKeyEvent(const os::KeyboardEvent& event)
{
	Message cReq(DR_REGISTER_KEY_EVNT);
	
	cReq.AddString("event",event.GetEventName());
	cReq.AddObject("key",event.GetShortcutKey());
	cReq.AddString("app",GetName());
	
	Messenger( m->m_hServerPort ).SendMessage( &cReq );
	
	
	//store the key for deletion
	m->m_cKeyEvents.push_back(event);
}

void Application::UnregisterKeyEvent(const os::String& event)
{
	for (int i=0; i<m->m_cKeyEvents.size(); i++)
	{
		if (event == m->m_cKeyEvents[i].GetEventName())
		{
			Message cReq(DR_UNREGISTER_KEY_EVNT);
			cReq.AddString("app",GetName());
			cReq.AddObject("key",m->m_cKeyEvents[i].GetShortcutKey());
			Messenger( m->m_hServerPort ).SendMessage( &cReq );
			
			//remove it from our storage
			m->m_cKeyEvents.erase(m->m_cKeyEvents.begin()+i);
			return;
		}
	}
}

int Application::GetCurrentKeyShortcuts(std::vector<os::KeyboardEvent> *pcTable)
{
	Message cReq( DR_REGISTERED_KEY_EVNTS );
	Message cReply;
	int32 count;
	
	Messenger( m->m_hServerPort ).SendMessage( &cReq, &cReply );
	
	if (cReply.FindInt32("count",&count) == 0)
	{
		for (int i=0; i<count; i++)
		{
			os::String event;
			os::String cApp;
			os::ShortcutKey key;
			os::KeyboardEvent cKeyEvent;
			
			cReply.FindString("event",&event,i);
			cReply.FindObject("key",key,i);
			cReply.FindString("app",&cApp,i);
			
			cKeyEvent.SetEventName(event);
			cKeyEvent.SetShortcutKey(key);
			cKeyEvent.SetApplicationName(cApp); 
			
			pcTable->push_back(cKeyEvent);
		}
	}
	else
	{
		return (-1);
	}
	return (0);
}

void Application::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case M_COLOR_CONFIG_CHANGED:
		for( int i = 0; i < COL_COUNT; ++i )
		{
			Color32_s sColor;

			if( pcMessage->FindColor32( "_colors", &sColor, i ) != 0 )
			{
				break;
			}
			__set_default_color( static_cast < default_color_t > ( i ), sColor );
		}
//          for ( uint i = 0 ; i < m_cWindows.size() ; ++i ) {
//              m_cWindows[i]->_DefaultColorsChanged();
//          }
		break;
	default:
		Looper::HandleMessage( pcMessage );
		break;
	}
}

/** \brief Entry point for the message loop.
 *
 *	The Application class is different from other loopers in that it does not
 *	spawn a new thread when the Run() member is called. Instead the Run() member
 *	directly enter the message loop, and does not return until the message loop
 *	quits.
 *
 *  \return The thread id of the thread calling it.
 *  \sa
 *  \author	Kurt Skauen (kurt.skauen@c2i.net)
    *//////////////////////////////////////////////////////////////////////////////

thread_id Application::Run()
{
	thread_id hThread = get_thread_id( NULL );

	_SetThread( hThread );
	Unlock();
	_Loop();

	delete this;

	return ( hThread );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Application::AddWindow( Window * pcWindow )
{
	Lock();
	m->m_cWindows.push_back( pcWindow );
	Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Application::RemoveWindow( Window * pcWindow )
{
	Lock();
	std::vector < Window * >::iterator i = std::find( m->m_cWindows.begin(), m->m_cWindows.end(  ), pcWindow );

	if( i == m->m_cWindows.end() )
	{
		dbprintf( "Error: Application::RemoveWindow() could not find window %p\n", pcWindow );
	}

	if( i != m->m_cWindows.end() )
	{
		m->m_cWindows.erase( i );
	}
	Unlock();
}



void Application::__reserved1__()
{
}
void Application::__reserved2__()
{
}
void Application::__reserved3__()
{
}
void Application::__reserved4__()
{
}
void Application::__reserved5__()
{
}
void Application::__reserved6__()
{
}
void Application::__reserved7__()
{
}
void Application::__reserved8__()
{
}
void Application::__reserved9__()
{
}
void Application::__reserved10__()
{
}




















