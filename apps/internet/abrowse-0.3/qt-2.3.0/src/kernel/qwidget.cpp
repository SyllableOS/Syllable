/****************************************************************************
** $Id$
**
** Implementation of QWidget class
**
** Created : 931031
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


#include "qobjectlist.h"
#include "qobjectdict.h"
#include "qwidget.h"
#include "qwidgetlist.h"
#include "qwidgetintdict.h"
#include "qptrdict.h"
#include "qfocusdata.h"
#include "qpixmap.h"
#include "qapplication.h"
#include "qbrush.h"
#include "qlayout.h"
#if defined(_WS_WIN_)
#include "qt_windows.h"
#endif
#if defined(_WS_QWS_)
#include "qwsmanager_qws.h"
#endif

// NOT REVISED
/*!
  \class QWidget qwidget.h
  \brief The QWidget class is the base class of all user interface objects.

  \ingroup abstractwidgets

  The widget is the atom of the user interface: It receives mouse,
  keyboard and other events from the window system, and paints a
  representation of itself on the screen.  Every widget is
  rectangular, and they are sorted in a Z-order.  A widget is clipped
  by its parent and by the widgets in front of it.

  A widget that isn't embedded in a parent widget is called a
  top-level widget. Usually, top-level widgets are windows with a
  frame and a title bar (though it is also possible to create top
  level widgets without such decoration by the use of <a
  href="qt.html#WidgetFlags">widget flags</a>).  In Qt, QMainWindow and the
  various subclasses of QDialog are the most common top-level windows.

  A widget without a parent widget is always a top-level widget.

  The opposite of top-level widgets are child widgets. Those are child
  windows in their parent widgets.  You usually cannot distinguish a
  child widget from its parent visually.  Most other widgets in Qt are
  useful only as child widgets.  (You \e can make a e.g. button into a
  top-level widget, but most people prefer to put their buttons in
  e.g. dialogs.)

  QWidget has many member functions, but some of them have little
  direct functionality - for example it has a font but never uses it
  itself. There are many subclasses which provide real functionality,
  as diverse as QPushButton, QListBox and QTabDialog.

  <strong>Groups of functions:</strong>
  <ul>

  <li> Window functions:
	show(),
	hide(),
	raise(),
	lower(),
	close().

  <li> Top level windows:
	caption(),
	setCaption(),
	icon(),
	setIcon(),
	iconText(),
	setIconText(),
	isActiveWindow(),
	setActiveWindow(),
	showMinimized().
	showMaximized(),
	showFullScreen(),
	showNormal().

  <li> Window contents:
	update(),
	repaint(),
	erase(),
	scroll(),
	updateMask().

  <li> Geometry:
	pos(),
	size(),
	rect(),
	x(),
	y(),
	width(),
	height(),
	sizePolicy(),
	setSizePolicy(),
	sizeHint(),
	updateGeometry(),
	layout(),
	move(),
	resize(),
	setGeometry(),
	frameGeometry(),
	geometry(),
	childrenRect(),
	adjustSize(),
	mapFromGlobal(),
	mapFromParent()
	mapToGlobal(),
	mapToParent(),
	maximumSize(),
	minimumSize(),
	sizeIncrement(),
	setMaximumSize(),
	setMinimumSize(),
	setSizeIncrement(),
	setBaseSize(),
	setFixedSize()

  <li> Mode:
	isVisible(),
	isVisibleTo(),
	visibleRect(),
	isMinimized(),
	isDesktop(),
	isEnabled(),
	isEnabledTo(),
	isModal(),
	isPopup(),
	isTopLevel(),
	setEnabled(),
	hasMouseTracking(),
	setMouseTracking(),
	isUpdatesEnabled(),
	setUpdatesEnabled(),

  <li> Look and feel:
	style(),
	setStyle(),
	cursor(),
	setCursor()
	font(),
	setFont(),
	palette(),
	setPalette(),
	backgroundMode(),
	setBackgroundMode(),
	backgroundPixmap(),
	setBackgroundPixmap(),
	setTranslateBackground(),
	backgroundColor(),
	colorGroup(),
	fontMetrics(),
	fontInfo().

  <li> Keyboard focus functions:
	isFocusEnabled(),
	setFocusPolicy(),
	focusPolicy(),
	hasFocus(),
	setFocus(),
	clearFocus(),
	setTabOrder(),
	setFocusProxy().

  <li> Mouse and keyboard grabbing:
	grabMouse(),
	releaseMouse(),
	grabKeyboard(),
	releaseKeyboard(),
	mouseGrabber(),
	keyboardGrabber().

  <li> Event handlers:
	event(),
	mousePressEvent(),
	mouseReleaseEvent(),
	mouseDoubleClickEvent(),
	mouseMoveEvent(),
	keyPressEvent(),
	keyReleaseEvent(),
	focusInEvent(),
	focusOutEvent(),
	wheelEvent(),
	enterEvent(),
	leaveEvent(),
	paintEvent(),
	moveEvent(),
	resizeEvent(),
	closeEvent(),
	dragEnterEvent(),
	dragMoveEvent(),
	dragLeaveEvent(),
	dropEvent(),
	childEvent(),
	showEvent(),
	hideEvent(),
	customEvent().

  <li> Change handlers:
	backgroundColorChange(),
	backgroundPixmapChange(),
	enabledChange(),
	fontChange(),
	paletteChange(),
	styleChange().

  <li> System functions:
	parentWidget(),
	topLevelWidget(),
	reparentolish((),
	winId(),
	find(),
	metric().

  <li> Internal kernel functions:
	setFRect(),
	setCRect(),
	focusNextPrevChild(),
	wmapper(),
	clearWFlags(),
	getWFlags(),
	setWFlags(),
	testWFlags().

  <li> What's this help:
	customWhatsThis()
  </ul>

  Every widget's constructor accepts two or three standard arguments:
  <ul>
  <li><code>QWidget *parent = 0</code> is the parent of the new widget.
  If it is 0 (the default), the new widget will be a top-level window.
  If not, it will be a child of \e parent, and be constrained by \e
  parent's geometry (Unless you specify \c WType_TopLevel as
  widget flag).
  <li><code>const char *name = 0</code> is the widget name of the new
  widget.  You can access it using name().  The widget name is little
  used by programmers but is quite useful with GUI builders such as the
  Qt Designer (you can name a widget in the builder, and connect() to
  it by name in your code).  The dumpObjectTree() debugging function also
  uses it.
  <li><code>WFlags f = 0</code> (where available) sets the <a
  href="qt.html#WidgetFlags">widget flags</a>; the default is good for almost
  all widgets, but to get e.g. top-level widgets without a window
  system frame you must use special flags.
  </ul>

  The tictac/tictac.cpp example program is good example of a simple
  widget.  It contains a few event handlers (as all widgets must), a
  few custom routines that are peculiar to it (as all useful widgets
  must), and has a few children and connections.  Everything it does
  is done in response to an event: This is by far the most common way
  to design GUI applications.

  You will need to supply the content for your widgets yourself, but
  here is a brief run-down of the events, starting with the most common
  ones: <ul>

  <li> paintEvent() - called whenever the widget needs to be
  repainted.  Every widget which displays output must implement it,
  and it is sensible to \e never paint on the screen outside
  paintEvent().

  <li> resizeEvent() - called when the widget has been resized.

  <li> mousePressEvent() - called when a mouse button is pressed.
  There are six mouse-related events, mouse press and mouse release
  events are by far the most important.  A widget receives mouse press
  events when the widget is inside it, or when it has grabbed the
  mouse using grabMouse().

  <li> mouseReleaseEvent() - called when a mouse button is released.
  A widget receives mouse release events when it has received the
  corresponding mouse press event.  This means that if the user
  presses the mouse inside \e your widget, then drags the mouse to
  somewhere else, then releases, \e your widget receives the release
  event.  There is one exception, however: If a popup menu appears
  while the mouse button is held down, that popup steals the mouse
  events at once.

  <li> mouseDoubleClickEvent() - not quite as obvious as it might seem.
  If the user double-clicks, the widget receives a mouse press event
  (perhaps a mouse move event or two if he/she does not hold the mouse
  quite steady), a mouse release event and finally this event.  It is \e
  not \e possible to distinguish a click from a double click until you've
  seen whether the second click arrives.  (This is one reason why most GUI
  books recommend that double clicks be an extension of single clicks,
  rather than trigger a different action.)
  </ul>

  If your widget only contains child widgets, you probably do not need to
  implement any event handlers.

  Widgets that accept keyboard input need to reimplement a few more
  event handlers: <ul>

  <li> keyPressEvent() - called whenever a key is pressed, and again
  when a key has been held down long enough for it to auto-repeat.
  Note that the Tab and shift-Tab keys are only passed to the widget
  if they are not used by the focus-change mechanisms.  To force those
  keys to be processed by your widget, you must reimplement
  QWidget::event().

  <li> focusInEvent() - called when the widget gains keyboard focus
  (assuming you have called setFocusPolicy(), of course). Well
  written widgets indicate that they own the keyboard focus in a clear
  but discreet way.

  <li> focusOutEvent() - called when the widget loses keyboard
  focus.
  </ul>

  Some widgets will need to reimplement some more obscure event
  handlers, too: <ul>

  <li> mouseMoveEvent() - called whenever the mouse moves while a
  button is held down.  This is useful for e.g. dragging.  If you call
  setMouseTracking(TRUE), you get mouse move events even when no
  buttons are held down.  (Note that applications which make use of
  mouse tracking are often not very useful on low-bandwidth X
  connections.)

  <li> keyReleaseEvent() - called whenever a key is released, and also
  while it is held down if the key is auto-repeating.  In that case
  the widget receives a key release event and immediately a key press
  event for every repeat.  Note that the Tab and shift-Tab keys are
  only passed to the widget if they are not used by the focus-change
  mechanisms.  To force those keys to be processed by your widget, you
  must reimplement QWidget::event().

  <li> wheelEvent() -- called whenever the user turns the mouse wheel
  while the widget has the focus.

  <li> enterEvent() - called when the mouse enters the widget's screen
  space.  (This excludes screen space owned by any children of the
  widget.)

  <li> leaveEvent() - called when the mouse leaves the widget's screen
  space.

  <li> moveEvent() - called when the widget has been moved relative to its
  parent.

  <li> closeEvent() - called when the user closes the widget (or when
  close() is called).
  </ul>

  There are also some \e really obscure events.  They are listed in
  qevent.h and you need to reimplement event() to handle them.  The
  default implementation of event() handles Tab and shift-Tab (to move
  the keyboard focus), and passes on most other events to one of the
  more specialized handlers above.

  When writing a widget, there are a few more things to look out
  for. <ul>

  <li> In the constructor, be sure to set up your member variables
  early on, before there's any chance that you might receive an event.

  <li>It is almost always useful to reimplement sizeHint() and to set
  the correct size policy with setSizePolicy(), so users of your class
  can set up layout management more easily.  A size policy lets you
  supply good defaults for the layout management handling, so that
  other widgets can contain and manage yours easily.  sizeHint()
  indicates a "good" size for the widget.

  <li>If your widget is a top-level window, setCaption() and setIcon() set
  the title bar and icon respectively.

  </ul>

  \sa QEvent, QPainter, QGridLayout, QBoxLayout
*/


/*****************************************************************************
  Internal QWidgetMapper class

  The purpose of this class is to map widget identifiers to QWidget objects.
  All QWidget objects register themselves in the QWidgetMapper when they
  get an identifier. Widgets unregister themselves when they change ident-
  ifier or when they are destroyed. A widget identifier is really a window
  handle.

  The widget mapper is created and destroyed by the main application routines
  in the file qapp_xxx.cpp.
 *****************************************************************************/

#ifdef _WS_QWS_
static const int WDictSize = 163; // plenty for small devices
#else
static const int WDictSize = 1123; // plenty for 5 big complex windows
#endif

class QWidgetMapper : public QWidgetIntDict
{						// maps ids -> widgets
public:
    QWidgetMapper();
   ~QWidgetMapper();
    QWidget *find( WId id );			// find widget
    void     insert( const QWidget * );		// insert widget
    bool     remove( WId id );			// remove widget
private:
    WId	     cur_id;
    QWidget *cur_widget;
};

QWidgetMapper *QWidget::mapper = 0;		// app global widget mapper


QWidgetMapper::QWidgetMapper() : QWidgetIntDict(WDictSize)
{
    cur_id = 0;
    cur_widget = 0;
}

QWidgetMapper::~QWidgetMapper()
{
    clear();
}

inline QWidget *QWidgetMapper::find( WId id )
{
    if ( id != cur_id ) {			// need to lookup
	cur_widget = QWidgetIntDict::find((long)id);
	if ( cur_widget )
	    cur_id = id;
	else
	    cur_id = 0;
    }
    return cur_widget;
}

inline void QWidgetMapper::insert( const QWidget *widget )
{
#ifdef _OS_MAC_
    qDebug("Insert for %d\n",(int)widget->winId());
#endif
    QWidgetIntDict::insert((long)widget->winId(),widget);
}

inline bool QWidgetMapper::remove( WId id )
{
    if ( cur_id == id ) {			// reset current widget
	cur_id = 0;
	cur_widget = 0;
    }
#ifdef _OS_MAC_
    qDebug("Remove for %d\n",(int)id);
#endif
    return QWidgetIntDict::remove((long)id);
}


/*****************************************************************************
  QWidget member functions
 *****************************************************************************/

/*
    Widget state flags:
  <dl compact>
  <dt>WState_Created<dd> The widget has a valid winId().
  <dt>WState_Disabled<dd> The widget does not receive any mouse
       or keyboard events.
  <dt>WState_ForceDisabled<dd> The widget is explicitly disabled, i.e. it will remain
	disabled even when all its ancestors are set to enabled
	state. This implies WState_Disabled.
  <dt>WState_Visible<dd> The widget is currently visible.
  <dt>WState_ForceHide<dd> The widget is explicitly hidden, i.e. it won't become
  	visible unless you call show() on it.  ForceHide implies !WState_Visible
  <dt>WState_OwnCursor<dd> A cursor has been set for this widget.
  <dt>WState_MouseTracking<dd> Mouse tracking is enabled.
  <dt>WState_CompressKeys<dd> Compress keyboard events.
  <dt>WState_BlockUpdates<dd> Repaints and updates are disabled.
  <dt>WState_InPaintEvent<dd> Currently processing a paint event.
  <dt>WState_Reparented<dd> The widget has been reparented.
  <dt>WState_ConfigPending<dd> A config (resize/move) event is pending.
  <dt>WState_Resized<dd> The widget has been resized.
  <dt>WState_AutoMask<dd> The widget has an automatic mask, see setAutoMask().
  <dt>WState_Polished<dd> The widget has been "polished" (i.e. late initializated ) by a QStyle.
  <dt>WState_DND<dd> The widget supports drag and drop, see setAcceptDrops().
  <dt>WState_Modal<dd> Only for WType_Modal. Defines whether the widget is
        actually performing  modality when shown. Modality can be switched on/off with
        this flag.
  <dt> WState_Exposed<dd> the widget was finally exposed (x11 only,
        helps avoiding paint event doubling).
  <dt>WState_TranslateBackground<dd> The widget tanslates its pixmap
        background to the parent's coordinate system, see setTranslateBackground().
  </dl>
*/


