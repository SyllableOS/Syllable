/*  FFMPEG Demuxer plugin
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
 
#include <media/input.h>
#include <posix/fcntl.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
extern "C" 
{
	#include "avformat.h"
}

class FFMpegDemuxer : public os::MediaInput
{
public:
	FFMpegDemuxer();
	~FFMpegDemuxer();
	os::String 		GetIdentifier();
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
	void			RegisterFormats();
	void			RegisterCodecs();

	AVFormatContext *m_psContext;
	bool			m_bStream;
	uint64			m_nCurrentPosition;
	uint64			m_nLength;
	int				m_nBitRate;
};

FFMpegDemuxer::FFMpegDemuxer()
{
	RegisterFormats();
	m_psContext = NULL;
}

FFMpegDemuxer::~FFMpegDemuxer()
{
}

os::View* FFMpegDemuxer::GetConfigurationView()
{
	return( NULL );
}


os::String FFMpegDemuxer::GetIdentifier()
{
	return( "FFMpeg Input" );
}

bool FFMpegDemuxer::FileNameRequired()
{
	return( true );
}

status_t FFMpegDemuxer::Open( os::String zFileName )
{
	if( av_open_input_file( &m_psContext, zFileName.c_str(), NULL,
		4096, NULL ) == 0 )
	{
		av_find_stream_info( m_psContext );
		
		
		/* Calculate total bitrate */
		m_nBitRate = 0;
		for( int i = 0; i < m_psContext->nb_streams; i++ ) 
		{
			m_nBitRate += m_psContext->streams[i]->codec.bit_rate;
		}
		
		/* Do not use url_fseek for getting the file length, it does not work ( but why ? ) */
	
		int hFile = open( zFileName.c_str(), O_RDONLY, 0666 );
		if( hFile < 0 ) 
		{
			m_bStream = true;
		} else {
			close( hFile );
			m_bStream = false;
		}
		m_nLength = (int)m_psContext->duration / AV_TIME_BASE;
		
		if( !m_bStream && m_psContext->start_time != AV_NOPTS_VALUE )
			av_seek_frame( m_psContext, -1, m_psContext->start_time, 0 );
		
		m_nCurrentPosition = 0;
		
		return( 0 );
	}
	return( -1 );
}

void FFMpegDemuxer::Close()
{
	if( m_psContext ) {
		av_close_input_file( m_psContext );
		m_psContext = NULL;
	}
}

bool FFMpegDemuxer::PacketBased()
{
	return( true );
}


bool FFMpegDemuxer::StreamBased()
{
	return( m_bStream );
}

uint32 FFMpegDemuxer::GetTrackCount()
{
	return( 1 );
}

uint32 FFMpegDemuxer::SelectTrack( uint32 nTrack )
{
	return( 0 );
}

status_t FFMpegDemuxer::ReadPacket( os::MediaPacket_s* psPacket )
{
	AVPacket sPacket;
	
	if( av_read_frame( m_psContext, &sPacket ) < 0 )
	{
		printf("Read packet error!\n" );
		return( -1 );
	}
	
	AVPacket* psSavePacket = ( AVPacket* )malloc( sizeof( AVPacket ) );
	*psSavePacket = sPacket;
	psPacket->nStream = sPacket.stream_index;
	psPacket->pBuffer[0] = sPacket.data;
	psPacket->nSize[0] = sPacket.size;
	psPacket->pPrivate = ( void* )psSavePacket;
	
	if( sPacket.dts == AV_NOPTS_VALUE )
	{
		m_nCurrentPosition = m_psContext->pb.pos * 8 / m_nBitRate;
		psPacket->nTimeStamp = ~0;
	}
	else if( m_psContext->start_time == AV_NOPTS_VALUE )
	{
		m_nCurrentPosition = ( sPacket.dts ) / AV_TIME_BASE;
		psPacket->nTimeStamp = ( sPacket.dts ) / 1000;
	}
	else
	{
		m_nCurrentPosition = ( sPacket.dts - m_psContext->start_time ) / AV_TIME_BASE;
		psPacket->nTimeStamp = ( sPacket.dts - m_psContext->streams[sPacket.stream_index]->start_time ) / 1000;
	}
	
	
	//std::cout<<"Stream "<<sPacket.stream_index<<" Start "<<m_psContext->streams[sPacket.stream_index]->start_time<<" DTS "<<sPacket.dts<<" PTS "<<sPacket.pts<<" STAMP "<<
		//psPacket->nTimeStamp<<std::endl;
	//printf("Read packet %i\n", (int)m_nCurrentPosition );
		
	return( 0 );
}

