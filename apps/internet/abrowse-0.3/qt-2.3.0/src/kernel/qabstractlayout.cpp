/****************************************************************************
** $Id$
**
** Implementation of the abstract layout base class
**
** Created : 960416
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

#include "qabstractlayout.h"

#ifndef QT_NO_LAYOUT
#include "qwidget.h"
#include "qmenubar.h"
#include "qapplication.h"

// NOT REVISED
/*!
  \class QLayoutItem qabstractlayout.h
  \brief The abstract items which a QLayout manipulates.

  For custom layouts.

  \sa QLayout
*/

/*!
  \class QSpacerItem qabstractlayout.h
  \brief The QLayoutItem class that represents blank space.

  For custom layouts.

  \sa QLayout
*/

/*!
  \class QWidgetItem qabstractlayout.h
  \brief A QLayoutItem that represents widget.

  For custom layouts.

  \sa QLayout
*/




/*! \fn QLayoutItem::QLayoutItem (int alignment)
  Constructs a layout item with an \a alignment
  that is a bitwise OR of Qt::AlignmentFlags.
  Alignment may not be supported by all subclasses.
 */

/*! \fn int QLayoutItem::alignment () const
  Returns the alignment of this item.
*/


/*! Sets the alignment of this item to \a a,
  which is a bitwise OR of Qt::AlignmentFlags.
*/

void QLayoutItem::setAlignment( int a )
{
     align = a;
}


/*! \fn  QSize QLayoutItem::maximumSize () const
  Implemented in subclasses to return the maximum size of this item.
*/

/*! \fn QSize QLayoutItem::minimumSize () const
  Implemented in subclasses to return the minimum size of this item.
*/

/*! \fn QSize QLayoutItem::sizeHint () const
  Implemented in subclasses to return the preferred size of this item.
*/

/*! \fn QSizePolicy::ExpandData QLayoutItem::expanding () const
  Implemented in subclasses to return whether this item "wants" to expand.
*/

/*! \fn void QLayoutItem::setGeometry (const QRect &r )
  Implemented in subclasses to set this item's geometry to \a r.
*/

/*!
  \fn QRect QLayoutItem::geometry() const

  Returns the rectangle covered by this layout item.
 */



/*! \fn virtual bool QLayoutItem::isEmpty () const
  Implemented in subclasses to return whether this item is empty,
  i.e. whether it contains any widgets.
*/


/*! \fn QSpacerItem::QSpacerItem (int w, int h, QSizePolicy::SizeType hData=QSizePolicy::Minimum, QSizePolicy::SizeType vData= QSizePolicy::Minimum)

  Constructs a spacer item with preferred width \a w, preferred height
  \a h, horizontal size policy \a hData and vertical size policy
  \a vData.

  The default values gives a gap that is able to stretch,
  if nothing else wants the space.
*/


/*!
  Changes this spacer item to have preferred width \a w, preferred height
  \a h, horizontal size policy \a hData and vertical size policy
  \a vData.

  The default values gives a gap that is able to stretch,
  if nothing else wants the space.
*/
void QSpacerItem::changeSize( int w, int h, QSizePolicy::SizeType hData,
			      QSizePolicy::SizeType vData )
{
    width = w;
    height = h;
    sizeP = QSizePolicy( hData, vData );
}


/*! \fn QWidgetItem::QWidgetItem (QWidget * w)

  Creates an item containing \a w.
*/

 /*!
  Destructs the QLayoutItem.
*/
QLayoutItem::~QLayoutItem()
{
}


/*!
  Invalidates any cached information in this layout item.
*/

void QLayoutItem::invalidate()
{

}




/*!
  If this item is a QLayout, return it as a QLayout, otherwise return 0.
  This function provides type-safe casting.
*/

QLayout * QLayoutItem::layout()
{
    return 0;
}




/*!
  If this item is a QSpacerItem, return it as a QSpacerItem, otherwise
  return 0.  This function provides type-safe casting.

*/

QSpacerItem * QLayoutItem::spacerItem()
{
    return 0;
}




/*!
  \reimp
*/

QLayout * QLayout::layout()
{
    return this;
}


/*!
  \reimp
*/

QSpacerItem * QSpacerItem::spacerItem()
{
    return this;
}




/*!
  If this item is a QWidgetItem, the managed widget is returned.
  The default implementation returns 0;
*/

QWidget * QLayoutItem::widget()
{
    return 0;
}


/*!
  Returns the widget managed by this item.
*/

QWidget * QWidgetItem::widget()
{
    return wid;
}


/*!
  Returns TRUE if this layout's preferred height depends on its
  width. The default implementation returns FALSE;

  Reimplement this function in layout managers that support
  height for width.

  \sa heightForWidth(), QWidget::heightForWidth()
*/

bool QLayoutItem::hasHeightForWidth() const
{
    return FALSE;
}





/*!
  Returns an iterator over this item's QLayoutItem children.
  The default implementation returns an empty iterator.

  Reimplement this function in subclasses that can have
  children.
*/

QLayoutIterator QLayoutItem::iterator()
{
    return QLayoutIterator( 0 );
}



/*!
  Returns the preferred height for this layout item, given the width
  \a w.

  The default implementation returns -1, indicating that the preferred
  height is independent of the width of the item.  Using the function
  hasHeightForWidth() will typically be much faster than calling this
  function and testing for -1.

  Reimplement this function in layout managers that support
  height for width. A typical implementation will look like this:
  \code
  int MyLayout::heightForWidth( int w ) const
  {
      if ( cache_dirty || cached_width != w ) {
	  //Not all C++ compilers support "mutable" yet:
	  MyLayout * mthis = (MyLayout*)this;
	  int h = calculateHeightForWidth( w );
	  mthis->cached_hfw = h;
	  return h;
      }
      return cached_hfw;
  }
  \endcode

  Caching is strongly recommended, without it layout will take
  exponential time.  \sa hasHeightForWidth()
*/

