/*  Screen output plugin
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
#include <gui/point.h>
#include <gui/desktop.h>
#include <atheos/threads.h>
#include <atheos/semaphore.h>
#include <atheos/kernel.h>
#include <atheos/time.h>
#include <util/messenger.h>
#include <util/message.h>
#include <util/application.h>
#include <appserver/protocol.h>
#include <inttypes.h>
#include <iostream>
extern "C" 
{
	#include "mplayercomp.h"
	#include "rgb2rgb.h"
}

#define VIDEO_FRAME_NUMBER 20

/* Private structure for communication with the appserver */
struct video_overlay
{
	int			m_nSrcWidth;
	int			m_nSrcHeight;
	int			m_nDstX;
	int			m_nDstY;
	int			m_nDstWidth;
	int			m_nDstHeight;
	os::color_space m_eFormat;
	os::Color32_s	m_sColorKey;
	area_id		m_hPhysArea;
	area_id		m_hArea;
	uint8*		m_pAddress;	
};

class VideoView : public os::View
{
public:
	VideoView( bool bUseOverlay, const os::IPoint & cSrcSize, os::color_space eFormat, os::Color32_s sColorKey, const os::Rect& cFrame, const std::string&cTitle ) 
				: os::View( cFrame, cTitle, os::CF_FOLLOW_ALL )
	{
		m_bUseOverlay = bUseOverlay;
		if( bUseOverlay )
		{
			m_pcBitmap = NULL;
			m_sOverlay.m_pAddress = NULL;
			m_sOverlay.m_hArea = -1;
			m_sOverlay.m_eFormat = eFormat;
			m_sOverlay.m_nSrcWidth = cSrcSize.x;
			m_sOverlay.m_nSrcHeight = cSrcSize.y;
			m_sOverlay.m_sColorKey = sColorKey;
		} else
		{
			m_pcBitmap = new os::Bitmap( cSrcSize.x, cSrcSize.y, eFormat );
		}
	}
	~VideoView()
	{
		if( m_bUseOverlay && m_sOverlay.m_hArea > 0 )
			delete_area( m_sOverlay.m_hArea );
		
		if( m_pcBitmap != NULL )
			delete( m_pcBitmap );
	}
	void Update()
	{
		if( !m_bUseOverlay )
			return;
	
		/* Check if the application object has been created */
		if( os::Application::GetInstance() == NULL )
		{
			dbprintf( "Error: Invalid application object\n" );
		}

		/* Check if the position of the overlay has changed */
		if( ConvertToScreen( GetBounds() ) != m_cCurrentFrame )
		{
			m_cCurrentFrame = ConvertToScreen( GetBounds() );
			Recreate( os::IPoint( m_sOverlay.m_nSrcWidth, m_sOverlay.m_nSrcHeight ), m_cCurrentFrame );
		}

		/* Send update message */
		os::Message cReq( os::WR_UPDATE_VIDEO_OVERLAY );

		cReq.AddInt32( "area", m_sOverlay.m_hPhysArea );
		os::Application *pcApp = os::Application::GetInstance();

		os::Messenger( pcApp->GetAppPort() ).SendMessage( &cReq );
	}
	