void FFMpegDemuxer::FreePacket( os::MediaPacket_s* psPacket )
{
	AVPacket* psSavePacket = ( AVPacket* )psPacket->pPrivate;
	av_free_packet( psSavePacket );
}

uint64 FFMpegDemuxer::GetLength()
{
	return( m_nLength );
}


uint64 FFMpegDemuxer::GetCurrentPosition()
{
	return( m_nCurrentPosition );
}

uint64 FFMpegDemuxer::Seek( uint64 nPosition )
{
	nPosition *= AV_TIME_BASE;
	if( m_psContext->start_time != AV_NOPTS_VALUE )
		nPosition += m_psContext->start_time;
	av_seek_frame( m_psContext, -1, nPosition, 0 );
	
//	m_nCurrentPosition = url_fseek( &m_psContext->pb, nPosition * m_nBitRate / 8, SEEK_SET ) * 8 / m_nBitRate;
	return( nPosition );
}

uint32 FFMpegDemuxer::GetStreamCount()
{
	return( m_psContext->nb_streams );
}

os::MediaFormat_s FFMpegDemuxer::GetStreamFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	
	/* Save Media Type */
	if( m_psContext->streams[nIndex]->codec.codec_type == CODEC_TYPE_VIDEO )
		sFormat.nType = os::MEDIA_TYPE_VIDEO;
	else if( m_psContext->streams[nIndex]->codec.codec_type == CODEC_TYPE_AUDIO )
		sFormat.nType = os::MEDIA_TYPE_AUDIO;
	else
		sFormat.nType = os::MEDIA_TYPE_OTHER;
		
	if( avcodec_find_decoder( m_psContext->streams[nIndex]->codec.codec_id ) == NULL )
	{
		std::cout<<"Warning Unknown Codec :"<<m_psContext->streams[nIndex]->codec.codec_id<<std::endl;
		sFormat.zName = "Unknown";
	} else {
		sFormat.zName = avcodec_find_decoder( m_psContext->streams[nIndex]->codec.codec_id )->name;
	}

	sFormat.nBitRate = m_psContext->streams[nIndex]->codec.bit_rate;
	
	if( sFormat.nType == os::MEDIA_TYPE_AUDIO ) {
		sFormat.nSampleRate = m_psContext->streams[nIndex]->codec.sample_rate;
		sFormat.nChannels = m_psContext->streams[nIndex]->codec.channels;
	}
	
	if( sFormat.nType == os::MEDIA_TYPE_VIDEO ) {
		if( m_psContext->streams[nIndex]->codec.frame_rate_base > 1 )
			sFormat.bVFR = true;
		else
			sFormat.bVFR = false;
		sFormat.vFrameRate = (float)m_psContext->streams[nIndex]->codec.frame_rate / (float)m_psContext->streams[nIndex]->codec.frame_rate_base;
		sFormat.nWidth = m_psContext->streams[nIndex]->codec.width;
		sFormat.nHeight = m_psContext->streams[nIndex]->codec.height;
		
		if( m_psContext->streams[nIndex]->codec.pix_fmt == PIX_FMT_YUV422 )
			sFormat.eColorSpace = os::CS_YUV422;
		else if( m_psContext->streams[nIndex]->codec.pix_fmt == PIX_FMT_YUV411P )
			sFormat.eColorSpace = os::CS_YUV411;
		else if( m_psContext->streams[nIndex]->codec.pix_fmt == PIX_FMT_YUV420P )
			sFormat.eColorSpace = os::CS_YUV420;
		else if( m_psContext->streams[nIndex]->codec.pix_fmt == PIX_FMT_YUV422P )
			sFormat.eColorSpace = os::CS_YUV422;
		else if( m_psContext->streams[nIndex]->codec.pix_fmt == PIX_FMT_RGB24 )
			sFormat.eColorSpace = os::CS_RGB24;
		//cout<<"Format "<<m_psContext->streams[nIndex]->codec.pix_fmt<<endl;
			
	}
	sFormat.pPrivate = &m_psContext->streams[nIndex]->codec;
	
	return( sFormat );
}

void FFMpegDemuxer::RegisterFormats()
{
	/* Our own custom init function which avoids initialising formats and codecs we don't want E.g. Ogg */
    avcodec_init();
    RegisterCodecs();

    mpegps_init();
    mpegts_init();

/* CONFIG_ENCODERS */
    crc_init();
    img_init();
    img2_init();

    raw_init();
    mp3_init();
    rm_init();
    asf_init();

/* CONFIG_ENCODERS */
    avienc_init();

    avidec_init();
    ff_wav_init();
    swf_init();
    au_init();
/* CONFIG_ENCODERS */
    gif_init();

    mov_init();
/* CONFIG_ENCODERS */
    movenc_init();
    jpeg_init();

    ff_dv_init();
    fourxm_init();
/* CONFIG_ENCODERS */
    flvenc_init();

    flvdec_init();
    str_init();
    roq_init();
    ipmovie_init();
    wc3_init();
    westwood_init();
    film_init();
    idcin_init();
    flic_init();
    vmd_init();

    yuv4mpeg_init();

    ffm_init();

    nut_init();
    matroska_init();
    sol_init();
    ea_init();
    nsvdec_init();

    /* file protocols */
    register_protocol(&file_protocol);
    register_protocol(&pipe_protocol);
}

