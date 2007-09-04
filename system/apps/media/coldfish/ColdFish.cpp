/*  ColdFish Music Player
 *  Copyright (C) 2003 Kristian Van Der Vliet
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

#include "ColdFish.h"
#include <iostream>
#include <fstream>
#include <queue>
using namespace os;

/* CFListItem class */
class CFListItem : public os::ListViewStringRow
{
public:
		os::String BuildTitle()
		{
			if( zTitle == "" )
			{
				/* Use filename */
				return( os::Path( zPath ).GetLeaf() );
			}
			/* Build title string */
			os::String zRowTitle = zAuthor;
								
			if( !zRowTitle.empty() )
				zRowTitle += " - ";
					
			zRowTitle += zTitle;
			return( zRowTitle );
		}
		os::String zPath;
		os::String zAuthor;
		os::String zTitle;
		os::String zAlbum;
		int nTrack;
		int nStream;
		int64 nLength;
};

void SetCButtonImageFromResource( os::CImageButton* pcButton, os::String zResource )
{
	os::File cSelf( open_image_file( get_image_id() ) );
	os::Resources cCol( &cSelf );		
	os::ResStream *pcStream = cCol.GetResourceStream( zResource );
	pcButton->SetImage( pcStream );
	pcButton->Flush();
	delete( pcStream );
}


static inline void secs_to_ms( uint64 nTime, uint32 *nM, uint32 *nS )
{
	if ( nTime > 59 )
	{
		*nM = ( nTime / 60 );
		*nS = nTime - ( *nM * 60 );
	}
	else
	{
		*nM = 0;
		*nS = ( uint32 )nTime;
	}
}

/* CFPlaylist class */
CFPlaylist::CFPlaylist( const os::Rect & cFrame ):os::ListView( cFrame, "cf_playlist", os::ListView::F_RENDER_BORDER, os::CF_FOLLOW_ALL )
{
}

void CFPlaylist::MouseUp( const os::Point & cPos, uint32 nButtons, os::Message * pcData )
{
	os::String zFile;

	if ( pcData == NULL || pcData->FindString( "file/path", &zFile.str() ) != 0 )
	{
		return ( os::ListView::MouseUp( cPos, nButtons, pcData ) );
	}
	/* Tell CFApp class */
	os::Message cMsg( *pcData );
	cMsg.SetCode( CF_ADD_FILE );
	os::Application::GetInstance()->PostMessage( &cMsg, os::Application::GetInstance(  ) );
}



/* CFWindow class */

CFWindow::CFWindow( const os::Rect & cFrame, const os::String & cName, const os::String & cTitle, uint32 nFlags ):os::Window( cFrame, cName, cTitle, nFlags )
{
	m_nState = CF_STATE_STOPPED;
	
	os::Rect cNewFrame = GetBounds();
	
	/* Create menubar */
	m_pcMenuBar = new os::Menu( os::Rect( 0, 0, 1, 1 ), "cf_menu_bar", os::ITEMS_IN_ROW );
	
	os::Menu* pcAppMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_APPLICATION, os::ITEMS_IN_COLUMN );
	pcAppMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_ABOUT, new os::Message( CF_GUI_ABOUT ) );
	pcAppMenu->AddItem( new os::MenuSeparator() );
	pcAppMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_QUIT, new os::Message( CF_GUI_QUIT ) );
	m_pcMenuBar->AddItem( pcAppMenu );
	
	os::Menu* pcPlaylistMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_PLAYLIST, os::ITEMS_IN_COLUMN );
	pcPlaylistMenu->AddItem( MSG_MAINWND_MENU_PLAYLIST_SELECT, new os::Message( CF_GUI_SELECT_LIST ) );
	pcPlaylistMenu->AddItem( MSG_MAINWND_MENU_PLAYLIST_INPUT, new os::Message( CF_GUI_OPEN_INPUT ) );
	m_pcMenuBar->AddItem( pcPlaylistMenu );
	
	os::Menu* pcFileMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_FILE, os::ITEMS_IN_COLUMN );
	pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_ADD, new os::Message( CF_GUI_ADD_FILE ) );
	pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_REMOVE, new os::Message( CF_GUI_REMOVE_FILE ) );
	m_pcMenuBar->AddItem( pcFileMenu );
	
	os::Menu* pcViewMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_VIEW, os::ITEMS_IN_COLUMN );
	pcViewMenu->AddItem( MSG_MAINWND_MENU_VIEW_PLAYLIST, new os::Message( CF_GUI_VIEW_LIST ) );
	m_pcMenuBar->AddItem( pcViewMenu );
	
	cNewFrame = m_pcMenuBar->GetFrame();
	cNewFrame.right = GetBounds().Width();
	cNewFrame.bottom = cNewFrame.top + m_pcMenuBar->GetPreferredSize( false ).y;
	m_pcMenuBar->SetFrame( cNewFrame );
	m_pcMenuBar->SetTargetForItems( this );

	cNewFrame = GetBounds();
	cNewFrame.top += m_pcMenuBar->GetPreferredSize( false ).y + 1;
	cNewFrame.bottom = cNewFrame.top + 60;

	/* Create root view */
	m_pcRoot = new os::LayoutView( cNewFrame, "cf_root", NULL, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP | os::CF_FOLLOW_RIGHT );

	/* Create control view */
	m_pcControls = new os::HLayoutNode( "cf_controls" );
	m_pcControls->SetBorders( os::Rect( 2, 5, 2, 5 ) );

	/* Create buttons */
	m_pcPlay = new os::CImageButton( os::Rect( 0, 0, 1, 1 ), "cf_playorpause", MSG_MAINWND_PLAY, new os::Message( CF_GUI_PLAY ), NULL, os::ImageButton::IB_TEXT_BOTTOM, false, false, true );
	SetCButtonImageFromResource( m_pcPlay, "play.png" );

	m_pcStop = new os::CImageButton( os::Rect( 0, 0, 1, 1 ), "cf_stop", MSG_MAINWND_STOP, new os::Message( CF_GUI_STOP ), NULL, os::ImageButton::IB_TEXT_BOTTOM, false, false, true );
	SetCButtonImageFromResource( m_pcStop, "stop.png" );
	#if 0
	m_pcShowList = new os::CImageButton( os::Rect( 0, 0, 1, 1 ), "cf_show_list", MSG_MAINWND_PLAYLIST, new os::Message( CF_GUI_SHOW_LIST ), NULL, os::ImageButton::IB_TEXT_BOTTOM, false, false, true );
	SetCButtonImageFromResource( m_pcShowList, "list_hide.png" );
	#endif
	m_pcStop->SetEnable( false );

	os::VLayoutNode * pcCenter = new os::VLayoutNode( "cf_v_center" );

	/* Create LCD */
	m_pcLCD = new os::Lcd( os::Rect( 0, 0, 1, 1 ), new os::Message( CF_GUI_SEEK ) );
	
	m_pcLCD->SetTarget( this );
	m_pcLCD->SetEnable( false );
	pcCenter->AddChild( m_pcLCD );
	AddTimer(m_pcLCD,123,1000000,false);

	m_pcControls->AddChild( m_pcPlay );
	m_pcControls->AddChild( m_pcStop );
	m_pcControls->AddChild( pcCenter );
	#if 0
	m_pcControls->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcControls->AddChild( m_pcShowList );
	#endif

	m_pcControls->SameWidth( "cf_playorpause","cf_stop", NULL );

	m_pcRoot->SetRoot( m_pcControls );

	/* Create playlist */
	cNewFrame = GetBounds();
	cNewFrame.top = 61 + m_pcMenuBar->GetPreferredSize( false ).y + 1;
	m_pcPlaylist = new CFPlaylist( cNewFrame );
	m_pcPlaylist->SetInvokeMsg( new os::Message( CF_GUI_LIST_INVOKED ) );
	m_pcPlaylist->SetTarget( this );
	m_pcPlaylist->SetAutoSort( false );
	m_pcPlaylist->SetMultiSelect( false );
	m_pcPlaylist->InsertColumn( MSG_MAINWND_FILE.c_str(), (int)cNewFrame.Width() - 105 );
	m_pcPlaylist->InsertColumn( MSG_MAINWND_TRACK.c_str(), 50 );
	m_pcPlaylist->InsertColumn( MSG_MAINWND_LENGTH.c_str(), 55 );
	
	/* Create Visualization View */
	m_pcVisView = new os::View( cNewFrame, "vis_view", os::CF_FOLLOW_ALL );
	m_pcVisView->Show( false );
	
	AddChild( m_pcPlaylist );
	AddChild( m_pcVisView );
	AddChild( m_pcMenuBar );
	AddChild( m_pcRoot );
	
	
	
	/* Create file selector */
	m_pcFileDialog = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( os::Application::GetInstance() ), "", os::FileRequester::NODE_DIR, true );

	m_pcFileDialog->Lock();
	m_pcFileDialog->Start();
	m_pcFileDialog->Unlock();

	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
	
	SetSizeLimits( os::Point( 400,150 ), os::Point( 4096, 4096 ) );
}

