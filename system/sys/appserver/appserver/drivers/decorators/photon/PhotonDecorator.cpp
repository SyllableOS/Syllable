/*
 *  "Photon" Window-Decorator
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

#include "PhotonDecorator.h"
#include <layer.h>

#include <gui/window.h>

using namespace os;

/* A few constants */
static const int BORDER_SIZE  = 4;	// Total thickness of border (including frame lines)
static const int CORNER_SIZE  = 16;	// Size of diagonal resize area
static const int TITLE_HEIGHT = 20;	// Height of title-text area
static const int BUTTON_SIZE  = 12;

/* Colour blending function stolen (does "standard" alpha-blend between 2 colours) */
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


PhotonDecorator::PhotonDecorator( Layer* pcLayer, uint32 nWndFlags )
:	WindowDecorator( pcLayer, nWndFlags )
{
    m_nFlags = nWndFlags;

    m_bHasFocus   = false;
    m_bCloseState = false;
    m_bZoomState  = false;
    m_bMinimizeState = false;

    m_sFontHeight = pcLayer->GetFontHeight();
  
    CalculateBorderSizes();
}

PhotonDecorator::~PhotonDecorator()
{
}

void PhotonDecorator::CalculateBorderSizes()
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
		    m_vTopBorder = BORDER_SIZE;
		}
		else
		{
			m_vTopBorder = TITLE_HEIGHT;
		}
		m_vLeftBorder   = BORDER_SIZE;
		m_vRightBorder  = BORDER_SIZE;
		m_vBottomBorder = BORDER_SIZE;
	}
}

void PhotonDecorator::FontChanged()
{
	Layer* pcView = GetLayer();
	m_sFontHeight = pcView->GetFontHeight();
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

void PhotonDecorator::SetFlags( uint32 nFlags )
{
	Layer* pcView = GetLayer();
	m_nFlags = nFlags;
	CalculateBorderSizes();
	pcView->Invalidate();
	Layout();
}

Point PhotonDecorator::GetMinimumSize()
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
		cMinSize.x += m_cMinimizeRect.Width();
	}
	if ( cMinSize.x < m_vLeftBorder + m_vRightBorder )
	{
		cMinSize.x = m_vLeftBorder + m_vRightBorder;
	}
	return( cMinSize );
}

