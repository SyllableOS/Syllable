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

using namespace os;

int media_flush_entry( void* pData )
{
	MediaServer* pcServer = ( MediaServer* )pData;
	pcServer->FlushThread();
	return( 0 );
}

MediaServer::MediaServer()
			: Application( "media_server" )
{
	/* Set default values */
	m_zDefaultInput = "";
	m_zDefaultAudioOutput = "Media Server Audio Output";
	m_zDefaultVideoOutput = "Video Overlay Output";
	m_zStartupSound = "startup.wav";
	
	/* Load Settings */
	os::Settings* pcSettings = new os::Settings( "mediaserver", String( getenv( "HOME" ) ) + "/Settings/" );
	//cout<<Application::GetInstance()->GetName().c_str()<<endl;
	if( pcSettings->Load() == 0 )
	{
		m_zDefaultInput = pcSettings->GetString( "default_input", "" );
		m_zDefaultAudioOutput = pcSettings->GetString( "default_audio_output", "Media Server Audio Output" );
		m_zDefaultVideoOutput = pcSettings->GetString( "default_video_output", "Video Overlay Output" );
		m_zStartupSound = pcSettings->GetString( "startup_sound", "startup.wav" );
		m_nDefaultDsp = pcSettings->GetInt32( "default_dsp", 0 );
	}
	delete( pcSettings );
	
	
	m_hLock = create_semaphore( "media_server_lock", 1, 0 );
	m_hClearLock = create_semaphore( "media_server_clear_lock", 1, 0 );
	
	/* Find available DSP devices */
	m_nDspCount = FindDsps( "/dev/sound/" );
	std::cout<<"Found "<<m_nDspCount<<" DSP's"<<std::endl;
	if( m_nDspCount < 0 )
		m_nDefaultDsp = -1;

	/* Create mix buffer */
	m_pMixBuffer = ( signed int* )malloc( MEDIA_SERVER_AUDIO_BUFFER * 2 );
	memset( m_pMixBuffer, 0, MEDIA_SERVER_AUDIO_BUFFER * 2 );
	
	/* Create value buffer */
	m_pValueBuffer = ( uint32* )malloc( MEDIA_SERVER_AUDIO_BUFFER * 2 );
	memset( m_pValueBuffer, 0, MEDIA_SERVER_AUDIO_BUFFER * 2 );
	
	m_nActiveStreamCount = 0;
	
	/* Clear stream memory */
	for( int i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ )
		m_sAudioStream[i].bUsed = false;
		
	/* Clear packet buffer */
	for( int i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ ) {
		m_nQueuedPackets[i] = 0;
		for( int j = 0; j < MEDIA_MAX_STREAM_PACKETS; j++ ) {
			m_psPacket[i][j] = NULL;
		}
	}
	/* Spawn flush thread */
	m_hThread = spawn_thread( "media_server_flush", (void*)media_flush_entry, 0, 0, this );
	resume_thread( m_hThread );
	
	
	m_nMasterValueCount = 0;
	m_vMasterValue = 0;
	m_nMasterVolume = 100;
	
	/* Create controls window */
	m_pcControls = new MediaControls( this, Rect( 50, 50, 350, 400 ) );
	m_cControlsFrame = m_pcControls->GetFrame();
	m_bControlsShown = false;
}

MediaServer::~MediaServer()
{
	/* Stop flush thread */
	m_bRunThread = false;
	wait_for_thread( m_hThread );
	delete_semaphore( m_hLock );
	delete_semaphore( m_hClearLock );
	free( m_pMixBuffer );
	free( m_pValueBuffer );
	/* Close window */
	m_pcControls->Close();
}

/* Save all settings */
void MediaServer::SaveSettings()
{
	/* Save */
	os::Settings* pcSettings = new os::Settings( "mediaserver", String( getenv( "HOME" ) ) + "/Settings/" );
	pcSettings->SetString( "default_input", m_zDefaultInput );
	pcSettings->SetString( "default_audio_output", m_zDefaultAudioOutput );
	pcSettings->SetString( "default_video_output", m_zDefaultVideoOutput );
	pcSettings->SetString( "startup_sound", m_zStartupSound );
	pcSettings->SetInt32( "default_dsp", m_nDefaultDsp );
	pcSettings->Save();
	delete( pcSettings );
}

