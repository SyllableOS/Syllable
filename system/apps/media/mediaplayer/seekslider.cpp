/*  Syllable Media Player
 *  Copyright (C) 2003 - 2004 Arno Klenke
 *  Copyright (C) 2003 Kristian Van Der Vliet
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

#include "seekslider.h"
#include <time.h>
#include <iostream>

using namespace os;


SeekSlider::SeekSlider( const Rect & cFrame, String zName, Message* pcMessage, uint32 nResizeMask ):Control( cFrame, zName, zName, pcMessage, nResizeMask,
																				WID_WILL_DRAW | WID_CLEAR_BACKGROUND | 
																				WID_FULL_UPDATE_ON_RESIZE )
{
	SetValue( 0, false );

	SetDrawingMode( DM_COPY );
}

SeekSlider::~SeekSlider()
{
}

void SeekSlider::Paint( const Rect & cUpdateRect )
{
	SetDrawingMode( DM_COPY );
	SetFgColor( os::get_default_color( os::COL_NORMAL ) );

	FillRect( cUpdateRect );
	
	/* Draw background and its shadow */
	#if 0
	SetBgColor( 255, 255, 255 );
	SetFgColor( 255, 255, 255 );
	
	os::Rect cMiddle( GetBounds() );
	/* Leave room for the shadow */
	cMiddle.right -= 4;
	cMiddle.bottom -= 4;
	
	
	for( int i = 4; i >= 0; i-- )
	{
		SetFgColor( i * 50, i * 50, i * 50 );
		
		
		DrawLine( os::Point( cMiddle.left + i, cMiddle.bottom + i ), os::Point( cMiddle.right + i, cMiddle.bottom + i ) );
		DrawLine( os::Point( cMiddle.right + i, cMiddle.top + i ), os::Point( cMiddle.right + i, cMiddle.bottom + i ) );
	}
	
	SetFgColor( 255, 255, 255 );
	FillRect( cMiddle );
	
	#endif
	/* Draw slider frame */
	SetFgColor( 180, 180, 180 );
	float vYPos = ( GetBounds().Height() - 15 ) / 2 - 1;
	os::Rect cSliderRect( 3, vYPos, GetBounds().Width() - 10, vYPos + 15 );
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

void SeekSlider::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
	float vYPos = ( GetBounds().Height() - 15 ) / 2 - 3;
	os::Rect cSliderRect( 10, vYPos, GetBounds().Width() - 10, vYPos + 15 );
	
	if( cSliderRect.DoIntersect( cPosition ) && IsEnabled() )
	{
		//std::cout<< ( cPosition.x - 10 ) * 1000 / ( cSliderRect.Width() + 1 )<<std::endl;
		SetValue( ( cPosition.x - 10 ) * 1000 / ( cSliderRect.Width() + 1 ) );
	}
	os::Control::MouseUp( cPosition, nButtons, pcData );
}


void SeekSlider::PostValueChange( const Variant & cNewValue )
{
	float vYPos = ( GetBounds().Height() - 15 ) / 2 - 3;
	os::Rect cSliderRect( 10, vYPos, GetBounds().Width() - 10, vYPos + 15 );
	
	Paint( cSliderRect );
	Flush();
}

void SeekSlider::EnableStatusChanged( bool bIsEnabled )
{
	Invalidate();
	Flush();
}
