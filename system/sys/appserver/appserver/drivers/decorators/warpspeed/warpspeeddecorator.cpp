/*
 *  "WarpSpeed" Window-Decorator
 *  Copyright (C) 2004 Terry Glass
 *
 *  Based on "Amiga" Window-Decorator
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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
#include "warpspeeddecorator.h"
#include <layer.h>
#include <gui/window.h>

WarpSpeedDecorator::WarpSpeedDecorator (Layer * pcLayer, uint32 nWndFlags):WindowDecorator (pcLayer,
		 nWndFlags)
{
  m_nFlags = nWndFlags;
  m_bHasFocus = false;
  m_bCloseState = false;
  m_bZoomState = false;
  m_bMinimizeState = false;
  m_sFontHeight = pcLayer->GetFontHeight ();
  CalculateBorderSizes ();
}



WarpSpeedDecorator::~WarpSpeedDecorator ()
{
}



WindowDecorator::hit_item
  WarpSpeedDecorator::HitTest (const Point & cPosition)
{
  if (cPosition.y < 4)
    {
      if (cPosition.x < 4)
	{
	  return HIT_SIZE_LT;
	}
      else if (cPosition.x > m_cBounds.right - 4)
	{
	  return HIT_SIZE_RT;;
	}
      else
	return HIT_SIZE_TOP;
    }
  else if (cPosition.y > m_cBounds.bottom - 4)
    {
      if (cPosition.x < 16)
	{
	  return (HIT_SIZE_LB);
	}
      else if (cPosition.x > m_cBounds.right - 16)
	{
	  return (HIT_SIZE_RB);
	}
      else
	{
	  return (HIT_SIZE_BOTTOM);
	}
    }
  else if (cPosition.x < 4)
    {
      return (HIT_SIZE_LEFT);
    }
  else if (cPosition.x > m_cBounds.right - 4)
    {
      return (HIT_SIZE_RIGHT);
    }

  if (m_cZoomRect.DoIntersect (cPosition))
    {
      return HIT_ZOOM;
    }
  else if (m_cMinimizeRect.DoIntersect (cPosition))
    {
      return HIT_MINIMIZE;
    }
  else if (m_cCloseRect.DoIntersect (cPosition))
    {
      return HIT_CLOSE;
    }
  else if (m_cTitleRect.DoIntersect (cPosition))
    {
      return HIT_DRAG;
    }
  else
    {
      return HIT_NONE;
    }

}



void
WarpSpeedDecorator::FrameSized (const Rect & cNewFrame)
{
	/*
	m_cBounds = cNewFrame;
	Layout();
	GetLayer()->Invalidate(m_cBounds);
	*/
	Layer* pcView = GetLayer();
	Point cDelta( cNewFrame.Width() - m_cBounds.Width(), cNewFrame.Height() - m_cBounds.Height() );
	m_cBounds = cNewFrame.Bounds();

	Layout();
	if ( cDelta.x != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.left = m_cCloseRect.left - fabs(cDelta.x)  - 6.0f;
		pcView->Invalidate( cDamage );
	}
	if ( cDelta.y != 0.0f )
	{
		Rect cDamage = m_cBounds;
		cDamage.top = cDamage.bottom - std::max( m_vBottomBorder, m_vBottomBorder + cDelta.y ) - 1.0f;
		pcView->Invalidate( cDamage );
	}

}


void
WarpSpeedDecorator::CalculateBorderSizes ()
{
  if (m_nFlags & WND_NO_BORDER)
    {
      m_vLeftBorder = 0;
      m_vRightBorder = 0;
      m_vTopBorder = 0;
      m_vBottomBorder = 0;
    }
  else
    {
      if ((m_nFlags &
	   (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT |
	    WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE)) ==
	  (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT |
	   WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE))
	{
	  m_vTopBorder = 4;
	}
      else
	{
	  m_vTopBorder =
	    float (m_sFontHeight.ascender + m_sFontHeight.descender + 10);
	  m_vLeftBorder = 4;
	  m_vRightBorder = 4;
	  m_vBottomBorder = 4;
	}
    }
}

