/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 The Syllable Team
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

class Spinner::Private {
	public:
	Private() {		
		m_vStep = 0.1f;
		m_vSpeedScale = 1.0f;
	
		m_vMinValue = -100;
		m_vMaxValue = 100;
	
		m_bUpButtonPushed = false;
		m_bDownButtonPushed = false;
		m_cStrFormat = "%.2f";
	}
	
    double	m_vMinValue;
    double	m_vMaxValue;
    double	m_vSpeedScale;
    double	m_vStep;
    double 	m_vHitValue;
    Point 	m_cHitPos;
    String	m_cStrFormat;
    Rect	m_cEditFrame;
    Rect	m_cUpArrowRect;
    Rect	m_cDownArrowRect;
    bool	m_bUpButtonPushed;
    bool	m_bDownButtonPushed;
    TextView*   m_pcEditBox;
};

#define ARROW_WIDTH  7
#define ARROW_HEIGHT 4

static uint8 g_anArrowUp[] = {
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8 g_anArrowDown[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff
};

static Bitmap *g_pcArrows = NULL;

//static Bitmap* g_pcArrowDown = NULL;

enum
{ ID_TEXT_CHANGED };

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Spinner::Spinner( Rect cFrame, const String& cName, double vValue, Message * pcMessage, uint32 nResizeMask, uint32 nFlags ):Control( cFrame, cName, "", pcMessage, nResizeMask, nFlags | WID_FULL_UPDATE_ON_RESIZE )
{
	m = new Private;

	SetFgColor( 0, 0, 0 );
	SetBgColor( 255, 255, 255 );
	SetEraseColor( 255, 255, 255 );

	m->m_pcEditBox = new TextView( Rect( 0, 0, 1, 1 ), "text_view", NULL );
	m->m_pcEditBox->SetEventMask( TextView::EI_FOCUS_LOST | TextView::EI_ENTER_PRESSED );

	AddChild( m->m_pcEditBox );
	FrameSized( Point( 0.0f, 0.0f ) );
	m->m_pcEditBox->SetMessage( new Message( ID_TEXT_CHANGED ) );
	m->m_pcEditBox->SetNumeric( true );

	if( g_pcArrows == NULL )
	{
		g_pcArrows = new Bitmap( ARROW_WIDTH, ARROW_HEIGHT * 6, CS_CMAP8, Bitmap::SHARE_FRAMEBUFFER );
		uint8 *pRaster = g_pcArrows->LockRaster();

		for( int y = 0; y < ARROW_HEIGHT; ++y )
		{
			for( int x = 0; x < ARROW_WIDTH; ++x )
			{
				*pRaster++ = g_anArrowUp[y * ARROW_WIDTH + x];
			}
			pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
		}
		for( int y = 0; y < ARROW_HEIGHT; ++y )
		{
			for( int x = 0; x < ARROW_WIDTH; ++x )
			{
				*pRaster++ = g_anArrowDown[y * ARROW_WIDTH + x];
			}
			pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
		}
		for( int y = 0; y < ARROW_HEIGHT; ++y )
		{
			for( int x = 0; x < ARROW_WIDTH; ++x )
			{
				*pRaster++ = ( g_anArrowUp[y * ARROW_WIDTH + x] == 0xff ) ? 0xff : 63;
			}
			pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
		}
		for( int y = 0; y < ARROW_HEIGHT; ++y )
		{
			for( int x = 0; x < ARROW_WIDTH; ++x )
			{
				*pRaster++ = ( g_anArrowDown[y * ARROW_WIDTH + x] == 0xff ) ? 0xff : 63;
			}
			pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
		}
		for( int y = 0; y < ARROW_HEIGHT; ++y )
		{
			for( int x = 0; x < ARROW_WIDTH; ++x )
			{
				*pRaster++ = ( g_anArrowUp[y * ARROW_WIDTH + x] == 0xff ) ? 0xff : 14;
			}
			pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
		}
		for( int y = 0; y < ARROW_HEIGHT; ++y )
		{
			for( int x = 0; x < ARROW_WIDTH; ++x )
			{
				*pRaster++ = ( g_anArrowDown[y * ARROW_WIDTH + x] == 0xff ) ? 0xff : 14;
			}
			pRaster += g_pcArrows->GetBytesPerRow() - ARROW_WIDTH;
		}
	}
	SetValue( vValue, false );
}

Spinner::~Spinner()
{
	delete m;
}

void Spinner::AllAttached()
{
	View::AllAttached();
	m->m_pcEditBox->SetTarget( this );
	m->m_pcEditBox->SetFont( GetFont() );
}

void Spinner::FontChanged( Font* pcNewFont )
{
	m->m_pcEditBox->SetFont( pcNewFont );
}

void Spinner::SetTabOrder( int nOrder )
{
	m->m_pcEditBox->SetTabOrder( nOrder );
}

int Spinner::GetTabOrder() const
{
	return( m->m_pcEditBox->GetTabOrder() );
}

void Spinner::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case ID_TEXT_CHANGED:
		{
			double vValue;

			sscanf( m->m_pcEditBox->GetBuffer()[0].c_str(  ), "%lf", &vValue );

			if( vValue < m->m_vMinValue )
			{
				vValue = m->m_vMinValue;
			}
			if( vValue > m->m_vMaxValue )
			{
				vValue = m->m_vMaxValue;
			}
			SetValue( vValue );
			break;
		}
	default:
		View::HandleMessage( pcMessage );
		break;
	}
}

