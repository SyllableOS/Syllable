/*
 *  The Syllable application server
 *  Copyright (C) 1999 - 2000 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdexcept>

#include <atheos/types.h>
#include <atheos/threads.h>
#include <atheos/kernel.h>
#include <atheos/time.h>
#include <atheos/image.h>
#include <atheos/msgport.h>


#include <gui/guidefines.h>
#include <gui/window.h>
#include <gui/desktop.h>
#include <util/locker.h>
#include <util/messenger.h>
#include <appserver/protocol.h>
#include <atheos/filesystem.h>
#include <util/shortcutkey.h>

#include "ddriver.h"
#include "server.h"
#include "swindow.h"
#include "layer.h"
#include "toplayer.h"
#include "sfont.h"
#include "keyboard.h"
#include "sapplication.h"
#include "winselect.h"
#include "bitmap.h"
#include "clipboard.h"
#include "inputnode.h"
#include "config.h"
#include "sprite.h"
#include "defaultdecorator.h"
#include "wndborder.h"
#include "event.h"

using namespace os;

WinSelect*				g_pcWinSelector = NULL;
AppServer*				AppServer::s_pcInstance = NULL;

Array < Layer >*		g_pcLayers;
Array < SrvBitmap >*	g_pcBitmaps;

FontServer*				g_pcFontServer;

SrvApplication*			g_pcFirstApp = NULL;
TopLayer*				g_pcTopView = NULL;
DisplayDriver*			g_pcDispDrv = NULL;
SrvEvents* 				g_pcEvents = NULL;



/** Loads the decorator
 * \par 	Description:
 *		This method loads a decorator based on the path and then places it into
 *		the decorator passed to it.
 * \param	cPath			- The path of the decorator
 * \param	ppfCreate		- The decorator pointer 
 * \author Kurt Skauen
 *****************************************************************************/
int AppServer::LoadDecorator( const std::string & cPath, op_create_decorator **ppfCreate, op_unload_decorator** ppfUnload )
{
	int nPlugin;
	op_create_decorator *pfCreate = NULL;
	op_unload_decorator *pfUnload = NULL;
	
	nPlugin = load_library( cPath.c_str(), 0 );
	if( nPlugin < 0 )
	{
		dbprintf( "Error: Failed to load window decorator %s\n", cPath.c_str() );
		return ( -1 );
	}
	op_get_decorator_version *pfGetVersion;

	int nError;

	nError = get_symbol_address( nPlugin, "get_api_version", -1, ( void ** )&pfGetVersion );
	if( nError < 0 )
	{
		dbprintf( "Error: window decorator '%s' does not export get_api_version()\n", cPath.c_str() );
		unload_library( nPlugin );
		return ( -1 );
	}
	int nVersion = pfGetVersion();

	if( nVersion != WNDDECORATOR_APIVERSION )
	{
		if( nVersion < WNDDECORATOR_APIVERSION )
		{
			dbprintf( "Error: window decorator '%s' too old. VER=%d, CUR VER=%d\n", cPath.c_str(), nVersion, WNDDECORATOR_APIVERSION );
		}
		else
		{
			dbprintf( "Error: window decorator '%s' too new. VER=%d, CUR VER=%d\n", cPath.c_str(), nVersion, WNDDECORATOR_APIVERSION );
		}
		unload_library( nPlugin );
		return ( -1 );
	}

	nError = get_symbol_address( nPlugin, "create_decorator", -1, ( void ** )&pfCreate );
	if( nError < 0 )
	{
		dbprintf( "Error: window decorator '%s' does not export create_decorator()\n", cPath.c_str() );
		unload_library( nPlugin );
		return ( -1 );
	}
	*ppfCreate = pfCreate;
	
	nError = get_symbol_address( nPlugin, "unload_decorator", -1, ( void** )&pfUnload );
	if( nError < 0 ) {
		pfUnload = NULL;
	}
	*ppfUnload = pfUnload;
	return ( nPlugin );
}

/** Loads window decorator based on path
 * \par 	Description:
 *		This method is called by the appserver to load the window decorator
 * \param	cPath - The path to the decorator
 * \sa LoadDecorator
 * \author Kurt Skauen
 *****************************************************************************/
int AppServer::LoadWindowDecorator( const std::string & cPath )
{
	op_create_decorator *pfCreate = NULL;
	op_unload_decorator *pfUnload = NULL;
	int nLib;

	nLib = LoadDecorator( cPath, &pfCreate, &pfUnload );
	if( nLib >= 0 )
	{
		m_pfDecoratorCreator = pfCreate;
		int nOldPlugin = m_hCurrentDecorator;

		m_hCurrentDecorator = nLib;
		SrvApplication::ReplaceDecorators();
		if( m_pfDecoratorUnload != NULL ) m_pfDecoratorUnload();
		unload_library( nOldPlugin );
		m_pfDecoratorUnload = pfUnload;
		return ( 0 );
	}
	else
	{
		return ( -1 );
	}
}

