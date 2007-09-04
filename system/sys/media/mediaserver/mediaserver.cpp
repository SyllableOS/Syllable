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
#include <cassert>

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
	m_zDefaultAudioOutput = "Media Server";
	m_zDefaultVideoOutput = "Screen Video Output";
	m_zStartupSound = "startup.wav";
	
	
	/* Load Settings */
	try
	{
		Settings* pcSettings = new Settings( new File( "/system/config/mediaserver" ) );
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
	} catch( ... )
	{
	}
	
	m_hLock = create_semaphore( "media_server_lock", 1, 0 );
	m_hActiveStreamCount = create_semaphore( "media_active_stream_count", 0, 0 );
	/* Find available DSP devices */
	m_nDspCount = LoadAudioPlugins();

	std::cout<<"Found "<<m_nDspCount<<" DSP's"<<std::endl;
	if( m_nDspCount < 0 )
		m_nDefaultDsp = -1;
	
	/* Create mix buffer */
	m_pMixBuffer = ( signed int* )malloc( PAGE_SIZE * 32 * 2 );
	memset( m_pMixBuffer, 0, PAGE_SIZE * 32 * 2 );
	
	m_nActiveStreamCount = 0;
	
	/* Clear stream memory */
	for( int i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ )
		m_sAudioStream[i].bUsed = false;
		
	
	/* Spawn flush thread */
	m_hThread = spawn_thread( "media_server_flush", (void*)media_flush_entry, DISPLAY_PRIORITY, 0, this );
	resume_thread( m_hThread );
	
	m_nMasterVolume = 100;
	
	/* Set defaults for controls window */
	m_cControlsFrame = os::Rect( 50, 50, 500, 350 );
	m_pcControls = NULL;
	
	/* We want to know if a process has quit */
	m_pcProcessQuitEvent = new os::Event();
	if( m_pcProcessQuitEvent->SetToRemote( "os/System/ProcessHasQuit", 0 ) != 0 )
	{
		printf( "Error: Failed to install process event handler\n" );
		delete( m_pcProcessQuitEvent );
		m_pcProcessQuitEvent = NULL;
		return;
	}
	m_pcProcessQuitEvent->SetMonitorEnabled( true, this, MEDIA_SERVER_PROCESS_QUIT );
}

MediaServer::~MediaServer()
{
	/* Stop flush thread */
	delete( m_pcProcessQuitEvent );
	m_bRunThread = false;
	wait_for_thread( m_hThread );
	delete_semaphore( m_hLock );
	delete_semaphore( m_hActiveStreamCount );
	free( m_pMixBuffer );
	/* Close window */
	if( m_pcControls )
		m_pcControls->Close();
}

/* Save all settings */
void MediaServer::SaveSettings()
{
	/* Save */
	Settings* pcSettings = new Settings( new File( "/system/config/mediaserver", O_RDWR | O_CREAT ) );
	pcSettings->SetString( "default_input", m_zDefaultInput );
	pcSettings->SetString( "default_audio_output", m_zDefaultAudioOutput );
	pcSettings->SetString( "default_video_output", m_zDefaultVideoOutput );
	pcSettings->SetString( "startup_sound", m_zStartupSound );
	pcSettings->SetInt32( "default_dsp", m_nDefaultDsp );
	pcSettings->Save();
	delete( pcSettings );
}

