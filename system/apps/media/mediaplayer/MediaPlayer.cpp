/*  Syllable Media Player
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

#include "MediaPlayer.h"
#include <iostream>

/* TODO: 
 * - Reopen streams after track change.
 * - Change to the next track after the current ends.
 */
 

void SetCButtonImageFromResource( os::CImageButton* pcButton, os::String zResource )
{
	os::File cSelf( open_image_file( get_image_id() ) );
	os::Resources cCol( &cSelf );		
	os::ResStream *pcStream = cCol.GetResourceStream( zResource );
	pcButton->SetImage( pcStream );
	delete( pcStream );
}



/* MPWindow class */

MPWindow::MPWindow( const os::Rect & cFrame, const std::string & cName, const std::string & cTitle ):os::Window( cFrame, cName, cTitle )
{
	m_nState = MP_STATE_STOPPED;
	m_pcVideo = NULL;
	
	os::Rect cNewFrame;
	
	/* Set new flags */
	SetFlags( GetFlags() | os::WND_NOT_RESIZABLE );
	
	/* Create menubar */
	m_pcMenuBar = new os::Menu( os::Rect( 0, 0, 1, 1 ), "mp_menu_bar", os::ITEMS_IN_ROW );
	
	
	os::Menu* pcAppMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_APPLICATION, os::ITEMS_IN_COLUMN );
	pcAppMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_ABOUT, new os::Message( MP_GUI_ABOUT ) );
	pcAppMenu->AddItem( new os::MenuSeparator() );
	pcAppMenu->AddItem( MSG_MAINWND_MENU_APPLICATION_QUIT, new os::Message( MP_GUI_QUIT ) );
	m_pcMenuBar->AddItem( pcAppMenu );
	
	os::Menu* pcFileMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_FILE, os::ITEMS_IN_COLUMN );
	pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_OPEN, new os::Message( MP_GUI_OPEN ) );
	pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_OPEN_INPUT, new os::Message( MP_GUI_OPEN_INPUT ) );
	pcFileMenu->AddItem( new os::MenuSeparator() );
	pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_FULLSCREEN, new os::Message( MP_GUI_FULLSCREEN ) );
	pcFileMenu->GetItemAt( 3 )->SetEnable( false );
	m_pcMenuBar->AddItem( pcFileMenu );
	
	m_pcTracksMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_TRACKS, os::ITEMS_IN_COLUMN );
	m_pcMenuBar->AddItem( m_pcTracksMenu );
	
	cNewFrame = m_pcMenuBar->GetFrame();
	cNewFrame.right = GetBounds().Width();
	cNewFrame.bottom = cNewFrame.top + m_pcMenuBar->GetPreferredSize( false ).y;
	m_pcMenuBar->SetFrame( cNewFrame );
	m_pcMenuBar->SetTargetForItems( this );
	
	/* Set window size */
	cNewFrame = cFrame;
	cNewFrame.right = cNewFrame.left + 300;
	cNewFrame.bottom = cNewFrame.top + 40 + m_pcMenuBar->GetPreferredSize( false ).y;
	SetFrame( cNewFrame );
	
	
	/* Create control view */
	cNewFrame = GetBounds();
	cNewFrame.top = m_pcMenuBar->GetPreferredSize( false ).y + 1;
	cNewFrame.bottom = m_pcMenuBar->GetPreferredSize( false ).y + 40;
	m_pcControls = new os::View( cNewFrame, "mp_controls", os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
	/* Create buttons */
	m_pcPlayPause = new os::CImageButton( os::Rect( 5, 1, 40, 38 ), "mp_play_pause", "", new os::Message( MP_GUI_PLAY_PAUSE ), NULL, os::ImageButton::IB_TEXT_BOTTOM, false, false, true );
	SetCButtonImageFromResource( m_pcPlayPause, "play.png" );
	m_pcPlayPause->SetEnable( false );

	m_pcStop = new os::CImageButton( os::Rect( 41, 1, 76, 38 ), "mp_stop", "", new os::Message( MP_GUI_STOP ), NULL, os::ImageButton::IB_TEXT_BOTTOM, false, false, true );
	SetCButtonImageFromResource( m_pcStop, "stop.png" );
	m_pcStop->SetEnable( false );
	

	/* Create slider */
	m_pcSlider = new os::SeekSlider( os::Rect( 80, 6, 299, 36 ), "mp_slider", new os::Message( MP_GUI_SEEK ) );
	m_pcSlider->SetResizeMask( os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
	m_pcSlider->SetTarget( this );
	m_pcSlider->SetEnable( false );

	m_pcControls->AddChild( m_pcPlayPause );
	m_pcControls->AddChild( m_pcStop );
	m_pcControls->AddChild( m_pcSlider );
	
	AddChild( m_pcMenuBar );
	AddChild( m_pcControls );
	
	/* Create file selector */
	m_pcFileDialog = new os::FileRequester( os::FileRequester::LOAD_REQ, new os::Messenger( os::Application::GetInstance() ), "", os::FileRequester::NODE_FILE, false );
	m_pcFileDialog->Lock();
	m_pcFileDialog->Start();
	m_pcFileDialog->Unlock();
	
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
}



MPWindow::~MPWindow()
{
	m_pcFileDialog->Close();
}

void MPWindow::HandleMessage( os::Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case MP_GUI_PLAY_PAUSE:
		/* Forward message to the MPApp class */
		os::Application::GetInstance()->PostMessage( MP_GUI_PLAY_PAUSE, os::Application::GetInstance(  ) );
		break;
	case MP_GUI_STOP:
		/* Forward message to the MPApp class */
		os::Application::GetInstance()->PostMessage( MP_GUI_STOP, os::Application::GetInstance(  ) );
		break;
	case MP_GUI_SEEK:
		/* Forward message to the MPApp class */
		os::Application::GetInstance()->PostMessage( MP_GUI_SEEK, os::Application::GetInstance(  ) );
		break;
	case MP_GUI_FULLSCREEN:
		/* Forward message to the MPApp class */
		os::Application::GetInstance()->PostMessage( MP_GUI_FULLSCREEN, os::Application::GetInstance(  ) );
		break;
	case MP_GUI_OPEN:
		/* Open file selector */
		os::Application::GetInstance()->PostMessage( MP_GUI_STOP, os::Application::GetInstance(  ) );
		m_pcFileDialog->Show();
		m_pcFileDialog->MakeFocus( true );
		break;
	case MP_GUI_OPEN_INPUT:
		/* Forward message to the MPApp class */
		os::Application::GetInstance()->PostMessage( MP_GUI_OPEN_INPUT, os::Application::GetInstance(  ) );
		break;
	
	case MP_GUI_QUIT:
		/* Quit */
		os::Application::GetInstance()->PostMessage( os::M_QUIT );
		break;
	case MP_GUI_ABOUT:
		{
		/* Show about alert */
		os::String cBodyText;
		cBodyText = os::String( "Media Player V1.1\n" ) + MSG_ABOUTWND_TEXT;
		
		os::Alert* pcAbout = new os::Alert( MSG_ABOUTWND_TITLE, cBodyText, os::Alert::ALERT_INFO, 
										os::WND_NOT_RESIZABLE, MSG_ABOUTWND_OK.c_str(), NULL );
		pcAbout->Go( new os::Invoker( 0 ) ); 
		}
		break;
	case MP_CREATE_VIDEO:
		{
			/* Message sent by the MPApp class */
			int32 nWidth, nHeight;

			os::Rect cFrame = GetFrame();
			if ( pcMessage->FindInt32( "width", &nWidth ) == 0 && pcMessage->FindInt32( "height", &nHeight ) == 0 )
			{
				m_pcVideo->MoveBy( 0, m_pcMenuBar->GetPreferredSize( false ).y + 1 );
				if ( cFrame.Width() < nWidth )
					cFrame.right = cFrame.left + nWidth;
				cFrame.bottom = cFrame.top + nHeight + 40 + m_pcMenuBar->GetPreferredSize( false ).y;
				SetFrame( cFrame );
				AddChild( m_pcVideo );
				SetFlags( GetFlags() & ~os::WND_NOT_RESIZABLE );
				m_pcMenuBar->GetSubMenuAt( 1 )->GetItemAt( 3 )->SetEnable( true );
			}
		}
		break;
	case MP_DELETE_VIDEO:
		{
			/* Message sent by the MPApp class */
			os::Rect cFrame = GetFrame();
			if ( m_pcVideo )
			{
				RemoveChild( m_pcVideo );
				delete( m_pcVideo );
			}
			m_pcVideo = NULL;
			cFrame.right = cFrame.left + 300;
			cFrame.bottom = cFrame.top + 40 + m_pcMenuBar->GetPreferredSize( false ).y;
			SetFrame( cFrame );
			SetFlags( GetFlags() | os::WND_NOT_RESIZABLE );
			m_pcMenuBar->GetSubMenuAt( 1 )->GetItemAt( 3 )->SetEnable( false );
		}
		break;
	case MP_STATE_CHANGED:
		{
			/* Message sent by the MPApp class */
			if ( m_nState == MP_STATE_STOPPED )
			{
				m_pcPlayPause->SetEnable( true );
				m_pcStop->SetEnable( false );
				m_pcSlider->SetEnable( false );
				SetCButtonImageFromResource( m_pcPlayPause, "play.png" );
			}
			else if ( m_nState == MP_STATE_PLAYING )
			{
				m_pcPlayPause->SetEnable( true );
				m_pcStop->SetEnable( true );
				m_pcSlider->SetEnable( true );
				SetCButtonImageFromResource( m_pcPlayPause, "pause.png" );	
			}
			else if ( m_nState == MP_STATE_PAUSED )
			{
				m_pcPlayPause->SetEnable( true );
				m_pcStop->SetEnable( true );
				m_pcSlider->SetEnable( false );
				SetCButtonImageFromResource( m_pcPlayPause, "play.png" );
			}
			m_pcControls->Invalidate( true );
			Sync();
		}
		break;

	default:
		os::Window::HandleMessage( pcMessage );
		break;
	}
}

