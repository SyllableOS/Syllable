/*
 *  AtheMgr - System Manager for AtheOS
 *  Copyright (C) 2001 John Hall
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "cpuview.h"

#include <atheos/time.h>

#include <gui/window.h>
#include <gui/stringview.h>
#include <gui/tableview.h>

using namespace os;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

CpuMeter::CpuMeter(  const Rect& cFrame, const char* pzTitle, uint32 nResizeMask, uint32 nFlags  )
    : View( cFrame, pzTitle, nResizeMask, nFlags  )
{
    memset( m_avValues, 0, sizeof( m_avValues ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

CpuMeter::~CpuMeter()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void CpuMeter::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetBounds();

    float	 x;
    float	 y;
    int	 i;
   
    DrawFrame( cBounds, FRAME_RECESSED );
  
    cBounds.left += 2;
    cBounds.top  += 2;
    cBounds.right  -= 2;
    cBounds.bottom -= 2;

    SetFgColor( 255, 255, 255, 0 );
   
    FillRect( cBounds );
  
    SetFgColor( 0, 0, 0, 0 );
    for ( i = 0, x = cBounds.right ; x >= cBounds.left ; ++i )
    {
	y = (1.0f - m_avValues[ i ]) * (cBounds.Height()-1.0f);
	DrawLine( Point( x, cBounds.bottom ), Point( x, y ) );
	x -= 1.0f;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void CpuMeter::AddValue( float vValue )
{
    for ( int i = 1023 ; i > 0 ; --i )	{
	m_avValues[ i ] = m_avValues[ i - 1 ];
    }

    m_avValues[ 0 ] = vValue; //min( 1000, nValue );

    Rect cBounds = GetBounds();
  
    cBounds.left += 2;
    cBounds.top += 2;
    cBounds.right -= 2;
    cBounds.bottom -= 2;
 
    ScrollRect( cBounds + Point( 1, 0 ), cBounds );
    float y = (1.0f - m_avValues[ 0 ]) * (cBounds.Height()+1.0f);
   
    SetFgColor( 0, 0, 0, 0 );
    DrawLine( Point( cBounds.right, cBounds.bottom ), Point( cBounds.right, y ) );
  
    SetFgColor( 255, 255, 255, 0 );
    DrawLine( Point( cBounds.right, 2 ), Point( cBounds.right, y) );
  
    Flush();
}
