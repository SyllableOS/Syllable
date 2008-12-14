/*  Syllable Dock
 *  Copyright (C) 2003 Arno Klenke
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
 

#include <atheos/image.h>
#include <appserver/protocol.h>
#include <gui/requesters.h>
#include <util/settings.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include "Dock.h"
#include "resources/Dock.h"


enum 
{
	DOCK_QUIT = 10000,
	DOCK_MENU_CLOSED,
	DOCK_OPEN_APP,
	DOCK_ABOUT,
	DOCK_ABOUT_ALERT,
	DOCK_QUIT_ALERT,
	DOCK_WINDOW_EV,
	DOCK_APP_LIST_EV
};



static os::Locker g_cWindowLock( "dock_lock" );

/* From the Photon Decorator */
static os::Color32_s Tint( const os::Color32_s & sColor, float vTint )
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
	return ( os::Color32_s( r, g, b, sColor.alpha ) );
}

/* Colour blending function stolen (does "standard" alpha-blend between 2 colours) */
static os::Color32_s BlendColours( const os::Color32_s& sColour1, const os::Color32_s& sColour2, float vBlend )
{
	int r = int( (float(sColour1.red)   * vBlend + float(sColour2.red)   * (1.0f - vBlend)) );
	int g = int( (float(sColour1.green) * vBlend + float(sColour2.green) * (1.0f - vBlend)) );
	int b = int( (float(sColour1.blue)  * vBlend + float(sColour2.blue)  * (1.0f - vBlend)) );
	if ( r < 0 ) r = 0; else if (r > 255) r = 255;
	if ( g < 0 ) g = 0; else if (g > 255) g = 255;
	if ( b < 0 ) b = 0; else if (b > 255) b = 255;
	return os::Color32_s(r, g, b, sColour1.alpha);
}

DockView::DockView( DockWin* pcWin, os::Rect cFrame )
		: os::View( cFrame, "dock_view", os::CF_FOLLOW_ALL )
{
	m_pcWin = pcWin;
	m_nCurrentIcon = -1;
	m_pcCurrentIcon = NULL;
	m_pcCurrentInfo = NULL;
	m_pcSyllableMenu = NULL;
	m_bSyllableMenuOpened = false;
	m_bSyllableMenuInvalid = true;
}

DockView::~DockView()
{
}

