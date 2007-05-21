/*  OGG Vorbis Input plugin
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 2003 Kristian Van Der Vliet
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
 
#include <media/input.h>
#include <media/codec.h>
#include <media/addon.h>
#include <posix/fcntl.h>
#include <fcntl.h>
#include <unistd.h>
extern "C" 
{
	#include "vorbis/vorbisfile.h"
}

class VorbisInput : public os::MediaInput
{
public:
	VorbisInput();
	~VorbisInput();
	os::String 		GetIdentifier();
	uint32			GetPhysicalType()
	{
		return( os::MEDIA_PHY_SOFT_DEMUX );
	}	
	os::View*		GetConfigurationView();
	
	 bool			FileNameRequired();
	status_t 		Open( os::String zFileName );
	void 			Close();
	
	
	bool			PacketBased();
	bool			StreamBased();
	
	uint32			GetTrackCount();
	uint32			SelectTrack( uint32 nTrack );
	
	status_t		ReadPacket( os::MediaPacket_s* psPacket );
	void			FreePacket( os::MediaPacket_s* psPacket );
	
	uint64			GetLength();
	uint64			GetCurrentPosition();
	uint64			Seek( uint64 nPosition );
	
	uint32			GetStreamCount();
	os::MediaFormat_s	GetStreamFormat( uint32 nIndex );
private:
	FILE*			m_phFile;
	OggVorbis_File*	m_psVorbis;
	bool			m_bStream;
	uint64			m_nCurrentPosition;
	uint64			m_nLength;
	int				m_nBitRate;
};

VorbisInput::VorbisInput()
{
	m_phFile = NULL;
	m_psVorbis = NULL;
}

VorbisInput::~VorbisInput()
{
	Close();
}

os::View* VorbisInput::GetConfigurationView()
{
	return( NULL );
}


os::String VorbisInput::GetIdentifier()
{
	return( "Ogg Vorbis Input" );
}

bool VorbisInput::FileNameRequired()
{
	return( true );
}

status_t VorbisInput::Open( os::String zFileName )
{
	Close();
	/* Test if we can open this file */
	m_phFile = fopen( zFileName.c_str(), "r" );
	if( m_phFile == NULL )
		return( -1 );
		
		
	if( m_psVorbis != NULL )
		ov_clear( m_psVorbis );
		
	
	m_psVorbis = ( OggVorbis_File* )malloc( sizeof( OggVorbis_File ) );
	if( m_psVorbis == NULL )
	{
		fclose( m_phFile );
		m_phFile = NULL;		
		return( false );
	}
	
	
	/* If we can then open it! */
		
	if( ov_open( m_phFile, m_psVorbis, NULL, 0 ) < 0 )
	{
		ov_clear( m_psVorbis );
		free( m_psVorbis );		
		m_psVorbis = NULL;
		m_phFile = NULL;
		return( -1 );
	}
	
	m_bStream = !ov_seekable( m_psVorbis );
	m_nLength = (uint64)ov_time_total( m_psVorbis, -1 );
	m_nCurrentPosition = 0;
	
	
	return( 0 );
	
}

void VorbisInput::Close()
{
	if( m_psVorbis != NULL )
	{
		ov_clear( m_psVorbis );
		free( m_psVorbis );
		m_psVorbis = NULL;
		m_phFile = NULL;		
	}
}

bool VorbisInput::PacketBased()
{
	return( true );
}


bool VorbisInput::StreamBased()
{
	return( m_bStream );
}

uint32 VorbisInput::GetTrackCount()
{
	return( 1 );
}

uint32 VorbisInput::SelectTrack( uint32 nTrack )
{
	return( 0 );
}

status_t VorbisInput::ReadPacket( os::MediaPacket_s* psPacket )
{
	char nBuffer[4096];
	int nStream;
	
	if( m_psVorbis == NULL )
		return( -1 );
	
	int nRead = ov_read( m_psVorbis, nBuffer, 4096, 0, 2, 1, &nStream );
	if( nRead <= 0 )
		return( -1 );
	
	psPacket->nStream = nStream;
	psPacket->pBuffer[0] = ( uint8* )malloc( nRead );
	memcpy( psPacket->pBuffer[0], nBuffer, nRead );
	psPacket->nSize[0] = nRead;
	psPacket->nTimeStamp = ~0;
	psPacket->pPrivate = m_psVorbis;
	
	//cout<<"Read packet of stream "<<psPacket->nStream<<" Size: "<<psPacket->nSize[0]<<endl;
	
	return( 0 );
}

void VorbisInput::FreePacket( os::MediaPacket_s* psPacket )
{
	free( psPacket->pBuffer[0] );
}

uint64 VorbisInput::GetLength()
{
	return( m_nLength );
}


uint64 VorbisInput::GetCurrentPosition()
{
	m_nCurrentPosition = (uint64)ov_time_tell( m_psVorbis );
	return( m_nCurrentPosition );
}

uint64 VorbisInput::Seek( uint64 nPosition )
{
	ov_time_seek( m_psVorbis, nPosition );
	m_nCurrentPosition = (uint64)ov_time_tell( m_psVorbis );
	return( m_nCurrentPosition );
}

uint32 VorbisInput::GetStreamCount()
{
	if( m_psVorbis == NULL )
		return( 0 );
	return( ov_streams( m_psVorbis ) );
}

os::MediaFormat_s VorbisInput::GetStreamFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	
	if( m_psVorbis == NULL )
		return( sFormat );
	
	sFormat.nType = os::MEDIA_TYPE_AUDIO; // ???
	sFormat.zName = "Ogg Vorbis Raw Audio";
	sFormat.nBitRate = ov_bitrate( m_psVorbis, nIndex );
	sFormat.nSampleRate = ov_info( m_psVorbis, nIndex )->rate; 
	sFormat.nChannels = ov_info( m_psVorbis, nIndex )->channels;
	sFormat.bVBR = false;
	
	
	sFormat.pPrivate = m_psVorbis;
	
	return( sFormat );
}


extern os::MediaCodec* init_vorbis_codec();

class VorbisAddon : public os::MediaAddon
{
public:
	status_t Initialize() { return( 0 ); }
	os::String GetIdentifier() { return( "Ogg Vorbis" ); }
	uint32			GetCodecCount() { return( 1 ); }
	os::MediaCodec*		GetCodec( uint32 nIndex ) { return( init_vorbis_codec() ); }
	uint32			GetInputCount() { return( 1 ); }
	os::MediaInput*		GetInput( uint32 nIndex ) { return( new VorbisInput() ); }
};

extern "C"
{
	os::MediaAddon* init_media_addon()
	{
		return( new VorbisAddon() );
	}

}

