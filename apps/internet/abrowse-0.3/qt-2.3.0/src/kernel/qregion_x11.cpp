/****************************************************************************
** $Id$
**
** Implementation of QRegion class for X11
**
** Created : 940729
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Unix/X11 may use this file in accordance with the Qt Commercial
** License Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qregion.h"
#include "qpointarray.h"
#include "qbuffer.h"
#include "qimage.h"
#include "qbitmap.h"
#include "qt_x11.h"

// NOT REVISED

static QRegion *empty_region = 0;

static void cleanup_empty_region()
{
    delete empty_region;
    empty_region = 0;
}


/*!
  Constructs a null region.
  \sa isNull()
*/

QRegion::QRegion()
{
    if ( !empty_region ) {			// avoid too many allocs
	qAddPostRoutine( cleanup_empty_region );
	empty_region = new QRegion( TRUE );
	CHECK_PTR( empty_region );
    }
    data = empty_region->data;
    data->ref();
}

/*!
  Internal constructor that creates a null region.
*/

QRegion::QRegion( bool is_null )
{
    data = new QRegionData;
    CHECK_PTR( data );
    data->rgn = XCreateRegion();
    data->is_null = is_null;
}

/*!
\overload

If the rectangle is invalid, the rectangle will be normalized() before the region is created. This semantics will change in Qt-3
and an invalid rectangle will construct an empty region.

\sa QRect::isValid() QRect::normalize()
 */

QRegion::QRegion( const QRect &r, RegionType t )
{
    QRect rr = r.normalize();
    data = new QRegionData;
    CHECK_PTR( data );
    data->is_null = FALSE;
    if ( t == Rectangle ) {			// rectangular region
	data->rgn = XCreateRegion();
	XRectangle xr;
	xr.x = rr.x();
	xr.y = rr.y();
	xr.width  = rr.width();
	xr.height = rr.height();
	XUnionRectWithRegion( &xr, data->rgn, data->rgn );
    } else if ( t == Ellipse ) {		// elliptic region
	QPointArray a;
	a.makeEllipse( rr.x(), rr.y(), rr.width(), rr.height() );
	data->rgn = XPolygonRegion( (XPoint*)a.shortPoints(), a.size(),
				    EvenOddRule );
    }
}


/*!
  Constructs a polygon region from the point array \a a.

  If \a winding is TRUE, the polygon
  region is filled using the winding algorithm, otherwise the default
  even-odd fill algorithm is used.

  This constructor may create complex regions that will slow
  down painting when used.
*/

QRegion::QRegion( const QPointArray &a, bool winding )
{
    data = new QRegionData;
    CHECK_PTR( data );
    data->is_null = FALSE;
    data->rgn = XPolygonRegion( (XPoint*)a.shortPoints(), a.size(),
				winding ? WindingRule : EvenOddRule );
}


/*!
  Constructs a new region which is equal to \a r.
*/

QRegion::QRegion( const QRegion &r )
{
    data = r.data;
    data->ref();
}

