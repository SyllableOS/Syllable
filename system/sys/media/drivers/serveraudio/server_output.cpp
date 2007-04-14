/*  Media Server output plugin
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
#include <media/addon.h>
#include <media/manager.h>
#include <util/messenger.h>
#include <util/message.h>
#include <atheos/msgport.h>
#include <atheos/areas.h>
#include <atheos/kernel.h>
#include <iostream>
#include <cassert>
#include "resampler.h"

class ServerOutput : public os::MediaOutput
{
public:
	ServerOutput( int nIndex );
	~ServerOutput();
	uint32			GetPhysicalType()
	{
		ReadFormats();
		return( m_ePhysType );
	}
	os::String		GetIdentifier();
	os::View*		GetConfigurationView();
	
	bool			FileNameRequired();
	status_t 		Open( os::String zFileName );
	void 			Close();
	
	void			Clear();
	void			Flush();
	
	void			ReadFormats();
	uint32			GetOutputFormatCount();
	os::MediaFormat_s	GetOutputFormat( uint32 nIndex );
	uint32			GetSupportedStreamCount();
	status_t		AddStream( os::String zName, os::MediaFormat_s sFormat );
	
	status_t		WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket );
	uint64			GetDelay( bool bNonSharedOnly );
	uint64			GetBufferSize( bool bNonSharedOnly );
	
private:
	int					m_nIndex;
	os::Messenger		m_cMediaServerLink;
	os::MediaFormat_s 	m_sSrcFormat;
	os::MediaFormat_s 	m_sDstFormat;
	bool				m_bResample;
	int32				m_nHandle;
	int32				m_nBufferSize;
	int32				m_nHWBufferSize;
	area_id				m_hServerArea;
	area_id				m_hArea;
	atomic_t*			m_pnHWDelay;
	atomic_t*			m_pnReadPointer;
	atomic_t*			m_pnWritePointer;
	uint8*				m_pData;
	std::vector<os::MediaFormat_s> m_asFormats;
	uint32				m_ePhysType;
};


ServerOutput::ServerOutput( int nIndex )
{
	m_nIndex = nIndex;
	m_hArea = -1;
	m_nHandle = -1;
	m_asFormats.clear();
	/* Look for media manager port */	
	port_id nPort;
	m_hArea = -1;
	if( ( nPort = find_port( "l:media_server" ) ) < 0 ) {
		std::cout<<"Could not connect to media server!"<<std::endl;
		return;
	}
	m_cMediaServerLink = os::Messenger( nPort );
}

ServerOutput::~ServerOutput()
{
	Close();
}

os::String ServerOutput::GetIdentifier()
{
	os::String cName;

	os::Message cReply;
	os::Message cMsg( os::MEDIA_SERVER_GET_DSP_INFO );
	cMsg.AddInt32( "handle", m_nIndex );
	os::MediaManager::GetInstance()->GetServerLink().SendMessage( &cMsg, &cReply );

	if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindString( "name", &cName ) == 0 )
		return( os::String( "Media Server - " ) + cName );
	else
		return( "Media Server Audio Output" );
}


os::View* ServerOutput::GetConfigurationView()
{
	return( NULL );
}


bool ServerOutput::FileNameRequired()
{
	return( false );
}

status_t ServerOutput::Open( os::String zFileName )
{
	os::Message cReply;
	m_cMediaServerLink.SendMessage( os::MEDIA_SERVER_PING, &cReply );
	if( cReply.GetCode() != os::MEDIA_SERVER_OK ) {
		return( -1 );
	}
	
	/* Set this dsp as the default one */
	os::Message cDspMsg( os::MEDIA_SERVER_SET_DEFAULT_DSP );
	cDspMsg.AddInt32( "handle", m_nIndex );
	m_cMediaServerLink.SendMessage( &cDspMsg, &cReply );
	if( cReply.GetCode() != os::MEDIA_SERVER_OK )
	{
		return( -1 );
	}
	
	m_nHandle = -1;
	return( 0 );
	
}

void ServerOutput::Close()
{
	if( m_nHandle >= 0 ) 
	{
		Clear();
		os::Message cMsg( os::MEDIA_SERVER_DELETE_AUDIO_STREAM );
		os::Message cReply;
		cMsg.AddInt32( "handle", m_nHandle );
		m_cMediaServerLink.SendMessage( &cMsg, &cReply );
		m_nHandle = -1;
		if( m_hArea > -1 )
			delete_area( m_hArea );
	}
}