bool MPWindow::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return ( false );
}

void MPWindow::FrameMoved( const os::Point & cDelta )
{
	if ( m_pcVideo && m_nState != MP_STATE_PLAYING )
		os::Application::GetInstance()->PostMessage( MP_UPDATE_VIDEO, os::Application::GetInstance(  ) );
	os::Window::FrameMoved( cDelta );
}


/* MPApp class */
MPApp::MPApp( const char *pzMimeType, os::String zFileName, bool bLoad ):os::Application( pzMimeType )
{
	/* Select string catalogue */
	try {
		SetCatalog( "mediaplayer.catalog" );
	} catch( ... ) {
		std::cout << "Failed to load catalog file!" << std::endl;
	}
	
	/* Set default values */
	m_nState = MP_STATE_STOPPED;
	m_pcInputSelector = NULL;
	m_bFileLoaded = false;
	m_zFileName = "";
	m_pcInput = NULL;
	m_bAudio = m_bVideo = false;
	m_nAudioStream = m_nVideoStream = 0;
	m_pcAudioCodec = m_pcVideoCodec = NULL;
	m_pcAudioOutput = m_pcVideoOutput = NULL;
	m_bPlayThread = false;
	m_bFullScreen = false;

	/* Create media manager */
	m_pcManager = os::MediaManager::Get();

	if ( !m_pcManager->IsValid() )
	{
		std::cout << "Media server is not running" << std::endl;
		PostMessage( os::M_QUIT );
		return;
	}

	/* Create window */
	m_pcWin = new MPWindow( os::Rect( 100, 100, 400, 400 ), "mp_window", MSG_MAINWND_TITLE );
	m_pcWin->Show();
	m_pcWin->MakeFocus( true );
	
	/* Open file if neccessary */
	if ( bLoad )
	{
		os::MediaInput * pcInput = m_pcManager->GetBestInput( zFileName );
		if ( pcInput != NULL )
		{
			Open( zFileName, pcInput->GetIdentifier() );
			delete( pcInput );
		}
	}
	
	

}