CFWindow::~CFWindow()
{
	m_pcFileDialog->Close();
}

void CFWindow::HandleMessage( os::Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case CF_GUI_PLAY:
		if (m_nState == CF_STATE_STOPPED || m_nState == CF_STATE_PAUSED)
			os::Application::GetInstance()->PostMessage( CF_GUI_PLAY, os::Application::GetInstance(  ) );
		else if (m_nState == CF_STATE_PLAYING)
			os::Application::GetInstance()->PostMessage( CF_GUI_PAUSE, os::Application::GetInstance(  ) );
		break;
	case CF_GUI_PAUSE:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_PAUSE, os::Application::GetInstance(  ) );
		break;
	case CF_GUI_STOP:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_STOP, os::Application::GetInstance(  ) );
		break;
	case CF_GUI_SEEK:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_SEEK, os::Application::GetInstance(  ) );
		break;
	case CF_GUI_ADD_FILE:
		/* Open file selector */
		if( !m_pcFileDialog->IsVisible() )
			m_pcFileDialog->Show();
		m_pcFileDialog->MakeFocus( true );
		break;
	case CF_GUI_REMOVE_FILE:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_REMOVE_FILE, os::Application::GetInstance(  ) );
		break;
	case CF_GUI_SELECT_LIST:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_SELECT_LIST, os::Application::GetInstance(  ) );
		break;
	case CF_GUI_OPEN_INPUT:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_OPEN_INPUT, os::Application::GetInstance(  ) );
		break;
	case CF_GUI_QUIT:
		/* Quit */
		os::Application::GetInstance()->PostMessage( os::M_QUIT );
		break;
	case CF_GUI_ABOUT:
		{
			/* Show about alert */
			os::String cBodyText;
			
			cBodyText = os::String( "ColdFish V1.2.3\n" ) + MSG_ABOUTWND_TEXT;
			
			os::Alert* pcAbout = new os::Alert( MSG_ABOUTWND_TITLE, cBodyText, os::Alert::ALERT_INFO, 
											os::WND_NOT_RESIZABLE, MSG_ABOUTWND_OK.c_str(), NULL );
			pcAbout->Go( new os::Invoker( 0 ) ); 
		}
		break;
	case CF_GUI_SHOW_LIST:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_SHOW_LIST, os::Application::GetInstance(  ) );
		break;
	case CF_GUI_VIEW_LIST:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_VIEW_LIST, os::Application::GetInstance(  ) );
		break;
	case CF_GUI_LIST_INVOKED:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_LIST_INVOKED, os::Application::GetInstance(  ) );
		break;
	case CF_STATE_CHANGED:
		{
			/* Message sent by the CFApp class */
			if ( m_nState == CF_STATE_STOPPED )
			{
				SetCButtonImageFromResource( m_pcPlay, "play.png" );
				
				m_pcPlay->Paint( m_pcPlay->GetBounds() );
				m_pcStop->SetEnable( false );
				m_pcLCD->SetEnable( false );
			}
			else if ( m_nState == CF_STATE_PLAYING )
			{
				SetCButtonImageFromResource( m_pcPlay, "pause.png" );
				m_pcPlay->Paint( m_pcPlay->GetBounds() );
				m_pcStop->SetEnable( true );
				m_pcLCD->SetEnable( true );
			}
			else if ( m_nState == CF_STATE_PAUSED )
			{
				SetCButtonImageFromResource( m_pcPlay, "play.png" );
				m_pcPlay->Paint( m_pcPlay->GetBounds() );
				m_pcStop->SetEnable( true );
				m_pcLCD->SetEnable( false );
			}
			Sync();
		}
		break;

	default:
		os::Window::HandleMessage( pcMessage );
		break;
	}
}

bool CFWindow::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return ( false );
}

void CFWindow::SetList( bool bEnabled )
{
	if( bEnabled )
		SetCButtonImageFromResource( m_pcShowList, "list_hide.png" );
	else
		SetCButtonImageFromResource( m_pcShowList, "list_show.png" );
	m_pcShowList->Invalidate();
	m_pcShowList->Flush();
}

/* CFApp class */
CFApp::CFApp( ):os::Looper( "cf_app" )
{
}

void CFApp::Start( os::String zFileName, bool bLoad )
{
	SetPublic(true);
	
	
	/* Set default values */
	m_nState = CF_STATE_STOPPED;

	m_zListName = os::String ( getenv( "HOME" ) ) + "/" + MSG_PLAYLIST_DEFAULT + ".plst";
	m_pcInput = NULL;
	m_bLockedInput = false;
	m_nPlaylistPosition = 0;
	m_zAudioFile = "";
	m_nAudioTrack = 0;
	m_nAudioStream = 0;
	m_pcAudioCodec = NULL;
	m_pcAudioOutput = NULL;
	m_bPlayThread = false;
	m_pcCurrentVisPlugin = NULL;
	m_bListShown = true;
	m_zTrackName = "Unknown";
	m_pcInputSelector = NULL;
	//m_bIsPlaying = false;

	/* Load settings */
	os::Settings * pcSettings = new os::Settings();
	if ( pcSettings->Load() == 0 )
	{
		m_zListName = pcSettings->GetString( "playlist", m_zListName.c_str() );
	}	
	delete( pcSettings );

	/* Create registrar manager */
	m_pcRegManager = NULL;
	try
	{
		m_pcRegManager = os::RegistrarManager::Get();
		
		/* Register playlist type */
		m_pcRegManager->RegisterType( "application/x-coldfish-playlist", MSG_MIMETYPE_APPLICATION_XCOLDFISHPLAYLIST );
		m_pcRegManager->RegisterTypeIconFromRes( "application/x-coldfish-playlist", "application_coldfish_playlist.png" );
		m_pcRegManager->RegisterTypeExtension( "application/x-coldfish-playlist", "plst" );
		m_pcRegManager->RegisterAsTypeHandler( "application/x-coldfish-playlist" );
	}
	catch(...)
	{
	}

	/* Create media manager */
	m_pcManager = os::MediaManager::Get();
	if ( !m_pcManager->IsValid() )
	{
		std::cout << "Media server is not running" << std::endl;
		os::Alert* pcAlert = new os::Alert( MSG_ALERTWND_NOMEDIASERVER_TITLE, MSG_ALERTWND_NOMEDIASERVER_TEXT, Alert::ALERT_WARNING,0, MSG_ALERTWND_NOMEDIASERVER_OK.c_str(),NULL);
		pcAlert->Go();
		pcAlert->MakeFocus();
		PostMessage( os::M_QUIT );
		return;
	}

	/* Create window */
	m_pcWin = new CFWindow( os::Rect( 0, 0, 500, 350 ), "cf_window", MSG_PLAYLIST_DEFAULT + ".plst - ColdFish",0);
	m_pcWin->CenterInScreen();
	m_pcWin->Show();
	m_pcWin->MakeFocus( true );
	
	/* Load Plugins */
	LoadPlugins();

	/* Open list */
	if ( bLoad )
	{
		if ( !OpenList( zFileName ) )
			OpenList( m_zListName );
	}
	else
	{
		if ( !OpenList( m_zListName ) )
		{
			OpenList( os::String ( getenv( "HOME" ) ) + "/" + MSG_PLAYLIST_DEFAULT + ".plst" );
		}
	}	
}

CFApp::~CFApp()
{	
	
	/* Close and delete everything */
	if (m_pcRegManager)
	{
		m_pcRegManager->Put();
	}
	
	CloseCurrentFile();
	
	m_pcWin->Terminate();
	for( uint i = 0; i < m_cPlugins.size(); i++ )
	{
		delete( m_cPlugins[i].pi_pcPlugin );
	}
	if ( m_pcManager->IsValid() )
	{
		m_pcManager->Put();
	}


}

	

