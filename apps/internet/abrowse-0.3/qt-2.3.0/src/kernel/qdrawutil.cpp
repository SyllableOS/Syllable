/****************************************************************************
** $Id$
**
** Implementation of draw utilities
**
** Created : 950920
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

#include "qdrawutil.h"
#ifndef QT_NO_DRAWUTIL
#include "qbitmap.h"
#include "qpixmapcache.h"

// REVISED: paul

/*!
  \relates QPainter

  <code>#include &lt;qdrawutil.h&gt;</code>

  Draws a horizontal (\a y1 == \a y2) or vertical (\a x1 == \a x2) shaded
  line using the painter \a p.

  Nothing is drawn if \a y1 != \a y2 and \a x1 != \a x2 (i.e. the line
  is neither horizontal nor vertical).

  The color group argument \a g specifies the shading colors
  (\link QColorGroup::light() light\endlink,
  \link QColorGroup::dark() dark\endlink and
  \link QColorGroup::mid() middle\endlink colors).

  The line appears sunken if \a sunken is TRUE, or raised if \a sunken is
  FALSE.

  The \a lineWidth argument specifies the line width for each of the
  lines. It is not the total line width.

  The \a midLineWidth argument specifies the width of a middle line drawn
  in the QColorGroup::mid() color.

  If you want to use a QFrame widget instead, you can make it display a
  shaded line, for example
  <code>QFrame::setFrameStyle( QFrame::HLine | QFrame::Sunken )</code>.

  \warning This function does not look at QWidget::style()
  or QApplication::style(). Use the drawing functions in QStyle to make
  widgets that follow the current GUI style.

  \sa qDrawShadeRect(), qDrawShadePanel(), QStyle::drawSeparator()
*/

void qDrawShadeLine( QPainter *p, int x1, int y1, int x2, int y2,
		     const QColorGroup &g, bool sunken,
		     int lineWidth, int midLineWidth )
{
    if (!( p && lineWidth >= 0 && midLineWidth >= 0 ) )	{
#if defined(CHECK_RANGE)
	qWarning( "qDrawShadeLine invalid parameters." );
#endif
	return;
    }
    int tlw = lineWidth*2 + midLineWidth;	// total line width
    QPen oldPen = p->pen();			// save pen
    if ( sunken )
	p->setPen( g.dark() );
    else
	p->setPen( g.light() );
    QPointArray a;
    int i;
    if ( y1 == y2 ) {				// horizontal line
	int y = y1 - tlw/2;
	if ( x1 > x2 ) {			// swap x1 and x2
	    int t = x1;
	    x1 = x2;
	    x2 = t;
	}
	x2--;
	for ( i=0; i<lineWidth; i++ ) {		// draw top shadow
	    a.setPoints( 3, x1+i, y+tlw-1,
			    x1+i, y+i,
			    x2,	  y+i );
	    p->drawPolyline( a );
	}
	if ( midLineWidth > 0 ) {
	    p->setPen( g.mid() );
	    for ( i=0; i<midLineWidth; i++ )	// draw lines in the middle
		p->drawLine( x1+lineWidth, y+lineWidth+i,
			     x2-lineWidth, y+lineWidth+i );
	}
	if ( sunken )
	    p->setPen( g.light() );
	else
	    p->setPen( g.dark() );
	for ( i=0; i<lineWidth; i++ ) {		// draw bottom shadow
	    a.setPoints( 3, x1+lineWidth, y+tlw-i-1,
			    x2-i,	  y+tlw-i-1,
			    x2-i,	  y+lineWidth );
	    p->drawPolyline( a );
	}
    }
    else if ( x1 == x2 ) {			// vertical line
	int x = x1 - tlw/2;
	if ( y1 > y2 ) {			// swap y1 and y2
	    int t = y1;
	    y1 = y2;
	    y2 = t;
	}
	y2--;
	for ( i=0; i<lineWidth; i++ ) {		// draw left shadow
	    a.setPoints( 3, x+i,     y2,
			    x+i,     y1+i,
			    x+tlw-1, y1+i );
	    p->drawPolyline( a );
	}
	if ( midLineWidth > 0 ) {
	    p->setPen( g.mid() );
	    for ( i=0; i<midLineWidth; i++ )	// draw lines in the middle
		p->drawLine( x+lineWidth+i, y1+lineWidth, x+lineWidth+i, y2 );
	}
	if ( sunken )
	    p->setPen( g.light() );
	else
	    p->setPen( g.dark() );
	for ( i=0; i<lineWidth; i++ ) {		// draw right shadow
	    a.setPoints( 3, x+lineWidth,      y2-i,
			    x+tlw-i-1, y2-i,
			    x+tlw-i-1, y1+lineWidth );
	    p->drawPolyline( a );
	}
    }
    p->setPen( oldPen );
}


