/*  Video Overlay class
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

#include <malloc.h>
#include <appserver/protocol.h>
#include <util/application.h>
#include <util/message.h>
#include <util/messenger.h>
#include <gui/videooverlay.h>

using namespace os;

class VideoOverlayView::Private
{
	public:
    video_overlay 	m_sOverlay;
    Rect			m_cCurrentFrame;
};

/** Create the overlay.
 * \param	cFrame Size of the frame.
 * \param	cTitle Title of the view.
 * \param	nResizeMask CF_FOLLOW_* parameters.
 * \param	cSrcRect Size of the source data.
 * \param	eFormat  Format of the overlay.
 * \param   sColorKey ColorKey color.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
VideoOverlayView::VideoOverlayView( const Rect & cFrame, const String & cTitle, uint32 nResizeMask, const IPoint & cSrcSize, color_space eFormat, Color32_s sColorKey ):View( cFrame, cTitle, nResizeMask )
{
	m = new Private;
	m->m_sOverlay.m_pAddress = NULL;
	m->m_sOverlay.m_hArea = -1;
	m->m_sOverlay.m_eFormat = eFormat;
	m->m_sOverlay.m_nSrcWidth = cSrcSize.x;
	m->m_sOverlay.m_nSrcHeight = cSrcSize.y;
	m->m_sOverlay.m_sColorKey = sColorKey;
}


VideoOverlayView::~VideoOverlayView()
{
	if( m->m_sOverlay.m_hArea > 0 )
		delete_area( m->m_sOverlay.m_hArea );
	delete m;
}

/** Return the current memory address of the overlay.
* \author Arno Klenke (arno_klenke@yahoo.de)
*****************************************************************************/
uint8* VideoOverlayView::GetRaster() const
{
	return( m->m_sOverlay.m_pAddress );
}
	
/** Return the current physical memory area of the overlay.
* \author Arno Klenke (arno_klenke@yahoo.de)
*****************************************************************************/
area_id VideoOverlayView::GetPhysicalArea() const
{
	return( m->m_sOverlay.m_hPhysArea );
}

/** Update the overlay content on the screen.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void VideoOverlayView::Update()
{
	/* Check if the application object has been created */
	if( Application::GetInstance() == NULL )
	{
		dbprintf( "VideoOverlay::VideoOverlay() called without valid application object\n" );
	}

	/* Check if the position of the overlay has changed */
	if( ConvertToScreen( GetBounds() ) != m->m_cCurrentFrame )
	{
		m->m_cCurrentFrame = ConvertToScreen( GetBounds() );
		Recreate( IPoint( m->m_sOverlay.m_nSrcWidth, m->m_sOverlay.m_nSrcHeight ), Rect( m->m_cCurrentFrame.left, m->m_cCurrentFrame.top, m->m_cCurrentFrame.right + 1, m->m_cCurrentFrame.bottom + 1 ) );
	}

	/* Send update message */
	Message cReq( WR_UPDATE_VIDEO_OVERLAY );

	cReq.AddInt32( "area", m->m_sOverlay.m_hPhysArea );
	Application *pcApp = Application::GetInstance();

	Messenger( pcApp->GetAppPort() ).SendMessage( &cReq );
}

void VideoOverlayView::AttachedToWindow()
{
	/* Save information */
	m->m_cCurrentFrame = ConvertToScreen( GetBounds() );
	m->m_sOverlay.m_nDstX = ( int )m->m_cCurrentFrame.left;
	m->m_sOverlay.m_nDstY = ( int )m->m_cCurrentFrame.top;
	m->m_sOverlay.m_nDstWidth = ( int )m->m_cCurrentFrame.Width();
	m->m_sOverlay.m_nDstHeight = ( int )m->m_cCurrentFrame.Height();
	m->m_sOverlay.m_pAddress = NULL;
	m->m_sOverlay.m_hArea = -1;

	/* Check if the application object has been created */
	if( Application::GetInstance() == NULL )
	{
		dbprintf( "VideoOverlay::VideoOverlay() called without valid application object\n" );
	}

	/* Send message */
	Message cReq( WR_CREATE_VIDEO_OVERLAY );
	Message cReply;

	int32 nFormat = static_cast < int32 >( m->m_sOverlay.m_eFormat );

	cReq.AddIPoint( "size", IPoint( m->m_sOverlay.m_nSrcWidth, m->m_sOverlay.m_nSrcHeight ) );
	cReq.AddRect( "dst", IRect( m->m_sOverlay.m_nDstX, m->m_sOverlay.m_nDstY, m->m_sOverlay.m_nDstX + m->m_sOverlay.m_nDstWidth + 1, m->m_sOverlay.m_nDstY + m->m_sOverlay.m_nDstHeight + 1 ) );
	cReq.AddInt32( "format", nFormat );
	cReq.AddColor32( "color_key", m->m_sOverlay.m_sColorKey );

	Application *pcApp = Application::GetInstance();

	Messenger( pcApp->GetAppPort() ).SendMessage( &cReq, &cReply );

	area_id hArea;
	int nError;
	void *pAddress = NULL;

	cReply.FindInt( "error", &nError );

	if( nError != 0 )
	{
		errno = nError;
		m->m_sOverlay.m_pAddress = NULL;
		return;
	}

	cReply.FindInt32( "area", ( int32 * )&hArea );

	m->m_sOverlay.m_hPhysArea = hArea;
	m->m_sOverlay.m_hArea = clone_area( "video_overlay", &pAddress, AREA_FULL_ACCESS, AREA_NO_LOCK, hArea );
	m->m_sOverlay.m_pAddress = ( uint8 * )pAddress;
}

