/*  AAC Codec plugin
 *  Copyright (C) 2006 Arno Klenke
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <media/codec.h>
#include <media/addon.h>
#include <gui/stringview.h>
#include <neaacdec.h>
#include <iostream>

#ifdef USE_FFMPEG
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#endif

class AACCodec : public os::MediaCodec
{
public:
	AACCodec();
	~AACCodec();
	
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
	
	status_t		DecodePacket( os::MediaPacket_s* psPacket, os::MediaPacket_s* psOutput );
private:
	os::MediaFormat_s m_sFormat;
	os::MediaFormat_s m_sExternalFormat;
	bool m_bStreamInfoRead;
	bool m_bReorder;
	NeAACDecHandle m_sHandle;
};

AACCodec::AACCodec()
{
	m_sHandle = NeAACDecOpen();
}

AACCodec::~AACCodec()
{
	NeAACDecClose( m_sHandle );
}

os::String AACCodec::GetIdentifier()
{
	return( "AAC Codec" );
}


os::View* AACCodec::GetConfigurationView()
{
	os::StringView* pcView = new os::StringView( os::Rect(), "info", "FAAD2 AAC/HE-AAC/HE-AACv2/DRM decoder (c) Nero AG, www.nero.com" );
	return( pcView );
}

status_t AACCodec::Open( os::MediaFormat_s sFormat, os::MediaFormat_s sExternal, bool bEncode )
{
	if( bEncode )
		return( -1 );
	if( sFormat.zName.str() != "aac" )
		return( -1 );
	
	if( sExternal.zName.str() != "Raw Audio" )
		return( -1 );
	m_bStreamInfoRead = false;
	m_bReorder = false;
		
	/* Check if we have ffmpeg information */
	#ifdef USE_FFMPEG
	if( sFormat.pPrivate != NULL )
	{
		AVStream sDecodeStream;
		sDecodeStream = *( ( AVStream * ) ( sFormat.pPrivate ) );
		AVCodecContext* psCodec = sDecodeStream.codec;
		
		
		if( psCodec->extradata )
		{
			unsigned long nSampleRate = 0;
			unsigned char nChannels = 0;
			int nError = NeAACDecInit2( m_sHandle, (uint8*)psCodec->extradata,
				psCodec->extradata_size, &nSampleRate, &nChannels );
			if( nError == 0 )
			{
				sFormat.nSampleRate = nSampleRate;
				sFormat.nChannels = nChannels;
				m_bStreamInfoRead = true;
				printf( "AAC: found ffmpeg information, channels: %i samplerate: %i\n", nChannels, (int)nSampleRate );
			}
			
		}
	}
	#endif
	
	if( !m_bStreamInfoRead )
	{
		printf( "AAC: Will try to get stream info while decoding!\n" );
		sFormat.nSampleRate = 44100;
	}
	
	m_sFormat = sFormat;
	m_sExternalFormat = sExternal;
	m_sExternalFormat.nChannels = sFormat.nChannels;
	m_sExternalFormat.nSampleRate = sFormat.nSampleRate;
	
	/* Check decoding parameters */
	if( ( sExternal.nSampleRate !=0 && sFormat.nSampleRate != sExternal.nSampleRate ) || 
	( sExternal.nChannels != 0 && sFormat.nChannels != sExternal.nChannels )
	|| ( sExternal.nWidth != 0 && sExternal.nWidth != sFormat.nWidth )
	 || ( sExternal.nHeight != 0 && sExternal.nHeight != sFormat.nHeight ) ) {
	 	if( !( sFormat.nChannels > 2 && ( sExternal.nChannels >= 2 &&
	 	sExternal.nChannels <= 8 ) ) )
		{
		 	printf( "%i %i %i %i\n", sExternal.nSampleRate, sFormat.nSampleRate, sExternal.nChannels, sFormat.nChannels );
			std::cout<<"Format conversion not supported by the aac codec"<<std::endl;
			return( -1 );
		}
	}
	
	if( sExternal.nChannels == 0 )
		sExternal.nChannels = sFormat.nChannels;
	
	/* Check if we should reorder the channels */
	if( sFormat.nChannels > 2 && ( sExternal.nChannels >= 2 &&
	 	sExternal.nChannels <= 8 ) )
 	{
 		m_bReorder = true;
 		printf( "Reordering channels from %i to %i\n", sFormat.nChannels, sExternal.nChannels );
 		m_sExternalFormat.nChannels = sExternal.nChannels;
 	}
	
	
	/* Set the configuration */
	NeAACDecConfigurationPtr psConfig = NeAACDecGetCurrentConfiguration( m_sHandle );
	if( psConfig )
	{
		psConfig->outputFormat = FAAD_FMT_16BIT;
		psConfig->defSampleRate = m_sExternalFormat.nSampleRate;
		psConfig->defObjectType = LC;
		NeAACDecSetConfiguration( m_sHandle, psConfig );
	}
	return( 0 );
}

void AACCodec::Close()
{
}

os::MediaFormat_s AACCodec::GetInternalFormat()
{
	return( m_sFormat );		
}

os::MediaFormat_s AACCodec::GetExternalFormat()
{
	return( m_sExternalFormat );		
}

status_t AACCodec::CreateAudioOutputPacket( os::MediaPacket_s* psOutput )
{
	psOutput->pBuffer[0] = ( uint8* )malloc( m_sExternalFormat.nSampleRate * m_sFormat.nChannels );
	if(	psOutput->pBuffer[0] == NULL )
		return( -1 );
	return( 0 );
}


