/*  FFMPEG Codec plugin
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
#include <atheos/kernel.h>
#include <iostream>

#define FFMPEG_CACHE_SIZE 256 * 1024

extern "C"
{
#include <ffmpeg/avformat.h>
#include <ffmpeg/avcodec.h>
}

class FFMpegCodec:public os::MediaCodec
{
public:
	FFMpegCodec();
	~FFMpegCodec();

	os::String GetIdentifier();
	uint32			GetPhysicalType()
	{
		return( os::MEDIA_PHY_SOFT_CODEC );
	}
	os::View * GetConfigurationView();

	status_t Open( os::MediaFormat_s sFormat, os::MediaFormat_s sExternal, bool bEncode );
	void Close();

	os::MediaFormat_s GetInternalFormat();
	os::MediaFormat_s GetExternalFormat();

	status_t CreateAudioOutputPacket( os::MediaPacket_s * psOutput );
	void DeleteAudioOutputPacket( os::MediaPacket_s * psOutput );

	status_t CreateVideoOutputPacket( os::MediaPacket_s * psOutput );
	void DeleteVideoOutputPacket( os::MediaPacket_s * psOutput );

	status_t DecodePacket( os::MediaPacket_s * psPacket, os::MediaPacket_s * psOutput );
	status_t ParsePacket( os::MediaPacket_s * psPacket, os::MediaPacket_s * psOutput );
	status_t EncodePacket( os::MediaPacket_s * psPacket, os::MediaPacket_s * psOutput );
private:
	uint32 Resample( uint16 *pDst, uint16 *pSrc, uint32 nLength );
	bool m_bEncode;
	AVStream* m_sDecodeStream;
	AVCodecContext *m_sDecodeContext;
	AVStream m_sEncodeStream;
	os::MediaFormat_s m_sInternalFormat;
	os::MediaFormat_s m_sExternalFormat;
	uint8 *m_pCache;
	uint32 m_nCachePointer;
	bool m_bEncodeResample;
	int m_nThreads;
};

FFMpegCodec::FFMpegCodec()
{
	/* Create encoding cache */
	m_pCache = ( uint8 * )malloc( FFMPEG_CACHE_SIZE );
	m_nThreads = 1;
	memset( m_pCache, 0, FFMPEG_CACHE_SIZE );
}

FFMpegCodec::~FFMpegCodec()
{
	free( m_pCache );
}

os::String FFMpegCodec::GetIdentifier()
{
	return ( "FFMpeg Codec" );
}


os::View * FFMpegCodec::GetConfigurationView()
{
	return ( NULL );
}