void DockView::Paint( const os::Rect& cUpdateRect )
{
	bool bHorizontal = m_pcWin->GetPosition() == os::ALIGN_TOP || m_pcWin->GetPosition() == os::ALIGN_BOTTOM;
	uint32 nDesktopMask = 1 << m_pcWin->GetDesktop();
	/* Draw the general background */
	os::Color32_s sCurrentColor = BlendColours(get_default_color(os::COL_SHINE),  get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	os::Color32_s sBottomColor = BlendColours(get_default_color(os::COL_SHADOW), get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	
	os::Color32_s sColorStep = os::Color32_s( ( sCurrentColor.red-sBottomColor.red ) / 30, 
											( sCurrentColor.green-sBottomColor.green ) / 30, 
											( sCurrentColor.blue-sBottomColor.blue ) / 30, 0 );
	
	SetFgColor( Tint( get_default_color( os::COL_SHADOW ), 0.5f ) );
	
	if( bHorizontal ) {
		if( cUpdateRect.DoIntersect( os::Rect( 0, 30, GetBounds().right, 30 ) ) )
			DrawLine( os::Point( cUpdateRect.left, 30 ), os::Point( cUpdateRect.right, 30 ) );
	} else {
		if( cUpdateRect.DoIntersect( os::Rect( 30, 0, 30, GetBounds().bottom ) ) )
			DrawLine( os::Point( 30, cUpdateRect.top ), os::Point( 30, cUpdateRect.bottom ) );
	}

	if( bHorizontal )
	{
		if( cUpdateRect.DoIntersect( os::Rect( 0, 0, GetBounds().right, 29 ) ) )
		{
			sCurrentColor.red -= (int)cUpdateRect.top * sColorStep.red;
			sCurrentColor.green -= (int)cUpdateRect.top * sColorStep.green;
			sCurrentColor.blue -= (int)cUpdateRect.top * sColorStep.blue;
			for( int i = (int)cUpdateRect.top; i < ( (int)cUpdateRect.bottom < 30 ? (int)cUpdateRect.bottom + 1 : 30 ); i++ )
			{				
				SetFgColor( sCurrentColor );
				DrawLine( os::Point( cUpdateRect.left, i ), os::Point( cUpdateRect.right, i ) );
				sCurrentColor.red -= sColorStep.red;
				sCurrentColor.green -= sColorStep.green;
				sCurrentColor.blue -= sColorStep.blue;
			}
		}
	}
	else
	{
		if( cUpdateRect.DoIntersect( os::Rect( 0, 0, 29, GetBounds().bottom ) ) )
		{
			sCurrentColor.red -= (int)cUpdateRect.left * sColorStep.red;
			sCurrentColor.green -= (int)cUpdateRect.left * sColorStep.green;
			sCurrentColor.blue -= (int)cUpdateRect.left * sColorStep.blue;
			for( int i = (int)cUpdateRect.left; i < ( (int)cUpdateRect.right < 30 ? (int)cUpdateRect.right + 1 : 30 ); i++ )
			{
				SetFgColor( sCurrentColor );
				DrawLine( os::Point( i, cUpdateRect.top ), os::Point( i, cUpdateRect.bottom ) );
				sCurrentColor.red -= sColorStep.red;
				sCurrentColor.green -= sColorStep.green;
				sCurrentColor.blue -= sColorStep.blue;
			}
		}
	}
	
	/* Draw the background of the Syllable logo if the menu is opened */
	if( m_bSyllableMenuOpened && cUpdateRect.DoIntersect( os::Rect( 0, 0, 30, 30 ) ) )
	{
		os::Rect cRect = cUpdateRect & os::Rect( 0, 0, 30, 30 );
		os::Color32_s sBottomColor = BlendColours(get_default_color(os::COL_SHADOW), get_default_color(os::COL_SEL_WND_BORDER), 0.5f);
		os::Color32_s sCurrentColor = BlendColours(get_default_color(os::COL_SHINE),  get_default_color(os::COL_SEL_WND_BORDER), 0.5f);
		os::Color32_s sColorStep = os::Color32_s( ( sCurrentColor.red-sBottomColor.red ) / 30, 
											( sCurrentColor.green-sBottomColor.green ) / 30, 
											( sCurrentColor.blue-sBottomColor.blue ) / 30, 0 );
		sCurrentColor.red -= (int)cRect.top * sColorStep.red;
		sCurrentColor.green -= (int)cRect.top * sColorStep.green;
		sCurrentColor.blue -= (int)cRect.top * sColorStep.blue;
		for( int i = (int)cRect.top; i < ( (int)cRect.bottom < 30 ? (int)cRect.bottom + 1 : 30 ); i++ )
		{
			SetFgColor( sCurrentColor );	
			DrawLine( os::Point( cRect.left, i ), os::Point( cRect.right, i ) );
			sCurrentColor.red -= sColorStep.red;
			sCurrentColor.green -= sColorStep.green;
			sCurrentColor.blue -= sColorStep.blue;
		}
	}
	
	/* Draw the icons */
	SetDrawingMode( os::DM_BLEND );
	g_cWindowLock.Lock();
	uint nIcon = 0;
	for( uint i = 0; i < m_pcWin->GetIcons().size(); i++ )
	{
		os::Rect cIconFrame;
		os::Point cIconPos;
		
		if( !m_pcWin->GetIcons()[i]->GetVisible( nDesktopMask ) )
			continue;
			
		if( bHorizontal )
		{
			cIconFrame = os::Rect( 4 + nIcon * 30, 4, 4 + nIcon * 30 + 23, 4 + 23 );
			cIconPos = os::Point( 4 + nIcon * 30, 4 );
		} else {
			cIconFrame = os::Rect( 4, 4 + nIcon * 30, 4 + 23, 4 + nIcon * 30 + 23 );
			cIconPos = os::Point( 4, 4 + nIcon * 30 );
		}
		
		if( cUpdateRect.DoIntersect( cIconFrame ) ) 
			if( m_nCurrentIcon == (int)nIcon )
				m_pcCurrentIcon->Draw( cIconPos, this );
			else
			{
				if( m_pcWin->GetIcons()[i]->GetMinimized() )
					m_pcWin->GetIcons()[i]->GetMinimizedBitmap()->Draw( cIconPos, this );
				else
					m_pcWin->GetIcons()[i]->GetBitmap()->Draw( cIconPos, this );
			}	
		nIcon++;
	}
	g_cWindowLock.Unlock();
	SetDrawingMode( os::DM_COPY );
}

void DockView::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	bool bIconFound = false;
	bool bHorizontal = m_pcWin->GetPosition() == os::ALIGN_TOP || m_pcWin->GetPosition() == os::ALIGN_BOTTOM;
	os::Rect cLastIconFrame;
	if( bHorizontal )
		cLastIconFrame = os::Rect( 4 + m_nCurrentIcon * 30, 4, 4 + m_nCurrentIcon * 30 + 23, 4 + 23 );
	else
		cLastIconFrame = os::Rect( 4, 4 + m_nCurrentIcon * 30, 4 + 23, 4 + m_nCurrentIcon * 30 + 23 );
	uint32 nDesktopMask = 1 << m_pcWin->GetDesktop();

	if( nCode == os::MOUSE_INSIDE )
	{
		g_cWindowLock.Lock();
		/* Check mouse position */
		uint nIcon = 0;
		for( uint i = 0; i < m_pcWin->GetIcons().size(); i++ )
		{
			os::Rect cIconFrame;
			
			if( !m_pcWin->GetIcons()[i]->GetVisible( nDesktopMask ) )
				continue;
			
			if( bHorizontal )
			{
				cIconFrame = os::Rect( 4 + nIcon * 30, 4, 4 + nIcon * 30 + 23, 4 + 23 );
			} else {
				cIconFrame = os::Rect( 4, 4 + nIcon * 30, 4 + 23, 4 + nIcon * 30 + 23 );
			}
			
			if( cIconFrame.DoIntersect( cNewPos ) )
			{
				/* Select the current icon */
				if( m_nCurrentIcon != (int)nIcon )
				{
					/* Update the previous icon and delete its highlighted icon and info window */
					Invalidate( cLastIconFrame );
					m_nCurrentIcon = nIcon;
					if( m_pcCurrentIcon )
						delete( m_pcCurrentIcon );
					m_pcCurrentIcon = NULL;
					if( m_pcCurrentInfo )
						m_pcCurrentInfo->Close();
					m_pcCurrentInfo = NULL;
					/* Create a highlighted copy of the current icon */
					m_pcCurrentIcon = new os::BitmapImage( *m_pcWin->GetIcons()[i]->GetBitmap() );
					m_pcCurrentIcon->ApplyFilter( os::BitmapImage::F_HIGHLIGHT );
					Invalidate( cIconFrame );
					Flush();
					
					/* Create an information window */
					os::Rect cFrame;
					if( m_pcWin->GetPosition() == os::ALIGN_TOP )
					{
						cFrame.top = 31;
						cFrame.left = cIconFrame.left;
					}
					else if( m_pcWin->GetPosition() == os::ALIGN_BOTTOM )
					{
						cFrame.top = m_pcWin->GetFrame().top - GetFont()->GetSize() - 8;
						cFrame.left = cIconFrame.left;
					} else if( m_pcWin->GetPosition() == os::ALIGN_LEFT )
					{
						cFrame.top = cIconFrame.top;
						cFrame.left = 31;
					} else 
					{
						cFrame.top = cIconFrame.top;
						cFrame.left = m_pcWin->GetFrame().left - GetStringWidth( m_pcWin->GetIcons()[i]->GetTitle() );
					}
					
					cFrame.bottom = cFrame.top + GetFont()->GetSize() + 8;
					
					cFrame.right = cFrame.left + GetStringWidth( m_pcWin->GetIcons()[i]->GetTitle() );
					m_pcCurrentInfo = new os::Window( cFrame, "dock_info", "Dock", os::WND_NO_BORDER );
					os::StringView* pcView = new os::StringView( m_pcCurrentInfo->GetBounds(), "dock_info", 
																m_pcWin->GetIcons()[i]->GetTitle().c_str() );
					m_pcCurrentInfo->AddChild( pcView );
					m_pcCurrentInfo->Show();
				}
				bIconFound = true;
			}
			nIcon++;
		}
		g_cWindowLock.Unlock();
	} 
	
	if( !bIconFound ) {
		/* Check if any icon is still selected -> delete the highlighted icon and info window */
		if( m_nCurrentIcon > -1 )
		{
			if( m_pcCurrentIcon )
				delete( m_pcCurrentIcon );
			m_pcCurrentIcon = NULL;
			if( m_pcCurrentInfo )
				m_pcCurrentInfo->Close();
			m_pcCurrentInfo = NULL;
			Invalidate( cLastIconFrame );
			m_nCurrentIcon = -1;
			Flush();
		}
	}
	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}

void DockView::MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData )
{
	/* Check mouse position */
	bool bHorizontal = m_pcWin->GetPosition() == os::ALIGN_TOP || m_pcWin->GetPosition() == os::ALIGN_BOTTOM;
	os::Rect cIconFrame;
	g_cWindowLock.Lock();
	uint nIcon = 0;
	uint32 nDesktopMask = 1 << m_pcWin->GetDesktop();
	for( uint i = 0; i < m_pcWin->GetIcons().size(); i++ )
	{
		if( !m_pcWin->GetIcons()[i]->GetVisible( nDesktopMask ) )
			continue;
		
		if( bHorizontal )
			cIconFrame = os::Rect( 4 + nIcon * 30, 4, 4 + nIcon * 30 + 23, 4 + 23 );
		else
			cIconFrame = os::Rect( 4, 4 + nIcon * 30, 4 + 23, 4 + nIcon * 30 + 23 );
			
		if( cIconFrame.DoIntersect( cPosition ) )
		{
			if( m_pcWin->GetIcons()[i]->GetID() != ICON_SYLLABLE )
			{
				/* Activate this window */
				printf( "Activate %i\n", m_pcWin->GetIcons()[i]->GetMsgPort() );
				os::Messenger cMessenger( m_pcWin->GetIcons()[i]->GetMsgPort() );
				cMessenger.SendMessage( os::WR_ACTIVATE );
				
			} else {
				/* Open syllable menu */
				if( m_bSyllableMenuInvalid )
				{
					if( m_pcSyllableMenu )
					{
						delete( m_pcSyllableMenu );
						m_pcSyllableMenu = NULL;
					}
					
					m_pcSyllableMenu = new DockMenu( this, os::Rect(), "dock_menu", os::ITEMS_IN_COLUMN );
					m_pcSyllableMenu->SetCloseMessage( os::Message( DOCK_MENU_CLOSED ) );
					m_pcSyllableMenu->SetCloseMsgTarget( m_pcWin );
					m_pcSyllableMenu->AddItem( new os::MenuSeparator() );
					m_pcSyllableMenu->AddItem( new os::MenuItem( MSG_MENU_ABOUT_MENU, new os::Message( DOCK_ABOUT ),
					 "", new os::BitmapImage( *m_pcWin->GetAboutIcon() ) ) );
					m_pcSyllableMenu->AddItem( new os::MenuItem( MSG_MENU_QUIT_MENU, new os::Message( DOCK_QUIT ),
					 "", new os::BitmapImage( *m_pcWin->GetLogoutIcon() ) ) );
					m_pcSyllableMenu->SetTargetForItems( m_pcWin );
					m_bSyllableMenuInvalid = false;
				}
				
				if( m_pcWin->GetPosition() == os::ALIGN_TOP )
					m_pcSyllableMenu->Open( os::Point( 0, 31 ) );
				else if( m_pcWin->GetPosition() == os::ALIGN_BOTTOM )
					m_pcSyllableMenu->Open( os::Point( 0, m_pcWin->GetFrame().top - m_pcSyllableMenu->GetPreferredSize( false ).y ) );
				else if( m_pcWin->GetPosition() == os::ALIGN_LEFT )
					m_pcSyllableMenu->Open( os::Point( 31, 0 ) );
				else
					m_pcSyllableMenu->Open( os::Point( m_pcWin->GetFrame().left - m_pcSyllableMenu->GetPreferredSize( false ).x, 0 ) );
				m_bSyllableMenuOpened = true;
				Invalidate( os::Rect( 0, 0, 30, 30 ) );
				Flush();
			}
		}
		nIcon++;
	}
	g_cWindowLock.Unlock();
	/* Check if it is a drag & drop event */
	if( pcData != NULL )
	{
		os::String zPath;
		if( pcData->FindString( "file/path", &zPath.str() ) == 0 )
		{
			m_pcWin->AddPlugin( zPath );
		}
	}
	os::View::MouseUp( cPosition, nButton, pcData );
}

