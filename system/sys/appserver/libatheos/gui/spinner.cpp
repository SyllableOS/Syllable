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

#include <stdio.h>
#include <errno.h>
#include <math.h>

#include <gui/spinner.h>
#include <gui/textview.h>
#include <gui/bitmap.h>
#include <gui/desktop.h>
#include <util/message.h>

using namespace os;

#define ARROW_WIDTH  7
#define ARROW_HEIGHT 4

static uint8 g_anArrowUp[] = {
    0xff,0xff,0xff,0x00,0xff,0xff,0xff,
    0xff,0xff,0x00,0x00,0x00,0xff,0xff,
    0xff,0x00,0x00,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static uint8 g_anArrowDown[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0x00,0x00,0x00,0x00,0x00,0xff,
    0xff,0xff,0x00,0x00,0x00,0xff,0xff,
    0xff,0xff,0xff,0x00,0xff,0xff,0xff
};

static Bitmap* g_pcArrows = NULL;
//static Bitmap* g_pcArrowDown = NULL;

enum { ID_TEXT_CHANGED };

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Spinner::Spinner( Rect cFrame, const char* pzName, double vValue, Message* pcMessage, uint32 nResizeMask , uint32 nFlags ) :
	Control( cFrame, pzName, "", pcMessage, nResizeMask, nFlags | WID_FULL_UPDATE_ON_RESIZE ), m_cStrFormat( "%.2f" )
{
    m_vStep  = 0.1f;
    m_vSpeedScale = 1.0f;

    m_vMinValue = -100;
    m_vMaxValue = 100;
  
    m_bUpButtonPushed   = false;
    m_bDownButtonPushed = false;
  
    SetFgColor( 0, 0, 0 );
    SetBgColor( 255, 255, 255 );
    SetEraseColor( 255, 255, 255 );

    m_pcEditBox = new TextView( Rect( 0, 0, 1, 1 ), "text_view", NULL );
    m_pcEditBox->SetEventMask( TextView::EI_FOCUS_LOST | TextView::EI_ENTER_PRESSED );
    
    AddChild( m_pcEditBox );
    FrameSized( Point( 0.0f, 0.0f ) );
    m_pcEditBox->SetMessage( new Message( ID_TEXT_CHANGED ) );
    m_pcEditBox->SetNumeric( true );
    
    if ( g_pcArrows == NULL ) {
	g_pcArrows = new Bitmap( ARROW_WIDTH, ARROW_HEIGHT * 6, CS_CMAP8, Bitmap::SHARE_FRAMEBUFFER );
	uint8* pRaster = g_pcArrows->LockRaster();
	for ( int y = 0 ; y < ARROW_HEIGHT ; ++y ) {
	    for ( int x = 0 ; x < ARROW_WIDTH ; ++x ) {
		*pRaster++ = g_anArrowUp[y*ARROW_WIDTH+x];
	    }
	    pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
	}
	for ( int y = 0 ; y < ARROW_HEIGHT ; ++y ) {
	    for ( int x = 0 ; x < ARROW_WIDTH ; ++x ) {
		*pRaster++ = g_anArrowDown[y*ARROW_WIDTH+x];
	    }
	    pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
	}
	for ( int y = 0 ; y < ARROW_HEIGHT ; ++y ) {
	    for ( int x = 0 ; x < ARROW_WIDTH ; ++x ) {
		*pRaster++ = (g_anArrowUp[y*ARROW_WIDTH+x]==0xff) ? 0xff : 63;
	    }
	    pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
	}
	for ( int y = 0 ; y < ARROW_HEIGHT ; ++y ) {
	    for ( int x = 0 ; x < ARROW_WIDTH ; ++x ) {
		*pRaster++ = (g_anArrowDown[y*ARROW_WIDTH+x]==0xff) ? 0xff : 63;
	    }
	    pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
	}
	for ( int y = 0 ; y < ARROW_HEIGHT ; ++y ) {
	    for ( int x = 0 ; x < ARROW_WIDTH ; ++x ) {
		*pRaster++ = (g_anArrowUp[y*ARROW_WIDTH+x]==0xff) ? 0xff : 14;
	    }
	    pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
	}
	for ( int y = 0 ; y < ARROW_HEIGHT ; ++y ) {
	    for ( int x = 0 ; x < ARROW_WIDTH ; ++x ) {
		*pRaster++ = (g_anArrowDown[y*ARROW_WIDTH+x]==0xff) ? 0xff : 14;
	    }
	    pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
	}
    }
    SetValue( vValue, false );
}

void Spinner::AllAttached()
{
    View::AllAttached();
    m_pcEditBox->SetTarget( this );
}

void Spinner::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_TEXT_CHANGED:
	{
	    double vValue;
	    sscanf( m_pcEditBox->GetBuffer()[0].c_str(), "%lf", &vValue );
		
	    if ( vValue < m_vMinValue ) {
		vValue = m_vMinValue;
	    }
	    if ( vValue > m_vMaxValue ) {
		vValue = m_vMaxValue;
	    }
	    SetValue( vValue );
	    break;
	}
	default:
	    View::HandleMessage( pcMessage );
	    break;
    }
}