/*!
  \relates QPainter

  <code>#include &lt;qdrawutil.h&gt;</code>

  Draws a shaded rectangle/box given by (\a x, \a y, \a w, \a h)
  using the painter \a p.

  The color group argument \a g specifies the shading colors
  (\link QColorGroup::light() light\endlink,
  \link QColorGroup::dark() dark\endlink and
  \link QColorGroup::mid() middle\endlink colors).

  The rectangle appears sunken if \a sunken is TRUE, or raised if \a
  sunken is FALSE.

  The \a lineWidth argument specifies the line width for each of the
  lines. It is not the total line width.

  The \a midLineWidth argument specifies the width of a middle line drawn
  in the QColorGroup::mid() color.

  The rectangle interior is filled with the \a fill brush unless \a fill
  is null.

  If you want to use a QFrame widget instead, you can make it display a
  shaded rectangle, for example
  <code>QFrame::setFrameStyle( QFrame::Box | QFrame::Raised )</code>.

  \warning This function does not look at QWidget::style()
  or QApplication::style(). Use the drawing functions in QStyle to make
  widgets that follow the current GUI style.

  \sa qDrawShadeLine(), qDrawShadePanel(), qDrawPlainRect(),
  QStyle::drawRect(), QStyle::drawRectStrong()
*/

void qDrawShadeRect( QPainter *p, int x, int y, int w, int h,
		     const QColorGroup &g, bool sunken,
		     int lineWidth, int midLineWidth,
		     const QBrush *fill )
{
    if ( w == 0 || h == 0 )
	return;
    if ( ! ( w > 0 && h > 0 && lineWidth >= 0 && midLineWidth >= 0 ) ) {
#if defined(CHECK_RANGE)
	qWarning( "qDrawShadeRect() Invalid parameters." );
#endif
	return;
    }
    QPen oldPen = p->pen();			// save pen
    if ( sunken )
	p->setPen( g.dark() );
    else
	p->setPen( g.light() );
    int x1=x, y1=y, x2=x+w-1, y2=y+h-1;
    QPointArray a;
    if ( lineWidth == 1 && midLineWidth == 0 ) {// standard shade rectangle
	a.setPoints( 8, x1,y1, x2,y1, x1,y1+1, x1,y2, x1+2,y2-1,
		     x2-1,y2-1, x2-1,y1+2,  x2-1,y2-2 );
	p->drawLineSegments( a );		// draw top/left lines
	if ( sunken )
	    p->setPen( g.light() );
	else
	    p->setPen( g.dark() );
	a.setPoints( 8, x1+1,y1+1, x2,y1+1, x1+1,y1+2, x1+1,y2-1,
		     x1+1,y2, x2,y2,  x2,y1+2, x2,y2-1 );
	p->drawLineSegments( a );		// draw bottom/right lines
    }
    else {					// more complicated
	int m = lineWidth+midLineWidth;
	int i, j=0, k=m;
	for ( i=0; i<lineWidth; i++ ) {		// draw top shadow
	    p->drawLine( x1+j, y2-j, x1+j, y1+j );
	    p->drawLine( x1+j, y1+j, x2-j, y1+j );
	    p->drawLine( x1+k, y2-k, x2-k, y2-k );
	    p->drawLine( x2-k, y2-k, x2-k, y1+k );
	    j++;
	    k++;
	}
	p->setPen( g.mid() );
	j = lineWidth*2;
	for ( i=0; i<midLineWidth; i++ ) {	// draw lines in the middle
	    p->drawRect( x1+lineWidth+i, y1+lineWidth+i, w-j, h-j );
	    j += 2;
	}
	if ( sunken )
	    p->setPen( g.light() );
	else
	    p->setPen( g.dark() );
	j = 0;
	k = m;
	for ( i=0; i<lineWidth; i++ ) {		// draw bottom shadow
	    p->drawLine( x1+1+j,y2-j, x2-j, y2-j );
	    p->drawLine( x2-j,	y2-j, x2-j, y1+j+1 );
	    p->drawLine( x1+k,	y2-k, x1+k, y1+k );
	    p->drawLine( x1+k,	y1+k, x2-k, y1+k );
	    j++;
	    k++;
	}
    }
    if ( fill ) {
	QBrush oldBrush = p->brush();
	int tlw = lineWidth + midLineWidth;
	p->setPen( Qt::NoPen );
	p->setBrush( *fill );
	p->drawRect( x+tlw, y+tlw, w-2*tlw, h-2*tlw );
	p->setBrush( oldBrush );
    }
    p->setPen( oldPen );			// restore pen
}


