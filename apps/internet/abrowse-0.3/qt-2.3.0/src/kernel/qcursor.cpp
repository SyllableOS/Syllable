/****************************************************************************
** $Id$
**
** Implementation of QCursor class
**
** Created : 940220
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

#include "qcursor.h"

#ifndef QT_NO_CURSOR

#include "qbitmap.h"
#include "qimage.h"
#include "qdatastream.h"


// NOT REVISED
/*!
  \class QCursor qcursor.h

  \brief The QCursor class provides a mouse cursor with an arbitrary shape.

  \ingroup environment
  \ingroup shared

  This class is used for mainly two things; to create mouse cursors to be
  associated with widgets and to get and set the position of the mouse
  cursor.

  Qt has a number of standard cursor shapes, but you can also make
  custom cursor shapes based on a QBitmap, a mask and a hotspot.

  To associate a cursor with a widget, use QWidget::setCursor().  To
  associate a cursor with all widgets (normally for a short period of
  time), use QApplication::setOverrideCursor().

  To set a cursor shape use QCursor::setShape() or use the QCursor constructor
  which takes the shape as argument, or use one of the \e predefined cursors:

  <ul>
  <li> \c arrowCursor - standard arrow cursor
  <li> \c upArrowCursor - upwards arrow
  <li> \c crossCursor - crosshair
  <li> \c waitCursor - hourglass/watch
  <li> \c ibeamCursor - ibeam/text entry
  <li> \c sizeVerCursor - vertical resize
  <li> \c sizeHorCursor - horizontal resize
  <li> \c sizeBDiagCursor - diagonal resize (/)
  <li> \c sizeFDiagCursor - diagonal resize (\)
  <li> \c sizeAllCursor - all directions resize
  <li> \c blankCursor - blank/invisible cursor
  <li> \c splitVCursor - vertical splitting
  <li> \c splitHCursor - horziontal splitting
  <li> \c pointingHandCursor - a pointing hand
  <li> \c forbiddenCursor - a slashed circle
  </ul>

  If you want to create a cursor with an own bitmap, use either the QCursor
  constructor which takes a a bitmap and a mask or the constructor which
  takes a pixmap as arguments.

  To set or get the position of the mouse cursor use the static methods
  QCursor::pos() and QCursor::setPos().

  \sa QWidget
  <a href="guibooks.html#fowler">GUI Design Handbook: Cursors.</a>
*/


/*****************************************************************************
  QCursor stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM


/*!
  \relates QCursor
  Writes the cursor \a c to the stream \a s.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<( QDataStream &s, const QCursor &c )
{
    s << (Q_INT16)c.shape();			// write shape id to stream
    if ( c.shape() == BitmapCursor ) {		// bitmap cursor
	s << *c.bitmap() << *c.mask();
	s << c.hotSpot();
    }
    return s;
}

/*!
  \relates QCursor
  Reads a cursor from the stream \a s and sets \a c to the read data.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>( QDataStream &s, QCursor &c )
{
    Q_INT16 shape;
    s >> shape;					// read shape id from stream
    if ( shape == BitmapCursor ) {		// read bitmap cursor
	QBitmap bm, bmm;
	QPoint	hot;
	s >> bm >> bmm >> hot;
	c = QCursor( bm, bmm, hot.x(), hot.y() );
    } else {
	c.setShape( (int)shape );		// create cursor with shape
    }
    return s;
}
#endif // QT_NO_DATASTREAM


/*! Constructs a custom pixmap cursor.

  \a pixmap is the image
	    (usually it should have a mask (set using QPixmap::setMask())
  \a hotX and
  \a hotY define the hot spot of this cursor.

  If \a hotX is negative, it is set to the pixmap().width()/2.
  If \a hotY is negative, it is set to the pixmap().height()/2.

  Allowed cursor sizes depend on the display hardware (or the underlying
  window system). We recommend using 32x32 cursors, because this size
  is supported on all platforms. Some platforms also support 16x16, 48x48
  and 64x64 cursors.

  Currently, only black-and-white pixmaps can be used.

  \sa QPixmap::QPixmap(), QPixmap::setMask()
*/

QCursor::QCursor( const QPixmap &pixmap, int hotX, int hotY )
{
    QImage img = pixmap.convertToImage().
		    convertDepth( 8, Qt::ThresholdDither|Qt::AvoidDither );
    QBitmap bm;
    bm.convertFromImage( img, Qt::ThresholdDither|Qt::AvoidDither );
    QBitmap bmm;
    if ( bm.mask() ) {
	bmm = *bm.mask();
	QBitmap nullBm;
	bm.setMask( nullBm );
    }
    else if ( pixmap.mask() ) {
	QImage mimg = pixmap.mask()->convertToImage().
	              convertDepth( 8, Qt::ThresholdDither|Qt::AvoidDither );
	bmm.convertFromImage( mimg, Qt::ThresholdDither|Qt::AvoidDither );
    }
    else {
	bmm.resize( bm.size() );
	bmm.fill( Qt::color1 );
    }

    setBitmap(bm,bmm,hotX,hotY);
}



/*!
  Constructs a custom bitmap cursor.

  \a bitmap and
  \a mask make up the bitmap.
  \a hotX and
  \a hotY define the hot spot of this cursor.

  If \a hotX is negative, it is set to the bitmap().width()/2.
  If \a hotY is negative, it is set to the bitmap().height()/2.

  The cursor \a bitmap (B) and \a mask (M) bits are combined this way:
  <ol>
  <li> B=1 and M=1 gives black.
  <li> B=0 and M=1 gives white.
  <li> B=0 and M=0 gives transparency.
  <li> B=1 and M=0 gives an undefined result.
  </ol>

  Use the global color \c color0 to draw 0-pixels and \c color1 to draw
  1-pixels in the bitmaps.

  Allowed cursor sizes depend on the display hardware (or the underlying
  window system). We recommend using 32x32 cursors, because this size
  is supported on all platforms. Some platforms also support 16x16, 48x48
  and 64x64 cursors.

  \sa QBitmap::QBitmap(), QBitmap::setMask()
*/

QCursor::QCursor( const QBitmap &bitmap, const QBitmap &mask,
		  int hotX, int hotY )
{
    setBitmap(bitmap,mask,hotX,hotY);
}

#endif // QT_NO_CURSOR


