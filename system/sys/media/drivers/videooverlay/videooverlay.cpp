/*  Video Overlay output plugin
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
#include <gui/videooverlay.h>
#include <gui/point.h>
#include <atheos/threads.h>
#include <atheos/semaphore.h>
#include <atheos/kernel.h>
#include <atheos/time.h>
#include <util/messenger.h>
#include <util/message.h>
#include <util/application.h>
#include <appserver/protocol.h>
#include <inttypes.h>
extern "C" 
{
	#include "mplayercomp.h"
	#include "rgb2rgb.h"
}

#define VIDEO_FRAME_NUMBER 20


CpuCaps gCpuCaps;

class VideoOverlayOutput : public os::MediaOutput
{
public:
	VideoOverlayOutput();
	~VideoOverlayOutput();
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
	
	os::View*		GetVideoView( uint32 nIndex );
	void			UpdateView( uint32 nStream );
	
	status_t		WritePacket( uint32 nIndex, os::MediaPacket_s *psFrame );
	uint64			GetDelay();
	uint32			GetUsedBufferPercentage();
private:
	bool			CheckVideoOverlay( os::color_space eSpace );
	os::color_space m_eColorSpace;
	os::VideoOverlayView* m_pcView;
	os::MediaFormat_s	m_sFormat;
	bool			m_bNoCache;
	int				m_nQueuedFrames;
	os::MediaPacket_s* m_psFrame[VIDEO_FRAME_NUMBER];
	sem_id			m_hLock;
	uint32			m_nCurrentFrame;
	bigtime_t		m_nFirstTime;
};


VideoOverlayOutput::VideoOverlayOutput()
{
	/* Set default values */
	m_pcView = NULL;
	m_nQueuedFrames = 0;
	m_nFirstTime = 0;
	m_nCurrentFrame = 0;
	m_hLock = create_semaphore( "overlay_lock", 1, 0 );
}

VideoOverlayOutput::~VideoOverlayOutput()
{
	Close();
	delete_semaphore( m_hLock );
}

os::String VideoOverlayOutput::GetIdentifier()
{
	return( "Video Overlay Output" );
}


os::View* VideoOverlayOutput::GetConfigurationView()
{
	return( NULL );
}

os::View* VideoOverlayOutput::GetVideoView( uint32 nIndex )
{
	if( nIndex == 0 )
		return( m_pcView );
	return( NULL );	
}

void VideoOverlayOutput::UpdateView( uint32 nIndex )
{
	if( m_pcView ) {
		lock_semaphore( m_hLock );
		m_pcView->Update();
		unlock_semaphore( m_hLock );
	}
}