/*!
  \relates QPainter

  <code>#include &lt;qdrawutil.h&gt;</code>

  Draws a shaded panel given by (\a x, \a y, \a w, \a h)
  using the painter \a p.

  The color group argument \a g specifies the shading colors
  (\link QColorGroup::light() light\endlink,
  \link QColorGroup::dark() dark\endlink and
  \link QColorGroup::mid() middle\endlink colors).

  The panel appears sunken if \a sunken is TRUE, or raised if \a sunken is
  FALSE.

  The \a lineWidth argument specifies the line width.

  The panel interior is filled with the \a fill brush unless \a fill is
  null.

  If you want to use a QFrame widget instead, you can make it display a
  shaded panel, for example
  <code>QFrame::setFrameStyle( QFrame::Panel | QFrame::Sunken )</code>.

  \warning This function does not look at QWidget::style()
  or QApplication::style(). Use the drawing functions in QStyle to make
  widgets that follow the current GUI style.


  \sa qDrawWinPanel(), qDrawShadeLine(), qDrawShadeRect(), QStyle::drawPanel()
*/

void qDrawShadePanel( QPainter *p, int x, int y, int w, int h,
		      const QColorGroup &g, bool sunken,
		      int lineWidth, const QBrush *fill )
{
    if ( w == 0 || h == 0 )
	return;
    if ( !( w > 0 && h > 0 && lineWidth >= 0 ) ) {
#if defined(CHECK_RANGE)
    	qWarning( "qDrawShadePanel() Invalid parameters." );
#endif
    }
    QPen oldPen = p->pen();			// save pen
    QPointArray a( 4*lineWidth );
    if ( sunken )
	p->setPen( g.dark() );
    else
	p->setPen( g.light() );
    int x1, y1, x2, y2;
    int i;
    int n = 0;
    x1 = x;
    y1 = y2 = y;
    x2 = x+w-2;
    for ( i=0; i<lineWidth; i++ ) {		// top shadow
	a.setPoint( n++, x1, y1++ );
	a.setPoint( n++, x2--, y2++ );
    }
    x2 = x1;
    y1 = y+h-2;
    for ( i=0; i<lineWidth; i++ ) {		// left shadow
	a.setPoint( n++, x1++, y1 );
	a.setPoint( n++, x2++, y2-- );
    }
    p->drawLineSegments( a );
    n = 0;
    if ( sunken )
	p->setPen( g.light() );
    else
	p->setPen( g.dark() );
    x1 = x;
    y1 = y2 = y+h-1;
    x2 = x+w-1;
    for ( i=0; i<lineWidth; i++ ) {		// bottom shadow
	a.setPoint( n++, x1++, y1-- );
	a.setPoint( n++, x2, y2-- );
    }
    x1 = x2;
    y1 = y;
    y2 = y+h-lineWidth-1;
    for ( i=0; i<lineWidth; i++ ) {		// right shadow
	a.setPoint( n++, x1--, y1++ );
	a.setPoint( n++, x2--, y2 );
    }
    p->drawLineSegments( a );
    if ( fill ) {				// fill with fill color
	QBrush oldBrush = p->brush();
	p->setPen( Qt::NoPen );
	p->setBrush( *fill );
	p->drawRect( x+lineWidth, y+lineWidth, w-lineWidth*2, h-lineWidth*2 );
	p->setBrush( oldBrush );
    }
    p->setPen( oldPen );			// restore pen
}


