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
	AVFormatContext *m_psContext;
	bool			m_bStream;
	uint64			m_nCurrentPosition;
	uint64			m_nLength;
	int				m_nBitRate;
};

FFMpegDemuxer::FFMpegDemuxer()
{
	av_register_all();
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
			m_nLength = lseek( hFile, 0, SEEK_END ) * 8 / m_nBitRate;	
			close( hFile );
			m_bStream = false;
		}
		
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
	
	if( av_read_packet( m_psContext, &sPacket ) < 0 )
		return( -1 );
	
	AVPacket* psSavePacket = ( AVPacket* )malloc( sizeof( AVPacket ) );
	*psSavePacket = sPacket;
	psPacket->nStream = sPacket.stream_index;
	psPacket->pBuffer[0] = sPacket.data;
	psPacket->nSize[0] = sPacket.size;
	psPacket->pPrivate = ( void* )psSavePacket;
	
	m_nCurrentPosition = m_psContext->pb.pos * 8 / m_nBitRate;
		
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
	m_nCurrentPosition = url_fseek( &m_psContext->pb, nPosition * m_nBitRate / 8, SEEK_SET ) * 8 / m_nBitRate;
	return( m_nCurrentPosition );
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
		
	/* Save codec identifier ( really incomplete! ) */
	switch( m_psContext->streams[nIndex]->codec.codec_id )
	{
		case CODEC_ID_MPEG1VIDEO:
			sFormat.zName = "MPEG1 Video";
		break;
		case CODEC_ID_H263:
			sFormat.zName = "H263 Video";
		break;
		case CODEC_ID_RV10:
			sFormat.zName = "RV10 Video";
		break;
		case CODEC_ID_MP2:
			sFormat.zName = "MP2 Audio";
		break;
		case CODEC_ID_MP3LAME:
			sFormat.zName = "MP3 Audio";
		break;
		case CODEC_ID_MJPEG:
			sFormat.zName = "MJPEG Video";
		break;
		case CODEC_ID_MPEG4:
			sFormat.zName = "MPEG4 Audio";
		break;
		case CODEC_ID_MSMPEG4V1:
			sFormat.zName = "MS MPEG4V1 Video";
		break;
		case CODEC_ID_MSMPEG4V2:
			sFormat.zName = "MS MPEG4V2 Video";
		break;
		case CODEC_ID_MSMPEG4V3:
			sFormat.zName = "MS MPEG4V3 Video";
		break;
		case CODEC_ID_WMV1:
			sFormat.zName = "WMV1 Video";
		break;
		case CODEC_ID_WMV2:
			sFormat.zName = "WMV2 Video";
		break;
		case CODEC_ID_PCM_S16LE:
			sFormat.zName = "PCM_S16LE Audio";
		break;
		case CODEC_ID_AC3:
			sFormat.zName = "AC3 Audio";
		break;
		case CODEC_ID_WMAV1:
			sFormat.zName = "WMAV1 Audio";
		break;
		case CODEC_ID_WMAV2:
			sFormat.zName = "WMAV2 Audio";
		break;
		case CODEC_ID_DVVIDEO:
			sFormat.zName = "DV Video";
		break;
		case CODEC_ID_DVAUDIO:
			sFormat.zName = "DV Audio";
		break;
		case CODEC_ID_SVQ1:
			sFormat.zName = "SVQ1 Video";
		break;
		case CODEC_ID_SVQ3:
			sFormat.zName = "SVQ3 Video";
		break;
		case CODEC_ID_VP3:
			sFormat.zName = "VP3 Video";
		break;
		case CODEC_ID_H264:
			sFormat.zName = "H264 Video";
		break;
		case CODEC_ID_INDEO3:
			sFormat.zName = "INDEO3 Video";
		break;
		case CODEC_ID_PCM_U8:
			sFormat.zName = "PCM_U8 Audio";
		break;
		case CODEC_ID_ADPCM_IMA_QT:
			sFormat.zName = "QT Audio";
		break;
		case CODEC_ID_ADPCM_IMA_WAV:
			sFormat.zName = "WAV Audio";
		break;
		case CODEC_ID_ADPCM_MS:
			sFormat.zName = "MS PCM Audio";
		break;
		default:
			sFormat.zName = "Unknown";
			cout<<"Warning Unknown Codec ( please add to ffmpeg_demux.c ) :"<<m_psContext->streams[nIndex]->codec.codec_id<<endl;
			
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

extern "C"
{
	os::MediaInput* init_media_input()
	{
		return( new FFMpegDemuxer() );
	}

}












































































