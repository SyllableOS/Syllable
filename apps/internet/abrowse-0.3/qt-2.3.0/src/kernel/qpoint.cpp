/****************************************************************************
** $Id$
**
** Implementation of QPoint class
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

#include "qpoint.h"
#include "qdatastream.h"


// REVISED: paul
/*!
  \class QPoint qpoint.h
  \brief The QPoint class defines a point in the plane.

  \ingroup drawing

  A point is specified by an x coordinate and a y coordinate.

  The coordinate type is \c QCOORD (a 32-bit integer).
  The minimum value of \c QCOORD is \c QCOORD_MIN (-2147483648) and the maximum
  value is  \c QCOORD_MAX (2147483647).

  The coordinates are accessed by the functions x() and y(), they can
  be set by setX() and setY(), or by the reference functions rx() and ry().
  
  Given a point \e p, the following statements are all equivalent:
  \code
     p.setX( p.x() + 1 );
     p += QPoint( 1, 0 );
     p.rx()++;
  \endcode
  
  
  A QPoint can also be used as a vector.  Addition and subtraction of
  QPoint are defined as for vectors (each component is added
  separately). You can divide or multiply a QPoint by an \c int or a
  \c double. The function manhattanLength() gives an inexpensive
  approximation to the length of the QPoint interpreted as a vector.

  Example:
  \code
     //QPoint oldPos is defined somewhere else 
     MyWidget::mouseMoveEvent( QMouseEvent *e )
     {
        QPoint vector = e->pos() - oldPos;
	if ( vector.manhattanLength() > 3 )
	   ... //mouse has moved more than 3 pixels since oldPos
     }
  \endcode
  
  QPoints can be compared for equality or inequality, and they can be
  written to and read from a QStream.
  
  \sa QSize, QRect
*/


/*****************************************************************************
  QPoint member functions
 *****************************************************************************/

/*!
  \fn QPoint::QPoint()
  Constructs a point with coordinates (0,0) (isNull() returns TRUE).
*/

/*!
  \fn QPoint::QPoint( int xpos, int ypos )
  Constructs a point with the x value  \a xpos and y value \a ypos.
*/

/*!
  \fn bool QPoint::isNull() const
  Returns TRUE if both the x value and the y value are 0.
*/

/*!
  \fn int QPoint::x() const
  Returns the x coordinate of the point.
  \sa setX() y()
*/

/*!
  \fn int QPoint::y() const
  Returns the y coordinate of the point.
  \sa setY() x()
*/

/*!
  \fn void QPoint::setX( int x )
  Sets the x coordinate of the point to \a x.
  \sa x() setY()
*/

/*!
  \fn void QPoint::setY( int y )
  Sets the y coordinate of the point to \a y.
  \sa y() setX()
*/


/*!
  \fn QCOORD &QPoint::rx()
  Returns a reference to the x coordinate of the point.

  Using a reference makes it possible to directly manipulate x.

  Example:
  \code
    QPoint p( 1, 2 );
    p.rx()--;			// p becomes (0,2)
  \endcode

  \sa ry()
*/

/*!
  \fn QCOORD &QPoint::ry()
  Returns a reference to the y coordinate of the point.

  Using a reference makes it possible to directly manipulate y.

  Example:
  \code
    QPoint p( 1, 2 );
    p.ry()++;			// p becomes (1,3)
  \endcode

  \sa rx()
*/


/*!
  \fn QPoint &QPoint::operator+=( const QPoint &p )
  Adds \a p to the point and returns a reference to this point.

  Example:
  \code
    QPoint p(  3, 7 );
    QPoint q( -1, 4 );
    p += q;			// p becomes (2,11)
  \endcode
*/

/*!
  \fn QPoint &QPoint::operator-=( const QPoint &p )
  Subtracts \a p from the point and returns a reference to this point.

  Example:
  \code
    QPoint p(  3, 7 );
    QPoint q( -1, 4 );
    p -= q;			// p becomes (4,3)
  \endcode
*/

/*!
  \fn QPoint &QPoint::operator*=( int c )
  Multiplies both x and y with \a c, and return a reference to this point.

  Example:
  \code
    QPoint p( -1, 4 );
    p *= 2;			// p becomes (-2,8)
  \endcode
*/