DockWin::DockWin() : 
			os::Window( os::Rect(), "dock_win", "Dock", 
				os::WND_NO_BORDER, os::ALL_DESKTOPS )
{
	/* Get the registrar manager */
	m_pcManager = NULL;
	try
	{
		m_pcManager = os::RegistrarManager::Get();
	} catch( ... )
	{
	}
	
	/* Position */
	m_eAlign = os::ALIGN_TOP;
	
	/* Create desktop handle */
	m_pcDesktop = new os::Desktop();
	
	/* Resize ourself */
	
	SetFrame( GetDockFrame() );
	
	
	/* Load default icons */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "window.png" );
	m_pcDefaultWindowIcon = new os::BitmapImage( pcStream );
	if( !( m_pcDefaultWindowIcon->GetSize() == os::Point( 24, 24 ) ) )
		m_pcDefaultWindowIcon->SetSize( os::Point( 24, 24 ) );
	
	pcStream = cCol.GetResourceStream( "Syllable.png" );
	os::BitmapImage* pcLogo = new os::BitmapImage( pcStream );
	delete( pcStream );
	
	pcStream = cCol.GetResourceStream( "logout.png" );
	m_pcLogoutIcon = new os::BitmapImage( pcStream );
	if( !( m_pcLogoutIcon->GetSize() == os::Point( 24, 24 ) ) )
		m_pcLogoutIcon->SetSize( os::Point( 24, 24 ) );
	delete( pcStream );
	
	pcStream = cCol.GetResourceStream( "Syllable.png" );
	m_pcAboutIcon = new os::BitmapImage( pcStream );
	if( !( m_pcAboutIcon->GetSize() == os::Point( 24, 24 ) ) )
		m_pcAboutIcon->SetSize( os::Point( 24, 24 ) );
	delete( pcStream );
	
	/* Create Syllable icon */
	m_pcSyllableIcon = new DockIcon( ICON_SYLLABLE, "Syllable" );
	m_pcSyllableIcon->SetDesktopMask( 0xffffffff );
	m_pcSyllableIcon->GetBitmap()->SetBitmapData( pcLogo->LockBitmap()->LockRaster(), 
			os::IPoint( pcLogo->GetSize() ), pcLogo->GetColorSpace() );
	delete( pcLogo );
	
	m_pcIcons.clear();
	m_pcIcons.push_back( m_pcSyllableIcon );
	
	/* Create view */
	m_pcView = new DockView( this, GetBounds() );
	AddChild( m_pcView );
	
	
	/* Load settings */
	LoadSettings();
	
	
	/* Register calls */
	m_pcGetPluginsEv = os::Event::Register( "os/Dock/GetPlugins", "Returns a list of enabled plugins",
					os::Application::GetInstance(), os::DOCK_GET_PLUGINS );
	m_pcSetPluginsEv = os::Event::Register( "os/Dock/SetPlugins", "Sets the list of enabled plugins",
					os::Application::GetInstance(), os::DOCK_SET_PLUGINS );
	m_pcGetPosEv = os::Event::Register( "os/Dock/GetPosition", "Returns the position of the dock",
					os::Application::GetInstance(), os::DOCK_GET_POSITION );
	m_pcSetPosEv = os::Event::Register( "os/Dock/SetPosition", "Set the position of the dock",
					os::Application::GetInstance(), os::DOCK_SET_POSITION );

	/* Bind to window event */					
	m_pcWindowEv = new os::Event();
	m_pcWindowEv->SetToRemote( "os/Window/", -1 );
	m_pcWindowEv->SetMonitorEnabled( true, this, DOCK_WINDOW_EV );
	
	/* Bind to application list event */
	m_pcAppListEv = new os::Event();
	m_pcAppListEv->SetToRemote( "os/Registrar/AppList", -1 );
	m_pcAppListEv->SetMonitorEnabled( true, this, DOCK_APP_LIST_EV );
}

