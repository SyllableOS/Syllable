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

/*
 * Changes:
 *
 * 02-07-24: Added code for disabling. The disabled look may need more work
 *           (right now it's just made grey).
 *
 */

#include <assert.h>
#include <atheos/types.h>

#include <gui/scrollbar.h>
#include <util/looper.h>
#include <util/message.h>

#include "stdbitmaps.h"

#include <macros.h>

using namespace os;

enum { HIT_NONE, HIT_KNOB, HIT_ARROW };

enum { ID_SCROLL_TIMER = 1 };

class ScrollBar::Private
{
public:
    Private() {
	m_nHitState    = HIT_NONE;
	m_nHitButton   = 0;
	m_vProportion  = 0.1f;
	m_nOrientation = VERTICAL;
	m_vMin	       = 0.0f;
	m_vMax	       = FLT_MAX;
	m_vSmallStep   = 1.0f;
	m_vBigStep     = 10.0f;
	m_bChanged     = false;
	m_pcTarget     = NULL;
	m_bFirstTick   = true;
	memset( m_abArrowStates, 0, sizeof(m_abArrowStates) );
    }
    View*	m_pcTarget;
    float	m_vMin;
    float	m_vMax;
    float	m_vProportion;
    float	m_vSmallStep;
    float	m_vBigStep;
    int		m_nOrientation;
    Rect	m_acArrowRects[4];
    bool	m_abArrowStates[4];
    Rect	m_cKnobArea;
    bool	m_bChanged;
    bool	m_bFirstTick;
    Point	m_cHitPos;
    int		m_nHitButton;
    int		m_nHitState;
};

