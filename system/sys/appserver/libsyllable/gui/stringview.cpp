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
#include <gui/stringview.h>
#include <gui/font.h>

#include <macros.h>
#include <string>

using namespace os;

class StringView::Private
{
	public:
	Private() {
		m_cMinSize = IPoint( 0, 0 );
		m_cMaxSize = IPoint( 0, 0 );
		m_bHasBorder = false;
	}

	String		m_cString;
	IPoint		m_cMinSize;
	IPoint		m_cMaxSize;
    alignment	m_eAlign;
    bool		m_bHasBorder;
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

StringView::StringView( Rect cFrame, const String& cName, const String& cString, alignment eAlign, uint32 nResizeMask, uint32 nFlags ):View( cFrame, cName, nResizeMask, nFlags )
{
	m = new Private;
	m->m_eAlign = eAlign;
	SetString( cString );
	SetFgColor( 0, 0, 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

StringView::~StringView()
{
	delete m;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void StringView::SetString( const String& cString )
{
	m->m_cString = cString;
	Invalidate();
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

const String& StringView::GetString( void ) const
{
	return ( m->m_cString );
}

bool StringView::HasBorder() const
{
	return m->m_bHasBorder;
}

void StringView::SetRenderBorder( bool bRender )
{
	m->m_bHasBorder = bRender;
}

void StringView::SetMinPreferredSize( int nWidthChars, int nHeightChars )
{
	m->m_cMinSize.x = nWidthChars;
	m->m_cMinSize.y = nHeightChars;
}

void StringView::SetMaxPreferredSize( int nWidthChars, int nHeightChars )
{
	m->m_cMaxSize.x = nWidthChars;
	m->m_cMaxSize.y = nHeightChars;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point StringView::GetPreferredSize( bool bLargest ) const
{
	Point cExt = GetTextExtent( m->m_cString );
	Point cBorder( 0, 0 );

	font_height	sHeight;
	GetFontHeight( &sHeight );

	if( m->m_bHasBorder ) {
		cBorder.x += 4;
		cBorder.y += 4;
	}

	if( bLargest )
	{
		return ( Point(
			( ( m->m_cMaxSize.x > 0 ) ? float ( m->m_cMaxSize.x ) * GetStringWidth( "M" ) +  + cBorder.x : COORD_MAX ),
			( ( m->m_cMaxSize.y > 0 ) ? float ( m->m_cMaxSize.y ) * ( sHeight.ascender + sHeight.descender )  + cBorder.y : COORD_MAX ) ) );
		/* FIXME: This code assumes that M is the widest character. */
	}
	else
	{
		return ( Point(
			( ( m->m_cMinSize.x > 0 ) ? float ( m->m_cMinSize.x ) * GetStringWidth( "M" )  + cBorder.x : cExt.x + cBorder.x ),
			( ( m->m_cMinSize.y > 0 ) ? float ( m->m_cMinSize.y ) * ( sHeight.ascender + sHeight.descender ) + cBorder.y : cExt.y + cBorder.y ) ) );
		/* FIXME: This code assumes that M is the widest character. */
	}
}

void StringView::AttachedToWindow()
{
	View *pcParent = GetParent();

	if( pcParent != NULL )
	{
		SetBgColor( pcParent->GetBgColor() );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void StringView::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetBounds();

	if( m->m_bHasBorder ) {
		SetEraseColor( GetBgColor() );
		DrawFrame( cBounds, FRAME_RECESSED | FRAME_THIN );
		cBounds.Resize( 2, 2, -2, -2 );
	} else {
		FillRect( cBounds, GetBgColor() );
	}

	if( m->m_eAlign == ALIGN_LEFT )
	{
		DrawText( cBounds, m->m_cString );
	}
	else if( m->m_eAlign == ALIGN_RIGHT )
	{
		DrawText( cBounds, m->m_cString, DTF_ALIGN_RIGHT );
	}
	else
	{
		DrawText( cBounds, m->m_cString, DTF_CENTER );
	}
}

void StringView::__SV_reserved1__()
{
}

void StringView::__SV_reserved2__()
{
}

void StringView::__SV_reserved3__()
{
}

void StringView::__SV_reserved4__()
{
}

void StringView::__SV_reserved5__()
{
}