WindowDecorator::hit_item PhotonDecorator::HitTest( const Point& cPosition )
{
	if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE )
	{
		if ( cPosition.y < 2 )
		{
			if ( cPosition.x < CORNER_SIZE )
			{
				return( HIT_SIZE_LT );
			}
			else if ( cPosition.x > m_cBounds.right - CORNER_SIZE )
			{
				return( HIT_SIZE_RT );
			}
			else
			{
				return( HIT_SIZE_TOP );
			}
		}
		else if ( cPosition.y > m_cBounds.bottom - BORDER_SIZE )
		{
			if ( cPosition.x < CORNER_SIZE )
			{
				return( HIT_SIZE_LB );
			}
			else if ( cPosition.x > m_cBounds.right - CORNER_SIZE )
			{
				return( HIT_SIZE_RB );
			}
			else
			{
				return( HIT_SIZE_BOTTOM );
			}
		}
		else if ( cPosition.y > m_cBounds.bottom - CORNER_SIZE )
		{
			if ( cPosition.x < BORDER_SIZE )
			{
				return( HIT_SIZE_LB );
			}
			else if ( cPosition.x > m_cBounds.right - CORNER_SIZE )
			{
				return( HIT_SIZE_RB );
			}
			else
			{
				return( HIT_SIZE_BOTTOM );
			}
		}
		else if ( cPosition.x < BORDER_SIZE )
		{
			return( HIT_SIZE_LEFT );
		}
		else if ( cPosition.x > m_cBounds.right - BORDER_SIZE )
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

Rect PhotonDecorator::GetBorderSize()
{
	return( Rect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void PhotonDecorator::SetTitle( const char* pzTitle )
{
	m_cTitle = pzTitle;
	Render( m_cBounds );
}

void PhotonDecorator::SetFocusState( bool bHasFocus )
{
	m_bHasFocus = bHasFocus;
	Render( m_cBounds );
}

void PhotonDecorator::SetWindowFlags( uint32 nFlags )
{
	m_nFlags = nFlags;
}

void PhotonDecorator::FrameSized( const Rect& cFrame )
{
	Layer* pcView = GetLayer();
	Point cDelta( cFrame.Width() - m_cBounds.Width(), cFrame.Height() - m_cBounds.Height() );
	m_cBounds = cFrame.Bounds();

	Layout();
	if ( cDelta.x != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.left = m_cZoomRect.left - fabs(cDelta.x)  - 6.0f;
		pcView->Invalidate( cDamage );
	}
	if ( cDelta.y != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.top = cDamage.bottom - std::max( m_vBottomBorder, m_vBottomBorder + cDelta.y ) - 1.0f;
		pcView->Invalidate( cDamage );
		// Make sure vertical resize handles are redrawn
		cDamage.left = cDamage.right - 2;
		cDamage.top = TITLE_HEIGHT;
		pcView->Invalidate( cDamage );
		cDamage.left = 0;
		cDamage.right = cDamage.left + 2;
		pcView->Invalidate( cDamage );
	}
}

void PhotonDecorator::Layout()
{
	float vRight = m_cBounds.right - 3;
	
	// Close button
	if ( m_nFlags & WND_NO_CLOSE_BUT )
	{
		m_cCloseRect.left  = m_cCloseRect.right = 0;
	}
	else
	{
		m_cCloseRect.right = vRight;
		m_cCloseRect.left  = m_cCloseRect.right - BUTTON_SIZE;
		vRight = m_cCloseRect.left - 3;
	}
	m_cCloseRect.top    = 5;
	m_cCloseRect.bottom = 16;
	// Minimize button
	if ( m_nFlags & WND_NO_DEPTH_BUT )
	{
		m_cMinimizeRect.left = m_cMinimizeRect.right = 0;
	}
	else
	{
		m_cMinimizeRect.right = vRight;
		m_cMinimizeRect.left   = m_cMinimizeRect.right - 12;
		vRight = m_cMinimizeRect.left - 3;
	}
	m_cMinimizeRect.top    = m_cCloseRect.top;
	m_cMinimizeRect.bottom = m_cCloseRect.bottom;
	// Zoom button
	if ( m_nFlags & WND_NO_ZOOM_BUT )
	{
		m_cZoomRect.left = m_cZoomRect.right = 0;
	}
	else
	{
		m_cZoomRect.right = vRight;
		m_cZoomRect.left  = m_cZoomRect.right - 12;
		vRight = m_cZoomRect.left - 3;
	}
	m_cZoomRect.top    = m_cCloseRect.top;
	m_cZoomRect.bottom = m_cCloseRect.bottom;
	// Title area (dragable)
	m_cDragRect.right  = vRight - 1;
	m_cDragRect.left   = m_cBounds.left;
	m_cDragRect.top    = 0;
	m_cDragRect.bottom = 20 - 1;
}

void PhotonDecorator::SetButtonState( uint32 nButton, bool bPushed )
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
			SetMinimizeButtonState( bPushed );
			break;
	}
}

void PhotonDecorator::SetCloseButtonState( bool bPushed )
{
	m_bCloseState = bPushed;
	if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 )
	{
		DrawClose( m_cCloseRect, m_bHasFocus, m_bCloseState == 1 );
	}
}

void PhotonDecorator::SetZoomButtonState( bool bPushed )
{
	m_bZoomState = bPushed;
	if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 )
	{
		DrawZoom( m_cZoomRect, m_bHasFocus, m_bZoomState == 1 );
	}
}

void PhotonDecorator::SetMinimizeButtonState( bool bPushed )
{
	m_bMinimizeState = bPushed;
	if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )
	{
		DrawMinimize( m_cMinimizeRect, m_bHasFocus, m_bMinimizeState == 1 );
	}
}

void PhotonDecorator::DrawPanel( const Rect& cRect, bool bActive, bool bRecessed )
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

