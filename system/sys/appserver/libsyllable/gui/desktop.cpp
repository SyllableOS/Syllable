
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
#include <gui/bitmap.h>
#include <util/application.h>
#include <util/message.h>
#include <util/messenger.h>

using namespace os;

class Desktop::Private
{
public:
	int		 m_nCookie;
    int		 m_nDesktop;
    screen_mode* m_psScreenMode;
    void*	 m_pFrameBuffer;
    uint32	 unused[6];

    area_id	m_hServerAreaID;
    area_id	m_hLocalAreaID;
};


Desktop::Desktop( int nDesktop )
{
	m = new Private;
	
	m->m_psScreenMode = new screen_mode;
	m->m_hServerAreaID = -1;

	Message cDesktopParams;

	Application::GetInstance()->LockDesktop( nDesktop, &cDesktopParams );

	int32 nColorSpace;

	m->m_pFrameBuffer = NULL;
	m->m_hServerAreaID = -1;
	m->m_hLocalAreaID = -1;

	IPoint cResolution;

	cDesktopParams.FindInt( "desktop", &m->m_nDesktop );
	cDesktopParams.FindIPoint( "resolution", &cResolution );
	cDesktopParams.FindInt( "bytes_per_line", &m->m_psScreenMode->m_nBytesPerLine );
	cDesktopParams.FindInt32( "color_space", &nColorSpace );
	cDesktopParams.FindFloat( "refresh_rate", &m->m_psScreenMode->m_vRefreshRate );
	cDesktopParams.FindFloat( "h_pos", &m->m_psScreenMode->m_vHPos );
	cDesktopParams.FindFloat( "v_pos", &m->m_psScreenMode->m_vVPos );
	cDesktopParams.FindFloat( "h_size", &m->m_psScreenMode->m_vHSize );
	cDesktopParams.FindFloat( "v_size", &m->m_psScreenMode->m_vVSize );

	m->m_psScreenMode->m_nWidth = cResolution.x;
	m->m_psScreenMode->m_nHeight = cResolution.y;
	m->m_psScreenMode->m_eColorSpace = ( color_space ) nColorSpace;
	cDesktopParams.FindInt( "screen_area", &m->m_hServerAreaID );
}

Desktop::~Desktop()
{
	delete m->m_psScreenMode;

//    Application::GetInstance()->UnlockDesktop( m_nCookie );
	if( m->m_hLocalAreaID != -1 )
	{
		delete_area( m->m_hLocalAreaID );
	}
	delete m;
}



/** Return the desktop number.
 * \par Description:
 * Returns the number of the desktop this object is set to.
 * \return Desktop number.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
int Desktop::GetDesktop() const
{
	return( m->m_nDesktop );
}


screen_mode Desktop::GetScreenMode() const
{
	return ( *m->m_psScreenMode );
}

IPoint Desktop::GetResolution() const
{
	return ( IPoint( m->m_psScreenMode->m_nWidth, m->m_psScreenMode->m_nHeight ) );
}

color_space Desktop::GetColorSpace() const
{
	return ( m->m_psScreenMode->m_eColorSpace );
}

void *Desktop::GetFrameBuffer()
{
	if( m->m_pFrameBuffer != NULL )
	{
		return ( m->m_pFrameBuffer );
	}
	if( m->m_hServerAreaID != -1 )
	{
		m->m_hLocalAreaID = clone_area( "fb_clone", ( void ** )&m->m_pFrameBuffer, AREA_FULL_ACCESS, AREA_NO_LOCK, m->m_hServerAreaID );
	}
	return ( m->m_pFrameBuffer );
}

bool Desktop::SetScreenMode( screen_mode * psMode )
{
	m->m_psScreenMode->m_nWidth = psMode->m_nWidth;
	m->m_psScreenMode->m_nHeight = psMode->m_nHeight;
	m->m_psScreenMode->m_eColorSpace = psMode->m_eColorSpace;
	m->m_psScreenMode->m_vRefreshRate = psMode->m_vRefreshRate;
	m->m_psScreenMode->m_vHPos = psMode->m_vHPos;
	m->m_psScreenMode->m_vVPos = psMode->m_vVPos;
	m->m_psScreenMode->m_vHSize = psMode->m_vHSize;
	m->m_psScreenMode->m_vVSize = psMode->m_vVSize;
	Application::GetInstance()->SetScreenMode( m->m_nDesktop, 0, m->m_psScreenMode );
	return ( true );
}

bool Desktop::SetResolution( int nWidth, int nHeight )
{
	m->m_psScreenMode->m_nWidth = nWidth;
	m->m_psScreenMode->m_nHeight = nHeight;

	Application::GetInstance()->SetScreenMode( m->m_nDesktop, 0, m->m_psScreenMode );
	return ( true );
}

bool Desktop::SetColorSpace( color_space eColorSpace )
{
	m->m_psScreenMode->m_eColorSpace = eColorSpace;
	Application::GetInstance()->SetScreenMode( m->m_nDesktop, 0, m->m_psScreenMode );
	return ( true );
}

bool Desktop::SetRefreshRate( float vRefreshRate )
{
	m->m_psScreenMode->m_vRefreshRate = vRefreshRate;
	Application::GetInstance()->SetScreenMode( m->m_nDesktop, 0, m->m_psScreenMode );
	return ( true );
}

bool Desktop::Activate() const
{
	Application::GetInstance()->SwitchDesktop( m->m_nDesktop );
	return ( true );
}



/** Minimizes all windows on the desktop(besides the dock and the desktop)
 * \par Description:
 * 
 * \param
 * \return Number of windows minimized
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/
int Desktop::MinimizeAll() const
{
	Message cReq( DR_MINIMIZE_ALL );
	Message cReply;
	Application* pcApp = Application::GetInstance();
	cReq.AddInt32( "desktop", m->m_nDesktop );
	Messenger( pcApp->GetServerPort() ).SendMessage( &cReq, &cReply );
	
	int nMinimized;
	cReply.FindInt32("minimized",&nMinimized);
	return (nMinimized);
}
