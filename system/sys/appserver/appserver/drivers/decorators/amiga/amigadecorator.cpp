/*
 *  The Syllable application server
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

#include "amigadecorator.h"
#include <layer.h>

#include <gui/window.h>

using namespace os;

AmigaDecorator::AmigaDecorator( Layer* pcLayer, uint32 nWndFlags ) :
    WindowDecorator( pcLayer, nWndFlags )
{
    m_nFlags = nWndFlags;

    m_bHasFocus   = false;

	for( int i = 0; i < HIT_DRAG + 1; i++ ) {
		m_bObjectState[ i ] = false;
	}
  
    m_sFontHeight = pcLayer->GetFontHeight();
    
    CalculateBorderSizes();
}

AmigaDecorator::~AmigaDecorator()
{
}

uint32 AmigaDecorator::CheckIndex( uint32 nButton )
{
	return ( nButton < HIT_DRAG+1 && nButton > 0 ) ? nButton : 0 ;
}

void AmigaDecorator::CalculateBorderSizes()
{
    if ( m_nFlags & WND_NO_BORDER ) {
	m_vLeftBorder   = 0;
	m_vRightBorder  = 0;
	m_vTopBorder    = 0;
	m_vBottomBorder = 0;
    } else {
	if ( (m_nFlags & (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE)) ==
	     (WND_NO_TITLE | WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT | WND_NOT_MOVEABLE) ) {
	    m_vTopBorder = 4;
	} else {
	    m_vTopBorder = float(m_sFontHeight.ascender + m_sFontHeight.descender + 6);
	}
	m_vLeftBorder   = 4;
	m_vRightBorder  = 4;
	m_vBottomBorder = 4;
    }
    
}

void AmigaDecorator::SetFlags( uint32 nFlags )
{
    Layer* pcView = GetLayer();
    m_nFlags = nFlags;
    CalculateBorderSizes();
    pcView->Invalidate();
    Layout();
    
}

void AmigaDecorator::FontChanged()
{
    Layer* pcView = GetLayer();
    m_sFontHeight = pcView->GetFontHeight();
    CalculateBorderSizes();
    pcView->Invalidate();
    Layout();
}

void AmigaDecorator::Layout()
{
	m_cObjectFrame[ HIT_DEPTH ] = Rect(0, 0, 0, 0);
	m_cObjectFrame[ HIT_CLOSE ] = Rect(0, 0, 0, 0);
	m_cObjectFrame[ HIT_ZOOM ] = Rect(0, 0, 0, 0);
	m_cObjectFrame[ HIT_MINIMIZE ] = Rect(0, 0, 0, 0);

	float vRight = m_cBounds.right;
	float vBtnWidth = m_vTopBorder;
	float vLeft = m_cBounds.left;

    if( ( m_nFlags & WND_NO_CLOSE_BUT ) == 0 ) {
		m_cObjectFrame[ HIT_CLOSE ] = Rect(0,0,m_vTopBorder - 1,m_vTopBorder - 1);
		vLeft = m_cObjectFrame[ HIT_CLOSE ].right + 1.0f;
	}

    if( ( m_nFlags & WND_NO_DEPTH_BUT ) == 0 ) {
		m_cObjectFrame[ HIT_DEPTH ] = Rect(vRight - vBtnWidth, 0, vRight,  m_vTopBorder - 1);
		vRight = m_cObjectFrame[ HIT_DEPTH ].left - 1.0f;
	}  

    if( ( m_nFlags & WND_NO_ZOOM_BUT ) == 0 ) {
		m_cObjectFrame[ HIT_ZOOM ] = Rect(vRight - vBtnWidth, 0, vRight,  m_vTopBorder - 1);
		vRight = m_cObjectFrame[ HIT_ZOOM ].left - 1.0f;
	}  

    if( ( m_nFlags & WND_NO_ZOOM_BUT ) == 0 ) {
		m_cObjectFrame[ HIT_MINIMIZE ] = Rect(vRight - vBtnWidth, 0, vRight,  m_vTopBorder - 1);
		vRight = m_cObjectFrame[ HIT_MINIMIZE ].left - 1.0f;
	} else {
		m_cObjectFrame[ HIT_MINIMIZE ] = Rect( vRight, 0, vRight, 0 );
	}    

	m_cObjectFrame[ HIT_DRAG ] = Rect( vLeft, 0, vRight, m_vTopBorder - 1 );
}

Point AmigaDecorator::GetMinimumSize()
{
    Point cMinSize( 0.0f, m_vTopBorder + m_vBottomBorder );

	cMinSize.x = m_cObjectFrame[ HIT_CLOSE ].right + m_cBounds.right - m_cObjectFrame[ HIT_MINIMIZE ].left;

	if ( cMinSize.x < m_vLeftBorder + m_vRightBorder ) {
		cMinSize.x = m_vLeftBorder + m_vRightBorder;
	}

	return( cMinSize );
}

WindowDecorator::hit_item AmigaDecorator::HitTest( const Point& cPosition )
{
    if ( cPosition.x < 4 ) {
	if ( cPosition.y < 4 ) {
	    return( HIT_SIZE_LT );
	} else if ( cPosition.y > m_cBounds.bottom - 4 ) {
	    return( HIT_SIZE_LB );
	} else {
	    return( HIT_SIZE_LEFT );
	}
    } else if ( cPosition.x > m_cBounds.right - 4 ) {
	if ( cPosition.y < 4 ) {
	    return( HIT_SIZE_RT );
	} else if ( cPosition.y > m_cBounds.bottom - 4 ) {
	    return( HIT_SIZE_RB );
	} else {
	    return( HIT_SIZE_RIGHT );
	}
    } else if ( cPosition.y < 4 ) {
	return( HIT_SIZE_TOP );
    } else if ( cPosition.y > m_cBounds.bottom - 4 ) {
	return( HIT_SIZE_BOTTOM );
    }
  if ( m_cObjectFrame[HIT_CLOSE].DoIntersect( cPosition ) ) {
    return( HIT_CLOSE );
  } else if ( m_cObjectFrame[HIT_ZOOM].DoIntersect( cPosition ) ) {
    return( HIT_ZOOM );
  } else if ( m_cObjectFrame[HIT_DEPTH].DoIntersect( cPosition ) ) {
    return( HIT_DEPTH );
  } else if ( m_cObjectFrame[HIT_MINIMIZE].DoIntersect( cPosition ) ) {
    return( HIT_MINIMIZE );
  } else if ( m_cObjectFrame[HIT_DRAG].DoIntersect( cPosition ) ) {
    return( HIT_DRAG );
  }
    return( HIT_NONE );
}

Rect AmigaDecorator::GetBorderSize()
{
    return( Rect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void AmigaDecorator::SetTitle( const char* pzTitle )
{
    m_cTitle = pzTitle;
    Render( m_cBounds );
}

void AmigaDecorator::SetFocusState( bool bHasFocus )
{
    m_bHasFocus = bHasFocus;
    Render( m_cBounds );
}

void AmigaDecorator::SetWindowFlags( uint32 nFlags )
{
    m_nFlags = nFlags;
}

void AmigaDecorator::FrameSized( const Rect& cFrame )
{
    Layer* pcView = GetLayer();
    Point cDelta( cFrame.Width() - m_cBounds.Width(), cFrame.Height() - m_cBounds.Height() );
    m_cBounds = cFrame.Bounds();

    Layout();
    if ( cDelta.x != 0.0f ) {
	Rect cDamage = m_cBounds;

	cDamage.left = m_cObjectFrame[ HIT_MINIMIZE ].left - fabs(cDelta.x)  - 2.0f;
	pcView->Invalidate( cDamage );
    }
    if ( cDelta.y != 0.0f ) {
	Rect cDamage = m_cBounds;

	cDamage.top = cDamage.bottom - std::max( m_vBottomBorder, m_vBottomBorder + cDelta.y ) - 1.0f;
	pcView->Invalidate( cDamage );
    }
}

void AmigaDecorator::SetButtonState( uint32 nButton, bool bPushed )
{
	if( nButton == HIT_CLOSE || nButton == HIT_ZOOM || nButton == HIT_DEPTH ||
		nButton == HIT_MINIMIZE ) {
			m_bObjectState[ nButton ] = bPushed;
		    Color32_s sFillColor =  m_bHasFocus ? get_default_color( COL_SEL_WND_BORDER ) : get_default_color( COL_NORMAL_WND_BORDER );
//			Color32_s sFillColor =  m_bHasFocus ? GetDefaultColor( PEN_SELWINTITLE ) : GetDefaultColor( PEN_WINTITLE );
			DrawButton( nButton, sFillColor );
	}
}

#define SetRect( xa, ya, xb, yb )	{									\
										r.left = floor(xa * cScale.x);	\
										r.right = ceil(xb * cScale.x);	\
										r.top = floor(ya * cScale.y);	\
										r.bottom = ceil(yb * cScale.y);	\
										r.MoveTo( cFrame.left, 			\
												cFrame.top );			\
									}

void AmigaDecorator::DrawButton( uint32 nButton, const Color32_s& sFillColor )
{
	Layer *pcView = GetLayer();
	nButton = CheckIndex( nButton );
	Rect cFrame = m_cObjectFrame[ nButton ];
	bool bState = m_bObjectState[ nButton ];
	font_height fh;
	fh = pcView->GetFontHeight();
	Rect r;
	Point cScale( cFrame.Size() );

	if( cFrame.Width() < 1 )
		return;

	pcView->FillRect( cFrame, sFillColor );
	pcView->DrawFrame( cFrame, FRAME_TRANSPARENT | FRAME_THIN |
		( bState ? FRAME_RECESSED : FRAME_RAISED ) );

	switch( nButton ) {
		case WindowDecorator::HIT_DRAG:
			pcView->SetFgColor( 255, 255, 255, 0 );
			pcView->SetBgColor( sFillColor );
			pcView->MovePenTo( cFrame.left + 5,
			   (cFrame.Height()+1.0f) / 2 -
			   (fh.ascender + fh.descender) / 2 +
				fh.ascender );
			pcView->DrawString( m_cTitle.c_str(), -1 );
			break;
		case WindowDecorator::HIT_CLOSE:
			SetRect( 0.33, 0.33, 0.67, 0.67 );
			pcView->DrawFrame( r, FRAME_TRANSPARENT | FRAME_THIN |
				( bState ? FRAME_RAISED : FRAME_RECESSED ) );
			break;
		case WindowDecorator::HIT_MINIMIZE:
			SetRect( 0.2, 0.2, 0.8, 0.8 );
			pcView->DrawFrame( r, FRAME_TRANSPARENT | FRAME_THIN |
				( bState ? FRAME_RAISED : FRAME_RECESSED ) );
			SetRect( 0.2, 0.6, 0.4, 0.8 );
			r.left++;
			r.right++;
			r.top--;
			r.bottom--;
			pcView->DrawFrame( r, FRAME_TRANSPARENT | FRAME_THIN |
				( bState ? FRAME_RECESSED : FRAME_RAISED ) );
			break;
		case WindowDecorator::HIT_DEPTH:
			if( bState ) {
				SetRect( 0.2, 0.2, 0.6, 0.6 );
				pcView->DrawFrame( r, FRAME_TRANSPARENT | FRAME_THIN |
					( FRAME_RAISED ) );
				SetRect( 0.4, 0.4, 0.8, 0.8 );
				pcView->FillRect( r, sFillColor );
				pcView->DrawFrame( r, FRAME_TRANSPARENT | FRAME_THIN |
					( FRAME_RAISED ) );
			} else {
				SetRect( 0.4, 0.4, 0.8, 0.8 );
				pcView->DrawFrame( r, FRAME_TRANSPARENT | FRAME_THIN |
					( FRAME_RECESSED ) );
				SetRect( 0.2, 0.2, 0.6, 0.6 );
				pcView->FillRect( r, sFillColor );
				pcView->DrawFrame( r, FRAME_TRANSPARENT | FRAME_THIN |
					( FRAME_RECESSED ) );
			}
			break;
		case WindowDecorator::HIT_ZOOM:
			SetRect( 0.2, 0.2, 0.8, 0.8 );
			pcView->DrawFrame( r, FRAME_TRANSPARENT |  FRAME_THIN |
				( bState ? FRAME_RAISED : FRAME_RECESSED ) );
			SetRect( 0.2, 0.2, 0.6, 0.6 );
			r.left++;
			r.top++;
			pcView->DrawFrame( r, FRAME_TRANSPARENT |  FRAME_THIN |
				( bState ? FRAME_RECESSED : FRAME_RAISED ) );
			break;
		}
}

void AmigaDecorator::Render( const Rect& cUpdateRect )
{
    if ( m_nFlags & WND_NO_BORDER ) {
	return;
    }
  
    Color32_s sFillColor =  m_bHasFocus ? get_default_color( COL_SEL_WND_BORDER ) : get_default_color( COL_NORMAL_WND_BORDER );

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
    pcView->FillRect( Rect( cOBounds.left + 1, cOBounds.top + m_vTopBorder,
			    cIBounds.left - 1, cIBounds.bottom ), sFillColor );
      // Right
    pcView->FillRect( Rect( cIBounds.right + 1, cOBounds.top + m_vTopBorder,
			    cOBounds.right - 1, cIBounds.bottom ), sFillColor );

    if ( (m_nFlags & WND_NO_TITLE) == 0 ) {
    	DrawButton( HIT_DRAG, sFillColor );
    } else {
		pcView->FillRect( Rect( cOBounds.left + 1, cOBounds.top - 1, cOBounds.right - 1, cIBounds.top + 1 ), sFillColor );
    }

	DrawButton( HIT_ZOOM, sFillColor );
	DrawButton( HIT_MINIMIZE, sFillColor );
	DrawButton( HIT_DEPTH, sFillColor );
	DrawButton( HIT_CLOSE, sFillColor );
}

extern "C" int get_api_version()
{
    return( WNDDECORATOR_APIVERSION );
}

extern "C" WindowDecorator* create_decorator( Layer* pcLayer, uint32 nFlags )
{
    return( new AmigaDecorator( pcLayer, nFlags ) );
}

