/*  CPUMon - Small AtheOS utility for displaying the CPU load.
 *  Copyright (C) 2001 Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
#include <atheos/time.h>
#include <gui/window.h>

#include "main.h"
#include "barview.h"

using namespace os;


enum { TIMER_ID = 1 };

enum
{
    NUM_SQUARES = 18,
    LEFT_BORDER = 5,
    RIGHT_BORDER = 10,
    BAR_WIDTH = 10,
    BAR_HEIGHT = 22,
    BAR_HSPACING = 0,
    BAR_VSPACING = 10
};

static Color32_s g_sBgColor( 130, 130, 130 );

static Color32_s get_bar_color( int nIndex )
{
    int nRedMarking = NUM_SQUARES - 4;
    
    float vRed;
    float vGreen = float(nIndex) / float(NUM_SQUARES-1);
    
    if ( nIndex <= nRedMarking ) {
	vRed = 0.0f;
    } else {
	vRed = float(nIndex - nRedMarking) / float(NUM_SQUARES - nRedMarking - 1);
	vRed = 0.6f + vRed * 0.4f;
    }
    return( Color32_s( int(255.0f*vRed), int(((100.0f + ( 155.0f * vGreen )) * (1.0f-vRed))*0.8f), 0 ) );
}


Rect BarView::GetBarRect( int nCol, int nBar )
{
    float x = m_vNumWidth + nCol * (BAR_WIDTH + BAR_HSPACING) + LEFT_BORDER;
    float y = nBar * (BAR_HEIGHT + BAR_VSPACING) + 4.0f;
    return( Rect( x + 2.0f, y + 2.0f, x + BAR_WIDTH - 1.0f, y + BAR_HEIGHT - 2.0f ) );
//    return( Rect( x + 2.0f, y + 2.0f, x + BAR_WIDTH - 2.0f, y + BAR_HEIGHT - 2.0f ) );
}

BarView::BarView( const Rect& cFrame, int nCPUCount ) : View( cFrame, "bar_view", CF_FOLLOW_ALL )
{
    m_nCPUCount = nCPUCount;

    Font* pcFont = GetFont();

    pcFont->SetSize( 9.0f );
    
    GetFontHeight( &m_sFontHeight );

    m_vNumWidth = 0.0f;
    for ( int i = 0 ; i < nCPUCount ; ++i ) {
	m_anNumLighted[i] = 0;
	char zStr[16];
	sprintf( zStr, "%d", i );
	float vWidth = GetStringWidth( zStr );
	if ( vWidth > m_vNumWidth ) {
	    m_vNumWidth = vWidth;
	}
    }
    m_vNumWidth += 8.0f;
}

void BarView::AttachedToWindow()
{
    GetWindow()->AddTimer( this, TIMER_ID, 150000, false );
}

void BarView::DetachedFromWindow()
{
    GetWindow()->RemoveTimer( this, TIMER_ID );
}


Point BarView::GetPreferredSize( bool bLargest ) const
{
    return( Point( m_vNumWidth + LEFT_BORDER + RIGHT_BORDER + (BAR_WIDTH+BAR_HSPACING) * NUM_SQUARES,
		   10.0f + BAR_HEIGHT * m_nCPUCount + BAR_VSPACING * (m_nCPUCount - 1) ) );
}

void BarView::Paint( const Rect& cUpdateRect )
{
    FillRect( cUpdateRect, get_default_color( COL_NORMAL ) );

    
    for ( int i = 0 ; i < m_nCPUCount ; ++i ) {
	float x = m_vNumWidth + LEFT_BORDER;
	float y = i * (BAR_HEIGHT + BAR_VSPACING) + 4.0f;

	Rect cBarRect( x - 1, y - 1, x + (BAR_WIDTH + BAR_HSPACING) * NUM_SQUARES + 1, y + BAR_HEIGHT + 2 );

	DrawFrame( cBarRect, FRAME_RECESSED | FRAME_TRANSPARENT );
	cBarRect.Resize( 2.0f, 2.0f, -2.0f, -2.0f );
	FillRect( cBarRect, g_sBgColor );
	char zStr[16];
	sprintf( zStr, "%d", i );
	
	Rect cIRect = GetBarRect( 0, i );
	Rect cORect( cIRect );

	MovePenTo( 4.0f, cIRect.top + (cIRect.bottom - cIRect.top) * 0.5f + m_sFontHeight.descender + m_sFontHeight.line_gap / 2 );
	SetBgColor( get_default_color( COL_NORMAL ) );
	SetFgColor( 0, 0, 0 );
	DrawString( zStr );
	
	cORect.Resize( -2.0f, -2.0f, 2.0f, 2.0f );
	
	for ( int j = 0 ; j < NUM_SQUARES ; ++j ) {
//	    DrawFrame( cORect, FRAME_RECESSED | FRAME_TRANSPARENT );
	    cORect += Point( BAR_WIDTH + BAR_HSPACING, 0.0f );
	    if ( j < m_anNumLighted[i] ) {
		FillRect( cIRect, get_bar_color( j ) );
		cIRect += Point( BAR_WIDTH + BAR_HSPACING, 0.0f );
	    }
	}
    }
}

void BarView::TimerTick( int nID )
{
    float avLoads[MAX_CPU_COUNT];
    GetLoad( m_nCPUCount, avLoads );
    SetLoads( avLoads );
}

void BarView::SetLoads( float* pavLoads )
{
    for ( int i = 0 ; i < m_nCPUCount ; ++i ) {
	int nNumHighlighted = int(pavLoads[i] * float(NUM_SQUARES) + 0.5f);

	if ( nNumHighlighted > m_anNumLighted[i] ) {
	    Rect cRect = GetBarRect( m_anNumLighted[i], i );
	    
	    for ( int j = m_anNumLighted[i] ; j < nNumHighlighted ; ++j ) {
		FillRect( cRect, get_bar_color( j ) );
		cRect += Point( BAR_WIDTH + BAR_HSPACING, 0.0f );
	    }
	    Flush();
	} else if ( nNumHighlighted < m_anNumLighted[i] ) {
	    Rect cRect = GetBarRect( nNumHighlighted, i );
	    for ( int j = nNumHighlighted ; j < m_anNumLighted[i] ; ++j ) {
		FillRect( cRect, g_sBgColor/*get_default_color( COL_NORMAL )*/ );
		cRect += Point( BAR_WIDTH + BAR_HSPACING, 0.0f );
	    }
	    Flush();
	}
	m_anNumLighted[i] = nNumHighlighted;
    }
}

