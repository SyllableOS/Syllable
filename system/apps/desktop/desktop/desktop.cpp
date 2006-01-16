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
#include <util/settings.h>
#include <storage/directory.h>
#include "desktop.h"
#include "messages.h"

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

Desktop::Desktop() : os::Window( os::Rect(), "desktop", "Desktop", os::WND_NO_BORDER | os::WND_BACKMOST, os::ALL_DESKTOPS )
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
	m_pcView->SetBackgroundColor( get_default_color( os::COL_MENU_BACKGROUND ) );
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
	} else {
		/* Create default links */
		os::String zTemp = os::String( pzHome ) + ( "/Desktop/" );
		#ifdef NEW_FILESYSTEM
		create_desktop_icon( "/boot/applications", "Applications", "/system/icons/applications.png" );
		create_desktop_icon( "/", "Disks", "" );
		create_desktop_icon( "/boot/applications/preferences", "Preferences", "/system/icons/settings.png" );
		create_desktop_icon( "/boot/system/bin/aterm", "Terminal", "/system/icons/aterm.png" );
		#else
		create_desktop_icon( "/boot/atheos/Applications", "Applications", "/system/icons/applications.png" );
		create_desktop_icon( "/", "Disks", "" );
		create_desktop_icon( "/boot/atheos/Applications/Preferences", "Preferences", "/system/icons/settings.png" );
		create_desktop_icon( "/boot/atheos/sys/bin/aterm", "Terminal", "/system/icons/aterm.png" );
		#endif
		create_desktop_icon( pzHome, "Home", "/system/icons/home.png" );
		create_desktop_icon( os::String( pzHome ) + "/Trash", "Trash", "/system/icons/trash.png" );
	}
	delete( pcSettings );
	
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
		os::RegistrarManager* pcManager = os::RegistrarManager::Get();
		if( pcManager ) {

			pcManager->RegisterCall( "os/Desktop/SetDesktopFont", "Sets a new desktop font",
							os::Application::GetInstance(), M_SET_DESKTOP_FONT );
			pcManager->RegisterCall( "os/Desktop/GetDesktopFont", "Gets a new desktop font",
							os::Application::GetInstance(), M_GET_DESKTOP_FONT );

			pcManager->RegisterCall( "os/Desktop/SetDesktopFontColor", "Sets the desktop font color",
							os::Application::GetInstance(), M_SET_DESKTOP_FONT_COLOR );
			pcManager->RegisterCall( "os/Desktop/GetDesktopFontColor", "Gets the desktop font color",
							os::Application::GetInstance(), M_GET_DESKTOP_FONT_COLOR );

			pcManager->RegisterCall( "os/Desktop/SetDesktopFontShadowColor", "Sets color for desktop font",
							os::Application::GetInstance(), M_SET_DESKTOP_FONT_SHADOW_COLOR );
			pcManager->RegisterCall( "os/Desktop/GetDesktopFontShadowColor", "Gets color for desktop font",
							os::Application::GetInstance(), M_GET_DESKTOP_FONT_SHADOW_COLOR );

			pcManager->RegisterCall( "os/Desktop/SetDesktopFontShadow", "Enables/Disables font shadow",
							os::Application::GetInstance(), M_SET_DESKTOP_FONT_SHADOW );
			pcManager->RegisterCall( "os/Desktop/GetDesktopFontShadow", "Gets current state of font shadow",
							os::Application::GetInstance(), M_GET_DESKTOP_FONT_SHADOW );
			pcManager->RegisterCall( "os/Desktop/GetSingleClickInterface", "Returns whether the single-click interface is enabled",
							os::Application::GetInstance(), M_GET_SINGLE_CLICK );
			pcManager->RegisterCall( "os/Desktop/SetSingleClickInterface", "Enables/Disables the single-click interface",
							os::Application::GetInstance(), M_SET_SINGLE_CLICK );
			pcManager->RegisterCall( "os/Desktop/GetBackgroundImage", "Returns the current background image",
							os::Application::GetInstance(), M_GET_BACKGROUND );
			pcManager->RegisterCall( "os/Desktop/SetBackgroundImage", "Sets the current background image",
							os::Application::GetInstance(), M_SET_BACKGROUND );
			pcManager->RegisterCall( "os/Desktop/Refresh","Refreshes the desktop",
				os::Application::GetInstance(),M_REFRESH );
			pcManager->Put();
		}
	} catch( ... )
	{
	}
	
	LaunchFiles();
	
	
}