Rect
WarpSpeedDecorator::GetBorderSize ()
{
  return Rect (m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder);
}



Point
WarpSpeedDecorator::GetMinimumSize ()
{
  return Point (0, 0);
}



void
WarpSpeedDecorator::SetTitle (const char *pzTitle)
{
  m_cTitle = pzTitle;
  Render (m_cBounds);
}



void
WarpSpeedDecorator::SetFlags (uint32 nFlags)
{
  Layer *pcView = GetLayer ();
  m_nFlags = nFlags;
  CalculateBorderSizes ();
  pcView->Invalidate ();
  Layout ();
}



void
WarpSpeedDecorator::FontChanged ()
{
  Layer *pcView = GetLayer ();
  m_sFontHeight = pcView->GetFontHeight ();
  CalculateBorderSizes ();
  pcView->Invalidate ();
  Layout ();
}



void
WarpSpeedDecorator::SetWindowFlags (uint32 nFlags)
{
  m_nFlags = nFlags;
}



void
WarpSpeedDecorator::SetFocusState (bool bHasFocus)
{
  m_bHasFocus = bHasFocus;
  Render (m_cBounds);
}



void
WarpSpeedDecorator::SetButtonState (uint32 nButton, bool bPushed)
{
  switch (nButton)
    {
    case HIT_ZOOM:
      m_bZoomState = bPushed;
      if ((m_nFlags & WND_NO_ZOOM_BUT) == 0)
	{
	  DrawZoomButton ();
	}
      break;
    case HIT_CLOSE:
      m_bCloseState = bPushed;
      if ((m_nFlags & WND_NO_CLOSE_BUT) == 0)
	{
	  DrawCloseButton ();
	}
      break;
    case HIT_MINIMIZE:
      m_bMinimizeState = bPushed;
      if ((m_nFlags & WND_NO_DEPTH_BUT) == 0)
	{
	  DrawMinimizeButton ();
	}
    }
}




void
WarpSpeedDecorator::Render (const Rect & cUpdateRect)
{
  Layer *pcView = GetLayer ();
  Rect cBounds = pcView->GetBounds ();
  Color32_s sFillColor = get_default_color (COL_NORMAL);
  pcView->DrawFrame (cBounds, FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT);



  //Top 
  pcView->
    FillRect (Rect
	      (cBounds.left + 1, cBounds.top + 1, cBounds.right - 1,
	       cBounds.top + m_vTopBorder - 1), sFillColor);

  DrawTitle ();

  // Bottom
  pcView->
    FillRect (Rect
	      (cBounds.left + 1, cBounds.bottom - m_vBottomBorder,
	       cBounds.right - 1, cBounds.bottom - 1), sFillColor);
  // Left
  pcView->FillRect (Rect (cBounds.left + 1, cBounds.top + 1,
			  cBounds.left + m_vLeftBorder,
			  cBounds.bottom - m_vBottomBorder), sFillColor);
  // Right
  pcView->
    FillRect (Rect
	      (cBounds.right - m_vRightBorder, cBounds.top + 1,
	       cBounds.right - 1, cBounds.bottom - m_vBottomBorder),
	      sFillColor);





  // Zoom Button
  if ((m_nFlags & WND_NO_ZOOM_BUT) == 0)
    {
      DrawZoomButton ();
    }

  // Minimize Button
  if ((m_nFlags & WND_NO_DEPTH_BUT) == 0)
    {
      DrawMinimizeButton ();
    }

  // Close Button
  if ((m_nFlags & WND_NO_CLOSE_BUT) == 0)
    {
      DrawCloseButton ();
    }
}

