/*  Syllable Desktop Preferences
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
 
#include "Prefs.h" 
#include <appserver/protocol.h>
#include <storage/registrar.h>
#include <iostream>

enum {
	DP_APPLY,
	DP_UNDO,
	DP_DEFAULT,
	DP_BACKGROUND_CHANGED,
	
	/* Desktop messages */
	M_GET_SETTINGS = 100,
	M_SET_SETTINGS
};


PrefsDesktopWin::PrefsDesktopWin( os::Rect cFrame )
			: os::Window( cFrame, "desktop_prefs_win", "Desktop", 0/*os::WND_NOT_RESIZABLE*/ )
{
	/* Create root view */
	m_pcRoot = new os::LayoutView( GetBounds(), "desktop_prefs_root" );
	
	/* Create vertical node */
	m_pcVLayout = new os::VLayoutNode( "desktop_prefs_vlayout" );
	m_pcVLayout->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	
	/* Create background frame */
	m_pcVBackground = new os::VLayoutNode( "desktop_prefs_vbackground" );
	m_pcVBackground->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	m_pcBackground = new os::FrameView( os::Rect(), "desktop_prefs_background", "Background" );
	
	
	/* Create background list */
	m_pcBackgroundList = new os::ListView( os::Rect(), "desktop_prefs_plist", os::ListView::F_RENDER_BORDER );
	m_pcBackgroundList->SetAutoSort( false );
	m_pcBackgroundList->SetSelChangeMsg( new os::Message( DP_BACKGROUND_CHANGED ) );
	m_pcBackgroundList->InsertColumn( "Image", (int)GetBounds().Width() );
	m_pcBackgroundNote = new os::StringView( os::Rect(), "desktop_prefs_note", "Note: Please copy images to Documents/Pictures" );
	m_pcVBackground->AddChild( m_pcBackgroundList, 1.0f );
	m_pcVBackground->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVBackground->AddChild( m_pcBackgroundNote, 0.0f );
	
	/* Create window settings */
	m_pcVWindows = new os::VLayoutNode( "desktop_prefs_vwindows" );
	m_pcVWindows->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	m_pcPopupWindows = new os::CheckBox( os::Rect(), "desktop_prefs_popup", "Popup selected window", NULL );
	m_pcSingleClick = new os::CheckBox( os::Rect(), "desktop_prefs_sc", "Use single-click interface", NULL );
	m_pcVWindows->AddChild( m_pcPopupWindows );
	m_pcVWindows->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVWindows->AddChild( m_pcSingleClick );
	
	/* Create windows frame */
	m_pcWindows = new os::FrameView( os::Rect(), "desktop_prefs_windows", "Windows" );
	
	/* Create buttons node */
	m_pcHButtons = new os::HLayoutNode( "desktop_prefs_buttons" );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "" ) );
	
	/* Create Apply Button */
	m_pcApply = new os::Button( os::Rect(), "desktop_prefs_apply", "Apply", new os::Message( DP_APPLY ) );
	m_pcHButtons->AddChild( m_pcApply );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcUndo = new os::Button( os::Rect(), "desktop_prefs_undo", "Undo", new os::Message( DP_UNDO ) );
	m_pcHButtons->AddChild( m_pcUndo );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcDefault = new os::Button( os::Rect(), "desktop_prefs_default", "Default", new os::Message( DP_DEFAULT ) );
	m_pcHButtons->AddChild( m_pcDefault );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcHButtons->SameWidth( "desktop_prefs_apply", "desktop_prefs_undo", "desktop_prefs_default", NULL );
	m_pcHButtons->SetWeight( 0.0f );
	
	m_pcVLayout->AddChild( m_pcBackground, 1.0f );
	m_pcVLayout->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVLayout->AddChild( m_pcWindows, 0.0f );
	m_pcVLayout->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVLayout->AddChild( m_pcHButtons );
	m_pcBackground->SetRoot( m_pcVBackground );
	m_pcWindows->SetRoot( m_pcVWindows );
	
	m_pcRoot->SetRoot( m_pcVLayout );
	AddChild( m_pcRoot );
	
	/* Set Tab order and default button */
	int nTabOrder = 0;
	m_pcApply->SetTabOrder( nTabOrder++ );
	m_pcUndo->SetTabOrder( nTabOrder++ );
	m_pcDefault->SetTabOrder( nTabOrder++ );
	SetFocusChild( m_pcApply );
	SetDefaultButton( m_pcApply );
	
	/* Set Icon */
	os::File cFile( open_image_file( get_image_id() ) );
	os::Resources cCol( &cFile );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
	delete( pcStream );
	
	/* Connect to the desktop and get the currently activated wallpaper */
	try
	{
	os::RegistrarManager* pcManager = os::RegistrarManager::Get();
	os::RegistrarCall_s sCall;
	os::Message cReply;
	os::Message cDummy;
	m_zBackground = "None";
	m_bSingleClickSave = false;
	if( pcManager->QueryCall( "os/Desktop/GetSingleClickInterface", 0, &sCall ) == 0 )
	{
		if( pcManager->InvokeCall( &sCall, &cDummy, &cReply ) == 0 )
		{
			cReply.FindBool( "single_click", &m_bSingleClickSave );
		}
	}
	if( pcManager->QueryCall( "os/Desktop/GetBackgroundImage", 0, &sCall ) == 0 )
	{
		if( pcManager->InvokeCall( &sCall, &cDummy, &cReply ) == 0 )
		{
			cReply.FindString( "background_image", &m_zBackground );
		}
	}
	pcManager->Put();
	} catch(...) {
	}
	m_pcSingleClick->SetValue( m_bSingleClickSave );
	
	/* Get appserver configuration */
	os::Message cMessage;
	if( os::Messenger( os::Application::GetInstance()->GetServerPort(  ) ).SendMessage( os::DR_GET_APPSERVER_CONFIG, &cMessage ) == 0 )
	{
		cMessage.FindBool( "popoup_sel_win", &m_bPopupWindowSave );
	}
	m_pcPopupWindows->SetValue( m_bPopupWindowSave );
	
	/* Create a backup of the list */
	m_zSavedBackground = m_zBackground;
	
	/* Update plugins list */
	UpdateBackgroundList();
}

