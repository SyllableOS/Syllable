
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
 * 02-07-23: Added code for disabling (Paint, MouseDown).
 *           MouseUp, MouseDown passes the event on the super class now (if it's
 *           not used).
 *           Does only react on LMB clicks now.
 *           Prepared for keyboard control (binary compatibilty will break when
 *           its implemented!).
 *
 */

#include <atheos/types.h>

#include <gui/checkbox.h>
#include <gui/font.h>

#include <macros.h>

using namespace os;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

CheckBox::CheckBox( Rect cFrame, const char *pzName, const char *pzLabel, Message * pcMessage, uint32 nResizeMask, uint32 nFlags ):Control( cFrame, pzName, pzLabel, pcMessage, nResizeMask, nFlags )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

CheckBox::~CheckBox()
{
}

void CheckBox::PostValueChange( const Variant & cNewValue )
{
	Invalidate();
	Flush();
}

void CheckBox::LabelChanged( const std::string & cNewLabel )
{
	Invalidate();
	Flush();
}

void CheckBox::EnableStatusChanged( bool bIsEnabled )
{
	Invalidate();
	Flush();
}

bool CheckBox::Invoked( Message * pcMessage )
{
	Control::Invoked( pcMessage );
	return ( true );
}

Point CheckBox::GetPreferredSize( bool bLargest ) const
{
	std::string cLabel = GetLabel();

	Point cSize( 13, 13 );

	if( cLabel.empty() == false )
	{
		font_height sHeight;

		GetFontHeight( &sHeight );
		if( sHeight.ascender + sHeight.descender > 13 )
		{
			cSize.y = sHeight.ascender + sHeight.descender;
		}
		cSize.x += 3 + GetStringWidth( cLabel );
	}
	return ( cSize );
}

// This method will provide keyboard control (ENTER/SPACE will toggle the state of
// the checkmark.
// Adding this will however mean that a virtual method has to be added to the class
// and this will break binary compatibilty.

/*

void CheckBox::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    if ( IsEnabled() == false ) {
	View::KeyDown( pzString, pzRawString, nQualifiers );
	return;
    }
    if ( pzString[1] == '\0' && (pzString[0] == VK_ENTER || pzString[0] == ' ')  ) {
	SetValue( !GetValue().AsBool() );
    } else {
	Control::KeyDown( pzString, pzRawString, nQualifiers );
    }
}

*/

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void CheckBox::MouseDown( const Point & cPosition, uint32 nButtons )
{

	if( nButtons != 1 || IsEnabled() == false )
	{
		View::MouseDown( cPosition, nButtons );
		return;
	}

	MakeFocus( true );
	if( GetBounds().DoIntersect( cPosition ) )
	{
		SetValue( !GetValue().AsBool(  ) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void CheckBox::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	View::MouseUp( cPosition, nButtons, pcData );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void CheckBox::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetBounds();

	std::string cLabel = GetLabel();

	SetFgColor( get_default_color( COL_NORMAL ) );
	FillRect( cUpdateRect );

	if( cLabel.empty() == false )
	{
		font_height sHeight;
		float vStrWidth;

		vStrWidth = GetStringWidth( cLabel );
		GetFontHeight( &sHeight );

		float x = 16.0f;
		float y = ( cBounds.Height() + 1.0f ) * 0.5f - ( sHeight.ascender + sHeight.descender + 1.0f ) * 0.5f + sHeight.ascender;

		MovePenTo( x, y );

		if( IsEnabled() )
		{
			SetFgColor( 0, 0, 0 );
		}
		else
		{
			SetFgColor( 255, 255, 255 );
		}

		SetBgColor( get_default_color( COL_NORMAL ) );
		DrawString( cLabel );

		if( IsEnabled() == false )
		{
			SetFgColor( 100, 100, 100 );
			MovePenTo( x - 1.0f, y - 1.0f );
			SetDrawingMode( DM_OVER );
			DrawString( GetLabel() );
			SetDrawingMode( DM_COPY );
		}

		// For this to work, we need the FocusChanged callback => breaking binary compatibilty

/*	if ( IsEnabled() && HasFocus() ) {
	    DrawLine( Point( x, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ),
		      Point( x + vStrWidth, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ) );
	}*/
	}

	SetEraseColor( get_default_color( COL_NORMAL ) );

	Rect cButFrame( 0, 0, 13, 13 );

	float nDelta = ( cBounds.Height() - cButFrame.Height(  ) ) / 2;

	cButFrame.top += nDelta;
	cButFrame.bottom += nDelta;

	DrawFrame( cButFrame, FRAME_RAISED | FRAME_TRANSPARENT );

	if( GetValue().AsBool(  ) != false )
	{
		Point cPnt1;
		Point cPnt2;

		if( IsEnabled() )
		{
			SetFgColor( 0, 0, 0 );
		}
		else
		{
			SetFgColor( 255, 255, 255 );
		}

		// TODO: Replace this hard-coded image with something that can be changed!

		cPnt1 = Point( cButFrame.left + 3, cButFrame.top + 3 );
		cPnt2 = Point( cButFrame.right - 3, cButFrame.bottom - 3 );

		DrawLine( cPnt1, cPnt2 );
		DrawLine( cPnt1 + Point( 1, 0 ), cPnt2 + Point( 1, 0 ) );

		cPnt1 = Point( cButFrame.left + 3, cButFrame.bottom - 3 );
		cPnt2 = Point( cButFrame.right - 3, cButFrame.top + 3 );

		DrawLine( cPnt1, cPnt2 );
		DrawLine( cPnt1 + Point( 1, 0 ), cPnt2 + Point( 1, 0 ) );

		if( IsEnabled() == false )
		{
			SetFgColor( 100, 100, 100 );
			SetDrawingMode( DM_OVER );

			cPnt1 = Point( cButFrame.left + 2, cButFrame.top + 2 );
			cPnt2 = Point( cButFrame.right - 4, cButFrame.bottom - 4 );

			DrawLine( cPnt1, cPnt2 );
			DrawLine( cPnt1 + Point( 1, 0 ), cPnt2 + Point( 1, 0 ) );

			cPnt1 = Point( cButFrame.left + 2, cButFrame.bottom - 4 );
			cPnt2 = Point( cButFrame.right - 4, cButFrame.top + 2 );

			DrawLine( cPnt1, cPnt2 );
			DrawLine( cPnt1 + Point( 1, 0 ), cPnt2 + Point( 1, 0 ) );

			SetDrawingMode( DM_COPY );
		}

	}
}