int QLayoutItem::heightForWidth( int ) const
{
    return -1;
}



static const int HorAlign = Qt::AlignHCenter | Qt::AlignRight | Qt::AlignLeft;
static const int VerAlign = Qt::AlignVCenter | Qt::AlignBottom | Qt::AlignTop;

static QSize smartMinSize( const QWidgetItem *i )
{
    QWidget *w = ( (QWidgetItem*)i )->widget();
    QSize s(0,0);
    if ( w->layout() ) {
	s = w->layout()->totalMinimumSize();
    } else {
	QSize sh;
	if ( w->sizePolicy().mayShrinkHorizontally() )
	    s.setWidth( w->minimumSizeHint().width() );
	else {
	    sh = w->sizeHint();
	    s.setWidth( sh.width() );
	}
	if ( w->sizePolicy().mayShrinkVertically() )
	    s.setHeight( w->minimumSizeHint().height() );
	else
	    s.setHeight( sh.isValid() ? sh.height() : w->sizeHint().height() );
    }
    s = s.boundedTo( w->maximumSize() );
    QSize min = w->minimumSize();
    if ( min.width() > 0 )
	s.setWidth( min.width() );
    if ( min.height() > 0 )
	s.setHeight( min.height() );

    if ( i->hasHeightForWidth() && min.height() == 0 && min.width() > 0 )
	s.setHeight( i->heightForWidth( s.width() ) );
    
    s = s.expandedTo( QSize(1,1) );
    return s;
}

//returns the max size of a box containing \a w with alignment \a align.
static QSize smartMaxSize( QWidget *w, int align = 0 )
{
    if ( align & HorAlign && align & VerAlign )
	return QSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
    QSize s = w->maximumSize();
    if ( s.width() == QWIDGETSIZE_MAX && !(align&HorAlign) )
	if ( !w->sizePolicy().mayGrowHorizontally() )
	    s.setWidth( w->sizeHint().width() );

    if ( s.height() ==  QWIDGETSIZE_MAX && !(align&VerAlign) )
	if ( !w->sizePolicy().mayGrowVertically() )
	    s.setHeight( w->sizeHint().height() );

    s = s.expandedTo( w->minimumSize() );

    if (align & HorAlign )
	s.setWidth( QWIDGETSIZE_MAX );
    if (align & VerAlign )
	s.setHeight( QWIDGETSIZE_MAX );
    return s;
}




/*!
  This function stores \a r, so it can be returned by geometry().
*/
void QSpacerItem::setGeometry( const QRect &r )
{
    rect = r;
}

/*!
  Sets the geometry of this item's widget to be contained within \a r,
  taking alignment and maximum size into account.
*/
void QWidgetItem::setGeometry( const QRect &r )
{
    QSize s = r.size().boundedTo( smartMaxSize( wid ) );
    int x = r.x();
    int y = r.y();
    if ( align & (HorAlign|VerAlign) ) {
	QSize pref = wid->sizeHint().expandedTo( wid->minimumSize() ); //###
	if ( align & HorAlign )
	    s.setWidth( QMIN( s.width(), pref.width() ) );
	if ( align & VerAlign ) {
	    if ( hasHeightForWidth() )
		s.setHeight( QMIN( s.height(), heightForWidth(s.width()) ) );
	    else
		s.setHeight( QMIN( s.height(), pref.height() ) );
	}
    }
    if ( align & Qt::AlignRight )
	x = x + ( r.width() - s.width() );
    else if ( !(align & Qt::AlignLeft) )
	x = x + ( r.width() - s.width() ) / 2;

    if ( align & Qt::AlignBottom )
	y = y + ( r.height() - s.height() );
    else if ( !(align & Qt::AlignTop) )
	y = y + ( r.height() - s.height() ) / 2;

    if ( !wid->isHidden() && !wid->isTopLevel() )
	wid->setGeometry( x, y, s.width(), s.height() );
}



/*!
  \reimp
*/

QRect QSpacerItem::geometry() const
{
    return rect;
}


/*!
  \reimp
*/

QRect QWidgetItem::geometry() const
{
    return wid->geometry();
}


/*!
  \reimp
*/

QRect QLayout::geometry() const
{
    return rect;
}



/*!
  \reimp
*/

bool QWidgetItem::hasHeightForWidth() const
{
    if ( isEmpty() )
	return FALSE;
    if ( wid->layout() )
	return wid->layout()->hasHeightForWidth();
    return wid->sizePolicy().hasHeightForWidth();
}

/*!
  \reimp
*/

int QWidgetItem::heightForWidth( int w ) const
{
    if ( isEmpty() )
	return 0;
    if ( wid->layout() )
	return wid->layout()->totalHeightForWidth( w );
    return wid->heightForWidth( w );
}

/*!
  Returns whether this space item is expanding.
*/
QSizePolicy::ExpandData QSpacerItem::expanding() const
{
    return sizeP.expanding();
}

/*!
  Returns whether this item's widget is expanding.
*/

QSizePolicy::ExpandData QWidgetItem::expanding() const
{
    if ( isEmpty() || align&HorAlign && align&VerAlign )
	return QSizePolicy::NoDirection;
    int e =  wid->layout() ? wid->layout()->expanding()
	     : wid->sizePolicy().expanding();
    if ( align&HorAlign )
	e = e & ~QSizePolicy::Horizontal;
    else if  ( align&VerAlign )
	e = e & ~QSizePolicy::Vertical;

    return (QSizePolicy::ExpandData)e;
}

/*!
  Returns the minimum size of this space item.
*/
QSize QSpacerItem::minimumSize() const
{
    return QSize( sizeP.mayShrinkHorizontally() ? 0 : width,
		  sizeP.mayShrinkVertically() ? 0 : height );
}

