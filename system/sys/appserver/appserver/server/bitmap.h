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

#include <atheos/types.h>

#include <gui/guidefines.h>
#include <gui/rect.h>
#include <util/resource.h>
#include <util/array.h>


struct	ModeInfo_s;
class	DisplayDriver;


class SrvBitmap : public Resource
{
public:
    SrvBitmap( int nWidth, int nHeight,
	       os::color_space eColorSpc,
	       uint8* pRaster = NULL, int nBytesPerLine = 0 );
	os::IRect GetBounds()
	{
		return( os::IRect( 0, 0, m_nWidth - 1, m_nHeight - 1 ) );
	}
	int	GetToken( void ) const	{ return( m_hHandle ); }

    area_id	    m_hArea;
    uint32	    m_nFlags;
    bool	    m_bFreeRaster;	// true if the raster memory is allocated by the constructor
    int		    m_nWidth;
    int		    m_nHeight;
    int		    m_nBytesPerLine;
    os::color_space m_eColorSpc;
    uint8*	    m_pRaster;		// adress of frame buffer
    DisplayDriver*  m_pcDriver;
    bool	    m_bVideoMem;
    uint32		m_nVideoMemOffset;
   	int	m_hHandle;
protected:
    ~SrvBitmap();
};


extern Array<SrvBitmap>* g_pcBitmaps;