DockWin::~DockWin()
{
	/* Unregister events */
	delete( m_pcWindowEv );
	delete( m_pcGetPluginsEv );
	delete( m_pcSetPluginsEv );
	delete( m_pcGetPosEv );
	delete( m_pcSetPosEv );
	
	delete( m_pcDesktop );
}


void DockWin::Quit( int nAction )
{
	
	/* Close filetype manager */
	if( m_pcManager ) {
		
		m_pcManager->Put();
		m_pcManager = NULL;
	}
	
	/* Close all windows */
	os::Messenger cMsg( os::Application::GetInstance()->GetServerPort() );
	os::Message cMessage( os::DR_CLOSE_WINDOWS );
	cMessage.AddInt32( "action", nAction );
	cMsg.SendMessage( &cMessage );
}

int dock_logout_entry( void* pData )
{
	DockWin* pcWin = (DockWin*)(pData);
	pcWin->Quit( 0 );
	return( 0 );
}
int dock_shutdown_entry( void* pData )
{
	DockWin* pcWin = (DockWin*)(pData);
	pcWin->Quit( 1 );
	return( 0 );
}
int dock_reboot_entry( void* pData )
{
	DockWin* pcWin = (DockWin*)(pData);
	pcWin->Quit( 2 );
	return( 0 );
}

/* Returns the frame of the dock */
os::Rect DockWin::GetDockFrame()
{
	if( m_pcDesktop == NULL )
		return( GetFrame() );
	
	os::Rect cFrame;
	
	if( m_eAlign == os::ALIGN_TOP )
		cFrame = os::Rect( 0, 0, m_pcDesktop->GetResolution().x - 1, 30 );
	else if( m_eAlign == os::ALIGN_BOTTOM )
		cFrame = os::Rect( 0, m_pcDesktop->GetResolution().y - 31, m_pcDesktop->GetResolution().x - 1, m_pcDesktop->GetResolution().y - 1 );
	else if( m_eAlign == os::ALIGN_LEFT )
		cFrame = os::Rect( 0, 0, 30, m_pcDesktop->GetResolution().y - 1 );
	else
		cFrame = os::Rect( m_pcDesktop->GetResolution().x - 31, 0, m_pcDesktop->GetResolution().x - 1, m_pcDesktop->GetResolution().y - 1 );
	
	return( cFrame );
}