/*!
  Returns the minimum size of this item.
*/

QSize QWidgetItem::minimumSize() const
{
    if ( isEmpty() )
	return QSize(0,0);
    return smartMinSize( this );
}


/*!
  Returns the maximum size of this space item.
*/
QSize QSpacerItem::maximumSize() const
{
    return QSize( sizeP.mayGrowHorizontally() ? QWIDGETSIZE_MAX : width,
		  sizeP.mayGrowVertically() ? QWIDGETSIZE_MAX : height );
}

/*!
  Returns the maximum size of this item.
*/
QSize QWidgetItem::maximumSize() const
{
    if ( isEmpty() )
	return QSize(0,0);
    return smartMaxSize( wid, align );
}

/*!
  Returns the preferred size of this space item.
*/
QSize QSpacerItem::sizeHint() const
{
    return QSize( width, height );
}

///*!
//  Invalidates any cached information.
// */
//void QWidgetItem::invalidate()
//{
//    cachedSizeHint = QSize();
//}

/*!
  Returns the preferred size of this item.
*/
QSize QWidgetItem::sizeHint() const
{
    //if ( cachedSizeHint.isValid() )
    //	return cachedSizeHint;
    QSize s;
    if ( isEmpty() )
	s =  QSize(0,0);
    else
	s = wid->sizeHint().boundedTo( wid->maximumSize() )
	    .expandedTo( wid->minimumSize() ).expandedTo( QSize(1,1) );

    //((QWidgetItem*)this)->cachedSizeHint = s; //mutable hack
    return s;
}

/*!
  Returns TRUE, since a space item never contains widgets.
*/
bool QSpacerItem::isEmpty() const
{
    return TRUE;
}

/*!
  Returns TRUE, if the widget has been hidden, FALSE otherwise.
*/
bool QWidgetItem::isEmpty() const
{
    return wid->isHidden() || wid->isTopLevel();
}




/*!
  \class QLayout qabstractlayout.h
  \brief The QLayout class is the base class of geometry specifiers.

  \ingroup geomanagement

  This is an abstract base class. The concrete layout managers
  QBoxLayout and QGridLayout inherit from this one.

  For users of Q*Layout or QMainWindow, there is seldom need to use
  any of the basic functions provided by QLayout, such as setResizeMode()
  or setMenuBar(). See the \link layout.html layout overview page \endlink
  for more information.

  To make your own layout manager, make a subclass of QGLayoutIterator
  and implement the functions addItem(), sizeHint(), setGeometry() and
  iterator(). You should also implement minimumSize(), otherwise your
  layout will be resized to zero size if there is little space. To
  support children whose height depend on their widths, implement
  hasHeightForWidth() and heightForWidth().
  See the \link customlayout.html custom layout page \endlink for an
  in-depth description.

  Geometry management stops when the layout manager is deleted.
*/


/*!
  \fn QLayoutIterator QLayout::iterator()

  Returns an iterator over the children of this layout.

  This function must be implemented by subclasses.
*/

/*!
  Constructs a new top-level QLayout with main widget \a
  parent.  \a parent may not be 0.

  \a border is the number of pixels between the edge of the widget and
  the managed children.  \a space sets the value of spacing(), which
  gives the spacing between widgets.  The default value for \a space
  is -1, which means that the value of \a border is used.

  \a name is the internal object name

  There can only be one top-level layout for a widget. It is returned
  by QWidget::layout()
*/

QLayout::QLayout( QWidget *parent, int border, int space, const char *name )
    : QObject( parent, name )
{
    init();
    if ( parent ) {
	if ( parent->layout() ) {
	    qWarning( "QLayout \"%s\" added to %s \"%s\","
		     " which already had a layout.", QObject::name(),
		     parent->className(), parent->name() );
	    parent->removeChild( this );
	} else {
	    topLevel = TRUE;
	    if ( parent->isTopLevel() )
		autoMinimum = TRUE;
	    parent->installEventFilter( this );
	    setWidgetLayout( parent, this );
	}
    }
    outsideBorder = border;
    if ( space < 0 )
	insideSpacing = border;
    else
	insideSpacing = space;
    installEventFilter( this );//###binary compatibility.
}

void QLayout::init()
{
    insideSpacing = 0;
    outsideBorder = 0;
    topLevel = FALSE;
    autoMinimum = FALSE;
    autoNewChild = FALSE;
    frozen = FALSE;
    activated = FALSE;
    marginImpl = FALSE;
    extraData = 0;
#ifndef QT_NO_MENUBAR
    menubar = 0;
#endif
    enabled = TRUE;
}

/*!
  Constructs a new child QLayout, and places it inside
  \a parentLayout, using the default placement defined by
  addItem().

  If \a space is -1, this QLayout inherits \a parentLayout's
  spacing(), otherwise \a space is used.

*/

QLayout::QLayout( QLayout *parentLayout, int space, const char *name )
    : QObject( parentLayout, name )

{
    init();
    insideSpacing = space < 0 ? parentLayout->insideSpacing : space;
    parentLayout->addItem( this );
    installEventFilter( this );//###binary compatibility.
}


/*!
  Constructs a new child QLayout,
  If \a space is -1, this QLayout inherits its parent's
  spacing(), otherwise \a space is used.

  This layout has to be inserted into another layout before geometry
  management will work.
*/

QLayout::QLayout( int space, const char *name )
    : QObject( 0, name )
{
    init();
    insideSpacing = space;
    installEventFilter( this );//###binary compatibility.
}



/*! \fn void QLayout::addItem (QLayoutItem *item )
    Implemented in subclasses to add \a item. How it is
    added is specific to each subclass.

    Note that the ownership of \a item is transferred to
    the layout, and it is the layout's responsibility to
    delete it.
*/