Region qt_x11_bitmapToRegion(const QBitmap& bitmap)
{
    Region region = XCreateRegion();
    QImage image = bitmap.convertToImage();

    XRectangle xr;

#define AddSpan \
	{ \
	    xr.x = prev1; \
	    xr.y = y; \
	    xr.width = x-prev1-1; \
	    xr.height = 1; \
	    XUnionRectWithRegion( &xr, region, region ); \
	}

    // deal with 0<->1 problem (not on X11 anymore)
    int zero=0;//(qGray(image.color(0)) < qGray(image.color(1)) ? 0x00 : 0xff);
    bool little = image.bitOrder() == QImage::LittleEndian;

    int x, y;
    for (y=0; y<image.height(); y++) {
	uchar *line = image.scanLine(y);
	int w = image.width();
	uchar all=zero;
	int prev1 = -1;
	for (x=0; x<w; ) {
	    uchar byte = line[x/8];
	    if ( x>w-8 || byte!=all ) {
		if ( little ) {
		    for ( int b=8; b>0 && x<w; b-- ) {
			if ( !(byte&0x01) == !all ) {
			    // More of the same
			} else {
			    // A change.
			    if ( all!=zero ) {
				AddSpan;
				all = zero;
			    } else {
				prev1 = x;
				all = ~zero;
			    }
			}
			byte >>= 1;
			x++;
		    }
		} else {
		    for ( int b=8; b>0 && x<w; b-- ) {
			if ( !(byte&0x80) == !all ) {
			    // More of the same
			} else {
			    // A change.
			    if ( all!=zero ) {
				AddSpan;
				all = zero;
			    } else {
				prev1 = x;
				all = ~zero;
			    }
			}
			byte <<= 1;
			x++;
		    }
		}
	    } else {
		x+=8;
	    }
	}
	if ( all != zero ) {
	    AddSpan;
	}
    }

    return region;
}



/*!
  Constructs a region from the bitmap \a bm.

  The resulting region consists of the pixels in \a bm that are \c
  color1, as if each pixel was a 1 by 1 rectangle.

  This constructor may create complex regions that will slow
  down painting when used. Note that drawing masked pixmaps
  can be done much faster using QPixmap::setMask().

*/
QRegion::QRegion( const QBitmap & bm )
{
    data = new QRegionData;
    CHECK_PTR( data );
    data->is_null = FALSE;
    data->rgn = qt_x11_bitmapToRegion(bm);
}

/*!
  Destructs the region.
*/

QRegion::~QRegion()
{
    if ( data->deref() ) {
	XDestroyRegion( data->rgn );
	delete data;
    }
}


/*!
  Assigns \a r to this region and returns a reference to the
  region.

*/

QRegion &QRegion::operator=( const QRegion &r )
{
    r.data->ref();				// beware of r = r
    if ( data->deref() ) {
	XDestroyRegion( data->rgn );
	delete data;
    }
    data = r.data;
    return *this;
}


/*!
  Returns a \link shclass.html deep copy\endlink of the region.

  \sa detach()
*/

QRegion QRegion::copy() const
{
    QRegion r( data->is_null );
    XUnionRegion( data->rgn, r.data->rgn, r.data->rgn );
    return r;
}

/*!
  Returns TRUE if the region is a null region, otherwise FALSE.

  A null region is a region that has not been initialized. A
  null region is always empty.

  \sa isEmpty()
*/

bool QRegion::isNull() const
{
    return data->is_null;
}


/*!
  Returns TRUE if the region is empty, or FALSE if it is non-empty.
  An empty region is a region that contains no points.

  Example:
  \code
    QRegion r1( 10, 10, 20, 20 );
    QRegion r2( 40, 40, 20, 20 );
    QRegion r3;
    r1.isNull();		// FALSE
    r1.isEmpty();		// FALSE
    r3.isNull();		// TRUE
    r3.isEmpty();		// TRUE
    r3 = r1.intersect( r2 );	// r3 = intersection of r1 and r2
    r3.isNull();		// FALSE
    r3.isEmpty();		// TRUE
    r3 = r1.unite( r2 );	// r3 = union of r1 and r2
    r3.isNull();		// FALSE
    r3.isEmpty();		// FALSE
  \endcode

  \sa isNull()
*/

bool QRegion::isEmpty() const
{
    return data->is_null || XEmptyRegion( data->rgn );
}


/*!
  Returns TRUE if the region contains the point \a p, or FALSE if \a p is
  outside the region.
*/

bool QRegion::contains( const QPoint &p ) const
{
    return XPointInRegion( data->rgn, p.x(), p.y() );
}

/*!
  Returns TRUE if the region overlaps the rectangle \a r, or FALSE if \a r is
  completely outside the region.
*/

bool QRegion::contains( const QRect &r ) const
{
    return XRectInRegion( data->rgn, r.left(), r.top(),
			  r.width(), r.height() ) != RectangleOut;
}