void Spinner::PostValueChange( const Variant & cNewValue )
{
	UpdateEditBox();
}

void Spinner::LabelChanged( const String & cNewLabel )
{
}

void Spinner::EnableStatusChanged( bool bIsEnabled )
{
}

bool Spinner::Invoked( Message * pcMessage )
{
	Control::Invoked( pcMessage );
	return ( true );
}

void Spinner::FrameSized( const Point & cDelta )
{
	m->m_cEditFrame = GetNormalizedBounds();
	
	m->m_cEditFrame.Resize( 1, 1, -1, -1 );

	float nArrowHeight = m->m_cEditFrame.Height() + 1.0f;
	float vArrowWidth = float ( ( int ( ceil( nArrowHeight * 0.7f ) ) + 1 )&~1 );

	m->m_cUpArrowRect = Rect( floor( m->m_cEditFrame.right - vArrowWidth ), 0, m->m_cEditFrame.right - 0, 0 + nArrowHeight / 2 - 1 );
	m->m_cDownArrowRect = Rect( m->m_cUpArrowRect.left, m->m_cUpArrowRect.bottom + 1, m->m_cUpArrowRect.right, m->m_cEditFrame.bottom - 0 );


	m->m_cEditFrame.right = m->m_cUpArrowRect.left - 1.0f;
	m->m_pcEditBox->SetFrame( m->m_cEditFrame );
}

void Spinner::SetEnable( bool bEnabled )
{
	if( bEnabled != m->m_pcEditBox->IsEnabled() )
	{
		Invalidate();
		m->m_pcEditBox->SetEnable( bEnabled );
		if( bEnabled == false )
		{
			m->m_bUpButtonPushed = false;
			m->m_bDownButtonPushed = false;
		}
		Flush();
	}
}

bool Spinner::IsEnabled() const
{
	return ( m->m_pcEditBox->IsEnabled() );
}

void Spinner::SetMinPreferredSize( int nWidthChars )
{
	if( nWidthChars == 0 )
	{
		m->m_pcEditBox->SetMinPreferredSize( 0, 0 );
	}
	else
	{
		m->m_pcEditBox->SetMinPreferredSize( nWidthChars, 1 );
	}
}

void Spinner::SetMaxPreferredSize( int nWidthChars )
{
	if( nWidthChars == 0 )
	{
		m->m_pcEditBox->SetMaxPreferredSize( 0, 0 );
	}
	else
	{
		m->m_pcEditBox->SetMaxPreferredSize( nWidthChars, 1 );
	}
}

Point Spinner::GetPreferredSize( bool bLargest ) const
{
	Point cSize = m->m_pcEditBox->GetPreferredSize( bLargest );

	if( cSize.y < 20.0f )
	{
		cSize.y = 20.0f;
	}
	cSize.x += float ( ( int ( ceil( cSize.y * 0.7f ) ) + 1 )&~1 ) + 1.0f;

	return ( cSize + Point( 1, 1 ) );
}

void Spinner::SetFormat( const char *pzStr )
{
	m->m_cStrFormat = pzStr;
	UpdateEditBox();
}

const String & Spinner::GetFormat() const
{
	return ( m->m_cStrFormat );
}

//----------------------------------------------------------------------------
// NAME: Spinner::Decrement()
// DESC: Decreases the current value by the step amount, down to the Spinner's minimum value.
// NOTE: Equivalent to clicking the downarrow.
// SEE ALSO: Spinner::Increment()
//----------------------------------------------------------------------------