/*! \fn QLayoutIterator iterator();
  Implemented in subclasses to return an iterator that iterates over
  the children of this layout.

  A typical implementation will be:
  \code
  QLayoutIterator MyLayout::iterator()
  {
      QGLayoutIterator *i = new MyLayoutIterator( internal_data );
      return QLayoutIterator( i );
  }
  \endcode
  where MyLayoutIterator is a subclass of QGLayoutIterator.
*/



/*!
  \fn void QLayout::add (QWidget * w)

  Adds \a w to this layout in a manner specific to the layout. This
  function uses addItem.
*/

/*!
  \fn QMenuBar* QLayout::menuBar () const
  Returns the menu bar set for this layout, or a null pointer if no
  menu bar is set.
 */



/*!
  \fn bool QLayout::isTopLevel () const

  Returns TRUE if this layout is a top level layout, i.e. not a child
  of another layout.
 */



/*!
  \fn int QLayout::margin () const
  returns the width of the outside border of the layout.
  \sa spacing() setMargin()
 */




/*!
  \fn int QLayout::defaultBorder() const

  \obsolete
  Returns the internal spacing for the geometry manager. Replaced by
  spacing()
*/

/*!
  \fn int QLayout::spacing() const
  Returns the spacing between widgets inside the layout.
  \sa margin() setSpacing()
*/




/*!
  Sets the outside border of the layout to \a border.

  For some layout classes, this function only has an effect on
  top-level layouts; QBoxLayout and QGridLayout support margins for
  child layouts.

  For some layout classes, this function only has an effect on
  top-level layouts; QBoxLayout and QGridLayout support margins for
  child layouts.

  \sa margin() setSpacing() supportsMargin()
 */

void QLayout::setMargin( int border )
{
    outsideBorder = border;
    invalidate();
    if ( mainWidget() ) {
	QEvent *lh = new QEvent( QEvent::LayoutHint );
	QApplication::postEvent( mainWidget(), lh );
    }
}


/*!
  Sets the internal spacing of the layout to \a space.

  \sa spacing() setMargin()
 */
//##### bool recursive = FALSE ????
void QLayout::setSpacing( int space )
{
    insideSpacing = space;
    invalidate();
    if ( mainWidget() ) {
	QEvent *lh = new QEvent( QEvent::LayoutHint );
	QApplication::postEvent( mainWidget(), lh );
    }
}




/*!
  Returns the main widget (parent widget) of this layout, or 0 if this
  layout is a sub-layout which is not yet inserted.
*/

QWidget * QLayout::mainWidget()
{
    if ( !topLevel ) {
	if ( parent() ) {
	    ASSERT( parent()->inherits( "QLayout" ) );
	    return ((QLayout*)parent())->mainWidget();
	} else {
	    return 0;
	}
    } else {
	ASSERT( parent() && parent()->isWidgetType() );
	return	(QWidget*)parent();
    }
}


/*!
  Returns TRUE if this layout is empty.
  The default implementation returns FALSE.
*/
bool QLayout::isEmpty() const
{
    return FALSE; //### should check
}

/*!
  Sets \a w's layout to \a l.
*/

void QLayout::setWidgetLayout( QWidget *w, QLayout *l )
{
    w->setLayout( l );
}



/*!
  This function is reimplemented in subclasses to
  perform layout.

  The default implementation maintains the geometry() information.
  Reimplementors must call this function.
 */
void QLayout::setGeometry( const QRect &r )
{
    rect = r;
}




/*!
  Invalidates cached information. Reimplementations must call this.
*/

void QLayout::invalidate()
{
    rect = QRect();
}


// ###

// this is an ugly workaround.  for some reason, the layout system
// would add TWO QWidgetItems for some QWidgets, and can then later
// delete one and derefence the other.  the scope of this bug is
// unknown.  I submit this hack so that we can work while I look for
// the 'double-add' bug.

// ###

static bool removeWidget( QLayoutItem *lay, QWidget *w )
{
    bool foo = FALSE;
    QLayoutIterator it = lay->iterator();
    QLayoutItem *child;
    while ( (child = it.current() ) ) {
	if ( child->widget() == w ) {
	    it.deleteCurrent();
	    lay->invalidate();
	    foo = TRUE;
	} else if ( removeWidget( child, w ) ) {
	    lay->invalidate();
	    foo = TRUE;
	} else {
	    ++it;
	}
    }
    return foo;
}

/*!
  Performs child widget layout when the parent widget is resized.
  Also handles removal of widgets and child layouts.
*/

