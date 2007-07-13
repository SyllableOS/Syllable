/*  Syllable Desktop
 *  Copyright (C) 2003 Arno Klenke
 *
 *  Andreas Benzler 2006 - some font functions and clean ups.
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
 
#include <gui/image.h>
#include <gui/imagebutton.h>
#include <gui/desktop.h>
#include <iostream>
#include <unistd.h>
#include <util/application.h>
#include <util/settings.h>
#include <storage/directory.h>
#include "desktop.h"
#include "messages.h"
#include "resources/desktop.h"

#undef NEW_FILESYSTEM

static void create_desktop_icon( os::String cAim, os::String cName, os::String cIcon )
{
	const char *pzHome = getenv( "HOME" );
	os::String zTemp = os::String( pzHome ) + ( "/Desktop/" );
	symlink( cAim.c_str(), os::String( zTemp + cName ).c_str() );
	if( cIcon == "" )
		return;
	int nFile = open( cAim.c_str(), O_RDWR );
	if( nFile >= 0 )
	{
		write_attr( nFile, "os::Icon", O_TRUNC, ATTR_TYPE_STRING, cIcon.c_str(), 0, cIcon.Length() );
		close( nFile );
	}
}

Desktop::Desktop() : os::Window( os::Rect(), "desktop", MSG_DESKTOP_TITLE, os::WND_NO_BORDER | os::WND_BACKMOST, os::ALL_DESKTOPS )
{
	const char *pzHome = getenv( "HOME" );
	os::Path cPath = os::Path( pzHome );
	cPath.Append( "Desktop" );
	
	os::Desktop cDesktop;
	SetFrame( os::Rect( os::Point( 0, 0 ), os::Point( cDesktop.GetResolution() ) ) );
	
	/* Create icon view */
	m_pcView = new os::IconDirectoryView( GetBounds(), pzHome, os::CF_FOLLOW_ALL );
	m_pcView->SetPath( cPath.GetPath() );
	m_pcView->SetDirChangeMsg( new os::Message( M_CHANGE_DIR ) );
	m_pcView->SetDirectoryLocked( true );
	m_pcView->SetView( os::IconView::VIEW_ICONS_DESKTOP );
	m_pcView->SetBackgroundColor( os::Color32_s( 58, 110, 170 ) );
	m_pcView->SetTextColor( os::Color32_s( 255, 255, 255 ) );
	m_pcView->SetTextShadowColor( os::Color32_s( 255, 255, 255 ) );
	
	
	
	/* Set background */
	m_pcBackground = NULL;
	m_zBackground = "None";
	m_bSingleClick = false;
	m_bFontShadows = true;
	
	/* Load settings */
	os::Settings* pcSettings = new os::Settings();
	if( pcSettings->Load() == 0 )
	{
		m_zBackground = pcSettings->GetString( "background_image", "None" );
		m_bSingleClick = pcSettings->GetBool( "single_click", false );
		m_bFontShadows =  pcSettings->GetBool( "desktop_font_shadow",true);

		os::String cPath = os::String( pzHome ) + os::String( "/Desktop" );
		os::FSNode cFileNode;
		cFileNode.SetTo( cPath );
		char zBuffer[PATH_MAX];
		cFileNode.ReadAttr( "DesktopLink::Language", ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX );
/*		os::String cLocale = os::Application::GetInstance()->GetApplicationLocale()->GetName(); */
		if( os::String(zBuffer) != MSG_DESKTOP_LINKLANGUAGE )
		{
			os::FSNode cFileNode;
			cFileNode.SetTo( cPath );
			char g_zLinks[][255] = { "Applications","Disks","Preferences","Terminal","Home","Trash" };
			for( int i = 0 ; i < 6 ; i++ )
			{
				memset( zBuffer, 0, PATH_MAX );
				cFileNode.ReadAttr( os::String( "DesktopLink::" ) + g_zLinks[i], ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX );
				unlink( (cPath + "/" + zBuffer).c_str() );
			}
			create_desktop_icon( "/Applications", MSG_DESKTOP_SYMLINKS_APPLICATIONS, "/system/icons/applications.png" );
			create_desktop_icon( "/", MSG_DESKTOP_SYMLINKS_DISKS, "" );
			create_desktop_icon( "/Applications/Preferences", MSG_DESKTOP_SYMLINKS_PREFERENCES, "/system/icons/settings.png" );
			create_desktop_icon( "/system/bin/aterm", MSG_DESKTOP_SYMLINKS_TERMINAL, "/system/icons/aterm.png" );
			create_desktop_icon( pzHome, MSG_DESKTOP_SYMLINKS_HOME, "/system/icons/home.png" );
			create_desktop_icon( os::String( pzHome ) + "/Trash", MSG_DESKTOP_SYMLINKS_TRASH, "/system/icons/trash.png" );
		}
	} else {
		os::String cPath = os::String( pzHome ) + os::String( "/Desktop" );
		os::FSNode cFileNode;
		cFileNode.SetTo( cPath );
		char zBuffer[PATH_MAX];
		char g_zLinks[][255] = { "Applications","Disks","Preferences","Terminal","Home","Trash" };
		for( int i = 0 ; i < 6 ; i++ )
		{
			memset( zBuffer, 0, PATH_MAX );
			cFileNode.ReadAttr( os::String( "DesktopLink::" ) + g_zLinks[i], ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX );
			unlink( (cPath + "/" + zBuffer).c_str() );
		}
		/* Create default links */
		create_desktop_icon( "/Applications", MSG_DESKTOP_SYMLINKS_APPLICATIONS, "/system/icons/applications.png" );
		create_desktop_icon( "/", MSG_DESKTOP_SYMLINKS_DISKS, "" );
		create_desktop_icon( "/Applications/Preferences", MSG_DESKTOP_SYMLINKS_PREFERENCES, "/system/icons/settings.png" );
		create_desktop_icon( "/system/bin/aterm", MSG_DESKTOP_SYMLINKS_TERMINAL, "/system/icons/aterm.png" );
		create_desktop_icon( pzHome, MSG_DESKTOP_SYMLINKS_HOME, "/system/icons/home.png" );
		create_desktop_icon( os::String( pzHome ) + "/Trash", MSG_DESKTOP_SYMLINKS_TRASH, "/system/icons/trash.png" );
	}
	delete( pcSettings );

