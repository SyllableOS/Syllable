/****************************************************************************
** $Id$
**
** Implementation of event classes
**
** Created : 931029
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

#include "qevent.h"
#include "qcursor.h"
#include "qapplication.h"


// BEING REVISED: paul
/*!
  \class QEvent qevent.h
  \brief The QEvent class is base class of all
  event classes. Event objects contain event parameters.

  \ingroup environment
  \ingroup event

  The   main event loop of Qt (QApplication::exec()) fetches
  native window system events from the event queue, translates them
  into QEvent and sends the translated events to QObjects.

  Generally, events come from the underlying window system, but it is
  also possible to manually send events through the QApplication class
  using QApplication::sendEvent() and QApplication::postEvent().

  QObject received events by having its QObject::event() function
  called. The function can be reimplemented in subclasses to customize
  event handling and add additional event types. QWidget::event() is
  a notable example. By default, events are dispatched to event handlers
  like QObject::timerEvent() and QWidget::mouseMoveEvent().
  QObject::installEventFilter() allows an object to intercept events
  to another object.

  The basic QEvent only contains an event type parameter.  Subclasses
  of QEvent contain additional parameters that describe the particular
  event.

  \sa QObject::event() QObject::installEventFilter() QWidget::event()
  QApplication::sendEvent() QAppilcation::postEvent()
  QApplication::processEvents()
*/


/*! \enum Qt::ButtonState
  This enum type describes the state of the mouse buttons and the
  modifier buttons.  The currently defined values are:<ul>

  <li> \c NoButton - used when the button state does not refer to any
  button (see QMouseEvent::button()).

  <li> \c LeftButton - set if the left button is pressed, or this
  event refers to the left button.  Note that the left button may be
  the right button on left-handed mice.

  <li> \c RightButton - the right button.

  <li> \c MidButton - the middle button

  <li> \c ShiftButton - a shift key on the keyboard is also pressed.

  <li> \c ControlButton - a control key on the keyboard is also pressed.

  <li> \c AltButton - an alt (or meta) key on the keyboard is also pressed.

  <li> \c Keypad - a keypad button is pressed.

  </ul>
*/

/*! \enum QEvent::Type

  This enum type defines the valid event types in Qt.  The currently
  defined event types, and the specialized classes for each type, are: <ul>

  <li> \c None - not an event
  <li> \c Timer - regular timer events, QTimerEvent
  <li> \c MouseButtonPress - mouse press, QMouseEvent
  <li> \c MouseButtonRelease - mouse release, QMouseEvent
  <li> \c MouseButtonDblClick - mouse press again, QMouseEvent
  <li> \c MouseMove - mouse move, QMouseEvent
  <li> \c KeyPress - key press (including e.g. shift), QKeyEvent
  <li> \c KeyRelease - key release, QKeyEvent
  <li> \c FocusIn - widget gains keyboard focus, QFocusEvent
  <li> \c FocusOut - widget loses keyboard focus, QFocusEvent
  <li> \c Enter - mouse enters widget's space
  <li> \c Leave - mouse leaves widget's soace
  <li> \c Paint - screen update necessary, QPaintEvent
  <li> \c Move - widget's position changed, QMoveEvent
  <li> \c Resize - widget's size changed, QResizeEvent
  <li> \c Show - widget was shown on screen, QShowEvent
  <li> \c Hide - widget was removed from screen, QHideEvent
  <li> \c Close - widget was closed (permanently), QCloseEvent
  <li> \c Accel - key press in child, for shortcut key handling, QKeyEvent
  <li> \c Wheel - mouse wheel rolled, QWheelEvent
  <li> \c AccelAvailable - an internal event used by Qt on some platforms.
  <li> \c AccelOverride - key press in child, for overriding shortcut key handling, QKeyEvent
 <li> \c WindowActivate - the window was activated
 <li> \c WindowDeactivate - the window was deactivated
  <li> \c CaptionChange - widget's caption changed
  <li> \c IconChange - widget's icon changed
  <li> \c ParentFontChange - the font of the parent widget changed.
  <li> \c ApplicationFontChange - the default application font changed.
  <li> \c ParentPaletteChange - the palette of the parent widget changed.
  <li> \c ApplicationPaletteChange - the default application palette changed.
  <li> \c Clipboard - system clipboard contents have changed
  <li> \c SockAct - socket activated, used to implement QSocketNotifier
  <li> \c DragEnter - drag-and-drop enters widget, QDragEnterEvent
  <li> \c DragMove - drag-and-drop in progress, QDragMoveEvent
  <li> \c DragLeave - drag-and-drop leaves widget, QDragLeaveEvent
  <li> \c Drop - drag-and-drop is completed, QDropEvent
  <li> \c DragResponse - an internal event used by Qt on some platforms.
  <li> \c ChildInserted - object gets a child, QChildEvent
  <li> \c ChildRemoved - object loses a child, QChildEvent
  <li> \c LayoutHint - a widget child has changed layout properties
  <li> \c ActivateControl - an internal event used by Qt on some platforms.
  <li> \c DeactivateControl - an internal event used by Qt on some platforms.
  <li> \c Quit - reserved
  <li> \c Create - reserved
  <li> \c Destroy - reserved
  <li> \c Reparent - reserved
  <li> \c Configure - reserved
  <li> \c ConfigureLayout - reserved

  <li> \c User - user defined event
</ul>
*/
/*!
  \fn QEvent::QEvent( Type type )
  Contructs an event object with a \a type.
*/