/** Creates a window decorator
 * \par 	Description:
 *		This method is creates a window decorator with the passed view and flags
 * \param	pcView	- the view the decorator is attached to.
 * \param	nFlags	- the flags for the decorator
 * \author Kurt Skauen
 *****************************************************************************/
WindowDecorator *AppServer::CreateWindowDecorator( Layer * pcView, uint32 nFlags )
{
	if( m_pfDecoratorCreator != NULL )
	{
		return ( m_pfDecoratorCreator( pcView, nFlags ) );
	}
	else
	{
		return ( new DefaultDecorator( pcView, nFlags ) );
	}
}

/** Appserver constructor
 * \par 	Description:
 *		This method is the appserver constructor.  Here everything is initialized
 * \author Kurt Skauen
 *****************************************************************************/
AppServer::AppServer()
{
	s_pcInstance = this;
	m_pfDecoratorCreator = NULL;
	m_pfDecoratorUnload = NULL;
	
	g_pcEvents = new SrvEvents;
	
	m_pcProcessQuitEvent = g_pcEvents->RegisterEvent( "os/System/ProcessHasQuit", 0, "Called when a process has quit", -1, -1, -1 );
	m_pcClipboardEvent = g_pcEvents->RegisterEvent( "os/System/ClipboardHasChanged", 0, "Called when the clipboard content has changed", -1, -1, -1 );
	m_pcScreenshotEvent = g_pcEvents->RegisterEvent( "os/System/PrintKeyWasHit",0,"Called when the 'Print Screen' key has been hit",-1,-1,-1);

	dbprintf( "Load default fonts\n" );
	m_pcWindowTitleFont = new FontNode;
	m_pcToolWindowTitleFont = new FontNode;

	const font_properties *psProp;

	psProp = AppserverConfig::GetInstance()->GetFontConfig( DEFAULT_FONT_WINDOW );
	m_pcWindowTitleFont->SetProperties( *psProp );

	psProp = AppserverConfig::GetInstance()->GetFontConfig( DEFAULT_FONT_TOOL_WINDOW );
	m_pcToolWindowTitleFont->SetProperties( *psProp );

	m_hCurrentDecorator = LoadDecorator( AppserverConfig::GetInstance()->GetWindowDecoratorPath(  ).c_str(  ), &m_pfDecoratorCreator, &m_pfDecoratorUnload );
}

/** Returns *this* instance of the appserver
 * \par 	Description:
 *		This method will return this instance of the appserver.  Only one instance is allowed for each
 *		bootup
 * \author Kurt Skauen
 *****************************************************************************/
AppServer *AppServer::GetInstance()
{
	return ( s_pcInstance );
}

/**	Resets the event time to the current system time
 * \par 	Description:
 *		This method is the appserver constructor.  Here everything is initialized
 * \author Kurt Skauen
 *****************************************************************************/
void AppServer::ResetEventTime()
{
	m_nLastEvenTime = get_system_time();
}

/**
 * \par 	Description:
 *		This method sends M_QUIT to the app that we want to quit(based on the thread id) 
 *		and then it posts an event telling apps that it has been terminated
 * \author Kurt Skauen and Arno Klenke
 *****************************************************************************/
void AppServer::R_ClientDied( thread_id hClient )
{
	SrvApplication *pcApp = SrvApplication::FindApp( hClient );

	/* Tell the application thread to quit */
	if( NULL != pcApp )
	{
		send_msg( pcApp->GetReqPort(), M_QUIT, NULL, 0 );
	}
	
	/* Tell the registrar that the application has died
	   FIXME: Switch the registrar to the os::Event interface
	*/
	int nPort;
	if( ( nPort = find_port( "l:registrar" ) ) < 0 )
		return;
		
	
	Messenger cRegistrarLink = Messenger( nPort );
	Message cMsg( -1 );
	cMsg.AddInt64( "process", hClient );
	cRegistrarLink.SendMessage( &cMsg );
	
	/* Send this event to other applications */
	g_pcEvents->PostEvent( m_pcProcessQuitEvent, &cMsg );
}

/** Send a keyboard event
 * \par		Description:
 *		Called by the keyboard handler to input keyboard events to
 *		the appserver. Global hotkey sequences are filtered out
 *		here. Events that are not used by the appserver
 *		itself are passed on to the active window via SendKeyboardEvent.
 * \param	nKeyCode The raw key code (as delivered by the keyboard driver)
 * \param	nQual Qualifiers eg. QUAL_SHIFT, QUAL_CTRL etc.
 * \author Kurt Skauen, Henrik Isaksson
 *****************************************************************************/
