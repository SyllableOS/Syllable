/*
 *  "Mulvane" Window-Decorator
 *  Copyright (C) 2002 Iain Hutchison
 *
 *  Based on "Amiga" Window-Decorator
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

#include "MulvaneDecorator.h"
#include <layer.h>

#include <gui/window.h>

using namespace os;

/* Colour blending function (does standard alpha-blend between 2 colours) */
static Color32_s BlendColours( const Color32_s& sColour1, const Color32_s& sColour2, float vBlend )
{
	int r = int( (float(sColour1.red)   * vBlend + float(sColour2.red)   * (1.0f - vBlend)) );
	int g = int( (float(sColour1.green) * vBlend + float(sColour2.green) * (1.0f - vBlend)) );
	int b = int( (float(sColour1.blue)  * vBlend + float(sColour2.blue)  * (1.0f - vBlend)) );
	if ( r < 0 ) r = 0; else if (r > 255) r = 255;
	if ( g < 0 ) g = 0; else if (g > 255) g = 255;
	if ( b < 0 ) b = 0; else if (b > 255) b = 255;
	return Color32_s(r, g, b, sColour1.alpha);
}


MulvaneDecorator::MulvaneDecorator( Layer* pcLayer, uint32 nWndFlags )
:	WindowDecorator( pcLayer, nWndFlags )
{
    m_nFlags = nWndFlags;

    m_bHasFocus   = false;
    m_bCloseState = false;
    m_bZoomState  = false;
    m_bDepthState = false;

    m_sFontHeight = pcLayer->GetFontHeight();
  
    CalculateBorderSizes();
}

MulvaneDecorator::~MulvaneDecorator()
{
}

void MulvaneDecorator::CalculateBorderSizes()
{
	if ( m_nFlags & WND_NO_BORDER )
	{
		m_vLeftBorder   = 0;
		m_vRightBorder  = 0;
		m_vTopBorder    = 0;
		m_vBottomBorder = 0;
	}
	else
	{
		if ( (m_nFlags & (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE)) ==
		     (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE) )
		{
		    m_vTopBorder = 3;
		}
		else
		{
//			m_vTopBorder = float(m_sFontHeight.ascender + m_sFontHeight.descender + 6);
			m_vTopBorder = 20;
		}
		m_vLeftBorder   = 3;
		m_vRightBorder  = 3;
		m_vBottomBorder = 3;
	}
}

void MulvaneDecorator::FontChanged()
{
	Layer* pcView = GetLayer();
	m_sFontHeight = pcView->GetFontHeight();
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

void MulvaneDecorator::SetFlags( uint32 nFlags )
{
	Layer* pcView = GetLayer();
	m_nFlags = nFlags;
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

Point MulvaneDecorator::GetMinimumSize()
{
	Point cMinSize( 0.0f, m_vTopBorder + m_vBottomBorder );

	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		cMinSize.x += m_cCloseRect.Width();
	}
	if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 )
	{
		cMinSize.x += m_cZoomRect.Width();
	}
	if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )
	{
		cMinSize.x += m_cToggleRect.Width();
	}
	if ( cMinSize.x < m_vLeftBorder + m_vRightBorder )
	{
		cMinSize.x = m_vLeftBorder + m_vRightBorder;
	}
	return( cMinSize );
}

WindowDecorator::hit_item MulvaneDecorator::HitTest( const Point& cPosition )
{
	if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE )
	{
		if ( cPosition.y < 3 )
		{
			if ( cPosition.x < 16 )
			{
				return( HIT_SIZE_LT );
			}
			else if ( cPosition.x > m_cBounds.right - 16 )
			{
				return( HIT_SIZE_RT );
			}
			else
			{
				return( HIT_SIZE_TOP );
			}
		}
		else if ( cPosition.y > m_cBounds.bottom - 3 )
		{
			if ( cPosition.x < 16 )
			{
				return( HIT_SIZE_LB );
			}
			else if ( cPosition.x > m_cBounds.right - 16 )
			{
				return( HIT_SIZE_RB );
			}
			else
			{
				return( HIT_SIZE_BOTTOM );
			}
		}
		else if ( cPosition.x < 3 )
		{
			return( HIT_SIZE_LEFT );
		}
		else if ( cPosition.x > m_cBounds.right - 3 )
		{
			return( HIT_SIZE_RIGHT );
		}
	}

	if ( m_cCloseRect.DoIntersect( cPosition ) )
	{
		return( HIT_CLOSE );
	}
	else if ( m_cZoomRect.DoIntersect( cPosition ) )
	{
		return( HIT_ZOOM );
	}
	else if ( m_cToggleRect.DoIntersect( cPosition ) )
	{
		return( HIT_MINIMIZE );
	}
	else if ( m_cDragRect.DoIntersect( cPosition ) )
	{
		return( HIT_DRAG );
	}
	return( HIT_NONE );
}