void
WarpSpeedDecorator::Layout ()
{
  float vRight = m_cBounds.right - 5;
  float vBtnWidth = m_vTopBorder - 5;

  // Zoom Button
  if (m_nFlags & WND_NO_ZOOM_BUT)
    {
      m_cZoomRect.left = m_cZoomRect.right = 0;
    }
  else
    {
      m_cZoomRect.top = 4;
      m_cZoomRect.bottom = m_vTopBorder - 1;
      m_cZoomRect.right = vRight;
      m_cZoomRect.left = m_cZoomRect.right - vBtnWidth;
      vRight = m_cZoomRect.left - 1.0f;
    }

  if (m_nFlags & WND_NO_DEPTH_BUT)
    {
      m_cMinimizeRect.left = m_cMinimizeRect.right = 0;
    }
  else
    {
      m_cMinimizeRect.top = 4;
      m_cMinimizeRect.bottom = m_vTopBorder - 1;
      m_cMinimizeRect.right = vRight;
      m_cMinimizeRect.left = m_cMinimizeRect.right - vBtnWidth;
      vRight = m_cMinimizeRect.left - 1.0f;
    }


  if (m_nFlags & WND_NO_CLOSE_BUT)
    {
      m_cCloseRect.left = m_cCloseRect.right = 0;
    }
  else
    {
      m_cCloseRect.top = 4;
      m_cCloseRect.bottom = m_vTopBorder - 1;
      m_cCloseRect.right = vRight;
      m_cCloseRect.left = m_cCloseRect.right - vBtnWidth;
      vRight = m_cCloseRect.left - 1.0f;
    }
  m_cTitleRect.top = 4;
  m_cTitleRect.bottom = m_vTopBorder - 1;
  m_cTitleRect.right = vRight;
  m_cTitleRect.left = m_vLeftBorder;


}

extern "C" int
get_api_version ()
{
  return (WNDDECORATOR_APIVERSION);
}


void
WarpSpeedDecorator::DrawZoomButton ()
{
  Layer *pcView = GetLayer ();
  Rect cFrame = m_cZoomRect;

  pcView->FillRect (cFrame, get_default_color (COL_NORMAL));

  cFrame.left += 2;
  cFrame.top += 2;
  cFrame.right -= 2;
  cFrame.bottom -= 2;

  pcView->DrawFrame (cFrame,
		     (m_bZoomState ? FRAME_RECESSED : FRAME_RAISED) |
		     FRAME_THIN | FRAME_TRANSPARENT);
  cFrame.left += 2;
  cFrame.top += 2;
  cFrame.right -= 2;
  cFrame.bottom -= 2;
  pcView->DrawFrame (cFrame,
		     (m_bZoomState ? FRAME_RAISED : FRAME_RECESSED) |
		     FRAME_THIN | FRAME_TRANSPARENT);
}



void
WarpSpeedDecorator::DrawMinimizeButton ()
{
  Layer *pcView = GetLayer ();
  Rect cFrame = m_cMinimizeRect;
  pcView->FillRect (cFrame, get_default_color (COL_NORMAL));

  cFrame.left += 4;
  cFrame.top += 4;
  cFrame.right -= 4;
  cFrame.bottom -= 4;

  pcView->DrawFrame (cFrame,
		     (m_bMinimizeState ? FRAME_RECESSED : FRAME_RAISED) |
		     FRAME_THIN | FRAME_TRANSPARENT);
  cFrame.left += 2;
  cFrame.top += 2;
  cFrame.right -= 2;
  cFrame.bottom -= 2;
  pcView->DrawFrame (cFrame,
		     (m_bMinimizeState ? FRAME_RAISED : FRAME_RECESSED) |
		     FRAME_THIN | FRAME_TRANSPARENT);
  pcView->SetFgColor (get_default_color (COL_NORMAL));
  pcView->
    FillRect (Rect
	      (cFrame.left - 2, cFrame.top + cFrame.Height () / 2 - 0,
	       cFrame.right + 2, cFrame.top + cFrame.Height () / 2 + 0));
  pcView->
    FillRect (Rect
	      (cFrame.left + cFrame.Width () / 2, cFrame.top - 2,
	       cFrame.left + cFrame.Width () / 2, cFrame.bottom + 2));
  //pcView->DrawLine(Point(cFrame.left - 2, cFrame.top + cFrame.Height() / 2), Point(cFrame.right + 2, cFrame.top + cFrame.Height() /2));
}



