/****************************************************************************
** $Id$
**
** Definition of QRect class
**
** Created : 931028
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
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
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

#ifndef QRECT_H
#define QRECT_H

#ifndef QT_H
#include "qsize.h"
#endif // QT_H

#if defined(topLeft)
#error "Macro definition of topLeft conflicts with QRect"
// don't just silently undo people's defines: #undef topLeft
#endif

class Q_EXPORT QRect					// rectangle class
{
public:
    QRect()	{ x1 = y1 = 0; x2 = y2 = -1; }
    QRect( const QPoint &topleft, const QPoint &bottomright );
    QRect( const QPoint &topleft, const QSize &size );
    QRect( int left, int top, int width, int height );

    bool   isNull()	const;
    bool   isEmpty()	const;
    bool   isValid()	const;
    QRect  normalize()	const;

    int	   left()	const;
    int	   top()	const;
    int	   right()	const;
    int	   bottom()	const;

    QCOORD &rLeft();
    QCOORD &rTop();
    QCOORD &rRight();
    QCOORD &rBottom();
	
    int	   x()		const;
    int	   y()		const;
    void   setLeft( int pos );
    void   setTop( int pos );
    void   setRight( int pos );
    void   setBottom( int pos );
    void   setX( int x );
    void   setY( int y );

    QPoint topLeft()	 const;
    QPoint bottomRight() const;
    QPoint topRight()	 const;
    QPoint bottomLeft()	 const;
    QPoint center()	 const;

    void   rect( int *x, int *y, int *w, int *h ) const;
    void   coords( int *x1, int *y1, int *x2, int *y2 ) const;

    void   moveTopLeft( const QPoint &p );
    void   moveBottomRight( const QPoint &p );
    void   moveTopRight( const QPoint &p );
    void   moveBottomLeft( const QPoint &p );
    void   moveCenter( const QPoint &p );
    void   moveBy( int dx, int dy );

    void   setRect( int x, int y, int w, int h );
    void   setCoords( int x1, int y1, int x2, int y2 );

    QSize  size()	const;
    int	   width()	const;
    int	   height()	const;
    void   setWidth( int w );
    void   setHeight( int h );
    void   setSize( const QSize &s );

    QRect  operator|(const QRect &r) const;
    QRect  operator&(const QRect &r) const;
    QRect&  operator|=(const QRect &r);
    QRect&  operator&=(const QRect &r);

    bool   contains( const QPoint &p, bool proper=FALSE ) const;
    bool   contains( int x, int y, bool proper=FALSE ) const;
    bool   contains( const QRect &r, bool proper=FALSE ) const;
    QRect  unite( const QRect &r ) const;
    QRect  intersect( const QRect &r ) const;
    bool   intersects( const QRect &r ) const;

    friend Q_EXPORT bool operator==( const QRect &, const QRect & );
    friend Q_EXPORT bool operator!=( const QRect &, const QRect & );

private:
#if defined(_OS_MAC_)
    QCOORD y1;
    QCOORD x1;
    QCOORD y2;
    QCOORD x2;
#else
    QCOORD x1;
    QCOORD y1;
    QCOORD x2;
    QCOORD y2;
#endif
};

Q_EXPORT bool operator==( const QRect &, const QRect & );
Q_EXPORT bool operator!=( const QRect &, const QRect & );


/*****************************************************************************
  QRect stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_EXPORT QDataStream &operator<<( QDataStream &, const QRect & );
Q_EXPORT QDataStream &operator>>( QDataStream &, QRect & );
#endif

/*****************************************************************************
  QRect inline member functions
 *****************************************************************************/

inline QRect::QRect( int left, int top, int width, int height )
{
    x1 = (QCOORD)left;
    y1 = (QCOORD)top;
    x2 = (QCOORD)(left+width-1);
    y2 = (QCOORD)(top+height-1);
}

inline bool QRect::isNull() const
{ return x2 == x1-1 && y2 == y1-1; }

inline bool QRect::isEmpty() const
{ return x1 > x2 || y1 > y2; }

inline bool QRect::isValid() const
{ return x1 <= x2 && y1 <= y2; }

inline int QRect::left() const
{ return x1; }

inline int QRect::top() const
{ return y1; }

inline int QRect::right() const
{ return x2; }

inline int QRect::bottom() const
{ return y2; }

inline QCOORD &QRect::rLeft()
{ return x1; }

inline QCOORD & QRect::rTop()
{ return y1; }

inline QCOORD & QRect::rRight()
{ return x2; }

inline QCOORD & QRect::rBottom()
{ return y2; }

inline int QRect::x() const
{ return x1; }

inline int QRect::y() const
{ return y1; }

inline void QRect::setLeft( int pos )
{ x1 = (QCOORD)pos; }

inline void QRect::setTop( int pos )
{ y1 = (QCOORD)pos; }

inline void QRect::setRight( int pos )
{ x2 = (QCOORD)pos; }

inline void QRect::setBottom( int pos )
{ y2 = (QCOORD)pos; }

inline void QRect::setX( int x )
{ x1 = (QCOORD)x; }

inline void QRect::setY( int y )
{ y1 = (QCOORD)y; }

inline QPoint QRect::topLeft() const
{ return QPoint(x1, y1); }

inline QPoint QRect::bottomRight() const
{ return QPoint(x2, y2); }

inline QPoint QRect::topRight() const
{ return QPoint(x2, y1); }

inline QPoint QRect::bottomLeft() const
{ return QPoint(x1, y2); }

inline QPoint QRect::center() const
{ return QPoint((x1+x2)/2, (y1+y2)/2); }

inline int QRect::width() const
{ return  x2 - x1 + 1; }

inline int QRect::height() const
{ return  y2 - y1 + 1; }

inline QSize QRect::size() const
{ return QSize(x2-x1+1, y2-y1+1); }

inline bool QRect::contains( int x, int y, bool proper ) const
{
    if ( proper )
        return x > x1 && x < x2 &&
               y > y1 && y < y2;
    else
        return x >= x1 && x <= x2 &&
               y >= y1 && y <= y2;
}

#endif // QRECT_H
