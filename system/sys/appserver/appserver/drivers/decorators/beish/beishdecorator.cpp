 /*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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
 * 
 *  Modification of NextDecorator by Edwin de Jonge
 *  Updated July 2004 by Mike Saunders
 *  BeIsh 0.2
 */

#include "beishdecorator.h"
#include "gradient.h"

#include <layer.h>
#include "sfont.h"
#include <math.h>
#include <gui/window.h>

using namespace os;

BeIshDecorator::BeIshDecorator( Layer* pcLayer, uint32 nWndFlags ) :
    WindowDecorator( pcLayer, nWndFlags )
{
    m_nFlags = nWndFlags;

    m_bHasFocus   = false;
    m_bCloseState = false;
    m_bZoomState  = false;
    m_bDepthState = false;

    m_sFontHeight = pcLayer->GetFontHeight();
  
    CalculateBorderSizes();
}

BeIshDecorator::~BeIshDecorator()
{
}

void BeIshDecorator::CalculateBorderSizes()
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
	    m_vTopBorder = 5;
	}
        else
	{
	   //m_vTopBorder = float(m_sFontHeight.ascender + m_sFontHeight.descender + 6);
//	   m_vTopBorder = 23.0f;
	   m_vTopBorder = 24.0f;
	}
	m_vLeftBorder   = 5;
	m_vRightBorder  = 5;
	if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE ) 
	{
	    m_vBottomBorder = 5;
	}
        else
	{
	    m_vBottomBorder = 5;
	}
    }
    
}

void BeIshDecorator::FontChanged()
{
    Layer* pcView = GetLayer();
    m_sFontHeight = pcView->GetFontHeight();
    CalculateBorderSizes();
    pcView->Invalidate();
    Layout();
}

void BeIshDecorator::SetFlags( uint32 nFlags )
{
    Layer* pcView = GetLayer();
    m_nFlags = nFlags;
    CalculateBorderSizes();
    pcView->Invalidate();
    Layout();
}

Point BeIshDecorator::GetMinimumSize()
{
    Point cMinSize( 0.0f, m_vTopBorder + m_vBottomBorder );

    if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 ) {
	cMinSize.x += m_cCloseRect.Width();
    }
    if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 ) {
	cMinSize.x += m_cZoomRect.Width();
    }
    if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 ) {
	cMinSize.x += m_cToggleRect.Width();
    }
    if ( cMinSize.x < m_vLeftBorder + m_vRightBorder ) {
	cMinSize.x = m_vLeftBorder + m_vRightBorder;
    }
    return( cMinSize );
}

