/****************************************************************************
** $Id$
**
** Implementation of QIconSet class
**
** Created : 980318
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

#include "qiconset.h"

#ifndef QT_NO_ICONSET

#include "qimage.h"
#include "qbitmap.h"
#include "qapplication.h"
#include "qpainter.h"


struct QIconSetPrivate: public QShared
{
    struct Variant {
	Variant(): pm(0), generated(0) {}
	~Variant()
	{
	    delete pm;
	}
	void clearGenerate()
	{
	    if ( generated ) {
		delete pm;
		pm = 0;
		generated = FALSE;
	    }
	}
	QPixmap * pm;
	bool generated;
    };
    Variant small;
    Variant large;
    Variant smallActive;
    Variant largeActive;
    Variant smallDisabled;
    Variant largeDisabled;
    QPixmap defpm;
};


// REVISED: warwick
/*! \class QIconSet qiconset.h

  \brief The QIconSet class provides a set of differently styled and sized
  icons.

  \ingroup misc
  \ingroup shared

  Once a QIconSet is fed some pixmaps,
  it can generate smaller, larger, active and disabled pixmaps.
  Such pixmaps are used by
  QToolButton, QHeader, QPopupMenu, etc. to show an icon representing
  a piece of functionality.

  The simplest usage of QIconSet is to create one from a QPixmap then
  use it, allowing Qt to work out all the required icon sizes. For example:

  \code
  QToolButton *tb = new QToolButton( QIconSet( QPixmap("open.xpm") ), ... );
  \endcode

  Using whichever pixmaps you specify as a base,
  QIconSet provides a set of six icons each with
    a \link QIconSet::Size Size\endlink and
    a \link QIconSet::Mode Mode\endlink:
  <ul>
  <li> <i>Small Normal</i> - can only be calculated from Large Normal.
  <li> <i>Small Disabled</i> - calculated from Large Disabled or Small Normal.
  <li> <i>Small Active</i> - same as Small Normal unless you set it.
  <li> <i>Large Normal</i> - can only be calculated from Small Normal.
  <li> <i>Large Disabled</i> - calculated from Small Disabled or Large Normal.
  <li> <i>Large Active</i> - same as Large Normal unless you set it.
  </ul>

  You can set any of the icons using setPixmap() and when you retrieve
  one using pixmap(Size,Mode), QIconSet will compute it from the
  closest other icon and cache it for later.

  The \c Disabled appearance is computed using a "shadow" algorithm
  which produces results very similar to that used in Microsoft
  Windows 95.

  The \c Active appearance is identical to the \c Normal appearance
  unless you use setPixmap() to set it to something special.

  When scaling icons, QIconSet uses \link QImage::smoothScale
  smooth scaling\endlink, which can partially blend the color component
  of pixmaps.  If the results look poor, the best solution
  is to supply both large and small sizes of pixmap.

  QIconSet provides a function, isGenerated(), that indicates whether
  an icon was set by the application programmer or computed by
  QIconSet itself.

  <h3>Making classes that use QIconSet</h3>

  If you write your own widgets that have an option to set a small pixmap,
  you should consider instead, or additionally, allowing a QIconSet to be
  set for that pixmap.  The Qt class QToolButton is an example of such
  a widget.

  Provide a method to set a QIconSet, and when you draw the icon, choose
  whichever icon is appropriate for the current state of your widget.
  For example:
  \code
  void YourWidget::drawIcon( QPainter* p, QPoint pos )
  {
      p->drawPixmap( pos, icons->pixmap(isEnabled(), QIconSet::Small) );
  }
  \endcode

  You might also make use of the Active mode, perhaps making your widget
  Active when the mouse in inside the widget (see QWidget::enterEvent),
  while the mouse is pressed pending the release that will activate
  the function, or when it is the currently selected item.

  \sa QPixmap QLabel QToolButton QPopupMenu
	QIconViewItem::setViewMode() QMainWindow::setUsesBigPixmaps()
  <a href="guibooks.html#fowler">GUI Design Handbook: Iconic Label</a>
  <a href="http://cgl.microsoft.com/clipgallerylive/cgl30/eula.asp?nInterface=0">Microsoft
  Icon Gallery.</a>
*/


