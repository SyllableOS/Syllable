/****************************************************************************
** $Id$
**
** Implementation of QDialog class
**
** Created : 950502
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

#include "qdialog.h"

#ifndef QT_NO_DIALOG

#include "qpushbutton.h"
#include "qapplication.h"
#include "qobjectlist.h"
#include "qobjectdict.h"
#include "qwidgetlist.h"
#include "qlayout.h"
#include "qsizegrip.h"


// REVISED: arnt
/*!
  \class QDialog qdialog.h
  \brief The QDialog class is the base class of dialog windows.

  \ingroup dialogs
  \ingroup abstractwidgets

  A dialog window is a top-level window used for short-term tasks and
  brief communications with the user. QDialog offers mechanisms such
  as modality, default buttons, extensibility and a result value.

  Modality means that the dialog blocks input to other windows: The
  user \e has to finish interacting with the dialog and close it
  before resuming work with the other window(s).  The only way to set
  modality is by using the constructor, and the default is
  modelessness.  For modal dialog, it's generally better to call
  exec() than show; exec() returns when the dialog has been closed and
  has a useful return value (see below).

  The default button is the button that's pressed when the user
  presses Enter or Return, to accept the things done using this dialog
  and close it. QDialog uses QPushButton::autoDefault(),
  QPushButton::isDefault() and QPushButton::setDefault() to make Enter
  or Return map to the right button at any time.

  Extensibility is the ability to show more or less of the
  dialog. Typically, the dialog starts out small, has a "More" button,
  and when the user clicks "More", the dialog becomes bigger, and
  shows some less-used options.  QDialog supports this using
  setExtension(), setOrientation() and showExtension().

  Since dialogs typically tend to have result value (pressing
  Enter/Return maps to one value and pressing Escape to the other),
  QDialog supports that.  A dialog can finish by calling the slots
  accept() or reject(), and exec() returns that result.

  Note that QDialog uses the parent widget a bit differently from
  other classes in Qt.  A dialog is always a top-level widget, but if
  it has a parent, its default location is on top of the parent, it
  shares taskbar entry with its parent, and there are some minor
  details.

  QDialog also can provide a QSizeGrip in its lower-right corner. If
  you want that, call setSizeGripEnabled( TRUE ).

  \sa QTabDialog QWidget QSemiModal
  <a href="guibooks.html#fowler">GUI Design Handbook: Dialogs, Standard.</a>
*/



class QDialogPrivate : public Qt
{
public:

    QDialogPrivate()
	: mainDef(0), orientation(Horizontal),extension(0)
#ifndef QT_NO_SIZEGRIP
	,resizer(0)
#endif
	{
    }

    QPushButton* mainDef;
    Orientation orientation;
    QWidget* extension;
    QSize size, min, max;
#ifndef QT_NO_SIZEGRIP
    QSizeGrip* resizer;
#endif
};


/*!
  Constructs a dialog named \a name, which has a parent widget \a parent.

  The dialog is modal (blocks input to other windows) if \a modal is
  TRUE, and modeless if \a modal is FALSE (this is the default).

  The widget flags \a f are sent to the QWidget constructor, as usual.

  If you e.g. don't want a What's this button in the titlebar of the dialog,
  pass WStyle_Customize | WStyle_NormalBorder | WStyle_Title | WStyle_SysMenu
  here.

  We recommend always passing a parent.

  \sa QWidget::setWFlags Qt::WidgetFlags
*/

QDialog::QDialog( QWidget *parent, const char *name, bool modal, WFlags f )
    : QWidget( parent, name,
	       (modal ? (f|WType_Modal) : f) | WType_TopLevel | WStyle_Dialog )
{
    rescode = 0;
    did_move = FALSE;
    did_resize = FALSE;
    in_loop = FALSE;
    d = new QDialogPrivate;
}

/*!
  Destructs the QDialog, deleting all its children.
*/

QDialog::~QDialog()
{
    // Need to hide() here, as our (to-be) overridden hide()
    // will not be called in ~QWidget.
    hide();
    delete d;
}

/*!
  \internal
  This function is called by the push button \a pushButton when it
  becomes the default button. If \a pushButton is 0, the dialogs
  default default button becomes the default button. This is what a
  push button calls when it loses focus.
*/

void QDialog::setDefault( QPushButton *pushButton )
{
#ifndef QT_NO_DIALOG
    QObjectList *list = queryList( "QPushButton" );
    ASSERT(list);
    QObjectListIt it( *list );
    QPushButton *pb;
    bool hasMain = FALSE;
    while ( (pb = (QPushButton*)it.current()) ) {
	++it;
	if ( pb == d->mainDef )
	    hasMain = TRUE;
	if ( pb != pushButton )
	    pb->setDefault( FALSE );
    }
    if (!pushButton && hasMain)
	d->mainDef->setDefault( TRUE );
    if (!hasMain)
	d->mainDef = pushButton;
    delete list;
#endif
}