int MediaServer::LoadAudioPlugins()
{
	String zFileName;
	String zPath = String( "/dev/audio" );
	
	int nDspCount = 0;
	
	/* Load audio drivers */
	Directory *pcDirectory = new Directory();
	zPath = String( "/dev/audio" );
	if( pcDirectory->SetTo( zPath ) != 0 )
		return( 0 );
	while( pcDirectory->GetNextEntry( &zFileName ) )
	{
		if( zFileName == "." || zFileName == ".." )
			continue;
		zPath = String( "/dev/audio" );
		os::String zDevFileName = zPath + String( "/" ) + zFileName;
			
		/* Get userspace driver name */
		char zDriverPath[PATH_MAX];
		memset( zDriverPath, 0, PATH_MAX );
		int nFd = open( zDevFileName.c_str(), O_RDONLY );
		if( nFd < 0 )
			continue;
		if( ioctl( nFd, IOCTL_GET_USERSPACE_DRIVER, zDriverPath ) != 0 )
		{
			close( nFd );
			continue;
		}
		close( nFd );
			
		/* Construct plugin path */
		zPath = String( "/system/extensions/media" );
		zFileName = zPath + String( "/" ) + zDriverPath;
			
		image_id nID = load_library( zFileName.c_str(), 0 );
		if( nID >= 0 ) {
			init_media_addon *pInit;
			/* Call init_media_addon() */
			if( get_symbol_address( nID, "init_media_addon",
			-1, (void**)&pInit ) == 0 ) {
				MediaAddon* pcAddon = pInit( zDevFileName );
				if( pcAddon ) {
					if( pcAddon->GetAPIVersion() != MEDIA_ADDON_API_VERSION )
					{
						std::cout<<zFileName.c_str()<<" has wrong api version"<<std::endl;
						delete( pcAddon );
						unload_library( nID );
					}
					else if( pcAddon->Initialize() != 0 )
					{
						std::cout<<pcAddon->GetIdentifier().c_str()<<" failed to initialize"<<std::endl;
						delete( pcAddon );
						unload_library( nID );
					} else {
						std::cout<<pcAddon->GetIdentifier().c_str()<<" initialized"<<std::endl;			
						/* Find output streams */
						for( uint i = 0; i < pcAddon->GetOutputCount(); i++ )
						{
							os::MediaOutput* pcOutput = pcAddon->GetOutput( i );
							if( pcOutput == NULL )
								continue;
							if( pcOutput->FileNameRequired() )
							{
								pcOutput->Release();
								continue;
							}	
							/* Find next free sound card handle */
							for( int j=0; j<MEDIA_MAX_DSPS; j++ )
							{
								if( m_sDsps[j].bUsed )
									continue;

								m_sDsps[j].bUsed = true;
								sprintf( m_sDsps[j].zName, "%s", pcOutput->GetIdentifier().c_str() );
								m_sDsps[j].pcAddon = pcAddon;
								m_sDsps[j].nOutputStream = i;
								nDspCount++;
								std::cout<<"Added "<<m_sDsps[j].zName<<" with handle #"<<i<<std::endl;
								m_sDsps[j].ePhysType = pcOutput->GetPhysicalType();
								/* Read formats */
								for( int f = 0; f < pcOutput->GetOutputFormatCount(); f++ )
								{
									MediaFormat_s sFormat = pcOutput->GetOutputFormat( f );
									m_sDsps[j].asFormats.push_back( sFormat );
									std::cout<<"  "<<sFormat.nChannels<<" channels "<<sFormat.nSampleRate<<" Hz"<<std::endl;
								}

								pcOutput->Release();

								break;
							}
						}
					}
				}
			} else {
				std::cout<<zFileName.c_str()<<" does not export init_media_addon()"<<std::endl;
			}
		}
	}		

	return( nDspCount );
}

bool MediaServer::CheckFormat( MediaDSP_s* psDSP, int nChannels, int nSampleRate, int nRealChannels, int nRealSampleRate )
{
	/* Try to set the provided format */
	MediaFormat_s sFormat;
	for( uint i = 0; i < psDSP->asFormats.size(); i++ )
	{
		sFormat = psDSP->asFormats[i];
		if( sFormat.zName == "Raw Audio" && ( nSampleRate == 0 || sFormat.nSampleRate == nSampleRate )
			&& ( nChannels == 0 || sFormat.nChannels == nChannels ) && ( sFormat.nChannels >= 2 ||
			sFormat.nChannels == 0 ) ) {
			m_sCardFormat = sFormat;
			if( nChannels == 0 && m_sCardFormat.nChannels == 0 )
				m_sCardFormat.nChannels = nRealChannels;
			if( nSampleRate == 0 && m_sCardFormat.nSampleRate == 0 )
				m_sCardFormat.nSampleRate = nRealSampleRate;
			return( true );
		}
	}
	return( false );
}