/*! \enum Qt::WidgetFlags

<a name="widgetflags"></a>

This enum type is used to specify various window-system properties
of the widget.  Mostly they are fairly unusual, but necessary in a
few cases.

The main types are <ul>

<li> \c WType_TopLevel - indicates that this widget is a top-level
widget, usually with a window-system frame and so on.

<li> \c WType_Modal - indicates that this widget is a modal top-level
widget, ie. that it prevents widgets in all other top-level widget
from getting any input.  \c WType_Modal inplies \c WStyle_Dialog.

<li> \c WType_Popup - indicates that this widget is a popup top-level
window, ie. that it is modal, but has a window system frame
appropriate for popup menus.

<li> \c WType_Desktop - indicates that this widget is the desktop.
See also \c WPaintDesktop below.

</ul> There are also a number of flags to let you customize the
appearance of top-level windows.  These have no effect on other
windows.<ul>

<li> \c WStyle_Customize - indicates that instead of the default, the
WStyle_* flags should be used to build the window.

<li> \c WStyle_NormalBorder - gives the window a normal border. Cannot
be combined with \c WStyle_DialogBorder or \c WStyle_NoBorder.

<li> \c WStyle_DialogBorder - gives the window a thin dialog border.
Cannot be combined with \c WStyle_NormalBorder or \c WStyle_NoBorder.

<li> \c WStyle_NoBorder - gives a borderless window.  Note that the
user cannot move or resize a borderless window via the window system.
Cannot be combined with \c WStyle_NormalBorder or \c
WStyle_DialogBorder. On Windows, the flag works fine. On X11, it
bypasses the window manager completely. This results in a borderless
window, but also in a window that is not managed at all (i.e. for
example no keyboard focus unless you call setActiveWindow()
manually. ) For compatibility, the flag was not changed for Qt-2.1. We
suggest using WStyle_NoBorderEx instead.

<li> \c WStyle_NoBorderEx - gives a borderless window.  Note that the
user cannot move or resize a borderless window via the window system.
Cannot be combined with \c WStyle_NormalBorder or \c
WStyle_DialogBorder. On X11, the result of the flag is depending on
the window manager and its ability to understand MOTIF hints to some
degree.  Most existing modern window managers do this. With \c
WX11BypassWM, you can bypass the window manager completely. This
results in a borderless window for sure, but also in a window that is
not managed at all (i.e. for example no keyboard input unless you call
setActiveWindow() manually )

<li> \c WStyle_Title - gives the window a title bar.

<li> \c WStyle_SysMenu - adds a window system menu.

<li> \c WStyle_Minimize - adds a minimize button.  Note that on Windows
this has to be combined with WStyle_SysMenu for it to work.

<li> \c WStyle_Maximize - adds a maximize button.  Note that on Windows
this has to be combined with WStyle_SysMenu for it to work.

<li> \c WStyle_MinMax - is equal to \c WStyle_Minimize|WStyle_Maximize.
Note that on Windows this has to be combined with WStyle_SysMenu for
it to work.

<li> \c WStyle_ContextHelp - adds a context help button to dialogs.

<li> \c WStyle_Tool - makes the window a tool window.  A tool window
is a small window that lives for a short time and it is typically used
for creating popup windows.  It there is a parent, the tool window
will always be kept on top of it.  If there isn't a parent, you may
consider passing WStyle_StaysOnTop as well.  If the window system
supports it, a tool window is be decorated with a somewhat lighter
frame.  It can, however, be combined with \c WStyle_NoBorder as well.

<li> \c WStyle_StaysOnTop - informs the window system that the window
should stay on top of all other windows.

<li> \c WStyle_Dialog - indicates that the window is a logical
subwindow of its parent, in other words: a dialog.  The window will
not get its own taskbar entry and be kept on top of its parent by
the window system.  Usually, it will also be minimized when the
parent is minimized.  If not customized, the window is decorated
with a slightly simpler title bar.  This is the flag QDialog uses.

</ul> Finally, there are some modifier flags: <ul>

<li> \c WDestructiveClose - makes Qt delete this object when the object has
accepted closeEvent(), or when the widget tried to ignore closeEvent() but
could not.

<li> \c WPaintDesktop - gives this widget paint events for the desktop.

<li> \c WPaintUnclipped - makes all painters operating on this widget
unclipped.  Children of this widget, or other widgets in front of it,
do not clip the area the painter can paint on.

<li> \c WPaintClever - indicates that Qt should not try to optimize
repainting for the widget, but instead pass on window system repaint
events directly.  (This tends to produce more events and smaller
repaint regions.)

<li> \c WResizeNoErase - indicates that resizing the widget should not
erase it. This allows smart-repainting to avoid flicker.

<li> \c WMouseNoMask - indicates that even if the widget has a mask,
it wants mouse events for its entire rectangle.

<li> \c WNorthWestGravity - indicates that the widget contents is
north-west aligned and static. On resize, such a widget will receive
paint events only for the newly visible part of itself.

<li> \c WRepaintNoErase - indicates that the widget paints all its
pixels.  Updating, scrolling and focus changes should therefore not
erase the widget.  This allows smart-repainting to avoid flicker.

<li> \c WGroupLeader - makes this widget or window a group
leader. Modality of secondary windows only affects windows within the
same group.

</ul>

*/


/*!
  Constructs a widget which is a child of \a parent, with the name \a name and
  widget flags set to \a f.

  If \a parent is 0, the new widget becomes a top-level window.  If \a
  parent is another widget, this widget becomes a child window inside
  \a parent.  The new widget is deleted when \a parent is.

  The \a name is sent to the QObject constructor.

  The widget flags argument \a f is normally 0, but it can be set to
  customize the window frame of a top-level widget (i.e. \a parent must be
  zero). To customize the frame, set the \c WStyle_Customize flag OR'ed with
  any of the Qt::WidgetFlags.

  Note that the X11 version of Qt may not be able to deliver all
  combinations of style flags on all systems.  This is because on X11,
  Qt can only ask the window manager, and the window manager can
  override the application's settings.  On Windows, Qt can set
  whatever flags you want.

  Example:
  \code
    QLabel *spashScreen = new QLabel( 0, "mySplashScreen",
				  WStyle_Customize | WStyle_NoBorder |
				  WStyle_Tool );
  \endcode
*/

QWidget::QWidget( QWidget *parent, const char *name, WFlags f )
    : QObject( parent, name ), QPaintDevice( QInternal::Widget )
{
    isWidget = TRUE;				// is a widget
    winid = 0;					// default attributes
    widget_state = 0;
    widget_flags = f;
    focus_policy = 0;
    own_font = 0;
    own_palette = 0;
    sizehint_forced = 0;
    is_closing = 0;
    in_show = 0;
#ifndef QT_NO_LAYOUT
    lay_out = 0;
#endif
    extra = 0;					// no extra widget info
#ifndef QT_NO_PALETTE
    bg_col = pal.normal().background();		// default background color
#endif
    create();					// platform-dependent init
#ifndef QT_NO_PALETTE
    pal = isTopLevel() ? QApplication::palette() : parentWidget()->palette();
#endif
    fnt = isTopLevel() ? QApplication::font() : parentWidget()->font();

    if ( !isDesktop() )
	setBackgroundFromMode(); //### parts of this are done in create but not all (see reparent(...) )
    // make sure move/resize events are sent to all widgets
    QApplication::postEvent( this, new QMoveEvent( fpos, fpos ) );
    QApplication::postEvent( this, new QResizeEvent(crect.size(),
						    crect.size()) );
    if ( isTopLevel() ) {
	setWState( WState_ForceHide );
	if ( testWFlags( WType_Modal ) )
	    setWState( WState_Modal ); // default for modal windows is to be modal
	QFocusData *fd = focusData( TRUE );
	if ( fd->focusWidgets.findRef(this) < 0 )
	    fd->focusWidgets.append( this );
    } else {
	if ( !parentWidget()->isEnabled() )
	    setWState( WState_Disabled ); 	// propagate enabled state
   	if ( parentWidget()->isVisibleTo( 0 ) )
 	    setWState( WState_ForceHide );	// new widgets do not show up in already visible parents
    }
}

static bool noMoreToplevels();

/*!
  Destructs the widget.

  All children of this widget are deleted first.
  The application exits if this widget is (was) the main widget.
*/

QWidget::~QWidget()
{
#if defined (CHECK_STATE)
    if ( paintingActive() )
	qWarning( "%s (%s): deleted while being painted", className(), name() );
#endif

    // Remove myself and all children from the can-take-focus list
    QFocusData *f = focusData( FALSE );
    if ( f ) {
	QListIterator<QWidget> it(f->focusWidgets);
	QWidget *w;
	while ( (w = it.current()) ) {
	    ++it;
	    QWidget * p = w;
	    while( p && p != this )
		p = p->parentWidget();
	    if ( p ) // my descendant
		f->focusWidgets.removeRef( w );
	}
    }

    if ( QApplication::main_widget == this ) {	// reset main widget
	QApplication::main_widget = 0;
	if (qApp)
	    qApp->quit();
    }

    if ( focusWidget() == this )
	clearFocus();
    if ( QApplication::focus_widget == this )
	QApplication::focus_widget = 0;

    if ( isTopLevel() && !isHidden() && winId() )
	hide();

    // A parent widget must destroy all its children before destroying itself
    if ( childObjects ) {			// delete children objects
	QObjectListIt it(*childObjects);
	QObject *obj;
	while ( (obj=it.current()) ) {
	    ++it;
	    obj->parentObj = 0;
	    // ### nest line is a QGList workaround - remove in 3.0
	    childObjects->removeRef( obj );
	    delete obj;
	}
	delete childObjects;
	childObjects = 0;
    }

    QApplication::removePostedEvents( this );
    if ( extra )
	deleteExtra();
    destroy();					// platform-dependent cleanup
}


/*!
  \internal
  Creates the global widget mapper.
  The widget mapper converts window handles to widget pointers.
  \sa destroyMapper()
*/

void QWidget::createMapper()
{
    mapper = new QWidgetMapper;
    CHECK_PTR( mapper );
}

/*!
  \internal
  Destroys the global widget mapper.
  \sa createMapper()
*/

void QWidget::destroyMapper()
{
    if ( !mapper )				// already gone
	return;
    QWidgetIntDictIt it( *((QWidgetIntDict*)mapper) );
    QWidgetMapper * myMapper = mapper;
    mapper = 0;
    register QWidget *w;
    while ( (w=it.current()) ) {		// remove parents widgets
	++it;
	if ( !w->parentObj )			// widget is a parent
	    w->destroy( TRUE, TRUE );
    }
    delete myMapper;
}


static QWidgetList *wListInternal( QWidgetMapper *mapper, bool onlyTopLevel )
{
    QWidgetList *list = new QWidgetList;
    CHECK_PTR( list );
    if ( mapper ) {
	QWidget *w;
	QWidgetIntDictIt it( *((QWidgetIntDict*)mapper) );
	while ( (w=it.current()) ) {
	    ++it;
	    if ( !onlyTopLevel || w->isTopLevel() )
		list->append( w );
	}
    }
    return list;
}

/*!
  \internal
  Returns a list of all widgets.
  \sa tlwList(), QApplication::allWidgets()
*/

QWidgetList *QWidget::wList()
{
    return wListInternal( mapper, FALSE );
}

/*!
  \internal
  Returns a list of all top level widgets.
  \sa wList(), QApplication::topLevelWidgets()
*/

QWidgetList *QWidget::tlwList()
{
    return wListInternal( mapper, TRUE );
}


void QWidget::setWinId( WId id )		// set widget identifier
{
    if ( !mapper )				// mapper destroyed
	return;
    if ( winid )
	mapper->remove( winid );
    winid = id;
#if defined(_WS_X11_)
    hd = id;					// X11: hd == ident
#endif
    if ( id )
	mapper->insert( this );
}


/*!
  \internal
  Returns a pointer to the block of extra widget data.
*/

QWExtra *QWidget::extraData()
{
    return extra;
}


/*!
  \internal
  Returns a pointer to the block of extra toplevel widget data.

  This data is guaranteed to exist for toplevel widgets.
*/

QTLWExtra *QWidget::topData()
{
    createTLExtra();
    return extra->topextra;
}


void QWidget::createTLExtra()
{
    if ( !extra )
	createExtra();
    if ( !extra->topextra ) {
	QTLWExtra* x = extra->topextra = new QTLWExtra;
	x->icon = 0;
	x->focusData = 0;
	x->fsize = crect.size();
	x->incw = x->inch = 0;
	x->basew = x->baseh = 0;
	x->iconic = 0;
	x->fullscreen = 0;
	x->showMode = 0;
	x->normalGeometry = QRect(0,0,-1,-1);
#if defined(_WS_X11_)
	x->embedded = 0;
	x->parentWinId = 0;
	x->dnd = 0;
	x->uspos = 0;
	x->ussize = 0;
#endif
#if defined(_WS_QWS_) && !defined(QT_NO_QWS_MANAGER)
	x->decor_allocated_region = QRegion();
	x->qwsManager = 0;
#endif
	createTLSysExtra();
    }
}

/*!
  \internal
  Creates the widget extra data.
*/

void QWidget::createExtra()
{
    if ( !extra ) {				// if not exists
	extra = new QWExtra;
	CHECK_PTR( extra );
	extra->minw = extra->minh = 0;
	extra->maxw = extra->maxh = QWIDGETSIZE_MAX;
	extra->bg_pix = 0;
	extra->focus_proxy = 0;
#ifndef QT_NO_CURSOR
	extra->curs = 0;
#endif
	extra->topextra = 0;
	extra->bg_mode = PaletteBackground;
#ifndef QT_NO_STYLE
	extra->style = 0;
#endif
	extra->size_policy = QSizePolicy( QSizePolicy::Preferred,
					  QSizePolicy::Preferred );
	extra->posted_events = 0;
	createSysExtra();
    }
}


/*!
  \internal
  Deletes the widget extra data.
*/

void QWidget::deleteExtra()
{
    if ( extra ) {				// if exists
	delete extra->bg_pix;
#ifndef QT_NO_CURSOR
	delete extra->curs;
#endif
	deleteSysExtra();
	if ( extra->topextra ) {
	    deleteTLSysExtra();
	    delete extra->topextra->icon;
	    delete extra->topextra->focusData;
#if defined(_WS_QWS_) && !defined(QT_NO_QWS_MANAGER)
	    delete extra->topextra->qwsManager;
#endif
	    delete extra->topextra;
	}
#if defined(DEUBG)
	if ( extra->posted_events )
	    qWarning( "QWidget::deleteExtra: Memory leak for "
		      "extra->posted_events (%p)", extra->posted_events );
	// removePostedEvents( this ) must be called before deleteExtra()
#endif
	delete extra;
	// extra->xic destroyed in QWidget::destroy()
	extra = 0;
    }
}


/*!
  \internal
  This function is called when a widget is hidden or destroyed.
  It resets some application global pointers that should only refer active,
  visible widgets.
*/

void QWidget::deactivateWidgetCleanup()
{
    extern QWidget *qt_button_down;
    // If this was the active application window, reset it
    if ( this == QApplication::active_window )
	QApplication::active_window = 0;
    // If the is the active mouse press widget, reset it
    if ( qt_button_down == this )
	qt_button_down = 0;
}


/*!
  Returns a pointer to the widget with window identifer/handle \a id.

  The window identifier type depends by the underlying window system,
  see qwindowdefs.h for the actual definition.
  If there is no widget with this identifier, a null pointer is returned.

  \sa wmapper(), id()
*/

QWidget *QWidget::find( WId id )
{
    return mapper ? mapper->find( id ) : 0;
}

/*!
  \fn QWidgetMapper *QWidget::wmapper()
  \internal
  Returns a pointer to the widget mapper.

  The widget mapper is an internal dictionary that is used to map from
  window identifiers/handles to widget pointers.
  \sa find(), id()
*/

/*!
  \fn WFlags QWidget::getWFlags() const

  Returns the widget flags for this this widget.

  Widget flags are a combination of Qt::WidgetFlags.

  \sa testWFlags(), setWFlags(), clearWFlags()
*/

/*!
  \fn void QWidget::setWFlags( WFlags f )

  Sets the widget flags \a f.

  Widget flags are a combination of Qt::WidgetFlags.

  \sa testWFlags(), getWFlags(), clearWFlags()
*/

/*!
  \fn void QWidget::clearWFlags( WFlags f )

  Clears the widget flags \a f.

  Widget flags are a combination of Qt::WidgetFlags.

  \sa testWFlags(), getWFlags(), setWFlags()
*/



/*!
  \fn WId QWidget::winId() const

  Returns the window system identifier of the widget.

  Portable in principle, but if you use it you are probably about to do
  something non-portable. Be careful.

  \sa find()
*/

#ifndef QT_NO_STYLE
/*!
  Returns the GUI style for this widget

  \sa QWidget::setStyle(), QApplication::setStyle(),
  QApplication::style()
*/

QStyle& QWidget::style() const
{
    if ( extra && extra->style )
	return *extra->style;
    return qApp->style();
}


/*!
  \fn void QWidget::styleChange( QStyle& oldStyle )

  This virtual function is called when the style of the widgets.
  changes.\a oldStyle is the
  previous GUI style; you can get the new style from style().

  Reimplement this function if your widget needs to know when its GUI
  style changes.  You will almost certainly need to update the widget
  using update().

  The default implementation updates the widget including its
  geometry.

  \sa QApplication::setStyle(), style(), update(), updateGeometry()
*/

void QWidget::styleChange( QStyle& )
{
    update();
    updateGeometry();
}
#endif

/*!
  \fn bool QWidget::isTopLevel() const
  Returns TRUE if the widget is a top-level widget, otherwise FALSE.

  A top-level widget is a widget which usually has a frame and a \link
  setCaption() caption\endlink (title bar). \link isPopup() Popup\endlink
  and \link isDesktop() desktop\endlink widgets are also top-level
  widgets.

  A top-level widgets can have a \link parentWidget() parent
  widget\endlink. It will then be grouped with its parent: deleted
  when the parent is deleted, minimized when the parent is minimized
  etc. If supported by the window manager, it will also have a common
  taskbar entry with its parent.

  QDialog and QMainWindow widgets are by default top-level, even if a
  parent widget is specified in the constructor. This behavior is
  specified by the \c WType_TopLevel widget flag.

  Child widgets are the opposite of top-level widgets.

  \sa topLevelWidget(), isModal(), isPopup(), isDesktop(), parentWidget()
*/

/*!
  \fn bool QWidget::isModal() const
  Returns TRUE if the widget is a modal widget, otherwise FALSE.

  A modal widget is also a top-level widget.

  \sa isTopLevel(), QDialog
*/