Rect MulvaneDecorator::GetBorderSize()
{
	return( Rect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void MulvaneDecorator::SetTitle( const char* pzTitle )
{
	m_cTitle = pzTitle;
	Render( m_cBounds );
}

void MulvaneDecorator::SetFocusState( bool bHasFocus )
{
	m_bHasFocus = bHasFocus;
	Render( m_cBounds );
}

void MulvaneDecorator::SetWindowFlags( uint32 nFlags )
{
	m_nFlags = nFlags;
}

void MulvaneDecorator::FrameSized( const Rect& cFrame )
{
	Layer* pcView = GetLayer();
	Point cDelta( cFrame.Width() - m_cBounds.Width(), cFrame.Height() - m_cBounds.Height() );
	m_cBounds = cFrame.Bounds();

	Layout();
	if ( cDelta.x != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.left = m_cZoomRect.left - fabs(cDelta.x)  - 2.0f;
		pcView->Invalidate( cDamage );
	}
	if ( cDelta.y != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.top = cDamage.bottom - std::max( m_vBottomBorder, m_vBottomBorder + cDelta.y ) - 1.0f;
		pcView->Invalidate( cDamage );
	}
}

void MulvaneDecorator::Layout()
{
	if ( m_nFlags & WND_NO_CLOSE_BUT )
	{
		m_cCloseRect.left   = 0;
		m_cCloseRect.right  = 0;
		m_cCloseRect.top    = 0;
		m_cCloseRect.bottom = 0;
	}
	else
	{
		m_cCloseRect.left   = 2;
		m_cCloseRect.right  = 17;
		m_cCloseRect.top    = 2;
		m_cCloseRect.bottom = 17;
	}

	m_cToggleRect.right = m_cBounds.right;
	if ( m_nFlags & WND_NO_DEPTH_BUT )
	{
		m_cToggleRect.left = m_cToggleRect.right;
	}
	else
	{
		m_cToggleRect.left = m_cToggleRect.right - 15;
	}
	m_cToggleRect.top = 2;
	m_cToggleRect.bottom = 17;

	if ( m_nFlags & WND_NO_ZOOM_BUT )
	{
		m_cZoomRect.left  = m_cToggleRect.left;
		m_cZoomRect.right = m_cToggleRect.left;
	}
	else
	{
		if ( m_nFlags & WND_NO_DEPTH_BUT )
		{
			m_cZoomRect.right = m_cBounds.right;
		}
		else
		{
			m_cZoomRect.right = m_cToggleRect.left - 1;
		}
		m_cZoomRect.left = m_cZoomRect.right - 15;
	}
	m_cZoomRect.top    = 2;
	m_cZoomRect.bottom = 17;

	if ( m_nFlags & WND_NO_CLOSE_BUT )
	{
		m_cDragRect.left  = 0;
	}
	else
	{
		m_cDragRect.left  = m_cCloseRect.right + 1;
	}

	if ( m_nFlags & WND_NO_ZOOM_BUT )
	{
		if ( m_nFlags & WND_NO_DEPTH_BUT )
		{
			m_cDragRect.right = m_cBounds.right;
		}
		else
		{
		    m_cDragRect.right = m_cToggleRect.left - 1;
		}
	}
	else
	{
		m_cDragRect.right  = m_cZoomRect.left - 1;
	}
    m_cDragRect.top    = 2;
    m_cDragRect.bottom = 17;
}

void MulvaneDecorator::SetButtonState( uint32 nButton, bool bPushed )
{
	switch( nButton )
	{
		case HIT_CLOSE:
			SetCloseButtonState( bPushed );
			break;
		case HIT_ZOOM:
			SetZoomButtonState( bPushed );
			break;
		case HIT_MINIMIZE:
			SetDepthButtonState( bPushed );
			break;
	}
}

void MulvaneDecorator::SetCloseButtonState( bool bPushed )
{
	m_bCloseState = bPushed;
	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	}
}

void MulvaneDecorator::SetZoomButtonState( bool bPushed )
{
	m_bZoomState = bPushed;
	if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 )
	{
		DrawZoom( m_cZoomRect, m_bHasFocus, m_bZoomState == 1 );
	}
}