bool MediaServer::OpenSoundCard( int nDevice, int nChannels, int nSampleRate )
{
	/* Try to open the soundcard device */
	if( nDevice < 0 || nDevice > MEDIA_MAX_DSPS )
		return( false );
		
	MediaDSP_s* psDSP = &m_sDsps[nDevice];
	m_pcCurrentOutput = m_sDsps[nDevice].pcAddon->GetOutput( m_sDsps[nDevice].nOutputStream );
	
	if( m_pcCurrentOutput == NULL )
		return( false );

	if( m_pcCurrentOutput->Open( "" ) != 0 ) {
		m_pcCurrentOutput->Release();
		return( false );
	}
	
	/* We want at least 2 channels */
	if( nChannels < 2 )
		nChannels = 2;
	
	/* Try to set provided format */
	if( !CheckFormat( psDSP, nChannels, nSampleRate, nChannels, nSampleRate ) )
	{
		/* Try to set a format with the same number of channels */
		if( !CheckFormat( psDSP, nChannels, 0, nChannels, nSampleRate ) )
		{
			/* Try to set a format with the same samplerate */
			if( !CheckFormat( psDSP, 0, nSampleRate, nChannels, nSampleRate ) )
			{
				/* Try 44100 stereo */
				if( !CheckFormat( psDSP, 2, 44100, nChannels, nSampleRate ) )
				{
					/* Try anything with two channels */
					if( !CheckFormat( psDSP, 2, 0, nChannels, nSampleRate ) )
					{
						/* Anything else */
						if( !CheckFormat( psDSP, 0, 0, nChannels, nSampleRate ) )
						{
							dbprintf( "Could not find a matching sound format\n" );
							m_pcCurrentOutput->Release();
							return( false );
						}
					}
				}
			}
		}
	}
	
	if( m_pcCurrentOutput->AddStream( "MediaServer", m_sCardFormat ) != 0 )
	{
		dbprintf( "Could not add sound stream\n" );
		m_pcCurrentOutput->Release();
		return( false );
	}
	
	/* Get buffer size */
	m_nBufferSize = m_pcCurrentOutput->GetBufferSize();

	std::cout<<"Hardware format: "<<m_sCardFormat.nChannels<<" channels, "<<m_sCardFormat.nSampleRate<<" samplerate, "<<m_nBufferSize<<"ms buffer"<<std::endl;
	
	return( true );
}

void MediaServer::CloseSoundCard()
{
	std::cout<<"Closing soundcard device"<<std::endl;
	/* Close soundcard */
	if( m_pcCurrentOutput != NULL ) {
		m_pcCurrentOutput->Clear();
		m_pcCurrentOutput->Close();
		m_pcCurrentOutput->Release();
	}
}