/*!
  \fn QEvent::Type QEvent::type() const
  Returns the event type.
*/


/*!
  \class QTimerEvent qevent.h
  \brief The QTimerEvent class contains parameters that describe a
  timer event.

  \ingroup event

  Timer events are sent at regular intervals to objects that have
  started one or more timers.  Each timer has a unique identifier.
  A timer is started with  QObject::startTimer().

  The QTimer class provides a high-level programming interface that
  uses signals instead of events. It also provides one-shot timers.

  The event handler QObject::timerEvent() receives timer events.

  \sa QTimer, QObject::timerEvent(), QObject::startTimer(),
  QObject::killTimer(), QObject::killTimers()
*/

/*!
  \fn QTimerEvent::QTimerEvent( int timerId )
  Constructs a timer event object with the timer identifier set to \a timerId.
*/

/*!
  \fn int QTimerEvent::timerId() const
  Returns the unique timer identifier, which is the same identifier as
  returned from QObject::startTimer().
*/


/*!
  \class QMouseEvent qevent.h

  \brief The QMouseEvent class contains parameters that describe a mouse event.

  \ingroup event

  Mouse events occur when a mouse button is pressed or released inside a
  widget, or when the mouse cursor is moved.

  Mouse move events will only occur when some mouse button is pressed
  down, unless mouse tracking has been enabled with
  QWidget::setMouseTracking().

  Qt automatically grabs the mouse when a mouse button is pressed inside a
  widget, and the widget will continue to receive mouse events until the
  last mouse button is released.

  The functions pos(), x() and y() give the cursor position relative
  to the widget that receives the mouse event. If you move the widget
  as a result of the mouse event, use the global position returned by
  globalPos() to avoid a shaking motion.

  The QWidget::setEnable() function can be used to enable or disable mouse
  and keyboard events for a widget.

  The event handlers QWidget::mousePressEvent(), QWidget::mouseReleaseEvent(),
  QWidget::mouseDoubleClickEvent() and QWidget::mouseMoveEvent() receive
  mouse events.

  \sa QWidget::setMouseTracking(), QWidget::grabMouse(), QCursor::pos()
*/

/*!
  \fn QMouseEvent::QMouseEvent( Type type, const QPoint &pos, int button, int state )

  Constructs a mouse event object.

  The \a type parameter must be one of \c QEvent::MouseButtonPress,
  \c QEvent::MouseButtonRelease,
  \c QEvent::MouseButtonDblClick or \c QEvent::MouseMove.

  The \a pos parameter specifies the position relative to the
  receiving widget; \a button specifies the ButtonState of the button
  that caused the event, which should be 0 if \a type is \c
  MouseMove; \a state is the ButtonState at the time of the event.

  The globalPos() is initialized to QCursor::pos(), which may not be
  appropriate. Use the other constructor to specify the global position
  explicitly.
*/

QMouseEvent::QMouseEvent( Type type, const QPoint &pos, int button, int state )
    : QEvent(type), p(pos), b(button),s((ushort)state){
	g = QCursor::pos();
}


/*!
  \fn QMouseEvent::QMouseEvent( Type type, const QPoint &pos, const QPoint &globalPos,  int button, int state )

  Constructs a mouse event object.

  The \a type parameter must be \c QEvent::MouseButtonPress,
  \c QEvent::MouseButtonRelease,
  \c QEvent::MouseButtonDblClick or \c QEvent::MouseMove.

  The \a pos parameter specifies the position relative to the
  receiving widget; \a globalPos is the position in absolute
  coordinates; \a button specifies the ButtonState of the button that
  caused the event, which should be 0 if \a type is \c MouseMove; and
  \a state is the ButtonState at the time of the event.
*/

/*!
  \fn const QPoint &QMouseEvent::pos() const
  Returns the position of the mouse pointer, relative to the widget that
  received the event.

  If you move the widget as a result of the mouse event, use the
  global position returned by globalPos() to avoid a shaking motion.

  \sa x(), y(), globalPos()
*/

/*!
  \fn const QPoint &QMouseEvent::globalPos() const

  Returns the global position of the mouse pointer \e at \e the \e
  time of the event. This is important on asynchronous window systems
  like X11: Whenever you move your widgets around in response to mouse
  evens, globalPos() can differ a lot from the current pointer
  position QCursor::pos(), and from
  <code> QWidget::mapToGlobal( pos() ) </code>.

  \sa globalX(), globalY()
*/