void PhotonDecorator::DrawFrame( const Rect& cRect, bool bSolid, bool bRecessed )
{
	Layer* pcView = GetLayer();
	Color32_s	col_Normal	= get_default_color(COL_NORMAL);
	Color32_s	col_Dark	= BlendColours(get_default_color(COL_SHADOW), col_Normal, 0.8f);
	Color32_s	col_Shine;
	Color32_s	col_Shadow;
	// Pick colours based on state
	if (!bRecessed)
	{
		col_Shine	= BlendColours(get_default_color(COL_SHINE),  col_Normal, 0.5f);
		col_Shadow	= BlendColours(col_Dark, col_Normal, 0.5f);
	}
	else
	{
		col_Shine	= BlendColours(col_Dark, col_Normal, 0.5f);
		col_Shadow	= BlendColours(get_default_color(COL_SHINE),  col_Normal, 0.5f);
	}
	// Draw outer shadow
	pcView->SetFgColor( col_Shadow );
	pcView->DrawLine( Point( cRect.left-1, cRect.top-1 ), Point( cRect.right,  cRect.top-1  ) );
	pcView->DrawLine( Point( cRect.left-1, cRect.top-1 ), Point( cRect.left-1, cRect.bottom ) );
	// Draw outer shine
	pcView->SetFgColor( col_Shine );
	pcView->DrawLine( Point( cRect.left,    cRect.bottom+1), Point( cRect.right+1, cRect.bottom+1 ) );
	pcView->DrawLine( Point( cRect.right+1, cRect.top     ), Point( cRect.right+1, cRect.bottom+1 ) );
	// Draw main rectangle
	pcView->SetFgColor( col_Dark );
	pcView->DrawLine( Point( cRect.left,  cRect.top    ), Point( cRect.right, cRect.top    ) );
	pcView->DrawLine( Point( cRect.left,  cRect.top    ), Point( cRect.left,  cRect.bottom ) );
	pcView->DrawLine( Point( cRect.left,  cRect.bottom ), Point( cRect.right, cRect.bottom ) );
	pcView->DrawLine( Point( cRect.right, cRect.bottom ), Point( cRect.right, cRect.top    ) );
	// Draw cenral area (if required)
	if (bSolid)
	{
		pcView->FillRect( Rect( cRect.left+1, cRect.top+1, cRect.right-1, cRect.bottom-1 ), col_Normal );
	}
	// Draw inner shine
	pcView->SetFgColor( col_Shine );
	pcView->DrawLine( Point( cRect.left+1, cRect.top+1 ), Point( cRect.right-2, cRect.top+1    ) );
	pcView->DrawLine( Point( cRect.left+1, cRect.top+1 ), Point( cRect.left+1,  cRect.bottom-2 ) );
	// Draw inner shadow
	pcView->SetFgColor( col_Shadow );
	pcView->DrawLine( Point( cRect.left +2, cRect.bottom-1), Point( cRect.right-1, cRect.bottom-1 ) );
	pcView->DrawLine( Point( cRect.right-1, cRect.top+2   ), Point( cRect.right-1, cRect.bottom-1 ) );
}

