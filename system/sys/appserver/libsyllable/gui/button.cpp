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

#include <atheos/types.h>

#include <util/looper.h>
#include <util/message.h>
#include <util/shortcutkey.h>
#include <gui/button.h>
#include <gui/window.h>
#include <gui/font.h>

#include <macros.h>

using namespace os;

class Button::Private {
	public:
	bool m_bClicked;
	InputMode m_eInputMode;
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Button::Button( Rect cFrame, const String& cName, const String& cLabel, Message * pcMessage, uint32 nResizeMask, uint32 nFlags )
 : Control( cFrame, cName, cLabel, pcMessage, nResizeMask, nFlags )
{
	m = new Private;
	m->m_bClicked = false;
	m->m_eInputMode = InputModeNormal;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Button::~Button()
{
	delete m;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Button::GetPreferredSize( bool bLargest ) const
{
	if( bLargest )
	{
		return Point( COORD_MAX, COORD_MAX );
	} else {
		font_height sHeight;
		GetFontHeight( &sHeight );
		Point cStringExt = GetTextExtent( GetLabel() );
		return Point( cStringExt.x+16, cStringExt.y + 10 + sHeight.line_gap );
	}
}

void Button::PostValueChange( const Variant & cNewValue )
{
	Invalidate();
	Flush();
}

void Button::LabelChanged( const String & cNewLabel )
{
	Invalidate();
	Flush();
}

void Button::EnableStatusChanged( bool bIsEnabled )
{
	Invalidate();
	Flush();
}

bool Button::Invoked( Message * pcMessage )
{
	Control::Invoked( pcMessage );
	return ( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Button::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( IsEnabled() == false )
	{
		View::KeyDown( pzString, pzRawString, nQualifiers );
		return;
	}
	if( ( pzString[1] == '\0' && ( pzString[0] == VK_ENTER || pzString[0] == ' ' ) ) ||
		( GetShortcut() == ShortcutKey( pzRawString, nQualifiers ) ) )
	{
		if( m->m_eInputMode == InputModeToggle ) {
			SetValue( GetValue().AsBool() ? 0 : 1, false );
		} else {
			SetValue( 1, false );
		}
		MakeFocus();
	}
	else
	{
		if( m->m_eInputMode == InputModeNormal ) SetValue( 0, false );
		Control::KeyDown( pzString, pzRawString, nQualifiers );
	}
}

void Button::KeyUp( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( IsEnabled() == false )
	{
		View::KeyUp( pzString, pzRawString, nQualifiers );
		return;
	}
	if( ( pzString[1] == '\0' && ( pzString[0] == VK_ENTER || pzString[0] == ' ' ) ) ||
		( GetShortcut() == ShortcutKey( pzRawString, nQualifiers ) ) )
	{
		if( m->m_eInputMode == InputModeNormal && GetValue().AsBool(  ) == true )
		{
			SetValue( false );
		}
	}
	else
	{
		Control::KeyDown( pzString, pzRawString, nQualifiers );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Button::MouseDown( const Point & cPosition, uint32 nButton )
{
	if( nButton != 1 || IsEnabled() == false )
	{
		View::MouseDown( cPosition, nButton );
		return;
	}

	MakeFocus( true );

	m->m_bClicked = GetBounds().DoIntersect( cPosition );
	if( m->m_eInputMode == InputModeToggle ) {
		if( m->m_bClicked ) {
			SetValue( GetValue().AsBool() ? 0 : 1 );
		}
	} else if( m->m_eInputMode == InputModeRadio ) {
		SetValue( m->m_bClicked );
	} else {
		SetValue( m->m_bClicked, false );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Button::MouseUp( const Point & cPosition, uint32 nButton, Message * pcData )
{
	if( nButton != 1 || IsEnabled() == false )
	{
		View::MouseUp( cPosition, nButton, pcData );
		return;
	}
	if( m->m_eInputMode == InputModeNormal ) {
		if( GetValue().AsBool(  ) != false )
		{
			SetValue( false );
		}
	}
	m->m_bClicked = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Button::MouseMove( const Point & cPosition, int nCode, uint32 nButtons, Message * pcData )
{
	if( IsEnabled() == false )
	{
		View::MouseMove( cPosition, nCode, nButtons, pcData );
		return;
	}

	if( m->m_bClicked && m->m_eInputMode == InputModeNormal )
	{
		uint32 nButtons;

		GetMouse( NULL, &nButtons );

		if( nButtons == 0 )
		{
			return;
		}

		if( nCode == MOUSE_OUTSIDE )
		{
			SetValue( false, false );
		}
		else
		{
			SetValue( true, false );
		}
	}
}

void Button::Activated( bool bIsActive )
{
	Invalidate();
	Flush();
}

void Button::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetBounds();

	SetEraseColor( get_default_color( COL_NORMAL ) );

	uint32 nFrameFlags = ( GetValue().AsBool(  ) && IsEnabled(  ) )? FRAME_RECESSED : FRAME_RAISED;

	if( IsEnabled() && ( HasFocus() || ( GetWindow(  )->GetDefaultButton(  ) == this ) ) )
	{
		if( HasFocus() ) {
			SetFgColor( get_default_color( COL_FOCUS ) );
		} else {
			SetFgColor( 0, 0, 0 );
		}
		DrawLine( Point( cBounds.left, cBounds.top ), Point( cBounds.right, cBounds.top ) );
		DrawLine( Point( cBounds.right, cBounds.bottom ) );
		DrawLine( Point( cBounds.left, cBounds.bottom ) );
		DrawLine( Point( cBounds.left, cBounds.top ) );
		cBounds.Resize( 1, 1, -1, -1 );
	}

	DrawFrame( cBounds, nFrameFlags );

	if( IsEnabled() )
	{
		SetFgColor( 0, 0, 0 );
	}
	else
	{
		SetFgColor( 255, 255, 255 );
	}
	SetBgColor( get_default_color( COL_NORMAL ) );

	if( GetValue().AsBool(  ) )
	{
		cBounds.MoveTo( 1, 1 );
	}

	DrawText( cBounds, GetLabel(), DTF_ALIGN_CENTER | DTF_UNDERLINES );
	if( IsEnabled() == false )
	{
		SetFgColor( 100, 100, 100 );
		SetDrawingMode( DM_OVER );
		cBounds.MoveTo( -1, -1 );
		DrawText( cBounds, GetLabel(), DTF_ALIGN_CENTER | DTF_UNDERLINES );
		SetDrawingMode( DM_COPY );
	}
}

void Button::SetInputMode( InputMode eInputMode )
{
	m->m_eInputMode = eInputMode;
	Flush();
}

Button::InputMode Button::GetInputMode() const
{
	return m->m_eInputMode;
}