status_t FFMpegCodec::Open( os::MediaFormat_s sFormat, os::MediaFormat_s sExternal, bool bEncode )
{
	CodecID id;
	AVCodec *psCodec;
	
	if( bEncode )
		psCodec = avcodec_find_encoder_by_name( sFormat.zName.c_str() );
	else
		psCodec = avcodec_find_decoder_by_name( sFormat.zName.c_str() );

	if( psCodec == NULL )
		return ( -1 );

	id = psCodec->id;

	if( sExternal.zName.str() != "Raw Audio" && sExternal.zName.str(  ) != "Raw Video" )
		return ( -1 );


	if( !bEncode )
	{
		if( sFormat.pPrivate == NULL )
		{
			std::cout << "The FFMpeg codec does only work together with the FFMpeg input" << std::endl;
			return ( -1 );
		}
		/* Check decoding parameters */
		if( ( sExternal.nSampleRate != 0 && sFormat.nSampleRate != sExternal.nSampleRate ) || ( sExternal.nChannels != 0 && sFormat.nChannels != sExternal.nChannels ) || ( sExternal.nWidth != 0 && sExternal.nWidth != sFormat.nWidth ) || ( sExternal.nHeight != 0 && sExternal.nHeight != sFormat.nHeight ) )
		{
			printf( "%i %i %i %i\n", sExternal.nSampleRate, sFormat.nSampleRate, sExternal.nChannels, sFormat.nChannels );
			std::cout << "Format conversion not supported by the FFMpeg codec" << std::endl;
			return ( -1 );
		}
	}
	else
	{
		/* Check encoding parameters */
		if( ( sFormat.nSampleRate != 0 && sFormat.nSampleRate != sExternal.nSampleRate ) || ( sFormat.nChannels != 0 && sFormat.nChannels != sExternal.nChannels ) || ( sFormat.nWidth != 0 && sExternal.nWidth != sFormat.nWidth ) || ( sFormat.nHeight != 0 && sExternal.nHeight != sFormat.nHeight ) )
		{
			printf( "%i %i %i %i\n", sExternal.nSampleRate, sFormat.nSampleRate, sExternal.nChannels, sFormat.nChannels );
			std::cout << "Format conversion not supported by the FFMpeg codec" << std::endl;
			return ( -1 );
		}
	}

	MEDIA_CLEAR_FORMAT( m_sInternalFormat );
	MEDIA_CLEAR_FORMAT( m_sExternalFormat );
	m_bEncode = bEncode;

	m_sInternalFormat = sFormat;
	m_sInternalFormat.pPrivate = NULL;
	m_sExternalFormat = sExternal;
	m_sExternalFormat.bVFR = m_sInternalFormat.bVFR;



	/* Set decoding parameters */

	if( !m_bEncode )
	{
		m_sDecodeStream = ( ( AVStream * ) ( sFormat.pPrivate ) );
		m_sDecodeContext = m_sDecodeStream->codec;

		m_sDecodeContext->flags |= CODEC_FLAG_EMU_EDGE;
		m_sDecodeContext->flags2 |= CODEC_FLAG2_FAST;

	    m_sDecodeContext->idct_algo= FF_IDCT_AUTO;

	    m_sDecodeContext->skip_frame= AVDISCARD_DEFAULT;
	    m_sDecodeContext->skip_idct= AVDISCARD_DEFAULT;
	    m_sDecodeContext->skip_loop_filter= AVDISCARD_DEFAULT;

		m_sDecodeContext->workaround_bugs = 1;
		m_sDecodeContext->error_resilience = FF_ER_CAREFUL;
		m_sDecodeContext->error_concealment = 3;

		AVCodec *psCodec = avcodec_find_decoder( id );

		if( psCodec == NULL || avcodec_open( m_sDecodeContext, psCodec ) < 0 )
		{
			std::cout << "Error while opening codec " << sFormat.zName.c_str() << std::endl;
			return ( -1 );
		}

		if( sFormat.nType == os::MEDIA_TYPE_VIDEO )
		{
			system_info sInfo;
			get_system_info( &sInfo );
			m_nThreads = std::min( sInfo.nCPUCount, m_sDecodeContext->height );
			if( m_nThreads > 1 )
			{
				if( avcodec_thread_init( m_sDecodeContext, m_nThreads ) == 0 )
					m_sDecodeContext->thread_count = m_nThreads;
			}
		}
	}


	/* Set encoding parameters */
	if( m_bEncode && sFormat.nType == os::MEDIA_TYPE_AUDIO )
	{
		/* Prepare stream for the ffmpeg output */
		memset( &m_sEncodeStream, 0, sizeof( m_sEncodeStream ) );
		m_sEncodeStream.codec = avcodec_alloc_context2( CODEC_TYPE_AUDIO );
		m_sEncodeStream.start_time = AV_NOPTS_VALUE;
		m_sEncodeStream.duration = AV_NOPTS_VALUE;
		m_sEncodeStream.cur_dts = AV_NOPTS_VALUE;
		m_sEncodeStream.first_dts = AV_NOPTS_VALUE;
		av_set_pts_info( &m_sEncodeStream, 33, 1, 90000 );
		m_sEncodeStream.last_IP_pts = AV_NOPTS_VALUE;
		for( int i=0; i < MAX_REORDER_DELAY + 1; i++ )
        	m_sEncodeStream.pts_buffer[i]= AV_NOPTS_VALUE;
		AVCodecContext *sEnc = m_sEncodeStream.codec;

		sEnc->codec_type = CODEC_TYPE_AUDIO;
		sEnc->codec_id = id;


		if( sFormat.nBitRate == 0 )
			sEnc->bit_rate = sExternal.nBitRate;
		else
			sEnc->bit_rate = sFormat.nBitRate;
		sEnc->sample_rate = sExternal.nSampleRate;
		sEnc->time_base = (AVRational){1, sEnc->sample_rate};
		sEnc->strict_std_compliance = 1;
		sEnc->thread_count = 1;
		
		
		/* Always do channel conversion for now */

		if( sExternal.nChannels > 2 )
		{
			sEnc->channels = 2;
			m_bEncodeResample = true;
		}
		else
		{
			sEnc->channels = sExternal.nChannels;
			m_bEncodeResample = false;
		}

		/* Open encoder */

		AVCodec *psEncCodec = avcodec_find_encoder( sEnc->codec_id );

		if( psEncCodec )
		{
			if( avcodec_open( sEnc, psEncCodec ) >= 0 )
			{
				m_sInternalFormat.pPrivate = &m_sEncodeStream;
			} else
			{
				printf( "Error: Could not open encoder %s\n", sFormat.zName.c_str() );
				return( -1 );
			}
		} else
		{
			printf( "Error: Could not find encoder %s\n", sFormat.zName.c_str() );
			return( -1 );
		}
	}

	if( m_bEncode && sFormat.nType == os::MEDIA_TYPE_VIDEO )
	{
		/* Prepare stream for the ffmpeg output */
		memset( &m_sEncodeStream, 0, sizeof( m_sEncodeStream ) );
		m_sEncodeStream.codec = avcodec_alloc_context2( CODEC_TYPE_VIDEO );
		m_sEncodeStream.start_time = AV_NOPTS_VALUE;
		m_sEncodeStream.duration = AV_NOPTS_VALUE;
		m_sEncodeStream.cur_dts = AV_NOPTS_VALUE;
		m_sEncodeStream.first_dts = AV_NOPTS_VALUE;		
		av_set_pts_info( &m_sEncodeStream, 33, 1, 90000 );
		m_sEncodeStream.last_IP_pts = AV_NOPTS_VALUE;
		for( int i=0; i < MAX_REORDER_DELAY + 1; i++ )
        	m_sEncodeStream.pts_buffer[i]= AV_NOPTS_VALUE;

		AVCodecContext *sEnc = m_sEncodeStream.codec;

		sEnc->codec_type = CODEC_TYPE_VIDEO;
		sEnc->codec_id = id;
		if( sFormat.nBitRate == 0 )
			sEnc->bit_rate = sExternal.nBitRate;
		else
			sEnc->bit_rate = sFormat.nBitRate;


		/* No variable framerates supported for now */
#if 0
		if( sExternal.bVFR )
		{
			std::cout << "Cannot encode video with variable framerate!" << std::endl;
			return ( -1 );
		}
#endif
        AVRational time_base = av_d2q(sExternal.vFrameRate, DEFAULT_FRAME_RATE_BASE);
		sEnc->bit_rate_tolerance = 100000000;
		//printf( "%f %i %i %i %i %f\n", sExternal.vFrameRate, sEnc->bit_rate, sEnc->bit_rate_tolerance, time_base.num, time_base.den, av_q2d( time_base ) );

		sEnc->time_base.den = time_base.num;
		sEnc->time_base.num = time_base.den;
		sEnc->width = sExternal.nWidth;
		sEnc->height = sExternal.nHeight;
//              sEnc->aspect_ratio = sExternal.nWidth / sExternal.nHeight;
		sEnc->pix_fmt = PIX_FMT_YUV420P;
		
		
		sEnc->max_qdiff = 3;
		sEnc->rc_eq = "tex^qComp";		
		sEnc->thread_count = 1;
        sEnc->me_threshold= 0;
        sEnc->intra_dc_precision= 0;
        sEnc->strict_std_compliance = 0;

		/* Open encoder */
		AVCodec *psEncCodec = avcodec_find_encoder( sEnc->codec_id );

		if( psEncCodec )
		{
			if( avcodec_open( sEnc, psEncCodec ) >= 0 )
			{
				m_sInternalFormat.pPrivate = &m_sEncodeStream;
			}
			else
			{
				printf( "Error: Could not open encoder %s\n", sFormat.zName.c_str() );
				return ( -1 );
			}
		}
		else
		{
			printf( "Error: Could not find encoder %s\n", sFormat.zName.c_str() );
			return ( -1 );
		}
	}
	m_nCachePointer = 0;

	return ( 0 );
}