/*!
  \internal
  This function draws a rectangle with two pixel line width.
  It is called from qDrawWinButton() and qDrawWinPanel().

  c1..c4 and fill are used:

    1 1 1 1 1 2
    1 3 3 3 4 2
    1 3 F F 4 2
    1 3 F F 4 2
    1 4 4 4 4 2
    2 2 2 2 2 2
*/

static void qDrawWinShades( QPainter *p,
			   int x, int y, int w, int h,
			   const QColor &c1, const QColor &c2,
			   const QColor &c3, const QColor &c4,
			   const QBrush *fill )
{
    if ( w < 2 || h < 2 )			// can't do anything with that
	return;
    QPen oldPen = p->pen();
    QPointArray a( 3 );
    a.setPoint( 0, x, y+h-2 );
    a.setPoint( 1, x, y );
    a.setPoint( 2, x+w-2, y );
    p->setPen( c1 );
    p->drawPolyline( a );
    a.setPoint( 0, x, y+h-1 );
    a.setPoint( 1, x+w-1, y+h-1 );
    a.setPoint( 2, x+w-1, y );
    p->setPen( c2 );
    p->drawPolyline( a );
    if ( w > 4 && h > 4 ) {
	a.setPoint( 0, x+1, y+h-3 );
	a.setPoint( 1, x+1, y+1 );
	a.setPoint( 2, x+w-3, y+1 );
	p->setPen( c3 );
	p->drawPolyline( a );
	a.setPoint( 0, x+1, y+h-2 );
	a.setPoint( 1, x+w-2, y+h-2 );
	a.setPoint( 2, x+w-2, y+1 );
	p->setPen( c4 );
	p->drawPolyline( a );
	if ( fill ) {
	    QBrush oldBrush = p->brush();
	    p->setBrush( *fill );
	    p->setPen( Qt::NoPen );
	    p->drawRect( x+2, y+2, w-4, h-4 );
	    p->setBrush( oldBrush );
	}
    }
    p->setPen( oldPen );
}


/*!
  \relates QPainter

  <code>#include &lt;qdrawutil.h&gt;</code>

  Draws a Windows-style button given by (\a x, \a y, \a w, \a h)
  using the painter \a p.

  The color group argument \a g specifies the shading colors
  (\link QColorGroup::light() light\endlink,
  \link QColorGroup::dark() dark\endlink and
  \link QColorGroup::mid() middle\endlink colors).

  The button appears sunken if \a sunken is TRUE, or raised if \a sunken
  is FALSE.

  The line width is 2 pixels.

  The button interior is filled with the \a *fill brush unless \a fill is
  null.

  \warning This function does not look at QWidget::style()
  or QApplication::style(). Use the drawing functions in QStyle to make
  widgets that follow the current GUI style.

  \sa qDrawWinPanel(), QStyle::drawButton()
*/

void qDrawWinButton( QPainter *p, int x, int y, int w, int h,
		     const QColorGroup &g, bool sunken,
		     const QBrush *fill )
{
    if ( sunken )
	qDrawWinShades( p, x, y, w, h,
		       g.shadow(), g.light(), g.dark(), g.button(), fill );
    else
	qDrawWinShades( p, x, y, w, h,
		       g.light(), g.shadow(), g.midlight(), g.dark(), fill );
}

/*!
  \relates QPainter

  <code>#include &lt;qdrawutil.h&gt;</code>

  Draws a Windows-style panel given by (\a x, \a y, \a w, \a h)
  using the painter \a p.

  The color group argument \a g specifies the shading colors.

  The panel appears sunken if \a sunken is TRUE, or raised if \a sunken is
  FALSE.

  The line width is 2 pixels.

  The button interior is filled with the \a fill brush unless \a fill is
  null.

  If you want to use a QFrame widget instead, you can make it display a
  shaded panel, for example
  <code>QFrame::setFrameStyle( QFrame::WinPanel | QFrame::Raised )</code>.

  \warning This function does not look at QWidget::style()
  or QApplication::style(). Use the drawing functions in QStyle to make
  widgets that follow the current GUI style.

  \sa qDrawShadePanel(), qDrawWinButton(), QStyle::drawPanel()
*/

void qDrawWinPanel( QPainter *p, int x, int y, int w, int h,
		    const QColorGroup &g, bool	sunken,
		    const QBrush *fill )
{
    if ( sunken )
	qDrawWinShades( p, x, y, w, h,
			g.dark(), g.light(), g.shadow(), g.midlight(), fill );
    else
	qDrawWinShades( p, x, y, w, h,
			g.midlight(), g.shadow(), g.light(), g.dark(), fill );
}