/*	os::String cLocale = os::Application::GetInstance()->GetApplicationLocale()->GetName(); */
	os::String cLocale = MSG_DESKTOP_LINKLANGUAGE;
	int nFile = open( (os::String( pzHome ) + ( "/Desktop" )).c_str(), O_RDWR | O_NOTRAVERSE );
	write_attr( nFile, "DesktopLink::Language", O_TRUNC, ATTR_TYPE_STRING, cLocale.c_str(), 0, strlen( cLocale.c_str() ) );
	write_attr( nFile, "DesktopLink::Applications", O_TRUNC, ATTR_TYPE_STRING, MSG_DESKTOP_SYMLINKS_APPLICATIONS.c_str(), 0, strlen( MSG_DESKTOP_SYMLINKS_APPLICATIONS.c_str() ) );
	write_attr( nFile, "DesktopLink::Disks", O_TRUNC, ATTR_TYPE_STRING, MSG_DESKTOP_SYMLINKS_DISKS.c_str(), 0, strlen( MSG_DESKTOP_SYMLINKS_DISKS.c_str() ) );
	write_attr( nFile, "DesktopLink::Preferences", O_TRUNC, ATTR_TYPE_STRING, MSG_DESKTOP_SYMLINKS_PREFERENCES.c_str(), 0, strlen( MSG_DESKTOP_SYMLINKS_PREFERENCES.c_str() ) );
	write_attr( nFile, "DesktopLink::Terminal", O_TRUNC, ATTR_TYPE_STRING, MSG_DESKTOP_SYMLINKS_TERMINAL.c_str(), 0, strlen( MSG_DESKTOP_SYMLINKS_TERMINAL.c_str() ) );
	write_attr( nFile, "DesktopLink::Home", O_TRUNC, ATTR_TYPE_STRING, MSG_DESKTOP_SYMLINKS_HOME.c_str(), 0, strlen( MSG_DESKTOP_SYMLINKS_HOME.c_str() ) );
	write_attr( nFile, "DesktopLink::Trash", O_TRUNC, ATTR_TYPE_STRING, MSG_DESKTOP_SYMLINKS_TRASH.c_str(), 0, strlen( MSG_DESKTOP_SYMLINKS_TRASH.c_str() ) );
	close( nFile );

	LoadBackground();
	
	AddChild( m_pcView );
	m_pcView->ReRead();
	m_pcView->MakeFocus();
	m_pcView->SetSingleClick( m_bSingleClick );

	if( m_bFontShadows )
		m_pcView->SetTextShadowColor( os::Color32_s( 80, 80, 80 ) );
	
	/* Register calls  */
	try
	{
		m_pcGetSingleClickEv = os::Event::Register( "os/Desktop/GetSingleClickInterface", "Returns whether the single-click interface is enabled",
						os::Application::GetInstance(), M_GET_SINGLE_CLICK );
		m_pcSetSingleClickEv = os::Event::Register( "os/Desktop/SetSingleClickInterface", "Enables/Disables the single-click interface",
						os::Application::GetInstance(), M_SET_SINGLE_CLICK );
		m_pcGetFontShadowEv = os::Event::Register( "os/Desktop/GetDesktopFontShadow", "Gets current state of font shadow",
						os::Application::GetInstance(), M_GET_DESKTOP_FONT_SHADOW );
		m_pcSetFontShadowEv = os::Event::Register( "os/Desktop/SetDesktopFontShadow", "Enables/Disables font shadow",
						os::Application::GetInstance(), M_SET_DESKTOP_FONT_SHADOW );
		m_pcGetBackgroundEv = os::Event::Register( "os/Desktop/GetBackgroundImage", "Returns the current background image",
						os::Application::GetInstance(), M_GET_BACKGROUND );
		m_pcSetBackgroundEv = os::Event::Register( "os/Desktop/SetBackgroundImage", "Sets the current background image",
						os::Application::GetInstance(), M_SET_BACKGROUND );
		m_pcRefreshEv = os::Event::Register( "os/Desktop/Refresh", "Refreshes the desktop",
						os::Application::GetInstance(), M_REFRESH );
		os::Message cMsg;
		cMsg.AddBool( "single_click", m_bSingleClick );
		m_pcSetSingleClickEv->PostEvent( &cMsg );
						
	} catch( ... )
	{
	}
	
	
	
	LaunchFiles();
	
	
}

