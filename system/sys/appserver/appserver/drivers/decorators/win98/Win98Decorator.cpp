/*
 *  "Win98" Window-Decorator
 *  Copyright (C) 2002 Iain Hutchison
 *
 *  Updated Jul 2004 by Mike Saunders
 *  (Added minimize; fixed resize; better buttons and icon; fixed glitches)
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

#include "Win98Decorator.h"
#include <layer.h>

#include <gui/window.h>

using namespace os;

/* A few constants */
static const int BORDER_SIZE  = 3;	// Total thickness of border (including frame lines)
static const int FRAME_SIZE   = 1;	// Thickness of a frame line (2 in each border)
static const int TITLE_HEIGHT = 19;	// Height of title-text area
static const int BUTTON_SIZE  = 17;


/* Colour blending function stolen (does standard alpha-blend between 2 colours) */
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


Win98Decorator::Win98Decorator( Layer* pcLayer, uint32 nWndFlags )
:	WindowDecorator( pcLayer, nWndFlags )
{
    m_nFlags = nWndFlags;

    m_bHasFocus   = false;
    m_bCloseState = false;
    m_bZoomState  = false;
    m_bDepthState = false;
    m_bMinimizeState = false;

    m_sFontHeight = pcLayer->GetFontHeight();
  
    CalculateBorderSizes();
}

Win98Decorator::~Win98Decorator()
{
}

void Win98Decorator::CalculateBorderSizes()
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
		    m_vTopBorder = BORDER_SIZE * 2;
		}
		else
		{
			m_vTopBorder = BORDER_SIZE * 2 + TITLE_HEIGHT;
		}
		m_vLeftBorder   = BORDER_SIZE * 2;
		m_vRightBorder  = BORDER_SIZE * 2;
		m_vBottomBorder = BORDER_SIZE * 2;
	}
}

