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
	uint32			GetPhysicalType()
	{
		return( os::MEDIA_PHY_SCREEN_OUT );
	}
	os::View*		GetConfigurationView();
	
	bool			FileNameRequired();
	
	status_t 		Open( os::String zFileName );
	void 			Close();
	
	void			Clear();
	
	uint32			GetOutputFormatCount();
	os::MediaFormat_s	GetOutputFormat( uint32 nIndex );
	uint32			GetSupportedStreamCount();
	status_t		AddStream( os::String zName, os::MediaFormat_s sFormat );
	
	
	os::View*		GetVideoView( uint32 nIndex );
	void			UpdateView( uint32 nStream );
	
	status_t		WritePacket( uint32 nIndex, os::MediaPacket_s *psFrame );
	uint64			GetDelay( bool bNonSharedOnly );
	uint64			GetBufferSize( bool bNonSharedOnly );
private:
	bool			CheckVideoOverlay( os::color_space eSpace );
	bool			m_bUseOverlay;
	os::color_space m_eColorSpace;
	VideoView*		m_pcView;
	os::MediaFormat_s	m_sFormat;
	sem_id			m_hLock;
};


ScreenOutput::ScreenOutput()
{
	/* Set default values */
	m_bUseOverlay = false;
	m_pcView = NULL;
	m_hLock = create_semaphore( "overlay_lock", 1, 0 );
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

status_t ScreenOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psNewFrame )
{
	/* Create a new media frame and queue it */
	bigtime_t nTime = get_system_time();
	if( !m_bUseOverlay )
	{
		/* No overlay */
		
		yuv2rgb( m_pcView->GetRaster(), psNewFrame->pBuffer[0], psNewFrame->pBuffer[1], psNewFrame->pBuffer[2],
			m_sFormat.nWidth, m_sFormat.nHeight, m_sFormat.nWidth * BitsPerPixel( m_eColorSpace ) / 8, psNewFrame->nSize[0],
			psNewFrame->nSize[1] );
		
		m_pcView->Paint( m_pcView->GetBounds() );
		m_pcView->Sync();
		
	}
	else if( m_eColorSpace == os::CS_YUV12 )
	{
		/* YV12 overlay */
		uint32 nDstPitch = ( ( m_sFormat.nWidth ) + 0x1ff ) & ~0x1ff;
		for( int i = 0; i < m_sFormat.nHeight; i++ )
		{
			memcpy( m_pcView->GetRaster() + i * nDstPitch, psNewFrame->pBuffer[0] + psNewFrame->nSize[0] * i, m_sFormat.nWidth );
		}
		for( int i = 0; i < m_sFormat.nHeight / 2; i++ )
		{
			memcpy( m_pcView->GetRaster() + m_sFormat.nHeight * nDstPitch * 5 / 4 + nDstPitch * i / 2, 
				psNewFrame->pBuffer[1] + psNewFrame->nSize[1] * i, m_sFormat.nWidth / 2 );
		}
		for( int i = 0; i < m_sFormat.nHeight / 2; i++ )
		{
			memcpy( m_pcView->GetRaster() +  m_sFormat.nHeight * nDstPitch + nDstPitch * i / 2, 
						psNewFrame->pBuffer[2] + psNewFrame->nSize[2] * i, m_sFormat.nWidth / 2 );
		}
	} 
	else if( m_eColorSpace == os::CS_YUV422 )
	{
		/* UYVY overlay */
		uint32 nDstPitch = ( ( m_sFormat.nWidth * 2 ) + 0xff ) & ~0xff;
		
		yv12touyvy( psNewFrame->pBuffer[0], psNewFrame->pBuffer[1], psNewFrame->pBuffer[2], m_pcView->GetRaster(),
					m_sFormat.nWidth, m_sFormat.nHeight, psNewFrame->nSize[0], psNewFrame->nSize[1],
					nDstPitch );
		
	}
	else if( m_eColorSpace == os::CS_YUY2 )
	{
		/* YUY2 */
		uint32 nDstPitch = ( ( m_sFormat.nWidth * 2 ) + 0xff ) & ~0xff;
		
		yv12toyuy2( psNewFrame->pBuffer[0], psNewFrame->pBuffer[1], psNewFrame->pBuffer[2], m_pcView->GetRaster(),
					m_sFormat.nWidth, m_sFormat.nHeight, psNewFrame->nSize[0], psNewFrame->nSize[1],
					nDstPitch );
	}
	else 
	{
		/* RGB32 */
		yuv2rgb( m_pcView->GetRaster(), psNewFrame->pBuffer[0], psNewFrame->pBuffer[1], psNewFrame->pBuffer[2],
			m_sFormat.nWidth, m_sFormat.nHeight, m_sFormat.nWidth * 4, psNewFrame->nSize[0],
			psNewFrame->nSize[1] );
		
	}
	
	
	m_pcView->Update();
	
	return( 0 );	
}

uint64 ScreenOutput::GetDelay( bool bNonSharedOnly )
{
	return( 0 );
}


uint64 ScreenOutput::GetBufferSize( bool bNonSharedOnly )
{
	return( 0 );
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
	os::MediaAddon* init_media_addon( os::String zDevice )
	{
		return( new ScreenAddon() );
	}
}











































































