bool QLayout::eventFilter( QObject *o, QEvent *e )
{
    if ( !enabled )
	return FALSE; // nothing we can do, should do or want to do

    if ( o == this && e->type() == QEvent::ChildRemoved ) {
	//we cannot implement childEvent() or event() because of
	//###binary compatibility.
	QChildEvent *c = (QChildEvent*)e;
	QLayoutIterator it = iterator();
	QLayoutItem *item;
	while ( (item = it.current() ) ) {
	    if ( item == (QLayout*)c->child() ) {
		it.takeCurrent();
		invalidate();
		return FALSE;
	    }
	    ++it;
	}
	return FALSE;
    }

    if ( !o->isWidgetType() )
	return FALSE;

    switch ( e->type() ) {
    case QEvent::Resize:
	if ( activated ) {
	    QResizeEvent *r = (QResizeEvent*)e;
	    int mbh = 0;
#ifndef QT_NO_MENUBAR
	    if ( menubar && !menubar->isHidden() && !menubar->isTopLevel() )
		mbh = menubar->heightForWidth( r->size().width() );
#endif
	    int b = marginImpl ? 0 : outsideBorder;
	    setGeometry( QRect( b, mbh + b, r->size().width() - 2*b,
				r->size().height() - mbh - 2*b ) );
	} else {
	    activate();
	}
	break;
    case QEvent::ChildRemoved: {
	QChildEvent *c = (QChildEvent*)e;
	if ( c->child()->isWidgetType() ) {
	    QWidget *w = (QWidget*)c->child();
#ifndef QT_NO_MENUBAR
	    if ( w == menubar )
		menubar = 0;
#endif
	    if ( removeWidget( this, w ) ) {
		QEvent *lh = new QEvent( QEvent::LayoutHint );
		QApplication::postEvent( o, lh );
	    }
	}
	break;
    }
    case QEvent::ChildInserted:
	if ( topLevel && autoNewChild ) {
	    QChildEvent *c = (QChildEvent*)e;
	    if ( c->child()->isWidgetType() ) {
		QWidget *w = (QWidget*)c->child();
		if ( !w->isTopLevel() ) {
#ifndef QT_NO_MENUBAR
		    if ( w->inherits( "QMenuBar" ) && ( !w->parent() || !w->parent()->inherits( "QToolBar" ) ) )
			menubar = (QMenuBar*)w;
		    else
#endif
			addItem( new QWidgetItem( w ) );
		    QEvent *lh = new QEvent( QEvent::LayoutHint );
		    QApplication::postEvent( o, lh );
		}
	    }
	}
	break;
    case QEvent::LayoutHint:
	activate();
	break;
    default:
	break;
    }
    return QObject::eventFilter( o, e );

}


/*!
  \internal
  Also takes margin() and menu bar into account.
*/

int QLayout::totalHeightForWidth( int w ) const
{
    if ( topLevel ) {
	QWidget *mw = (QWidget*)parent();
	if ( mw && !mw->testWState(WState_Polished) ) {
	    mw->polish();
	}
    }
    int b = (topLevel && !marginImpl) ? 2*outsideBorder : 0;
    int h = heightForWidth( w - b ) + b;
#ifndef QT_NO_MENUBAR
    if ( menubar && !menubar->isTopLevel() )
	h += menubar->heightForWidth( w );
#endif
    return h;
}

/*!
  \internal
  Also takes margin() and menu bar into account.
*/

QSize QLayout::totalMinimumSize() const
{
    if ( topLevel ) {
	QWidget *mw = (QWidget*)parent();
	if ( mw && !mw->testWState(WState_Polished) ) {
	    mw->polish();
	}
    }
    int b = (topLevel && !marginImpl) ? 2*outsideBorder : 0;

    QSize s = minimumSize();
    int h = b;
#ifndef QT_NO_MENUBAR
    if ( menubar && !menubar->isTopLevel() )
	h += menubar->heightForWidth( s.width() );
#endif
    return s + QSize(b,h);
}



/*!
  \internal
  Also takes margin() and menu bar into account.
*/

QSize QLayout::totalSizeHint() const
{
    if ( topLevel ) {
	QWidget *mw = (QWidget*)parent();
	if ( mw && !mw->testWState(WState_Polished) ) {
	    mw->polish();
	}
    }
    int b = (topLevel && !marginImpl) ? 2*outsideBorder : 0;

    QSize s = sizeHint();
    int h = b;
#ifndef QT_NO_MENUBAR
    if ( menubar && !menubar->isTopLevel() )
	h += menubar->heightForWidth( s.width() );
#endif
    return s + QSize(b,h);
}


/*!
  \internal
  Also takes margin() and menu bar into account.
*/

QSize QLayout::totalMaximumSize() const
{
    if ( topLevel ) {
	QWidget *mw = (QWidget*)parent();
	if ( mw && !mw->testWState(WState_Polished) ) {
	    mw->polish();
	}
    }
    int b = (topLevel && !marginImpl) ? 2*outsideBorder : 0;

    QSize s = maximumSize();
    int h = b;
#ifndef QT_NO_MENUBAR
    if ( menubar && !menubar->isTopLevel() )
	h += menubar->heightForWidth( s.width() );
#endif

    if ( isTopLevel() )
	s = QSize( QMIN( s.width() + b, QWIDGETSIZE_MAX ),
		   QMIN( s.height() + h, QWIDGETSIZE_MAX ) );
    return s;
}




/*!
  Destructs the layout, deleting all child layouts.
  Geometry management stops when a toplevel layout is deleted.
  \internal
  The layout classes will probably be fatally confused if you delete
  a sublayout.
*/

QLayout::~QLayout()
{
    //note that this function may be called during the QObject destructor,
    //when the parent no longer is a QWidget.
    if ( isTopLevel() && parent() && parent()->isWidgetType() &&
	 ((QWidget*)parent())->layout() == this )
	setWidgetLayout( (QWidget*)parent(), 0 );
}

/*!
  Removes and deletes all items in this layout.
 */
void QLayout::deleteAllItems()
{
    QLayoutIterator it = iterator();
    QLayoutItem *l;
    while ( (l=it.takeCurrent()) )
	delete l;
}

/*!
  This function is called from addLayout functions in subclasses,
  to add \a l layout as a sublayout.
*/
void QLayout::addChildLayout( QLayout *l )
{
    if ( l->parent() ) {
#if defined(CHECK_NULL)
	qWarning( "QLayout::addChildLayout(), layout already has a parent." );
#endif
	return;
    }
    insertChild( l );
    if ( l->insideSpacing < 0 )
	l->insideSpacing = insideSpacing;
}



/*!
  \overload void QLayout::freeze()


  Fixes the main widget at its minimum size.
  The recommended way is to call setResizeMode( \c Fixed )
*/


/*!
  \obsolete
  Fixes the size of the main widget and distributes the available
  space to the child widgets. For widgets which should not be
  resizable, but where a QLayout subclass is used to set up the initial
  geometry.

  As a special case, freeze(0,0) is equivalent to setResizeMode( \c Fixed )
*/

