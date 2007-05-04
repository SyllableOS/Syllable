
/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#include <stdio.h>

#include "sprite.h"
#include "bitmap.h"
#include "ddriver.h"

#include <gui/region.h>

#include <algorithm>

using namespace os;

Locker SrvSprite::s_cLock( "sprite_lock" );

std::vector < SrvSprite * >SrvSprite::s_cInstances;
atomic_t SrvSprite::s_nHideCount = ATOMIC_INIT(0);

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SrvSprite::SrvSprite( const IRect & cBounds, const IPoint & cPos, const IPoint & cHotSpot, SrvBitmap * pcTarget, SrvBitmap * pcImage, bool bLastPos )
{
	s_cLock.Lock();
	m_pcImage = pcImage;
	m_pcTarget = pcTarget;
	if( pcImage != NULL )
	{
		m_pcBackground = new SrvBitmap( pcImage->m_nWidth, pcImage->m_nHeight, pcTarget->m_eColorSpc );
	}
	else
	{
		m_pcBackground = NULL;
	}
	m_cPosition = cPos;
	m_cHotSpot = cHotSpot;
	m_bVisible = false;

	if( pcImage != NULL )
	{
		m_cBounds = IRect( 0, 0, pcImage->m_nWidth - 1, pcImage->m_nHeight - 1 );
	}
	else
	{
		m_cBounds = cBounds;
	}

	pcTarget->AddRef();
	if( NULL != pcImage )
	{
		pcImage->AddRef();
	}
	
	SrvSprite::Hide();
	if( pcImage == NULL )
		s_cInstances.push_back( this );
	else {
		s_cInstances.insert( s_cInstances.begin() + (bLastPos ? s_cInstances.size() : 0), this );
	}
	SrvSprite::Unhide();
	s_cLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SrvSprite::~SrvSprite()
{
	s_cLock.Lock();
	SrvSprite::Hide();
	m_pcTarget->Release();
	if( NULL != m_pcBackground )
	{
		m_pcBackground->Release();
	}
	if( NULL != m_pcImage )
	{
		m_pcImage->Release();
	}
	std::vector < SrvSprite * >::iterator i = find( s_cInstances.begin(), s_cInstances.end(  ), this );

	s_cInstances.erase( i );
	SrvSprite::Unhide();
	s_cLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------


void SrvSprite::ColorSpaceChanged()
{
	for( uint i = 0; i < s_cInstances.size(); ++i )
	{
		SrvSprite *pcSprite = s_cInstances[i];

		if( pcSprite->m_pcImage != NULL )
		{
			pcSprite->m_pcBackground->Release();
			pcSprite->m_pcBackground = new SrvBitmap( pcSprite->m_pcImage->m_nWidth, pcSprite->m_pcImage->m_nHeight, pcSprite->m_pcTarget->m_eColorSpc );
		}
	}
}

void SrvSprite::Hide( const IRect & cFrame )
{
	bool bDoHide = false;

	s_cLock.Lock();
	atomic_inc( &s_nHideCount );

	for( int i = s_cInstances.size() - 1; i >= 0 ; --i )
	{
		SrvSprite *pcSprite = s_cInstances[i];

		if( cFrame.DoIntersect( pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot ) )
		{
			bDoHide = true;
			break;
		}
	}
	if( bDoHide )
	{
		for( int i = s_cInstances.size() - 1; i >= 0; --i )
		{
			s_cInstances[i]->Erase();
		}
	}
	s_cLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Hide()
{
	s_cLock.Lock();
	atomic_inc( &s_nHideCount );

	for( int i = s_cInstances.size() - 1; i >= 0 ; --i )
	{
		SrvSprite *pcSprite = s_cInstances[i];

		if( pcSprite->m_bVisible )
		{
			pcSprite->Erase();
		}
	}
	s_cLock.Unlock();
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::HideForCopy( const IRect & cFrame )
{
	/* If we do a blit operation and the target frame covers all our sprites,
	   we do not have to erase them. We just need to set the visible flags to false. */
	bool bHidePossible = true;

	s_cLock.Lock();

	for( int i = s_cInstances.size() - 1; i >= 0 ; --i )
	{
		SrvSprite *pcSprite = s_cInstances[i];

		if( !cFrame.Includes( pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot ) )
		{
			bHidePossible = false;
			break;
		}
	}
	if( !bHidePossible )
	{
		SrvSprite::Hide( cFrame );
		s_cLock.Unlock();		
		return;
	}
	atomic_inc( &s_nHideCount );	
		
	for( int i = s_cInstances.size() - 1; i >= 0; --i )
	{
		s_cInstances[i]->m_bVisible = false;
	}
	s_cLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Unhide()
{
	s_cLock.Lock();
	if( !atomic_dec_and_test( &s_nHideCount ) )
	{
		s_cLock.Unlock();
		return;
	}
	for( uint i = 0; i < s_cInstances.size(); ++i )
	{
		SrvSprite *pcSprite = s_cInstances[i];

		if( pcSprite->m_bVisible == false )
		{
			pcSprite->Draw();
		}
	}
	s_cLock.Unlock();
}

bool SrvSprite::DoIntersect( const os::IRect& cRect )
{
	s_cLock.Lock();
	for( uint i = 0; i < s_cInstances.size(); ++i )
	{
		SrvSprite *pcSprite = s_cInstances[i];

		if( cRect.DoIntersect( pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot ) )
		{
			s_cLock.Unlock();
			return( true );
		}
	}
	s_cLock.Unlock();
	return( false );
}

bool SrvSprite::CoveredByRect( const os::IRect& cRect )
{
	s_cLock.Lock();
	for( uint i = 0; i < s_cInstances.size(); ++i )
	{
		SrvSprite *pcSprite = s_cInstances[i];

		if( !cRect.Includes( pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot ) )
		{
			s_cLock.Unlock();
			return( false );
		}
	}
	s_cLock.Unlock();
	return( true );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Draw()
{
	if( m_bVisible )
	{
		return;
	}
	m_bVisible = true;
	DisplayDriver *pcDriver = m_pcTarget->m_pcDriver;

	IRect cBitmapRect( 0, 0, m_pcTarget->m_nWidth - 1, m_pcTarget->m_nHeight - 1 );
	
	if( m_pcImage != NULL )
	{
		IRect cRect = m_cBounds + m_cPosition - m_cHotSpot;
		IRect cClipRect = cRect & cBitmapRect;
		IPoint cOffset = cClipRect.LeftTop() - cRect.LeftTop(  );
		
		if( cClipRect.IsValid() )
		{
			pcDriver->BltBitmap( m_pcBackground, m_pcTarget, cClipRect, cClipRect.Bounds() + cOffset, DM_COPY, 0xff );
			pcDriver->BltBitmap( m_pcTarget, m_pcImage, cClipRect.Bounds() + cOffset, cClipRect.Bounds() + m_cPosition - m_cHotSpot + cOffset, DM_BLEND, 0xff );
		}
	}
	else
	{
		IRect cFrame = m_cBounds + m_cPosition - m_cHotSpot;

		pcDriver->DrawLine( m_pcTarget, cBitmapRect, IPoint( cFrame.left, cFrame.top ), IPoint( cFrame.right, cFrame.top ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( m_pcTarget, cBitmapRect, IPoint( cFrame.right, cFrame.top ), IPoint( cFrame.right, cFrame.bottom ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( m_pcTarget, cBitmapRect, IPoint( cFrame.right, cFrame.bottom ), IPoint( cFrame.left, cFrame.bottom ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( m_pcTarget, cBitmapRect, IPoint( cFrame.left, cFrame.bottom ), IPoint( cFrame.left, cFrame.top ), Color32_s(), DM_INVERT );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Erase()
{
	if( m_bVisible == false )
	{
		return;
	}
	DisplayDriver *pcDriver = m_pcTarget->m_pcDriver;

	IRect cBitmapRect( 0, 0, m_pcTarget->m_nWidth - 1, m_pcTarget->m_nHeight - 1 );

	if( m_pcImage != NULL )
	{
		IRect cRect = m_cBounds + m_cPosition - m_cHotSpot;
		IRect cClipRect = cRect & cBitmapRect;
		IPoint cOffset = cClipRect.LeftTop() - cRect.LeftTop(  );

		if( cClipRect.IsValid() )
		{
			pcDriver->BltBitmap( m_pcTarget, m_pcBackground, cClipRect.Bounds() + cOffset, cClipRect.Bounds() + m_cPosition - m_cHotSpot + cOffset, DM_COPY, 0xff );
		}
	}
	else
	{
		IRect cFrame = m_cBounds + m_cPosition - m_cHotSpot;

		pcDriver->DrawLine( m_pcTarget, cBitmapRect, IPoint( cFrame.left, cFrame.top ), IPoint( cFrame.right, cFrame.top ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( m_pcTarget, cBitmapRect, IPoint( cFrame.right, cFrame.top ), IPoint( cFrame.right, cFrame.bottom ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( m_pcTarget, cBitmapRect, IPoint( cFrame.right, cFrame.bottom ), IPoint( cFrame.left, cFrame.bottom ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( m_pcTarget, cBitmapRect, IPoint( cFrame.left, cFrame.bottom ), IPoint( cFrame.left, cFrame.top ), Color32_s(), DM_INVERT );
	}
	m_bVisible = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Draw( SrvBitmap * pcTarget, const IPoint & cPos )
{
	DisplayDriver *pcDriver = m_pcTarget->m_pcDriver;

	IRect cBitmapRect( 0, 0, pcTarget->m_nWidth - 1, pcTarget->m_nHeight - 1 );

	if( m_pcImage != NULL )
	{

		IRect cRect = m_cBounds + cPos;
		IRect cClipRect = cRect & cBitmapRect;

		if( cClipRect.IsValid() )
		{
			IPoint cOffset = cClipRect.LeftTop() - cRect.LeftTop(  );

			pcDriver->BltBitmap( pcTarget, m_pcImage, cClipRect.Bounds() + cOffset, cClipRect.Bounds() + cPos + cOffset, DM_BLEND, 0xff );
		}
	}
	else
	{
		IRect cFrame = m_cBounds + cPos;

		pcDriver->DrawLine( pcTarget, cBitmapRect, IPoint( cFrame.left, cFrame.top ), IPoint( cFrame.right, cFrame.top ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( pcTarget, cBitmapRect, IPoint( cFrame.right, cFrame.top ), IPoint( cFrame.right, cFrame.bottom ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( pcTarget, cBitmapRect, IPoint( cFrame.right, cFrame.bottom ), IPoint( cFrame.left, cFrame.bottom ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( pcTarget, cBitmapRect, IPoint( cFrame.left, cFrame.bottom ), IPoint( cFrame.left, cFrame.top ), Color32_s(), DM_INVERT );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Capture( SrvBitmap * pcTarget, const IPoint & cPos )
{
	if( m_pcImage != NULL && m_pcBackground != NULL )
	{
		DisplayDriver *pcDriver = m_pcTarget->m_pcDriver;

		IRect cBitmapRect( 0, 0, pcTarget->m_nWidth - 1, pcTarget->m_nHeight - 1 );
		IRect cRect = m_cBounds + cPos;
		IRect cClipRect = cRect & cBitmapRect;
		IPoint cOffset = cClipRect.LeftTop() - cRect.LeftTop(  );

		if( cClipRect.IsValid() )
		{
			pcDriver->BltBitmap( m_pcBackground, pcTarget, cClipRect, cClipRect.Bounds() + cOffset, DM_COPY, 0xff );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Erase( SrvBitmap * pcTarget, const IPoint & cPos )
{
	DisplayDriver *pcDriver = m_pcTarget->m_pcDriver;

	IRect cBitmapRect( 0, 0, pcTarget->m_nWidth - 1, pcTarget->m_nHeight - 1 );

	if( m_pcImage != NULL )
	{
		IRect cRect = m_cBounds + cPos;
		IRect cClipRect = cRect & cBitmapRect;

		if( cClipRect.IsValid() )
		{
			IPoint cOffset = cClipRect.LeftTop() - cRect.LeftTop(  );

			pcDriver->BltBitmap( pcTarget, m_pcBackground, cClipRect.Bounds() + cOffset, cClipRect.Bounds() + cPos + cOffset, DM_COPY, 0xff );
		}
	}
	else
	{
		IRect cFrame = m_cBounds + cPos;

		pcDriver->DrawLine( pcTarget, cBitmapRect, IPoint( cFrame.left, cFrame.top ), IPoint( cFrame.right, cFrame.top ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( pcTarget, cBitmapRect, IPoint( cFrame.right, cFrame.top ), IPoint( cFrame.right, cFrame.bottom ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( pcTarget, cBitmapRect, IPoint( cFrame.right, cFrame.bottom ), IPoint( cFrame.left, cFrame.bottom ), Color32_s(), DM_INVERT );
		pcDriver->DrawLine( pcTarget, cBitmapRect, IPoint( cFrame.left, cFrame.bottom ), IPoint( cFrame.left, cFrame.top ), Color32_s(), DM_INVERT );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::MoveBy( const IPoint & cDelta )
{
	#if 0
	if( this != s_cInstances[0] )
	{
		return;
	}
	#endif
	s_cLock.Lock();
	if( atomic_read( &s_nHideCount ) > 0 )
	{
		for( uint i = s_cInstances.size() - 1; i >= 0; --i )
		{
			SrvSprite *pcSprite = s_cInstances[i];

			pcSprite->m_cPosition += cDelta;
		}
		s_cLock.Unlock();
		return;
	}

	DisplayDriver *pcDriver = m_pcTarget->m_pcDriver;

	IRect cOldRect( 100000, 100000, -100000, -100000 );
	IRect cNewRect( 100000, 100000, -100000, -100000 );

	for( uint i = 0; i < s_cInstances.size(); ++i )
	{
		SrvSprite *pcSprite = s_cInstances[i];

		if( pcSprite->m_pcImage == NULL )
		{
			pcSprite->Erase();
			continue;
		}
		IRect cRect( pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot );

		cOldRect |= cRect;
		cNewRect |= cRect + cDelta;
	}

	if( cOldRect.DoIntersect( cNewRect ) == false )
	{
		for( int i = s_cInstances.size() - 1; i >= 0; --i )
		{
			SrvSprite *pcSprite = s_cInstances[i];

			if( pcSprite->m_pcImage != NULL )
			{
				pcSprite->Erase();
				continue;
			}
		}
		for( uint i = 0; i < s_cInstances.size(); ++i )
		{
			s_cInstances[i]->m_cPosition += cDelta;
			s_cInstances[i]->Draw();
		}
	}
	else
	{
		IRect cBitmapRect( 0, 0, m_pcTarget->m_nWidth - 1, m_pcTarget->m_nHeight - 1 );

		IRect cFullRect = cOldRect | cNewRect;
		IRect cTotRect = cFullRect & cBitmapRect;
		IPoint cLeftTop = cTotRect.LeftTop();

		Region cReg( cTotRect );

		SrvBitmap *pcTmp = new SrvBitmap( cTotRect.Width() + 1, cTotRect.Height(  ) + 1, m_pcTarget->m_eColorSpc );

		for( int i = s_cInstances.size() - 1; i >= 0; --i )
		{
			SrvSprite *pcSprite = s_cInstances[i];

			if( pcSprite->m_pcImage == NULL )
			{
				continue;
			}
			IRect cRect = pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot;

			cReg.Exclude( cRect );
			pcSprite->Erase( pcTmp, cRect.LeftTop() - cLeftTop );
		}

		ClipRect *pcClip;

		ENUMCLIPLIST( &cReg.m_cRects, pcClip )
		{
			pcDriver->BltBitmap( pcTmp, m_pcTarget, pcClip->m_cBounds, pcClip->m_cBounds - cLeftTop, DM_COPY, 0xff );
		}

		for( uint i = 0; i < s_cInstances.size(); ++i )
		{
			SrvSprite *pcSprite = s_cInstances[i];

			if( pcSprite->m_pcImage == NULL )
			{
				continue;
			}
			IRect cRect = pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot;
			IPoint cClipOff = cRect.LeftTop() - cLeftTop;

			pcSprite->Capture( pcTmp, cClipOff + cDelta );
		}
		for( uint i = 0; i < s_cInstances.size(); ++i )
		{
			SrvSprite *pcSprite = s_cInstances[i];

			if( pcSprite->m_pcImage != NULL )
			{
				pcSprite->m_cPosition += cDelta;

				IRect cRect = pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot;
				IPoint cClipOff = cRect.LeftTop() - cLeftTop;

				pcSprite->Draw( pcTmp, cClipOff );
			}
		}
		if( cTotRect.IsValid() )
		{
			pcDriver->BltBitmap( m_pcTarget, pcTmp, cTotRect.Bounds(), cTotRect.Bounds() + cLeftTop, DM_COPY, 0xff );
		}
		for( uint i = 0; i < s_cInstances.size(); ++i )
		{
			SrvSprite *pcSprite = s_cInstances[i];

			if( pcSprite->m_pcImage == NULL )
			{
				pcSprite->m_cPosition += cDelta;
				pcSprite->Draw();
			}
		}
		pcTmp->Release();
	}
	s_cLock.Unlock();
}