int MediaServer::FindDsps( const char *pzPath )
{
	int nRet = 0, nDspCount = 0;
	DIR *hAudioDir, *hDspDir;
	struct dirent *hAudioDev, *hDspNode;
	char *zCurrentPath = NULL;

	hAudioDir = opendir( pzPath );
	if( hAudioDir == NULL )
	{
		std::cout<<"Unable to open"<<pzPath<<std::endl;
		return nRet;
	}

	while( (hAudioDev = readdir( hAudioDir )) )
	{
		if( !strcmp( hAudioDev->d_name, "." ) || !strcmp( hAudioDev->d_name, ".." ) )
			continue;

		std::cout<<"Found audio device "<<hAudioDev->d_name<<std::endl;
		zCurrentPath = (char*)calloc( 1, strlen( pzPath ) + strlen( hAudioDev->d_name ) + 5 );
		if( zCurrentPath == NULL )
		{
			std::cout<<"Out of memory"<<std::endl;
			closedir( hAudioDir );
			return nRet;
		}

		zCurrentPath = strcpy( zCurrentPath, pzPath );
		zCurrentPath = strcat( zCurrentPath, hAudioDev->d_name );
		zCurrentPath = strcat( zCurrentPath, "/dsp" );

		/* Look for DSP device nodes for this device */
		hDspDir = opendir( zCurrentPath );
		if( hDspDir == NULL )
		{
			std::cout<<"Unable to open"<<zCurrentPath<<std::endl;
			free( zCurrentPath );
			continue;
		}

		while( (hDspNode = readdir( hDspDir )) )
		{
			char *zDspPath = NULL;

			if( !strcmp( hDspNode->d_name, "." ) || !strcmp( hDspNode->d_name, ".." ) )
				continue;

			/* We have found a DSP device node */
			++nDspCount;

			zDspPath = (char*)calloc( 1, strlen( zCurrentPath ) + strlen( hDspNode->d_name ) + 1 );
			if( zDspPath == NULL )
			{
				printf( "Out of memory\n" );
				closedir( hDspDir );
				free( zCurrentPath );
				closedir( hAudioDir );
				return nRet;
			}

			zDspPath = strcpy( zDspPath, zCurrentPath );
			zDspPath = strcat( zDspPath, "/" );
			zDspPath = strcat( zDspPath, hDspNode->d_name );

			/* Find next free sound card handle */
			for( int i=0; i<MEDIA_MAX_DSPS; i++ )
			{
				if( m_sDsps[i].bUsed )
					continue;

				m_sDsps[i].bUsed = true;
				sprintf( m_sDsps[i].zName, "%s - %s", hAudioDev->d_name, hDspNode->d_name );
				strcpy( m_sDsps[i].zPath, zDspPath );

				std::cout<<"Added "<<zDspPath<<" with handle #"<<i<<std::endl;
				break;
			}
			free( zDspPath );
		}

		closedir( hDspDir );
		free( zCurrentPath );
	}

	nRet = nDspCount;
	closedir( hAudioDir );
	return nRet;
}

bool MediaServer::OpenSoundCard( int nDevice )
{
	/* Try to open the soundcard device */
	if( nDevice < 0 || nDevice > MEDIA_MAX_DSPS )
		return( false );

	m_hOSS = open( m_sDsps[nDevice].zPath, O_RDWR );
	if( m_hOSS >= 0 )
		std::cout<<"Soundcard detected"<<std::endl;	
	else
		return( false );
		
	/* Get buffer size */
	audio_buf_info sInfo;
	ioctl( m_hOSS, SNDCTL_DSP_GETOSPACE, &sInfo );
	m_nBufferSize = sInfo.bytes;

	std::cout<<"Using "<<m_nBufferSize<<" Bytes of soundcard buffer"<<std::endl;
		
	/* Set parameters */
	ioctl( m_hOSS, SNDCTL_DSP_RESET, NULL );
	int nVal = 44100;
	ioctl( m_hOSS, SNDCTL_DSP_SPEED, &nVal );
	nVal = 1;
	ioctl( m_hOSS, SNDCTL_DSP_STEREO, &nVal );
	nVal = AFMT_S16_LE;
	ioctl( m_hOSS, SNDCTL_DSP_SETFMT, &nVal );
	
	m_nMasterValueCount = 0;
	m_vMasterValue = 0;
	m_pcControls->SetMasterValue( 0 );
	return( true );
}

void MediaServer::CloseSoundCard()
{
	std::cout<<"Closing soundcard device"<<std::endl;
	m_nMasterValueCount = 0;
	m_vMasterValue = 0;
	m_pcControls->SetMasterValue( 0 );
	/* Close soundcard */
	if( m_hOSS >= 0 ) {
		ioctl( m_hOSS, SNDCTL_DSP_SYNC, NULL ); 
		close( m_hOSS );
	}
}