void QLayout::freeze( int w, int h )
{
    if ( w <= 0 || h <= 0 ) {
	setResizeMode( Fixed );
    } else {
	setResizeMode( FreeResize ); // layout will not change min/max size
	mainWidget()->setFixedSize( w, h );
    }
}

#ifndef QT_NO_MENUBAR

/*!
  Makes the geometry manager take account of the menu bar \a w. All
  child widgets are placed below the bottom edge of the menu bar.

  A menu bar does its own geometry managing, never do addWidget()
  on a QMenuBar.
*/

void QLayout::setMenuBar( QMenuBar *w )
{
    menubar = w;
}

#endif

/*!
  Returns the minimum size of this layout. This is the smallest size
  that the layout can have, while still respecting the specifications.
  Does not include what's needed by margin() or menuBar().

  The default implementation allows unlimited resizing.
*/

QSize QLayout::minimumSize() const
{
    return QSize( 0, 0 );
}


/*!
  Returns the maximum size of this layout. This is the largest size
  that the layout can have, while still respecting the specifications.
  Does not include what's  needed by margin() or menuBar().

  The default implementation allows unlimited resizing.
*/

QSize QLayout::maximumSize() const
{
    return QSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
}


/*!
  Returns whether this layout can make use of more space than
  sizeHint().  A value of Vertical or Horizontal means that it wants
  to grow in only one dimension, while BothDirections means that it wants to
  grow in both dimensions.

  The default implementation returns BothDirections.
*/

QSizePolicy::ExpandData QLayout::expanding() const
{
    return QSizePolicy::BothDirections;
}




static void  invalidateRecursive( QLayoutItem *lay )
{
    lay->invalidate();
    QLayoutIterator it = lay->iterator();
    QLayoutItem *child;
    while ( (child = it.current() ) ) {
	invalidateRecursive( child );
	++it;
    }
}




/*!  Redoes the layout for mainWidget().  You should generally not
  need to call this, as it is automatically called at most appropriate
  times.

  However, if you set up a QLayout for a visible widget without
  resizing that widget, you need to call this function in order to lay
  it out.

  \sa QWidget::updateGeometry()
*/

bool QLayout::activate()
{
    // Paul: If adding stuff to a QLayout for a widget causes
    // postEvent(thatWidget, QEvent::LayoutHint), activate() becomes
    // unnecessary in that case too.
    invalidateRecursive( this );
    if ( !topLevel )
	return FALSE;

    QWidget *mainW = mainWidget();
    if ( !mainW ) {
#if defined( CHECK_NULL )
	    qWarning( "QLayout::activate(): %s \"%s\" does not have a "
		      "main widget.",
		     QObject::className(), QObject::name() );

#endif
	return FALSE;
    }
    activated = TRUE;
    QSize s = mainWidget()->size();
    int mbh = 0;
#ifndef QT_NO_MENUBAR
    mbh = menubar && !menubar->isTopLevel() ? menubar->heightForWidth( s.width() ) : 0;
#endif
    int b = marginImpl ? 0 : outsideBorder;
    setGeometry( QRect( b, mbh + b,
			s.width() - 2*b,
			s.height() - mbh - 2*b ) );
    if ( frozen )
	mainWidget()->setFixedSize( totalSizeHint() ); //### will trigger resize
    else if ( autoMinimum )
	mainWidget()->setMinimumSize( totalMinimumSize() );

    //###if ( sizeHint or sizePolicy has changed )
    mainWidget()->updateGeometry();
    return TRUE;
}





/*!
  \class QSizePolicy qsizepolicy.h
  \brief A layout attribute describing horizontal and vertical resizing.

  The size policy of a widget is an expression of its willingness to
  be resized in various ways.

  Widgets which reimplement QWidget::sizePolicy() return a QSizePolicy
  describing the horizontal and vertical resizing policy best used
  when laying out the widget.  Only <a href="#interesting">one of the
  constructors</a> is of interest in most applications.

  QSizePolicy contains two independent SizeType objects, one which describes
  the widgets's horizontal size policy and one which describes its
  vertical size policy, and it contains a flag to indicate whether the
  height and width of its preferred size are related.

  The per-dimension SizeType objects are set in the usual constructor,
  and can be queried using a variety of functions, none of which are
  really interesting to application programmers.

  The hasHeightForWidth() flag indicates whether the widget's sizeHint()
  is width-dependent (like a word-wrapping label).

  \sa QSizePolicy::SizeType
*/

/*! \enum QSizePolicy::SizeType

The per-dimension sizing types used when constructing a QSizePolicy
are: <ul>

<li> \c Fixed - the sizeHint() is the only acceptable alternative, so
widget can never grow or shrink (eg. the vertical direction of a
pushbutton).

<li> \c Minimum - the sizeHint() is minimal, and sufficient. The
widget can be expanded, but there is no advantage to it being
larger (eg. the horizontal direction of a pushbutton).

<li> \c Maximum - the sizeHint() is a maximum.  The widget can be
shrunk any amount without detriment if other widgets need the space
(eg. a separator line).

<li> \c Preferred - the sizeHint() is best, but the widget can be
shrunk below that and still be useful. The widget can be expanded, but
there is no advantage to it being larger than sizeHint() (the default
QWidget policy).

<li> \c MinimumExpanding - the sizeHint() is a minimum.  The widget
can make use of extra space, so it should get as much space as
possible (not currently used by any standard Qt widgets).

<li> \c Expanding - the sizeHint() is a sensible size, but the
widget can be shrunk below that and still be useful. The widget can
make use of extra space, so it should get as much space as
possible (eg. the horizontal direction of a slider).

</ul>
*/