CFWindow* CFApp::GetWindow()
{
	return( m_pcWin );
}

/* Load plugins */
typedef ColdFishPluginEntry* init_coldfish_plugin();

void CFApp::LoadPlugins()
{
	os::String zFileName;
//	os::String zPath;
//	This is not relocatable, but works from the live CD:
	os::String zPath = os::String( "/Applications/Media/Plugins" );

	os::Directory *pcDirectory = new os::Directory();
//	if( pcDirectory->SetTo( "^/Plugins" ) != 0 )
	if( pcDirectory->SetTo( zPath.c_str() ) != 0 )
		return;
	
//	pcDirectory->GetPath( &zPath );
	m_cPlugins.clear();
	
	if (DEBUG)
		std::cout<<"Start plugin scan.."<<std::endl;
	
	while( pcDirectory->GetNextEntry( &zFileName ) )
	{
		/* Load image */
		if( zFileName == "." || zFileName == ".." )
			continue;
		zFileName = zPath + os::String( "/" ) + zFileName;
		
		image_id nID = load_library( zFileName.c_str(), 0 );
		if( nID >= 0 ) {
			init_coldfish_plugin *pInit;
			/* Call init_coldfish_addon() */
			if( get_symbol_address( nID, "init_coldfish_plugin",
			-1, (void**)&pInit ) == 0 ) {
				ColdFishPluginEntry* pcPluginEntry = pInit();
				for( uint i = 0; i < pcPluginEntry->GetPluginCount(); i++ )
				{
					ColdFishPlugin* pcPlugin = pcPluginEntry->GetPlugin( i );

					pcPlugin->SetApp( this, GetWindow()->GetMenuBar() );
					if( pcPlugin ) {
						if( pcPlugin->Initialize() != 0 )
						{
							std::cout<<pcPlugin->GetIdentifier().c_str()<<" failed to initialize"<<std::endl;
						} else {
							m_cPlugins.push_back( CFPluginItem( nID, pcPlugin ) );
						}
					}
				}
				delete( pcPluginEntry );
			} else {
				if (DEBUG)
					std::cout<<zFileName.c_str()<<" does not export init_coldfish_plugin()"<<std::endl;
			}
			
		}
	}
}

/* Activate the current visualization plugin and set all the neccessary information */
void CFApp::ActivateVisPlugin()
{
	if( m_pcCurrentVisPlugin )
	{
		UpdatePluginPlaylist();
		m_pcCurrentVisPlugin->PlaylistPositionChanged( m_nPlaylistPosition );
		m_pcCurrentVisPlugin->FileChanged( m_zAudioFile );
		m_pcCurrentVisPlugin->NameChanged( m_zAudioName );
		m_pcCurrentVisPlugin->TrackChanged( m_nAudioTrack + 1 );
		m_pcCurrentVisPlugin->TimeChanged( m_nLastPosition );
		m_pcCurrentVisPlugin->Activated();
	}
}

/* Deactivate the current visualization plugin */
void CFApp::DeactivateVisPlugin()
{
	if( m_pcCurrentVisPlugin )
	{
		m_pcCurrentVisPlugin->Deactivated();
	}
}

/* Update the pluging playlist */
void CFApp::UpdatePluginPlaylist()
{
	if( m_pcCurrentVisPlugin )
	{
		m_pcCurrentVisPlugin->GetPlaylist()->clear();
		/* Add files */
		for ( uint i = 0; i < m_pcWin->GetPlaylist()->GetRowCount(  ); i++ )
		{
			CFListItem * pcRow = ( CFListItem * ) m_pcWin->GetPlaylist()->GetRow( i );
			m_pcCurrentVisPlugin->GetPlaylist()->push_back( pcRow->zPath );
		}
		m_pcCurrentVisPlugin->PlaylistChanged();
	}
}

/* Thread which is responsible to play the file */
void CFApp::PlayThread()
{
	bigtime_t nTime = get_system_time();
	bigtime_t nNextAnimationTime = get_system_time();
	
	m_bPlayThread = true;
	
	os::MediaPacket_s sPacket;
	os::MediaPacket_s sCurrentAudioPacket;
	std::queue<os::MediaPacket_s> cAudioPackets;
	uint64 nAudioPacketPosition = 0;
	uint64 nAudioBytes = 0;
	uint8 nErrorCount = 0;
	bool bError = false;
	int16 nAnimationBuffer[2][512];
	uint64 nAnBufferPosition = 0;
	
	if (DEBUG)
		std::cout << "Play thread running" << std::endl;
	
	/* Seek to last position */
	if ( !m_bStream )
		m_pcInput->Seek( m_nLastPosition );
	/* Create audio output packet */
	if ( m_bPacket )
	{
		m_pcAudioCodec->CreateAudioOutputPacket( &sCurrentAudioPacket );
	}
	while ( m_bPlayThread )
	{
		if ( m_bPacket )
		{
			/* Look if we have to grab data */
			//printf("%i %i\n", (int)m_pcAudioOutput->GetDelay(), (int)m_pcAudioOutput->GetBufferSize() );
			
			if ( m_pcAudioOutput->GetDelay() < m_pcAudioOutput->GetBufferSize() )
			{
				/* Grab data */
				if ( m_pcInput->ReadPacket( &sPacket ) == 0 )
				{
					nErrorCount = 0;
					/* Decode audio data */
					if ( sPacket.nStream == m_nAudioStream )
					{
						if ( m_pcAudioCodec->DecodePacket( &sPacket, &sCurrentAudioPacket ) == 0 )
						{
							if ( sCurrentAudioPacket.nSize[0] > 0 )
							{
								sCurrentAudioPacket.nTimeStamp = ~0;
								m_pcAudioOutput->WritePacket( 0, &sCurrentAudioPacket );
								nAudioBytes += sCurrentAudioPacket.nSize[0];
								/* Put the packet in the queue and allocate a new one */
								if( cAudioPackets.size() < 20 )
								{
									cAudioPackets.push( sCurrentAudioPacket );
									m_pcAudioCodec->CreateAudioOutputPacket( &sCurrentAudioPacket );
								}
							}
						}
					}
					m_pcInput->FreePacket( &sPacket );
				}
				else
				{
					/* Increase error count */
					nErrorCount++;
					if ( m_pcAudioOutput->GetDelay( true ) > 0 )
						nErrorCount--;
					if ( nErrorCount > 10 && !bError )
					{
						os::Application::GetInstance()->PostMessage( CF_PLAY_NEXT, os::Application::GetInstance(  ) );
						bError = true;
					}
				}

			}
		}
		
		snooze( 1000 );
		

		/* Build animation buffer and pass it to the plugin */
		if( m_bPacket && get_system_time() > nNextAnimationTime && m_bPlayThread )
		{
			while( ( nAnBufferPosition < 512 ) && !cAudioPackets.empty() )
			{
				
				os::MediaPacket_s sTopPacket = cAudioPackets.front();
				int nCopySamples = std::min( 512 - nAnBufferPosition, ( sTopPacket.nSize[0] - nAudioPacketPosition ) / 2 / m_sAudioFormat.nChannels );
					
				
				for( int i = 0; i < nCopySamples; i++ )
				{
					nAnimationBuffer[0][nAnBufferPosition+i] = *( (int16*)(&sTopPacket.pBuffer[0][nAudioPacketPosition] ) );
					nAudioPacketPosition += m_sAudioFormat.nChannels * 2;
				}	
				
				nAnBufferPosition += nCopySamples;
				//printf( "%i %i %i\n", nCopySamples, (int)nAudioPacketPosition, nAnBufferPosition );
				//printf( "%i %i\n", sTopPacket.nSize[0], nAudioPacketPosition );
				
				if( sTopPacket.nSize[0] <= nAudioPacketPosition )
				{
					//printf( "Release %i %i\n", cAudioPackets.size(), (int)( 512 * 1000000 / m_sAudioFormat.nSampleRate ) );
					m_pcAudioCodec->DeleteAudioOutputPacket( &sTopPacket );
					cAudioPackets.pop();
					nAudioPacketPosition = 0;
				}	
			}
			
			if( nAnBufferPosition == 512 && m_bPlayThread )
			{
				GetWindow()->Lock();
				if( m_pcCurrentVisPlugin )
				{					
					m_pcCurrentVisPlugin->AudioData( m_pcWin->GetVisView(), nAnimationBuffer );
				}
				GetWindow()->Unlock();
				nAnBufferPosition = 0;
				nNextAnimationTime += ( 512 * 1000000 / m_sAudioFormat.nSampleRate );
			}			
		}
		if ( !m_bStream && get_system_time() > nTime + 1000000 )
		{
			/* Move slider */
			m_pcWin->GetLCD()->SetValue( os::Variant( ( int )( m_pcInput->GetCurrentPosition(  ) * 1000 / m_pcInput->GetLength(  ) ) ), false );
			m_pcWin->GetLCD()->UpdateTime( m_pcInput->GetCurrentPosition(  ) );
			
			if( m_pcCurrentVisPlugin )
				m_pcCurrentVisPlugin->TimeChanged( m_pcInput->GetCurrentPosition(  ) );
			
			//cout<<"Position "<<m_pcInput->GetCurrentPosition()<<endl;
			nTime = get_system_time();
			/* For non packet based devices check if we have finished */
			if ( !m_bPacket && m_nLastPosition == m_pcInput->GetCurrentPosition() )
			{
				nErrorCount++;
				if ( nErrorCount > 2 && !bError )
				{
					os::Application::GetInstance()->PostMessage( CF_PLAY_NEXT, os::Application::GetInstance(  ) );
					bError = true;
				}
			}
			else
				nErrorCount = 0;

			m_nLastPosition = m_pcInput->GetCurrentPosition();
		}
	}
	/* Stop tread */
	if (DEBUG)
		std::cout << "Stop thread" << std::endl;
	
	if ( !m_bPacket )
	{
		m_pcInput->StopTrack();
	}
	if ( m_bPacket )
	{
		m_pcAudioOutput->Clear();
		/* Clear packets */
		while( !cAudioPackets.empty() )
		{
			os::MediaPacket_s sPacket = cAudioPackets.front();
			m_pcAudioCodec->DeleteAudioOutputPacket( &sPacket );
			cAudioPackets.pop();
		}
		m_pcAudioCodec->DeleteAudioOutputPacket( &sCurrentAudioPacket );
	}
}