/*!
  \internal
  Hides the default button indicator. Called when non auto-default
  push button get focus.
 */
void QDialog::hideDefault()
{
#ifndef QT_NO_DIALOG
    QObjectList *list = queryList( "QPushButton" );
    QObjectListIt it( *list );
    QPushButton *pb;
    while ( (pb = (QPushButton*)it.current()) ) {
	++it;
	pb->setDefault( FALSE );
    }
#endif
}


/*!
  \fn int  QDialog::result() const

  Returns the result code of the dialog.
*/

/*!
  \fn void  QDialog::setResult( int )

  Sets the result code of the dialog.
*/


/*! Starts the (modal) dialog, waits, and returns the result code when
  it is done.

  If the dialog is modeless, the behaviour of this function is
  undefined.

  \sa show(), result()
*/

int QDialog::exec()
{
#if defined(CHECK_STATE)
    if ( !testWFlags(WType_Modal) )
	qWarning( "QDialog::exec: Calling this function for a modeless dialog "
		 "makes no sense" );
#endif
    setResult( 0 );
    show();

#ifdef _WS_QWS_ // QDialog::show is changed to 3.0 semantics for Qt/Embedded
    if ( testWFlags(WType_Modal) && !in_loop ) {
	in_loop = TRUE;
	qApp->enter_loop();
    }
#endif

    return result();
}


/*! Hides the (modal) dialog and sets its result code to \a r. This
  uses the local event loop to finish and exec() to return \a r.

  If the dialog has the \c WDestructiveClose flag set, done() also
  deletes the dialog. If the dialog is the applications's main widget,
  the application quits.

  \sa accept(), reject(), QApplication::mainWidget(), QApplication::quit()
*/

void QDialog::done( int r )
{
    hide();
    setResult( r );

    // We cannot use close() here, as close() calls closeEvent() calls
    // reject() calls close().  But we can at least keep the
    // mainWidget() and WDestructiveClose semantics. There should not
    // be much of a difference whether the users types Alt-F4 or
    // Escape. Without that, destructive-close dialogs were more or
    // less useless without subclassing.
    if ( qApp->mainWidget() == this )
	qApp->quit();
    
    if ( testWFlags(WDestructiveClose) )
	delete this;
}

/*!
  Hides the dialog and sets the result code to \c Accepted.
*/

void QDialog::accept()
{
    done( Accepted );
}

/*!
  Hides the dialog and sets the result code to \c Rejected.
*/

void QDialog::reject()
{
    done( Rejected );
}


/*****************************************************************************
  Event handlers
 *****************************************************************************/

/*! \reimp */
void QDialog::keyPressEvent( QKeyEvent *e )
{
    //   Calls reject() if Escape is pressed.  Simulates a button
    //   click for the default button if Enter is pressed.  Move focus
    //   for the arrow keys.  Ignore the rest.
    if ( e->state() == 0 || ( e->state() & Keypad && e->key() == Key_Enter ) ) {
	switch ( e->key() ) {
	case Key_Enter:
	case Key_Return: {
#ifndef QT_NO_DIALOG
	    QObjectList *list = queryList( "QPushButton" );
	    QObjectListIt it( *list );
	    QPushButton *pb;
	    while ( (pb = (QPushButton*)it.current()) ) {
		if ( pb->isDefault() && pb->isVisible() ) {
		    delete list;
		    if ( pb->isEnabled() ) {
			emit pb->clicked();
		    }
		    return;
		}
		++it;
	    }
	    delete list;
#endif
	}
	break;
	case Key_Escape:
	    reject();
	    break;
	case Key_Up:
	case Key_Left:
	    if ( focusWidget() &&
		 ( focusWidget()->focusPolicy() == QWidget::StrongFocus ||
		   focusWidget()->focusPolicy() == QWidget::WheelFocus ) ) {
		e->ignore();
		break;
	    }
	    // call ours, since c++ blocks us from calling the one
	    // belonging to focusWidget().
	    focusNextPrevChild( FALSE );
	    break;
	case Key_Down:
	case Key_Right:
	    if ( focusWidget() &&
		 ( focusWidget()->focusPolicy() == QWidget::StrongFocus ||
		   focusWidget()->focusPolicy() == QWidget::WheelFocus ) ) {
		e->ignore();
		break;
	    }
	    focusNextPrevChild( TRUE );
	    break;
	default:
	    e->ignore();
	    return;
	}
    } else {
	e->ignore();
    }
}