/*! \enum QSizePolicy::ExpandData

This enum type describes in which directions a widget can make use of
extra space.  There are four possible values: <ul>

<li> \c NoDirection - the widget cannot make us of extra space in any
direction.

<li> \c Horizontal - the widget can usefully be wider than sizeHint()
says.

<li> \c Vertical - the widget can usefully be taller than sizeHint()
says.

<li> \c BothDirections - the widget can usefully be both wider and
taller than sizeHint() says.

</ul>
*/

/*!
  \fn QSizePolicy::QSizePolicy ()

  Default constructor, produces a minimally initialized QSizePolicy.
*/

/*!
  \fn QSizePolicy::QSizePolicy( SizeType hor, SizeType ver, bool hfw )

  <a name="interesting"></a>
  This is the constructor normally used to return a value in the
  overridden \l QWidget::sizePolicy() function of a QWidget subclass.

  It constructs a QSizePolicy with independent horizontal and vertical
  sizing types, \a hor and \a ver respectively.  These \link
  QSizePolicy::SizeType sizing types\endlink affect how the widget is
  treated by the \link QLayout layout engine\endlink.

  If \a hfw is TRUE, the preferred height of the widget is dependent on the
  width of the widget (for example, a QLabel with automatic word-breaking).
*/

#if defined(__GNUC__) && __GNUC__ == 2 && __GNUC_MINOR__ == 96

QSizePolicy::QSizePolicy( SizeType hor, SizeType ver, bool hfw )
    : data( hor | (ver<<HSize) | (hfw ? (1<<2*HSize) : 0) ) {}

#endif

/*! \fn QSizePolicy::SizeType QSizePolicy::horData() const
Returns the horizontal component of the size policy.
*/


/*! \fn QSizePolicy::SizeType QSizePolicy::verData() const
Returns the vertical component of the size policy.
*/

/*! \fn bool QSizePolicy::mayShrinkHorizontally() const
Returns TRUE if the widget can sensibly be narrower than its sizeHint().
*/


/*! \fn bool QSizePolicy::mayShrinkVertically() const
Returns TRUE if the widget can sensibly be lower than its sizeHint().
*/


/*! \fn bool QSizePolicy::mayGrowHorizontally() const
Returns TRUE if the widget can sensibly be wider than its sizeHint().
*/


/*! \fn bool QSizePolicy::mayGrowVertically() const
Returns TRUE if the widget can sensibly be taller than its sizeHint().
*/

/*! \fn QSizePolicy::ExpandData QSizePolicy::expanding() const
Returns a value indicating if the widget can make use of extra space
(ie. if it "wants" to grow) horizontally and/or vertically.
*/

/*! \fn void QSizePolicy::setHorData( SizeType d )
Sets the horizontal component of the size policy to \a d.
*/


/*! \fn void QSizePolicy::setVerData( SizeType d )
Sets the vertical component of the size policy to \a d.
*/

/*! \fn bool QSizePolicy::hasHeightForWidth() const
Returns TRUE if the widget's preferred height depends on its width.
*/


/*! \fn void QSizePolicy::setHeightForWidth( bool b )
Sets the hasHeightForWidth() flag to \a b.
*/


/*! \fn void QSizePolicy::operator==( const QSizePolicy &s )
  Returns TRUE if this policy is equal to \a s, or FALSE if
  they are different.
*/

/*! \fn void QSizePolicy::operator!=( const QSizePolicy &s )
  Returns TRUE if this policy is different from \a s, or FALSE if
  they are equal.
*/


/*!
  \class QGLayoutIterator qabstractlayout.h
  \brief The abstract base class of internal layout iterators.

  To be subclassed by custom layout implementors. The functions that
  need to be implemented are next(), current() and takeCurrent().

  The QGLayoutIterator implements the functionality of
  QLayoutIterator. Each subclass of QLayout needs a
  QGLayoutIterator subclass.
*/


/*! \fn QLayoutItem *QGLayoutIterator::next()
  Implemented in subclasses to move the iterator to the next item and
  return that item, or 0 if there is no next item.
 */

/*! \fn QLayoutItem *QGLayoutIterator::current()
  Implemented in subclasses to return the current item, or 0 if there
  is no current item.
 */

/*! \fn QLayoutItem *QGLayoutIterator::takeCurrent()
  Implemented in subclasses to remove the current item from the layout
  without deleting it, move the iterator to the next item and return
  the removed item, or 0 if no item was removed.
 */


/*!
  Destroys the iterator
*/

QGLayoutIterator::~QGLayoutIterator()
{
}


/*!
  \class QLayoutIterator qabstractlayout.h
  \brief The QLayoutIterator class provides iterators over QLayoutItem

  Use QLayoutItem::iterator() to create an iterator over a layout.

  QLayoutIterator uses explicit sharing with a reference count. If
  an iterator is copied, and one of the copies is modified,
  both iterators will be modified.

  A QLayoutIterator is not protected against changes in its layout. If
  the layout is modified or deleted, the iterator will become invalid.
  It is not possible to test for validity. It is safe to delete an
  invalid layout. Any other access may lead to an illegal memory
  reference, and the abnormal termination of the program.

  Calling takeCurrent() or deleteCurrent() leaves the iterator in a
  valid state, but may invalidate any other iterators that access the
  same layout.

  The following code will draw a rectangle for each layout item
  in the layout structure of the widget.
  \code
  static void paintLayout( QPainter *p, QLayoutItem *lay )
  {
      QLayoutIterator it = lay->iterator();
      QLayoutItem *child;
      while ( (child = it.current() ) ) {
	  paintLayout( p, child );
	  it.next();
      }
      p->drawRect( lay->geometry() );
  }
  void ExampleWidget::paintEvent( QPaintEvent * )
  {
      QPainter p( this );
      if ( layout() )
	  paintLayout( &p, layout() );
  }
  \endcode

  All the functionality of QLayoutIterator is implemented by
  subclasses of QGLayoutIterator. Note that there is not much
  point in subclassing QLayoutIterator, since none of the functions
  are virtual.
*/




