/****************************************************************************
** $Id$
**
** Implementation of QCursor class for X11
**
** Created : 940219
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

#include "qcursor.h"
#include "qbitmap.h"
#include "qimage.h"
#include "qapplication.h"
#include "qdatastream.h"
#include "qt_x11.h"
#include <X11/cursorfont.h>

// NOT REVISED

// Define QT_USE_APPROXIMATE_CURSORS when compiling if you REALLY want to
// use the ugly X11 cursors.

/*****************************************************************************
  Internal QCursorData class
 *****************************************************************************/

struct QCursorData : public QShared
{
    QCursorData( int s = 0 );
   ~QCursorData();
    int	      cshape;
    QBitmap  *bm, *bmm;
    short     hx, hy;
    XColor    fg,bg;
    Cursor    hcurs;
    Pixmap    pm, pmm;
};

QCursorData::QCursorData( int s )
{
    cshape = s;
    hcurs = 0;
    bm = bmm = 0;
    hx = hy  = 0;
    pm = pmm = 0;
}

QCursorData::~QCursorData()
{
    Display *dpy = qt_xdisplay();
    if ( hcurs )
	XFreeCursor( dpy, hcurs );
    if ( pm )
	XFreePixmap( dpy, pm );
    if ( pmm )
	XFreePixmap( dpy, pmm );
    if ( bm )
	delete bm;
    if ( bmm )
	delete bmm;
}


/*****************************************************************************
  Global cursors
 *****************************************************************************/

static const int cursors = 15;
static QCursor cursorTable[cursors];

static const int arrowCursorIdx = 0;

QT_STATIC_CONST_IMPL QCursor & Qt::arrowCursor = cursorTable[0];
QT_STATIC_CONST_IMPL QCursor & Qt::upArrowCursor = cursorTable[1];
QT_STATIC_CONST_IMPL QCursor & Qt::crossCursor = cursorTable[2];
QT_STATIC_CONST_IMPL QCursor & Qt::waitCursor = cursorTable[3];
QT_STATIC_CONST_IMPL QCursor & Qt::ibeamCursor = cursorTable[4];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeVerCursor = cursorTable[5];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeHorCursor = cursorTable[6];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeBDiagCursor = cursorTable[7];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeFDiagCursor = cursorTable[8];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeAllCursor = cursorTable[9];
QT_STATIC_CONST_IMPL QCursor & Qt::blankCursor = cursorTable[10];
QT_STATIC_CONST_IMPL QCursor & Qt::splitHCursor = cursorTable[11];
QT_STATIC_CONST_IMPL QCursor & Qt::splitVCursor = cursorTable[12];
QT_STATIC_CONST_IMPL QCursor & Qt::pointingHandCursor = cursorTable[13];
QT_STATIC_CONST_IMPL QCursor & Qt::forbiddenCursor = cursorTable[14];


QCursor *QCursor::find_cur( int shape )		// find predefined cursor
{
    return (uint)shape <= LastCursor ? &cursorTable[shape] : 0;
}


static bool initialized = FALSE;

/*!
  Internal function that deinitializes the predefined cursors.
  This function is called from the QApplication destructor.
  \sa initialize()
*/
void QCursor::cleanup()
{
    if ( !initialized )
	return;
    
    int shape;
    for( shape = 0; shape < cursors; shape++ ) {
	delete cursorTable[shape].data;
	cursorTable[shape].data = 0;
    }
    initialized = FALSE;
}


/*!
  Internal function that initializes the predefined cursors.
  This function is called from the QApplication constructor.
  \sa cleanup()
*/

void QCursor::initialize()
{
    int shape;
    for( shape = 0; shape < cursors; shape++ )
	cursorTable[shape].data = new QCursorData( shape );
    initialized = TRUE;
    qAddPostRoutine( cleanup );
}


/*!
  Constructs a cursor with the default arrow shape.
*/
QCursor::QCursor()
{
    if ( !initialized ) {
	if ( qApp->startingUp() ) {
	    data = 0;
	    return;
	}
	initialize();
    }
    QCursor* c = &cursorTable[arrowCursorIdx];
    c->data->ref();
    data = c->data;
}