int play_thread_entry( void *pData )
{
	CFApp *pcApp = ( CFApp * ) pData;

	pcApp->PlayThread();
	return ( 0 );
}

/* Open one playlist */
bool CFApp::OpenList( os::String zFileName )
{
	char zTemp[PATH_MAX];
	char zTemp2[PATH_MAX];
	
	m_bLockedInput = false;
	m_pcWin->GetPlaylist()->Clear(  );
	m_pcWin->GetLCD()->SetValue( os::Variant( 0 ) );
	m_pcWin->GetLCD()->UpdateTime( 0 );
	m_pcWin->GetLCD()->SetTrackName( MSG_PLAYLIST_UNKNOWN );
	m_pcWin->GetLCD()->SetTrackNumber( 0 );
	std::ifstream hIn;

	hIn.open( zFileName.c_str() );
	if ( !hIn.is_open() )
	{
		if (DEBUG)
			std::cout << "Could not open playlist!" << std::endl;
		return ( false );
	}
	/* Read header */
	if ( hIn.getline( zTemp, 255, '\n' ) < 0 )
		return ( false );

	if ( strcmp( zTemp, "<PLAYLIST-V1>" ) )
	{
		if (DEBUG)
			std::cout << "Invalid playlist!" << std::endl;
		return ( false );
	}

	/* Read entries */
	bool bInEntry = false;
	os::String zFile;
	int nTrack = 0;
	int nStream = 0;
	os::String zAuthor;
	os::String zTitle;
	os::String zAlbum;
	int64 nLength = 0;	
	
	m_pcWin->Lock();
	m_pcWin->SetTitle( "Loading... - ColdFish" );
	m_pcWin->Unlock();

	while ( !hIn.eof() )
	{
		if ( hIn.getline( zTemp, 255, '\n' ) < 0 )
			return ( false );
		if ( !strcmp( zTemp, "<END>" ) )
		{
			goto finished;
		}
		if ( !strcmp( zTemp, "<ENTRYSTART>" ) )
		{
			bInEntry = true;
			zFile = "";
			nTrack = 0;
			nStream = 0;
			zAuthor = "Unknown";
			zTitle = "Unknown";
			zAlbum = "Unknown";
			nLength = -1;
			continue;
		}
		if ( !strcmp( zTemp, "<ENTRYEND>" ) )
		{
			uint32 nM, nS;


			bInEntry = false;
			
	
			/* Try to read the length and title from the attributes */				
			try
			{
				os::FSNode cNode( zFile );
				if( cNode.ReadAttr( "Media::Length", ATTR_TYPE_INT64, &nLength, 0, sizeof( int64 ) ) == sizeof( int64 ) )
				{
				} 
				char zTemp[PATH_MAX];
				memset( zTemp, 0, PATH_MAX );
				if( cNode.ReadAttr( "Media::Author", ATTR_TYPE_STRING, zTemp, 0, PATH_MAX ) >= 0 )
				{
					zAuthor = zTemp;
				}
				memset( zTemp, 0, PATH_MAX );				
				if( cNode.ReadAttr( "Media::Title", ATTR_TYPE_STRING, zTemp, 0, PATH_MAX ) >= 0 )
				{
					zTitle = zTemp;
				}
				memset( zTemp, 0, PATH_MAX );				
				if( cNode.ReadAttr( "Media::Album", ATTR_TYPE_STRING, zTemp, 0, PATH_MAX ) >= 0 )
				{
					zAlbum = zTemp;
				}
				cNode.Unset();
			} catch(...)
			{
			}


			/* Read length from input if required */
			if( zAuthor == "Unknown" || 
				zTitle == "Unknown" || 
				zAlbum == "Unknown" || 
				nLength == -1 )
			{
				os::MediaInput * pcInput = m_pcManager->GetBestInput( zFile );
				if ( pcInput == NULL )
					continue;
				pcInput->Open( zFile );
				pcInput->SelectTrack( nTrack );
				nLength = pcInput->GetLength();
				zAuthor = pcInput->GetAuthor();
				zTitle = pcInput->GetTitle();
				zAlbum = pcInput->GetAlbum();
				nLength = pcInput->GetLength();
				try {
					os::FSNode cNode( zFile );
					cNode.WriteAttr( "Media::Length", O_TRUNC, ATTR_TYPE_INT64, &nLength, 0, sizeof( int64 ) );
					cNode.WriteAttr( "Media::Author", O_TRUNC, ATTR_TYPE_STRING, zAuthor.c_str(), 0, zAuthor.Length() );
					cNode.WriteAttr( "Media::Title", O_TRUNC, ATTR_TYPE_STRING, zTitle.c_str(), 0, zTitle.Length() );
					cNode.WriteAttr( "Media::Album", O_TRUNC, ATTR_TYPE_STRING, zAlbum.c_str(), 0, zAlbum.Length() );					
					cNode.Unset();
				} catch( ... ) {
				}
				pcInput->Release();				
			}
			
			secs_to_ms( nLength, &nM, &nS );
		
			/* Add new row */
			CFListItem * pcRow = new CFListItem();

			pcRow->zPath = zFile;
			pcRow->zAuthor = zAuthor;
			pcRow->zTitle = zTitle;
			pcRow->zAlbum = zAlbum;
			
			pcRow->AppendString( pcRow->BuildTitle() );
			
			sprintf( zTemp, "%i", ( int )nTrack + 1 );
			pcRow->AppendString( zTemp );
			pcRow->nTrack = nTrack;
			pcRow->nStream = nStream;
			pcRow->nLength = nLength;
			
			sprintf( zTemp, "%.2u:%.2u", nM, nS );
			pcRow->AppendString( zTemp );
			m_pcWin->Lock();
			m_pcWin->GetPlaylist()->InsertRow( pcRow );
			
			if ( m_pcWin->GetPlaylist()->GetRowCount(  ) == 1 )
				m_pcWin->GetPlaylist()->Select( 0, 0 );
			m_pcWin->Unlock();	
			
			continue;
		}
		/* We expect a second line with data */
		if ( hIn.getline( zTemp2, 255, '\n' ) < 0 )
			return ( false );
		if ( !strcmp( zTemp, "<FILE>" ) )
		{
			zFile = zTemp2;
		}
		else if ( !strcmp( zTemp, "<TRACK>" ) )
			nTrack = atoi( zTemp2 ) - 1;
		else if ( !strcmp( zTemp, "<STREAM>" ) )
			nStream = atoi( zTemp2 ) - 1;
		else if ( !strcmp( zTemp, "<LENGTH>" ) )
			nLength = atoi( zTemp2 );
		else if ( !strcmp( zTemp, "<AUTHOR>" ) )
			zAuthor = zTemp2;
		else if ( !strcmp( zTemp, "<TITLE>" ) )
			zTitle = zTemp2;
		else if ( !strcmp( zTemp, "<ALBUM>" ) )
			zAlbum = zTemp2;
	}
	m_pcWin->Lock();
	m_pcWin->SetTitle( "ColdFish" );
	m_pcWin->Unlock();
	return ( false );
	finished:
	hIn.close();
	m_zListName = zFileName;
	
	/* Set title */
	os::Path cPath( zFileName.c_str() );
	m_pcWin->Lock();
	m_pcWin->SetTitle( os::String ( cPath.GetLeaf() ) + " - ColdFish" );
	m_pcWin->Unlock();
	
	if (DEBUG)
		std::cout << "List openened" << std::endl;
	UpdatePluginPlaylist();
	return ( true );

}

