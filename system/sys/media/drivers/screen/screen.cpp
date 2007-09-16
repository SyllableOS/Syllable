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
#include "mmx.h"
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
		SetEraseColor( os::Color32_s( 0, 0, 0 ) );
		m_bBitmapCleared = false;
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
		int nError = 0;
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

		int nError = 0;
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
		} 
		else
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
	void SetBitmapCleared( const bool bCleared )
	{
		if( !m_bBitmapCleared && bCleared )
		{
			/* If we switch from drawing the video using DrawBitmap() to direct framebuffer
			   access we clear the bitmap */
			memset( m_pcBitmap->LockRaster(), 0, m_pcBitmap->GetBytesPerRow() * (int)( m_pcBitmap->GetBounds().Height() + 1 ) );
			/* These calls are necessary to update the window backbuffer */
			Invalidate();
			Flush();
		}
		m_bBitmapCleared = bCleared;
	}
private:
	bool m_bUseOverlay;
	os::Bitmap* m_pcBitmap;
	video_overlay 	m_sOverlay;
    os::Rect		m_cCurrentFrame;
    bool			m_bBitmapCleared;
};


CpuCaps gCpuCaps;

namespace os
{

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
	inline void		mmx_memcpy( uint8 *pTo, uint8 *pFrom, int nLen );
	bool			m_bUseOverlay;
	os::color_space m_eColorSpace;
	VideoView*		m_pcView;
	os::MediaFormat_s	m_sFormat;
	sem_id			m_hLock;
	bool			m_bUseMMX;
	bool			m_bUseMMX2;
	bool			m_bUseFB;
	port_id			m_hReplyPort;
	os::Desktop		m_cDesktop;
};

};

using namespace os;

ScreenOutput::ScreenOutput()
{
	/* Set default values */
	m_bUseOverlay = false;
	m_pcView = NULL;
	m_hLock = create_semaphore( "overlay_lock", 1, 0 );
	m_hReplyPort = create_port( "screen_reply", DEFAULT_PORT_SIZE );
	
	/* Get system information and check if we have MMX support */
	system_info sSysInfo;
	get_system_info( &sSysInfo );

	m_bUseMMX = sSysInfo.nCPUType & CPU_FEATURE_MMX;
	m_bUseMMX2 = sSysInfo.nCPUType & CPU_FEATURE_MMX2;
}