void AACCodec::DeleteAudioOutputPacket( os::MediaPacket_s* psOutput )
{
	if( psOutput->pBuffer[0] != NULL )
		free( psOutput->pBuffer[0] );
}

static int g_asChannelTable[][8] = 
{
	{ FRONT_CHANNEL_LEFT, FRONT_CHANNEL_CENTER, 0, 0, 0, 0, 0, 0 }, // 2 channels
	{ FRONT_CHANNEL_LEFT, FRONT_CHANNEL_RIGHT, FRONT_CHANNEL_CENTER, 0, 0, 0, 0, 0 }, // 3 channels
	{ FRONT_CHANNEL_LEFT, FRONT_CHANNEL_CENTER, BACK_CHANNEL_LEFT, BACK_CHANNEL_RIGHT, 0, 0, 0, 0 }, // 4 channels
	{ FRONT_CHANNEL_LEFT, FRONT_CHANNEL_RIGHT, BACK_CHANNEL_LEFT, BACK_CHANNEL_RIGHT, FRONT_CHANNEL_CENTER, 0, 0, 0 }, // 5 channels	
	{ FRONT_CHANNEL_LEFT, FRONT_CHANNEL_RIGHT, BACK_CHANNEL_LEFT, BACK_CHANNEL_RIGHT, FRONT_CHANNEL_CENTER, LFE_CHANNEL, 0, 0 }, // 6 channels	
	{ FRONT_CHANNEL_LEFT, FRONT_CHANNEL_RIGHT, BACK_CHANNEL_LEFT, BACK_CHANNEL_RIGHT, FRONT_CHANNEL_CENTER, LFE_CHANNEL, SIDE_CHANNEL_LEFT, 0 }, // 7 channels			
	{ FRONT_CHANNEL_LEFT, FRONT_CHANNEL_RIGHT, BACK_CHANNEL_LEFT, BACK_CHANNEL_RIGHT, FRONT_CHANNEL_CENTER, LFE_CHANNEL, SIDE_CHANNEL_LEFT, SIDE_CHANNEL_RIGHT } // 8 channels		
};

status_t AACCodec::DecodePacket( os::MediaPacket_s* psPacket, os::MediaPacket_s* psOutput )
{
	if( !m_bStreamInfoRead )
	{
		/* Try to get stream info now */
		unsigned long nSampleRate = 0;
		unsigned char nChannels = 0;
		int nError = NeAACDecInit2( m_sHandle, psPacket->pBuffer[0],
			psPacket->nSize[0], &nSampleRate, &nChannels );
		if( nError < 0 )
		{
			printf( "AAC: Could not get stream info!\n" );
			return( -1 );
		}
		else
		{
			if( (int)nChannels != m_sExternalFormat.nChannels || (int)nSampleRate != m_sExternalFormat.nSampleRate )
				printf( "AAC: Stream info does not match default values!\n" );
		}
		m_bStreamInfoRead = true;
	}
	
	NeAACDecFrameInfo sFrame;
	void* pOut = NeAACDecDecode( m_sHandle, &sFrame, psPacket->pBuffer[0], psPacket->nSize[0] );
	
	if( sFrame.error > 0 )
	{
		printf( "AAC: decode error!\n" );
		return( -1 );
	}
	
	sFrame.samples *= 2;
	
	
	
	//printf( "Decoded %i bytes with %i samples\n", sFrame.bytesconsumed, sFrame.samples );
	if( m_bReorder )
	{
		/* Reorder samples */
		int nSamples = sFrame.samples / m_sFormat.nChannels / 2;
		int nSrcChannels = m_sFormat.nChannels;
		int nDstChannels = m_sExternalFormat.nChannels;
		int* pnTable = &g_asChannelTable[m_sExternalFormat.nChannels-2][0];
		for( int i = 0; i < nDstChannels; i++ )
		{
			int nChannel = pnTable[i];
			
			for( int j = 0; j < nSrcChannels; j++ )
			{
				if( sFrame.channel_position[j] == nChannel )
				{
					uint16* pSrc = ((uint16*)pOut) + j;
					uint16* pDst = ((uint16*)psOutput->pBuffer[0]) + i;
					//printf( "Channel %i -> %i\n", i, j );
					for( int s = 0; s < nSamples; s++ )
					{
						*pDst = *pSrc;
						pSrc += nSrcChannels;
						pDst += nDstChannels;
					}
					break;
				}
			}
		}
		psOutput->nSize[0] = nSamples * 2 * nDstChannels;
		//printf( "%i %i %i\n", sFrame.samples, nSamples, psOutput->nSize[0] );
	}
	else {
		memcpy( psOutput->pBuffer[0], pOut, sFrame.samples );
		psOutput->nSize[0] = sFrame.samples;
	}
	
	psOutput->nTimeStamp = psPacket->nTimeStamp;
	
	return( 0 );
}


class AACAddon : public os::MediaAddon
{
public:
	status_t Initialize() { return( 0 ); }
	os::String GetIdentifier() { return( "AAC" ); }
	uint32			GetCodecCount() { return( 1 ); }
	os::MediaCodec*		GetCodec( uint32 nIndex ) { return( new AACCodec() ); }
};

extern "C"
{
	os::MediaAddon* init_media_addon( os::String zDevice )
	{
		return( new AACAddon() );
	}

}





