/*!
  \fn int QMouseEvent::x() const
  Returns the X position of the mouse pointer, relative to the widget that
  received the event.
  \sa y(), pos()
*/

/*!
  \fn int QMouseEvent::y() const
  Returns the Y position of the mouse pointer, relative to the widget that
  received the event.
  \sa x(), pos()
*/

/*!
  \fn int QMouseEvent::globalX() const
  Returns the global X position of the mouse pointer at the time of the event
  \sa globalY(), globalPos()
*/

/*!
  \fn int QMouseEvent::globalY() const
  Returns the global Y position of the mouse pointer at the time of the event
  \sa globalX(), globalPos()
*/

/*!
  \fn ButtonState QMouseEvent::button() const
  Returns the button that caused the event.

  Possible return values are \c LeftButton, \c RightButton, \c MidButton and
  \c NoButton.

  Note that the returned value is always \c NoButton (0) for mouse move
  events.

  \sa state()
*/


/*!
  \fn ButtonState QMouseEvent::state() const

  Returns the button state (a combination of mouse buttons and keyboard
  modifiers), i.e. what buttons and keys were being held depressed
  immediately before the event was generated.

  Note that this means that for \c QEvent::MouseButtonPress and \c
  QEvent::MouseButtonDblClick, the flag for the button() itself will not be
  set in the state; while for \c QEvent::MouseButtonRelease, it will.

  This value is mainly interesting for \c QEvent::MouseMove, for the
  other cases, button() is more useful.

  The returned value is \c LeftButton, \c RightButton, \c MidButton,
  \c ShiftButton, \c ControlButton and \c AltButton OR'ed together.

  \sa button() stateAfter()
*/

/*!
  \fn ButtonState QMouseEvent::stateAfter() const

  Returns the state of buttons after the event.

  \sa state()
*/
Qt::ButtonState QMouseEvent::stateAfter() const
{
    return Qt::ButtonState(state()^button());
}



/*!
  \class QWheelEvent qevent.h
  \brief The QWheelEvent class contains parameters that describe a wheel event.

  \ingroup event


  Wheel events occur when a mouse wheel is turned while the widget has
  focus.  The rotation distance is provided by delta(). The functions
  pos() and globalPos() return the mouse pointer location at the
  time of the event.

  A wheel event contains a special accept flag which tells whether the
  receiver wants the event.  You should call QWheelEvent::accept() if you
  handle the wheel event, otherwise it will be sent to the parent widget.

  The QWidget::setEnable() function can be used to enable or disable mouse
  and keyboard events for a widget.

  The event handler QWidget::wheelEvent() receive wheel events.

  \sa QMouseEvent, QWidget::grabMouse()
*/

/*!
  \fn QWheelEvent::QWheelEvent( const QPoint &pos, int delta, int state )

  Constructs a wheel event object.

  The globalPos() is initialized to QCursor::pos(), which usually is
  right but not always. Use the other constructor if you need to
  specify the global position explicitly.

  \sa pos(), delta(), state()
*/
QWheelEvent::QWheelEvent( const QPoint &pos, int delta, int state )
    : QEvent(Wheel), p(pos), d(delta), s((ushort)state),
      accpt(TRUE)
{
    g = QCursor::pos();
}

/*!
  \fn QWheelEvent::QWheelEvent( const QPoint &pos, const QPoint&globalPos, int delta, int state )

  Constructs a wheel event object.

  \sa pos(), globalPos(), delta(), state()
*/

/*!
  \fn int QWheelEvent::delta() const

  Returns the distance that the wheel is rotated, expressed in
  multiples or divisions of WHEEL_DELTA, which is set at 120
  currently.A positive value indicates that the wheel was rotated
  forward, away from the user; a negative value indicates that the
  wheel was rotated backward, toward the user.

  The WHEEL_DELTA constant was set to 120 by the wheel mouse vendors
  to allow building finer-resolution wheels in the future, including
  perhaps a freely-rotating wheel with no notches. The expectation is
  that such a device would send more messages per rotation, but with a
  smaller value in each message.
*/

/*!
  \fn const QPoint &QWheelEvent::pos() const
  Returns the position of the mouse pointer, relative to the widget that
  received the event.

  If you move your widgets around in response to mouse
  evens, use globalPos() instead of this function.

  \sa x(), y(), globalPos()
*/

/*!
  \fn int QWheelEvent::x() const
  Returns the X position of the mouse pointer, relative to the widget that
  received the event.
  \sa y(), pos()
*/

/*!
  \fn int QWheelEvent::y() const
  Returns the Y position of the mouse pointer, relative to the widget that
  received the event.
  \sa x(), pos()
*/


