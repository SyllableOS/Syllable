/*  clockview.cpp - the component for the AtheOS Clock v0.3
 *  Copyright (C) 2001 Chir
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

#include <gui/font.h>
#include "clockview.h"
#include <macros.h>

ClockView::ClockView( Rect cFrame, Color32_s sColor, bool bShowSeconds, bool bDigital  ) : LayoutView( cFrame, "my_view" )
{
    Font* f = new Font(DEFAULT_FONT_BOLD);
    f->SetSize( 15 );
    SetFont( f );
    f->Release();    // Decrease reference count.   // delete f;
	
	sBackColor = sColor;
    m_bShowSec = bShowSeconds;
    m_bShowDigital = bDigital;
    m_nHour = -1;  m_nMin = -1;  m_nSec = -1;
	m_bIsFirst = true;
}

void ClockView::Paint( const Rect& cUpdateRect )
{
    if( cUpdateRect != GetBounds() )
    {
        Invalidate( true );     // Ensure the whole area is redrawn.
    }
    else
    {
        draw( GetBounds() );
    }
}

void ClockView::FrameSized( const Point& cDelta )
{
   Invalidate(true);
}

void ClockView::TimerTick( int nID )
{   
    // long nCurSysTime = get_real_time() / 1000000;
	time_t sTime;
    int h, m, s;
    
    time( &sTime );
	tm *psTime = localtime( &sTime );
	
	h = psTime->tm_hour; // (nCurSysTime/3600) % 12;
    m = psTime->tm_min; // (nCurSysTime/60) % 60;
    s = psTime->tm_sec; // nCurSysTime % 60;
   
    if( h != m_nHour  ||  m != m_nMin )  { m_nHour = h;  m_nMin = m;  m_nSec = s;  Invalidate(true);  Flush();  return; }

    // --- Animate seconds ---
    if( m_bShowSec  &&  m_nSec != s )
    {
        if( m_bShowDigital )
		{
	   		Invalidate( true );  /* Flush(); */
		}
        else
		{
            float x0 = Width()/2, y0 = Height()/2;
            float R = std::min( Width(), Height() ) / 2 - 10;
            float r = R * 0.9;
            // --- Draw seconds at new position ---
            float alpha = 2*M_PI*((float)s/60);
        
            SetDrawingMode( DM_INVERT );
//          SetFgColor( 255, 128, 0 );
            MovePenTo( x0 , y0 );
            DrawLine( Point( x0 + r*sin(alpha) , y0 - r*cos(alpha) ) );
            if( m_nSec >= 0 )
            {
                // --- Erase previous position for seconds ---
                alpha = 2*M_PI*((float)m_nSec/60);
                MovePenTo( x0 , y0 );
                DrawLine( Point( x0 + r*sin(alpha) , y0 - r*cos(alpha) ) );
            }
            SetDrawingMode( DM_OVER );
		}
        m_nSec = (int) s;
        Flush();
    }
}

void ClockView::showSeconds( bool bShow )
{
    m_bShowSec = bShow;
    Invalidate( true );                             // Force full refresh.
}

void ClockView::showDigital( bool bShow )
{
    m_bShowDigital = bShow;
	Invalidate( true );                             // Force full refresh.
}

bool ClockView::GetDigital()
{
	return m_bShowDigital;
}

bool ClockView::GetShowSeconds()
{
	return m_bShowSec;
}

void ClockView::SetBackColor(Color32_s s_BackColor)
{
	sBackColor = s_BackColor;
}

Color32_s ClockView::GetBackColor()
{
	return sBackColor;
}


void ClockView::draw( const Rect& cUpdateRect )
{
    if( m_bIsFirst )
    {
       m_bIsFirst = false;
       GetWindow()->AddTimer( this, 0, 5000, false );
    }
    if( m_nHour < 0 )  return;                      // Not initialised yet.

    SetFgColor( sBackColor);
    FillRect( cUpdateRect );

    if( ! m_bShowDigital )
    {
    	float x0 = Width()/2, y0 = Height() / 2;           // Center of circle.
    	float R = std::min( Width(), Height() ) / 2 - 10;  // Radius.
    	float r;
    	float alpha;
    
    	// --- Draw a nice circle ---
    	r = R;
    	SetFgColor( 160, 160, 160 );
    	for( alpha=0; alpha<2*M_PI; alpha += 2*M_PI/60 )
    	{
        	Point p( x0 + r*sin(alpha) , y0 - r*cos(alpha) );
        	MovePenTo( p );  DrawLine( p );             // Plot just one point.
    	}
    	SetFgColor( 255, 0, 0 );
    	for( alpha=0; alpha<2*M_PI; alpha += 2*M_PI/12 )
    	{
        	Point p( x0 + r*sin(alpha) , y0 - r*cos(alpha) );
        	MovePenTo( p );  DrawLine( p );             // Plot just one point.
    	}
    
    	// --- Draw hours ---
    	r = R * 0.5;
    	alpha = 2*M_PI*((float)m_nHour/12.0);
    	SetFgColor( 0, 0, 255 );
    	MovePenTo( x0 , y0 );
    	DrawLine( Point( x0 + r*sin(alpha) , y0 - r*cos(alpha) ) );
    
    	// --- Draw minutes ---
    	r = R * 0.7;
    	alpha = 2*M_PI*((float)m_nMin/60);
    	SetFgColor( 0, 0, 255 );
    	MovePenTo( x0 , y0 );
    	DrawLine(Point( x0 + r*sin(alpha) , y0 - r*cos(alpha) ) );

    	if( m_bShowSec )
    	{
        	// --- Draw seconds ---
        	r = R * 0.9;
        	alpha = 2*M_PI*((float)m_nSec/60);
        	SetDrawingMode( DM_INVERT );
    		//  SetFgColor( 255, 128, 0 );
        	MovePenTo( x0 , y0 );    
        	DrawLine( Point( x0 + r*sin(alpha) , y0 - r*cos(alpha) ) );
        	SetDrawingMode( DM_OVER );
    	}
    }
    else
    {
        float x0 = Width()/2  ,  y0 = Height()/2 ;        // Center of rectangle.
        char s[80];
        
        if( m_bShowSec )
		{
	    	sprintf( s, " %02d : %02d : %02d ", m_nHour, m_nMin, m_nSec );
		}
        else
		{
	    	sprintf( s, " %02d : %02d ", m_nHour, m_nMin );
		}

        float l = GetStringWidth( s );
        struct font_height fh;
        GetFont()->GetHeight( &fh );
        SetFgColor( 0, 0, 255 );
        SetBgColor( sBackColor );

        DrawString( Point(x0-l/2,y0+(fh.ascender-fh.descender)/2), s );
    }
   
    Flush();
}

