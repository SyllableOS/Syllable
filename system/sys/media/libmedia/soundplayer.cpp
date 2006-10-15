/*  Media Sound Player class
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

#include <atheos/kernel.h>
#include <media/soundplayer.h>

using namespace os;

class MediaSoundPlayer::Private
{
public:
	os::MediaInput* m_pcInput;
	os::MediaCodec* m_pcAudioCodec;
	os::MediaOutput* m_pcAudioOutput;
	os::MediaFormat_s m_sAudioFormat;
	uint32 m_nAudioStream;
	bool m_bReady;
	bool m_bRunThread;
	thread_id m_hPlayThread;
};


/** Create a new sound player.
 * \par Description:
 * Creates a new sound player object. Use the SetFile() method to select 
 * the file you want to play.
 * \author	Arno Klenke
 *****************************************************************************/
MediaSoundPlayer::MediaSoundPlayer()		
{
	m = new Private;
	m->m_pcInput = NULL;
	m->m_pcAudioCodec = NULL;
	m->m_pcAudioOutput = NULL;
	m->m_bReady = false;
}

MediaSoundPlayer::~MediaSoundPlayer()
{
	Clear();
	delete( m );
}

void MediaSoundPlayer::Clear()
{
	if( m->m_pcInput ) {
		m->m_pcInput->Close();
		m->m_pcInput->Release();
		m->m_pcInput = NULL;
	}
	if( m->m_pcAudioOutput ) {
		m->m_pcAudioOutput->Close();
		m->m_pcAudioOutput->Release();
		m->m_pcAudioOutput = NULL;
	}
	if( m->m_pcAudioCodec ) {
		m->m_pcAudioCodec->Close();
		m->m_pcAudioCodec->Release();
		m->m_pcAudioCodec = NULL;
	}
}

/** Select the file.
 * \par Description:
 * Selects the file you want to play. Returns 0 if the file has be openend
 * correctly.
 * \author	Arno Klenke
 *****************************************************************************/
status_t MediaSoundPlayer::SetFile( String zFileName )
{
	/* Stop if another file is still playing */
	Stop();
	Clear();
	
	
	/* Check if we have a media manager */
	if( MediaManager::GetInstance() == NULL ) {
		m->m_bReady = false;
		return( -1 );
	}
	
	
	MediaManager* pcManager = MediaManager::GetInstance();
	
	/* Get input */
	m->m_pcInput = pcManager->GetBestInput( zFileName );
	if( m->m_pcInput == NULL ) {
		m->m_bReady = false;
		return( -1 );
	}
	
	m->m_pcInput->Open( zFileName );
		
	/* We do not support non packet based devices */
	if( !m->m_pcInput->PacketBased() )
	{
		m->m_bReady = false;
		m->m_pcInput->Release();
		return( -1 );
	}
	/* Always select the first track */
	m->m_pcInput->SelectTrack( 0 );
	
	/* Look for an audio stream */
	bool bStreamFound = false;
	
	for( uint32 i = 0; i < m->m_pcInput->GetStreamCount(); i++ )
	{
		if( m->m_pcInput->GetStreamFormat( i ).nType == os::MEDIA_TYPE_AUDIO ) {
			m->m_nAudioStream = i;
			m->m_sAudioFormat = m->m_pcInput->GetStreamFormat( i );
			bStreamFound = true;
			break;
		}
	}
	if( !bStreamFound )
	{
		m->m_bReady = false;
		return( -1 );
	}
	/* Open audio output */
	m->m_pcAudioOutput = pcManager->GetDefaultAudioOutput();
	if( m->m_pcAudioOutput == NULL || ( m->m_pcAudioOutput && m->m_pcAudioOutput->FileNameRequired() )
		|| ( m->m_pcAudioOutput && m->m_pcAudioOutput->Open( "" ) != 0 ) ) {
		m->m_bReady = false;
		return( -1 );	
	}
	/* Open audio codec */
	m->m_pcAudioCodec = NULL;
	for( uint32 i = 0; i < m->m_pcAudioOutput->GetOutputFormatCount(); i++ ) {
		if( ( m->m_pcAudioCodec = pcManager->GetBestCodec( m->m_sAudioFormat, m->m_pcAudioOutput->GetOutputFormat( i ), false ) ) != NULL )
			if( m->m_pcAudioCodec->Open( m->m_sAudioFormat, m->m_pcAudioOutput->GetOutputFormat( i ), false ) == 0 )
				break;
			else {
				m->m_pcAudioCodec->Release();
				m->m_pcAudioCodec = NULL;
			}
	}
	if( m->m_pcAudioCodec == NULL ) {
		m->m_bReady = false;
		return( -1 );
	}
	if( m->m_pcAudioOutput->AddStream( zFileName, m->m_pcAudioCodec->GetExternalFormat() ) != 0 ) {
		m->m_bReady = false;
		return( -1 );
	}
	
	m->m_bReady = true;
	m->m_hPlayThread = -1;
	return( 0 );
}