void VideoOverlayOutput::Flush()
{
	/* Always play the next packet in the queue */
	os::MediaPacket_s *psFrame;
	if( m_nFirstTime == 0 )
		m_nFirstTime = get_system_time();
	/* Calculate time for the next frame */
	bigtime_t nNextTime = m_nFirstTime + ( bigtime_t )( (float)m_nCurrentFrame  / m_sFormat.vFrameRate * 1000000 );

	if( ( nNextTime < get_system_time() ) || m_bNoCache )
	{	
	
		lock_semaphore( m_hLock );
		
		if( ( m_nQueuedFrames > 0 && m_pcView->GetRaster() != NULL ) ) 
		{
			psFrame = m_psFrame[0];
			bigtime_t nNextNextTime = m_nFirstTime + ( bigtime_t )( (float)( m_nCurrentFrame + 1 ) / m_sFormat.vFrameRate * 1000000 );
			if( ( nNextNextTime >= nNextTime ) || m_bNoCache ) {
			
			/* Draw frame */
			if( m_eColorSpace == os::CS_YUV12 )
			{
				/* YV12 Overlay */
				for( int i = 0; i < m_sFormat.nHeight; i++ )
				{
					memcpy( m_pcView->GetRaster() + i * m_sFormat.nWidth, psFrame->pBuffer[0] + psFrame->nSize[0] * i, psFrame->nSize[0] );
				}
				for( int i = 0; i < m_sFormat.nHeight / 2; i++ )
				{
					memcpy( m_pcView->GetRaster() +  m_sFormat.nHeight * m_sFormat.nWidth * 5 / 4 + m_sFormat.nWidth * i / 2, 
						psFrame->pBuffer[1] + psFrame->nSize[1] * i, psFrame->nSize[1] );
				}
				for( int i = 0; i < m_sFormat.nHeight / 2; i++ )
				{
					memcpy( m_pcView->GetRaster() +  m_sFormat.nHeight * m_sFormat.nWidth + m_sFormat.nWidth * i / 2, 
							psFrame->pBuffer[2] + psFrame->nSize[2] * i, psFrame->nSize[2] );
				}
			} 
			else if( m_eColorSpace == os::CS_YUV422 )
			{
				/* UYVY Overlay */
				for( int i = 0; i < m_sFormat.nHeight; i++ )
				{
					memcpy( m_pcView->GetRaster() + i * m_sFormat.nWidth * 2, psFrame->pBuffer[0] + psFrame->nSize[0] * i, psFrame->nSize[0] );
				}
			} else {
				/* RGB32 */
				for( int i = 0; i < m_sFormat.nHeight; i++ )
				{
					memcpy( m_pcView->GetRaster() + i * m_sFormat.nWidth * 4, psFrame->pBuffer[0] + psFrame->nSize[0] * i, psFrame->nSize[0] );
				}
			}
			
			m_pcView->Update();
			} else {
				cout<<"Framedrop"<<endl;
			}
			
			/* Move frames up */
			free( psFrame->pBuffer[0] );
			free( psFrame->pBuffer[1] );
			free( psFrame->pBuffer[2] );
			free( psFrame );
			m_nQueuedFrames--;
			for( int i = 0; i < m_nQueuedFrames; i++ )
			{
				m_psFrame[i] = m_psFrame[i+1];
			}
			m_nCurrentFrame++;
			//cout<<"Video:"<<m_nQueuedFrames<<" left"<<endl;
		} 
		
		unlock_semaphore( m_hLock );
	}
}


bool VideoOverlayOutput::FileNameRequired()
{
	return( false );
}

bool VideoOverlayOutput::CheckVideoOverlay( os::color_space eSpace )
{
	/* Check one type of overlay */
	os::Application *pcApp = os::Application::GetInstance();
	os::Message cReq( os::WR_CREATE_VIDEO_OVERLAY );
	os::Message cReply;
	cReq.AddIPoint( "size", os::IPoint( 1, 1 ) );
	cReq.AddRect( "dst", os::IRect( 0, 0, 1, 1 ) );
	cReq.AddInt32( "format", (int32)eSpace );
	cReq.AddColor32( "color_key", os::Color32_s( 0, 0, 0 ) );
	
	os::Messenger( pcApp->GetAppPort() ).SendMessage( &cReq, &cReply );
    
    int nError;
    area_id hArea;
	cReply.FindInt( "error", &nError );
	if( nError != 0 ) {
		return( false );
	}
	cReply.FindInt32( "area", (int32*)&hArea );
	
	os::Message cDelete( os::WR_DELETE_VIDEO_OVERLAY );
	cDelete.AddInt32( "area", hArea );
	os::Messenger( pcApp->GetAppPort() ).SendMessage( &cDelete );
	return( true );
}