static Color32_s Tint( const Color32_s& sColor, float vTint )
{
    int r = int( (float(sColor.red) * vTint + 127.0f * (1.0f - vTint)) );
    int g = int( (float(sColor.green) * vTint + 127.0f * (1.0f - vTint)) );
    int b = int( (float(sColor.blue) * vTint + 127.0f * (1.0f - vTint)) );
    if ( r < 0 ) r = 0; else if (r > 255) r = 255;
    if ( g < 0 ) g = 0; else if (g > 255) g = 255;
    if ( b < 0 ) b = 0; else if (b > 255) b = 255;
    return( Color32_s( r, g, b, sColor.alpha ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ScrollBar::ScrollBar( const Rect& cFrame, const char* pzName, Message* pcMsg, float vMin, float vMax,
		      int nOrientation, uint32 nResizeMask )
	: Control( cFrame, pzName, "", pcMsg, nResizeMask, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE )
{
    m = new Private;
    m->m_nOrientation = nOrientation;
    m->m_vMin	   = vMin;
    m->m_vMax	   = vMax;
    FrameSized( Point( 0.0f, 0.0f ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ScrollBar::~ScrollBar()
{
    if ( m->m_pcTarget != NULL ) {
	if ( m->m_nOrientation == HORIZONTAL ) {
	    m->m_pcTarget->_SetHScrollBar( NULL );
	} else {
	    m->m_pcTarget->_SetVScrollBar( NULL );
	}
    }
    delete m;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScrollBar::SetProportion( float vProp )
{
    m->m_vProportion = vProp;
    Invalidate( m->m_cKnobArea );
    Flush();
}

float ScrollBar::GetProportion() const
{
    return( m->m_vProportion );
}

void ScrollBar::SetSteps( float vSmall, float vBig )
{
    m->m_vSmallStep = vSmall;
    m->m_vBigStep   = vBig;
}

void ScrollBar::GetSteps( float* pvSmall, float* pvBig ) const
{
    *pvSmall = m->m_vSmallStep;
    *pvBig   = m->m_vBigStep;
}

void ScrollBar::SetMinMax( float vMin, float vMax )
{
    m->m_vMin = vMin;
    m->m_vMax = vMax;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScrollBar::SetScrollTarget( View* pcTarget )
{
    if ( m->m_pcTarget != NULL ) {
	if ( m->m_nOrientation == HORIZONTAL ) {
	    assert( m->m_pcTarget->GetHScrollBar() == this );
	    m->m_pcTarget->_SetHScrollBar( NULL );
	} else {
	    assert( m->m_pcTarget->GetVScrollBar() == this );
	    m->m_pcTarget->_SetVScrollBar( NULL );
	}
    }
    m->m_pcTarget = pcTarget;
    if ( m->m_pcTarget != NULL ) {
	if ( m->m_nOrientation == HORIZONTAL ) {
	    assert( m->m_pcTarget->GetHScrollBar() == NULL );
	    m->m_pcTarget->_SetHScrollBar( this );
	    SetValue( m->m_pcTarget->GetScrollOffset().x );
	} else {
	    assert( m->m_pcTarget->GetVScrollBar() == NULL );
	    m->m_pcTarget->_SetVScrollBar( this );
	    SetValue( m->m_pcTarget->GetScrollOffset().y );
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

View* ScrollBar::GetScrollTarget( void )
{
    return( m->m_pcTarget );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point ScrollBar::GetPreferredSize( bool bLargest ) const
{
    float w = 16.0f;
    float l = (bLargest) ? COORD_MAX : 0;

    if ( m->m_nOrientation == HORIZONTAL )	{
	return( Point( l, w ) );
    } else {
	return( Point( w, l ) );
    }
}

void ScrollBar::PostValueChange( const Variant& cNewValue )
{
    if ( m->m_pcTarget != NULL ) {
	Point cPos = m->m_pcTarget->GetScrollOffset();

	if ( m->m_nOrientation == HORIZONTAL )	{
	    cPos.x = -floor( cNewValue.AsFloat() );
	} else {
	    cPos.y = -floor( cNewValue.AsFloat() );
	}
	m->m_pcTarget->ScrollTo( cPos );
	m->m_pcTarget->Flush();
    }    
    Invalidate( m->m_cKnobArea );
    Flush();
}

void ScrollBar::LabelChanged( const std::string& cNewLabel )
{
}

void ScrollBar::EnableStatusChanged( bool bIsEnabled )
{
    Invalidate();
    Flush();
}

bool ScrollBar::Invoked( Message* pcMessage )
{
    Control::Invoked( pcMessage );
    if ( m->m_nHitState == HIT_KNOB ) {
	pcMessage->AddBool( "final", false );
    } else {
	pcMessage->AddBool( "final", true );
    }
    return( true );
}

void ScrollBar::TimerTick( int nID )
{
    if ( nID == ID_SCROLL_TIMER ) {
	float vValue = GetValue();
	if ( m->m_nHitButton & 0x01 ) {
	    vValue += m->m_vSmallStep;
	} else {
	    vValue -= m->m_vSmallStep;
	}
	if ( vValue < m->m_vMin ) {
	    vValue = m->m_vMin;
	} else if ( vValue > m->m_vMax ) {
	    vValue = m->m_vMax;
	}
	SetValue( vValue );
	if ( m->m_bFirstTick ) {
	    GetLooper()->AddTimer( this, ID_SCROLL_TIMER, 30000LL, false );
	    m->m_bFirstTick = false;
	}
    } else {
	Control::TimerTick( nID );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScrollBar::MouseDown( const Point& cPosition, uint32 nButtons )
{
    if ( IsEnabled() == false ) {
	View::MouseDown( cPosition, nButtons );
	return;
    }

    MakeFocus( true );
    m->m_nHitState  = HIT_NONE;
    m->m_bChanged = false;
    
    for ( int i = 0 ; i < 4 ; ++i ) {
	if ( m->m_acArrowRects[i].DoIntersect( cPosition ) ) {
	    m->m_abArrowStates[i] = true;
	    m->m_nHitButton = i;
	    m->m_nHitState  = HIT_ARROW;
	    Invalidate( m->m_acArrowRects[i] );
	    Flush();
	    m->m_bFirstTick = true;
	    GetLooper()->AddTimer( this, ID_SCROLL_TIMER, 300000LL, true );
	    return;
	}
    }
    if ( m->m_cKnobArea.DoIntersect( cPosition ) ) {
	Rect cKnobFrame = GetKnobFrame();
	if ( cKnobFrame.DoIntersect( cPosition ) ) {
	    m->m_cHitPos = cPosition - cKnobFrame.LeftTop();
	    m->m_nHitState  = HIT_KNOB;
	    return;
	}
	float vValue = GetValue();
	if ( m->m_nOrientation == HORIZONTAL ) {
	    if ( cPosition.x < cKnobFrame.left ) {
		vValue -= m->m_vBigStep;
	    } else if ( cPosition.x > cKnobFrame.right ) {
		vValue += m->m_vBigStep;
	    }
	} else {
	    if ( cPosition.y < cKnobFrame.top ) {
		vValue -= m->m_vBigStep;
	    } else if ( cPosition.y > cKnobFrame.bottom ) {
		vValue += m->m_vBigStep;
	    }
	}
	if ( vValue < m->m_vMin ) {
	    vValue = m->m_vMin;
	} else if ( vValue > m->m_vMax ) {
	    vValue = m->m_vMax;
	}
	SetValue( vValue );
    }
    
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScrollBar::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
    if ( IsEnabled() == false ) {
	View::MouseUp( cPosition, nButtons, pcData );
	return;
    }

    if ( m->m_nHitState == HIT_ARROW ) {
	float vValue = GetValue();
	bool bChanged = false;
	if ( m->m_bFirstTick ) {
	    for ( int i = 0 ; i < 4 ; ++i ) {
		if ( m->m_acArrowRects[i].DoIntersect( cPosition ) ) {
		    if ( i == m->m_nHitButton ) {
			bChanged = true;
			if ( i & 0x01 ) {
			    vValue += m->m_vSmallStep;
			} else {
			    vValue -= m->m_vSmallStep;
			}
		    }
		    break;
		}
	    }
	}
	if ( m->m_abArrowStates[m->m_nHitButton] ) {
	    GetLooper()->RemoveTimer( this, ID_SCROLL_TIMER );	    
	    m->m_abArrowStates[m->m_nHitButton] = false;
	}
	Invalidate( m->m_acArrowRects[m->m_nHitButton] );
	if ( bChanged ) {
	    if ( vValue < m->m_vMin ) {
		vValue = m->m_vMin;
	    } else if ( vValue > m->m_vMax ) {
		vValue = m->m_vMax;
	    }
	    SetValue( vValue );
	}
	Flush();
    } else if ( m->m_nHitState == HIT_KNOB ) {
	if ( m->m_bChanged ) {
	    Invoke();	// Send a 'final' message
	    m->m_bChanged = false;
	}
    }

    m->m_nHitState  = HIT_NONE;
    MakeFocus( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScrollBar::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
    if ( IsEnabled() == false ) {
	View::MouseMove( cNewPos, nCode, nButtons, pcData );
	return;
    }

    if ( m->m_nHitState == HIT_ARROW ) {
	int i;
	for ( i = 0 ; i < 4 ; ++i ) {
	    if ( m->m_acArrowRects[i].DoIntersect( cNewPos ) ) {
		break;
	    }
	}
	bool bHit = m->m_nHitButton == i;
	if ( bHit != m->m_abArrowStates[m->m_nHitButton] ) {
	    if ( m->m_abArrowStates[m->m_nHitButton] ) {
		GetLooper()->RemoveTimer( this, ID_SCROLL_TIMER );	    
	    } else {
		if ( m->m_bFirstTick ) {
		    GetLooper()->AddTimer( this, ID_SCROLL_TIMER, 300000LL, true );
		} else {
		    GetLooper()->AddTimer( this, ID_SCROLL_TIMER, 30000LL, false );
		}
	    }
	    m->m_abArrowStates[m->m_nHitButton] = bHit;
	    Invalidate( m->m_acArrowRects[m->m_nHitButton] );
	    Flush();
	}

    } else if ( m->m_nHitState == HIT_KNOB ) {
	SetValue( _PosToVal( cNewPos ) );
    }
}

void ScrollBar::WheelMoved( const Point& cDelta )
{
    float vValue = GetValue();

    if ( IsEnabled() == false ) {
	View::WheelMoved( cDelta );
	return;
    }

    if ( m->m_nOrientation == VERTICAL && cDelta.y != 0.0f ) {
	vValue += cDelta.y * m->m_vSmallStep;
    } else if ( m->m_nOrientation == HORIZONTAL && cDelta.x != 0.0f ) {
	vValue += cDelta.y * m->m_vSmallStep;
    }
    if ( vValue < m->m_vMin ) {
	vValue = m->m_vMin;
    } else if ( vValue > m->m_vMax ) {
	vValue = m->m_vMax;
    }
    
    SetValue( vValue );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScrollBar::Paint( const Rect& cUpdateRect )
{
    Rect cBounds    = GetBounds();
    Rect cKnobFrame = GetKnobFrame();

//  SetEraseColor( get_default_color( COL_SCROLLBAR_BG ) );

//    DrawFrame( cBounds, FRAME_RECESSED | FRAME_TRANSPARENT | FRAME_THIN );

    if( IsEnabled() ) {
	SetEraseColor( get_default_color( COL_SCROLLBAR_KNOB ) );
    } else {
	SetEraseColor( get_default_color( COL_NORMAL ) );
    }


    DrawFrame( cKnobFrame, FRAME_RAISED );

    if ( m->m_nOrientation == HORIZONTAL ) {
	if ( cKnobFrame.Width() > 30.0f ) {
	    const float vDist = 5.0f;
	    float vCenter = cKnobFrame.left + (cKnobFrame.Width() + 1.0f) * 0.5f;
	    SetFgColor( get_default_color( COL_SHINE ) );
	    DrawLine( Point( vCenter - vDist, cKnobFrame.top + 4.0f ), Point( vCenter - vDist, cKnobFrame.bottom - 4.0f ) );
	    DrawLine( Point( vCenter, cKnobFrame.top + 4.0f ), Point( vCenter, cKnobFrame.bottom - 4.0f ) );
	    DrawLine( Point( vCenter + vDist, cKnobFrame.top + 4.0f ), Point( vCenter + vDist, cKnobFrame.bottom - 4.0f ) );
	    
	    SetFgColor( get_default_color( COL_SHADOW ) );
	    vCenter -= 1.0f;
	    DrawLine( Point( vCenter - vDist, cKnobFrame.top + 4.0f ), Point( vCenter - vDist, cKnobFrame.bottom - 4.0f ) );
	    DrawLine( Point( vCenter, cKnobFrame.top + 4.0f ), Point( vCenter, cKnobFrame.bottom - 4.0f ) );
	    DrawLine( Point( vCenter + vDist, cKnobFrame.top + 4.0f ), Point( vCenter + vDist, cKnobFrame.bottom - 4.0f ) );
	}
    } else {
	if ( cKnobFrame.Height() > 30.0f ) {
	    const float vDist = 5.0f;
	    float vCenter = cKnobFrame.top + (cKnobFrame.Height() + 1.0f) * 0.5f;
	    SetFgColor( get_default_color( COL_SHINE ) );
	    DrawLine( Point( cKnobFrame.left + 4.0f, vCenter - vDist ), Point( cKnobFrame.right - 4.0f, vCenter - vDist ) );
	    DrawLine( Point( cKnobFrame.left + 4.0f, vCenter ), Point( cKnobFrame.right - 4.0f, vCenter ) );
	    DrawLine( Point( cKnobFrame.left + 4.0f, vCenter + vDist ), Point( cKnobFrame.right - 4.0f, vCenter + vDist ) );
	    
	    SetFgColor( get_default_color( COL_SHADOW ) );
	    vCenter -= 1.0f;
	    DrawLine( Point( cKnobFrame.left + 4.0f, vCenter - vDist ), Point( cKnobFrame.right - 4.0f, vCenter - vDist ) );
	    DrawLine( Point( cKnobFrame.left + 4.0f, vCenter ), Point( cKnobFrame.right - 4.0f, vCenter ) );
	    DrawLine( Point( cKnobFrame.left + 4.0f, vCenter + vDist ), Point( cKnobFrame.right - 4.0f, vCenter + vDist ) );
	}
    }

    if( IsEnabled() ) {
	SetFgColor( get_default_color( COL_SCROLLBAR_BG ) );
    } else {
    	SetFgColor( get_default_color( COL_NORMAL ) );
    }

    cBounds.left   += 1;
    cBounds.top    += 1;
    cBounds.right  -= 1;
    cBounds.bottom -= 1;
  
    Rect cFrame( m->m_cKnobArea );
    if ( m->m_nOrientation == HORIZONTAL ) {
	cFrame.right = cKnobFrame.left - 1.0f;
    } else {
	cFrame.bottom = cKnobFrame.top - 1.0f;
    }
    FillRect( cFrame );
  
    cFrame = m->m_cKnobArea;
    if ( m->m_nOrientation == HORIZONTAL ) {
	cFrame.left = cKnobFrame.right + 1.0f;
    } else {
	cFrame.top = cKnobFrame.bottom + 1.0f;
    }
    FillRect( cFrame );
    if ( m->m_nOrientation == HORIZONTAL ) {
	SetFgColor( Tint( get_default_color( COL_SHADOW ), 0.5f ) );
	DrawLine( Point( m->m_cKnobArea.left, m->m_cKnobArea.top - 1.0f ), Point( m->m_cKnobArea.right, m->m_cKnobArea.top - 1.0f ) );
	SetFgColor( Tint( get_default_color( COL_SHINE ), 0.6f ) );
	DrawLine( Point( m->m_cKnobArea.left, m->m_cKnobArea.bottom + 1.0f ), Point( m->m_cKnobArea.right, m->m_cKnobArea.bottom + 1.0f ) );
    } else {
	SetFgColor( Tint( get_default_color( COL_SHADOW ), 0.5f ) );
	DrawLine( Point( m->m_cKnobArea.left - 1.0f, m->m_cKnobArea.top ), Point( m->m_cKnobArea.left - 1.0f, m->m_cKnobArea.bottom ) );
	SetFgColor( Tint( get_default_color( COL_SHINE ), 0.6f ) );
	DrawLine( Point( m->m_cKnobArea.right + 1.0f, m->m_cKnobArea.top ), Point( m->m_cKnobArea.right + 1.0f, m->m_cKnobArea.bottom ) );
    }
    for ( int i = 0 ; i < 4 ; ++i ) {
	if ( m->m_acArrowRects[i].IsValid() ) {
	    Rect cBmRect;
	    Bitmap* pcBitmap;

	    if ( m->m_nOrientation == HORIZONTAL ) {
		pcBitmap = get_std_bitmap( ((i & 0x01) ? BMID_ARROW_RIGHT : BMID_ARROW_LEFT), 0, &cBmRect );
	    } else {
		pcBitmap = get_std_bitmap( ((i & 0x01) ? BMID_ARROW_DOWN : BMID_ARROW_UP), 0, &cBmRect );
	    }
	    DrawFrame( m->m_acArrowRects[i], (m->m_abArrowStates[i] ? FRAME_RECESSED : FRAME_RAISED) | FRAME_THIN );

	    SetDrawingMode( DM_OVER );
	    
	    DrawBitmap( pcBitmap, cBmRect, cBmRect.Bounds() + Point( (m->m_acArrowRects[i].Width() + 1.0f) * 0.5 - (cBmRect.Width() + 1.0f) * 0.5,
							    (m->m_acArrowRects[i].Height() + 1.0f) * 0.5 - (cBmRect.Height() + 1.0f) * 0.5 ) +
			m->m_acArrowRects[i].LeftTop() );
	    
	    SetDrawingMode( DM_COPY );
	    
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float ScrollBar::_PosToVal( Point cPos ) const
{
    float	vValue	= m->m_vMin;

    cPos	-= m->m_cHitPos;

    if ( m->m_nOrientation == HORIZONTAL ) {
	float	vSize =	(m->m_cKnobArea.Width()+1.0f) * m->m_vProportion;
	float	vLen  =	(m->m_cKnobArea.Width()+1.0f) - vSize;

	if ( vLen > 0.0f ) {
	    float vPos = (cPos.x - m->m_cKnobArea.left) / vLen;
	    vValue = m->m_vMin + (m->m_vMax - m->m_vMin) * vPos;
	}
    } else {
	float vSize = (m->m_cKnobArea.Height()+1.0f) * m->m_vProportion;
	float vLen  = (m->m_cKnobArea.Height()+1.0f) - vSize;

	if ( vLen > 0.0f ) {
	    float vPos = (cPos.y - m->m_cKnobArea.top) / vLen;
	    vValue = m->m_vMin + (m->m_vMax - m->m_vMin) * vPos;
	}
    }
    return( min( max( vValue, m->m_vMin ), m->m_vMax ) );
}

void ScrollBar::FrameSized( const Point& cDelta )
{
    Rect cBounds = GetBounds();
    Rect cArrowRect = cBounds;
    
    m->m_cKnobArea = cBounds;
    if ( m->m_nOrientation == HORIZONTAL ) {
	cArrowRect.right = ceil( (cArrowRect.Height() + 1.0f) * 0.7 ) - 1.0f;
	float vWidth = cArrowRect.Width() + 1.0f;
	
	m->m_acArrowRects[0] = cArrowRect;
	m->m_acArrowRects[1] = cArrowRect + Point( vWidth, 0.0f );
	m->m_acArrowRects[2] = cArrowRect + Point( cBounds.Width() - vWidth * 2.0f + 1.0f, 0.0f );
	m->m_acArrowRects[3] = cArrowRect + Point( cBounds.Width() - vWidth + 1.0f, 0.0f );

	if ( m->m_acArrowRects[0].right > m->m_acArrowRects[3].left ) {
	    m->m_acArrowRects[0].right = floor( (cBounds.Width() + 1.0f) * 0.5f - 1.0f );
	    m->m_acArrowRects[3].left  = m->m_acArrowRects[0].right + 1.0f;
	    m->m_acArrowRects[1].left  = m->m_acArrowRects[0].right + 1.0f;
	    m->m_acArrowRects[1].right = m->m_acArrowRects[1].left - 1.0f;
	    m->m_acArrowRects[2].right = m->m_acArrowRects[3].left - 1.0f;
	    m->m_acArrowRects[2].left  = m->m_acArrowRects[2].right + 1.0f;
	} else if ( m->m_acArrowRects[1].right + 16.0f > m->m_acArrowRects[2].left ) {
	    m->m_acArrowRects[1].right = m->m_acArrowRects[1].left - 1.0f;
	    m->m_acArrowRects[2].left = m->m_acArrowRects[2].right + 1.0f;
	}
	m->m_cKnobArea.left    = m->m_acArrowRects[1].right + 1.0f;
	m->m_cKnobArea.right   = m->m_acArrowRects[2].left  - 1.0f;
	m->m_cKnobArea.top    += 1.0f;
	m->m_cKnobArea.bottom -= 1.0f;
    } else {
	cArrowRect.bottom = ceil( (cArrowRect.Width() + 1.0f) * 0.7 ) - 1.0f;

	float vWidth = cArrowRect.Height() + 1.0f;
	
	m->m_acArrowRects[0] = cArrowRect;
	m->m_acArrowRects[1] = cArrowRect + Point( 0.0f, vWidth );
	m->m_acArrowRects[2] = cArrowRect + Point( 0.0f, cBounds.Height() - vWidth * 2.0f + 1.0f );
	m->m_acArrowRects[3] = cArrowRect + Point( 0.0f, cBounds.Height() - vWidth + 1.0f );

	if ( m->m_acArrowRects[0].bottom > m->m_acArrowRects[3].top ) {
	    m->m_acArrowRects[0].bottom = floor( (cBounds.Width() + 1.0f) * 0.5f - 1.0f );
	    m->m_acArrowRects[3].top    = m->m_acArrowRects[0].bottom + 1.0f;
	    m->m_acArrowRects[1].top    = m->m_acArrowRects[0].bottom + 1.0f;
	    m->m_acArrowRects[1].bottom = m->m_acArrowRects[1].top - 1.0f;
	    m->m_acArrowRects[2].bottom = m->m_acArrowRects[3].top - 1.0f;
	    m->m_acArrowRects[2].top    = m->m_acArrowRects[2].bottom + 1.0f;
	} else if ( m->m_acArrowRects[1].bottom + 16.0f > m->m_acArrowRects[2].top ) {
	    m->m_acArrowRects[1].bottom = m->m_acArrowRects[1].top - 1.0f;
	    m->m_acArrowRects[2].top    = m->m_acArrowRects[2].bottom + 1.0f;
	}
	m->m_cKnobArea.top    = m->m_acArrowRects[1].bottom + 1.0f;
	m->m_cKnobArea.bottom = m->m_acArrowRects[2].top  - 1.0f;
	m->m_cKnobArea.left  += 1.0f;
	m->m_cKnobArea.right -= 1.0f;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect ScrollBar::GetKnobFrame( void ) const
{
    Rect	cBounds	= GetBounds();
    Rect	cRect;

    cBounds.left   += 1.0f;
    cBounds.top	   += 1.0f;
    cBounds.right  -= 1.0f;
    cBounds.bottom -= 1.0f;

    cRect = m->m_cKnobArea;

    float vValue = GetValue().AsFloat();
    if ( vValue > m->m_vMax ) {
	vValue = m->m_vMax;
    } else if ( vValue < m->m_vMin ) {
	vValue = m->m_vMin;
    }
    if ( m->m_nOrientation == HORIZONTAL ) {
	float vViewLength = m->m_cKnobArea.Width() + 1.0f;
	float vSize = max( float(SB_MINSIZE), vViewLength * m->m_vProportion );
	float vLen  = vViewLength - vSize;
	float vDelta = m->m_vMax - m->m_vMin;
	
	if ( vDelta > 0.0f ) {
	    cRect.left  = cRect.left + vLen * (vValue - m->m_vMin) / vDelta;
	    cRect.right = cRect.left + vSize - 1.0f;
	}
    } else {
	float vViewLength = m->m_cKnobArea.Height() + 1.0f;
	float vSize  = max( float(SB_MINSIZE), vViewLength * m->m_vProportion );
	float vLen   = vViewLength - vSize;
	float vDelta = m->m_vMax - m->m_vMin;
	
	if ( vDelta > 0.0f ) {
	    cRect.top = cRect.top + vLen * (vValue - m->m_vMin) / vDelta;
	    cRect.bottom = cRect.top + vSize - 1.0f;
	}
    }
    return( cRect & m->m_cKnobArea );
}