WindowDecorator::hit_item BeIshDecorator::HitTest( const Point& cPosition )
{
    if ( (m_nFlags & WND_NOT_RESIZABLE) != WND_NOT_RESIZABLE ) 
     {
  
	if ( cPosition.y < 4 ) 
	{
	    if ( cPosition.x < 4 ) 
	    {
		return( HIT_SIZE_LT );
	    } 
	    else if ( cPosition.x > m_cBounds.right - 4 ) 
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

void BeIshDecorator::DrawGradientRect(const Rect& r,const Color32_s& sHighlight,const Color32_s &sNormal,const Color32_s &sShadow)
{
   Layer *pcView = GetLayer();
   Rect rGradient(r.left + 2.0,r.top + 2.0,r.right - 2.0,r.bottom - 2.0);
   int height = (int)floor(rGradient.bottom - rGradient.top);
   Gradient gradientHN(sHighlight,sNormal,height);
   Gradient gradientSN(sShadow,sNormal,height);
   for (int i = 0; i < height; i++)
   {
      Point left(rGradient.left,rGradient.top + float(i));
      Point top(rGradient.left + float(i),rGradient.top);
      pcView->SetFgColor(gradientHN.color[i]);
      pcView->DrawLine(left,top);
      Point right(rGradient.right,rGradient.bottom - float(i));
      Point bottom(rGradient.right - float(i), rGradient.bottom);
      if (bottom == top)
	continue;
      pcView->SetFgColor(gradientSN.color[i]);
      pcView->DrawLine(right,bottom);
   }
   pcView->SetFgColor(sHighlight);
   pcView->DrawLine(Point(r.left + 1,r.top + 1),Point(r.left + 1, r.bottom));
   pcView->DrawLine(Point(r.left + 2,r.top + 1),Point(r.right, r.top + 1));
   pcView->DrawLine(Point(r.right,r.top + 1),Point(r.right,r.bottom));
   pcView->DrawLine(Point(r.left + 1,r.bottom),Point(r.right,r.bottom));

   pcView->SetFgColor(sShadow);
   pcView->DrawLine(Point(r.left,r.top),Point(r.left,r.bottom));
   pcView->DrawLine(Point(r.left,r.top),Point(r.right,r.top));
   pcView->DrawLine(Point(r.right - 1,r.top + 2),Point(r.right - 1,r.bottom - 1));
   pcView->DrawLine(Point(r.left + 2,r.bottom - 1),Point(r.right - 1,r.bottom - 1));
   
}

Rect BeIshDecorator::GetBorderSize()
{
    return( Rect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void BeIshDecorator::SetTitle( const char* pzTitle )
{
    m_cTitle = pzTitle;
    Render( m_cBounds );
}

void BeIshDecorator::SetFocusState( bool bHasFocus )
{
    m_bHasFocus = bHasFocus;
    Render( m_cBounds );
}

void BeIshDecorator::SetWindowFlags( uint32 nFlags )
{
    m_nFlags = nFlags;
}

void BeIshDecorator::FrameSized( const Rect& cFrame )
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
    }
}

void BeIshDecorator::Layout()
{
//   dbprintf("hello"); 
   //if ( m_nFlags & WND_NO_CLOSE_BUT ) 
    {
	m_cCloseRect.left   = 0;
	m_cCloseRect.right  = 0;
	m_cCloseRect.top    = 0;
	m_cCloseRect.bottom = 0;
    } 
    //else 
    {
	m_cCloseRect.left = 4;
//	m_cCloseRect.right = m_vTopBorder - 3;
	m_cCloseRect.right = 14 + 2;
	m_cCloseRect.top = 4;
//	m_cCloseRect.bottom = m_vTopBorder - 3;
	m_cCloseRect.bottom = 14 + 2;
//        dbprintf("hello");
    }

    m_cToggleRect.right = m_cBounds.right - 4;
    if ( m_nFlags & WND_NO_DEPTH_BUT ) 
    {
	m_cToggleRect.left = m_cToggleRect.right;
    }
   else 
    {
	m_cToggleRect.left = m_cToggleRect.right - 14.0f;
    }
    m_cToggleRect.top = 4;
//    m_cToggleRect.bottom = m_vTopBorder - 3;
    m_cToggleRect.bottom = m_cToggleRect.top + 12;
    

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
	    m_cZoomRect.right  = m_cToggleRect.left - 4.0f;
	}
	m_cZoomRect.left   = m_cZoomRect.right - 16.0f;
    }
    m_cZoomRect.top  = 4;
    m_cZoomRect.bottom  = m_cZoomRect.top + 12;
//    m_cZoomRect.bottom  = m_vTopBorder - 3;

    if ( m_nFlags & WND_NO_CLOSE_BUT ) 
    {
	m_cDragRect.left  = 0.0f;
    }
    else
    {
	m_cDragRect.left  = m_cCloseRect.right + 1.0f;
//	m_cDragRect.left  = 0.0f;
    }
    if ( m_nFlags & WND_NO_ZOOM_BUT ) 
    {
	if ( m_nFlags & WND_NO_DEPTH_BUT ) 
	{
	    m_cDragRect.right = m_cBounds.right;
	}
        else
	{
	    m_cDragRect.right = m_cToggleRect.left - 1.0f;
	}
    }
    else
    {
	m_cDragRect.right  = m_cZoomRect.left - 1.0f;
    }
    m_cDragRect.top  = 1;
//    m_cDragRect.bottom  = m_vTopBorder - 2;
    m_cDragRect.bottom  = m_cDragRect.top + 18.0;
}

void BeIshDecorator::SetButtonState( uint32 nButton, bool bPushed )
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
	}
}

