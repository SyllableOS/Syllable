
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
#include <iostream.h>
#include <fstream.h>



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
	os::Message * pcMsg = new os::Message( CF_ADD_FILE );
	pcMsg->AddString( "file/path", zFile );
	os::Application::GetInstance()->PostMessage( pcMsg, os::Application::GetInstance(  ) );
}



/* CFWindow class */

CFWindow::CFWindow( const os::Rect & cFrame, const std::string & cName, const std::string & cTitle, uint32 nFlags ):os::Window( cFrame, cName, cTitle, nFlags )
{
	m_nState = CF_STATE_STOPPED;
	os::Rect cNewFrame = GetBounds();
	
	/* Create menubar */
	m_pcMenuBar = new os::Menu( os::Rect( 0, 0, 1, 1 ), "cf_menu_bar", os::ITEMS_IN_ROW );
	
	os::Menu* pcAppMenu = new os::Menu( os::Rect(), "Application", os::ITEMS_IN_COLUMN );
	pcAppMenu->AddItem( "About...", new os::Message( CF_GUI_ABOUT ) );
	pcAppMenu->AddItem( new os::MenuSeparator() );
	pcAppMenu->AddItem( "Quit", new os::Message( CF_GUI_QUIT ) );
	m_pcMenuBar->AddItem( pcAppMenu );
	
	os::Menu* pcPlaylistMenu = new os::Menu( os::Rect(), "Playlist", os::ITEMS_IN_COLUMN );
	pcPlaylistMenu->AddItem( "Select...", new os::Message( CF_GUI_SELECT_LIST ) );
	m_pcMenuBar->AddItem( pcPlaylistMenu );
	
	os::Menu* pcFileMenu = new os::Menu( os::Rect(), "File", os::ITEMS_IN_COLUMN );
	pcFileMenu->AddItem( "Add...", new os::Message( CF_GUI_ADD_FILE ) );
	pcFileMenu->AddItem( "Remove", new os::Message( CF_GUI_REMOVE_FILE ) );
	m_pcMenuBar->AddItem( pcFileMenu );
	
	cNewFrame = m_pcMenuBar->GetFrame();
	cNewFrame.right = GetBounds().Width();
	cNewFrame.bottom = cNewFrame.top + m_pcMenuBar->GetPreferredSize( false ).y;
	m_pcMenuBar->SetFrame( cNewFrame );
	m_pcMenuBar->SetTargetForItems( this );

	cNewFrame = GetBounds();
	cNewFrame.top += m_pcMenuBar->GetPreferredSize( false ).y + 1;
	cNewFrame.bottom = cNewFrame.top + 70;

	/* Create root view */
	m_pcRoot = new os::LayoutView( cNewFrame, "cf_root", NULL, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP );

	/* Create control view */
	m_pcControls = new os::HLayoutNode( "cf_controls" );
	m_pcControls->SetBorders( os::Rect( 2, 5, 2, 5 ) );

	/* Create buttons */
	m_pcPlay = new os::ImageButton( os::Rect( 0, 0, 1, 1 ), "cf_play", "Play", new os::Message( CF_GUI_PLAY ), NULL, os::ImageButton::IB_TEXT_BOTTOM, true, true, true );
	m_pcPlay->SetImageFromResource( "play.png" );

	m_pcPause = new os::ImageButton( os::Rect( 0, 0, 1, 1 ), "cf_pause", "Pause", new os::Message( CF_GUI_PAUSE ), NULL, os::ImageButton::IB_TEXT_BOTTOM, true, true, true );
	m_pcPause->SetImageFromResource( "pause.png" );

	m_pcStop = new os::ImageButton( os::Rect( 0, 0, 1, 1 ), "cf_stop", "Stop", new os::Message( CF_GUI_STOP ), NULL, os::ImageButton::IB_TEXT_BOTTOM, true, true, true );
	m_pcStop->SetImageFromResource( "stop.png" );

	
	m_pcShowList = new os::ImageButton( os::Rect( 0, 0, 1, 1 ), "cf_show_list", "Playlist", new os::Message( CF_GUI_SHOW_LIST ), NULL, os::ImageButton::IB_TEXT_BOTTOM, true, true, true );
	m_pcShowList->SetImageFromResource( "list.png" );

	m_pcPause->SetEnable( false );
	m_pcStop->SetEnable( false );

	os::VLayoutNode * pcCenter = new os::VLayoutNode( "cf_v_center" );

	/* Create LCD */
	m_pcLCD = new os::Lcd( os::Rect( 0, 0, 1, 1 ) );
	pcCenter->AddChild( m_pcLCD );

	/* Create slider */
	m_pcSlider = new os::SeekSlider( os::Rect( 0, 0, 1, 1 ), "cf_slider", new os::Message( CF_GUI_SEEK ) );
	m_pcSlider->SetResizeMask( os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
	m_pcSlider->SetTarget( this );
	m_pcSlider->SetMinMax( 0, 1000 );
	pcCenter->AddChild( m_pcSlider );

	m_pcControls->AddChild( m_pcPlay );
	m_pcControls->AddChild( m_pcPause );
	m_pcControls->AddChild( m_pcStop );
	m_pcControls->AddChild( pcCenter );
	m_pcControls->AddChild( m_pcShowList );

	m_pcControls->SameWidth( "cf_play", "cf_pause", "cf_stop", "cf_show_list", NULL );

	m_pcRoot->SetRoot( m_pcControls );

	/* Create playlist */
	cNewFrame = GetBounds();
	cNewFrame.top = 71 + m_pcMenuBar->GetPreferredSize( false ).y + 1;
	m_pcPlaylist = new CFPlaylist( cNewFrame );
	m_pcPlaylist->SetInvokeMsg( new os::Message( CF_GUI_LIST_INVOKED ) );
	m_pcPlaylist->SetTarget( this );
	m_pcPlaylist->SetAutoSort( false );
	m_pcPlaylist->SetMultiSelect( false );
	m_pcPlaylist->InsertColumn( "File", cNewFrame.Width() - 160 );
	m_pcPlaylist->InsertColumn( "Track", 50 );
	m_pcPlaylist->InsertColumn( "Stream", 50 );
	m_pcPlaylist->InsertColumn( "Length", 50 );
	AddChild( m_pcPlaylist );
	AddChild( m_pcMenuBar );
	AddChild( m_pcRoot );
	
	/* Create file selector */
	m_pcFileDialog = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( os::Application::GetInstance() ), NULL, os::FileRequester::NODE_FILE, false );
}

