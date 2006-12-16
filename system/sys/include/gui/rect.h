/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Syllable Team
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

#ifndef	__F_GUI_RECT_H__
#define	__F_GUI_RECT_H__

#include <atheos/types.h>
#include "point.h"
#include <math.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

//#define	min(a,b)	(((a)<(b)) ? (a) : (b) )
//#define	max(a,b)	(((a)>(b)) ? (a) : (b) )

class IRect;

#define	DM_CM		Point(0.3937, 0.3937)
#define DM_INCH		Point(1.0, 1.0)
#define DM_PIXEL	Point(1.0, 1.0)

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class	Rect
{
public:
    float left;
    float top;
    float right;
    float bottom;

    Rect()					 { left = top = 999999.0f;	right = bottom = -999999.0f;	}
    Rect( float l, float t, float r, float b )	 { left = l; top = t; right = r; bottom = b;	}
    Rect( const Point& cMin, const Point& cMax ) { left = cMin.x; top = cMin.y; right = cMax.x; bottom = cMax.y;	}
    inline Rect( const IRect& cRect );

    ~Rect() {}

    inline Rect	Scale( const Point& cFromScale, const Point& cToScale );

    bool	IsValid() const	{ return( left <= right && top <= bottom );	 }
    void	Invalidate( void ) { left = top = 999999.0f; right = bottom = -999999.0f;	}
    bool	DoIntersect( const Point& cPoint ) const
    { return( !( cPoint.x < left || cPoint.x > right || cPoint.y < top || cPoint.y > bottom ) ); }
  
    bool DoIntersect( const Rect& cRect ) const
    { return( !( cRect.right < left || cRect.left > right || cRect.bottom < top || cRect.top > bottom ) ); }
    
    bool Includes( const Rect& cRect ) const
    {
    	return( cRect.IsValid() && IsValid() &&
    			cRect.left >= left && cRect.top >= top && cRect.right <= right && cRect.bottom <= bottom );
    }

    float  Width( void ) const	 { return( right - left ); }
    float  Height( void ) const	 { return( bottom - top ); }
    Point  Size() const 	 { return( Point( right - left, bottom - top ) ); }
    Point  LeftTop() const	 { return( Point( left, top ) );	}
    Point  RightBottom() const	 { return( Point( right, bottom ) );	}
    Rect   Bounds( void ) const  { return( Rect( 0, 0, right - left, bottom - top ) );		}
    Rect&  Floor( void )         { left = floor( left ); right = floor( right ); top = floor( top ); bottom = floor( bottom ); return( *this ); }
    Rect&  Ceil( void )          { left = ceil( left ); right = ceil( right ); top = ceil( top ); bottom = ceil( bottom ); return( *this ); }
    
    Rect&	Resize( float nLeft, float nTop, float nRight, float nBottom ) {
	left += nLeft; top += nTop; right += nRight; bottom += nBottom;
	return( *this );
    }

	Rect& MoveTo( float nLeft, float nTop ) {
		left += nLeft; top += nTop; right += nLeft; bottom += nTop;
		return( *this );
	}

    Rect operator+( const Point&	cPoint ) const
    { return( Rect( left + cPoint.x, top + cPoint.y, right + cPoint.x, bottom + cPoint.y ) ); }
    Rect operator-( const Point&	cPoint ) const
    { return( Rect( left - cPoint.x, top - cPoint.y, right - cPoint.x, bottom - cPoint.y ) ); }

    Point operator+( const Rect& cRect ) const	{ return( Point( left + cRect.left, top + cRect.top ) ); }
    Point operator-( const Rect& cRect ) const	{ return( Point( left - cRect.left, top - cRect.top ) ); }

    Rect operator&( const Rect& cRect ) const
    { return( Rect( _max( left, cRect.left ), _max( top, cRect.top ), _min( right, cRect.right ), _min( bottom, cRect.bottom ) ) ); }
    void 	operator&=( const Rect& cRect )
    { left = _max( left, cRect.left ); top = _max( top, cRect.top );
    right = _min( right, cRect.right ); bottom = _min( bottom, cRect.bottom ); }
    Rect operator|( const Rect& cRect ) const
    { return( Rect( _min( left, cRect.left ), _min( top, cRect.top ), _max( right, cRect.right ), _max( bottom, cRect.bottom ) ) ); }
    void 	operator|=( const Rect& cRect )
    { left = _min( left, cRect.left ); top = _min( top, cRect.top );
    right = _max( right, cRect.right ); bottom = _max( bottom, cRect.bottom ); }
    Rect operator|( const Point& cPoint ) const
    { return( Rect( _min( left, cPoint.x ), _min( top, cPoint.y ), _max( right, cPoint.x ), _max( bottom, cPoint.y ) ) ); }
    void operator|=( const Point& cPoint )
    { left = _min( left, cPoint.x ); top =  _min( top, cPoint.y ); right = _max( right, cPoint.x ); bottom = _max( bottom, cPoint.y ); }


    void operator+=( const Point& cPoint ) { left += cPoint.x; top += cPoint.y; right += cPoint.x; bottom += cPoint.y; }
    void operator-=( const Point& cPoint ) { left -= cPoint.x; top -= cPoint.y; right -= cPoint.x; bottom -= cPoint.y; }
  
    bool operator==( const Rect& cRect ) const
    { return( left == cRect.left && top == cRect.top && right == cRect.right && bottom == cRect.bottom ); }
  
    bool operator!=( const Rect& cRect ) const
    { return( left != cRect.left || top != cRect.top || right != cRect.right || bottom != cRect.bottom ); }
private:
    float _min( float a, float b ) const { return( (a<b) ? a : b ); }
    float _max( float a, float b ) const { return( (a>b) ? a : b ); }    
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class IRect
{
public:
    int	left;
    int	top;
    int	right;
    int	bottom;

    IRect( void )	{ left = top = 999999;	right = bottom = -999999;	}
    IRect( int l, int t, int r, int b )	{ left = l; top = t; right = r; bottom = b;	}
    IRect( const IPoint& cMin, const IPoint& cMax )	{ left = cMin.x; top = cMin.y; right = cMax.x; bottom = cMax.y;	}
    inline IRect( const Rect& cRect );
    ~IRect() {}

    bool	IsValid() const	{ return( left <= right && top <= bottom );	 }
    void	Invalidate( void ) { left = top = 999999; right = bottom = -999999;	}
    bool	DoIntersect( const IPoint& cPoint ) const
    { return( !( cPoint.x < left || cPoint.x > right || cPoint.y < top || cPoint.y > bottom ) ); }
  
    bool DoIntersect( const IRect& cRect ) const
    { return( !( cRect.right < left || cRect.left > right || cRect.bottom < top || cRect.top > bottom ) ); }
    
    bool Includes( const Rect& cRect ) const
    {
    	return( cRect.IsValid() && IsValid() &&
    			cRect.left >= left && cRect.top >= top && cRect.right <= right && cRect.bottom <= bottom );
    }

    int	   Width() const	{ return( right - left ); }
    int	   Height() const	{ return( bottom - top ); }
    IPoint Size() const 	{ return( IPoint( right - left, bottom - top ) ); }
    IPoint LeftTop() const	{ return( IPoint( left, top ) );	}
    IPoint RightBottom() const	{ return( IPoint( right, bottom ) );	}
    IRect  Bounds( void ) const	{ return( IRect( 0, 0, right - left, bottom - top ) );		}
    IRect&	Resize( int nLeft, int nTop, int nRight, int nBottom ) {
	left += nLeft; top += nTop; right += nRight; bottom += nBottom;
	return( *this );
    }
    IRect operator+( const IPoint& cPoint ) const
    { return( IRect( left + cPoint.x, top + cPoint.y, right + cPoint.x, bottom + cPoint.y ) ); }
    IRect operator-( const IPoint& cPoint ) const
    { return( IRect( left - cPoint.x, top - cPoint.y, right - cPoint.x, bottom - cPoint.y ) ); }

    IPoint operator+( const IRect& cRect ) const	{ return( IPoint( left + cRect.left, top + cRect.top ) ); }
    IPoint operator-( const IRect& cRect ) const	{ return( IPoint( left - cRect.left, top - cRect.top ) ); }

    IRect operator&( const IRect& cRect ) const
    { return( IRect( _max( left, cRect.left ), _max( top, cRect.top ), _min( right, cRect.right ), _min( bottom, cRect.bottom ) ) ); }
    void 	operator&=( const IRect& cRect )
    { left = _max( left, cRect.left ); top = _max( top, cRect.top );
    right = _min( right, cRect.right ); bottom = _min( bottom, cRect.bottom ); }
    IRect operator|( const IRect& cRect ) const
    { return( IRect( _min( left, cRect.left ), _min( top, cRect.top ), _max( right, cRect.right ), _max( bottom, cRect.bottom ) ) ); }
    void 	operator|=( const IRect& cRect )
    { left = _min( left, cRect.left ); top = _min( top, cRect.top );
    right = _max( right, cRect.right ); bottom = _max( bottom, cRect.bottom ); }
    IRect operator|( const IPoint& cPoint ) const
    { return( IRect( _min( left, cPoint.x ), _min( top, cPoint.y ), _max( right, cPoint.x ), _max( bottom, cPoint.y ) ) ); }
    void operator|=( const IPoint& cPoint )
    { left = _min( left, cPoint.x ); top =  _min( top, cPoint.y ); right = _max( right, cPoint.x ); bottom = _max( bottom, cPoint.y ); }


    void operator+=( const IPoint& cPoint ) { left += cPoint.x; top += cPoint.y; right += cPoint.x; bottom += cPoint.y; }
    void operator-=( const IPoint& cPoint ) { left -= cPoint.x; top -= cPoint.y; right -= cPoint.x; bottom -= cPoint.y; }
  
    bool operator==( const IRect& cRect ) const
    { return( left == cRect.left && top == cRect.top && right == cRect.right && bottom == cRect.bottom ); }
  
    bool operator!=( const IRect& cRect ) const
    { return( left != cRect.left || top != cRect.top || right != cRect.right || bottom != cRect.bottom ); }
private:
    int _min( int a, int b ) const { return( (a<b) ? a : b ); }
    int _max( int a, int b ) const { return( (a>b) ? a : b ); }    

};

Rect::Rect( const IRect& cRect )
{
    left   = float(cRect.left);
    top    = float(cRect.top);
    right  = float(cRect.right);
    bottom = float(cRect.bottom);
}

Rect Rect::Scale( const Point& cFromScale, const Point& cToScale )
{
	Rect r;
	r.left = left*cFromScale.x/cToScale.x;
	r.top = top*cFromScale.y/cToScale.y;
	r.right = right*cFromScale.x/cToScale.x;
	r.bottom = bottom*cFromScale.y/cToScale.y;
	return r;
}

IRect::IRect( const Rect& cRect )
{
    left   = int(cRect.left);
    top    = int(cRect.top);
    right  = int(cRect.right);
    bottom = int(cRect.bottom);
}


} // End of namespace

#endif // __F_GUI_RECT_H__