/* Thread which flushes the audio stream buffers */
void MediaServer::FlushThread()
{
	uint32 nSize = 0;
	MediaPacket_s *psNextPacket;
	m_bRunThread = true;
	while( m_bRunThread ) 
	{
		lock_semaphore( m_hClearLock );
		lock_semaphore( m_hLock );
		
		if( m_nActiveStreamCount > 0 ) 
		{
		nSize = 0;
	
		/* Go through all audio streams and look for the smallest amount of data */
		for( uint32 i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ ) 
		{
			if( m_sAudioStream[i].bUsed && m_sAudioStream[i].bActive ) {
				if( m_nQueuedPackets[i] > 0 ) {
					if( m_psPacket[i][0]->nSize[0] > 0 && ( m_psPacket[i][0]->nSize[0] < nSize || nSize == 0 ) ) {
						nSize = m_psPacket[i][0]->nSize[0];
					}
				}
			}
		}
		unlock_semaphore( m_hLock );
		
		/* We have data to flush */
		if( nSize > 0 ) 
		{
			//cout<<"Flushing "<<nSize<<endl;
			memset( m_pMixBuffer, 0, nSize * 2 );
			memset( m_pValueBuffer, 0, nSize * 2 );
			for( uint32 i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ ) 
			{
				if( m_sAudioStream[i].bUsed && m_sAudioStream[i].bActive && m_nQueuedPackets[i] > 0 ) {
					psNextPacket = m_psPacket[i][0];
					//cout<<"Flushing data of Stream "<<i<<" of "<<psNextPacket->nSize[0]<<endl;
					
					/* Write packet into mix buffer */
					signed short *pData = ( signed short* )psNextPacket->pBuffer[0];
					signed int *pMix = m_pMixBuffer;
					uint32* pValue = m_pValueBuffer;
					for( uint32 j = 0; j < nSize / 2; j++ ) {
						
						pMix[0] += (signed int)( ( pData[0] ) * m_sAudioStream[i].nVolume / 100 );
						if( ( uint32 )( ( uint16 )pData[0] * m_sAudioStream[i].nVolume / 100 ) > pValue[0] )
							pValue[0] = ( uint16 )pData[0] * m_sAudioStream[i].nVolume / 100;
						
						m_sAudioStream[i].vValue += ( ( float )( uint16 )pData[0] ) * ( float )m_sAudioStream[i].nVolume / 100;
		
						m_sAudioStream[i].nValueCount++;
						if( m_sAudioStream[i].nValueCount > 2500 ) {
							if( m_bControlsShown )
								m_pcControls->SetStreamValue( i, ( float )( m_sAudioStream[i].vValue / 2500 / 0xffff ) );
							m_sAudioStream[i].nValueCount = 0;
							m_sAudioStream[i].vValue = 0;
						
						}
						
						pMix++;
						pData++;
						pValue++;
					}
						
					if( nSize == psNextPacket->nSize[0] )
					{
						/* Move packet queue up */
						free( psNextPacket->pBuffer[0] );
						free( psNextPacket );
						m_nQueuedPackets[i]--;
						for( uint32 j = 0; j < m_nQueuedPackets[i]; j++ )
						{
							m_psPacket[i][j] = m_psPacket[i][j+1];
						}
						//cout<<"Audio :"<<m_nQueuedPackets[i]<<" packets left"<<endl;
					} else {
						/* Split packet */
						psNextPacket->nSize[0] -= nSize;
						memcpy( psNextPacket->pBuffer[0], psNextPacket->pBuffer[0] + nSize, psNextPacket->nSize[0] );
					}
					m_sAudioStream[i].nBufferPlayed = get_system_time() + nSize * 10 * 1000 / 441 / 2 / 2;
				}
			}
			
			/* Do signed int -> signed short conversion and update stream bars */
			signed int* pData = m_pMixBuffer;
			signed short* pMix = ( signed short* )m_pMixBuffer;
			uint32* pValue = m_pValueBuffer;
			
			for( uint32 i = 0; i < nSize / 2; i++ )
			{
				signed int nData = *pData * (signed int)m_nMasterVolume / 100;
				if( nData > 0xffff / 2 )
					*pMix = 0xffff / 2;
				else if( nData < -0xffff / 2 )
					*pMix = -0xffff / 2;
				else
					*pMix = (signed short)nData;
				
				
				m_vMasterValue += (float)( *pValue * m_nMasterVolume / 100 );
				m_nMasterValueCount++;
				if( m_nMasterValueCount > 2500 ) {
					float vValue = m_vMasterValue / 2500 / 0xffff;
					if( vValue > 1.0f )
						vValue = 1.0f;
					if( m_bControlsShown )
						m_pcControls->SetMasterValue( vValue );
					
					m_nMasterValueCount = 0;
					m_vMasterValue = 0;
				}
					
				pData++;
				pMix++;
				pValue++;
			}
			
			write( m_hOSS, m_pMixBuffer, nSize );
		} else {
			if( m_pcControls )
				m_pcControls->SetMasterValue( 0 );
		}
		lock_semaphore( m_hLock );
		} 
		unlock_semaphore( m_hLock );
		unlock_semaphore( m_hClearLock );
		snooze( 1000 );
	}
}

