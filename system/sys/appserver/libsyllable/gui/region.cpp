
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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <gui/region.h>

#include <vector>
#include <algorithm>

using namespace os;

void **Region::s_pFirstClip = NULL;
Locker Region::s_cClipListMutex( "region_cliplist_lock" );


ClipRectList::ClipRectList()
{
	m_pcFirst = NULL;
	m_pcLast = NULL;
	m_nCount = 0;
}

ClipRectList::~ClipRectList()
{
	while( m_pcFirst != NULL )
	{
		ClipRect *pcRect = m_pcFirst;

		m_pcFirst = pcRect->m_pcNext;
		Region::FreeClipRect( pcRect );
	}
}

void ClipRectList::Clear()
{
	while( m_pcFirst != NULL )
	{
		ClipRect *pcRect = m_pcFirst;

		m_pcFirst = pcRect->m_pcNext;
		Region::FreeClipRect( pcRect );
	}
	m_pcLast = NULL;
	m_nCount = 0;
}

void ClipRectList::AddRect( ClipRect * pcRect )
{
	pcRect->m_pcPrev = NULL;
	pcRect->m_pcNext = m_pcFirst;
	if( m_pcFirst != NULL )
	{
		m_pcFirst->m_pcPrev = pcRect;
	}
	m_pcFirst = pcRect;
	if( m_pcLast == NULL )
	{
		m_pcLast = pcRect;
	}
	m_nCount++;
}

void ClipRectList::RemoveRect( ClipRect * pcRect )
{
	if( pcRect->m_pcPrev != NULL )
	{
		pcRect->m_pcPrev->m_pcNext = pcRect->m_pcNext;
	}
	else
	{
		m_pcFirst = pcRect->m_pcNext;
	}
	if( pcRect->m_pcNext != NULL )
	{
		pcRect->m_pcNext->m_pcPrev = pcRect->m_pcPrev;
	}
	else
	{
		m_pcLast = pcRect->m_pcPrev;
	}
	m_nCount--;
}

ClipRect *ClipRectList::RemoveHead()
{
	if( m_nCount == 0 )
	{
		return ( NULL );
	}
	ClipRect *pcClip = m_pcFirst;

	m_pcFirst = pcClip->m_pcNext;
	if( m_pcFirst == NULL )
	{
		m_pcLast = NULL;
	}
	m_nCount--;
	return ( pcClip );
}

