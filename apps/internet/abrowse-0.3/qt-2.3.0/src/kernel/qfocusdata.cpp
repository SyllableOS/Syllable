/****************************************************************************
** $Id$
**
** Implementation of QFocusData class
**
** Created : 980622
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

#include "qfocusdata.h"

// NOT REVISED
/*!
  \class QFocusData qfocusdata.h
  \brief Maintains the list of widgets which can take focus.

  This read-only list always contains at least one widget (the
  top-level widget, actually).  It provides a simple cursor, which can
  be reset to the current focus widget using home(), or moved to its
  neighboring widgets using next() and prev(), and a count() of
  widgets in the list.

  Note that some widgets in the list may not accept focus.  Widgets
  are added to the list as necessary, but not removed from it.  This
  lets widgets change focus policy dynamically without disrupting the
  focus chain the user sees: When a widget disables and re-enables tab
  focus, its position in the focus chain does not change.

  When reimplementing QWidget::focusNextPrevChild() to provide special
  focus flow, you will usually call QWidget::focusData() to retrieve
  the focus data stored at the top-level widget - the focus data for
  that hierarchy of widgets.

  The cursor may change at any time; this class is not thread-safe.

  \sa QWidget::focusNextPrevChild() QWidget::setTabOrder()
  QWidget::setFocusPolicy()
*/

/*!
  \fn QWidget* QFocusData::focusWidget() const

  Returns the widgets in the hierarchy which currently has focus.
*/

/*!
  \fn int QFocusData::count() const

  Returns a count of the number of widgets in the hierarchy which accept focus.
*/

/*!
  Moves the cursor to the focusWidget() and returns that widget.
  You must call this before next() or prev() to iterate meaningfully.
*/
QWidget* QFocusData::home()
{
    focusWidgets.find(it.current());
    return focusWidgets.current();
}

/*!
  Moves the cursor to the right.  Note that the focus widgets
  are a \e loop of widgets.  If you keep calling next(), it will
  loop, without ever returning 0.
*/
QWidget* QFocusData::next()
{
    QWidget* r = focusWidgets.next();
    if ( !r )
	r = focusWidgets.first();
    return r;
}

/*!
  Moves the cursor to the left.  Note that the focus widgets
  are a \e loop of widgets.  If you keep calling prev(), it will
  loop, without ever returning 0.
*/
QWidget* QFocusData::prev()
{
    QWidget* r = focusWidgets.prev();
    if ( !r )
	r = focusWidgets.last();
    return r;
}