/*!
  \fn bool QWidget::isPopup() const
  Returns TRUE if the widget is a popup widget, otherwise FALSE.

  A popup widget is created by specifying the widget flag \c WType_Popup
  to the widget constructor.

  A popup widget is also a top-level widget.

  \sa isTopLevel()
*/


/*!
  \fn bool QWidget::isDesktop() const
  Returns TRUE if the widget is a desktop widget, otherwise FALSE.

  A desktop widget is also a top-level widget.

  \sa isTopLevel(), QApplication::desktop()
*/


/*!
  \fn bool QWidget::isEnabled() const
  Returns TRUE if the widget is enabled, or FALSE if it is disabled.
  \sa setEnabled()
*/


/*!
  Returns TRUE if this widget would become enabled if \a ancestor is
  enabled.

  This is the case if neither the widget itself nor every parent up to
  but excluding \a ancestor has been explicitly disabled.

  isEnabledTo(0) is equivalent to isEnabled().

  \sa setEnabled() isEnabled()
*/

bool QWidget::isEnabledTo(QWidget* ancestor) const
{
    const QWidget * w = this;
    while ( w && !w->testWFlags( WState_ForceDisabled )
	    && !w->isTopLevel()
	    && w->parentWidget()
	    && w->parentWidget()!=ancestor
	)
	w = w->parentWidget();
    return !w->testWFlags( WState_ForceDisabled );
}


/*!\obsolete

  This function is deprecated. It is equivalent to isEnabled()

  \sa setEnabled() isEnabled()
*/

bool QWidget::isEnabledToTLW() const
{
    return isEnabled();
}


/*!
  Enables widget input events if \a enable is TRUE, otherwise disables
  input events.

  An enabled widget receives keyboard and mouse events; a disabled
  widget does not.  Note that an enabled widget receives keyboard
  events only when it is in focus.

  Some widgets display themselves differently when they are disabled.
  For example a button might draw its label grayed out. If your widget
  needs to know when it becomes enabled or disabled, you can
  reimplement the enabledChange() function.

  Disabling a widget implicitely disables all its children.  Enabling
  respectively enables all child widgets unless they have been
  explicitly disabled.

  \sa isEnabled(), isEnabledTo(), QKeyEvent, QMouseEvent, enabledChange()
*/

void QWidget::setEnabled( bool enable )
{
    if ( enable )
	clearWState( WState_ForceDisabled );
    else
	setWState( WState_ForceDisabled );

    if ( !isTopLevel() && !parentWidget()->isEnabled() && enable )
	return; // nothing we can do

    if ( enable ) {
	if ( testWState(WState_Disabled) ) {
	    clearWState( WState_Disabled );
	    setBackgroundFromMode();
	    enabledChange( TRUE );
	    if ( children() ) {
		QObjectListIt it( *children() );
		QWidget *w;
		while( (w=(QWidget *)it.current()) != 0 ) {
		    ++it;
		    if ( w->isWidgetType() && !w->testWState( WState_ForceDisabled ) )
			w->setEnabled( TRUE );
		}
	    }
	}
    } else {
	if ( !testWState(WState_Disabled) ) {
	    if ( focusWidget() == this )
		focusNextPrevChild( TRUE );
	    setWState( WState_Disabled );
	    setBackgroundFromMode();
	    enabledChange( FALSE );
	    if ( children() ) {
		QObjectListIt it( *children() );
		QWidget *w;
		while( (w=(QWidget *)it.current()) != 0 ) {
		    ++it;
		    if ( w->isWidgetType() && w->isEnabled() ) {
			w->setEnabled( FALSE );
			w->clearWState( WState_ForceDisabled );
		    }
		}
	    }
	}
    }
}

/*!
  Disables widget input events if \a disable is TRUE, otherwise enables
  input events.

  An enabled widget receives keyboard and mouse events; a disabled
  widget does not.  Note that an enabled widget receives keyboard
  events only when it is in focus.

  Some widgets display themselves differently when they are disabled.
  For example a button might draw its label grayed out. If your widget
  needs to know when it becomes enabled or disabled, you can
  reimplement the enabledChange() function.

  Disabling a widget implicitely disables all its children.  Enabling
  respectively enables all child widgets unless they have been
  explicitly disabled.

  \sa setEnabled(), isEnabled(), isEnabledTo(), QKeyEvent, QMouseEvent, enabledChange()
*/
void QWidget::setDisabled( bool disable )
{
    setEnabled( !disable );
}

/*!
  \fn void QWidget::enabledChange( bool oldEnabled )

  This virtual function is called from setEnabled(). \a oldEnabled is the
  previous setting; you can get the new setting from isEnabled().

  Reimplement this function if your widget needs to know when it becomes
  enabled or disabled. You will almost certainly need to update the widget
  using update().

  The default implementation repaints the visible part of the widget.

  \sa setEnabled(), isEnabled(), repaint(), update(), visibleRect()
*/

void QWidget::enabledChange( bool )
{
    update();
}


/*!
  \fn QRect QWidget::frameGeometry() const
  Returns the geometry of the widget, relative to its parent and
  including the window frame.

  See the \link geometry.html Window Geometry documentation\endlink
  for an overview of geometry issues with top-level widgets.

  \sa geometry(), x(), y(), pos()
*/

QRect QWidget::frameGeometry() const
{
    return QRect(fpos,frameSize());
}



/*!
  \fn const QRect &QWidget::geometry() const
  Returns the geometry of the widget, relative to its parent widget
  and excluding the window frame.

  See the \link geometry.html Window Geometry documentation\endlink
  for an overview of geometry issues with top-level widgets.

  \sa frameGeometry(), size(), rect()
*/

/*!
  \fn int QWidget::x() const
  Returns the x coordinate of the widget, relative to its parent
  widget and including the window frame.

  See the \link geometry.html Window Geometry documentation\endlink
  for an overview of geometry issues with top-level widgets.

  \sa frameGeometry(), y(), pos()
*/

/*!
  \fn int QWidget::y() const
  Returns the y coordinate of the widget, relative to its parent
  widget and including the window frame.

  See the \link geometry.html Window Geometry documentation\endlink
  for an overview of geometry issues with top-level widgets.

  \sa frameGeometry(), x(), pos()
*/

/*!
  \fn QPoint QWidget::pos() const
  Returns the position of the widget in its parent widget, including
  the window frame.

  See the \link geometry.html Window Geometry documentation\endlink
  for an overview of geometry issues with top-level widgets.

  \sa move(), frameGeometry(), x(), y()
*/

/*!
  \fn QSize QWidget::size() const
  Returns the size of the widget, excluding the window frame.
  \sa geometry(), width(), height()
*/

/*!
  \fn int QWidget::width() const
  Returns the width of the widget, excluding the window frame.

  See the \link geometry.html Window Geometry documentation\endlink
  for an overview of geometry issues with top-level widgets.

  \sa geometry(), height(), size()
*/

/*!
  \fn int QWidget::height() const
  Returns the height of the widget, excluding the window frame.

  See the \link geometry.html Window Geometry documentation\endlink
  for an overview of geometry issues with top-level widgets.

  \sa geometry(), width(), size()
*/

/*!
  \fn QRect QWidget::rect() const
  Returns the the internal geometry of the widget, excluding the window frame.
  rect() equals QRect(0,0,width(),height()).

  See the \link geometry.html Window Geometry documentation\endlink
  for an overview of geometry issues with top-level widgets.

  \sa size()
*/


/*!
  Returns the bounding rectangle of the widget's children.

  Explicitely hidden children are excluded.

  \sa childrenRegion()
*/

QRect QWidget::childrenRect() const
{
    QRect r( 0, 0, 0, 0 );
    if ( !children() )
	return r;
    QObjectListIt it( *children() );		// iterate over all children
    QObject *obj;
    while ( (obj=it.current()) ) {
	++it;
	if ( obj->isWidgetType() && !((QWidget*)obj)->isHidden() )
	    r = r.unite( ((QWidget*)obj)->geometry() );
    }
    return r;
}

/*!
  Returns the combined region of the widget's children geometry().

  Explicitely hidden children are excluded.

  \sa childrenRect()
*/

QRegion QWidget::childrenRegion() const
{
    QRegion r;
    if ( !children() )
	return r;
    QObjectListIt it( *children() );		// iterate over all children
    QObject *obj;
    while ( (obj=it.current()) ) {
	++it;
	if ( obj->isWidgetType() && !((QWidget*)obj)->isHidden() )
	    r = r.unite( ((QWidget*)obj)->geometry() );
    }
    return r;
}


/*!
  Returns the minimum widget size.

  The widget cannot be resized to a smaller size than the minimum widget
  size.

  If the returned minimum size equals (0,0) then it means that there are
  no constraints on the minimum size. However, Qt does nevertheless not
  allow you to shrink widgets to less than 1 pixel width/height.

  \sa maximumWidth(), maximumHeight(), setMinimumSize(),
  maximumSize(), sizeIncrement()
*/

QSize QWidget::minimumSize() const
{
    return extra ? QSize(extra->minw,extra->minh) : QSize(0,0);
}

/*!
  Returns the maximum widget size.

  The widget cannot be resized to a larger size than the maximum widget
  size.

  \sa maximumWidth(), maximumHeight(), setMaximumSize(),
  minimumSize(), sizeIncrement()
*/

QSize QWidget::maximumSize() const
{
    return extra ? QSize(extra->maxw,extra->maxh)
		 : QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
}


/*!
  \fn int QWidget::minimumWidth() const

  Returns the widget's minimum width.

  \sa minimumSize(), minimumHeight()
*/

/*!
  \fn int QWidget::minimumHeight() const

  Returns the widget's minimum height.

  \sa minimumSize(), minimumWidth()
*/

/*!
  \fn int QWidget::maximumWidth() const

  Returns the widget's maximum width.

  \sa maximumSize(), maximumHeight()
*/

/*!
  \fn int QWidget::maximumHeight() const

  Returns the widget's maximum height.

  \sa maximumSize(), maximumWidth()
*/


/*!
  Returns the widget size increment.

  \sa setSizeIncrement(), minimumSize(), maximumSize()
*/

QSize QWidget::sizeIncrement() const
{
    return extra && extra->topextra
	? QSize(extra->topextra->incw,extra->topextra->inch)
	: QSize(0,0);
}

/*!
  Returns the widget base size

  The base size is used to calculate a proper widget size in case the
  widget defines sizeIncrement().

  \sa setBaseSize(), setSizeIncrement()
*/

QSize QWidget::baseSize() const
{
    return extra && extra->topextra
	? QSize(extra->topextra->basew,extra->topextra->baseh)
	: QSize(0,0);
}


/*!
  Sets both the minimum and maximum sizes of the widget to \a s,
  thereby preventing it from ever growing or shrinking.

  \sa setMaximumSize() setMinimumSize()
*/

void QWidget::setFixedSize( const QSize & s)
{
    setMinimumSize( s );
    setMaximumSize( s );
    resize( s );
}


/*!
  \overload void QWidget::setFixedSize( int w, int h )
*/

void QWidget::setFixedSize( int w, int h )
{
    setMinimumSize( w, h );
    setMaximumSize( w, h );
    resize( w, h );
}


/*!
  Sets the minimum width of the widget to \a w without changing the
  height.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize()
  setFixedSize() and more
*/

void QWidget::setMinimumWidth( int w )
{
    setMinimumSize( w, minimumSize().height() );
}


/*!
  Sets the minimum height of the widget to \a h without changing the
  width.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setMinimumHeight( int h )
{
    setMinimumSize( minimumSize().width(), h );
}


/*!
  Sets the maximum width of the widget to \a w without changing the
  height.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setMaximumWidth( int w )
{
    setMaximumSize( w, maximumSize().height() );
}


/*!
  Sets the maximum height of the widget to \a h without changing the
  width.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setMaximumHeight( int h )
{
    setMaximumSize( maximumSize().width(), h );
}


/*!
  Sets both the minimum and maximum width of the widget to \a w
  without changing the heights.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setFixedWidth( int w )
{
    setMinimumSize( w, minimumSize().height() );
    setMaximumSize( w, maximumSize().height() );
}


/*!
  Sets both the minimum and maximum heights of the widget to \a h
  without changing the widths.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setFixedHeight( int h )
{
    setMinimumSize( minimumSize().width(), h );
    setMaximumSize( maximumSize().width(), h );
}


/*! Translates the widget coordinate \a pos to the coordinate system
  of \a parent, which must be non-null and be a parent widget of this.

  \sa mapFrom() mapToParent() mapToGlobal()
*/

QPoint QWidget::mapTo( QWidget * parent, const QPoint & pos ) const
{
    QPoint p = pos;
    if ( parent ) {
	const QWidget * w = this;
	while ( w != parent ) {
	    p = w->mapToParent( p );
	    w = w->parentWidget();
	}
    }
    return p;
}


/*! Translates the widget coordinate \a pos from the coordinate system
  of \a parent to this widget's coordinate system, which must be non-null
  and be a parent widget of this.

  \sa mapTo() mapFromParent() mapFromGlobal()
*/

QPoint QWidget::mapFrom( QWidget * parent, const QPoint & pos ) const
{
    QPoint p( pos );
    if ( parent ) {
	const QWidget * w = this;
	while ( w != parent ) {
	    p = w->mapFromParent( p );
	    w = w->parentWidget();
	}
    }
    return p;
}


/*!
  Translates the widget coordinate \a pos to a coordinate in the parent widget.

  Same as mapToGlobal() if the widget has no parent.

  \sa mapFromParent() mapTo() mapToGlobal()
*/

QPoint QWidget::mapToParent( const QPoint &pos ) const
{
    return pos + crect.topLeft();
}

/*!
  Translates the parent widget coordinate \a pos to widget coordinates.

  Same as mapFromGlobal() if the widget has no parent.

  \sa mapToParent() mapFrom() mapFromGlobal()
*/

QPoint QWidget::mapFromParent( const QPoint &pos ) const
{
    return pos - crect.topLeft();
}


/*!

  Returns the top-level widget for this widget, i.e. the next ancestor
  widget that has a window-system frame (or at least may have one).

  If the widget is a top-level, the widget itself is returned.

  Typical usage is changing the window caption:

  \code
    aWidget->topLevelWidget()->setCaption( "New Caption" );
  \endcode

  \sa isTopLevel()
*/

QWidget *QWidget::topLevelWidget() const
{
    QWidget *w = (QWidget *)this;
    QWidget *p = w->parentWidget();
    while ( !w->testWFlags(WType_TopLevel) && p ) {
	w = p;
	p = p->parentWidget();
    }
    return w;
}


// Please do NOT remove the FAQ answer from this doc again.  It's a
// FAQ, it remains a FAQ, and people apparently will not follow three
// links to find the right answer.

/*!
  Sets the widget to be cleared to the fixed color \a color before
  paintEvent() is called.

  Note that using this function is very often a mistake.  Here are the
  most common mistakes:

  If you want to set the background color of a widget to one of the
  "usual" colors, setBackgroundMode() is usually the best function.
  For example, man widgets that usually use white backgrounds (and
  black text on it) can use this:

  \code
    thatWidget->setBackgroundMode( QWidget::PaletteBase );
  \endcode

  If you want to change the color scheme of a widget, the setPalette()
  function is better suited.  Here is how to set \e thatWidget to use a
  light green (RGB value 80, 255, 80) as background color, with shades
  of green used for all the 3D effects:

  \code
    thatWidget->setPalette( QPalette( QColor(80, 255, 80) ) );
  \endcode

  A fixed background color sometimes is just the right thing, but if
  you use it, make sure that your application looks right when the
  desktop color scheme has been changed.  (On X11, a quick way to test
  is e.g. "./yourapp -bg paleblue".  On Windows, you have to use the
  control panel.)

  \sa setPalette(), QApplication::setPalette(), backgroundColor(),
      setBackgroundPixmap(), setBackgroundMode()
*/

void QWidget::setBackgroundColor( const QColor &color )
{
    setBackgroundModeDirect( FixedColor );
    setBackgroundColorDirect( color );
}


/*!
  Sets the background pixmap of the widget to \e pixmap.

  The background pixmap is tiled to cover the entire widget.  Note
  that some widgets do not work well with a background pixmap, for
  example QLineEdit.

  If \a pixmap is part of the widget's palette(), we recommend calling
  setBackgroundMode() instead.

  A fixed background pixmap sometimes is just the right thing, but if
  you use it, make sure that your application looks right when the
  desktop color scheme has been changed.  (On X11, a quick way to test
  is e.g. "./yourapp -bg paleblue".  On Windows, you have to use the
  control panel.)

  \sa setBackgroundMode(), backgroundPixmap(), backgroundPixmapChange(),
  setBackgroundColor()
*/

void QWidget::setBackgroundPixmap( const QPixmap &pixmap )
{ // This function is called with a null pixmap by setBackgroundEmpty().
    setBackgroundPixmapDirect( pixmap );
    setBackgroundModeDirect( FixedPixmap );

}