void BeIshDecorator::SetCloseButtonState( bool bPushed )
{
    m_bCloseState = bPushed;
    if ( (m_nFlags & WND_NO_CLOSE_BUT) == 0 ) 
    {
	Color32_s sFillColor =  m_bHasFocus ? get_default_color( COL_SEL_WND_BORDER ) : get_default_color( COL_NORMAL_WND_BORDER );
	DrawClose( m_cCloseRect, sFillColor, m_bCloseState == 1 );
    }
}

void BeIshDecorator::SetZoomButtonState( bool bPushed )
{
    m_bZoomState = bPushed;
    if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 ) 
    {
        Color32_s sFillColor =  m_bHasFocus ? get_default_color( COL_SEL_WND_BORDER ) : get_default_color( COL_NORMAL_WND_BORDER );
	DrawZoom( m_cZoomRect, sFillColor, m_bZoomState == 1 );
    }
}

void BeIshDecorator::SetDepthButtonState( bool bPushed )
{
    m_bDepthState = bPushed;
    if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 ) 
    {
	Color32_s sFillColor =  m_bHasFocus ? get_default_color( COL_SEL_WND_BORDER ) : get_default_color( COL_NORMAL_WND_BORDER );
        DrawDepth( m_cToggleRect, sFillColor, m_bDepthState == 1 );
    }
}


