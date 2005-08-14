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


enum 
{
	DOCK_QUIT = 10000,
	DOCK_MENU_CLOSED,
	DOCK_OPEN_APP,
	DOCK_ABOUT,
	DOCK_ABOUT_ALERT,
	DOCK_QUIT_ALERT	
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
	for( uint i = 0; i < m_pcWin->GetIcons().size(); i++ )
	{
		os::Rect cIconFrame;
		os::Point cIconPos;
		
		if( bHorizontal )
		{
			cIconFrame = os::Rect( 4 + i * 30, 4, 4 + i * 30 + 23, 4 + 23 );
			cIconPos = os::Point( 4 + i * 30, 4 );
		} else {
			cIconFrame = os::Rect( 4, 4 + i * 30, 4 + 23, 4 + i * 30 + 23 );
			cIconPos = os::Point( 4, 4 + i * 30 );
		}
		
		if( cUpdateRect.DoIntersect( cIconFrame ) ) 
			if( m_nCurrentIcon == (int)i )
				m_pcCurrentIcon->Draw( cIconPos, this );
			else
				m_pcWin->GetIcons()[i]->GetBitmap()->Draw( cIconPos, this );
			
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

	if( nCode == os::MOUSE_INSIDE )
	{
		g_cWindowLock.Lock();
		/* Check mouse position */
		for( uint i = 0; i < m_pcWin->GetIcons().size(); i++ )
		{
			os::Rect cIconFrame;
			
			if( bHorizontal )
			{
				cIconFrame = os::Rect( 4 + i * 30, 4, 4 + i * 30 + 23, 4 + 23 );
			} else {
				cIconFrame = os::Rect( 4, 4 + i * 30, 4 + 23, 4 + i * 30 + 23 );
			}
			
			if( cIconFrame.DoIntersect( cNewPos ) )
			{
				/* Select the current icon */
				if( m_nCurrentIcon != (int)i )
				{
					/* Update the previous icon and delete its highlighted icon and info window */
					Invalidate( cLastIconFrame );
					m_nCurrentIcon = i;
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
	for( uint i = 0; i < m_pcWin->GetIcons().size(); i++ )
	{
		if( bHorizontal )
			cIconFrame = os::Rect( 4 + i * 30, 4, 4 + i * 30 + 23, 4 + 23 );
		else
			cIconFrame = os::Rect( 4, 4 + i * 30, 4 + 23, 4 + i * 30 + 23 );
			
		if( cIconFrame.DoIntersect( cPosition ) )
		{
			if( m_pcWin->GetIcons()[i]->GetID() != ICON_SYLLABLE )
			{
				/* Activate this window */
				m_pcWin->ActivateWindow( m_pcWin->GetIcons()[i]->GetID() );
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
					m_pcSyllableMenu->AddItem( new os::MenuItem( "About...", new os::Message( DOCK_ABOUT ),
					 "", new os::BitmapImage( *m_pcWin->GetAboutIcon() ) ) );
					m_pcSyllableMenu->AddItem( new os::MenuItem( "Quit...", new os::Message( DOCK_QUIT ),
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
				os::WND_NO_BORDER | os::WND_SEND_WINDOWS_CHANGED, os::ALL_DESKTOPS )
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
	
	pcStream = cCol.GetResourceStream( "syllable.png" );
	os::BitmapImage* pcLogo = new os::BitmapImage( pcStream );
	delete( pcStream );
	
	pcStream = cCol.GetResourceStream( "logout.png" );
	m_pcLogoutIcon = new os::BitmapImage( pcStream );
	if( !( m_pcLogoutIcon->GetSize() == os::Point( 24, 24 ) ) )
		m_pcLogoutIcon->SetSize( os::Point( 24, 24 ) );
	delete( pcStream );
	
	pcStream = cCol.GetResourceStream( "syllable.png" );
	m_pcAboutIcon = new os::BitmapImage( pcStream );
	if( !( m_pcAboutIcon->GetSize() == os::Point( 24, 24 ) ) )
		m_pcAboutIcon->SetSize( os::Point( 24, 24 ) );
	delete( pcStream );
	
	/* Create Syllable icon */
	m_pcSyllableIcon = new DockIcon( ICON_SYLLABLE, "Syllable" );
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
	if( m_pcManager )
	{
		m_pcManager->RegisterCall( "os/Dock/GetPlugins", "Returns a list of enabled plugins",
							os::Application::GetInstance(), os::DOCK_GET_PLUGINS );
		m_pcManager->RegisterCall( "os/Dock/SetPlugins", "Sets the list of enabled plugins",
							os::Application::GetInstance(), os::DOCK_SET_PLUGINS );
		m_pcManager->RegisterCall( "os/Dock/GetPosition", "Returns the position of the dock",
							os::Application::GetInstance(), os::DOCK_GET_POSITION );
		m_pcManager->RegisterCall( "os/Dock/SetPosition", "Set the position of the dock",
							os::Application::GetInstance(), os::DOCK_SET_POSITION );
	}
}

DockWin::~DockWin()
{
	delete( m_pcDesktop );
}


void DockWin::Quit( int nAction )
{
	
	/* Close filetype manager */
	if( m_pcManager ) {
		
		m_pcManager->UnregisterCall( "os/Dock/GetPlugins" );
		m_pcManager->UnregisterCall( "os/Dock/SetPlugins" );
		m_pcManager->UnregisterCall( "os/Dock/GetPosition" );
		m_pcManager->UnregisterCall( "os/Dock/SetPosition" );
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
		case os::M_NODE_MONITOR:
		{
			m_pcView->InvalidateSyllableMenu();
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
						execlp( "/system/bin/SlbMgr", "/system/bin/SlbMgr", NULL );
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
			char zWWWInfo[128];
			
			/* Build information text */
			
			sprintf( zWWWInfo, "See http://syllable.sf.net for updates and information" );
			
			if( sSysInfo.nKernelVersion & 0xffff000000000000LL )
			{
				sprintf( zInfo, "Syllable %d.%d.%d%c\n\nBuild date: %s\n\n%s", ( int )( ( sSysInfo.nKernelVersion >> 32 ) & 0xffff ), ( int )( ( sSysInfo.nKernelVersion >> 16 ) & 0xffff ), 
									( int )( sSysInfo.nKernelVersion & 0xffff ), 'a' + ( int )( sSysInfo.nKernelVersion >> 48 ), sSysInfo.zKernelBuildDate, zWWWInfo );
			}
			else
			{
				sprintf( zInfo, "Syllable %d.%d.%d\n\nBuild date: %s\n\n%s", ( int )( ( sSysInfo.nKernelVersion >> 32 ) & 0xffff ), ( int )( ( sSysInfo.nKernelVersion >> 16 ) % 0xffff ),
									 ( int )( sSysInfo.nKernelVersion & 0xffff ), sSysInfo.zKernelBuildDate, zWWWInfo );
			}
			
			os::Alert* pcAlert = new os::Alert( "About Syllable", zInfo, os::Alert::ALERT_INFO,
												0, "Ok", "Advanced...", NULL );
			os::Invoker* pcInvoker = new os::Invoker( new os::Message( DOCK_ABOUT_ALERT ), this );
			pcAlert->Go( pcInvoker );
			
			break;
		}
		case DOCK_QUIT:
		{
			os::Alert* pcAlert = new os::Alert( "Quit", "What do you want to do?", os::Alert::ALERT_QUESTION,
												0, "Logout", "Shutdown", "Reboot", "Cancel", NULL );
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
					m_pcManager->Launch( NULL, zPath, true, "Open with..." );
				} else if( fork() == 0 )
				{
					set_thread_priority( -1, 0 );
					execlp( zPath.c_str(), zPath.c_str(), NULL );
				}
			}
			break;
		}
		case os::DOCK_UPDATE_FRAME:
		{
			/* Called by a plugin to update its frame */
			UpdatePlugins();
			break;
		}
		case os::DOCK_REMOVE:
		{
			/* Called by a plugin to remove itself from the dock */
			os::DockPlugin* pcPlugin;
			if( pcMessage->FindPointer( "plugin", (void**)&pcPlugin ) == 0 )
			{
				DeletePlugin( pcPlugin );
			}
			break;
		}
		default:
			os::Window::HandleMessage( pcMessage );
	}
}

bool DockWin::OkToQuit()
{
	return( true );
}

void DockWin::AddPlugin( os::String zPath )
{
	/* Look if this plugins has already been added */
	for( uint i = 0; i < m_pcPlugins.size(); i++ )
	{
		if( os::String( m_pcPlugins[i]->GetPath().GetPath() ) == zPath )
			return;		
	}
	
	/* Check if the plugin path is in "/system/drivers/dock" */
	os::Path cPath( zPath.c_str() );
	if( !( os::String( cPath.GetDir().GetPath() ) == "/system/drivers/dock" ||
		   os::String( cPath.GetDir().GetPath() ) == "/boot/atheos/sys/drivers/dock" ||
		   os::String( cPath.GetDir().GetPath() ) == "/boot/system/drivers/dock" ) )
	{
		os::Alert* pcAlert = new os::Alert( "Dock", "Please copy dock plugins to /system/drivers/dock\n"
													"before you try to add them to the dock", os::Alert::ALERT_INFO,
													0, "Ok", NULL );
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
				os::DockPlugin* pcPlugin = pInit( cPath, this );
				m_pcView->AddChild( pcPlugin );
				m_pcPlugins.push_back( pcPlugin );
				SaveSettings();
				return( UpdatePlugins() );
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
	for( uint32 i = 0; i < m_pcPlugins.size(); i++ )
	{
		
		if( bHorizontal )
			m_pcPlugins[i]->SetFrame( os::Rect( nCurrentPos - (int)m_pcPlugins[i]->GetPreferredSize( false ).x, 3, nCurrentPos, 
											3 + 23 ) );
		else
			m_pcPlugins[i]->SetFrame( os::Rect( 3, nCurrentPos - (int)m_pcPlugins[i]->GetPreferredSize( false ).y, 3 + 23, 
										nCurrentPos ) );
		if( bHorizontal )
			nCurrentPos -= (int)m_pcPlugins[i]->GetPreferredSize( false ).x + 8;
		else
			nCurrentPos -= (int)m_pcPlugins[i]->GetPreferredSize( false ).y + 8;
	} 
}

void DockWin::DeletePlugin( os::DockPlugin* pcPlugin )
{
	/* Find application */
	for( uint32 i = 0; i < m_pcPlugins.size(); i++ )
	{
		if( pcPlugin == m_pcPlugins[i] )
		{
			m_pcView->RemoveChild( m_pcPlugins[i] );
			delete(  m_pcPlugins[i] );
			m_pcPlugins.erase( m_pcPlugins.begin() + i );
			UpdatePlugins();
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
		return;
		
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

void DockWin::ActivateWindow( int32 nWindow )
{
	/* Activate window */
	m_pcDesktop->ActivateWindow( nWindow );
}

void DockWin::UpdateWindows( os::Message* pcWindows, int32 nCount )
{
	/* Remove all window icons */
	g_cWindowLock.Lock();
	for( uint32 i = 0; i < m_pcIcons.size(); i++ )
	{
		if( m_pcIcons[i]->GetID() != ICON_SYLLABLE )
			delete( m_pcIcons[i] );
	}
	m_pcIcons.clear();
	m_pcIcons.push_back( m_pcSyllableIcon );
	
	
	os::String zWinTitle;
	bool bIconPresent;
	bool bMinimized;
	
	
	//cout<<"Windows "<<nCount<<endl;
	
	
	int32 nWindow = nCount - 1;
		
	/* Iterate through the windows */	
	while( pcWindows->FindString( "title", &zWinTitle.str(), nWindow ) == 0 && 
			pcWindows->FindBool( "icon_present", &bIconPresent, nWindow ) == 0 && 
			pcWindows->FindBool( "minimized", &bMinimized, nWindow ) == 0 &&
			nWindow >= 0 )
	{
		
		
		
		/* Create icon */
		DockIcon* pcIcon = new DockIcon( nWindow, zWinTitle );
		
		if( bIconPresent )
		{
			os::Bitmap* pcWindowIcon = m_pcDesktop->GetWindowIcon( nWindow );
			if( pcWindowIcon ) 
			{
				pcIcon->GetBitmap()->SetBitmapData( pcWindowIcon->LockRaster(), os::IPoint( 24, 24 ), os::CS_RGB32 );
				delete( pcWindowIcon );
			}
			
			/* Scale if necessary */
			if( !( pcIcon->GetBitmap()->GetSize() == os::Point( 24, 24 ) ) ) {
				pcIcon->GetBitmap()->SetSize( os::Point( 24, 24 ) );
			}
		} else 
		{
			pcIcon->GetBitmap()->SetBitmapData( m_pcDefaultWindowIcon->LockBitmap()->LockRaster(), 
			os::IPoint( m_pcDefaultWindowIcon->GetSize() ), m_pcDefaultWindowIcon->GetColorSpace() );
		}
		
		if( bMinimized ) {
			pcIcon->GetBitmap()->ApplyFilter( os::Image::F_GRAY );
		}
			
		m_pcIcons.push_back( pcIcon );
		nWindow--;
	}
	m_nLastWindowCount = nCount;
	m_cLastWindows = *pcWindows;
	g_cWindowLock.Unlock();
	m_pcView->Invalidate();
	Flush();
}

void DockWin::WindowsChanged()
{
	/* Check what has changed */
	os::Message cWindows;
	int32 nCount = m_pcDesktop->GetWindows( &cWindows );
	
	/* Number of windows has changed */
	if( nCount != m_nLastWindowCount )
	{
		/* Check if one of the information windows is currently openened */
		return( UpdateWindows( &cWindows, nCount ) );
	}
	
	/* Compare list */
	for( int32 i = 0; i < m_nLastWindowCount; i++ )
	{
		os::String zWinTitle, zLastWinTitle;
		bool bIconPresent, bLastIconPresent;
		area_id hIconArea, hLastIconArea;
		int32 nIconWidth, nLastIconWidth;
		int32 nIconHeight, nLastIconHeight;
		os::color_space eSpace, eLastSpace;
		bool bMinimized, bLastMinimized;
		
		int32 nWindow = i;
		
		if( cWindows.FindString( "title", &zWinTitle.str(), nWindow ) == 0 && 
			cWindows.FindBool( "icon_present", &bIconPresent, nWindow ) == 0 && 
			cWindows.FindInt32( "icon_area", (int32*)&hIconArea, nWindow ) == 0 && 
			cWindows.FindInt32( "icon_width", &nIconWidth, nWindow ) == 0 && 
			cWindows.FindInt32( "icon_height", &nIconHeight, nWindow ) == 0 && 
			cWindows.FindInt32( "icon_colorspace", (int32*)&eSpace, nWindow ) == 0 &&
			cWindows.FindBool( "minimized", &bMinimized, nWindow ) == 0 &&
			m_cLastWindows.FindString( "title", &zLastWinTitle.str(), nWindow ) == 0 && 
			m_cLastWindows.FindBool( "icon_present", &bLastIconPresent, nWindow ) == 0 && 
			m_cLastWindows.FindInt32( "icon_area", (int32*)&hLastIconArea, nWindow ) == 0 && 
			m_cLastWindows.FindInt32( "icon_width", &nLastIconWidth, nWindow ) == 0 && 
			m_cLastWindows.FindInt32( "icon_height", &nLastIconHeight, nWindow ) == 0 && 
			m_cLastWindows.FindInt32( "icon_colorspace", (int32*)&eLastSpace, nWindow ) == 0 &&
			m_cLastWindows.FindBool( "minimized", &bLastMinimized, nWindow ) == 0 )
		{
			if( !( zLastWinTitle == zWinTitle ) )
			{
				/* We don’t update the information window here if its opened */
				m_pcIcons[m_nLastWindowCount-i]->SetTitle( zWinTitle );
				//cout<<"Title changed"<<endl;
			}
			if( ( bIconPresent != bLastIconPresent ) ||
				( hIconArea != hLastIconArea ) || 
				( bMinimized != bLastMinimized ) ) 
			{
				return( UpdateWindows( &cWindows, nCount ) );
				
			}
				
		} else {
			return( UpdateWindows( &cWindows, nCount ) );
		}
		
	}
}

void DockWin::ScreenModeChanged( const os::IPoint& cNewRes, os::color_space eSpace )
{
	/* Update desktop parameters */
	delete( m_pcDesktop );
	m_pcDesktop = new os::Desktop();
	
	
	/* Resize ourself */
	SetFrame( GetDockFrame() );
	
	/* Reload window list */
	os::Message cWindows;
	int32 nCount = m_pcDesktop->GetWindows( &cWindows );
	
	/* Update list */
	UpdateWindows( &cWindows, nCount );
	UpdatePlugins();
}

void DockWin::DesktopActivated( int nDesktop, bool bActive )
{
	/* Update desktop parameters */
	delete( m_pcDesktop );
	m_pcDesktop = new os::Desktop();
	
	/* Resize ourself */
	SetFrame( GetDockFrame() );
	
	/* Reload window list */
	os::Message cWindows;
	int32 nCount = m_pcDesktop->GetWindows( &cWindows );
	
	/* Update list */
	UpdateWindows( &cWindows, nCount );
	UpdatePlugins();
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
}

DockApp::DockApp( const char *pzMimeType ):os::Application( pzMimeType )
{
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
			for( uint i = 0; i < m_pcWindow->GetPlugins().size(); i++ )
				cMsg.AddString( "plugin", m_pcWindow->GetPlugins()[i]->GetPath().GetPath() );
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
					m_pcWindow->DeletePlugin( m_pcWindow->GetPlugins()[i] );
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
			
			/* Called by another application to set the position of the dock */
			int32 nPos;
			if( pcMessage->FindInt32( "position", &nPos ) == 0 )
			{
				m_pcWindow->SetPosition( (os::alignment)nPos );
			}
			m_pcWindow->SaveSettings();
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



