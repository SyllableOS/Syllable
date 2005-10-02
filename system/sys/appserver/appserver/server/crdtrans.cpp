
/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "layer.h"

using namespace os;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Layer::ConvertToParent( const Point & cPoint ) const
{
	return ( cPoint + GetLeftTop() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::ConvertToParent( Point * pcPoint ) const
{
	*pcPoint += GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect Layer::ConvertToParent( const Rect & cRect ) const
{
	return ( cRect + GetLeftTop() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::ConvertToParent( Rect * pcRect ) const
{
	*pcRect += GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Layer::ConvertFromParent( const Point & cPoint ) const
{
	return ( cPoint - GetLeftTop() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::ConvertFromParent( Point * pcPoint ) const
{
	*pcPoint -= GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect Layer::ConvertFromParent( const Rect & cRect ) const
{
	return ( cRect - GetLeftTop() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::ConvertFromParent( Rect * pcRect ) const
{
	*pcRect -= GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Layer::ConvertToRoot( const Point & cPoint ) const
{
	if( m_pcParent != NULL )
	{
		return ( m_pcParent->ConvertToRoot( cPoint + GetLeftTop() ) );
	}
	else
	{
		return ( cPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::ConvertToRoot( Point * pcPoint ) const
{
	if( m_pcParent != NULL )
	{
		*pcPoint += GetLeftTop();
		m_pcParent->ConvertToRoot( pcPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect Layer::ConvertToRoot( const Rect & cRect ) const
{
	if( m_pcParent != NULL )
	{
		return ( m_pcParent->ConvertToRoot( cRect + GetLeftTop() ) );
	}
	else
	{
		return ( cRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::ConvertToRoot( Rect * pcRect ) const
{
	if( m_pcParent != NULL )
	{
		*pcRect += GetLeftTop();
		m_pcParent->ConvertToRoot( pcRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Layer::ConvertFromRoot( const Point & cPoint ) const
{
	if( m_pcParent != NULL )
	{
		return ( m_pcParent->ConvertFromRoot( cPoint - GetLeftTop() ) );
	}
	else
	{
		return ( cPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::ConvertFromRoot( Point * pcPoint ) const
{
	if( m_pcParent != NULL )
	{
		*pcPoint -= GetLeftTop();
		m_pcParent->ConvertFromRoot( pcPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect Layer::ConvertFromRoot( const Rect & cRect ) const
{
	if( m_pcParent != NULL )
	{
		return ( m_pcParent->ConvertFromRoot( cRect - GetLeftTop() ) );
	}
	else
	{
		return ( cRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::ConvertFromRoot( Rect * pcRect ) const
{
	if( m_pcParent != NULL )
	{
		*pcRect -= GetLeftTop();
		m_pcParent->ConvertFromRoot( pcRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Layer::ConvertToBitmap( const Point & cPoint ) const
{
	if( m_pcParent != NULL && m_pcBackbuffer == NULL )
	{
		return ( m_pcParent->ConvertToBitmap( cPoint + GetLeftTop() ) );
	}
	else
	{
		return ( cPoint );
	}
}
