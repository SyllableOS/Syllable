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
#include <math.h>

using namespace os;


/* Window, which contains the media server controls */

MediaControls::MediaControls( MediaServer* pcServer, Rect cFrame )
			: Window( cFrame, "media_server_controls", "Media Controls" )
{
	m_pcServer = pcServer;
	uint32 nYPos = 10;

	Lock();
	
	/* Create Tabview */
	m_pcTabs = new TabView( GetBounds(), "tabs", CF_FOLLOW_ALL );

	/* Find all available mixer devices */
	m_nMixerCount = FindMixers( "/dev/sound/" );

	/* Try to open soundcard mixers */
	for( int j=0; j<m_nMixerCount; j++ )
	{
		/* Reset vertical position for new mixer tab */
		nYPos = 10;

		m_pcMixerDev[j].hMixerDev = open( m_pcMixerDev[j].zPath, O_RDWR );
		if( m_pcMixerDev[j].hMixerDev >= 0 )
		{
		
			mixer_info sInfo;
			ioctl( m_pcMixerDev[j].hMixerDev, SOUND_MIXER_INFO, &sInfo );
		
			/* Create frame */
			m_pcMixerDev[j].pcFrame = new View( os::Rect( 5, 5, cFrame.Width() - 5, 20 ), "soundcard" );
		
			/* Add channels */
			unsigned int nSetting;
			int nVolume;
			int nLeft, nRight;
			static const char *zSources[] = SOUND_DEVICE_LABELS;
			ioctl( m_pcMixerDev[j].hMixerDev, MIXER_READ(SOUND_MIXER_DEVMASK), &nSetting);
		
		
			for ( int i = 0; i < SOUND_MIXER_NRDEVICES; i++ ) {
				if ( nSetting & (1 << i) ) {
					if (ioctl( m_pcMixerDev[j].hMixerDev, MIXER_READ(i), &nVolume ) > -1 ) {
			
						m_pcMixerChannel[j][i] = new MixerChannel( Rect( 5, nYPos, 240, nYPos + 40 ), 
						zSources[i], this, i, j );
			
						nLeft  = nVolume & 0x00FF;
						nRight = (nVolume & 0xFF00) >> 8;
				
						m_pcMixerChannel[j][i]->SetLeftValue( nLeft );
						m_pcMixerChannel[j][i]->SetRightValue( nRight );
			
						if( nLeft == nRight )
							m_pcMixerChannel[j][i]->SetLockValue( 1 );
						else
							m_pcMixerChannel[j][i]->SetLockValue( 0 );
				
						m_pcMixerDev[j].pcFrame->AddChild( m_pcMixerChannel[j][i] );
						nYPos += 45;
					}
				}
			}
			/* Set framesize */
			Rect cNewFrame = m_pcMixerDev[j].pcFrame->GetFrame();
			cNewFrame.bottom = nYPos + 15;
			m_pcMixerDev[j].pcFrame->SetFrame( cNewFrame );
		
			/* Create tab */
			m_pcTabs->AppendTab( sInfo.name, m_pcMixerDev[j].pcFrame );
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
	m_pcStreamBar->SetStreamVolume( 0, 100 );
	m_pcStreamBar->SetStreamActive( 0, true );
	m_pcStreamBar->SetStreamValue( 0, 0 );

	pcFrame->AddChild( m_pcStreamBar );
	/* Create tab */
	m_pcTabs->AppendTab( "Streams", pcFrame );
	
	AddChild( m_pcTabs );
	
	cFrame.bottom = cFrame.top + nYPos + 45;
	SetFrame( cFrame );
	
	Unlock();
}

MediaControls::~MediaControls()
{
}

bool MediaControls::OkToQuit()
{
	m_pcServer->ControlsHidden( GetFrame() );
	Show( false );
	return( false );
}

void MediaControls::HandleMessage( Message* pcMessage )
{
	int8 nChannel, nMixer;
	int nLeft, nRight;
	switch( pcMessage->GetCode() )
	{
		case MESSAGE_LOCK_CHANGED:
			if( !pcMessage->FindInt8( "mixer", &nMixer ) ) {
				if( !pcMessage->FindInt8( "channel", &nChannel ) ) {
					if( m_pcMixerChannel[nMixer][nChannel]->GetLockValue() == 1 )
					{
						nLeft = m_pcMixerChannel[nMixer][nChannel]->GetLeftValue();
						nRight = m_pcMixerChannel[nMixer][nChannel]->GetRightValue();
					
						m_pcMixerChannel[nMixer][nChannel]->SetLeftValue( ( nLeft + nRight ) / 2 );
						m_pcMixerChannel[nMixer][nChannel]->SetRightValue( ( nLeft + nRight ) / 2 );
					}
				}
			}
		break;
		
		case MESSAGE_CHANNEL_CHANGED:
			bool bLeftSlider;
			int nSetting, nVolume;
			if( !pcMessage->FindInt8( "mixer", &nMixer ) ) {
				if( !pcMessage->FindInt8( "channel", &nChannel ) ) {

					nLeft = m_pcMixerChannel[nMixer][nChannel]->GetLeftValue();
					nRight = m_pcMixerChannel[nMixer][nChannel]->GetRightValue();
				
					if( m_pcMixerChannel[nMixer][nChannel]->GetLockValue() == 1 ) {
						if( !pcMessage->FindBool( "left", &bLeftSlider ) ) {
							if( bLeftSlider == true ) 
								nRight = nLeft;
							else 
								nLeft = nRight;
						}
					}	
				
					nSetting = ( nLeft & 0xFF ) + ( ( nRight & 0xFF ) << 8 );
					ioctl( m_pcMixerDev[nMixer].hMixerDev, MIXER_WRITE( nChannel ), &nSetting );
				
					ioctl( m_pcMixerDev[nMixer].hMixerDev, MIXER_READ( nChannel ), &nVolume );		
					nLeft  = nVolume & 0x00FF;
					nRight = (nVolume & 0xFF00) >> 8;
				
					m_pcMixerChannel[nMixer][nChannel]->SetLeftValue( nLeft );
					m_pcMixerChannel[nMixer][nChannel]->SetRightValue( nRight );
				}
			}
		break;
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

void MediaControls::SetMasterValue( float vValue )
{
	m_pcStreamBar->SetStreamValue( 0, sin( vValue * M_PI / 2 ) );
}

void MediaControls::SetStreamValue( uint32 nNum, float vValue )
{
	m_pcStreamBar->SetStreamValue( nNum + 1, sin( vValue * M_PI / 2 ) );
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
		m_pcStreamBar->SetStreamValue( nNum + 1, 0 );
	}
	
	if( !psStream->bUsed && m_bStreamActive[nNum] )
	{
		m_bStreamActive[nNum] = false;
		m_pcStreamBar->SetStreamActive( nNum + 1, false );
		
	}
	
}

int MediaControls::FindMixers( const char *pzPath )
{
	int nRet = 0, nMixerCount = 0;
	DIR *hAudioDir, *hMixerDir;
	struct dirent *hAudioDev, *hMixerNode;
	char *zCurrentPath = NULL;

	hAudioDir = opendir( pzPath );
	if( hAudioDir == NULL )
	{
		cout<<"Unable to open"<<pzPath<<endl;
		return nRet;
	}

	while( (hAudioDev = readdir( hAudioDir )) )
	{
		if( !strcmp( hAudioDev->d_name, "." ) || !strcmp( hAudioDev->d_name, ".." ) )
			continue;

		zCurrentPath = (char*)calloc( 1, strlen( pzPath ) + strlen( hAudioDev->d_name ) + 7 );
		if( zCurrentPath == NULL )
		{
			cout<<"Out of memory"<<endl;
			closedir( hAudioDir );
			return nRet;
		}

		zCurrentPath = strcpy( zCurrentPath, pzPath );
		zCurrentPath = strcat( zCurrentPath, hAudioDev->d_name );
		zCurrentPath = strcat( zCurrentPath, "/mixer" );

		/* Look for mixer device nodes for this device */
		hMixerDir = opendir( zCurrentPath );
		if( hMixerDir == NULL )
		{
			cout<<"Unable to open"<<zCurrentPath<<endl;
			free( zCurrentPath );
			continue;
		}

		while( (hMixerNode = readdir( hMixerDir )) )
		{
			char *zMixerPath = NULL;

			if( !strcmp( hMixerNode->d_name, "." ) || !strcmp( hMixerNode->d_name, ".." ) )
				continue;

			/* We have found a mixer device node */
			++nMixerCount;

			zMixerPath = (char*)calloc( 1, strlen( zCurrentPath ) + strlen( hMixerNode->d_name ) + 1 );
			if( zMixerPath == NULL )
			{
				cout<<"Out of memory"<<endl;
				closedir( hMixerDir );
				free( zCurrentPath );
				closedir( hAudioDir );
				return nRet;
			}

			zMixerPath = strcpy( zMixerPath, zCurrentPath );
			zMixerPath = strcat( zMixerPath, "/" );
			zMixerPath = strcat( zMixerPath, hMixerNode->d_name );

			/* Find next free mixer handle */
			for( int i=0; i<MEDIA_MAX_DSPS; i++ )
			{
				if( m_pcMixerDev[i].bUsed )
					continue;

				m_pcMixerDev[i].bUsed = true;
				strcpy( m_pcMixerDev[i].zPath, zMixerPath );
				break;
			}
			free( zMixerPath );
		}

		closedir( hMixerDir );
		free( zCurrentPath );
	}

	nRet = nMixerCount;
	closedir( hAudioDir );
	return nRet;
}








