ScreenOutput::~ScreenOutput()
{
	Close();
	delete_semaphore( m_hLock );
	delete_port( m_hReplyPort );
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
    
    int nError = 0;
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
	/* Check for direct framebuffer access */
	if( zFileName == "-fb" )
		m_bUseFB = true;
	
	/* Clear frame buffer */
	Close();
	
		
	/* Intialize cpu structure for mplayer code */
	memset( &gCpuCaps, 0, sizeof( CpuCaps ) );
	gCpuCaps.hasMMX = 0;
	gCpuCaps.hasMMX2 = m_bUseMMX2;
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

inline void ScreenOutput::mmx_memcpy( uint8 *pTo, uint8 *pFrom, int nLen )
{
	int i;
	
	if( !m_bUseMMX ) {
		memcpy( pTo, pFrom, nLen );
		return;
	}

	for ( i = 0; i < nLen / 8; i++ )
	{
		__asm__ __volatile__( "movq (%0), %%mm0\n" "movq %%mm0, (%1)\n"::"r"( pFrom ), "r"( pTo ):"memory" );

		pFrom += 8;
		pTo += 8;
	}

	if ( nLen & 7 )
		memcpy( pTo, pFrom, nLen & 7 );
}

static uint64 __attribute__((aligned(8))) g_anMMX80W = 0x0080008000800080LL;
static uint64 __attribute__((aligned(8))) g_anMMX10W = 0x1010101010101010LL;
static uint64 __attribute__((aligned(8))) g_anMMX00FFW = 0x00ff00ff00ff00ffLL;
static uint64 __attribute__((aligned(8))) g_anMMXYCoeff = 0x253f253f253f253fLL;
static uint64 __attribute__((aligned(8))) g_anUGreen = 0xf37df37df37df37dLL;
static uint64 __attribute__((aligned(8))) g_anVGreen = 0xe5fce5fce5fce5fcLL;
static uint64 __attribute__((aligned(8))) g_anUBlue = 0x4093409340934093LL;
static uint64 __attribute__((aligned(8))) g_anVRed = 0x3312331233123312LL;

static void yv12torgb32_scaled( uint8* pDstPtr, uint8* pYPtr, uint8* pUPtr, uint8* pVPtr,
							uint32 nSrcWidth, uint32 nSrcHeight, uint32 nDstWidth, uint32 nDstHeight,
							uint32 nRGBStride, uint32 nYStride, uint32 nUVStride )
{
	
	uint32 nXStep = ( nDstWidth << 16 ) / nSrcWidth;
	uint32 nYStep = ( nDstHeight << 16 ) / nSrcHeight;
	uint32 nCurrentY = 0;
	uint32 nDstY = 0;
	
	for( uint32 y = 0; y < nSrcHeight; y++ )
	{
		nCurrentY += nYStep;

		while( nCurrentY >= 0x10000 )
		{
		uint32* pDst = (uint32*)pDstPtr;		
		uint8* pY = pYPtr + nYStride * y;
		uint8* pU = pUPtr + nUVStride * ( y / 2 );
		uint8* pV = pVPtr + nUVStride * ( y / 2 );
		
		uint32 nCurrentX = 0;
		
		
		for( uint32 x = 0; x < nSrcWidth; x+= 8 )
		{
			/* Load data for 8 pixels */
			movq_m2r( *pY, mm6 ); /* Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 */
			movd_m2r( *pU, mm0 ); /*  0  0  0  0 U3 U2 U1 U0 */
			movd_m2r( *pV, mm1 ); /*  0  0  0  0 V3 V2 V1 V0 */
			
			pxor_r2r( mm4, mm4 ); /* 0 0 0 0 0 0 0 0 */
			
			/* Convert the chroma part */
			punpcklbw_r2r( mm4, mm0 ); /* 0 U3 0 U2 0 U1 0 U0 */
			punpcklbw_r2r( mm4, mm1 ); /* 0 V3 0 V2 0 V1 0 V0 */
			
			psubsw_m2r( g_anMMX80W, mm0 ); /* mm0 -= 128 */
			psubsw_m2r( g_anMMX80W, mm1 ); /* mm1 -= 128 */
			
			psllw_i2r( 3, mm0 ); /* mm0 << 3 */
			psllw_i2r( 3, mm1 ); /* mm1 << 3 */
			
			movq_r2r( mm0, mm2 ); /* mm0 -> mm2 */
			movq_r2r( mm1, mm3 ); /* mm1 -> mm3 */
			
			pmulhw_m2r( g_anUGreen, mm2 );
			pmulhw_m2r( g_anVGreen, mm3 );
			
			pmulhw_m2r( g_anUBlue, mm0 );
			pmulhw_m2r( g_anVRed, mm1 );
			
			paddsw_r2r( mm3, mm2 );
			
			/* Convert the luma part */
			psubusb_m2r( g_anMMX10W, mm6 ); /* mm6 -= 16 */
			movq_r2r( mm6, mm7 ); /* mm6 -> mm7 */
			pand_m2r( g_anMMX00FFW, mm6 ); /* 0 Y6 0 Y4 0 Y2 0 Y0 */
			psrlw_i2r( 8, mm7 ); /* 0 Y7 0 Y5 0 Y3 0 Y1 */
			
			psllw_i2r( 3, mm6 ); /* mm6 << 3 */
			psllw_i2r( 3, mm7 ); /* mm7 << 3 */
			
			pmulhw_m2r( g_anMMXYCoeff, mm6 );
			pmulhw_m2r( g_anMMXYCoeff, mm7 );
			
			/* Create the final RGB values */
			movq_r2r( mm0, mm3 ); /* mm0 -> mm3 */
			movq_r2r( mm1, mm4 ); /* mm1 -> mm4 */
			movq_r2r( mm2, mm5 ); /* mm2 -> mm5 */
			
			paddsw_r2r( mm6, mm0 ); /* Y + Blue */
			paddsw_r2r( mm7, mm3 ); /* Y + Blue */

			paddsw_r2r( mm6, mm1 ); /* Y + Red */
			paddsw_r2r( mm7, mm4 ); /* Y + Red */
			
			paddsw_r2r( mm6, mm2 ); /* Y + Green */
			paddsw_r2r( mm7, mm5 ); /* Y + Green */
			
			/* Pack RGB components to bytes */
			packuswb_r2r( mm0, mm0 ); /* B6 B4 B2 B0 B6 B4 B2 B0 */
			packuswb_r2r( mm1, mm1 ); /* R6 R4 R2 R0 R6 R4 R2 R0 */
			packuswb_r2r( mm2, mm2 ); /* G6 G4 G2 G0 G6 G4 G2 G0 */
			packuswb_r2r( mm3, mm3 ); /* B7 B5 B3 B1 B7 B5 B3 B1 */
			packuswb_r2r( mm4, mm4 ); /* R7 R5 R3 R1 R7 R5 R3 R1 */
			packuswb_r2r( mm5, mm5 ); /* G7 G5 G3 G1 G7 G5 G3 G1 */
			
			/* Interleave the RGB components */
			punpcklbw_r2r( mm3, mm0 ); /* R7 R6 R5 R4 R3 R2 R1 R0 */
			punpcklbw_r2r( mm4, mm1 ); /* B7 B6 B5 B4 B3 B2 B1 B0 */
			punpcklbw_r2r( mm5, mm2 ); /* G7 G6 G5 G4 G3 G2 G1 G0 */
			
			
			/* Build RGB values */
			pxor_r2r( mm3, mm3 );
			
			movq_r2r( mm0, mm6 ); /* mm0 -> mm6 */
			movq_r2r( mm1, mm7 ); /* mm1 -> mm7 */
			movq_r2r( mm0, mm4 ); /* mm0 -> mm4 */
			movq_r2r( mm1, mm5 ); /* mm1 -> mm5 */
			
			punpcklbw_r2r( mm2, mm6 );
			punpcklbw_r2r( mm3, mm7 );
			punpcklwd_r2r( mm7, mm6 );
						
			nCurrentX += nXStep;
			while( nCurrentX >= 0x10000 )
			{
				movd_r2m( mm6, *pDst );
				pDst++;
				nCurrentX -= 0x10000;
			}
			
			psrlq_i2r( 32, mm6 );
			
			nCurrentX += nXStep;
			while( nCurrentX >= 0x10000 )
			{
				movd_r2m( mm6, *pDst );
				pDst++;
				nCurrentX -= 0x10000;
			}

			movq_r2r( mm0, mm6 );
			punpcklbw_r2r( mm2, mm6 );
			punpckhwd_r2r( mm7, mm6 );
			
			nCurrentX += nXStep;
			while( nCurrentX >= 0x10000 )
			{
				movd_r2m( mm6, *pDst );
				pDst++;
				nCurrentX -= 0x10000;
			}

			psrlq_i2r( 32, mm6 );
			
			nCurrentX += nXStep;
			while( nCurrentX >= 0x10000 )
			{
				movd_r2m( mm6, *pDst );
				pDst++;
				nCurrentX -= 0x10000;
			}
			
			punpckhbw_r2r( mm2, mm4 );
			punpckhbw_r2r( mm3, mm5 );
			punpcklwd_r2r( mm5, mm4 );
	
			nCurrentX += nXStep;
			while( nCurrentX >= 0x10000 )
			{
				movd_r2m( mm4, *pDst );
				pDst++;
				nCurrentX -= 0x10000;
			}
			
			psrlq_i2r( 32, mm4 );
			
			nCurrentX += nXStep;
			while( nCurrentX >= 0x10000 )
			{
				movd_r2m( mm4, *pDst );
				pDst++;
				nCurrentX -= 0x10000;
			}
	
			movq_r2r( mm0, mm4 );
			punpckhbw_r2r( mm2, mm4 );
			punpckhwd_r2r( mm5, mm4 );

			nCurrentX += nXStep;
			while( nCurrentX >= 0x10000 )
			{
				movd_r2m( mm4, *pDst );
				pDst++;
				nCurrentX -= 0x10000;
			}
			
			psrlq_i2r( 32, mm4 );
			
			nCurrentX += nXStep;
			while( nCurrentX >= 0x10000 )
			{
				movd_r2m( mm4, *pDst );
				pDst++;
				nCurrentX -= 0x10000;
			}

			pY += 8;
			pU += 4;
			pV += 4;
		}
		nCurrentY -= 0x10000;
		pDstPtr += nRGBStride;
		nDstY++;
		}
	}
	
	while( nDstY < nDstHeight )
	{
		memset( pDstPtr, 0, nDstWidth * 4 );
		nDstY++;
	}
	
	emms();
}

status_t ScreenOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psNewFrame )
{
	/* Show video frame */
	if( !m_bUseOverlay )
	{
		/* No overlay */
		//bigtime_t nTime = get_system_time();		
		
		bool bUseView = true;
		bool bUnlockFB = false;
		os::WR_LockFbReply_s sReply;
		
		if( m_bUseMMX && m_bUseFB && m_pcView->GetWindow() != NULL )
		{
			/* Try to lock the framebuffer */
			os::WR_LockFb_s sReq;

			sReq.m_hReply = m_hReplyPort;
			if( send_msg( m_pcView->GetWindow()->_GetAppserverPort(), os::WR_LOCK_FB, &sReq, sizeof( sReq ) ) == 0 )
			{
				if( get_msg( m_hReplyPort, NULL, &sReply, sizeof( sReply ) ) < 0 )
				{
					dbprintf( "Error: Appserver did not respond to WR_LOCK_FB request\n" );
					bUseView = true;
				}
				else
				{
					if( sReply.m_bSuccess )
					{
						bUnlockFB = true;
						if( sReply.m_eColorSpc == os::CS_RGB32 || sReply.m_eColorSpc == os::CS_RGBA32 )
							bUseView = false;
					}
				}
			}
		}
		
		if( bUseView )
		{
			if( bUnlockFB )
			{
				/* Unlock the framebuffer. Without this call the system will lock up! */
				unlock_semaphore( sReply.m_hSem );			
			}					
			/* Convert the YUV video frame to RGB and then draw it using the appserver */
			yuv2rgb( m_pcView->GetRaster(), psNewFrame->pBuffer[0], psNewFrame->pBuffer[1], psNewFrame->pBuffer[2],
				m_sFormat.nWidth, m_sFormat.nHeight, m_sFormat.nWidth * BitsPerPixel( m_eColorSpace ) / 8, psNewFrame->nSize[0],
				psNewFrame->nSize[1] );
			
			m_pcView->SetBitmapCleared( false );
			m_pcView->Paint( m_pcView->GetBounds() );
			m_pcView->Sync();
		}
		else
		{
			/* Convert the YUV video frame to RGB, scale it and write it directly to the
			   framebuffer */
			os::Rect cViewFrame = m_pcView->ConvertToScreen( m_pcView->GetBounds() );			

			cViewFrame &= sReply.m_cFrame;
			
			uint8* pFB = (uint8*)m_cDesktop.GetFrameBuffer();
			pFB += sReply.m_nBytesPerLine * (int)cViewFrame.top;
			pFB += 4 * (int)cViewFrame.left;
			

			yv12torgb32_scaled( pFB, psNewFrame->pBuffer[0], psNewFrame->pBuffer[1], psNewFrame->pBuffer[2],
				m_sFormat.nWidth, m_sFormat.nHeight, (int)( cViewFrame.Width() + 1 ), (int)(cViewFrame.Height() + 1 ), sReply.m_nBytesPerLine, psNewFrame->nSize[0],
				psNewFrame->nSize[1] );
			m_pcView->SetBitmapCleared( true );				
			if( bUnlockFB )
			{
				/* Unlock the framebuffer. Without this call the system will lock up! */
				unlock_semaphore( sReply.m_hSem );			
			}		
		}
		//printf( "%i\n", (int)( get_system_time() - nTime ) );
	}
	else if( m_eColorSpace == os::CS_YUV12 )
	{
		/* YV12 overlay */
		uint32 nDstPitch = ( ( m_sFormat.nWidth ) + 0x1ff ) & ~0x1ff;
		for( int i = 0; i < m_sFormat.nHeight; i++ )
		{
			mmx_memcpy( m_pcView->GetRaster() + i * nDstPitch, psNewFrame->pBuffer[0] + psNewFrame->nSize[0] * i, m_sFormat.nWidth );
		}
		for( int i = 0; i < m_sFormat.nHeight / 2; i++ )
		{
			mmx_memcpy( m_pcView->GetRaster() + m_sFormat.nHeight * nDstPitch * 5 / 4 + nDstPitch * i / 2, 
				psNewFrame->pBuffer[1] + psNewFrame->nSize[1] * i, m_sFormat.nWidth / 2 );
		}
		for( int i = 0; i < m_sFormat.nHeight / 2; i++ )
		{
			mmx_memcpy( m_pcView->GetRaster() +  m_sFormat.nHeight * nDstPitch + nDstPitch * i / 2, 
						psNewFrame->pBuffer[2] + psNewFrame->nSize[2] * i, m_sFormat.nWidth / 2 );
		}
		
		if( m_bUseMMX )
			__asm __volatile( "emms":::"memory" );
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











































































































