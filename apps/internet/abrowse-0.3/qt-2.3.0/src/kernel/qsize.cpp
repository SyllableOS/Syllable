/****************************************************************************
** $Id$
**
** Implementation of QSize class
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

#include "qsize.h"
#include "qdatastream.h"


// NOT REVISED
/*!
  \class QSize qsize.h
  \brief The QSize class defines the size of a two-dimensional object.

  \ingroup drawing

  A size is specified by a width and a height.

  The coordinate type is QCOORD (defined in qwindowdefs.h as \c int).
  The minimum value of QCOORD is QCOORD_MIN (-2147483648) and the maximum
  value is  QCOORD_MAX (2147483647).

  \sa QPoint, QRect
*/


/*****************************************************************************
  QSize member functions
 *****************************************************************************/

/*!
  \fn QSize::QSize()
  Constructs a size with invalid (negative) width and height.
*/

/*!
  \fn QSize::QSize( int w, int h )
  Constructs a size with width \e w and height \e h.
*/

/*!
  \fn bool QSize::isNull() const
  Returns TRUE if the width is 0 and the height is 0, otherwise FALSE.
*/

/*!
  \fn bool QSize::isEmpty() const
  Returns TRUE if the width is <= 0 or the height is <= 0,
  otherwise FALSE.
*/

/*!
  \fn bool QSize::isValid() const
  Returns TRUE if the width is equal to or greater than 0 and the height is
  equal to or greater than 0, otherwise FALSE.
*/

/*!
  \fn int QSize::width() const
  Returns the width.
  \sa height()
*/

/*!
  \fn int QSize::height() const
  Returns the height.
  \sa width()
*/

/*!
  \fn void QSize::setWidth( int w )
  Sets the width to \e w.
  \sa width(), setHeight()
*/

/*!
  \fn void QSize::setHeight( int h )
  Sets the height to \e h.
  \sa height(), setWidth()
*/

/*!
  Swaps the values of width and height.
*/

void QSize::transpose()
{
    QCOORD tmp = wd;
    wd = ht;
    ht = tmp;
}

/*!
  \fn QCOORD &QSize::rwidth()
  Returns a reference to the width.

  Using a reference makes it possible to directly manipulate the width.

  Example:
  \code
    QSize s( 100, 10 );
    s.rwidth() += 20;		// s becomes (120,10)
  \endcode

  \sa rheight()
*/

/*!
  \fn QCOORD &QSize::rheight()
  Returns a reference to the height.

  Using a reference makes it possible to directly manipulate the height.

  Example:
  \code
    QSize s( 100, 10 );
    s.rheight() += 5;		// s becomes (100,15)
  \endcode

  \sa rwidth()
*/

/*!
  \fn QSize &QSize::operator+=( const QSize &s )

  Adds \e s to the size and returns a reference to this size.

  Example:
  \code
    QSize s(  3, 7 );
    QSize r( -1, 4 );
    s += r;			// s becomes (2,11)
\endcode
*/

/*!
  \fn QSize &QSize::operator-=( const QSize &s )

  Subtracts \e s from the size and returns a reference to this size.

  Example:
  \code
    QSize s(  3, 7 );
    QSize r( -1, 4 );
    s -= r;			// s becomes (4,3)
  \endcode
*/

/*!
  \fn QSize &QSize::operator*=( int c )
  Multiplies both the width and height with \e c and returns a reference to
  the size.
*/

/*!
  \fn QSize &QSize::operator*=( double c )

  Multiplies both the width and height with \e c and returns a reference to
  the size.

  Note that the result is truncated.
*/

/*!
  \fn bool operator==( const QSize &s1, const QSize &s2 )
  \relates QSize
  Returns TRUE if \e s1 and \e s2 are equal, or FALSE if they are different.
*/

/*!
  \fn bool operator!=( const QSize &s1, const QSize &s2 )
  \relates QSize
  Returns TRUE if \e s1 and \e s2 are different, or FALSE if they are equal.
*/

/*!
  \fn QSize operator+( const QSize &s1, const QSize &s2 )
  \relates QSize
  Returns the sum of \e s1 and \e s2; each component is added separately.
*/

/*!
  \fn QSize operator-( const QSize &s1, const QSize &s2 )
  \relates QSize
  Returns \e s2 subtracted from \e s1; each component is
  subtracted separately.
*/

/*!
  \fn QSize operator*( const QSize &s, int c )
  \relates QSize
  Multiplies \e s by \e c and returns the result.
*/

/*!
  \fn QSize operator*( int c, const QSize &s )
  \relates QSize
  Multiplies \e s by \e c and returns the result.
*/

/*!
  \fn QSize operator*( const QSize &s, double c )
  \relates QSize
  Multiplies \e s by \e c and returns the result.
*/

/*!
  \fn QSize operator*( double c, const QSize &s )
  \relates QSize
  Multiplies \e s by \e c and returns the result.
*/

/*!
  \fn QSize &QSize::operator/=( int c )
  Divides both the width and height by \e c and returns a reference to the
  size.
*/

/*!
  \fn QSize &QSize::operator/=( double c )

  Divides both the width and height by \e c and returns a reference to the
  size.

  Note that the result is truncated.
*/

/*!
  \fn QSize operator/( const QSize &s, int c )
  \relates QSize
  Divides \e s by \e c and returns the result.
*/

/*!
  \fn QSize operator/( const QSize &s, double c )
  \relates QSize
  Divides \e s by \e c and returns the result.

  Note that the result is truncated.
*/

/*!
  \fn QSize QSize::expandedTo( const QSize & otherSize ) const

  Returns a size with the maximum width and height of this size and
  \a otherSize.
*/

/*!
  \fn QSize QSize::boundedTo( const QSize & otherSize ) const

  Returns a size with the minimum width and height of this size and
  \a otherSize.
*/


void QSize::warningDivByZero()
{
#if defined(CHECK_MATH)
    qWarning( "QSize: Division by zero error" );
#endif
}


/*****************************************************************************
  QSize stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
  \relates QSize
  Writes the size to the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<( QDataStream &s, const QSize &sz )
{
    if ( s.version() == 1 )
	s << (Q_INT16)sz.width() << (Q_INT16)sz.height();
    else
	s << (Q_INT32)sz.width() << (Q_INT32)sz.height();
    return s;
}

/*!
  \relates QSize
  Reads the size from the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>( QDataStream &s, QSize &sz )
{
    if ( s.version() == 1 ) {
	Q_INT16 w, h;
	s >> w;  sz.rwidth() = w;
	s >> h;  sz.rheight() = h;
    }
    else {
	Q_INT32 w, h;
	s >> w;  sz.rwidth() = w;
	s >> h;  sz.rheight() = h;
    }
    return s;
}
#endif // QT_NO_DATASTREAM
