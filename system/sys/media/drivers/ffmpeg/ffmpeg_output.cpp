/*  FFMPEG output plugin
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
#include <fcntl.h>
#include <unistd.h>
extern "C" 
{
	#include "avformat.h"
}

#define FFMPEG_MAX_STREAMS 4

class FFMpegOutput : public os::MediaOutput
{
public:
	FFMpegOutput();
	~FFMpegOutput();
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
	void			StartEncoding();
	bool			m_bStarted;
	int				m_nVideoStream;
	int				m_nAudioStream;
	int				m_nCurrentStream;
	AVFormatContext m_sContext;
	AVStream*		m_sStream[FFMPEG_MAX_STREAMS];
};


FFMpegOutput::FFMpegOutput()
{
	m_nVideoStream = m_nAudioStream = -1;
}

FFMpegOutput::~FFMpegOutput()
{
}

os::String FFMpegOutput::GetIdentifier()
{
	return( "FFMpeg Output" );
}


os::View* FFMpegOutput::GetConfigurationView()
{
	return( NULL );
}

void FFMpegOutput::Flush()
{
}

bool FFMpegOutput::FileNameRequired()
{
	return( true );
}

status_t FFMpegOutput::Open( os::String zFileName )
{	
	/* Look if we can guess the filetype */
	AVOutputFormat* psOutputFormat;
	psOutputFormat = guess_format( NULL, zFileName.c_str(), NULL );
	if( !psOutputFormat ) {
		cout<<"Could not guess output format!"<<endl;
		return( -1 );
	}
	
	if( psOutputFormat->video_codec == CODEC_ID_NONE || psOutputFormat->audio_codec == CODEC_ID_NONE ) {
		cout<<"Format does not support video and audio!"<<endl;
		return( -1 );
	}
	
	m_sContext.oformat = psOutputFormat;
	
	/* Open file */
	if( url_fopen( &m_sContext.pb, zFileName.c_str(), URL_WRONLY ) < 0 )
		return( -1 );
		
	
	m_nCurrentStream = 0;
	m_bStarted = false;
	
	
	return( 0 );
}

void FFMpegOutput::Close()
{
	if( m_bStarted ) {
		/* Write trailer */
		av_write_trailer( &m_sContext );
		cout<<"Trailer written"<<endl;
	}
	url_fclose( &m_sContext.pb );
}

void FFMpegOutput::StartEncoding()
{
	/* Prepare everything for encoding */
	if( m_bStarted )
		return;
	
	
	/* Set streams */
	m_sContext.nb_streams = m_nCurrentStream;
	for( int i = 0; i < m_nCurrentStream; i++ ) {
		m_sContext.streams[i] = m_sStream[i];
	}
	cout<<m_nCurrentStream<<" Streams added"<<endl;
	
	/* Set parameters */
	AVFormatParameters sParams;
	memset( &sParams, 0, sizeof( sParams ) );
	sParams.image_format = NULL;
	if( av_set_parameters( &m_sContext, &sParams ) < 0 ) {
		cout<<"Could not set parameters"<<endl;
		return;
	}
	
	/* Writing header */
	if( av_write_header( &m_sContext ) < 0 ) {
		cout<<"Could not write file header"<<endl;
		return;
	} 
	cout<<"File header written"<<endl;
	
	m_bStarted = true;
}


void FFMpegOutput::Clear()
{
}

uint32 FFMpegOutput::GetOutputFormatCount()
{
	return( 8 );
}

os::MediaFormat_s FFMpegOutput::GetOutputFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	memset( &sFormat, 0, sizeof( os::MediaFormat_s ) );
	
	switch( nIndex )
	{
		/* No all ffmpeg formats are supported when encoding */
		case 0:
			sFormat.zName = "MPEG1 Video";
			sFormat.nType = os::MEDIA_TYPE_VIDEO;
		break;
		case 1:
			sFormat.zName = "MS MPEG4V2 Video";
			sFormat.nType = os::MEDIA_TYPE_VIDEO;
		break;
		case 2:
			sFormat.zName = "MS MPEG4V3 Video";
			sFormat.nType = os::MEDIA_TYPE_VIDEO;
		break;
		case 3:
			sFormat.zName = "MP2 Audio";
			sFormat.nType = os::MEDIA_TYPE_AUDIO;
		break;
		case 4:
			sFormat.zName = "PCM_S16LE Audio";
			sFormat.nType = os::MEDIA_TYPE_AUDIO;
		break;
		case 5:
			sFormat.zName = "PCM_U8 Audio";
			sFormat.nType = os::MEDIA_TYPE_AUDIO;
		break;
		case 6:
			sFormat.zName = "MJPEG Video";
			sFormat.nType = os::MEDIA_TYPE_VIDEO;
		break;
		break;
		
	}
	return( sFormat );
}

uint32 FFMpegOutput::GetSupportedStreamCount()
{
	return( 2 );
}

status_t FFMpegOutput::AddStream( os::String zName, os::MediaFormat_s sFormat )
{
	/* Add stream */
	if( sFormat.pPrivate == NULL ) {
		cout<<"The FFMpeg output does only work with the FFMpeg codec!"<<endl;
		return( -1 );
		
	}
	m_sStream[m_nCurrentStream] = ( ( AVStream * ) sFormat.pPrivate );
	m_nCurrentStream++;
	
	return( 0 );
}


struct ffmpeg_frame
{
	uint32 nSize;
	ffmpeg_frame* pNext;
};

status_t FFMpegOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket )
{
	StartEncoding();
	
	if( m_sStream[nIndex]->codec.codec_type == CODEC_TYPE_AUDIO )
	{
		if( psPacket->pPrivate == NULL )
			return( -1 );
		
		/* Split the packet with the information given by the ffmpeg codec */
		ffmpeg_frame* psFrames = ( ffmpeg_frame* )psPacket->pPrivate;
		uint8* pBuffer = psPacket->pBuffer[0];
	
		while( psFrames != NULL )
		{
		//cout<<"Write :"<<psFrames->nSize<<endl;
			av_write_frame( &m_sContext, nIndex, pBuffer, psFrames->nSize );
			pBuffer += psFrames->nSize;
			ffmpeg_frame* psOldFrame = psFrames;
			psFrames = psFrames->pNext;
			free( psOldFrame );
		}
	}
	else 
	{
		av_write_frame( &m_sContext, nIndex, psPacket->pBuffer[0], psPacket->nSize[0] );
	}
	
	
		
	return( 0 );
}

uint64 FFMpegOutput::GetDelay()
{
	return( 0 );
}

uint32 FFMpegOutput::GetUsedBufferPercentage()
{
	return( 0 );
}

extern "C"
{
	os::MediaOutput* init_media_output()
	{
		return( new FFMpegOutput() );
	}

}





















































