/* Save the opened playlist */
void CFApp::SaveList()
{
	uint32 i;
	
		
	if( m_bLockedInput || m_zListName == "" )
		return;
		
	/* Open output file */
	std::ofstream hOut;

	hOut.open( m_zListName.c_str() );
	if ( !hOut.is_open() )
	{
		std::cout << "Could not save playlist!" << std::endl;
		return;
	}


	hOut << "<PLAYLIST-V1>" << std::endl;

	/* Save files */
	for ( i = 0; i < m_pcWin->GetPlaylist()->GetRowCount(  ); i++ )
	{
		char zTemp[100];
		hOut << "<ENTRYSTART>" << std::endl;
		CFListItem * pcRow = ( CFListItem * ) m_pcWin->GetPlaylist()->GetRow( i );
		hOut << "<FILE>" << std::endl;
		hOut << pcRow->zPath.c_str() << std::endl;
		hOut << "<TRACK>" << std::endl;
		sprintf( zTemp, "%i", pcRow->nTrack + 1 );
		hOut << zTemp << std::endl;
		hOut << "<STREAM>" << std::endl;
		sprintf( zTemp, "%i", pcRow->nStream + 1 );
		hOut << zTemp << std::endl;
		hOut << "<LENGTH>" << std::endl;
		hOut << pcRow->nLength << std::endl;
		hOut << "<AUTHOR>" << std::endl;
		hOut << pcRow->zAuthor.c_str() << std::endl;
		hOut << "<TITLE>" << std::endl;
		hOut << pcRow->zTitle.c_str() << std::endl;
		hOut << "<ALBUM>" << std::endl;
		hOut << pcRow->zAlbum.c_str() << std::endl;		
		hOut << "<ENTRYEND>" << std::endl;
	}
	hOut << "<END>" << std::endl;

	hOut.close();
	

}