/*!
  \fn const QPoint &QWheelEvent::globalPos() const

  Returns the global position of the mouse pointer \e at \e the \e
  time of the event. This is important on asynchronous window systems
  like X11: Whenever you move your widgets around in response to mouse
  evens, globalPos() can differ a lot from the current pointer
  position QCursor::pos().

  \sa globalX(), globalY()
*/

/*!
  \fn int QWheelEvent::globalX() const
  Returns the global X position of the mouse pointer at the time of the event
  \sa globalY(), globalPos()
*/

/*!
  \fn int QWheelEvent::globalY() const
  Returns the global Y position of the mouse pointer at the time of the event
  \sa globalX(), globalPos()
*/


/*!
  \fn ButtonState QWheelEvent::state() const
  Returns the keyboard modifier flags of the event.

  The returned value is \c ShiftButton, \c ControlButton and \c AltButton
  OR'ed together.
*/

/*!
  \fn bool QWheelEvent::isAccepted() const
  Returns TRUE if the receiver of the event handles the wheel event
*/

/*!
  \fn void QWheelEvent::accept()
  Sets the accept flag of the wheel event object.

  Setting the accept parameter indicates that the receiver of the event wants
  the wheel event. Unwanted wheel events are sent to the parent widget.

  The accept flag is set by default.

  \sa ignore()
*/

/*!
  \fn void QWheelEvent::ignore()
  Clears the accept flag parameter of the wheel event object.

  Clearing the accept parameter indicates that the event receiver does
  not want the wheel event. Unwanted wheel events are sent to the parent widget.
  The accept flag is set by default.

  \sa accept()
*/



/*!
  \class QKeyEvent qevent.h
  \brief The QKeyEvent class contains parameters that describe a key event.

  \ingroup event

  Key events occur when a key is pressed or released when a widget has
  keyboard input focus.

  A key event contains a special accept flag which tells whether the
  receiver wants the key.  You should call QKeyEvent::ignore() if the
  key press or release event is not handled by your widget.

  The QWidget::setEnable() function can be used to enable or disable mouse
  and keyboard events for a widget.

  The event handlers QWidget::keyPressEvent() and QWidget::keyReleaseEvent()
  receive key events.

  \sa QFocusEvent, QWidget::grabKeyboard()
*/

/*!
  \fn QKeyEvent::QKeyEvent( Type type, int key, int ascii, int state,
			    const QString& text, bool autorep, ushort count )
  Constructs a key event object.

  The \a type parameter must be \c QEvent::KeyPress or \c QEvent::KeyRelease.

  If \a key is 0, the event is not a result of a known key (e.g. it
  may be the result of a compose sequence or keyboard macro).

  \a text will be returned by text().

  If \a autorep is TRUE then isAutoRepeat() will be TRUE.

  \a count is the number of single keys.

  The accept flag is set to TRUE.
*/

/*!
  \fn int QKeyEvent::key() const
  Returns the code of the key that was pressed or released.

  The header file qnamespace.h lists the possible keyboard codes.  These codes
  are independent of the underlying window system.

  Key code 0 means that the event is not a result of a known key (e.g. it
  may be the result of a compose sequence or keyboard macro).
*/

/*!
  \fn int QKeyEvent::ascii() const
  Returns the ASCII code of the key that was pressed or released.
  We recommend using text() instead.

  \sa text()
*/

/*!
  \fn QString QKeyEvent::text() const
  Returns the Unicode text which this key generated.

  \sa QWidget::setKeyCompression()
*/

/*!
  \fn ButtonState QKeyEvent::state() const
  Returns the keyboard modifier flags that existed immediately before
  the event occurred.

  The returned value is \c ShiftButton, \c ControlButton and \c AltButton
  OR'ed together.

  \sa stateAfter()
*/

/*!
  \fn ButtonState QKeyEvent::stateAfter() const

  Returns the keyboard modifier flags that existed immediately after
  the event occurred.

  \warning This function cannot be trusted.

  \sa state()
*/
//###### We must check with XGetModifierMapping
Qt::ButtonState QKeyEvent::stateAfter() const
{
    if ( key() == Key_Shift )
	return Qt::ButtonState(state()^ShiftButton);
    if ( key() == Key_Control )
	return Qt::ButtonState(state()^ControlButton);
    if ( key() == Key_Alt )
	return Qt::ButtonState(state()^AltButton);
    return state();
}

/*!
  \fn bool QKeyEvent::isAccepted() const
  Returns TRUE if the receiver of the event wants to keep the key.
*/

/*!
  \fn void QKeyEvent::accept()
  Sets the accept flag of the key event object.

  Setting the accept parameter indicates that the receiver of the event wants
  the key event. Unwanted key events are sent to the parent widget.

  The accept flag is set by default.

  \sa ignore()
*/

/*! \fn bool QKeyEvent::isAutoRepeat() const

  Returns TRUE if this event comes from an auto-repeating key and
  FALSE if it comes from an initial press.

  Note that if the event is a multiple-key compressed event which
  partly is due to autorepeat, this function returns an indeterminate
  value.
*/

