/*  Syllable Dock Preferences
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
#include "resources/Dock.h"

enum {
	DP_APPLY,
	DP_UNDO,
	DP_DEFAULT,
	DP_ENABLE,
	DP_PLUGIN_CHANGED
};


class EventReceiver : public os::Looper
{
public:
	EventReceiver() : os::Looper( "event_receiver" )
	{
		m_bReceived = false;
	}
	
	void HandleMessage( os::Message* pcMsg )
	{
		if( m_bReceived )
			return;
		m_cMsg = *pcMsg;
		m_bReceived = true;
	}
	
	os::Message GetMessage()
	{
		Lock();
		while( !m_bReceived )
		{
			Unlock();
			snooze( 1000 );
			Lock();
		}
		m_bReceived = false;
		os::Message cMsg = m_cMsg;
		Unlock();
		return( cMsg );
	}
	
private:
	bool m_bReceived;
	os::Message m_cMsg;
};

PrefsDockWin::PrefsDockWin( os::Rect cFrame )
			: os::Window( cFrame, "dock_prefs_win", MSG_MAINWND_TITLE, 0/*os::WND_NOT_RESIZABLE*/ )
{
	/* Create root view */
	m_pcRoot = new os::LayoutView( GetBounds(), "dock_prefs_root" );
	
	/* Create vertical node */
	m_pcVLayout = new os::VLayoutNode( "dock_prefs_vlayout" );
	m_pcVLayout->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	
	/* Create position frame */
	m_pcHPos = new os::HLayoutNode( "dock_prefs_hpos" );
	m_pcHPos->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	m_pcPosition = new os::FrameView( os::Rect(), "dock_prefs_position", MSG_MAINWND_POSITION );
	
	/* Create position selector */
	m_pcPos = new os::DropdownMenu( os::Rect(), "dock_prefs_pos" );
	m_pcPos->AppendItem( MSG_MAINWND_POSITION_TOP );
	m_pcPos->AppendItem( MSG_MAINWND_POSITION_BOTTOM );
	m_pcPos->AppendItem( MSG_MAINWND_POSITION_LEFT );
	m_pcPos->AppendItem( MSG_MAINWND_POSITION_RIGHT );
	m_pcHPos->AddChild( m_pcPos );
	m_pcHPos->AddChild( new os::HLayoutSpacer( "" ) );
	
	/* Create plugins frame */
	m_pcVPlugins = new os::VLayoutNode( "dock_prefs_vplugins" );
	m_pcVPlugins->SetBorders( os::Rect( 5, 5, 5, 5 ) );
	m_pcPlugins = new os::FrameView( os::Rect(), "dock_prefs_plugins", MSG_MAINWND_PLUGINS );
	
	
	/* Create plugins list */
	m_pcPluginsList = new os::ListView( os::Rect(), "dock_prefs_plist", os::ListView::F_RENDER_BORDER );
	m_pcPluginsList->SetAutoSort( false );
	m_pcPluginsList->SetSelChangeMsg( new os::Message( DP_PLUGIN_CHANGED ) );
	m_pcPluginsList->SetInvokeMsg( new os::Message( DP_ENABLE ) );
	m_pcPluginsList->InsertColumn( MSG_MAINWND_PLUGINS_PLUGIN.c_str(), 200 );
	m_pcPluginsList->InsertColumn( MSG_MAINWND_PLUGINS_ENABLED.c_str(), 100 );
	m_pcEnable = new os::Button( os::Rect(), "dock_prefs_enable", "Enable", new os::Message( DP_ENABLE ) );
	m_pcEnable->SetEnable( false );
	m_pcPluginNote = new os::StringView( os::Rect(), "dock_prefs_note", MSG_MAINWND_PLUGINS_TEXT );
	m_pcVPlugins->AddChild( m_pcPluginsList );
	m_pcVPlugins->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVPlugins->AddChild( m_pcEnable );
	m_pcVPlugins->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVPlugins->AddChild( m_pcPluginNote );
	m_pcVPlugins->SameHeight( "dock_prefs_enable", "dock_prefs_note", NULL );
	
	/* Create buttons node */
	m_pcHButtons = new os::HLayoutNode( "dock_prefs_buttons" );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "" ) );
	
	/* Create Apply Button */
	m_pcApply = new os::Button( os::Rect(), "dock_prefs_apply", MSG_MAINWND_BUTTON_APPLY, new os::Message( DP_APPLY ) );
	m_pcHButtons->AddChild( m_pcApply );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcUndo = new os::Button( os::Rect(), "dock_prefs_undo", MSG_MAINWND_BUTTON_UNDO, new os::Message( DP_UNDO ) );
	m_pcHButtons->AddChild( m_pcUndo );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcDefault = new os::Button( os::Rect(), "dock_prefs_default", MSG_MAINWND_BUTTON_DEFAULT, new os::Message( DP_DEFAULT ) );
	m_pcHButtons->AddChild( m_pcDefault );
	m_pcHButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcHButtons->SameWidth( "dock_prefs_apply", "dock_prefs_undo", "dock_prefs_default", NULL );
	m_pcHButtons->SameHeight( "dock_prefs_apply", "dock_prefs_undo", "dock_prefs_default", NULL );
	
	m_pcVLayout->AddChild( m_pcPosition );
	m_pcVLayout->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVLayout->AddChild( m_pcPlugins );
	m_pcVLayout->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVLayout->AddChild( m_pcHButtons );
	m_pcPosition->SetRoot( m_pcHPos );
	m_pcPlugins->SetRoot( m_pcVPlugins );
	m_pcRoot->SetRoot( m_pcVLayout );
	AddChild( m_pcRoot );
	
	/* Set Tab order and default button */
	int nTabOrder = 0;
	m_pcApply->SetTabOrder( nTabOrder++ );
	m_pcUndo->SetTabOrder( nTabOrder++ );
	m_pcDefault->SetTabOrder( nTabOrder++ );
	m_pcEnable->SetTabOrder( nTabOrder++ );
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
	
	/* Connect to the dock and get a list of the currently enabled plugins */
	m_cEnabledPlugins.clear();
	m_cDefaultEnabledPlugins.clear();
	m_cSavedEnabledPlugins.clear();
	
	try
	{
	os::Message cReply;
	os::Message cDummy;
	
	os::Event cEvent;
	os::Message cMsg;
	EventReceiver* pcReceiver = new EventReceiver();
	pcReceiver->Run();
	
	/* Get enabled plugins */
	if( cEvent.SetToRemote( "os/Dock/GetPlugins", 0 ) == 0 )
	{
		if( cEvent.PostEvent( &cMsg, pcReceiver, 0 ) == 0 )
		{
			cReply = pcReceiver->GetMessage();
			int i = 0;
			os::String zEnabledPlugin;
			while( cReply.FindString( "plugin", &zEnabledPlugin.str(), i ) == 0 )
			{
				m_cEnabledPlugins.push_back( zEnabledPlugin );
				i++;
			}
		}
	}
	/* Get the position */
	if( cEvent.SetToRemote( "os/Dock/GetPosition", 0 ) == 0 )
	{
		if( cEvent.PostEvent( &cMsg, pcReceiver, 0 ) == 0 )
		{
			cReply = pcReceiver->GetMessage();
			int32 nPosition;
			cReply.FindInt32( "position", &nPosition );
		
			if( nPosition == os::ALIGN_TOP )
				m_pcPos->SetSelection( 0 );
			else if( nPosition == os::ALIGN_BOTTOM )
				m_pcPos->SetSelection( 1 );
			else if( nPosition == os::ALIGN_LEFT )
				m_pcPos->SetSelection( 2 );
			else if( nPosition == os::ALIGN_RIGHT )
				m_pcPos->SetSelection( 3 );
		}
	}
	pcReceiver->Quit();
	} catch(...) {
	}
	
	/* Create a backup of the list */
	m_cSavedEnabledPlugins = m_cEnabledPlugins;
	
	/* Update plugins list */
	UpdatePluginsList();
}