void Spinner::PostValueChange( const Variant& cNewValue )
{
    UpdateEditBox();
}

void Spinner::LabelChanged( const std::string& cNewLabel )
{
}

void Spinner::EnableStatusChanged( bool bIsEnabled )
{
}

bool Spinner::Invoked( Message* pcMessage )
{
    return( true );
}

void Spinner::FrameSized( const Point& cDelta )
{
    m_cEditFrame = GetNormalizedBounds();

    float nArrowHeight = m_cEditFrame.Height() + 1.0f;
    float vArrowWidth  = float((int(ceil( nArrowHeight * 0.7f )) + 1) & ~1);

    m_cUpArrowRect   = Rect( floor( m_cEditFrame.right - vArrowWidth ), 0, m_cEditFrame.right - 0, 0 + nArrowHeight / 2 - 1 );
    m_cDownArrowRect = Rect( m_cUpArrowRect.left, m_cUpArrowRect.bottom + 1, m_cUpArrowRect.right, m_cEditFrame.bottom - 0 );

    
    m_cEditFrame.right = m_cUpArrowRect.left - 1.0f;
    m_pcEditBox->SetFrame( m_cEditFrame );
}

void Spinner::SetEnable( bool bEnabled )
{
    if ( bEnabled != m_pcEditBox->IsEnabled() ) {
	Invalidate();
	m_pcEditBox->SetEnable( bEnabled );
	if ( bEnabled == false ) {
	    m_bUpButtonPushed   = false;
	    m_bDownButtonPushed = false;
	}
	Flush();
    }
}

bool Spinner::IsEnabled() const
{
    return( m_pcEditBox->IsEnabled() );
}

void Spinner::SetMinPreferredSize( int nWidthChars )
{
    if ( nWidthChars == 0 ) {
	m_pcEditBox->SetMinPreferredSize( 0, 0 );
    } else {
	m_pcEditBox->SetMinPreferredSize( nWidthChars, 1 );
    }
}

void Spinner::SetMaxPreferredSize( int nWidthChars )
{
    if ( nWidthChars == 0 ) {
	m_pcEditBox->SetMaxPreferredSize( 0, 0 );
    } else {
	m_pcEditBox->SetMaxPreferredSize( nWidthChars, 1 );
    }
}

Point Spinner::GetPreferredSize( bool bLargest ) const
{
    Point cSize = m_pcEditBox->GetPreferredSize( bLargest );

    if ( cSize.y < 20.0f ) {
	cSize.y = 20.0f;
    }
    cSize.x += float((int(ceil( cSize.y * 0.7f )) + 1) & ~1) + 1.0f;
    
    return( cSize );
}

void Spinner::SetFormat( const char* pzStr )
{
    m_cStrFormat = pzStr;
    UpdateEditBox();
}

