/*  Video Bitmap output plugin
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
#include <media/addon.h>
#include <gui/view.h>
#include <gui/bitmap.h>
#include <atheos/threads.h>
#include <atheos/semaphore.h>
#include <atheos/kernel.h>
#include <atheos/time.h>
#include <inttypes.h>
#include <iostream>
extern "C" 
{
	#include "mplayercomp.h"
	#include "rgb2rgb.h"
}
#define VIDEO_FRAME_NUMBER 20

class VideoBitmapView : public os::View
{
public:
	VideoBitmapView( const os::Rect& cFrame, const std::string&cTitle ) 
				: os::View( cFrame, cTitle, os::CF_FOLLOW_ALL )
	{
		m_pcBitmap = new os::Bitmap( (int)cFrame.Width() + 1, (int)cFrame.Height() + 1, os::CS_RGB16 );
	}
	~VideoBitmapView()
	{
		if( m_pcBitmap != NULL )
			delete( m_pcBitmap );
	}
	void Paint( const os::Rect& cUpdateRect )
	{
		/* Fill view with black and draw the bitmap in the center */
		SetFgColor( 0, 0, 0 );
		
		float nPosX = ( GetBounds().Width() - m_pcBitmap->GetBounds().Width() ) / 2 - 1;
		float nPosY = ( GetBounds().Height() - m_pcBitmap->GetBounds().Height() ) / 2 - 1;
		float nPosX2 = nPosX + m_pcBitmap->GetBounds().Width() + 2;
		float nPosY2 = nPosY + m_pcBitmap->GetBounds().Height() + 2;
		FillRect( os::Rect( 0, 0, nPosX, GetBounds().bottom ) );
		FillRect( os::Rect( nPosX2, 0, GetBounds().right, GetBounds().bottom ) );
		FillRect( os::Rect( nPosX, 0, nPosX2, nPosY ) );
		FillRect( os::Rect( nPosX, nPosY2, nPosX2, GetBounds().bottom ) );
		
		DrawBitmap( m_pcBitmap, m_pcBitmap->GetBounds(), os::Rect( nPosX + 1, nPosY + 1, 
					nPosX2 - 1, nPosY2 - 1 ) );
	}
	uint8* GetRaster()
	{
		return( m_pcBitmap->LockRaster() );
	}
private:
	os::Bitmap* m_pcBitmap;
};

CpuCaps gCpuCaps;

class VideoBitmapOutput : public os::MediaOutput
{
public:
	VideoBitmapOutput();
	~VideoBitmapOutput();
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
	void			UpdateView( uint32 nIndex );
	
	status_t		WritePacket( uint32 nIndex, os::MediaPacket_s *psFrame );
	uint64			GetDelay();
	uint32			GetUsedBufferPercentage();
	
private:
	VideoBitmapView* 	m_pcView;
	os::MediaFormat_s	m_sFormat;
	bool			m_bNoCache;
	int				m_nQueuedFrames;
	os::MediaPacket_s* m_psFrame[VIDEO_FRAME_NUMBER];
	sem_id			m_hLock;
	uint32			m_nCurrentFrame;
	bigtime_t		m_nFirstTime;
};


VideoBitmapOutput::VideoBitmapOutput()
{
	/* Set default values */
	m_pcView = NULL;
	m_nQueuedFrames = 0;
	m_nFirstTime = 0;
	m_nCurrentFrame = 0;
	m_hLock = create_semaphore( "bitmap_lock", 1, 0 );
	
}

VideoBitmapOutput::~VideoBitmapOutput()
{
	Close();
	delete_semaphore( m_hLock );
}

os::String VideoBitmapOutput::GetIdentifier()
{
	return( "Video Bitmap Output" );
}

os::View* VideoBitmapOutput::GetConfigurationView()
{
	return( NULL );
}

os::View* VideoBitmapOutput::GetVideoView( uint32 nIndex )
{
	if( nIndex == 0 )
		return( m_pcView );
	return( NULL );	
}

void VideoBitmapOutput::UpdateView( uint32 nIndex )
{
}

void VideoBitmapOutput::Flush()
{
	/* Always play the next packet in the queue */
	os::MediaPacket_s *psFrame;
	if( m_nFirstTime == 0 )
		m_nFirstTime = get_system_time();
	/* Calculate time for the next frame */
	bigtime_t nNextTime = m_nFirstTime + ( bigtime_t )( (float)m_nCurrentFrame  / m_sFormat.vFrameRate * 1000000 );

	if( nNextTime < get_system_time() || m_bNoCache )
	{
		lock_semaphore( m_hLock );
		if( m_nQueuedFrames > 0 ) 
		{
			/* Draw frame */
			psFrame = m_psFrame[0];
			bigtime_t nNextNextTime = m_nFirstTime + ( bigtime_t )( (float)( m_nCurrentFrame + 1 ) / m_sFormat.vFrameRate * 1000000 );
			if( ( nNextNextTime >= nNextTime ) || m_bNoCache ) {
			memcpy( m_pcView->GetRaster(), psFrame->pBuffer[0], m_sFormat.nWidth * m_sFormat.nHeight * 2 );
			m_pcView->Paint( m_pcView->GetBounds() );
			m_pcView->Sync();
			} else {
				std::cout<<"Framedrop"<<std::endl;
			}
			/* Move frames up */
			free( psFrame->pBuffer[0] );
			free( psFrame );
			
			m_nQueuedFrames--;
			for( int i = 0; i < m_nQueuedFrames; i++ )
			{
				m_psFrame[i] = m_psFrame[i+1];
			}
			
			//cout<<"Video:"<<m_nQueuedFrames<<" left "<<get_system_time()<<endl;
		}
		unlock_semaphore( m_hLock );
		/* Schedule next draw */
		m_nCurrentFrame++;
	}
}