void DockWin::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case DOCK_APP_LIST_EV:
		{
			m_pcView->InvalidateSyllableMenu();
			break;
		}
		case DOCK_WINDOW_EV:
		{
			UpdateWindows( pcMessage );
			break;
		}
		case DOCK_ABOUT_ALERT:
		{
			int32 nButton;
			if( pcMessage->FindInt32( "which", &nButton ) == 0 )
				if( nButton == 1 )
				{
					/* Open Syllable Manager */
					if( fork() == 0 )
					{
						set_thread_priority( -1, 0 );
						execlp( "/Applications/System Tools/System Information/System Information", "/Applications/System Tools/System Information/System Information", NULL );
					}
				}
			break;
		}
		case DOCK_QUIT_ALERT:
		{
			/* Shutdown/logout or reboot */
			int32 nCode;
			if( pcMessage->FindInt32( "which", &nCode ) == 0 )
			{
				switch( nCode )
				{
					case 0:
						/* Logout */
						resume_thread( spawn_thread( "dock_logout", (void*)dock_logout_entry, 0, 0, this ) );
						break;
					case 1:
						/* Shutdown */
						resume_thread( spawn_thread( "dock_shutdown", (void*)dock_shutdown_entry, 0, 0, this ) );
						break;
					case 2:
						/* Reboot */
						resume_thread( spawn_thread( "dock_logout", (void*)dock_reboot_entry, 0, 0, this ) );
						break;
				}				
			}
			break;
		}
		case DOCK_ABOUT:
		{
			/* Show about window */
			system_info sSysInfo;
			get_system_info( &sSysInfo );
			
			char zInfo[255];
			os::String zWWWInfo;
			os::String zBuildDateA;
			os::String zBuildDateB;
			
			/* Ugly Workaround to make localization work. Someone should have a look at it... */	
			os::String zBuildDateWorkaround;
			zBuildDateWorkaround = os::String( "" );

			/* Build information text */
			
			zWWWInfo = os::String( MSG_MENU_ABOUT_TEXTONE + " http://www.syllable.org " + MSG_MENU_ABOUT_TEXTTWO );
			zBuildDateA = os::String( zBuildDateWorkaround + "Syllable %d.%d.%d%c\n\n" + MSG_MENU_ABOUT_BUILDDATE + " %s\n\n%s" );
			zBuildDateB = os::String( zBuildDateWorkaround + "Syllable %d.%d.%d\n\n" + MSG_MENU_ABOUT_BUILDDATE + " %s\n\n%s" );
			
			if( sSysInfo.nKernelVersion & 0xffff000000000000LL )
			{
				sprintf( zInfo, zBuildDateA.c_str(), ( int )( ( sSysInfo.nKernelVersion >> 32 ) & 0xffff ), ( int )( ( sSysInfo.nKernelVersion >> 16 ) & 0xffff ), 
									( int )( sSysInfo.nKernelVersion & 0xffff ), 'a' + ( int )( sSysInfo.nKernelVersion >> 48 ), sSysInfo.zKernelBuildDate, zWWWInfo.c_str() );
			}
			else
			{
				sprintf( zInfo, zBuildDateB.c_str(), ( int )( ( sSysInfo.nKernelVersion >> 32 ) & 0xffff ), ( int )( ( sSysInfo.nKernelVersion >> 16 ) % 0xffff ),
									 ( int )( sSysInfo.nKernelVersion & 0xffff ), sSysInfo.zKernelBuildDate, zWWWInfo.c_str() );
			}
			
			os::Alert* pcAlert = new os::Alert( MSG_MENU_ABOUT_TITLE, zInfo, os::Alert::ALERT_INFO,
												0, MSG_MENU_ABOUT_BUTTON_OK.c_str(), MSG_MENU_ABOUT_BUTTON_ADVANCED.c_str(), NULL );
			os::Invoker* pcInvoker = new os::Invoker( new os::Message( DOCK_ABOUT_ALERT ), this );
			pcAlert->Go( pcInvoker );
			
			break;
		}
		case DOCK_QUIT:
		{
			os::Alert* pcAlert = new os::Alert( MSG_MENU_QUIT_TITLE, MSG_MENU_QUIT_TEXT, os::Alert::ALERT_QUESTION,
												0, MSG_MENU_QUIT_LOGOUT.c_str(), MSG_MENU_QUIT_SHUTDOWN.c_str(), MSG_MENU_QUIT_REBOOT.c_str(), MSG_MENU_QUIT_CANCEL.c_str(), NULL );
			os::Invoker* pcInvoker = new os::Invoker( new os::Message( DOCK_QUIT_ALERT ), this );
			pcAlert->Go( pcInvoker );
			
			break;
		}
		case DOCK_MENU_CLOSED:
		{
			m_pcView->SetSyllableMenuClosed();
			break;
		}
		case DOCK_OPEN_APP:
		{
			os::String zPath;
			if( pcMessage->FindString( "file/path", &zPath.str() ) == 0 )
			{
				if( m_pcManager )
				{
					/* Tell the filetype manager to open it */
					m_pcManager->Launch( NULL, zPath, true, MSG_OPENAPP );
				} else if( fork() == 0 )
				{
					set_thread_priority( -1, 0 );
					execlp( zPath.c_str(), zPath.c_str(), NULL );
				}
			}
			break;
		}
		case os::DOCK_ADD_VIEW:
		{
			/* Add a view */
			os::DockPlugin* pcPlugin = NULL;			
			os::View* pcView = NULL;
			if( pcMessage->FindPointer( "view", (void**)&pcView ) == 0 && pcMessage->FindPointer( "plugin", (void**)&pcPlugin ) == 0 )
			{
				pcPlugin->SetViewCount( pcPlugin->GetViewCount() + 1 );
				m_pcView->AddChild( pcView );
				m_pcPluginViews.push_back( pcView );
				UpdatePlugins();
			}
			break;
		}
		case os::DOCK_REMOVE_VIEW:
		{
			/* Add a view */
			os::DockPlugin* pcPlugin = NULL;
			os::View* pcView = NULL;
			if( pcMessage->FindPointer( "view", (void**)&pcView ) == 0 && pcMessage->FindPointer( "plugin", (void**)&pcPlugin ) == 0 )
			{
				bool bRemoved = false;
				for( int i = 0; i < m_pcPluginViews.size(); i++ )
				{
					if( m_pcPluginViews[i] == pcView )
					{
						pcPlugin->SetViewCount( pcPlugin->GetViewCount() - 1 );
						m_pcView->RemoveChild( pcView );
						delete( pcView );
						m_pcPluginViews.erase( m_pcPluginViews.begin() + i );
						bRemoved = true;
						if( pcPlugin->GetDeleted() && pcPlugin->GetViewCount() == 0 )
						{
							thread_id nThread = -1;
							if( pcMessage->FindInt32( "thread", &nThread ) == 0 && nThread != -1 )
								wait_for_thread( nThread );
							DeletePlugin( pcPlugin );
						}
						break;
					}
				}
				if( !bRemoved )
					dbprintf( "Error: Tried to remove a non-added view\n" );			
				UpdatePlugins();
			}
			break;
		}
		case os::DOCK_UPDATE_FRAME:
		{
			/* Called by a plugin to update its frame */
			UpdatePlugins();
			break;
		}
		case os::DOCK_REMOVE_PLUGIN:
		{
			/* Called by a plugin to remove itself from the dock */
			os::DockPlugin* pcPlugin;
			if( pcMessage->FindPointer( "plugin", (void**)&pcPlugin ) == 0 )
			{
				DeletePlugin( pcPlugin );
			}
			break;
		}
		case os::DOCK_SET_POSITION:
		{
			/* Called by another application to set the position of the dock */
			int32 nPos;
			if( pcMessage->FindInt32( "position", &nPos ) == 0 )
			{
				SetPosition( (os::alignment)nPos );
			}
			SaveSettings();
		}
		default:
		{
			os::Window::HandleMessage( pcMessage );
		}
	}
}