void BeIshDecorator::DrawDepth( const Rect& cRect, const Color32_s& sFillColor, bool bRecessed )
{
    Layer* pcView = GetLayer();
    pcView->SetBgColor(sFillColor);
    Rect rBar;
    rBar.left = cRect.left - 3;
    rBar.top = cRect.top - 3;
    rBar.right = cRect.right + 2;
    rBar.bottom = cRect.bottom + 3;
    pcView->FillRect(rBar,sFillColor);
    
    Color32_s sHighlight = get_default_color( COL_SHINE );
    sHighlight.red /= 2;
    sHighlight.green /= 2;
    sHighlight.blue /= 2;
    sHighlight.red += sFillColor.red / 2;
    sHighlight.green += sFillColor.green / 2;
    sHighlight.blue += sFillColor.blue / 2;

    Color32_s sShadow = get_default_color(COL_SHADOW);
    sShadow.red /= 2;
    sShadow.green /= 2;
    sShadow.blue /= 2;
    sShadow.red += sFillColor.red / 2;
    sShadow.green += sFillColor.green / 2;
    sShadow.blue += sFillColor.blue / 2;
    
    pcView->SetFgColor(sHighlight);
    pcView->DrawLine(Point(rBar.left,rBar.top),Point(rBar.right,rBar.top));
    pcView->SetFgColor(sShadow);
    pcView->DrawLine(Point(rBar.right,rBar.bottom),Point(rBar.right,rBar.top));

    Rect	L,R;

    L.left = cRect.left;
    L.top = cRect.top;

    L.right = L.left + 9.0f;
    L.bottom = L.top + 9.0f;

    R.left = L.left + 3.0f;
    R.top = L.top + 3.0f;

    R.right = R.left + 9.0f;
    R.bottom = R.top + 9.0f;

  
    if ( bRecessed )
    {
        DrawGradientRect(L,sShadow,sFillColor,sHighlight);
        pcView->FillRect(R,sFillColor);
        DrawGradientRect(R,sShadow,sFillColor,sHighlight);
    }
    else
    {
        DrawGradientRect(L,sHighlight,sFillColor,sShadow);
        pcView->FillRect(R,sFillColor);
        DrawGradientRect(R,sHighlight,sFillColor,sShadow);
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BeIshDecorator::DrawZoom(  const Rect& cRect, const Color32_s& sFillColor, bool bRecessed )
{

    Layer* pcView = GetLayer();
    Rect rBar;
    rBar.left = cRect.left - 3;
    rBar.top = cRect.top - 3;
    rBar.right = cRect.right + 3;
    rBar.bottom = cRect.bottom + 3;
    pcView->FillRect(rBar,sFillColor);
    
    Color32_s sHighlight = get_default_color(COL_SHINE);
    sHighlight.red /= 2;
    sHighlight.green /= 2;
    sHighlight.blue /= 2;
    sHighlight.red += sFillColor.red / 2;
    sHighlight.green += sFillColor.green / 2;
    sHighlight.blue += sFillColor.blue / 2;
   
    pcView->SetFgColor(sHighlight);
    //pcView->DrawLine(Point(rBar.left,rBar.bottom),Point(rBar.left,rBar.top));
    pcView->DrawLine(Point(rBar.left,rBar.top),Point(rBar.right,rBar.top));

    Color32_s sShadow = get_default_color(COL_SHADOW);
    sShadow.red /= 2;
    sShadow.green /= 2;
    sShadow.blue /= 2;
    sShadow.red += sFillColor.red / 2;
    sShadow.green += sFillColor.green / 2;
    sShadow.blue += sFillColor.blue / 2;
    
    Rect	L,R;

    L.left = cRect.left;
    L.top = cRect.top;

    L.right = L.left + 7.0f;
    L.bottom = L.top + 7.0f;

    R.left = L.left + 3.0f;
    R.top = L.top + 3.0f;

    R.right = R.left + 9.0f;
    R.bottom = R.top + 9.0f;

  
    if ( bRecessed )
    {
        DrawGradientRect(R,sShadow,sFillColor,sHighlight);
        pcView->FillRect(L,sFillColor);
        DrawGradientRect(L,sShadow,sFillColor,sHighlight);
    }
    else
    {
        DrawGradientRect(R,sHighlight,sFillColor,sShadow);
        pcView->FillRect(L,sFillColor);
        DrawGradientRect(L,sHighlight,sFillColor,sShadow);
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BeIshDecorator::DrawClose(  const Rect& cRect, const Color32_s& sFillColor, bool bRecessed  )
{
//    Rect L;

//    L.left = cRect.left + ((cRect.Width()+1.0f) / 3);
//    L.top = cRect.top + ((cRect.Height()+1.0f) / 3);

//   L.right = cRect.right - ((cRect.Width()+1.0f) / 3);
//    L.bottom = cRect.bottom - ((cRect.Height()+1.0f) / 3);

    Layer* pcView = GetLayer();
    Rect rBar;
    rBar.left = cRect.left - 3;
    rBar.top = cRect.top - 3;
    rBar.right = cRect.right + 3;
    rBar.bottom = cRect.bottom + 3;
    pcView->FillRect(rBar,sFillColor);
   
    Color32_s sHighlight = get_default_color(COL_SHINE);
    sHighlight.red /= 2;
    sHighlight.green /= 2;
    sHighlight.blue /= 2;
    sHighlight.red += sFillColor.red / 2;
    sHighlight.green += sFillColor.green / 2;
    sHighlight.blue += sFillColor.blue / 2;

    pcView->SetFgColor(sHighlight);
    pcView->MovePenTo(Point(rBar.left,rBar.bottom));
//    pcView->DrawLine(Point(rBar.left,rBar.bottom),Point(rBar.left,rBar.top));
//    pcView->DrawLine(Point(rBar.left,rBar.top),Point(rBar.right,rBar.top));
    pcView->DrawLine(Point(rBar.left,rBar.top));
    pcView->DrawLine(Point(rBar.right,rBar.top));

   Color32_s sShadow = get_default_color(COL_SHADOW);
    sShadow.red /= 2;
    sShadow.green /= 2;
    sShadow.blue /= 2;
    sShadow.red += sFillColor.red / 2;
    sShadow.green += sFillColor.green / 2;
    sShadow.blue += sFillColor.blue / 2;
  
    if ( bRecessed ) 
    {
        DrawGradientRect(cRect,sShadow,sFillColor,sHighlight);
//	pcView->FillRect( cRect, sFillColor );
//	pcView->DrawFrame( cRect, FRAME_RECESSED | FRAME_TRANSPARENT );
//	pcView->DrawFrame( L, FRAME_RAISED | FRAME_TRANSPARENT );
    }
    else 
    {
        DrawGradientRect(cRect,sHighlight,sFillColor,sShadow);
//	pcView->FillRect( cRect, sFillColor );
//	pcView->DrawFrame( cRect, FRAME_RAISED | FRAME_TRANSPARENT );
//	pcView->DrawFrame( L, FRAME_RECESSED | FRAME_TRANSPARENT );
    }
}

void BeIshDecorator::Render( const Rect& cUpdateRect )
{
    float font_y = 0;

    if ( m_nFlags & WND_NO_BORDER ) 
    {
	return;
    }
  
    Color32_s sFillColor =  m_bHasFocus ? get_default_color( COL_SEL_WND_BORDER ) : get_default_color( COL_NORMAL_WND_BORDER );

    Layer* pcView = GetLayer();
 
    Rect  cOBounds = pcView->GetBounds();
    
    pcView->SetFgColor(get_default_color(COL_SHADOW));
    pcView->DrawLine(Point(cOBounds.left,cOBounds.bottom),Point(cOBounds.right,cOBounds.bottom));
    pcView->DrawLine(Point(cOBounds.right,cOBounds.top));
    //remove shadow
    cOBounds.right -= 1;
    cOBounds.bottom -= 1;
   
    //pcView->DrawRect(cOBounds,get_default_color(COL_NORMAL_WND_BORDER));
    pcView->SetFgColor(get_default_color(COL_NORMAL_WND_BORDER));
    pcView->DrawLine(Point(cOBounds.left,cOBounds.bottom),Point(cOBounds.left,cOBounds.top));
    pcView->DrawLine(Point(cOBounds.left,cOBounds.top),Point(cOBounds.right,cOBounds.top));
    pcView->DrawLine(Point(cOBounds.right,cOBounds.top),Point(cOBounds.right,cOBounds.bottom));
    pcView->DrawLine(Point(cOBounds.right,cOBounds.bottom),Point(cOBounds.left,cOBounds.bottom));

    Rect  cIBounds = cOBounds;    
    cIBounds.left += m_vLeftBorder - 3;
    cIBounds.right -= m_vRightBorder - 4;
    cIBounds.top += m_vTopBorder - 3;
    cIBounds.bottom -= m_vBottomBorder - 4;
    
    Color32_s inbetween = get_default_color(COL_NORMAL);
    inbetween.red /= 2;
    inbetween.green /= 2;
    inbetween.blue /= 2;

    Color32_s darker = get_default_color(COL_NORMAL_WND_BORDER);
    darker.red /= 2;
    darker.green /= 2;
    darker.blue /= 2;
    
    inbetween.red += darker.red;
    inbetween.blue += darker.blue;
    inbetween.green += darker.green;
   
    pcView->SetFgColor(inbetween);
    pcView->DrawLine(Point(cIBounds.left,cIBounds.bottom),Point(cIBounds.right,cIBounds.bottom));
    pcView->DrawLine(Point(cIBounds.right,cIBounds.top));
    pcView->DrawLine(Point(cIBounds.left,cIBounds.top));
    pcView->DrawLine(Point(cIBounds.left,cIBounds.bottom));

    pcView->SetFgColor(get_default_color(COL_NORMAL_WND_BORDER));
    pcView->DrawLine(Point(cIBounds.left,cIBounds.bottom + 1),Point(cIBounds.right+1,cIBounds.bottom + 1));
    pcView->DrawLine(Point(cIBounds.right + 1, cIBounds.top - 1));
    pcView->SetFgColor(get_default_color(COL_SHINE));
    pcView->DrawLine(Point(cIBounds.left - 1, cIBounds.top - 1), Point(cIBounds.left - 1, cIBounds.bottom));

    //extra line 
    pcView->SetFgColor(get_default_color(COL_NORMAL));
    pcView->DrawLine(Point(cIBounds.left,cIBounds.top - 1),Point(cIBounds.right,cIBounds.top - 1));
    pcView->DrawLine(Point(cIBounds.left,cIBounds.top),Point(cIBounds.right,cIBounds.top));
   
    Rect  cIIBounds = cOBounds;
    cIIBounds.left += m_vLeftBorder - 1;
    cIIBounds.right -= m_vRightBorder - 2;
    cIIBounds.top += m_vTopBorder - 1;
    cIIBounds.bottom -= m_vBottomBorder - 2;

    pcView->SetFgColor(get_default_color(COL_NORMAL_WND_BORDER));
    pcView->DrawLine(Point(cIIBounds.left,cIIBounds.bottom),Point(cIIBounds.right,cIIBounds.bottom));
    pcView->DrawLine(Point(cIIBounds.right,cIIBounds.top));
    pcView->DrawLine(Point(cIIBounds.left,cIIBounds.top));
    pcView->DrawLine(Point(cIIBounds.left,cIIBounds.bottom));

    pcView->SetFgColor(get_default_color(COL_SHINE));
    pcView->DrawLine(Point(cIIBounds.left,cIIBounds.bottom + 1),Point(cIIBounds.right+1,cIIBounds.bottom + 1));
    pcView->DrawLine(Point(cIIBounds.right + 1, cIIBounds.top - 1));
    pcView->SetFgColor(get_default_color(COL_NORMAL_WND_BORDER));
    pcView->DrawLine(Point(cIIBounds.left - 1, cIIBounds.top - 1));
    pcView->DrawLine(Point(cIIBounds.left - 1, cIIBounds.bottom + 1));
   
    pcView->SetFgColor(get_default_color(COL_SHADOW));
    pcView->DrawLine(Point(cIIBounds.left, cIIBounds.top), Point(cIIBounds.right, cIIBounds.top));
    pcView->DrawLine(Point(cIIBounds.left, cIIBounds.top), Point(cIIBounds.left, cIIBounds.bottom));

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
	DrawClose( m_cCloseRect, sFillColor, m_bCloseState == 1 );
    }

    if ( (m_nFlags & WND_NO_TITLE) == 0 ) 
    {
	pcView->FillRect( m_cDragRect, sFillColor );
        pcView->SetFgColor( 0, 0, 0, 0);
	//pcView->SetFgColor( 255, 255, 255, 0 );
	pcView->SetBgColor( sFillColor );

	font_y = m_cDragRect.Height() / 2 - (m_sFontHeight.ascender + m_sFontHeight.descender) / 2 + m_sFontHeight.ascender;

	// Move font down just a tiny bit
	font_y += 2;

	pcView->MovePenTo( m_cDragRect.left + 17, font_y);

/*	pcView->MovePenTo( m_cDragRect.left + 17,
			   (m_cDragRect.Height()+1.0f) / 2 -
			   (m_sFontHeight.ascender + m_sFontHeight.descender) / 2 + m_sFontHeight.ascender +
			   m_sFontHeight.line_gap * 0.5f + 1);*/
    
       const char *title = m_cTitle.c_str();
       int titlewidth = pcView->m_pcFont->GetInstance()->GetStringWidth(title,strlen(title));
        
//        printf("titlewidth = %d",titlewidth);
        pcView->DrawString( m_cTitle.c_str(), -1 );
        Color32_s sHighlight = get_default_color(COL_SHINE);
        sHighlight.red /= 2;
        sHighlight.green /= 2;
        sHighlight.blue /= 2;
        sHighlight.red += sFillColor.red / 2;
        sHighlight.green += sFillColor.green / 2;
        sHighlight.blue += sFillColor.blue / 2;
       
        pcView->SetFgColor(sHighlight);
        pcView->DrawLine(Point(m_cDragRect.left,m_cDragRect.top),Point(m_cDragRect.right,m_cDragRect.top));
        int rightside = (int)(m_cDragRect.left + 36 + titlewidth);
        if (rightside > m_cDragRect.right)
	 rightside = (int)m_cDragRect.right;
        //pcView->DrawLine(Point(rightside,m_cDragRect.top),Point(rightside,m_cDragRect.bottom));
	//pcView->DrawFrame( m_cDragRect, FRAME_RAISED | FRAME_TRANSPARENT );
    } else 
    {
	pcView->FillRect( Rect( cOBounds.left + 1, cOBounds.top - 1, cOBounds.right - 1, cIBounds.top + 1 ), sFillColor );
    }

    if ( (m_nFlags & WND_NO_ZOOM_BUT) == 0 ) 
    {
	DrawZoom( m_cZoomRect, sFillColor, m_bZoomState == 1 );
    }
    if ( (m_nFlags & WND_NO_DEPTH_BUT) == 0 ) {
	DrawDepth( m_cToggleRect, sFillColor, m_bDepthState == 1 );
    }
}

extern "C" int get_api_version()
{
    return( WNDDECORATOR_APIVERSION );
}

extern "C" WindowDecorator* create_decorator( Layer* pcLayer, uint32 nFlags )
{
    return( new BeIshDecorator( pcLayer, nFlags ) );
}
