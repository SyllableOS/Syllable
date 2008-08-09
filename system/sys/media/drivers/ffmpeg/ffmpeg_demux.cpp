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
#include <vector>
extern "C" 
{
	#include <ffmpeg/avformat.h>
}

#define MAX_STREAM 8

class FFMpegDemuxer : public os::MediaInput
{
public:
	FFMpegDemuxer();
	~FFMpegDemuxer();
	os::String 		GetIdentifier();
	uint32			GetPhysicalType()
	{
		return( os::MEDIA_PHY_SOFT_DEMUX );
	}
	os::View*		GetConfigurationView();
	
	bool			FileNameRequired();
	status_t 		Open( os::String zFileName );
	void 			Close();
	void			Clear();
	
	bool			PacketBased();
	bool			StreamBased();
	
	uint32			GetTrackCount();
	uint32			SelectTrack( uint32 nTrack );
	

	os::String		GetTitle();
	os::String		GetAuthor();	
	os::String		GetAlbum();	
	os::String		GetComment();	
	
	status_t		ReadPacket( os::MediaPacket_s* psPacket );
	void			FreePacket( os::MediaPacket_s* psPacket );
	
	uint64			GetLength();
	uint64			GetCurrentPosition();
	uint64			Seek( uint64 nPosition );
	
	uint32			GetStreamCount();
	os::MediaFormat_s	GetStreamFormat( uint32 nIndex );
private:
	AVFormatContext *m_psContext;
	bool			m_bStream;
	uint64			m_nCurrentPosition;
	uint64			m_nLength;
	int				m_nBitRate;
};

