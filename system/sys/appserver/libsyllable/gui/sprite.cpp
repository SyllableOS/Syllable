
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

#include <assert.h>

#include <gui/sprite.h>
#include <gui/bitmap.h>
#include <util/application.h>
#include <gui/exceptions.h>

using namespace os;

Sprite::Sprite( const Point & cPosition, Bitmap * pcBitmap ):m_cPosition( cPosition )
{
	assert( Application::GetInstance() != NULL );

	int nError = Application::GetInstance()->CreateSprite( Rect( 0, 0, 0, 0 ) + cPosition, pcBitmap->m_hHandle, &m_nHandle );

	if( nError < 0 )
	{
		throw( GeneralFailure( "Failed to create message port", -nError ) );
	}

}

Sprite::~Sprite()
{
	assert( Application::GetInstance() != NULL );
	Application::GetInstance()->DeleteSprite( m_nHandle );
}

void Sprite::MoveTo( const Point & cNewPos )
{
	assert( Application::GetInstance() != NULL );
	Application::GetInstance()->MoveSprite( m_nHandle, cNewPos );
	m_cPosition = cNewPos;
}

void Sprite::MoveBy( const Point & cPos )
{
	MoveTo( m_cPosition + cPos );
}