void FFMpegCodec::Close()
{
	if( !m_bEncode )
	{
		if( m_nThreads > 1 )
			avcodec_thread_free( m_sDecodeContext );
		avcodec_close( m_sDecodeContext );
	}
	else
		avcodec_close( m_sEncodeStream.codec );
	
}

os::MediaFormat_s FFMpegCodec::GetInternalFormat()
{
	os::MediaFormat_s sFormat = m_sInternalFormat;
	if( !m_bEncode )
	{
		if( m_sDecodeContext->pix_fmt == PIX_FMT_YUV422 )
			sFormat.eColorSpace = os::CS_YUV422;
		else if( m_sDecodeContext->pix_fmt == PIX_FMT_YUV411P )
			sFormat.eColorSpace = os::CS_YUV411;
		else if( m_sDecodeContext->pix_fmt == PIX_FMT_YUV420P ||
				 m_sDecodeContext->pix_fmt == PIX_FMT_YUVJ420P )
			sFormat.eColorSpace = os::CS_YUV12;
		else if( m_sDecodeContext->pix_fmt == PIX_FMT_YUV422P ||
				 m_sDecodeContext->pix_fmt == PIX_FMT_YUVJ422P )
			sFormat.eColorSpace = os::CS_YUV422;
		else if( m_sDecodeContext->pix_fmt == PIX_FMT_RGB24 )
			sFormat.eColorSpace = os::CS_RGB24;

	}
	else
	{
		AVCodecContext *sEnc = m_sEncodeStream.codec;

		sFormat.eColorSpace = os::CS_YUV12;
		sFormat.nSampleRate = sEnc->sample_rate;
		sFormat.nChannels = sEnc->channels;
		sFormat.vFrameRate = ( double )m_sEncodeStream.r_frame_rate.num / ( double )m_sEncodeStream.r_frame_rate.den;
		sFormat.nWidth = sEnc->width;
		sFormat.nHeight = sEnc->height;
	}

	return ( sFormat );
}

