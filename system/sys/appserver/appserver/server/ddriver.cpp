/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

extern "C" {
	#include "ddriver_mmx.h"
}

#include <gui/bitmap.h>

#define RAS_OFFSET8( ptr, x, y, bpl )  (((uint8*)(ptr)) + (x) + (y) * (bpl))
#define RAS_OFFSET16( ptr, x, y, bpl ) ((uint16*)(((uint8*)(ptr)) + (x*2) + (y) * (bpl)))
#define RAS_OFFSET32( ptr, x, y, bpl ) ((uint32*)(((uint8*)(ptr)) + (x*4) + (y) * (bpl)))


Color32_s g_asDefaultPallette[] = {
    Color32_s( 0x00, 0x00, 0x00, 0x00 ),	// 0
    Color32_s( 0x08, 0x08, 0x08, 0x00 ),
    Color32_s( 0x10, 0x10, 0x10, 0x00 ),
    Color32_s( 0x18, 0x18, 0x18, 0x00 ),
    Color32_s( 0x20, 0x20, 0x20, 0x00 ),
    Color32_s( 0x28, 0x28, 0x28, 0x00 ),	// 5
    Color32_s( 0x30, 0x30, 0x30, 0x00 ),
    Color32_s( 0x38, 0x38, 0x38, 0x00 ),
    Color32_s( 0x40, 0x40, 0x40, 0x00 ),
    Color32_s( 0x48, 0x48, 0x48, 0x00 ),
    Color32_s( 0x50, 0x50, 0x50, 0x00 ),	// 10
    Color32_s( 0x58, 0x58, 0x58, 0x00 ),
    Color32_s( 0x60, 0x60, 0x60, 0x00 ),
    Color32_s( 0x68, 0x68, 0x68, 0x00 ),
    Color32_s( 0x70, 0x70, 0x70, 0x00 ),
    Color32_s( 0x78, 0x78, 0x78, 0x00 ),	// 15
    Color32_s( 0x80, 0x80, 0x80, 0x00 ),
    Color32_s( 0x88, 0x88, 0x88, 0x00 ),
    Color32_s( 0x90, 0x90, 0x90, 0x00 ),
    Color32_s( 0x98, 0x98, 0x98, 0x00 ),
    Color32_s( 0xa0, 0xa0, 0xa0, 0x00 ),	// 20
    Color32_s( 0xa8, 0xa8, 0xa8, 0x00 ),
    Color32_s( 0xb0, 0xb0, 0xb0, 0x00 ),
    Color32_s( 0xb8, 0xb8, 0xb8, 0x00 ),
    Color32_s( 0xc0, 0xc0, 0xc0, 0x00 ),
    Color32_s( 0xc8, 0xc8, 0xc8, 0x00 ),	// 25
    Color32_s( 0xd0, 0xd0, 0xd0, 0x00 ),
    Color32_s( 0xd9, 0xd9, 0xd9, 0x00 ),
    Color32_s( 0xe2, 0xe2, 0xe2, 0x00 ),
    Color32_s( 0xeb, 0xeb, 0xeb, 0x00 ),
    Color32_s( 0xf5, 0xf5, 0xf5, 0x00 ),	// 30
    Color32_s( 0xfe, 0xfe, 0xfe, 0x00 ),
    Color32_s( 0x00, 0x00, 0xff, 0x00 ),
    Color32_s( 0x00, 0x00, 0xe5, 0x00 ),
    Color32_s( 0x00, 0x00, 0xcc, 0x00 ),
    Color32_s( 0x00, 0x00, 0xb3, 0x00 ),	// 35
    Color32_s( 0x00, 0x00, 0x9a, 0x00 ),
    Color32_s( 0x00, 0x00, 0x81, 0x00 ),
    Color32_s( 0x00, 0x00, 0x69, 0x00 ),
    Color32_s( 0x00, 0x00, 0x50, 0x00 ),
    Color32_s( 0x00, 0x00, 0x37, 0x00 ),	// 40
    Color32_s( 0x00, 0x00, 0x1e, 0x00 ),
    Color32_s( 0xff, 0x00, 0x00, 0x00 ),
    Color32_s( 0xe4, 0x00, 0x00, 0x00 ),
    Color32_s( 0xcb, 0x00, 0x00, 0x00 ),
    Color32_s( 0xb2, 0x00, 0x00, 0x00 ),	// 45
    Color32_s( 0x99, 0x00, 0x00, 0x00 ),
    Color32_s( 0x80, 0x00, 0x00, 0x00 ),
    Color32_s( 0x69, 0x00, 0x00, 0x00 ),
    Color32_s( 0x50, 0x00, 0x00, 0x00 ),
    Color32_s( 0x37, 0x00, 0x00, 0x00 ),	// 50
    Color32_s( 0x1e, 0x00, 0x00, 0x00 ),
    Color32_s( 0x00, 0xff, 0x00, 0x00 ),
    Color32_s( 0x00, 0xe4, 0x00, 0x00 ),
    Color32_s( 0x00, 0xcb, 0x00, 0x00 ),
    Color32_s( 0x00, 0xb2, 0x00, 0x00 ),	// 55
    Color32_s( 0x00, 0x99, 0x00, 0x00 ),
    Color32_s( 0x00, 0x80, 0x00, 0x00 ),
    Color32_s( 0x00, 0x69, 0x00, 0x00 ),
    Color32_s( 0x00, 0x50, 0x00, 0x00 ),
    Color32_s( 0x00, 0x37, 0x00, 0x00 ),	// 60
    Color32_s( 0x00, 0x1e, 0x00, 0x00 ),
    Color32_s( 0x00, 0x98, 0x33, 0x00 ),
    Color32_s( 0xff, 0xff, 0xff, 0x00 ),
    Color32_s( 0xcb, 0xff, 0xff, 0x00 ),
    Color32_s( 0xcb, 0xff, 0xcb, 0x00 ),	// 65
    Color32_s( 0xcb, 0xff, 0x98, 0x00 ),
    Color32_s( 0xcb, 0xff, 0x66, 0x00 ),
    Color32_s( 0xcb, 0xff, 0x33, 0x00 ),
    Color32_s( 0xcb, 0xff, 0x00, 0x00 ),
    Color32_s( 0x98, 0xff, 0xff, 0x00 ),
    Color32_s( 0x98, 0xff, 0xcb, 0x00 ),
    Color32_s( 0x98, 0xff, 0x98, 0x00 ),
    Color32_s( 0x98, 0xff, 0x66, 0x00 ),
    Color32_s( 0x98, 0xff, 0x33, 0x00 ),
    Color32_s( 0x98, 0xff, 0x00, 0x00 ),
    Color32_s( 0x66, 0xff, 0xff, 0x00 ),
    Color32_s( 0x66, 0xff, 0xcb, 0x00 ),
    Color32_s( 0x66, 0xff, 0x98, 0x00 ),
    Color32_s( 0x66, 0xff, 0x66, 0x00 ),
    Color32_s( 0x66, 0xff, 0x33, 0x00 ),
    Color32_s( 0x66, 0xff, 0x00, 0x00 ),
    Color32_s( 0x33, 0xff, 0xff, 0x00 ),
    Color32_s( 0x33, 0xff, 0xcb, 0x00 ),
    Color32_s( 0x33, 0xff, 0x98, 0x00 ),
    Color32_s( 0x33, 0xff, 0x66, 0x00 ),
    Color32_s( 0x33, 0xff, 0x33, 0x00 ),
    Color32_s( 0x33, 0xff, 0x00, 0x00 ),
    Color32_s( 0xff, 0x98, 0xff, 0x00 ),
    Color32_s( 0xff, 0x98, 0xcb, 0x00 ),
    Color32_s( 0xff, 0x98, 0x98, 0x00 ),
    Color32_s( 0xff, 0x98, 0x66, 0x00 ),
    Color32_s( 0xff, 0x98, 0x33, 0x00 ),
    Color32_s( 0xff, 0x98, 0x00, 0x00 ),
    Color32_s( 0x00, 0x66, 0xff, 0x00 ),
    Color32_s( 0x00, 0x66, 0xcb, 0x00 ),
    Color32_s( 0xcb, 0xcb, 0xff, 0x00 ),
    Color32_s( 0xcb, 0xcb, 0xcb, 0x00 ),
    Color32_s( 0xcb, 0xcb, 0x98, 0x00 ),
    Color32_s( 0xcb, 0xcb, 0x66, 0x00 ),
    Color32_s( 0xcb, 0xcb, 0x33, 0x00 ),
    Color32_s( 0xcb, 0xcb, 0x00, 0x00 ),
    Color32_s( 0x98, 0xcb, 0xff, 0x00 ),
    Color32_s( 0x98, 0xcb, 0xcb, 0x00 ),
    Color32_s( 0x98, 0xcb, 0x98, 0x00 ),
    Color32_s( 0x98, 0xcb, 0x66, 0x00 ),
    Color32_s( 0x98, 0xcb, 0x33, 0x00 ),
    Color32_s( 0x98, 0xcb, 0x00, 0x00 ),
    Color32_s( 0x66, 0xcb, 0xff, 0x00 ),
    Color32_s( 0x66, 0xcb, 0xcb, 0x00 ),
    Color32_s( 0x66, 0xcb, 0x98, 0x00 ),
    Color32_s( 0x66, 0xcb, 0x66, 0x00 ),
    Color32_s( 0x66, 0xcb, 0x33, 0x00 ),
    Color32_s( 0x66, 0xcb, 0x00, 0x00 ),
    Color32_s( 0x33, 0xcb, 0xff, 0x00 ),
    Color32_s( 0x33, 0xcb, 0xcb, 0x00 ),
    Color32_s( 0x33, 0xcb, 0x98, 0x00 ),
    Color32_s( 0x33, 0xcb, 0x66, 0x00 ),
    Color32_s( 0x33, 0xcb, 0x33, 0x00 ),
    Color32_s( 0x33, 0xcb, 0x00, 0x00 ),
    Color32_s( 0xff, 0x66, 0xff, 0x00 ),
    Color32_s( 0xff, 0x66, 0xcb, 0x00 ),
    Color32_s( 0xff, 0x66, 0x98, 0x00 ),
    Color32_s( 0xff, 0x66, 0x66, 0x00 ),
    Color32_s( 0xff, 0x66, 0x33, 0x00 ),
    Color32_s( 0xff, 0x66, 0x00, 0x00 ),
    Color32_s( 0x00, 0x66, 0x98, 0x00 ),
    Color32_s( 0x00, 0x66, 0x66, 0x00 ),
    Color32_s( 0xcb, 0x98, 0xff, 0x00 ),
    Color32_s( 0xcb, 0x98, 0xcb, 0x00 ),
    Color32_s( 0xcb, 0x98, 0x98, 0x00 ),
    Color32_s( 0xcb, 0x98, 0x66, 0x00 ),
    Color32_s( 0xcb, 0x98, 0x33, 0x00 ),
    Color32_s( 0xcb, 0x98, 0x00, 0x00 ),
    Color32_s( 0x98, 0x98, 0xff, 0x00 ),
    Color32_s( 0x98, 0x98, 0xcb, 0x00 ),
    Color32_s( 0x98, 0x98, 0x98, 0x00 ),
    Color32_s( 0x98, 0x98, 0x66, 0x00 ),
    Color32_s( 0x98, 0x98, 0x33, 0x00 ),
    Color32_s( 0x98, 0x98, 0x00, 0x00 ),
    Color32_s( 0x66, 0x98, 0xff, 0x00 ),
    Color32_s( 0x66, 0x98, 0xcb, 0x00 ),
    Color32_s( 0x66, 0x98, 0x98, 0x00 ),
    Color32_s( 0x66, 0x98, 0x66, 0x00 ),
    Color32_s( 0x66, 0x98, 0x33, 0x00 ),
    Color32_s( 0x66, 0x98, 0x00, 0x00 ),
    Color32_s( 0x33, 0x98, 0xff, 0x00 ),
    Color32_s( 0x33, 0x98, 0xcb, 0x00 ),
    Color32_s( 0x33, 0x98, 0x98, 0x00 ),
    Color32_s( 0x33, 0x98, 0x66, 0x00 ),
    Color32_s( 0x33, 0x98, 0x33, 0x00 ),
    Color32_s( 0x33, 0x98, 0x00, 0x00 ),
    Color32_s( 0xe6, 0x86, 0x00, 0x00 ),
    Color32_s( 0xff, 0x33, 0xcb, 0x00 ),
    Color32_s( 0xff, 0x33, 0x98, 0x00 ),
    Color32_s( 0xff, 0x33, 0x66, 0x00 ),
    Color32_s( 0xff, 0x33, 0x33, 0x00 ),
    Color32_s( 0xff, 0x33, 0x00, 0x00 ),
    Color32_s( 0x00, 0x66, 0x33, 0x00 ),
    Color32_s( 0x00, 0x66, 0x00, 0x00 ),
    Color32_s( 0xcb, 0x66, 0xff, 0x00 ),
    Color32_s( 0xcb, 0x66, 0xcb, 0x00 ),
    Color32_s( 0xcb, 0x66, 0x98, 0x00 ),
    Color32_s( 0xcb, 0x66, 0x66, 0x00 ),
    Color32_s( 0xcb, 0x66, 0x33, 0x00 ),
    Color32_s( 0xcb, 0x66, 0x00, 0x00 ),
    Color32_s( 0x98, 0x66, 0xff, 0x00 ),
    Color32_s( 0x98, 0x66, 0xcb, 0x00 ),
    Color32_s( 0x98, 0x66, 0x98, 0x00 ),
    Color32_s( 0x98, 0x66, 0x66, 0x00 ),
    Color32_s( 0x98, 0x66, 0x33, 0x00 ),
    Color32_s( 0x98, 0x66, 0x00, 0x00 ),
    Color32_s( 0x66, 0x66, 0xff, 0x00 ),
    Color32_s( 0x66, 0x66, 0xcb, 0x00 ),
    Color32_s( 0x66, 0x66, 0x98, 0x00 ),
    Color32_s( 0x66, 0x66, 0x66, 0x00 ),
    Color32_s( 0x66, 0x66, 0x33, 0x00 ),
    Color32_s( 0x66, 0x66, 0x00, 0x00 ),
    Color32_s( 0x33, 0x66, 0xff, 0x00 ),
    Color32_s( 0x33, 0x66, 0xcb, 0x00 ),
    Color32_s( 0x33, 0x66, 0x98, 0x00 ),
    Color32_s( 0x33, 0x66, 0x66, 0x00 ),
    Color32_s( 0x33, 0x66, 0x33, 0x00 ),
    Color32_s( 0x33, 0x66, 0x00, 0x00 ),
    Color32_s( 0xff, 0x00, 0xff, 0x00 ),
    Color32_s( 0xff, 0x00, 0xcb, 0x00 ),
    Color32_s( 0xff, 0x00, 0x98, 0x00 ),
    Color32_s( 0xff, 0x00, 0x66, 0x00 ),
    Color32_s( 0xff, 0x00, 0x33, 0x00 ),
    Color32_s( 0xff, 0xaf, 0x13, 0x00 ),
    Color32_s( 0x00, 0x33, 0xff, 0x00 ),
    Color32_s( 0x00, 0x33, 0xcb, 0x00 ),
    Color32_s( 0xcb, 0x33, 0xff, 0x00 ),
    Color32_s( 0xcb, 0x33, 0xcb, 0x00 ),
    Color32_s( 0xcb, 0x33, 0x98, 0x00 ),
    Color32_s( 0xcb, 0x33, 0x66, 0x00 ),
    Color32_s( 0xcb, 0x33, 0x33, 0x00 ),
    Color32_s( 0xcb, 0x33, 0x00, 0x00 ),
    Color32_s( 0x98, 0x33, 0xff, 0x00 ),
    Color32_s( 0x98, 0x33, 0xcb, 0x00 ),
    Color32_s( 0x98, 0x33, 0x98, 0x00 ),
    Color32_s( 0x98, 0x33, 0x66, 0x00 ),
    Color32_s( 0x98, 0x33, 0x33, 0x00 ),
    Color32_s( 0x98, 0x33, 0x00, 0x00 ),
    Color32_s( 0x66, 0x33, 0xff, 0x00 ),
    Color32_s( 0x66, 0x33, 0xcb, 0x00 ),
    Color32_s( 0x66, 0x33, 0x98, 0x00 ),
    Color32_s( 0x66, 0x33, 0x66, 0x00 ),
    Color32_s( 0x66, 0x33, 0x33, 0x00 ),
    Color32_s( 0x66, 0x33, 0x00, 0x00 ),
    Color32_s( 0x33, 0x33, 0xff, 0x00 ),
    Color32_s( 0x33, 0x33, 0xcb, 0x00 ),
    Color32_s( 0x33, 0x33, 0x98, 0x00 ),
    Color32_s( 0x33, 0x33, 0x66, 0x00 ),
    Color32_s( 0x33, 0x33, 0x33, 0x00 ),
    Color32_s( 0x33, 0x33, 0x00, 0x00 ),
    Color32_s( 0xff, 0xcb, 0x66, 0x00 ),
    Color32_s( 0xff, 0xcb, 0x98, 0x00 ),
    Color32_s( 0xff, 0xcb, 0xcb, 0x00 ),
    Color32_s( 0xff, 0xcb, 0xff, 0x00 ),
    Color32_s( 0x00, 0x33, 0x98, 0x00 ),
    Color32_s( 0x00, 0x33, 0x66, 0x00 ),
    Color32_s( 0x00, 0x33, 0x33, 0x00 ),
    Color32_s( 0x00, 0x33, 0x00, 0x00 ),
    Color32_s( 0xcb, 0x00, 0xff, 0x00 ),
    Color32_s( 0xcb, 0x00, 0xcb, 0x00 ),
    Color32_s( 0xcb, 0x00, 0x98, 0x00 ),
    Color32_s( 0xcb, 0x00, 0x66, 0x00 ),
    Color32_s( 0xcb, 0x00, 0x33, 0x00 ),
    Color32_s( 0xff, 0xe3, 0x46, 0x00 ),
    Color32_s( 0x98, 0x00, 0xff, 0x00 ),
    Color32_s( 0x98, 0x00, 0xcb, 0x00 ),
    Color32_s( 0x98, 0x00, 0x98, 0x00 ),
    Color32_s( 0x98, 0x00, 0x66, 0x00 ),
    Color32_s( 0x98, 0x00, 0x33, 0x00 ),
    Color32_s( 0x98, 0x00, 0x00, 0x00 ),
    Color32_s( 0x66, 0x00, 0xff, 0x00 ),
    Color32_s( 0x66, 0x00, 0xcb, 0x00 ),
    Color32_s( 0x66, 0x00, 0x98, 0x00 ),
    Color32_s( 0x66, 0x00, 0x66, 0x00 ),
    Color32_s( 0x66, 0x00, 0x33, 0x00 ),
    Color32_s( 0x66, 0x00, 0x00, 0x00 ),
    Color32_s( 0x33, 0x00, 0xff, 0x00 ),
    Color32_s( 0x33, 0x00, 0xcb, 0x00 ),
    Color32_s( 0x33, 0x00, 0x98, 0x00 ),
    Color32_s( 0x33, 0x00, 0x66, 0x00 ),
    Color32_s( 0x33, 0x00, 0x33, 0x00 ),
    Color32_s( 0x33, 0x00, 0x00, 0x00 ),
    Color32_s( 0xff, 0xcb, 0x33, 0x00 ),
    Color32_s( 0xff, 0xcb, 0x00, 0x00 ),
    Color32_s( 0xff, 0xff, 0x00, 0x00 ),
    Color32_s( 0xff, 0xff, 0x33, 0x00 ),
    Color32_s( 0xff, 0xff, 0x66, 0x00 ),
    Color32_s( 0xff, 0xff, 0x98, 0x00 ),
    Color32_s( 0xff, 0xff, 0xcb, 0x00 ),	
    Color32_s( 0xff, 0xff, 0xff, 0xff )
};