void Win98Decorator::FontChanged()
{
	Layer* pcView = GetLayer();
	m_sFontHeight = pcView->GetFontHeight();
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

void Win98Decorator::SetFlags( uint32 nFlags )
{
	Layer* pcView = GetLayer();
	m_nFlags = nFlags;
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

Point Win98Decorator::GetMinimumSize()
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

WindowDecorator::hit_item Win98Decorator::HitTest( const Point& cPosition )
{
	if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE )
	{
		if ( cPosition.y < BORDER_SIZE )
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
		else if ( cPosition.y < BORDER_SIZE + 16 )
		{
			if ( cPosition.x < BORDER_SIZE + 2 )
			{
				return( HIT_SIZE_LT );
			}
			else if ( cPosition.x > m_cBounds.right - BORDER_SIZE - 1 )
			{
				return( HIT_SIZE_RT );
			}
		}
		else if ( cPosition.y > m_cBounds.bottom - BORDER_SIZE - 2 )
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
		else if ( cPosition.y > m_cBounds.bottom - BORDER_SIZE - 18 )
		{
			if ( cPosition.x < BORDER_SIZE + 2 )
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
		else if ( cPosition.x < BORDER_SIZE + 2 )
		{
			return( HIT_SIZE_LEFT );
		}
		else if ( cPosition.x > m_cBounds.right - BORDER_SIZE - 2 )
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
	else if ( m_cMinimizeRect.DoIntersect( cPosition ) )
	{
		return( HIT_MINIMIZE );
	}
	else if ( m_cDragRect.DoIntersect( cPosition ) )
	{
		return( HIT_DRAG );
	}
	return( HIT_NONE );
}

Rect Win98Decorator::GetBorderSize()
{
	return( Rect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void Win98Decorator::SetTitle( const char* pzTitle )
{
	m_cTitle = pzTitle;
	Render( m_cBounds );
}

void Win98Decorator::SetFocusState( bool bHasFocus )
{
	m_bHasFocus = bHasFocus;
	Render( m_cBounds );
}

void Win98Decorator::SetWindowFlags( uint32 nFlags )
{
	m_nFlags = nFlags;
}

void Win98Decorator::FrameSized( const Rect& cFrame )
{
	Layer* pcView = GetLayer();
	Point cDelta( cFrame.Width() - m_cBounds.Width(), cFrame.Height() - m_cBounds.Height() );
	m_cBounds = cFrame.Bounds();

	Layout();
	if ( cDelta.x != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.left = m_cMinimizeRect.left - fabs(cDelta.x)  - 2.0f;
		pcView->Invalidate( cDamage );
	}
	if ( cDelta.y != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.top = cDamage.bottom - std::max( m_vBottomBorder, m_vBottomBorder + cDelta.y ) - 1.0f;
		pcView->Invalidate( cDamage );
	}
}

void Win98Decorator::Layout()
{
	float vRight = m_cBounds.right - 1 - BORDER_SIZE;
	// Close button
	if ( m_nFlags & WND_NO_CLOSE_BUT )
	{
		m_cCloseRect.left  = m_cCloseRect.right = 0;
	}
	else
	{
		m_cCloseRect.right = vRight - 1;
		m_cCloseRect.left  = m_cCloseRect.right - BUTTON_SIZE;
		vRight = m_cCloseRect.left - 1;
	}
	m_cCloseRect.top    = BORDER_SIZE + 1 + (TITLE_HEIGHT - BUTTON_SIZE)/2;
	m_cCloseRect.bottom = m_cCloseRect.top + BUTTON_SIZE - 2;

	// Zoom button
	if ( m_nFlags & WND_NO_ZOOM_BUT )
	{
		m_cZoomRect.left = m_cZoomRect.right = 0;
	}
	else
	{
		m_cZoomRect.right = vRight - 1;
		m_cZoomRect.left  = m_cZoomRect.right - BUTTON_SIZE;
		vRight = m_cZoomRect.left;
	}

	// Minimize button
	m_cMinimizeRect.right = vRight;
	m_cMinimizeRect.left  = m_cMinimizeRect.right - BUTTON_SIZE;
	vRight = m_cMinimizeRect.left;

	m_cMinimizeRect.top    = m_cCloseRect.top;
	m_cMinimizeRect.bottom = m_cCloseRect.bottom;
	m_cZoomRect.top    = m_cCloseRect.top;
	m_cZoomRect.bottom = m_cCloseRect.bottom;
	// Title area (dragable)
	m_cDragRect.right  = vRight - 1;
	m_cDragRect.left   = m_cBounds.left + BORDER_SIZE + TITLE_HEIGHT;
	m_cDragRect.top    = BORDER_SIZE;
	m_cDragRect.bottom = BORDER_SIZE + TITLE_HEIGHT - 1;
}

void Win98Decorator::SetButtonState( uint32 nButton, bool bPushed )
{
	switch( nButton )
	{
		case HIT_CLOSE:
			SetCloseButtonState( bPushed );
			break;
		case HIT_ZOOM:
			SetZoomButtonState( bPushed );
			break;
		case HIT_DEPTH:
			SetDepthButtonState( bPushed );
			break;
		case HIT_MINIMIZE:
			SetMinimizeButtonState( bPushed );
			break;
	}
}

void Win98Decorator::SetCloseButtonState( bool bPushed )
{
	m_bCloseState = bPushed;
	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	}
}

void Win98Decorator::SetZoomButtonState( bool bPushed )
{
	m_bZoomState = bPushed;
	if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 )
	{
		DrawZoom( m_cZoomRect, m_bHasFocus, m_bZoomState == 1 );
	}
}

void Win98Decorator::SetDepthButtonState( bool bPushed )
{
	m_bDepthState = bPushed;
	if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )
	{
		DrawDepth( m_cToggleRect, m_bHasFocus, m_bDepthState == 1 );
	}
}

void Win98Decorator::SetMinimizeButtonState( bool bPushed )
{
	m_bMinimizeState = bPushed;
	DrawMinimize( m_cMinimizeRect, m_bHasFocus, m_bMinimizeState == 1 );
}

void Win98Decorator::DrawFrame(const Rect& cRect, bool bOuter)
{
	Rect		R = cRect;
	Layer*		pcView = GetLayer();
	Color32_s	col_White  = Color32_s(255,255,255,255);
	Color32_s	col_Black  = Color32_s(  0,  0,  0,255);
	Color32_s	col_Normal = get_default_color(COL_NORMAL);
	Color32_s	col_Light  = BlendColours(col_Normal, col_White, 0.5f);
	Color32_s	col_Dark   = BlendColours(col_Normal, col_Black, 0.5f);

	if (bOuter)
	{
		pcView->SetFgColor(col_Normal);
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.left   ,R.bottom-1) );
		pcView->SetFgColor(col_Black);
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
		R.left   += 1;	R.top    += 1;	R.right  -= 1;	R.bottom -= 1;
		pcView->SetFgColor(col_Light);
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.left   ,R.bottom-1) );
		pcView->SetFgColor(col_Dark);
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
		R.left   += 1;	R.top    += 1;	R.right  -= 1;	R.bottom -= 1;
		pcView->SetFgColor(col_Normal);
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.left   ,R.bottom-1) );
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
		// Fix gap 1px to right of title-bar BG
		pcView->DrawLine( Point(R.right-1,R.top+FRAME_SIZE), Point(R.right-1,  R.top+TITLE_HEIGHT ) );
	}
	else
	{
		pcView->SetFgColor(col_Normal);
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.left   ,R.bottom-1) );
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
		R.left   += 1;	R.top    += 1;	R.right  -= 1;	R.bottom -= 1;
		pcView->SetFgColor(col_Dark);
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.left   ,R.bottom-1) );
		pcView->SetFgColor(col_Light);
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
		R.left   += 1;	R.top    += 1;	R.right  -= 1;	R.bottom -= 1;
		pcView->SetFgColor(col_Black);
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.left   ,R.bottom-1) );
		pcView->SetFgColor(col_Normal);
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
	}
}