/*!
  \fn int QKeyEvent::count() const

  Returns the number of single keys for this event. If text() is not
  empty, this is simply the length of the string.

  However, Qt also compresses invisible keycodes, such as BackSpace.
  For those, count() returns the number of key presses/repeats this
  event represents.

  \sa QWidget::setKeyCompression()
*/

/*!
  \fn void QKeyEvent::ignore()
  Clears the accept flag parameter of the key event object.

  Clearing the accept parameter indicates that the event receiver does
  not want the key event. Unwanted key events are sent to the parent
  widget.

  The accept flag is set by default.

  \sa accept()
*/


/*!
  \class QFocusEvent qevent.h
  \brief The QFocusEvent class contains event parameters for widget focus
  events.

  \ingroup event

  Focus events are sent to widgets when the keyboard input focus
  changes.  This happens due to either a mouse action, the tab key,
  the window system, a keyboard shortcut or some other application
  specific issue. The actual reason for a specific event is obtained by
  reason() in the appropriate event handler.

  The event handlers QWidget::focusInEvent() and QWidget::focusOutEvent()
  receive focus events.

  \sa QWidget::setFocus(), QWidget::setFocusPolicy()
*/

/*!
  \fn QFocusEvent::QFocusEvent( Type type )
  Constructs a focus event object.

  The \a type parameter must be either \a QEvent::FocusIn or \a QEvent::FocusOut.
*/



QFocusEvent::Reason QFocusEvent::m_reason = QFocusEvent::Other;
QFocusEvent::Reason QFocusEvent::prev_reason = QFocusEvent::Other;


/*! \enum QFocusEvent::Reason

  <ul>
  <li> \c Mouse - the focus change happened because of a mouse action
  <li> \c Tab - the focus change happened because of a Tab press
        (possibly including shift/control)
  <li> \c ActiveWindow - the window system made this window (in)active
  <li> \c Popup - the application opened/closed a popup that grabbed/released focus
  <li> \c Shortcut - the focus change happened because of a keyboard shortcut
  <li> \c Other - any other reason, usually application-specific
  </ul>

   See the \link focus.html focus overview \endlink for more about focus.

*/

/*!
  Returns the reason for this focus event.

  \sa setReason()
 */
QFocusEvent::Reason QFocusEvent::reason()
{
    return m_reason;
}

/*!
  Sets the reason for all future focus events to \a reason.

  \sa reason(), resetReason()
 */
void QFocusEvent::setReason( Reason reason )
{
    prev_reason = m_reason;
    m_reason = reason;
}

/*!
  Resets the reason for all future focus events to the value before
  the last setReason() call.

  \sa reason(), setReason()
 */
void QFocusEvent::resetReason()
{
    m_reason = prev_reason;
}

/*!
  \fn bool QFocusEvent::gotFocus() const
  Returns TRUE if the widget received the text input focus.
*/

/*!
  \fn bool QFocusEvent::lostFocus() const
  Returns TRUE if the widget lost the text input focus.
*/


/*!
  \class QPaintEvent qevent.h
  \brief The QPaintEvent class contains event parameters for paint events.

  \ingroup event


  Paint events are sent to widgets that need to update themselves, for instance
  when a part of a widget is exposed because an overlying widget is moved away.

  The event handler QWidget::paintEvent() receives paint events.

  \sa QPainter, QWidget::update(), QWidget::repaint()
*/

/*!
  \fn QPaintEvent::QPaintEvent( const QRegion &paintRegion, bool erased=TRUE )
  Constructs a paint event object with the region that should be updated.
*/

/*!
  \fn QPaintEvent::QPaintEvent( const QRect &paintRect, bool erased=TRUE )
  Constructs a paint event object with the rectangle that should be updated.
*/

/*!
  \fn const QRect &QPaintEvent::rect() const
  Returns the rectangle that should be updated.

  \sa region(), QPainter::setClipRect()
*/

/*!
  \fn const QRegion &QPaintEvent::region() const
  Returns the region that should be updated.

  \sa rect(), QPainter::setClipRegion()
*/

/*!
  \fn bool QPaintEvent::erased() const
  Returns whether the paint event region (or rectangle) has been
  erased with the widget's background.
*/

/*!
  \class QMoveEvent qevent.h
  \brief The QMoveEvent class contains event parameters for move events.

  \ingroup event


  Move events are sent to widgets that have been moved to a new position
  relative to their parent.

  The event handler QWidget::moveEvent() receives move events.

  \sa QWidget::move(), QWidget::setGeometry()
*/

/*!
  \fn QMoveEvent::QMoveEvent( const QPoint &pos, const QPoint &oldPos )
  Constructs a move event with the new and old widget positions.
*/

/*!
  \fn const QPoint &QMoveEvent::pos() const
  Returns the new position of the widget, which is the same as
  QWidget::pos().
*/