/*!
  Constructs a cursor with the specified \a shape.

  \a shape can be one of
  <ul>
  <li> \c ArrowCursor - standard arrow cursor
  <li> \c UpArrowCursor - upwards arrow
  <li> \c CrossCursor - crosshair
  <li> \c WaitCursor - hourglass/watch
  <li> \c IbeamCursor - ibeam/text entry
  <li> \c SizeVerCursor - vertical resize
  <li> \c SizeHorCursor - horizontal resize
  <li> \c SizeBDiagCursor - diagonal resize (/)
  <li> \c SizeFDiagCursor - diagonal resize (\)
  <li> \c SizeAllCursor - all directions resize
  <li> \c BlankCursor - blank/invisible cursor
  <li> \c SplitVCursor - vertical splitting
  <li> \c SplitHCursor - horziontal splitting
  <li> \c PointingHandCursor - a pointing hand
  <li> \c BitmapCursor - userdefined bitmap cursor
  </ul>

  These correspond to the <a href="#cursors">predefined</a>
  global QCursor objects.

  \sa setShape()
*/

QCursor::QCursor( int shape )
{
    if ( !initialized )
	initialize();
    QCursor *c = find_cur( shape );
    if ( !c )					// not found
	c = &cursorTable[arrowCursorIdx];	//   then use arrowCursor
    c->data->ref();
    data = c->data;
}


void QCursor::setBitmap( const QBitmap &bitmap, const QBitmap &mask,
			 int hotX, int hotY )
{
    if ( !initialized )
	initialize();
    if ( bitmap.depth() != 1 || mask.depth() != 1 ||
	 bitmap.size() != mask.size() ) {
#if defined(CHECK_NULL)
	qWarning( "QCursor: Cannot create bitmap cursor; invalid bitmap(s)" );
#endif
	QCursor *c = &cursorTable[arrowCursorIdx];
	c->data->ref();
	data = c->data;
	return;
    }
    data = new QCursorData;
    CHECK_PTR( data );
    data->bm  = new QBitmap( bitmap );
    data->bmm = new QBitmap( mask );
    data->hcurs = 0;
    data->cshape = BitmapCursor;
    data->hx = hotX >= 0 ? hotX : bitmap.width()/2;
    data->hy = hotY >= 0 ? hotY : bitmap.height()/2;
    data->fg.red   = 0 << 8;
    data->fg.green = 0 << 8;
    data->fg.blue  = 0 << 8;
    data->bg.red   = 255 << 8;
    data->bg.green = 255 << 8;
    data->bg.blue  = 255 << 8;
}


/*!
  Constructs a copy of the cursor \a c.
*/

QCursor::QCursor( const QCursor &c )
{
    if ( !initialized )
	initialize();
    data = c.data;				// shallow copy
    data->ref();
}

/*!
  Destructs the cursor.
*/

QCursor::~QCursor()
{
    if ( data && data->deref() )
	delete data;
}


/*!
  Assigns \a c to this cursor and returns a reference to this cursor.
*/

QCursor &QCursor::operator=( const QCursor &c )
{
    if ( !initialized )
	initialize();
    c.data->ref();				// avoid c = c
    if ( data->deref() )
	delete data;
    data = c.data;
    return *this;
}


/*!
  Returns the cursor shape identifer. The return value is one of
  following values (cast to an int)

  <ul>
  <li> \c ArrowCursor - standard arrow cursor
  <li> \c UpArrowCursor - upwards arrow
  <li> \c CrossCursor - crosshair
  <li> \c WaitCursor - hourglass/watch
  <li> \c IbeamCursor - ibeam/text entry
  <li> \c SizeVerCursor - vertical resize
  <li> \c SizeHorCursor - horizontal resize
  <li> \c SizeBDiagCursor - diagonal resize (/)
  <li> \c SizeFDiagCursor - diagonal resize (\)
  <li> \c SizeAllCursor - all directions resize
  <li> \c BlankCursor - blank/invisible cursor
  <li> \c SplitVCursor - vertical splitting
  <li> \c SplitHCursor - horziontal splitting
  <li> \c PointingHandCursor - a pointing hand
  <li> \c ForbiddenCursor - a slashed circle
  <li> \c BitmapCursor - userdefined bitmap cursor
  </ul>

  These correspond to the <a href="#cursors">predefined</a>
  global QCursor objects.

  \sa setShape()
*/

