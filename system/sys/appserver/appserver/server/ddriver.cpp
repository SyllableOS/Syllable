/*
 *  The Syllable application server
 *  Copyright (C) 1999 - 2000 Kurt Skauen
 *  Copyright (C) 2005 Arno Klenke
 *  Copyright (C) 1997-2004 Sam Lantinga (SDL Code)
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/kernel.h>

#include "server.h"
#include "ddriver.h"
#include "sfont.h"
#include "bitmap.h"
#include "sprite.h"



#include <gui/bitmap.h>

using namespace os;

#define RAS_OFFSET8( ptr, x, y, bpl )  (((uint8*)(ptr)) + (x) + (y) * (bpl))
#define RAS_OFFSET16( ptr, x, y, bpl ) ((uint16*)(((uint8*)(ptr)) + (x*2) + (y) * (bpl)))
#define RAS_OFFSET32( ptr, x, y, bpl ) ((uint32*)(((uint8*)(ptr)) + (x*4) + (y) * (bpl)))

#include "ddriver_mmx.cpp"

Color32_s g_asDefaultPallette[] = {
	Color32_s( 0x00, 0x00, 0x00, 0xff ),	// 0
	Color32_s( 0x08, 0x08, 0x08, 0xff ),
	Color32_s( 0x10, 0x10, 0x10, 0xff ),
	Color32_s( 0x18, 0x18, 0x18, 0xff ),
	Color32_s( 0x20, 0x20, 0x20, 0xff ),
	Color32_s( 0x28, 0x28, 0x28, 0xff ),	// 5
	Color32_s( 0x30, 0x30, 0x30, 0xff ),
	Color32_s( 0x38, 0x38, 0x38, 0xff ),
	Color32_s( 0x40, 0x40, 0x40, 0xff ),
	Color32_s( 0x48, 0x48, 0x48, 0xff ),
	Color32_s( 0x50, 0x50, 0x50, 0xff ),	// 10
	Color32_s( 0x58, 0x58, 0x58, 0xff ),
	Color32_s( 0x60, 0x60, 0x60, 0xff ),
	Color32_s( 0x68, 0x68, 0x68, 0xff ),
	Color32_s( 0x70, 0x70, 0x70, 0xff ),
	Color32_s( 0x78, 0x78, 0x78, 0xff ),	// 15
	Color32_s( 0x80, 0x80, 0x80, 0xff ),
	Color32_s( 0x88, 0x88, 0x88, 0xff ),
	Color32_s( 0x90, 0x90, 0x90, 0xff ),
	Color32_s( 0x98, 0x98, 0x98, 0xff ),
	Color32_s( 0xa0, 0xa0, 0xa0, 0xff ),	// 20
	Color32_s( 0xa8, 0xa8, 0xa8, 0xff ),
	Color32_s( 0xb0, 0xb0, 0xb0, 0xff ),
	Color32_s( 0xb8, 0xb8, 0xb8, 0xff ),
	Color32_s( 0xc0, 0xc0, 0xc0, 0xff ),
	Color32_s( 0xc8, 0xc8, 0xc8, 0xff ),	// 25
	Color32_s( 0xd0, 0xd0, 0xd0, 0xff ),
	Color32_s( 0xd9, 0xd9, 0xd9, 0xff ),
	Color32_s( 0xe2, 0xe2, 0xe2, 0xff ),
	Color32_s( 0xeb, 0xeb, 0xeb, 0xff ),
	Color32_s( 0xf5, 0xf5, 0xf5, 0xff ),	// 30
	Color32_s( 0xfe, 0xfe, 0xfe, 0xff ),
	Color32_s( 0x00, 0x00, 0xff, 0xff ),
	Color32_s( 0x00, 0x00, 0xe5, 0xff ),
	Color32_s( 0x00, 0x00, 0xcc, 0xff ),
	Color32_s( 0x00, 0x00, 0xb3, 0xff ),	// 35
	Color32_s( 0x00, 0x00, 0x9a, 0xff ),
	Color32_s( 0x00, 0x00, 0x81, 0xff ),
	Color32_s( 0x00, 0x00, 0x69, 0xff ),
	Color32_s( 0x00, 0x00, 0x50, 0xff ),
	Color32_s( 0x00, 0x00, 0x37, 0xff ),	// 40
	Color32_s( 0x00, 0x00, 0x1e, 0xff ),
	Color32_s( 0xff, 0x00, 0x00, 0xff ),
	Color32_s( 0xe4, 0x00, 0x00, 0xff ),
	Color32_s( 0xcb, 0x00, 0x00, 0xff ),
	Color32_s( 0xb2, 0x00, 0x00, 0xff ),	// 45
	Color32_s( 0x99, 0x00, 0x00, 0xff ),
	Color32_s( 0x80, 0x00, 0x00, 0xff ),
	Color32_s( 0x69, 0x00, 0x00, 0xff ),
	Color32_s( 0x50, 0x00, 0x00, 0xff ),
	Color32_s( 0x37, 0x00, 0x00, 0xff ),	// 50
	Color32_s( 0x1e, 0x00, 0x00, 0xff ),
	Color32_s( 0x00, 0xff, 0x00, 0xff ),
	Color32_s( 0x00, 0xe4, 0x00, 0xff ),
	Color32_s( 0x00, 0xcb, 0x00, 0xff ),
	Color32_s( 0x00, 0xb2, 0x00, 0xff ),	// 55
	Color32_s( 0x00, 0x99, 0x00, 0xff ),
	Color32_s( 0x00, 0x80, 0x00, 0xff ),
	Color32_s( 0x00, 0x69, 0x00, 0xff ),
	Color32_s( 0x00, 0x50, 0x00, 0xff ),
	Color32_s( 0x00, 0x37, 0x00, 0xff ),	// 60
	Color32_s( 0x00, 0x1e, 0x00, 0xff ),
	Color32_s( 0x00, 0x98, 0x33, 0xff ),
	Color32_s( 0xff, 0xff, 0xff, 0xff ),
	Color32_s( 0xcb, 0xff, 0xff, 0xff ),
	Color32_s( 0xcb, 0xff, 0xcb, 0xff ),	// 65
	Color32_s( 0xcb, 0xff, 0x98, 0xff ),
	Color32_s( 0xcb, 0xff, 0x66, 0xff ),
	Color32_s( 0xcb, 0xff, 0x33, 0xff ),
	Color32_s( 0xcb, 0xff, 0x00, 0xff ),
	Color32_s( 0x98, 0xff, 0xff, 0xff ),
	Color32_s( 0x98, 0xff, 0xcb, 0xff ),
	Color32_s( 0x98, 0xff, 0x98, 0xff ),
	Color32_s( 0x98, 0xff, 0x66, 0xff ),
	Color32_s( 0x98, 0xff, 0x33, 0xff ),
	Color32_s( 0x98, 0xff, 0x00, 0xff ),
	Color32_s( 0x66, 0xff, 0xff, 0xff ),
	Color32_s( 0x66, 0xff, 0xcb, 0xff ),
	Color32_s( 0x66, 0xff, 0x98, 0xff ),
	Color32_s( 0x66, 0xff, 0x66, 0xff ),
	Color32_s( 0x66, 0xff, 0x33, 0xff ),
	Color32_s( 0x66, 0xff, 0x00, 0xff ),
	Color32_s( 0x33, 0xff, 0xff, 0xff ),
	Color32_s( 0x33, 0xff, 0xcb, 0xff ),
	Color32_s( 0x33, 0xff, 0x98, 0xff ),
	Color32_s( 0x33, 0xff, 0x66, 0xff ),
	Color32_s( 0x33, 0xff, 0x33, 0xff ),
	Color32_s( 0x33, 0xff, 0x00, 0xff ),
	Color32_s( 0xff, 0x98, 0xff, 0xff ),
	Color32_s( 0xff, 0x98, 0xcb, 0xff ),
	Color32_s( 0xff, 0x98, 0x98, 0xff ),
	Color32_s( 0xff, 0x98, 0x66, 0xff ),
	Color32_s( 0xff, 0x98, 0x33, 0xff ),
	Color32_s( 0xff, 0x98, 0x00, 0xff ),
	Color32_s( 0x00, 0x66, 0xff, 0xff ),
	Color32_s( 0x00, 0x66, 0xcb, 0xff ),
	Color32_s( 0xcb, 0xcb, 0xff, 0xff ),
	Color32_s( 0xcb, 0xcb, 0xcb, 0xff ),
	Color32_s( 0xcb, 0xcb, 0x98, 0xff ),
	Color32_s( 0xcb, 0xcb, 0x66, 0xff ),
	Color32_s( 0xcb, 0xcb, 0x33, 0xff ),
	Color32_s( 0xcb, 0xcb, 0x00, 0xff ),
	Color32_s( 0x98, 0xcb, 0xff, 0xff ),
	Color32_s( 0x98, 0xcb, 0xcb, 0xff ),
	Color32_s( 0x98, 0xcb, 0x98, 0xff ),
	Color32_s( 0x98, 0xcb, 0x66, 0xff ),
	Color32_s( 0x98, 0xcb, 0x33, 0xff ),
	Color32_s( 0x98, 0xcb, 0x00, 0xff ),
	Color32_s( 0x66, 0xcb, 0xff, 0xff ),
	Color32_s( 0x66, 0xcb, 0xcb, 0xff ),
	Color32_s( 0x66, 0xcb, 0x98, 0xff ),
	Color32_s( 0x66, 0xcb, 0x66, 0xff ),
	Color32_s( 0x66, 0xcb, 0x33, 0xff ),
	Color32_s( 0x66, 0xcb, 0x00, 0xff ),
	Color32_s( 0x33, 0xcb, 0xff, 0xff ),
	Color32_s( 0x33, 0xcb, 0xcb, 0xff ),
	Color32_s( 0x33, 0xcb, 0x98, 0xff ),
	Color32_s( 0x33, 0xcb, 0x66, 0xff ),
	Color32_s( 0x33, 0xcb, 0x33, 0xff ),
	Color32_s( 0x33, 0xcb, 0x00, 0xff ),
	Color32_s( 0xff, 0x66, 0xff, 0xff ),
	Color32_s( 0xff, 0x66, 0xcb, 0xff ),
	Color32_s( 0xff, 0x66, 0x98, 0xff ),
	Color32_s( 0xff, 0x66, 0x66, 0xff ),
	Color32_s( 0xff, 0x66, 0x33, 0xff ),
	Color32_s( 0xff, 0x66, 0x00, 0xff ),
	Color32_s( 0x00, 0x66, 0x98, 0xff ),
	Color32_s( 0x00, 0x66, 0x66, 0xff ),
	Color32_s( 0xcb, 0x98, 0xff, 0xff ),
	Color32_s( 0xcb, 0x98, 0xcb, 0xff ),
	Color32_s( 0xcb, 0x98, 0x98, 0xff ),
	Color32_s( 0xcb, 0x98, 0x66, 0xff ),
	Color32_s( 0xcb, 0x98, 0x33, 0xff ),
	Color32_s( 0xcb, 0x98, 0x00, 0xff ),
	Color32_s( 0x98, 0x98, 0xff, 0xff ),
	Color32_s( 0x98, 0x98, 0xcb, 0xff ),
	Color32_s( 0x98, 0x98, 0x98, 0xff ),
	Color32_s( 0x98, 0x98, 0x66, 0xff ),
	Color32_s( 0x98, 0x98, 0x33, 0xff ),
	Color32_s( 0x98, 0x98, 0x00, 0xff ),
	Color32_s( 0x66, 0x98, 0xff, 0xff ),
	Color32_s( 0x66, 0x98, 0xcb, 0xff ),
	Color32_s( 0x66, 0x98, 0x98, 0xff ),
	Color32_s( 0x66, 0x98, 0x66, 0xff ),
	Color32_s( 0x66, 0x98, 0x33, 0xff ),
	Color32_s( 0x66, 0x98, 0x00, 0xff ),
	Color32_s( 0x33, 0x98, 0xff, 0xff ),
	Color32_s( 0x33, 0x98, 0xcb, 0xff ),
	Color32_s( 0x33, 0x98, 0x98, 0xff ),
	Color32_s( 0x33, 0x98, 0x66, 0xff ),
	Color32_s( 0x33, 0x98, 0x33, 0xff ),
	Color32_s( 0x33, 0x98, 0x00, 0xff ),
	Color32_s( 0xe6, 0x86, 0x00, 0xff ),
	Color32_s( 0xff, 0x33, 0xcb, 0xff ),
	Color32_s( 0xff, 0x33, 0x98, 0xff ),
	Color32_s( 0xff, 0x33, 0x66, 0xff ),
	Color32_s( 0xff, 0x33, 0x33, 0xff ),
	Color32_s( 0xff, 0x33, 0x00, 0xff ),
	Color32_s( 0x00, 0x66, 0x33, 0xff ),
	Color32_s( 0x00, 0x66, 0x00, 0xff ),
	Color32_s( 0xcb, 0x66, 0xff, 0xff ),
	Color32_s( 0xcb, 0x66, 0xcb, 0xff ),
	Color32_s( 0xcb, 0x66, 0x98, 0xff ),
	Color32_s( 0xcb, 0x66, 0x66, 0xff ),
	Color32_s( 0xcb, 0x66, 0x33, 0xff ),
	Color32_s( 0xcb, 0x66, 0x00, 0xff ),
	Color32_s( 0x98, 0x66, 0xff, 0xff ),
	Color32_s( 0x98, 0x66, 0xcb, 0xff ),
	Color32_s( 0x98, 0x66, 0x98, 0xff ),
	Color32_s( 0x98, 0x66, 0x66, 0xff ),
	Color32_s( 0x98, 0x66, 0x33, 0xff ),
	Color32_s( 0x98, 0x66, 0x00, 0xff ),
	Color32_s( 0x66, 0x66, 0xff, 0xff ),
	Color32_s( 0x66, 0x66, 0xcb, 0xff ),
	Color32_s( 0x66, 0x66, 0x98, 0xff ),
	Color32_s( 0x66, 0x66, 0x66, 0xff ),
	Color32_s( 0x66, 0x66, 0x33, 0xff ),
	Color32_s( 0x66, 0x66, 0x00, 0xff ),
	Color32_s( 0x33, 0x66, 0xff, 0xff ),
	Color32_s( 0x33, 0x66, 0xcb, 0xff ),
	Color32_s( 0x33, 0x66, 0x98, 0xff ),
	Color32_s( 0x33, 0x66, 0x66, 0xff ),
	Color32_s( 0x33, 0x66, 0x33, 0xff ),
	Color32_s( 0x33, 0x66, 0x00, 0xff ),
	Color32_s( 0xff, 0x00, 0xff, 0xff ),
	Color32_s( 0xff, 0x00, 0xcb, 0xff ),
	Color32_s( 0xff, 0x00, 0x98, 0xff ),
	Color32_s( 0xff, 0x00, 0x66, 0xff ),
	Color32_s( 0xff, 0x00, 0x33, 0xff ),
	Color32_s( 0xff, 0xaf, 0x13, 0xff ),
	Color32_s( 0x00, 0x33, 0xff, 0xff ),
	Color32_s( 0x00, 0x33, 0xcb, 0xff ),
	Color32_s( 0xcb, 0x33, 0xff, 0xff ),
	Color32_s( 0xcb, 0x33, 0xcb, 0xff ),
	Color32_s( 0xcb, 0x33, 0x98, 0xff ),
	Color32_s( 0xcb, 0x33, 0x66, 0xff ),
	Color32_s( 0xcb, 0x33, 0x33, 0xff ),
	Color32_s( 0xcb, 0x33, 0x00, 0xff ),
	Color32_s( 0x98, 0x33, 0xff, 0xff ),
	Color32_s( 0x98, 0x33, 0xcb, 0xff ),
	Color32_s( 0x98, 0x33, 0x98, 0xff ),
	Color32_s( 0x98, 0x33, 0x66, 0xff ),
	Color32_s( 0x98, 0x33, 0x33, 0xff ),
	Color32_s( 0x98, 0x33, 0x00, 0xff ),
	Color32_s( 0x66, 0x33, 0xff, 0xff ),
	Color32_s( 0x66, 0x33, 0xcb, 0xff ),
	Color32_s( 0x66, 0x33, 0x98, 0xff ),
	Color32_s( 0x66, 0x33, 0x66, 0xff ),
	Color32_s( 0x66, 0x33, 0x33, 0xff ),
	Color32_s( 0x66, 0x33, 0x00, 0xff ),
	Color32_s( 0x33, 0x33, 0xff, 0xff ),
	Color32_s( 0x33, 0x33, 0xcb, 0xff ),
	Color32_s( 0x33, 0x33, 0x98, 0xff ),
	Color32_s( 0x33, 0x33, 0x66, 0xff ),
	Color32_s( 0x33, 0x33, 0x33, 0xff ),
	Color32_s( 0x33, 0x33, 0x00, 0xff ),
	Color32_s( 0xff, 0xcb, 0x66, 0xff ),
	Color32_s( 0xff, 0xcb, 0x98, 0xff ),
	Color32_s( 0xff, 0xcb, 0xcb, 0xff ),
	Color32_s( 0xff, 0xcb, 0xff, 0xff ),
	Color32_s( 0x00, 0x33, 0x98, 0xff ),
	Color32_s( 0x00, 0x33, 0x66, 0xff ),
	Color32_s( 0x00, 0x33, 0x33, 0xff ),
	Color32_s( 0x00, 0x33, 0x00, 0xff ),
	Color32_s( 0xcb, 0x00, 0xff, 0xff ),
	Color32_s( 0xcb, 0x00, 0xcb, 0xff ),
	Color32_s( 0xcb, 0x00, 0x98, 0xff ),
	Color32_s( 0xcb, 0x00, 0x66, 0xff ),
	Color32_s( 0xcb, 0x00, 0x33, 0xff ),
	Color32_s( 0xff, 0xe3, 0x46, 0xff ),
	Color32_s( 0x98, 0x00, 0xff, 0xff ),
	Color32_s( 0x98, 0x00, 0xcb, 0xff ),
	Color32_s( 0x98, 0x00, 0x98, 0xff ),
	Color32_s( 0x98, 0x00, 0x66, 0xff ),
	Color32_s( 0x98, 0x00, 0x33, 0xff ),
	Color32_s( 0x98, 0x00, 0x00, 0xff ),
	Color32_s( 0x66, 0x00, 0xff, 0xff ),
	Color32_s( 0x66, 0x00, 0xcb, 0xff ),
	Color32_s( 0x66, 0x00, 0x98, 0xff ),
	Color32_s( 0x66, 0x00, 0x66, 0xff ),
	Color32_s( 0x66, 0x00, 0x33, 0xff ),
	Color32_s( 0x66, 0x00, 0x00, 0xff ),
	Color32_s( 0x33, 0x00, 0xff, 0xff ),
	Color32_s( 0x33, 0x00, 0xcb, 0xff ),
	Color32_s( 0x33, 0x00, 0x98, 0xff ),
	Color32_s( 0x33, 0x00, 0x66, 0xff ),
	Color32_s( 0x33, 0x00, 0x33, 0xff ),
	Color32_s( 0x33, 0x00, 0x00, 0xff ),
	Color32_s( 0xff, 0xcb, 0x33, 0xff ),
	Color32_s( 0xff, 0xcb, 0x00, 0xff ),
	Color32_s( 0xff, 0xff, 0x00, 0xff ),
	Color32_s( 0xff, 0xff, 0x33, 0xff ),
	Color32_s( 0xff, 0xff, 0x66, 0xff ),
	Color32_s( 0xff, 0xff, 0x98, 0xff ),
	Color32_s( 0xff, 0xff, 0xcb, 0xff ),
	Color32_s( 0xff, 0xff, 0xff, 0xff )
};

static bool g_bUseMMX = false;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DisplayDriver::DisplayDriver():m_cCursorHotSpot( 0, 0 )
{
	m_pcMouseImage = NULL;
	m_pcMouseSprite = NULL;
	m_psFirstMemObj = NULL;

	/* Get system information and check if we have MMX support */
	system_info sSysInfo;

	if( get_system_info( &sSysInfo ) == 0 && sSysInfo.nCPUType & CPU_FEATURE_MMX )
		g_bUseMMX = true;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DisplayDriver::~DisplayDriver()
{
	delete m_pcMouseSprite;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::Close( void )
{
}

/** Get offset between raster-area start and frame-buffer start
 * \par Description:
 *	Should return the offset from the frame-buffer area's logical
 *	address to the actual start of the frame-buffer.
 * \par Warning:
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


int DisplayDriver::GetFramebufferOffset()
{
	return ( 0 );
}

void DisplayDriver::SetCursorBitmap( mouse_ptr_mode eMode, const IPoint & cHotSpot, const void *pRaster, int nWidth, int nHeight )
{
	SrvSprite::Hide();

	m_cCursorHotSpot = cHotSpot;
	delete m_pcMouseSprite;

	if( m_pcMouseImage != NULL )
	{
		m_pcMouseImage->Release();
	}
	if( eMode == MPTR_RGB32 ) 
	{
		m_pcMouseImage = new SrvBitmap( nWidth, nHeight, CS_RGB32 );
		memcpy( m_pcMouseImage->m_pRaster, pRaster, nWidth * nHeight * 4 );
	}
	else
	{
		m_pcMouseImage = new SrvBitmap( nWidth, nHeight, CS_CMAP8 );

		uint8 *pDstRaster = m_pcMouseImage->m_pRaster;
		const uint8 *pSrcRaster = static_cast < const uint8 *>( pRaster );

		for( int i = 0; i < nWidth * nHeight; ++i )
		{
			uint8 anPalette[] = { 255, 0, 0, 63 };
			if( pSrcRaster[i] < 4 )
			{
				pDstRaster[i] = anPalette[pSrcRaster[i]];
			}
			else
			{
				pDstRaster[i] = 255;
			}
		}
	}
	if( m_pcMouseImage != NULL )
	{
		m_pcMouseSprite = new SrvSprite( IRect( 0, 0, nWidth - 1, nHeight - 1 ), m_cMousePos, m_cCursorHotSpot, g_pcScreenBitmap, m_pcMouseImage, true );
	}
	SrvSprite::Unhide();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::MouseOn( void )
{
	if( m_pcMouseSprite == NULL )
	{
		m_pcMouseSprite = new SrvSprite( IRect( 0, 0, m_pcMouseImage->m_nWidth - 1, m_pcMouseImage->m_nHeight - 1 ), m_cMousePos, m_cCursorHotSpot, g_pcScreenBitmap, m_pcMouseImage, true );
	}
	else
	{
		dbprintf( "Warning: DisplayDriver::MouseOn() called while mouse visible\n" );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::MouseOff( void )
{
	if( m_pcMouseSprite == NULL )
	{
		dbprintf( "Warning: VDisplayDriver::MouseOff() called while mouse hidden\n" );
	}
	delete m_pcMouseSprite;

	m_pcMouseSprite = NULL;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::SetMousePos( IPoint cNewPos )
{
	if( m_pcMouseSprite != NULL )
	{
		m_pcMouseSprite->MoveTo( cNewPos );
	}
	m_cMousePos = cNewPos;
}


/** Locks source and destination bitmaps when they are accessed by the software
 * renderer.
 * \par Description:
 * Called before the software renderer accesses bitmaps. After this call all pending
 * drawing operations of the graphics card have to be finished and written back to 
 * video memory. 
 * \par
 * \param pcDstBitMap - Destination bitmap.
 * \param pcSrcBitmap - Source bitmap. Might be NULL.
 * \param cSrcRect - Source rectangle.
 * \param cDstRect - Destination rectangle.
 * \sa UnlockBitmap()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
 
void DisplayDriver::LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrcRect, os::IRect cDstRect )
{
}


/** Unlocks source and destination bitmaps after they have been accessed by the software
 * renderer.
 * \par Description:
 * Called after the software renderer has finished drawing from/to this bitmaps.
 * \par
 * \param pcDstBitMap - Destination bitmap.
 * \param pcSrcBitmap - Source bitmap. Might be NULL.
 * \param cSrcRect - Source rectangle.
 * \param cDstRect - Destination rectangle.
 * \sa LockBitmap()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
 
void DisplayDriver::UnlockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrcRect, os::IRect cDstRect )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::FillBlit16( uint16 *pDst, int nMod, int W, int H, uint16 nColor )
{
	int X, Y;
	
	bool bAddLast = false;
	if( W & 1 )
	{
		bAddLast = true;
		W -= 1;
	}
	
	for( Y = 0; Y < H; Y++ )
	{
		uint32* pDstConvert = (uint32*)pDst;
		for( X = 0; X < W / 2; X++ )
		{
			
			*pDstConvert++ = nColor << 16 | nColor;
			pDst += 2;
		}
		if( bAddLast )
		{
			*pDst++ = nColor;
		}		
		pDst += nMod;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::FillBlit32( uint32 *pDst, int nMod, int W, int H, uint32 nColor )
{
	if( g_bUseMMX )
	{
		mmx_fill32( pDst, nMod, W, H, nColor );
		emms();
		return;
	}
	
	int X, Y;
	for( Y = 0; Y < H; ++Y )
	{
		for( X = 0; X < W; ++X )
		{
			*pDst++ = nColor;
		}
		pDst += nMod;
	}
}

void DisplayDriver::FillAlpha16( uint16 *pDst, int nMod, int W, int H, uint32 nColor )
{
	unsigned nAlpha = nColor >> 27;
	
	for( int y = 0; y < H; ++y )
	{
		for( int x = 0; x < W; ++x )
		{
			uint32 nSrcColor = nColor;	
			if( nAlpha == ( 0xff >> 3 ) ) {
				*pDst = ( nSrcColor >> 8 & 0xf800 ) + ( nSrcColor >> 5 & 0x7e0 ) 
						+ ( nSrcColor >> 3  & 0x1f );
			} else if( nAlpha != 0x00 ) {
				uint32 nDstColor = *pDst;
				nSrcColor = ( ( nSrcColor & 0xfc00 ) << 11 ) + ( nSrcColor >> 8 & 0xf800 )
					      + ( nSrcColor >> 3 & 0x1f );
				nDstColor = ( nDstColor | nDstColor << 16 ) & 0x07e0f81f;
				nDstColor += ( nSrcColor - nDstColor ) * nAlpha >> 5;
				nDstColor &= 0x07e0f81f;
				*pDst = nDstColor | nDstColor >> 16;
			}		
			pDst++;
		}
		pDst += nMod;
	}
}
void DisplayDriver::FillAlpha32( uint32 *pDst, int nMod, int W, int H, uint32 nColor, uint32 nFlags )
{
	if( g_bUseMMX )
	{
		mmx_fill_alpha32( pDst, nMod, W, H, nColor, nFlags );
		emms();
		return;
	}
	
	uint32 nAlpha = nColor >> 24;
	bool bNoAlphaChannel = nFlags & Bitmap::NO_ALPHA_CHANNEL;
	for( int y = 0; y < H; ++y )
	{
		for( int x = 0; x < W; ++x )
		{
			if( nAlpha == 0xff )
			{
				if( bNoAlphaChannel )
					*pDst = ( 0xff000000 ) | ( nColor & 0x00ffffff );
				else
					*pDst = ( *pDst & 0xff000000 ) | ( nColor & 0x00ffffff );
			}
			else
			{
				uint32 nSrcColor = nColor;			
				uint32 nDstColor = *pDst;
				uint32 nSrc1;
				uint32 nDst1;
				uint32 nDstAlpha = nDstColor & 0xff000000;

				nSrc1 = nSrcColor & 0xff00ff;
				nDst1 = nDstColor & 0xff00ff;
				nDst1 = ( nDst1 + ( ( nSrc1 - nDst1 ) * nAlpha >> 8 ) ) & 0xff00ff;
				nSrcColor &= 0xff00;
				nDstColor &= 0xff00;
				nDstColor = ( nDstColor + ( ( nSrcColor - nDstColor ) * nAlpha >> 8)) & 0xff00;
				*pDst = nDst1 | nDstColor | nDstAlpha;
			}
			pDst++;
		}
		pDst = pDst + nMod;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool DisplayDriver::FillRect( SrvBitmap * pcBitmap, const IRect & cRect, const Color32_s & sColor, int nMode )
{
	int BltX, BltY, BltW, BltH;
	
	BltX = cRect.left;
	BltY = cRect.top;
	BltW = cRect.Width() + 1;
	BltH = cRect.Height() + 1;
	
	LockBitmap( pcBitmap, NULL, IRect(), cRect );
	
	switch ( pcBitmap->m_eColorSpc )
	{
	case CS_RGB16:
		if( nMode == DM_BLEND )
			FillAlpha16( ( uint16 * )&pcBitmap->m_pRaster[BltY * pcBitmap->m_nBytesPerLine + BltX * 2], pcBitmap->m_nBytesPerLine / 2 - BltW, BltW, BltH, sColor );
		else
			FillBlit16( ( uint16 * )&pcBitmap->m_pRaster[BltY * pcBitmap->m_nBytesPerLine + BltX * 2], pcBitmap->m_nBytesPerLine / 2 - BltW, BltW, BltH, COL_TO_RGB16( sColor ) );
		break;
	case CS_RGB32:
	case CS_RGBA32:
		if( nMode == DM_BLEND )
			FillAlpha32( ( uint32 * )&pcBitmap->m_pRaster[BltY * pcBitmap->m_nBytesPerLine + BltX * 4], pcBitmap->m_nBytesPerLine / 4 - BltW, BltW, BltH, sColor, pcBitmap->m_nFlags );
		else
			FillBlit32( ( uint32 * )&pcBitmap->m_pRaster[BltY * pcBitmap->m_nBytesPerLine + BltX * 4], pcBitmap->m_nBytesPerLine / 4 - BltW, BltW, BltH, COL_TO_RGB32( sColor ) );
		break;
	default:
		dbprintf( "DisplayDriver::FillRect() unknown color space %d\n", pcBitmap->m_eColorSpc );
	}
	
	UnlockBitmap( pcBitmap, NULL, IRect(), cRect );

	return ( true );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline void Blit( uint8 *Src, uint8 *Dst, int SMod, int DMod, int W, int H, bool Rev )
{
	int i;
	int X, Y;
	uint32 *LSrc;
	uint32 *LDst;

	if( Rev )
	{
		if( g_bUseMMX )
		{
			Src -= W - 1;
			Dst -= W - 1;
			DMod += W;
			SMod += W;
			for( Y = 0; Y < H; ++Y )
			{
				mmx_revcpy( Dst, Src, W );
				Dst -= DMod;
				Src -= SMod;
			}
			return;
		}

		for( Y = 0; Y < H; Y++ )
		{
			for( X = 0; ( X < W ) && ( ( uint32 ( Src - 7 ) )&7 ); X++ )
			{
				*Dst-- = *Src--;
			}

			LSrc = ( uint32 * )( ( ( uint32 )Src ) - 7 );
			LDst = ( uint32 * )( ( ( uint32 )Dst ) - 7 );

			
			i = ( W - X ) / 4;
			X += i * 4;
			for( ; i; i-- )
			{
				*LDst-- = *LSrc--;
			}

			Src = ( uint8 * )( ( ( uint32 )LSrc ) + 7 );
			Dst = ( uint8 * )( ( ( uint32 )LDst ) + 7 );

			for( ; X < W; X++ )
			{
				*Dst-- = *Src--;
			}

			Dst -= ( int32 )DMod;
			Src -= ( int32 )SMod;
		}
	}
	else
	{
		if( g_bUseMMX )
		{
			DMod += W;
			SMod += W;
			for( Y = 0; Y < H; ++Y )
			{
				mmx_memcpy( Dst, Src, W );
				Dst += DMod;
				Src += SMod;
			}
			return;
		}
		
		for( Y = 0; Y < H; Y++ )
		{
			for( X = 0; ( X < W ) && ( ( ( uint32 )Src ) & 7 ); ++X )
			{
				*Dst++ = *Src++;
			}

			LSrc = ( uint32 * )Src;
			LDst = ( uint32 * )Dst;
			
			i = ( W - X ) / 4;
			X += i * 4;

			for( ; i; i-- )
			{
				*LDst++ = *LSrc++;
			}

			Src = ( uint8 * )LSrc;
			Dst = ( uint8 * )LDst;

			for( ; X < W; X++ )
			{
				*Dst++ = *Src++;
			}

			Dst += DMod;
			Src += SMod;
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline void BitBlit( SrvBitmap * sbm, SrvBitmap * dbm, int sx, int sy, int dx, int dy, int w, int h )
{
	int Smod, Dmod;
	int BytesPerPix = 1;

	int InPtr, OutPtr;

	int nBitsPerPix = BitsPerPixel( dbm->m_eColorSpc );

	BytesPerPix = nBitsPerPix / 8;
	
	sx *= BytesPerPix;
	dx *= BytesPerPix;
	w *= BytesPerPix;

	if( sx >= dx )
	{
		if( sy >= dy )
		{
			Smod = sbm->m_nBytesPerLine - w;
			Dmod = dbm->m_nBytesPerLine - w;
			InPtr = sy * sbm->m_nBytesPerLine + sx;
			OutPtr = dy * dbm->m_nBytesPerLine + dx;

			Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, false );
		}
		else
		{
			Smod = -sbm->m_nBytesPerLine - w;
			Dmod = -dbm->m_nBytesPerLine - w;
			InPtr = ( ( sy + h - 1 ) * sbm->m_nBytesPerLine ) + sx;
			OutPtr = ( ( dy + h - 1 ) * dbm->m_nBytesPerLine ) + dx;

			Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, false );
		}
	}
	else
	{
		if( sy > dy )
		{
			Smod = -( sbm->m_nBytesPerLine + w );
			Dmod = -( dbm->m_nBytesPerLine + w );
			InPtr = ( sy * sbm->m_nBytesPerLine ) + sx + w - 1;
			OutPtr = ( dy * dbm->m_nBytesPerLine ) + dx + w - 1;
			Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, true );
		}
		else
		{
			Smod = sbm->m_nBytesPerLine - w;
			Dmod = dbm->m_nBytesPerLine - w;
			InPtr = ( sy + h - 1 ) * sbm->m_nBytesPerLine + sx + w - 1;
			OutPtr = ( dy + h - 1 ) * dbm->m_nBytesPerLine + dx + w - 1;
			Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, true );
		}
	}
	if( g_bUseMMX )
		emms();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------


static inline void blit_convert_copy( SrvBitmap * pcDst, SrvBitmap * pcSrc, const IRect & cSrcRect, const IPoint & cDstPos )
{
	switch ( ( int )pcSrc->m_eColorSpc )
	{
	case CS_CMAP8:
		{
			uint8 *pSrc = RAS_OFFSET8( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );

			int nSrcModulo = pcSrc->m_nBytesPerLine - ( cSrcRect.Width() + 1 );

			switch ( ( int )pcDst->m_eColorSpc )
			{
			case CS_CMAP8:
				{
					if( pcSrc == pcDst )
					{
						BitBlit( pcSrc, pcDst, cSrcRect.left, cSrcRect.top, cDstPos.x, cDstPos.y, cSrcRect.Width() + 1, cSrcRect.Height(  ) + 1 );
						break;
					}

					uint8 *pDst = RAS_OFFSET8( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 );

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						if( g_bUseMMX )
							mmx_memcpy( ( uint8 * )pDst, ( uint8 * )pSrc, ( cSrcRect.right - cSrcRect.left + 1 ) );
						else
							memcpy( ( uint8 * )pDst, ( uint8 * )pSrc, ( cSrcRect.right - cSrcRect.left + 1 ) );
						pSrc += ( cSrcRect.right - cSrcRect.left + 1 );
						pDst += ( cSrcRect.right - cSrcRect.left + 1 );
						pSrc = ( uint8 * )( ( ( uint8 * )pSrc ) + nSrcModulo );
						pDst = ( uint8 * )( ( ( uint8 * )pDst ) + nDstModulo );
					}
					if( g_bUseMMX )
						emms();
					break;
				}
			case CS_RGB16:
				{
					uint16 *pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );

					int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 2;

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
						{
							*pDst++ = COL_TO_RGB16( g_asDefaultPallette[*pSrc++] );
						}
						pSrc += nSrcModulo;
						pDst = ( uint16 * )( ( ( uint8 * )pDst ) + nDstModulo );
					}
					break;
				}
			case CS_RGB32:
			case CS_RGBA32:
				{
					uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 4;

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
						{
							*pDst++ = COL_TO_RGB32( g_asDefaultPallette[*pSrc++] );
						}
						pSrc += nSrcModulo;
						pDst = ( uint32 * )( ( ( uint8 * )pDst ) + nDstModulo );
					}
					break;
				}
			}
			break;
		}
	case CS_RGB16:
		{
			uint16 *pSrc = RAS_OFFSET16( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );

			switch ( ( int )pcDst->m_eColorSpc )
			{
			case CS_RGB16:
				{

					if( pcSrc == pcDst )
					{
						BitBlit( pcSrc, pcDst, cSrcRect.left, cSrcRect.top, cDstPos.x, cDstPos.y, cSrcRect.Width() + 1, cSrcRect.Height(  ) + 1 );
						break;
					}

					uint16 *pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		
					int nDstModulo = pcDst->m_nBytesPerLine / 2;
					int nSrcModulo = pcSrc->m_nBytesPerLine / 2;			
					int nWidth = 2 * ( cSrcRect.right - cSrcRect.left + 1 );

					if( g_bUseMMX )
					{
						for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
						{
							mmx_memcpy( ( uint8 * )pDst, ( uint8 * )pSrc, nWidth );
							pSrc += nSrcModulo;
							pDst += nDstModulo;
						
						}
						emms();
					}				
					else
					{
						for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
						{
							memcpy( ( uint8 * )pDst, ( uint8 * )pSrc, nWidth );
							pSrc += nSrcModulo;
							pDst += nDstModulo;
						
						}
					}				
					break;
				}
			case CS_RGB32:
			case CS_RGBA32:
				{
					uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = ( pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 4 ) / 4;
					int nSrcModulo = ( pcSrc->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 2 ) / 2;
					int nWidth = ( cSrcRect.right - cSrcRect.left + 1 );

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						if( g_bUseMMX )
						{
							mmx_rgb16_to_rgb32( ( uint8 * )pSrc, ( uint8 * )pDst, cSrcRect.Width() + 1 );
							pSrc += nWidth;
							pDst += nWidth;
						}
						else
						{
							for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
							{
								*pDst++ = COL_TO_RGB32( RGB16_TO_COL( *pSrc++ ) );
							}
						}
						pSrc += nSrcModulo;
						pDst += nDstModulo;
					}
					if( g_bUseMMX )
						emms();
					break;
				}
			}
			break;
		}
	case CS_RGB32:
	case CS_RGBA32:
		{
			uint32 *pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );

			switch ( ( int )pcDst->m_eColorSpc )
			{
			case CS_RGB16:
				{
					uint16 *pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = ( pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 2 ) / 2;
					int nSrcModulo = ( pcSrc->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 4 ) / 4;					
					int nWidth = ( cSrcRect.right - cSrcRect.left + 1 );

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						if( g_bUseMMX )
						{
							mmx_rgb32_to_rgb16( ( uint8 * )pSrc, ( uint8 * )pDst, nWidth );
							pSrc += nWidth;
							pDst += nWidth;
						}
						else
						{
							for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
							{
								*pDst++ = COL_TO_RGB16( RGB32_TO_COL( *pSrc++ ) );
							}
						}
						pSrc += nSrcModulo;
						pDst += nDstModulo;
					}
					if( g_bUseMMX )
						emms();
					break;
				}
			case CS_RGB32:
			case CS_RGBA32:
				{
					if( pcSrc == pcDst )
					{
						BitBlit( pcSrc, pcDst, cSrcRect.left, cSrcRect.top, cDstPos.x, cDstPos.y, cSrcRect.Width() + 1, cSrcRect.Height(  ) + 1 );
						break;
					}
					uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine / 4;
					int nSrcModulo = pcSrc->m_nBytesPerLine / 4;					
					int nWidth = 4 * ( cSrcRect.right - cSrcRect.left + 1 );

					if( g_bUseMMX )
					{
						for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y ) {
							mmx_memcpy( ( uint8 * )pDst, ( uint8 * )pSrc, nWidth );							
							pSrc += nSrcModulo;
							pDst += nDstModulo;
						}
						emms();
					}
					else
					{
						for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y ) {
							memcpy( ( uint8 * )pDst, ( uint8 * )pSrc, nWidth );
							pSrc += nSrcModulo;
							pDst += nDstModulo;
						}					
					}
					break;
				}
			}
			break;
		}
	default:
		dbprintf( "blit_convert_copy() unknown src color space %d\n", pcSrc->m_eColorSpc );
		break;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline void blit_convert_over( SrvBitmap * pcDst, SrvBitmap * pcSrc, const IRect & cSrcRect, const IPoint & cDstPos )
{
	switch ( ( int )pcSrc->m_eColorSpc )
	{
	case CS_CMAP8:
		{
			uint8 *pSrc = RAS_OFFSET8( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );

			int nSrcModulo = pcSrc->m_nBytesPerLine - ( cSrcRect.Width() + 1 );

			switch ( ( int )pcDst->m_eColorSpc )
			{
			case CS_RGB16:
				{
					uint16 *pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 2;

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
						{
							int nPix = *pSrc++;

							if( nPix != TRANSPARENT_CMAP8 )
							{
								*pDst = COL_TO_RGB16( g_asDefaultPallette[nPix] );
							}
							pDst++;
						}
						pSrc += nSrcModulo;
						pDst = ( uint16 * )( ( ( uint8 * )pDst ) + nDstModulo );
					}
					break;
				}
			case CS_RGB32:
			case CS_RGBA32:
				{
					uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 4;

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
						{
							int nPix = *pSrc++;

							if( nPix != TRANSPARENT_CMAP8 )
							{
								*pDst = COL_TO_RGB32( g_asDefaultPallette[nPix] );
							}
							pDst++;
						}
						pSrc += nSrcModulo;
						pDst = ( uint32 * )( ( ( uint8 * )pDst ) + nDstModulo );
					}
					break;
				}
			default:
				dbprintf( "blit_convert_over() unknown dst colorspace for 8 bit src %d\n", pcDst->m_eColorSpc );
				break;
			}
			break;
		}
	case CS_RGB16:
		{
			uint16 *pSrc = RAS_OFFSET16( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
			int nSrcModulo = pcSrc->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 2;

			switch ( ( int )pcDst->m_eColorSpc )
			{
			case CS_RGB32:
			case CS_RGBA32:
				{
					uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 4;

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
						{
							*pDst++ = COL_TO_RGB32( RGB16_TO_COL( *pSrc++ ) );
						}
						pSrc = ( uint16 * )( ( ( uint8 * )pSrc ) + nSrcModulo );
						pDst = ( uint32 * )( ( ( uint8 * )pDst ) + nDstModulo );
					}
					break;
				}
			}
			break;
		}
	case CS_RGB32:
	case CS_RGBA32:
		{
			uint32 *pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
			int nSrcModulo = pcSrc->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 4;

			switch ( ( int )pcDst->m_eColorSpc )
			{
			case CS_RGB16:
				{
					uint16 *pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 2;

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
						{
							uint32 nPix = *pSrc++;

							if( nPix != 0xffffffff )
							{
								*pDst = COL_TO_RGB16( RGB32_TO_COL( nPix ) );
							}
							pDst++;
						}
						pSrc = ( uint32 * )( ( ( uint8 * )pSrc ) + nSrcModulo );
						pDst = ( uint16 * )( ( ( uint8 * )pDst ) + nDstModulo );
					}
					break;
				}
			case CS_RGB32:
			case CS_RGBA32:
				{
					uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 4;

					for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
					{
						for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
						{
							uint32 nPix = *pSrc++;

							if( nPix != 0xffffffff )
							{
								*pDst = nPix;
							}
							pDst++;
						}
						pSrc = ( uint32 * )( ( ( uint8 * )pSrc ) + nSrcModulo );
						pDst = ( uint32 * )( ( ( uint8 * )pDst ) + nDstModulo );
					}
					break;
				}
			}
			break;
		}
	default:
		dbprintf( "blit_convert_over() unknown src color space %d\n", pcSrc->m_eColorSpc );
		break;
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline void blit_convert_alpha( SrvBitmap * pcDst, SrvBitmap * pcSrc, const IRect & cSrcRect, const IPoint & cDstPos )
{
	uint32 *pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
	int nSrcModulo = pcSrc->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 4;
	bool bNoAlphaChannel = pcDst->m_nFlags & Bitmap::NO_ALPHA_CHANNEL;

	switch ( ( int )pcDst->m_eColorSpc )
	{
	case CS_RGB16:
		{
			uint16 *pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
			int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 2;

			nSrcModulo >>= 2;
			nDstModulo >>= 1;

			for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
			{
				for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
				{
					
					uint32 nSrcColor = *pSrc++;
					unsigned nAlpha = nSrcColor >> 27;
					
					if( nAlpha == ( 0xff >> 3 ) ) {
						*pDst = ( nSrcColor >> 8 & 0xf800 ) + ( nSrcColor >> 5 & 0x7e0 ) 
								+ ( nSrcColor >> 3  & 0x1f );
					} else if( nAlpha != 0x00 ) {
						uint32 nDstColor = *pDst;
						nSrcColor = ( ( nSrcColor & 0xfc00 ) << 11 ) + ( nSrcColor >> 8 & 0xf800 )
							      + ( nSrcColor >> 3 & 0x1f );
						nDstColor = ( nDstColor | nDstColor << 16 ) & 0x07e0f81f;
						nDstColor += ( nSrcColor - nDstColor ) * nAlpha >> 5;
						nDstColor &= 0x07e0f81f;
						*pDst = nDstColor | nDstColor >> 16;
					}
					
					pDst++;
				}
				pSrc = pSrc + nSrcModulo;
				pDst = pDst + nDstModulo;
			}
			break;
		}
	case CS_RGB32:
	case CS_RGBA32:
		{
			uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
			int nDstModulo = pcDst->m_nBytesPerLine - ( cSrcRect.Width() + 1 ) * 4;

			if( g_bUseMMX )
			{
				mmx_blit_alpha32( pcDst, pcSrc, cSrcRect, cDstPos );
				emms();
				return;
			}
			else
			{
				nSrcModulo >>= 2;
				nDstModulo >>= 2;

				uint32 nSrcColor;

				for( int y = cSrcRect.top; y <= cSrcRect.bottom; ++y )
				{
					for( int x = cSrcRect.left; x <= cSrcRect.right; ++x )
					{
						nSrcColor = *pSrc++;
						uint32 nAlpha = nSrcColor >> 24;
								
						if( nAlpha == 0xff && bNoAlphaChannel )
							*pDst = 0xff000000 | ( nSrcColor & 0x00ffffff );
						else if( nAlpha == 0xff )
							*pDst = ( *pDst & 0xff000000 ) | ( nSrcColor & 0x00ffffff );
						else if( nAlpha != 0x00 )
						{
							uint32 nDstColor = *pDst;
							uint32 nSrc1;
							uint32 nDst1;
							uint32 nDstAlpha = nDstColor & 0xff000000;

							nSrc1 = nSrcColor & 0xff00ff;
							nDst1 = nDstColor & 0xff00ff;
							nDst1 = ( nDst1 + ( ( nSrc1 - nDst1 ) * nAlpha >> 8 ) ) & 0xff00ff;
							nSrcColor &= 0xff00;
							nDstColor &= 0xff00;
							nDstColor = ( nDstColor + ( ( nSrcColor - nDstColor ) * nAlpha >> 8)) & 0xff00;
							*pDst = nDst1 | nDstColor | nDstAlpha;
						}
						pDst++;
					}
					pSrc = pSrc + nSrcModulo;
					pDst = pDst + nDstModulo;			
				}
			}
			break;
		}
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

extern uint8 g_pCopyRowCode[16384] __attribute__ ((alias ("g_nCopyRow")));
static uint8 g_nCopyRow[16834];
Locker g_cCopyRowLock( "copy_row_lock" );

static inline void blit_convert_stretch_copy( SrvBitmap * pcDst, SrvBitmap * pcSrc, const IRect & cSrcRect, const IRect & cDstRect )
{
	uint32 nStepX = ( ( cSrcRect.Width() + 1 ) << 16 ) / ( cDstRect.Width() + 1 );
	uint32 nStepY = ( ( cSrcRect.Height() + 1 ) << 16 ) / ( cDstRect.Height() + 1 );
	uint32 nCurrentX = 0;
	uint32 nCurrentY = 0;
	uint32 nSrcY = cSrcRect.top;
			
	switch ( ( int )pcSrc->m_eColorSpc )
	{
	case CS_RGB16:
		{
			switch ( ( int )pcDst->m_eColorSpc )
			{
			case CS_RGB16:
				{
					uint16 *pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstRect.left, cDstRect.top, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cDstRect.Width() + 1 ) * 2;
										
					if( g_bUseMMX )				
						g_cCopyRowLock.Lock();
					if( g_bUseMMX )
					{
						/* Create assembler code */
						uint8* pCode = g_nCopyRow;
						for( int x = cDstRect.left; x <= cDstRect.right; ++x )
						{
							nCurrentX += nStepX;
							
							while( nCurrentX >= 0x10000 ) {
								*pCode++ = 0x66; // 16bit
								*pCode++ = 0xAD; // Load
								nCurrentX -= 0x10000;
							}
							*pCode++ = 0x66; // 16bit
							*pCode++ = 0xAB; // Store
						}
						*pCode++ = 0xC3; // Return
					}
					
					for( int y = cDstRect.top; y <= cDstRect.bottom; ++y )
					{
						nCurrentX = 0;
						uint16 *pSrc = RAS_OFFSET16( pcSrc->m_pRaster, cSrcRect.left, nSrcY, pcSrc->m_nBytesPerLine );
						uint16 nDst = *pSrc;
						if( g_bUseMMX )
						{
							int nTemp1, nTemp2;
							__asm__ __volatile__ ( "call g_pCopyRowCode"	: "=&D" ( nTemp1 ), "=&S" ( nTemp2 ) : "0" ( (uint8*)pDst ), "1" ( (uint8*)pSrc ) : "memory" );
							pDst += cDstRect.Width() + 1;
						}
						else
						{
							for( int x = cDstRect.left; x <= cDstRect.right; ++x )
							{
								*pDst++ = nDst;
								nCurrentX += nStepX;
								
								while( nCurrentX >= 0x10000 ) {
									pSrc++;
									nDst = *pSrc;
									nCurrentX -= 0x10000;
								}
							}
						}
						pDst = ( uint16 * )( ( ( uint8 * )pDst ) + nDstModulo );
						nCurrentY += nStepY;	
						while( nCurrentY >= 0x10000 ) {
							nSrcY++;
							nCurrentY -= 0x10000;
						}
					}
					if( g_bUseMMX )				
						g_cCopyRowLock.Unlock();
					break;
				}
			case CS_RGB32:
			case CS_RGBA32:
				{
					uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstRect.left, cDstRect.top, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cDstRect.Width() + 1 ) * 4;

					for( int y = cDstRect.top; y <= cDstRect.bottom; ++y )
					{
						uint16 *pSrc = RAS_OFFSET16( pcSrc->m_pRaster, cSrcRect.left, nSrcY, pcSrc->m_nBytesPerLine );
						uint32 nDst = COL_TO_RGB32( RGB16_TO_COL( *pSrc ) );
						nCurrentX = 0;
						for( int x = cDstRect.left; x <= cDstRect.right; ++x )
						{
							*pDst++ = nDst;
							nCurrentX += nStepX;
							
							while( nCurrentX >= 0x10000 ) {
								pSrc++;
								nDst = COL_TO_RGB32( RGB16_TO_COL( *pSrc ) );
								nCurrentX -= 0x10000;
							}
						}
						pDst = ( uint32 * )( ( ( uint8 * )pDst ) + nDstModulo );
						nCurrentY += nStepY;	
						while( nCurrentY >= 0x10000 ) {
							nSrcY++;
							nCurrentY -= 0x10000;
						}
					}
					break;
				}
			}
			break;
		}
	case CS_RGB32:
	case CS_RGBA32:
		{
			switch ( ( int )pcDst->m_eColorSpc )
			{
			case CS_RGB16:
				{
					uint16 *pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstRect.left, cDstRect.top, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cDstRect.Width() + 1 ) * 2;

					for( int y = cDstRect.top; y <= cDstRect.bottom; ++y )
					{
						nCurrentX = 0;
						uint32 *pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, nSrcY, pcSrc->m_nBytesPerLine );
						uint32 nSrc = *pSrc;
						uint16 nDst = ( nSrc >> 8 & 0xf800 ) + ( nSrc >> 5 & 0x7e0 ) + ( nSrc >> 3  & 0x1f );
						for( int x = cDstRect.left; x <= cDstRect.right; ++x )
						{
							*pDst++ = nDst;
							nCurrentX += nStepX;
							
							while( nCurrentX >= 0x10000 ) {
								pSrc++;
								uint32 nSrc = *pSrc; 
								nDst = ( nSrc >> 8 & 0xf800 ) + ( nSrc >> 5 & 0x7e0 ) + ( nSrc >> 3  & 0x1f );
								nCurrentX -= 0x10000;
							}
						}
						pDst = ( uint16 * )( ( ( uint8 * )pDst ) + nDstModulo );
						nCurrentY += nStepY;	
						while( nCurrentY >= 0x10000 ) {
							nSrcY++;
							nCurrentY -= 0x10000;
						}
					}
					break;
				}
			case CS_RGB32:
			case CS_RGBA32:
				{
					uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstRect.left, cDstRect.top, pcDst->m_nBytesPerLine );
					int nDstModulo = pcDst->m_nBytesPerLine - ( cDstRect.Width() + 1 ) * 4;
										
					if( g_bUseMMX )				
						g_cCopyRowLock.Lock();
					if( g_bUseMMX )
					{
						/* Create assembler code */
						uint8* pCode = g_nCopyRow;
						for( int x = cDstRect.left; x <= cDstRect.right; ++x )
						{
							nCurrentX += nStepX;
							
							while( nCurrentX >= 0x10000 ) {
								*pCode++ = 0xAD; // Load
								nCurrentX -= 0x10000;
							}
							*pCode++ = 0xAB; // Store
						}
						*pCode++ = 0xC3; // Return
					}
					
					for( int y = cDstRect.top; y <= cDstRect.bottom; ++y )
					{
						nCurrentX = 0;
						uint32 *pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, nSrcY, pcSrc->m_nBytesPerLine );
						uint32 nDst = *pSrc;
						if( g_bUseMMX )
						{
							int nTemp1, nTemp2;
							__asm__ __volatile__ ( "call g_pCopyRowCode"	: "=&D" ( nTemp1 ), "=&S" ( nTemp2 ) : "0" ( (uint8*)pDst ), "1" ( (uint8*)pSrc ) : "memory" );
							pDst += cDstRect.Width() + 1;
						}
						else
						{
							for( int x = cDstRect.left; x <= cDstRect.right; ++x )
							{
								*pDst++ = nDst;
								nCurrentX += nStepX;
								
								while( nCurrentX >= 0x10000 ) {
									pSrc++;
									nDst = *pSrc;
									nCurrentX -= 0x10000;
								}
							}
						}
						pDst = ( uint32 * )( ( ( uint8 * )pDst ) + nDstModulo );
						nCurrentY += nStepY;	
						while( nCurrentY >= 0x10000 ) {
							nSrcY++;
							nCurrentY -= 0x10000;
						}
					}
					if( g_bUseMMX )				
						g_cCopyRowLock.Unlock();
					break;
				}
			}
			break;
		}
	default:
		dbprintf( "blit_convert_stretch_copy() unknown src color space %d\n", pcSrc->m_eColorSpc );
		break;
	}
}

