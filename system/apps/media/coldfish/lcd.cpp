//  Coldfish 1.0 -:-  (C)opyright 2003 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "lcd.h"
#include "resources/coldfish.h"
#include <time.h>
#include <iostream>

using namespace os;

static inline void secs_to_ms( uint64 nTime, uint32 *nM, uint32 *nS )
{
	if ( nTime > 59 )
	{
		*nM = ( nTime / 60 );
		*nS = nTime - ( *nM * 60 );
	}
	else
	{
		*nM = 0;
		*nS = ( uint32 )nTime;
	}
	
}

Lcd::Lcd( const Rect & cFrame, Message* pcMessage, uint32 nResizeMask ):Control( cFrame, "", "", pcMessage, nResizeMask,
																				WID_WILL_DRAW | WID_CLEAR_BACKGROUND | 
																				WID_FULL_UPDATE_ON_RESIZE )
{
	m_zName = "Unknown";
	m_nTrack = 1;
	m_nTime = 0;
	SetValue( 0, false );

	m_pcLcdFont = new Font();
	m_pcLcdFont->SetFamilyAndStyle( "Luxi Sans", "Regular" );
	m_pcLcdFont->SetSize( 10 );
	m_pcLcdFont->SetFlags( GetFont()->GetFlags(  ) );

	SetFont( m_pcLcdFont );
	m_pcLcdFont->Release();

	SetDrawingMode( DM_COPY );
}

Lcd::~Lcd()
{
}

void Lcd::Paint( const Rect & cUpdateRect )
{
	uint32 nM, nS;
	char zTimeString[6];
	char zTrackString[13];
	float vX, vWidth;
	
	/* Draw background */
	SetDrawingMode( DM_COPY );
	SetFgColor( 230, 230, 230 );

	os::Rect cFillRect = GetBounds();
	cFillRect.top += 4;
	cFillRect.bottom -= 4;
	FillRect( cFillRect & cUpdateRect );
	
	cFillRect.top = 0;
	cFillRect.bottom = 3;
	
	if( cUpdateRect.DoIntersect( cFillRect ) )
	{
		SetFgColor( os::get_default_color( os::COL_NORMAL ) );
		FillRect( cFillRect );
		SetFgColor( 230, 230, 230 );
		DrawLine( os::Point( 4, 0 ), os::Point( GetBounds().right - 4, 0 ) );
		DrawLine( os::Point( 2, 1 ), os::Point( GetBounds().right - 2, 1 ) );
		DrawLine( os::Point( 1, 2 ), os::Point( GetBounds().right - 1, 2 ) );
		DrawLine( os::Point( 1, 3 ), os::Point( GetBounds().right - 1, 3 ) );
	}
	
	cFillRect.top = GetBounds().bottom - 3;
	cFillRect.bottom = GetBounds().bottom;
	
	if( cUpdateRect.DoIntersect( cFillRect ) )
	{
		SetFgColor( os::get_default_color( os::COL_NORMAL ) );
		FillRect( cFillRect );
		SetFgColor( 230, 230, 230 );
		DrawLine( os::Point( 4, cFillRect.bottom ), os::Point( GetBounds().right - 4, cFillRect.bottom ) );
		DrawLine( os::Point( 2, cFillRect.bottom - 1 ), os::Point( GetBounds().right - 2, cFillRect.bottom - 1 ) );
		DrawLine( os::Point( 1, cFillRect.bottom - 2 ), os::Point( GetBounds().right - 1, cFillRect.bottom - 2 ) );
		DrawLine( os::Point( 1, cFillRect.bottom - 3 ), os::Point( GetBounds().right - 1, cFillRect.bottom - 3 ) );
	}
	
	
	/* Draw text */
	SetFgColor( 0, 0, 0 );
	SetBgColor( 230, 230, 230 );
	

	secs_to_ms( m_nTime, &nM, &nS );
	sprintf( zTimeString, "%.2li:%.2li", nM, nS );

	vWidth = GetStringWidth( zTimeString );
	vWidth += 5;

	vX = GetBounds().Width(  ) - 5;
	vX -= vWidth;
	MovePenTo( vX, 20.0 );
	DrawString( zTimeString );
	
	MovePenTo( 10.0, 20.0 );
		
	sprintf( zTrackString, " (%s %.2i)", MSG_LCD_TRACK.c_str(), m_nTrack );
	os::String zTitle = os::String( m_zName ) + os::String( zTrackString );
	
	if( GetStringWidth( zTitle ) > GetBounds().Width() - vWidth - 15 )
	{
		float vAvailable = GetBounds().Width() - vWidth - 10;
		vAvailable -= GetStringWidth( zTrackString );
		int nChars = (int)( vAvailable / GetStringWidth( "x" ) );
		nChars -= 3;
		zTitle = os::String( m_zName ).substr( 0, nChars ) + "..." + os::String( zTrackString );
	}
	
		
	DrawString( zTitle );
	
	
	/* Draw slider frame */
	SetFgColor( 180, 180, 180 );
	os::Rect cSliderRect( 10, GetBounds().Height() - 25, GetBounds().Width() - 10, GetBounds().Height() - 10 );
	DrawLine( os::Point( cSliderRect.left, cSliderRect.top ), os::Point( cSliderRect.right, cSliderRect.top ) );
	DrawLine( os::Point( cSliderRect.right, cSliderRect.top ), os::Point( cSliderRect.right, cSliderRect.bottom ) );
	DrawLine( os::Point( cSliderRect.right, cSliderRect.bottom ), os::Point( cSliderRect.left, cSliderRect.bottom ) );
	DrawLine( os::Point( cSliderRect.left, cSliderRect.bottom ), os::Point( cSliderRect.left, cSliderRect.top ) );

	/* Draw progressbar */
	int32 nValue = GetValue().AsInt32();
	if( nValue < 0 ) nValue = 0;
	if( nValue > 999 ) nValue = 999;
	
	os::Rect cLeft( cSliderRect.left + 1, cSliderRect.top + 1, cSliderRect.left + 1 
					+ ( cSliderRect.Width() - 1 ) * nValue / 1000, cSliderRect.bottom - 1 );
	SetFgColor( 186, 199, 227 );
	FillRect( cLeft );
	
	os::Rect cRight( cSliderRect.left + 1 + ( cSliderRect.Width() - 1 ) * nValue / 1000 + 1, 
					cSliderRect.top + 1, cSliderRect.right - 1, cSliderRect.bottom - 1 );
	SetFgColor( 211, 211, 211 );
	FillRect( cRight );
	
	/* Draw knob */
	if( IsEnabled() )
		SetFgColor( 0, 0, 0 );
	else
		SetFgColor( 120, 120, 120 );
	float vKnobPos = cSliderRect.left + 3 + ( cSliderRect.Width() - 15 ) / 1000 * nValue;
	os::Rect cKnob( vKnobPos, cSliderRect.top + 3, vKnobPos + 9, cSliderRect.bottom - 3 );
	FillRect( cKnob );
}