void
WarpSpeedDecorator::DrawCloseButton ()
{
  Layer *pcView = GetLayer ();

  // Stroke outer rect
  Rect cOuter = m_cCloseRect;
  pcView->FillRect (cOuter, get_default_color (COL_NORMAL));
  cOuter.left += 2;
  cOuter.top += 2;
  cOuter.right -= 2;
  cOuter.bottom -= 2;
  pcView->
    SetFgColor (get_default_color (m_bCloseState ? COL_SHADOW : COL_SHINE));
  pcView->DrawLine (Point (cOuter.left, cOuter.top),
		    Point (cOuter.right - 1, cOuter.top));
  pcView->DrawLine (Point (cOuter.left, cOuter.top),
		    Point (cOuter.left, cOuter.bottom));
  pcView->
    SetFgColor (get_default_color (m_bCloseState ? COL_SHINE : COL_SHADOW));
  pcView->DrawLine (Point (cOuter.right, cOuter.top),
		    Point (cOuter.right, cOuter.bottom));
  pcView->DrawLine (Point (cOuter.left + 1, cOuter.bottom),
		    Point (cOuter.right, cOuter.bottom));
  // Stroke inner rect
  cOuter.left += 2;
  cOuter.top += 2;
  cOuter.right -= 2;
  cOuter.bottom -= 2;
  pcView->
    SetFgColor (get_default_color (m_bCloseState ? COL_SHINE : COL_SHADOW));
  pcView->DrawLine (Point (cOuter.left, cOuter.top),
		    Point (cOuter.right - 2, cOuter.top));
  pcView->DrawLine (Point (cOuter.left, cOuter.top),
		    Point (cOuter.left, cOuter.bottom - 2));
  pcView->
    SetFgColor (get_default_color (m_bCloseState ? COL_SHADOW : COL_SHINE));
  pcView->DrawLine (Point (cOuter.right, cOuter.top + 2),
		    Point (cOuter.right, cOuter.bottom));
  pcView->DrawLine (Point (cOuter.left + 2, cOuter.bottom),
		    Point (cOuter.right, cOuter.bottom));

  // Draw 45 degree line 
  pcView->DrawLine (Point (cOuter.left + 1, cOuter.bottom - 2),
		    Point (cOuter.right - 2, cOuter.top + 1));
  pcView->
    SetFgColor (get_default_color (m_bCloseState ? COL_SHINE : COL_SHADOW));
  pcView->DrawLine (Point (cOuter.left + 2, cOuter.bottom - 1),
		    Point (cOuter.right - 1, cOuter.top + 2));

}

void
WarpSpeedDecorator::DrawTitle ()
{
  Rect cFrame = m_cTitleRect;
  Layer *pcView = GetLayer ();
  Color32_s sFillColor =
    m_bHasFocus ? get_default_color (COL_SEL_WND_BORDER) :
    get_default_color (COL_NORMAL_WND_BORDER);

  pcView->DrawFrame (cFrame, FRAME_RECESSED | FRAME_THIN | FRAME_TRANSPARENT);
  cFrame.top += 1;
  cFrame.left += 1;
  cFrame.bottom -= 1;
  cFrame.right -= 1;

  pcView->FillRect (cFrame, sFillColor);

  if((m_nFlags & WND_NO_TITLE) == 0)
  {
  	pcView->SetFgColor (255, 255, 255, 0);
 	 pcView->SetBgColor (sFillColor);
  // Make sure title doesn't overflow into controls
  	cFrame.left += 5;
  	cFrame.right -= 5;

  	float vTextWidth = pcView->m_pcFont->GetInstance()->GetStringWidth (m_cTitle.c_str(),strlen(m_cTitle.c_str()));
  	if( vTextWidth > cFrame.Width() ) {
	  	std::string cText;
  		int nLen = (int)floor( cFrame.Width() / ceil( vTextWidth / m_cTitle.length() ) );
  		cText = m_cTitle.substr( 0, nLen ) + "...";
  		pcView->DrawText (cFrame, cText.c_str (), -1, 0);
  	} else {
  		pcView->DrawText (cFrame, m_cTitle.c_str (), -1, 0);
  	}
  }
}




extern "C" WindowDecorator *
create_decorator (Layer * pcLayer, uint32 nFlags)
{
  return (new WarpSpeedDecorator (pcLayer, nFlags));
}