CFWindow::~CFWindow()
{
}

void CFWindow::HandleMessage( os::Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case CF_GUI_PLAY:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_PLAY, os::Application::GetInstance(  ) );
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
	case CF_GUI_QUIT:
		/* Quit */
		os::Application::GetInstance()->PostMessage( os::M_QUIT );
		break;
	case CF_GUI_ABOUT:
		{
		/* Show about alert */
		os::Alert* pcAbout = new os::Alert( "About ColdFish", "ColdFish 1.0 Beta\n\nA Music Player\nCopyright 2003 Arno Klenke\n"
										"Copyright 2003 Kristian Van Der Vliet\n\n"
										"ColdFish is released under the LGPL.", os::Alert::ALERT_INFO, 
										os::WND_NOT_RESIZABLE, "O.K.", NULL );
		pcAbout->Go( new os::Invoker( 0 ) ); 
		}
		break;
	case CF_GUI_SHOW_LIST:
		/* Forward message to the CFApp class */
		os::Application::GetInstance()->PostMessage( CF_GUI_SHOW_LIST, os::Application::GetInstance(  ) );
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
				m_pcPlay->SetEnable( true );
				m_pcPause->SetEnable( false );
				m_pcStop->SetEnable( false );
			}
			else if ( m_nState == CF_STATE_PLAYING )
			{
				m_pcPlay->SetEnable( false );
				m_pcPause->SetEnable( true );
				m_pcStop->SetEnable( true );
			}
			else if ( m_nState == CF_STATE_PAUSED )
			{
				m_pcPlay->SetEnable( true );
				m_pcPause->SetEnable( false );
				m_pcStop->SetEnable( true );
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


/* MPApp class */
CFApp::CFApp( const char *pzMimeType, os::String zFileName, bool bLoad ):os::Application( pzMimeType )
{
	/* Set default values */
	m_nState = CF_STATE_STOPPED;

	m_zListName = os::String ( getenv( "HOME" ) ) + "/Default Playlist.plst";

	m_pcInput = NULL;
	m_zAudioFile = "";
	m_nAudioTrack = 0;
	m_nAudioStream = 0;
	m_pcAudioCodec = NULL;
	m_pcAudioOutput = NULL;
	m_bPlayThread = false;

	/* Load settings */
	os::Settings * pcSettings = new os::Settings();
	if ( pcSettings->Load() == 0 )
	{
		m_zListName = pcSettings->GetString( "playlist", m_zListName.c_str() );
	}
	delete( pcSettings );

	/* Create media manager */
	m_pcManager = new os::MediaManager();

	if ( !m_pcManager->IsValid() )
	{
		cout << "Media server is not running" << endl;
		PostMessage( os::M_QUIT );
		return;
	}

	/* Create window */
	m_pcWin = new CFWindow( os::Rect( 50, 100, 500, 350 ), "cf_window", "Default Playlist.plst - ColdFish", os::WND_NO_ZOOM_BUT | os::WND_NO_DEPTH_BUT | os::WND_NOT_H_RESIZABLE );
	m_pcWin->Show();
	m_pcWin->MakeFocus( true );

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
			OpenList( os::String ( getenv( "HOME" ) ) + "/Default Playlist.plst" );
		}
	}
}