void ServerOutput::Clear()
{
	os::Message cMsg( os::MEDIA_SERVER_CLEAR_AUDIO_STREAM );
	os::Message cReply;
	cMsg.AddInt32( "handle", m_nHandle );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
}

void ServerOutput::ReadFormats()
{
	if( m_asFormats.size() > 0 )
		return;
		
	os::Message cMsg( os::MEDIA_SERVER_GET_DSP_FORMATS );
	os::Message cReply;
	cMsg.AddInt32( "handle", m_nIndex );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
	
	cReply.FindInt32( "type", (int32*)&m_ePhysType );

	
	int nIndex = 0;
	int32 nSampleRate = 0;
	int32 nChannels = 0;
	while( cReply.FindInt32( "samplerate", &nSampleRate, nIndex ) == 0 )
	{
		cReply.FindInt32( "channels", &nChannels, nIndex );
		os::MediaFormat_s sFormat;
		MEDIA_CLEAR_FORMAT( sFormat );
		sFormat.zName = "Raw Audio";
		sFormat.nType = os::MEDIA_TYPE_AUDIO;
		sFormat.nSampleRate = nSampleRate;
		sFormat.nChannels = nChannels;
		m_asFormats.push_back( sFormat );
		nIndex++;
	}
}

uint32 ServerOutput::GetOutputFormatCount()
{
	ReadFormats();
	return( m_asFormats.size() );
}

os::MediaFormat_s ServerOutput::GetOutputFormat( uint32 nIndex )
{
	ReadFormats();
	
	return( m_asFormats[nIndex] );
}

uint32 ServerOutput::GetSupportedStreamCount()
{
	return( 1 );
}

status_t ServerOutput::AddStream( os::String zName, os::MediaFormat_s sFormat )
{
	if( sFormat.zName == "Raw Audio" && sFormat.nType == os::MEDIA_TYPE_AUDIO ) {}
	else
		return( -1 );
	
	/* Save source format  */
	m_sSrcFormat = sFormat;
	
	/* Tell media server */
	os::Message cMsg( os::MEDIA_SERVER_CREATE_AUDIO_STREAM );
	os::Message cReply;
	
	cMsg.AddInt64( "process", get_process_id( NULL ) );
	cMsg.AddString( "name", zName );
	cMsg.AddInt32( "channels", m_sSrcFormat.nChannels );
	cMsg.AddInt32( "sample_rate", m_sSrcFormat.nSampleRate );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
	if( cReply.GetCode() != os::MEDIA_SERVER_OK ) {
		return( -1 );
	}
	if( cReply.FindInt32( "handle", &m_nHandle ) != 0 ||
		cReply.FindInt32( "area", (int32*)&m_hServerArea ) != 0 ||
		cReply.FindInt32( "buffer_size", &m_nBufferSize ) != 0 ) {
		return( -1 );
	}
	if(	cReply.FindInt32( "hw_buffer_size", &m_nHWBufferSize ) != 0 ||
		cReply.FindInt32( "channels", &m_sDstFormat.nChannels ) != 0 ||
		cReply.FindInt32( "sample_rate", &m_sDstFormat.nSampleRate ) != 0 ) {
		return( -1 );
	}
	printf( "Buffer size %i Channels %i Samplerate %i\n", m_nBufferSize, m_sDstFormat.nChannels,
	m_sDstFormat.nSampleRate );
	
	if( m_sSrcFormat.nChannels != m_sDstFormat.nChannels ||
		m_sSrcFormat.nSampleRate != m_sDstFormat.nSampleRate )
	{
		printf( "Resampler enabled\n" );
		m_bResample = true;
	} else
		m_bResample = false;
	
	/* Clone area */
	m_pnHWDelay = NULL;
	m_hArea = clone_area( "media_server_output", (void**)&m_pnHWDelay, AREA_FULL_ACCESS, AREA_NO_LOCK,
						m_hServerArea );
	if( m_hArea < 0 )
		return( -1 );
	m_pnReadPointer = m_pnHWDelay + 1;
	m_pnWritePointer = m_pnReadPointer + 1;
	m_pData = (uint8*)( m_pnWritePointer + 1 );
		
	return( 0 );
}

