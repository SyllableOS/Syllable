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
#include <time.h>

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

Lcd::Lcd( const Rect & cFrame ):View( cFrame, "" )
{
	m_zName = "Unknown";
	m_nTrack = 1;
	m_nTime = 0;

	m_pcLcdFont = new Font();
	m_pcLcdFont->SetFamilyAndStyle( "Luxi Sans", "Regular" );
	m_pcLcdFont->SetSize( 10 );
	m_pcLcdFont->SetFlags( GetFont()->GetFlags(  ) );

	SetFont( m_pcLcdFont );

	SetBgColor( 150, 200, 255 );
	SetEraseColor( 150, 200, 255 );
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
	char zTrackString[9];
	float vX, vWidth;

	FillRect( cUpdateRect );

	DrawFrame( GetBounds(), FRAME_RECESSED );

	SetFgColor( 0, 0, 0 );
	MovePenTo( 5.0, 20.0 );
	DrawString( m_zName );

	sprintf( zTrackString, "Track %.2i", m_nTrack );
	MovePenTo( 5.0, 40.0 );
	DrawString( zTrackString );

	secs_to_ms( m_nTime, &nM, &nS );
	sprintf( zTimeString, "%.2li:%.2li", nM, nS );

	vWidth = GetStringWidth( zTimeString );
	vWidth += 5;

	vX = GetBounds().Width(  );
	vX -= vWidth;

	MovePenTo( vX, 40.0 );
	DrawString( zTimeString );
	Flush();
}

void Lcd::SetTrackName( std::string zName )
{
	m_zName = zName;
	Paint( GetBounds() );
}

void Lcd::SetTrackNumber( int nTrack )
{
	m_nTrack = nTrack;
	Paint( GetBounds() );
}

void Lcd::UpdateTime( uint64 nTime )
{
	m_nTime = nTime;
	Paint( Rect( 100, 39, 163, 51 ) );
}