void Win98Decorator::DrawButton(const Rect& cRect, bool bRecessed)
{
	Rect		R = cRect;
	Layer*		pcView = GetLayer();
	Color32_s	col_White  = Color32_s(255,255,255,255);
	Color32_s	col_Black  = Color32_s(  0,  0,  0,255);
	Color32_s	col_Normal = get_default_color(COL_NORMAL);
	Color32_s	col_Light  = BlendColours(col_Normal, col_White, 0.5f);
	Color32_s	col_Dark   = BlendColours(col_Normal, col_Black, 0.5f);

	R.right  -= 1;
	R.bottom -= 1;

	if (bRecessed)
	{
		pcView->SetFgColor(col_Black);
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.left   ,R.bottom-1) );
		pcView->SetFgColor(col_Normal);
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
		R.left   += 1;	R.top    += 1;	R.right  -= 1;	R.bottom -= 1;
		pcView->SetFgColor(col_Dark);
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.left   ,R.bottom-1) );
		pcView->SetFgColor(col_Light);
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
		R.left   += 1;	R.top    += 1;	R.right  -= 1;	R.bottom -= 1;
	}
	else
	{
		pcView->SetFgColor(col_Normal);
		pcView->DrawLine( Point(R.left,R.top    ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left,R.top    ), Point(R.left   ,R.bottom-1) );
		pcView->SetFgColor(col_Black);
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
		R.left   += 1;	R.top    += 1;	R.right  -= 1;	R.bottom -= 1;
		pcView->SetFgColor(col_Light);
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.right-1,R.top     ) );
		pcView->DrawLine( Point(R.left, R.top   ), Point(R.left   ,R.bottom-1) );
		pcView->SetFgColor(col_Dark);
		pcView->DrawLine( Point(R.left, R.bottom), Point(R.right,  R.bottom  ) );
		pcView->DrawLine( Point(R.right,R.top   ), Point(R.right,  R.bottom  ) );
		R.left   += 1;	R.top    += 1;	R.right  -= 1;	R.bottom -= 1;
	}
	pcView->FillRect(R, col_Normal);
}