/*!
  \enum QIconSet::Size

  This enum type describes the size for which a pixmap is intended to be
  provided.
  The currently defined sizes are:

  <ul>
    <li> \c Automatic - the size of the pixmap is determined from its
		    pixel size. This is a useful default.
    <li> \c Small - the pixmap is the smaller of two.
    <li> \c Large - the pixmap is the larger of two
  </ul>

  If a Small pixmap is not set by QIconSet::setPixmap(), then the
  Large pixmap may be automatically scaled to two-thirds of its size to
  generate the Small pixmap.  Conversely, a Small pixmap will be
  automatically scaled up by 50% to create a Large pixmap if needed.

  \sa setPixmap() pixmap() QIconViewItem::setViewMode()
      QMainWindow::setUsesBigPixmaps()
*/

/*!
  \enum QIconSet::Mode

  This enum type describes the mode for which a pixmap is intended to be
  provided.
  The currently defined modes are:

  <ul>
    <li> \c Normal
	- the pixmap to be displayed when the user is
	not interacting with the icon, but when the
	functionality represented by the icon is available.
    <li> \c Disabled
	- the pixmap to be displayed when the
	functionality represented by the icon is not available.
    <li> \c Active
	- the pixmap to be displayed when the
	functionality represented by the icon is available and
	the user is interacting with the icon, such as by pointing
	at it or by invoking it.
  </ul>
*/


/*!
  Constructs an icon set of \link QPixmap::isNull() null\endlink pixmaps.
  Use setPixmap(), reset(), or operator=() to set some pixmaps.

  \sa reset()
*/
QIconSet::QIconSet()
{
    d = 0;
    reset( QPixmap(), Automatic );
}


/*!  Constructs an icon set for which the Normal pixmap is
  \a pixmap, which is assumed to be the given \a size.

  The default for \a size is \c Automatic, which means that
  QIconSet will determine if the pixmap is Small or Large
  from its pixel size.
  Pixmaps less than 23 pixels wide are considered to be Small.

  \sa reset()
*/
QIconSet::QIconSet( const QPixmap& pixmap, Size size )
{
    d = 0;
    reset( pixmap, size );
}


/*!
  Constructs a copy of \a other.  This is very fast.
*/
QIconSet::QIconSet( const QIconSet& other )
{
    d = other.d;
    if ( d )
	d->ref();
}

/*!  Creates an iconset which uses the pixmap \a smallPix for for
  displaying small a small icon, and \a largePix for displaying a large
  one.
*/

QIconSet::QIconSet( const QPixmap &smallPix, const QPixmap &largePix )
{
    d = 0;
    reset( smallPix, Small );
    reset( largePix, Large );
}

/*!
  Destructs the icon set and frees any allocated resources.
*/
QIconSet::~QIconSet()
{
    if ( d && d->deref() )
	delete d;
}

/*!
  Assigns \a other to this icon set and returns a reference to this
  icon set.

  This is very fast.

  \sa detach()
*/
QIconSet &QIconSet::operator=( const QIconSet &other )
{
    if ( other.d ) {
	other.d->ref();				// beware of other = other
	if ( d->deref() )
	    delete d;
	d = other.d;
	return *this;
    } else {
	if ( d && d->deref() )
	    delete d;
	d = 0;
	return *this;
    }
}

/*!
  Returns TRUE if the icon set is empty. Currently, a QIconSet
  is never empty (although it may contain null pixmaps).
*/
bool QIconSet::isNull() const
{
    return ( d == 0 );
}

/*!
  Sets this icon set to provide \a pm for the Normal pixmap,
  assuming it to be of size \a s.

  This is equivalent to assigning QIconSet(pm,s) to this icon set.
*/
void QIconSet::reset( const QPixmap & pm, Size s )
{
    detach();
    if ( s == Small ||
	 (s == Automatic && pm.width() <= 22 ) )
	setPixmap( pm, Small, Normal );
    else
	setPixmap( pm, Large, Normal );
    d->defpm = pm;
}


