/****************************************************************************
** $Id$
**
** Implementation of QSemiModal class
**
** Created : 970627
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

#include "qsemimodal.h"

#ifndef QT_NO_SEMIMODAL
#include "qapplication.h"

// NOT REVISED
/*!
  \class QSemiModal qsemimodal.h
  \brief The QSemiModal class is the base class of semi-modal dialog windows.

  \ingroup abstractwidgets
  \ingroup dialogs

  The semi-modal dialog window can disable events to other windows
  while it is open.  To enable this, the QSemiModal must be
  constructed with TRUE for the \e modal argument, which is FALSE by
  default, for consistency with QDialog.  Such a QSemiModal is modal
  like QDialog, but it does not have its own event loop. The flow of
  control is still within your code and it is up to you to update the
  semi-modal dialog (for example a progressbar) from time to time.

  Note that the parent widget has a different meaning for dialogs than
  for other types of widgets. A dialog is placed on top of the parent
  widget. The dialog is centered on the screen if the parent widget is
  zero.
*/


/*!
  Constructs a semi-modal dialog named \a name, which has a parent
  widget \a parent.  If \a modal is FALSE (the default), the only
  behavior different to a QWidget is automatic sizing and positioning.
*/

QSemiModal::QSemiModal( QWidget *parent, const char *name, bool modal, WFlags f )
    : QWidget( parent, name, (modal ? (f | WType_Modal) : f) | WType_TopLevel | WStyle_Dialog )
{
    did_move = did_resize = FALSE;
}

/*!
  Destructs the widget, deleting all its children.
*/

QSemiModal::~QSemiModal()
{
}


/*!
  Shows the widget.
  This implementation also does automatic resizing and automatic
  positioning. If you have not already resized or moved the dialog, it
  will find a size that fits the contents and a position near the middle
  of the screen (or centered relative to the parent widget if any).
*/

void QSemiModal::show()
{
    if ( !did_resize )
	adjustSize();
    if ( !did_move ) {
	QWidget *w = parentWidget();
	QPoint p( 0, 0 );
	if ( w )
	    p = w->mapToGlobal( p );
	else
	    w = QApplication::desktop();
	move( p.x() + w->width()/2  - width()/2,
	      p.y() + w->height()/2 - height()/2 );
    }
    QWidget::show();
}

/*****************************************************************************
  Geometry management.
 *****************************************************************************/



/*****************************************************************************
  Detects any widget geometry changes done by the user.
 *****************************************************************************/

/*!
  Reimplements QWidget::move() for internal purposes.
*/

void QSemiModal::move( int x, int y )
{
    did_move = TRUE;
    QWidget::move( x, y );
}

/*!
  Reimplements QWidget::move() for internal purposes.
*/

void QSemiModal::move( const QPoint &p )
{
    did_move = TRUE;
    QWidget::move( p );
}

/*!
  Reimplements QWidget::resize() for internal purposes.
*/

void QSemiModal::resize( int w, int h )
{
    did_resize = TRUE;
    QWidget::resize( w, h );
}

/*!
  Reimplements QWidget::resize() for internal purposes.
*/

void QSemiModal::resize( const QSize &s )
{
    did_resize = TRUE;
    QWidget::resize( s );
}

/*!
  Reimplements QWidget::setGeometry() for internal purposes.
*/

void QSemiModal::setGeometry( int x, int y, int w, int h )
{
    did_move   = TRUE;
    did_resize = TRUE;
    QWidget::setGeometry( x, y, w, h );
}

/*!
  Reimplements QWidget::setGeometry() for internal purposes.
*/

void QSemiModal::setGeometry( const QRect &r )
{
    did_move   = TRUE;
    did_resize = TRUE;
    QWidget::setGeometry( r );
}
#endif //QT_NO_SEMIMODAL