os::MediaFormat_s FFMpegCodec::GetExternalFormat()
{
	os::MediaFormat_s sFormat = m_sExternalFormat;
	if( !m_bEncode )
	{
		if( m_sDecodeContext->pix_fmt == PIX_FMT_YUV422 )
			sFormat.eColorSpace = os::CS_YUV422;
		else if( m_sDecodeContext->pix_fmt == PIX_FMT_YUV411P )
			sFormat.eColorSpace = os::CS_YUV411;
		else if( m_sDecodeContext->pix_fmt == PIX_FMT_YUV420P ||
				 m_sDecodeContext->pix_fmt == PIX_FMT_YUVJ420P )
			sFormat.eColorSpace = os::CS_YUV12;
		else if( m_sDecodeContext->pix_fmt == PIX_FMT_YUV422P ||
				m_sDecodeContext->pix_fmt == PIX_FMT_YUVJ422P )
			sFormat.eColorSpace = os::CS_YUV422;
		else if( m_sDecodeContext->pix_fmt == PIX_FMT_RGB24 )
			sFormat.eColorSpace = os::CS_RGB24;

		sFormat.nSampleRate = m_sDecodeContext->sample_rate;
		sFormat.nChannels = m_sDecodeContext->channels;
		sFormat.vFrameRate = ( double )m_sDecodeStream->r_frame_rate.num / ( double )m_sDecodeStream->r_frame_rate.den;
		sFormat.nWidth = m_sDecodeContext->width;
		sFormat.nHeight = m_sDecodeContext->height;
	}
	else
	{
		sFormat.eColorSpace = os::CS_YUV12;
	}


	return ( sFormat );
}