bool VideoBitmapOutput::FileNameRequired()
{
	return( false );
}

status_t VideoBitmapOutput::Open( os::String zFileName )
{
	/* Clear frame buffer */
	Close();
	m_nQueuedFrames = 0;
	for( int i = 0; i < VIDEO_FRAME_NUMBER; i++ ) {
		m_psFrame[i] = NULL;
	}
	if( zFileName == "-nocache" ) {
		m_bNoCache = true;
		std::cout<<"Video caching disabled"<<std::endl;
	}
	else
		m_bNoCache = false;
	/* Initialize yuv to rgb converter */
	/* Intialize cpu structure for mplayer code */
	memset( &gCpuCaps, 0, sizeof( CpuCaps ) );
	gCpuCaps.hasMMX = 0;
	gCpuCaps.hasMMX2 = 1;
	gCpuCaps.isX86 = 1;
	yuv2rgb_init( 16, MODE_RGB );
	return( 0 );
}

void VideoBitmapOutput::Close()
{
	
	Clear();
	//cout<<"Video closed"<<endl;
}

void VideoBitmapOutput::Clear()
{
	m_nFirstTime = 0;
	m_nCurrentFrame = 0;
	/* Delete all pending frames */
	for( int i = 0; i < m_nQueuedFrames; i++ ) {
		free( m_psFrame[i]->pBuffer[0] );
		free( m_psFrame[i] );
	}
	m_nQueuedFrames = 0;
}

uint32 VideoBitmapOutput::GetOutputFormatCount()
{
	return( 1 );
}

os::MediaFormat_s VideoBitmapOutput::GetOutputFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	MEDIA_CLEAR_FORMAT( sFormat );
	sFormat.zName = "Raw Video";
	sFormat.nType = os::MEDIA_TYPE_VIDEO;
	
	return( sFormat );
}

uint32 VideoBitmapOutput::GetSupportedStreamCount()
{
	return( 1 );
}

status_t VideoBitmapOutput::AddStream( os::String zName, os::MediaFormat_s sFormat )
{
	if( sFormat.zName == "Raw Video" && sFormat.nType == os::MEDIA_TYPE_VIDEO 
		&& sFormat.eColorSpace == os::CS_YUV12 ) {}
	else
		return( -1 );
	
	/* Save format */
	m_sFormat = sFormat;
	
	/* Create view */
	os::Rect cFrame( 0, 0, sFormat.nWidth - 1, sFormat.nHeight - 1 );
	m_pcView = new VideoBitmapView( cFrame, "media_video" );
	
	if( m_pcView == NULL )
		return( -1 );
	return( 0 );
}

status_t VideoBitmapOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psFrame )
{
	/* Create a new media frame and queue it */
	os::MediaPacket_s* psQueueFrame = ( os::MediaPacket_s* )malloc( sizeof( os::MediaPacket_s ) );
	if( psQueueFrame == NULL )
		return( -1 );
	memset( psQueueFrame, 0, sizeof( os::MediaPacket_s ) );
	psQueueFrame->nSize[0] = m_sFormat.nWidth * 2;
	psQueueFrame->pBuffer[0] = ( uint8* )malloc( m_sFormat.nWidth * m_sFormat.nHeight * 2 );
	
	yuv2rgb( psQueueFrame->pBuffer[0], psFrame->pBuffer[0], psFrame->pBuffer[1], psFrame->pBuffer[2],
			m_sFormat.nWidth, m_sFormat.nHeight, m_sFormat.nWidth * 2, psFrame->nSize[0],
			psFrame->nSize[1] );
	
	lock_semaphore( m_hLock );
	/* Queue packet */
	if( m_nQueuedFrames > VIDEO_FRAME_NUMBER - 1 ) {
		unlock_semaphore( m_hLock );
		free( psQueueFrame->pBuffer[0] );
		free( psQueueFrame );
		std::cout<<"Frame buffer full"<<std::endl;
		return( -1 );
	}
	m_psFrame[m_nQueuedFrames] = psQueueFrame;
	m_nQueuedFrames++;
	unlock_semaphore( m_hLock );
	//cout<<"Frame queued at position "<<m_nQueuedFrames - 1<<"( "<<m_nQueuedFrames * 100 / VIDEO_FRAME_NUMBER<<
	//"% of the buffer used )"<<endl;
	return( 0 );	
}

uint64 VideoBitmapOutput::GetDelay()
{
	return( 1000 * m_nQueuedFrames / (int)m_sFormat.vFrameRate );
}


uint32 VideoBitmapOutput::GetUsedBufferPercentage()
{
	uint32 nValue;
	lock_semaphore( m_hLock );
	nValue = m_nQueuedFrames * 100 / VIDEO_FRAME_NUMBER;
	unlock_semaphore( m_hLock );
	return( nValue );
}

class VideoBitmapAddon : public os::MediaAddon
{
public:
	status_t Initialize() { 
		return( 0 );
	}
	os::String GetIdentifier() { return( "Video Bitmap" ); }
	uint32			GetOutputCount() { return( 1 ); }
	os::MediaOutput* GetOutput( uint32 nIndex ) { return( new VideoBitmapOutput() ); }
private:
};

extern "C"
{
	os::MediaAddon* init_media_addon()
	{
		return( new VideoBitmapAddon() );
	}
}