void FFMpegDemuxer::RegisterCodecs()
{
    /* encoders */
/* CONFIG_ENCODERS */
    register_avcodec(&ac3_encoder);
    register_avcodec(&mp2_encoder);

    register_avcodec(&mpeg1video_encoder);
//    register_avcodec(&h264_encoder);
    register_avcodec(&mpeg2video_encoder);
    register_avcodec(&h261_encoder);
    register_avcodec(&h263_encoder);
    register_avcodec(&h263p_encoder);
    register_avcodec(&flv_encoder);
    register_avcodec(&rv10_encoder);
    register_avcodec(&rv20_encoder);
    register_avcodec(&mpeg4_encoder);
    register_avcodec(&msmpeg4v1_encoder);
    register_avcodec(&msmpeg4v2_encoder);
    register_avcodec(&msmpeg4v3_encoder);
    register_avcodec(&wmv1_encoder);
    register_avcodec(&wmv2_encoder);
    register_avcodec(&svq1_encoder);
    register_avcodec(&mjpeg_encoder);
    register_avcodec(&ljpeg_encoder);
    register_avcodec(&ppm_encoder);
    register_avcodec(&pgm_encoder);
    register_avcodec(&pgmyuv_encoder);
    register_avcodec(&pbm_encoder);
    register_avcodec(&pam_encoder);
    register_avcodec(&huffyuv_encoder);
    register_avcodec(&ffvhuff_encoder);
    register_avcodec(&asv1_encoder);
    register_avcodec(&asv2_encoder);
    register_avcodec(&ffv1_encoder);
    register_avcodec(&snow_encoder);
    register_avcodec(&zlib_encoder);
    register_avcodec(&dvvideo_encoder);
    register_avcodec(&sonic_encoder);
    register_avcodec(&sonic_ls_encoder);

    register_avcodec(&rawvideo_encoder);
    register_avcodec(&rawvideo_decoder);

    /* decoders */
	/* CONFIG_DECODERS */
    register_avcodec(&h263_decoder);
    register_avcodec(&h261_decoder);
    register_avcodec(&mpeg4_decoder);
    register_avcodec(&msmpeg4v1_decoder);
    register_avcodec(&msmpeg4v2_decoder);
    register_avcodec(&msmpeg4v3_decoder);
    register_avcodec(&wmv1_decoder);
    register_avcodec(&wmv2_decoder);
    register_avcodec(&vc9_decoder);
    register_avcodec(&wmv3_decoder);
    register_avcodec(&h263i_decoder);
    register_avcodec(&flv_decoder);
    register_avcodec(&rv10_decoder);
    register_avcodec(&rv20_decoder);
    register_avcodec(&svq1_decoder);
    register_avcodec(&svq3_decoder);
    register_avcodec(&wmav1_decoder);
    register_avcodec(&wmav2_decoder);
    register_avcodec(&indeo3_decoder);
    register_avcodec(&tscc_decoder);
    register_avcodec(&ulti_decoder);
    register_avcodec(&qdraw_decoder);
    register_avcodec(&xl_decoder);
    register_avcodec(&qpeg_decoder);
    register_avcodec(&loco_decoder);
    register_avcodec(&wnv1_decoder);
    register_avcodec(&aasc_decoder);
    register_avcodec(&mpeg1video_decoder);
    register_avcodec(&mpeg2video_decoder);
    register_avcodec(&mpegvideo_decoder);
    register_avcodec(&dvvideo_decoder);
    register_avcodec(&mjpeg_decoder);
    register_avcodec(&mjpegb_decoder);
    register_avcodec(&sp5x_decoder);
    register_avcodec(&mp2_decoder);
    register_avcodec(&mp3_decoder);
    register_avcodec(&mp3adu_decoder);
    register_avcodec(&mp3on4_decoder);
    register_avcodec(&mace3_decoder);
    register_avcodec(&mace6_decoder);
    register_avcodec(&huffyuv_decoder);
    register_avcodec(&ffvhuff_decoder);
    register_avcodec(&ffv1_decoder);
    register_avcodec(&snow_decoder);
    register_avcodec(&cyuv_decoder);
    register_avcodec(&h264_decoder);
    register_avcodec(&vp3_decoder);
    register_avcodec(&theora_decoder);
    register_avcodec(&asv1_decoder);
    register_avcodec(&asv2_decoder);
    register_avcodec(&vcr1_decoder);
    register_avcodec(&cljr_decoder);
    register_avcodec(&fourxm_decoder);
    register_avcodec(&mdec_decoder);
    register_avcodec(&roq_decoder);
    register_avcodec(&interplay_video_decoder);
    register_avcodec(&xan_wc3_decoder);
    register_avcodec(&rpza_decoder);
    register_avcodec(&cinepak_decoder);
    register_avcodec(&msrle_decoder);
    register_avcodec(&msvideo1_decoder);
    register_avcodec(&vqa_decoder);
    register_avcodec(&idcin_decoder);
    register_avcodec(&eightbps_decoder);
    register_avcodec(&smc_decoder);
    register_avcodec(&flic_decoder);
    register_avcodec(&truemotion1_decoder);
    register_avcodec(&vmdvideo_decoder);
    register_avcodec(&vmdaudio_decoder);
    register_avcodec(&mszh_decoder);
    register_avcodec(&zlib_decoder);
    register_avcodec(&sonic_decoder);
    register_avcodec(&ra_144_decoder);
    register_avcodec(&ra_288_decoder);
    register_avcodec(&roq_dpcm_decoder);
    register_avcodec(&interplay_dpcm_decoder);
    register_avcodec(&xan_dpcm_decoder);
    register_avcodec(&sol_dpcm_decoder);
    register_avcodec(&qtrle_decoder);
    register_avcodec(&flac_decoder);
    register_avcodec(&shorten_decoder);
    register_avcodec(&alac_decoder);
    register_avcodec(&ws_snd1_decoder);

    /* pcm codecs */

#define PCM_CODEC(id, name) \
    register_avcodec(& name ## _encoder); \
    register_avcodec(& name ## _decoder); \

PCM_CODEC(CODEC_ID_PCM_S16LE, pcm_s16le);
PCM_CODEC(CODEC_ID_PCM_S16BE, pcm_s16be);
PCM_CODEC(CODEC_ID_PCM_U16LE, pcm_u16le);
PCM_CODEC(CODEC_ID_PCM_U16BE, pcm_u16be);
PCM_CODEC(CODEC_ID_PCM_S8, pcm_s8);
PCM_CODEC(CODEC_ID_PCM_U8, pcm_u8);
PCM_CODEC(CODEC_ID_PCM_ALAW, pcm_alaw);
PCM_CODEC(CODEC_ID_PCM_MULAW, pcm_mulaw);

    /* adpcm codecs */
PCM_CODEC(CODEC_ID_ADPCM_IMA_QT, adpcm_ima_qt);
PCM_CODEC(CODEC_ID_ADPCM_IMA_WAV, adpcm_ima_wav);
PCM_CODEC(CODEC_ID_ADPCM_IMA_DK3, adpcm_ima_dk3);
PCM_CODEC(CODEC_ID_ADPCM_IMA_DK4, adpcm_ima_dk4);
PCM_CODEC(CODEC_ID_ADPCM_IMA_WS, adpcm_ima_ws);
PCM_CODEC(CODEC_ID_ADPCM_IMA_SMJPEG, adpcm_ima_smjpeg);
PCM_CODEC(CODEC_ID_ADPCM_MS, adpcm_ms);
PCM_CODEC(CODEC_ID_ADPCM_4XM, adpcm_4xm);
PCM_CODEC(CODEC_ID_ADPCM_XA, adpcm_xa);
PCM_CODEC(CODEC_ID_ADPCM_ADX, adpcm_adx);
PCM_CODEC(CODEC_ID_ADPCM_EA, adpcm_ea);
PCM_CODEC(CODEC_ID_ADPCM_G726, adpcm_g726);
PCM_CODEC(CODEC_ID_ADPCM_CT, adpcm_ct);
PCM_CODEC(CODEC_ID_ADPCM_SWF, adpcm_swf);

#undef PCM_CODEC

    /* parsers */ 
    av_register_codec_parser(&mpegvideo_parser);
    av_register_codec_parser(&mpeg4video_parser);
    av_register_codec_parser(&h261_parser);
    av_register_codec_parser(&h263_parser);
    av_register_codec_parser(&h264_parser);
    av_register_codec_parser(&mjpeg_parser);
    av_register_codec_parser(&pnm_parser);

    av_register_codec_parser(&mpegaudio_parser);
}

os::MediaInput* init_ffmpeg_input()
{
	return( new FFMpegDemuxer() );
}