status_t FFMpegCodec::CreateAudioOutputPacket( os::MediaPacket_s * psOutput )
{
	psOutput->pBuffer[0] = ( uint8 * )malloc( 4 * AVCODEC_MAX_AUDIO_FRAME_SIZE );
	if( psOutput->pBuffer[0] == NULL )
		return ( -1 );
	return ( 0 );
}


void FFMpegCodec::DeleteAudioOutputPacket( os::MediaPacket_s * psOutput )
{
	if( psOutput->pBuffer[0] != NULL )
		free( psOutput->pBuffer[0] );
}


status_t FFMpegCodec::CreateVideoOutputPacket( os::MediaPacket_s * psOutput )
{
	if( !m_bEncode )
		return ( 0 );
	AVCodecContext *sEnc = m_sEncodeStream.codec;

	psOutput->pBuffer[0] = ( uint8 * )malloc( sEnc->width * sEnc->height * 4 );
	if( psOutput->pBuffer[0] == NULL )
		return ( -1 );
	return ( 0 );
}


void FFMpegCodec::DeleteVideoOutputPacket( os::MediaPacket_s * psOutput )
{
	if( !m_bEncode )
		return;
	if( psOutput->pBuffer[0] != NULL )
		free( psOutput->pBuffer[0] );

	AVFrame *psFrame = (AVFrame*)psOutput->pPrivate;
	if( psFrame );
		av_free( psFrame );
}


status_t FFMpegCodec::ParsePacket( os::MediaPacket_s * psPacket, os::MediaPacket_s* psOutput )
{
	m_sDecodeContext->skip_frame = AVDISCARD_NONKEY;
	status_t nError = DecodePacket( psPacket, psOutput );
	m_sDecodeContext->skip_frame = AVDISCARD_DEFAULT;
	return( nError );
}


status_t FFMpegCodec::DecodePacket( os::MediaPacket_s * psPacket, os::MediaPacket_s * psOutput )
{
	if( m_sInternalFormat.nType == os::MEDIA_TYPE_AUDIO )
	{
		int nSize = psPacket->nSize[0];
		uint8 *pInput = psPacket->pBuffer[0];
		uint8 *pOut = psOutput->pBuffer[0];
		int nOutSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;

		psOutput->nSize[0] = 0;

		while( nSize > 0 )
		{

			int nLen = avcodec_decode_audio2( m_sDecodeContext, ( short * )pOut, &nOutSize,
				pInput, nSize );

			if( nLen < 0 )
			{
				return ( -1 );
			}
			if( nOutSize <= 0 )
			{
			}
			else
			{
				psOutput->nSize[0] += nOutSize;
				pOut += nOutSize;
			}
			nSize -= nLen;
			pInput += nLen;
		}
		
		if( m_sInternalFormat.nChannels == 6 )
		{
			/* Reorder samples:
				Left - Right - Center - LFE - Back left - Back right
				->
				Left - Right - Back left - Back right - Center - LFE
			*/
			uint32 *pSwap1 = (uint32*)psOutput->pBuffer[0] + 1;
			uint32 *pSwap2 = (uint32*)psOutput->pBuffer[0] + 2;
			uint32 nTemp;
			for( uint i = 0; i < psOutput->nSize[0]; i+= 12 )
			{
				nTemp = *pSwap1;
				*pSwap1 = *pSwap2;
				*pSwap2 = nTemp;
				pSwap1 += 3;
				pSwap2 += 3;
			}
		}

		if( psOutput->nSize[0] > 0 )
			psOutput->nTimeStamp = psPacket->nTimeStamp;
	}
	else
	{
		int nSize = psPacket->nSize[0];
		uint8 *pInput = psPacket->pBuffer[0];
		int bDecoded;
		AVFrame *psFrame = avcodec_alloc_frame();

		psOutput->nSize[0] = 0;

		while( nSize > 0 )
		{
			int nLen = avcodec_decode_video( m_sDecodeContext, psFrame, &bDecoded,
				pInput, nSize );

			if( bDecoded > 0 )
			{
				//std::cout<<"DECODED "<<psPacket->nTimeStamp<<std::endl;
				psOutput->nTimeStamp = psPacket->nTimeStamp;
				for( int i = 0; i < 4; i++ )
				{
					psOutput->pBuffer[i] = psFrame->data[i];
					psOutput->nSize[i] = psFrame->linesize[i];
					psOutput->pPrivate = (void*)psFrame;
				}
			}
			else
			{
				//std::cout<<"NOT DECODED "<<psPacket->nTimeStamp<<std::endl;
				//nLen = avcodec_decode_video( m_sDecodeContext, psFrame, &bDecoded, pInput, nSize );
			}

			if( nLen > nSize )
				nLen = nSize;

			if( nLen < 0 )
				break;

			nSize -= nLen;
			pInput += nLen;
		}
	}

	return ( 0 );
}


