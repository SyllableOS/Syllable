/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#include <atheos/types.h>

#include <gui/checkbox.h>
#include <util/shortcutkey.h>
#include <gui/font.h>
#include <gui/window.h>

#include <macros.h>

using namespace os;

class CheckBox::Private {
	public:
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

CheckBox::CheckBox( Rect cFrame, const String& cName, const String& cLabel, Message * pcMessage, uint32 nResizeMask, uint32 nFlags ):Control( cFrame, cName, cLabel, pcMessage, nResizeMask, nFlags )
{
//	m = new Private;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

CheckBox::~CheckBox()
{
//	delete m;
}

void CheckBox::PostValueChange( const Variant & cNewValue )
{
	Invalidate();
	Flush();
}

void CheckBox::LabelChanged( const String & cNewLabel )
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

void CheckBox::Activated( bool bIsActive )
{
	Invalidate();
	Flush();
}

Point CheckBox::GetPreferredSize( bool bLargest ) const
{
	if( bLargest )
	{
		return Point( COORD_MAX, COORD_MAX );
	} else {
		Point cSize( 14, 14 );
		Point cStringExt = GetTextExtent( GetLabel() );
		if( cStringExt.y > 13 ) {
			cSize.y = cStringExt.y;
		}

		cSize.x += 3 + cStringExt.x;

		return ( cSize );
	}
}

void CheckBox::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( IsEnabled() == false )
	{
		View::KeyDown( pzString, pzRawString, nQualifiers );
		return;
	}
	if( ( pzString[1] == '\0' && ( pzString[0] == VK_ENTER || pzString[0] == ' ' ) ) ||
		( GetShortcut() == ShortcutKey( pzRawString, nQualifiers ) ) )
	{
		SetValue( !GetValue().AsBool() );
		MakeFocus();
	}
	else
	{
		Control::KeyDown( pzString, pzRawString, nQualifiers );
	}
}

void CheckBox::KeyUp( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	View::KeyUp( pzString, pzRawString, nQualifiers );
}

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
		Rect cLabelRect( cBounds );

		cLabelRect.left += 16;

		if( IsEnabled() )
		{
			SetFgColor( 0, 0, 0 );
		}
		else
		{
			SetFgColor( 255, 255, 255 );
		}

		SetBgColor( get_default_color( COL_NORMAL ) );
		DrawText( cLabelRect, cLabel, DTF_ALIGN_MIDDLE | DTF_ALIGN_LEFT | DTF_UNDERLINES );

		if( IsEnabled() == false )
		{
			SetFgColor( 100, 100, 100 );
			SetDrawingMode( DM_OVER );
			cLabelRect.MoveTo( -1, -1 );
			DrawText( cLabelRect, cLabel, DTF_ALIGN_MIDDLE | DTF_ALIGN_LEFT | DTF_UNDERLINES );
			SetDrawingMode( DM_COPY );
		}
	}

	SetEraseColor( get_default_color( COL_NORMAL ) );

	Rect cButFrame( 0, 0, 13, 13 );

	float nDelta = ( cBounds.Height() - cButFrame.Height(  ) ) / 2;
	
	cButFrame.top += nDelta;
	cButFrame.bottom += nDelta;

	if( IsEnabled() && HasFocus() )
	{
		SetFgColor( get_default_color( COL_FOCUS ) );
		DrawLine( Point( cButFrame.left, cButFrame.top ), Point( cButFrame.right, cButFrame.top ) );
		DrawLine( Point( cButFrame.right, cButFrame.bottom ) );
		DrawLine( Point( cButFrame.left, cButFrame.bottom ) );
		DrawLine( Point( cButFrame.left, cButFrame.top ) );
		cButFrame.Resize( 1, 1, -1, -1 );
	}

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
