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

/** Create the overlay.
 * \param	cFrame Size of the frame.
 * \param	cTitle Title of the view.
 * \param	nResizeMask CF_FOLLOW_* parameters.
 * \param	cSrcRect Size of the source data.
 * \param	eFormat  Format of the overlay.
 * \param   sColorKey ColorKey color.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
VideoOverlayView::VideoOverlayView( const Rect& cFrame, const std::string& cTitle, uint32 nResizeMask, const IPoint& cSrcSize, color_space eFormat,
    				Color32_s sColorKey ) : View( cFrame, cTitle, nResizeMask )
{
	m_sOverlay.m_pAddress = NULL;
	m_sOverlay.m_hArea = -1;
	m_sOverlay.m_eFormat = eFormat;
	m_sOverlay.m_nSrcWidth = cSrcSize.x;
	m_sOverlay.m_nSrcHeight = cSrcSize.y;
	m_sOverlay.m_sColorKey = sColorKey;
}


VideoOverlayView::~VideoOverlayView()
{
	if( m_sOverlay.m_hArea > 0 )
		delete_area( m_sOverlay.m_hArea );
}

/** Update the overlay content on the screen.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void VideoOverlayView::Update()
{
	/* Check if the application object has been created */
	if( Application::GetInstance() == NULL ) {
		dbprintf("VideoOverlay::VideoOverlay() called without valid application object\n");
	}
	
	/* Check if the position of the overlay has changed */
	if( ConvertToScreen( GetFrame() ) != m_cCurrentFrame ) 
	{
		m_cCurrentFrame = ConvertToScreen( GetFrame() );
		Recreate( IPoint( m_sOverlay.m_nSrcWidth, m_sOverlay.m_nSrcHeight ), 
		Rect( m_cCurrentFrame.left, m_cCurrentFrame.top, m_cCurrentFrame.right + 1, m_cCurrentFrame.bottom + 1 ) );
	}
	
	/* Send update message */
	Message cReq( WR_UPDATE_VIDEO_OVERLAY );
	
	cReq.AddInt32( "area", m_sOverlay.m_hPhysArea );
	Application *pcApp = Application::GetInstance();
	Messenger( pcApp->GetAppPort() ).SendMessage( &cReq );
}

void VideoOverlayView::AttachedToWindow()
{
	/* Save information */
	m_cCurrentFrame = ConvertToScreen( GetFrame() );
	m_sOverlay.m_nDstX = (int)m_cCurrentFrame.left;
	m_sOverlay.m_nDstY = (int)m_cCurrentFrame.top;
	m_sOverlay.m_nDstWidth = (int)m_cCurrentFrame.Width();
	m_sOverlay.m_nDstHeight = (int)m_cCurrentFrame.Height();
	m_sOverlay.m_pAddress = NULL;
	m_sOverlay.m_hArea = -1;
	
	/* Check if the application object has been created */
	if( Application::GetInstance() == NULL ) {
		dbprintf("VideoOverlay::VideoOverlay() called without valid application object\n");
	}
	
	/* Send message */
	Message cReq( WR_CREATE_VIDEO_OVERLAY );
	Message cReply;
	
	int32 nFormat = static_cast<int32>(m_sOverlay.m_eFormat);
	
	cReq.AddIPoint( "size", IPoint( m_sOverlay.m_nSrcWidth, m_sOverlay.m_nSrcHeight ) );
	cReq.AddRect( "dst", IRect( m_sOverlay.m_nDstX, m_sOverlay.m_nDstY, 
						m_sOverlay.m_nDstX + m_sOverlay.m_nDstWidth + 1, m_sOverlay.m_nDstY + m_sOverlay.m_nDstHeight + 1 ) );
	cReq.AddInt32( "format", nFormat );
	cReq.AddColor32( "color_key", m_sOverlay.m_sColorKey );
  
 	Application *pcApp = Application::GetInstance();
	Messenger( pcApp->GetAppPort() ).SendMessage( &cReq, &cReply );
    
	area_id hArea;
	int nError;
	void *pAddress = NULL;
	
	cReply.FindInt( "error", &nError );
	
	if ( nError != 0 ) {
		errno = nError;
		m_sOverlay.m_pAddress = NULL;
		return;
	}
	
	cReply.FindInt32( "area", (int32*)&hArea );
	
	m_sOverlay.m_hPhysArea = hArea;
	m_sOverlay.m_hArea = clone_area( "video_overlay", &pAddress, AREA_FULL_ACCESS, AREA_NO_LOCK, hArea );
	m_sOverlay.m_pAddress = ( uint8* )pAddress;
}

