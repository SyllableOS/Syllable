/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

#include <stdio.h>
#include <atheos/kernel.h>
#include <atheos/areas.h>

#include <appserver/protocol.h>
#include <gui/desktop.h>
#include <gui/guidefines.h>
#include <util/application.h>
#include <util/message.h>

using namespace os;

Desktop::Desktop( int nDesktop )
{
    m_psScreenMode = new screen_mode;
    m_hServerAreaID = -1;

    Message cDesktopParams;
    
    Application::GetInstance()->LockDesktop( nDesktop, &cDesktopParams );

    int32  nColorSpace;

    m_pFrameBuffer = NULL;
    m_hServerAreaID = -1;
    m_hLocalAreaID = -1;

    IPoint cResolution;
    
    cDesktopParams.FindInt( "desktop", &m_nDesktop );
    cDesktopParams.FindIPoint( "resolution", &cResolution );
    cDesktopParams.FindInt32( "color_space", &nColorSpace );
    cDesktopParams.FindFloat( "refresh_rate", &m_psScreenMode->m_vRefreshRate );
    cDesktopParams.FindFloat( "h_pos", &m_psScreenMode->m_vHPos );
    cDesktopParams.FindFloat( "v_pos", &m_psScreenMode->m_vVPos );
    cDesktopParams.FindFloat( "h_size", &m_psScreenMode->m_vHSize );
    cDesktopParams.FindFloat( "v_size", &m_psScreenMode->m_vVSize );
    
    m_psScreenMode->m_nWidth  = cResolution.x;
    m_psScreenMode->m_nHeight = cResolution.y;
    m_psScreenMode->m_eColorSpace = (color_space)nColorSpace;
    cDesktopParams.FindInt( "screen_area", &m_hServerAreaID );
}

Desktop::~Desktop()
{
    delete m_psScreenMode;
//    Application::GetInstance()->UnlockDesktop( m_nCookie );
    if ( m_hLocalAreaID != -1 ) {
	delete_area( m_hLocalAreaID );
    }
}

screen_mode Desktop::GetScreenMode() const
{
    return( *m_psScreenMode );
}

IPoint Desktop::GetResolution() const
{
    return( IPoint( m_psScreenMode->m_nWidth, m_psScreenMode->m_nHeight ) );
}

color_space Desktop::GetColorSpace() const
{
    return( m_psScreenMode->m_eColorSpace );
}

void* Desktop::GetFrameBuffer()
{
    if ( m_pFrameBuffer != NULL ) {
	return( m_pFrameBuffer );
    }
    if ( m_hServerAreaID != -1 ) {
	m_hLocalAreaID = clone_area( "fb_clone", (void**) &m_pFrameBuffer, AREA_FULL_ACCESS, AREA_NO_LOCK, m_hServerAreaID );
    }
    return( m_pFrameBuffer );
}

bool Desktop::SetScreenMode( screen_mode* psMode )
{
    m_psScreenMode->m_nWidth = psMode->m_nWidth;
    m_psScreenMode->m_nHeight = psMode->m_nHeight;
    m_psScreenMode->m_eColorSpace = psMode->m_eColorSpace;
    m_psScreenMode->m_vRefreshRate = psMode->m_vRefreshRate;
    m_psScreenMode->m_vHPos  = psMode->m_vHPos;
    m_psScreenMode->m_vVPos  = psMode->m_vVPos;
    m_psScreenMode->m_vHSize = psMode->m_vHSize;
    m_psScreenMode->m_vVSize = psMode->m_vVSize;
    Application::GetInstance()->SetScreenMode( m_nDesktop, SCRMF_RES | SCRMF_COLORSPACE | SCRMF_REFRESH | SCRMF_POS | SCRMF_SIZE,
					       m_psScreenMode );
    return( true );
}

bool Desktop::SetResoulution( int nWidth, int nHeight )
{
    m_psScreenMode->m_nWidth = nWidth;
    m_psScreenMode->m_nHeight = nHeight;
    
    Application::GetInstance()->SetScreenMode( m_nDesktop, SCRMF_RES, m_psScreenMode );
    return( true );
}

bool Desktop::SetColorSpace( color_space eColorSpace )
{
    m_psScreenMode->m_eColorSpace = eColorSpace;
    Application::GetInstance()->SetScreenMode( m_nDesktop, SCRMF_COLORSPACE, m_psScreenMode );
    return( true );
}

bool Desktop::SetRefreshRate( float vRefreshRate )
{
    m_psScreenMode->m_vRefreshRate = vRefreshRate;
    Application::GetInstance()->SetScreenMode( m_nDesktop, SCRMF_REFRESH, m_psScreenMode );
    return( true );
}