static bool g_bUseMMX = false;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DisplayDriver::DisplayDriver() : m_cCursorHotSpot(0,0)
{
    m_pcMouseImage = NULL;
    m_pcMouseSprite = NULL;
    
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
 *	address to the actual start of the frame-buffer. This should
 *	normally be 0 but might be between 0-4095 if the frame-buffer
 *	is not page-aligned (Like when running under WMVare).
 * \par Note:
 *	The driver should try to avoid using this offset since it will give
 *	user-space applications access to the memory area between the start
 *	of the raster-area and the start of the bitmap. 
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


int DisplayDriver::GetFramebufferOffset()
{
    return( 0 );
}

void DisplayDriver::SetCursorBitmap( mouse_ptr_mode eMode, const IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight )
{
    SrvSprite::Hide();
    
    m_cCursorHotSpot = cHotSpot;
    delete m_pcMouseSprite;
    
    if ( m_pcMouseImage != NULL ) {
	m_pcMouseImage->Release();
    }
    m_pcMouseImage = new SrvBitmap( nWidth, nHeight, CS_CMAP8 );

    uint8*       pDstRaster = m_pcMouseImage->m_pRaster;
    const uint8* pSrcRaster = static_cast<const uint8*>(pRaster);
    
    for ( int i = 0 ; i < nWidth * nHeight ; ++i ) {
	uint8 anPalette[] = { 255, 0, 0, 63 };
	if ( pSrcRaster[i] < 4 ) {
	    pDstRaster[i] = anPalette[ pSrcRaster[i] ];
	} else {
	    pDstRaster[i] = 255;
	}
    }
    if ( m_pcMouseImage != NULL ) {
	m_pcMouseSprite = new SrvSprite( IRect( 0, 0, nWidth - 1, nHeight - 1 ), m_cMousePos, m_cCursorHotSpot,
					 g_pcScreenBitmap, m_pcMouseImage );
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
    if ( m_pcMouseSprite == NULL ) {
	m_pcMouseSprite = new SrvSprite( IRect( 0, 0, m_pcMouseImage->m_nWidth - 1, m_pcMouseImage->m_nHeight - 1 ), m_cMousePos, m_cCursorHotSpot,
					 g_pcScreenBitmap, m_pcMouseImage );
    } else {
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
    if ( m_pcMouseSprite == NULL ) {
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
    if ( m_pcMouseSprite != NULL ) {
	m_pcMouseSprite->MoveTo( cNewPos );
    }
    m_cMousePos = cNewPos;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::FillBlit8( uint8* pDst, int nMod, int W,int H, int nColor )
{
    int X,Y;

    for ( Y = 0 ; Y < H ; Y++ ) {
	for ( X = 0 ; X < W ; X++ ) {
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

void DisplayDriver::FillBlit16( uint16 *pDst, int nMod, int W, int H, uint32 nColor )
{
    int X,Y;

    for ( Y = 0 ; Y < H ; Y++ ) {
	for ( X = 0 ; X < W ; X++ ) {
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

void DisplayDriver::FillBlit24( uint8* pDst, int nMod, int W, int H, uint32 nColor )
{
    int X,Y;

    for ( Y = 0 ; Y < H ; Y++ ) {
	for ( X = 0 ; X < W ; X++ ) {
	    *pDst++ = nColor & 0xff;
	    *((uint16*)pDst) = nColor >> 8;
	    pDst	+= 2;
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
    int X,Y;

    for ( Y = 0 ; Y < H ; Y++ ) {
	for ( X = 0 ; X < W ; X++ ) {
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

bool DisplayDriver::FillRect( SrvBitmap* pcBitmap, const IRect& cRect, const Color32_s& sColor )
{
    int BltX,BltY,BltW,BltH;

    BltX = cRect.left;
    BltY = cRect.top;
    BltW = cRect.Width() + 1;
    BltH = cRect.Height() + 1;

    switch( pcBitmap->m_eColorSpc )
    {
//    case CS_CMAP8:
//      FillBlit8( pcBitmap->m_pRaster + ((BltY * pcBitmap->m_nBytesPerLine) + BltX),
//		 pcBitmap->m_nBytesPerLine - BltW, BltW, BltH, nColor );
//      break;
	case CS_RGB15:
	    FillBlit16( (uint16*) &pcBitmap->m_pRaster[ BltY * pcBitmap->m_nBytesPerLine + BltX * 2 ],
			pcBitmap->m_nBytesPerLine / 2 - BltW, BltW, BltH, COL_TO_RGB15( sColor ) );
	    break;
	case CS_RGB16:
	    FillBlit16( (uint16*) &pcBitmap->m_pRaster[ BltY * pcBitmap->m_nBytesPerLine + BltX * 2 ],
			pcBitmap->m_nBytesPerLine / 2 - BltW, BltW, BltH, COL_TO_RGB16( sColor ) );
	    break;
//    case CS_RGB24:
//      FillBlit24( &pcBitmap->m_pRaster[ BltY * pcBitmap->m_nBytesPerLine + BltX * 3 ],
//		  pcBitmap->m_nBytesPerLine - BltW * 3, BltW, BltH, nColor );
//      break;
	case CS_RGB32:
	    FillBlit32( (uint32*) &pcBitmap->m_pRaster[ BltY * pcBitmap->m_nBytesPerLine + BltX * 4 ],
			pcBitmap->m_nBytesPerLine / 4 - BltW, BltW, BltH, COL_TO_RGB32( sColor ) );
	    break;
	default:
	    dbprintf( "DisplayDriver::FillRect() unknown color space %d\n", pcBitmap->m_eColorSpc );
    }

    return( true );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline void Blit( uint8 *Src, uint8* Dst, int SMod, int DMod, int W, int H, bool Rev )
{
    int	  i;
    int 	  X,Y;
    uint32* LSrc;
    uint32* LDst;
  
    if (Rev)
    {
	for ( Y = 0 ; Y < H ; Y++ )
	{
	    for( X = 0 ; (X < W) && ((uint32(Src - 3)) & 3) ; X++ ) {
		*Dst-- = *Src--;
	    }

	    LSrc=(uint32*)(((uint32)Src)-3);
	    LDst=(uint32*)(((uint32)Dst)-3);

	    i = (W - X) / 4;

	    X += i * 4;

	    for ( ; i ; i-- ) {
		*LDst--=*LSrc--;
	    }

	    Src=(uint8*)(((uint32)LSrc)+3);
	    Dst=(uint8*)(((uint32)LDst)+3);

	    for ( ; X < W ; X++ ) {
		*Dst--=*Src--;
	    }

	    Dst-= (int32)DMod;
	    Src-= (int32)SMod;
	}
    }
    else
    {
	for ( Y = 0 ; Y < H ; Y++ )
	{
	    for ( X = 0 ; (X < W) && (((uint32)Src) & 3) ; ++X ) {
		*Dst++ = *Src++;
	    }

	    LSrc = (uint32*) Src;
	    LDst = (uint32*) Dst;

	    i = (W - X) / 4;

	    X += i * 4;

	    for( ; i ; i-- ) {
		*LDst++ = *LSrc++;
	    }

	    Src	=	(uint8*) LSrc;
	    Dst	=	(uint8*) LDst;

	    for ( ; X < W ; X++ ) {
		*Dst++=*Src++;
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

static inline void BitBlit( SrvBitmap *sbm, SrvBitmap *dbm,
			    int sx, int sy, int dx, int dy, int w, int h )
{
    int	Smod,Dmod;
    int	BytesPerPix = 1;

    int	InPtr,OutPtr;

    int nBitsPerPix = BitsPerPixel( dbm->m_eColorSpc );

    if ( nBitsPerPix == 15 ) {
	BytesPerPix = 2;
    } else {
	BytesPerPix = nBitsPerPix / 8;
    }

    sx *= BytesPerPix;
    dx *= BytesPerPix;
    w  *= BytesPerPix;

    if ( sx >= dx )
    {
	if ( sy >= dy )
	{
	    Smod = sbm->m_nBytesPerLine - w;
	    Dmod = dbm->m_nBytesPerLine - w;
	    InPtr  = sy * sbm->m_nBytesPerLine + sx;
	    OutPtr = dy * dbm->m_nBytesPerLine + dx;

	    Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, false );
	}
	else
	{
	    Smod	=-sbm->m_nBytesPerLine - w;
	    Dmod	=-dbm->m_nBytesPerLine - w;
	    InPtr	= ((sy+h-1)*sbm->m_nBytesPerLine)+sx;
	    OutPtr	= ((dy+h-1)*dbm->m_nBytesPerLine)+dx;

	    Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, false );
	}
    }
    else
    {
	if ( sy > dy )
	{
	    Smod=-(sbm->m_nBytesPerLine + w);
	    Dmod=-(dbm->m_nBytesPerLine + w);
	    InPtr		= (sy*sbm->m_nBytesPerLine)+sx+w-1;
	    OutPtr	= (dy*dbm->m_nBytesPerLine)+dx+w-1;
	    Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, true );
	}
	else
	{
	    Smod = sbm->m_nBytesPerLine - w;
	    Dmod = dbm->m_nBytesPerLine - w;
	    InPtr		= (sy + h - 1) * sbm->m_nBytesPerLine + sx + w - 1;
	    OutPtr	= (dy + h - 1) * dbm->m_nBytesPerLine + dx + w - 1;
	    Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, true );
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------


static inline void blit_convert_copy( SrvBitmap* pcDst, SrvBitmap* pcSrc,
				      const IRect& cSrcRect, const IPoint& cDstPos )
{
    switch( (int)pcSrc->m_eColorSpc )
    {
	case CS_CMAP8:
	{
	    uint8* pSrc = RAS_OFFSET8( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
      
	    int nSrcModulo = pcSrc->m_nBytesPerLine - (cSrcRect.Width() + 1);
     
	    switch( (int)pcDst->m_eColorSpc )
	    {
	    case CS_CMAP8:
		{
			if( pcSrc == pcDst )
			{
				BitBlit( pcSrc, pcDst, cSrcRect.left, cSrcRect.top, cDstPos.x,
						cDstPos.y, cSrcRect.Width() + 1, cSrcRect.Height() + 1 );
				break;
			}
			
		    uint8* pDst = RAS_OFFSET8( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1);
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
		    memcpy( (uint8*)pDst, (uint8*)pSrc, ( cSrcRect.right - cSrcRect.left + 1 ) );
		    pSrc += ( cSrcRect.right - cSrcRect.left + 1 );
		    pDst += ( cSrcRect.right - cSrcRect.left + 1 );
			pSrc = (uint8*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint8*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB15:
		case CS_RGBA15:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
	  
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB15( g_asDefaultPallette[*pSrc++] );
			}
			pSrc += nSrcModulo;
			pDst  = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB16:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
	  
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB16( g_asDefaultPallette[*pSrc++] );
			}
			pSrc += nSrcModulo;
			pDst  = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB32:
		case CS_RGBA32:
		{
		    uint32* pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB32( g_asDefaultPallette[*pSrc++] );
			}
			pSrc += nSrcModulo;
			pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
	    }
	    break;
	}
	case CS_RGB15:
	case CS_RGBA15:
	{
	    uint16* pSrc = RAS_OFFSET16( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
	    int nSrcModulo = pcSrc->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
     
	    switch( (int)pcDst->m_eColorSpc )
	    {
	    case CS_RGB15:
	    case CS_RGBA15:
		{
			if( pcSrc == pcDst )
			{
				BitBlit( pcSrc, pcDst, cSrcRect.left, cSrcRect.top, cDstPos.x,
						cDstPos.y, cSrcRect.Width() + 1, cSrcRect.Height() + 1 );
				break;
			}
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
		    memcpy( (uint8*)pDst, (uint8*)pSrc, 2 * ( cSrcRect.right - cSrcRect.left + 1 ) );
		    pSrc += ( cSrcRect.right - cSrcRect.left + 1 );
		    pDst += ( cSrcRect.right - cSrcRect.left + 1 );
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB16:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB16( RGB15_TO_COL( *pSrc++ ) );
			}
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB32:
		case CS_RGBA32:
		{
		    uint32* pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB32( RGB15_TO_COL( *pSrc++ ) );
			}
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
	    }
	    break;
	}
	case CS_RGB16:
	{
	    uint16* pSrc = RAS_OFFSET16( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
	    int nSrcModulo = pcSrc->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
     
	    switch( (int)pcDst->m_eColorSpc )
	    {
		case CS_RGB15:
		case CS_RGBA15:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB15( RGB16_TO_COL( *pSrc++ ) );
			}
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB16:
		{
			
			if( pcSrc == pcDst )
			{
				BitBlit( pcSrc, pcDst, cSrcRect.left, cSrcRect.top, cDstPos.x,
						cDstPos.y, cSrcRect.Width() + 1, cSrcRect.Height() + 1 );
				break;
			}
			
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    		
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
		    memcpy( (uint8*)pDst, (uint8*)pSrc, 2 * ( cSrcRect.right - cSrcRect.left + 1 ) );
		    pSrc += ( cSrcRect.right - cSrcRect.left + 1 );
		    pDst += ( cSrcRect.right - cSrcRect.left + 1 );
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }		   
		    break;
		}
		case CS_RGB32:
		case CS_RGBA32:
		{
		    uint32* pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB32( RGB16_TO_COL( *pSrc++ ) );
			}
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
	    }
	    break;
	}
	case CS_RGB32:
	case CS_RGBA32:
	{
	    uint32* pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
	    int nSrcModulo = pcSrc->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
     
	    switch( (int)pcDst->m_eColorSpc )
	    {
		case CS_RGB15:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB15( RGB32_TO_COL( *pSrc++ ) );
			}
			pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB16:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;

		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
				if( g_bUseMMX ) {
		    		mmx_rgb32_to_rgb16( (uint8*)pSrc, (uint8*)pDst, cSrcRect.Width() + 1 );
		    		pSrc += cSrcRect.Width() + 1;
		    		pDst += cSrcRect.Width() + 1;
		   		} else {
					for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    		*pDst++ = COL_TO_RGB16( RGB32_TO_COL( *pSrc++ ) );
					}
				}
				pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
				pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB32:
		{
			if( pcSrc == pcDst )
			{
				BitBlit( pcSrc, pcDst, cSrcRect.left, cSrcRect.top, cDstPos.x,
						cDstPos.y, cSrcRect.Width() + 1, cSrcRect.Height() + 1 );
				break;
			}
		    uint32* pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
		    memcpy( (uint8*)pDst, (uint8*)pSrc, 4 * ( cSrcRect.right - cSrcRect.left + 1 ) );
		    pSrc += ( cSrcRect.right - cSrcRect.left + 1 );
		    pDst += ( cSrcRect.right - cSrcRect.left + 1 );
			pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
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

static inline void blit_convert_over( SrvBitmap* pcDst, SrvBitmap* pcSrc,
				      const IRect& cSrcRect, const IPoint& cDstPos )
{
    switch( (int)pcSrc->m_eColorSpc )
    {
	case CS_CMAP8:
	{
	    uint8* pSrc = RAS_OFFSET8( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
      
	    int nSrcModulo = pcSrc->m_nBytesPerLine - (cSrcRect.Width()+1);
     
	    switch( (int)pcDst->m_eColorSpc )
	    {
		case CS_RGB15:
		case CS_RGBA15:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
	  
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    int nPix = *pSrc++;
			    if ( nPix != TRANSPARENT_CMAP8 ) {
				*pDst = COL_TO_RGB15( g_asDefaultPallette[nPix] );
			    }
			    pDst++;
			}
			pSrc += nSrcModulo;
			pDst  = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB16:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    int nPix = *pSrc++;
			    if ( nPix != TRANSPARENT_CMAP8 ) {
				*pDst = COL_TO_RGB16( g_asDefaultPallette[nPix] );
			    }
			    pDst++;
			}
			pSrc += nSrcModulo;
			pDst  = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB32:
		case CS_RGBA32:
		{
		    uint32* pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;

		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    int nPix = *pSrc++;
			    if ( nPix != TRANSPARENT_CMAP8 ) {
				*pDst = COL_TO_RGB32( g_asDefaultPallette[nPix] );
			    }
			    pDst++;
			}
			pSrc += nSrcModulo;
			pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		default:
		    dbprintf( "blit_convert_over() unknown dst colorspace for 8 bit src %d\n", pcDst->m_eColorSpc );
		    break;
	    }
	    break;
	}
	case CS_RGB15:
	case CS_RGBA15:
	{
	    uint16* pSrc = RAS_OFFSET16( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
	    int nSrcModulo = pcSrc->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
     
	    switch( (int)pcDst->m_eColorSpc )
	    {
		case CS_RGB16:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB16( RGB15_TO_COL( *pSrc++ ) );
			}
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB32:
		case CS_RGBA32:
		{
		    uint32* pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
	  
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB32( RGB15_TO_COL( *pSrc++ ) );
			}
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
	    }
	    break;
	}
	case CS_RGB16:
	{
	    uint16* pSrc = RAS_OFFSET16( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
	    int nSrcModulo = pcSrc->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
     
	    switch( (int)pcDst->m_eColorSpc )
	    {
		case CS_RGB15:
		case CS_RGBA15:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB15( RGB16_TO_COL( *pSrc++ ) );
			}
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB32:
		case CS_RGBA32:
		{
		    uint32* pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    *pDst++ = COL_TO_RGB32( RGB16_TO_COL( *pSrc++ ) );
			}
			pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
	    }
	    break;
	}
	case CS_RGB32:
	case CS_RGBA32:
	{
	    uint32* pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
	    int nSrcModulo = pcSrc->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
     
	    switch( (int)pcDst->m_eColorSpc )
	    {
		case CS_RGB16:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;

		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    uint32 nPix = *pSrc++;
			    if ( nPix != 0xffffffff ) {
				*pDst = COL_TO_RGB16( RGB32_TO_COL( nPix ) );
			    }
			    pDst++;
			}
			pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB15:
		{
		    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    uint32 nPix = *pSrc++;
			    if ( nPix != 0xffffffff ) {
				*pDst = COL_TO_RGB15( RGB32_TO_COL( nPix ) );
			    }
			    pDst++;
			}
			pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
		    }
		    break;
		}
		case CS_RGB32:
		case CS_RGBA32:
		{
		    uint32* pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
		    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
    
		    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
			for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
			    uint32 nPix = *pSrc++;
			    if ( nPix != 0xffffffff ) {
				*pDst = nPix;
			    }
			    pDst++;
			}
			pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
			pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
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

static inline void blit_convert_alpha( SrvBitmap* pcDst, SrvBitmap* pcSrc,
				       const IRect& cSrcRect, const IPoint& cDstPos )
{
    uint32* pSrc = RAS_OFFSET32( pcSrc->m_pRaster, cSrcRect.left, cSrcRect.top, pcSrc->m_nBytesPerLine );
    int nSrcModulo = pcSrc->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
     
    switch( (int)pcDst->m_eColorSpc )
    {
	case CS_RGB16:
	{
	    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
	    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;

	    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
		for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
		    Color32_s sSrcColor = RGB32_TO_COL( *pSrc++ );

		    int nAlpha = sSrcColor.alpha;
		    if ( nAlpha == 0xff ) {
			*pDst = COL_TO_RGB16( sSrcColor );
		    } else if ( nAlpha != 0x00 ) {
			Color32_s sDstColor = RGB16_TO_COL( *pDst );
			*pDst = COL_TO_RGB16(
			    Color32_s( sDstColor.red   * (256-nAlpha) / 256 + sSrcColor.red   * nAlpha / 256,
				       sDstColor.green * (256-nAlpha) / 256 + sSrcColor.green * nAlpha / 256,
				       sDstColor.blue  * (256-nAlpha) / 256 + sSrcColor.blue  * nAlpha / 256,
				       0 ) );
		    }
		    pDst++;
		}
		pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
		pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
	    }
	    break;
	}
	case CS_RGB15:
	{
	    uint16* pDst = RAS_OFFSET16( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
	    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 2;
    
	    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
		for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
		    Color32_s sSrcColor = RGB32_TO_COL( *pSrc++ );

		    int nAlpha = sSrcColor.alpha;
		    if ( nAlpha == 0xff ) {
			*pDst = COL_TO_RGB15( sSrcColor );
		    } else if ( nAlpha != 0x00 ) {
			Color32_s sDstColor = RGB15_TO_COL( *pDst );
			*pDst = COL_TO_RGB15(
			    Color32_s( sDstColor.red   * (256-nAlpha) / 256 + sSrcColor.red   * nAlpha / 256,
				       sDstColor.green * (256-nAlpha) / 256 + sSrcColor.green * nAlpha / 256,
				       sDstColor.blue  * (256-nAlpha) / 256 + sSrcColor.blue  * nAlpha / 256,
				       0 ) );
		    }
		    pDst++;
		}
		pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
		pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
	    }
	    break;
	}
	case CS_RGB32:
	{
	    uint32* pDst = RAS_OFFSET32( pcDst->m_pRaster, cDstPos.x, cDstPos.y, pcDst->m_nBytesPerLine );
	    int nDstModulo = pcDst->m_nBytesPerLine - (cSrcRect.Width()+1) * 4;
    
	    for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
		for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
		    Color32_s sSrcColor = RGB32_TO_COL( *pSrc++ );

		    int nAlpha = sSrcColor.alpha;
		    if ( nAlpha == 0xff ) {
			*pDst = COL_TO_RGB32( sSrcColor );
		    } else if ( nAlpha != 0x00 ) {
			Color32_s sDstColor = RGB32_TO_COL( *pDst );
			*pDst = COL_TO_RGB32(
			    Color32_s( sDstColor.red   * (256-nAlpha) / 256 + sSrcColor.red   * nAlpha / 256,
				       sDstColor.green * (256-nAlpha) / 256 + sSrcColor.green * nAlpha / 256,
				       sDstColor.blue  * (256-nAlpha) / 256 + sSrcColor.blue  * nAlpha / 256,
				       0 ) );
		    }
		    pDst++;
		}
		pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
		pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
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

bool DisplayDriver::BltBitmap( SrvBitmap *dstbm, SrvBitmap *srcbm, IRect cSrcRect, IPoint cDstPos, int nMode )
{
    switch( nMode )
    {
	case DM_COPY:
		blit_convert_copy( dstbm, srcbm, cSrcRect, cDstPos );
	    break;
	case DM_OVER:
	    blit_convert_over( dstbm, srcbm, cSrcRect, cDstPos );
	    break;
	case DM_BLEND:
	    if ( srcbm->m_eColorSpc == CS_RGB32 ) {
		blit_convert_alpha( dstbm, srcbm, cSrcRect, cDstPos );
	    } else {
		blit_convert_over( dstbm, srcbm, cSrcRect, cDstPos );
	    }
	    break;
    }
    return( true );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DisplayDriver::RenderGlyph( SrvBitmap *pcBitmap, Glyph* pcGlyph,
			 const IPoint& cPos, const IRect& cClipRect, const Color32_s& sFgColor )
{
    IRect	cBounds	= pcGlyph->m_cBounds + cPos;
    IRect	cRect 	= cBounds & cClipRect;

    if ( cRect.IsValid() == false ) {
	return;
    }
    int sx = cRect.left - cBounds.left;
    int sy = cRect.top - cBounds.top;

    int nWidth	= cRect.Width()+1;
    int nHeight	= cRect.Height()+1;

    int	nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;

    uint8*  pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

    Color32_s	sCurCol;
    Color32_s	sBgColor;
  
    if ( pcBitmap->m_eColorSpc == CS_RGB15 ) {
	int	nDstModulo = pcBitmap->m_nBytesPerLine / 2 - nWidth;
	uint16* pDst = (uint16*)pcBitmap->m_pRaster + cRect.left + (cRect.top * pcBitmap->m_nBytesPerLine / 2);

	int nFgClut = COL_TO_RGB15( sFgColor );

	for ( int y = 0 ; y < nHeight ; ++y ) {
	    for ( int x = 0 ; x < nWidth ; ++x ) {
		int nAlpha = *pSrc++;

		if ( nAlpha > 0 ) {
		    if ( nAlpha == NUM_FONT_GRAYS - 1 ) {
			*pDst = nFgClut;
		    } else {
			int	nClut = *pDst;

			sBgColor = RGB16_TO_COL( nClut );


			sCurCol.red   = sBgColor.red   + (sFgColor.red   - sBgColor.red)   * nAlpha / (NUM_FONT_GRAYS-1);
			sCurCol.green = sBgColor.green + (sFgColor.green - sBgColor.green) * nAlpha / (NUM_FONT_GRAYS-1);
			sCurCol.blue  = sBgColor.blue  + (sFgColor.blue  - sBgColor.blue)  * nAlpha / (NUM_FONT_GRAYS-1);
	    
			*pDst = COL_TO_RGB15( sCurCol );
		    }
		}
		pDst++;
	    }
	    pSrc += nSrcModulo;
	    pDst += nDstModulo;
	}
    } else if ( pcBitmap->m_eColorSpc == CS_RGB16 ) {
	int	nDstModulo = pcBitmap->m_nBytesPerLine / 2 - nWidth;
	uint16* pDst = (uint16*)pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 2;

	int nFgClut = COL_TO_RGB16( sFgColor );

	for ( int y = 0 ; y < nHeight ; ++y ) {
	    for ( int x = 0 ; x < nWidth ; ++x ) {
		int nAlpha = *pSrc++;

		if ( nAlpha > 0 ) {
		    if ( nAlpha == NUM_FONT_GRAYS - 1 ) {
			*pDst = nFgClut;
		    } else {
			int	nClut = *pDst;

			sBgColor = RGB16_TO_COL( nClut );

			sCurCol.red   = sBgColor.red   + int(sFgColor.red   - sBgColor.red)   * nAlpha / (NUM_FONT_GRAYS-1);
			sCurCol.green = sBgColor.green + int(sFgColor.green - sBgColor.green) * nAlpha / (NUM_FONT_GRAYS-1);
			sCurCol.blue  = sBgColor.blue  + int(sFgColor.blue  - sBgColor.blue)  * nAlpha / (NUM_FONT_GRAYS-1);
	    
			*pDst = COL_TO_RGB16( sCurCol );
		    }
		}
		pDst++;
	    }
	    pSrc += nSrcModulo;
	    pDst += nDstModulo;
	}
    } else if ( pcBitmap->m_eColorSpc == CS_RGB32 ) {
	int	nDstModulo = pcBitmap->m_nBytesPerLine / 4 - nWidth;
	uint32* pDst = (uint32*)pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 4;

	int nFgClut = COL_TO_RGB32( sFgColor );

	for ( int y = 0 ; y < nHeight ; ++y ) {
	    for ( int x = 0 ; x < nWidth ; ++x ) {
		int nAlpha = *pSrc++;

		if ( nAlpha > 0 ) {
		    if ( nAlpha == NUM_FONT_GRAYS - 1 ) {
			*pDst = nFgClut;
		    } else {
			int	nClut = *pDst;

			sBgColor = RGB32_TO_COL( nClut );

			sCurCol.red   = sBgColor.red   + (sFgColor.red   - sBgColor.red)   * nAlpha / (NUM_FONT_GRAYS-1);
			sCurCol.green = sBgColor.green + (sFgColor.green - sBgColor.green) * nAlpha / (NUM_FONT_GRAYS-1);
			sCurCol.blue  = sBgColor.blue  + (sFgColor.blue  - sBgColor.blue)  * nAlpha / (NUM_FONT_GRAYS-1);
	    
			*pDst = COL_TO_RGB32( sCurCol );
		    }
		}
		pDst++;
	    }
	    pSrc += nSrcModulo;
	    pDst += nDstModulo;
	}
    }
}

void DisplayDriver::RenderGlyphBlend( SrvBitmap *pcBitmap, Glyph* pcGlyph,
			      const IPoint& cPos, const IRect& cClipRect, const Color32_s& sFgColor )
{
    IRect	cBounds	= pcGlyph->m_cBounds + cPos;
    IRect	cRect 	= cBounds & cClipRect;

    if ( cRect.IsValid() == false ) {
	return;
    }
    int sx = cRect.left - cBounds.left;
    int sy = cRect.top - cBounds.top;

    int nWidth	= cRect.Width()+1;
    int nHeight	= cRect.Height()+1;

    int	nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;

    uint8*  pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

    int	nDstModulo = pcBitmap->m_nBytesPerLine / 4 - nWidth;
    uint32* pDst = (uint32*)pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 4;

    for ( int y = 0 ; y < nHeight ; ++y ) {
	for ( int x = 0 ; x < nWidth ; ++x ) {
	    int nAlpha = *pSrc++;
	    *pDst++ = COL_TO_RGB32( Color32_s( sFgColor.red, sFgColor.green, sFgColor.blue, int(sFgColor.alpha * nAlpha) / 255 ) );
	}
	pSrc += nSrcModulo;
	pDst += nDstModulo;
    }
    
}

void DisplayDriver::RenderGlyph( SrvBitmap *pcBitmap, Glyph* pcGlyph,
			 const IPoint& cPos, const IRect& cClipRect, const uint32* anPallette )
{
    IRect cBounds = pcGlyph->m_cBounds + cPos;
    IRect cRect   = cBounds & cClipRect;

    if ( cRect.IsValid() == false ) {
	return;
    }
    int sx = cRect.left - cBounds.left;
    int sy = cRect.top - cBounds.top;

    int nWidth	= cRect.Width()+1;
    int nHeight	= cRect.Height()+1;

    int	nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;

    uint8*  pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

    if ( pcBitmap->m_eColorSpc == CS_RGB16 || pcBitmap->m_eColorSpc == CS_RGB15 ) {
	int	nDstModulo = pcBitmap->m_nBytesPerLine / 2 - nWidth;
	uint16* pDst = (uint16*)pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 2;

	for ( int y = 0 ; y < nHeight ; ++y ) {
	    for ( int x = 0 ; x < nWidth ; ++x ) {
		int nAlpha = *pSrc++;
		if ( nAlpha > 0 ) {
		    *pDst = anPallette[nAlpha];
		}
		pDst++;
	    }
	    pSrc += nSrcModulo;
	    pDst += nDstModulo;
	}
    } else if ( pcBitmap->m_eColorSpc == CS_RGB32 ) {
	int	nDstModulo = pcBitmap->m_nBytesPerLine / 4 - nWidth;
	uint32* pDst = (uint32*)pcBitmap->m_pRaster + cRect.left + cRect.top * pcBitmap->m_nBytesPerLine / 4;

	for ( int y = 0 ; y < nHeight ; ++y ) {
	    for ( int x = 0 ; x < nWidth ; ++x ) {
		int nAlpha = *pSrc++;

		if ( nAlpha > 0 ) {
		    *pDst = anPallette[nAlpha];
		}
		pDst++;
	    }
	    pSrc += nSrcModulo;
	    pDst += nDstModulo;
	}
    }
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

bool DisplayDriver::CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
										os::color_space eFormat, os::Color32_s sColorKey, area_id *phArea )
{
	return( false );
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

bool DisplayDriver::RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
										os::color_space eFormat, area_id *phArea )
{
	return( false );
}

/** Update a created Video overlay.
 * \par Description:
 * Should update the video overlay on the screen. Probably not necessary for hw
 * accelerated drivers.
 * \par
 * \param phArea - Pointer to the area of the overlay.
 * \sa
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

void DisplayDriver::UpdateVideoOverlay( area_id *phArea )
{
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


void DisplayDriver::__VW_reserved1__() {}
void DisplayDriver::__VW_reserved2__() {}
void DisplayDriver::__VW_reserved3__() {}
void DisplayDriver::__VW_reserved4__() {}
void DisplayDriver::__VW_reserved5__() {}
void DisplayDriver::__VW_reserved6__() {}
void DisplayDriver::__VW_reserved7__() {}
void DisplayDriver::__VW_reserved8__() {}
void DisplayDriver::__VW_reserved9__() {}







































































