void MulvaneDecorator::SetDepthButtonState( bool bPushed )
{
	m_bDepthState = bPushed;
	if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )
	{
		DrawDepth( m_cToggleRect, m_bHasFocus, m_bDepthState == 1 );
	}
}

void MulvaneDecorator::DrawPanel( const Rect& cRect, bool bActive, bool bRecessed )
{
	Rect		R;
	Layer*		pcView = GetLayer();
	Color32_s	col_Black = Color32_s(0,0,0);
	Color32_s	col_Shine, col_Light, col_Dark, col_Shadow;

	if (bActive)
	{
		col_Light	= BlendColours(get_default_color(COL_SHINE),  get_default_color(COL_SEL_WND_BORDER), 0.5f);
		col_Dark	= BlendColours(get_default_color(COL_SHADOW), get_default_color(COL_SEL_WND_BORDER), 0.5f);
		col_Shine	= BlendColours(get_default_color(COL_SEL_WND_BORDER), col_Dark, 0.5f);
		col_Shadow	= BlendColours(get_default_color(COL_SHADOW),         col_Dark, 0.5f);
	}
	else
	{
		col_Light	= BlendColours(get_default_color(COL_SHINE), get_default_color(COL_NORMAL_WND_BORDER), 0.5f);
		col_Dark	= BlendColours(get_default_color(COL_SHADOW), get_default_color(COL_NORMAL_WND_BORDER), 0.5f);
		col_Shine	= BlendColours(get_default_color(COL_NORMAL_WND_BORDER), col_Dark, 0.5f);
		col_Shadow	= BlendColours(get_default_color(COL_SHADOW),         col_Dark, 0.5f);
	}

	// Top and bottom line
	pcView->SetFgColor( col_Shadow );
	pcView->DrawLine( Point(cRect.left,cRect.top), Point(cRect.right-1,cRect.top) );
	pcView->DrawLine( Point(cRect.left,cRect.bottom-1), Point(cRect.right-1,cRect.bottom-1) );
	// Shine
	pcView->SetFgColor( col_Shine );
	pcView->DrawLine( Point(cRect.left,cRect.top+1), Point(cRect.right-1,cRect.top+1) );
	pcView->DrawLine( Point(cRect.left,cRect.top+1), Point(cRect.left,   cRect.bottom-2) );
	// Shadow
	pcView->SetFgColor( col_Shadow );
	pcView->DrawLine( Point(cRect.left+1, cRect.bottom-2), Point(cRect.right-1,cRect.bottom-2) );
	pcView->DrawLine( Point(cRect.right-1,cRect.bottom-2), Point(cRect.right-1,cRect.top+2) );
	// Gradient
	Point Left  = Point(cRect.left+1,  cRect.top+2);
	Point Right = Point(cRect.right-1, cRect.top+2);
	Color32_s col_Grad = col_Light;
	Color32_s col_Step = Color32_s( (col_Light.red-col_Dark.red)/15, (col_Light.green-col_Dark.green)/15, (col_Light.blue-col_Dark.blue)/15, 0 );
	for (int i=0; i<16; i++)
	{
		pcView->SetFgColor( col_Grad );
		pcView->DrawLine( Left, Right );
		Left.y  += 1;
		Right.y += 1;
		col_Grad.red   -= col_Step.red;
		col_Grad.green -= col_Step.green;
		col_Grad.blue  -= col_Step.blue;
	}
}