bool DockWin::OkToQuit()
{
	return( false );
}

void DockWin::AddPlugin( os::String zPath )
{
	/* Look if this plugins has already been added */
	for( uint i = 0; i < m_pcPlugins.size(); i++ )
	{
		if( os::String( m_pcPlugins[i]->GetPath().GetPath() ) == zPath )
			return;		
	}
	
	/* Check if the plugin path is in "/system/extensions/dock" */
	os::Path cPath( zPath.c_str() );
	if( !( os::String( cPath.GetDir().GetPath() ) == "/system/extensions/dock" ||
		   os::String( cPath.GetDir().GetPath() ) == "/boot/system/extensions/dock" ) )
	{
		os::Alert* pcAlert = new os::Alert( "Dock", MSG_PLUGIN_ALERT_TEXT,
													os::Alert::ALERT_INFO,
													0, MSG_PLUGIN_ALERT_OK.c_str(), NULL );
		pcAlert->Go( new os::Invoker( 0 ) );
		return;
	}
	
	
	/* Try to check if this is a plugin ( TODO: Use the registrar ) */
	if( zPath.CountChars() > 3 && zPath.c_str()[zPath.CountChars()-3] == '.' 
		&& zPath.c_str()[zPath.CountChars()-2] == 's' && zPath.c_str()[zPath.CountChars()-1] == 'o' )
	{
		int nID;
		/* Try to load the plugin */
		if( ( nID = load_library( zPath.c_str(), IM_ADDON_SPACE ) ) >= 0 )
		{
			os::init_dock_plugin* pInit;
			if( get_symbol_address( nID, "init_dock_plugin", -1, (void**)&pInit ) == 0 )
			{
				os::Path cPath = os::Path( zPath.c_str() );
				os::DockPlugin* pcPlugin = pInit();
				pcPlugin->SetApp( this, nID );
				if( pcPlugin->Initialize() != 0 )
				{
					dbprintf( "Error: Plugin %s failed to initialize\n", zPath.c_str() );
				} else {
					//dbprintf( "Plugin %s loaded %i\n", pcPlugin->GetIdentifier().c_str(), nID );
					m_pcPlugins.push_back( pcPlugin );
					SaveSettings();
				}
			}
		}
	}
}


void DockWin::UpdatePlugins()
{
	int nCurrentPos;
	
	bool bHorizontal = GetPosition() == os::ALIGN_TOP || GetPosition() == os::ALIGN_BOTTOM;
	
	if( bHorizontal )
		nCurrentPos = (int)GetBounds().right - 4;
	else
		nCurrentPos = (int)GetBounds().bottom - 4;
	
	/* Update the positions of the plugins */
	for( uint32 i = 0; i < m_pcPluginViews.size(); i++ )
	{
		if( bHorizontal )
			m_pcPluginViews[i]->SetFrame( os::Rect( nCurrentPos - (int)m_pcPluginViews[i]->GetPreferredSize( false ).x, 3, nCurrentPos, 
											3 + 23 ) );
		else
			m_pcPluginViews[i]->SetFrame( os::Rect( 3, nCurrentPos - (int)m_pcPluginViews[i]->GetPreferredSize( false ).y, 3 + 23, 
										nCurrentPos ) );
		if( bHorizontal )
			nCurrentPos -= (int)m_pcPluginViews[i]->GetPreferredSize( false ).x + 8;
		else
			nCurrentPos -= (int)m_pcPluginViews[i]->GetPreferredSize( false ).y + 8;
	} 
}

void DockWin::DeletePlugin( os::DockPlugin* pcPlugin )
{
	/* Find application */
	for( uint32 i = 0; i < m_pcPlugins.size(); i++ )
	{
		if( pcPlugin == m_pcPlugins[i] )
		{
			if( !pcPlugin->GetDeleted() )
				pcPlugin->Delete();
			if( pcPlugin->GetViewCount() > 0 )
			{
				/* Delete plugin later */
				pcPlugin->SetDeleted();
				return;
			}
			image_id nPlugin = pcPlugin->GetPluginId();
			delete( m_pcPlugins[i] );
			m_pcPlugins.erase( m_pcPlugins.begin() + i );
			//dbprintf("Unload %i\n", nPlugin );
			unload_library( nPlugin );
			SaveSettings();
			return;
		}
	} 
}

void DockWin::LoadSettings()
{
	/* Create settings object */
	os::Settings* pcSettings = new os::Settings();
	if( pcSettings->Load() != 0 )
	{
		SetPosition( os::ALIGN_TOP );
		return;
	}
		
	/* Load position */
	int32 nPosition = pcSettings->GetInt32( "position", os::ALIGN_TOP );
	SetPosition( (os::alignment)nPosition );
		
	/* Load pugins */
	int32 nPluginCount = pcSettings->GetInt32( "plugin_count", 0 );
	
	for( int32 i = 0; i < nPluginCount; i++ )
	{
		os::String zPath = pcSettings->GetString( "plugin", "", i );
		if( !( zPath == os::String( "" ) ) )
		{
			AddPlugin( zPath );
		}
	}
	delete( pcSettings );
}