PrefsDesktopWin::~PrefsDesktopWin()
{
}
void PrefsDesktopWin::UpdateBackgroundList()
{
	/* Clear list */
	m_pcBackgroundList->Clear();
	int nSelect = 0;
	
	os::ListViewStringRow* pcRow = new os::ListViewStringRow();
	pcRow->AppendString( "None" );
	m_pcBackgroundList->InsertRow( pcRow );
	
	
	/* Fill list with entries from /system/drivers/dock/ */
	os::Directory* pcDir = NULL;
	os::Path cPath;
	
	try
	{
		cPath = getenv( "HOME" );
		cPath.Append( "Documents/Pictures" );
		pcDir = new os::Directory( cPath.GetPath() );
	} catch( ... )
	{
		return;
	}
	pcDir->Rewind();
	
	/* Iterate through the directory */
	os::String zFile;
	while( pcDir->GetNextEntry( &zFile ) == 1 )
	{
		if( zFile == os::String( "." ) ||
			zFile == os::String( ".." ) )
			continue;
			
		/* Construct path */
		os::Path cFilePath( cPath );
		cFilePath.Append( zFile.c_str() );
		
		/* Validate entry */
		os::FSNode* pcNode;
		try {
			pcNode = new os::FSNode( cFilePath );
		} catch( ... ) {
			continue;
		}
		if( !pcNode->IsValid() )
		{
			delete( pcNode );
			continue;
		}
		
		/* Look if this is a valid plugin */
		if( pcNode->IsFile()  )
		{

			os::ListViewStringRow* pcRow = new os::ListViewStringRow();
			pcRow->AppendString( zFile );
			
			m_pcBackgroundList->InsertRow( pcRow );
			if( m_zBackground == zFile )
				nSelect = m_pcBackgroundList->GetRowCount() - 1;
			
		}
		delete( pcNode );
	}
	
	m_pcBackgroundList->Select( nSelect );
	
	delete( pcDir );
}