/*!
  \relates QPainter

  <code>#include &lt;qdrawutil.h&gt;</code>

  Draws a plain rectangle given by (\a x, \a y, \a w, \a h)
  using the painter \a p.

  The color argument \a c specifies the line color.

  The \a lineWidth argument specifies the line width.

  The rectangle interior is filled with the \a fill brush unless \a fill
  is null.

  If you want to use a QFrame widget instead, you can make it display a
  plain rectangle, for example
  <code>QFrame::setFrameStyle( QFrame::Box | QFrame::Plain )</code>.

  \warning This function does not look at QWidget::style()
  or QApplication::style(). Use the drawing functions in QStyle to make
  widgets that follow the current GUI style.

  \sa qDrawShadeRect(), QStyle::drawRect()
*/

void qDrawPlainRect( QPainter *p, int x, int y, int w, int h, const QColor &c,
		     int lineWidth, const QBrush *fill )
{
    if ( w == 0 || h == 0 )
	return;
    if ( !( w > 0 && h > 0 && lineWidth >= 0 ) ) {
#if defined(CHECK_RANGE)
	qWarning( "qDrawPlainRect() Invalid parameters." );
#endif
    }
    QPen   oldPen   = p->pen();
    QBrush oldBrush = p->brush();
    p->setPen( c );
    p->setBrush( Qt::NoBrush );
    for ( int i=0; i<lineWidth; i++ )
	p->drawRect( x+i, y+i, w-i*2, h-i*2 );
    if ( fill ) {				// fill with fill color
	p->setPen( Qt::NoPen );
	p->setBrush( *fill );
	p->drawRect( x+lineWidth, y+lineWidth, w-lineWidth*2, h-lineWidth*2 );
    }
    p->setPen( oldPen );
    p->setBrush( oldBrush );
}


QRect qItemRect( QPainter *p, Qt::GUIStyle gs,
		int x, int y, int w, int h,
		int flags,
		bool enabled,
		const QPixmap *pixmap,
		const QString& text, int len )
{
    QRect result;

    if ( pixmap ) {
	if ( (flags & Qt::AlignVCenter) == Qt::AlignVCenter )
	    y += h/2 - pixmap->height()/2;
	else if ( (flags & Qt::AlignBottom) == Qt::AlignBottom)
	    y += h - pixmap->height();
	if ( (flags & Qt::AlignRight) == Qt::AlignRight )
	    x += w - pixmap->width();
	else if ( (flags & Qt::AlignHCenter) == Qt::AlignHCenter )
	    x += w/2 - pixmap->width()/2;
	result = QRect(x, y, pixmap->width(), pixmap->height());
    } else if ( !text.isNull() && p ) {
	result = p->boundingRect( x, y, w, h, flags, text, len );
	if ( gs == Qt::WindowsStyle && !enabled ) {
	    result.setWidth(result.width()+1);
	    result.setHeight(result.height()+1);
	}
    } else {
	result = QRect(x, y, w, h);
    }

    return result;
}