Desktop::~Desktop()
{
	try
	{
		delete( m_pcGetSingleClickEv );
		delete( m_pcSetSingleClickEv );
		delete( m_pcGetFontShadowEv );
		delete( m_pcSetFontShadowEv );
		delete( m_pcGetBackgroundEv );
		delete( m_pcSetBackgroundEv );
		delete( m_pcRefreshEv );
		
	} catch ( ... )
	{
	}
}

void Desktop::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_CHANGE_DIR:
		{
			os::String zPath;
			if( pcMessage->FindString( "file/path", &zPath.str() ) == 0 )
			{
				/* Launch filemanager */
				if( fork() == 0 )
				{
					set_thread_priority( -1, 0 );
					execlp( "/system/bin/FileBrowser", "/system/bin/FileBrowser", zPath.c_str(), NULL );
				}
			}
			break;
		}
		
		case M_APP_QUIT:
		{
			OkToQuit();
			break;
		}
		
		case M_REFRESH:
		{
			m_pcView->SetSelectionColor(os::get_default_color(os::COL_ICON_SELECTED));
			m_pcView->Layout();
			break;
		}
		case M_RELAYOUT:
		{
			
			break;
		}
		break;
	}
}

bool Desktop::OkToQuit()
{
	return( false );
}

void Desktop::ScreenModeChanged( const os::IPoint& cRes, os::color_space eSpace )
{
	os::Desktop cDesktop;
	SetFrame( os::Rect( os::Point( 0, 0 ), os::Point( cDesktop.GetResolution() ) ) );
	m_pcView->SetFrame( os::Rect( os::Point( 0, 0 ), os::Point( cDesktop.GetResolution() ) ) );
	LoadBackground();
}