void Spinner::Decrement()
{
	double vNewValue = GetValue().AsDouble(  ) - m->m_vStep;

	if( vNewValue > m->m_vMaxValue )
	{
		vNewValue = m->m_vMaxValue;
	}
	if( vNewValue < m->m_vMinValue )
	{
		vNewValue = m->m_vMinValue;
	}
	SetValue( vNewValue );
}

//----------------------------------------------------------------------------
// NAME: Spinner::Increment()
// DESC: Increases the current value by the step amount, up to the Spinner's maximum value.
// NOTE: Equivalent to clicking the uparrow.
// SEE ALSO: Spinner::Decrement()
//----------------------------------------------------------------------------

void Spinner::Increment()
{
	double vNewValue = GetValue().AsDouble(  ) + m->m_vStep;

	if( vNewValue > m->m_vMaxValue )
	{
		vNewValue = m->m_vMaxValue;
	}
	if( vNewValue < m->m_vMinValue )
	{
		vNewValue = m->m_vMinValue;
	}
	SetValue( vNewValue );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::SetMinValue( double VValue )
{
	m->m_vMinValue = VValue;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::SetMaxValue( double VValue )
{
	m->m_vMaxValue = VValue;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::SetStep( double vStep )
{
	m->m_vStep = vStep;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::SetScale( double vScale )
{
	m->m_vSpeedScale = vScale;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

double Spinner::GetMinValue() const
{
	return ( m->m_vMinValue );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

double Spinner::GetMaxValue() const
{
	return ( m->m_vMaxValue );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

double Spinner::GetStep() const
{
	return ( m->m_vStep );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

double Spinner::GetScale() const
{
	return ( m->m_vSpeedScale );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( IsEnabled() == false )
	{
		View::KeyDown( pzString, pzRawString, nQualifiers );
		return;
	}
	if( ( pzString[1] == '\0' && ( pzString[0] == VK_ENTER || pzString[0] == ' ' ) ) ||
		( GetShortcut() == ShortcutKey( pzRawString, nQualifiers ) ) )
	{
		MakeFocus();
	}
	else
	{
		Control::KeyDown( pzString, pzRawString, nQualifiers );
	}
}


/* Although this method doesn't do anything, it needs to exist to preserve binary compatibility.
   It may be removed in the next libsyllable version.
*/
void Spinner::KeyUp( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
	Control::KeyUp( pzString, pzRawString, nQualifiers );
}

void Spinner::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	if( m->m_pcEditBox->IsEnabled() == false )
	{
		View::MouseMove( cNewPos, nCode, nButtons, pcData );
		return;
	}
	if( ( m->m_bUpButtonPushed || m->m_bDownButtonPushed ) && !( m->m_bUpButtonPushed && m->m_bDownButtonPushed ) )
	{
		m->m_bUpButtonPushed = true;
		m->m_bDownButtonPushed = true;
		Invalidate();
		Flush();
	}
	if( m->m_bUpButtonPushed )
	{
		double vNewValue = m->m_vHitValue - m->m_vSpeedScale * ( cNewPos.y - m->m_cHitPos.y );

		Point cScreenPos = ConvertToScreen( cNewPos );

		if( int ( cScreenPos.y ) == 0 )
		{
			IPoint cRes = Desktop().GetResolution(  );

			cScreenPos.y = cRes.y - 2.0f;
			m->m_vHitValue += m->m_vSpeedScale * ( cRes.y - 2.0f );
			SetMousePos( ConvertFromScreen( cScreenPos ) );
		}
		else
		{
			IPoint cRes = Desktop().GetResolution(  );

			if( int ( cScreenPos.y ) == cRes.y - 1 )
			{
				m->m_vHitValue -= m->m_vSpeedScale * ( cRes.y - 2.0f );
				cScreenPos.y = 1;
				SetMousePos( ConvertFromScreen( cScreenPos ) );
			}
		}

		if( vNewValue > m->m_vMaxValue )
		{
			vNewValue = m->m_vMaxValue;
		}
		if( vNewValue < m->m_vMinValue )
		{
			vNewValue = m->m_vMinValue;
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

void Spinner::MouseDown( const Point & cPosition, uint32 nButton )
{
	if( nButton != 1 || m->m_pcEditBox->IsEnabled() == false )
	{
		View::MouseDown( cPosition, nButton );
		return;
	}

	if( m->m_cUpArrowRect.DoIntersect( cPosition ) )
	{
		m->m_bUpButtonPushed = true;
		Invalidate();
		Flush();
	}
	if( m->m_cDownArrowRect.DoIntersect( cPosition ) )
	{
		m->m_bDownButtonPushed = true;
		Invalidate();
		Flush();
	}
	MakeFocus( true );
	m->m_cHitPos = cPosition;
	m->m_vHitValue = GetValue();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::MouseUp( const Point & cPosition, uint32 nButton, Message * pcData )
{
	if( nButton != 1 || m->m_pcEditBox->IsEnabled() == false )
	{
		View::MouseUp( cPosition, nButton, pcData );
		return;
	}

	if( m->m_bUpButtonPushed || m->m_bDownButtonPushed )
	{
		if( m->m_bUpButtonPushed == false )
		{
			Decrement();
		}
		else if( m->m_bDownButtonPushed == false )
		{
			Increment();
		}
//		SetMousePos( m->m_cHitPos );
		m->m_bUpButtonPushed = false;
		m->m_bDownButtonPushed = false;
		Invalidate();
		Flush();
	}
	MakeFocus( false );
}

void Spinner::WheelMoved( const Point & cDelta )
{
	double vNewValue = GetValue();

	vNewValue -= m->m_vSpeedScale * cDelta.y;

	if( vNewValue > m->m_vMaxValue )
	{
		vNewValue = m->m_vMaxValue;
	}
	if( vNewValue < m->m_vMinValue )
	{
		vNewValue = m->m_vMinValue;
	}
	SetValue( vNewValue );
}

String Spinner::FormatString( double vValue )
{
	char zString[1024];

	snprintf( zString, 1024, m->m_cStrFormat.c_str(), vValue );
	return ( String( zString ) );
}

void Spinner::UpdateEditBox()
{
	m->m_pcEditBox->Set( FormatString( GetValue().AsDouble() ).c_str(), false );
}

void Spinner::Activated( bool bIsActive )
{
	Invalidate();
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Spinner::Paint( const os::Rect & cUpdateRect )
{
	SetEraseColor( get_default_color( COL_NORMAL ) );
	DrawFrame( m->m_cUpArrowRect, ( m->m_bUpButtonPushed ) ? FRAME_RECESSED : FRAME_RAISED );
	DrawFrame( m->m_cDownArrowRect, ( m->m_bDownButtonPushed ) ? FRAME_RECESSED : FRAME_RAISED );

	float nCenterX = floor( m->m_cUpArrowRect.left + ( m->m_cUpArrowRect.Width() + 1.0f ) * 0.5 );
	float nUpCenterY = floor( m->m_cUpArrowRect.top + ( m->m_cUpArrowRect.Height() + 1.0f ) * 0.5 );
	float nDownCenterY = floor( m->m_cDownArrowRect.top + ( m->m_cDownArrowRect.Height() + 1.0f ) * 0.5f );

	Rect cBounds = GetBounds();

	if( IsEnabled() && HasFocus() )
	{
		SetFgColor( get_default_color( COL_FOCUS ) );
	} else {
		SetFgColor( get_default_color( COL_NORMAL ) );
	}

	DrawLine( Point( cBounds.left, cBounds.top ), Point( cBounds.right, cBounds.top ) );
	DrawLine( Point( cBounds.right, cBounds.bottom ) );
	DrawLine( Point( cBounds.left, cBounds.bottom ) );
	DrawLine( Point( cBounds.left, cBounds.top ) );

	if( m->m_pcEditBox->IsEnabled() == false )
	{
		nCenterX += 1.0f;
//      nUpCenterY += 1.0f;
//      nDownCenterY += 1.0f;
	}

	SetFgColor( 0, 0, 0 );

	SetDrawingMode( DM_OVER );
	Rect cArrow( 0, 0, ARROW_WIDTH - 1, ARROW_HEIGHT - 1 );
	Point cBmOffset( 0, ( m->m_pcEditBox->IsEnabled() )? 0 : ARROW_HEIGHT * 2 );

	DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 3, nUpCenterY - 2 ) );
	cBmOffset.y += ARROW_HEIGHT;
	DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 3, nDownCenterY - 2 ) );

	if( m->m_pcEditBox->IsEnabled() == false )
	{
		Point cBmOffset( 0, ARROW_HEIGHT * 4 );

		DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 4, nUpCenterY - 3 ) );
		cBmOffset.y += ARROW_HEIGHT;
		DrawBitmap( g_pcArrows, cArrow + cBmOffset, cArrow + Point( nCenterX - 4, nDownCenterY - 3 ) );
	}

	SetDrawingMode( DM_COPY );
}


void Spinner::__SP_reserved1__()
{
}

void Spinner::__SP_reserved2__()
{
}

void Spinner::__SP_reserved3__()
{
}

void Spinner::__SP_reserved4__()
{
}

void Spinner::__SP_reserved5__()
{
}