/*!
  Sets this icon set to provide \a pm for a \a size and \a mode.
  It may also use \a pm for deriving some other varieties if those
  are not set.

  The \a size can be one of Automatic, Large or Small.  If Automatic is used,
  QIconSet will determine if the pixmap is Small or Large from its pixel size.
  Pixmaps less than 23 pixels wide are considered to be Small.

  \sa reset()
*/
void QIconSet::setPixmap( const QPixmap & pm, Size size, Mode mode )
{
    detach();
    if ( d ) {
	d->small.clearGenerate();
	d->large.clearGenerate();
	d->smallDisabled.clearGenerate();
	d->largeDisabled.clearGenerate();
	d->smallActive.clearGenerate();
	d->largeActive.clearGenerate();
    } else {
	d = new QIconSetPrivate;
    }
    if ( size == Large || (size == Automatic && pm.width() > 22)) {
	switch( mode ) {
	case Active:
	    if ( !d->largeActive.pm )
		d->largeActive.pm = new QPixmap();
	    *d->largeActive.pm = pm;
	    break;
	case Disabled:
	    if ( !d->largeDisabled.pm )
		d->largeDisabled.pm = new QPixmap();
	    *d->largeDisabled.pm = pm;
	    break;
	case Normal:
	default:
	    if ( !d->large.pm )
		d->large.pm = new QPixmap();
	    *d->large.pm = pm;
	    break;
	}
    } else if ( size == Small  || (size == Automatic && pm.width() <= 22)) {
	switch( mode ) {
	case Active:
	    if ( !d->smallActive.pm )
		d->smallActive.pm = new QPixmap();
	    *d->smallActive.pm = pm;
	    break;
	case Disabled:
	    if ( !d->smallDisabled.pm )
		d->smallDisabled.pm = new QPixmap();
	    *d->smallDisabled.pm = pm;
	    break;
	case Normal:
	default:
	    if ( !d->small.pm )
		d->small.pm = new QPixmap();
	    *d->small.pm = pm;
	    break;
	}
#if defined(CHECK_RANGE)
    } else {
	qWarning("QIconSet::setPixmap: invalid size passed");
#endif
    }
}


/*!
  Sets this icon set to load \a fileName as a pixmap and provide it
  for size \a s and mode \a m.
  It may also use the pixmap for deriving some other varieties if those
  are not set.

  The \a size can be one of Automatic, Large or Small.  If Automatic is used,
  QIconSet will determine if the pixmap is Small or Large from its pixel size.
  Pixmaps less than 23 pixels wide are considered to be Small.
*/
void QIconSet::setPixmap( const QString &fileName, Size s, Mode m )
{
    QPixmap p;
    p.load( fileName );
    if ( !p.isNull() )
	setPixmap( p, s, m );
}