void QWidget::setBackgroundFromMode()
{
#ifndef QT_NO_PALETTE
    QColorGroup::ColorRole r = QColorGroup::Background;
    if ( extra ) {
	int i = (BackgroundMode)extra->bg_mode;
	if ( i == FixedColor || i == FixedPixmap || i == NoBackground ) {
	    // Mode is for fixed color, not one based on palette,
	    // so nothing to do.
	    return;
	}
	switch( i ) {
	case PaletteForeground:
	    r = QColorGroup::Foreground;
	    break;
	case PaletteButton:
	    r = QColorGroup::Button;
	    break;
	case PaletteLight:
	    r = QColorGroup::Light;
	    break;
	case PaletteMidlight:
	    r = QColorGroup::Midlight;
	    break;
	case PaletteDark:
	    r = QColorGroup::Dark;
	    break;
	case PaletteMid:
	    r = QColorGroup::Mid;
	    break;
	case PaletteText:
	    r = QColorGroup::Text;
	    break;
	case PaletteBrightText:
	    r = QColorGroup::BrightText;
	    break;
	case PaletteBase:
	    r = QColorGroup::Base;
	    break;
	case PaletteBackground:
	    r = QColorGroup::Background;
	    break;
	case PaletteShadow:
	    r = QColorGroup::Shadow;
	    break;
	case PaletteHighlight:
	    r = QColorGroup::Highlight;
	    break;
	case PaletteHighlightedText:
	    r = QColorGroup::HighlightedText;
	    break;
	case PaletteButtonText:
	    r = QColorGroup::ButtonText;
	    break;
	case X11ParentRelative:
#if defined(_WS_X11_)
	    setBackgroundX11Relative();
#endif
	    return;
	}
    }
    QPixmap * p = palette().active().brush( r ).pixmap();
    if ( p )
	setBackgroundPixmapDirect( *p );
    else
	setBackgroundColorDirect( palette().active().color( r ) );
#endif
}

/*!
  Returns the mode most recently set by setBackgroundMode().  The
  default is PaletteBackground.

  \sa BackgroundMode setBackgroundMode()
*/
QWidget::BackgroundMode QWidget::backgroundMode() const
{
    return extra ? (BackgroundMode)extra->bg_mode : PaletteBackground;
}


/*! \enum QWidget::BackgroundMode

  This enum describes how the background of a widget changes, as the
  widget's palette changes.

  The background is what the widget contains when paintEvent() is
  called.  To minimize flicker, this should be the most common color
  or pixmap in the widget.  For \c PaletteBackground, use
  colorGroup().brush( \c QColorGroup::Background ), and so on.  There
  are also three special values, listed at the end: <ul>

  <li> \c PaletteForeground
  <li> \c PaletteBackground
  <li> \c PaletteButton
  <li> \c PaletteLight
  <li> \c PaletteMidlight
  <li> \c PaletteDark
  <li> \c PaletteMid
  <li> \c PaletteText
  <li> \c PaletteBrightText
  <li> \c PaletteButtonText
  <li> \c PaletteBase
  <li> \c PaletteShadow
  <li> \c PaletteHighlight
  <li> \c PaletteHighlightedText
  <li> \c NoBackground - the widget is not cleared before paintEvent().
  If the widget's paint event always draws on all the pixels, using
  this mode can be both fast and flicker-free.
  <li> \c FixedColor - the widget is cleared to a fixed color,
  normally different from all the ones in the palette().  Set using
  setBackgroundColor().
  <li> \c FixedPixmap - the widget is cleared to a fixed pixmap,
  normally different from all the ones in the palette().  Set using
  setBackgroundPixmap().

  </ul>

  \c FixedColor and \c FixedPixmap sometimes are just the right
  thing, but if you use them, make sure that your application looks
  right when the desktop color scheme has been changed.  (On X11, a
  quick way to test is e.g. "./yourapp -bg paleblue".  On Windows, you
  have to use the control panel.)

  \sa setBackgroundMode() backgroundMode() setBackgroundPixmap()
  setBackgroundColor()
*/

/*!
  Tells the window system how to clear this widget when sending a
  paint event.  In other words, this decides how the widgets looks
  when paintEvent() is called.

  For most widgets the default (PaletteBackground, normally
  gray) suffices, but some need to use PaletteBase (the
  background color for text output, normally white) and a few need
  other colors.

  QListBox, which is "sunken" and uses the base color to contrast with
  its environment, does this:

  \code
    setBackgroundMode( PaletteBase );
  \endcode

  Note that two of the BackgroundMode values cannot be used with this
  function.  For \c FixedPixmap, call setBackgroundPixmap() instead,
  and for \c FixedColor, call setBackgroundColor().
*/

void QWidget::setBackgroundMode( BackgroundMode m )
{
    if ( m == NoBackground )
	setBackgroundEmpty();
    else if ( m == FixedColor || m == FixedPixmap ) {
	qWarning("May not pass FixedColor or FixedPixmap to setBackgroundMode()");
	return;
    }
    setBackgroundModeDirect(m);
}

/*!
  \internal
*/
void QWidget::setBackgroundModeDirect( BackgroundMode m )
{
    if (m==PaletteBackground && !extra) return;

    createExtra();
    if ((BackgroundMode)extra->bg_mode != m) {
	extra->bg_mode = m;
	setBackgroundFromMode();
    }
}


/*!
  \fn const QColor &QWidget::backgroundColor() const

  Returns the background color of this widget, which is normally set
  implicitly by setBackgroundMode(), but can also be set explicitly by
  setBackgroundColor().

  If there is a background pixmap (set using setBackgroundPixmap()),
  then the return value of this function is indeterminate.

  \sa setBackgroundColor(), foregroundColor(), colorGroup(), palette()
*/

/*!
  Returns the foreground color of this widget.

  The foreground color is also accessible as colorGroup().foreground().

  \sa backgroundColor(), colorGroup()
*/

const QColor &QWidget::foregroundColor() const
{
#ifndef QT_NO_PALETTE
    return colorGroup().foreground();
#else
    return black; //###
#endif
}


/*!
  \fn void QWidget::backgroundColorChange( const QColor &oldBackgroundColor )

  This virtual function is called from setBackgroundColor().
  \e oldBackgroundColor is the previous background color; you can get the new
  background color from backgroundColor().

  Reimplement this function if your widget needs to know when its
  background color changes.  You will almost certainly need to call
  this implementation of the function.

  \sa setBackgroundColor(), backgroundColor(), setPalette(), repaint(),
  update()
*/

void QWidget::backgroundColorChange( const QColor & )
{
    update();
}


/*!
  Returns the background pixmap if one has been set.  If the widget
  has backgroundMode() NoBackground, the return value is a pixmap for
  which QPixmao:isNull() is true.  If the widget has no pixmap is the
  background, the return value is a null pointer.

  \sa setBackgroundPixmap(), setBackgroundMode()
*/

const QPixmap *QWidget::backgroundPixmap() const
{
    return (extra && extra->bg_pix) ? extra->bg_pix : 0;
}


/*!
  \fn void QWidget::backgroundPixmapChange( const QPixmap & oldBackgroundPixmap )

  This virtual function is called from setBackgroundPixmap().
  \e oldBackgroundPixmap is the previous background pixmap; you can get the
  new background pixmap from backgroundPixmap().

  Reimplement this function if your widget needs to know when its
  background pixmap changes.  You will almost certainly need to call
  this implementation of the function.

  \sa setBackgroundPixmap(), backgroundPixmap(), repaint(), update()
*/

void QWidget::backgroundPixmapChange( const QPixmap & )
{
    update();
}


/*!
  Returns the current color group of the widget palette.

  The color group is determined by the state of the widget.

  A disabled widget returns the QPalette::disabled() color group, a
  widget in the window with keyboard focus returns the
  QPalette::active() color group, and all inactive  widgets return the
  QPalette::inactive() color group.

  \sa palette(), setPalette()
*/
#ifndef QT_NO_PALETTE
const QColorGroup &QWidget::colorGroup() const
{
    if ( !isEnabled() )
	return palette().disabled();
    else if ( isActiveWindow() )
	return palette().active();
    else
	return palette().inactive();
}
#endif
/*!
  \fn const QPalette &QWidget::palette() const
  Returns the widget palette.

  As long as no special palette has been set, this is either a special
  palette for the widget class, the parent's palette or - if this
  widget is a toplevel widget - the default application palette.

  \sa setPalette(), colorGroup(), QApplication::palette()
*/


/*! \enum QWidget::PropagationMode

  \obsolete

  This enum used to determine how fonts and palette changes are propagated to
  children of a widget.

*/

/*!
  Sets the widget palette to \e palette and informs all children about the change.

  \sa QApplication::setPalette(), palette(), paletteChange(), unsetPalette(), ownPalette()
  colorGroup()
*/
#ifndef QT_NO_PALETTE
void QWidget::setPalette( const QPalette &palette )
{
    own_palette = TRUE;
    if ( pal == palette )
	return;
    QPalette old = pal;
    pal = palette;
    setBackgroundFromMode();
    paletteChange( old );
    if ( children() ) {
	QEvent e( QEvent::ParentPaletteChange );
	QObjectListIt it( *children() );
	QWidget *w;
	while( (w=(QWidget *)it.current()) != 0 ) {
	    ++it;
	    if ( w->isWidgetType() )
		QApplication::sendEvent( w, &e );
	}
    }
    update();
}


/*!
  Unsets the palette for this widget. The widget will use its natural
  default palette from now on.

\sa setPalette(), ownPalette()
 */
void QWidget::unsetPalette()
{
    if ( own_palette ) {
	if ( !isTopLevel() && QApplication::palette( this ).isCopyOf( QApplication::palette() ) )
	    setPalette( parentWidget()->palette() );
	else
	    setPalette( QApplication::palette( this ) );
	own_palette = FALSE;
    }
}

/*!\obsolete

  Use setPalette( const QPalette& p ) instead.
*/

void QWidget::setPalette( const QPalette &p, bool )
{
    setPalette( p );
}

/*!
  \fn void QWidget::paletteChange( const QPalette &oldPalette )

  This virtual function is called from setPalette().  \e oldPalette is the
  previous palette; you can get the new palette from palette().

  Reimplement this function if your widget needs to know when its
  palette changes.  You will almost certainly need to call this
  implementation of the function.

  \sa setPalette(), palette()
*/

void QWidget::paletteChange( const QPalette & )
{
}
#endif // QT_NO_PALETTE

/*!
  \fn QFont QWidget::font() const

  Returns the font currently set for the widget.

  fontInfo() tells you what font is actually being used.

  As long as no special font has been set, this is either a special
  font for the widget class, the parent's font or - if this widget is
  a toplevel widget - the default application font.

  \sa setFont(), fontInfo(), fontMetrics(), QApplication::font()
*/


/*!
  Sets the font for the widget and informs all children about the
  change.

  The fontInfo() function reports the actual font that is being used by the
  widget.

  This code fragment sets a 12 point helvetica bold font:
  \code
    QFont f("Helvetica", 12, QFont::Bold);
    setFont( f );
  \endcode

  \sa font(), fontChange(), fontInfo(), fontMetrics(), unsetFont(), ownFont()
*/

void QWidget::setFont( const QFont &font )
{
    own_font = TRUE;
    if ( fnt == font )
	return;
    QFont old = fnt;
    fnt = font;
    fnt.handle();				// force load font
    fontChange( old );
    if ( children() ) {
	QEvent e( QEvent::ParentFontChange );
	QObjectListIt it( *children() );
	QWidget *w;
	while( (w=(QWidget *)it.current()) != 0 ) {
	    ++it;
	    if ( w->isWidgetType() )
		QApplication::sendEvent( w, &e );
	}
    }
    if ( hasFocus() )
	setFontSys();
}

/*!
  Unsets the font for this widget. The widget will use its natural
  default font from now on.  This is either a special font for the
  widget class, the parent's font or - if this widget is a toplevel
  widget - the default application font.

\sa setFont(), ownFont()
 */
void QWidget::unsetFont()
{
    if ( own_font ) {
	if ( !isTopLevel() && QApplication::font( this ).isCopyOf( QApplication::font() ) )
	    setFont( parentWidget()->font() );
	else
	    setFont( QApplication::font( this ) );
	own_font = FALSE;
    }
}

/*!\obsolete

  Use setFont( const QFont& font) instead.
*/

void QWidget::setFont( const QFont &font, bool )
{
    setFont( font );
}

/*!
  \fn void QWidget::fontChange( const QFont &oldFont )

  This virtual function is called from setFont().  \e oldFont is the
  previous font; you can get the new font from font().

  Reimplement this function if your widget needs to know when its font
  changes.  You will almost certainly need to update the widget using
  update().

  The default implementation updates the widget including its
  geometry.

  \sa setFont(), font(), update(), updateGeometry()
*/

void QWidget::fontChange( const QFont & )
{
    update();
    updateGeometry();
}


/*!
  \fn QFontMetrics QWidget::fontMetrics() const

  Returns the font metrics for the widget's current font.
  Equivalent to QFontMetrics(widget->font()).

  \sa font(), fontInfo(), setFont()
*/

/*!
  \fn QFontInfo QWidget::fontInfo() const

  Returns the font info for the widget's current font.
  Equivalent to QFontInto(widget->font()).

  \sa font(), fontMetrics(), setFont()
*/


/*!
  Returns the widget cursor. If no cursor has been set the parent
  widget's cursor is returned.
  \sa setCursor(), unsetCursor();
*/
#ifndef QT_NO_CURSOR
const QCursor &QWidget::cursor() const
{
    if ( testWState(WState_OwnCursor) )
	return (extra && extra->curs)
	    ? *extra->curs
	    : arrowCursor;
    else
	return isTopLevel() ? arrowCursor : parentWidget()->cursor();
}
#endif

/*!
  Returns the widget caption.  If no caption has been set (common for
  child widgets), this functions returns a null string.

  \sa setCaption(), icon(), iconText(), QString::isNull()
*/

QString QWidget::caption() const
{
    return extra && extra->topextra
	? extra->topextra->caption
	: QString::null;
}

/*!
  Returns the widget icon pixmap, or null if no icon has been set.

  \sa setIcon(), iconText(), caption()
*/

const QPixmap *QWidget::icon() const
{
    return extra && extra->topextra
	? extra->topextra->icon
	: 0;
}

/*!
  Returns the widget icon text.  If no icon text has been set (common for
  child widgets), this functions returns a null string.

  \sa setIconText(), icon(), caption(), QString::isNull()
*/

QString QWidget::iconText() const
{
    return extra && extra->topextra
	? extra->topextra->iconText
	: QString::null;
}


/*!
  \fn bool QWidget::hasMouseTracking() const

  Returns TRUE if mouse tracking is enabled for this widget, or FALSE
  if mouse tracking is disabled.

  \sa setMouseTracking()
*/

/*!
  \fn void QWidget::setMouseTracking( bool enable )

  Enables mouse tracking if \e enable is TRUE, or disables it if \e enable
  is FALSE.

  If mouse tracking is disabled (default), this widget only receives
  mouse move events when at least one mouse button is pressed down while
  the mouse is being moved.

  If mouse tracking is enabled, this widget receives mouse move events
  even if no buttons are pressed down.

  \sa hasMouseTracking(), mouseMoveEvent(),
    QApplication::setGlobalMouseTracking()
*/

#if !defined(_WS_X11_)
void QWidget::setMouseTracking( bool enable )
{
    if ( enable )
	setWState( WState_MouseTracking );
    else
	clearWState( WState_MouseTracking );
    return;
}
#endif // _WS_X11_


/*!  Sets this widget's focus proxy to \a w. If \a w is 0, this
  function resets this widget to not have any focus proxy.

  Some widgets, such as QComboBox, can "have focus," but create a
  child widget to actually handle the focus.  QComboBox, for example,
  creates a QLineEdit.

  setFocusProxy() sets the widget which will actually get focus when
  "this widget" gets it.  If there is a focus proxy, focusPolicy(),
  setFocusPolicy(), setFocus() and hasFocus() all operate on the focus
  proxy.

  \sa focusProxy()
*/

void QWidget::setFocusProxy( QWidget * w )
{
    if ( !w && !extra )
	return;

    createExtra();

    if ( extra->focus_proxy ) {
	disconnect( extra->focus_proxy, SIGNAL(destroyed()),
		    this, SLOT(focusProxyDestroyed()) );
	extra->focus_proxy = 0;
    }

    if ( w ) {
	setFocusPolicy( w->focusPolicy() );
	connect( w, SIGNAL(destroyed()),
		 this, SLOT(focusProxyDestroyed()) );
    }
    extra->focus_proxy = w;
}


/*!  Returns a pointer to the focus proxy, or 0 if there is no focus
  proxy.

  \sa setFocusProxy()
*/

QWidget * QWidget::focusProxy() const
{
    return extra ? extra->focus_proxy : 0;
}


/*!  Internal slot used to clean up if the focus proxy is destroyed.
  \sa setFocusProxy()
*/

void QWidget::focusProxyDestroyed()
{
    if ( extra )
	extra->focus_proxy = 0;
    setFocusPolicy( NoFocus );
}