int QCursor::shape() const
{
    if ( !initialized )
	initialize();
    return data->cshape;
}

/*!
  Sets the cursor to the shape identified by \a shape.

  <ul>
  <li> \c ArrowCursor - standard arrow cursor
  <li> \c UpArrowCursor - upwards arrow
  <li> CrossCursor - crosshair
  <li> \c WaitCursor - hourglass/watch
  <li> \c IbeamCursor - ibeam/text entry
  <li> \c SizeVerCursor - vertical resize
  <li> \c SizeHorCursor - horizontal resize
  <li> \c SizeBDiagCursor - diagonal resize (/)
  <li> \c SizeFDiagCursor - diagonal resize (\)
  <li> \c SizeAllCursor - all directions resize
  <li> \c BlankCursor - blank/invisible cursor
  <li> \c SplitVCursor - vertical splitting
  <li> \c SplitHCursor - horziontal splitting
  <li> \c PointingHandCursor - a pointing hand
  <li> \c ForbiddenCursor - a slashed circle
  <li> \c BitmapCursor - userdefined bitmap cursor
  </ul>

  These correspond to the <a href="#cursors">predefined</a>
  global QCursor objects.

  \sa shape()
*/

void QCursor::setShape( int shape )
{
    if ( !initialized )
	initialize();
    QCursor *c = find_cur( shape );		// find one of the global ones
    if ( !c )					// not found
	c = &cursorTable[arrowCursorIdx];	//   then use arrowCursor
    c->data->ref();
    if ( data->deref() )			// make shallow copy
	delete data;
    data = c->data;
}


/*!
  Returns the cursor bitmap, or 0 if it is one of the standard cursors.
*/
const QBitmap *QCursor::bitmap() const
{
    if ( !initialized )
	initialize();
    return data->bm;
}

/*!
  Returns the cursor bitmap mask, or 0 if it is one of the standard cursors.
*/

const QBitmap *QCursor::mask() const
{
    if ( !initialized )
	initialize();
    return data->bmm;
}

/*!
  Returns the cursor hot spot, or (0,0) if it is one of the standard cursors.
*/

QPoint QCursor::hotSpot() const
{
    if ( !initialized )
	initialize();
    return QPoint( data->hx, data->hy );
}


/*!
  Returns the window system cursor handle.

  \warning
  Portable in principle, but if you use it you are probably about to do
  something non-portable. Be careful.
*/

HANDLE QCursor::handle() const
{
    if ( !initialized )
	initialize();
    if ( !data->hcurs )
	update();
    return data->hcurs;
}


/*!
  Returns the position of the cursor (hot spot) in global screen
  coordinates.

  You can call QWidget::mapFromGlobal() to translate it to widget
  coordinates.

  \sa setPos(), QWidget::mapFromGlobal(), QWidget::mapToGlobal()
*/

QPoint QCursor::pos()
{
    Window root;
    Window child;
    int root_x, root_y, win_x, win_y;
    uint buttons;
    XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
		   &root_x, &root_y, &win_x, &win_y, &buttons );
    return QPoint( root_x, root_y );
}

/*!
  Moves the cursor (hot spot) to the global screen position \a x and \a y.

  You can call QWidget::mapToGlobal() to translate widget coordinates
  to global screen coordinates.

  \sa pos(), QWidget::mapFromGlobal(), QWidget::mapToGlobal()
*/

void QCursor::setPos( int x, int y )
{
    // Need to check, since some X servers generate null mouse move
    // events, causing looping in applications which call setPos() on
    // every mouse move event.
    //
    if (pos() == QPoint(x,y))
	return;

    XWarpPointer( qt_xdisplay(), None, qt_xrootwin(), 0, 0, 0, 0, x, y );
}

/*!
  \overload void QCursor::setPos ( const QPoint & )
*/


/*!
  \internal Creates the cursor.
*/