void AppServer::SendKeyCode( int nKeyCode, int nQual )
{
	ResetEventTime();
	
	os::ShortcutKey key(nKeyCode,nQual);
	
	
	for (uint i=0; i<cKeyEvents.size(); i++)
	{
		if (key == cKeyEvents[i].cKey)
		{
			os::Message cMsg;
			g_pcEvents->PostEvent(cKeyEvents[i].pcEvent, &cMsg );
			return;
		}
	}
	
	
	/*these are the standard key shortcuts*/
	/*these can be overwritten by registering the same shortcut through another program*/
	if( 0x34 == nKeyCode )	// DEL
	{
		if( ( nQual & QUAL_ALT ) && ( nQual & QUAL_CTRL ) )
		{
			reboot();
			return;
		}
	}
	
	if( nKeyCode == 0x0e )
	{			// Print-screen
		os::Message cMsg;
		g_pcEvents->PostEvent( m_pcScreenshotEvent, &cMsg );		
		return;
	}

	if( ( nQual & QUAL_LALT ) && nKeyCode == 0x28 )
	{			// ALT-W
		g_cLayerGate.Close();
		SrvWindow *pcWnd = get_active_window( false );

		if( pcWnd != NULL )
		{
			Message cMsg( M_QUIT );

			pcWnd->PostUsrMessage( &cMsg );
		}
		g_cLayerGate.Open();
		return;
	}
	if( ( nQual & QUAL_LALT ) && nKeyCode == 0x27 )
	{			// ALT-Q
		g_cLayerGate.Close();
		SrvApplication *pcApp = get_active_app();

		if( pcApp != NULL )
		{
			Message cMsg( M_QUIT );

			pcApp->PostUsrMessage( &cMsg );
		}
		g_cLayerGate.Open();
		return;
	}

	if( 0x1e == nKeyCode )	// BACKSPACE
	{
		if( ( nQual & QUAL_ALT ) && ( nQual & QUAL_CTRL ) )
		{
			FILE *hFile;

			dbprintf( "Load configuration\n" );
			hFile = fopen( "/system/config/appserver", "r" );

			if( hFile != NULL )
			{
				AppserverConfig::GetInstance()->LoadConfig( hFile, true );
				fclose( hFile );
			}
			else
			{
				dbprintf( "Error: failed to open appserver configuration file: /system/config/appserver" );
			}
			return;
		}
	}

	if( g_pcWinSelector != NULL )
	{
		g_cLayerGate.Close();
		if( ( nQual & QUAL_LALT ) == 0 || nKeyCode == 0x01 || nKeyCode == 17 )
		{
			if( ( nKeyCode & 0x7f ) == 0x5d )
			{	// L-ALT up
				g_pcWinSelector->UpdateWinList( true, true );
			}
			else if( nKeyCode == 17 )
			{
				g_pcWinSelector->UpdateWinList( false, true );
			}
			else
			{
				g_pcWinSelector->UpdateWinList( false, false );
			}
			delete g_pcWinSelector;

			g_pcWinSelector = NULL;
			SrvSprite::Hide();
			g_pcTopView->UpdateRegions();
			SrvWindow::HandleMouseTransaction();
			SrvSprite::Unhide();
		}
		else if( nKeyCode == 0x26 )
		{
			g_pcWinSelector->Step( ( nQual & QUAL_SHIFT ) == 0 );
		}
		g_cLayerGate.Open();
		return;
	}
	if( nQual & QUAL_LALT )
	{
		if( nKeyCode >= 0x02 && nKeyCode <= 0x0d )	// F1-F12
		{
			SwitchDesktop( nKeyCode - 0x02, true );
			return;
		}
		if( nKeyCode == 17 )
		{
			SwitchDesktop( get_prev_desktop(), true );
			return;
		}
		if( nKeyCode == 0x26 )	// ALT
		{
			g_cLayerGate.Close();

			if( g_pcTopView->GetTopChild() != NULL )
			{
				g_pcWinSelector = new WinSelect();
				g_pcTopView->UpdateRegions();
				SrvWindow::HandleMouseTransaction();
			}
			g_cLayerGate.Open();
			return;
		}
	}

	if( !( ( nQual & QUAL_ALT ) && ( nQual & QUAL_CTRL ) && ( 0x1e == nKeyCode || 0x9e == nKeyCode ) ) )
	{			// BACKSPACE
		char zConvString[16];
		char zRawString[16];
		static int nDeadKeyState = 0;	// This variable keeps track of the last dead key hit (if any), otherwise NULL

		convert_key_code( zConvString, nKeyCode & 0x7f, nQual, ( nKeyCode & 0x80 ) ? NULL : &nDeadKeyState );
		convert_key_code( zRawString, nKeyCode & 0x7f, 0, NULL );

		SrvWindow::SendKbdEvent( nKeyCode, nQual | ( nDeadKeyState ? QUAL_DEADKEY : 0 ), zConvString, zRawString );
	}
}

/**
 * \par 	Description:
 *		A message dispatcher
 * \author Kurt Skauen
 *****************************************************************************/