/*!
  \fn const QPoint &QMoveEvent::oldPos() const
  Returns the old position of the widget.
*/


/*!
  \class QResizeEvent qevent.h
  \brief The QResizeEvent class contains event parameters for resize events.

  \ingroup event


  Resize events are sent to widgets that have been resized.

  The event handler QWidget::resizeEvent() receives resize events.

  \sa QWidget::resize(), QWidget::setGeometry()
*/

/*!
  \fn QResizeEvent::QResizeEvent( const QSize &size, const QSize &oldSize )
  Constructs a resize event with the new and old widget sizes.
*/

/*!
  \fn const QSize &QResizeEvent::size() const
  Returns the new size of the widget, which is the same as
  QWidget::size().
*/

/*!
  \fn const QSize &QResizeEvent::oldSize() const
  Returns the old size of the widget.
*/


/*!
  \class QCloseEvent qevent.h
  \brief The QCloseEvent class contains parameters that describe a close event.

  \ingroup event

  Close events are sent to widgets that the user wants to close, usually
  by choosing "Close" from the window menu. They are also sent when you
  call QWidget::close() to close a widget from inside the program.

  Close events contain a special accept flag which tells whether the
  receiver wants the widget to be closed.  When a widget accepts the
  close event, it is hidden. If it refuses to accept the close event,
  either nothing happens or it is forcibly closed, if the sender of
  the close event really wants to.

  The main widget of the application - QApplication::mainWidget() - is
  a special case.  When it accepts the close event, Qt leaves the main
  event loop and the application is immediately terminated (i.e. it
  returns back from the call to QApplication::exec() in your main()
  function).

  The event handler QWidget::closeEvent() receives close events.  The
  default implementation of this event handler accepts the close
  event.  If you do not want your widget to be hidden, or want some
  special handing, you need to reimplement the event handler.

  The <a href="simple-application.html#closeEvent">closeEvent() in the
  Application Walkthrough</a> shows a close event handler that asks
  whether to save the document before closing.

  If you want your widget also to be deleted when it is closed, simply
  create it with the \c WDestructiveClose widget flag.  This is very
  useful for the independent top-level windows of a multi window
  application. The Application Walkthrough example uses this too.

  QObject emits the \link QObject::destroyed() destroyed()\endlink signal
  when it is deleted.  This is a useful signal if a widget needs to know
  when another widget is deleted.

  If the last toplevel window is closed, the
  QApplication::lastWindowClosed() signal is emitted.

  \sa QWidget::close(), QWidget::hide(), QObject::destroyed(),
  QApplication::setMainWidget(), QApplication::lastWindowClosed(),
  QApplication::exec(), QApplication::quit()
*/

/*!
  \fn QCloseEvent::QCloseEvent()
  Constructs a close event object with the accept parameter flag set to FALSE.
*/

/*!
  \fn bool QCloseEvent::isAccepted() const
  Returns TRUE if the receiver of the event has agreed to close the widget.
  \sa accept(), ignore()
*/

/*!
  \fn void QCloseEvent::accept()
  Sets the accept flag of the close event object.

  Setting the accept flag indicates that the receiver of this event agrees
  to close the widget.

  The accept flag is not set by default.

  If you choose to accept in QWidget::closeEvent(), the widget will be
  hidden.  If the widget's WDestructiveClose flag is set, it is also
  destroyed.

  \sa ignore(), QWidget::hide()
*/

/*!
  \fn void QCloseEvent::ignore()
  Clears the accept flag of the close event object.

  Clearing the accept flag indicates that the receiver of this event does not
  want the widget to be closed.

  The accept flag is not set by default.

  \sa accept()
*/


/*!
  \class QChildEvent qevent.h
  \brief The QChildEvent class contains event parameters for child object
  events.

  \ingroup event

  Child events are sent to objects when children are inserted or removed.

  A \c ChildRemoved event is sent immediately, but a \c ChildInserted event
  is \e posted (with QApplication::postEvent())



  The handler for these events is QObject::childEvent().
*/

/*!
  \fn QChildEvent::QChildEvent( Type type, QObject *child )
  Constructs a child event object.

  The \a type parameter must be either \a QEvent::ChildInserted
  or \a QEvent::ChildRemoved.
*/

/*!
  \fn QObject *QChildEvent::child() const
  Returns the child widget inserted or removed.
*/

/*!
  \fn bool QChildEvent::inserted() const
  Returns TRUE if the widget received a new child.
*/

/*!
  \fn bool QChildEvent::removed() const
  Returns TRUE if the object lost a child.
*/




