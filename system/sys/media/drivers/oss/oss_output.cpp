/*  OSS output plugin
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

#include <media/output.h>
#include <media/server.h>
#include <atheos/soundcard.h>
#include <atheos/threads.h>
#include <atheos/semaphore.h>
#include <atheos/time.h>
#include <atheos/kernel.h>
#include <atheos/threads.h>
#include <atheos/msgport.h>
#include <util/message.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream.h>
#include "mixerview.h"

#define OSS_PACKET_NUMBER 10

class OSSOutput : public os::MediaOutput
{
public:
	OSSOutput();
	~OSSOutput();
	os::String		GetIdentifier();
	os::View*		GetConfigurationView();
	
	bool			FileNameRequired();
	status_t 		Open( os::String zFileName );
	void 			Close();
	
	void			Clear();
	void			Flush();
	
	uint32			GetOutputFormatCount();
	os::MediaFormat_s	GetOutputFormat( uint32 nIndex );
	uint32			GetSupportedStreamCount();
	status_t		AddStream( os::String zName, os::MediaFormat_s sFormat );
	
	status_t		WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket );
	uint64			GetDelay();
	uint32			GetUsedBufferPercentage();
	
	void			FlushThread();
	
private:
	void			Resample( uint16* pDst, uint16* pSrc, uint32 nLength );
	bool			m_bRunThread;
	thread_id		m_hThread;
	uint64			m_nFactor;
	int				m_hOSS;
	int				m_nBufferSize;
	int				m_nQueuedPackets;
	os::MediaPacket_s* m_psPacket[OSS_PACKET_NUMBER];
	sem_id			m_hLock;
	os::MediaFormat_s m_sFormat;
	bool			m_bResample;
	os::Messenger	m_cMediaServerLink;
};


/*
 * Note : Using threads here, because the non-threaded version caused bad noises 
 */

OSSOutput::OSSOutput()
{
	/* Set default values */
	m_hOSS = -1;
	m_hLock = create_semaphore( "oss_lock", 1, 0 );
	m_nQueuedPackets = 0;
	m_hThread = -1;
}

OSSOutput::~OSSOutput()
{
	Close();
	delete_semaphore( m_hLock );
}

os::String OSSOutput::GetIdentifier()
{
	return( "OSS Audio Output" );
}


os::View* OSSOutput::GetConfigurationView()
{
	os::MixerView* pcView = new os::MixerView( os::Rect( 0, 0, 1, 1 ) );
	return( pcView );
}

void OSSOutput::FlushThread()
{
	/* Always play the next packet in the queue */
	os::MediaPacket_s *psNextPacket;
	cout<<"OSS flush thread running"<<endl;
	while( m_bRunThread )
	{
		lock_semaphore( m_hLock );

		if( m_nQueuedPackets > 0 ) 
		{
			/* Get first packet */
			psNextPacket = m_psPacket[0];
		
			unlock_semaphore( m_hLock );
			write( m_hOSS, psNextPacket->pBuffer[0], psNextPacket->nSize[0] );
			lock_semaphore( m_hLock );
			//cout<<"Packet of "<<psNextPacket->nSize<<" flushed"<<endl;
			free( psNextPacket->pBuffer[0] );
			free( psNextPacket );
			m_nQueuedPackets--;
			for( int i = 0; i < m_nQueuedPackets; i++ )
			{
				m_psPacket[i] = m_psPacket[i+1];
			}
			//cout<<"Audio :"<<m_nQueuedPackets<<" packets left"<<endl;
		}
		unlock_semaphore( m_hLock );
		snooze( 1000 );
	} 
	
}

void OSSOutput::Flush()
{
	/* Do nothing */
}

bool OSSOutput::FileNameRequired()
{
	return( false );
}

int flush_thread_entry( void* pData )
{
	OSSOutput *pcOutput = ( OSSOutput* )pData;
	pcOutput->FlushThread();
	return( 0 );
}