void ClipRectList::StealRects( ClipRectList * pcList )
{
	if( pcList->m_pcFirst == NULL )
	{
		assert( pcList->m_pcLast == NULL );
		return;
	}

	if( m_pcFirst == NULL )
	{
		assert( m_pcLast == NULL );
		m_pcFirst = pcList->m_pcFirst;
		m_pcLast = pcList->m_pcLast;
	}
	else
	{
		m_pcLast->m_pcNext = pcList->m_pcFirst;
		pcList->m_pcFirst->m_pcPrev = m_pcLast;
		m_pcLast = pcList->m_pcLast;
	}
	m_nCount += pcList->m_nCount;

	pcList->m_pcFirst = NULL;
	pcList->m_pcLast = NULL;
	pcList->m_nCount = 0;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Region::Region()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Region::Region( const IRect & cRect )
{
	AddRect( cRect );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Region::Region( const Region & cReg )
{
	Set( cReg );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Region::Region( const Region & cReg, const IRect & cRectangle, bool bNormalize )
{
	ClipRect *pcOldClip;

	IPoint cLefTop = cRectangle.LeftTop();

	ENUMCLIPLIST( &cReg.m_cRects, pcOldClip )
	{
		IRect cRect;
		ClipRect *pcNewClip;

		cRect = pcOldClip->m_cBounds & cRectangle;

		if( cRect.IsValid() )
		{
			pcNewClip = AllocClipRect();
			if( pcNewClip != NULL )
			{
				if( bNormalize )
				{
					cRect -= cLefTop;
				}
				pcNewClip->m_cBounds = cRect;
				m_cRects.AddRect( pcNewClip );
			}
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Region::~Region()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ClipRect *Region::AllocClipRect()
{
	int nError;

	while( ( nError = s_cClipListMutex.Lock() ) < 0 )
	{
		dbprintf( "Region::AllocClipRect failed to lock list : %s (%d)\n", strerror( errno ), nError );
		if( EINTR != errno )
		{
			return ( NULL );
		}
	}
	if( s_pFirstClip == NULL )
	{
		s_cClipListMutex.Unlock();
		return ( new ClipRect );
	}

	void **pHeader = s_pFirstClip;

	s_pFirstClip = ( void ** )pHeader[0];

	s_cClipListMutex.Unlock();

	return ( ( ClipRect * ) pHeader );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Region::FreeClipRect( ClipRect * pcRect )
{
	int nError;

	while( ( nError = s_cClipListMutex.Lock() ) < 0 )
	{
		dbprintf( "Region::FreeClipRect failed to lock list : %s (%d)\n", strerror( errno ), nError );
		if( EINTR != errno )
		{
			return;
		}
	}

	( ( void ** )pcRect )[0] = s_pFirstClip;
	s_pFirstClip = ( void ** )pcRect;

	s_cClipListMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Region::Set( const IRect & cRect )
{
	Clear();
	AddRect( cRect );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Region::Set( const Region & cReg )
{
	ClipRect *pcClip;

	Clear();

	ENUMCLIPLIST( &cReg.m_cRects, pcClip )
	{
		AddRect( pcClip->m_cBounds );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Region::Clear( void )
{
	m_cRects.Clear();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ClipRect *Region::AddRect( const IRect & cRect )
{
	ClipRect *pcClipRect;

	pcClipRect = AllocClipRect();

	if( pcClipRect != NULL )
	{
		pcClipRect->m_cBounds = cRect;
		m_cRects.AddRect( pcClipRect );
		return ( pcClipRect );
	}
	return ( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Region::Include( const IRect & cRect )
{
	ClipRect *DClip;
	Region cTmpReg( cRect );


	ENUMCLIPLIST( &m_cRects, DClip )
	{			/* remove all areas already present in drawlist */
		cTmpReg.Exclude( DClip->m_cBounds );
	}
	m_cRects.StealRects( &cTmpReg.m_cRects );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Region::Exclude( const IRect & cRect )
{
	ClipRectList cNewList;
	ClipRect *pcClip;
	IRect cNew[4];

	while( m_cRects.m_pcFirst != NULL )
	{
		pcClip = m_cRects.RemoveHead();

		IRect cHide = cRect & pcClip->m_cBounds;

		if( cHide.IsValid() == false )
		{
			cNewList.AddRect( pcClip );
			continue;
		}

	  /*** boundaries of the four possible rectangles surounding the one to remove.	***/

		cNew[3] = IRect( pcClip->m_cBounds.left, pcClip->m_cBounds.top, pcClip->m_cBounds.right, cHide.top - 1 );
		cNew[2] = IRect( pcClip->m_cBounds.left, cHide.bottom + 1, pcClip->m_cBounds.right, pcClip->m_cBounds.bottom );
		cNew[0] = IRect( pcClip->m_cBounds.left, cHide.top, cHide.left - 1, cHide.bottom );
		cNew[1] = IRect( cHide.right + 1, cHide.top, pcClip->m_cBounds.right, cHide.bottom );

		FreeClipRect( pcClip );

		for( int i = 0; i < 4; ++i )
		{		// Create clip rects for the remainding areas.
			if( cNew[i].IsValid() )
			{
				ClipRect *pcNewClip;

				pcNewClip = AllocClipRect();

				if( pcNewClip != NULL )
				{
					pcNewClip->m_cBounds = cNew[i];
					cNewList.AddRect( pcNewClip );
				}
			}
		}
	}
	m_cRects.m_pcLast = NULL;
	m_cRects.StealRects( &cNewList );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Region::Exclude( const Region & cReg )
{
	ClipRect *pcClip;

	ENUMCLIPLIST( &cReg.m_cRects, pcClip )
	{
		Exclude( pcClip->m_cBounds );
	}
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \since 0.3.7
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Region::Exclude( const Region & cReg, const IPoint & cOffset )
{
	ClipRect *pcClip;

	ENUMCLIPLIST( &cReg.m_cRects, pcClip )
	{
		Exclude( pcClip->m_cBounds + cOffset );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Region::Intersect( const Region & cReg )
{
	ClipRect *pcVClip;
	ClipRect *pcHClip;

	ClipRectList cList;

	ENUMCLIPLIST( &cReg.m_cRects, pcVClip )
	{
		ENUMCLIPLIST( &m_cRects, pcHClip )
		{
			IRect cRect = pcVClip->m_cBounds & pcHClip->m_cBounds;

			if( cRect.IsValid() )
			{
				ClipRect *pcNewClip = AllocClipRect();

				if( pcNewClip != NULL )
				{
					pcNewClip->m_cBounds = cRect;
					cList.AddRect( pcNewClip );
				}
			}
		}
	}

	Clear();

	m_cRects.StealRects( &cList );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \since 0.3.7
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Region::Intersect( const Region & cReg, const IPoint & cOffset )
{
	ClipRect *pcVClip;
	ClipRect *pcHClip;

	ClipRectList cList;

	ENUMCLIPLIST( &cReg.m_cRects, pcVClip )
	{
		ENUMCLIPLIST( &m_cRects, pcHClip )
		{
			IRect cRect = ( pcVClip->m_cBounds + cOffset ) & pcHClip->m_cBounds;

			if( cRect.IsValid() )
			{
				ClipRect *pcNewClip = AllocClipRect();

				if( pcNewClip != NULL )
				{
					pcNewClip->m_cBounds = cRect;
					cList.AddRect( pcNewClip );
				}
			}
		}
	}

	Clear();

	m_cRects.StealRects( &cList );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

IRect Region::GetBounds() const
{
	IRect cBounds( 999999, 999999, -999999, -999999 );
	ClipRect *pcClip;

	ENUMCLIPLIST( &m_cRects, pcClip )
	{
		cBounds |= pcClip->m_cBounds;
	}
	return ( cBounds );
}


class HSortCmp
{
      public:
	bool operator() ( const ClipRect * pcRect1, ClipRect * pcRect2 )
	{
		return ( pcRect1->m_cBounds.left < pcRect2->m_cBounds.left );
	}
};

class VSortCmp
{
      public:
	bool operator() ( const ClipRect * pcRect1, ClipRect * pcRect2 )
	{
		return ( pcRect1->m_cBounds.top < pcRect2->m_cBounds.top );
	}
};

void Region::Optimize()
{
	std::vector < ClipRect * >cList;

	if( m_cRects.GetCount() <= 1 )
	{
		return;
	}
	cList.reserve( m_cRects.GetCount() );

	ClipRect *pcClip;

	ENUMCLIPLIST( &m_cRects, pcClip )
	{
		cList.push_back( pcClip );
	}
	bool bSomeRemoved = true;

	while( cList.size() > 1 && bSomeRemoved )
	{
		bSomeRemoved = false;
		std::sort( cList.begin(), cList.end(  ), HSortCmp(  ) );
		for( int i = 0; i < int ( cList.size() ) - 1; )
		{
			if( cList[i]->m_cBounds.right == cList[i + 1]->m_cBounds.left - 1 && cList[i]->m_cBounds.top == cList[i + 1]->m_cBounds.top && cList[i]->m_cBounds.bottom == cList[i + 1]->m_cBounds.bottom )
			{
				cList[i]->m_cBounds.right = cList[i + 1]->m_cBounds.right;
				m_cRects.RemoveRect( cList[i + 1] );
				Region::FreeClipRect( cList[i + 1] );
				cList.erase( cList.begin() + i + 1 );
				bSomeRemoved = true;
			}
			else
			{
				++i;
			}
		}
		if( cList.size() <= 1 )
		{
			break;
		}
		std::sort( cList.begin(), cList.end(  ), VSortCmp(  ) );
		for( int i = 0; i < int ( cList.size() ) - 1; )
		{
			if( cList[i]->m_cBounds.bottom == cList[i + 1]->m_cBounds.top - 1 && cList[i]->m_cBounds.left == cList[i + 1]->m_cBounds.left && cList[i]->m_cBounds.right == cList[i + 1]->m_cBounds.right )
			{
				cList[i]->m_cBounds.bottom = cList[i + 1]->m_cBounds.bottom;
				m_cRects.RemoveRect( cList[i + 1] );
				Region::FreeClipRect( cList[i + 1] );
				cList.erase( cList.begin() + i + 1 );
				bSomeRemoved = true;
			}
			else
			{
				++i;
			}
		}
	}
}