void MulvaneDecorator::DrawDepth( const Rect& cRect, bool bActive, bool bRecessed )
{
	Layer* pcView = GetLayer();

	if ( bRecessed )
	{
		pcView->SetFgColor( get_default_color(COL_SHINE) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 3), Point(cRect.left+14,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+14,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+ 3,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+12), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+14,cRect.top+ 4), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 10), Point(cRect.left+5,cRect.top+ 10) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+ 10), Point(cRect.left+5,cRect.top+ 12) );
	}
	else
	{
		pcView->SetFgColor( get_default_color(COL_SHADOW) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 3), Point(cRect.left+14,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+14,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+ 3,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+12), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+14,cRect.top+ 4), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 10), Point(cRect.left+5,cRect.top+ 10) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+ 10), Point(cRect.left+5,cRect.top+ 12) );
		pcView->SetFgColor( get_default_color(COL_SHINE) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 2), Point(cRect.left+13,cRect.top+ 2) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 3), Point(cRect.left+13,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 3), Point(cRect.left+ 2,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+11), Point(cRect.left+13,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+13,cRect.top+ 3), Point(cRect.left+13,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 8), Point(cRect.left+ 6,cRect.top+ 8) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 9), Point(cRect.left+ 6,cRect.top+ 9) );
		pcView->DrawLine( Point(cRect.left+ 6,cRect.top+ 9), Point(cRect.left+ 6,cRect.top+ 11) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MulvaneDecorator::DrawZoom(  const Rect& cRect, bool bActive, bool bRecessed )
{
	Layer* pcView = GetLayer();

	if ( bRecessed )
	{
		pcView->SetFgColor( get_default_color(COL_SHINE) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 3), Point(cRect.left+14,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+14,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+ 3,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+12), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+14,cRect.top+ 4), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 8), Point(cRect.left+10,cRect.top+ 8) );
		pcView->DrawLine( Point(cRect.left+10,cRect.top+ 4), Point(cRect.left+10,cRect.top+ 8) );
	}
	else
	{
		pcView->SetFgColor( get_default_color(COL_SHADOW) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 3), Point(cRect.left+14,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+14,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+ 3,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+12), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+14,cRect.top+ 4), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 8), Point(cRect.left+10,cRect.top+ 8) );
		pcView->DrawLine( Point(cRect.left+10,cRect.top+ 4), Point(cRect.left+10,cRect.top+ 8) );
		pcView->SetFgColor( get_default_color(COL_SHINE) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 2), Point(cRect.left+13,cRect.top+ 2) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 3), Point(cRect.left+13,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 3), Point(cRect.left+ 2,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+11), Point(cRect.left+13,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+13,cRect.top+ 3), Point(cRect.left+13,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 7), Point(cRect.left+ 9,cRect.top+ 7) );
		pcView->DrawLine( Point(cRect.left+ 9,cRect.top+ 3), Point(cRect.left+ 9,cRect.top+ 7) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MulvaneDecorator::DrawClose(  const Rect& cRect, bool bActive, bool bRecessed  )
{
	Layer* pcView = GetLayer();

	if ( bRecessed )
	{
		pcView->SetFgColor( get_default_color(COL_SHINE) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+ 4), Point(cRect.left+11,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+ 4), Point(cRect.left+12,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+11), Point(cRect.left+11,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+11), Point(cRect.left+12,cRect.top+ 4) );
	}
	else
	{
		pcView->SetFgColor( get_default_color(COL_SHADOW) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+ 4), Point(cRect.left+11,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+ 4), Point(cRect.left+12,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+11), Point(cRect.left+11,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+11), Point(cRect.left+12,cRect.top+ 4) );
		pcView->SetFgColor( get_default_color(COL_SHINE) );
		pcView->DrawLine( Point(cRect.left+3,cRect.top+ 3), Point(cRect.left+10,cRect.top+10) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+ 3), Point(cRect.left+11,cRect.top+10) );
		pcView->DrawLine( Point(cRect.left+3,cRect.top+10), Point(cRect.left+10,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+10), Point(cRect.left+11,cRect.top+ 3) );
	}
}