/*!
  Returns TRUE if this widget (or its focus proxy) has the keyboard
  input focus, otherwise FALSE.

  Equivalent to <code>qApp->focusWidget() == this</code>.

  \sa setFocus(), clearFocus(), setFocusPolicy(), QApplication::focusWidget()
*/

bool QWidget::hasFocus() const
{
    const QWidget* w = this;
    while ( w->focusProxy() )
	w = w->focusProxy();
    return qApp->focusWidget() == w;
}

/*!
  Gives the keyboard input focus to the widget (or its focus proxy).

  First, a \link focusOutEvent() focus out event\endlink is sent to the
  focus widget (if any) to tell it that it is about to lose the
  focus. Then a \link focusInEvent() focus in event\endlink is sent to
  this widget to tell it that it just received the focus.

  setFocus() gives focus to a widget regardless of its focus policy.

  \warning If you call setFocus() in a function which may itself be
  called from focusOutEvent() or focusInEvent(), you may see infinite
  recursion.

  \sa hasFocus(), clearFocus(), focusInEvent(), focusOutEvent(),
  setFocusPolicy(), QApplication::focusWidget()
*/

void QWidget::setFocus()
{
    if ( !isEnabled() )
	return;

    if ( focusProxy() ) {
	focusProxy()->setFocus();
	return;
    }

    QFocusData * f = focusData(TRUE);
    if ( f->it.current() == this && qApp->focusWidget() == this )
	return;

    // ### why do we do this twice? - bad merge?

    f->it.toFirst();
    while ( f->it.current() != this && !f->it.atLast() )
	++f->it;
    // at this point, the iterator should point to 'this'.  if it
    // does not, 'this' must not be in the list - an error, but
    // perhaps possible.  fix it.
    if ( f->it.current() != this ) {
	f->focusWidgets.append( this );
	f->it.toLast();
    }

    f->it.toFirst();
    while ( f->it.current() != this && !f->it.atLast() )
	++f->it;
    // at this point, the iterator should point to 'this'.  if it
    // does not, 'this' must not be in the list - an error, but
    // perhaps possible.  fix it.
    if ( f->it.current() != this ) {
	f->focusWidgets.append( this );
	f->it.toLast();
    }

    if ( isActiveWindow() ) {
	QWidget * prev = qApp->focus_widget;
	qApp->focus_widget = this;
	if ( prev != this ) {
	    if ( prev ) {
		QFocusEvent out( QEvent::FocusOut );
		QApplication::sendEvent( prev, &out );
	    }

	    QFocusEvent in( QEvent::FocusIn );
	    QApplication::sendEvent( this, &in );
	}
    }
}

/*!
  Takes keyboard input focus from the widget.

  If the widget has active focus, a \link focusOutEvent() focus out
  event\endlink is sent to this widget to tell it that it is about to
  lose the focus.

  This widget must enable focus setting in order to get the keyboard input
  focus, i.e. it must call setFocusPolicy().

  \sa hasFocus(), setFocus(), focusInEvent(), focusOutEvent(),
  setFocusPolicy(), QApplication::focusWidget()
*/

void QWidget::clearFocus()
{
    if ( focusProxy() ) {
	focusProxy()->clearFocus();
	return;
    } else {
	QWidget* w = qApp->focusWidget();
	if ( w && w->focusWidget() == this ) {
	    // clear active focus
	    qApp->focus_widget = 0;
	    QFocusEvent out( QEvent::FocusOut );
	    QApplication::sendEvent( w, &out );
	}
    }
}


/*!
  Finds a new widget to give the keyboard focus to, as appropriate for
  Tab/shift-Tab, and returns TRUE if is can find a new widget and
  FALSE if it can't,

  If \a next is true, this function searches "forwards", if \a next is
  FALSE, "backwards".

  Sometimes, you will want to reimplement this function.  For example,
  a web browser might reimplement it to move its "current active link"
  forwards or backwards, and call QWidget::focusNextPrevChild() only
  when it reaches the last/first.

  Child widgets call focusNextPrevChild() on their parent widgets, and
  only the top-level widget will thus make the choice of where to redirect
  focus.  By overriding this method for an object, you thus gain control
  of focus traversal for all child widgets.

  \sa focusData()
*/

bool QWidget::focusNextPrevChild( bool next )
{
    QWidget* p = parentWidget();
    if ( !testWFlags(WType_TopLevel) && p )
	return p->focusNextPrevChild(next);

    QFocusData *f = focusData( TRUE );

    QWidget *startingPoint = f->it.current();
    QWidget *candidate = 0;
    QWidget *w = next ? f->focusWidgets.last() : f->focusWidgets.first();
    do {
	if ( w && w != startingPoint &&
	     ( ( w->focusPolicy() & TabFocus ) == TabFocus )
	     && !w->focusProxy() && w->isVisible() && w->isEnabled())
	    candidate = w;
	w = next ? f->focusWidgets.prev() : f->focusWidgets.next();
    } while( w && !(candidate && w==startingPoint) );

    if ( !candidate )
	return FALSE;

    candidate->setFocus();
    return TRUE;
}

/*!
  Returns the focus widget in this widget's window.  This
  is not the same as QApplication::focusWidget(), which returns the
  focus widget in the currently active window.
*/

QWidget *QWidget::focusWidget() const
{
    QWidget *that = (QWidget *)this;		// mutable
    QFocusData *f = that->focusData( FALSE );
    if ( f && f->focusWidgets.count() && f->it.current() == 0 )
	f->it.toFirst();
    return f ? f->it.current() : 0;
}


/*!
  Returns a pointer to the focus data for this widget's top-level
  widget.

  Focus data always belongs to the top-level widget.  The focus data
  list contains all the widgets in this top-level widget that can
  accept focus, in tab order.  An iterator points to the current focus
  widget (focusWidget() returns a pointer to this widget).

  This information is useful for implementing advanced versions
  of focusNextPrevChild().
*/
QFocusData * QWidget::focusData()
{
    return focusData(TRUE);
}

/*!
  Internal function which lets us not create it too.
*/
QFocusData * QWidget::focusData( bool create )
{
    QWidget * tlw = topLevelWidget();
    QWExtra * ed = tlw->extraData();
    if ( !ed || !ed->topextra ) {
	if ( !create )
	    return 0;
	tlw->createTLExtra();
	ed = tlw->extraData();
    }
    if ( create && !ed->topextra->focusData ) {
	ed->topextra->focusData = new QFocusData;
    }
    return ed->topextra->focusData;
}


/*!
  Enables key event compression, if \a enable is TRUE, and disables it
  if \a enable is FALSE.

  By default key compression is off, so widgets receive one key press
  event for each key press (or more, since autorepeat is usually on).
  If you turn it on and your program doesn't keep up with key input,
  Qt tries to compress key events so that more than one character can
  be processed in each event.

  For example, a word processor widget might receive 2, 3 or more
  characters in each QKeyEvent::text(), if the layout recalculation
  takes too long for the CPU.

  If a widget supports multiple character unicode input, it is always
  safe to turn the compression on.

  \sa QKeyEvent::text();
*/

void QWidget::setKeyCompression(bool compress)
{
    if ( compress )
	setWState( WState_CompressKeys );
    else
	clearWState( WState_CompressKeys );
}


/*!
  Returns TRUE if this widget is in the active window, i.e. the
  window that has keyboard focus.

  When popup windows are visible, this function returns TRUE for both
  the active window and the popup.

  \sa setActiveWindow(), QApplication::activeWindow()
*/

bool QWidget::isActiveWindow() const
{
    return (topLevelWidget() == qApp->activeWindow() )||
	     ( isVisible() && topLevelWidget()->isPopup() );
}




/*!
  Moves the \a second widget around the ring of focus widgets
  so that keyboard focus moves from \a first widget to \a second
  widget when Tab is pressed.

  Note that since the tab order of the \e second widget is changed,
  you should order a chain like this:

  \code
    setTabOrder(a, b ); // a to b
    setTabOrder(b, c ); // a to b to c
    setTabOrder(c, d ); // a to b to c to d
  \endcode

  not like this:

  \code
    setTabOrder(c, d); // c to d
    setTabOrder(a, b); // a to b AND c to d
    setTabOrder(b, c); // a to b to c, but not c to d
  \endcode

  If either \a first or \a second has a focus proxy, setTabOrder()
  substitutes its/their proxies.

  \sa setFocusPolicy(), setFocusProxy()
*/
void QWidget::setTabOrder( QWidget* first, QWidget *second )
{
    if ( !first || !second )
	return;

    while ( first->focusProxy() )
	first = first->focusProxy();
    while ( second->focusProxy() )
	second = second->focusProxy();

    QFocusData *f = first->focusData( TRUE );
    bool focusThere = (f->it.current() == second );
    f->focusWidgets.removeRef( second );
    if ( f->focusWidgets.findRef( first ) >= 0 )
	f->focusWidgets.insert( f->focusWidgets.at() + 1, second );
    else
	f->focusWidgets.append( second );
    if ( focusThere ) { // reset iterator so tab will work appropriately
	f->it.toFirst();
	while( f->it.current() && f->it.current() != second )
	    ++f->it;
    }
}

/*!\internal

  Moves the relevant subwidgets of this widget from the \a oldtlw's
  tab chain to that of the new parent, if there's anything to move and
  we're really moving

  This function is called from QWidget::reparent() *after* the widget
  has been reparented.

  \sa reparent()
*/

void QWidget::reparentFocusWidgets( QWidget * oldtlw )
{
    if ( oldtlw == topLevelWidget() )
	return; // nothing to do

    QFocusData * from = oldtlw ? oldtlw->topData()->focusData : 0;
    QFocusData * to;
    to = focusData();

    if ( from ) {
	from->focusWidgets.first();
	do {
	    QWidget * pw = from->focusWidgets.current();
	    while( pw && pw != this )
		pw = pw->parentWidget();
	    if ( pw == this ) {
		QWidget * w = from->focusWidgets.take();
		if ( w == from->it.current() )
		    // probably best to clear keyboard focus, or
		    // the user might become rather confused
		    w->clearFocus();
		if ( !isTopLevel() )
		    to->focusWidgets.append( w );
	    } else {
		from->focusWidgets.next();
	    }
	} while( from->focusWidgets.current() );
    }

    if ( to->focusWidgets.findRef(this) < 0 )
	to->focusWidgets.append( this );

    if ( !isTopLevel() && extra && extra->topextra && extra->topextra->focusData ) {
	// this widget is no longer a top-level widget, so get rid
	// of old focus data
	delete extra->topextra->focusData;
	extra->topextra->focusData = 0;
    }
}

/*!
  \fn void recreate( QWidget *parent, WFlags f, const QPoint & p, bool showIt )

  \obsolete

  This method is provided to aid porting to Qt 2.0.  The function is
  renamed to reparent() in 2.0, and we hope the FAQs about it will
  stop.
*/



/*!
  Returns the size of the window system frame (for top level widgets).
*/
QSize QWidget::frameSize() const
{
    return extra && extra->topextra
	? extra->topextra->fsize
	: crect.size();
}

/*!
  \internal
  Sets the frame rectangle and recomputes the client rectangle.

  The frame rectangle is the geometry of this widget including any
  decorative borders, in its parent's coordinate system.

  The client rectangle is the geometry of just this widget in its
  parent's coordinate system.
*/

void QWidget::setFRect( const QRect &r )
{
    if ( extra && extra->topextra ) {
	QRect frect = frameGeometry();
	crect.setLeft( crect.left() + r.left() - frect.left() );
	crect.setTop( crect.top() + r.top() - frect.top() );
	crect.setRight( crect.right() + r.right() - frect.right() );
	crect.setBottom( crect.bottom() + r.bottom() - frect.bottom() );
	fpos = r.topLeft();
	extra->topextra->fsize = r.size();
    } else {
	// One rect is both the same.
	fpos = r.topLeft();
	crect = r;
    }
}

/*!
  \internal
  Sets the client rectangle and recomputes the frame rectangle.

  The client rectangle is the geometry of just this widget in its
  parent's coordinate system.

  The frame rectangle is the geometry of this widget including any
  decorative borders, in its parent's coordinate system.
*/

void QWidget::setCRect( const QRect &r )
{
    if ( extra && extra->topextra ) {
	QRect frect = frameGeometry();
	frect.setLeft( frect.left() + r.left() - crect.left() );
	frect.setTop( frect.top() + r.top() - crect.top() );
	frect.setRight( frect.right() + r.right() - crect.right() );
	frect.setBottom( frect.bottom() + r.bottom() - crect.bottom() );
	fpos = frect.topLeft();
	extra->topextra->fsize = frect.size();
    } else {
	// One rect is both the same.
	fpos = r.topLeft();
    }
    crect = r;
}


/*!
  \overload void QWidget::move( const QPoint & )
*/

/*!
  Moves the widget to the position \e (x,y) relative to the parent widget and
  including the window frame.

  If the widget is visible, it receives a \link moveEvent() move
  event\endlink immediately. If the widget is not shown yet, it is
  guaranteed to receive an event before it actually becomes visible.

  This function is virtual, and all other overloaded move()
  implementations call it.

  \warning If you call move() or setGeometry() from moveEvent(), you
  may see infinite recursion.

  See the \link geometry.html Window Geometry documentation\endlink
  for an overview of geometry issues with top-level widgets.

  \sa pos(), resize(), setGeometry(), moveEvent()
*/

void QWidget::move( int x, int y )
{
    internalSetGeometry( x + geometry().x() - QWidget::x(),
			 y + geometry().y() - QWidget::y(),
			 width(), height(), TRUE );
}



/*!
  \overload void QWidget::resize( const QSize & )
*/

/*!
  Resizes the widget to size \e w by \e h pixels.

  If the widget is visible, it receives a \link resizeEvent() resize
  event\endlink immediately. If the widget is not shown yet, it is
  guaranteed to receive an event before it actually becomes visible.

  The size is adjusted if it is outside the \link setMinimumSize()
  minimum\endlink or \link setMaximumSize() maximum\endlink widget size.

  This function is virtual, and all other overloaded resize()
  implementations call it.

  \warning If you call resize() or setGeometry() from resizeEvent(),
  you may see infinite recursion.

  \sa size(), move(), setGeometry(), resizeEvent(),
  minimumSize(),  maximumSize()
*/
void QWidget::resize( int w, int h )
{
    internalSetGeometry( geometry().x(), geometry().y(), w, h, FALSE );
    setWState( WState_Resized );
}


/*!
  \overload void QWidget::setGeometry( const QRect & )
*/

/*!
  Sets the widget geometry to \e w by \e h, positioned at \e x,y in its
  parent widget.

  If the widget is visible, it receives a \link moveEvent() move
  event\endlink and/or \link resizeEvent() resize event \endlink
  immediately. If the widget is not shown yet, it is guaranteed to
  receive appropriate events before it actually becomes visible.

  The size is adjusted if it is outside the \link setMinimumSize()
  minimum\endlink or \link setMaximumSize() maximum\endlink widget size.

  This function is virtual, and all other overloaded setGeometry()
  implementations call it.

  \warning If you call setGeometry() from resizeEvent() or moveEvent(),
  you may see infinite recursion.

  \sa geometry(), move(), resize(), moveEvent(), resizeEvent(),
  minimumSize(), maximumSize()
*/

void QWidget::setGeometry( int x, int y, int w, int h )
{
    internalSetGeometry( x, y, w, h, TRUE );
    setWState( WState_Resized );
}


/*!
  \fn bool QWidget::isFocusEnabled() const

  Returns TRUE if the widget accepts keyboard focus, or FALSE if it does
  not.

  Keyboard focus is initially disabled (i.e. focusPolicy() ==
  \c QWidget::NoFocus).

  You must enable keyboard focus for a widget if it processes keyboard
  events.  This is normally done from the widget's constructor.  For
  instance, the QLineEdit constructor calls
  setFocusPolicy(\c QWidget::StrongFocus).

  \sa setFocusPolicy(), focusInEvent(), focusOutEvent(), keyPressEvent(),
  keyReleaseEvent(), isEnabled()
*/

/*! \enum QWidget::FocusPolicy

  This enum type defines the various policies a widget can have with
  respect to acquiring keyboard focus.

  The \e policy can be:
  <ul>
  <li> \c QWidget::TabFocus - the widget accepts focus by tabbing.
  <li> \c QWidget::ClickFocus - the widget accepts focus by clicking.
  <li> \c QWidget::StrongFocus - the widget accepts focus by both tabbing
  and clicking.
  <li> \c QWidget::WheelFocus - like StrongFocus plus the widget accepts
  focus by using the mouse wheel.
  <li> \c QWidget::NoFocus - the widget does not accept focus.
  </ul>
*/

/*!
  \fn QWidget::FocusPolicy QWidget::focusPolicy() const

  Returns \c QWidget::TabFocus if the widget accepts focus by tabbing, \c
  QWidget::ClickFocus if the widget accepts focus by clicking, \c
  QWidget::StrongFocus if it accepts both and \c QWidget::NoFocus if it
  does not accept focus at all.

  \sa isFocusEnabled(), setFocusPolicy(), focusInEvent(), focusOutEvent(),
  keyPressEvent(), keyReleaseEvent(), isEnabled()
*/