void AppServer::DispatchMessage( Message * pcReq )
{
	switch ( pcReq->GetCode() )
	{
	case DR_CREATE_APP:
		{
			const char *pzName;
			proc_id hOwner = -1;
			port_id hEventPort = -1;

			pcReq->FindInt( "process_id", &hOwner );
			pcReq->FindInt( "event_port", &hEventPort );
			pcReq->FindString( "app_name", &pzName );

			SrvApplication *pcApp;

			try
			{
				pcApp = new SrvApplication( pzName, hOwner, hEventPort );
			} catch( ... )
			{
				Message cReply;

				cReply.AddInt32( "app_cmd_port", -1 );
				pcReq->SendReply( &cReply );
				break;
			}

			Message cReply;

			cReply.AddInt32( "app_cmd_port", pcApp->GetReqPort() );

			cReply.AddFloat( "cfg_shine_tint", 0.9f );
			cReply.AddFloat( "cfg_shadow_tint", 0.9f );

			for( int i = 0; i < COL_COUNT; ++i )
			{
				cReply.AddColor32( "cfg_colors", get_default_color( static_cast < default_color_t > ( i ) ) );
			}
			pcReq->SendReply( &cReply );
			break;
		}
	case DR_SET_COLOR_CONFIG:
		{
			for( int i = 0; i < COL_COUNT; ++i )
			{
				Color32_s sColor;

				if( pcReq->FindColor32( "_colors", &sColor, i ) != 0 )
				{
					break;
				}
				AppserverConfig::GetInstance()->SetDefaultColor( static_cast < default_color_t > ( i ), sColor );
			}
			SrvApplication::NotifyColorCfgChanged();
			break;
		}
	case DR_GET_KEYBOARD_CFG:
		{
			Message cReply;

			cReply.AddInt32( "delay", AppserverConfig::GetInstance()->GetKeyDelay(  ) );
			cReply.AddInt32( "repeat", AppserverConfig::GetInstance()->GetKeyRepeat(  ) );
			cReply.AddString( "keymap", AppserverConfig::GetInstance()->GetKeymapPath().c_str());
			pcReq->SendReply( &cReply );
			break;
		}
	case DR_SET_KEYBOARD_CFG:
		{
			const char *pzKeyMap;
			int32 nKeyDelay = 0;
			int32 nKeyRepeat = 0;

			if( pcReq->FindInt( "delay", &nKeyDelay ) == 0 )
			{
				AppserverConfig::GetInstance()->SetKeyDelay( nKeyDelay );
			}
			if( pcReq->FindInt( "repeat", &nKeyRepeat ) == 0 )
			{
				AppserverConfig::GetInstance()->SetKeyRepeat( nKeyRepeat );
			}

			if( pcReq->FindString( "keymap", &pzKeyMap ) == 0 )
			{
				AppserverConfig::GetInstance()->SetKeymapPath( pzKeyMap );
			}
			break;
		}
	case DR_RESCAN_FONTS:
		{
			Message cReply;

			FontServer::GetInstance()->ScanDirectory( "/system/fonts/" );
			cReply.AddBool( "changed", true );
			pcReq->SendReply( &cReply );
			break;
		}
	case DR_GET_DEFAULT_FONT_NAMES:
		{
			Message cReply;

			AppserverConfig::GetInstance()->GetDefaultFontNames( &cReply );
			pcReq->SendReply( &cReply );
			break;
		}
	case DR_GET_DEFAULT_FONT:
		{
			const char *pzConfigName;
			Message cReply;

			if( pcReq->FindString( "config_name", &pzConfigName ) != 0 )
			{
				cReply.AddInt32( "error", -EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			const font_properties *psProps = AppserverConfig::GetInstance()->GetFontConfig( pzConfigName );

			if( psProps == NULL )
			{
				cReply.AddInt32( "error", -EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			cReply.AddString( "family", psProps->m_cFamily );
			cReply.AddString( "style", psProps->m_cStyle );
			cReply.AddFloat( "size", psProps->m_vSize );
			cReply.AddFloat( "shear", psProps->m_vShear );
			cReply.AddFloat( "rotation", psProps->m_vRotation );
			cReply.AddInt32( "flags", psProps->m_nFlags );
			cReply.AddInt32( "error", 0 );
			pcReq->SendReply( &cReply );
			break;
		}
	case DR_SET_DEFAULT_FONT:
	case DR_ADD_DEFAULT_FONT:
		{
			const char *pzConfigName;
			Message cReply;

			if( pcReq->FindString( "config_name", &pzConfigName ) != 0 )
			{
				cReply.AddInt32( "error", -EINVAL );
				pcReq->SendReply( &cReply );
				break;
			}
			font_properties sProps;

			pcReq->FindString( "family", &sProps.m_cFamily );
			pcReq->FindString( "style", &sProps.m_cStyle );
			pcReq->FindFloat( "size", &sProps.m_vSize );
			pcReq->FindFloat( "shear", &sProps.m_vShear );
			pcReq->FindFloat( "rotation", &sProps.m_vRotation );
			pcReq->FindInt32( "flags", ( int32 * )&sProps.m_nFlags );

			if( pcReq->GetCode() == DR_SET_DEFAULT_FONT )
			{
				cReply.AddInt32( "error", AppserverConfig::GetInstance()->SetFontConfig( pzConfigName, sProps ) );
			}
			else
			{
				cReply.AddInt32( "error", AppserverConfig::GetInstance()->AddFontConfig( pzConfigName, sProps ) );
			}
			if( strcmp( pzConfigName, DEFAULT_FONT_WINDOW ) == 0 )
			{
				m_pcWindowTitleFont->SetProperties( sProps );
				SrvApplication::NotifyWindowFontChanged( false );
			}

			if( strcmp( pzConfigName, DEFAULT_FONT_TOOL_WINDOW ) == 0 )
			{
				m_pcToolWindowTitleFont->SetProperties( sProps );
				SrvApplication::NotifyWindowFontChanged( true );
			}

			pcReq->SendReply( &cReply );
			break;
		}
	case DR_SET_APPSERVER_CONFIG:
		{
			AppserverConfig::GetInstance()->SetConfig( pcReq );
			break;
		}
	case DR_GET_APPSERVER_CONFIG:
		{
			Message cReply;

			AppserverConfig::GetInstance()->GetConfig( &cReply );
			pcReq->SendReply( &cReply );
			break;
		}
	case DR_SET_DESKTOP:
		{
			int nDesktop = 0;

			pcReq->FindInt( "desktop", &nDesktop );
			if( nDesktop < 32 && nDesktop >= 0 )
				SwitchDesktop( nDesktop, false );
				/* 
				NOTE: As currently implemented (November 2006), appserver crashes could result if the user is
				dragging or resizing a window or clicking on a minimise, etc button when an app sends DR_SET_DESKTOP.
				This is because when bBringWindow is false in SwitchDesktop(), no checking is done if something is being clicked or dragged,
				and the appserver is left in an undefined state with the dragged/clicked window not attached to the screen.
				This does not occur when changing desktops via Alt+Fn as bBringWindow is true in that case.
				However, bBringWindow needs to be false here, as if it were true, clicking a 'change desktop' button in an app would always
				have the side-effect of bringing that app to the new desktop. In the case of the Dock & switcher, the dock would be moved
				off the old desktop to only the new desktop.
				  -- AWM  awmorp@gmail.com
				*/
			break;
		}
	case DR_MINIMIZE_ALL:
	{
		dbprintf("minimize all\n");
		int count=0;
		int nDesktop=0;
		
		SrvWindow *pcWindow;
		os::Message cReply;

		if( pcReq->FindInt( "desktop", &nDesktop ) != 0 ) {
			pcReq->SendReply( &cReply );
			break;
		}
		dbprintf("desktop: %d\n",nDesktop);
		g_cLayerGate.Close();

 		for( pcWindow = get_first_window( nDesktop ); pcWindow != NULL; pcWindow = pcWindow->m_asDTState[nDesktop].m_pcNextWindow )
		{
			if ( pcWindow->GetTopView()->IsVisible() && !pcWindow->IsMinimized() && !pcWindow->GetTopView()->IsBackdrop() && !(pcWindow->GetFlags() & WND_NO_BORDER)) 
			{
				dbprintf("window title: %s\n",pcWindow->GetTitle());
				
				pcWindow->SetMinimized( true );
				pcWindow->Show( false );
				remove_from_focusstack( pcWindow );
				
				pcWindow->GetTopView()->GetParent()->UpdateRegions(false);
				count++;
			}
		}
		g_cLayerGate.Open();
		cReply.AddInt32("minimized",count);
		pcReq->SendReply(&cReply);
		
		break;
	}
	case DR_CLOSE_WINDOWS:
		{
			g_cLayerGate.Close();
			int* pnAction = new int;
			*pnAction = 0;
			pcReq->FindInt( "action", pnAction );
			thread_id hCloseThread = spawn_thread( "close_thread", (void*)AppServer::CloseWindows, 0, 0, pnAction );

			resume_thread( hCloseThread );
			g_cLayerGate.Open();
			break;
		}
	case DR_SET_DESKTOP_MAX_WINFRAME:
		{
			int nDesktop = 0;
			os::Rect cFrame;

			if( pcReq->FindInt( "desktop", &nDesktop ) != 0 )
				break;
			if( pcReq->FindRect( "frame", &cFrame ) != 0 )
				break;
			g_cLayerGate.Close();
			if( ( nDesktop < 32 && nDesktop >= 0 ) || ( nDesktop == os::Desktop::ACTIVE_DESKTOP ) )
				set_desktop_max_window_frame( nDesktop, cFrame );
			g_cLayerGate.Open();
			
			if( pcReq->IsSourceWaiting() )
			{
				os::Message cReply;
				pcReq->SendReply( &cReply );
			}
			break;
		}
	case DR_GET_DESKTOP_MAX_WINFRAME:
		{
			int nDesktop = 0;
			os::Rect cFrame;
			os::Message cReply;

			if( pcReq->FindInt( "desktop", &nDesktop ) != 0 )
				break;
			g_cLayerGate.Close();				
			if( ( nDesktop < 32 && nDesktop >= 0 ) || ( nDesktop == os::Desktop::ACTIVE_DESKTOP ) )
				cFrame = get_desktop_max_window_frame( nDesktop );
			g_cLayerGate.Open();
			
			cReply.AddRect( "frame", cFrame );
			
			pcReq->SendReply( &cReply );
			break;
		}
	case DR_GET_MOUSE_CFG:
		{
			Message cReply;

			cReply.AddFloat( "speed", AppserverConfig::GetInstance()->GetMouseSpeed() );
			cReply.AddFloat( "acceleration", AppserverConfig::GetInstance()->GetMouseAcceleration() );
			cReply.AddInt32( "doubleclick", AppserverConfig::GetInstance()->GetDoubleClickTime() );
			cReply.AddBool( "swapbuttons", AppserverConfig::GetInstance()->GetMouseSwapButtons() );
			pcReq->SendReply( &cReply );
			break;
		}
	case DR_SET_MOUSE_CFG:
		{
			float nMouseSpeed = 0;
			float nMouseAcceleration = 0;
			int32 nMouseDoubleClick = 0 ;
			bool bMouseSwapButtons = false;
			if( pcReq->FindFloat( "speed", &nMouseSpeed ) == 0 )
			{
				AppserverConfig::GetInstance()->SetMouseSpeed( nMouseSpeed );
			}
			if( pcReq->FindFloat( "acceleration", &nMouseAcceleration ) == 0 )
			{
				AppserverConfig::GetInstance()->SetMouseAcceleration( nMouseAcceleration );
			}
			if( pcReq->FindInt( "doubleclick", &nMouseDoubleClick ) == 0 )
			{
				AppserverConfig::GetInstance()->SetDoubleClickTime( nMouseDoubleClick );
			}
			if( pcReq->FindBool( "swapbuttons", &bMouseSwapButtons ) == 0 )
			{
				AppserverConfig::GetInstance()->SetMouseSwapButtons( bMouseSwapButtons );
			}

			break;
		}
		
	case DR_REGISTER_KEY_EVNT:
		{
			KeyEvent_s cKeyEvent;
			
			

			pcReq->FindString( "event", &cKeyEvent.cEventName );
			pcReq->FindObject( "key", cKeyEvent.cKey);
			pcReq->FindString( "app", &cKeyEvent.cApplication);		
			cKeyEvent.pcEvent = g_pcEvents->RegisterEvent(cKeyEvent.cEventName,0,cKeyEvent.cEventName,-1,-1,-1);
		
			cKeyEvents.push_back(cKeyEvent);
			break;
		}
		
	case DR_UNREGISTER_KEY_EVNT:
		{
			os::ShortcutKey cKey;
			os::String cApp;
			
			pcReq->FindObject("key",cKey);
			pcReq->FindString("app",&cApp);
			
			for (uint i=0; i<cKeyEvents.size(); i++)
			{
				if ( (cKeyEvents[i].cApplication == cApp)  && (cKey == cKeyEvents[i].cKey) )
				{
					g_pcEvents->UnregisterEvent(cKeyEvents[i].cEventName,0,-1);
					cKeyEvents[i].pcEvent = NULL;
					cKeyEvents.erase(cKeyEvents.begin()+i);
					break;
				}
			}
			break;
		}
		
	case DR_REGISTERED_KEY_EVNTS:
		{
			os::Message cReply;
			cReply.AddInt32("count",cKeyEvents.size());
			
			for (uint i=0; i<cKeyEvents.size(); i++)
			{
				cReply.AddString("event",cKeyEvents[i].cEventName);
				cReply.AddObject("key",cKeyEvents[i].cKey);
				cReply.AddString("app",cKeyEvents[i].cApplication);
			}
			
			pcReq->SendReply(&cReply);
			break;
		}
	}
}

/** Runs the appserver thread
 * \par 	Description:
 *		This method runs the appserver thread and waits for appserver messages to act on
 * \author Kurt Skauen
 *****************************************************************************/
void AppServer::Run( void )
{
	enum
	{ e_MessageSize = 1024 * 64 };
	uint8 *pBuffer = new uint8[e_MessageSize];

	set_thread_priority( get_thread_id( NULL ), DISPLAY_PRIORITY );

	m_hRequestPort = create_port( "gui_server_cmd", DEFAULT_PORT_SIZE );


	init_desktops();

	InitInputSystem();

	set_app_server_port( m_hRequestPort );

	int nFile = open( "/system/fonts/", O_RDONLY | O_NOTRAVERSE );

	if( nFile > -1 )
	{
		watch_node( nFile, m_hRequestPort, NWATCH_DIR, ( void * )NULL );	// (void*)NULL = userdata, not used since we only have one node monitor
	}


	for( ;; )
	{
		uint32 nCode;

		if( get_msg_x( m_hRequestPort, &nCode, pBuffer, e_MessageSize, 1000000 ) >= 0 )
		{
			if( AppserverConfig::GetInstance()->IsDirty(  ) )
			{
				static bigtime_t nLastSaved = 0;
				bigtime_t nCurTime = get_system_time();

				if( nCurTime > nLastSaved + 1000000 )
				{
					AppserverConfig::GetInstance()->SaveConfig(  );
					nLastSaved = nCurTime;
				}
			}
			switch ( nCode )
			{
			case -1:
				R_ClientDied( ( ( DR_ThreadDied_s * ) pBuffer )->hThread );
				break;
			case DR_CREATE_APP:
			case DR_SET_COLOR_CONFIG:
			case DR_GET_KEYBOARD_CFG:
			case DR_SET_KEYBOARD_CFG:
			case DR_RESCAN_FONTS:
			case DR_GET_DEFAULT_FONT_NAMES:
			case DR_GET_DEFAULT_FONT:
			case DR_SET_DEFAULT_FONT:
			case DR_ADD_DEFAULT_FONT:
			case DR_SET_APPSERVER_CONFIG:
			case DR_GET_APPSERVER_CONFIG:
			case DR_SET_DESKTOP:
			case DR_CLOSE_WINDOWS:
			case DR_SET_DESKTOP_MAX_WINFRAME:
			case DR_GET_DESKTOP_MAX_WINFRAME:
			case DR_MINIMIZE_ALL:
			case DR_GET_MOUSE_CFG:
			case DR_SET_MOUSE_CFG:
			case DR_REGISTER_KEY_EVNT:				
			case DR_UNREGISTER_KEY_EVNT:
			case DR_REGISTERED_KEY_EVNTS:			
				{
					try
					{
						Message cReq( pBuffer );

						DispatchMessage( &cReq );
					}
					catch( ... )
					{
						dbprintf( "Error: Catched exception while handling request %d\n", nCode );
					}
					break;
				}
			case DR_SET_CLIPBOARD_DATA:
				{
					DR_SetClipboardData_s *psReq = ( DR_SetClipboardData_s * ) pBuffer;

					SrvClipboard::SetData( psReq->m_zName, psReq->m_anBuffer, psReq->m_nTotalSize );
					os::Message cMsg;
					g_pcEvents->PostEvent( m_pcClipboardEvent, &cMsg );
					break;
				}
			case DR_GET_CLIPBOARD_DATA:
				{
					DR_GetClipboardData_s *psReq = ( DR_GetClipboardData_s * ) pBuffer;
					DR_GetClipboardDataReply_s sReply;
					int nSize;
					uint8 *pData = SrvClipboard::GetData( psReq->m_zName, &nSize );


					if( pData == NULL || nSize == 0 )
					{
						sReply.m_nTotalSize = 0;
						sReply.m_nFragmentSize = 0;
						send_msg( psReq->m_hReply, 0, &sReply, sizeof( sReply ) - CLIPBOARD_FRAGMENT_SIZE );
					}
					else
					{
						sReply.m_nTotalSize = nSize;
						int nOffset = 0;

						while( nSize > 0 )
						{
							int nCurSize = std::min( CLIPBOARD_FRAGMENT_SIZE, nSize );

							memcpy( sReply.m_anBuffer, pData + nOffset, nCurSize );
							sReply.m_nFragmentSize = nCurSize;
							send_msg( psReq->m_hReply, 0, &sReply, sizeof( sReply ) - CLIPBOARD_FRAGMENT_SIZE + nCurSize );
							nSize -= nCurSize;
							nOffset += nCurSize;
						}
					}
					break;
				}
			case DR_SET_WINDOW_DECORATOR:
				{
					const DR_SetWindowDecorator_s *psReq = reinterpret_cast < const DR_SetWindowDecorator_s * >( pBuffer );

					AppserverConfig::GetInstance()->SetWindowDecoratorPath( psReq->m_zDecoratorPath );
					break;
				}
			case EV_REGISTER:
			case EV_UNREGISTER:
			case EV_GET_INFO:
			case EV_GET_LAST_EVENT_MESSAGE:
			case EV_ADD_MONITOR:
			case EV_REMOVE_MONITOR:
			case EV_CALL:
			case EV_GET_CHILDREN:
				{
					try
					{
						Message cReq( pBuffer );

						g_pcEvents->DispatchMessage( &cReq );
					}
					catch( ... )
					{
						dbprintf( "Error: Catched exception while handling request %d\n", nCode );
					}
					break;
				}
			case M_NODE_MONITOR:
				{
					dbprintf( "Font directory has changed, scanning font directory!\n" );
					FontServer::GetInstance()->ScanDirectory( "/system/fonts/" );
					break;
				}
			
			default:
				dbprintf( "WARNING : AppServer::Run() Unknown command %d\n", nCode );
				break;
			}
		}
	}
}

/** Close all opened windows.
 * \par		Description:
 *		Called after receiving a DR_CLOSE_WINDOWS message. The method
 *		will close all windows. It will run in its own thread.
 * \author Arno Klenke
 *****************************************************************************/
int32 AppServer::CloseWindows( void *pData )
{
	int* pnAction = (int*)pData;
	
	restart:
	SrvApplication::LockAppList();
	SrvApplication* pcApp = SrvApplication::GetFirstApp();
	
	/* Close all applications */
	while( pcApp )
	{
		if( !( pcApp->GetName() == "application/syllable-Desktop" ||
			   pcApp->GetName() == "media_server" ||
			   pcApp->GetName() == "registrar" ||
			   pcApp->GetName() == "application/x-vnd.syllable-login_application" ) )
		{
			if( !pcApp->IsClosing() )
			{
				dbprintf( "Closing %s ... \n", pcApp->GetName().c_str() );
				pcApp->SetClosing( true );
				os::Message cMsg( os::M_QUIT );
				if( pcApp->GetName() == "application/syllable-Dock" )
					cMsg.SetCode( os::M_TERMINATE );
				pcApp->PostUsrMessage( &cMsg );
			}
			SrvApplication::UnlockAppList();
			snooze( 100000 );
			goto restart;
		}
		pcApp = pcApp->GetNextApp();
	}
	
	/* Close the desktop */
	pcApp = SrvApplication::GetFirstApp();
	while( pcApp )
	{
		if( pcApp->GetName() == "application/syllable-Desktop" && !pcApp->IsClosing() )
		{
			dbprintf( "Closing desktop...\n" );
			os::Message cMsg( os::M_TERMINATE );
			pcApp->SetClosing( true );
			pcApp->PostUsrMessage( &cMsg );
			break;
		}
		pcApp = pcApp->GetNextApp();
	}
	SrvApplication::UnlockAppList();

	g_pcDispDrv->LockBitmap( g_pcScreenBitmap, NULL, IRect(), IRect() );
	if( *pnAction == 1 ) { apm_poweroff(); for(;;) { snooze( 10000 ); } } 
	else if( *pnAction == 2 ) { reboot(); for(;;) { snooze( 10000 );} }
	g_pcDispDrv->UnlockBitmap( g_pcScreenBitmap, NULL, IRect(), IRect() );

	return ( 0 );
}

/** Changes the desktop
 * \par 	Description:
 *		This method switches to the desktop passed to it and also will bring the active
 *		window along if wanted.
 * \param	nDesktop		- the desktop number to switch to.
 * \param	bBringWindow	- should we bring the active window along 
 * \author ???
 *****************************************************************************/
void AppServer::SwitchDesktop( int nDesktop, bool bBringWindow )
{
	if( nDesktop == get_active_desktop() ) return;
	
	g_cLayerGate.Close();

	SrvWindow *pcActiveWnd = get_active_window( false );

	SrvSprite::Hide();
	/* See note in DispatchMessage(), DR_SET_DESKTOP, about bBringWindow = false 
	   - appserver will crash if user is clicking on a titlebar button (eg minimize) when SwitchDesktop( n, false ) is called */
	if( bBringWindow && pcActiveWnd != NULL && ( InputNode::GetMouseButtons() ) ) /* If clicking on a window, bring it to new desktop */
	{
		Rect cFrame = pcActiveWnd->GetFrame();
		uint32 nNewMask = pcActiveWnd->GetDesktopMask();

		if( !(InputNode::GetMouseButtons() & 0x02) && !(nNewMask == os::ALL_DESKTOPS) )
		{
			nNewMask &= ~(1 << get_active_desktop());  /* Remove it from the old desktop */
		}
		nNewMask |= 1 << nDesktop;   /* Add window to new desktop */
		pcActiveWnd->SetDesktopMask( nNewMask );
		set_active_window( pcActiveWnd ); /* Restore focus to pcActiveWnd, which is messed up by SetDesktopMask() */
		
		/* If moving a window between desktops, make it keep its previous frame */
		pcActiveWnd->m_asDTState[nDesktop].m_cFrame = cFrame;

		set_desktop( nDesktop, false ); 
		set_active_window( pcActiveWnd );
	}
	else
	{
		set_desktop( nDesktop, true );
	}
	
	g_pcTopView->UpdateRegions();
	SrvSprite::Unhide();
	g_cLayerGate.Open();
}

/** main
 *****************************************************************************/
int main( int argc, char **argv )
{
	dbprintf( "Appserver Alive %d\n", get_thread_id( NULL ) );

	signal( SIGINT, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGTERM, SIG_IGN );
	
	g_pcFontServer = new FontServer();

	g_pcBitmaps = new Array < SrvBitmap >;
	g_pcLayers = new Array < Layer >;

	AppserverConfig *pcConfig = new AppserverConfig();
	screen_mode sMode;


	sMode.m_nWidth = 640;
	sMode.m_nHeight = 480;
	sMode.m_nBytesPerLine = 1280;
	sMode.m_eColorSpace = CS_RGB16;
	sMode.m_vRefreshRate = 60.0f;
	sMode.m_vHPos = 80;
	sMode.m_vVPos = 50;
	sMode.m_vHSize = 70;
	sMode.m_vVSize = 80;

	for( int i = 0; i < 32; ++i )
	{
		set_desktop_config( i, sMode, "" );
	}

	FILE *hFile;

	dbprintf( "Load configuration\n" );
	hFile = fopen( "/system/config/appserver", "r" );

	if( hFile != NULL )
	{
		pcConfig->LoadConfig( hFile, false );
		fclose( hFile );
	}
	else
	{
		dbprintf( "Error: failed to open appserver configuration file: /system/config/appserver" );
	}
	dbprintf( "Start keyboard thread\n" );

	InitKeyboard();

	AppServer *pcDevice = new AppServer();

	pcDevice->Run();
	dbprintf( "WARNING : layers.device failed to initiate itself!!!\n" );
	return ( 0 );
}