MPApp::~MPApp()
{
	/* Close and delete everything */
	if ( m_pcManager->IsValid() )
	{
		Close();
	}
	m_pcManager->Put();
}

class AVSyncTime : public os::MediaTimeSource
{
public:
	AVSyncTime( os::MediaOutput* pcOutput ) { m_pcOutput = pcOutput; m_nTime = 0; }
	void SetTime( bigtime_t nTime ) { m_nTime = nTime; }
	os::String GetIdentifier() { return( "Media Player AV Sync" ); }
	bigtime_t GetCurrentTime() { return( m_nTime - m_pcOutput->GetDelay() ); }
private:
	os::MediaOutput* m_pcOutput;
	bigtime_t m_nTime;
};

/* Thread which is responsible to play the file */
void MPApp::PlayThread()
{
	bigtime_t nTime = get_system_time();

	m_bPlayThread = true;
	bool bGrab;
	bool bNoGrab;
	bool bStarted = false;

	os::MediaPacket_s sPacket;
	os::MediaPacket_s sAudioPacket;
	os::MediaPacket_s sVideoFrame;
	uint64 nVideoFrames = 0;
	uint64 nAudioBytes = 0;
	uint8 nErrorCount = 0;
	bool bError = false;
	uint32 nGrabValue = 80;
	bool bDoubleDraw = false;
	bool bSkipFrame = false;
	bigtime_t nLastVideo = 0;
	bigtime_t nLastAudio = 0;
	AVSyncTime cAVTime( m_pcAudioOutput ); 
	
	std::cout << "Play thread running" << std::endl;
	/* Seek to last position */
	if ( !m_bStream )
		m_pcInput->Seek( m_nLastPosition );
	/* Create audio output packet */
	if ( m_bPacket && m_bAudio )
	{
		m_pcAudioCodec->CreateAudioOutputPacket( &sAudioPacket );
	}
	/* Create video output frame */
	if ( m_bPacket && m_bVideo )
	{
		m_pcVideoCodec->CreateVideoOutputPacket( &sVideoFrame );
	}
	/* Set sync object */
	if( m_bVideo && m_bAudio )
		m_pcVideoOutput->SetTimeSource( &cAVTime );
	while ( m_bPlayThread )
	{
		if ( m_bPacket )
		{
		      grab:
			/* Look if we have to grab data */
			bGrab = false;
			bNoGrab = false;
			if ( m_bVideo )
			{
				if ( m_pcVideoOutput->GetUsedBufferPercentage() < nGrabValue )
					bGrab = true;
				if ( m_pcVideoOutput->GetUsedBufferPercentage() >= nGrabValue )
					bNoGrab = true;
			}
			if ( m_bAudio )
			{
				if ( m_pcAudioOutput->GetUsedBufferPercentage() < nGrabValue )
					bGrab = true;
				if ( m_pcAudioOutput->GetUsedBufferPercentage() >= nGrabValue )
					bNoGrab = true;
			}
			if ( bGrab && !bNoGrab )
			{
				/* Grab data */
				if ( m_pcInput->ReadPacket( &sPacket ) == 0 )
				{
					nErrorCount = 0;
					/* Decode audio data */
					if ( m_bAudio && sPacket.nStream == m_nAudioStream )
					{
						//std::cout<<"Audio "<<sPacket.nTimeStamp<<" "<<m_pcAudioOutput->GetUsedBufferPercentage()<<std::endl;
						if ( m_pcAudioCodec->DecodePacket( &sPacket, &sAudioPacket ) == 0 )
						{
							if ( sAudioPacket.nSize[0] > 0 )
							{
								nLastAudio = sAudioPacket.nTimeStamp;
								
								sAudioPacket.nTimeStamp = ~0;
								m_pcAudioOutput->WritePacket( 0, &sAudioPacket );
								nAudioBytes += sAudioPacket.nSize[0];
								cAVTime.SetTime( nLastAudio );
							}
						}
					}
					if ( m_bVideo && sPacket.nStream == m_nVideoStream )
					{
						//std::cout<<"Video "<<sPacket.nTimeStamp<<" "<<m_pcVideoOutput->GetUsedBufferPercentage()<<std::endl;
						if ( m_pcVideoCodec->DecodePacket( &sPacket, &sVideoFrame ) == 0 )
							if ( sVideoFrame.nSize[0] > 0 )
							{
								if ( !bSkipFrame )
								{
									nLastVideo = sVideoFrame.nTimeStamp;
									m_pcVideoOutput->WritePacket( 0, &sVideoFrame );
									if ( bDoubleDraw )
									{
										m_pcVideoOutput->WritePacket( 0, &sVideoFrame );
										bDoubleDraw = false;
									}
								}
								nVideoFrames++;
								bSkipFrame = false;
							}
					}
					m_pcInput->FreePacket( &sPacket );
					goto grab;
				}
				else
				{
					/* Increase error count */
					nErrorCount++;
					if ( ( m_bVideo && m_pcVideoOutput->GetDelay() > 0 ) || ( m_bAudio && m_pcAudioOutput->GetDelay(  ) > 0 ) )
						nErrorCount--;
					
					if ( nErrorCount > 10 && !bError )
					{
						os::Application::GetInstance()->PostMessage( MP_GUI_STOP, os::Application::GetInstance(  ) );
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
					std::cout << "Go" << std::endl;
				}
			}
			/* If we have started then flush the media data */
			if ( bStarted )
			{
				if ( m_bAudio )
					m_pcAudioOutput->Flush();
				if ( m_bVideo )
					m_pcVideoOutput->Flush();
				
			}

		}
		snooze( 1000 );

		if ( !m_bStream && get_system_time() > nTime + 1000000 )
		{
			/* Move slider */
			m_pcWin->GetSlider()->SetValue( os::Variant( ( int )( m_pcInput->GetCurrentPosition(  ) * 1000 / m_pcInput->GetLength(  ) ) ), false );
			//cout<<"Position "<<m_pcInput->GetCurrentPosition()<<endl;
			if ( m_bPacket && m_bVideo && m_bAudio )
			{
#if 0				
				uint64 nVideo = ( ( uint64 )nVideoFrames * 1000 / ( uint64 )m_sVideoFormat.vFrameRate - m_pcVideoOutput->GetDelay() );
				uint64 nAudio = ( ( uint64 )nAudioBytes * 1000 / 2 / ( uint64 )m_sAudioFormat.nSampleRate  / m_sAudioFormat.nChannels - m_pcAudioOutput->GetDelay() );

				std::cout<<"Video "<<nVideo<<" "<<nLastVideo<<" Audio "<<nAudio<<" "<<nLastAudio<<std::endl;

				if ( nVideo > nAudio )
				{
					std::cout << "AV delay ( Video ahead ): " << nVideo - nAudio << std::endl;
					if ( nVideo - nAudio > 100 )
					{
						std::cout << "Draw frame two times" << std::endl;
						bDoubleDraw = true;
					}
				}
				else
				{
					std::cout << "AV delay ( Audio ahead ): " << nAudio - nVideo << std::endl;
					if ( nAudio - nVideo > 100 )
					{
						std::cout << "Skip frame" << std::endl;
						bSkipFrame = true;
					}
				}
#endif
			}

			nTime = get_system_time();
			/* For non packet based devices check if we have finished */
			if ( !m_bPacket && m_nLastPosition == m_pcInput->GetCurrentPosition() )
			{
				nErrorCount++;
				if ( nErrorCount > 2 && !bError )
				{
					os::Application::GetInstance()->PostMessage( MP_GUI_STOP, os::Application::GetInstance(  ) );
					bError = true;
				}
			}
			else
				nErrorCount = 0;

			m_nLastPosition = m_pcInput->GetCurrentPosition();
		}
	}
	/* Stop tread */
	std::cout << "Stop thread" << std::endl;
	if ( !m_bPacket )
	{
		m_pcInput->StopTrack();
	}
	if ( m_bPacket && m_bVideo )
	{
		m_pcVideoOutput->Clear();
		m_pcVideoCodec->DeleteVideoOutputPacket( &sVideoFrame );
	}
	if ( m_bPacket && m_bAudio )
	{
		m_pcAudioOutput->Clear();
		m_pcAudioCodec->DeleteAudioOutputPacket( &sAudioPacket );
	}
}

int play_thread_entry( void *pData )
{
	MPApp *pcApp = ( MPApp * ) pData;

	pcApp->PlayThread();
	return ( 0 );
}

/* Open one file / device */
void MPApp::Open( os::String zFileName, os::String zInput )
{
	/* Look for the input */
	Close();
	uint32 i = 0;

	while ( ( m_pcInput = m_pcManager->GetInput( i ) ) != NULL )
	{
		if ( m_pcInput->GetIdentifier() == zInput )
		{
			break;
		}
		delete( m_pcInput );
		m_pcInput = NULL;
		i++;
	}
	
	if ( m_pcInput == NULL )
	{
		/* This should not happen! */
		return;
	}
	
	/* Open input */
	if ( m_pcInput->Open( zFileName ) != 0 )
	{
		/* This should not happen! */
		return;
	}
	
	m_bPacket = m_pcInput->PacketBased();
	m_bStream = m_pcInput->StreamBased();

	std::cout << zFileName.c_str() << std::endl;
	std::cout << zInput.c_str() << std::endl;
	if ( m_bPacket )
		std::cout << "Packet based" << std::endl;
	if ( m_bStream )
		std::cout << "Stream based" << std::endl;

	/* Save track information */
	m_nTrackCount = m_pcInput->GetTrackCount();

	std::cout << m_nTrackCount << " Tracks" << std::endl;

	/* REMOVE!!! */
	for ( i = 0; i < m_nTrackCount; i++ )
	{
		m_pcInput->SelectTrack( i );
		std::cout << "Track " << i << " Length: " << m_pcInput->GetLength() << std::endl;
		if ( m_bPacket )
		{
			for ( uint32 j = 0; j < m_pcInput->GetStreamCount(); j++ )
			{
				if ( m_pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_VIDEO )
				{
					std::cout << "Stream " << j << " " << m_pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
					std::cout << "BitRate: " << m_pcInput->GetStreamFormat( j ).nBitRate << std::endl;
					std::cout << "Size: " << m_pcInput->GetStreamFormat( j ).nWidth << "x" << m_pcInput->GetStreamFormat( j ).nHeight << std::endl;
					std::cout << "Framerate: " << m_pcInput->GetStreamFormat( j ).vFrameRate << std::endl;
				}
				if ( m_pcInput->GetStreamFormat( j ).nType == os::MEDIA_TYPE_AUDIO )
				{
					std::cout << "Stream " << j << " " << m_pcInput->GetStreamFormat( j ).zName.c_str() << std::endl;
					std::cout << "BitRate: " << m_pcInput->GetStreamFormat( j ).nBitRate << std::endl;
					std::cout << "Channels: " << m_pcInput->GetStreamFormat( j ).nChannels << std::endl;
				}
			}
		}
	}

	/* Select track 0 */
	m_nTrack = m_pcInput->SelectTrack( 0 );
	/* Select right streams */
	if ( m_bPacket )
	{
		for ( i = 0; i < m_pcInput->GetStreamCount(); i++ )
		{
			if ( m_pcInput->GetStreamFormat( i ).nType == os::MEDIA_TYPE_VIDEO && !m_bVideo )
			{
				m_nVideoStream = i;
				m_sVideoFormat = m_pcInput->GetStreamFormat( i );
				m_bVideo = true;
			}
			if ( m_pcInput->GetStreamFormat( i ).nType == os::MEDIA_TYPE_AUDIO && !m_bAudio )
			{
				m_nAudioStream = i;
				m_sAudioFormat = m_pcInput->GetStreamFormat( i );
				m_bAudio = true;
			}
		}
	}

	/* Open output devices */
	if ( m_bVideo )
	{
		os::String zParameters = "";

		m_pcVideoOutput = m_pcManager->GetDefaultVideoOutput();
		if ( m_pcVideoOutput == NULL || ( m_pcVideoOutput && m_pcVideoOutput->FileNameRequired() ) || ( m_pcVideoOutput && m_pcVideoOutput->Open( zParameters ) != 0 ) )
		{
			if ( m_pcVideoOutput )
			{
				delete( m_pcVideoOutput );
				m_pcVideoOutput = NULL;
			}
			m_bVideo = false;
		}
		else
			std::cout << "Using Video Out " << m_pcVideoOutput->GetIdentifier().c_str(  ) << std::endl;
	}
	if ( m_bAudio )
	{
		m_pcAudioOutput = m_pcManager->GetDefaultAudioOutput();
		if ( m_pcAudioOutput == NULL || ( m_pcAudioOutput && m_pcAudioOutput->FileNameRequired() ) || ( m_pcAudioOutput && m_pcAudioOutput->Open( "" ) != 0 ) )
		{
			if ( m_pcAudioOutput )
			{
				delete( m_pcAudioOutput );
				m_pcAudioOutput = NULL;
			}
			m_bAudio = false;
		}
		else
			std::cout << "Using Audio Out " << m_pcAudioOutput->GetIdentifier().c_str(  ) << std::endl;
	}

	/* Open video codec */
	if ( m_bVideo )
	{
		for ( i = 0; i < m_pcVideoOutput->GetOutputFormatCount(); i++ )
		{
			if ( ( m_pcVideoCodec = m_pcManager->GetBestCodec( m_sVideoFormat, m_pcVideoOutput->GetOutputFormat( i ), false ) ) != NULL )
				if ( m_pcVideoCodec->Open( m_sVideoFormat, m_pcVideoOutput->GetOutputFormat( i ), false ) == 0 )
					break;
				else
				{
					delete( m_pcVideoCodec );
					m_pcVideoCodec = NULL;
				}
		}
		if ( m_pcVideoCodec == NULL || m_pcVideoOutput->AddStream( os::String( os::Path( zFileName.c_str() ).GetLeaf() ), m_pcVideoCodec->GetExternalFormat() ) != 0 )
		{
			m_bVideo = false;
			m_pcVideoOutput->Close();
		}
		else
		{
			std::cout << "Using Video codec " << m_pcVideoCodec->GetIdentifier().c_str(  ) << std::endl;
		}
	}
	/* Open audio codec */
	if ( m_bAudio )
	{
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
		if ( m_pcAudioCodec == NULL || m_pcAudioOutput->AddStream( os::String( os::Path( zFileName.c_str() ).GetLeaf() ), m_pcAudioCodec->GetExternalFormat() ) != 0 )
		{
			m_bAudio = false;
			m_pcAudioOutput->Close();
		}
		else
		{
			std::cout << "Using Audio codec " << m_pcAudioCodec->GetIdentifier().c_str(  ) << std::endl;
		}
	}

	if ( m_bPacket && !m_bAudio && !m_bVideo )
	{
		Close();
		os::Alert * pcAlert = new os::Alert( MSG_ERRWND_TITLE, MSG_ERRWND_CANTPLAY, os::Alert::ALERT_WARNING, 0, MSG_ERRWND_OK.c_str(), NULL );
		pcAlert->Go();
		return;
	}
	/* Fill tracks menu */
	while( m_pcWin->GetTracksMenu()->GetItemCount() > 0 ) { 
		os::MenuItem* pcItem = m_pcWin->GetTracksMenu()->RemoveItem( 0 ); 
		delete( pcItem );
	}
	for( int i = 0; i < ( int )m_pcInput->GetTrackCount(); i++ )
	{
		char zTest[30];
		
		sprintf( zTest, "%s %i", MSG_TRACK_MENU_ITEM.c_str(), i + 1 );
		os::Message * pcItemMessage = new os::Message( MP_GUI_TRACK_SELECTED );
		pcItemMessage->AddInt32( "track", i );
		m_pcWin->GetTracksMenu()->AddItem( zTest, pcItemMessage );
	}
	m_pcWin->GetTracksMenu()->SetTargetForItems( this );
	
	/* Finished! */
	m_bFileLoaded = true;
	m_zFileName = zFileName;
	m_nLastPosition = 0;
	if ( m_pcInput->FileNameRequired() )
		m_pcWin->SetTitle( zFileName + " - " + MSG_MAINWND_TITLE );
	else
		m_pcWin->SetTitle( m_pcInput->GetIdentifier() + " - " + MSG_MAINWND_TITLE );

	m_pcWin->PostMessage( MP_STATE_CHANGED, m_pcWin );

	/* Tell window to create space for video */
	if ( m_bVideo && m_pcVideoOutput->GetVideoView( 0 ) != NULL )
	{
		m_pcWin->SetVideo( m_pcVideoOutput->GetVideoView( 0 ) );
		os::Message cVideoMsg( MP_CREATE_VIDEO );
		cVideoMsg.AddInt32( "width", m_sVideoFormat.nWidth );
		cVideoMsg.AddInt32( "height", m_sVideoFormat.nHeight );
		m_pcWin->PostMessage( &cVideoMsg, m_pcWin );
	}

	/* Set slider */
	m_pcWin->GetSlider()->SetValue( os::Variant( 0 ) );

}

void MPApp::Close()
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
	if ( m_pcVideoOutput )
	{
		m_pcVideoOutput->Close();
		delete( m_pcVideoOutput );
		/* Delete video view */
		if ( m_pcWin )
		{
			m_pcWin->PostMessage( MP_DELETE_VIDEO, m_pcWin );
		}
		m_pcVideoOutput = NULL;

	}
	if ( m_pcAudioCodec )
	{
		m_pcAudioCodec->Close();
		delete( m_pcAudioCodec );
		m_pcAudioCodec = NULL;
	}
	if ( m_pcVideoCodec )
	{
		m_pcVideoCodec->Close();
		delete( m_pcVideoCodec );
		m_pcVideoCodec = NULL;
	}
	if ( m_pcInput )
	{
		m_pcInput->Close();
		delete( m_pcInput );
		m_pcInput = NULL;
	}
	m_bVideo = m_bAudio = false;
}

