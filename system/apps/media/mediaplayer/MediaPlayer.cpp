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
#include <cassert>
#include <gui/requesters.h>
using namespace os;

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

/* Background view class */

class MPBackgroundView : public os::View
{
public:
	MPBackgroundView( os::Rect cFrame ) : os::View( cFrame, "background", os::CF_FOLLOW_ALL )
	{
		SetFgColor( os::Color32_s( 0, 0, 0 ) );
	}
	void Paint( const os::Rect& cUpdate )
	{
		FillRect( cUpdate );
	}
};

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
	pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_INFO, new os::Message( MP_GUI_FILE_INFO ) );
	pcFileMenu->AddItem( MSG_MAINWND_MENU_FILE_FULLSCREEN, new os::Message( MP_GUI_FULLSCREEN ) );
	pcFileMenu->GetItemAt( 4 )->SetEnable( false );
	m_pcMenuBar->AddItem( pcFileMenu );
	
	m_pcTracksMenu = new os::Menu( os::Rect(), MSG_MAINWND_MENU_TRACKS, os::ITEMS_IN_COLUMN );
	m_pcMenuBar->AddItem( m_pcTracksMenu );
	
	cNewFrame = m_pcMenuBar->GetFrame();
	cNewFrame.right = GetBounds().Width();
	cNewFrame.bottom = cNewFrame.top + m_pcMenuBar->GetPreferredSize( false ).y - 1;
	m_pcMenuBar->SetFrame( cNewFrame );
	m_pcMenuBar->SetTargetForItems( this );
	
	/* Set window size */
	cNewFrame = cFrame;
	cNewFrame.right = cNewFrame.left + 300;
	cNewFrame.bottom = cNewFrame.top + 40 + m_pcMenuBar->GetPreferredSize( false ).y - 1;
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
	m_pcFileDialog->Start();
	
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
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
	case MP_GUI_FILE_INFO:
		/* Forward message to the MPApp class */
		os::Application::GetInstance()->PostMessage( MP_GUI_FILE_INFO, os::Application::GetInstance(  ) );
		break;
	case MP_GUI_FULLSCREEN:
		/* Forward message to the MPApp class */
		os::Application::GetInstance()->PostMessage( MP_GUI_FULLSCREEN, os::Application::GetInstance(  ) );
		break;
	case MP_GUI_OPEN:
		/* Open file selector */
		os::Application::GetInstance()->PostMessage( MP_GUI_STOP, os::Application::GetInstance(  ) );
		os::Application::GetInstance()->PostMessage( MP_GUI_OPEN, os::Application::GetInstance(  ) );
		if( !m_pcFileDialog->IsVisible() )
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
		cBodyText = os::String( "Media Player V1.2\n" ) + MSG_ABOUTWND_TEXT;
		
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
				/* Resize window if necessary */
				m_cVideoSize = os::IPoint( nWidth, nHeight );
				if ( cFrame.Width() < nWidth )
					cFrame.right = cFrame.left + nWidth - 1;
				cFrame.bottom = cFrame.top + nHeight + 40 + m_pcMenuBar->GetPreferredSize( false ).y - 1;

				/* Resize window to keep it on the desktop */
				os::Desktop cDesktop;
				float vDeltaX = 0;
				float vDeltaY = 0;
				if( cFrame.right >= cDesktop.GetResolution().x )
					vDeltaX = cFrame.right - cDesktop.GetResolution().x + 5;
				if( cFrame.bottom >= cDesktop.GetResolution().y )
					vDeltaY = cFrame.bottom - cDesktop.GetResolution().y + 5;
				m_pcVideo->ResizeBy( -vDeltaX, -vDeltaY );
				cFrame.right -= vDeltaX;
				cFrame.bottom -= vDeltaY;
				nHeight -= (int32)vDeltaY;
				
				
				SetFrame( cFrame );
				
				/* Create a black background view and add the video */
				os::View* pcView = new MPBackgroundView( os::Rect( 0, m_pcMenuBar->GetPreferredSize( false ).y, cFrame.Width(),
														m_pcMenuBar->GetPreferredSize( false ).y + nHeight - 1 ) );
				AddChild( pcView );
				pcView->AddChild( m_pcVideo );
				pcView->SetEraseColor( os::Color32_s( 0, 0, 0 ) );
				
				FrameSized( os::Point() ); /* Set the video frame correctly */
 				SetFlags( GetFlags() & ~os::WND_NOT_RESIZABLE );
				m_pcMenuBar->GetSubMenuAt( 1 )->GetItemAt( 4 )->SetEnable( true );
				
			}
		}
		break;
	case MP_DELETE_VIDEO:
		{
			/* Message sent by the MPApp class */
			os::Rect cFrame = GetFrame();
			if ( m_pcVideo )
			{
				delete( m_pcVideo->GetParent() );
			}
			m_pcVideo = NULL;
			cFrame.right = cFrame.left + 300;
			cFrame.bottom = cFrame.top + 40 + m_pcMenuBar->GetPreferredSize( false ).y;
			SetFrame( cFrame );
			SetFlags( GetFlags() | os::WND_NOT_RESIZABLE );
			m_pcMenuBar->GetSubMenuAt( 1 )->GetItemAt( 4 )->SetEnable( false );
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

void MPWindow::FrameSized( const os::Point & cDelta )
{
	/* Calculate the frame of the video view */	
	if( m_pcVideo == NULL || m_pcVideo->GetParent() == NULL )
		return;
	
	
	/* Try to use the full width of the window first, then the full height */
	float vWidth = m_pcVideo->GetParent()->GetBounds().Width() + 1;
	float vHeight = m_pcVideo->GetParent()->GetBounds().Height() + 1;
	
	int nTargetWidth = m_cVideoSize.x;
	int nTargetHeight = m_cVideoSize.y;
	
	if( m_cVideoSize.y * (int)vWidth / m_cVideoSize.x > vHeight )
	{
		nTargetWidth = m_cVideoSize.x * (int)vHeight / m_cVideoSize.y;
		nTargetHeight = (int)vHeight;
	}
	else
	{
		nTargetWidth = (int)vWidth;
		nTargetHeight = m_cVideoSize.y * (int)vWidth / m_cVideoSize.x;
	}
	
	int nXPos = ( (int)vWidth - nTargetWidth ) / 2;
	int nYPos = ( (int)vHeight - nTargetHeight ) / 2;
	
	/* Set frame */
	m_pcVideo->SetFrame( os::Rect( nXPos, nYPos, nXPos + nTargetWidth - 1, nYPos + nTargetHeight - 1 ) ); 
	if ( m_nState != MP_STATE_PLAYING )
		os::Application::GetInstance()->PostMessage( MP_UPDATE_VIDEO, os::Application::GetInstance(  ) );
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
		os::Alert* pcAlert = new os::Alert( MSG_ALERTWND_NOMEDIASERVER_TITLE, MSG_ALERTWND_NOMEDIASERVER_TEXT, Alert::ALERT_WARNING,0, MSG_ALERTWND_NOMEDIASERVER_OK.c_str(),NULL);
		pcAlert->Go();
		pcAlert->MakeFocus();
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
		os::Message cMsg( os::M_LOAD_REQUESTED );
		cMsg.AddString( "file/path", zFileName );
		PostMessage( &cMsg, this );
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


/* Thread which is responsible to play the file */
void MPApp::PlayThread()
{
	bigtime_t nTime = get_system_time();
	bigtime_t nPlayTime = 0;
	m_bPlayThread = true;
	bigtime_t nStartTime = 0;
	
	os::MediaPacket_s sPacket;
	os::MediaPacket_s sAudioPacket;
	os::MediaPacket_s sVideoFrame;
	bool bVideoValid = false;
	bool bAudioValid = false;
	uint64 nVideoFrames = 0;
	uint64 nAudioBytes = 0;
	uint32 nAudioPacketPos = 0;
	uint8 nErrorCount = 0;
	bool bError = false;
	bigtime_t nLastVideo = 0;
	bigtime_t nLastAudio = 0;
	bool bDropNextFrame = false;
	std::vector<os::MediaPacket_s> acPackets;
	
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
	
	while ( m_bPlayThread )
	{
		if ( m_bPacket )
		{
			bool bCheckError = false;
again:
			if( acPackets.size() < ( ( m_bAudio && m_bVideo ) ? 100 : 50 ) )
			{
				if( m_pcInput->ReadPacket( &sPacket ) == 0 )
				{
					if( ( ( m_bVideo && sPacket.nStream == m_nVideoStream ) ||
							( m_bAudio && sPacket.nStream == m_nAudioStream ) ) ) {
						//printf( "Read Packet at %i:%i:%i\n", (int)acPackets.size(), sPacket.nStream, (int)sPacket.nSize[0] );
						acPackets.push_back( sPacket );
					}
					else
						m_pcInput->FreePacket( &sPacket );
					goto again;
				}
				else
					bCheckError = true;
			}
//video_again:
			/* Read Video frame */
			if( m_bVideo && !bVideoValid )
			{
				for( uint i = 0; i < acPackets.size(); i++ )
				{
					if( acPackets[i].nStream == m_nVideoStream )
					{
						sPacket = acPackets[i];
						//printf("Found video packet at position %i\n", i );
						if( bDropNextFrame )
						{
							m_pcVideoCodec->ParsePacket( &sPacket, &sVideoFrame );
							bDropNextFrame = false;
						}
						else
						{
							if ( m_pcVideoCodec->DecodePacket( &sPacket, &sVideoFrame ) == 0 )
							{
								if ( sVideoFrame.nSize[0] > 0 )
								{
									bVideoValid = true;								
								}
							}
						}
						m_pcInput->FreePacket( &sPacket );
						acPackets.erase( acPackets.begin() + i );
						//if( !bVideoValid )
							//goto video_again;
						break;
					}
				}
			}
			
			/* Show videoframe */
			if( m_bVideo && bVideoValid )
			{
				if( !m_bAudio )
				{
					if( nStartTime == 0 )
						nStartTime = get_system_time() - sVideoFrame.nTimeStamp * 1000;
					nPlayTime = ( get_system_time() - nStartTime ) / 1000;
				}
				/* Calculate current time */
				bigtime_t nCurrentTime = nPlayTime;
				if( m_bAudio )
				 	nCurrentTime -= m_pcAudioOutput->GetDelay();
				 
				uint64 nVideoFrameLength = 1000 / (uint64)m_sVideoFormat.vFrameRate;
			 
				if( nCurrentTime > sVideoFrame.nTimeStamp + nVideoFrameLength/* * 2*/ )
				{
					printf( "Droping Frame %i %i!\n", (int)sVideoFrame.nTimeStamp, (int)nCurrentTime );
					bVideoValid = false;
					bDropNextFrame = true;
				}
				else if( !( nCurrentTime < sVideoFrame.nTimeStamp ) )
				{
					/* Write video frame */
					//printf( "Write video frame %i %i\n", (int)sVideoFrame.nTimeStamp, (int)nCurrentTime );
					nLastVideo = sVideoFrame.nTimeStamp;
					m_pcVideoOutput->WritePacket( 0, &sVideoFrame );	
					nVideoFrames++;
					bVideoValid = false;						
				}
			}
audio_again:			
			/* Read audio packet */
			if( m_bAudio && !bAudioValid )
			{
				for( uint i = 0; i < acPackets.size(); i++ )
				{
					if( acPackets[i].nStream == m_nAudioStream )
					{
						//printf("Found audio packet at position %i\n", i );
						sPacket = acPackets[i];
						if ( m_pcAudioCodec->DecodePacket( &sPacket, &sAudioPacket ) == 0 )
						{
							if ( sAudioPacket.nSize[0] > 0 )
							{
								nAudioPacketPos = 0;
								bAudioValid = true;
							}
						} else
							printf( "Audio decode error!\n" );
						m_pcInput->FreePacket( &sPacket );
						acPackets.erase( acPackets.begin() + i );
						break;
					}
				}
			}
			
			/* Write audio */

			if( m_bAudio && bAudioValid )
			{
				uint64 nAudioBufferSize= m_pcAudioOutput->GetBufferSize( true );
				uint32 nFreeAudioBufSize = nAudioBufferSize - m_pcAudioOutput->GetDelay( true );
				
				/* We only write as much bytes as the free buffer size because we 
				do not want to block here */
				if( nAudioBufferSize <= 80 || ( nFreeAudioBufSize > 40 ) )
				{
					uint32 nFreeAudioBufBytes = (uint32)( nFreeAudioBufSize * bigtime_t( 2 * m_sAudioOutFormat.nChannels * m_sAudioOutFormat.nSampleRate ) / 1000 );
					nFreeAudioBufBytes -= nFreeAudioBufBytes % ( 2 * m_sAudioOutFormat.nChannels );
					uint32 nWriteBytes = std::min( nFreeAudioBufBytes, sAudioPacket.nSize[0] - nAudioPacketPos );

					bigtime_t nAudioLength = (bigtime_t)nWriteBytes * 1000;
					nAudioLength /= bigtime_t( 2 * m_sAudioOutFormat.nChannels * m_sAudioOutFormat.nSampleRate );
					
					//printf( "%i %i %i %i %i %i %i\n", (int)m_sAudioOutFormat.nChannels, (int)nAudioBufferSize, (int)nFreeAudioBufSize, sAudioPacket.nSize[0], nAudioPacketPos, nWriteBytes, (int)nAudioLength );
					
					if( nAudioPacketPos == 0 )
						if( sAudioPacket.nTimeStamp != ~0 ) {
							nLastAudio = sAudioPacket.nTimeStamp;
							//printf("Audio time stamp %i\n", (int)nLastAudio );
						}
					
					nLastAudio += nAudioLength;
					nAudioBytes += nWriteBytes;
					
					os::MediaPacket_s sWritePacket = sAudioPacket;
					sWritePacket.pBuffer[0] += nAudioPacketPos;
					sWritePacket.nSize[0] = nWriteBytes;
					
					m_pcAudioOutput->WritePacket( 0, &sWritePacket );

					nPlayTime = nLastAudio;
					
					nAudioPacketPos += nWriteBytes;
					
					if( nAudioPacketPos == sAudioPacket.nSize[0] )
					{
						//printf( "Packet complete!\n" );
						bAudioValid = false;
					}
					
					if( m_pcAudioOutput->GetDelay( true ) < m_pcAudioOutput->GetBufferSize( true ) / 4 )
						goto audio_again;
				}
			}

			/* Increase error count */
			if( bCheckError )
			{
					nErrorCount++;
					if ( ( m_bVideo && m_pcVideoOutput->GetDelay( true ) > 0 ) || ( m_bAudio && m_pcAudioOutput->GetDelay( true ) > 0 ) )
						nErrorCount--;
					if ( nErrorCount > 10 && !bError )
					{
						os::Application::GetInstance()->PostMessage( MP_GUI_STOP, os::Application::GetInstance(  ) );
						bError = true;
					}
			} 	
		}

		if( ( !m_bVideo || bVideoValid ) && ( !m_bAudio || bAudioValid ) )		
			snooze( 1000 );

		if ( !m_bStream && get_system_time() > nTime + 1000000 )
		{
			/* Move slider */
			m_pcWin->GetSlider()->SetValue( os::Variant( ( int )( m_pcInput->GetCurrentPosition(  ) * 1000 / m_pcInput->GetLength(  ) ) ), false );
			//cout<<"Position "<<m_pcInput->GetCurrentPosition()<<endl;
	
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
	else
	{
		m_pcInput->Clear();
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
		m_pcInput->Release();
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
		std::cout << "Track " << i << ": " << m_pcInput->GetAuthor().c_str() << " - " << m_pcInput->GetTitle().c_str() << " Length: " << m_pcInput->GetLength() << std::endl;
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

	std::cout << "Found video stream at #" << m_nVideoStream << ", audio stream at #" << m_nAudioStream << std::endl;

	/* Open output devices */
	if ( m_bVideo )
	{
		os::String zParameters = "-fb";

		m_pcVideoOutput = m_pcManager->GetDefaultVideoOutput();
		if ( m_pcVideoOutput == NULL || ( m_pcVideoOutput && m_pcVideoOutput->FileNameRequired() ) || ( m_pcVideoOutput && m_pcVideoOutput->Open( zParameters ) != 0 ) )
		{
			if ( m_pcVideoOutput )
			{
				m_pcVideoOutput->Release();
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
				m_pcAudioOutput->Release();
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
					m_pcVideoCodec->Release();
					m_pcVideoCodec = NULL;

					std::cout << "Failed to find video codec" << std::endl;
				}
		}
		if ( m_pcVideoCodec == NULL || m_pcVideoOutput->AddStream( os::String( os::Path( zFileName.c_str() ).GetLeaf() ), m_pcVideoCodec->GetExternalFormat() ) != 0 )
		{
			m_bVideo = false;
			m_pcVideoOutput->Close();

			std::cout << "Failed to add video stream to video output" << std::endl;
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
					m_pcAudioCodec->Release();
					m_pcAudioCodec = NULL;
				}
		}
		if ( m_pcAudioCodec == NULL || m_pcAudioOutput->AddStream( os::String( os::Path( zFileName.c_str() ).GetLeaf() ), m_pcAudioCodec->GetExternalFormat() ) != 0 )
		{
			m_bAudio = false;
			m_pcAudioOutput->Close();
			m_pcAudioOutput = NULL;
		}
		else
		{
			m_sAudioOutFormat = m_pcAudioCodec->GetExternalFormat();
			std::cout << "Using Audio codec " << m_pcAudioCodec->GetIdentifier().c_str(  ) << std::endl;
		}
	}

	if ( m_bPacket && !m_bAudio && !m_bVideo )
	{
		Close();
		os::Alert * pcAlert = new os::Alert( MSG_ERRWND_TITLE, MSG_ERRWND_CANTPLAY, os::Alert::ALERT_WARNING, 0, MSG_ERRWND_OK.c_str(), NULL );
		pcAlert->Go( new os::Invoker( 0 ) );
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
		printf( "%ix%i\n", m_sVideoFormat.nWidth, m_sVideoFormat.nHeight );
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
		printf("Close audio output!\n");
		m_pcAudioOutput->Close();
		m_pcAudioOutput->Release();
		m_pcAudioOutput = NULL;
	}
	if ( m_pcVideoOutput )
	{
		m_pcVideoOutput->Close();
		m_pcVideoOutput->Release();
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
		m_pcAudioCodec->Release();
		m_pcAudioCodec = NULL;
	}
	if ( m_pcVideoCodec )
	{
		m_pcVideoCodec->Close();
		m_pcVideoCodec->Release();
		m_pcVideoCodec = NULL;
	}
	if ( m_pcInput )
	{
		m_pcInput->Close();
		m_pcInput->Release();
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
	case MP_GUI_OPEN:
		/* Open ( sent by the MPWindow class ) */
		/* Change fullscreen mode if necessary */
		if ( m_bFullScreen )
			ChangeFullScreen();
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
	case MP_GUI_FILE_INFO:
		/* Show information about the open file ( sent by the MPWindow class ) */
		{
			if( m_pcInput == NULL )
				break;
			char zTemp[255];
			os::String cBodyText;
			if( m_bVideo )
			{
				sprintf( zTemp, "%s: %s %ix%i, %ifps\n", MSG_INFOWND_VIDEO.c_str(), m_sVideoFormat.zName.c_str(), m_sVideoFormat.nWidth,
						m_sVideoFormat.nHeight, (int)m_sVideoFormat.vFrameRate );
				
				cBodyText += zTemp;
			}
			if( m_bAudio )
			{
				sprintf( zTemp, "%s: %s %i %s, %ihz\n", MSG_INFOWND_AUDIO.c_str(), m_sAudioFormat.zName.c_str(), m_sAudioFormat.nChannels,
						MSG_INFOWND_CHANNELS.c_str(), m_sAudioFormat.nSampleRate );
				
				cBodyText += zTemp;
			}
		
			os::Alert* pcInfo = new os::Alert( MSG_INFOWND_TITLE, cBodyText, os::Alert::ALERT_INFO, 
										os::WND_NOT_RESIZABLE, MSG_INFOWND_OK.c_str(), NULL );
			pcInfo->Go( new os::Invoker( 0 ) ); 
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
					pcInput->Release();
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

































