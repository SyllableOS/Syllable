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
#include <gui/window.h>

#include "barview.h"

using namespace os;

enum
{
    NUM_SQUARES = 30,
    LEFT_BORDER = 4,
    RIGHT_BORDER = 8,
    BAR_WIDTH = 6,
    BAR_HEIGHT = 22,
    BAR_HSPACING = 0,
    BAR_VSPACING = 10
};

static Color32_s g_sBgColor( 130, 130, 130 );

static Color32_s get_bar_color( int nIndex )
{

    float vGreen = float(nIndex) / float(NUM_SQUARES-1);    
   
    return( Color32_s( 0, int(((100.0f + ( 155.0f * vGreen )) )*0.8f), 0 ) );
}


Rect BarView::GetBarRect( int nCol, int nBar )
{
	int nStreamCount = 0;
	for( int i = 0; i < nBar; i++ )
	{
		if( m_bStreamActive[i] )
			nStreamCount++;
	}
    float x = m_vNumWidth + nCol * (BAR_WIDTH + BAR_HSPACING) + LEFT_BORDER;
    float y = nStreamCount * (BAR_HEIGHT + BAR_VSPACING) + 4.0f;
    return( Rect( x + 2.0f, y + 2.0f, x + BAR_WIDTH - 1.0f, y + BAR_HEIGHT - 2.0f ) );
//    return( Rect( x + 2.0f, y + 2.0f, x + BAR_WIDTH - 2.0f, y + BAR_HEIGHT - 2.0f ) );
}

BarView::BarView( const Rect& cFrame ) : View( cFrame, "bar_view", CF_FOLLOW_ALL )
{
    Font* pcFont = GetFont();

    pcFont->SetSize( 9.0f );
    
    GetFontHeight( &m_sFontHeight );

    for ( int i = 0 ; i < MEDIA_MAX_AUDIO_STREAMS ; ++i ) {
    	m_bStreamActive[i] = false;
		m_anNumLighted[i] = 0;
		m_anSliderPos[i] = 100;
    }
    m_vNumWidth = 8.0f;
}


Point BarView::GetPreferredSize( bool bLargest ) const
{
	int nStreamCount = 0;
	for( int i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ )
	{
		if( m_bStreamActive[i] )
			nStreamCount++;
	}
    return( Point( m_vNumWidth + LEFT_BORDER + RIGHT_BORDER + (BAR_WIDTH+BAR_HSPACING) * NUM_SQUARES,
		   10.0f + BAR_HEIGHT * nStreamCount + BAR_VSPACING * (nStreamCount - 1) ) );
}

void BarView::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	 /*float x = m_vNumWidth + nCol * (BAR_WIDTH + BAR_HSPACING) + LEFT_BORDER;
    float y = nStreamCount * (BAR_HEIGHT + BAR_VSPACING) + 4.0f;
    return( Rect( x + 2.0f, y + 2.0f, x + BAR_WIDTH - 1.0f, y + BAR_HEIGHT - 2.0f ) );
    */
	if( nCode == MOUSE_INSIDE && nButtons )
	{
		if( cNewPos.y > 6.0f && cNewPos.y < 6.0f + MEDIA_MAX_AUDIO_STREAMS * (BAR_HEIGHT + BAR_VSPACING ) &&
			cNewPos.x > m_vNumWidth + LEFT_BORDER && cNewPos.x < m_vNumWidth + LEFT_BORDER +
					NUM_SQUARES * (BAR_WIDTH + BAR_HSPACING) ) {
			/* Calulate bar */
			int nBar = (int)( ( cNewPos.y - 6.0f ) / ( BAR_HEIGHT + BAR_VSPACING ) );
			
			
			/* Now calulate entry */
			int nNumber = 0;
			for( int i = 0; i < MEDIA_MAX_AUDIO_STREAMS; i++ )
			{
				if( m_bStreamActive[i] ) {
					if( nNumber == nBar )
					{
						/* Got it! -> Calculate column */
						int nCol = (int)( ( cNewPos.x - m_vNumWidth - LEFT_BORDER + 1.0f ) / ( BAR_WIDTH + BAR_HSPACING ) );
				
					
						m_anSliderPos[i] = nCol * 100 / NUM_SQUARES;
						m_anNumLighted[i] = nCol;
						
						os::Rect cRect = GetBarRect( NUM_SQUARES, i );
						cRect.left = 0;
						cRect.right = GetBounds().right;
						Invalidate( cRect );
						Flush();
						
						
						/* Send message */
						Message* pcMsg = new os::Message( MESSAGE_STREAM_VOLUME_CHANGED );
						pcMsg->AddInt8( "volume", m_anSliderPos[i] );
						pcMsg->AddInt32( "stream", i );
						GetWindow()->PostMessage( pcMsg, GetWindow() );
					}
					nNumber++;
				}
			}
		}
	} else
		os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}