void qDrawItem( QPainter *p, Qt::GUIStyle gs,
		int x, int y, int w, int h,
		int flags,
		const QColorGroup &g, bool enabled,
		const QPixmap *pixmap,
		const QString& text, int len , const QColor* penColor )
{
    p->setPen( penColor?*penColor:g.foreground() );
    if ( pixmap ) {
	QPixmap  pm( *pixmap );
	bool clip = (flags & Qt::DontClip) == 0;
	if ( clip ) {
	    if ( pm.width() < w && pm.height() < h )
		clip = FALSE;
	    else
		p->setClipRect( x, y, w, h );
	}
	if ( (flags & Qt::AlignVCenter) == Qt::AlignVCenter )
	    y += h/2 - pm.height()/2;
	else if ( (flags & Qt::AlignBottom) == Qt::AlignBottom)
	    y += h - pm.height();
	if ( (flags & Qt::AlignRight) == Qt::AlignRight )
	    x += w - pm.width();
	else if ( (flags & Qt::AlignHCenter) == Qt::AlignHCenter )
	    x += w/2 - pm.width()/2;
	if ( !enabled ) {
	    if ( pm.mask() ) {			// pixmap with a mask
		if ( !pm.selfMask() ) {		// mask is not pixmap itself
		    QPixmap pmm( *pm.mask() );
		    pmm.setMask( *((QBitmap *)&pmm) );
		    pm = pmm;
		}
	    } else if ( pm.depth() == 1 ) {	// monochrome pixmap, no mask
		pm.setMask( *((QBitmap *)&pm) );
	    } else {				// color pixmap, no mask
		QString k;
		k.sprintf( "$qt-drawitem-%x", pm.serialNumber() );
		QPixmap *mask = QPixmapCache::find(k);
		bool del=FALSE;
		if ( !mask ) {
		    mask = new QPixmap( pm.createHeuristicMask() );
		    mask->setMask( *((QBitmap*)mask) );
		    del = !QPixmapCache::insert( k, mask );
		}
		pm = *mask;
		if (del) delete mask;
	    }
	    if ( gs == Qt::WindowsStyle ) {
		p->setPen( g.light() );
		p->drawPixmap( x+1, y+1, pm );
		p->setPen( g.text() );
	    }
	}
	p->drawPixmap( x, y, pm );
	if ( clip )
	    p->setClipping( FALSE );
    } else if ( !text.isNull() ) {
	if ( gs == Qt::WindowsStyle && !enabled ) {
	    p->setPen( g.light() );
	    p->drawText( x+1, y+1, w, h, flags, text, len );
	    p->setPen( g.text() );
	}
	p->drawText( x, y, w, h, flags, text, len );
    }
}


/*****************************************************************************
  Overloaded functions.
 *****************************************************************************/

/*!
  \overload void qDrawShadeLine( QPainter *p, const QPoint &p1, const QPoint &p2, const QColorGroup &g, bool sunken, int lineWidth, int midLineWidth )
*/

void qDrawShadeLine( QPainter *p, const QPoint &p1, const QPoint &p2,
		     const QColorGroup &g, bool sunken,
		     int lineWidth, int midLineWidth )
{
    qDrawShadeLine( p, p1.x(), p1.y(), p2.x(), p2.y(), g, sunken,
		    lineWidth, midLineWidth );
}

/*!
  \overload void qDrawShadeRect( QPainter *p, const QRect &r, const QColorGroup &g, bool sunken, int lineWidth, int midLineWidth, const QBrush *fill )
*/

void qDrawShadeRect( QPainter *p, const QRect &r,
		     const QColorGroup &g, bool sunken,
		     int lineWidth, int midLineWidth,
		     const QBrush *fill )
{
    qDrawShadeRect( p, r.x(), r.y(), r.width(), r.height(), g, sunken,
		    lineWidth, midLineWidth, fill );
}

/*!
  \overload void qDrawShadePanel( QPainter *p, const QRect &r, const QColorGroup &g, bool sunken, int lineWidth, const QBrush *fill )
*/

void qDrawShadePanel( QPainter *p, const QRect &r,
		      const QColorGroup &g, bool sunken,
		      int lineWidth, const QBrush *fill )
{
    qDrawShadePanel( p, r.x(), r.y(), r.width(), r.height(), g, sunken,
		     lineWidth, fill );
}

/*!
  \overload void qDrawWinButton( QPainter *p, const QRect &r, const QColorGroup &g, bool sunken, const QBrush *fill )
*/

void qDrawWinButton( QPainter *p, const QRect &r,
		     const QColorGroup &g, bool sunken,
		     const QBrush *fill )
{
    qDrawWinButton( p, r.x(), r.y(), r.width(), r.height(), g, sunken, fill );
}

/*!
  \overload void qDrawWinPanel( QPainter *p, const QRect &r, const QColorGroup &g, bool sunken, const QBrush *fill )
*/

void qDrawWinPanel( QPainter *p, const QRect &r,
		    const QColorGroup &g, bool sunken,
		    const QBrush *fill )
{
    qDrawWinPanel( p, r.x(), r.y(), r.width(), r.height(), g, sunken, fill );
}

/*!
  \overload void qDrawPlainRect( QPainter *p, const QRect &r, const QColor &c, int lineWidth, const QBrush *fill )
*/