CFApp::~CFApp()
{
	/* Close and delete everything */
	if ( m_pcManager->IsValid() )
	{
		CloseCurrentFile();
	}
	delete( m_pcManager );
}

/* Thread which is responsible to play the file */
void CFApp::PlayThread()
{
	bigtime_t nTime = get_system_time();

	m_bPlayThread = true;
	bool bGrab;
	bool bNoGrab;
	bool bStarted = false;

	os::MediaPacket_s sPacket;
	os::MediaPacket_s sAudioPacket;
	uint64 nAudioBytes = 0;
	uint8 nErrorCount = 0;
	bool bError = false;
	uint32 nGrabValue = 80;

	cout << "Play thread running" << endl;
	/* Seek to last position */
	if ( !m_bStream )
		m_pcInput->Seek( m_nLastPosition );
	/* Create audio output packet */
	if ( m_bPacket )
	{
		m_pcAudioCodec->CreateAudioOutputPacket( &sAudioPacket );
	}
	while ( m_bPlayThread )
	{
		if ( m_bPacket )
		{
		      grab:
			/* Look if we have to grab data */
			bGrab = false;
			bNoGrab = false;

			if ( m_pcAudioOutput->GetUsedBufferPercentage() < nGrabValue )
				bGrab = true;
			if ( m_pcAudioOutput->GetUsedBufferPercentage() >= nGrabValue )
				bNoGrab = true;
			if ( bGrab && !bNoGrab )
			{
				/* Grab data */
				if ( m_pcInput->ReadPacket( &sPacket ) == 0 )
				{
					nErrorCount = 0;
					/* Decode audio data */
					if ( sPacket.nStream == m_nAudioStream )
					{
						if ( m_pcAudioCodec->DecodePacket( &sPacket, &sAudioPacket ) == 0 )
						{
							if ( sAudioPacket.nSize[0] > 0 )
							{
								m_pcAudioOutput->WritePacket( 0, &sAudioPacket );
								nAudioBytes += sAudioPacket.nSize[0];
							}
						}
					}
					m_pcInput->FreePacket( &sPacket );
					goto grab;
				}
				else
				{
					/* Increase error count */
					nErrorCount++;
					if ( m_pcAudioOutput->GetDelay() > 0 )
						nErrorCount--;
					if ( nErrorCount > 10 && !bError )
					{
						os::Application::GetInstance()->PostMessage( CF_PLAY_NEXT, os::Application::GetInstance(  ) );
						bError = true;
					}
				}

			}

			/* Look if we have to start now */
			if ( !bStarted )
			{
				if ( bNoGrab == true || m_pcInput->GetLength() < 5 )
				{
					bStarted = true;
					cout << "Go" << endl;
				}
			}
			/* If we have started then flush the media data */
			if ( bStarted )
			{
				m_pcAudioOutput->Flush();
			}

		}
		snooze( 1000 );

		if ( !m_bStream && get_system_time() > nTime + 1000000 )
		{
			/* Move slider */
			m_pcWin->GetSlider()->SetValue( os::Variant( ( int )( m_pcInput->GetCurrentPosition(  ) * 1000 / m_pcInput->GetLength(  ) ) ), false );
			m_pcWin->GetLCD()->UpdateTime( m_pcInput->GetCurrentPosition(  ) );
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
	cout << "Stop thread" << endl;
	if ( !m_bPacket )
	{
		m_pcInput->StopTrack();
	}
	if ( m_bPacket )
	{
		m_pcAudioOutput->Clear();
		m_pcAudioCodec->DeleteAudioOutputPacket( &sAudioPacket );
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
	char zTemp[255];
	char zTemp2[255];

	m_pcWin->GetPlaylist()->Clear(  );
	m_pcWin->GetSlider()->SetValue( os::Variant( 0 ) );
	m_pcWin->GetLCD()->UpdateTime( 0 );
	m_pcWin->GetLCD()->SetTrackName( "Unknown" );
	m_pcWin->GetLCD()->SetTrackNumber( 0 );
	ifstream hIn;

	hIn.open( zFileName.c_str() );
	if ( !hIn.is_open() )
	{
		cout << "Could not open playlist!" << endl;
		return ( false );
	}
	/* Read header */
	if ( hIn.getline( zTemp, 255, '\n' ) < 0 )
		return ( false );

	if ( strcmp( zTemp, "<PLAYLIST-V1>" ) )
	{
		cout << "Invalid playlist!" << endl;
		return ( false );
	}

	/* Read entries */
	bool bInEntry = false;
	os::String zFile;
	int nTrack = 0;
	int nStream = 0;

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
			continue;
		}
		if ( !strcmp( zTemp, "<ENTRYEND>" ) )
		{
			uint32 nM, nS;

			bInEntry = false;

			/* Check entry */
			os::MediaInput * pcInput = m_pcManager->GetBestInput( zFile );
			if ( pcInput == NULL )
				continue;
			pcInput->SelectTrack( nTrack );

			/* Add new row */
			os::ListViewStringRow * pcRow = new os::ListViewStringRow();
			pcRow->AppendString( zFile );
			sprintf( zTemp, "%i", ( int )nTrack + 1 );
			pcRow->AppendString( zTemp );
			sprintf( zTemp, "%i", ( int )nStream + 1 );
			pcRow->AppendString( zTemp );
			secs_to_ms( pcInput->GetLength(), &nM, &nS );
			sprintf( zTemp, "%.2li:%.2li", nM, nS );
			pcRow->AppendString( zTemp );
			m_pcWin->GetPlaylist()->InsertRow( pcRow );

			if ( m_pcWin->GetPlaylist()->GetRowCount(  ) == 1 )
				m_pcWin->GetPlaylist()->Select( 0, 0 );
			delete( pcInput );
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
	}
	return ( false );
      finished:
	hIn.close();
	m_zListName = zFileName;

	/* Set title */
	os::Path cPath( zFileName.c_str() );
	m_pcWin->SetTitle( os::String ( cPath.GetLeaf() ) + " - ColdFish" );
	cout << "List openened" << endl;
	return ( true );

}

/* Save the opened playlist */
void CFApp::SaveList()
{
	uint32 i;

	/* Open output file */
	ofstream hOut;

	hOut.open( m_zListName.c_str() );
	if ( !hOut.is_open() )
	{
		cout << "Could not save playlist!" << endl;
		return;
	}


	hOut << "<PLAYLIST-V1>" << endl;

	/* Save files */
	for ( i = 0; i < m_pcWin->GetPlaylist()->GetRowCount(  ); i++ )
	{
		hOut << "<ENTRYSTART>" << endl;
		os::ListViewStringRow * pcRow = ( os::ListViewStringRow * ) m_pcWin->GetPlaylist()->GetRow( i );
		hOut << "<FILE>" << endl;
		hOut << pcRow->GetString( 0 ).c_str() << endl;
		hOut << "<TRACK>" << endl;
		hOut << pcRow->GetString( 1 ).c_str() << endl;
		hOut << "<STREAM>" << endl;
		hOut << pcRow->GetString( 2 ).c_str() << endl;
		hOut << "<ENTRYEND>" << endl;
	}
	hOut << "<END>" << endl;

	hOut.close();
}


/* Check if this is a valid file */
void CFApp::AddFile( os::String zFileName )
{

	uint32 i;
	char zTemp[255];

	cout << "Add File " << zFileName.c_str() << endl;

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
				os::ListViewStringRow * pcRow = new os::ListViewStringRow();
				pcRow->AppendString( zFileName );
				sprintf( zTemp, "%i", ( int )i + 1 );
				pcRow->AppendString( zTemp );
				sprintf( zTemp, "%i", ( int )j + 1 );
				pcRow->AppendString( zTemp );
				secs_to_ms( pcInput->GetLength(), &nM, &nS );
				sprintf( zTemp, "%.2li:%.2li", nM, nS );
				pcRow->AppendString( zTemp );
				m_pcWin->GetPlaylist()->InsertRow( pcRow );

				if ( m_pcWin->GetPlaylist()->GetRowCount(  ) == 1 )
					m_pcWin->GetPlaylist()->Select( 0, 0 );
			}
		}
	}
	pcInput->Close();
	delete( pcInput );

	SaveList();

	return;
      invalid:
	if ( pcInput )
	{
		pcInput->Close();
		delete( pcInput );
	}
	os::Alert * pcAlert = new os::Alert( "ColdFish Error", "ColdFish cannot play this file.", os::Alert::ALERT_WARNING, 0, "Ok", NULL );
	pcAlert->Go( new os::Invoker( 0 ) );
}