/*!
  \class QCustomEvent qevent.h
  \brief The QCustomEvent class provides support for custom events.

  \ingroup event

  QCustomEvent is a generic event class for user-defined events. User
  defined events can be sent to widgets or other QObject instances
  using QApplication::postEvent() or
  QApplication::sendEvent(). Subclasses of QWidget can easily receive
  custom events by implementing the QWidget::customEvent() event
  handler function.

  QCustomEvent objects should be created with a type id that uniquely
  identifies the event type. To avoid clashes with the Qt-defined
  events types, the value should be at least as large as the value of
  the "User" entry in the QEvent::Type enum.

  QCustomEvent contains a generic void* data member that may be used
  for transferring event-specific data to the receiver. Note that
  since events are normally delivered asynchronously, the data
  pointer, if used, must remain valid until the event has been
  received and processed.

  QCustomEvent can be used as-is for simple user-defined event types,
  but normally you will want to make a subclass of it for your event
  types. In a subclass, you can add data members that are suitable for
  your event type.

  Example:
  \code
  class ColorChangeEvent : public QCustomEvent
  {
  public:
    ColorChangeEvent( QColor color )
	: QCustomEvent( 346798 ), c( color ) {};
    QColor color() const { return c; };
  private:
    QColor c;
  };

  // To send an event of this custom event type:

  ColorChangeEvent* ce = new ColorChangeEvent( blue );
  QApplication::postEvent( receiver, ce );    // Qt will delete it when done

  // To receive an event of this custom event type:

  void MyWidget::customEvent( QCustomEvent * e )
  {
    if ( e->type() == 346798 ) {              // It must be a ColorChangeEvent
      ColorChangeEvent* ce = (ColorChangeEvent*)e;
      newColor = ce->color();
    }
  }
  \endcode

  \sa QWidget::customEvent(), QApplication::notify()
*/


/*!
  Constructs a custom event object with event type \a type. The value
  of \a type must be at least as large as QEvent::User. The data
  pointer is set to 0.
*/

QCustomEvent::QCustomEvent( int type )
    : QEvent( (QEvent::Type)type ), d( 0 )
{
#if defined(CHECK_RANGE)
    if ( type < (int)QEvent::User )
	qWarning( "QCustomEvent: Illegal type id." );
#endif
}


/*!
  \fn QCustomEvent::QCustomEvent( Type type, void *data )
  Constructs a custom event object with the event type \a type and a
  pointer to \a data. (Note that any int value may safely be cast to
  QEvent::Type).
*/


/*!
  \fn void QCustomEvent::setData( void* data )

  Sets the generic data pointer to \a data.

  \sa data()
*/

/*!
  \fn void *QCustomEvent::data() const

  Returns a pointer to the generic event data.

  \sa setData()
*/



/*!
  \fn QDragMoveEvent::QDragMoveEvent( const QPoint& pos, Type type )

  Creates a QDragMoveEvent for which the mouse is at point \a pos,
  and the given event \a type.

  Note that internal state is also involved with QDragMoveEvent,
  so it is not useful to create these yourself.
*/

/*!
  \fn void   QDragMoveEvent::accept( const QRect & r )

  The same as accept(), but also notifies that future moves will
  also be acceptable if they remain within the rectangle \a r on the
  widget - this can improve performance, but may also be ignored by
  the underlying system.

  If the rectangle \link QRect::isEmpty() is empty\endlink, then drag
  move events will be sent continuously.  This is useful if the source is
  scrolling in a timer event.
*/

/*!
  \fn void   QDragMoveEvent::ignore( const QRect & r)

  The opposite of accept(const QRect&).
*/

/*!
  \fn QRect  QDragMoveEvent::answerRect() const

  Returns the rectangle for which the acceptance of
  the move event applies.
*/



/*!
  \fn const QPoint& QDropEvent::pos() const

  Returns the position where the drop was made.
*/

/*!
  \fn bool QDropEvent::isAccepted () const

  Returns TRUE if the drop target accepts the event.
*/

/*!
  \fn void QDropEvent::accept(bool y=TRUE)

  \reimp

  Call this to indicate whether the event provided data which your
  widget processed.  To get the data, use encodedData(), or
  preferably, the decode() methods of existing QDragObject subclasses,
  such as QTextDrag::decode(), or your own subclasses.

  \warning To accept or reject the drop, call acceptAction(), not this
  function.  This function indicates whether you processed the event
  at all.

  \sa acceptAction()
*/

/*!
  \fn void QDropEvent::acceptAction(bool y=TRUE)

  Call this to indicate that the action described by action() is accepted,
  not merely the default copy action.  If you call acceptAction(TRUE),
  there is no need to also call accept(TRUE).
*/

/*!
  \fn void QDragMoveEvent::accept( bool y )
  \reimp
  \internal
  Remove in 3.0
*/

/*!
  \fn void QDragMoveEvent::ignore()
  \reimp
  \internal
  Remove in 3.0
*/