PrefsDockWin::~PrefsDockWin()
{
}

bool PrefsDockWin::CheckPlugin( os::String zPath, os::String* pzName )
{
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
				if( pcPlugin )
				{
					*pzName = pcPlugin->GetIdentifier();
					delete( pcPlugin );
					unload_library( nID );
					return( true );
				}
			}
			unload_library( nID );
		}
	}
	return( false );
}

void PrefsDockWin::UpdatePluginsList()
{
	/* Clear list */
	m_cPluginFiles.clear();
	m_cDefaultEnabledPlugins.clear();
	m_pcPluginsList->Clear();
	m_pcEnable->SetLabel( MSG_MAINWND_PLUGINS_BUTTON_ENABLE );
	m_pcEnable->SetEnable( false );
	
	/* Fill list with entries from /system/extensions/dock/ */
	os::Directory* pcDir = NULL;
	
	try
	{
		pcDir = new os::Directory( "/boot/system/extensions/dock" );
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
		os::Path cFilePath( "/boot/system/extensions/dock" );
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
		os::String zPlugin;
		if( pcNode->IsFile() && CheckPlugin( cFilePath.GetPath(), &zPlugin ) )
		{
			/* Add it to the list */
			DockPluginFile sFile;
			sFile.zPath = cFilePath.GetPath();
			sFile.zName = zPlugin;
			m_cPluginFiles.push_back( sFile );
			
			os::ListViewStringRow* pcRow = new os::ListViewStringRow();
			pcRow->AppendString( zPlugin );
			pcRow->AppendString( MSG_MAINWND_PLUGINS_STATE_ENABLED_NO );
			
			/* Check if the plugin is enabled */
			for( uint i = 0; i < m_cEnabledPlugins.size(); i++ )
			{
				if( m_cEnabledPlugins[i] == cFilePath.GetPath() ) {
					pcRow->SetString( 1, MSG_MAINWND_PLUGINS_STATE_ENABLED_YES );
					break;
				}
			}
			
			/* If this is the clock then add it to the default list */
			if( zPlugin == "Clock" )
				m_cDefaultEnabledPlugins.push_back( cFilePath.GetPath() );
			
			m_pcPluginsList->InsertRow( pcRow );
			
		}
		delete( pcNode );
	}
	delete( pcDir );
}