status_t VideoOverlayOutput::Open( os::String zFileName )
{
	/* Clear frame buffer */
	Close();
	m_nQueuedFrames = 0;
	for( int i = 0; i < VIDEO_FRAME_NUMBER; i++ ) {
		m_psFrame[i] = NULL;
	}
	if( zFileName == "-nocache" ) {
		m_bNoCache = true;
		cout<<"Video caching disabled"<<endl;
	}
	else 
		m_bNoCache = false;
		
	/* Intialize cpu structure for mplayer code */
	memset( &gCpuCaps, 0, sizeof( CpuCaps ) );
	gCpuCaps.hasMMX = 0;
	gCpuCaps.hasMMX2 = 1;
	gCpuCaps.isX86 = 1;
		
	/* Check video overlay */
	m_eColorSpace = os::CS_YUV12;
	if( !CheckVideoOverlay( m_eColorSpace ) ) {
		m_eColorSpace = os::CS_YUV422;
		if( !CheckVideoOverlay( m_eColorSpace ) ) {
			m_eColorSpace = os::CS_RGB32;
			if( !CheckVideoOverlay( m_eColorSpace ) ) {
				cout<<"Video Overlays are not supported"<<endl;
				return( -1 );
			} else {
				cout<<"Using RGB32 Overlay"<<endl;
				yuv2rgb_init( 32, MODE_RGB );
			}
		} else {
			cout<<"Using UYVY Overlay"<<endl;
		}
	} else {
		cout<<"Using YV12 Overlay"<<endl;
	}
	return( 0 );
}

void VideoOverlayOutput::Close()
{
	
	Clear();
	/*if( m_pcView != NULL )
		delete( m_pcView );*/
	//cout<<"Video closed"<<endl;
}


void VideoOverlayOutput::Clear()
{
	m_nFirstTime = 0;
	m_nCurrentFrame = 0;
	/* Delete all pending frames */
	for( int i = 0; i < m_nQueuedFrames; i++ ) {
		free( m_psFrame[i]->pBuffer[0] );
		free( m_psFrame[i]->pBuffer[1] );
		free( m_psFrame[i]->pBuffer[2] );
		free( m_psFrame[i] );
	}
	m_nQueuedFrames = 0;
}

uint32 VideoOverlayOutput::GetOutputFormatCount()
{
	return( 1 );
}

os::MediaFormat_s VideoOverlayOutput::GetOutputFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	memset( &sFormat, 0, sizeof( os::MediaFormat_s ) );
	sFormat.zName = "Raw Video";
	sFormat.nType = os::MEDIA_TYPE_VIDEO;
	
	return( sFormat );
}

uint32 VideoOverlayOutput::GetSupportedStreamCount()
{
	return( 1 );
}

status_t VideoOverlayOutput::AddStream( os::String zName, os::MediaFormat_s sFormat )
{
	if( sFormat.zName == "Raw Video" && sFormat.nType == os::MEDIA_TYPE_VIDEO 
		&& sFormat.eColorSpace == os::CS_YUV12 ) {}
	else
		return( -1 );
	
	/* Save format */
	m_sFormat = sFormat;
	
	/* Open Overlay */
	os::Rect cFrame( 0, 0, sFormat.nWidth, sFormat.nHeight );
	m_pcView = new os::VideoOverlayView( cFrame, "media_video", os::CF_FOLLOW_ALL, 
			os::IPoint(	sFormat.nWidth, sFormat.nHeight ), m_eColorSpace, os::Color32_s( 0, 0, 255 ) );
	
	if( m_pcView == NULL )
		return( -1 );
	return( 0 );
}