/* Play one track of a file / device */
int CFApp::OpenFile( os::String zFileName, uint32 nTrack, uint32 nStream )
{
	/* Close */
	CloseCurrentFile();
	uint32 i = 0;

	m_pcInput = m_pcManager->GetBestInput( zFileName );
	if ( m_pcInput == NULL )
	{
		cout << "Cannot get input!" << endl;
		return ( -1 );
	}
	/* Open input */
	if ( m_pcInput->Open( zFileName ) != 0 )
	{
		cout << "Cannot open input!" << endl;
		return ( -1 );
	}
	m_bPacket = m_pcInput->PacketBased();
	m_bStream = m_pcInput->StreamBased();

	/* Look if this is a packet based file / device */
	if ( !m_bPacket )
	{
		cout << "Not packet based!" << endl;
		CloseCurrentFile();
		return ( -1 );
	}

	/* Select track */
	if ( m_pcInput->SelectTrack( nTrack ) != nTrack )
	{
		cout << "Track selection failed!" << endl;
		CloseCurrentFile();
		return ( -1 );
	}

	/* Look if the stream is valid */
	if ( nStream >= m_pcInput->GetStreamCount() )
	{
		cout << "Invalid stream number!" << endl;
		CloseCurrentFile();
		return ( -1 );
	}

	/* Check stream */
	if ( m_pcInput->GetStreamFormat( nStream ).nType != os::MEDIA_TYPE_AUDIO )
	{
		cout << "Invalid stream format!" << endl;
		CloseCurrentFile();
		return ( -1 );
	}
	m_sAudioFormat = m_pcInput->GetStreamFormat( nStream );


	/* Open audio output */
	m_pcAudioOutput = m_pcManager->GetDefaultAudioOutput();
	if ( m_pcAudioOutput == NULL || ( m_pcAudioOutput && m_pcAudioOutput->FileNameRequired() ) || ( m_pcAudioOutput && m_pcAudioOutput->Open( "" ) != 0 ) )
	{
		cout << "Cannot open audio output!" << endl;
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
				delete( m_pcAudioCodec );
				m_pcAudioCodec = NULL;
			}
	}
	if ( m_pcAudioCodec == NULL || m_pcAudioOutput->AddStream( os::String ( os::Path( zFileName.c_str() ).GetLeaf(  ) ), m_pcAudioCodec->GetExternalFormat(  ) ) != 0 )
	{
		cout << "Cannot open audio codec!" << endl;
		CloseCurrentFile();
		return ( -1 );
	}
	else
	{
		cout << "Using Audio codec " << m_pcAudioCodec->GetIdentifier().c_str(  ) << endl;
	}

	/* Construct name */
	os::Path cPath = os::Path( zFileName.c_str() );

	/* Set title */
	if ( m_pcInput->FileNameRequired() )
		m_pcWin->SetTitle( os::String ( os::Path( m_zListName.c_str() ).GetLeaf(  ) ) + " - ColdFish (Playing " + os::String ( cPath.GetLeaf(  ) ) + ")" );

	else
		m_pcWin->SetTitle( os::String ( os::Path( m_zListName.c_str() ).GetLeaf(  ) ) + " - ColdFish (Playing " + m_pcInput->GetIdentifier(  ) + ")" );

	/* Save information */
	m_nAudioTrack = nTrack;
	m_nAudioStream = nStream;
	m_zAudioFile = zFileName;

	/* Set LCD */

	m_pcWin->GetLCD()->SetTrackName( cPath.GetLeaf(  ) );
	m_pcWin->GetLCD()->SetTrackNumber( m_nAudioTrack + 1 );
	m_pcWin->GetLCD()->UpdateTime( 0 );

	/* Set slider */
	m_pcWin->GetSlider()->SetValue( os::Variant( 0 ) );

	return ( 0 );
}

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
		delete( m_pcAudioOutput );
		m_pcAudioOutput = NULL;
	}

	if ( m_pcAudioCodec )
	{
		m_pcAudioCodec->Close();
		delete( m_pcAudioCodec );
		m_pcAudioCodec = NULL;
	}

	if ( m_pcInput )
	{
		m_pcInput->Close();
		delete( m_pcInput );
		m_pcInput = NULL;
	}
	m_pcWin->SetTitle( os::String ( os::Path( m_zListName.c_str() ).GetLeaf(  ) ) + " - ColdFish" );

	m_zAudioFile = "";
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