/*!
  Enables or disables the keyboard focus for the widget.

  The keyboard focus is initially disabled (i.e. \a policy ==
  \c QWidget::NoFocus).

  You must enable keyboard focus for a widget if it processes keyboard
  events. This is normally done from the widget's constructor.  For
  instance, the QLineEdit constructor calls
  setFocusPolicy(\c QWidget::StrongFocus).

  \sa isFocusEnabled(), focusInEvent(), focusOutEvent(), keyPressEvent(),
  keyReleaseEvent(), isEnabled()
*/

void QWidget::setFocusPolicy( FocusPolicy policy )
{
    if ( focusProxy() )
	focusProxy()->setFocusPolicy( policy );
    if ( policy ) {
	QFocusData * f = focusData( TRUE );
	if ( f->focusWidgets.findRef( this ) < 0 )
	    f->focusWidgets.append( this );
    }
    focus_policy = (uint)policy;
}


/*!
  \fn bool QWidget::isUpdatesEnabled() const
  Returns TRUE if updates are enabled, otherwise FALSE.
  \sa setUpdatesEnabled()
*/

/*!
  Enables widget updates if \e enable is TRUE, or disables widget updates
  if \e enable is FALSE.

  Calling update() and repaint() has no effect if updates are disabled.
  Paint events from the window system are processed normally even if
  updates are disabled.

  This function is normally used to disable updates for a short period of
  time, for instance to avoid screen flicker during large changes.

  Example:
  \code
    setUpdatesEnabled( FALSE );
    bigVisualChanges();
    setUpdatesEnabled( TRUE );
    repaint();
  \endcode

  \sa isUpdatesEnabled(), update(), repaint(), paintEvent()
*/

void QWidget::setUpdatesEnabled( bool enable )
{
    if ( enable )
	clearWState( WState_BlockUpdates );
    else
	setWState( WState_BlockUpdates );
}


/*
  Returns TRUE if there's no non-withdrawn top level window left
  (except the desktop, dialogs or popups).  This is an internal
  function used by QWidget::close() to decide whether to emit
  QApplication::lastWindowClosed() or not.
*/

static bool noMoreToplevels()
{
    QWidgetList *list   = qApp->topLevelWidgets();
    QWidget     *widget = list->first();
    while ( widget ) {
	if ( !widget->isHidden()
	     && !widget->isDesktop()
	     && !widget->isPopup()
	     && !widget->testWFlags( Qt::WStyle_Dialog) )
	    break;
	widget = list->next();
    }
    delete list;
    return widget == 0;
}


void qt_enter_modal( QWidget * );		// defined in qapp_xxx.cpp
void qt_leave_modal( QWidget * );		// --- "" ---
bool qt_modal_state();				// --- "" ---


/*!
  Shows the widget and its child widgets.

  If its size or position has changed, Qt guarantees that a widget gets
  move and resize events just before the widget is shown.

  You almost never have to reimplement this function. If you need to
  change some settings before a widget is shown, use showEvent()
  instead. If you need to do some delayed initialization use polish().

  \sa showEvent(), hide(), showMinimized(), showMaximized(), showNormal(),
  isVisible(), polish()
*/

void QWidget::show()
{
    bool sendLayoutHint = !isTopLevel() && isHidden();
    clearWState( WState_ForceHide );

    if ( testWState(WState_Visible) )
	return; // nothing to do
    if ( !isTopLevel() && !parentWidget()->isVisibleTo( 0 ) ){
	// we should become visible, but our parents are explicitly
	// hidden. Don' worry, since we cleared the ForceHide flag,
	// our immediate parent will call show() on us again during
	// his own processing of show().
	if ( sendLayoutHint ) {
	    QEvent e( QEvent::ShowToParent );
	    QApplication::sendEvent( this, &e );
	}
	return;
    }

    in_show = TRUE;

    QApplication::sendPostedEvents( this, QEvent::ChildInserted );

    if ( isTopLevel() && !testWState( WState_Resized ) )  {
	// do this before sending the posted resize events. Otherwise
	// the layout would catch the resize event and may expand the
	// minimum size.
	QSize s = sizeHint();
	QSizePolicy::ExpandData exp;
#ifndef QT_NO_LAYOUT
	if ( layout() ) {
	    if ( layout()->hasHeightForWidth() )
		s.setHeight( layout()->totalHeightForWidth( s.width() ) );
	    exp =  layout()->expanding();
	} else
#endif
	    {
		if ( sizePolicy().hasHeightForWidth() )
		    s.setHeight( heightForWidth( s.width() ) );
		exp = sizePolicy().expanding();
	    }
 	if ( exp & QSizePolicy::Horizontal )
	    s.setWidth( QMAX( s.width(), 200 ) );
 	if ( exp & QSizePolicy::Vertical )
	    s.setHeight( QMAX( s.height(), 150 ) );
	QWidget * d = QApplication::desktop();
	s.setWidth( QMIN( s.width(), d->width()*2/3 ) );
	s.setHeight( QMIN( s.height(), d->height()*2/3 ) );
	if ( !s.isEmpty() )
	    resize( s );
    }

    QApplication::sendPostedEvents( this, QEvent::Move );
    QApplication::sendPostedEvents( this, QEvent::Resize );

    setWState( WState_Visible );

    if ( parentWidget() )
	QApplication::sendPostedEvents( parentWidget(),
					QEvent::ChildInserted );

    if ( extra ) {
	int w = crect.width();
	int h = crect.height();
	if ( w < extra->minw || h < extra->minh ||
	     w > extra->maxw || h > extra->maxh ) {
	    w = QMAX( extra->minw, QMIN( w, extra->maxw ));
	    h = QMAX( extra->minh, QMIN( h, extra->maxh ));
	    resize( w, h );			// deferred resize
	}
    }

    if ( testWFlags(WStyle_Tool) || isPopup() ) {
	raise();
    } else if ( testWFlags(WType_TopLevel) ) {
	while ( QApplication::activePopupWidget() )
	    QApplication::activePopupWidget()->close();
    }

    if ( !testWState(WState_Polished) )
	polish();

    if ( children() ) {
	QObjectListIt it(*children());
	register QObject *object;
	QWidget *widget;
	while ( it ) {				// show all widget children
	    object = it.current();		//   (except popups and other toplevels)
	    ++it;
	    if ( object->isWidgetType() ) {
		widget = (QWidget*)object;
		if ( !widget->isHidden() && !widget->isTopLevel() )
		    widget->show();
	    }
	}
    }


    bool sendShowWindowRequest = FALSE;

    if ( !isTopLevel() && !parentWidget()->isVisible() ) {
	// we should become visible, but somehow our parent is not
	// visible, so we can't do that. Since it is not explicitly
	// hidden (that we checked above with isVisibleTo(0) ), our
	// window is not withdrawn, but may for example be iconfied or
	// on another virtual desktop. Therefore we have to prepare
	// for simply receiving a show event without show() beeing
	// called again (see the call to sendShowEventsToChildren() in
	// qapplication).
	showWindow();
	clearWState( WState_Visible );
	if ( sendLayoutHint ) {
	    QEvent e( QEvent::ShowToParent );
	    QApplication::sendEvent( this, &e );
	}
    } else {

	QShowEvent e(FALSE);
	QApplication::sendEvent( this, &e );

	if ( testWFlags(WType_Modal) ) {
	    // qt_enter_modal *before* show, otherwise the initial
	    // stacking might be wrong
	    qt_enter_modal( this );
	}

	// do not show the window directly, but post a showWindow
	// request to reduce flicker with laid out widgets
	if ( !isTopLevel() && !parentWidget()->in_show )
	    sendShowWindowRequest = TRUE;
	else
	    showWindow();

	if ( testWFlags(WType_Popup) )
	    qApp->openPopup( this );
    }

    if ( sendLayoutHint )
	QApplication::postEvent( parentWidget(),
				 new QEvent( QEvent::LayoutHint) );
    if ( sendShowWindowRequest )
	QApplication::postEvent( this, new QEvent( QEvent::ShowWindowRequest ) );

    in_show = FALSE;
}



/*!
  Hides the widget.

  You almost never have to reimplement this function. If you need to
  do something after a widget is hidden, use hideEvent() instead.

  \sa isHhideEvent(), isHidden(), show(), showMinimized(), isVisible(), close()
*/

void QWidget::hide()
{
    if ( testWState(WState_ForceHide) )
	return;
    setWState( WState_ForceHide );

    if ( testWFlags(WType_Popup) )
	qApp->closePopup( this );

#if defined(_WS_WIN_)
    if ( isTopLevel() && !isPopup() && parentWidget() && isActiveWindow() )
	parentWidget()->setActiveWindow();	// Activate parent
#endif

    hideWindow();

    if ( !testWState(WState_Visible) ) {
	QEvent e( QEvent::HideToParent );
	QApplication::sendEvent( this, &e );
	// post layout hint for non toplevels. The parent widget check is
	// necessary since the function is called in the destructor
	if ( !isTopLevel() && parentWidget() )
	    QApplication::postEvent( parentWidget(),
				     new QEvent( QEvent::LayoutHint) );
	return;
    }
    clearWState( WState_Visible );

    // next bit tries to move the focus if the focus widget is now
    // hidden.
    if ( qApp && qApp->focusWidget() == this )
	focusNextPrevChild( TRUE );

    QHideEvent e(FALSE);
    QApplication::sendEvent( this, &e );

    // post layout hint for non toplevels. The parent widget check is
    // necessary since the function is called in the destructor
    if ( !isTopLevel() && parentWidget() )
	QApplication::postEvent( parentWidget(),
				 new QEvent( QEvent::LayoutHint) );

    sendHideEventsToChildren( FALSE );

    if ( testWFlags(WType_Modal) )
	qt_leave_modal( this );
}


void QWidget::sendShowEventsToChildren( bool spontaneous )
{
     if ( children() ) {
	QObjectListIt it(*children());
	register QObject *object;
	QWidget *widget;
	while ( it ) {
	    object = it.current();
	    ++it;
	    if ( object->isWidgetType() ) {
		widget = (QWidget*)object;
		if ( !widget->isTopLevel() && !widget->isVisible() && !widget->isHidden() ) {
		    widget->setWState( WState_Visible );
		    widget->sendShowEventsToChildren( spontaneous );
		    QShowEvent e( spontaneous );
		    QApplication::sendEvent( widget, &e );
		}
	    }
	}
    }
}

void QWidget::sendHideEventsToChildren( bool spontaneous )
{
     if ( children() ) {
	QObjectListIt it(*children());
	register QObject *object;
	QWidget *widget;
	while ( it ) {
	    object = it.current();
	    ++it;
	    if ( object->isWidgetType() ) {
		widget = (QWidget*)object;
		if ( !widget->isTopLevel() && widget->isVisible() ) {
		    widget->clearWState( WState_Visible );
		    widget->sendHideEventsToChildren( spontaneous );
		    QHideEvent e( spontaneous );
		    QApplication::sendEvent( widget, &e );
		}
	    }
	}
    }
}


/*!
  Delayed initialization of a widget.

  This function will be called \e after a widget has been fully created
  and \e before it is shown the very first time.

  Polishing is useful for final initialization depending on an
  instantiated widget. This is something a constructor cannot
  guarantee since the initialization of the subclasses might not be
  finished.

  After this function, the widget has a proper font and palette and
  QApplication::polish() has been called.

  Remember to call QWidget's implementation when reimplementing this
  function.

  \sa constPolish(), QApplication::polish()
*/

void QWidget::polish()
{
    if ( !testWState(WState_Polished) ) {
	if ( !own_font && !QApplication::font( this ).isCopyOf( QApplication::font() ) ) {
	    setFont( QApplication::font( this ) );
	    own_font = FALSE;
	}
#ifndef QT_NO_PALETTE
	if ( !own_palette && !QApplication::palette( this ).isCopyOf( QApplication::palette() ) ) {
	    setPalette( QApplication::palette( this ) );
	    own_palette = FALSE;
	}
#endif
	setWState(WState_Polished);
	qApp->polish( this );
	QApplication::sendPostedEvents( this, QEvent::ChildInserted );
    }
}


/*!
  \fn void QWidget::constPolish() const

  Ensures that the widget is properly initialized by calling polish().

  Call constPolish() from functions like sizeHint() that depends on
  the widget being initialized, and that may be called before show().

  \warning Do not call constPolish() on a widget from inside that
  widget's constructor.

  \sa polish()
 */


/*!
  Closes this widget. Returns TRUE if the widget was closed, otherwise
  FALSE.

  If \a alsoDelete is TRUE or the widget has the \c WDestructiveClose
  widget flag, the widget is also deleted.  The widget can prevent
  itself from being closed by rejecting the QCloseEvent it gets.

  The QApplication::lastWindowClosed() signal is emitted when the last
  visible top level widget is closed.

  Note that closing the \l QApplication::mainWidget() terminates the
  application.

  \sa closeEvent(), QCloseEvent, hide(), QApplication::quit(),
  QApplication::setMainWidget(), QApplication::lastWindowClosed()
*/

bool QWidget::close( bool alsoDelete )
{
    if ( is_closing )
	return TRUE;
    is_closing = 1;
    WId id	= winId();
    bool isMain = qApp->mainWidget() == this;
    bool checkLastWindowClosed = isTopLevel() && !isPopup();
    QCloseEvent e;
    QApplication::sendEvent( this, &e );
    bool deleted = !QWidget::find(id);
    if ( !deleted && !e.isAccepted() ) {
	is_closing = 0;
	return FALSE;
    }
    if ( !deleted )
	hide();
    if ( checkLastWindowClosed
	 && qApp->receivers(SIGNAL(lastWindowClosed()))
	 && noMoreToplevels() )
	emit qApp->lastWindowClosed();
    if ( isMain )
	qApp->quit();
    if ( deleted ) 
	return TRUE;
    is_closing = 0;
    if ( alsoDelete || testWFlags(WDestructiveClose) )
	delete this;
    return TRUE;
}


/*!
  \fn bool QWidget::close()
  Closes this widget. Returns TRUE if the widget was closed, otherwise
  FALSE.

  First it sends the widget a QCloseEvent. The widget is \link hide()
  hidden\endlink if it \link QCloseEvent::accept() accepts\endlink the
  close event. The default implementation of QWidget::closeEvent()
  accepts the close event.

  The QApplication::lastWindowClosed() signal is emitted when the last
  visible top level widget is closed.

  \sa close(bool)
*/

/*!
  \fn bool QWidget::isVisible() const

  Returns TRUE if the widget itself is visible, or else FALSE.

  Calling show() sets the widget to visible status if all its parent
  widgets up to the toplevel widget are visible. If an ancestor is not
  visible, the widget won't become visible until all its ancestors are
  shown.

  Calling hide() hides a widget explicitly. An explicitly hidden
  widget will never become visible, even if all its ancestors become
  visible.

  Iconified top-level widgets also have hidden status, as well as
  having isMinimized() return TRUE. Windows that live on another
  virtual desktop (on platforms that support this concept) also have
  hidden status.

  This function returns TRUE if the widget currently is obscured by
  other windows on the screen, but would be visible if moved.

  A widget receives show- and hide events when its visibility status
  changes. Between a hide and a show event, there is no need in
  wasting any CPU on preparing or displaying information to the
  user. A video application, for example, might simply stop generating
  new frames.

  \sa show(), hide(), isHidden(), isVisibleTo(), isMinimized(),
  showEvent(), hideEvent()
*/


/*!
  Returns TRUE if this widget would become visible if \a ancestor is
  shown.

  This is the case if neither the widget itself nor every parent up to
  but excluding \a ancestor has been explicitly hidden.

  This function returns TRUE if the widget it is obscured by other
  windows on the screen, but would be visible if moved.

  isVisibleTo(0) is very similar to isVisible(), with the exception
  that it does not cover the iconfied-case or the situation where the
  window lives on another virtual desktop.

  \sa show() hide() isVisible()
*/

bool QWidget::isVisibleTo(QWidget* ancestor) const
{
    const QWidget * w = this;
    while ( w
	    && !w->isHidden()
	    && !w->isTopLevel()
	    && w->parentWidget()
	    && w->parentWidget()!=ancestor )
	w = w->parentWidget();
    return !w->isHidden();
}


/*!\obsolete

  This function is deprecated. It is equivalent to isVisible()

  \sa show(), hide(), isVisible()
*/

bool QWidget::isVisibleToTLW() const
{
    return isVisible();
}


/*!
  \fn bool QWidget::isHidden() const

  Returns TRUE if the widget is explicitly hidden, or FALSE if it
  is visible or would become visible if all its ancestors became visible.

  \sa hide(), show(), isVisible(), isVisibleTo()
*/



/*!
  Returns the currently visible rectangle of the widget. This function
  is in particular useful to optimize immediate repainting of a
  widget. Typical usage is
  \code
  repaint( w->visibleRect() );
  \endcode
  or
  \code
  repaint( w->visibleRect(), FALSE );
  \endcode

  If nothing is visible, the rectangle returned is empty.
 */