void VideoOverlayView::DetachedFromWindow()
{
	/* Check if the application object has been created */
	if( Application::GetInstance() == NULL )
	{
		dbprintf( "VideoOverlay::VideoOverlay() called without valid application object\n" );
	}

	Message cReq( WR_DELETE_VIDEO_OVERLAY );

	if( m->m_sOverlay.m_hArea > 0 )
		delete_area( m->m_sOverlay.m_hArea );
	m->m_sOverlay.m_hArea = -1;
		
	cReq.AddInt32( "area", m->m_sOverlay.m_hPhysArea );
	Application *pcApp = Application::GetInstance();

	Messenger( pcApp->GetAppPort() ).SendMessage( &cReq );
}

void VideoOverlayView::Paint( const Rect & cUpdateRect )
{
	/* Paint everything in the color key color */
	SetFgColor( m->m_sOverlay.m_sColorKey );
	FillRect( cUpdateRect );
}


void VideoOverlayView::Recreate( const IPoint & cSrcSize, const IRect & cDstRect )
{
	/* Check if the application object has been created */
	if( Application::GetInstance() == NULL )
	{
		dbprintf( "VideoOverlay::VideoOverlay() called without valid application object\n" );
	}

	/* Save information */
	m->m_sOverlay.m_nSrcWidth = cSrcSize.x;
	m->m_sOverlay.m_nSrcHeight = cSrcSize.y;
	m->m_sOverlay.m_nDstX = cDstRect.left;
	m->m_sOverlay.m_nDstY = cDstRect.top;
	m->m_sOverlay.m_nDstWidth = cDstRect.Width();
	m->m_sOverlay.m_nDstHeight = cDstRect.Height();
	m->m_sOverlay.m_pAddress = NULL;

	/* Send message */
	Message cReq( WR_RECREATE_VIDEO_OVERLAY );
	Message cReply;

	if( m->m_sOverlay.m_hArea > 0 )
		delete_area( m->m_sOverlay.m_hArea );
	m->m_sOverlay.m_hArea = -1;

	int32 nFormat = static_cast < int32 >( m->m_sOverlay.m_eFormat );

	cReq.AddIPoint( "size", IPoint( m->m_sOverlay.m_nSrcWidth, m->m_sOverlay.m_nSrcHeight ) );
	cReq.AddRect( "dst", IRect( m->m_sOverlay.m_nDstX, m->m_sOverlay.m_nDstY, m->m_sOverlay.m_nDstX + m->m_sOverlay.m_nDstWidth, m->m_sOverlay.m_nDstY + m->m_sOverlay.m_nDstHeight ) );
	cReq.AddInt32( "format", nFormat );
	cReq.AddInt32( "area", m->m_sOverlay.m_hPhysArea );

	Application *pcApp = Application::GetInstance();

	Messenger( pcApp->GetAppPort() ).SendMessage( &cReq, &cReply );


	int nError;
	void *pAddress = NULL;
	area_id hArea;

	cReply.FindInt( "error", &nError );

	if( nError != 0 )
	{
		errno = nError;
		m->m_sOverlay.m_pAddress = NULL;
		return;
	}

	cReply.FindInt32( "area", ( int32 * )&hArea );

	m->m_sOverlay.m_hPhysArea = hArea;
	m->m_sOverlay.m_hArea = clone_area( "video_overlay", &pAddress, AREA_FULL_ACCESS, AREA_NO_LOCK, hArea );
	m->m_sOverlay.m_pAddress = ( uint8 * )pAddress;
}

void VideoOverlayView::__VOV_reserved1__()
{
}

void VideoOverlayView::__VOV_reserved2__()
{
}

void VideoOverlayView::__VOV_reserved3__()
{
}

void VideoOverlayView::__VOV_reserved4__()
{
}

void VideoOverlayView::__VOV_reserved5__()
{
}