void PhotonDecorator::DrawTitleBG( const Rect& cRect, bool bActive )
{
	Layer* pcView = GetLayer();
	Color32_s	col_TShine;
	Color32_s	col_TShadow;
	Color32_s	col_DTShadow;
	Color32_s	col_GradTop;
	Color32_s	col_GradBottom;
	// Pick colours based on state
	if (bActive)
	{
		col_GradTop		= BlendColours(get_default_color(COL_SHINE),  get_default_color(COL_SEL_WND_BORDER), 0.5f);
		col_GradBottom	= BlendColours(get_default_color(COL_SHADOW), get_default_color(COL_SEL_WND_BORDER), 0.5f);
		col_TShine		= BlendColours(col_GradBottom,                get_default_color(COL_SEL_WND_BORDER), 0.5f);
		col_TShadow		= BlendColours(get_default_color(COL_SHADOW), col_GradBottom, 0.5f);
		col_DTShadow	= BlendColours(get_default_color(COL_SHADOW), col_TShadow,    0.5f);
	}
	else
	{
		col_GradTop		= BlendColours(get_default_color(COL_SHINE),  get_default_color(COL_NORMAL_WND_BORDER), 0.5f);
		col_GradBottom	= BlendColours(get_default_color(COL_SHADOW), get_default_color(COL_NORMAL_WND_BORDER), 0.5f);
		col_TShine		= BlendColours(col_GradBottom,                get_default_color(COL_NORMAL_WND_BORDER), 0.5f);
		col_TShadow		= BlendColours(get_default_color(COL_SHADOW), col_GradBottom, 0.5f);
		col_DTShadow	= BlendColours(get_default_color(COL_SHADOW), col_TShadow,    0.5f);
	}
	// Draw outline
	pcView->SetFgColor( col_DTShadow );
	pcView->DrawLine( Point(cRect.left,  cRect.top   ), Point(cRect.right,  cRect.top   ) );
	pcView->DrawLine( Point(cRect.left,  cRect.top   ), Point(cRect.left,   cRect.bottom) );
	pcView->DrawLine( Point(cRect.left,  cRect.bottom), Point(cRect.right,  cRect.bottom) );
	// Draw highlight and shadow
	pcView->SetFgColor( col_TShine );
	pcView->DrawLine( Point(cRect.left+1, cRect.top+1  ), Point(cRect.right-1,  cRect.top   +1) );
	pcView->DrawLine( Point(cRect.left+1, cRect.top+1  ), Point(cRect.left +1,  cRect.bottom-1) );
	pcView->SetFgColor( col_TShadow );
	pcView->DrawLine( Point(cRect.left+2, cRect.bottom-1), Point(cRect.right, cRect.bottom-1) );
	pcView->DrawLine( Point(cRect.right,  cRect.top   +1), Point(cRect.right, cRect.bottom-1) );
	// Draw gradient
	Point Left  = Point(cRect.left+2,  cRect.top+2);
	Point Right = Point(cRect.right-1, cRect.top+2);
	Color32_s col_Grad = col_GradTop;
	Color32_s col_Step = Color32_s( (col_GradTop.red-col_GradBottom.red)/15, (col_GradTop.green-col_GradBottom.green)/15, (col_GradTop.blue-col_GradBottom.blue)/15, 0 );
	for (int i=0; i<16; i++)
	{
		pcView->SetFgColor( (i==1) ? col_TShine : col_Grad );
		pcView->DrawLine( Left, Right );
		Left.y  += 1;
		Right.y += 1;
		col_Grad.red   -= col_Step.red;
		col_Grad.green -= col_Step.green;
		col_Grad.blue  -= col_Step.blue;
	}
}

void PhotonDecorator::DrawMinimize( const Rect& cRect, bool bActive, bool bRecessed )
{
	Rect	R = cRect;
	
	DrawFrame( R, true, bRecessed);
	R.left+=8; R.top+=8;
	DrawFrame( R, true, bRecessed);
}

void PhotonDecorator::DrawZoom(  const Rect& cRect, bool bActive, bool bRecessed )
{
	Rect	R = cRect;
	DrawFrame( R, true, bRecessed);
	R.right-=5; R.bottom-=5;
	DrawFrame( R, true, bRecessed);
}

void PhotonDecorator::DrawClose(  const Rect& cRect, bool bActive, bool bRecessed  )
{
	Rect	R = cRect;
	DrawFrame( R, true, bRecessed);
	R.left+=4; R.top+=4; R.right-=4; R.bottom-=4;
	DrawFrame( R, false, bRecessed);
}

