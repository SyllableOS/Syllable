
/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
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

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

StringView::StringView( Rect cFrame, const char *pzName, const char *pzString, alignment eAlign, uint32 nResizeMask, uint32 nFlags ):View( cFrame, pzName, nResizeMask, nFlags )
{
	m = new data;
	m->m_nMinSize = 0;
	m->m_nMaxSize = 0;
	m_eAlign = eAlign;
	SetFgColor( 0, 0, 0 );
	SetString( pzString );
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

void StringView::SetString( const char *pzString )
{
	if( pzString == NULL )
	{
		m->m_cString.resize( 0 );
	}
	else
	{
		m->m_cString = pzString;
	}
	Invalidate();
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

const char *StringView::GetString( void ) const
{
	return ( m->m_cString.c_str() );
}

void StringView::SetMinPreferredSize( int nWidthChars )
{
	m->m_nMinSize = nWidthChars;
}

void StringView::SetMaxPreferredSize( int nWidthChars )
{
	m->m_nMaxSize = nWidthChars;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point StringView::GetPreferredSize( bool bLargest ) const
{
	font_height sHeight;

	GetFontHeight( &sHeight );
	float vStrWidth = GetStringWidth( m->m_cString );

	if( bLargest )
	{
		if( m->m_nMaxSize > 0 )
		{
			return ( Point( float ( m->m_nMaxSize ) * GetStringWidth( "M" ), sHeight.ascender + sHeight.descender ) );
		}
		else
		{
			return ( Point( vStrWidth, sHeight.ascender + sHeight.descender ) );
		}
	}
	else
	{
		if( m->m_nMinSize > 0 )
		{
			return ( Point( float ( m->m_nMinSize ) * GetStringWidth( "M" ), sHeight.ascender + sHeight.descender ) );
		}
		else
		{
			return ( Point( vStrWidth, sHeight.ascender + sHeight.descender ) );
		}
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

	font_height sHeight;
	float vStrWidth = GetStringWidth( m->m_cString );

	GetFontHeight( &sHeight );

	float x;

	if( m_eAlign == ALIGN_LEFT )
	{
		x = 0.0f;
	}
	else if( m_eAlign == ALIGN_RIGHT )
	{
		x = cBounds.Width() + 1.0f - vStrWidth;
	}
	else
	{
		x = ( cBounds.Width() + 1.0f ) / 2.0f - vStrWidth / 2.0f;
	}

	DrawString( m->m_cString, Point( x, ( cBounds.Height() + 1.0f ) / 2 - ( sHeight.ascender + sHeight.descender ) / 2 + sHeight.ascender ) );
}
