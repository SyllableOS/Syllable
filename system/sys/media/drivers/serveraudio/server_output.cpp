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
#include <iostream>


class ServerOutput : public os::MediaOutput
{
public:
	ServerOutput( int nIndex );
	~ServerOutput();
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
	
private:
	int					m_nIndex;
	os::Messenger		m_cMediaServerLink;
	os::MediaFormat_s 	m_sFormat;
	int32				m_nHandle;
	area_id				m_hServerArea;
	area_id				m_hArea;
	uint8*				m_pBuffer;
};


ServerOutput::ServerOutput( int nIndex )
{
	m_nIndex = nIndex;
}

ServerOutput::~ServerOutput()
{
}

os::String ServerOutput::GetIdentifier()
{
	os::String cName;

	os::Message cReply;
	os::Message cMsg( os::MEDIA_SERVER_GET_DSP_INFO );
	cMsg.AddInt32( "handle", m_nIndex );
	os::MediaManager::GetInstance()->GetServerLink().SendMessage( &cMsg, &cReply );

	if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindString( "name", &cName ) == 0 )
		return( os::String( os::String( "Media Server (" ) + cName + os::String( ")" ) ) );
	else
		return( "Media Server Audio Output" );
}


os::View* ServerOutput::GetConfigurationView()
{
	return( NULL );
//	return( new PrefsView( os::Rect() ) );
}

void ServerOutput::Flush()
{
	os::Message cMsg( os::MEDIA_SERVER_START_AUDIO_STREAM );
	cMsg.AddInt32( "handle", m_nHandle );
	m_cMediaServerLink.SendMessage( &cMsg );
}

bool ServerOutput::FileNameRequired()
{
	return( false );
}

status_t ServerOutput::Open( os::String zFileName )
{
	
	/* Look for media manager port */	
	port_id nPort;
	os::Message cReply;
	if( ( nPort = find_port( "l:media_server" ) ) < 0 ) {
		std::cout<<"Could not connect to media server!"<<std::endl;
		return( -1 );
	}
	m_cMediaServerLink = os::Messenger( nPort );
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
	}
}


void ServerOutput::Clear()
{
	os::Message cMsg( os::MEDIA_SERVER_CLEAR_AUDIO_STREAM );
	os::Message cReply;
	cMsg.AddInt32( "handle", m_nHandle );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
}

uint32 ServerOutput::GetOutputFormatCount()
{
	return( 1 );
}

os::MediaFormat_s ServerOutput::GetOutputFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	MEDIA_CLEAR_FORMAT( sFormat );
	sFormat.zName = "Raw Audio";
	sFormat.nType = os::MEDIA_TYPE_AUDIO;
	
	return( sFormat );
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
	
	/* Save Media format  */
	m_sFormat = sFormat;
	
	/* Tell media server */
	os::Message cMsg( os::MEDIA_SERVER_CREATE_AUDIO_STREAM );
	os::Message cReply;
	
	cMsg.AddString( "name", zName );
	cMsg.AddInt32( "channels", m_sFormat.nChannels );
	cMsg.AddInt32( "sample_rate", m_sFormat.nSampleRate );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
	if( cReply.GetCode() != os::MEDIA_SERVER_OK ) {
		return( -1 );
	}
	if( cReply.FindInt32( "handle", &m_nHandle ) != 0 ||
		cReply.FindInt32( "area", (int32*)&m_hServerArea ) != 0 )
		return( -1 );
	/* Clone area */
	m_pBuffer = NULL;
	m_hArea = clone_area( "media_server_output", (void**)&m_pBuffer, AREA_FULL_ACCESS, AREA_NO_LOCK,
						m_hServerArea );
	if( m_hArea < 0 )
		return( -1 );
		
	return( 0 );
}

status_t ServerOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket )
{
	/* Copy the packet into the area */
	memcpy( m_pBuffer, psPacket->pBuffer[0], psPacket->nSize[0] );
	
	/* Tell the media server about this */
	os::Message cMsg( os::MEDIA_SERVER_FLUSH_AUDIO_STREAM );
	os::Message cReply;
	cMsg.AddInt32( "handle", m_nHandle );
	cMsg.AddInt32( "size", psPacket->nSize[0] );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
	
	if( cReply.GetCode() != os::MEDIA_SERVER_OK )
		return( -1 );
	
	return( 0 );
}

uint64 ServerOutput::GetDelay()
{
	int64 nDelay;
	os::Message cMsg( os::MEDIA_SERVER_GET_AUDIO_STREAM_DELAY );
	os::Message cReply;
	cMsg.AddInt32( "handle", m_nHandle );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
	if( cReply.FindInt64( "delay", &nDelay ) != 0 )
		return( 0 );
	//std::cout<<"Delay "<<nDelay<<std::endl;
	return( nDelay );
}

uint32 ServerOutput::GetUsedBufferPercentage()
{
	int32 nPercentage;
	os::Message cMsg( os::MEDIA_SERVER_GET_AUDIO_STREAM_PERCENTAGE );
	os::Message cReply;
	cMsg.AddInt32( "handle", m_nHandle );
	m_cMediaServerLink.SendMessage( &cMsg, &cReply );
	if( cReply.FindInt32( "percentage", &nPercentage ) != 0 )
		return( 0 );
	return( nPercentage );
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
	os::MediaAddon* init_media_addon()
	{
		return( new ServerOutputAddon() );
	}
}