bool PrefsDesktopWin::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( true );
}

void PrefsDesktopWin::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{

		case DP_APPLY:
		{
			/* Tell the Desktop about the change */
			try
			{
			os::RegistrarManager* pcManager = os::RegistrarManager::Get();
			os::RegistrarCall_s sCall;
			os::Message cReply;
			os::Message cSingleClick;
			os::Message cBackgroundImage;
			cSingleClick.AddBool( "single_click", m_pcSingleClick->GetValue().AsBool() );
			cBackgroundImage.AddString( "background_image", m_zBackground );
			
			if( pcManager->QueryCall( "os/Desktop/SetSingleClickInterface", 0, &sCall ) == 0 )
				pcManager->InvokeCall( &sCall, &cSingleClick, NULL );
			if( pcManager->QueryCall( "os/Desktop/SetBackgroundImage", 0, &sCall ) == 0 )
				pcManager->InvokeCall( &sCall, &cBackgroundImage, NULL );
				
			pcManager->Put();
			} catch( ... ) {
			}
			
			/* Get appserver configuration */
			os::Message cMessage( os::DR_SET_APPSERVER_CONFIG );
			cMessage.AddBool( "popoup_sel_win", m_pcPopupWindows->GetValue().AsBool() );
			os::Application* pcApp = os::Application::GetInstance();
			os::Messenger( pcApp->GetServerPort(  ) ).SendMessage( &cMessage );
			break;
		}
		case DP_UNDO:
		{
			/* Revert to a previous copy of the enabled plugins */
			m_zBackground = m_zSavedBackground;
			UpdateBackgroundList();
			m_pcPopupWindows->SetValue( m_bPopupWindowSave );
			m_pcSingleClick->SetValue( m_bSingleClickSave );
			break;
		}
		case DP_DEFAULT:
		{
			/* Revert to the default */
			m_zBackground = "None";
			UpdateBackgroundList();
			m_pcPopupWindows->SetValue( true );
			m_pcSingleClick->SetValue( false );
			break;
		}
		case DP_BACKGROUND_CHANGED:
		{
			/* Check the selected listview entry */
			int nSelected;
			nSelected = m_pcBackgroundList->GetFirstSelected();
			os::ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcBackgroundList->GetRow( nSelected ) );
			if( pcRow )
			{
				m_zBackground = pcRow->GetString( 0 );
			}
			break;
		}
		default:
			os::Window::HandleMessage( pcMessage );
	}
}
 
PrefsDesktopApp::PrefsDesktopApp():os::Application( "application/x-vnd-DesktopPreferences" )
{
	/* Get screen width and height to centre the window */
	os::Desktop * pcDesktop = new os::Desktop();
	int iWidth = pcDesktop->GetScreenMode().m_nWidth;
	int iHeight = pcDesktop->GetScreenMode().m_nHeight;
	int iLeft = ( iWidth - 300 ) / 2;
	int iTop = ( iHeight - 350 ) / 2;
	
	/* Show main window */
	PrefsDesktopWin* pcWindow = new PrefsDesktopWin( os::Rect( iLeft, iTop, iLeft + 320, iTop + 350 ) );
	pcWindow->Show();
	pcWindow->MakeFocus();
}

bool PrefsDesktopApp::OkToQuit()
{
	return ( true );
}

PrefsDesktopApp::~PrefsDesktopApp()
{

}


int main( int argc, char *argv[] )
{
	PrefsDesktopApp *pcPrefsDesktopApp;

	pcPrefsDesktopApp = new PrefsDesktopApp();
	pcPrefsDesktopApp->Run();
}