void MPApp::ChangeFullScreen()
{
	/* Look if fullscreen mode is enabled */
	if ( m_bFullScreen )
	{
		/* Restore old mode */
		m_pcWin->SetFrame( m_sSavedFrame );
		m_bFullScreen = false;
		return;
	}
	/* Look if fullscreen is possible */
	if ( m_bFileLoaded && m_bPacket && m_bVideo )
	{
		/* Change to fullscreen mode */
		os::Desktop * pcDesktop = new os::Desktop();
		os::IPoint sRes = pcDesktop->GetResolution();
		m_sSavedFrame = m_pcWin->GetFrame();
		m_pcWin->SetFrame( os::Rect( 0, 0, sRes.x - 1, sRes.y - 1 ) );
		m_bFullScreen = true;
	}
}

void MPApp::HandleMessage( os::Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case MP_GUI_PLAY_PAUSE:
		/* play or pause ( sent by the MPWindow class ) */
		if ( m_nState == MP_STATE_STOPPED && m_bFileLoaded )
		{
			m_nState = MP_STATE_PLAYING;
			m_pcWin->SetState( MP_STATE_PLAYING );
			m_pcWin->PostMessage( MP_STATE_CHANGED, m_pcWin );
			/* Start thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}


			m_hPlayThread = spawn_thread( "play_thread", (void*)play_thread_entry, 0, 0, this );
			resume_thread( m_hPlayThread );
		}
		else if ( m_nState == MP_STATE_PLAYING )
		{
			m_nState = MP_STATE_PAUSED;
			m_pcWin->SetState( MP_STATE_PAUSED );
			m_pcWin->PostMessage( MP_STATE_CHANGED, m_pcWin );
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
		}
		else if ( m_nState == MP_STATE_PAUSED )
		{
			m_nState = MP_STATE_PLAYING;
			m_pcWin->SetState( MP_STATE_PLAYING );
			m_pcWin->PostMessage( MP_STATE_CHANGED, m_pcWin );
			/* Start thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			m_nLastPosition = m_pcWin->GetSlider()->GetValue(  ).AsInt32(  ) * m_pcInput->GetLength(  ) / 1000;
			m_hPlayThread = spawn_thread( "play_thread", (void*)play_thread_entry, 0, 0, this );
			resume_thread( m_hPlayThread );
		}
		break;
	case MP_GUI_STOP:
		/* Stop ( sent by the MPWindow class ) */
		if ( m_nState != MP_STATE_STOPPED )
		{
			m_nState = MP_STATE_STOPPED;
			m_pcWin->SetState( MP_STATE_STOPPED );
			m_pcWin->PostMessage( MP_STATE_CHANGED, m_pcWin );
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			m_nLastPosition = 0;
			m_pcWin->GetSlider()->SetValue( os::Variant( 0 ) );
		}
		break;
	case MP_GUI_OPEN_INPUT:
		/* Open ( sent by the MPWindow class ) */
		if ( m_nState != MP_STATE_STOPPED )
		{
			m_nState = MP_STATE_STOPPED;
			m_pcWin->SetState( MP_STATE_STOPPED );
			m_pcWin->PostMessage( MP_STATE_CHANGED, m_pcWin );
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			m_nLastPosition = 0;
			m_pcWin->GetSlider()->SetValue( os::Variant( 0 ) );
		}
		/* Change fullscreen mode if necessary */
		if ( m_bFullScreen )
			ChangeFullScreen();
		if ( !m_pcInputSelector )
		{
			/* Open input selector */
			m_pcInputSelector = new os::MediaInputSelector( os::Point( 150, 150 ), "Open", new os::Messenger( this ), new os::Message( MP_IS_OPEN ), new os::Message( MP_IS_CANCEL ) );
			m_pcInputSelector->Show();
			m_pcInputSelector->MakeFocus( true );
		}
		else
		{
			m_pcInputSelector->MakeFocus( true );
		}
		break;
	case MP_GUI_SEEK:
		/* Seek ( sent by the MPWindow class ) */
		if ( m_nState == MP_STATE_PLAYING && !m_bStream )
		{
			/* Stop thread */
			if ( m_bPlayThread )
			{
				m_bPlayThread = false;
				wait_for_thread( m_hPlayThread );
			}
			/* Set new position */
			m_nLastPosition = m_pcWin->GetSlider()->GetValue(  ).AsInt32(  ) * m_pcInput->GetLength(  ) / 1000;
			m_hPlayThread = spawn_thread( "play_thread", (void*)play_thread_entry, 0, 0, this );
			resume_thread( m_hPlayThread );
		} else {
			m_pcWin->GetSlider()->SetValue( 0 );
		}
		break;
	case MP_GUI_TRACK_SELECTED:
		{
			/* Select one track ( sent by the menu ) */
			int32 nTrack;

			if ( pcMessage->FindInt32( "track", &nTrack ) == 0 )
			{
				bool bRestart = false;

				if ( m_nState == MP_STATE_PLAYING )
					bRestart = true;
				/* Stop thread */
				if ( m_bPlayThread )
				{
					m_bPlayThread = false;
					wait_for_thread( m_hPlayThread );

				}
				/* Select new track */
				m_pcInput->SelectTrack( nTrack );
				m_nLastPosition = 0;

				/* Start thread if neccessary */
				if ( bRestart )
				{
					m_hPlayThread = spawn_thread( "play_thread", (void*)play_thread_entry, 0, 0, this );
					resume_thread( m_hPlayThread );
				}
			}
		}
		break;
	case MP_GUI_FULLSCREEN:
		/* Change to / from fullscreen mode ( sent by the MPWindow class ) */
		ChangeFullScreen();
		break;
	case MP_IS_CANCEL:
		m_pcInputSelector = NULL;
		break;
	case MP_IS_OPEN:
		{
			/* Message sent by the input selector */
			os::String zFile;
			os::String zInput;

			if ( pcMessage->FindString( "file/path", &zFile.str() ) == 0 && pcMessage->FindString( "input", &zInput.str(  ) ) == 0 )
			{
				Open( zFile, zInput );
			}
			m_pcInputSelector = NULL;
		}
		break;
	case MP_UPDATE_VIDEO:
		/* Update ( sent by the MPWindow class ) */
		if ( m_bVideo && m_pcVideoOutput )
			m_pcVideoOutput->UpdateView( 0 );
		break;
	case os::M_LOAD_REQUESTED:
		/* Open file ( sent by the filerequester ) */
		{
			os::String zFile;
			if( pcMessage->FindString( "file/path", &zFile.str() ) == 0 ) {
		
				os::MediaInput * pcInput = m_pcManager->GetBestInput( zFile );
				if ( pcInput != NULL )
				{
					Open( zFile, pcInput->GetIdentifier() );
					delete( pcInput );
				}
			}
		}
		break;
	default:
		os::Application::HandleMessage( pcMessage );
		break;
	}
}


bool MPApp::OkToQuit()
{
	std::cout << "Quit" << std::endl;
	if ( m_pcManager->IsValid() )
	{
		Close();
		m_pcWin->Close();
	}
	return ( true );
}

int main( int argc, char *argv[] )
{
	MPApp *pcApp = NULL;

	if ( argc > 1 )
	{
		pcApp = new MPApp( "application/x-vnd.syllable-MediaPlayer", argv[1], true );
	}
	else
	{
		pcApp = new MPApp( "application/x-vnd.syllable-MediaPlayer", "", false );
	}

	pcApp->Run();
	return ( 0 );
}

