	void AttachedToWindow()
	{
		if( !m_bUseOverlay )
			return;
	
		/* Save information */
		m_cCurrentFrame = ConvertToScreen( GetBounds() );
		m_sOverlay.m_nDstX = ( int )m_cCurrentFrame.left;
		m_sOverlay.m_nDstY = ( int )m_cCurrentFrame.top;
		m_sOverlay.m_nDstWidth = ( int )m_cCurrentFrame.Width() + 1;
		m_sOverlay.m_nDstHeight = ( int )m_cCurrentFrame.Height() + 1;
		m_sOverlay.m_pAddress = NULL;
		m_sOverlay.m_hArea = -1;

		/* Check if the application object has been created */
		if( os::Application::GetInstance() == NULL )
		{
			dbprintf( "Error: Invalid application object\n" );
		}

		/* Send message */
		os::Message cReq( os::WR_CREATE_VIDEO_OVERLAY );
		os::Message cReply;

		int32 nFormat = static_cast < int32 >( m_sOverlay.m_eFormat );

		cReq.AddIPoint( "size", os::IPoint( m_sOverlay.m_nSrcWidth, m_sOverlay.m_nSrcHeight ) );
		cReq.AddRect( "dst", os::IRect( m_sOverlay.m_nDstX, m_sOverlay.m_nDstY, m_sOverlay.m_nDstX + m_sOverlay.m_nDstWidth - 1, m_sOverlay.m_nDstY + m_sOverlay.m_nDstHeight - 1 ) );
		cReq.AddInt32( "format", nFormat );
		cReq.AddColor32( "color_key", m_sOverlay.m_sColorKey );
	
		os::Application *pcApp = os::Application::GetInstance();

		os::Messenger( pcApp->GetAppPort() ).SendMessage( &cReq, &cReply );

		area_id hArea;
		int nError;
		void *pAddress = NULL;

		cReply.FindInt( "error", &nError );

		if( nError != 0 )
		{
			errno = nError;
			m_sOverlay.m_pAddress = NULL;
			dbprintf( "Error: Could not create video overlay!\n" );
			return;
		}

		cReply.FindInt32( "area", ( int32 * )&hArea );

		m_sOverlay.m_hPhysArea = hArea;
		m_sOverlay.m_hArea = clone_area( "video_overlay", &pAddress, AREA_FULL_ACCESS, AREA_NO_LOCK, hArea );
		m_sOverlay.m_pAddress = ( uint8 * )pAddress;
	}

	void DetachedFromWindow()
	{
		if( !m_bUseOverlay )
			return;
		/* Check if the application object has been created */
		if( os::Application::GetInstance() == NULL )
		{
			dbprintf( "Error Invalid application object\n" );
		}

		os::Message cReq( os::WR_DELETE_VIDEO_OVERLAY );

		if( m_sOverlay.m_hArea > 0 )
			delete_area( m_sOverlay.m_hArea );
		m_sOverlay.m_hArea = -1;
		
		cReq.AddInt32( "area", m_sOverlay.m_hPhysArea );
		os::Application *pcApp = os::Application::GetInstance();

		os::Messenger( pcApp->GetAppPort() ).SendMessage( &cReq );
	}

	void Recreate( const os::IPoint & cSrcSize, const os::IRect & cDstRect )
	{
		if( !m_bUseOverlay )
			return;
		/* Check if the application object has been created */
		if( os::Application::GetInstance() == NULL )
		{
			dbprintf( "Error invalid application object\n" );
		}

		/* Save information */
		m_sOverlay.m_nSrcWidth = cSrcSize.x;
		m_sOverlay.m_nSrcHeight = cSrcSize.y;
		m_sOverlay.m_nDstX = cDstRect.left;
		m_sOverlay.m_nDstY = cDstRect.top;
		m_sOverlay.m_nDstWidth = cDstRect.Width() + 1;
		m_sOverlay.m_nDstHeight = cDstRect.Height() + 1;
		m_sOverlay.m_pAddress = NULL;

		/* Send message */
		os::Message cReq( os::WR_RECREATE_VIDEO_OVERLAY );
		os::Message cReply;

		if( m_sOverlay.m_hArea > 0 )
			delete_area( m_sOverlay.m_hArea );
		m_sOverlay.m_hArea = -1;

		int32 nFormat = static_cast < int32 >( m_sOverlay.m_eFormat );

		cReq.AddIPoint( "size", os::IPoint( m_sOverlay.m_nSrcWidth, m_sOverlay.m_nSrcHeight ) );
		cReq.AddRect( "dst", os::IRect( m_sOverlay.m_nDstX, m_sOverlay.m_nDstY, m_sOverlay.m_nDstX + m_sOverlay.m_nDstWidth - 1, m_sOverlay.m_nDstY + m_sOverlay.m_nDstHeight - 1 ) );
		cReq.AddInt32( "format", nFormat );
		cReq.AddInt32( "area", m_sOverlay.m_hPhysArea );

		os::Application *pcApp = os::Application::GetInstance();

		os::Messenger( pcApp->GetAppPort() ).SendMessage( &cReq, &cReply );

		int nError;
		void *pAddress = NULL;
		area_id hArea;

		cReply.FindInt( "error", &nError );

		if( nError != 0 )
		{
			errno = nError;
			m_sOverlay.m_pAddress = NULL;
			dbprintf( "Error: Could not create video overlay!\n" );
			return;
		}

		cReply.FindInt32( "area", ( int32 * )&hArea );

		m_sOverlay.m_hPhysArea = hArea;
		m_sOverlay.m_hArea = clone_area( "video_overlay", &pAddress, AREA_FULL_ACCESS, AREA_NO_LOCK, hArea );
		m_sOverlay.m_pAddress = ( uint8 * )pAddress;
	}

