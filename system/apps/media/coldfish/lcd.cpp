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

	SetDrawingMode( DM_COPY );
}

Lcd::~Lcd()
{
	m_pcLcdFont->Release();
}

void Lcd::Paint( const Rect & cUpdateRect )
{
	uint32 nM, nS;
	char zTimeString[6];
	char zTrackString[13];
	float vX, vWidth;
	
	SetDrawingMode( DM_COPY );
	SetFgColor( os::get_default_color( os::COL_NORMAL ) );

	FillRect( cUpdateRect );
	
	/* Draw background and its shadow */
	SetBgColor( 255, 255, 255 );
	SetFgColor( 255, 255, 255 );
	
	os::Rect cMiddle( GetBounds() );
	cMiddle.left += 5;
	cMiddle.top += 5;
	cMiddle.right -= 7;
	cMiddle.bottom -= 15;
	
	for( int i = 6; i >= 0; i-- )
	{
		SetFgColor( i * 35, i * 35, i * 35 );
		
		
		DrawLine( os::Point( cMiddle.left + i, cMiddle.bottom + i ), os::Point( cMiddle.right + i, cMiddle.bottom + i ) );
		DrawLine( os::Point( cMiddle.right + i, cMiddle.top + i ), os::Point( cMiddle.right + i, cMiddle.bottom + i ) );
	}
	
	SetFgColor( 255, 255, 255 );
	FillRect( cMiddle );
	
	
	/* Draw text */
	SetFgColor( 0, 0, 0 );
	MovePenTo( 10.0, 20.0 );
	
	sprintf( zTrackString, " (%s %.2i)", MSG_LCD_TRACK.c_str(), m_nTrack );
	
	os::String zTitle = os::String( m_zName ) + os::String( zTrackString );
	
	DrawString( zTitle );

	secs_to_ms( m_nTime, &nM, &nS );
	sprintf( zTimeString, "%.2li:%.2li", nM, nS );

	vWidth = GetStringWidth( zTimeString );
	vWidth += 5;

	vX = GetBounds().Width(  ) - 5;
	vX -= vWidth;
	MovePenTo( vX, 20.0 );
	DrawString( zTimeString );
	
	
	/* Draw slider frame */
	os::Rect cSliderRect( 10, GetBounds().Height() - 35, GetBounds().Width() - 10, GetBounds().Height() - 20 );
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
	os::Rect cSliderRect( 11, GetBounds().Height() - 36, GetBounds().Width() - 11, GetBounds().Height() - 21 );
	
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
	Paint( Rect( GetBounds().right - 100, 19, GetBounds().right, 31 ) );
	Flush();
}

void Lcd::PostValueChange( const Variant & cNewValue )
{
	os::Rect cSliderRect( 10, GetBounds().Height() - 35, GetBounds().Width() - 10, GetBounds().Height() - 20 );
	Paint( cSliderRect );
	Flush();
}

void Lcd::EnableStatusChanged( bool bIsEnabled )
{
	Invalidate();
	Flush();
}
