/*
 *  AtheMgr - System Manager for AtheOS
 *  Copyright (C) 2001 John Hall
 *  
 *  Sections of this code: Copyright (C) 1999 - 2001 Kurt Skauen 
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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

#include <util/application.h>
#include <util/message.h>

#include "memview.h"

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MemMeter::MemMeter(  const Rect& cFrame, const char* pzTitle, uint32 nResizeMask, uint32 nFlags  )
    : View( cFrame, pzTitle, nResizeMask, nFlags  )
{
  /*m_pcPopupMenu     = new Menu( Rect( 0, 0, 10, 10 ), "morph", ITEMS_IN_COLUMN );
  Menu* pcChangeMenu = new Menu( Rect( 0, 0, 10, 10 ), "Change", ITEMS_IN_COLUMN );

  m_pcPopupMenu->SetTargetForItems( Application::GetInstance() );
  pcChangeMenu->SetTargetForItems( Application::GetInstance() );*/
  
  memset( m_avTotUsedMem, 0, sizeof( m_avTotUsedMem ) );
  memset( m_avCacheSize, 0, sizeof( m_avCacheSize ) );
  memset( m_avDirtyCacheSize, 0, sizeof( m_avDirtyCacheSize ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MemMeter::~MemMeter()
{
  //delete m_pcPopupMenu;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MemMeter::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetBounds();
    float x;
    int	  i;

    DrawFrame( cBounds, FRAME_RECESSED );   
    cBounds.left += 2;
    cBounds.top += 2;
    cBounds.right -= 2;
    cBounds.bottom -= 2;
    
    SetFgColor( 0, 0, 0, 0 );

    Rect cDrawRect( cBounds & cUpdateRect );
	
    for ( i = int( cBounds.right - cDrawRect.right ), x = cDrawRect.right ; x >= cDrawRect.left ; ++i )
    {
	float y1 = m_avTotUsedMem[ i ]     * (cBounds.Height()+1.0f);
	float y2 = m_avCacheSize[ i ]      * (cBounds.Height()+1.0f);
	float y3 = m_avDirtyCacheSize[ i ] * (cBounds.Height()+1.0f);

	if ( y3 == 0 && m_avDirtyCacheSize[ i ] > 0.0f ) {
	    y3++;
	    y2--;
	}
	
	MovePenTo( x, cBounds.bottom );
	SetFgColor( 0, 0, 0, 0 );
	DrawLine( Point( x, cBounds.bottom - y1 ) );
	SetFgColor( 80, 140, 180, 0 );
	DrawLine( Point( x, cBounds.bottom -  y1 - y2 ) );
	SetFgColor( 255, 0, 0, 0 );
	DrawLine( Point( x, cBounds.bottom - y1 - y2 - y3 ) );
	SetFgColor( 255, 255, 255, 0 );
	DrawLine( Point( x, cBounds.top ) );
	x -= 1.0f;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MemMeter::AddValue( float vTotMem, float vCacheSize, float vDirtyCache )
{
  for ( int i = 1023 ; i > 0 ; --i )	{
    m_avTotUsedMem[ i ]     = m_avTotUsedMem[ i - 1 ];
    m_avCacheSize[ i ]      = m_avCacheSize[ i - 1 ];
    m_avDirtyCacheSize[ i ] = m_avDirtyCacheSize[ i - 1 ];
  }

   m_avTotUsedMem[ 0 ]     = std::min( 1.0f, vTotMem );
  m_avCacheSize[ 0 ]      = std::min( 1.0f, vCacheSize );
  m_avDirtyCacheSize[ 0 ] = std::min( 1.0f, vDirtyCache );

  Rect cBounds = GetBounds();
  
  cBounds.left += 2;
  cBounds.top += 2;
  cBounds.right -= 2;
  cBounds.bottom -= 2;
   
  ScrollRect( cBounds + Point( 1, 0 ), cBounds );
  float y1 = m_avTotUsedMem[ 0 ]     * (cBounds.Height()+1.0f);
  float y2 = m_avCacheSize[ 0 ]      * (cBounds.Height()+1.0f);
  float y3 = m_avDirtyCacheSize[ 0 ] * (cBounds.Height()+1.0f);

  if ( y3 == 0 && m_avDirtyCacheSize[ 0 ] > 0.0f ) {
      y3++;
      y2--;
  }
  
  MovePenTo( cBounds.right, cBounds.bottom );
  SetFgColor( 0, 0, 0, 0 );
  DrawLine( Point( cBounds.right, cBounds.bottom - y1 ) );
  SetFgColor( 80, 140, 180, 0 );
  DrawLine( Point( cBounds.right, cBounds.bottom -  y1 - y2 ) );
  SetFgColor( 255, 0, 0, 0 );
  DrawLine( Point( cBounds.right, cBounds.bottom - y1 - y2 - y3 ) );
  
  SetFgColor( 255, 255, 255, 0 );
  DrawLine( Point( cBounds.right, cBounds.top ) );
  Sync();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MemMeter::MouseDown( const Point& cPosition, uint32 nButtons )
{
    if ( nButtons != 2 ) {
	return;
    }
    MakeFocus( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MemMeter::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
    if ( nButtons != 2 ) {
	return;
    }
    //m_pcPopupMenu->Open( ConvertToScreen( cPosition ) );
}