void CFApp::HandleMessage( os::Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{

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

			os::ListViewStringRow * pcRow = ( os::ListViewStringRow * ) m_pcWin->GetPlaylist()->GetRow( nSelected );

			if ( OpenFile( pcRow->GetString( 0 ), atoi( pcRow->GetString( 1 ).c_str() ) - 1, atoi( pcRow->GetString( 2 ).c_str(  ) ) - 1 ) != 0 )
			{
				cout << "Cannot play file!" << endl;
				break;
			}


			m_nState = CF_STATE_PLAYING;
			m_pcWin->SetState( CF_STATE_PLAYING );
			m_pcWin->PostMessage( CF_STATE_CHANGED, m_pcWin );
			m_hPlayThread = spawn_thread( "play_thread", play_thread_entry, 0, 0, this );
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

			m_nState = CF_STATE_PLAYING;
			m_pcWin->SetState( CF_STATE_PLAYING );
			m_pcWin->PostMessage( CF_STATE_CHANGED, m_pcWin );
			m_nLastPosition = m_pcWin->GetSlider()->GetValue(  ).AsInt32(  ) * m_pcInput->GetLength(  ) / 1000;
			m_hPlayThread = spawn_thread( "play_thread", play_thread_entry, 0, 0, this );
			resume_thread( m_hPlayThread );
		}
		break;
	case CF_GUI_PAUSE:
		/* Pause ( sent by the CFWindow class ) */
		if ( m_nState == CF_STATE_PLAYING )
		{

			m_nState = CF_STATE_PAUSED;
			m_pcWin->SetState( CF_STATE_PAUSED );
			m_pcWin->PostMessage( CF_STATE_CHANGED, m_pcWin );
			/* Start thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
		}
		break;
	case CF_PLAY_NEXT:
	case CF_GUI_STOP:
		/* Stop ( sent by the CFWindow class ) */
		if ( m_nState != CF_STATE_STOPPED )
		{
			m_nState = CF_STATE_STOPPED;
			m_pcWin->SetState( CF_STATE_STOPPED );
			m_pcWin->PostMessage( CF_STATE_CHANGED, m_pcWin );
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			CloseCurrentFile();
			m_nLastPosition = 0;
			m_pcWin->GetSlider()->SetValue( os::Variant( 0 ) );
			m_pcWin->GetLCD()->UpdateTime( 0 );
		}
		/* Play next track */
		if ( pcMessage->GetCode() == CF_PLAY_NEXT )
			resume_thread( spawn_thread( "play_next", play_next_entry, 0, 0, this ) );
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
			m_nLastPosition = m_pcWin->GetSlider()->GetValue(  ).AsInt32(  ) * m_pcInput->GetLength(  ) / 1000;
			m_hPlayThread = spawn_thread( "play_thread", play_thread_entry, 0, 0, this );
			resume_thread( m_hPlayThread );
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
				os::ListViewStringRow * pcRow = ( os::ListViewStringRow * ) m_pcWin->GetPlaylist()->GetRow( nSelected );

				if ( os::String ( pcRow->GetString( 0 ) ) == m_zAudioFile && atoi( pcRow->GetString( 1 ).c_str() ) - 1 == ( int )m_nAudioTrack && atoi( pcRow->GetString( 2 ).c_str(  ) ) - 1 == ( int )m_nAudioStream )
				{
					os::Alert * pcAlert = new os::Alert( "ColdFish Error", "The currently played track cannot be deleted.", os::Alert::ALERT_WARNING, 0, "Ok", NULL );
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

		}
		break;
	case CF_GUI_SELECT_LIST:
		/* Select playlist */
		{
			/* Open window */
			os::Rect cFrame = m_pcWin->GetFrame();
			cFrame.top += 50;
			cFrame.left += 50;
			cFrame.right = cFrame.left + 210;
			cFrame.bottom = cFrame.top + 100;
			SelectWin *pcWin = new SelectWin( cFrame );

			pcWin->Show();
			pcWin->MakeFocus();
		}
		break;
	case CF_GUI_SHOW_LIST:
		/* Show or hide playlist */
		{
			/* Calling Show() doesn't work, so just resize the window to hide
			   the playlist under the controls */
			if ( m_pcWin->GetFlags() & os::WND_NOT_V_RESIZABLE )
			{
				os::Rect cFrame = m_pcWin->GetFrame();
				cFrame.bottom = cFrame.top + 250;
				m_pcWin->SetFrame( cFrame );
				m_pcWin->SetFlags( m_pcWin->GetFlags() & ~os::WND_NOT_V_RESIZABLE );
			}
			else
			{
				os::Rect cFrame = m_pcWin->GetFrame();
				cFrame.bottom = cFrame.top + 70 + m_pcWin->GetMenuBar()->GetBounds().Height();
				m_pcWin->SetFrame( cFrame );
				m_pcWin->SetFlags( m_pcWin->GetFlags() | os::WND_NOT_V_RESIZABLE );
			}

		}
		break;
	case CF_GUI_LIST_INVOKED:
		/* Play one item in the playlist */
		if ( m_nState != CF_STATE_STOPPED )
		{
			m_nState = CF_STATE_STOPPED;
			m_pcWin->SetState( CF_STATE_STOPPED );
			m_pcWin->PostMessage( CF_STATE_CHANGED, m_pcWin );
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			CloseCurrentFile();
			m_nLastPosition = 0;
			m_pcWin->GetSlider()->SetValue( os::Variant( 0 ) );
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
			resume_thread( spawn_thread( "play_next", play_next_entry, 0, 0, this ) );
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
					m_nState = CF_STATE_STOPPED;
					m_pcWin->SetState( CF_STATE_STOPPED );
					m_pcWin->PostMessage( CF_STATE_CHANGED, m_pcWin );
					/* Stop thread */
					if ( m_bPlayThread )
					{
						m_bPlayThread = false;
						wait_for_thread( m_hPlayThread );
					}
					CloseCurrentFile();
					m_nLastPosition = 0;
					m_pcWin->GetSlider()->SetValue( os::Variant( 0 ) );
					m_pcWin->GetLCD()->UpdateTime( 0 );
				}
				/* Save opened list */
				SaveList();
				/* Try to open the list */
				if ( !OpenList( zFilename ) )
				{
					ifstream hIn;

					hIn.open( zFilename.c_str() );
					if ( hIn.is_open() )
					{
						/* Do not overwrite any files */
						OpenList( m_zListName );
						os::Alert * pcAlert = new os::Alert( "ColdFish Error", "Selected file is not a playlist.", os::Alert::ALERT_WARNING, 0, "Ok", NULL );
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

			if ( pcMessage->FindString( "file/path", &zFile.str() ) == 0 )
			{
				AddFile( zFile );
			}
		}
		break;
	default:
		os::Application::HandleMessage( pcMessage );
		break;
	}
}

bool CFApp::OkToQuit()
{
	cout << "Quit" << endl;

	if ( m_pcManager->IsValid() )
	{
		SaveList();
		os::Settings * pcSettings = new os::Settings();
		pcSettings->AddString( "playlist", m_zListName );
		pcSettings->Save();
		delete( pcSettings );
		CloseCurrentFile();
		m_pcWin->Close();
	}
	return ( true );
}

int main( int argc, char *argv[] )
{
	CFApp *pcApp = NULL;

	if ( argc > 1 )
	{
		pcApp = new CFApp( "application/x-vnd-ColdFish", argv[1], true );
	}
	else
	{
		pcApp = new CFApp( "application/x-vnd-ColdFish", "", false );
	}

	pcApp->Run();
	return ( 0 );
}