void DockWin::SaveSettings()
{
	/* Create settings object */
	os::Settings* pcSettings = new os::Settings();
	
	/* Save position */
	pcSettings->SetInt32( "position", m_eAlign );
		
	/* Save plugins */
	pcSettings->SetInt32( "plugin_count", m_pcPlugins.size() );

	for( uint32 i = 0; i < m_pcPlugins.size(); i++ )
	{
		pcSettings->SetString( "plugin", m_pcPlugins[i]->GetPath().GetPath(), i );
	}
	pcSettings->Save();
	delete( pcSettings );
}

void DockWin::UpdateWindows( os::Message* pcMessage )
{
	g_cWindowLock.Lock();
	
	os::String zID;
	bool bDummy;
	os::String zTitle;
	int64 nHandle;
	int64 hMsgPort;
	
	pcMessage->FindString( "id", &zID );
	
	/* Check message */
	if( pcMessage->FindBool( "event_unregistered", &bDummy ) == 0 )
	{
		/* Remove window */
		for( uint i = 0; i < m_pcIcons.size(); i++ )
		{
			if( m_pcIcons[i]->GetID() == zID )
			{
				//printf("Delete icon!\n" );
				delete( m_pcIcons[i] );

				m_pcIcons.erase( m_pcIcons.begin() + i );
				m_pcView->Invalidate();
				Flush();
				g_cWindowLock.Unlock();
				return;
			}
		}
		printf( "Error: Cannot remove entry %s\n", zID.c_str() );
		g_cWindowLock.Unlock();
		return;
	}
			
	DockIcon* pcIcon = NULL;
	
	pcMessage->FindString( "title", &zTitle );
	
	os::String zApp;
	pcMessage->FindString( "application", &zApp );
	//printf( "%s\n", zApp.c_str() );
	
	if( pcMessage->FindBool( "event_registered", &bDummy ) == 0 )
	{
		/* Add new window */
		//printf("Add new icon!\n" );
		
		pcIcon = new DockIcon( zID, "" );
		m_pcIcons.push_back( pcIcon );
		pcIcon = m_pcIcons[m_pcIcons.size()-1];
		if( pcMessage->FindInt64( "target", &hMsgPort ) == 0 )
			pcIcon->SetMsgPort( hMsgPort );
	}
	else
	{
		//printf("Change!\n");
		/* Change already present window */
		uint i = 0;
		for( i = 0; i < m_pcIcons.size(); i++ )
		{
			if( m_pcIcons[i]->GetID() == zID )
			{
				pcIcon = m_pcIcons[i];
				break;
			}
		}
		if( i == m_pcIcons.size() )
		{
			printf( "Error: Cannot find entry %s\n", zID.c_str() );
			g_cWindowLock.Unlock();
			return;
		}
	}
	pcIcon->SetTitle( zTitle );

	bool bDefaultIcon = true;
	if( pcMessage->FindInt64( "icon_handle", &nHandle ) == 0 )
	{
		/* Map icon */
		bool bIconChanged = false;
		pcMessage->FindBool( "icon_changed", &bIconChanged );
		
		bDefaultIcon = false;
		if( nHandle != pcIcon->GetHandle() || bIconChanged )
		{
//			printf( "Icon Handle %x %i\n", (uint)nHandle, bIconChanged );
			os::Bitmap* pcBitmap = NULL;
			try
			{
				pcBitmap = new os::Bitmap( nHandle );
				os::Point cSize = pcBitmap->GetBounds().Size();
				pcIcon->GetBitmap()->SetBitmapData( pcBitmap->LockRaster(), os::IPoint( (int)cSize.x + 1, (int)cSize.y + 1 ), os::CS_RGB32 );
				if( pcIcon->GetBitmap()->GetSize() != os::Point( 24, 24 ) )
					pcIcon->GetBitmap()->SetSize( os::Point( 24, 24 ) );
				delete( pcBitmap );
				pcIcon->SetHandle( nHandle );
			} catch( ... )
			{
				bDefaultIcon = true;
				printf( "Exception!\n" );
			}
		}
	}
	
	if( bDefaultIcon )
	{
		pcIcon->GetBitmap()->SetBitmapData( m_pcDefaultWindowIcon->LockBitmap()->LockRaster(), 
		os::IPoint( m_pcDefaultWindowIcon->GetSize() ), m_pcDefaultWindowIcon->GetColorSpace() );
	}
		
	bool bVisible;
	if( pcMessage->FindBool( "visible", &bVisible ) == 0 )
	{
		//printf("Visible %i\n", bVisible );
		pcIcon->SetVisible( bVisible );
	}
	bool bMinimized;
	if( pcMessage->FindBool( "minimized", &bMinimized ) == 0 )
		pcIcon->SetMinimized( bMinimized );
		
	int64 nDesktopMask;
	if( pcMessage->FindInt64( "desktop_mask", &nDesktopMask ) == 0 )
	{
		pcIcon->SetDesktopMask( nDesktopMask );
		//printf( "Desktop mask %x\n", (uint)nDesktopMask );
	}
			

	g_cWindowLock.Unlock();
	m_pcView->Invalidate();
	Flush();
}

