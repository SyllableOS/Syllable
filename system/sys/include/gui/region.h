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

#ifndef	__F_GUI_REGION_H__
#define	__F_GUI_REGION_H__

#include <atheos/types.h>
#include <gui/gfxtypes.h>

#include <gui/rect.h>
#include <gui/point.h>

#include <util/locker.h>

#define ENUMCLIPLIST( list, node ) for ( node = (list)->m_pcFirst ; node != NULL ; node = node->m_pcNext )

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

struct	ClipRect
{
    ClipRect() {}
    ClipRect( const IRect& cFrame, const IPoint& cOffset ) : m_cBounds(cFrame), m_cMove(cOffset) {}

    ClipRect* m_pcNext;
    ClipRect* m_pcPrev;
    IRect     m_cBounds;
    IPoint    m_cMove;	// Used to sort rectangles, so no blits destroy needed areas

private:
    ClipRect& operator=( const ClipRect& );
    ClipRect( const ClipRect& );
};

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class ClipRectList
{
public:
    ClipRectList();
    ~ClipRectList();
    
    void	Clear();
    void	AddRect( ClipRect* pcRect );
    void	RemoveRect( ClipRect* pcRect );
    ClipRect*	RemoveHead();
    void 	StealRects( ClipRectList* pcList );
    int		GetCount() const { return( m_nCount ); }
   
    ClipRect*	m_pcFirst;
    ClipRect*	m_pcLast;
    int		m_nCount;

private:
    ClipRectList& operator=( const ClipRectList& );
    ClipRectList( const ClipRectList& );
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Region
{
public:
    Region();
    Region( const IRect& cRect );
    Region( const Region& cReg );
    Region( const Region& cReg, const IRect& cRect, bool bNormalize );

    ~Region();

    void		Set( const IRect& cRect );
    void		Set( const Region& cRect );
    void		Clear( void );
    bool		IsEmpty() const { return( m_cRects.GetCount() == 0 ); }
    int			GetClipCount() const { return( m_cRects.GetCount() ); } //! /since 0.3.7
    void		Include( const IRect& cRect );
    void		Intersect( const Region& cReg );
    void		Intersect( const Region& cReg, const IPoint& cOffset );

    void		Exclude( const IRect& cRect );
    void		Exclude( const Region& cReg );
    void		Exclude( const Region& cReg, const IPoint& cOffset );

    ClipRect*		AddRect( const IRect& cRect );
    
    Region*		Clone( const IRect& cRectangle, bool bNormalize );
    void		Optimize();

    static void		FreeClipRect( ClipRect* pcRect );
    static ClipRect*	AllocClipRect();
    IRect		GetBounds() const;
    ClipRectList	m_cRects;
private:
    Region& operator=( const Region& );

    static void** s_pFirstClip;
    static Locker s_cClipListMutex;
  
};

} // End of namespace

#endif	// __F_GUI_REGION_H__