void PhotonDecorator::Render( const Rect& cUpdateRect )
{
	if ( m_nFlags & WND_NO_BORDER )
	{
		return;
	}
  
	// Get title-bar fill-colour  
	Color32_s sFillColor = m_bHasFocus ? get_default_color(COL_SEL_WND_BORDER) : get_default_color(COL_NORMAL_WND_BORDER);
	sFillColor = BlendColours(sFillColor, get_default_color(COL_SHADOW), 0.82f);
	// Get the layer object that we're drawing into
	Layer* pcView = GetLayer();
	// Calculate the bounds for the inner and outer rectangles
	Rect  cOBounds = pcView->GetBounds();
	Rect  cIBounds = cOBounds;
	cIBounds.left += m_vLeftBorder - 1;
	cIBounds.right -= m_vRightBorder - 1;
	cIBounds.top += m_vTopBorder - 1;
	cIBounds.bottom -= m_vBottomBorder - 1;
	// Draw inner and outer frames
	pcView->DrawFrame( cOBounds, FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
	pcView->DrawFrame( cIBounds, FRAME_RECESSED | FRAME_THIN | FRAME_TRANSPARENT );
	// Bottom
	pcView->FillRect( Rect( cOBounds.left + 1, cIBounds.bottom + 1, cOBounds.right - 1, cOBounds.bottom - 1 ), sFillColor );
	// Left
	pcView->FillRect( Rect( cOBounds.left + 1, cOBounds.top + m_vTopBorder, cIBounds.left - 1, cIBounds.bottom ), sFillColor );
	// Right
	pcView->FillRect( Rect( cIBounds.right + 1, cOBounds.top + m_vTopBorder, cOBounds.right - 1, cIBounds.bottom ), sFillColor );


	// Draw corner markers on frames
	if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE )
	{
		pcView->SetFgColor( 0, 0, 0, 255 );
		pcView->DrawLine( Point( 15, cIBounds.bottom + 1 ), Point( 15, cOBounds.bottom - 1 ) );
		pcView->DrawLine( Point( cOBounds.right - 16, cIBounds.bottom + 1 ), Point( cOBounds.right - 16, cOBounds.bottom - 1 ) );
		pcView->DrawLine( Point( cIBounds.right + 1, cOBounds.bottom - 16 ), Point( cOBounds.right - 1, cOBounds.bottom - 16 ) );
		pcView->DrawLine( Point( cIBounds.left - 1, cOBounds.bottom - 16 ), Point( cOBounds.left + 1, cOBounds.bottom - 16 ) );
		pcView->SetFgColor( 255, 255, 255, 255 );
		pcView->DrawLine( Point( 16, cIBounds.bottom + 1 ), Point( 16, cOBounds.bottom - 1 ) );
		pcView->DrawLine( Point( cOBounds.right - 15, cIBounds.bottom + 1 ), Point( cOBounds.right - 15, cOBounds.bottom - 1 ) );
		pcView->DrawLine( Point( cIBounds.right + 1, cOBounds.bottom - 15 ), Point( cOBounds.right - 1, cOBounds.bottom - 15 ) );
		pcView->DrawLine( Point( cIBounds.left - 1, cOBounds.bottom - 15 ), Point( cOBounds.left + 1, cOBounds.bottom - 15 ) );
	}

	if ( (m_nFlags & WND_NO_TITLE) == 0 )
	{
		// Draw title-bar and text
		DrawTitleBG(m_cDragRect, m_bHasFocus);
		pcView->SetFgColor( 0,0,0,0 );
		pcView->SetBgColor( sFillColor );
		pcView->MovePenTo( m_cDragRect.left + 5,
			1 + m_cDragRect.Height() / 2 -
			(m_sFontHeight.ascender + m_sFontHeight.descender) / 2 + m_sFontHeight.ascender );
		pcView->DrawString( m_cTitle.c_str(), -1 );
	}
	else
	{
		// Fill in top border
		DrawTitleBG(m_cDragRect, m_bHasFocus);
	}

	// Draw button area background
	if ( (m_nFlags & (WND_NO_ZOOM_BUT|WND_NO_CLOSE_BUT)) != (WND_NO_ZOOM_BUT|WND_NO_CLOSE_BUT) )
	{
		Color32_s	col_DShadow = BlendColours( get_default_color(COL_SHADOW), get_default_color(COL_NORMAL), 0.8f);
		Rect		buttons;
		buttons.left   = m_cDragRect.right;
		buttons.top    = cOBounds.top + 1;
		buttons.right  = cOBounds.right + 1;
		buttons.bottom = m_cDragRect.bottom;

		pcView->FillRect( buttons, col_DShadow );

		buttons.top+=1; buttons.left+=1; buttons.right-=1;
		pcView->DrawFrame( buttons, FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
		buttons.top+=1; buttons.left+=1; buttons.right-=1; buttons.bottom-=1;
		pcView->FillRect( buttons, get_default_color(COL_NORMAL) );
		buttons.top-=3; buttons.left-=2; buttons.right +=2; buttons.bottom = buttons.top;
		
		pcView->SetFgColor( BlendColours(get_default_color(COL_SHADOW), 
						BlendColours(get_default_color(COL_SHADOW), 
					BlendColours(get_default_color(COL_SHADOW), get_default_color(COL_SEL_WND_BORDER), 0.5f),    0.5f), 0.5f ) );
		
		
		pcView->DrawLine( Point(buttons.left, buttons.top ), Point(buttons.right, buttons.bottom ) );

	}
	// Draw ZOOM button
	if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 )
	{
		DrawZoom( m_cZoomRect, m_bHasFocus, m_bZoomState == 1 );
	}
	// Draw MINIMIZE button
	if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 )
	{
		DrawMinimize( m_cMinimizeRect, m_bHasFocus, m_bMinimizeState == 1 );
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
    return( new PhotonDecorator( pcLayer, nFlags ) );
}
