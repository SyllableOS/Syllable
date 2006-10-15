/*  Ogg Vorbis RAW Codec plugin
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

#include <media/codec.h>
#include <iostream>

class RAWCodec : public os::MediaCodec
{
public:
	RAWCodec();
	~RAWCodec();
	
	os::String 		GetIdentifier();
	uint32			GetPhysicalType()
	{
		return( os::MEDIA_PHY_SOFT_CODEC );
	}
	
	os::View*		GetConfigurationView();
	
	status_t 		Open( os::MediaFormat_s sFormat, os::MediaFormat_s sExternal, bool bEncode );
	void 			Close();
	
	os::MediaFormat_s GetInternalFormat();
	os::MediaFormat_s GetExternalFormat();
	
	status_t 		CreateAudioOutputPacket( os::MediaPacket_s* psOutput );
	void			DeleteAudioOutputPacket( os::MediaPacket_s* psOutput );
	
	status_t		CreateVideoOutputPacket( os::MediaPacket_s* psOutput );
	void			DeleteVideoOutputPacket( os::MediaPacket_s* psOutput );
	
	status_t		DecodePacket( os::MediaPacket_s* psPacket, os::MediaPacket_s* psOutput );
	status_t		EncodePacket( os::MediaPacket_s* psPacket, os::MediaPacket_s* psOutput );
private:
	bool			m_bEncode;
	os::MediaFormat_s m_sFormat;
	os::MediaFormat_s m_sExternalFormat;
};

RAWCodec::RAWCodec()
{
}

RAWCodec::~RAWCodec()
{
}

os::String RAWCodec::GetIdentifier()
{
	return( "Ogg Vorbis Raw Codec" );
}


os::View* RAWCodec::GetConfigurationView()
{
	return( NULL );
}

status_t RAWCodec::Open( os::MediaFormat_s sFormat, os::MediaFormat_s sExternal, bool bEncode )
{
	if( sFormat.zName.str() != "Ogg Vorbis Raw Audio" )
		return( -1 );
		
	if( sExternal.zName.str() != "Raw Audio"  )
		return( -1 );
		
	if( !bEncode )
	{
		/* Check decoding parameters */
		if( sExternal.nSampleRate !=0 || sExternal.nChannels != 0
		|| sExternal.nWidth != 0 || sExternal.nHeight != 0 ) {
			std::cout<<"Format conversion not supported by the Ogg Vorbis RAW codec"<<std::endl;
			return( -1 );
		}
		m_sFormat = sFormat;
		m_sExternalFormat = sFormat;
		m_sExternalFormat.zName = "Raw Audio";
	} else
	{
		return( -1 );		
	}
	m_bEncode = bEncode;
	
	return( 0 );
}

void RAWCodec::Close()
{
}

os::MediaFormat_s RAWCodec::GetInternalFormat()
{
	return( m_sFormat );		
}

os::MediaFormat_s RAWCodec::GetExternalFormat()
{
	return( m_sExternalFormat );		
}

status_t RAWCodec::CreateAudioOutputPacket( os::MediaPacket_s* psOutput )
{
	psOutput->pBuffer[0] = ( uint8* )malloc( m_sFormat.nSampleRate * m_sFormat.nChannels );
	if(	psOutput->pBuffer[0] == NULL )
		return( -1 );
	return( 0 );
}


void RAWCodec::DeleteAudioOutputPacket( os::MediaPacket_s* psOutput )
{
	if( psOutput->pBuffer[0] != NULL )
		free( psOutput->pBuffer[0] );
}


status_t RAWCodec::CreateVideoOutputPacket( os::MediaPacket_s* psOutput )
{
	int i;
	for( i = 0; i < 4; i++ )
	{
		psOutput->pBuffer[i] = ( uint8* )malloc( m_sFormat.nWidth * m_sFormat.nHeight );
		if(	psOutput->pBuffer[i] == NULL )
			return( -1 );
	}
	
	return( 0 );
}


void RAWCodec::DeleteVideoOutputPacket( os::MediaPacket_s* psOutput )
{
	int i;
	for( i = 0; i < 4; i++ )
	{
		if( psOutput->pBuffer[i] != NULL )
			free( psOutput->pBuffer[i] );
	}
}


status_t RAWCodec::DecodePacket( os::MediaPacket_s* psPacket, os::MediaPacket_s* psOutput )
{
	int i;
	
	psOutput->nTimeStamp = psPacket->nTimeStamp;
	
	
	if( m_sFormat.nType == os::MEDIA_TYPE_AUDIO )
	{
		psOutput->nSize[0] = psPacket->nSize[0];
		psOutput->pPrivate = psPacket->pPrivate;
		memcpy( psOutput->pBuffer[0], psPacket->pBuffer[0], psPacket->nSize[0] );
	} 
	else 
	{
		for( i = 0; i < 4; i++ )
		{
			psOutput->nSize[i] = psPacket->nSize[i];
			psOutput->pPrivate = psPacket->pPrivate;
			memcpy( psOutput->pBuffer[i], psPacket->pBuffer[i], psPacket->nSize[i] * m_sFormat.nHeight );
		}
	}
	return( 0 );
}

status_t RAWCodec::EncodePacket( os::MediaPacket_s* psPacket, os::MediaPacket_s* psOutput )
{
	int i;
	
	psOutput->nTimeStamp = psPacket->nTimeStamp;
	
	
	if( m_sFormat.nType == os::MEDIA_TYPE_AUDIO )
	{
		psOutput->nSize[0] = psPacket->nSize[0];
		psOutput->pPrivate = psPacket->pPrivate;
		memcpy( psOutput->pBuffer[0], psPacket->pBuffer[0], psPacket->nSize[0] );
	} 
	else 
	{
		for( i = 0; i < 4; i++ )
		{
			psOutput->nSize[i] = psPacket->nSize[i];
			psOutput->pPrivate = psPacket->pPrivate;
			memcpy( psOutput->pBuffer[i], psPacket->pBuffer[i], psPacket->nSize[i] * m_sFormat.nHeight );
		}
	}
	return( 0 );
}

os::MediaCodec* init_vorbis_codec()
{
	return( new RAWCodec() );
}









































































































