/* Thread which flushes the audio stream buffers */
void MediaServer::FlushThread()
{
	uint32 nSize = 0;
	m_bRunThread = true;
	
	while( m_bRunThread ) 
	{
		
		lock_semaphore( m_hActiveStreamCount );
		lock_semaphore( m_hLock );
		
		uint64 nDelay = m_pcCurrentOutput->GetDelay();
		nSize = 0;
		bool bSizeSet = false;
		for( uint32 i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ ) 
		{
			if( m_sAudioStream[i].bUsed )
			{
				int32 nRp = atomic_read( m_sAudioStream[i].pnReadPointer );
				int32 nWp = atomic_read( m_sAudioStream[i].pnWritePointer );
				
				if( nRp != nWp )
				{
					
					uint32 nUsedBytes = 0;
					if( nWp > nRp )
						nUsedBytes = ( nWp - nRp );
					else
						nUsedBytes = m_sAudioStream[i].nBufferSize - ( nRp - nWp );
					//printf( "Data to flush %i %i %i!\n", nUsedBytes, nRp, nWp );
					assert( ( nUsedBytes % ( m_sCardFormat.nChannels * 2 ) ) == 0 );
					
					if( nSize == 0 && !bSizeSet )
						nSize = nUsedBytes;
					else
						nSize = std::min( nSize, nUsedBytes );
					bSizeSet = true;
					m_sAudioStream[i].bActive = true;
				}
				else
				{
					if( m_sAudioStream[i].bActive )
					{
						nSize = 0;
						bSizeSet = true;
					}
				}
				atomic_set( m_sAudioStream[i].pnHWDelay, nDelay );
			}
		}		
		
		
		if( nSize == 0 )
		{
			unlock_semaphore( m_hLock );
			unlock_semaphore( m_hActiveStreamCount );
			snooze( 1000 );
			continue;
		}
		
		/* Avoid blocks */
		nDelay = m_pcCurrentOutput->GetDelay();
		uint64 nFree = m_nBufferSize - nDelay;
		
		if( nFree < 40 && !( m_nBufferSize <= 80 ) )
		{
			unlock_semaphore( m_hLock );
			unlock_semaphore( m_hActiveStreamCount );
			snooze( 1000 );
			continue;
		}
		
		nFree = nFree * m_sCardFormat.nSampleRate * m_sCardFormat.nChannels * 2 / 1000;
		
		nSize = std::min( (int)nSize, (int)nFree );
		nSize -= nSize % ( m_sCardFormat.nChannels * 2 );
		
		nDelay += nSize * 1000 / m_sCardFormat.nSampleRate / m_sCardFormat.nChannels / 2;
		
		//printf( "Total %i\n", nSize );
		assert( ( nSize % ( m_sCardFormat.nChannels * 2 ) ) == 0 );
		
		memset( m_pMixBuffer, 0, nSize * 2 );
		
		for( uint32 i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ ) 
		{
			if( m_sAudioStream[i].bUsed && m_sAudioStream[i].bActive )
			{
				signed int *pMix = m_pMixBuffer;
				int32 nRp = atomic_read( m_sAudioStream[i].pnReadPointer );
				signed short* pSrc = (signed short*)( &(m_sAudioStream[i].pData[nRp]) );
				
				if( m_sAudioStream[i].nVolume == 100 )				
					for( uint32 j = 0; j < nSize / 2; j++ )
					{
						*pMix++ += *pSrc++;
						nRp += 2;
						if( nRp > m_sAudioStream[i].nBufferSize )
						{
							dbprintf( "Error: Invalid read pointer %i %i\n", (int)nRp, (int)m_sAudioStream[i].nBufferSize );
							nRp = 0;
						}
						if( nRp == m_sAudioStream[i].nBufferSize )
						{
							nRp = 0;

							pSrc = (signed short*)( m_sAudioStream[i].pData );
						}
					}
				else
					for( uint32 j = 0; j < nSize / 2; j++ )
					{
						*pMix++ += ( (signed int)( *pSrc ) * m_sAudioStream[i].nVolume / 100 );
						pSrc++;
						nRp += 2;
						if( nRp > m_sAudioStream[i].nBufferSize )
						{
							dbprintf( "Error: Invalid read pointer %i %i\n", (int)nRp, (int)m_sAudioStream[i].nBufferSize );
							nRp = 0;
						}
						if( nRp == m_sAudioStream[i].nBufferSize )
						{
							nRp = 0;

							pSrc = (signed short*)( m_sAudioStream[i].pData );
						}
					}
				atomic_set( m_sAudioStream[i].pnReadPointer, nRp );
				atomic_set( m_sAudioStream[i].pnHWDelay, nDelay );
			}
		}
		
		/* Do signed int -> signed short conversion and update stream bars */
		signed int* pData = m_pMixBuffer;
		signed short* pMix = ( signed short* )m_pMixBuffer;
			
		for( uint32 i = 0; i < nSize / 2; i++ )
		{
			signed int nData = *pData * (signed int)m_nMasterVolume / 100;
			if( nData > 0xffff / 2 )
				*pMix = 0xffff / 2;
			else if( nData < -0xffff / 2 )
				*pMix = -0xffff / 2;
			else
					*pMix = (signed short)nData;
			
							
			pData++;
			pMix++;
		}
		
		/* Write */
		os::MediaPacket_s sPacket;
		sPacket.pBuffer[0] = (uint8*)m_pMixBuffer;
		sPacket.nSize[0] = nSize;
			
		m_pcCurrentOutput->WritePacket( 0, &sPacket );
		
		unlock_semaphore( m_hLock );
		unlock_semaphore( m_hActiveStreamCount );
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
//		resume_thread( spawn_thread( "play_startup_sound", (void*)play_startup_sound, 0, 0, &m_zStartupSound ) );
	}
	SaveSettings();
	
}