void qDrawPlainRect( QPainter *p, const QRect &r, const QColor &c,
		     int lineWidth, const QBrush *fill )
{
    qDrawPlainRect( p, r.x(), r.y(), r.width(), r.height(), c,
		    lineWidth, fill );
}


static void qDrawWinArrow( QPainter *p, Qt::ArrowType type, bool down,
			   int x, int y, int w, int h,
			   const QColorGroup &g, bool enabled )
{
    QPointArray a;				// arrow polygon
    switch ( type ) {
    case Qt::UpArrow:
	a.setPoints( 7, -3,1, 3,1, -2,0, 2,0, -1,-1, 1,-1, 0,-2 );
	break;
    case Qt::DownArrow:
	a.setPoints( 7, -3,-1, 3,-1, -2,0, 2,0, -1,1, 1,1, 0,2 );
	break;
    case Qt::LeftArrow:
	a.setPoints( 7, 1,-3, 1,3, 0,-2, 0,2, -1,-1, -1,1, -2,0 );
	break;
    case Qt::RightArrow:
	a.setPoints( 7, -1,-3, -1,3, 0,-2, 0,2, 1,-1, 1,1, 2,0 );
	break;
    }
    if ( a.isNull() )
	return;

    if ( down ) {
	x++;
	y++;
    }

    QPen savePen = p->pen();			// save current pen
    if (down)
	p->setBrushOrigin(p->brushOrigin() + QPoint(1,1));
    p->fillRect( x, y, w, h, g.brush( QColorGroup::Button ) );
    if (down)
	p->setBrushOrigin(p->brushOrigin() - QPoint(1,1));
    if ( enabled ) {
	a.translate( x+w/2, y+h/2 );
	p->setPen( g.foreground() );
	p->drawLineSegments( a, 0, 3 );		// draw arrow
	p->drawPoint( a[6] );
    } else {
	a.translate( x+w/2+1, y+h/2+1 );
	p->setPen( g.light() );
	p->drawLineSegments( a, 0, 3 );		// draw arrow
	p->drawPoint( a[6] );
	a.translate( -1, -1 );
	p->setPen( g.mid() );
	p->drawLineSegments( a, 0, 3 );		// draw arrow
	p->drawPoint( a[6] );
    }
    p->setPen( savePen );			// restore pen
}


#if defined(_CC_MSVC_)
#pragma warning(disable: 4244)
#endif


