/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <gui/progressbar.h>
#include <gui/font.h>

using namespace os;


ProgressBar::ProgressBar( const Rect& cFrame, const std::string& cTitle, orientation eOrientation, uint32 nResizeMask, uint32 nFlags ) :
	View( cFrame, cTitle, nResizeMask, nFlags )
{
    m_vProgress = 0.0f;
    m_eOrientation = eOrientation;
}

void ProgressBar::SetProgress( float vValue )
{
    m_vProgress = vValue;
    Invalidate();
    Flush();
}

float ProgressBar::GetProgress() const
{
    return( m_vProgress );
}

void ProgressBar::AttachedToWindow()
{
    View* pcParent = GetParent();

    if ( pcParent != NULL ) {
	SetBgColor( pcParent->GetBgColor() );
    } else {
	SetBgColor( get_default_color( COL_NORMAL ) );
    }
}

void ProgressBar::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetNormalizedBounds();
    cBounds.Floor();
    
    DrawFrame( cBounds, FRAME_RECESSED | FRAME_TRANSPARENT );

    float vBarLength = (m_eOrientation == HORIZONTAL) ? cBounds.Width() : cBounds.Height();
    vBarLength = ceil( (vBarLength - 3.0f) * m_vProgress );

    Rect cBarFrame = cBounds;
    cBarFrame.Resize( 2.0f, 2.0f, -2.0f, -2.0f );
    Rect cRestFrame = cBarFrame;

    if ( vBarLength < 1.0f ) {
	FillRect( cRestFrame, GetBgColor() );
    } else {
	if ( m_eOrientation == HORIZONTAL ) {
	    cBarFrame.right = cBarFrame.left + vBarLength;
	    if ( cBarFrame.right > cBounds.right - 2.0f ) {
		cBarFrame.right = cBounds.right - 2.0f;
	    }
	    cRestFrame.left = cBarFrame.right + 1.0f;
	} else {
	    cBarFrame.top = cBarFrame.bottom - vBarLength;
	    if ( cBarFrame.right < cBounds.right + 2.0f ) {
		cBarFrame.top = cBounds.top + 2.0f;
	    }
	    cRestFrame.bottom = cBarFrame.top - 1.0f;
	}

	if ( vBarLength >= 1.0f ) {
	    FillRect( cBarFrame, Color32_s( 0x66, 0x88, 0xbb ) );
	}
	if ( cRestFrame.IsValid() ) {
	    FillRect( cRestFrame, GetBgColor() );
	}
    }
}

Point ProgressBar::GetPreferredSize( bool bLargest ) const
{
    font_height sHeight;
    GetFontHeight( &sHeight );
    float vHeight = sHeight.ascender + sHeight.descender + sHeight.line_gap;
    Point cSize( vHeight * 3, vHeight + 4 );
    if ( bLargest ) {
	cSize.x = 100000.0f;
    }
    return( cSize );
}

void ProgressBar::FrameSized( const Point& cDelta )
{
    Rect cBounds( GetBounds() );
    bool bNeedFlush = false;
    if ( (m_eOrientation == HORIZONTAL && cDelta.x != 0.0f) || (m_eOrientation == VERTICAL && cDelta.y != 0.0f) ) {
	Rect cDamage = cBounds;
	cDamage.Resize( 2.0f, 2.0f, -2.0f, -2.0f );
	Invalidate( cDamage );
	bNeedFlush = true;
    }
    
    if ( cDelta.x != 0.0f ) {
	Rect cDamage = cBounds;

	cDamage.left = cDamage.right - max( 4.0f, cDelta.x + 3.0f );
	Invalidate( cDamage );
	bNeedFlush = true;
    }
    if ( cDelta.y != 0.0f ) {
	Rect cDamage = cBounds;

	cDamage.top = cDamage.bottom - max( 3.0f, cDelta.y + 2.0f );
	Invalidate( cDamage );
	bNeedFlush = true;
    }
    
    if ( bNeedFlush ) {
	Flush();
    }
}