void Desktop::DesktopActivated( int nDesktop, bool bActive )
{
	os::Desktop cDesktop;
	SetFrame( os::Rect( os::Point( 0, 0 ), os::Point( cDesktop.GetResolution() ) ) );
	m_pcView->SetFrame( os::Rect( os::Point( 0, 0 ), os::Point( cDesktop.GetResolution() ) ) );
}

void Desktop::LaunchFiles()
{
	/* Launch dock */
 	if ( fork() == 0 )
    {
		set_thread_priority( -1, 0 );
		execlp( "/system/bin/Dock", "/system/bin/Dock", NULL );
		exit( 1 );
	}
	
    os::String zName;
    os::String zPath;
    os::Directory* pcDir = new os::Directory();

    if(pcDir->SetTo("~/Settings/Desktop/Startup")==0)
    {
        pcDir->GetPath(&zPath);
        while( pcDir->GetNextEntry( &zName ) ) {
            if ( (zName != "..") && (zName != ".") && (zName.find( "Disabled", 0 )==std::string::npos) )
            {
		        pid_t nPid = fork();
		        if ( nPid == 0 )
		        {
	        	    set_thread_priority( -1, 0 );
	    	        execlp( (zPath + '/' + zName).c_str(), zName.c_str(), NULL );
					printf( "desktop: failed to run startup program %s\n", zName.c_str() );
        	    	exit( 1 );
		        }
            }
	    }
    }
    delete pcDir;
}


void Desktop::SetBackground( os::String zBackground )
{
	m_zBackground = zBackground;
	
	LoadBackground();
	SaveSettings();
}

void Desktop::SetSingleClick( bool bSingleClick )
{
	m_bSingleClick = bSingleClick;
	os::Message cMsg;
	cMsg.AddBool( "single_click", bSingleClick );
	m_pcSetSingleClickEv->PostEvent( &cMsg );
	m_pcView->SetSingleClick( bSingleClick );
	SaveSettings();
}

void Desktop::SetDesktopFontShadow( bool bDesktopFontShadow )
{
	m_bFontShadows = bDesktopFontShadow;

	if ( m_bFontShadows ) 
		m_pcView->SetTextShadowColor( os::Color32_s( 80, 80, 80 ) );
	else 
		m_pcView->SetTextShadowColor( os::Color32_s( 255, 255, 255 ) );
	
	SaveSettings();
}
		
void Desktop::SaveSettings()
{
	os::Settings* pcSettings = new os::Settings();
	pcSettings->SetString( "background_image", m_zBackground );
	pcSettings->SetBool( "single_click", m_bSingleClick );
	pcSettings->SetBool( "desktop_font_shadow", m_bFontShadows );
	pcSettings->Save();
	delete( pcSettings );
}

void Desktop::LoadBackground()
{
	/* Load/reload background image */
	bool bNone = false;

	os::File cFile;
	
	try
	{
		if( m_zBackground == "None" )
			bNone = true;
		else {
			os::Path cPath = os::Path( "/system/resources/wallpapers/" );
			cPath.Append( m_zBackground.c_str() );
			cFile.SetTo( cPath );
			m_pcBackground = new os::BitmapImage( &cFile );
		}
	} catch( ... )
	{
		bNone = true;
	}
	
	
	if( bNone )
	{
		m_pcBackground = NULL;
	}
	else
	{
		/* Scale bitmap */
		os::Desktop cDesk;
		if( m_pcBackground->GetSize() != os::Point( cDesk.GetResolution() ) )
			m_pcBackground->SetSize( os::Point( cDesk.GetResolution() ) );
	}
	m_pcView->SetBackground( m_pcBackground );
}