static inline void blit_convert_stretch_alpha( SrvBitmap * pcDst, SrvBitmap * pcSrc, const IRect & cSrcRect, const IRect & cDstRect )
{
	uint32 nStepX = ( ( cSrcRect.Width() + 1 ) << 16 ) / ( cDstRect.Width() + 1 );
	uint32 nStepY = ( ( cSrcRect.Height() + 1 ) << 16 ) / ( cDstRect.Height() + 1 );
	uint32 nCurrentX = 0;
	uint32 nCurrentY = 0;
	uint32 nSrcY = cSrcRect.top;
			
	switch ( ( int )pcDst->m_eColorSpc )
	{
		case CS_RGB16:
		{
			uint16 *pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstRect.left, cDstRect.top, pcDst->m_nBytesPerLine );
			int nDstModulo = pcDst->m_nBytesPerLine - ( cDstRect.Width() + 1 ) * 2;

			for( int y = cDstRect.top; y <= cDstRect.bottom; ++y )
			{
				nCurrentX = 0;
				uint32 *pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, nSrcY, pcSrc->m_nBytesPerLine );
				uint32 nSrc = *pSrc;
				unsigned nAlpha = nSrc >> 27;
	
				for( int x = cDstRect.left; x <= cDstRect.right; ++x )
				{
					if( nAlpha == ( 0xff >> 3 ) ) {
						*pDst++ = ( nSrc >> 8 & 0xf800 ) + ( nSrc >> 5 & 0x7e0 ) 
								+ ( nSrc >> 3  & 0x1f );
					} else if( nAlpha != 0x00 ) {
						uint32 nDstColor = *pDst;
						uint32 nSrcColor = nSrc;
						nSrcColor = ( ( nSrcColor & 0xfc00 ) << 11 ) + ( nSrcColor >> 8 & 0xf800 )
							      + ( nSrcColor >> 3 & 0x1f );
						nDstColor = ( nDstColor | nDstColor << 16 ) & 0x07e0f81f;
						nDstColor += ( nSrcColor - nDstColor ) * nAlpha >> 5;
						nDstColor &= 0x07e0f81f;
						*pDst++ = nDstColor | nDstColor >> 16;
					} else
						pDst++;
					nCurrentX += nStepX;
						
					while( nCurrentX >= 0x10000 ) {
						pSrc++;
						nSrc = *pSrc;
						nAlpha = nSrc >> 27;
						nCurrentX -= 0x10000;
					}
				}
				pDst = ( uint16 * )( ( ( uint8 * )pDst ) + nDstModulo );
				nCurrentY += nStepY;	
				while( nCurrentY >= 0x10000 ) {
					nSrcY++;
					nCurrentY -= 0x10000;
				}
			}
			break;
		}
	case CS_RGB32:
	case CS_RGBA32:
		{
			uint32 *pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstRect.left, cDstRect.top, pcDst->m_nBytesPerLine );
			int nDstModulo = pcDst->m_nBytesPerLine - ( cDstRect.Width() + 1 ) * 4;
									
			for( int y = cDstRect.top; y <= cDstRect.bottom; ++y )
			{
				nCurrentX = 0;
				uint32 *pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, nSrcY, pcSrc->m_nBytesPerLine );
				uint32 nSrc = *pSrc;
				uint32 nAlpha = nSrc >> 24;
				
				for( int x = cDstRect.left; x <= cDstRect.right; ++x )
				{
					if( nAlpha == 0xff && ( pcDst->m_nFlags & Bitmap::NO_ALPHA_CHANNEL ) )
						*pDst++ = 0xff000000 | ( nSrc & 0x00ffffff );
					else if( nAlpha == 0xff )
						*pDst++ = ( ( *pDst ) & 0xff000000 ) | ( nSrc & 0x00ffffff );
					else if( nAlpha != 0x00 )
					{
						uint32 nDstColor = *pDst;
						uint32 nSrc1;
						uint32 nDst1;
						uint32 nDstAlpha = nDstColor & 0xff000000;
						uint32 nSrcColor = nSrc;
						nSrc1 = nSrcColor & 0xff00ff;
						nDst1 = nDstColor & 0xff00ff;
						nDst1 = ( nDst1 + ( ( nSrc1 - nDst1 ) * nAlpha >> 8 ) ) & 0xff00ff;
						nSrcColor &= 0xff00;
						nDstColor &= 0xff00;
						nDstColor = ( nDstColor + ( ( nSrcColor - nDstColor ) * nAlpha >> 8)) & 0xff00;
						*pDst++ = nDst1 | nDstColor | nDstAlpha;
					} else
						pDst++;
							
					nCurrentX += nStepX;
							
					while( nCurrentX >= 0x10000 ) {
						pSrc++;	
						nSrc = *pSrc;
						nAlpha = nSrc >> 24;
						nCurrentX -= 0x10000;
					}
				}
				pDst = ( uint32 * )( ( ( uint8 * )pDst ) + nDstModulo );
				nCurrentY += nStepY;	
				while( nCurrentY >= 0x10000 ) {
					nSrcY++;
					nCurrentY -= 0x10000;
				}
			}
			break;
		}
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool DisplayDriver::BltBitmap( SrvBitmap * dstbm, SrvBitmap * srcbm, IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha )
{
	LockBitmap( dstbm, srcbm, cSrcRect, cDstRect );
	switch ( nMode )
	{
	case DM_COPY:
		if( cSrcRect.Size() != cDstRect.Size() && dstbm != srcbm )
		{
			blit_convert_stretch_copy( dstbm, srcbm, cSrcRect, cDstRect );
		} else {
			blit_convert_copy( dstbm, srcbm, cSrcRect, cDstRect.LeftTop() );
		}
		break;
	case DM_OVER:
		blit_convert_over( dstbm, srcbm, cSrcRect, cDstRect.LeftTop() );
		break;
	case DM_BLEND:
		if( srcbm->m_eColorSpc == CS_RGB32 || srcbm->m_eColorSpc == CS_RGBA32 )
		{
			if( cSrcRect.Size() != cDstRect.Size() && dstbm != srcbm )
				blit_convert_stretch_alpha( dstbm, srcbm, cSrcRect, cDstRect );
			else
				blit_convert_alpha( dstbm, srcbm, cSrcRect, cDstRect.LeftTop() );
		}
		else if( srcbm->m_eColorSpc == CS_CMAP8 )
		{
			blit_convert_over( dstbm, srcbm, cSrcRect, cDstRect.LeftTop() );
		} 
		else 
		{
			blit_convert_copy( dstbm, srcbm, cSrcRect, cDstRect.LeftTop() );
		}
		break;
	}
	UnlockBitmap( dstbm, srcbm, cSrcRect, cDstRect );
	return ( true );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::RenderGlyph( SrvBitmap * pcBitmap, Glyph * pcGlyph, const IPoint & cPos, const IRect & cClipRect, const Color32_s & sFgColor )
{
	IRect cBounds = pcGlyph->m_cBounds + cPos;
	IRect cRect = cBounds & cClipRect;
	bool bNoAlphaChannel = pcBitmap->m_nFlags & Bitmap::NO_ALPHA_CHANNEL;
	
	if( cRect.IsValid() == false )
	{
		return;
	}
	int sx = cRect.left - cBounds.left;
	int sy = cRect.top - cBounds.top;

	int nWidth = cRect.Width() + 1;
	int nHeight = cRect.Height() + 1;

	int nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;
	
	LockBitmap( pcBitmap, NULL, IRect(), cRect );

	uint8 *pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

	Color32_s sCurCol;
	Color32_s sBgColor;

	if( pcBitmap->m_eColorSpc == CS_RGB16 )
	{
		int nDstModulo = pcBitmap->m_nBytesPerLine / 2 - nWidth;
		uint16 *pDst = ( uint16 * )pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 2;

		uint32 nFgColor = COL_TO_RGB16( sFgColor );
	
		for( int y = 0; y < nHeight; ++y )
		{
			for( int x = 0; x < nWidth; ++x )
			{
				int nAlpha = *pSrc++;
			
				if( nAlpha > 0 )
				{
					if( nAlpha == NUM_FONT_GRAYS - 1 )
					{
						*pDst = nFgColor;
					}
					else
					{
						nAlpha >>= 3;
						
						uint32 nDstColor = *pDst;
						uint32 nSrcColor = ( nFgColor | nFgColor << 16 ) & 0x07e0f81f;
						nDstColor = ( nDstColor | nDstColor << 16 ) & 0x07e0f81f;
						nDstColor += ( nSrcColor - nDstColor ) * nAlpha >> 5;
						nDstColor &= 0x07e0f81f;
						*pDst = nDstColor | nDstColor >> 16;
					}
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	else if( pcBitmap->m_eColorSpc == CS_RGB32 || pcBitmap->m_eColorSpc == CS_RGBA32 )
	{
		int nDstModulo = pcBitmap->m_nBytesPerLine / 4 - nWidth;
		uint32 *pDst = ( uint32 * )pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 4;

		uint32 nFgColor = COL_TO_RGB32( sFgColor );
		
		if( g_bUseMMX )
		{
			mmx_render_glyph_over32( pDst, pSrc, nWidth, nHeight, nFgColor, nDstModulo, nSrcModulo,
									bNoAlphaChannel );
			emms();
			UnlockBitmap( pcBitmap, NULL, IRect(), cRect );
			return;
		}
		

		for( int y = 0; y < nHeight; ++y )
		{
			for( int x = 0; x < nWidth; ++x )
			{
				uint32 nAlpha = *pSrc++;

				if( nAlpha > 0 )
				{
					if( nAlpha == NUM_FONT_GRAYS - 1 )
					{
						if( bNoAlphaChannel )
							*pDst = 0xff000000 | ( nFgColor & 0x00ffffff );
						else {
							uint32 nDstAlpha = *pDst & 0xff000000;
							*pDst = nDstAlpha | ( nFgColor & 0x00ffffff );
						}
					}
					else
					{
						uint32 nDstColor = *pDst;
						uint32 nDstAlpha = nDstColor & 0xff000000;
						uint32 nSrcColor = nFgColor;
						uint32 nSrc1 = nSrcColor & 0xff00ff;
						uint32 nDst1 = nDstColor & 0xff00ff;
						
						nDst1 = ( nDst1 + ( ( nSrc1 - nDst1 ) * nAlpha >> 8 ) ) & 0xff00ff;
						nSrcColor &= 0xff00;
						nDstColor &= 0xff00;
						nDstColor = ( nDstColor + ( ( nSrcColor - nDstColor ) * nAlpha >> 8 ) ) & 0xff00;
						*pDst = nDst1 | nDstColor | nDstAlpha;
					}
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	UnlockBitmap( pcBitmap, NULL, IRect(), cRect );
}

void DisplayDriver::RenderGlyphBlend( SrvBitmap * pcBitmap, Glyph * pcGlyph, const IPoint & cPos, const IRect & cClipRect, const Color32_s & sFgColor )
{
	IRect cBounds = pcGlyph->m_cBounds + cPos;
	IRect cRect = cBounds & cClipRect;

	if( cRect.IsValid() == false )
	{
		return;
	}
	int sx = cRect.left - cBounds.left;
	int sy = cRect.top - cBounds.top;

	int nWidth = cRect.Width() + 1;
	int nHeight = cRect.Height() + 1;

	int nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;
	
	LockBitmap( pcBitmap, NULL, IRect(), cRect );

	uint8 *pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

	int nDstModulo = pcBitmap->m_nBytesPerLine / 4 - nWidth;
	uint32 *pDst = ( uint32 * )pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 4;

	for( int y = 0; y < nHeight; ++y )
	{
		for( int x = 0; x < nWidth; ++x )
		{
			int nAlpha = *pSrc++;
			*pDst++ = COL_TO_RGB32( Color32_s( sFgColor.red, sFgColor.green, sFgColor.blue, int ( sFgColor.alpha * nAlpha ) / 255 ) );
		}
		pSrc += nSrcModulo;
		pDst += nDstModulo;
	}
	UnlockBitmap( pcBitmap, NULL, IRect(), cRect );
}

void DisplayDriver::RenderGlyph( SrvBitmap * pcBitmap, Glyph * pcGlyph, const IPoint & cPos, const IRect & cClipRect, const uint32 *anPallette )
{
	IRect cBounds = pcGlyph->m_cBounds + cPos;
	IRect cRect = cBounds & cClipRect;

	if( cRect.IsValid() == false )
	{
		return;
	}
	int sx = cRect.left - cBounds.left;
	int sy = cRect.top - cBounds.top;

	int nWidth = cRect.Width() + 1;
	int nHeight = cRect.Height() + 1;

	int nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;
	
	LockBitmap( pcBitmap, NULL, IRect(), cRect );

	uint8 *pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

	if( pcBitmap->m_eColorSpc == CS_RGB16 )
	{
		int nDstModulo = pcBitmap->m_nBytesPerLine / 2 - nWidth;
		uint16 *pDst = ( uint16 * )pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 2;
		bool bAddLast = false;
		if( nWidth & 1 )
		{
			bAddLast = true;
			nWidth -= 1;
		}

		for( int y = 0; y < nHeight; ++y )
		{
			
			for( int x = 0; x < nWidth / 2; ++x )
			{
				int nAlpha1 = *pSrc++;
				int nAlpha2 = *pSrc++;
				
				if( nAlpha1 > 0 && nAlpha2 > 0 )
				{
					uint32* pDstConvert = (uint32*)pDst;
					*pDstConvert = anPallette[nAlpha2] << 16 | anPallette[nAlpha1];
					pDst += 2;
				} else 
				{
					if( nAlpha1 > 0 )
						*pDst = anPallette[nAlpha1];
					pDst++;
					if( nAlpha2 > 0 )
						*pDst = anPallette[nAlpha2];
					pDst++;
				}	
			}
			if( bAddLast )
			{
				int nAlpha = *pSrc++;

				if( nAlpha > 0 )
					*pDst = anPallette[nAlpha];
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	else if( pcBitmap->m_eColorSpc == CS_RGB32 || pcBitmap->m_eColorSpc == CS_RGBA32 )
	{
		int nDstModulo = pcBitmap->m_nBytesPerLine / 4 - nWidth;
		uint32 *pDst = ( uint32 * )pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 4;

		for( int y = 0; y < nHeight; ++y )
		{
			for( int x = 0; x < nWidth; ++x )
			{
				int nAlpha = *pSrc++;

				if( nAlpha > 0 )
				{
					*pDst = anPallette[nAlpha];
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	UnlockBitmap( pcBitmap, NULL, IRect(), cRect );
	return;
}

/** Create a new Video overlay.
 * \par Description:
 * Should create a new video overlay.
 * \par
 * \param cSize - Size of the source.
 * \param cDst - Destination position and size.
 * \param eFormat - Format of the overlay.
 * \param phArea - Pointer to the area of the overlay.
 * \return
 * true if the call was successful
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

bool DisplayDriver::CreateVideoOverlay( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *phArea )
{
	return ( false );
}

/** Recreate a Video overlay.
 * \par Description:
 * Should recreate a video overlay.
 * \par
 * \param cSize - Size of the source.
 * \param cDst - Destination position and size.
 * \param eFormat - Format of the overlay.
 * \param phArea - Pointer to the area of the overlay.
 * \return
 * true if the call was successful
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

bool DisplayDriver::RecreateVideoOverlay( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, area_id *phArea )
{
	return ( false );
}

/** Delete a created Video overlay.
 * \par Description:
 * Should delete a video overlay.
 * \par
 * \param phArea - Pointer to the area of the overlay.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

void DisplayDriver::DeleteVideoOverlay( area_id *phArea )
{
}

/** Control call.
 * \par Description:
 * Control calls work like the ioctl function for kernel drivers. They can be used
 * to implement features which are not convered by the display driver api.
 * \par
 * \param nCommand - Command;
 * \param pData - Pointer to the data block.
 * \param nDataSize - Size of the data block.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t DisplayDriver::Control( int nCommand, void* pData, size_t nDataSize )
{
	return( -ENOENT );
}

void DisplayDriver::__VW_reserved1__()
{
}
void DisplayDriver::__VW_reserved2__()
{
}
void DisplayDriver::__VW_reserved3__()
{
}
void DisplayDriver::__VW_reserved4__()
{
}
void DisplayDriver::__VW_reserved5__()
{
}
void DisplayDriver::__VW_reserved6__()
{
}
void DisplayDriver::__VW_reserved7__()
{
}
void DisplayDriver::__VW_reserved8__()
{
}
void DisplayDriver::__VW_reserved9__()
{
}



