void Win98Decorator::DrawDepth( const Rect& cRect, bool bActive, bool bRecessed )
{
	Rect   R      = cRect;
	Layer* pcView = GetLayer();

	DrawButton(cRect, bRecessed);
	
	if (bRecessed)
	{
		R.left +=1; R.top += 1;
	}

	pcView->SetFgColor( 0, 0, 0, 255 );
	pcView->DrawLine( Point(R.left+ 2,R.top+ 2), Point(R.left+10,R.top+ 2) );
	pcView->DrawLine( Point(R.left+ 2,R.top+ 3), Point(R.left+10,R.top+ 3) );
	pcView->DrawLine( Point(R.left+ 2,R.top+ 3), Point(R.left+ 2,R.top+ 8) );
	pcView->DrawLine( Point(R.left+ 2,R.top+ 8), Point(R.left+ 5,R.top+ 8) );
	pcView->DrawLine( Point(R.left+10,R.top+ 3), Point(R.left+10,R.top+ 5) );
	pcView->DrawLine( Point(R.left+ 5,R.top+ 5), Point(R.left+13,R.top+ 5) );
	pcView->DrawLine( Point(R.left+ 5,R.top+ 6), Point(R.left+13,R.top+ 6) );
	pcView->DrawLine( Point(R.left+ 5,R.top+ 6), Point(R.left+ 5,R.top+11) );
	pcView->DrawLine( Point(R.left+ 5,R.top+11), Point(R.left+13,R.top+11) );
	pcView->DrawLine( Point(R.left+13,R.top+ 6), Point(R.left+13,R.top+11) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Win98Decorator::DrawZoom(  const Rect& cRect, bool bActive, bool bRecessed )
{
	Rect   R      = cRect;
	Layer* pcView = GetLayer();

	DrawButton(cRect, bRecessed);

	if (bRecessed)
	{
		R.left +=1; R.top += 1;
	}

	pcView->SetFgColor( 0, 0, 0, 255 );
	pcView->DrawLine( Point(R.left+4,R.top+ 3), Point(R.left+11,R.top+3) );
	pcView->DrawLine( Point(R.left+4,R.top+ 4), Point(R.left+11,R.top+4) );
	pcView->DrawLine( Point(R.left+4,R.top+ 3), Point(R.left+4,R.top+10) );
	pcView->DrawLine( Point(R.left+4,R.top+ 10), Point(R.left+11,R.top+10) );
	pcView->DrawLine( Point(R.left+11,R.top+ 10), Point(R.left+11,R.top+3) );
}

void Win98Decorator::DrawMinimize(  const Rect& cRect, bool bActive, bool bRecessed )
{
	Rect   R      = cRect;
	Layer* pcView = GetLayer();

	DrawButton(cRect, bRecessed);

	if (bRecessed)
	{
		R.left +=1; R.top += 1;
	}

	pcView->SetFgColor( 0, 0, 0, 255 );
	pcView->DrawLine( Point(R.left+4,R.top+ 9), Point(R.left+9,R.top+9) );
	pcView->DrawLine( Point(R.left+4,R.top+ 10), Point(R.left+9,R.top+10) );
}

void Win98Decorator::DrawIcon(const Rect& cRect)
{
	Rect		R		= cRect;
	Layer*		pcView		= GetLayer();
	Color32_s	col_Orange	= Color32_s(224,171,74);
	Color32_s	col_Blue	= Color32_s(120,149,191);

	// Background oval...
	pcView->SetFgColor(col_Orange);
	R.top = cRect.top + 2; R.bottom = R.top + 11;
	R.left = cRect.left + 4; R.right = R.left + 7;
	pcView->FillRect(R, col_Orange);
	pcView->DrawLine( Point(cRect.left+6,cRect.top+1), Point(cRect.left+9,cRect.top+1) );
	pcView->DrawLine( Point(cRect.left+6,cRect.top+14), Point(cRect.left+9,cRect.top+14) );
	pcView->DrawLine( Point(cRect.left+2,cRect.top+5), Point(cRect.left+2,cRect.top+10) );
	pcView->DrawLine( Point(cRect.left+3,cRect.top+3), Point(cRect.left+3,cRect.top+12) );
	pcView->DrawLine( Point(cRect.left+12,cRect.top+3), Point(cRect.left+12,cRect.top+12) );
	pcView->DrawLine( Point(cRect.left+13,cRect.top+5), Point(cRect.left+13,cRect.top+10) );

	// And the famous blue S :-)
	pcView->SetFgColor(col_Blue);
	pcView->DrawLine( Point(cRect.left+6,cRect.top+3), Point(cRect.left+9,cRect.top+3) );
	pcView->DrawLine( Point(cRect.left+5,cRect.top+4), Point(cRect.left+10,cRect.top+4) );
	pcView->DrawLine( Point(cRect.left+4,cRect.top+5), Point(cRect.left+10,cRect.top+5) );
	pcView->DrawLine( Point(cRect.left+4,cRect.top+6), Point(cRect.left+7,cRect.top+6) );
	pcView->DrawLine( Point(cRect.left+5,cRect.top+7), Point(cRect.left+8,cRect.top+7) );
	pcView->DrawLine( Point(cRect.left+6,cRect.top+8), Point(cRect.left+9,cRect.top+8) );
	pcView->DrawLine( Point(cRect.left+7,cRect.top+9), Point(cRect.left+10,cRect.top+9) );
	pcView->DrawLine( Point(cRect.left+8,cRect.top+10), Point(cRect.left+11,cRect.top+10) );
	pcView->DrawLine( Point(cRect.left+4,cRect.top+11), Point(cRect.left+11,cRect.top+11) );
	pcView->DrawLine( Point(cRect.left+5,cRect.top+12), Point(cRect.left+10,cRect.top+12) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Win98Decorator::DrawClose(  const Rect& cRect, bool bActive, bool bRecessed  )
{
	Rect   R      = cRect;
	Layer* pcView = GetLayer();

	DrawButton(cRect, bRecessed);

	if (bRecessed)
	{
		R.left +=1; R.top += 1;
	}

	pcView->SetFgColor( 0, 0, 0, 255 );
	pcView->DrawLine( Point(R.left+4,R.top+ 4), Point(R.left+10,R.top+10) );
	pcView->DrawLine( Point(R.left+5,R.top+ 4), Point(R.left+11,R.top+10) );
	pcView->DrawLine( Point(R.left+4,R.top+ 10), Point(R.left+10,R.top+ 4) );
	pcView->DrawLine( Point(R.left+5,R.top+ 10), Point(R.left+11,R.top+ 4) );
}

void Win98Decorator::Render( const Rect& cUpdateRect )
{
	// Quick exit if there's nothing to render
	if ( m_nFlags & WND_NO_BORDER )
	{
		return;
	}

	// Get title-bar fill-colour  
	Color32_s sFillColor = m_bHasFocus ? get_default_color(COL_SEL_WND_BORDER) : get_default_color(COL_NORMAL_WND_BORDER);
	// Get the layer object that we're drawing into
	Layer* pcView = GetLayer();
	// Calculate the bounds for the inner and outer rectangles
	Rect  cOBounds = pcView->GetBounds();
	Rect  cIBounds = cOBounds;
	cIBounds.left   += BORDER_SIZE;
	cIBounds.right  -= BORDER_SIZE;
	cIBounds.top    += BORDER_SIZE;
	cIBounds.bottom -= BORDER_SIZE;
	if ( (m_nFlags & (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE)) !=
	     (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE) )
	{
		cIBounds.top    += TITLE_HEIGHT;
	}

	// Draw outer frame
	DrawFrame( cOBounds, true  );
	// Fill the area between the inner and outer bounds (title BG)
	Rect cTitleBG;
	cTitleBG.left   = cIBounds.left;
	cTitleBG.top    = cOBounds.top   + BORDER_SIZE;
	cTitleBG.right  = cIBounds.right - 1;
	cTitleBG.bottom = cIBounds.top   - 1;
	if (cTitleBG.Height() > 0)
	{
		pcView->FillRect(cTitleBG, sFillColor);
		cTitleBG.left  += (TITLE_HEIGHT-16)/2;
		cTitleBG.top   += (TITLE_HEIGHT-16)/2;
		cTitleBG.right  = cTitleBG.left + 15;
		cTitleBG.bottom = cTitleBG.top  + 15;
		DrawIcon(cTitleBG);
	}
	// Draw inner frame
	DrawFrame( cIBounds, false );
#if 0
	// Draw corner markers on frames
	if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE )
	{
		pcView->SetFgColor( 0, 0, 0, 255 );
		pcView->DrawLine( Point( 15, cIBounds.bottom + 1 ), Point( 15, cOBounds.bottom - 1 ) );
		pcView->DrawLine( Point( cOBounds.right - 16, cIBounds.bottom + 1 ), Point( cOBounds.right - 16, cOBounds.bottom - 1 ) );
		pcView->SetFgColor( 255, 255, 255, 255 );
		pcView->DrawLine( Point( 16, cIBounds.bottom + 1 ), Point( 16, cOBounds.bottom - 1 ) );
		pcView->DrawLine( Point( cOBounds.right - 15, cIBounds.bottom + 1 ), Point( cOBounds.right - 15, cOBounds.bottom - 1 ) );
	}
#endif
	// Draw title text
	if ( (m_nFlags & WND_NO_TITLE) == 0 )
	{
		pcView->SetFgColor( 255, 255, 255, 0 );
		pcView->SetBgColor( sFillColor );
		pcView->MovePenTo( m_cDragRect.left + 2,
			m_cDragRect.top + m_cDragRect.Height() / 2 -
			(m_sFontHeight.ascender + m_sFontHeight.descender) / 2 + m_sFontHeight.ascender );
		pcView->DrawString( m_cTitle.c_str(), -1 );
	}

	// Draw MINIMIZE button
	DrawMinimize( m_cMinimizeRect, m_bHasFocus, m_bMinimizeState == 1 );
	// Draw ZOOM button
	if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 )
	{
		DrawZoom( m_cZoomRect, m_bHasFocus, m_bZoomState == 1 );
	}
	// Draw CLOSE button
	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	}
}

extern "C" int get_api_version()
{
    return( WNDDECORATOR_APIVERSION );
}

extern "C" WindowDecorator* create_decorator( Layer* pcLayer, uint32 nFlags )
{
    return( new Win98Decorator( pcLayer, nFlags ) );
}