/* Returns the default input if it exists */
void MediaServer::GetDefaultInput( Message* pcMessage )
{
	Message pcReply( MEDIA_SERVER_OK );
	if( m_zDefaultInput == "" ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	pcReply.AddString( "input", m_zDefaultInput );
	pcMessage->SendReply( &pcReply );
}


/* Sets the default input */
void MediaServer::SetDefaultInput( Message* pcMessage )
{
	pcMessage->FindString( "input", &m_zDefaultInput.str() );
	SaveSettings();
}

/* Returns the default audio output if it exists */
void MediaServer::GetDefaultAudioOutput( Message* pcMessage )
{
	Message pcReply( MEDIA_SERVER_OK );
	if( m_zDefaultAudioOutput == "" ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	pcReply.AddString( "output", m_zDefaultAudioOutput );
	pcMessage->SendReply( &pcReply );
}


/* Returns the default video output if it exists */
void MediaServer::GetDefaultVideoOutput( Message* pcMessage )
{
	Message pcReply( MEDIA_SERVER_OK );
	if( m_zDefaultVideoOutput == "" ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	pcReply.AddString( "output", m_zDefaultVideoOutput );
	pcMessage->SendReply( &pcReply );
}

/* Sets the default audio output */
void MediaServer::SetDefaultAudioOutput( Message* pcMessage )
{
	pcMessage->FindString( "output", &m_zDefaultAudioOutput.str() );
	SaveSettings();
}


/* Sets the default video output */
void MediaServer::SetDefaultVideoOutput( Message* pcMessage )
{
	pcMessage->FindString( "output", &m_zDefaultVideoOutput.str() );
	SaveSettings();
}


/* Returns the startup sound */
void MediaServer::GetStartupSound( Message* pcMessage )
{
	Message pcReply( MEDIA_SERVER_OK );
	
	pcReply.AddString( "sound", m_zStartupSound );
	pcMessage->SendReply( &pcReply );
}

int play_startup_sound( void* pData );

/* Sets the startup sound */
void MediaServer::SetStartupSound( Message* pcMessage )
{
	String zSound;
	pcMessage->FindString( "sound", &zSound.str() );
	/* Play startup sound if it has changed */
	if( !( zSound == m_zStartupSound ) ) {
		m_zStartupSound = zSound;
		resume_thread( spawn_thread( "play_startup_sound", (void*)play_startup_sound, 0, 0, &m_zStartupSound ) );
	}
	SaveSettings();
	
}

/* Create one audio stream which will be output to the soundcard */
void MediaServer::CreateAudioStream( Message* pcMessage )
{
	/* Look for a free stream */
	int32 nHandle = -1;
	Message cReply( MEDIA_SERVER_OK );
	
	/* Open soundcard */
	if( m_nActiveStreamCount == 0 )
		if( !OpenSoundCard( m_nDefaultDsp ) ) {
			/* No soundcard found */
			pcMessage->SendReply( MEDIA_SERVER_ERROR );
			return;
		}
	
	/* Search free handle */
	for( int i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ )
	{
		if( !m_sAudioStream[i].bUsed ) {
			nHandle = i;
			break;
		}
	}
	
	if( nHandle < 0 ) {
		if( m_nActiveStreamCount == 0 )
			CloseSoundCard();
		/* No free stream found */
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	const char* pzName;
	if( pcMessage->FindString( "name", &pzName ) != 0 ||
		pcMessage->FindInt32( "channels", &m_sAudioStream[nHandle].nChannels ) != 0 ||
		pcMessage->FindInt32( "sample_rate", &m_sAudioStream[nHandle].nSampleRate ) != 0 ) {
		if( m_nActiveStreamCount == 0 )
			CloseSoundCard();
		/* Message not complete */
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	strcpy( m_sAudioStream[nHandle].zName, pzName );
	
	m_sAudioStream[nHandle].bActive = false;
	if( m_sAudioStream[nHandle].nChannels != 2 || m_sAudioStream[nHandle].nSampleRate != 44100 ) {
		m_sAudioStream[nHandle].bResample = true;
	} else {
		m_sAudioStream[nHandle].bResample = false;
	}
	
	/* Create area */
	m_sAudioStream[nHandle].pAddress = NULL;
	m_sAudioStream[nHandle].hArea = create_area( "media_server_audio", (void**)&m_sAudioStream[nHandle].pAddress,
										MEDIA_SERVER_AUDIO_BUFFER, AREA_FULL_ACCESS, AREA_NO_LOCK );
	if( m_sAudioStream[nHandle].hArea < 0 ) {
		if( m_nActiveStreamCount == 0 )
			CloseSoundCard();
		/* Area could not be created */
		m_sAudioStream[nHandle].bUsed = false;
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	std::cout<<"New Audio Stream "<<nHandle<<" ( "<<m_sAudioStream[nHandle].nChannels<<" Channels, "<<
							m_sAudioStream[nHandle].nSampleRate<<" Hz"<<(m_sAudioStream[nHandle].bResample
							? ", Resampling" : "")<<" )"<<std::endl;
	
	/* Send information back */
	cReply.AddInt32( "area", m_sAudioStream[nHandle].hArea );
	cReply.AddInt32( "handle", nHandle );
	pcMessage->SendReply( &cReply );
	
	m_sAudioStream[nHandle].nVolume = 100;
	m_sAudioStream[nHandle].nValueCount = 0;
	m_sAudioStream[nHandle].vValue = 0;
	
	/* Mark stream as used */
	lock_semaphore( m_hLock );
	
	m_sAudioStream[nHandle].bUsed = true;
	m_nActiveStreamCount++;
	unlock_semaphore( m_hLock );
	
	/* Tell controls window */
	m_pcControls->StreamChanged( nHandle ); 
}

/* Delete one audio stream */
void MediaServer::DeleteAudioStream( Message* pcMessage )
{
	int32 nHandle;
	/* Get handle */
	if( pcMessage->FindInt32( "handle", &nHandle ) != 0 ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	if( !m_sAudioStream[nHandle].bUsed ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	lock_semaphore( m_hLock );
	m_sAudioStream[nHandle].bUsed = false;
	m_nActiveStreamCount--;
	if( m_nActiveStreamCount == 0 )
		CloseSoundCard();
	unlock_semaphore( m_hLock );
	delete_area( m_sAudioStream[nHandle].hArea );
	
	std::cout<<"Delete Audio Stream "<<nHandle<<std::endl;
	
	pcMessage->SendReply( MEDIA_SERVER_OK );
	
	/* Tell controls window */
	m_pcControls->StreamChanged( nHandle ); 
}

uint32 MediaServer::Resample( os::MediaFormat_s sFormat, uint16* pDst, uint16* pSrc, uint32 nLength )
{
	uint32 nSkipBefore = 0;
	uint32 nSkipBehind = 0;
	bool bOneChannel = false;
	/* Calculate skipping ( I just guess what is right here ) */
	if( sFormat.nChannels == 1 ) {
		bOneChannel = true;
	} else if( sFormat.nChannels == 3 ) {
		nSkipBefore = 0;
		nSkipBehind = 1;
	} else if( sFormat.nChannels == 4 ) {
		nSkipBefore = 2;
		nSkipBehind = 0;
	} else if( sFormat.nChannels == 5 ) {
		nSkipBefore = 2;
		nSkipBehind = 1;
	} else if( sFormat.nChannels == 6 ) {
		nSkipBefore = 2;
		nSkipBehind = 2;
	} 
	
	/* Calculate sample factors */
	float vFactor = 44100 / (float)sFormat.nSampleRate;
	float vSrcActiveSample = 0;
	float vDstActiveSample = 0;
	float vSrcFactor = 0;
	float vDstFactor = 0;
	uint32 nSamples = 0;
	
	if( vFactor < 1 ) {
		vSrcFactor = 1;
		vDstFactor = vFactor;
		nSamples = nLength / sFormat.nChannels / 2;
	} else {
		vSrcFactor = 1 / vFactor;
		vDstFactor = 1;
		float vSamples = (float)nLength * vFactor / (float)sFormat.nChannels / 2;
		nSamples = (uint32)vSamples;
	}
	
	uint32 nSrcPitch;
	uint32 nDstPitch = 2;
	if( bOneChannel )
		nSrcPitch = nSkipBefore + 1 + nSkipBehind;
	else
		nSrcPitch = nSkipBefore + 2 + nSkipBehind;
	uint32 nBytes = 0;
	
	for( uint32 i = 0; i < nSamples; i++ ) 
	{
		uint16* pSrcWrite = &pSrc[nSrcPitch * (int)vSrcActiveSample];
		uint16* pDstWrite = &pDst[nDstPitch * (int)vDstActiveSample];
		/* Skip */
		pSrcWrite += nSkipBefore;
		
		if( bOneChannel )
		{
			*pDstWrite++ = *pSrcWrite;
			*pDstWrite++ = *pSrcWrite;
		} else {
			/* Channel 1 */
			*pDstWrite++ = *pSrcWrite++;
		
			/* Channel 2 */
			*pDstWrite++ = *pSrcWrite++;
		}
		vSrcActiveSample += vSrcFactor;
		vDstActiveSample += vDstFactor;
		nBytes += 4;
	}
	return( nBytes );
}

/* Flush one audio stream */
void MediaServer::FlushAudioStream( Message* pcMessage )
{
	int32 nSize;
	int32 nHandle;
	if( pcMessage->FindInt32( "handle", &nHandle ) != 0 ||
		pcMessage->FindInt32( "size", &nSize ) != 0 ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	/* Create new packet */
	MediaPacket_s* psQueuePacket = ( MediaPacket_s* )malloc( sizeof( MediaPacket_s ) );
	memset( psQueuePacket, 0, sizeof( MediaPacket_s ) );
	psQueuePacket->nSize[0] = nSize;
	if( !m_sAudioStream[nHandle].bResample )
	{
		psQueuePacket->pBuffer[0] = ( uint8* )malloc( nSize );
		memcpy( psQueuePacket->pBuffer[0], m_sAudioStream[nHandle].pAddress, psQueuePacket->nSize[0] );
	} else {
		uint64 nFullSize = ( uint64 )nSize * 2 * 44100;
		nFullSize /= ( uint64 )m_sAudioStream[nHandle].nChannels * (uint64)m_sAudioStream[nHandle].nSampleRate;
		psQueuePacket->pBuffer[0] = ( uint8* )malloc( (uint32) nFullSize + 100 );
		os::MediaFormat_s sFormat;
		sFormat.nSampleRate = m_sAudioStream[nHandle].nSampleRate;
		sFormat.nChannels = m_sAudioStream[nHandle].nChannels;
		psQueuePacket->nSize[0] = Resample( sFormat, ( uint16* )psQueuePacket->pBuffer[0], ( uint16* )m_sAudioStream[nHandle].pAddress, nSize );
	}
	lock_semaphore( m_hLock );
	
	if( m_nQueuedPackets[nHandle] > MEDIA_MAX_STREAM_PACKETS ) {
		unlock_semaphore( m_hLock );
		free( psQueuePacket->pBuffer[0] );
		free( psQueuePacket );
		std::cout<<"Packet buffer of Stream "<<nHandle<<" full"<<std::endl;
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	m_psPacket[nHandle][m_nQueuedPackets[nHandle]] = psQueuePacket;
	m_nQueuedPackets[nHandle]++;
	unlock_semaphore( m_hLock );
	//cout<<"Packet queued at position "<<m_nQueuedPackets[nHandle] - 1<<"( "<<
		//m_nQueuedPackets[nHandle] * 100 / MEDIA_MAX_STREAM_PACKETS<<"% of the buffer used )"<<endl;
	pcMessage->SendReply( MEDIA_SERVER_OK );
}

/* Get delay of the stream */
void MediaServer::GetDelay( Message* pcMessage )
{
	int nDataSize = 0;
	int nDelay;
	uint64 nTime;
	int32 nHandle;
	Message cReply( MEDIA_SERVER_OK );
	
	if( pcMessage->FindInt32( "handle", &nHandle ) != 0 ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	/* Get data size in the packet buffer */
	lock_semaphore( m_hLock );
	for( uint32 i = 0; i < m_nQueuedPackets[nHandle]; i++ ) 
	{
		nDataSize += m_psPacket[nHandle][i]->nSize[0];
	}
	/* Add the data in the soundcard buffer */
	ioctl( m_hOSS, SNDCTL_DSP_GETODELAY, &nDelay );
	if( m_sAudioStream[nHandle].nBufferPlayed > get_system_time() )
		nDataSize += nDelay;
	unlock_semaphore( m_hLock );
	nTime = nDataSize;
	nTime *= 1000;
	nTime /= ( 2 * 2 * 44100 );
	
	cReply.AddInt64( "delay", nTime );
	pcMessage->SendReply( &cReply );
}

/* Get used percentage of the audio stream */
void MediaServer::GetPercentage( Message* pcMessage )
{
	uint32 nValue;
	int32 nHandle;
	Message cReply( MEDIA_SERVER_OK );
	
	if( pcMessage->FindInt32( "handle", &nHandle ) != 0 ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	lock_semaphore( m_hLock );
	nValue = m_nQueuedPackets[nHandle] * 100 / MEDIA_MAX_STREAM_PACKETS;
	unlock_semaphore( m_hLock );
	cReply.AddInt32( "percentage", nValue );
	pcMessage->SendReply( &cReply );
}

/* Clear audio buffer of one stream */
void MediaServer::Clear( Message* pcMessage )
{
	int32 nHandle;
	if( pcMessage->FindInt32( "handle", &nHandle ) != 0 ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	/* Stop thread */
	lock_semaphore( m_hClearLock );
	for( uint32 i = 0; i < m_nQueuedPackets[nHandle]; i++ ) {
		free( m_psPacket[nHandle][i]->pBuffer[0] );
		free( m_psPacket[nHandle][i] );
	}
	m_nQueuedPackets[nHandle] = 0;
	m_sAudioStream[nHandle].bActive = false;
	m_sAudioStream[nHandle].nValueCount = 0;
	m_sAudioStream[nHandle].vValue = 0;
	m_pcControls->SetStreamValue( nHandle, 0 );
	
	/* Look if we are the only active stream */
	if( m_nActiveStreamCount == 1 ) {
		/* Reset to avoid playing the data which is still in the soundcard buffer */
		ioctl( m_hOSS, SNDCTL_DSP_RESET, NULL );
		int nVal = 44100;
		ioctl( m_hOSS, SNDCTL_DSP_SPEED, &nVal );
		nVal = 1;
		ioctl( m_hOSS, SNDCTL_DSP_STEREO, &nVal );
		nVal = AFMT_S16_LE;
		ioctl( m_hOSS, SNDCTL_DSP_SETFMT, &nVal );
	}
	
	unlock_semaphore( m_hClearLock );
	pcMessage->SendReply( MEDIA_SERVER_OK );
}


/* Start audio buffer of one stream */
void MediaServer::Start( Message* pcMessage )
{
	int32 nHandle;
	if( pcMessage->FindInt32( "handle", &nHandle ) == 0 ) 
	{
		m_sAudioStream[nHandle].bActive = true;
	}
}

void MediaServer::GetDspCount( Message* pcMessage )
{
	Message cReply( MEDIA_SERVER_OK );
	cReply.AddInt32( "dsp_count", m_nDspCount );
	pcMessage->SendReply( &cReply );
}

void MediaServer::GetDspInfo( Message* pcMessage )
{
	int32 nHandle;
	std::string cName, cPath;
	Message cReply( MEDIA_SERVER_OK );

	if( pcMessage->FindInt32( "handle", &nHandle ) != 0 ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}

	if( nHandle < 0 || nHandle > m_nDspCount ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}

	cName = m_sDsps[nHandle].zName;
	cReply.AddString( "name", cName );
	cPath = m_sDsps[nHandle].zPath;
	cReply.AddString( "path", cPath );
	pcMessage->SendReply( &cReply );
}

void MediaServer::GetDefaultDsp( Message* pcMessage )
{
	Message cReply( MEDIA_SERVER_OK );

	if( m_nDefaultDsp < 0 )
		pcMessage->SendReply( MEDIA_SERVER_ERROR );

	cReply.AddInt32( "handle", m_nDefaultDsp );
	pcMessage->SendReply( &cReply );
}

void MediaServer::SetDefaultDsp( Message* pcMessage )
{
	int32 nHandle;
	Message cReply( MEDIA_SERVER_OK );

	if( pcMessage->FindInt32( "handle", &nHandle ) != 0 ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}

	if( nHandle < 0 || nHandle > MEDIA_MAX_DSPS ) {
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	lock_semaphore( m_hLock );
	m_nDefaultDsp = nHandle;
	unlock_semaphore( m_hLock );
	pcMessage->SendReply( &cReply );
}

void MediaServer::HandleMessage( Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case MEDIA_SERVER_PING:
			if( pcMessage->IsSourceWaiting() )
				pcMessage->SendReply( MEDIA_SERVER_OK );
		break;
		case MEDIA_SERVER_GET_DEFAULT_INPUT:
			if( pcMessage->IsSourceWaiting() )
				GetDefaultInput( pcMessage );
		break;
		case MEDIA_SERVER_SET_DEFAULT_INPUT:
			SetDefaultInput( pcMessage );
		break;
		case MEDIA_SERVER_GET_DEFAULT_AUDIO_OUTPUT:
			if( pcMessage->IsSourceWaiting() )
				GetDefaultAudioOutput( pcMessage );
		break;
		case MEDIA_SERVER_GET_DEFAULT_VIDEO_OUTPUT:
			if( pcMessage->IsSourceWaiting() )
				GetDefaultVideoOutput( pcMessage );
		break;
		case MEDIA_SERVER_SET_DEFAULT_AUDIO_OUTPUT:
			SetDefaultAudioOutput( pcMessage );
		break;
		case MEDIA_SERVER_SET_DEFAULT_VIDEO_OUTPUT:
			SetDefaultVideoOutput( pcMessage );
		break;
		case MEDIA_SERVER_GET_STARTUP_SOUND:
			if( pcMessage->IsSourceWaiting() )
				GetStartupSound( pcMessage );
		break;
		case MEDIA_SERVER_SET_STARTUP_SOUND:
			SetStartupSound( pcMessage );
		break;
		case MEDIA_SERVER_CREATE_AUDIO_STREAM:
			if( pcMessage->IsSourceWaiting() )
				CreateAudioStream( pcMessage );
		break;
		case MEDIA_SERVER_DELETE_AUDIO_STREAM:
			if( pcMessage->IsSourceWaiting() )
				DeleteAudioStream( pcMessage );
		break;
		case MEDIA_SERVER_FLUSH_AUDIO_STREAM:
			if( pcMessage->IsSourceWaiting() )
				FlushAudioStream( pcMessage );
		break;
		case MEDIA_SERVER_GET_AUDIO_STREAM_DELAY:
			if( pcMessage->IsSourceWaiting() )
				GetDelay( pcMessage );
		break;
		case MEDIA_SERVER_GET_AUDIO_STREAM_PERCENTAGE:
			if( pcMessage->IsSourceWaiting() )
				GetPercentage( pcMessage );
		break;
		case MEDIA_SERVER_CLEAR_AUDIO_STREAM:
			if( pcMessage->IsSourceWaiting() )
				Clear( pcMessage );
		break;
		case MEDIA_SERVER_START_AUDIO_STREAM:
			Start( pcMessage );
		break;
		case MEDIA_SERVER_SHOW_CONTROLS:
			if( !m_bControlsShown ) {
				m_pcControls->Lock();
				m_pcControls->Show();
				m_pcControls->Unlock();
			}
			m_pcControls->MakeFocus();
			m_bControlsShown = true;
		break;
		case MEDIA_SERVER_GET_DSP_COUNT:
			if( pcMessage->IsSourceWaiting() )
				GetDspCount( pcMessage );
		break;
		case MEDIA_SERVER_GET_DSP_INFO:
			if( pcMessage->IsSourceWaiting() )
				GetDspInfo( pcMessage );
		break;
		case MEDIA_SERVER_GET_DEFAULT_DSP:
			if( pcMessage->IsSourceWaiting() )
				GetDefaultDsp( pcMessage );
		break;
		case MEDIA_SERVER_SET_DEFAULT_DSP:
			SetDefaultDsp( pcMessage );
		break;
		default:
			Looper::HandleMessage( pcMessage );
		break;
	}
}



/* Called to play the startup sound */
int play_startup_sound( void* pData )
{
	String zSound = String( "/system/sounds/" ) + *( String* )pData; 
	snooze( 10000 );
	
	/* Create media manager ( yes, we connect to this server ) */
	if( MediaManager::GetInstance() == NULL ) {
		MediaManager* pcManager = new MediaManager();
		pcManager->IsValid();
	}
	
	/* Create sound player and play the sound */
	MediaSoundPlayer* pcPlayer = new MediaSoundPlayer();
	
	if( pcPlayer->SetFile( zSound ) == 0 )
	{
		pcPlayer->Play();
		while( pcPlayer->IsPlaying() ) { snooze( 1000 ); }
	}
	delete( pcPlayer );
	
	return( 0 );
}

thread_id MediaServer::Start()
{
	make_port_public( GetMsgPort() );
	std::cout<<"Media Server running at port "<<GetMsgPort()<<std::endl;
	/* Play startup sound */
	resume_thread( spawn_thread( "play_startup_sound", (void*)play_startup_sound, 0, 0, &m_zStartupSound ) );
	
	return( Application::Run() );
}

bool MediaServer::OkToQuit()
{
	return( false );
}

int main()
{
	while( 1 ) 
	{
		MediaServer* pcServer = new MediaServer();
		pcServer->Start();
	}
}
































































































































