void MulvaneDecorator::Render( const Rect& cUpdateRect )
{
	if ( m_nFlags & WND_NO_BORDER )
	{
		return;
	}
  
	// Get title-bar fill-colour  
	Color32_s sFillColor = m_bHasFocus ? get_default_color(COL_SEL_WND_BORDER) : Color32_s( 128,128,128 );
	sFillColor = BlendColours(sFillColor, get_default_color(COL_SHADOW), 0.82f);
	// Get the layer object that we're drawing into
	Layer* pcView = GetLayer();
  
	Rect  cOBounds = pcView->GetBounds();
	Rect  cIBounds = cOBounds;
  
	cIBounds.left += m_vLeftBorder - 1;
	cIBounds.right -= m_vRightBorder - 1;
	cIBounds.top += m_vTopBorder - 1;
	cIBounds.bottom -= m_vBottomBorder - 1;

	pcView->DrawFrame( cOBounds, FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
	pcView->DrawFrame( cIBounds, FRAME_RECESSED | FRAME_THIN | FRAME_TRANSPARENT );
	// Bottom
	pcView->FillRect( Rect( cOBounds.left + 1, cIBounds.bottom + 1, cOBounds.right - 1, cOBounds.bottom - 1 ), sFillColor );
	// Left
	pcView->FillRect( Rect( cOBounds.left + 1, cOBounds.top + m_vTopBorder, cIBounds.left - 1, cIBounds.bottom ), sFillColor );
	// Right
	pcView->FillRect( Rect( cIBounds.right + 1, cOBounds.top + m_vTopBorder, cOBounds.right - 1, cIBounds.bottom ), sFillColor );


	if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE )
	{
		pcView->SetFgColor( 0, 0, 0, 255 );
		pcView->DrawLine( Point( 15, cIBounds.bottom + 1 ), Point( 15, cOBounds.bottom - 1 ) );
		pcView->DrawLine( Point( cOBounds.right - 16, cIBounds.bottom + 1 ), Point( cOBounds.right - 16, cOBounds.bottom - 1 ) );
		pcView->SetFgColor( 255, 255, 255, 255 );
		pcView->DrawLine( Point( 16, cIBounds.bottom + 1 ), Point( 16, cOBounds.bottom - 1 ) );
		pcView->DrawLine( Point( cOBounds.right - 15, cIBounds.bottom + 1 ), Point( cOBounds.right - 15, cOBounds.bottom - 1 ) );
	}

	if ( (m_nFlags & (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE)) !=
	     (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE) )
	{
		DrawPanel( Rect( cOBounds.left, cOBounds.top, cOBounds.right, cIBounds.top), m_bHasFocus, false);
	}

	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	}

	if ( (m_nFlags & WND_NO_TITLE) == 0 )
	{
		if (m_bHasFocus)
		{
			pcView->SetFgColor( 0,0,0,255 );
		}
		else
		{
			pcView->SetFgColor( 50, 50, 50, 50 );
		}
		pcView->SetBgColor( sFillColor );
		pcView->SetDrawingMode( DM_OVER );
		pcView->MovePenTo( m_cDragRect.left + 5,
			   m_cDragRect.top + m_cDragRect.Height() / 2 -
			   (m_sFontHeight.ascender + m_sFontHeight.descender) / 2 + m_sFontHeight.ascender );
		pcView->DrawString( m_cTitle.c_str(), -1 );
		pcView->SetDrawingMode( DM_COPY );
	}
	else
	{
		pcView->FillRect( Rect( cOBounds.left + 1, cOBounds.top - 1, cOBounds.right - 1, cIBounds.top + 1 ), sFillColor );
	}

	if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 )
	{
		DrawZoom( m_cZoomRect, m_bHasFocus, m_bZoomState == 1 );
	}
	if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )
	{
		DrawDepth( m_cToggleRect, m_bHasFocus, m_bDepthState == 1 );
	}
}

extern "C" int get_api_version()
{
    return( WNDDECORATOR_APIVERSION );
}

extern "C" WindowDecorator* create_decorator( Layer* pcLayer, uint32 nFlags )
{
    return( new MulvaneDecorator( pcLayer, nFlags ) );
}