/*! \fn QLayoutIterator::QLayoutIterator( QGLayoutIterator *gi )
  Constructs an iterator based on \a gi. The constructed iterator takes
  ownership of \a gi, and will delete it.

  This constructor is provided for layout implementors. Application
  programmers should use QLayoutItem::iterator() to create an iterator
  over a layout.
*/

/*! \fn QLayoutIterator::QLayoutIterator( const QLayoutIterator &i )
  Creates a shallow copy of \a i; if the copy is modified, then the
  original will also be modified.
*/

/*! \fn QLayoutIterator::~QLayoutIterator()
  Destroys the iterator.
*/

/*! \fn QLayoutIterator &QLayoutIterator::operator=( const QLayoutIterator &i )
  Assigns \a i to this iterator and returns a reference to this iterator.
*/

/*! \fn QLayoutItem *QLayoutIterator::operator++()
  Moves the iterator to the next child item, and returns that item, or 0
  if there is no such item.
*/

/*! \fn QLayoutItem *QLayoutIterator::current()
  Returns the current item, or 0 if there is no current item.
*/

/*! \fn QLayoutItem *QLayoutIterator::takeCurrent()
  Removes the current child item from the layout without deleting it
  and moves the iterator to the next item. Returns the removed item, or
  0 if there was no item to be removed. This iterator will still be
  valid, but any other iterator over the same layout may become
  invalid.
*/


/*! \fn void QLayoutIterator::deleteCurrent()
  Removes and deletes the current child item from the layout and moves the
  iterator to the next item. This iterator will still be valid, but any
  other iterator over the same layout may become invalid.
*/


/*!
  \enum QLayout::ResizeMode

    The possible values are are:
<ul>
    <li> \c Fixed - the main widget's size is set to sizeHint(), it
    cannot be resized at all.
    <li> \c Minimum - The main widget's minimum size is set to
    minimumSize(), it cannot be smaller.
    <li> \c FreeResize - the widget is not constrained.
</ul>

*/

/*!
  Sets the resize mode to \a mode.

  The default mode is \c Minimum for top level widgets, and \c FreeResize
  for all others.

  \sa QLayout::ResizeMode
*/

void QLayout::setResizeMode( ResizeMode mode )
{
    if ( mode == resizeMode() )
	return;
    switch (mode) {
    case Fixed:
	frozen = TRUE;
	break;
    case FreeResize:
	frozen = FALSE;
	autoMinimum = FALSE;
	break;
    case Minimum:
	frozen = FALSE;
	autoMinimum = TRUE;
	break;
    }
    activate();
}


/*!
  Returns the resize mode.
*/

QLayout::ResizeMode QLayout::resizeMode() const
{
    return frozen ? Fixed : (autoMinimum ? Minimum : FreeResize );
}


/*! \fn bool QLayout::autoAdd() const
  Returns TRUE if this layout automatically grabs all new
  mainWidget()'s new children and adds them as defined by
  addItem(). This only has effect for top-level layouts, ie. layouts
  that are direct children of their mainWidget().

  autoAdd() is disabled by default.

  \sa setAutoAdd()
*/

/*!
  Sets autoAdd() if \a b is TRUE.

  \sa autoAdd()
*/

void QLayout::setAutoAdd( bool b )
{
    autoNewChild = b;
}


/*!
  \fn  bool QLayout::supportsMargin() const

  Returns TRUE if this layout supports setMargin() on non-toplevel
  layouts.

  \sa setMargin()
*/


/*!
  Sets the value returned by supportsMargin(). If \a b is TRUE,
  margin() handling is implemented by the subclass. If \a b is
  FALSE (the default) QLayout will add margin() around top-level
  layouts.

  If \a b is TRUE, margin handling needs to be implemented in
  setGeometry(), maximumSize(), minimumSize(), sizeHint() and
  heightForWidth().

  \sa supportsMargin()
*/

void QLayout::setSupportsMargin( bool b )
{
    marginImpl = b;
}


/*!
  Returns the rectangle that should be covered when the geometry of
  this layout is set to \a r, if this layout supports setAlignment().

  The result is calculated from sizeHint() and expanding(). It is
  never larger than \a r.
 */

QRect QLayout::alignmentRect( const QRect &r ) const
{
    QSize s = sizeHint();
    int a = alignment();
    if ( expanding() & QSizePolicy::Horizontal || !(a & HorAlign ) ) {
	s.setWidth( r.width() );
    }
    if ( expanding() & QSizePolicy::Vertical || !(a & VerAlign )) {
	s.setHeight( r.height() );
    } else if ( hasHeightForWidth() ) {
	s.setHeight( QMIN( s.height(), heightForWidth(s.width()) ) );
    }

    int x = r.x();
    int y = r.y();

    if ( a & Qt::AlignRight )
	x = x + ( r.width() - s.width() );
    else if ( !(a & Qt::AlignLeft) )
	x = x + ( r.width() - s.width() ) / 2;

    if ( a & Qt::AlignBottom )
	y = y + ( r.height() - s.height() );
    else if ( !(a & Qt::AlignTop) )
	y = y + ( r.height() - s.height() ) / 2;

    return QRect( x, y, s.width(), s.height() );

}



/*!
  Enables this layout if \a enable is TRUE, otherwise disables it.

  An enabled layout adjusts dynamically to changes, a disabled layout
  acts as if it was not existing.

  By default, all layouts are enabled.

  \sa isEnabled()
 */
void QLayout::setEnabled( bool enable)
{
    enabled = enable;
}


/*!
  Returns whether or not this layout is enabled.

  \sa setEnabled()
 */
bool QLayout::isEnabled() const
{
    return enabled;
}

#endif //QT_NO_LAYOUT