status_t VideoOverlayOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psFrame )
{
	/* Create a new media frame and queue it */
	os::MediaPacket_s* psQueueFrame = ( os::MediaPacket_s* )malloc( sizeof( os::MediaPacket_s ) );
	if( psQueueFrame == NULL )
		return( -1 );
	memset( psQueueFrame, 0, sizeof( os::MediaPacket_s ) );
	if( m_eColorSpace == os::CS_YUV12 )
	{
		/* YV12 */
		psQueueFrame->nSize[0] = psFrame->nSize[0];
		psQueueFrame->nSize[1] = psFrame->nSize[1];
		psQueueFrame->nSize[2] = psFrame->nSize[2];
		psQueueFrame->pBuffer[0] = ( uint8* )malloc( psQueueFrame->nSize[0] * m_sFormat.nHeight );
		psQueueFrame->pBuffer[1] = ( uint8* )malloc( psQueueFrame->nSize[1] * m_sFormat.nHeight / 2 );
		psQueueFrame->pBuffer[2] = ( uint8* )malloc( psQueueFrame->nSize[2] * m_sFormat.nHeight / 2 );
	
		memcpy( psQueueFrame->pBuffer[0], psFrame->pBuffer[0], psQueueFrame->nSize[0] * m_sFormat.nHeight );
		memcpy( psQueueFrame->pBuffer[1], psFrame->pBuffer[1], psQueueFrame->nSize[1] * m_sFormat.nHeight / 2 );
		memcpy( psQueueFrame->pBuffer[2], psFrame->pBuffer[2], psQueueFrame->nSize[2] * m_sFormat.nHeight / 2 );
	} 
	else if( m_eColorSpace == os::CS_YUV422 )
	{
		/* Convert to UYVY */
		psQueueFrame->nSize[0] = m_sFormat.nWidth * 2;
		psQueueFrame->pBuffer[0] = ( uint8* )malloc( psQueueFrame->nSize[0] * m_sFormat.nHeight );
		yv12touyvy( psFrame->pBuffer[0], psFrame->pBuffer[1], psFrame->pBuffer[2], psQueueFrame->pBuffer[0],
					m_sFormat.nWidth, m_sFormat.nHeight, psFrame->nSize[0], psFrame->nSize[1],
					psQueueFrame->nSize[0] );
	} 
	else 
	{
		/* Convert to RGB32 */
		psQueueFrame->nSize[0] = m_sFormat.nWidth * 4;
		psQueueFrame->pBuffer[0] = ( uint8* )malloc( psQueueFrame->nSize[0] * m_sFormat.nHeight );
		yuv2rgb( psQueueFrame->pBuffer[0], psFrame->pBuffer[0], psFrame->pBuffer[1], psFrame->pBuffer[2],
			m_sFormat.nWidth, m_sFormat.nHeight, psQueueFrame->nSize[0], psFrame->nSize[0],
			psFrame->nSize[1] );
		
	}
	
	lock_semaphore( m_hLock );
	/* Queue packet */
	if( m_nQueuedFrames > VIDEO_FRAME_NUMBER - 1 ) {
		unlock_semaphore( m_hLock );
		if( m_eColorSpace == os::CS_YUV12 ) {
			/* YV12 */
			free( psQueueFrame->pBuffer[0] );
			free( psQueueFrame->pBuffer[1] );
			free( psQueueFrame->pBuffer[2] );
		} else {
			/* UYVY / RGB32 */
			free( psQueueFrame->pBuffer[0] );
		}
		free( psQueueFrame );
		cout<<"Frame buffer full"<<endl;
		return( -1 );
	}
	m_psFrame[m_nQueuedFrames] = psQueueFrame;
	m_nQueuedFrames++;
	unlock_semaphore( m_hLock );
	//cout<<"Frame queued at position "<<m_nQueuedFrames - 1<<"( "<<m_nQueuedFrames * 100 / VIDEO_FRAME_NUMBER<<
	//"% of the buffer used )"<<endl;
	return( 0 );	
}

uint64 VideoOverlayOutput::GetDelay()
{
	return( 1000 * m_nQueuedFrames / (int)m_sFormat.vFrameRate );
}


uint32 VideoOverlayOutput::GetUsedBufferPercentage()
{
	uint32 nValue;
	lock_semaphore( m_hLock );
	nValue = m_nQueuedFrames * 100 / VIDEO_FRAME_NUMBER;
	unlock_semaphore( m_hLock );
	return( nValue );
}


extern "C"
{
	os::MediaOutput* init_media_output()
	{
		return( new VideoOverlayOutput() );
	}

}



































































