void BarView::MouseDown( const os::Point& cNewPos, uint32 nButtons )
{
	MouseMove( cNewPos, MOUSE_INSIDE, nButtons, NULL );
	os::View::MouseDown( cNewPos, nButtons );
}

void BarView::Paint( const Rect& cUpdateRect )
{
    FillRect( cUpdateRect, get_default_color( COL_NORMAL ) );

    int nStreamCount = 0;
    for ( int i = 0 ; i < MEDIA_MAX_AUDIO_STREAMS ; ++i ) {
	float x = m_vNumWidth + LEFT_BORDER;
	float y = nStreamCount * (BAR_HEIGHT + BAR_VSPACING) + 4.0f;
	
	if( !m_bStreamActive[i] )
		continue;
	else
		nStreamCount++;

	Rect cBarRect( x - 1, y - 1, x + (BAR_WIDTH + BAR_HSPACING) * NUM_SQUARES + 1, y + BAR_HEIGHT + 2 );

	DrawFrame( cBarRect, FRAME_RECESSED | FRAME_TRANSPARENT );
	cBarRect.Resize( 2.0f, 2.0f, -2.0f, -2.0f );
	FillRect( cBarRect, g_sBgColor );
	
	Rect cIRect = GetBarRect( 0, i );
	Rect cORect( cIRect );

	MovePenTo( 4.0f, cIRect.top + ( cIRect.bottom - cIRect.top ) * 0.5f + m_sFontHeight.descender + m_sFontHeight.line_gap );
	SetBgColor( get_default_color( COL_NORMAL ) );
	SetFgColor( 0, 0, 0 );
	DrawString( m_zStreamLabel[i] );
	
	cORect.Resize( -2.0f, -2.0f, 2.0f, 2.0f );
	
	for ( int j = 0 ; j < NUM_SQUARES ; ++j ) {
	    cORect += Point( BAR_WIDTH + BAR_HSPACING, 0.0f );
	    if( m_anSliderPos[i] * NUM_SQUARES / 101 == j ) {
	    FillRect( cIRect, get_default_color( COL_SEL_MENU_BACKGROUND ) );
	    } else if ( j < m_anNumLighted[i] ) {
		FillRect( cIRect, get_bar_color( j ) );
	    }
	    cIRect += Point( BAR_WIDTH + BAR_HSPACING, 0.0f );
	}
    }
}

void BarView::SetStreamActive( int nNum, bool bActive )
{
	m_bStreamActive[nNum] = bActive;
	
	/* Update widths */
	m_vNumWidth = 0.0f;
    for ( int i = 0 ; i < MEDIA_MAX_AUDIO_STREAMS; ++i ) {
		if( !m_bStreamActive[i] )
			continue;
		float vWidth = GetStringWidth( m_zStreamLabel[i] );
		if ( vWidth > m_vNumWidth ) {
	    	m_vNumWidth = vWidth;
		}
    }
    m_vNumWidth += 8.0f;
	
	Invalidate();
	Flush();
}

void BarView::SetStreamLabel( int nNum, String zLabel )
{
	m_zStreamLabel[nNum] = zLabel;
	Invalidate();
	Flush();
} 



void BarView::SetStreamVolume( int nNum, int nVolume )
{
	m_anSliderPos[nNum] = nVolume;
	m_anNumLighted[nNum] = int((float)m_anSliderPos[nNum] * float(NUM_SQUARES) + 0.5f);
	os::Rect cRect = GetBarRect( NUM_SQUARES, nNum );
	cRect.left = 0;
	cRect.right = GetBounds().right;
	Invalidate( cRect );
	Flush();
} 
