/*!
  \enum QDropEvent::Action

  This type describes the action which a source requests that a target
  perform with dropped data.  The values are:

  <ul>
   <li>\c Copy - the default action.  The source simply users the data
	    provided in the operation.
   <li>\c Link. The source should somehow create a link to the location
	    specified by the data.
   <li>\c Move.  The source should somehow move the object from the location
	    specified by the data to a new location.
   <li>\c Private.  The target has special knowledge of the MIME type, which
	    the source should respond to similar to a Copy.
   <li>\c UserAction.  The source and target can co-operate using special
	    actions.  This feature is not supported in Qt at this time.
  </ul>

  The Link and Move actions only makes sense if the data is
  a reference, such as text/uri-list file lists (see QUriDrag).
*/

/*!
  \fn void QDropEvent::setAction( Action a )

  Sets the action.  This is used internally, you should not need to
  call this in your code - the \e source decides the action, not the
  target.
*/

/*!
  \fn Action QDropEvent::action() const

  Returns the Action which the target is requesting be performed with
  the data.  If your application understands the action and can
  process the supplied data, call acceptAction(); if your application
  can process the supplied data but can only perform the Copy action,
  call accept().
*/

/*!
  \fn void QDropEvent::ignore()

  The opposite of accept().
*/

/*! \fn bool QDropEvent::isActionAccepted () const

  Returns TRUE if the drop action was accepted by the drop site, and
  FALSE if not.
*/


/*! \fn void QDropEvent::setPoint (const QPoint & np)

  Sets the drop to happen at \a np.  You do normally not need to use
  this as it will be set internally before your widget receives the
  drop event.
*/ // ### here too - what coordinate system?


/*!
  \class QDragEnterEvent qevent.h
  \brief The event sent to widgets when a drag-and-drop first drags onto it.

  This event is always immediate followed by a QDragMoveEvent, thus you need
  only respond to one or the other event.  Note that this class inherits most
  of its functionality from QDragMoveEvent, which in turn inherits most
  of its functionality from QDropEvent.

  \sa QDragLeaveEvent, QDragMoveEvent, QDropEvent
*/

/*!
  \fn QDragEnterEvent::QDragEnterEvent (const QPoint & pos)
  Constructs a QDragEnterEvent entering at the given point.
  Note that QDragEnterEvent constructed outside of the Qt internals
  will not work - they currently rely on internal state.
*/

/*!
  \class QDragLeaveEvent qevent.h
  \brief The event sent to widgets when a drag-and-drop leaves it.

  This event is always preceded by a QDragEnterEvent and a series
  of QDragMoveEvent.  It is not sent if a QDropEvent is sent instead.

  \sa QDragEnterEvent, QDragMoveEvent, QDropEvent
*/

/*!
  \fn QDragLeaveEvent::QDragLeaveEvent()
  Constructs a QDragLeaveEvent.
  Note that QDragLeaveEvent constructed outside of the Qt internals
  will not work - they currently rely on internal state.
*/

/*!
  \class QHideEvent qevent.h
  \brief The event sent after a widget is hidden.

  This event is sent just before QWidget::hide() returns, and also when
  a top-level window has been hidden (iconified) by the user.

  \sa QShowEvent
*/

/*!
  \fn QHideEvent::QHideEvent(bool spontaneous)

  Constructs a QHideEvent.  \a spontaneous is TRUE if the event
  originated outside the application - ie. the user hid the window via the
  window manager controls.
*/

/*!
  \fn bool QHideEvent::spontaneous () const

  Returns TRUE if the event originated outside the application -
  ie. the user hid the window via the window manager controls, either
  by iconifying the window or by switching to another virtual desktop where
  the window isn't visible. The window will become hidden, but not withdrawn.
  If the window was iconified, QWidget::isMinimized() is TRUE.
*/

/*!
  \class QShowEvent qevent.h
  \brief The event sent when a widget is shown.

  There are two kind of show events: spontaneous show events by the
  window system and internal show events. Spontaneous show events are
  sent just after the window system shows the window, including after
  a top-level window has been shown (un-iconified) by the
  user. Internal show events are delivered just before the widget
  becomes visible.

  \sa QHideEvent
*/

/*!
  \fn QShowEvent::QShowEvent(bool spontaneous)

  Constructs a QShowEvent.  \a spontaneous is TRUE if the event
  originated outside the application - ie. the user revealed the window via the
  window manager controls.
*/

/*!
  \fn bool QShowEvent::spontaneous () const
  Returns TRUE if the event
  originated outside the application - ie. the user revealed the window via the
  window manager controls.
*/


/*!
  \fn QByteArray QDropEvent::data(const char* f) const

  \obsolete

  Use QDropEvent::encodedData().
*/


/*!
  Destructs the event.  If it was \link
  QApplication::postEvent() posted \endlink,
  it will be removed from the list of events to be posted.

  \internal
  It used to print an error (useful for people who posted events
  that were on the stack).
*/

QEvent::~QEvent()
{
#ifndef __ATHEOS__
    if (posted)
	QApplication::removePostedEvent( this );
#endif    
}