Desktop::~Desktop()
{
	try
	{
		os::RegistrarManager* pcManager = os::RegistrarManager::Get();
		if( pcManager ) {
			pcManager->UnregisterCall( "os/Desktop/SetDesktopFont" );
			pcManager->UnregisterCall( "os/Desktop/SetDesktopFontColor" );
			pcManager->UnregisterCall( "os/Desktop/SetDesktopFontShadowColor" );
			pcManager->UnregisterCall( "os/Desktop/SetDesktopFontShadow" );
			pcManager->UnregisterCall( "os/Desktop/GetDesktopFont" );
			pcManager->UnregisterCall( "os/Desktop/GetDesktopFontColor" );
			pcManager->UnregisterCall( "os/Desktop/GetDesktopFontShadowColor" );
			pcManager->UnregisterCall( "os/Desktop/GetDesktopFontShadow" );

			pcManager->UnregisterCall( "os/Desktop/GetSingleClickInterface" );
			pcManager->UnregisterCall( "os/Desktop/SetSingleClickInterface" );
			pcManager->UnregisterCall( "os/Desktop/GetBackgroundImage" );
			pcManager->UnregisterCall( "os/Desktop/SetBackgroundImage" );

			pcManager->UnregisterCall( "os/Desktop/Refresh" );
			pcManager->Put();
		}
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
			m_pcView->Invalidate();
			m_pcView->Flush();
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
	
	/* Taken from Rick’s desktop */
    std::vector<std::string> launch_files;
    std::string zName;
    std::string zPath;
	os::String zTemp;
    os::Directory* pcDir = new os::Directory();

    if(pcDir->SetTo("~/Settings/Desktop/Startup")==0)
    {
        pcDir->GetPath(&zTemp);
		zName = zTemp.str();
        while (pcDir->GetNextEntry(&zTemp))
			zName = zTemp.str();
            if ( (zName.find( "..",0,1)==std::string::npos) && (zName.find( "Disabled",0)==std::string::npos))
            {

                launch_files.push_back(zName);
            }
    }
    for (uint32 n = 0; n < launch_files.size(); n++)
    {
        pid_t nPid = fork();
        if ( nPid == 0 )
        {
            set_thread_priority( -1, 0 );
            std::string sLaunch = launch_files[n];
            execlp(launch_files[n].c_str(), sLaunch.c_str(), NULL );
            exit( 1 );
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
	bool bLoadDefault = false;

	os::File cFile;
	
	try
	{
		if( m_zBackground == "None" )
			bLoadDefault = true;
		else {
			const char *pzHome = getenv( "HOME" );
			os::Path cPath = os::Path( pzHome );
			cPath.Append( "Documents/Pictures" );
			cPath.Append( m_zBackground.c_str() );
			cFile.SetTo( cPath );
			m_pcBackground = new os::BitmapImage( &cFile );
		}
	} catch( ... )
	{
		bLoadDefault = true;
	}
	
	/* Load default background */
	if( bLoadDefault )
	{
		cFile.SetTo( open_image_file( get_image_id() ) );
		os::Resources cCol( &cFile );		
		os::ResStream *pcStream = cCol.GetResourceStream( "background.jpg" );
		m_pcBackground = new os::BitmapImage( pcStream );
		delete( pcStream );
	}
	
	/* Scale bitmap */
	os::Desktop cDesk;
	m_pcBackground->SetSize( os::Point( cDesk.GetResolution() ) );
		
	m_pcView->SetBackground( m_pcBackground );
}