const std::string& Spinner::GetFormat() const
{
    return( m_cStrFormat );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::SetMinValue( double VValue )
{
    m_vMinValue = VValue;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::SetMaxValue( double VValue )
{
    m_vMaxValue = VValue;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::SetStep( double vStep )
{
    m_vStep = vStep;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::SetScale( double vScale )
{
    m_vSpeedScale = vScale;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

double Spinner::GetMinValue() const
{
    return( m_vMinValue );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

double Spinner::GetMaxValue() const
{
    return( m_vMaxValue );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

double Spinner::GetStep() const
{
    return( m_vStep );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

double Spinner::GetScale() const
{
    return( m_vSpeedScale );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
    if ( m_pcEditBox->IsEnabled() == false ) {
	View::MouseMove( cNewPos, nCode, nButtons, pcData );
	return;
    }
    if ( (m_bUpButtonPushed || m_bDownButtonPushed) && !(m_bUpButtonPushed && m_bDownButtonPushed) ) {
	m_bUpButtonPushed = true;
	m_bDownButtonPushed = true;
	Invalidate();
	Flush();
    }
    if ( m_bUpButtonPushed ) {
	double vNewValue = m_vHitValue - m_vSpeedScale * (cNewPos.y - m_cHitPos.y);

	Point cScreenPos = ConvertToScreen( cNewPos );
	
	if ( int(cScreenPos.y) == 0 ) {
	    IPoint cRes = Desktop().GetResolution();
	    cScreenPos.y = cRes.y - 2.0f;
	    m_vHitValue += m_vSpeedScale * (cRes.y-2.0f);
	    SetMousePos( ConvertFromScreen( cScreenPos ) );
	} else {
	    IPoint cRes = Desktop().GetResolution();
	    if ( int(cScreenPos.y) == cRes.y - 1 ) {
		m_vHitValue -= m_vSpeedScale * (cRes.y-2.0f);
		cScreenPos.y = 1;
		SetMousePos( ConvertFromScreen( cScreenPos ) );
	    }
	}

	if ( vNewValue > m_vMaxValue ) {
	    vNewValue = m_vMaxValue;
	}
	if ( vNewValue < m_vMinValue ) {
	    vNewValue = m_vMinValue;
	}
	SetValue( vNewValue );
	Invalidate();
	Flush();
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::MouseDown( const Point& cPosition, uint32 nButton )
{
    if ( nButton != 1 ||  m_pcEditBox->IsEnabled() == false ) {
	View::MouseDown( cPosition, nButton );
	return;
    }
    
    if ( m_cUpArrowRect.DoIntersect( cPosition ) ) {
	m_bUpButtonPushed = true;
	Invalidate();
	Flush();
    }
    if ( m_cDownArrowRect.DoIntersect( cPosition ) ) {
	m_bDownButtonPushed = true;
	Invalidate();
	Flush();
    }
    MakeFocus( true );
    m_cHitPos = cPosition;
    m_vHitValue = GetValue();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::MouseUp( const Point& cPosition, uint32 nButton, Message* pcData )
{
    if ( nButton != 1 ||  m_pcEditBox->IsEnabled() == false ) {
	View::MouseUp( cPosition, nButton, pcData );
	return;
    }
    
    if ( m_bUpButtonPushed || m_bDownButtonPushed ) {
	if ( m_bUpButtonPushed == false ) {
	    double vNewValue = GetValue().AsDouble() - m_vStep;
	    if ( vNewValue > m_vMaxValue ) {
		vNewValue = m_vMaxValue;
	    }
	    if ( vNewValue < m_vMinValue ) {
		vNewValue = m_vMinValue;
	    }
	    SetValue( vNewValue );
	} else if ( m_bDownButtonPushed == false ) {
	    double vNewValue = GetValue().AsDouble() + m_vStep;
	    if ( vNewValue > m_vMaxValue ) {
		vNewValue = m_vMaxValue;
	    }
	    if ( vNewValue < m_vMinValue ) {
		vNewValue = m_vMinValue;
	    }
	    SetValue( vNewValue );
	}
	SetMousePos( m_cHitPos );
	m_bUpButtonPushed = false;
	m_bDownButtonPushed = false;
	Invalidate();
	Flush();
    }
    MakeFocus( false );
}

void Spinner::WheelMoved( const Point& cDelta )
{
    double vNewValue = GetValue();

    vNewValue -= m_vSpeedScale * cDelta.y;
    
    if ( vNewValue > m_vMaxValue ) {
	vNewValue = m_vMaxValue;
    }
    if ( vNewValue < m_vMinValue ) {
	vNewValue = m_vMinValue;
    }
    SetValue( vNewValue );
}

std::string Spinner::FormatString( double vValue )
{
    char zString[1024];
    sprintf( zString, m_cStrFormat.c_str(), vValue );
    return( std::string( zString ) );
}

void Spinner::UpdateEditBox()
{
    
    m_pcEditBox->Set( FormatString( GetValue().AsDouble() ).c_str(), false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::Paint( const os::Rect& cUpdateRect )
{
    SetEraseColor( get_default_color( COL_NORMAL ) );
    DrawFrame( m_cUpArrowRect, (m_bUpButtonPushed) ? FRAME_RECESSED : FRAME_RAISED );
    DrawFrame( m_cDownArrowRect, (m_bDownButtonPushed) ? FRAME_RECESSED : FRAME_RAISED );

    float nCenterX     = floor( m_cUpArrowRect.left + (m_cUpArrowRect.Width()+1.0f) * 0.5 );
    float nUpCenterY   = floor( m_cUpArrowRect.top + (m_cUpArrowRect.Height()+1.0f) * 0.5 );
    float nDownCenterY = floor( m_cDownArrowRect.top + (m_cDownArrowRect.Height()+1.0f) * 0.5f );

    if ( m_pcEditBox->IsEnabled() == false ) {
	nCenterX += 1.0f;
//	nUpCenterY += 1.0f;
//	nDownCenterY += 1.0f;
    }
    
    SetFgColor( 0, 0, 0 );

    SetDrawingMode( DM_OVER );
    Rect cArrow( 0, 0, ARROW_WIDTH-1, ARROW_HEIGHT-1 );
    Point cBmOffset( 0, (m_pcEditBox->IsEnabled()) ? 0 : ARROW_HEIGHT * 2 );
    
    DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 3, nUpCenterY - 2 ) );
    cBmOffset.y += ARROW_HEIGHT;
    DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 3, nDownCenterY - 2 ) );

    if ( m_pcEditBox->IsEnabled() == false ) {
	Point cBmOffset( 0, ARROW_HEIGHT * 4 );
	
	DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 4, nUpCenterY - 3 ) );
	cBmOffset.y += ARROW_HEIGHT;
	DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 4, nDownCenterY - 3 ) );
    }

    SetDrawingMode( DM_COPY );
}