QRect QWidget::visibleRect() const
{
    QRect r = rect();
    const QWidget * w = this;
    int ox = 0;
    int oy = 0;
    while ( w
	    && w->isVisible()
	    && !w->isTopLevel()
	    && w->parentWidget() ) {
	ox -= w->x();
	oy -= w->y();
	w = w->parentWidget();
	r = r.intersect( QRect( ox, oy, w->width(), w->height() ) );
    }
    if ( !w->isVisible() )
	return QRect();
    else
	return r;
}


/*!
  Adjusts the size of the widget to fit the contents.

  Uses sizeHint() if valid (i.e if the size hint's width and height are
  equal to or greater than 0), otherwise sets the size to the children
  rectangle (the union of all child widget geometries).

  \sa sizeHint(), childrenRect()
*/

void QWidget::adjustSize()
{
    if ( !testWState(WState_Polished) )
	polish();
    QSize s = sizeHint();
    if ( s.isValid() ) {
	resize( s );
	return;
    }
    QRect r = childrenRect();			// get children rectangle
    if ( r.isNull() )				// probably no widgets
	return;
    resize( r.width()+2*r.x(), r.height()+2*r.y() );
}


/*!
  Returns a recommended size for the widget, or an invalid size if
  no size is recommended.

  The default implementation returns an invalid size if there is no layout
  for this widget, the layout's preferred size otherwise.

  \sa QSize::isValid(), minimumSizeHint(), sizePolicy(), setMinimumSize(), updateGeometry()
*/

QSize QWidget::sizeHint() const
{
#ifndef QT_NO_LAYOUT
    if ( layout() )
	return layout()->totalSizeHint();
#endif
    constPolish();
    return QSize( -1, -1 );
}



/*!
  Returns a recommended minimum size for the widget, or an invalid
  size if no minimum size is recommended.

  The default implementation returns an invalid size if there is no layout
  for this widget, the layout's minimum size otherwise.

  \sa QSize::isValid(), resize(), setMinimumSize(), sizePolicy()

*/

QSize QWidget::minimumSizeHint() const
{
#ifndef QT_NO_LAYOUT
    if ( layout() )
	return layout()->totalMinimumSize();
#endif
    constPolish();
    return QSize( -1, -1 );
}


/*!
  \fn QWidget *QWidget::parentWidget() const
  Returns a pointer to the parent of this widget, or a null pointer if
  it does not have any parent widget.
*/

/*!
  \fn bool QWidget::testWFlags( WFlags f ) const

  Returns TRUE if any of the widget flags in \a f are set.

  Widget flags are a combination of Qt::WidgetFlags.

  \sa getWFlags(), setWFlags(), clearWFlags()
*/

/*!
  \fn bool QWidget::testWState( uint n ) const
  \internal
  Tests the widget states \a n, returning TRUE if any are set.
*/
/*!
  \fn uint QWidget::getWState() const
  \internal
  Returns the current widget state.
*/
/*!
  \fn void QWidget::clearWState( uint n )
  \internal
  Clears the widgets states \a n.
*/
/*!
  \fn void QWidget::setWState( uint n )
  \internal
  Sets the widgets states \a n.
*/



/*****************************************************************************
  QWidget event handling
 *****************************************************************************/


/*!
  This is the main event handler. You may reimplement this function
  in a subclass, but we recommend using one of the specialized event
  handlers instead.

  The main event handler first passes an event through all \link
  QObject::installEventFilter() event filters\endlink that have been
  installed.  If none of the filters intercept the event, it calls one
  of the specialized event handlers.

  Key press/release events are treated differently from other events.
  event() checks for Tab and shift-Tab and tries to move the focus
  appropriately.  If there is no widget to move the focus to (or the
  key press is not Tab or shift-Tab), event() calls keyPressEvent().

  This function returns TRUE if it is able to pass the event over to
  someone, or FALSE if nobody wanted the event.

  \sa closeEvent(), focusInEvent(), focusOutEvent(), enterEvent(),
  keyPressEvent(), keyReleaseEvent(), leaveEvent(),
  mouseDoubleClickEvent(), mouseMoveEvent(), mousePressEvent(),
  mouseReleaseEvent(), moveEvent(), paintEvent(),
  resizeEvent(), QObject::event(), QObject::timerEvent()
*/

bool QWidget::event( QEvent *e )
{
    if ( eventFilters ) {			// try filters
	if ( activate_filters(e) )		// stopped by a filter
	    return TRUE;
    }

    switch ( e->type() ) {

	case QEvent::Timer:
	    timerEvent( (QTimerEvent*)e );
	    break;

	case QEvent::MouseMove:
	    mouseMoveEvent( (QMouseEvent*)e );
	    break;

	case QEvent::MouseButtonPress:
	    mousePressEvent( (QMouseEvent*)e );
	    break;

	case QEvent::MouseButtonRelease:
	    mouseReleaseEvent( (QMouseEvent*)e );
	    break;

	case QEvent::MouseButtonDblClick:
	    mouseDoubleClickEvent( (QMouseEvent*)e );
	    break;

	case QEvent::Wheel:
	    wheelEvent( (QWheelEvent*)e );
	    if ( ! ((QWheelEvent*)e)->isAccepted() )
		return FALSE;
	    break;
	case QEvent::KeyPress: {
	    QKeyEvent *k = (QKeyEvent *)e;
	    bool res = FALSE;
	    if ( k->key() == Key_Backtab ||
		 (k->key() == Key_Tab &&
		  (k->state() & ShiftButton)) ) {
		QFocusEvent::setReason( QFocusEvent::Tab );
		res = focusNextPrevChild( FALSE );
		QFocusEvent::resetReason();

	    } else if ( k->key() == Key_Tab ) {
		QFocusEvent::setReason( QFocusEvent::Tab );
		res = focusNextPrevChild( TRUE );
		QFocusEvent::resetReason();
	    }
	    if ( res )
		break;
	    QWidget *w = this;
	    while ( w ) {
		w->keyPressEvent( k );
		if ( k->isAccepted() || w->isTopLevel() )
		    break;
		w = w->parentWidget();
		k->accept();
	    }
	    }
	    break;

	case QEvent::KeyRelease: {
	    QKeyEvent *k = (QKeyEvent *)e;
	    QWidget *w = this;
	    while ( w ) {
		k->accept();
		w->keyReleaseEvent( k );
		if ( k->isAccepted() || w->isTopLevel() )
		    break;
		w = w->parentWidget();
	    }
	    }
	    break;

	case QEvent::FocusIn:
	    setFontSys();
	    focusInEvent( (QFocusEvent*)e );
	    break;

	case QEvent::FocusOut:
	    focusOutEvent( (QFocusEvent*)e );
	    break;

	case QEvent::Enter:
	    enterEvent( e );
	    break;

	case QEvent::Leave:
	     leaveEvent( e );
	    break;

	case QEvent::Paint:
	    // At this point the event has to be delivered, regardless
	    // whether the widget isVisible() or not because it
	    // already went through the filters
	    paintEvent( (QPaintEvent*)e );
	    break;

	case QEvent::Move:
	    moveEvent( (QMoveEvent*)e );
	    break;

	case QEvent::Resize:
	    resizeEvent( (QResizeEvent*)e );
	    break;

	case QEvent::Close: {
	    QCloseEvent *c = (QCloseEvent *)e;
	    closeEvent( c );
	    if ( !c->isAccepted() )
		return FALSE;
	    }
	    break;
#ifndef QT_NO_DRAGANDDROP
	case QEvent::Drop:
	    dropEvent( (QDropEvent*) e);
	    break;
	case QEvent::DragEnter:
	    dragEnterEvent( (QDragEnterEvent*) e);
	    break;
	case QEvent::DragMove:
	    dragMoveEvent( (QDragMoveEvent*) e);
	    break;
	case QEvent::DragLeave:
	    dragLeaveEvent( (QDragLeaveEvent*) e);
	    break;
#endif
	case QEvent::Show:
	    showEvent( (QShowEvent*) e);
	    break;
	case QEvent::Hide:
	    hideEvent( (QHideEvent*) e);
	    break;
	case QEvent::ShowWindowRequest:
	    if ( !isHidden() )
		showWindow();
	    break;
	case QEvent::ChildInserted:
	case QEvent::ChildRemoved:
	    childEvent( (QChildEvent*) e);
	    break;
	case QEvent::ParentFontChange:
	    if ( isTopLevel() )
		break;
	    // FALL THROUGH
	case QEvent::ApplicationFontChange:
	    if ( !own_font && !isDesktop() ) {
		if ( !isTopLevel() && QApplication::font( this ).isCopyOf( QApplication::font() ) )
		    setFont( parentWidget()->font() );
		else
		    setFont( QApplication::font( this ) );
		own_font = FALSE;
	    }
	    break;
#ifndef QT_NO_PALETTE
	case QEvent::ParentPaletteChange:
 	    if ( isTopLevel() )
 		break;
	    // FALL THROUGH
	case QEvent::ApplicationPaletteChange:
	    if ( !own_palette && !isDesktop() ) {
		if ( !isTopLevel() && QApplication::palette( this ).isCopyOf( QApplication::palette() ) )
		    setPalette( parentWidget()->palette() );
		else
		    setPalette( QApplication::palette( this ) );
		own_palette = FALSE;
	    }
	    break;
#endif
        default:
	    if ( e->type() >= QEvent::User ) {
		customEvent( (QCustomEvent*) e );
		return TRUE;
	    }
	    return FALSE;
    }
    return TRUE;
}


/*!
  This event handler can be reimplemented in a subclass to receive
  mouse move events for the widget.

  If mouse tracking is switched off, mouse move events only occur if a
  mouse button is down while the mouse is being moved.	If mouse
  tracking is switched on, mouse move events occur even if no mouse
  button is down.

  QMouseEvent::pos() reports the position of the mouse cursor, relative to
  this widget.  For press and release events, the position is usually
  the same as the position of the last mouse move event, but it might be
  different if the user moves and clicks the mouse fast.  This is
  a feature of the underlying window system, not Qt.

  \sa setMouseTracking(), mousePressEvent(), mouseReleaseEvent(),
  mouseDoubleClickEvent(), event(), QMouseEvent
*/

