
/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <malloc.h>

#include <atheos/kernel.h>

#include <gui/bitmap.h>

#include "bitmap.h"
#include "ddriver.h"
#include "server.h"

using namespace os;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SrvBitmap::SrvBitmap( int nWidth, int nHeight, color_space eColorSpc, uint8 *pRaster, int nBytesPerLine )
{
	m_nFlags = 0;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_eColorSpc = eColorSpc;
	m_pcDriver = NULL;
	m_bVideoMem = false;
	m_nVideoMemOffset = false;
	m_hArea = -1;

	if( nBytesPerLine != 0 )
	{
		m_nBytesPerLine = nBytesPerLine;
	}
	else
	{
		int nBitsPerPixel = BitsPerPixel( eColorSpc );

		if( nBitsPerPixel == 15 )
		{
			nBitsPerPixel = 16;
		}
		m_nBytesPerLine = ( nWidth * nBitsPerPixel ) / 8;
	}
	if( m_nWidth == 0 || pRaster != NULL )
	{
		m_bFreeRaster = false;
		m_pRaster = pRaster;
	}
	else
	{
		m_bFreeRaster = true;
		m_pRaster = ( uint8 * )calloc( m_nHeight, m_nBytesPerLine );
	}
	m_hHandle = g_pcBitmaps->Insert( this );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SrvBitmap::~SrvBitmap()
{
	g_pcBitmaps->Remove( m_hHandle );
	if( m_bFreeRaster )
	{
		if( m_bVideoMem )
			g_pcDispDrv->FreeMemory( m_nVideoMemOffset );
		else
			delete[]m_pRaster;
	}
	if( m_hArea >= 0 )
	{
		delete_area( m_hArea );
	}
}