void QCursor::update() const
{
    if ( !initialized )
	initialize();
    register QCursorData *d = data;		// cheat const!
    if ( d->hcurs )				// already loaded
	return;

    Display *dpy = qt_xdisplay();
    Window rootwin = qt_xrootwin();

    if ( d->cshape == BitmapCursor ) {
	d->hcurs = XCreatePixmapCursor( dpy, d->bm->handle(), d->bmm->handle(),
					&d->fg, &d->bg, d->hx, d->hy );
	return;
    }

    static uchar cur_blank_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  // Non-standard X11 cursors are created from bitmaps

#ifndef QT_USE_APPROXIMATE_CURSORS
    static uchar cur_ver_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0xc0, 0x03, 0xe0, 0x07, 0xf0, 0x0f,
	0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xf0, 0x0f,
	0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00 };
    static uchar mcur_ver_bits[] = {
	0x00, 0x00, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x3f,
	0xfc, 0x7f, 0xc0, 0x07, 0xc0, 0x07, 0xc0, 0x07, 0xfc, 0x7f, 0xf8, 0x3f,
	0xf0, 0x1f, 0xe0, 0x0f, 0xc0, 0x07, 0x80, 0x03 };
    static uchar cur_hor_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x08, 0x30, 0x18,
	0x38, 0x38, 0xfc, 0x7f, 0xfc, 0x7f, 0x38, 0x38, 0x30, 0x18, 0x20, 0x08,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static uchar mcur_hor_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x60, 0x0c, 0x70, 0x1c, 0x78, 0x3c,
	0xfc, 0x7f, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfc, 0x7f, 0x78, 0x3c,
	0x70, 0x1c, 0x60, 0x0c, 0x40, 0x04, 0x00, 0x00 };
    static uchar cur_bdiag_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x3e, 0x00, 0x3c, 0x00, 0x3e,
	0x00, 0x37, 0x88, 0x23, 0xd8, 0x01, 0xf8, 0x00, 0x78, 0x00, 0xf8, 0x00,
	0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static uchar mcur_bdiag_bits[] = {
	0x00, 0x00, 0xc0, 0x7f, 0x80, 0x7f, 0x00, 0x7f, 0x00, 0x7e, 0x04, 0x7f,
	0x8c, 0x7f, 0xdc, 0x77, 0xfc, 0x63, 0xfc, 0x41, 0xfc, 0x00, 0xfc, 0x01,
	0xfc, 0x03, 0xfc, 0x07, 0x00, 0x00, 0x00, 0x00 };
    static uchar cur_fdiag_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0xf8, 0x00, 0x78, 0x00,
	0xf8, 0x00, 0xd8, 0x01, 0x88, 0x23, 0x00, 0x37, 0x00, 0x3e, 0x00, 0x3c,
	0x00, 0x3e, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00 };
    static uchar mcur_fdiag_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0xfc, 0x07, 0xfc, 0x03, 0xfc, 0x01, 0xfc, 0x00,
	0xfc, 0x41, 0xfc, 0x63, 0xdc, 0x77, 0x8c, 0x7f, 0x04, 0x7f, 0x00, 0x7e,
	0x00, 0x7f, 0x80, 0x7f, 0xc0, 0x7f, 0x00, 0x00 };
    static uchar *cursor_bits16[] = {
	cur_ver_bits, mcur_ver_bits, cur_hor_bits, mcur_hor_bits,
	cur_bdiag_bits, mcur_bdiag_bits, cur_fdiag_bits, mcur_fdiag_bits,
	0, 0, cur_blank_bits, cur_blank_bits };

    static uchar vsplit_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x41, 0x82, 0x00, 0x80, 0x41, 0x82, 0x01, 0xc0, 0x7f, 0xfe, 0x03,
	0x80, 0x41, 0x82, 0x01, 0x00, 0x41, 0x82, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static uchar vsplitm_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
	0x00, 0xe0, 0x07, 0x00, 0x00, 0xe2, 0x47, 0x00, 0x00, 0xe3, 0xc7, 0x00,
	0x80, 0xe3, 0xc7, 0x01, 0xc0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x07,
	0xc0, 0xff, 0xff, 0x03, 0x80, 0xe3, 0xc7, 0x01, 0x00, 0xe3, 0xc7, 0x00,
	0x00, 0xe2, 0x47, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
	0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static uchar hsplit_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static uchar hsplitm_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00,
	0x00, 0xf8, 0x0f, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
	0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
	0x80, 0xff, 0xff, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00,
	0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static uchar phand_bits[] = {
	0x00, 0x00, 0x00, 0x00,	0xfe, 0x01, 0x00, 0x00,	0x01, 0x02, 0x00, 0x00,
	0x7e, 0x04, 0x00, 0x00,	0x08, 0x08, 0x00, 0x00,	0x70, 0x08, 0x00, 0x00,
	0x08, 0x08, 0x00, 0x00,	0x70, 0x14, 0x00, 0x00,	0x08, 0x22, 0x00, 0x00,
	0x30, 0x41, 0x00, 0x00,	0xc0, 0x20, 0x00, 0x00,	0x40, 0x12, 0x00, 0x00,
	0x80, 0x08, 0x00, 0x00,	0x00, 0x05, 0x00, 0x00,	0x00, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00 };
    static uchar phandm_bits[] = {
	0xfe, 0x01, 0x00, 0x00,	0xff, 0x03, 0x00, 0x00,	0xff, 0x07, 0x00, 0x00,
	0xff, 0x0f, 0x00, 0x00,	0xfe, 0x1f, 0x00, 0x00,	0xf8, 0x1f, 0x00, 0x00,
	0xfc, 0x1f, 0x00, 0x00,	0xf8, 0x3f, 0x00, 0x00,	0xfc, 0x7f, 0x00, 0x00,
	0xf8, 0xff, 0x00, 0x00,	0xf0, 0x7f, 0x00, 0x00,	0xe0, 0x3f, 0x00, 0x00,
	0xc0, 0x1f, 0x00, 0x00,	0x80, 0x0f, 0x00, 0x00,	0x00, 0x07, 0x00, 0x00,
	0x00, 0x02, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00 };

    static uchar *cursor_bits32[] = {
	vsplit_bits, vsplitm_bits, hsplit_bits, hsplitm_bits,
	    phand_bits, phandm_bits
    };

    static uchar forbidden_bits[] = {
	0x00,0x00,0x00,0x80,0x1f,0x00,0xe0,0x7f,0x00,0xf0,0xf0,0x00,0x38,0xc0,0x01,
	    0x7c,0x80,0x03,0xec,0x00,0x03,0xce,0x01,0x07,0x86,0x03,0x06,0x06,0x07,0x06,
	    0x06,0x0e,0x06,0x06,0x1c,0x06,0x0e,0x38,0x07,0x0c,0x70,0x03,0x1c,0xe0,0x03,
	    0x38,0xc0,0x01,0xf0,0xe0,0x00,0xe0,0x7f,0x00,0x80,0x1f,0x00,0x00,0x00,0x00 };

    static unsigned char forbiddenm_bits[] = {
	0x80,0x1f,0x00,0xe0,0x7f,0x00,0xf0,0xff,0x00,0xf8,0xff,0x01,0xfc,0xf0,0x03,
	    0xfe,0xc0,0x07,0xfe,0x81,0x07,0xff,0x83,0x0f,0xcf,0x07,0x0f,0x8f,0x0f,0x0f,
	    0x0f,0x1f,0x0f,0x0f,0x3e,0x0f,0x1f,0xfc,0x0f,0x1e,0xf8,0x07,0x3e,0xf0,0x07,
	    0xfc,0xe0,0x03,0xf8,0xff,0x01,0xf0,0xff,0x00,0xe0,0x7f,0x00,0x80,0x1f,0x00};

    static uchar *cursor_bits20[] = {
	    forbidden_bits, forbiddenm_bits
    };

    if ( d->cshape >= SizeVerCursor && d->cshape < SizeAllCursor ||
	 d->cshape == BlankCursor ) {
	XColor bg, fg;				// ignore stupid CFront message
	bg.red   = 255 << 8;
	bg.green = 255 << 8;
	bg.blue  = 255 << 8;
	fg.red   = 0;
	fg.green = 0;
	fg.blue  = 0;
	int i = (d->cshape - SizeVerCursor)*2;
	d->pm  = XCreateBitmapFromData( dpy, rootwin, (char *)cursor_bits16[i],
					16, 16 );
	d->pmm = XCreateBitmapFromData( dpy, rootwin, (char *)cursor_bits16[i+1],
					16,16);
	d->hcurs = XCreatePixmapCursor( dpy, d->pm, d->pmm, &fg, &bg, 8, 8 );
	return;
    }
    if ( d->cshape >= SplitVCursor && d->cshape <= PointingHandCursor ) {
	XColor bg, fg;				// ignore stupid CFront message
	bg.red   = 255 << 8;
	bg.green = 255 << 8;
	bg.blue  = 255 << 8;
	fg.red   = 0;
	fg.green = 0;
	fg.blue  = 0;
	int i = (d->cshape - SplitVCursor)*2;
	d->pm  = XCreateBitmapFromData( dpy, rootwin, (char *)cursor_bits32[i],
					32, 32 );
	d->pmm = XCreateBitmapFromData( dpy, rootwin, (char *)cursor_bits32[i+1],
					32, 32);
	int hs = d->cshape != PointingHandCursor? 16 : 0;
	d->hcurs = XCreatePixmapCursor( dpy, d->pm, d->pmm, &fg, &bg, hs, hs );
	return;
    }
    if ( d->cshape == ForbiddenCursor ) {
	XColor bg, fg;				// ignore stupid CFront message
	bg.red   = 255 << 8;
	bg.green = 255 << 8;
	bg.blue  = 255 << 8;
	fg.red   = 0;
	fg.green = 0;
	fg.blue  = 0;
	int i = (d->cshape - ForbiddenCursor)*2;
	d->pm  = XCreateBitmapFromData( dpy, rootwin, (char *)cursor_bits20[i],
					20, 20 );
	d->pmm = XCreateBitmapFromData( dpy, rootwin, (char *)cursor_bits20[i+1],
					20, 20);
	d->hcurs = XCreatePixmapCursor( dpy, d->pm, d->pmm, &fg, &bg, 10, 10 );
	return;
    }
