/*
 *  "Red" Window-Decorator
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

#include "RedDecorator.h"
#include <layer.h>

#include <gui/window.h>

using namespace os;

RedDecorator::RedDecorator( Layer* pcLayer, uint32 nWndFlags )
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

RedDecorator::~RedDecorator()
{
}

void RedDecorator::CalculateBorderSizes()
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
		    m_vTopBorder = 4;
		}
		else
		{
//			m_vTopBorder = float(m_sFontHeight.ascender + m_sFontHeight.descender + 6);
			m_vTopBorder = 16;
		}
		m_vLeftBorder   = 4;
		m_vRightBorder  = 4;
		m_vBottomBorder = 4;
	}
}

void RedDecorator::FontChanged()
{
	Layer* pcView = GetLayer();
	m_sFontHeight = pcView->GetFontHeight();
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

void RedDecorator::SetFlags( uint32 nFlags )
{
	Layer* pcView = GetLayer();
	m_nFlags = nFlags;
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

Point RedDecorator::GetMinimumSize()
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

WindowDecorator::hit_item RedDecorator::HitTest( const Point& cPosition )
{
	if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE )
	{
		if ( cPosition.y < 2 )
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
		else if ( cPosition.y > m_cBounds.bottom - 4 )
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
		else if ( cPosition.x < 4 )
		{
			return( HIT_SIZE_LEFT );
		}
		else if ( cPosition.x > m_cBounds.right - 4 )
		{
			return( HIT_SIZE_RIGHT );
		}
	}

	if ( m_cCloseRect.DoIntersect( cPosition ) )
	{
		return( HIT_CLOSE );
	}
//	else if ( m_cCloseRect2.DoIntersect( cPosition ) )
//	{
//		return( HIT_CLOSE );
//	}
	else if ( m_cZoomRect.DoIntersect( cPosition ) )
	{
		return( HIT_ZOOM );
	}
	else if ( m_cToggleRect.DoIntersect( cPosition ) )
	{
		return( HIT_DEPTH );
	}
	else if ( m_cDragRect.DoIntersect( cPosition ) )
	{
		return( HIT_DRAG );
	}
	return( HIT_NONE );
}

Rect RedDecorator::GetBorderSize()
{
	return( Rect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void RedDecorator::SetTitle( const char* pzTitle )
{
	m_cTitle = pzTitle;
	Render( m_cBounds );
}

void RedDecorator::SetFocusState( bool bHasFocus )
{
	m_bHasFocus = bHasFocus;
	Render( m_cBounds );
}

void RedDecorator::SetWindowFlags( uint32 nFlags )
{
	m_nFlags = nFlags;
}

void RedDecorator::FrameSized( const Rect& cFrame )
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

void RedDecorator::Layout()
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
		m_cCloseRect.left   = 0;
		m_cCloseRect.right  = 15;
		m_cCloseRect.top    = 0;
		m_cCloseRect.bottom = 15;
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
	m_cToggleRect.top = 0;
	m_cToggleRect.bottom = 15;

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
	m_cZoomRect.top    = 0;
	m_cZoomRect.bottom = 15;

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
    m_cDragRect.top    = 0;
    m_cDragRect.bottom = 15;
}

void RedDecorator::SetButtonState( uint32 nButton, bool bPushed )
{
	switch( nButton )
	{
	}
}

void RedDecorator::SetCloseButtonState( bool bPushed )
{
	m_bCloseState = bPushed;
	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	}
}

void RedDecorator::SetZoomButtonState( bool bPushed )
{
	m_bZoomState = bPushed;
	if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 )
	{
		DrawZoom( m_cZoomRect, m_bHasFocus, m_bZoomState == 1 );
	}
}

void RedDecorator::SetMinimizeButtonState( bool bPushed )
{
	m_bZoomState = bPushed;
	if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 )
	{
		DrawZoom( m_cZoomRect, m_bHasFocus, m_bZoomState == 1 );
	}
}

void RedDecorator::SetDepthButtonState( bool bPushed )
{
	m_bDepthState = bPushed;
	if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )
	{
		DrawDepth( m_cToggleRect, m_bHasFocus, m_bDepthState == 1 );
	}
}

void RedDecorator::DrawPanel( const Rect& cRect, bool bActive, bool bRecessed )
{
	Rect		R;
	Layer*		pcView = GetLayer();
	Color32_s	col_L2 = Color32_s(171, 103, 104);
	Color32_s	col_L  = Color32_s(135,  41,  41);
	Color32_s	col_D  = Color32_s(101,  10,   9);
	Color32_s	col_D2 = Color32_s( 67,  17,  20);
	Color32_s	col_D3 = Color32_s( 48,   0,   2);

	if (bActive)
	{
		col_L2 = Color32_s(171, 103, 104);
		col_L  = Color32_s(135,  41,  41);
		col_D  = Color32_s(101,  10,   9);
		col_D2 = Color32_s( 67,  17,  20);
		col_D3 = Color32_s( 48,   0,   2);
	}
	else
	{
		col_L2 = Color32_s(103, 103, 103);
		col_L  = Color32_s( 64,  64,  64);
		col_D  = Color32_s( 48,  48,  48);
		col_D2 = Color32_s( 32,  32,  32);
		col_D3 = Color32_s( 16,  16,  16);
	}

	// Main body
	R.left   = cRect.left   + 1;
	R.top    = cRect.top    + 1;
	R.right  = cRect.right  - 2;
	R.bottom = cRect.bottom - 2;
	pcView->FillRect( cRect, col_D );
	// Highlight
	pcView->SetFgColor( col_L2.red, col_L2.green, col_L2.blue, 255 );
	pcView->DrawLine( Point(cRect.left,cRect.top), Point(cRect.right-1,cRect.top) );
	pcView->DrawLine( Point(cRect.left,cRect.top), Point(cRect.left,   cRect.bottom-1) );
	// Shadow
	pcView->SetFgColor( col_D2.red, col_D2.green, col_D2.blue, 255 );
	pcView->DrawLine( Point(cRect.left +1,cRect.bottom-1), Point(cRect.right-1,cRect.bottom-1) );
	pcView->DrawLine( Point(cRect.right-1,cRect.bottom-1), Point(cRect.right-1,cRect.top+1) );
	pcView->SetFgColor( col_D3.red, col_D3.green, col_D3.blue, 255 );
	pcView->DrawLine( Point(cRect.left, cRect.bottom), Point(cRect.right,cRect.bottom) );
	pcView->DrawLine( Point(cRect.right,cRect.bottom), Point(cRect.right,cRect.top) );
	// Stripes
	Point Left  = Point(cRect.left+1,  cRect.top+2);
	Point Right = Point(cRect.right-1, cRect.top+2);
	pcView->SetFgColor( col_L.red, col_L.green, col_L.blue, 255 );
	for (int i=0; i<6; i++)
	{
		pcView->DrawLine( Left, Right );
		Left.y  += 2;
		Right.y += 2;
	}
}

void RedDecorator::DrawDepth( const Rect& cRect, bool bActive, bool bRecessed )
{
	Layer* pcView = GetLayer();

	DrawPanel(cRect, bActive, bRecessed);

	if ( bRecessed )
	{
		pcView->SetFgColor( 255, 255, 255, 255 );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 3), Point(cRect.left+11,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+11,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+ 3,cRect.top+ 9) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 9), Point(cRect.left+ 6,cRect.top+ 9) );
		pcView->DrawLine( Point(cRect.left+11,cRect.top+ 4), Point(cRect.left+11,cRect.top+ 6) );
		pcView->DrawLine( Point(cRect.left+ 6,cRect.top+ 6), Point(cRect.left+14,cRect.top+ 6) );
		pcView->DrawLine( Point(cRect.left+ 6,cRect.top+ 7), Point(cRect.left+14,cRect.top+ 7) );
		pcView->DrawLine( Point(cRect.left+ 6,cRect.top+ 7), Point(cRect.left+ 6,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 6,cRect.top+12), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+14,cRect.top+ 7), Point(cRect.left+14,cRect.top+12) );
	}
	else
	{
		pcView->SetFgColor( 0, 0, 0, 255 );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 3), Point(cRect.left+11,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+11,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+ 3,cRect.top+ 9) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 9), Point(cRect.left+ 6,cRect.top+ 9) );
		pcView->DrawLine( Point(cRect.left+11,cRect.top+ 4), Point(cRect.left+11,cRect.top+ 6) );
		pcView->DrawLine( Point(cRect.left+ 6,cRect.top+ 6), Point(cRect.left+14,cRect.top+ 6) );
		pcView->DrawLine( Point(cRect.left+ 6,cRect.top+ 7), Point(cRect.left+14,cRect.top+ 7) );
		pcView->DrawLine( Point(cRect.left+ 6,cRect.top+ 7), Point(cRect.left+ 6,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 6,cRect.top+12), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+14,cRect.top+ 7), Point(cRect.left+14,cRect.top+12) );
		pcView->SetFgColor( 255, 255, 255, 255 );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 2), Point(cRect.left+10,cRect.top+ 2) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 3), Point(cRect.left+10,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 3), Point(cRect.left+ 2,cRect.top+ 8) );
		pcView->DrawLine( Point(cRect.left+ 2,cRect.top+ 8), Point(cRect.left+ 5,cRect.top+ 8) );
		pcView->DrawLine( Point(cRect.left+10,cRect.top+ 3), Point(cRect.left+10,cRect.top+ 5) );
		pcView->DrawLine( Point(cRect.left+ 5,cRect.top+ 5), Point(cRect.left+13,cRect.top+ 5) );
		pcView->DrawLine( Point(cRect.left+ 5,cRect.top+ 6), Point(cRect.left+13,cRect.top+ 6) );
		pcView->DrawLine( Point(cRect.left+ 5,cRect.top+ 6), Point(cRect.left+ 5,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+ 5,cRect.top+11), Point(cRect.left+13,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+13,cRect.top+ 6), Point(cRect.left+13,cRect.top+11) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void RedDecorator::DrawZoom(  const Rect& cRect, bool bActive, bool bRecessed )
{
	Layer* pcView = GetLayer();

	DrawPanel(cRect, bActive, bRecessed);

	if ( bRecessed )
	{
		pcView->SetFgColor( 255, 255, 255, 255 );
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
		pcView->SetFgColor( 0, 0, 0, 255 );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 3), Point(cRect.left+14,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+14,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 4), Point(cRect.left+ 3,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+12), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+14,cRect.top+ 4), Point(cRect.left+14,cRect.top+12) );
		pcView->DrawLine( Point(cRect.left+ 3,cRect.top+ 8), Point(cRect.left+10,cRect.top+ 8) );
		pcView->DrawLine( Point(cRect.left+10,cRect.top+ 4), Point(cRect.left+10,cRect.top+ 8) );
		pcView->SetFgColor( 255, 255, 255, 255 );
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

void RedDecorator::DrawClose(  const Rect& cRect, bool bActive, bool bRecessed  )
{
	Layer* pcView = GetLayer();

	DrawPanel(cRect, bActive, bRecessed);

	if ( bRecessed )
	{
		pcView->SetFgColor( 255, 255, 255, 255 );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+ 4), Point(cRect.left+11,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+ 4), Point(cRect.left+12,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+11), Point(cRect.left+11,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+11), Point(cRect.left+12,cRect.top+ 4) );
	}
	else
	{
		pcView->SetFgColor( 0, 0, 0, 255 );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+ 4), Point(cRect.left+11,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+ 4), Point(cRect.left+12,cRect.top+11) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+11), Point(cRect.left+11,cRect.top+ 4) );
		pcView->DrawLine( Point(cRect.left+5,cRect.top+11), Point(cRect.left+12,cRect.top+ 4) );
		pcView->SetFgColor( 255, 255, 255, 255 );
		pcView->DrawLine( Point(cRect.left+3,cRect.top+ 3), Point(cRect.left+10,cRect.top+10) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+ 3), Point(cRect.left+11,cRect.top+10) );
		pcView->DrawLine( Point(cRect.left+3,cRect.top+10), Point(cRect.left+10,cRect.top+ 3) );
		pcView->DrawLine( Point(cRect.left+4,cRect.top+10), Point(cRect.left+11,cRect.top+ 3) );
	}
}

void RedDecorator::Render( const Rect& cUpdateRect )
{
	if ( m_nFlags & WND_NO_BORDER )
	{
		return;
	}
  
	Color32_s sFillColor =  m_bHasFocus ? Color32_s(135,41,41,255) : Color32_s(64,64,64,255);

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

   if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	}

	if ( (m_nFlags & WND_NO_TITLE) == 0 )
	{
		DrawPanel(m_cDragRect, m_bHasFocus, false);
		pcView->SetFgColor( 255, 255, 255, 0 );
		pcView->SetBgColor( sFillColor );
		pcView->MovePenTo( m_cDragRect.left + 5,
			   m_cDragRect.Height() / 2 -
			   (m_sFontHeight.ascender + m_sFontHeight.descender) / 2 + m_sFontHeight.ascender );
		pcView->DrawString( m_cTitle.c_str(), -1 );
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
    return( new RedDecorator( pcLayer, nFlags ) );
}