	void Paint( const os::Rect& cUpdateRect )
	{
		if( m_bUseOverlay )
		{
			/* Paint everything in the color key color */
			SetFgColor( m_sOverlay.m_sColorKey );
			FillRect( cUpdateRect );
		} else
		{
			DrawBitmap( m_pcBitmap, m_pcBitmap->GetBounds(), GetBounds() );
		}
	}
	uint8* GetRaster()
	{
		if( m_bUseOverlay )
			return( m_sOverlay.m_pAddress );
		else
			return( m_pcBitmap->LockRaster() );
	}
private:
	bool m_bUseOverlay;
	os::Bitmap* m_pcBitmap;
	video_overlay 	m_sOverlay;
    os::Rect		m_cCurrentFrame;
};


CpuCaps gCpuCaps;

class ScreenOutput : public os::MediaOutput
{
public:
	ScreenOutput();
	~ScreenOutput();
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
	
	void			SetTimeSource( os::MediaTimeSource* pcSource );
	
	os::View*		GetVideoView( uint32 nIndex );
	void			UpdateView( uint32 nStream );
	
	status_t		WritePacket( uint32 nIndex, os::MediaPacket_s *psFrame );
	uint64			GetDelay();
	uint32			GetUsedBufferPercentage();
private:
	bool			CheckVideoOverlay( os::color_space eSpace );
	bool			m_bUseOverlay;
	os::color_space m_eColorSpace;
	VideoView*		m_pcView;
	os::MediaFormat_s	m_sFormat;
	int				m_nQueuedFrames;
	os::MediaPacket_s* m_psFrame[VIDEO_FRAME_NUMBER];
	sem_id			m_hLock;
	uint32			m_nCurrentFrame;
	uint64			m_nFirstTimeStamp;
	bigtime_t		m_nStartTime;
	os::MediaTimeSource* m_pcTimeSource;
};


ScreenOutput::ScreenOutput()
{
	/* Set default values */
	m_bUseOverlay = false;
	m_pcView = NULL;
	m_nQueuedFrames = 0;
	m_nStartTime = 0;
	m_nCurrentFrame = 0;
	m_hLock = create_semaphore( "overlay_lock", 1, 0 );
	m_pcTimeSource = NULL;
}

ScreenOutput::~ScreenOutput()
{
	Close();
	delete_semaphore( m_hLock );
}

os::String ScreenOutput::GetIdentifier()
{
	return( "Screen Video Output" );
}


os::View* ScreenOutput::GetConfigurationView()
{
	return( NULL );
}

os::View* ScreenOutput::GetVideoView( uint32 nIndex )
{
	if( nIndex == 0 )
		return( m_pcView );
	return( NULL );	
}

void ScreenOutput::UpdateView( uint32 nIndex )
{
	if( m_pcView ) {
		lock_semaphore( m_hLock );
		m_pcView->Update();
		unlock_semaphore( m_hLock );
	}
}