bool PrefsDockWin::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( true );
}

void PrefsDockWin::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case DP_APPLY:
		{
			/* Tell the Dock about the change */
			try
			{
			os::Event cEvent;
			os::Message cReply;
			os::Message cPlugins;
			os::Message cPos;
	
			/* Plugins */
			if( cEvent.SetToRemote( "os/Dock/SetPlugins", 0 ) == 0 )
			{
				for( uint i = 0; i < m_cEnabledPlugins.size(); i++ )
					cPlugins.AddString( "plugin", m_cEnabledPlugins[i] );
				cEvent.PostEvent( &cPlugins, NULL, 0 );
			}
			if( cEvent.SetToRemote( "os/Dock/SetPosition", 0 ) == 0 )
			{
				int32 nPosition = os::ALIGN_TOP;
				if( m_pcPos->GetSelection() == 0 )
					nPosition = os::ALIGN_TOP;
				else if( m_pcPos->GetSelection() == 1 )
					nPosition = os::ALIGN_BOTTOM;
				else if( m_pcPos->GetSelection() == 2 )
					nPosition = os::ALIGN_LEFT;
				else if( m_pcPos->GetSelection() == 3 )
					nPosition = os::ALIGN_RIGHT;
				cPos.AddInt32( "position", nPosition );
				cEvent.PostEvent( &cPos, NULL, 0 );
			}
			} catch( ... ) {
			}
			break;
		}
		case DP_UNDO:
		{
			/* Revert to a previous copy of the enabled plugins */
			m_cEnabledPlugins = m_cSavedEnabledPlugins;
			UpdatePluginsList();
			break;
		}
		case DP_DEFAULT:
		{
			/* Revert to the default */
			m_cEnabledPlugins = m_cDefaultEnabledPlugins;
			UpdatePluginsList();
			break;
		}
		case DP_ENABLE:
		{
			/* Check the selected listview entry */
			int nSelected;
			nSelected = m_pcPluginsList->GetFirstSelected();
			os::ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcPluginsList->GetRow( nSelected ) );
			if( pcRow )
			{
				os::String zPlugin = pcRow->GetString( 0 );
				os::String zNewState;
				/* Check if the plugin is disabled or enabled */
				if( pcRow->GetString( 1 ) == MSG_MAINWND_PLUGINS_STATE_ENABLED_YES ) {
					zNewState = MSG_MAINWND_PLUGINS_STATE_ENABLED_NO;
					/* Remove from list */
					for( uint i = 0; i < m_cPluginFiles.size(); i++ )
					{
						if( m_cPluginFiles[i].zName == zPlugin ) {
							for( uint j = 0; j < m_cEnabledPlugins.size(); j++ )
							{
								if( m_cEnabledPlugins[j] == m_cPluginFiles[i].zPath )
								{
									m_cEnabledPlugins.erase( m_cEnabledPlugins.begin() + j );
								}
							}
						}
					}	
					
				} else {
					zNewState = MSG_MAINWND_PLUGINS_STATE_ENABLED_YES;
					/* Add to list */
					for( uint i = 0; i < m_cPluginFiles.size(); i++ )
					{
						if( m_cPluginFiles[i].zName == zPlugin )
							m_cEnabledPlugins.push_back( m_cPluginFiles[i].zPath );
					}	
				}
				/* Replace the rows */
				delete( m_pcPluginsList->RemoveRow( nSelected ) );
				os::ListViewStringRow* pcRow = new os::ListViewStringRow();
				pcRow->AppendString( zPlugin );
				pcRow->AppendString( zNewState );
				m_pcPluginsList->InsertRow( nSelected, pcRow );
				m_pcPluginsList->Select( nSelected );
			}
			break;
		}
		case DP_PLUGIN_CHANGED:
		{
			/* Check the selected listview entry */
			int nSelected;
			nSelected = m_pcPluginsList->GetFirstSelected();
			os::ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcPluginsList->GetRow( nSelected ) );
			if( pcRow )
			{
				/* Check if the plugin is disabled or enabled */
				if( pcRow->GetString( 1 ) == MSG_MAINWND_PLUGINS_STATE_ENABLED_YES )
					m_pcEnable->SetLabel( MSG_MAINWND_PLUGINS_BUTTON_DISABLE );
				else
					m_pcEnable->SetLabel( MSG_MAINWND_PLUGINS_BUTTON_ENABLE );
				m_pcEnable->SetEnable( true );
			}
			break;
		}
		default:
			os::Window::HandleMessage( pcMessage );
	}
}
 
PrefsDockApp::PrefsDockApp():os::Application( "application/x-vnd-DockPreferences" )
{
	SetCatalog("Dock.catalog");

	/* Show main window */
	PrefsDockWin* pcWindow = new PrefsDockWin( os::Rect( 0, 0, 360, 350 ) );
	pcWindow->CenterInScreen();
	pcWindow->Show();
	pcWindow->MakeFocus();
}

bool PrefsDockApp::OkToQuit()
{
	return ( true );
}

PrefsDockApp::~PrefsDockApp()
{

}


int main( int argc, char *argv[] )
{
	PrefsDockApp *pcPrefsDockApp;

	pcPrefsDockApp = new PrefsDockApp();
	pcPrefsDockApp->Run();
}