status_t OSSOutput::Open( os::String zFileName )
{
	/* Look for media manager port */	
	port_id nPort;
	os::Message cReply;
	int32 hDefaultDsp;
	std::string cDspPath;

	if( ( nPort = find_port( "l:media_server" ) ) < 0 ) {
		cout<<"Could not connect to media server!"<<endl;
		return( -1 );
	}
	m_cMediaServerLink = os::Messenger( nPort );
	m_cMediaServerLink.SendMessage( os::MEDIA_SERVER_PING, &cReply );
	if( cReply.GetCode() != os::MEDIA_SERVER_OK ) {
		return( -1 );
	}

	/* Find soundcard */
	os::Message cMsg( os::MEDIA_SERVER_GET_DEFAULT_DSP );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
	if( cReply.GetCode() != os::MEDIA_SERVER_OK )
		return( -1 );

	if( cReply.FindInt32( "handle", &hDefaultDsp ) != 0 )
		return( -1 );

	cMsg.SetCode( os::MEDIA_SERVER_GET_DSP_INFO );
	cMsg.AddInt32( "handle", hDefaultDsp );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
	if( cReply.GetCode() != os::MEDIA_SERVER_OK )
		return( -1 );

	if( cReply.FindString( "path", &cDspPath ) )
		return( -1 );

	/* Open soundcard */
	Close();
	m_hOSS = open( cDspPath.c_str(), O_WRONLY );
	if( m_hOSS < 0 )
		return( -1 );
	
		
	/* Clear packet buffer */
	m_nQueuedPackets = 0;
	for( int i = 0; i < OSS_PACKET_NUMBER; i++ ) {
		m_psPacket[i] = NULL;
	}
	
	/* Spawn thread */
	m_bRunThread = true;
	m_hThread = spawn_thread( "oss_flush", flush_thread_entry, 0, 0, this );
	resume_thread( m_hThread );
		
	return( 0 );
}

void OSSOutput::Close()
{
	Clear();
	if( m_hOSS > -1 )
		close( m_hOSS );
	m_hOSS = -1;
	m_bRunThread = false;
	if( m_hThread > -1 )
		wait_for_thread( m_hThread );
	m_hThread = -1;
	//cout<<"OSS closed"<<endl;
}


void OSSOutput::Clear()
{
	/* Stop playback */
	m_bRunThread = false;
	bool bRestart = true;
	if( m_hThread > -1 ) {
		wait_for_thread( m_hThread );
	} else {
		bRestart = false;
	}
	if( m_hOSS ) {
		ioctl( m_hOSS, SNDCTL_DSP_RESET, NULL );
		int nVal = m_sFormat.nSampleRate;
		ioctl( m_hOSS, SNDCTL_DSP_SPEED, &nVal );
		nVal = m_sFormat.nChannels;
		if( m_sFormat.nChannels > 2 )
			nVal = 2;
		else
			nVal = m_sFormat.nChannels - 1;
		ioctl( m_hOSS, SNDCTL_DSP_STEREO, &nVal );
		nVal = AFMT_S16_LE;
		ioctl( m_hOSS, SNDCTL_DSP_SETFMT, &nVal );
	}
	/* Delete all pending packets */
	for( int i = 0; i < m_nQueuedPackets; i++ ) {
		free( m_psPacket[i]->pBuffer[0] );
		free( m_psPacket[i] );
	}
	m_nQueuedPackets = 0;
	/* Spawn thread */
	if( bRestart ) {
		m_bRunThread = true;
		m_hThread = spawn_thread( "oss_flush", flush_thread_entry, 0, 0, this );
		resume_thread( m_hThread );
	}
}

uint32 OSSOutput::GetOutputFormatCount()
{
	return( 1 );
}

os::MediaFormat_s OSSOutput::GetOutputFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	memset( &sFormat, 0, sizeof( os::MediaFormat_s ) );
	sFormat.zName = "Raw Audio";
	sFormat.nType = os::MEDIA_TYPE_AUDIO;
	
	return( sFormat );
}

uint32 OSSOutput::GetSupportedStreamCount()
{
	return( 1 );
}

status_t OSSOutput::AddStream( os::String zName, os::MediaFormat_s sFormat )
{
	if( sFormat.zName == "Raw Audio" && sFormat.nType == os::MEDIA_TYPE_AUDIO ) {}
	else
		return( -1 );
	
	/* Set Media format ( TODO: Check if the soundcard accepts the values ) */
	m_sFormat = sFormat;
	int nVal = m_sFormat.nSampleRate;
	ioctl( m_hOSS, SNDCTL_DSP_SPEED, &nVal );
	nVal = AFMT_S16_LE;
	ioctl( m_hOSS, SNDCTL_DSP_SETFMT, &nVal );
	
	if( sFormat.nChannels > 2 ) {
		cout<<sFormat.nChannels<<" Channels -> 2 Channels"<<endl;
		nVal = 2;
		m_bResample = true;
		/* Calculate size -> time factor */
		m_nFactor = (uint64)sFormat.nSampleRate * 2 * 2;
	} else {
		nVal = m_sFormat.nChannels - 1;
		m_bResample =false;
		/* Calculate size -> time factor */
		m_nFactor = (uint64)sFormat.nSampleRate * (uint64)sFormat.nChannels * 2;
	}
	ioctl( m_hOSS, SNDCTL_DSP_STEREO, &nVal );
	
	/* Get buffer size */
	audio_buf_info sInfo;
	ioctl( m_hOSS, SNDCTL_DSP_GETOSPACE, &sInfo );
	m_nBufferSize = sInfo.bytes;

	return( 0 );
}