void ScreenOutput::Flush()
{
	/* Always play the next packet in the queue */
	os::MediaPacket_s *psFrame;
	
	lock_semaphore( m_hLock );
	if( m_nQueuedFrames > 0 ) 
	{
		/* Get next frame */
		psFrame = m_psFrame[0];
		bool bDrawFrame = false;
		uint64 nNoTimeStamp = ~0;
		
		/* Start timer */
		if( m_nStartTime == 0 )
		{
			std::cout<<"Video Start!"<<std::endl;
			m_nFirstTimeStamp = psFrame->nTimeStamp;
			m_nStartTime = get_system_time();
		}
		
		if( psFrame->nTimeStamp != nNoTimeStamp )
		{
			/* Use the timestamp */
			uint64 nFrameLength = 1000 / (uint64)m_sFormat.vFrameRate;
			uint64 nPacketPosition = psFrame->nTimeStamp + nFrameLength - ( m_pcTimeSource != NULL ? 0 : m_nFirstTimeStamp );
			uint64 nRealPosition;
			if( m_pcTimeSource )
				nRealPosition = m_pcTimeSource->GetCurrentTime();
			else
				nRealPosition = ( get_system_time() - m_nStartTime ) / 1000;
			
			//std::cout<<psFrame->nTimeStamp<<std::endl;
			#if 0
			std::cout<<"VIDEO Time "<<nRealPosition<<" Stamp "<<
							nPacketPosition<<" "<<nFrameLength<<" "<<m_nQueuedFrames<<std::endl;
			#endif							
			//std::cout<<(int64)nPacketPosition - (int64)nRealPosition<<std::endl;
			
			if( nPacketPosition > nRealPosition )
			{
				/* Frame in the future */
				unlock_semaphore( m_hLock );
				return;
			}
			
			
			if( nRealPosition > nPacketPosition + nFrameLength * 2 )
			{
				std::cout<<"Framedrop"<<std::endl;
			} else {
				//std::cout<<"Draw!"<<std::endl;
				bDrawFrame = true;
			}
			
		} else {
			/* Calculate time for the next frame */
			bigtime_t nNextTime;
			if( m_pcTimeSource )
				nNextTime = ( bigtime_t )( (float)m_nCurrentFrame  / m_sFormat.vFrameRate * 1000000 );
			else
				nNextTime = m_nStartTime + ( bigtime_t )( (float)m_nCurrentFrame / m_sFormat.vFrameRate * 1000000 );

			if( nNextTime < ( m_pcTimeSource ? m_pcTimeSource->GetCurrentTime() : get_system_time() ) )
			{
			
				bigtime_t nNextNextTime = nNextTime + 1000000 / (bigtime_t)m_sFormat.vFrameRate * 1000000;
				if( ( nNextNextTime >= nNextTime ) ) {
					bDrawFrame = true;
				} else {
					std::cout<<"Framedrop"<<std::endl;
				}
			} else {
				/* Frame in the future */
				unlock_semaphore( m_hLock );
				return;
			}
		}
		
		if( bDrawFrame )
		{
			/* Draw frame */
			if( !m_bUseOverlay )
			{
				/* No video overlay */
				memcpy( m_pcView->GetRaster(), psFrame->pBuffer[0], m_sFormat.nWidth * m_sFormat.nHeight * BitsPerPixel( m_eColorSpace ) / 8 );
				m_pcView->Paint( m_pcView->GetBounds() );
				m_pcView->Sync();
			}
			else if( m_eColorSpace == os::CS_YUV12 )
			{
				/* YV12 Overlay */
				uint32 nDstPitch = ( ( m_sFormat.nWidth ) + 0x1ff ) & ~0x1ff;
				for( int i = 0; i < m_sFormat.nHeight; i++ )
				{
					memcpy( m_pcView->GetRaster() + i * nDstPitch, psFrame->pBuffer[0] + psFrame->nSize[0] * i, m_sFormat.nWidth );
				}
				for( int i = 0; i < m_sFormat.nHeight / 2; i++ )
				{
					memcpy( m_pcView->GetRaster() + m_sFormat.nHeight * nDstPitch * 5 / 4 + nDstPitch * i / 2, 
						psFrame->pBuffer[1] + psFrame->nSize[1] * i, m_sFormat.nWidth / 2 );
				}
				for( int i = 0; i < m_sFormat.nHeight / 2; i++ )
				{
					memcpy( m_pcView->GetRaster() +  m_sFormat.nHeight * nDstPitch + nDstPitch * i / 2, 
							psFrame->pBuffer[2] + psFrame->nSize[2] * i, m_sFormat.nWidth / 2 );
				}
				free( psFrame->pBuffer[1] );
				free( psFrame->pBuffer[2] );
			} 
			else if( m_eColorSpace == os::CS_YUV422 || m_eColorSpace == os::CS_YUY2 )
			{
				/* UYVY Overlay */
				uint32 nDstPitch = ( ( m_sFormat.nWidth * 2 ) + 0xff ) & ~0xff;
				for( int i = 0; i < m_sFormat.nHeight; i++ )
				{
					memcpy( m_pcView->GetRaster() + i * nDstPitch, psFrame->pBuffer[0] + m_sFormat.nWidth * 2 * i, m_sFormat.nWidth * 2 );
				}
			} else {
				/* RGB32 */
				for( int i = 0; i < m_sFormat.nHeight; i++ )
				{
					memcpy( m_pcView->GetRaster() + i * m_sFormat.nWidth * 4, psFrame->pBuffer[0] + m_sFormat.nWidth * 4 * i, m_sFormat.nWidth * 4 );
				}
			}
			
			m_pcView->Update();
		}
		
		/* Move frames up */
		free( psFrame->pBuffer[0] );
		free( psFrame );
		
		m_nQueuedFrames--;
		for( int i = 0; i < m_nQueuedFrames; i++ )
		{
			m_psFrame[i] = m_psFrame[i+1];
		}
		
		/* Frame counter */
		m_nCurrentFrame++;
	}
	unlock_semaphore( m_hLock );
}