// motif arrows look the same whether they are used or not
// is this correct?
static void qDrawMotifArrow( QPainter *p, Qt::ArrowType type, bool down,
			     int x, int y, int w, int h,
			     const QColorGroup &g, bool )
{
    QPointArray bFill;				// fill polygon
    QPointArray bTop;				// top shadow.
    QPointArray bBot;				// bottom shadow.
    QPointArray bLeft;				// left shadow.
#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix	matrix;				// xform matrix
#endif
    bool vertical = type == Qt::UpArrow || type == Qt::DownArrow;
    bool horizontal = !vertical;
    int	 dim = w < h ? w : h;
    int	 colspec = 0x0000;			// color specification array

    if ( dim < 2 )				// too small arrow
	return;

    if ( dim > 3 ) {
	if ( dim > 6 )
	    bFill.resize( dim & 1 ? 3 : 4 );
	bTop.resize( (dim/2)*2 );
	bBot.resize( dim & 1 ? dim + 1 : dim );
	bLeft.resize( dim > 4 ? 4 : 2 );
	bLeft.putPoints( 0, 2, 0,0, 0,dim-1 );
	if ( dim > 4 )
	    bLeft.putPoints( 2, 2, 1,2, 1,dim-3 );
	bTop.putPoints( 0, 4, 1,0, 1,1, 2,1, 3,1 );
	bBot.putPoints( 0, 4, 1,dim-1, 1,dim-2, 2,dim-2, 3,dim-2 );

	for( int i=0; i<dim/2-2 ; i++ ) {
	    bTop.putPoints( i*2+4, 2, 2+i*2,2+i, 5+i*2, 2+i );
	    bBot.putPoints( i*2+4, 2, 2+i*2,dim-3-i, 5+i*2,dim-3-i );
	}
	if ( dim & 1 )				// odd number size: extra line
	    bBot.putPoints( dim-1, 2, dim-3,dim/2, dim-1,dim/2 );
	if ( dim > 6 ) {			// dim>6: must fill interior
	    bFill.putPoints( 0, 2, 1,dim-3, 1,2 );
	    if ( dim & 1 )			// if size is an odd number
		bFill.setPoint( 2, dim - 3, dim / 2 );
	    else
		bFill.putPoints( 2, 2, dim-4,dim/2-1, dim-4,dim/2 );
	}
    }
    else {
	if ( dim == 3 ) {			// 3x3 arrow pattern
	    bLeft.setPoints( 4, 0,0, 0,2, 1,1, 1,1 );
	    bTop .setPoints( 2, 1,0, 1,0 );
	    bBot .setPoints( 2, 1,2, 2,1 );
	}
	else {					// 2x2 arrow pattern
	    bLeft.setPoints( 2, 0,0, 0,1 );
	    bTop .setPoints( 2, 1,0, 1,0 );
	    bBot .setPoints( 2, 1,1, 1,1 );
	}
    }

    if ( type == Qt::UpArrow || type == Qt::LeftArrow ) {
#ifndef QT_NO_TRANSFORMATIONS	// #### fix me!
	matrix.translate( x, y );
	if ( vertical ) {
	    matrix.translate( 0, h - 1 );
	    matrix.rotate( -90 );
	} else {
	    matrix.translate( w - 1, h - 1 );
	    matrix.rotate( 180 );
	}
#endif
	if ( down )
	    colspec = horizontal ? 0x2334 : 0x2343;
	else
	    colspec = horizontal ? 0x1443 : 0x1434;
    }
    else if ( type == Qt::DownArrow || type == Qt::RightArrow ) {
#ifndef QT_NO_TRANSFORMATIONS	// #### fix me!
	matrix.translate( x, y );
	if ( vertical ) {
	    matrix.translate( w-1, 0 );
	    matrix.rotate( 90 );
	}
#endif
	if ( down )
	    colspec = horizontal ? 0x2443 : 0x2434;
	else
	    colspec = horizontal ? 0x1334 : 0x1343;
    }

    QColor *cols[5];
    cols[0] = 0;
    cols[1] = (QColor *)&g.button();
    cols[2] = (QColor *)&g.mid();
    cols[3] = (QColor *)&g.light();
    cols[4] = (QColor *)&g.dark();
#define CMID	*cols[ (colspec>>12) & 0xf ]
#define CLEFT	*cols[ (colspec>>8) & 0xf ]
#define CTOP	*cols[ (colspec>>4) & 0xf ]
#define CBOT	*cols[ colspec & 0xf ]

    QPen     savePen   = p->pen();		// save current pen
    QBrush   saveBrush = p->brush();		// save current brush
#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix wxm = p->worldMatrix();
#endif
    QPen     pen( Qt::NoPen );
    QBrush brush = g.brush( QColorGroup::Button );

    p->setPen( pen );
    p->setBrush( brush );
#ifndef QT_NO_TRANSFORMATIONS
    p->setWorldMatrix( matrix, TRUE );		// set transformation matrix
#endif
    p->drawPolygon( bFill );			// fill arrow
    p->setBrush( Qt::NoBrush );			// don't fill

    p->setPen( CLEFT );
    p->drawLineSegments( bLeft );
    p->setPen( CTOP );
    p->drawLineSegments( bTop );
    p->setPen( CBOT );
    p->drawLineSegments( bBot );

#ifndef QT_NO_TRANSFORMATIONS
    p->setWorldMatrix( wxm );
#endif
    p->setBrush( saveBrush );			// restore brush
    p->setPen( savePen );			// restore pen

#undef CMID
#undef CLEFT
#undef CTOP
#undef CBOT
}


void qDrawArrow( QPainter *p, Qt::ArrowType type, Qt::GUIStyle style, bool down,
		 int x, int y, int w, int h,
		 const QColorGroup &g, bool enabled )
{
    switch ( style ) {
	case Qt::WindowsStyle:
	    qDrawWinArrow( p, type, down, x, y, w, h, g, enabled );
	    break;
	case Qt::MotifStyle:
	    qDrawMotifArrow( p, type, down, x, y, w, h, g, enabled );
	    break;
	default:
#if defined(CHECK_RANGE)
	    qWarning( "qDrawArrow: Requested GUI style not supported" );
#else
	    ;
#endif
    }
}
#endif //QT_NO_DRAWUTIL