/*! \reimp */
void QDialog::closeEvent( QCloseEvent *e )
{
    e->accept();
    reject();
}


/*****************************************************************************
  Geometry management.
 *****************************************************************************/

/*! Shows the dialog box on the screen, as QWidget::show(), and
  selects a suitable position and size if none has been specified yet.

  \warning

  In Qt 2.x, calling show() on a modal dialog enters a local event
  loop, and work like exec(), but not returning the result code exec()
  returns. Trolltech has always warned against doing this.

  In Qt 3.0 and later, calling show() on a modal dialog will return
  immediately, \e not enter a local event loop. The dialog will of
  course be modal.

  \sa exec()
*/

void QDialog::show()
{
    if ( testWState(WState_Visible) )
	return;
    if ( !did_resize )
	adjustSize();
    if ( !did_move ) {
	QWidget *w = parentWidget();
	QPoint p( 0, 0 );
	int extraw = 0, extrah = 0;
	QWidget * desk = QApplication::desktop();
	if ( w )
	    w = w->topLevelWidget();

	QWidgetList  *list = QApplication::topLevelWidgets();
	QWidgetListIt it( *list );
	while ( (extraw == 0 || extrah == 0) &&
		it.current() != 0 ) {
	    int w, h;
	    QWidget * current = it.current();
	    ++it;
	    w = current->geometry().x() - current->x();
	    h = current->geometry().y() - current->y();

	    extraw = QMAX( extraw, w );
	    extrah = QMAX( extrah, h );
	}
	delete list;
	
	// sanity check for decoration frames. With embedding, we
	// might get extraordinary values
	if ( extraw >= 10 || extrah >= 40 )
	    extraw = extrah = 0;
	
	if ( w ) {
	    // Use mapToGlobal rather than geometry() in case w might
	    // be embedded in another application
	    QPoint pp = w->mapToGlobal( QPoint(0,0) );
	    p = QPoint( pp.x() + w->width()/2,
			pp.y() + w->height()/ 2 );
	} else {
	    p = QPoint( desk->width()/2, desk->height()/2 );
	}

	p = QPoint( p.x()-width()/2 - extraw,
		    p.y()-height()/2 - extrah );

	if ( p.x() + extraw + width() > desk->width() )
	    p.setX( desk->width() - width() - extraw );
	if ( p.x() < 0 )
	    p.setX( 0 );

	if ( p.y() + extrah + height() > desk->height() )
	    p.setY( desk->height() - height() - extrah );
	if ( p.y() < 0 )
	    p.setY( 0 );

	move( p );
    }
    QWidget::show();


#ifndef _WS_QWS_ // We remove this NOW for Qt/Embedded

    /*########### 3.0:

      This 'feature' is nonsense and will be removed in 3.0.
      show()
      should do show() and nothing more.  If these lines are removed,
      we can finally kill QSemiModal and let QProgressBar inherit
      QDialog.
     */
    if ( testWFlags(WType_Modal) && !in_loop ) {
	in_loop = TRUE;
	qApp->enter_loop();
    }
#endif
}

/*! \reimp */
void QDialog::hide()
{
    // Reimplemented to exit a modal when the dialog is hidden.
    QWidget::hide();
    if ( in_loop ) {
	in_loop = FALSE;
	qApp->exit_loop();
    }
}


/*****************************************************************************
  Detects any widget geometry changes done by the user.
 *****************************************************************************/

/*! \reimp */

void QDialog::move( int x, int y )
{
    did_move = TRUE;
    QWidget::move( x, y );
}

/*! \reimp */

void QDialog::move( const QPoint &p )
{
    did_move = TRUE;
    QWidget::move( p );
}

/*! \reimp */

void QDialog::resize( int w, int h )
{
    did_resize = TRUE;
    QWidget::resize( w, h );
}

/*! \reimp */

void QDialog::resize( const QSize &s )
{
    did_resize = TRUE;
    QWidget::resize( s );
}

/*! \reimp */

void QDialog::setGeometry( int x, int y, int w, int h )
{
    did_move   = TRUE;
    did_resize = TRUE;
    QWidget::setGeometry( x, y, w, h );
}

/*! \reimp */

void QDialog::setGeometry( const QRect &r )
{
    did_move   = TRUE;
    did_resize = TRUE;
    QWidget::setGeometry( r );
}


/*!  Sets the dialog to display its extension to the right of the main
  are if \a orientation is \c Horizonal, and to display it below the
  main area if \a orientation is \c Vertical.

  \sa orientation(), setExtension()
*/
void QDialog::setOrientation( Orientation orientation )
{
    d->orientation = orientation;
}