status_t ServerOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket )
{
	
	
	uint8* pSource = psPacket->pBuffer[0];
	uint32 nBytesLeft = psPacket->nSize[0];
		
	
	while( nBytesLeft > 0 )
	{
		
	
		/* Calculate number of free samples in the ringbuffer */
		int32 nRp = atomic_read( m_pnReadPointer );
		int32 nWp = atomic_read( m_pnWritePointer );
		
		
		uint32 nFreeBytes = 0;
		if( nWp >= nRp )
			nFreeBytes = m_nBufferSize - ( nWp - nRp );
		else
			nFreeBytes = nRp - nWp;
		uint32 nFreeSamples = nFreeBytes / m_sDstFormat.nChannels / 2 - 1;
		assert( ( nBytesLeft % ( m_sSrcFormat.nChannels * 2 ) ) == 0 );
		uint32 nFreeSrcSamples = nBytesLeft / m_sSrcFormat.nChannels / 2;
		
		if( nFreeSamples == 0 )
		{
			snooze( 1000 );
			continue;
		}
		
		//printf( "Read pointer %i, Write pointer %i ( %i ) %i\n", nRp, nWp, m_nBufferSize, nBytesLeft );
		
		//printf( "Free %i %i %i\n", nFreeBytes, nFreeSamples, nFreeSrcSamples );
	
		
		if( m_bResample )
		{
			nFreeSamples = nFreeSamples * m_sSrcFormat.nSampleRate / m_sDstFormat.nSampleRate;
			if( nFreeSamples == 0 )
			{
				snooze( 10000 );
				continue;
			}
		}
		
		if( nFreeSrcSamples < nFreeSamples )
			nFreeSamples = nFreeSrcSamples;
		
		
		nFreeBytes = nFreeSamples * m_sSrcFormat.nChannels * 2;

	
		/* Copy into the ringbuffer */
		if( !m_bResample )
		{
			for( int i = 0; i < nFreeBytes; i++ )
			{
				m_pData[nWp++] = *pSource++;
				if( nWp == m_nBufferSize )
					nWp = 0;
			}			
		}
		else
		{
			uint32 nBytesWritten = Resample( m_sSrcFormat, m_sDstFormat, (uint16*)m_pData, nWp, m_nBufferSize, (uint16*)pSource, nFreeBytes );
			pSource += nFreeBytes;
			nWp = ( nWp + nBytesWritten ) % m_nBufferSize;
			if( nBytesWritten == 0 )
				return( -EIO );
			
		}
		
		//printf( "Write pointer now %i\n", nWp );
		/* Set write pointer */
		atomic_set( m_pnWritePointer, nWp );
		nBytesLeft -= nFreeBytes;
	}
	
	return( 0 );
}

uint64 ServerOutput::GetDelay( bool bNonSharedOnly )
{
	/* Calculate number of free samples in the ringbuffer */
	int32 nRp = atomic_read( m_pnReadPointer );
	int32 nWp = atomic_read( m_pnWritePointer );
		
	uint32 nUsedBytes = 0;
	if( nWp >= nRp )
		nUsedBytes = ( nWp - nRp );
	else
		nUsedBytes = m_nBufferSize - ( nRp - nWp );
	uint32 nUsedSamples = nUsedBytes / m_sDstFormat.nChannels / 2;
	int64 nDelay = nUsedSamples * 1000 / m_sDstFormat.nSampleRate + ( bNonSharedOnly ? 0 : atomic_read( m_pnHWDelay ) );
	//std::cout<<"Delay "<<nUsedSamples<<" "<<atomic_read( m_pnHWDelay )<<" "<<nDelay<<std::endl;
	return( nDelay );
}

uint64 ServerOutput::GetBufferSize( bool bNonSharedOnly )
{
	int64 nTotal = m_nBufferSize * 1000 / m_sDstFormat.nSampleRate / m_sDstFormat.nChannels / 2 + ( bNonSharedOnly ? 0 : m_nHWBufferSize );
	return( nTotal );	
}

class ServerOutputAddon : public os::MediaAddon
{
public:
	status_t Initialize() { 
		m_nDSPCount = 0;
		os::Message cReply;
		os::MediaManager::GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_GET_DSP_COUNT, &cReply );
		int32 nCount;

		if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindInt32( "dsp_count", &nCount ) == 0 )
		{
			m_nDSPCount = nCount;
			return( 0 ); 
		}
		std::cout<<"Error: Could not get a list of DSPs from the mediaserver"<<std::endl;
		return( -1 );
	}
	os::String GetIdentifier() { return( "Media Server Audio Output" ); }
	uint32			GetOutputCount() { return( m_nDSPCount ); }
	os::MediaOutput* GetOutput( uint32 nIndex ) { return( new ServerOutput( nIndex ) ); }
private:
	int m_nDSPCount;
};

extern "C"
{
	os::MediaAddon* init_media_addon( os::String zDevice )
	{
		return( new ServerOutputAddon() );
	}
}