void OSSOutput::Resample( uint16* pDst, uint16* pSrc, uint32 nLength )
{
	uint32 nSkipBefore = 0;
	uint32 nSkipBehind = 0;
	/* Calculate skipping ( I just guess what is right here ) */
	if( m_sFormat.nChannels == 3 ) {
		nSkipBefore = 0;
		nSkipBehind = 1;
	} else if( m_sFormat.nChannels == 4 ) {
		nSkipBefore = 2;
		nSkipBehind = 0;
	} else if( m_sFormat.nChannels == 5 ) {
		nSkipBefore = 2;
		nSkipBehind = 1;
	} else if( m_sFormat.nChannels == 6 ) {
		nSkipBefore = 2;
		nSkipBehind = 2;
	} 
	
	//cout<<"Resample"<<endl;
	for( uint32 i = 0; i < nLength / m_sFormat.nChannels / 2; i++ ) 
	{
		/* Skip */
		pSrc += nSkipBefore;
		/* Channel 1 */
		*pDst++ = *pSrc++;
		
		/* Channel 2 */
		*pDst++ = *pSrc++;
		
		/* Skip */
		pSrc += nSkipBehind;
	}
	//cout<<"Finished"<<endl;
}

status_t OSSOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket )
{
	/* Create a new media packet and queue it */
	if( psPacket->nSize[0] < 1 )
		return( -1 );
	os::MediaPacket_s* psQueuePacket = ( os::MediaPacket_s* )malloc( sizeof( os::MediaPacket_s ) );
	if( psQueuePacket == NULL )
		return( -1 );
	memset( psQueuePacket, 0, sizeof( os::MediaPacket_s ) );
	
	if( m_bResample ) {
		psQueuePacket->pBuffer[0] = ( uint8* )malloc( psPacket->nSize[0] * 2 / m_sFormat.nChannels );
		Resample( (uint16*)psQueuePacket->pBuffer[0], (uint16*)psPacket->pBuffer[0], psPacket->nSize[0] );
		psQueuePacket->nSize[0] = psPacket->nSize[0] * 2 / m_sFormat.nChannels;
	}
	else {
		psQueuePacket->pBuffer[0] = ( uint8* )malloc( psPacket->nSize[0] );
		memcpy( psQueuePacket->pBuffer[0], psPacket->pBuffer[0], psPacket->nSize[0] );
		psQueuePacket->nSize[0] = psPacket->nSize[0];
	}
	lock_semaphore( m_hLock );
	/* Queue packet */
	if( m_nQueuedPackets > OSS_PACKET_NUMBER - 1 ) {
		unlock_semaphore( m_hLock );
		free( psQueuePacket->pBuffer[0] );
		free( psQueuePacket );
		cout<<"Packet buffer full"<<endl;
		return( -1 );
	}
	m_psPacket[m_nQueuedPackets] = psQueuePacket;
	m_nQueuedPackets++;
	unlock_semaphore( m_hLock );
	//cout<<"Packet queued at position "<<m_nQueuedPackets - 1<<"( "<<m_nQueuedPackets * 100 / OSS_PACKET_NUMBER<<
	//"% of the buffer used )"<<endl;
	return( 0 );	
}

uint64 OSSOutput::GetDelay()
{
	int nDataSize = 0;
	int nDelay;
	uint64 nTime;
	/* Get data size in the packet buffer */
	lock_semaphore( m_hLock );
	for( int i = 0; i < m_nQueuedPackets; i++ ) 
	{
		nDataSize += m_psPacket[i]->nSize[0];
	}
	/* Add the data in the soundcard buffer */
	ioctl( m_hOSS, SNDCTL_DSP_GETODELAY, &nDelay );
	nDataSize += nDelay;
	unlock_semaphore( m_hLock );
	nTime = nDataSize;
	nTime *= 1000;
	nTime /= (uint64)m_nFactor;
	return( nTime );
}

uint32 OSSOutput::GetUsedBufferPercentage()
{
	uint32 nValue;
	lock_semaphore( m_hLock );
	nValue = m_nQueuedPackets * 100 / OSS_PACKET_NUMBER;
	unlock_semaphore( m_hLock );
	return( nValue );
}

extern "C"
{
	os::MediaOutput* init_media_output()
	{
		return( new OSSOutput() );
	}

}
































































































