/*!
  Returns a pixmap with size \a s and mode \a m, generating one if
  needed. Generated pixmaps are cached.
*/
QPixmap QIconSet::pixmap( Size s, Mode m ) const
{
    if ( !d ) {
	QPixmap r;
	return r;
    }

    QImage i;
    QIconSetPrivate * p = ((QIconSet *)this)->d;
    QPixmap * pm = 0;
    if ( s == Large ) {
	switch( m ) {
	case Normal:
	    if ( !p->large.pm ) {
		ASSERT( p->small.pm );
		i = p->small.pm->convertToImage();
		i = i.smoothScale( i.width() * 3 / 2, i.height() * 3 / 2 );
		p->large.pm = new QPixmap;
		p->large.generated = TRUE;
		p->large.pm->convertFromImage( i );
		if ( !p->large.pm->mask() ) {
		    i = i.createHeuristicMask();
		    QBitmap tmp;
		    tmp.convertFromImage( i, Qt::MonoOnly + Qt::ThresholdDither );
		    p->large.pm->setMask( tmp );
		}
	    }
	    pm = p->large.pm;
	    break;
	case Active:
	    if ( !p->largeActive.pm ) {
		p->largeActive.pm = new QPixmap( pixmap( Large, Normal ) );
		p->largeActive.generated = TRUE;
	    }
	    pm = p->largeActive.pm;
	    break;
	case Disabled:
	    if ( !p->largeDisabled.pm ) {
		QBitmap tmp;
		if ( p->large.generated && !p->smallDisabled.generated &&
		     p->smallDisabled.pm && !p->smallDisabled.pm->isNull() ) {
		    // if there's a hand-drawn disabled small image,
		    // but the normal big one is generated, use the
		    // hand-drawn one to generate this one.
		    i = p->smallDisabled.pm->convertToImage();
		    i = i.smoothScale( i.width() * 3 / 2, i.height() * 3 / 2 );
		    p->largeDisabled.pm = new QPixmap;
		    p->largeDisabled.pm->convertFromImage( i );
		    if ( !p->largeDisabled.pm->mask() ) {
			i = i.createHeuristicMask();
			tmp.convertFromImage( i, Qt::MonoOnly + Qt::ThresholdDither );
		    }
		} else {
		    if (pixmap( Large, Normal).mask())
			tmp = *pixmap( Large, Normal).mask();
		    else {
			QPixmap conv = pixmap( Large, Normal );
			if ( !conv.isNull() ) {
			    i = conv.convertToImage();
			    i = i.createHeuristicMask();
			    tmp.convertFromImage( i, Qt::MonoOnly + Qt::ThresholdDither );
			}
		    }
		    p->largeDisabled.pm
			= new QPixmap( p->large.pm->width()+1,
				       p->large.pm->height()+1);
		    QColorGroup dis( QApplication::palette().disabled() );
		    p->largeDisabled.pm->fill( dis.background() );
		    QPainter painter( p->largeDisabled.pm );
		    painter.setPen( dis.base() );
		    painter.drawPixmap( 1, 1, tmp );
		    painter.setPen( dis.foreground() );
		    painter.drawPixmap( 0, 0, tmp );
		}
		if ( !p->largeDisabled.pm->mask() ) {
		    if ( !tmp.mask() )
			tmp.setMask( tmp );
		    QBitmap mask( d->largeDisabled.pm->size() );
		    mask.fill( Qt::color0 );
		    QPainter painter( &mask );
		    painter.drawPixmap( 0, 0, tmp );
		    painter.drawPixmap( 1, 1, tmp );
		    painter.end();
		    p->largeDisabled.pm->setMask( mask );
		}
		p->largeDisabled.generated = TRUE;
	    }
	    pm = p->largeDisabled.pm;
	    break;
	}
    } else {
	switch( m ) {
	case Normal:
	    if ( !p->small.pm ) {
		ASSERT( p->large.pm );
		i = p->large.pm->convertToImage();
		i = i.smoothScale( i.width() * 2 / 3, i.height() * 2 / 3 );
		p->small.pm = new QPixmap;
		p->small.generated = TRUE;
		p->small.pm->convertFromImage( i );
		if ( !p->small.pm->mask() ) {
		    i = i.createHeuristicMask();
		    QBitmap tmp;
		    tmp.convertFromImage( i, Qt::MonoOnly + Qt::ThresholdDither );
		    p->small.pm->setMask( tmp );
		}
	    }
	    pm = p->small.pm;
	    break;
	case Active:
	    if ( !p->smallActive.pm ) {
		p->smallActive.pm = new QPixmap( pixmap( Small, Normal ) );
		p->smallActive.generated = TRUE;
	    }
	    pm = p->smallActive.pm;
	    break;
	case Disabled:
	    if ( !p->smallDisabled.pm ) {
		QBitmap tmp;
		if ( p->small.generated && !p->largeDisabled.generated &&
		     p->largeDisabled.pm && !p->largeDisabled.pm->isNull() ) {
		    // if there's a hand-drawn disabled large image,
		    // but the normal small one is generated, use the
		    // hand-drawn one to generate this one.
		    i = p->largeDisabled.pm->convertToImage();
		    i = i.smoothScale( i.width() * 3 / 2, i.height() * 3 / 2 );
		    p->smallDisabled.pm = new QPixmap;
		    p->smallDisabled.pm->convertFromImage( i );
		    if ( !p->smallDisabled.pm->mask() ) {
			i = i.createHeuristicMask();
			tmp.convertFromImage( i, Qt::MonoOnly + Qt::ThresholdDither );
		    }
		} else {
		    if ( pixmap( Small, Normal).mask())
			tmp = *pixmap( Small, Normal).mask();
		    else {
			QPixmap conv = pixmap( Small, Normal );
			if ( !conv.isNull() ) {
			    i = conv.convertToImage();
			    i = i.createHeuristicMask();
			    tmp.convertFromImage( i, Qt::MonoOnly + Qt::ThresholdDither );
			}
		    }
		    p->smallDisabled.pm
			= new QPixmap( p->small.pm->width()+1,
				       p->small.pm->height()+1);
		    QColorGroup dis( QApplication::palette().disabled() );
		    p->smallDisabled.pm->fill( dis.background() );
		    QPainter painter( p->smallDisabled.pm );
		    painter.setPen( dis.base() );
		    painter.drawPixmap( 1, 1, tmp );
		    painter.setPen( dis.foreground() );
		    painter.drawPixmap( 0, 0, tmp );
		}
		if ( !p->smallDisabled.pm->mask() ) {
 		    if ( !tmp.mask() )
 			tmp.setMask( tmp );
		    QBitmap mask( d->smallDisabled.pm->size() );
		    mask.fill( Qt::color0 );
		    QPainter painter( &mask );
		    painter.drawPixmap( 0, 0, tmp );
		    painter.drawPixmap( 1, 1, tmp );
		    painter.end();
		    p->smallDisabled.pm->setMask( mask );
		}

		p->smallDisabled.generated = TRUE;
	    }
	    pm = p->smallDisabled.pm;
	    break;
	}
    }
    ASSERT( pm );
    return *pm;
}


