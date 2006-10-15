/*  Media Server
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

#include "mediaserver.h"
#include "mediacontrols.h"
#include <gui/image.h>
#include <math.h>

using namespace os;


/* Window, which contains the media server controls */

MediaControls::MediaControls( MediaServer* pcServer, Rect cFrame )
			: Window( cFrame, "media_server_controls", "Media Controls" )
{
	m_pcServer = pcServer;
	
	Lock();
	
	
	/* Create Tabview */
	m_pcTabs = new TabView( GetBounds(), "tabs", CF_FOLLOW_ALL );
	
	/* Add views */
	for( uint i = 0; i < MEDIA_MAX_DSPS; i++ )
	{
		if( m_pcServer->m_sDsps[i].bUsed )
		{
			os::MediaOutput* pcOutput = m_pcServer->m_sDsps[i].pcAddon->GetOutput( m_pcServer->m_sDsps[i].nOutputStream );
			if( pcOutput == NULL )
				continue;
			os::View* pcView = pcOutput->GetConfigurationView();
			if( pcView )
			{
				m_apcOutputs.push_back( pcOutput );
				m_pcTabs->AppendTab( m_pcServer->m_sDsps[i].zName, pcView );
			}
			else
				pcOutput->Release();
		}
	}
	/* Create streams tab */

	/* Clear */
	for( uint32 i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ )
	{
		m_bStreamActive[i] = false;
	}
	
	/* Create stream barview */
	View* pcFrame = new View( os::Rect( 5, 0, cFrame.Width() - 5, 300 ), "streams" );

	m_pcStreamBar = new BarView( Rect( 5, 10, pcFrame->GetBounds().Width() - 10, 300 ) );
	m_pcStreamBar->SetStreamLabel( 0, "Master" );
	m_pcStreamBar->SetStreamVolume( 0, pcServer->GetMasterVolume() );
	m_pcStreamBar->SetStreamActive( 0, true );
	
	for( int i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ )
		StreamChanged( i );

	pcFrame->AddChild( m_pcStreamBar );
	/* Create tab */
	m_pcTabs->AppendTab( "Streams", pcFrame );
	
	AddChild( m_pcTabs );

	Unlock();
	
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	pcIcon->SetSize( os::Point( 24, 24 ) );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
}

MediaControls::~MediaControls()
{
	for( uint i = 0; i < m_apcOutputs.size(); i++ )
		m_apcOutputs[i]->Release();
}

bool MediaControls::OkToQuit()
{
	m_pcServer->ControlsHidden( GetFrame() );
	//Show( false );
	return( true );
}

void MediaControls::HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case MESSAGE_STREAM_VOLUME_CHANGED:
			{
			int8 nVolume;
			int32 nStream;
			if( !pcMessage->FindInt8( "volume", &nVolume ) && 
				!pcMessage->FindInt32( "stream", &nStream ) ) {
				if( nStream == 0 ) {
					m_pcServer->SetMasterVolume( nVolume );
					/* Master */
				} else {
					/* Stream */
					m_pcServer->GetAudioStream( nStream - 1 )->nVolume = nVolume;
				}
				//printf( "Volume %i %i\n", (int)nStream, nVolume );
			}
			
			}
		break;
		default:
			Window::HandleMessage( pcMessage );
	}

}
void MediaControls::SetMasterVolume( int nValue )
{
	m_pcStreamBar->SetStreamVolume( 0, nValue );
}


void MediaControls::StreamChanged( uint32 nNum )
{
	MediaAudioStream_s* psStream = m_pcServer->GetAudioStream( nNum );
	
	/* Show or hide stream bar if necessary */
	if( psStream->bUsed && !m_bStreamActive[nNum] )
	{
		m_bStreamActive[nNum] = true;
		m_pcStreamBar->SetStreamLabel( nNum + 1, psStream->zName );
		m_pcStreamBar->SetStreamVolume( nNum + 1, 100 );
		m_pcStreamBar->SetStreamActive( nNum + 1, true );
	}
	
	if( !psStream->bUsed && m_bStreamActive[nNum] )
	{
		m_bStreamActive[nNum] = false;
		m_pcStreamBar->SetStreamActive( nNum + 1, false );
		
	}
	
}

int MediaControls::FindMixers( const char *pzPath )
{
	return 0;
}


























