/* Open an input */
void CFApp::OpenInput( os::String zFileName, os::String zInput )
{
	uint32 i = 0;
	char zTemp[255];
	m_bLockedInput = true;
	m_pcWin->GetPlaylist()->Clear(  );
	m_pcWin->GetLCD()->SetValue( os::Variant( 0 ) );
	m_pcWin->GetLCD()->UpdateTime( 0 );
	m_pcWin->GetLCD()->SetTrackName( MSG_PLAYLIST_UNKNOWN );
	m_pcWin->GetLCD()->SetTrackNumber( 0 );
	m_zListName = zInput;
	
	if( zFileName.empty() )
		zFileName = zInput;
	
	if (DEBUG)
		std::cout << "Open input " << zInput.c_str() << std::endl;
	
	while ( ( m_pcInput = m_pcManager->GetInput( i ) ) != NULL )
	{
		if ( m_pcInput->GetIdentifier() == zInput )
		{
			break;
		}
		m_pcInput->Release();
		m_pcInput = NULL;
		i++;
	}
	
	if ( m_pcInput == NULL )
		goto invalid;

	if ( m_pcInput->Open( zFileName ) != 0 )
		goto invalid;

	if ( m_pcInput->GetTrackCount() < 1 )
		goto invalid;

	/* Packet based ? */
	if ( !m_pcInput->PacketBased() )
		goto invalid;
		
	/* Search tracks with audio streams and add them to the list */

	
	for ( i = 0; i < m_pcInput->GetTrackCount(); i++ )
	{
		m_pcInput->SelectTrack( i );

		for ( uint32 j = 0; j < m_pcInput->GetStreamCount(); j++ )
		{
			if ( m_pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
			{
				uint32 nM, nS;

				/* Found something -> add it */
				CFListItem * pcRow = new CFListItem();
				
				pcRow->AppendString( os::Path( zFileName ).GetLeaf() );
				pcRow->zPath = zFileName;
				sprintf( zTemp, "%i", ( int )i + 1 );
				pcRow->AppendString( zTemp );
				pcRow->nTrack = i;
				pcRow->nStream = j;
				secs_to_ms( m_pcInput->GetLength(), &nM, &nS );
				sprintf( zTemp, "%.2u:%.2u", nM, nS );
				pcRow->AppendString( zTemp );
				m_pcWin->Lock();
				m_pcWin->GetPlaylist()->InsertRow( pcRow );

				if ( m_pcWin->GetPlaylist()->GetRowCount(  ) == 1 )
					m_pcWin->GetPlaylist()->Select( 0, 0 );
				m_pcWin->Unlock();
			}
		}
	}
	UpdatePluginPlaylist();
	
	m_pcWin->Lock();
	m_pcWin->SetTitle( m_zListName + " - ColdFish" );
	m_pcWin->Unlock();
	
	return;
      invalid:
	if ( m_pcInput )
	{
		m_pcInput->Close();
		m_pcInput->Release();
		m_pcInput = NULL;
	}
	os::Alert * pcAlert = new os::Alert( MSG_ERRWND_TITLE, MSG_ERRWND_CANTPLAY, os::Alert::ALERT_WARNING, 0, MSG_ERRWND_OK.c_str(), NULL );
	pcAlert->Go( new os::Invoker( 0 ) );
}

/* Check if this is a valid file */
void CFApp::AddFile( os::String zFileName )
{

	uint32 i;
	char zTemp[255];

	if(DEBUG)
		std::cout << "Add File " << zFileName.c_str() << std::endl;

	os::MediaInput * pcInput = m_pcManager->GetBestInput( zFileName );
	if ( pcInput == NULL )
		goto invalid;

	if ( pcInput->Open( zFileName ) != 0 )
		goto invalid;

	if ( pcInput->GetTrackCount() < 1 )
		goto invalid;

	/* Packet based ? */
	if ( !pcInput->PacketBased() )
		goto invalid;

	/* Search tracks with audio streams and add them to the list */


	for ( i = 0; i < pcInput->GetTrackCount(); i++ )
	{
		pcInput->SelectTrack( i );

		for ( uint32 j = 0; j < pcInput->GetStreamCount(); j++ )
		{
			if ( pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
			{
				uint32 nM, nS;

				/* Found something -> add it */
				CFListItem * pcRow = new CFListItem();

				pcRow->zPath = zFileName;				
				pcRow->zAuthor = pcInput->GetAuthor();
				pcRow->zTitle = pcInput->GetTitle();
				pcRow->zAlbum = pcInput->GetAlbum();
				pcRow->nLength = pcInput->GetLength();

				pcRow->AppendString( pcRow->BuildTitle() );

				sprintf( zTemp, "%i", ( int )i + 1 );
				pcRow->AppendString( zTemp );
				pcRow->nTrack = i;
				pcRow->nStream = j;
				secs_to_ms( pcInput->GetLength(), &nM, &nS );
				sprintf( zTemp, "%.2u:%.2u", nM, nS );
				pcRow->AppendString( zTemp );
				m_pcWin->Lock();
				m_pcWin->GetPlaylist()->InsertRow( pcRow );

				if ( m_pcWin->GetPlaylist()->GetRowCount(  ) == 1 )
					m_pcWin->GetPlaylist()->Select( 0, 0 );
				m_pcWin->Unlock();
			}
		}
	}
	
	if( pcInput->GetTrackCount() == 1 )
	try {
		os::FSNode cNode( zFileName );
		uint64 nLength = pcInput->GetLength();
		cNode.WriteAttr( "Media::Length", O_TRUNC, ATTR_TYPE_INT64, &nLength, 0, sizeof( int64 ) );
		cNode.WriteAttr( "Media::Author", O_TRUNC, ATTR_TYPE_STRING, pcInput->GetAuthor().c_str(), 0, pcInput->GetAuthor().Length() );
		cNode.WriteAttr( "Media::Title", O_TRUNC, ATTR_TYPE_STRING, pcInput->GetTitle().c_str(), 0, pcInput->GetTitle().Length() );
		cNode.WriteAttr( "Media::Album", O_TRUNC, ATTR_TYPE_STRING, pcInput->GetAlbum().c_str(), 0, pcInput->GetAlbum().Length() );
		
		cNode.Unset();
	} catch( ... ) {
	}
	
	pcInput->Close();
	pcInput->Release();

	SaveList();
	UpdatePluginPlaylist();
	return;
      invalid:
	if ( pcInput )
	{
		pcInput->Close();
		pcInput->Release();
	}
	os::Alert * pcAlert = new os::Alert( MSG_ERRWND_TITLE, MSG_ERRWND_CANTPLAY, os::Alert::ALERT_WARNING, 0, MSG_ERRWND_OK.c_str(), NULL );
	pcAlert->Go( new os::Invoker( 0 ) );
}

/* Play one track of a file / device */
int CFApp::OpenFile( os::String zFileName, uint32 nTrack, uint32 nStream, os::String zTitle )
{
	/* Close */
	CloseCurrentFile();
	uint32 i = 0;
	
	if( !m_bLockedInput )
	{
		m_pcInput = m_pcManager->GetBestInput( zFileName );
		if ( m_pcInput == NULL )
		{
			if (DEBUG)
				std::cout << "Cannot get input!" << std::endl;
			return ( -1 );
		}
		/* Open input */
		if ( m_pcInput->Open( zFileName ) != 0 )
		{
			if (DEBUG)
				std::cout << "Cannot open input!" << std::endl;
			return ( -1 );
		}
	} else {
		if ( m_pcInput == NULL )
		{
			if (DEBUG)
				std::cout << "Cannot get input!" << std::endl;
			return ( -1 );
		}
	}
	
	m_bPacket = m_pcInput->PacketBased();
	m_bStream = m_pcInput->StreamBased();

	/* Look if this is a packet based file / device */
	if ( !m_bPacket )
	{
		if (DEBUG)
			std::cout << "Not packet based!" << std::endl;
		CloseCurrentFile();
		return ( -1 );
	}

	/* Select track */
	if ( m_pcInput->SelectTrack( nTrack ) != nTrack )
	{
		if (DEBUG)
			std::cout << "Track selection failed!" << std::endl;
		CloseCurrentFile();
		return ( -1 );
	}
	
	/* Look if the stream is valid */
	if ( nStream >= m_pcInput->GetStreamCount() )
	{
		if (DEBUG)
			std::cout << "Invalid stream number!" << std::endl;
		CloseCurrentFile();
		return ( -1 );
	}

	/* Check stream */
	if ( m_pcInput->GetStreamFormat( nStream ).nType != os::MEDIA_TYPE_AUDIO )
	{
		if(DEBUG)
			std::cout << "Invalid stream format!" << std::endl;
		CloseCurrentFile();
		return ( -1 );
	}
	
	m_sAudioFormat = m_pcInput->GetStreamFormat( nStream );


	/* Open audio output */
	m_pcAudioOutput = m_pcManager->GetDefaultAudioOutput();
	if ( m_pcAudioOutput == NULL || ( m_pcAudioOutput && m_pcAudioOutput->FileNameRequired() ) || ( m_pcAudioOutput && m_pcAudioOutput->Open( "" ) != 0 ) )
	{
		if (DEBUG)
			std::cout << "Cannot open audio output!" << std::endl;
		CloseCurrentFile();
		return ( -1 );
	}
	
	/* Connect audio output with the codec */
	for ( i = 0; i < m_pcAudioOutput->GetOutputFormatCount(); i++ )
	{
		if ( ( m_pcAudioCodec = m_pcManager->GetBestCodec( m_sAudioFormat, m_pcAudioOutput->GetOutputFormat( i ), false ) ) != NULL )
			if ( m_pcAudioCodec->Open( m_sAudioFormat, m_pcAudioOutput->GetOutputFormat( i ), false ) == 0 )
				break;
			else
			{
				m_pcAudioCodec->Release();
				m_pcAudioCodec = NULL;
			}
	}
	if ( m_pcAudioCodec == NULL || m_pcAudioOutput->AddStream( os::String ( os::Path( zFileName.c_str() ).GetLeaf(  ) ), m_pcAudioCodec->GetExternalFormat(  ) ) != 0 )
	{
		if(DEBUG)
			std::cout << "Cannot open audio codec!" << std::endl;
		CloseCurrentFile();
		return ( -1 );
	}
	else
	{
		if(DEBUG)
			std::cout << "Using Audio codec " << m_pcAudioCodec->GetIdentifier().c_str(  ) << std::endl;
	}

	/* Construct name */
	os::Path cPath = os::Path( zFileName.c_str() );

	/* Set title */
	
	if ( m_pcInput->FileNameRequired() )
		m_zAudioName = zTitle;
	else
		m_zAudioName = m_pcInput->GetIdentifier();
	
	m_pcWin->SetTitle( os::String ( os::Path( m_zListName.c_str() ).GetLeaf(  ) ) + " - ColdFish (" + MSG_MAINWND_PLAYING + " " + m_zAudioName + ")" );

	/* Save information */
	m_nAudioTrack = nTrack;
	m_nAudioStream = nStream;
	m_zAudioFile = zFileName;
	
	ActivateVisPlugin();

	/* Set LCD */

	m_pcWin->GetLCD()->SetTrackName( m_zAudioName );
	m_pcWin->GetLCD()->SetTrackNumber( m_nAudioTrack + 1 );
	m_pcWin->GetLCD()->UpdateTime( 0 );
	m_pcWin->GetLCD()->SetValue( os::Variant( 0 ) );

	m_zTrackName = cPath.GetLeaf();
	return ( 0 );
}

/* Close the currently opened file */
void CFApp::CloseCurrentFile()
{
	/* Stop thread */
	if ( m_bPlayThread )
	{
		m_bPlayThread = false;
		wait_for_thread( m_hPlayThread );
	}
	if ( m_pcAudioOutput )
	{
		m_pcAudioOutput->Close();
		m_pcAudioOutput->Release();
		m_pcAudioOutput = NULL;
	}

	if ( m_pcAudioCodec )
	{
		m_pcAudioCodec->Close();
		m_pcAudioCodec->Release();
		m_pcAudioCodec = NULL;
	}

	if ( m_pcInput && !m_bLockedInput )
	{
		m_pcInput->Close();
		m_pcInput->Release();
		m_pcInput = NULL;
	}
	m_pcWin->SetTitle( os::String ( os::Path( m_zListName.c_str() ).GetLeaf(  ) ) + " - ColdFish" );

	m_zAudioFile = "";
}

/* Set the current state */
void CFApp::SetState( uint8 nState )
{
	m_nState = nState;
	m_pcWin->SetState( nState );
	m_pcWin->PostMessage( CF_STATE_CHANGED, m_pcWin );
	if( m_pcCurrentVisPlugin )
		m_pcCurrentVisPlugin->StateChanged( nState );
}

/* Switch to the next track ( called as a thread to avoid sound errors ) */
void CFApp::PlayNext()
{
	uint nSelected = m_pcWin->GetPlaylist()->GetFirstSelected(  );

	nSelected++;
	if ( nSelected >= m_pcWin->GetPlaylist()->GetRowCount(  ) )
		nSelected = 0;
	m_pcWin->GetPlaylist()->Select( nSelected );
	PostMessage( CF_GUI_PLAY );
}

int play_next_entry( void *pData )
{
	CFApp *pcApp = ( CFApp * ) pData;

	pcApp->PlayNext();
	return ( 0 );
}

void CFApp::PlayPrevious()
{
	int nSelected = m_pcWin->GetPlaylist()->GetFirstSelected(  );
	
	if ( nSelected <= 0 )
		nSelected = 0;
	else
		nSelected--;
	m_pcWin->GetPlaylist()->Select( nSelected );
	PostMessage( CF_GUI_PLAY );
}

int play_prev_entry( void *pData)
{
	CFApp *pcApp = ( CFApp * ) pData;

	pcApp->PlayPrevious();
	return ( 0 );
}

void CFApp::HandleMessage( os::Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case CF_APP_STARTED:
		/* Sent by the Application class when started */
		{
			os::String zFileName;
			bool bLoad = false;
			if( pcMessage->FindString( "file/path", &zFileName ) == 0 )
			{
				bLoad = true;
			}
			Start( zFileName, bLoad );
			break;
		}
	case CF_GUI_PLAY:
		
		/* Play  ( sent by the CFWindow class ) */
		if ( m_nState == CF_STATE_STOPPED )
		{
			/* Get Selection */
			if ( m_pcWin->GetPlaylist()->GetRowCount(  ) == 0 )
			{
				break;
			}

			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			CloseCurrentFile();

			/* Start thread */
			uint nSelected = m_pcWin->GetPlaylist()->GetFirstSelected(  );
			m_nPlaylistPosition = nSelected;

			CFListItem * pcRow = ( CFListItem* ) m_pcWin->GetPlaylist()->GetRow( nSelected );

			if ( OpenFile( pcRow->zPath, pcRow->nTrack, pcRow->nStream, pcRow->BuildTitle() ) != 0 )
			{
				std::cout << "Cannot play file!" << std::endl;
				break;
			}
			SetState( CF_STATE_PLAYING );
			
			m_hPlayThread = spawn_thread( "play_thread", (void*)play_thread_entry, 0, 0, this );
			resume_thread( m_hPlayThread );
		}
		else if ( m_nState == CF_STATE_PAUSED )
		{
			/* Start thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}

			SetState( CF_STATE_PLAYING );
			m_nLastPosition = m_pcWin->GetLCD()->GetValue(  ).AsInt32(  ) * m_pcInput->GetLength(  ) / 1000;
			m_hPlayThread = spawn_thread( "play_thread", (void*)play_thread_entry, 0, 0, this );
			resume_thread( m_hPlayThread );
		}
		break;
	case CF_GUI_PAUSE:
		
		/* Pause ( sent by the CFWindow class ) */
		if ( m_nState == CF_STATE_PLAYING )
		{
			SetState( CF_STATE_PAUSED );
			/* Start thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
		}
		break;
	case CF_PLAY_NEXT:
	case CF_PLAY_PREVIOUS:
	case CF_GUI_STOP:

		/* Stop ( sent by the CFWindow class ) */
		if ( m_nState != CF_STATE_STOPPED )
		{
			SetState( CF_STATE_STOPPED );
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			//m_bIsPlaying = false;
			CloseCurrentFile();
			m_nLastPosition = 0;
			m_pcWin->GetLCD()->SetValue( os::Variant( 0 ) );
			m_pcWin->GetLCD()->UpdateTime( 0 );
		}
		/* Play next track */
		if ( pcMessage->GetCode() == CF_PLAY_NEXT )
			resume_thread( spawn_thread( "play_next", (void*)play_next_entry, 0, 0, this ) );
		else if (pcMessage->GetCode() == CF_PLAY_PREVIOUS)
			resume_thread( spawn_thread( "play_prev", (void*)play_prev_entry, 0, 0, this ) );
		break;
	case CF_GUI_SEEK:

		/* Seek ( sent by the CFWindow class ) */
		if ( m_nState == CF_STATE_PLAYING && !m_bStream )
		{
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			/* Set new position */
			m_nLastPosition = m_pcWin->GetLCD()->GetValue(  ).AsInt32(  ) * m_pcInput->GetLength(  ) / 1000;
			m_hPlayThread = spawn_thread( "play_thread", (void*)play_thread_entry, 0, 0, this );
			resume_thread( m_hPlayThread );
		} else {
			m_pcWin->GetLCD()->SetValue( os::Variant( 0 ) );
		}
		break;
	case CF_GUI_REMOVE_FILE:
	
		/* Remove the selected file from the playlist */
		{
			if ( m_pcWin->GetPlaylist()->GetRowCount(  ) == 0 )
			{
				break;
			}
			uint nSelected = m_pcWin->GetPlaylist()->GetFirstSelected(  );

			/* Look if this file is already played */
			if ( m_nState == CF_STATE_PLAYING )
			{
				CFListItem * pcRow = ( CFListItem * ) m_pcWin->GetPlaylist()->GetRow( nSelected );

				if ( pcRow->zPath == m_zAudioFile && pcRow->nTrack == ( int )m_nAudioTrack && pcRow->nStream == ( int )m_nAudioStream )
				{
					os::Alert * pcAlert = new os::Alert( MSG_ERRWND_TITLE, MSG_ERRWND_CANTDELETE, os::Alert::ALERT_WARNING, 0, MSG_ERRWND_OK.c_str(), NULL );
					pcAlert->Go( new os::Invoker( 0 ) );
					break;
				}
			}
			/* Remove it */
			delete( m_pcWin->GetPlaylist()->RemoveRow( nSelected ) );
			if ( m_pcWin->GetPlaylist()->GetRowCount(  ) > 0 )
				m_pcWin->GetPlaylist()->Select( 0, 0 );
			m_pcWin->GetPlaylist()->Invalidate( true );
			m_pcWin->GetPlaylist()->Sync(  );
			UpdatePluginPlaylist();
		}
		break;
	case CF_GUI_SELECT_LIST:

		/* Select playlist */
		{
			/* Open window */
			os::Rect cFrame = os::Rect( 0, 0, 230, 100 );
			SelectWin *pcWin = new SelectWin( cFrame );
			pcWin->CenterInWindow( m_pcWin );
			pcWin->Show();
			pcWin->MakeFocus();
		}
		break;
	case CF_GUI_OPEN_INPUT:
	{

		/* Open ( sent by the CFWindow class ) */
		if ( m_nState != CF_STATE_STOPPED )
		{
			SetState( CF_STATE_STOPPED );
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			m_nLastPosition = 0;
			m_pcWin->GetLCD()->SetValue( os::Variant( 0 ) );
			m_pcWin->GetLCD()->UpdateTime( 0 );
		}

		if ( m_pcInputSelector== NULL )
		{
			/* Open input selector */
			m_pcInputSelector = new os::MediaInputSelector( os::Point( 150, 150 ), "Open", new os::Messenger( this ), new os::Message( CF_IS_OPEN ), new os::Message( CF_IS_CANCEL ) );
			m_pcInputSelector->Show();
			m_pcInputSelector->MakeFocus( true );
		}
		else
		{
			m_pcInputSelector->MakeFocus( true );
		}
		break;
	}
	case CF_IS_CANCEL:
	{
		m_pcInputSelector = NULL;
		break;
	}
	case CF_IS_OPEN:
		{

			/* Message sent by the input selector */
			os::String zFile;
			os::String zInput;

			if ( pcMessage->FindString( "file/path", &zFile.str() ) == 0 && pcMessage->FindString( "input", &zInput.str(  ) ) == 0 )
			{
				CloseCurrentFile();
				OpenInput( zFile, zInput );
			}
			m_pcInputSelector = NULL;
		}
		break;
	case CF_GUI_SHOW_LIST:
		/* Show or hide playlist / visualization */
		{
			
			/* Calling Show() doesn't work, so just resize the window to hide
			   the playlist under the controls */
			if ( m_pcWin->GetFlags() & os::WND_NOT_V_RESIZABLE )
			{		
				m_bListShown = true;
				os::Rect cFrame = m_pcWin->GetFrame();
				cFrame.bottom = cFrame.top + m_cSavedFrame.bottom - m_cSavedFrame.top;
				
				m_pcWin->Lock();
				m_pcWin->SetList( true );
				m_pcWin->SetFrame( cFrame );
				m_pcWin->SetFlags( m_pcWin->GetFlags() & ~os::WND_NOT_V_RESIZABLE );				
				m_pcWin->SetSizeLimits( os::Point( 400,150 ), os::Point( 4096, 4096 ) );
				ActivateVisPlugin();
				m_pcWin->Unlock();	
			}
			else
			{
				m_bListShown = false;
				m_pcWin->Lock();
				DeactivateVisPlugin();
				m_pcWin->SetList( false );
				os::Rect cFrame = m_cSavedFrame = m_pcWin->GetFrame();
				cFrame.bottom = cFrame.top + 60 + m_pcWin->GetMenuBar()->GetBounds().Height();
				m_pcWin->SetFrame( cFrame );
				m_pcWin->SetFlags( m_pcWin->GetFlags() | os::WND_NOT_V_RESIZABLE );
				m_pcWin->SetSizeLimits( os::Point( 400,0 ), os::Point( 4096, 4096 ) );
				m_pcWin->Unlock();
			}
		}
		break;
	case CF_GUI_LIST_INVOKED:

		/* Play one item in the playlist */
		if ( m_nState != CF_STATE_STOPPED )
		{
			SetState( CF_STATE_STOPPED );
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			CloseCurrentFile();
			m_nLastPosition = 0;
			m_pcWin->GetLCD()->SetValue( os::Variant( 0 ) );
			m_pcWin->GetLCD()->UpdateTime( 0 );
		}
		/* Small hack ( select item before the invoked one ) */
		{
			uint nSelected = m_pcWin->GetPlaylist()->GetFirstSelected(  );

			if ( nSelected == 0 )
				nSelected = m_pcWin->GetPlaylist()->GetRowCount(  ) - 1;
			else
				nSelected--;
			m_pcWin->GetPlaylist()->Select( nSelected );

			/* Play next track */
			resume_thread( spawn_thread( "play_next", (void*)play_next_entry, 0, 0, this ) );
		}


		break;
	case CF_GUI_LIST_SELECTED:
		/* Open one playlist */
		{

			os::String zFilename;

			if ( pcMessage->FindString( "file/path", &zFilename.str() ) == 0 )
			{
				/* Stop playback */
				if ( m_nState != CF_STATE_STOPPED )
				{
					SetState( CF_STATE_STOPPED );
					/* Stop thread */
					if ( m_bPlayThread )
					{
						m_bPlayThread = false;
						wait_for_thread( m_hPlayThread );
					}
					CloseCurrentFile();
					m_nLastPosition = 0;
					m_pcWin->GetLCD()->SetValue( os::Variant( 0 ) );
					m_pcWin->GetLCD()->UpdateTime( 0 );
				}
				/* Save opened list */
				SaveList();
				/* Try to open the list */
				if ( !OpenList( zFilename ) )
				{
					std::ifstream hIn;

					hIn.open( zFilename.c_str() );
					if ( hIn.is_open() )
					{
						/* Do not overwrite any files */
						OpenList( m_zListName );
						os::Alert * pcAlert = new os::Alert( MSG_ERRWND_TITLE, MSG_ERRWND_NOTPLAYLIST, os::Alert::ALERT_WARNING, 0, MSG_ERRWND_OK.c_str(), NULL );
						pcAlert->Go( new os::Invoker( 0 ) );
						break;
					}
					/* Create new playlist */
					m_pcWin->GetPlaylist()->Clear(  );
					m_zListName = zFilename;
					m_pcWin->SetTitle( os::String ( os::Path( zFilename.c_str() ).GetLeaf(  ) ) + " - ColdFish" );

					SaveList();
				}
			}
		}
		break;
	case os::M_LOAD_REQUESTED:
	case CF_ADD_FILE:
		{
			/* Add one file ( sent by the CFWindow class or the filerequester ) */
			os::String zFile;
			int i = 0;

			while( pcMessage->FindString( "file/path", &zFile.str(), i ) == 0 && !m_bLockedInput )
			{
				os::FSNode fileNode(zFile);

				if(fileNode.IsFile()) {
					/* Add file as normal */
					AddFile( zFile );
				}
				else if(fileNode.IsDir()) {
					os::Directory dirEntry(zFile);
					os::String lastFile;
					os::String nextFile;
					os::String finalString;

					dirEntry.GetNextEntry(&nextFile);

					while(lastFile != nextFile) {
						lastFile = nextFile;

						if(nextFile != "." && nextFile != "..") {
							finalString = zFile + "/" + nextFile;
							AddFile( finalString );
						}

						dirEntry.GetNextEntry(&nextFile);
					}
				}
				i++;
			}
		}
		break;
	case CF_SET_VIS_PLUGIN:
		{

			/* Set Visualization plugin */
			os::String zName;

			if ( pcMessage->FindString( "name", &zName ) == 0 )
			{
				/* Iterate through the plugins */
				for( uint i = 0; i < m_cPlugins.size(); i++ )
				{
					if( m_cPlugins[i].pi_pcPlugin->GetIdentifier() == zName )
					{
						m_pcWin->Lock();
						DeactivateVisPlugin();
						m_pcCurrentVisPlugin = m_cPlugins[i].pi_pcPlugin;
						if( m_bListShown )						
							ActivateVisPlugin();
						
						if( !m_pcWin->GetVisView()->IsVisible() )
						{
							m_pcWin->GetPlaylist()->Hide();
							m_pcWin->GetVisView()->Show();
						}
						m_pcWin->Unlock();
						break;
					}
				}
			}
		}
		break;
	case CF_GUI_VIEW_LIST:
		/* Show playlist */
		{

			if( !m_pcWin->GetPlaylist()->IsVisible() )
			{
				m_pcWin->Lock();
				DeactivateVisPlugin();
				m_pcCurrentVisPlugin = NULL;
				
				m_pcWin->GetVisView()->Hide();
				m_pcWin->GetPlaylist()->Show();
				m_pcWin->Unlock();
			}
		}
		break;
	
	case CF_GET_SONG:
	{
		int32 nCode;
		pcMessage->FindInt32( "reply_code", &nCode );
		os::Message cReply(nCode);
		if (pcMessage->IsSourceWaiting())
		{
			cReply.AddString("track_name",m_zTrackName.c_str());
			pcMessage->SendReply(&cReply);
		}	
		break;
	}
	
	case CF_GET_PLAYSTATE:
	{
		int32 nCode;
		pcMessage->FindInt32( "reply_code", &nCode );
		os::Message cReply(nCode);
		if (pcMessage->IsSourceWaiting())
		{
			cReply.AddInt32("play_state",m_nState);
			pcMessage->SendReply(&cReply);
		}
		break;
	}

	case CF_GUI_ADD_FILE:
	{
		m_pcWin->PostMessage(CF_GUI_ADD_FILE,m_pcWin);
		break;
	}

	case CF_GUI_ABOUT:
	{
		
		m_pcWin->PostMessage(CF_GUI_ABOUT,m_pcWin);
		break;	
	}

	default:
		os::Looper::HandleMessage( pcMessage );
		break;
	}
}

bool CFApp::OkToQuit()
{
	if ( m_pcManager->IsValid() )
	{
		SaveList();
		os::Settings * pcSettings = new os::Settings();
		pcSettings->AddString( "playlist", m_zListName );
		pcSettings->Save();
		delete( pcSettings );
		CloseCurrentFile();
		//m_pcWin->Close();
	}
	return ( true );
}

CFApplication::CFApplication( const char *pzMimeType, os::String zFileName, bool bLoad ):os::Application( pzMimeType )
{
	/* Select string catalogue */
	try {
		SetCatalog( "coldfish.catalog" );
	} catch( ... ) {
		if (DEBUG)
			std::cout << "Failed to load catalog file!" << std::endl;
	}
	
	m_pcApp = new CFApp();
	m_pcApp->Run();
	os::Message cMsg( CF_APP_STARTED );
	if( bLoad )
		cMsg.AddString( "file/path", zFileName );
	m_pcApp->PostMessage( &cMsg, m_pcApp );
}

CFApplication::~CFApplication()
{
	m_pcApp->Quit();
}

void CFApplication::HandleMessage( os::Message * pcMessage )
{
	m_pcApp->PostMessage( pcMessage, m_pcApp );
	return( os::Application::HandleMessage( pcMessage ) );
}

bool CFApplication::OkToQuit()
{
	m_pcApp->Lock();
	bool bReturn = m_pcApp->OkToQuit();
	m_pcApp->Unlock();
	return( bReturn );
}

int main( int argc, char *argv[] )
{
	CFApplication *pcApp = NULL;

	if ( argc > 1 )
	{
		pcApp = new CFApplication( "application/x-vnd.syllable-ColdFish", argv[1], true );
	}
	else
	{
		pcApp = new CFApplication( "application/x-vnd.syllable-ColdFish", "", false );
	}

	pcApp->Run();
	return ( 0 );
}