bool ScreenOutput::FileNameRequired()
{
	return( false );
}

bool ScreenOutput::CheckVideoOverlay( os::color_space eSpace )
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

status_t ScreenOutput::Open( os::String zFileName )
{
	/* Clear frame buffer */
	Close();
	m_nQueuedFrames = 0;
	m_nStartTime = 0;
	for( int i = 0; i < VIDEO_FRAME_NUMBER; i++ ) {
		m_psFrame[i] = NULL;
	}
		
	/* Intialize cpu structure for mplayer code */
	memset( &gCpuCaps, 0, sizeof( CpuCaps ) );
	gCpuCaps.hasMMX = 0;
	gCpuCaps.hasMMX2 = 1;
	gCpuCaps.isX86 = 1;
		
	/* Check video overlay */
	m_bUseOverlay = true;
	m_eColorSpace = os::CS_YUV12;
	if( !CheckVideoOverlay( m_eColorSpace ) ) {

		m_eColorSpace = os::CS_YUV422;
		if( !CheckVideoOverlay( m_eColorSpace ) ) {

			m_eColorSpace = os::CS_YUY2;
			if( !CheckVideoOverlay( m_eColorSpace ) ) {
				m_eColorSpace = os::CS_RGB32;

				if( !CheckVideoOverlay( m_eColorSpace ) ) {
					std::cout<<"Using bitmap for video output"<<std::endl;
					m_bUseOverlay = false;
					os::Desktop cDesktop;
					m_eColorSpace = cDesktop.GetColorSpace();
					if( m_eColorSpace == os::CS_RGB16 )
						yuv2rgb_init( 16, MODE_RGB );
					else
						yuv2rgb_init( 32, MODE_RGB );
				} else {
					std::cout<<"Using RGB32 Overlay"<<std::endl;
					yuv2rgb_init( 32, MODE_RGB );
				}

			} else {
				std::cout<<"Using YUY2 overlay"<<std::endl;
			}
		} else {
			std::cout<<"Using UYVY Overlay"<<std::endl;
		}
	} else {
		std::cout<<"Using YV12 Overlay"<<std::endl;
	}
	return( 0 );
}

void ScreenOutput::Close()
{
	
	Clear();
	/*if( m_pcView != NULL )
		delete( m_pcView );*/
	//cout<<"Video closed"<<endl;
}


void ScreenOutput::Clear()
{
	m_nStartTime = 0;
	m_nCurrentFrame = 0;
	/* Delete all pending frames */
	for( int i = 0; i < m_nQueuedFrames; i++ ) {
		free( m_psFrame[i]->pBuffer[0] );
		if( m_eColorSpace == os::CS_YUV12 ) {
			free( m_psFrame[i]->pBuffer[1] );
			free( m_psFrame[i]->pBuffer[2] );
		}
		free( m_psFrame[i] );
	}
	m_nQueuedFrames = 0;
}

uint32 ScreenOutput::GetOutputFormatCount()
{
	return( 1 );
}

os::MediaFormat_s ScreenOutput::GetOutputFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	MEDIA_CLEAR_FORMAT( sFormat );
	sFormat.zName = "Raw Video";
	sFormat.nType = os::MEDIA_TYPE_VIDEO;
	
	return( sFormat );
}