void VideoOverlayView::DetachedFromWindow()
{
	/* Check if the application object has been created */
	if( Application::GetInstance() == NULL ) {
		dbprintf("VideoOverlay::VideoOverlay() called without valid application object\n");
	}
	
	Message cReq( WR_DELETE_VIDEO_OVERLAY );
	
	if( m_sOverlay.m_hArea > 0 )
		delete_area( m_sOverlay.m_hArea );
	
	cReq.AddInt32( "area", m_sOverlay.m_hPhysArea );
	Application *pcApp = Application::GetInstance();
	Messenger( pcApp->GetAppPort() ).SendMessage( &cReq );
}

void VideoOverlayView::Paint( const Rect& cUpdateRect )
{
	/* Paint everything in the color key color */
	SetFgColor( m_sOverlay.m_sColorKey );
	FillRect( cUpdateRect );
}


void VideoOverlayView::Recreate( const IPoint& cSrcSize, const IRect& cDstRect )
{
	/* Check if the application object has been created */
	if( Application::GetInstance() == NULL ) {
		dbprintf("VideoOverlay::VideoOverlay() called without valid application object\n");
	}
	
	/* Save information */
	m_sOverlay.m_nSrcWidth = cSrcSize.x;
	m_sOverlay.m_nSrcHeight = cSrcSize.y;
	m_sOverlay.m_nDstX = cDstRect.left;
	m_sOverlay.m_nDstY = cDstRect.top;
	m_sOverlay.m_nDstWidth = cDstRect.Width();
	m_sOverlay.m_nDstHeight = cDstRect.Height();
	m_sOverlay.m_pAddress = NULL;
	
	/* Send message */
	Message cReq( WR_RECREATE_VIDEO_OVERLAY );
	Message cReply;
	
	if( m_sOverlay.m_hArea > 0 )
		delete_area( m_sOverlay.m_hArea );
	
	int32 nFormat = static_cast<int32>(m_sOverlay.m_eFormat);
	
	cReq.AddIPoint( "size", IPoint( m_sOverlay.m_nSrcWidth, m_sOverlay.m_nSrcHeight ) );
	cReq.AddRect( "dst", IRect( m_sOverlay.m_nDstX, m_sOverlay.m_nDstY, 
						m_sOverlay.m_nDstX + m_sOverlay.m_nDstWidth, m_sOverlay.m_nDstY + m_sOverlay.m_nDstHeight ) );
	cReq.AddInt32( "format", nFormat );
	cReq.AddInt32( "area", m_sOverlay.m_hPhysArea );
  
	Application *pcApp = Application::GetInstance();
	Messenger( pcApp->GetAppPort() ).SendMessage( &cReq, &cReply );
    
	
	int nError;
	void *pAddress = NULL;
	area_id hArea;
	
	cReply.FindInt( "error", &nError );
	
	if ( nError != 0 ) {
		errno = nError;
		m_sOverlay.m_pAddress = NULL;
		return;
	}
	
	cReply.FindInt32( "area", (int32*)&hArea );
	
	m_sOverlay.m_hPhysArea = hArea;
	m_sOverlay.m_hArea = clone_area( "video_overlay", &pAddress, AREA_FULL_ACCESS, AREA_NO_LOCK, hArea );
	m_sOverlay.m_pAddress = ( uint8* )pAddress;
}
