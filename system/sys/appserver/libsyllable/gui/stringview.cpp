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
	}

	String		m_cString;
	IPoint		m_cMinSize;
	IPoint		m_cMaxSize;
    alignment	m_eAlign;
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
	SetFgColor( 0, 0, 0 );
	SetString( cString );
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

	if( bLargest )
	{
		return ( Point(
			( ( m->m_cMaxSize.x > 0 ) ? float ( m->m_cMaxSize.x ) * GetStringWidth( "M" ) : COORD_MAX ),
			( ( m->m_cMaxSize.y > 0 ) ? float ( m->m_cMaxSize.y ) * GetStringWidth( "M" ) : COORD_MAX ) ) );
		/* FIXME: This code assumes that M is the widest character. */
	}
	else
	{
		return ( Point(
			( ( m->m_cMinSize.x > 0 ) ? float ( m->m_cMinSize.x ) * GetStringWidth( "M" ) : cExt.x ),
			( ( m->m_cMinSize.y > 0 ) ? float ( m->m_cMinSize.y ) * GetStringWidth( "M" ) : cExt.y ) ) );
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

	FillRect( cBounds, get_default_color( COL_NORMAL ) );

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