/*!
  \fn QPoint &QPoint::operator*=( double c )
  Multiplies both x and y with \a c, and return a reference to this point.

  Example:
  \code
    QPoint p( -1, 4 );
    p *= 2.5;			// p becomes (-3,10)
  \endcode

  Note that the result is truncated.
*/


/*!
  \fn bool operator==( const QPoint &p1, const QPoint &p2 )
  \relates QPoint
  Returns TRUE if \a p1 and \a p2 are equal, or FALSE if they are different.
*/

/*!
  \fn bool operator!=( const QPoint &p1, const QPoint &p2 )
  \relates QPoint
  Returns TRUE if \a p1 and \a p2 are different, or FALSE if they are equal.
*/

/*!
  \fn QPoint operator+( const QPoint &p1, const QPoint &p2 )
  \relates QPoint
  Returns the sum of \a p1 and \a p2; each component is added separately.
*/

/*!
  \fn QPoint operator-( const QPoint &p1, const QPoint &p2 )
  \relates QPoint
  Returns \a p2 subtracted from \a p1; each component is
  subtracted separately.
*/

/*!
  \fn QPoint operator*( const QPoint &p, int c )
  \relates QPoint
  Returns the QPoint formed by multiplying both components of \a p
  by \a c.
*/

/*!
  \fn QPoint operator*( int c, const QPoint &p )
  \relates QPoint
  Returns the QPoint formed by multiplying both components of \a p
  by \a c.
*/

/*!
  \fn QPoint operator*( const QPoint &p, double c )
  \relates QPoint
  Returns the QPoint formed by multiplying both components of \a p
  by \a c.

  Note that the result is truncated.
*/

/*!
  \fn QPoint operator*( double c, const QPoint &p )
  \relates QPoint
  Returns the QPoint formed by multiplying both components of \a p
  by \a c.

  Note that the result is truncated.
*/

/*!
  \fn QPoint operator-( const QPoint &p ) 
  \relates QPoint 
  
  Returns the QPoint formed by changing the sign of both components of
  \a p, equivalent to <code>QPoint(0,0) - p</code>.
*/

/*!
  \fn QPoint &QPoint::operator/=( int c )

  Divides both x and y by \a c, and return a reference to this point.

  Example:
  \code
    QPoint p( -2, 8 );
    p /= 2;			// p becomes (-1,4)
  \endcode
*/

/*!
  \fn QPoint &QPoint::operator/=( double c )

  Divides both x and y by \a c, and return a reference to this point.

  Example:
  \code
    QPoint p( -3, 10 );
    p /= 2.5;			// p becomes (-1,4)
  \endcode

  Note that the result is truncated.
*/

/*!
  \fn QPoint operator/( const QPoint &p, int c )
  \relates QPoint
  Returns the QPoint formed by dividing both components of \a p
  by \a c.
*/

/*!
  \fn QPoint operator/( const QPoint &p, double c )
  \relates QPoint

  Returns the QPoint formed by dividing both components of \a p
  by \a c.

  Note that the result is truncated.
*/


void QPoint::warningDivByZero()
{
#if defined(CHECK_MATH)
    qWarning( "QPoint: Division by zero error" );
#endif
}


/*****************************************************************************
  QPoint stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
  \relates QPoint
  Writes a QPoint to the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<( QDataStream &s, const QPoint &p )
{
    if ( s.version() == 1 )
	s << (Q_INT16)p.x() << (Q_INT16)p.y();
    else
	s << (Q_INT32)p.x() << (Q_INT32)p.y();
    return s;
}

/*!
  \relates QPoint
  Reads a QPoint from the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>( QDataStream &s, QPoint &p )
{
    if ( s.version() == 1 ) {
	Q_INT16 x, y;
	s >> x;  p.rx() = x;
	s >> y;  p.ry() = y;
    }
    else {
	Q_INT32 x, y;
	s >> x;  p.rx() = x;
	s >> y;  p.ry() = y;
    }
    return s;
}
#endif // QT_NO_DATASTREAM
/*!
  Returns the sum of the absolute values of x() and y(), traditionally
  known as the "Manhattan length" of the vector from the origin to the
  point. The tradition arises since such distances apply
  to travelers who can only travel on a rectangular grid, like the streets
  of Manhattan.

  This is a useful approximation to the true length,
  sqrt(pow(x(),2)+pow(y(),2)).
*/
int QPoint::manhattanLength() const
{
    return QABS(x())+QABS(y());
}