/* Create one audio stream which will be output to the soundcard */
void MediaServer::CreateAudioStream( Message* pcMessage )
{
	/* Look for a free stream */
	int32 nHandle = -1;
	int64 hProc = -1;
	Message cReply( MEDIA_SERVER_OK );
	
	
	/* Search free handle */
	lock_semaphore( m_hLock );
	for( int i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ )
	{
		if( !m_sAudioStream[i].bUsed ) {
			memset( &m_sAudioStream[i], 0, sizeof( m_sAudioStream[i] ) );
			m_sAudioStream[i].bUsed = true;
			nHandle = i;
			break;
		}
	}
	unlock_semaphore( m_hLock );
	
	if( nHandle < 0 ) {
		/* No free stream found */
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	const char* pzName;
	if( pcMessage->FindString( "name", &pzName ) != 0 ||
		pcMessage->FindInt64( "process", &hProc ) != 0 ||
		pcMessage->FindInt32( "channels", &m_sAudioStream[nHandle].nChannels ) != 0 ||
		pcMessage->FindInt32( "sample_rate", &m_sAudioStream[nHandle].nSampleRate ) != 0 ) {
		/* Message not complete */
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	/* Open soundcard */
	if( m_nActiveStreamCount == 0 )
		if( !OpenSoundCard( m_nDefaultDsp, m_sAudioStream[nHandle].nChannels, m_sAudioStream[nHandle].nSampleRate ) ) {
			/* No soundcard found */
			pcMessage->SendReply( MEDIA_SERVER_ERROR );
			return;
		}
	
	
	strcpy( m_sAudioStream[nHandle].zName, pzName );
	
	
	/* Create area */
	m_sAudioStream[nHandle].hProc = hProc;
	m_sAudioStream[nHandle].nBufferSize = ( 32 * PAGE_SIZE );
	m_sAudioStream[nHandle].nBufferSize -= m_sAudioStream[nHandle].nBufferSize % ( m_sCardFormat.nChannels * 2 );
	printf( "Buffer size %i\n", m_sAudioStream[nHandle].nBufferSize );
	m_sAudioStream[nHandle].hArea = create_area( "media_server_audio", (void**)&m_sAudioStream[nHandle].pnHWDelay,
										PAGE_ALIGN( m_sAudioStream[nHandle].nBufferSize + 12 ), AREA_FULL_ACCESS, AREA_NO_LOCK );
	if( m_sAudioStream[nHandle].hArea < 0 ) {
		if( m_nActiveStreamCount == 0 )
			CloseSoundCard();
		/* Area could not be created */
		m_sAudioStream[nHandle].bUsed = false;
		pcMessage->SendReply( MEDIA_SERVER_ERROR );
		return;
	}
	
	/* Set write pointer and data pointer */
	m_sAudioStream[nHandle].pnReadPointer = m_sAudioStream[nHandle].pnHWDelay + 1;
	m_sAudioStream[nHandle].pnWritePointer = m_sAudioStream[nHandle].pnReadPointer + 1;
	m_sAudioStream[nHandle].pData = (uint8*)( m_sAudioStream[nHandle].pnWritePointer + 1 );
	atomic_set( m_sAudioStream[nHandle].pnReadPointer, 0 );
	atomic_set( m_sAudioStream[nHandle].pnWritePointer, 0 );
	
//	printf( "%x %x %x\n", m_sAudioStream[nHandle].pnReadPointer, m_sAudioStream[nHandle].pnWritePointer, 
//							m_sAudioStream[nHandle].pData );
	
	std::cout<<"New Audio Stream "<<nHandle<<" ( "<<m_sAudioStream[nHandle].nChannels<<" Channels, "<<
							m_sAudioStream[nHandle].nSampleRate<<" Hz )"<<std::endl;
	
	/* Send information back */
	cReply.AddInt32( "area", m_sAudioStream[nHandle].hArea );
	cReply.AddInt32( "handle", nHandle );
	cReply.AddInt32( "hw_buffer_size", m_nBufferSize );
	cReply.AddInt32( "buffer_size", m_sAudioStream[nHandle].nBufferSize );
	cReply.AddInt32( "channels", m_sCardFormat.nChannels );
	cReply.AddInt32( "sample_rate", m_sCardFormat.nSampleRate );
	
	m_sAudioStream[nHandle].nVolume = 100;
	
	/* Mark stream as used */
	lock_semaphore( m_hLock );
	
	m_sAudioStream[nHandle].bUsed = true;
	m_nActiveStreamCount++;
	unlock_semaphore( m_hActiveStreamCount );
	unlock_semaphore( m_hLock );
	
	/* Tell controls window */
	if( m_pcControls )
		m_pcControls->StreamChanged( nHandle );
		
	pcMessage->SendReply( &cReply );
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
	lock_semaphore( m_hActiveStreamCount );
	if( m_nActiveStreamCount == 0 )
		CloseSoundCard();
	unlock_semaphore( m_hLock );
	delete_area( m_sAudioStream[nHandle].hArea );
	
	std::cout<<"Delete Audio Stream "<<nHandle<<std::endl;
	
	pcMessage->SendReply( MEDIA_SERVER_OK );
	
	/* Tell controls window */
	if( m_pcControls )
		m_pcControls->StreamChanged( nHandle ); 
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
	lock_semaphore( m_hLock );
	m_sAudioStream[nHandle].bActive = false;
	atomic_set( m_sAudioStream[nHandle].pnReadPointer, 0 );
	atomic_set( m_sAudioStream[nHandle].pnWritePointer, 0 );
	
	/* Look if we are the only active stream */
	if( m_nActiveStreamCount == 1 ) {
		m_pcCurrentOutput->Clear();
	}
	
	unlock_semaphore( m_hLock );
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
	if( pcMessage->IsSourceWaiting() ) 	
		pcMessage->SendReply( &cReply );
}

void MediaServer::GetDspFormats( Message* pcMessage )
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
	
	cReply.AddInt32( "type", m_sDsps[nHandle].ePhysType );
	for( uint i = 0; i < m_sDsps[nHandle].asFormats.size(); i++ )
	{
		cReply.AddInt32( "samplerate", m_sDsps[nHandle].asFormats[i].nSampleRate );
		cReply.AddInt32( "channels", m_sDsps[nHandle].asFormats[i].nChannels );
	}
	pcMessage->SendReply( &cReply );
}

void MediaServer::CheckProcess( proc_id hProc )
{
	if( m_nActiveStreamCount == 0 )
		return;
	lock_semaphore( m_hLock );
	for( uint nHandle = 0; nHandle < MEDIA_MAX_AUDIO_STREAMS; nHandle++ )
	{
		if( m_sAudioStream[nHandle].bUsed && m_sAudioStream[nHandle].hProc == hProc )
		{
			std::cout<<"Delete Audio Stream "<<nHandle<<std::endl;
			m_sAudioStream[nHandle].bUsed = false;
			m_nActiveStreamCount--;
			lock_semaphore( m_hActiveStreamCount );
			if( m_nActiveStreamCount == 0 )
				CloseSoundCard();
				
			if( m_pcControls )
				m_pcControls->StreamChanged( nHandle ); 
		}
	}
	unlock_semaphore( m_hLock );
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
		case MEDIA_SERVER_CLEAR_AUDIO_STREAM:
			if( pcMessage->IsSourceWaiting() )
				Clear( pcMessage );
		break;
		case MEDIA_SERVER_START_AUDIO_STREAM:
			Start( pcMessage );
		break;
		case MEDIA_SERVER_SHOW_CONTROLS:
			if( m_pcControls == NULL ) {
				m_pcControls = new MediaControls( this, m_cControlsFrame );
				m_pcControls->Lock();
				m_pcControls->Show();
				m_pcControls->Unlock();
			}
			m_pcControls->MakeFocus();
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
		case MEDIA_SERVER_GET_DSP_FORMATS:
			if( pcMessage->IsSourceWaiting() )
				GetDspFormats( pcMessage );
		break;
		case MEDIA_SERVER_SET_MASTER_VOLUME:
		{
			int32 nVolume;
			if( pcMessage->FindInt32( "volume", &nVolume ) == 0 ) {
				if( nVolume < 0 )
					nVolume = 0;
				if( nVolume > 100 )
					nVolume = 100;
				m_nMasterVolume = nVolume;
				if( m_pcControls )
					m_pcControls->SetMasterVolume( m_nMasterVolume );
			}
		}
		break;
		case MEDIA_SERVER_GET_MASTER_VOLUME:
		{
			os::Message cReply( MEDIA_SERVER_GET_MASTER_VOLUME );
			if( pcMessage->IsSourceWaiting() )
			{
				cReply.AddInt32( "volume", m_nMasterVolume );
				pcMessage->SendReply( &cReply );
			}
		}
		break;
		case MEDIA_SERVER_PROCESS_QUIT:
		{
			int64 nProcess = 0;
			if( pcMessage->FindInt64( "process", &nProcess ) != 0 )
				break;
			CheckProcess( nProcess );
		}
		break;
		default:
			Looper::HandleMessage( pcMessage );
		break;
	}
}



thread_id MediaServer::Start()
{
	make_port_public( GetMsgPort() );
	std::cout<<"Media Server running at port "<<GetMsgPort()<<std::endl;
	
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