void MediaSoundPlayer::PlayThread()
{
	os::MediaPacket_s sPacket;
	os::MediaPacket_s sAudioPacket;
	uint8 nErrorCount = 0;
	bool bError = false;
	
	m->m_bRunThread = true;
	
	/* Create audio output packet */
	m->m_pcAudioCodec->CreateAudioOutputPacket( &sAudioPacket );
	
	while( m->m_bRunThread )
	{
		
		if( m->m_pcAudioOutput->GetDelay() < m->m_pcAudioOutput->GetBufferSize() ) {
			/* Grab data */
			if( m->m_pcInput->ReadPacket( &sPacket ) == 0 ) {
				nErrorCount = 0;
				/* Decode audio data */
				if( sPacket.nStream == m->m_nAudioStream ) {
					if( m->m_pcAudioCodec->DecodePacket( &sPacket, &sAudioPacket ) == 0 ) {
						if( sAudioPacket.nSize[0] > 0 ) {
							m->m_pcAudioOutput->WritePacket( 0, &sAudioPacket );
						}
					}
				}
					
				m->m_pcInput->FreePacket( &sPacket );
			} else {
				/* Increase error count */
				nErrorCount++;
				if( m->m_pcAudioOutput->GetDelay() > 0 )
					nErrorCount--;
				if( nErrorCount > 10 && !bError ) {
					m->m_bRunThread = false;
					bError = true;
				}
			}
		}
				
		snooze( 1000 );
		
	}
	/* Stop */
	m->m_pcAudioOutput->Clear();
	m->m_pcAudioCodec->DeleteAudioOutputPacket( &sAudioPacket );
	m->m_hPlayThread = -1;
}

int play_thread_entry( void* pData )
{
	MediaSoundPlayer* pcPlayer = ( MediaSoundPlayer* )pData;
	pcPlayer->PlayThread();
	
	return( 0 );
}

/** Play file.
 * \par Description:
 * Plays the selected file.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaSoundPlayer::Play()
{
	if( !m->m_bReady )
		return;
	
	/* Seek to the beginning */
	if( !m->m_pcInput->StreamBased() )
		m->m_pcInput->Seek( 0 );
	
	/* Start Play thread */	
	m->m_hPlayThread = spawn_thread( "play_thread", (void*)play_thread_entry, 150, 0, this );
	resume_thread( m->m_hPlayThread );
}


/** Whether the file is playing.
 * \par Description:
 * Returns true if the file is played moment.
 * \author	Arno Klenke
 *****************************************************************************/
bool MediaSoundPlayer::IsPlaying()
{
	if( !m->m_bReady )
		return( false );
	if( m->m_hPlayThread < 0 )
		return( false );
	return( true );
}

/** Stop playing.
 * \par Description:
 * Stops playing if the file is played at the moment.
 * \author	Arno Klenke
 *****************************************************************************/
void MediaSoundPlayer::Stop()
{
	if( !m->m_bReady )
		return;
	if( m->m_hPlayThread < 0 )
		return;
	m->m_bRunThread = false;
	wait_for_thread( m->m_hPlayThread );
	m->m_hPlayThread = -1;
}


void MediaSoundPlayer::_reserved1() {}
void MediaSoundPlayer::_reserved2() {}
void MediaSoundPlayer::_reserved3() {}
void MediaSoundPlayer::_reserved4() {}
void MediaSoundPlayer::_reserved5() {}
void MediaSoundPlayer::_reserved6() {}
void MediaSoundPlayer::_reserved7() {}
void MediaSoundPlayer::_reserved8() {}
void MediaSoundPlayer::_reserved9() {}
void MediaSoundPlayer::_reserved10() {}