void Lcd::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
	os::Rect cSliderRect( 11, GetBounds().Height() - 26, GetBounds().Width() - 11, GetBounds().Height() - 11 );
	
	if( cSliderRect.DoIntersect( cPosition ) && IsEnabled() )
	{
		//std::cout<< ( cPosition.x - 10 ) * 1000 / ( cSliderRect.Width() + 1 )<<std::endl;
		SetValue( ( cPosition.x - 10 ) * 1000 / ( cSliderRect.Width() + 1 ) );
	}
	os::Control::MouseUp( cPosition, nButtons, pcData );
}

void Lcd::SetTrackName( std::string zName )
{
	m_zName = zName;
	Paint( GetBounds() );
	Flush();
}

void Lcd::SetTrackNumber( int nTrack )
{
	m_nTrack = nTrack;
	Paint( GetBounds() );
	Flush();
}

void Lcd::UpdateTime( uint64 nTime )
{
	m_nTime = nTime;
	Paint( Rect( GetBounds().right - 100, 0, GetBounds().right, GetBounds().bottom - 25 ) );
	Flush();
}

void Lcd::PostValueChange( const Variant & cNewValue )
{
	os::Rect cSliderRect( 10, GetBounds().Height() - 25, GetBounds().Width() - 10, GetBounds().Height() - 10 );
	Paint( cSliderRect );
	Flush();
}

void Lcd::EnableStatusChanged( bool bIsEnabled )
{
	Invalidate();
	Flush();
}