/*!
  Returns the extension direction of the dialog.

  \sa setOrientation()
*/
Qt::Orientation QDialog::orientation() const
{
    return d->orientation;
}

/*!  Sets \a extension to be the dialog's extension, or deletes the
  extensions if \a extension is 0.

  The dialogs takes over ownership of the extension. Any previously
  set extension is deleted.

  This function can only be called while the dialog is hidden.

  \sa showExtension(), setOrientation(), extension()
 */
void QDialog::setExtension( QWidget* extension )
{
    delete d->extension;
    d->extension = extension;

    if ( !extension )
	return;

    if ( extension->parentWidget() != this )
	extension->reparent( this, QPoint(0,0) );
    else
	extension->hide();
}

/*!
  Returns the dialog's extension or 0 if no extension has been
  defined.

  \sa setExtension()
 */
QWidget* QDialog::extension() const
{
    return d->extension;
}


/*!
  Extends the dialog to show its extension if \a showIt is TRUE
  and hides it else.

  This slot is usually connected to the \l QButton::toggled() signal
  of a QPushButton.

  If the dialog is not visible, nothing happens.

  \sa show(), setExtension(), setOrientation()
 */
void QDialog::showExtension( bool showIt )
{
    if ( !d->extension )
	return;
    if ( !testWState(WState_Visible) )
	return;

    if ( showIt ) {
	d->size = size();
	d->min = minimumSize();
	d->max = maximumSize();
#ifndef QT_NO_LAYOUT
	if ( layout() )
	    layout()->setEnabled( FALSE );
#endif
	QSize s( d->extension->sizeHint() );
	if ( d->orientation == Horizontal ) {
	    d->extension->setGeometry( width(), 0, s.width(), height() );
	    setFixedSize( width() + s.width(), height() );
	} else {
	    d->extension->setGeometry( 0, height(), width(), s.height() );
	    setFixedSize( width(), height() + s.height() );
	}
	d->extension->show();
    } else {
	d->extension->hide();
	setMinimumSize( d->min );
	setMaximumSize( d->max );
	resize( d->size );
#ifndef QT_NO_LAYOUT
	if ( layout() )
	    layout()->setEnabled( TRUE );
#endif
    }
}


/*! \reimp */
QSize QDialog::sizeHint() const
{
    if ( d->extension )
	if ( d->orientation == Horizontal )
	    return QSize( QWidget::sizeHint().width(),
			QMAX( QWidget::sizeHint().height(),d->extension->sizeHint().height() ) );
	else
	    return QSize( QMAX( QWidget::sizeHint().width(), d->extension->sizeHint().width() ),
			QWidget::sizeHint().height() );

    return QWidget::sizeHint();
}


/*! \reimp */
QSize QDialog::minimumSizeHint() const
{
    if ( d->extension )
	if (d->orientation == Horizontal )
	    return QSize( QWidget::minimumSizeHint().width(),
			QMAX( QWidget::minimumSizeHint().height(), d->extension->minimumSizeHint().height() ) );
	else
	    return QSize( QMAX( QWidget::minimumSizeHint().width(), d->extension->minimumSizeHint().width() ),
			QWidget::minimumSizeHint().height() );

    return QWidget::minimumSizeHint();
}


/*!
  \fn bool QDialog::isSizeGripEnabled() const

  Returns TRUE if the QDialog has a QSizeGrip in the bottom right of
  the dialog, and FALSE if it does not.

  \sa setSizeGripEnabled()
*/

bool QDialog::isSizeGripEnabled() const
{
#ifndef QT_NO_SIZEGRIP
    return !!d->resizer;
#else
    return FALSE;
#endif
}

/*!
  Enables or disables the QSizeGrip in the bottom right of the dialog.
  By default, the size grip is disabled.

  \sa isSizeGripEnabled()
*/
void QDialog::setSizeGripEnabled(bool enabled)
{
#ifndef QT_NO_SIZEGRIP
    if ( !enabled != !d->resizer ) {
	if ( enabled ) {
	    d->resizer = new QSizeGrip( this, "QDialog::resizer" );
	    d->resizer->adjustSize();
	    d->resizer->move( rect().bottomRight() -d->resizer->rect().bottomRight() );
	    d->resizer->raise();
	    d->resizer->show();
	} else {
	    delete d->resizer;
	    d->resizer = 0;
	}
    }
#endif //QT_NO_SIZEGRIP
}



/*! \reimp */
void QDialog::resizeEvent( QResizeEvent * )
{
#ifndef QT_NO_SIZEGRIP
    if ( d->resizer )
	d->resizer->move( rect().bottomRight() -d->resizer->rect().bottomRight() );
#endif
}

#endif // QT_NO_DIALOG