/*!
  Translates (moves) the region \a dx along the X axis and \a dy along the Y axis.
*/

void QRegion::translate( int dx, int dy )
{
    if ( data == empty_region->data )
	return;
    detach();
    XOffsetRegion( data->rgn, dx, dy );
}


/*!
  Returns a region which is the union of this region and \a r.

    <img src=runion.png>

  The figure shows the union of two elliptical regions.
*/

QRegion QRegion::unite( const QRegion &r ) const
{
    QRegion result( FALSE );
    XUnionRegion( data->rgn, r.data->rgn, result.data->rgn );
    return result;
}

/*!
  Returns a region which is the intersection of this region and \a r.

  <img src=rintersect.png>

  The figure shows the intersection of two elliptical regions.
*/

QRegion QRegion::intersect( const QRegion &r ) const
{
    QRegion result( FALSE );
    XIntersectRegion( data->rgn, r.data->rgn, result.data->rgn );
    return result;
}

/*!
  Returns a region which is \a r subtracted from this region.

  <img src=rsubtract.png>

  The figure shows the result when the ellipse on the right is subtracted
  from the ellipse on the left. (\c left-right )
*/

QRegion QRegion::subtract( const QRegion &r ) const
{
    QRegion result( FALSE );
    XSubtractRegion( data->rgn, r.data->rgn, result.data->rgn );
    return result;
}

/*!
  Returns a region which is the exclusive or (XOR) of this region and \a r.

  <img src=rxor.png>

  The figure shows the exclusive or of two elliptical regions.
*/

QRegion QRegion::eor( const QRegion &r ) const
{
    QRegion result( FALSE );
    XXorRegion( data->rgn, r.data->rgn, result.data->rgn );
    return result;
}


/*!
  Returns the bounding rectangle of this region.
  An empty region gives a rectangle that is  QRect::isNull().
*/

QRect QRegion::boundingRect() const
{
    XRectangle r;
    XClipBox( data->rgn, &r );
    return QRect( r.x, r.y, r.width, r.height );
}


/*
  This is how X represents regions internally.
*/

struct BOX {
    short x1, x2, y1, y2;
};

struct _XRegion {
    long size;
    long numRects;
    BOX *rects;
    BOX  extents;
};


/*!
  Returns an array of non-overlapping rectangles that make up the region.

  The union of all the rectangles is equal to the original region.
*/

QArray<QRect> QRegion::rects() const
{
    QArray<QRect> a( (int)data->rgn->numRects );
    BOX *r = data->rgn->rects;
    for ( int i=0; i<(int)a.size(); i++ ) { 
	a[i].setCoords( r->x1, r->y1, r->x2-1, r->y2-1);
	r++;
    }
    return a;
}

/*!
  Sets the region to be the given set of rectangles.  The rectangles
  \e must be optimal Y-X sorted bands as follows:
   <ul>
    <li> The rectangles must not intersect
    <li> All rectangles with a given top coordinate must have the same height.
    <li> No two rectangles may abut horizontally (they should be combined
		into a single wider rectangle in that case).
    <li> The rectangles must be sorted ascendingly by Y as the major sort key
		and X as the minor sort key.
   </ul>
  \internal
  Only some platforms have that restriction (QWS).
*/
void QRegion::setRects( const QRect *rects, int num )
{
    // Could be optimized
    *this = QRegion();
    for (int i=0; i<num; i++)
	*this |= rects[i];
}

/*!
  Returns TRUE if the region is equal to \a r, or FALSE if the regions are
  different.
*/

bool QRegion::operator==( const QRegion &r ) const
{
    return data == r.data ?
	TRUE : XEqualRegion( data->rgn, r.data->rgn );
}

/*!
  \fn bool QRegion::operator!=( const QRegion &r ) const
  Returns TRUE if the region is different from \a r, or FALSE if the regions
  are equal.
*/