#endif /* ! QT_USE_APPROXIMATE_CURSORS */

    uint sh;
    switch ( d->cshape ) {			// map Q cursor to X cursor
	case ArrowCursor:
	    sh = XC_left_ptr;
	    break;
	case UpArrowCursor:
	    sh = XC_center_ptr;
	    break;
	case CrossCursor:
	    sh = XC_crosshair;
	    break;
	case WaitCursor:
	    sh = XC_watch;
	    break;
	case IbeamCursor:
	    sh = XC_xterm;
	    break;
	case SizeAllCursor:
	    sh = XC_fleur;
	    break;
#ifdef QT_USE_APPROXIMATE_CURSORS
	case SizeVerCursor:
	    sh = XC_top_side;
	    break;
	case SizeHorCursor:
	    sh = XC_right_side;
	    break;
	case SizeBDiagCursor:
	    sh = XC_top_right_corner;
	    break;
	case SizeFDiagCursor:
	    sh = XC_bottom_right_corner;
	    break;
	case BlankCursor:
	    XColor bg, fg;                          // ignore stupid CFront message
	    bg.red   = 255 << 8;
	    bg.green = 255 << 8;
	    bg.blue  = 255 << 8;
	    fg.red   = 0;
	    fg.green = 0;
	    fg.blue  = 0;
	    d->pm  = XCreateBitmapFromData( dpy, rootwin,
		(char *)cur_blank_bits, 16, 16 );
	    d->pmm = XCreateBitmapFromData( dpy, rootwin,
		(char *)cur_blank_bits, 16,16);
	    d->hcurs = XCreatePixmapCursor( dpy, d->pm, d->pmm, &fg,
		&bg, 8, 8 );
	    return;
	    break;
	case SplitVCursor:
	    sh = XC_sb_h_double_arrow;
	    break;
	case SplitHCursor:
	    sh = XC_sb_v_double_arrow;
	    break;
	case PointingHandCursor:
	    sh = XC_hand1;
	    break;
	case ForbiddenCursor:
	    sh = XC_circle;
	    break;
#endif /* QT_USE_APPROXIMATE_CURSORS */
	default:
#if defined(CHECK_RANGE)
	    qWarning( "QCursor::update: Invalid cursor shape %d", d->cshape );
#endif
	    return;
    }
    d->hcurs = XCreateFontCursor( dpy, sh );
}