FFMpegDemuxer::FFMpegDemuxer()
{
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
		if( av_find_stream_info( m_psContext ) < 0 )
		{
			av_close_input_file( m_psContext );
			return( -1 );
		}
		
		
		/* Calculate total bitrate */
		m_nBitRate = 0;
		
		
		for( uint i = 0; i < m_psContext->nb_streams; i++ ) 
		{
			m_nBitRate += m_psContext->streams[i]->codec->bit_rate;
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
		m_nLength = (int)( m_psContext->duration / AV_TIME_BASE );
		
		if( !m_bStream && m_psContext->start_time != (int)AV_NOPTS_VALUE )
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


void FFMpegDemuxer::Clear()
{
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

os::String FFMpegDemuxer::GetTitle()
{
	if( m_psContext == NULL )
		return( "" );
	return( m_psContext->title );
}

os::String FFMpegDemuxer::GetAuthor()
{
	if( m_psContext == NULL )
		return( "" );
	return( m_psContext->author );
}

os::String FFMpegDemuxer::GetAlbum()
{
	if( m_psContext == NULL )
		return( "" );
	return( m_psContext->album );
}

os::String FFMpegDemuxer::GetComment()
{
	if( m_psContext == NULL )
		return( "" );
	return( m_psContext->comment );
}

status_t FFMpegDemuxer::ReadPacket( os::MediaPacket_s* psPacket )
{
	AVPacket* psAVPacket = (AVPacket*)malloc( sizeof( AVPacket ) );

	if( av_read_frame( m_psContext, psAVPacket ) < 0 )
	{
		printf("Read packet error!\n" );
		free( psAVPacket );
		return( -1 );
	}

	psPacket->nStream = psAVPacket->stream_index;
	psPacket->pBuffer[0] = (uint8*)malloc( psAVPacket->size + FF_INPUT_BUFFER_PADDING_SIZE );
	memcpy( psPacket->pBuffer[0], psAVPacket->data, psAVPacket->size );
	memset( psPacket->pBuffer[0] + psAVPacket->size, 0, FF_INPUT_BUFFER_PADDING_SIZE );
	psPacket->nSize[0] = psAVPacket->size;
	psPacket->pPrivate = ( void* )psAVPacket;
	
	if( psAVPacket->dts == (int)AV_NOPTS_VALUE )
	{
		m_nCurrentPosition = m_psContext->pb->pos * 8 / m_nBitRate;
		psPacket->nTimeStamp = ~0;
	}
	else if( m_psContext->start_time == (int)AV_NOPTS_VALUE )
	{
		
		m_nCurrentPosition = ( av_rescale_q( psAVPacket->dts, m_psContext->streams[psAVPacket->stream_index]->time_base, AV_TIME_BASE_Q ) ) / AV_TIME_BASE;
		psPacket->nTimeStamp = ( av_rescale_q( psAVPacket->dts, m_psContext->streams[psAVPacket->stream_index]->time_base, AV_TIME_BASE_Q ) ) / 1000;
	}
	else
	{
		m_nCurrentPosition = ( av_rescale_q( psAVPacket->dts, m_psContext->streams[psAVPacket->stream_index]->time_base, AV_TIME_BASE_Q ) - m_psContext->start_time ) / AV_TIME_BASE;
		if( m_psContext->streams[psAVPacket->stream_index]->start_time > psAVPacket->dts )
			psPacket->nTimeStamp = 0;
		else
			psPacket->nTimeStamp =  ( av_rescale_q( psAVPacket->dts - m_psContext->streams[psAVPacket->stream_index]->start_time, m_psContext->streams[psAVPacket->stream_index]->time_base, AV_TIME_BASE_Q ) ) / 1000;
	}
	
	
	//std::cout<<"Stream "<<psAVPacket->stream_index<<" Start "<<m_psContext->streams[psAVPacket->stream_index]->start_time<<" DTS "<<psAVPacket->dts<<" PTS "<<psAVPacket->pts<<" STAMP "<<
	//	psPacket->nTimeStamp<<std::endl;
	//printf("Read packet %i\n", (int)m_nCurrentPosition );
		
	return( 0 );
}

void FFMpegDemuxer::FreePacket( os::MediaPacket_s* psPacket )
{
	free( psPacket->pBuffer[0] );
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
	if( m_psContext->start_time != (int)AV_NOPTS_VALUE )
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
	if( m_psContext->streams[nIndex]->codec->codec_type == CODEC_TYPE_VIDEO )
		sFormat.nType = os::MEDIA_TYPE_VIDEO;
	else if( m_psContext->streams[nIndex]->codec->codec_type == CODEC_TYPE_AUDIO )
		sFormat.nType = os::MEDIA_TYPE_AUDIO;
	else
		sFormat.nType = os::MEDIA_TYPE_OTHER;
		
	if( avcodec_find_decoder( m_psContext->streams[nIndex]->codec->codec_id ) == NULL )
	{
		if( m_psContext->streams[nIndex]->codec->codec_id == CODEC_ID_AAC ||
			m_psContext->streams[nIndex]->codec->codec_id == CODEC_ID_MPEG4AAC ) {
			/* Pass to aac plugin */
			sFormat.zName = "aac";
		} else {
			std::cout<<"Warning Unknown Codec :"<<m_psContext->streams[nIndex]->codec->codec_id<<std::endl;
			sFormat.zName = "Unknown";
			sFormat.nType = os::MEDIA_TYPE_OTHER;
		}
	} else {
		//std::cout<<"Codec ID "<<m_psContext->streams[nIndex]->codec->codec_id<<" for stream #"<<nIndex<<std::endl;
		sFormat.zName = avcodec_find_decoder( m_psContext->streams[nIndex]->codec->codec_id )->name;
	}

	sFormat.nBitRate = m_psContext->streams[nIndex]->codec->bit_rate;
	
	if( sFormat.nType == os::MEDIA_TYPE_AUDIO ) {
		sFormat.nSampleRate = m_psContext->streams[nIndex]->codec->sample_rate;
		sFormat.nChannels = m_psContext->streams[nIndex]->codec->channels;
	}
	
	if( sFormat.nType == os::MEDIA_TYPE_VIDEO ) {
		if( m_psContext->streams[nIndex]->r_frame_rate.den > 1 )
			sFormat.bVFR = true;
		else
			sFormat.bVFR = false;
		sFormat.vFrameRate = (float)m_psContext->streams[nIndex]->r_frame_rate.num / (float)m_psContext->streams[nIndex]->r_frame_rate.den;
		sFormat.nWidth = m_psContext->streams[nIndex]->codec->width;
		sFormat.nHeight = m_psContext->streams[nIndex]->codec->height;
		
		if( m_psContext->streams[nIndex]->codec->pix_fmt == PIX_FMT_YUV422 )
			sFormat.eColorSpace = os::CS_YUV422;
		else if( m_psContext->streams[nIndex]->codec->pix_fmt == PIX_FMT_YUV411P )
			sFormat.eColorSpace = os::CS_YUV411;
		else if( m_psContext->streams[nIndex]->codec->pix_fmt == PIX_FMT_YUV420P ||
				 m_psContext->streams[nIndex]->codec->pix_fmt == PIX_FMT_YUVJ420P )
			sFormat.eColorSpace = os::CS_YUV420;
		else if( m_psContext->streams[nIndex]->codec->pix_fmt == PIX_FMT_YUV422P ||
				 m_psContext->streams[nIndex]->codec->pix_fmt == PIX_FMT_YUVJ422P )
			sFormat.eColorSpace = os::CS_YUV422;
		else if( m_psContext->streams[nIndex]->codec->pix_fmt == PIX_FMT_RGB24 )
			sFormat.eColorSpace = os::CS_RGB24;
		else
			std::cout << "Unsupported pixel format " << m_psContext->streams[nIndex]->codec->pix_fmt << std::endl;

		//cout<<"Format "<<m_psContext->streams[nIndex]->codec.pix_fmt<<endl;
			
	}
	sFormat.pPrivate = m_psContext->streams[nIndex];
	return( sFormat );
}

os::MediaInput* init_ffmpeg_input()
{
	return( new FFMpegDemuxer() );
}