void QWidget::mouseMoveEvent( QMouseEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  mouse press events for the widget.

  If you create new widgets in the mousePressEvent() the
  mouseReleaseEvent() may not end up where you expect, depending on the
  underlying window system (or X11 window manager), the widgets'
  location and maybe more.

  The default implementation implements the closing of popup widgets
  when you click outside the window. For other widget types it does
  nothing.

  \sa mouseReleaseEvent(), mouseDoubleClickEvent(),
  mouseMoveEvent(), event(),  QMouseEvent
*/

void QWidget::mousePressEvent( QMouseEvent *e )
{
    if ( isPopup() ) {
	QWidget* w;
	while ( (w = qApp->activePopupWidget() ) && w != this ){
	    w->close();
	    if (qApp->activePopupWidget() == w) // widget does not want to dissappear
		w->hide(); // hide at least
	}
	if (!rect().contains(e->pos()) ){
	    close();
	}
    }
}

/*!
  This event handler can be reimplemented in a subclass to receive
  mouse release events for the widget.

  \sa mouseReleaseEvent(), mouseDoubleClickEvent(),
  mouseMoveEvent(), event(),  QMouseEvent
*/

void QWidget::mouseReleaseEvent( QMouseEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  mouse double click events for the widget.

  The default implementation generates a normal mouse press event.

  Note that the widgets gets a mousePressEvent() and a mouseReleaseEvent()
  before the mouseDoubleClickEvent().

  \sa mousePressEvent(), mouseReleaseEvent()
  mouseMoveEvent(), event(),  QMouseEvent
*/

void QWidget::mouseDoubleClickEvent( QMouseEvent *e )
{
    mousePressEvent( e );			// try mouse press event
}


/*!
  This event handler can be reimplemented in a subclass to receive
  wheel events for the widget.

  If you reimplement this handler, it is very important that you \link
  QWheelEvent ignore()\endlink the event if you do not handle it, so
  that the widget's parent can interpret it.

  The default implementation ignores the event.

  \sa QWheelEvent::ignore(), QWheelEvent::accept(),
  event(), QWheelEvent
*/

void QWidget::wheelEvent( QWheelEvent *e )
{
    e->ignore();
}


/*!
  This event handler can be reimplemented in a subclass to receive key
  press events for the widget.

  A widget must call setFocusPolicy() to accept focus initially and
  have focus in order to receive a key press event.

  If you reimplement this handler, it is very important that you
  ignore() the event if you do not understand it, so that the widget's
  parent can interpret it.

  The default implementation closes popup widgets if you hit
  escape.  Otherwise the event is ignored.

  \sa keyReleaseEvent(), QKeyEvent::ignore(), setFocusPolicy(),
  focusInEvent(), focusOutEvent(), event(), QKeyEvent
*/

void QWidget::keyPressEvent( QKeyEvent *e )
{
    if ( isPopup() && e->key() == Key_Escape ) {
	e->accept();
	close();
    } else {
	e->ignore();
    }
}

/*!
  This event handler can be reimplemented in a subclass to receive
  key release events for the widget.

  A widget must \link setFocusPolicy() accept focus\endlink initially
  and \link hasFocus() have focus\endlink in order to receive a key
  release event.

  If you reimplement this handler, it is very important that you \link
  QKeyEvent ignore()\endlink the release if you do not understand it,
  so that the widget's parent can interpret it.

  The default implementation ignores the event.

  \sa keyPressEvent(), QKeyEvent::ignore(), setFocusPolicy(),
  focusInEvent(), focusOutEvent(), event(), QKeyEvent
*/

void QWidget::keyReleaseEvent( QKeyEvent *e )
{
    e->ignore();
}

/*!
  This event handler can be reimplemented in a subclass to receive
  keyboard focus events (focus received) for the widget.

  A widget normally must setFocusPolicy() to something other than
  NoFocus in order to receive focus events.  (Note that the
  application programmer can call setFocus() on any widget, even those
  that do not normally accept focus.)

  The default implementation updates the widget if it accepts
  focus (see focusPolicy()).  It also calls setMicroFocusHint(), hinting any
  system-specific input tools about the focus of the user's attention.

  \sa focusOutEvent(), setFocusPolicy(),
  keyPressEvent(), keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusInEvent( QFocusEvent * )
{
    if ( focusPolicy() != NoFocus || !isTopLevel() ) {
	update();
	if ( testWState(WState_AutoMask) )
	    updateMask();
	setMicroFocusHint(width()/2, 0, 1, height(), FALSE);
    }
}

/*!
  This event handler can be reimplemented in a subclass to receive
  keyboard focus events (focus lost) for the widget.

  A widget normally must setFocusPolicy() to something other than
  NoFocus in order to receive focus events.  (Note that the
  application programmer can call setFocus() on any widget, even those
  that do not normally accept focus.)

  The default implementation calls repaint() since the widget's
  colorGroup() changes from active to normal, so the widget probably
  needs repainting.  It also calls setMicroFocusHint(), hinting any
  system-specific input tools about the focus of the user's attention.

  \sa focusInEvent(), setFocusPolicy(),
  keyPressEvent(), keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusOutEvent( QFocusEvent * )
{
    if ( focusPolicy() != NoFocus || !isTopLevel() ){
	update();
	if ( testWState(WState_AutoMask) )
	    updateMask();
    }
}


/*!
  Returns the currently set micro focus hint for this widget.

  \sa setMicroFocusHint()
 */
QRect QWidget::microFocusHint() const
{
    if ( !extra || extra->micro_focus_hint.isEmpty() )
	return QRect(width()/2, 0, 1, height() );
    else
	return extra->micro_focus_hint;
}



/*!
  This event handler can be reimplemented in a subclass to receive
  widget enter events.

  An event is sent to the widget when the mouse cursor enters the widget.

  \sa leaveEvent(), mouseMoveEvent(), event()
*/

void QWidget::enterEvent( QEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  widget leave events.

  A leave event is sent to the widget when the mouse cursor leaves
  the widget.

  \sa enterEvent(), mouseMoveEvent(), event()
*/

void QWidget::leaveEvent( QEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  widget paint events.

  When the paint event occurs, the update region QPaintEvent::region()
  normally has been cleared to the background color or pixmap. An
  exception is when repaint(FALSE) is called or the widget sets the
  WRepaintNoErase or WResizeNoErase flag.  Inside the paint event
  handler, QPaintEvent::erased() carries this information.

  For many widgets it is sufficient to redraw the entire widget each time,
  but some need to consider the update
  \link QPaintEvent::rect() rectangle\endlink
  or
  \link QPaintEvent::region() region\endlink
  of the QPaintEvent to avoid slow update.

  During paintEvent(), any QPainter you create on the widget will be
  clipped to at most the area covered by the update region.

  update() and repaint() can be used to force a paint event.

  \sa event(), repaint(), update(), QPainter, QPixmap, QPaintEvent
*/

void QWidget::paintEvent( QPaintEvent * )
{
}


/*!
  This event handler can be reimplemented in a subclass to receive
  widget move events.  When the widget receives this event, it is
  already at the new position.

  The old position is accessible through QMoveEvent::oldPos().

  \sa resizeEvent(), event(), move(), QMoveEvent
*/

void QWidget::moveEvent( QMoveEvent * )
{
}


/*!
  This event handler can be reimplemented in a subclass to receive
  widget resize events. When resizeEvent() is called, the widget
  already has its new geometry. The old size is accessible through
  QResizeEvent::oldSize(), though.

  The widget will be erased and receive a paint event immediately
  after processing the resize event. No drawing has to (and should) be
  done inside this handler.

  Widgets that have been created with the \c WResizeNoErase flag will not
  be erased. Nevertheless, they will receive a paint event for their
  entire area afterwards. Again, no drawing needs to be done inside
  this handler.

  The default implementation calls updateMask() if the widget
  has \link QWidget::setAutoMask() automatic masking\endlink
  enabled.

  \sa moveEvent(), event(), resize(), QResizeEvent, paintEvent()
*/

void QWidget::resizeEvent( QResizeEvent * )
{
    if ( testWState(WState_AutoMask) )
	updateMask();
}

/*!
  This event handler can be reimplemented in a subclass to receive
  widget close events.

  The default implementation calls e->accept(), which hides this widget.
  See the QCloseEvent documentation for more details.

  \sa event(), hide(), close(), QCloseEvent
*/

void QWidget::closeEvent( QCloseEvent *e )
{
    e->accept();
}


#ifndef QT_NO_DRAGANDDROP

/*!
  This event handler is called when a drag is in progress and the
  mouse enters this widget.

  See the \link dnd.html Drag-and-drop documentation\endlink for
  an overview of how to provide drag-and-drop in your application.

  \sa QTextDrag, QImageDrag, QDragEnterEvent
*/
void QWidget::dragEnterEvent( QDragEnterEvent * )
{
}

/*!
  This event handler is called when a drag is in progress and the
  mouse enters this widget, and whenever it moves within
  the widget.

  See the \link dnd.html Drag-and-drop documentation\endlink for
  an overview of how to provide drag-and-drop in your application.

  \sa QTextDrag, QImageDrag, QDragMoveEvent
*/
void QWidget::dragMoveEvent( QDragMoveEvent * )
{
}

/*!
  This event handler is called when a drag is in progress and the
  mouse leaves this widget.

  See the \link dnd.html Drag-and-drop documentation\endlink for
  an overview of how to provide drag-and-drop in your application.

  \sa QTextDrag, QImageDrag, QDragLeaveEvent
*/
void QWidget::dragLeaveEvent( QDragLeaveEvent * )
{
}

/*!
  This event handler is called when the drag is dropped on this
  widget.

  See the \link dnd.html Drag-and-drop documentation\endlink for
  an overview of how to provide drag-and-drop in your application.

  \sa QTextDrag, QImageDrag, QDropEvent
*/
void QWidget::dropEvent( QDropEvent * )
{
}

#endif // QT_NO_DRAGANDDROP

/*!
  This event handler can be reimplemented in a subclass to receive
  widget show events.

  Non-spontaneous show events are sent to widgets right before they are
  shown. Spontaneous show events of toplevel widgets are delivered
  afterwards, naturally.

  \sa event(), QShowEvent
  */
void QWidget::showEvent( QShowEvent * )
{
    if ( focusWidget() )
	return;
    else if ( focusPolicy() )
	setFocus();
    else
	focusNextPrevChild( TRUE );
}

/*!
  This event handler can be reimplemented in a subclass to receive
  widget hide events.

  Hide events are sent to widgets right after they have been hidden.

  \sa event(), QHideEvent
  */
void QWidget::hideEvent( QHideEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  custom events. Custom events are user-defined events with a type
  value at least as large as the "User" item of the QEvent::Type enum,
  and is typically a QCustomEvent or QCustomEvent subclass.

  \sa event(), QCustomEvent
*/
void QWidget::customEvent( QCustomEvent * )
{
}


#if defined(_WS_MAC_)

/*!
  This special event handler can be reimplemented in a subclass to receive
  native Macintosh events.

  If the event handler returns FALSE, this native event is passed back to
  Qt, which translates the event into a Qt event and sends it to the
  widget.  If the event handler returns TRUE, the event is stopped.

  \warning This function is not portable.

  QApplication::macEventFilter()
*/

bool QWidget::macEvent( MSG * )
{
    return FALSE;
}

#elif defined(_WS_WIN_)

/*!
  This special event handler can be reimplemented in a subclass to receive
  native Windows events.

  If the event handler returns FALSE, this native event is passed back to
  Qt, which translates the event into a Qt event and sends it to the
  widget.  If the event handler returns TRUE, the event is stopped.

  \warning This function is not portable.

  QApplication::winEventFilter()
*/

bool QWidget::winEvent( MSG * )
{
    return FALSE;
}

#elif defined(_WS_X11_)

/*!
  This special event handler can be reimplemented in a subclass to receive
  native X11 events.

  If the event handler returns FALSE, this native event is passed back to
  Qt, which translates the event into a Qt event and sends it to the
  widget.  If the event handler returns TRUE, the event is stopped.

  \warning This function is not portable.

  QApplication::x11EventFilter()
*/

bool QWidget::x11Event( XEvent * )
{
    return FALSE;
}

#elif defined(_WS_QWS_)

/*!
  This special event handler can be reimplemented in a subclass to receive
  native Qt/Embedded events.

  If the event handler returns FALSE, this native event is passed back to
  Qt, which translates the event into a Qt event and sends it to the
  widget.  If the event handler returns TRUE, the event is stopped.

  \warning This function is not portable.

  QApplication::qwsEventFilter()
*/

bool QWidget::qwsEvent( QWSEvent * )
{
    return FALSE;
}

#endif


/*!\obsolete

  The return value is meaningless

  \sa setFontPropagation()
*/

QWidget::PropagationMode QWidget::fontPropagation() const
{
    return SameFont;
}


/*!\obsolete

  Calling this function has no effect.
*/
void QWidget::setFontPropagation( PropagationMode )
{
}


/*!  \obsolete

  The return value is meaningless
*/

QWidget::PropagationMode QWidget::palettePropagation() const
{
    return SamePalette;
}


/*!  \obsolete

  Calling this function has no effect.
*/
void QWidget::setPalettePropagation( PropagationMode )
{
}

/*!
  Transparent widgets use a \link setMask() mask \endlink to define
  their visible region. QWidget has some built-in support to make the
  task of recalculating the mask easier. When setting auto mask to
  TRUE, updateMask() will be called whenever the widget is resized or
  changes its focus state.

  Note: When you re-implement resizeEvent(), focusInEvent() or
  focusOutEvent() in your custom widgets and still want to ensure that
  the auto mask calculation works, you will have to add

    \code
    if ( autoMask() )
	  updateMask();
    \endcode

    at the end of your event handlers. Same holds for all member
    functions that change the appearance of the widget in a way that a
    recalculation of the mask is necessary.

    While being a technically appealing concept, masks have one big
    drawback: when using complex masks that cannot be expressed easily
    with relatively simple regions, they tend to be very slow on some
    window systems. The classic example is a transparent label. The
    complex shape of its contents makes it necessary to represent its
    mask by a bitmap, which consumes both memory and time.  If all you
    want is to blend the background of several neighboring widgets
    together seamlessly, you may probably want to use
    setBackgroundOrigin() rather than a mask.

  \sa autoMask(), updateMask(), setMask(), clearMask(), setBackgroundOrigin()
*/

void QWidget::setAutoMask( bool enable )
{
    if ( enable == autoMask() )
	return;

    if ( enable ) {
	setWState(WState_AutoMask);
	updateMask();
    } else {
	clearWState(WState_AutoMask);
	clearMask();
    }
}

/*!
  Returns whether or not the  widget has the auto mask feature enabled.

  \sa setAutoMask(), updateMask(), setMask(), clearMask()
*/

bool QWidget::autoMask() const
{
    return testWState(WState_AutoMask);
}

/*! \enum QWidget::BackgroundOrigin

  This enum defines the origin used to draw a widget's background
  pixmap.

  <ul>
  <li> \c WidgetOrigin - the pixmap is drawn in the widget's coordinate system.
  <li>\c ParentOrigin - the pixmap is drawn in the parent's coordinate system.
  </ul>

 */

/*!
  Sets the widget's background to be drawn relative to \a origin,
  which is either of \c WidgetOrigin (the default) or \c ParentOrigin.

  This makes a difference only if the widget has a background pixmap
  where the positioning matters. In such case, using \c ParentOrigin
  for several neighboring widgets makes the background blend together
  seamlessly.

  \sa backgroundOrigin(), backgroundPixmap(), setBackgroundMode()
 */


void QWidget::setBackgroundOrigin( BackgroundOrigin origin )
{
    if ( origin == backgroundOrigin() )
	return;

    if ( origin == ParentOrigin )
	setWState( WState_TranslateBackground );
    else
	clearWState( WState_TranslateBackground );
    update();
}

/*!
  Returns the current background origin.

  \sa setBackgroundOrigin()
*/
QWidget::BackgroundOrigin QWidget::backgroundOrigin() const
{
    if ( testWState( WState_TranslateBackground ) )
	return ParentOrigin;
    else
	return WidgetOrigin;
}


/*!
  This function can be reimplemented in a subclass to support
  transparent widgets. It is supposed to be called whenever a widget
  changes state in a way that the shape mask has to be recalculated.

  \sa setAutoMask(), updateMask(), setMask(), clearMask()
  */
void QWidget::updateMask()
{
}


/*!
  \fn QLayout* QWidget::layout () const

  Returns a pointer to the layout engine that manages the geometry of
  this widget's children.

  If the widget does not have a layout, layout() returns a null pointer.

  \sa  sizePolicy()
*/


/*!  Sets this widget to use \a l to manage the geometry of its
  children.

  If there already was a layout for this widget, the old layout is
  forgotten.  (Note that it is not deleted.)

  \sa layout() QLayout sizePolicy()
*/
#ifndef QT_NO_LAYOUT
void QWidget::setLayout( QLayout *l )
{
    lay_out = l;
}
#endif

/*!
  Returns the default layout behaviour of this widget.

  If there is a QLayout that manages this widget's children, the size
  policy specified by that layout is used. If there is no such
  QLayout, the result of this function is used.

  \sa setSizePolicy(), sizeHint() QLayout QSizePolicy, updateGeometry()
*/

QSizePolicy QWidget::sizePolicy() const
{
    return extra ? extra->size_policy : QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
}

/*!

  Sets the size policy for this widget to \a policy. The size policy
  specifies the default layout behaviour.

  The default policy is Preferred/Preferred, which means that the
  widget can be freely resized, but prefers to be the size sizeHint()
  returns. Button-like widgets set the size policy to specify that
  they may stretch horizontally, but are fixed vertically. The same
  applies to lineedit controls (such as QLineEdit, QSpinBox or an
  editable QComboBox) and other horizontally orientated widgets (such
  as QProgressBar).  A QToolButton on the other hand wants to be
  squared, therefore it allows growth in both directions. Widgets that
  support different directions (such as QSlider, QScrollBar or
  QHeader) specify stretching in the respective direction
  only. Widgets that can provide scrollbars (usually subclasses of
  QScrollView) tend to specify that they can use additional space, and
  that they can survive on less than sizeHint().

  \sa sizeHint() QLayout QSizePolicy, updateGeometry()
*/

void QWidget::setSizePolicy( QSizePolicy policy )
{
    if ( policy == sizePolicy() )
	return;
    createExtra();
    extra->size_policy = policy;
    updateGeometry();
}


/*!
  Returns the preferred height for this widget, given the width \a w.
  The default implementation returns 0, indicating that the preferred
  height does not depend on the width.

  \warning Does not look at the widget's layout
*/

int QWidget::heightForWidth( int w ) const
{
    (void)w;
    return 0;
}


/*!
  Returns whether the widget wants to handle What's This help
  manually. The default implementation returns FALSE, which means the
  widget will not receive any events in Whats This mode.

  The widget may leave Whats This mode by calling
  QWhatsThis::leaveWhatsThisMode(), with or without actually
  displaying any help text.

  You may also reimplement customWhatsThis() if your widget is a
  so-called "passive interactor" that is supposed to work under all
  circumstances. Simply don't call QWhatsThis::leaveWhatsThisMode() in
  that case.

  \sa QWhatsThis::inWhatsThisMode(), QWhatsThis::leaveWhatsThisMode()
 */
bool QWidget::customWhatsThis() const
{
    return FALSE;
}


/*!
  Notifies the layout system that this widget has changed and may need
  to change geometry.

  Call this function if the sizeHint() or sizePolicy() have changed.

  For explicitly hidden widgets, updateGeometry() is a no-op. The
  layout system will be notified as soon as the widget is shown.
*/

void QWidget::updateGeometry()
{
    if ( !isTopLevel() && !isHidden() )
	QApplication::postEvent( parentWidget(),
				 new QEvent( QEvent::LayoutHint ) );
}



/*!
  Sets the widget's GUI style to \a style. Ownership of the style
  object is not transferred.

  If no style is set, the widget uses the application's style
  QApplication::style() instead.

  Setting a widget's style has no effect on existing or future
  child widgets.

  \warning This function is particularly useful for demonstration
  purposes, where you want to show Qt's styling capabilities.  Real
  applications should stay away from it and use one consistent GUI
  style instead.

  \sa style(), QStyle, QApplication::style(), QApplication::setStyle()
*/
#ifndef QT_NO_STYLE

void QWidget::setStyle( QStyle *style )
{
    QStyle& old  = QWidget::style();
    createExtra();
    extra->style = style;
    if ( !testWFlags(WType_Desktop) // (except desktop)
	 && testWState(WState_Polished)) { // (and have been polished)
	old.unPolish( this );
	QWidget::style().polish( this );
    }
    styleChange( old );
}
#endif

/*!\overload

  A convenience version of reparent that does not take widget
  flags as argument.

  Calls reparent(\a parent, getWFlags()&~WType_Mask, \a p, \a showit )
*/
void  QWidget::reparent( QWidget *parent, const QPoint & p,
			 bool showIt )
{
    reparent( parent, getWFlags() & ~WType_Mask, p, showIt );
}



/*!
  Shows the widget in full-screen mode.

  Calling this function has no effect for other than top-level
  widgets.

  To return from full-screen mode, call showNormal().

  Full-screen mode works fine under Windows, but has certain problems
  under X.  These problems are due to limitations of the ICCCM
  protocol that specifies the communication between X11 clients and
  the window manager.  ICCCM simply does not know the concept of
  non-decorated full-screen windows. Therefore, the best we can do is
  to request a borderless window and place and resize it to fill the
  entire screen. Depending on the window manager, this may or may not
  work. The borderless window is requested using MOTIF hints, which
  are at least partially supported by virtually all modern window
  managers.

  An alternative would be to bypass the window manager at all and to
  create a window with the WX11BypassWM flag. This has other severe
  problems, though, like totally broken keyboard focus and very
  strange effects on desktop changes or when the user raises other
  windows.

  Future window managers that follow modern post-ICCCM specifications
  may support full-screen mode properly.

  \sa showNormal(), showMaximized(), show(), hide(), isVisible()
*/
void QWidget::showFullScreen()
{
    if ( !isTopLevel() )
	return;
    if ( topData()->fullscreen ) {
	show();
	raise();
	return;
    }
    if ( topData()->normalGeometry.width() < 0 )
	topData()->normalGeometry = QRect( pos(), size() );
    reparent( 0, WType_TopLevel | WStyle_Customize | WStyle_NoBorderEx | WStyle_StaysOnTop,
	      QPoint(0,0) );
    topData()->fullscreen = 1;
    resize( qApp->desktop()->size() );
    raise();
    show();
#if defined(_WS_X11_)
    extern void qt_wait_for_window_manager( QWidget* w ); // defined in qwidget_x11.cpp
    qt_wait_for_window_manager( this );
#endif

    setActiveWindow();
}

/*!
  \fn bool QWidget::isMaximized() const

  Returns TRUE if this widget is a top-level widget that is maximized,
  or else FALSE.

  Note that due to limitations in some window-systems,
  this does not always report expected results (eg. if the user on X11
  maximizes the window via the window manager, Qt has no way of telling
  this from any other resize). This will improve as window manager
  protocols advance.

  \sa showMaximized()
 */


/*!
  \fn bool QWidget::ownCursor() const
  Returns whether the widget uses its own cursor or its parent
  widget's cursor
 */

/*!
  \fn bool QWidget::ownFont() const
  Returns whether the widget uses its own font or its natural
  default font.

  \sa setFont(), unsetFont()
 */

/*!
  \fn bool QWidget::ownPalette() const
  Returns whether the widget uses its own palette or its natural
  default palette.

  \sa setPalette(), unsetPalette()
 */