uint32 ScreenOutput::GetSupportedStreamCount()
{
	return( 1 );
}

status_t ScreenOutput::AddStream( os::String zName, os::MediaFormat_s sFormat )
{
	if( sFormat.zName == "Raw Video" && sFormat.nType == os::MEDIA_TYPE_VIDEO 
		&& sFormat.eColorSpace == os::CS_YUV12 ) {}
	else
		return( -1 );
	
	/* Save format */
	m_sFormat = sFormat;
	
	/* Open Overlay */
	os::Rect cFrame( 0, 0, sFormat.nWidth - 1, sFormat.nHeight - 1 );
	m_pcView = new VideoView( m_bUseOverlay, os::IPoint( sFormat.nWidth, sFormat.nHeight ), m_eColorSpace, os::Color32_s( 0, 0, 255 ), 
				cFrame, "media_video" );
	if( m_pcView == NULL )
		return( -1 );
	return( 0 );
}


void ScreenOutput::SetTimeSource( os::MediaTimeSource* pcSource )
{
	std::cout<< "Using timesource "<<pcSource->GetIdentifier().c_str()<<std::endl;
	m_pcTimeSource = pcSource;
}

status_t ScreenOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psFrame )
{
	/* Create a new media frame and queue it */
	os::MediaPacket_s* psQueueFrame = ( os::MediaPacket_s* )malloc( sizeof( os::MediaPacket_s ) );
	if( psQueueFrame == NULL )
		return( -1 );
	memset( psQueueFrame, 0, sizeof( os::MediaPacket_s ) );
	psQueueFrame->nTimeStamp = psFrame->nTimeStamp;
	
	if( !m_bUseOverlay )
	{
		/* No overlay */
		psQueueFrame->nSize[0] = m_sFormat.nWidth * BitsPerPixel( m_eColorSpace ) / 8;
		psQueueFrame->pBuffer[0] = ( uint8* )malloc( m_sFormat.nWidth * m_sFormat.nHeight * BitsPerPixel( m_eColorSpace ) / 8 );
		psQueueFrame->nTimeStamp = psFrame->nTimeStamp;
	
		yuv2rgb( psQueueFrame->pBuffer[0], psFrame->pBuffer[0], psFrame->pBuffer[1], psFrame->pBuffer[2],
			m_sFormat.nWidth, m_sFormat.nHeight, psQueueFrame->nSize[0], psFrame->nSize[0],
			psFrame->nSize[1] );
	}
	else if( m_eColorSpace == os::CS_YUV12 )
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
	else if( m_eColorSpace == os::CS_YUY2 )
	{
		/* Convert to YUY2 */
		psQueueFrame->nSize[0] = m_sFormat.nWidth * 2;
		psQueueFrame->pBuffer[0] = ( uint8* )malloc( psQueueFrame->nSize[0] * m_sFormat.nHeight );
		yv12toyuy2( psFrame->pBuffer[0], psFrame->pBuffer[1], psFrame->pBuffer[2], psQueueFrame->pBuffer[0],
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
			/* UYVY / YUY2 / RGB32 / RGB16 */
			free( psQueueFrame->pBuffer[0] );
		}
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

uint64 ScreenOutput::GetDelay()
{
	return( 1000 * m_nQueuedFrames / (int)m_sFormat.vFrameRate );
}


uint32 ScreenOutput::GetUsedBufferPercentage()
{
	uint32 nValue;
	lock_semaphore( m_hLock );
	nValue = m_nQueuedFrames * 100 / VIDEO_FRAME_NUMBER;
	unlock_semaphore( m_hLock );
	return( nValue );
}


class ScreenAddon : public os::MediaAddon
{
public:
	status_t Initialize() { 
		return( 0 );
	}
	os::String GetIdentifier() { return( "Screen Video Output" ); }
	uint32			GetOutputCount() { return( 1 ); }
	os::MediaOutput* GetOutput( uint32 nIndex ) { return( new ScreenOutput() ); }
private:
};

extern "C"
{
	os::MediaAddon* init_media_addon()
	{
		return( new ScreenAddon() );
	}
}











































































































