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

#include <atheos/types.h>

#include <util/looper.h>
#include <util/message.h>
#include <gui/button.h>
#include <gui/window.h>
#include <gui/font.h>

#include <macros.h>

using namespace os;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Button::Button( Rect cFrame, const char* pzName, const char* pzLabel,
		Message* pcMessage, uint32 nResizeMask , uint32 nFlags )
    : Control( cFrame, pzName, pzLabel, pcMessage, nResizeMask, nFlags )
{
    m_bClicked = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Button::~Button()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Button::GetPreferredSize( bool bLargest ) const
{
    font_height sHeight;

    GetFontHeight( &sHeight );

    if ( bLargest ) {
	return( Point( (GetStringWidth( GetLabel() ) + 16) * 1.1f , sHeight.ascender + sHeight.descender + sHeight.line_gap + 8 ) );
    } else {
	return( Point( GetStringWidth( GetLabel() ) + 16 , sHeight.ascender + sHeight.descender + sHeight.line_gap + 8 ) );
    }
}

void Button::PostValueChange( const Variant& cNewValue )
{
    Invalidate();
    Flush();
}

void Button::LabelChanged( const std::string& cNewLabel )
{
    Invalidate();
    Flush();
}

void Button::EnableStatusChanged( bool bIsEnabled )
{
    Invalidate();
    Flush();
}

bool Button::Invoked( Message* pcMessage )
{
    Control::Invoked( pcMessage );
    return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Button::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    if ( IsEnabled() == false ) {
	View::KeyDown( pzString, pzRawString, nQualifiers );
	return;
    }
    if ( pzString[1] == '\0' && (pzString[0] == VK_ENTER || pzString[0] == ' ')  ) {
	SetValue(1, false );
    } else {
	SetValue(0, false);
	Control::KeyDown( pzString, pzRawString, nQualifiers );
    }
}

void Button::KeyUp( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    if ( IsEnabled() == false ) {
	View::KeyUp( pzString, pzRawString, nQualifiers );
	return;
    }
    if ( pzString[1] == '\0' && (pzString[0] == VK_ENTER || pzString[0] == ' ')  ) {
	if ( GetValue().AsBool() == true ) {
	    SetValue( false );
	}
    } else {
	Control::KeyDown( pzString, pzRawString, nQualifiers );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Button::MouseDown( const Point& cPosition, uint32 nButton )
{
    if ( nButton != 1 || IsEnabled() == false ) {
	View::MouseDown( cPosition, nButton );
	return;
    }
    
    MakeFocus( true );

    m_bClicked = GetBounds().DoIntersect( cPosition );
    SetValue( m_bClicked, false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Button::MouseUp( const Point& cPosition, uint32 nButton, Message* pcData )
{
    if ( nButton != 1 || IsEnabled() == false ) {
	View::MouseUp( cPosition, nButton, pcData );
	return;
    }
    if ( GetValue().AsBool() != false ) {
	SetValue( false );
    }
    m_bClicked = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Button::MouseMove( const Point& cPosition, int nCode, uint32 nButtons, Message* pcData )
{
    if ( IsEnabled() == false ) {
	View::MouseMove( cPosition, nCode, nButtons, pcData );
	return;
    }
    
    if ( m_bClicked )
    {
	uint32	nButtons;

	GetMouse( NULL, &nButtons );

	if ( nButtons == 0 ) {
	    return;
	}

	if ( nCode == MOUSE_OUTSIDE ) {
	    SetValue( false, false );
	}	else {
	    SetValue( true, false );
	}
    }
}

void Button::Activated( bool bIsActive )
{
    Invalidate();
    Flush();
}

void Button::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetBounds();


    float vStrWidth = GetStringWidth( GetLabel() );

    font_height sHeight;
    GetFontHeight( &sHeight );

    float vCharHeight = sHeight.ascender/* + sHeight.descender*/;
    float y = floor( cBounds.Height()*0.5f - vCharHeight*0.5f + sHeight.ascender );
    float x = floor((cBounds.Width()+1.0f) / 2.0f - vStrWidth / 2.0f);

    if ( GetValue().AsBool() ) {
	y += 1.0f;
	x += 1.0f;
    }
    
    SetEraseColor( get_default_color( COL_NORMAL ) );

    uint32 nFrameFlags = (GetValue().AsBool() && IsEnabled()) ? FRAME_RECESSED : FRAME_RAISED;

    if ( IsEnabled() && GetWindow()->GetDefaultButton() == this ) {
	SetFgColor( 0, 0, 0 );
	DrawLine( Point( cBounds.left, cBounds.top ), Point( cBounds.right, cBounds.top ) );
	DrawLine( Point( cBounds.right, cBounds.bottom ) );
	DrawLine( Point( cBounds.left, cBounds.bottom ) );
	DrawLine( Point( cBounds.left, cBounds.top ) );
	cBounds.left += 1;
	cBounds.top += 1;
	cBounds.right -= 1;
	cBounds.bottom -= 1;
	DrawFrame( cBounds, nFrameFlags );
    } else {
	DrawFrame( cBounds, nFrameFlags );
    }

    if ( IsEnabled() ) {
	SetFgColor( 0, 0, 0 );
    } else {
	SetFgColor( 255, 255, 255 );
    }
    SetBgColor( get_default_color( COL_NORMAL ) );
  
    MovePenTo( x, y );
  
    DrawString( GetLabel() );
    if ( IsEnabled() == false ) {
	SetFgColor( 100, 100, 100 );
	MovePenTo( x - 1.0f, y - 1.0f );
	SetDrawingMode( DM_OVER );
	DrawString( GetLabel() );
	SetDrawingMode( DM_COPY );
    }
    
    if ( IsEnabled() && HasFocus() ) {
	DrawLine( Point( (cBounds.Width()+1.0f)*0.5f - vStrWidth*0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ),
		  Point( (cBounds.Width()+1.0f)*0.5f + vStrWidth*0.5f, y + sHeight.descender - sHeight.line_gap / 2 - 1.0f ) );
    }
}