void DockWin::UpdateWindowArea()
{
	if( m_pcDesktop == NULL )
		return;
	
	os::Rect cFrame;
	
	if( m_eAlign == os::ALIGN_TOP )
		cFrame = os::Rect( 0, 31, m_pcDesktop->GetResolution().x - 1, m_pcDesktop->GetResolution().y - 1 );
	else if( m_eAlign == os::ALIGN_BOTTOM )
		cFrame = os::Rect( 0, 0, m_pcDesktop->GetResolution().x - 1, m_pcDesktop->GetResolution().y - 31 );
	else if( m_eAlign == os::ALIGN_LEFT )
		cFrame = os::Rect( 31, 0, m_pcDesktop->GetResolution().x - 1, m_pcDesktop->GetResolution().y - 1 );
	else
		cFrame = os::Rect( 0, 0, m_pcDesktop->GetResolution().x - 31, m_pcDesktop->GetResolution().y - 1 );
	
	os::Message cReq( os::DR_SET_DESKTOP_MAX_WINFRAME );
	os::Message cReply;
	cReq.AddInt32( "desktop", os::Desktop::ACTIVE_DESKTOP );
	cReq.AddRect( "frame", cFrame );
				
	os::Application* pcApp = os::Application::GetInstance();
	os::Messenger( pcApp->GetServerPort() ).SendMessage( &cReq, &cReply );
	
	/* Tell the desktop */
	os::Event cEvent;
	if( cEvent.SetToRemote( "os/Desktop/Refresh" ) == 0 )
	{
		os::Message cDummy;
		cEvent.PostEvent( &cDummy );
	}
}

void DockWin::ScreenModeChanged( const os::IPoint& cNewRes, os::color_space eSpace )
{
	/* Update desktop parameters */
	delete( m_pcDesktop );
	m_pcDesktop = new os::Desktop();
	
	/* Notify plugins */
	for( int i = m_pcPlugins.size(); --i >= 0; ) {
		m_pcPlugins[i]->ScreenModeChanged( cNewRes, eSpace );
	}
	
	/* Resize ourself */
	SetFrame( GetDockFrame() );
	
	#if 0
	/* Reload window list */
	os::Message cWindows;
	int32 nCount = m_pcDesktop->GetWindows( &cWindows );
	
	/* Update list */
	UpdateWindows( &cWindows, nCount );
	#endif
	UpdatePlugins();
	
	UpdateWindowArea();
	
	m_pcView->Invalidate();
	Flush();
}

void DockWin::DesktopActivated( int nDesktop, bool bActive )
{
	/* Update desktop parameters */
	delete( m_pcDesktop );
	m_pcDesktop = new os::Desktop();
	
	/* Notify plugins */
	for( int i = m_pcPlugins.size(); --i >= 0; ) {
		m_pcPlugins[i]->DesktopActivated( nDesktop, bActive );
	}
	
	/* Resize ourself */
	SetFrame( GetDockFrame() );
	
	
	UpdatePlugins();
	
	UpdateWindowArea();
	
	m_pcView->Invalidate();
	Flush();
}

void DockWin::SetPosition( os::alignment eAlign )
{
	/* Set the position */
	m_eAlign = eAlign;
	
	/* Update */
	SetFrame( GetDockFrame() );
	UpdatePlugins();
	m_pcView->Invalidate();
	Sync();
	
	UpdateWindowArea();
	
}

DockApp::DockApp( const char *pzMimeType ):os::Application( pzMimeType )
{
	SetCatalog("Dock.catalog");
	/* Create window */
	m_pcWindow = new DockWin();
	m_pcWindow->Show();
	
	/* Make our port public */
	SetPublic( true );
}

DockApp::~DockApp()
{
}

bool DockApp::OkToQuit()
{
	return( false );
}

void DockApp::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case os::DOCK_GET_PLUGINS:
		{
			/* Called by another application to get a list of plugins */
			if( !pcMessage->IsSourceWaiting() )
				break;
			os::Message cMsg( os::DOCK_GET_PLUGINS );
			m_pcWindow->Lock();
			for( uint i = 0; i < m_pcWindow->GetPlugins().size(); i++ )
				cMsg.AddString( "plugin", m_pcWindow->GetPlugins()[i]->GetPath().GetPath() );
			m_pcWindow->Unlock();
			pcMessage->SendReply( &cMsg );
			break;
		}
		case os::DOCK_SET_PLUGINS:
		{
			/* Called by another application to set a list of plugins */
			os::String zPlugin;
			os::String zMessage;
			int j;
			bool bFound;
			m_pcWindow->Lock();
			/* Look what plugins have been removed */
			for( uint i = 0; i < m_pcWindow->GetPlugins().size(); i++ )
			{
				zPlugin = m_pcWindow->GetPlugins()[i]->GetPath();
				bFound = false;
				j = 0;
				while( pcMessage->FindString( "plugin", &zMessage.str(), j ) == 0 )
				{
					if( zPlugin == zMessage )
						bFound = true;
					j++;
				}
				if( !bFound ) {
					/* Remove plugin */
					os::Message cMsg( os::DOCK_REMOVE_PLUGIN );
					cMsg.AddPointer( "plugin", m_pcWindow->GetPlugins()[i] );
					m_pcWindow->PostMessage( &cMsg, m_pcWindow );;
					//m_pcWindow->DeletePlugin( m_pcWindow->GetPlugins()[i] );
				}
			}
			/* Look what plugins have been added */
			
			j = 0;
			while( pcMessage->FindString( "plugin", &zMessage.str(), j ) == 0 )
			{
				bFound = false;
				for( uint i = 0; i < m_pcWindow->GetPlugins().size(); i++ )
				{
					zPlugin = m_pcWindow->GetPlugins()[i]->GetPath();
			
					if( zPlugin == zMessage )
						bFound = true;	
				}
				if( !bFound ) {
					/* Add plugin */
					m_pcWindow->AddPlugin( zMessage );
				}
				j++;
			}
			m_pcWindow->Unlock();
			break;
		}
		case os::DOCK_GET_POSITION:
		{
			
			/* Called by another application to get the position of the dock */
			if( !pcMessage->IsSourceWaiting() )
				break;
			os::Message cMsg( os::DOCK_GET_POSITION );		
			cMsg.AddInt32( "position", m_pcWindow->GetPosition() );
			pcMessage->SendReply( &cMsg );
			break;
		}
		case os::DOCK_SET_POSITION:
		{
			m_pcWindow->PostMessage( pcMessage );
			break;
		}
		default:
			os::Application::HandleMessage( pcMessage );
	}
}

int main( int argc, char *argv[] )
{
	
	DockApp* pcApp = new DockApp( "application/syllable-Dock" );
	pcApp->Run();
	return ( 0 );
}