struct ffmpeg_frame
{
	uint32 nSize;
	ffmpeg_frame *pNext;
};


uint32 FFMpegCodec::Resample( uint16 *pDst, uint16 *pSrc, uint32 nLength )
{
	uint32 nSkipBefore = 0;
	uint32 nSkipBehind = 0;

	/* Calculate skipping ( I just guess what is right here ) */
	if( m_sExternalFormat.nChannels == 3 )
	{
		nSkipBefore = 0;
		nSkipBehind = 1;
	}
	else if( m_sExternalFormat.nChannels == 4 )
	{
		nSkipBefore = 2;
		nSkipBehind = 0;
	}
	else if( m_sExternalFormat.nChannels == 5 )
	{
		nSkipBefore = 2;
		nSkipBehind = 1;
	}
	else if( m_sExternalFormat.nChannels == 6 )
	{
		nSkipBefore = 2;
		nSkipBehind = 2;
	}


	//cout<<"Resample"<<endl;
	for( uint32 i = 0; i < nLength / m_sExternalFormat.nChannels / 2; i++ )
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
	return ( nLength * 2 / m_sExternalFormat.nChannels );
}

status_t FFMpegCodec::EncodePacket( os::MediaPacket_s * psPacket, os::MediaPacket_s * psOutput )
{
	psOutput->nTimeStamp = psPacket->nTimeStamp;

	if( m_sExternalFormat.nType == os::MEDIA_TYPE_AUDIO )
	{
		int nSize = psPacket->nSize[0];
		uint8 *pOut = psOutput->pBuffer[0];

		psOutput->nSize[0] = 0;
		AVCodecContext *sEnc = m_sEncodeStream.codec;

		//std::cout<<m_sEncodeStream.pts.num<<" "<<m_sEncodeStream.pts.den<<std::endl;

		if( sEnc->frame_size == 1 )
		{
			switch ( m_sEncodeStream.codec->codec_id )
			{
			case CODEC_ID_PCM_S32LE:
			case CODEC_ID_PCM_S32BE:
			case CODEC_ID_PCM_U32LE:
			case CODEC_ID_PCM_U32BE:
				nSize = nSize << 1;
				break;
			case CODEC_ID_PCM_S24LE:
			case CODEC_ID_PCM_S24BE:
			case CODEC_ID_PCM_U24LE:
			case CODEC_ID_PCM_U24BE:
			case CODEC_ID_PCM_S24DAUD:
				nSize = nSize / 2 * 3;
				break;
			case CODEC_ID_PCM_S16LE:
			case CODEC_ID_PCM_S16BE:
			case CODEC_ID_PCM_U16LE:
			case CODEC_ID_PCM_U16BE:
				break;
			default:
				nSize = nSize >> 1;
				break;
			}

			m_nCachePointer = 0;
			if( !m_bEncodeResample )
			{
				memcpy( m_pCache, psPacket->pBuffer[0], psPacket->nSize[0] );
				m_nCachePointer += nSize;
			}
			else
			{

				m_nCachePointer += Resample( ( uint16 * )m_pCache, ( uint16 * )psPacket->pBuffer[0], psPacket->nSize[0] );
				m_nCachePointer /= psPacket->nSize[0] / nSize;
			}


			psOutput->nSize[0] = avcodec_encode_audio( m_sEncodeStream.codec, pOut, m_nCachePointer, ( const short * )m_pCache );
			ffmpeg_frame *psFrame = ( ffmpeg_frame * ) malloc( sizeof( ffmpeg_frame ) );

			psFrame->nSize = psOutput->nSize[0];
			psFrame->pNext = NULL;
			psOutput->pPrivate = psFrame;
			return ( 0 );
		}

		/* Put packet into cache first */
		if( ( m_nCachePointer + psPacket->nSize[0] ) > FFMPEG_CACHE_SIZE )
		{
			std::cout << "Cache overflow!" << std::endl;
			return ( -1 );
		}

		if( !m_bEncodeResample )
		{
			memcpy( m_pCache + m_nCachePointer, psPacket->pBuffer[0], psPacket->nSize[0] );
			m_nCachePointer += psPacket->nSize[0];
		}
		else
		{
			m_nCachePointer += Resample( ( uint16 * )m_pCache + m_nCachePointer / 2, ( uint16 * )psPacket->pBuffer[0], psPacket->nSize[0] );
		}
		//cout<<"Cache :"<<->nSize[0]<<endl;

		ffmpeg_frame *psFrames = NULL;

	      start:
		uint32 nFrameBytes = ( uint32 )sEnc->frame_size * sEnc->channels * 2;

		/* Look if we have enough data for one frame */
		if( nFrameBytes < m_nCachePointer )
		{
			/* Note: We have to tell the ffmpeg output how we have split the packets */
			//cout<<"Encode :"<<nFrameBytes<<endl;
			int nLen = avcodec_encode_audio( m_sEncodeStream.codec, pOut, 4 * AVCODEC_MAX_AUDIO_FRAME_SIZE, ( const short * )m_pCache );

			psOutput->nSize[0] += nLen;
			memcpy( m_pCache, m_pCache + nFrameBytes, m_nCachePointer - nFrameBytes );
			m_nCachePointer -= nFrameBytes;
			pOut += nLen;
			ffmpeg_frame *psFrame = ( ffmpeg_frame * ) malloc( sizeof( ffmpeg_frame ) );

			psFrame->nSize = nLen;
			psFrame->pNext = psFrames;
			psFrames = psFrame;
			goto start;
		}
		psOutput->pPrivate = psFrames;
		return ( 0 );
	}
	else
	{
		/* Encode frame */
		AVFrame sFrame;

		avcodec_get_frame_defaults( &sFrame );
		for( int i = 0; i < 4; i++ )
		{
			sFrame.data[i] = psPacket->pBuffer[i];
			sFrame.linesize[i] = psPacket->nSize[i];
		}
		sFrame.quality = (int)m_sEncodeStream.quality;
		AVCodecContext *sEnc = m_sEncodeStream.codec;

		psOutput->nSize[0] = avcodec_encode_video( m_sEncodeStream.codec, psOutput->pBuffer[0], psPacket->nSize[0] * sEnc->height, &sFrame );

		if( psOutput->nSize[0] < 0 )
			return ( -1 );
		return ( 0 );
	}
}

os::MediaCodec * init_ffmpeg_codec()
{
	return ( new FFMpegCodec() );
}