/*!
  Returns a pixmap with size \a s, and Mode either Normal or Disabled,
  depending on the value of \a enabled.
*/
QPixmap QIconSet::pixmap( Size s, bool enabled ) const
{
    return pixmap( s, enabled ? Normal : Disabled );
}


/*!
  Returns TRUE if the variant with size \a s and mode \a m was
  automatically generated, and FALSE if it was not. This mainly
  useful for development purposes.
*/
bool QIconSet::isGenerated( Size s, Mode m ) const
{
    if ( !d )
	return FALSE;

    if ( s == Large ) {
	if ( m == Disabled )
	    return d->largeDisabled.generated || !d->largeDisabled.pm;
	else if ( m == Active )
	    return d->largeActive.generated || !d->largeActive.pm;
	else
	    return d->large.generated || !d->large.pm;
    } else if ( s == Small ) {
	if ( m == Disabled )
	    return d->smallDisabled.generated || !d->smallDisabled.pm;
	else if ( m == Active )
	    return d->smallActive.generated || !d->smallActive.pm;
	else
	    return d->small.generated || !d->small.pm;
    }
    return FALSE;
}


/*!
  Returns the pixmap originally provided to the constructor or
  to reset().  This is the Normal pixmap of unspecified Size.

  \sa reset()
*/

QPixmap QIconSet::pixmap() const
{
    if ( !d )
	return QPixmap();

    return d->defpm;
}


/*!
  Detaches this icon set from others with which it may share data.

  You will never need to call this function; other QIconSet functions
  call it as necessary.
*/
void QIconSet::detach()
{
    if ( !d )
    {
	d = new QIconSetPrivate;
	return;
    }

    if ( d->count == 1 )
	return;

    QIconSetPrivate * p = new QIconSetPrivate;
    p->small.pm = d->small.pm;
    p->small.generated = d->small.generated;
    p->smallActive.pm = d->smallActive.pm;
    p->smallActive.generated = d->smallActive.generated;
    p->smallDisabled.pm = d->smallDisabled.pm;
    p->smallDisabled.generated = d->smallDisabled.generated;
    p->large.pm = d->large.pm;
    p->large.generated = d->large.generated;
    p->largeActive.pm = d->largeActive.pm;
    p->largeActive.generated = d->largeActive.generated;
    p->largeDisabled.pm = d->largeDisabled.pm;
    p->largeDisabled.generated = d->largeDisabled.generated;
    p->defpm = d->defpm;
    d->deref();
    d = p;
}

#endif // QT_NO_ICONSET
