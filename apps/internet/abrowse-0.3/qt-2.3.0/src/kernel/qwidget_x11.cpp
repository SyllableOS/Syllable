/****************************************************************************
** $Id$
**
** Implementation of QWidget and QWindow classes for X11
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

#include "qapplication.h"
#include "qpaintdevicemetrics.h"
#include "qpainter.h"
#include "qbitmap.h"
#include "qwidgetlist.h"
#include "qwidgetintdict.h"
#include "qobjectlist.h"
#include "qobjectdict.h"
#include "qaccel.h"
#include "qdragobject.h"
#include "qfocusdata.h"
#include "qabstractlayout.h"
#include "qtextcodec.h"
#include "qt_x11.h"

// NOT REVISED

void qt_enter_modal( QWidget * );		// defined in qapplication_x11.cpp
void qt_leave_modal( QWidget * );		// --- "" ---
bool qt_modal_state();				// --- "" ---
void qt_insert_sip( QWidget*, int, int );	// --- "" ---
int  qt_sip_count( QWidget* );			// --- "" ---
bool qt_wstate_iconified( WId );		// --- "" ---
void qt_updated_rootinfo();
#ifndef NO_XIM
extern XIM qt_xim;
extern XIMStyle qt_xim_style;
#endif

// Paint event clipping magic
extern void qt_set_paintevent_clipping( QPaintDevice* dev, const QRegion& region);
extern void qt_clear_paintevent_clipping();

extern bool qt_dnd_enable( QWidget* w, bool on );
extern bool qt_nograb();

extern void qt_deferred_map_add( QWidget* ); // defined in qapplication_x11.cpp
extern void qt_deferred_map_take( QWidget* );// defined in qapplication_x11.cpp

static QWidget *mouseGrb    = 0;
static QWidget *keyboardGrb = 0;

extern Time qt_x_time; // defined in qapplication_x11.cpp


/*****************************************************************************
  QWidget member functions
 *****************************************************************************/


extern Atom qt_wm_delete_window;	// defined in qapplication_x11.cpp
extern Atom qt_wm_take_focus;		// defined in qapplication_x11.cpp
extern Atom qt_wm_client_leader;	// defined in qapplication_x11.cpp
extern Atom qt_window_role;		// defined in qapplication_x11.cpp
extern Atom qt_sm_client_id;		// defined in qapplication_x11.cpp
extern Atom qt_net_wm_context_help;	// defined in qapplication_x11.cpp
extern Atom qt_xa_motif_wm_hints;	// defined in qapplication_x11.cpp

const uint stdWidgetEventMask =			// X event mask
	(uint)(
	    KeyPressMask | KeyReleaseMask |
	    ButtonPressMask | ButtonReleaseMask |
	    KeymapStateMask |
	    ButtonMotionMask |
	    EnterWindowMask | LeaveWindowMask |
	    FocusChangeMask |
	    ExposureMask |
	    PropertyChangeMask |
	    StructureNotifyMask | SubstructureRedirectMask
	);

const uint stdDesktopEventMask =			// X event mask
	(uint)(
	    KeyPressMask | KeyReleaseMask |
	    KeymapStateMask |
	    EnterWindowMask | LeaveWindowMask |
	    FocusChangeMask | PropertyChangeMask
	);


/*
  The qt_ functions below are implemented in qwidgetcreate_x11.cpp.
*/

Window qt_XCreateWindow( const QWidget *creator,
			 Display *display, Window parent,
			 int x, int y, uint w, uint h,
			 int borderwidth, int depth,
			 uint windowclass, Visual *visual,
			 ulong valuemask, XSetWindowAttributes *attributes );
Window qt_XCreateSimpleWindow( const QWidget *creator,
			       Display *display, Window parent,
			       int x, int y, uint w, uint h, int borderwidth,
			       ulong border, ulong background );
void qt_XDestroyWindow( const QWidget *destroyer,
			Display *display, Window window );



/*!
  Creates a new widget window if \a window is null, otherwise sets the
  widget's window to \a window.

  Initializes the window (sets the geometry etc.) if \a initializeWindow
  is TRUE.  If \a initializeWindow is FALSE, no initialization is
  performed.  This parameter makes only sense if \a window is a valid
  window.

  Destroys the old window if \a destroyOldWindow is TRUE.  If \a
  destroyOldWindow is FALSE, you are responsible for destroying
  the window yourself (using platform native code).

  The QWidget constructor calls create(0,TRUE,TRUE) to create a window for
  this widget.
*/

void QWidget::create( WId window, bool initializeWindow, bool destroyOldWindow)
{
    if ( testWState(WState_Created) && window == 0 )
	return;
    setWState( WState_Created );			// set created flag

    if ( !parentWidget() )
	setWFlags( WType_TopLevel );		// top-level widget

    static int sw = -1, sh = -1;		// screen size

    Display *dpy = x11Display();
    int	     scr = x11Screen();

    bool topLevel = testWFlags(WType_TopLevel);
    bool popup = testWFlags(WType_Popup);
    bool modal = testWFlags(WType_Modal);
    if ( modal )
	setWFlags(WStyle_Dialog);
    bool desktop = testWFlags(WType_Desktop);
    Window root_win = qt_xrootwin(); // ## should be in paintdevice, depends on x11Display and x11Screen
    Window parentw, destroyw = 0;
    WId	   id;

    if ( !window )				// always initialize
	initializeWindow = TRUE;

    if ( popup ) {
	setWFlags(WStyle_StaysOnTop); // a popup stays on top
    }

    if ( sw < 0 ) {				// get the screen size
	sw = DisplayWidth(dpy,scr);
	sh = DisplayHeight(dpy,scr);
    }

    if ( modal || popup || desktop ) {		// these are top-level, too
	topLevel = TRUE;
	setWFlags( WType_TopLevel );
    }

    if ( desktop ) {				// desktop widget
	modal = popup = FALSE;			// force these flags off
	crect.setRect( 0, 0, sw, sh );
    } else if ( topLevel ) {			// calc pos/size from screen
	crect.setRect( sw/4, 3*sh/10, sw/2, 4*sh/10 );
    } else {					// child widget
	crect.setRect( 0, 0, 100, 30 );
    }
    fpos = crect.topLeft();			// default frame rect

    parentw = topLevel ? root_win : parentWidget()->winId();

    XSetWindowAttributes wsa;

    if ( window ) {				// override the old window
	if ( destroyOldWindow )
	    destroyw = winid;
	id = window;
	setWinId( window );
    } else if ( desktop ) {			// desktop widget
	id = (WId)parentw;			// id = root window
	QWidget *otherDesktop = find( id );	// is there another desktop?
	if ( otherDesktop && otherDesktop->testWFlags(WPaintDesktop) ) {
	    otherDesktop->setWinId( 0 );	// remove id from widget mapper
	    setWinId( id );			// make sure otherDesktop is
	    otherDesktop->setWinId( id );	//   found first
	} else {
	    setWinId( id );
	}
    } else {
	if ( x11DefaultVisual() && x11DefaultColormap() ) {
	    id = (WId)qt_XCreateSimpleWindow( this, dpy, parentw,
					 crect.left(), crect.top(),
					 crect.width(), crect.height(),
					 0,
					 black.pixel(),
					 bg_col.pixel() );
	} else {
	    wsa.background_pixel = bg_col.pixel();
	    wsa.border_pixel = black.pixel();
	    wsa.colormap = (Colormap)x11Colormap();
	    id = (WId)qt_XCreateWindow( this, dpy, parentw,
				   crect.left(), crect.top(),
				   crect.width(), crect.height(),
				   0, x11Depth(), InputOutput,
				   (Visual*)x11Visual(),
				   CWBackPixel|CWBorderPixel|CWColormap,
				   &wsa );
	}
	setWinId( id );				// set widget id/handle + hd
    }
#ifdef QT_XFT
    qt_create_ft_draw (x11Display (), id, (Visual *) x11Visual (), x11Colormap());
#endif

    if ( !topLevel ) {
	if ( !testWFlags(WStyle_Customize) )
	    setWFlags( WStyle_NormalBorder | WStyle_Title | WStyle_MinMax | WStyle_SysMenu  );
    } else if ( !(desktop || popup) ) {

	ulong wsa_mask = 0;

	if ( testWFlags(WStyle_Customize) ) {	// customize top-level widget
	    if ( testWFlags(WStyle_NormalBorder) || testWFlags( WStyle_DialogBorder) ) {
		;				// ok, we already have it
	    } else if ( testWFlags( WStyle_NoBorderEx ) ){ // Style_NoBorderEx, sets Motif hint
		// ### this should really be WStyle_NoBorder in 3.0
		struct {
		    ulong flags;
		    ulong functions;
		    ulong decorations;
		    long input_mode;
		    ulong status;
		} hints;
		hints.decorations = 0;
		hints.flags =  (1L << 1); // MWM_HINTS_DECORATIONS;
		XChangeProperty (dpy, id, qt_xa_motif_wm_hints,
				 qt_xa_motif_wm_hints, 32, PropModeReplace,
				 (unsigned char*) &hints, 5 );
	    } else { // Style_NoBorder
		setWFlags( WX11BypassWM ); // ### compatibility
	    }
	    if ( testWFlags(WStyle_Tool) ) {
		wsa.save_under = TRUE;
		wsa_mask |= CWSaveUnder;
	    }
	} else {				// normal top-level widget
	    if ( testWFlags(WStyle_Dialog ) )
		setWFlags( WStyle_NormalBorder | WStyle_Title | WStyle_SysMenu | WStyle_ContextHelp );
	    else
		setWFlags( WStyle_NormalBorder | WStyle_Title | WStyle_MinMax | WStyle_SysMenu  );
	}

	if ( testWFlags( WX11BypassWM ) ) {
	    wsa.override_redirect = TRUE;
	    wsa_mask |= CWOverrideRedirect;
	}
	if ( wsa_mask && initializeWindow )
	    XChangeWindowAttributes( dpy, id, wsa_mask, &wsa );
    }

    if ( !initializeWindow ) {
	// do no initialization
    } else if ( popup ) {			// popup widget
	XSetTransientForHint( dpy, id, parentw ); //?!?! this looks pointless
	wsa.override_redirect = TRUE;
	wsa.save_under = TRUE;
	XChangeWindowAttributes( dpy, id, CWOverrideRedirect | CWSaveUnder,
				 &wsa );
    } else if ( topLevel && !desktop ) {	// top-level widget
	QWidget *p = parentWidget();	// real parent
	if (p)
	    p = p->topLevelWidget();
	if ( testWFlags(WStyle_DialogBorder)
	     || testWFlags(WStyle_StaysOnTop)
	     || testWFlags(WStyle_Dialog)
	     || testWFlags(WStyle_Tool) ) {
	    if ( testWFlags( WStyle_StaysOnTop ) )
		XSetTransientForHint( dpy, id, None );
	    else  if ( p )
		XSetTransientForHint( dpy, id, p->winId() );
	    else				// application-modal
		XSetTransientForHint( dpy, id, root_win );
	}

	// find the real client leader, i.e. a toplevel without parent
	while ( p && p->parentWidget()) {
	    p = p->parentWidget()->topLevelWidget();
	}

	XSizeHints size_hints;
	size_hints.flags = USSize | PSize | PWinGravity;
	size_hints.x = crect.left();
	size_hints.y = crect.top();
	size_hints.width = crect.width();
	size_hints.height = crect.height();
	size_hints.win_gravity = NorthWestGravity;
	char *title = qAppName();
	XWMHints wm_hints;			// window manager hints
	wm_hints.input = True;
	wm_hints.initial_state = NormalState;
	wm_hints.flags = InputHint | StateHint;

	if (p) { // the real client leader (head of the group)
	    wm_hints.window_group = p->winId();
	    wm_hints.flags |= WindowGroupHint;
	}

	XClassHint class_hint;
	class_hint.res_class = title; 	// app name and widget name
	class_hint.res_name = name() ? (char *)name() : title;
	XSetWMProperties( dpy, id, 0, 0, 0, 0, &size_hints, &wm_hints,
			  &class_hint );
	XResizeWindow( dpy, id, crect.width(), crect.height() );
	XStoreName( dpy, id, title );
	Atom protocols[3];
	int n = 0;
	protocols[n++] = qt_wm_delete_window;	// support del window protocol
	protocols[n++] = qt_wm_take_focus;	// support take focus window protocol
	if ( testWFlags( WStyle_ContextHelp ) )
	    protocols[n++] = qt_net_wm_context_help;
	XSetWMProtocols( dpy, id, protocols, n );
    }

    if ( initializeWindow ) {
	wsa.bit_gravity = NorthWestGravity;	// don't erase when resizing
	XChangeWindowAttributes( dpy, id, CWBitGravity, &wsa );
    }

    setWState( WState_MouseTracking );
    setMouseTracking( FALSE );			// also sets event mask
    if ( desktop ) {
	setWState( WState_Visible );
    } else if ( topLevel ) {			// set X cursor
	QCursor *oc = QApplication::overrideCursor();
	if ( initializeWindow )
	    XDefineCursor( dpy, winid, oc ? oc->handle() : cursor().handle() );
	setWState( WState_OwnCursor );
    }

    if ( window ) {				// got window from outside
	XWindowAttributes a;
	XGetWindowAttributes( dpy, window, &a );
	crect.setRect( a.x, a.y, a.width, a.height );
	fpos = crect.topLeft();
	if ( a.map_state == IsUnmapped )
	    clearWState( WState_Visible );
	else
	    setWState( WState_Visible );
	QPaintDeviceX11Data* xd = getX11Data( TRUE );
	xd->x_depth = a.depth;
	xd->x_visual = a.visual;
	xd->x_defvisual = ( XVisualIDFromVisual( a.visual ) ==
			    XVisualIDFromVisual( (Visual*)x11AppVisual() ) );
	xd->x_colormap = a.colormap;
	setX11Data( xd );
	delete xd;
    }

    if ( topLevel ) {
	// declare the widget's object name as window role
	XChangeProperty( dpy, id,
			 qt_window_role, XA_STRING, 8, PropModeReplace,
			 (unsigned char *)name(), qstrlen( name() ) );
	// If we are session managed, inform the window manager about it
	QCString session = qApp->sessionId().latin1();
	if ( !session.isEmpty() ) {
	    XChangeProperty( dpy, id,
			     qt_sm_client_id, XA_STRING, 8, PropModeReplace,
			     (unsigned char *)session.data(), session.length() );
	}
    }

    if ( destroyw )
	qt_XDestroyWindow( this, dpy, destroyw );

}


/*!
  Frees up window system resources.
  Destroys the widget window if \a destroyWindow is TRUE.

  destroy() calls itself recursively for all the child widgets,
  passing \a destroySubWindows for the \a destroyWindow parameter.
  To have more control over destruction of subwidgets,
  destroy subwidgets selectively first.

  This function is usually called from the QWidget destructor.
*/

void QWidget::destroy( bool destroyWindow, bool destroySubWindows )
{
    deactivateWidgetCleanup();
    if ( testWState(WState_Created) ) {
	clearWState( WState_Created );
	if ( children() ) {
	    QObjectListIt it(*children());
	    register QObject *obj;
	    while ( (obj=it.current()) ) {	// destroy all widget children
		++it;
		if ( obj->isWidgetType() )
		    ((QWidget*)obj)->destroy(destroySubWindows,
					     destroySubWindows);
	    }
	}
	if ( mouseGrb == this )
	    releaseMouse();
	if ( keyboardGrb == this )
	    releaseKeyboard();
	if ( testWState( WType_TopLevel ) )
	    qt_deferred_map_take( this );
	if ( testWFlags(WType_Modal) )		// just be sure we leave modal
	    qt_leave_modal( this );
	else if ( testWFlags(WType_Popup) )
	    qApp->closePopup( this );
#ifdef QT_XFT
	qt_destroy_ft_draw (x11Display (), winid);
#endif
	if ( testWFlags(WType_Desktop) ) {
	    if ( acceptDrops() )
		qt_dnd_enable( this, FALSE );
	} else {
	    if ( destroyWindow )
		qt_XDestroyWindow( this, x11Display(), winid );
	}
	setWinId( 0 );

	extern void qPRCleanup( QWidget *widget ); // from qapplication_x11.cpp
	if ( testWState(WState_Reparented) )
	    qPRCleanup(this);
    }
}

/*!
  Reparents the widget.  The widget gets a new \a parent, new widget
  flags (\a f, but as usual, use 0) at a new position in its new
  parent (\a p).

  If \a showIt is TRUE, show() is called once the widget has been
  reparented.

  If the new parent widget is in a different top-level widget, the
  reparented widget and its children are appended to the end of the
  \link setFocusPolicy() TAB chain \endlink of the new parent widget,
  in the same internal order as before.  If one of the moved widgets
  had keyboard focus, reparent() calls clearFocus() for that widget.

  If the new parent widget is in the same top-level widget as the old
  parent, reparent doesn't change the TAB order or keyboard focus.

  \warning Reparenting widgets should be a real exception. In normal
  applications, you will almost never need it. Dynamic masks can be
  achieved much easier and cleaner with classes like QWidgetStack or
  on a higher abstraction level, QWizard.

  \sa getWFlags()
*/

void QWidget::reparent( QWidget *parent, WFlags f, const QPoint &p,
			bool showIt )
{
    extern void qPRCreate( const QWidget *, Window );
    Display *dpy = x11Display();
    QCursor oldcurs;
    bool setcurs=testWState(WState_OwnCursor);
    if ( setcurs ) {
	oldcurs = cursor();
	unsetCursor();
    }
    bool topdata_dnd = FALSE;
    bool accept_drops = acceptDrops();
    if ( accept_drops )
	setAcceptDrops( FALSE ); // dnd unregister (we will register again below)
    if ( isTopLevel() ) {
	topdata_dnd = topData()->dnd;
	topData()->dnd = 0; // unregister XDND atom
    }

    QWidget* oldtlw = topLevelWidget();
    WId old_winid = winid;
    if ( testWFlags(WType_Desktop) )
	old_winid = 0;

    setWinId( 0 );
    if ( isTopLevel() )
	topData()->parentWinId = 0;


    if ( parentObj ) {				// remove from parent
	parentObj->removeChild( this );
    }
    if ( parent ) {				// insert into new parent
	parentObj = parent;			// avoid insertChild warning
	parent->insertChild( this );
    }
    bool     enable = isEnabled();		// remember status
    FocusPolicy fp = focusPolicy();
    QSize    s	    = size();
    QPixmap *bgp    = (QPixmap *)backgroundPixmap();
    QColor   bgc    = bg_col;			// save colors
    QString capt= caption();
    widget_flags = f;
    clearWState( WState_Created | WState_Visible | WState_ForceHide );
    if ( isTopLevel() || (!parent || parent->isVisibleTo( 0 ) ) )
	setWState( WState_ForceHide );	// new widgets do not show up in already visible parents
    create();
    const QObjectList *chlist = children();
    if ( chlist ) {				// reparent children
	QObjectListIt it( *chlist );
	QObject *obj;
	while ( (obj=it.current()) ) {
	    if ( obj->isWidgetType() ) {
		QWidget *w = (QWidget *)obj;
		if ( !w->isTopLevel() ) {
		    XReparentWindow( x11Display(), w->winId(), winId(),
				     w->geometry().x(), w->geometry().y() );
		} else if ( w->isPopup()
			    || w->testWFlags(WStyle_DialogBorder)
			    || w->testWFlags(WStyle_Dialog)
			    || w->testWFlags(WStyle_Tool) ) {
		    XSetTransientForHint( x11Display(), w->winId(), winId() );
		}
	    }
	    ++it;
	}
    }
    qPRCreate( this, old_winid );
    if ( bgp )
	XSetWindowBackgroundPixmap( dpy, winid, bgp->handle() );
    else
	XSetWindowBackground( dpy, winid, bgc.pixel() );
    setGeometry( p.x(), p.y(), s.width(), s.height() );
    setEnabled( enable );
    setFocusPolicy( fp );
    if ( !capt.isNull() ) {
	extra->topextra->caption = QString::null;
	setCaption( capt );
    }
    if ( showIt )
	show();
    if ( old_winid )
	qt_XDestroyWindow( this, dpy, old_winid );

    if ( setcurs ) {
	setCursor(oldcurs);
    }

    reparentFocusWidgets( oldtlw );

    // re-register dnd
    if ( accept_drops )
	setAcceptDrops( TRUE );
    if (topdata_dnd)
	qt_dnd_enable(this, topdata_dnd);

    QCustomEvent e( QEvent::Reparent, 0 );
    QApplication::sendEvent( this, &e );
}


/*!
  Translates the widget coordinate \e pos to global screen coordinates.
  For example, \code mapToGlobal(QPoint(0,0))\endcode would give the
  global coordinates of the top-left pixel of the widget.
  \sa mapFromGlobal() mapTo() mapToParent()
*/

QPoint QWidget::mapToGlobal( const QPoint &pos ) const
{
    int	   x, y;
    Window child;
    XTranslateCoordinates( x11Display(), winId(),
			   QApplication::desktop()->winId(),
			   pos.x(), pos.y(), &x, &y, &child );
    return QPoint( x, y );
}

/*!
  Translates the global screen coordinate \e pos to widget coordinates.
  \sa mapToGlobal() mapFrom() mapFromParent()
*/

QPoint QWidget::mapFromGlobal( const QPoint &pos ) const
{
    int	   x, y;
    Window child;
    XTranslateCoordinates( x11Display(), QApplication::desktop()->winId(),
			   winId(), pos.x(), pos.y(), &x, &y, &child );
    return QPoint( x, y );
}

/*!
  When a widget gets focus, it should call setMicroFocusHint for some
  appropriate position and size - \a x, \a y and \a w by \a h.  This
  has no \e visual effect, it just provides hints to any
  system-specific input handling tools.

  The \a text argument should be TRUE if this is a position for text
  input.

  In the Windows version of Qt, this method sets the system caret, which is
  used for user Accessibility focus handling.  If \a text is TRUE, it also
  sets the IME composition window in Far East Asian language input systems.

  In the X11 version of Qt, if \a text is TRUE, this method sets the
  XIM "spot" point for complex language input handling.

  \sa microFocusHint()
*/
void QWidget::setMicroFocusHint(int x, int y, int width, int height, bool text)
{
#ifndef NO_XIM
    if ( text ) {
	QWidget* tlw = topLevelWidget();
	if ( tlw->extra && tlw->extra->topextra &&
	     tlw->extra->topextra->xic ) {
	    QPoint p( x, y ); // ### use mapTo()
	    QWidget * w = this;
	    QWidget * tlw = topLevelWidget();
	    while ( w != tlw ) {
		p = w->mapToParent( p );
		w = w->parentWidget();
	    }
	    XPoint spot;
	    spot.x = p.x();
	    spot.y = p.y()+height;
	    XIC xic = (XIC)tlw->extra->topextra->xic;
	    XVaNestedList preedit_attr;
	    preedit_attr = XVaCreateNestedList(0, XNSpotLocation, &spot, 0);
	    XSetICValues(xic, XNPreeditAttributes, preedit_attr, 0);
	    XFree(preedit_attr);
	}
    }
#endif
    if ( QRect( x, y, width, height ) != microFocusHint() )
	extraData()->micro_focus_hint.setRect( x, y, width, height );
}

#ifndef NO_XIM
static XFontSet fixed_fontset = 0; // leaked once

static
void cleanup_ffs()
{
    if ( fixed_fontset )
        XFreeFontSet(QPaintDevice::x11AppDisplay(), fixed_fontset);
    fixed_fontset = 0;
}

static
XFontSet xic_fontset(void* qfs, int pt)
{
    XFontSet fontset = (XFontSet)qfs;
    if ( fontset )
	return fontset;

    // ##### TODO: this case cannot happen if we ensure that
    //              the default font etc. are for this locale.
    char** missing=0;
    int nmissing;
    if ( !fixed_fontset ) {
	QCString n;
	n.sprintf( "-*-Helvetica-*-*-normal-*-*-%d-*-*-*-*-*-*,"
		   "-*-*-*-*-normal-*-*-%d-*-*-*-*-*-*,"
		   "-*-*-*-*-*-*-*-%d-*-*-*-*-*-*",
		   pt*10,
		   pt*10,
		   pt*10 );
	fixed_fontset = XCreateFontSet( QPaintDevice::x11AppDisplay(), n,
					&missing, &nmissing, 0 );
	qAddPostRoutine(cleanup_ffs);
    }
    return fixed_fontset;
}
#endif


void QWidget::setFontSys()
{
#ifndef NO_XIM
    QWidget* tlw = topLevelWidget();
    if ( tlw->extra && tlw->extra->topextra && tlw->extra->topextra->xic ) {
	XIC xic = (XIC)tlw->extra->topextra->xic;

	XFontSet fontset = xic_fontset( fontMetrics().fontSet(),
					font().pointSize());

	XVaNestedList preedit_att = XVaCreateNestedList( 0, XNFontSet, fontset,
							 NULL );
	XVaNestedList status_att = XVaCreateNestedList( 0, XNFontSet, fontset,
							NULL );

	XSetICValues( xic,
		      XNPreeditAttributes, preedit_att,
		      XNStatusAttributes, status_att,
		      0 );

	XFree(preedit_att);
	XFree(status_att);
    }
#endif
}


void QWidget::setBackgroundColorDirect( const QColor &color )
{
    QColor old = bg_col;
    bg_col = color;
    if ( extra && extra->bg_pix ) {		// kill the background pixmap
	delete extra->bg_pix;
	extra->bg_pix = 0;
    }
    XSetWindowBackground( x11Display(), winId(), bg_col.pixel() );
    backgroundColorChange( old );
}

static int allow_null_pixmaps = 0;


void QWidget::setBackgroundPixmapDirect( const QPixmap &pixmap )
{
    QPixmap old;
    if ( extra && extra->bg_pix )
	old = *extra->bg_pix;
    if ( !allow_null_pixmaps && pixmap.isNull() ) {
	XSetWindowBackground( x11Display(), winId(), bg_col.pixel() );
	if ( extra && extra->bg_pix ) {
	    delete extra->bg_pix;
	    extra->bg_pix = 0;
	}
    } else {
	QPixmap pm = pixmap;
	if (!pm.isNull()) {
	    if ( pm.depth() == 1 && QPixmap::defaultDepth() > 1 ) {
		pm = QPixmap( pixmap.size() );
		bitBlt( &pm, 0, 0, &pixmap, 0, 0, pm.width(), pm.height() );
	    }
	}
	if ( extra && extra->bg_pix )
	    delete extra->bg_pix;
	else
	    createExtra();
	extra->bg_pix = new QPixmap( pm );
	XSetWindowBackgroundPixmap( x11Display(), winId(), pm.handle() );
	if ( testWFlags(WType_Desktop) )	// save rootinfo later
	    qt_updated_rootinfo();
    }
    if ( !allow_null_pixmaps ) {
	backgroundPixmapChange( old );
    }
}


/*!
  Sets the window-system background of the widget to nothing.

  Note that `nothing' is actually a pixmap that isNull(), thus you
  can check for an empty background by checking backgroundPixmap().

  \sa setBackgroundPixmap(), setBackgroundColor()

  This class should \e NOT be made virtual - it is an alternate usage
  of setBackgroundPixmap().
*/
void QWidget::setBackgroundEmpty()
{
    allow_null_pixmaps++;
    setBackgroundPixmap(QPixmap());
    allow_null_pixmaps--;
}


void QWidget::setBackgroundX11Relative()
{
    XSetWindowBackgroundPixmap( x11Display(), winId(), ParentRelative );
}


/*!
  Sets the widget cursor shape to \e cursor.

  The mouse cursor will assume this shape when it's over this widget.
  See a list of predefined cursor objects with a range of useful
  shapes in the QCursor documentation.

  An editor widget would for example use an I-beam cursor:
  \code
    setCursor( ibeamCursor );
  \endcode

  \sa cursor(), unsetCursor(), QApplication::setOverrideCursor()
*/

void QWidget::setCursor( const QCursor &cursor )
{
    if ( cursor.handle() != arrowCursor.handle()
	 || (extra && extra->curs) ) {
	createExtra();
	delete extra->curs;
	extra->curs = new QCursor(cursor);
    }
    setWState( WState_OwnCursor );
    QCursor *oc = QApplication::overrideCursor();
    XDefineCursor( x11Display(), winId(),
		   oc ? oc->handle() : cursor.handle() );
    XFlush( x11Display() );
}


/*!
  Unset the cursor for this widget. The widget will use the cursor of
  its parent from now on.

  This functions does nothing for top-level windows.

  \sa cursor(), setCursor(), QApplication::setOverrideCursor()
 */

void QWidget::unsetCursor()
{
    if ( !isTopLevel() ) {
	if (extra ) {
	    delete extra->curs;
	    extra->curs = 0;
	}
	clearWState( WState_OwnCursor );
	XDefineCursor( x11Display(), winId(), None );
	XFlush( x11Display() );
    }
}

static XTextProperty*
qstring_to_xtp( const QString& s )
{
    static XTextProperty tp = { 0, 0, 0, 0 };
    static bool free_prop = TRUE; // we can't free tp.value in case it references
    // the data of the static QCString below.
    if ( tp.value ) {
	if ( free_prop )
	    XFree( tp.value );
	tp.value = 0;
	free_prop = TRUE;
    }

    static const QTextCodec* mapper = QTextCodec::codecForLocale();
    int errCode = 0;
    if ( mapper ) {
	QCString mapped = mapper->fromUnicode(s);
	char* tl[2];
	tl[0] = mapped.data();
	tl[1] = 0;
	errCode = XmbTextListToTextProperty( QPaintDevice::x11AppDisplay(),
					     tl, 1, XStdICCTextStyle, &tp );
#if defined(DEBUG)
	if ( errCode < 0 )
	    qDebug( "qstring_to_xtp result code %d", errCode );
#endif
    }
    if ( !mapper || errCode < 0 ) {
	static QCString qcs;
	qcs = s.ascii();
	tp.value = (uchar*)qcs.data();
	tp.encoding = XA_STRING;
	tp.format = 8;
	tp.nitems = qcs.length();
	free_prop = FALSE;
    }

    // ### If we knew WM could understand unicode, we could use
    // ### a much simpler, cheaper encoding...
    /*
	tp.value = (XChar2b*)s.unicode();
	tp.encoding = XA_UNICODE; // wish
	tp.format = 16;
	tp.nitems = s.length();
    */

    return &tp;
}

/*!
  Sets the window caption (title) to \a caption.
  \sa caption(), setIcon(), setIconText()
*/

void QWidget::setCaption( const QString &caption )
{
    if ( QWidget::caption() == caption )
	return; // for less flicker
    topData()->caption = caption;
    XSetWMName( x11Display(), winId(), qstring_to_xtp(caption) );
    QCustomEvent e( QEvent::CaptionChange, 0 );
    QApplication::sendEvent( this, &e );
}

/*!
  Sets the window icon to \a pixmap.
  \sa icon(), setIconText(), setCaption(),
      \link appicon.html Setting the Application Icon\endlink
*/

void QWidget::setIcon( const QPixmap &pixmap )
{
    if ( extra && extra->topextra ) {
	delete extra->topextra->icon;
	extra->topextra->icon = 0;
    } else {
	createTLExtra();
    }
    Pixmap icon_pixmap = 0;
    Pixmap mask_pixmap = 0;
    if ( !pixmap.isNull() ) {
	QPixmap* pm = new QPixmap( pixmap );
	extra->topextra->icon = pm;
  	if ( !pm->mask() )
  	    pm->setMask( pm->createHeuristicMask() ); // may do detach()
	icon_pixmap = pm->handle();
  	if ( pm->mask() )
  	    mask_pixmap = pm->mask()->handle();
    }
    XWMHints *h = XGetWMHints( x11Display(), winId() );
    XWMHints  wm_hints;
    bool got_hints = h != 0;
    if ( !got_hints ) {
	h = &wm_hints;
	h->flags = 0;
    }
    h->icon_pixmap = icon_pixmap;
    h->icon_mask = mask_pixmap;
    h->flags |= IconPixmapHint | IconMaskHint;
    XSetWMHints( x11Display(), winId(), h );
    if ( got_hints )
	XFree( (char *)h );
    QCustomEvent e( QEvent::IconChange, 0 );
    QApplication::sendEvent( this, &e );
}


/*!
  Sets the text of the window's icon to \e iconText.
  \sa iconText(), setIcon(), setCaption()
*/

void QWidget::setIconText( const QString &iconText )
{
    createTLExtra();
    extra->topextra->iconText = iconText;
    XSetIconName( x11Display(), winId(), iconText.utf8() );
    XSetWMIconName( x11Display(), winId(), qstring_to_xtp(iconText) );
}


void QWidget::setMouseTracking( bool enable )
{
    bool gmt = QApplication::hasGlobalMouseTracking();
    if ( enable == testWState(WState_MouseTracking) && !gmt )
	return;
    uint m = (enable || gmt) ? (uint)PointerMotionMask : 0;
    if ( enable )
	setWState( WState_MouseTracking );
    else
	clearWState( WState_MouseTracking );
    if ( testWFlags(WType_Desktop) ) {		// desktop widget?
	QWidget* main_desktop = find( winId() );
	if ( main_desktop->testWFlags(WPaintDesktop) )
	    XSelectInput( x11Display(), winId(),
			  stdDesktopEventMask | ExposureMask );
	else
	    XSelectInput( x11Display(), winId(), stdDesktopEventMask );
    } else {
	XSelectInput( x11Display(), winId(),
		      m | stdWidgetEventMask );
    }
}


/*!
  Grabs the mouse input.

  This widget will be the only one to receive mouse events until
  releaseMouse() is called.

  \warning Grabbing the mouse might lock the terminal.

  It is almost never necessary to grab the mouse when using Qt since
  Qt grabs and releases it sensibly.  In particular, Qt grabs the
  mouse when a button is pressed and keeps it until the last button is
  released.

  Beware that only widgets actually shown on the screen may grab the
  mouse input.

  \sa releaseMouse(), grabKeyboard(), releaseKeyboard()
*/

void QWidget::grabMouse()
{
    if ( !qt_nograb() ) {
	if ( mouseGrb )
	    mouseGrb->releaseMouse();
#if defined(CHECK_STATE)
	int status =
#endif
	XGrabPointer( x11Display(), winId(), TRUE,
		      (uint)( ButtonPressMask | ButtonReleaseMask |
			      PointerMotionMask | EnterWindowMask |
			      LeaveWindowMask ),
		      GrabModeAsync, GrabModeAsync,
		      None, None, qt_x_time );
#if defined(CHECK_STATE)
	if ( status ) {
	    const char *s =
		status == GrabNotViewable ? "\"GrabNotViewable\"" :
		status == AlreadyGrabbed  ? "\"AlreadyGrabbed\"" :
		status == GrabFrozen      ? "\"GrabFrozen\"" :
		status == GrabInvalidTime ? "\"GrabInvalidTime\"" :
					    "<?>";
	    qWarning( "Grabbing the mouse failed with %s", s );
	}
#endif
	mouseGrb = this;
    }
}

/*!
  Grabs the mouse input and changes the cursor shape.

  The cursor will assume shape \e cursor (for as long as the mouse focus is
  grabbed) and this widget will be the only one to receive mouse events
  until releaseMouse() is called().

  \warning Grabbing the mouse might lock the terminal.

  \sa releaseMouse(), grabKeyboard(), releaseKeyboard(), setCursor()
*/

void QWidget::grabMouse( const QCursor &cursor )
{
    if ( !qt_nograb() ) {
	if ( mouseGrb )
	    mouseGrb->releaseMouse();
#if defined(CHECK_STATE)
	int status =
#endif
	XGrabPointer( x11Display(), winId(), TRUE,
		      (uint)(ButtonPressMask | ButtonReleaseMask |
			     PointerMotionMask | EnterWindowMask | LeaveWindowMask),
		      GrabModeAsync, GrabModeAsync,
		      None, cursor.handle(), qt_x_time );
#if defined(CHECK_STATE)
	if ( status ) {
	    const char *s =
		status == GrabNotViewable ? "\"GrabNotViewable\"" :
		status == AlreadyGrabbed  ? "\"AlreadyGrabbed\"" :
		status == GrabFrozen      ? "\"GrabFrozen\"" :
		status == GrabInvalidTime ? "\"GrabInvalidTime\"" :
					    "<?>";
	    qWarning( "Grabbing the mouse failed with %s", s );
	}
#endif
	mouseGrb = this;
    }
}

/*!
  Releases the mouse grab.

  \sa grabMouse(), grabKeyboard(), releaseKeyboard()
*/

void QWidget::releaseMouse()
{
    if ( !qt_nograb() && mouseGrb == this ) {
	XUngrabPointer( x11Display(),  qt_x_time );
	XFlush( x11Display() );
	mouseGrb = 0;
    }
}

/*!
  Grabs all keyboard input.

  This widget will receive all keyboard events, independent of the active
  window.

  \warning Grabbing the keyboard might lock the terminal.

  \sa releaseKeyboard(), grabMouse(), releaseMouse()
*/

void QWidget::grabKeyboard()
{
    if ( !qt_nograb() ) {
	if ( keyboardGrb )
	    keyboardGrb->releaseKeyboard();
	XGrabKeyboard( x11Display(), winid, TRUE, GrabModeAsync, GrabModeAsync,
		       qt_x_time );
	keyboardGrb = this;
    }
}

/*!
  Releases the keyboard grab.

  \sa grabKeyboard(), grabMouse(), releaseMouse()
*/

void QWidget::releaseKeyboard()
{
    if ( !qt_nograb() && keyboardGrb == this ) {
	XUngrabKeyboard( x11Display(), qt_x_time );
	keyboardGrb = 0;
    }
}


/*!
  Returns a pointer to the widget that is currently grabbing the
  mouse input.

  If no widget in this application is currently grabbing the mouse, 0 is
  returned.

  \sa grabMouse(), keyboardGrabber()
*/

QWidget *QWidget::mouseGrabber()
{
    return mouseGrb;
}

/*!
  Returns a pointer to the widget that is currently grabbing the
  keyboard input.

  If no widget in this application is currently grabbing the keyboard, 0
  is returned.

  \sa grabMouse(), mouseGrabber()
*/

QWidget *QWidget::keyboardGrabber()
{
    return keyboardGrb;
}


/*!
  Sets the top-level widget containing this widget to be the active
  window.

  An active window is a visible top-level window that has the keyboard input
  focus.

  This function performs the same operation as clicking the mouse on
  the title bar of a top-level window. On X11, the result depends on
  the Window Manager. If you want to ensure that the window is stacked
  on top as well, call raise() in addition. Note that the window has be
  to visible, otherwise setActiveWindow() has no effect.

  On Windows, if you are calling this when the application is not
  currently the active one then it will not make it the active window.  It
  will flash the task bar entry blue to indicate that the window has done
  something.  This is due to Microsoft not allowing an application to
  interrupt what the user is currently doing in another application.

  \sa isActiveWindow(), topLevelWidget(), show()
*/

void QWidget::setActiveWindow()
{
    QWidget *tlw = topLevelWidget();
    if ( tlw->isVisible() && !tlw->topData()->embedded ) {
	XSetInputFocus( x11Display(), tlw->winId(), RevertToNone, qt_x_time);

#ifndef NO_XIM
	if (tlw->topData()->xic)
	    XSetICFocus( (XIC) tlw->topData()->xic);
#endif
    }
}


/*!
  Updates the widget unless updates are disabled or the widget is hidden.

  Updating the widget will erase the widget contents and generate an
  appropriate paint event for the invalidated region. The paint event
  is processed after the program has returned to the main event loop.
  Calling update() many times in a row will generate a single paint
  event.

  If the widgets sets the WRepaintNoErase flag, update() will not erase
  its contents.

  \sa repaint(), paintEvent(), setUpdatesEnabled(), erase(), setWFlags()
*/

void QWidget::update()
{
    if ( (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
	QApplication::postEvent( this, new QPaintEvent( visibleRect(),
		!testWFlags(WRepaintNoErase) ) );
    }
}

/*!
  Updates a rectangle (\e x, \e y, \e w, \e h) inside the widget
  unless updates are disabled or the widget is hidden.

  Updating the widget erases the widget area \e (x,y,w,h) and generate
  an appropriate paint event for the invalidated region. The paint
  event is processed after the program has returned to the main event
  loop.  Calling update() many times in a row will generate a single
  paint event.

  If \e w is negative, it is replaced with <code>width() - x</code>.
  If \e h is negative, it is replaced width <code>height() - y</code>.


  If the widgets sets the WRepaintNoErase flag, update() will not erase
  its contents.

  \sa repaint(), paintEvent(), setUpdatesEnabled(), erase()
*/

void QWidget::update( int x, int y, int w, int h )
{
    if ( w && h &&
	 (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
	if ( w < 0 )
	    w = crect.width()  - x;
	if ( h < 0 )
	    h = crect.height() - y;
	if ( w != 0 && h != 0 )
	    QApplication::postEvent( this,
	        new QPaintEvent( visibleRect().intersect(QRect(x,y,w,h)),
				 !testWFlags( WRepaintNoErase ) ) );
    }
}

/*!
  \overload void QWidget::update( const QRect &r )
*/

/*!
  \overload void QWidget::repaint( bool erase )

  This version repaints the entire widget.
*/

/*!
  \overload void QWidget::repaint()

  This version erases and repaints the entire widget.
*/

/*!
  Repaints the widget directly by calling paintEvent() directly,
  unless updates are disabled or the widget is hidden.

  Erases the widget area  \e (x,y,w,h) if \e erase is TRUE.

  If \e w is negative, it is replaced with <code>width() - x</code>.
  If \e h is negative, it is replaced width <code>height() - y</code>.

  Use repaint if your widget needs to be repainted immediately, for
  example when doing some animation. In all other cases, update() is
  to be preferred. Calling update() many times in a row will generate
  a single paint event.

  \warning If you call repaint() in a function which may itself be called
  from paintEvent(), you may see infinite recursion. The update() function
  never generates recursion.

  \sa update(), paintEvent(), setUpdatesEnabled(), erase()
*/

void QWidget::repaint( int x, int y, int w, int h, bool erase )
{
    if ( (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
	if ( w < 0 )
	    w = crect.width()  - x;
	if ( h < 0 )
	    h = crect.height() - y;
	QRect r(x,y,w,h);
	if ( r.isEmpty() )
	    return; // nothing to do
	QPaintEvent e( r, erase );
 	if ( r != rect() )
	    qt_set_paintevent_clipping( this, r );
	if ( erase && w != 0 && h != 0 ) {
	    if ( backgroundOrigin() == WidgetOrigin )
		XClearArea( x11Display(), winId(), x, y, w, h, FALSE );
	    else
		this->erase( x, y, w, h);
	}
	QApplication::sendEvent( this, &e );
	qt_clear_paintevent_clipping();
    }
}

/*!
  Repaints the widget directly by calling paintEvent() directly,
  unless updates are disabled or the widget is hidden.

  Erases the widget region  \a reg if \a erase is TRUE.

  Use repaint if your widget needs to be repainted immediately, for
  example when doing some animation. In all other cases, update() is
  to be preferred. Calling update() many times in a row will generate
  a single paint event.

  \warning If you call repaint() in a function which may itself be called
  from paintEvent(), you may see infinite recursion. The update() function
  never generates recursion.

  \sa update(), paintEvent(), setUpdatesEnabled(), erase()
*/

void QWidget::repaint( const QRegion& reg, bool erase )
{
    if ( (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
	QPaintEvent e( reg );
	qt_set_paintevent_clipping( this, reg );
	if ( erase )
	    this->erase(reg);
	QApplication::sendEvent( this, &e );
	qt_clear_paintevent_clipping();
    }
}

/*!
  \overload void QWidget::repaint( const QRect &r, bool erase )
*/


/*!
  \internal
  Platform-specific part of QWidget::show().
*/

void QWidget::showWindow()
{

    if ( isTopLevel()  ) {
	int sm = topData()->showMode;
	if ( sm ) { // handles minimize and reset
	    XWMHints *h = XGetWMHints( x11Display(), winId() );
	    XWMHints  wm_hints;
	    bool got_hints = h != 0;
	    if ( !got_hints ) {
		h = &wm_hints;
		h->flags = 0;
	    }
	    h->initial_state = sm == 1? IconicState : NormalState;
	    h->flags |= StateHint;
	    XSetWMHints( x11Display(), winId(), h );
	    if ( got_hints )
		XFree( (char *)h );
	    topData()->showMode = sm == 1?3:0; // trigger reset to normal state next time
	}
	if ( topData()->parentWinId && topData()->parentWinId != qt_xrootwin() && !isMinimized() ) {
	    qt_deferred_map_add( this );
	    return;
	}
    }
    XMapWindow( x11Display(), winId() );
}


/*!
  \internal
  Platform-specific part of QWidget::hide().
*/

void QWidget::hideWindow()
{
    clearWState( WState_Exposed );
    deactivateWidgetCleanup();
    if ( isTopLevel() ) {
	qt_deferred_map_take( this );
	if ( winId() ) // in nsplugin, may be 0
	    XWithdrawWindow( x11Display(), winId(), x11Screen() );
	crect.moveTopLeft( fpos );
	topData()->fsize = crect.size();
    } else {
	if ( winId() ) // in nsplugin, may be 0
	    XUnmapWindow( x11Display(), winId() );
    }
}


/*!
  Shows the widget minimized, as an icon.

  Calling this function has no effect for other than \link isTopLevel()
  top-level widgets\endlink.

  \sa showNormal(), showMaximized(), show(), hide(), isVisible(), isMinimized()
*/

void QWidget::showMinimized()
{
    if ( isTopLevel() ) {
	if ( isVisible() )
	    XIconifyWindow( x11Display(), winId(), x11Screen() );
	else {
	    topData()->showMode = 1;
	    show();
	    clearWState( WState_Visible );
	    sendHideEventsToChildren(TRUE);
	}
    }
    QCustomEvent e( QEvent::ShowMinimized, 0 );
    QApplication::sendEvent( this, &e );
}

/*!
  Returns TRUE if this widget is a top-level widget that is minimized
  (iconified), or else FALSE.

  \sa showMinimized(), isVisible(), show(), hide(), showNormal()
 */
bool QWidget::isMinimized() const
{
    return qt_wstate_iconified( winId() );
}

bool QWidget::isMaximized() const
{
    return testWState(WState_Maximized);
}

// ### ### this really needs to wait for a maximum of 0.N seconds
void qt_wait_for_window_manager( QWidget* w )
{
    QApplication::flushX();
    XEvent ev;
    while (!XCheckTypedWindowEvent( w->x11Display(), w->winId(), ReparentNotify, &ev )) {
	if ( XCheckTypedWindowEvent( w->x11Display(), w->winId(), MapNotify, &ev ) )
	    break;
    }
    qApp->x11ProcessEvent( &ev );
}

/*!
  Shows the widget maximized.

  Calling this function has no effect for other than \link isTopLevel()
  top-level widgets\endlink.

  On X11, this function may not work properly with certain window
  managers. See the \link geometry.html Window Geometry
  documentation\endlink for details on why.

  \sa showNormal(), showMinimized(), show(), hide(), isVisible()
*/

void QWidget::showMaximized()
{
    if ( testWFlags(WType_TopLevel) ) {
	Display *dpy = x11Display();
	int scr = x11Screen();
	int sw = DisplayWidth(dpy,scr);
	int sh = DisplayHeight(dpy,scr);
	if ( topData()->normalGeometry.width() < 0 )
	    topData()->normalGeometry = geometry();
	if ( !topData()->parentWinId && !isVisible() ) {
	    setGeometry(0, 0, sw, sh );
	    show();
	    qt_wait_for_window_manager( this );
	}
	sw -= frameGeometry().width() - width();
	sh -= frameGeometry().height() - height();
	resize( sw, sh );
    }
    show();
    QCustomEvent e( QEvent::ShowMaximized, 0 );
    QApplication::sendEvent( this, &e );
    setWState(WState_Maximized);
}

/*!
  Restores the widget after it has been maximized or minimized.

  Calling this function has no effect for other than \link isTopLevel()
  top-level widgets\endlink.

  \sa showMinimized(), showMaximized(), show(), hide(), isVisible()
*/

void QWidget::showNormal()
{
    if ( isTopLevel() ) {
	if ( topData()->fullscreen ) {
	    reparent( 0, WType_TopLevel, QPoint(0,0) );
	    topData()->fullscreen = 0;
	}
	QRect r = topData()->normalGeometry;
	if ( r.width() >= 0 ) {
	    // the widget has been maximized
	    topData()->normalGeometry = QRect(0,0,-1,-1);
	    resize( r.size() );
	    move( r.topLeft() );
	}
    }
    show();
    QCustomEvent e( QEvent::ShowNormal, 0 );
    QApplication::sendEvent( this, &e );
}


/*!
  Raises this widget to the top of the parent widget's stack.

  If there are any siblings of this widget that overlap it on the screen,
  this widget will be visually in front of its siblings afterwards.

  \sa lower(), stackUnder()
*/

void QWidget::raise()
{
    QWidget *p = parentWidget();
    if ( p && p->childObjects && p->childObjects->findRef(this) >= 0 )
	p->childObjects->append( p->childObjects->take() );
    XRaiseWindow( x11Display(), winId() );
}

/*!
  Lowers the widget to the bottom of the parent widget's stack.

  If there are siblings of this widget that overlap it on the screen, this
  widget will be obscured by its siblings afterwards.

  \sa raise(), stackUnder()
*/

void QWidget::lower()
{
    QWidget *p = parentWidget();
    if ( p && p->childObjects && p->childObjects->findRef(this) >= 0 )
	p->childObjects->insert( 0, p->childObjects->take() );
    XLowerWindow( x11Display(), winId() );
}


/*!
  Places the widget under \a w in the parent widget's stack.

  To make this work, the widget itself and \a w have to be siblings.

  \sa raise(), lower()
*/
void QWidget::stackUnder( QWidget* w)
{
    QWidget *p = parentWidget();
    if ( !w || isTopLevel() || p != w->parentWidget() || this == w )
	return;
    if ( p && p->childObjects && p->childObjects->findRef(w) >= 0 && p->childObjects->findRef(this) >= 0 ) {
	p->childObjects->take();
	p->childObjects->insert( p->childObjects->findRef(w), this );
    }
    Window stack[2];
    stack[0] = w->winId();;
    stack[1] = winId();
    XRestackWindows( x11Display(), stack, 2 );
}



/*
  The global variable qwidget_tlw_gravity defines the window gravity of
  the next top level window to be created. We do this when setting the
  main widget's geometry and the "-geometry" command line option contains
  a negative position.
*/

int qwidget_tlw_gravity = NorthWestGravity;

static void do_size_hints( QWidget* widget, QWExtra *x )
{
    XSizeHints s;
    s.flags = 0;
    if ( x ) {
	s.x = widget->x();
	s.y = widget->y();
	s.width = widget->width();
	s.height = widget->height();
	if ( x->minw > 0 || x->minh > 0 ) {	// add minimum size hints
	    s.flags |= PMinSize;
	    s.min_width  = x->minw;
	    s.min_height = x->minh;
	}
	if ( x->maxw < QWIDGETSIZE_MAX || x->maxh < QWIDGETSIZE_MAX ) {
	    s.flags |= PMaxSize;		// add maximum size hints
	    s.max_width  = x->maxw;
	    s.max_height = x->maxh;
	}
	if ( x->topextra &&
	   (x->topextra->incw > 0 || x->topextra->inch > 0) )
	{					// add resize increment hints
	    s.flags |= PResizeInc | PBaseSize;
	    s.width_inc = x->topextra->incw;
	    s.height_inc = x->topextra->inch;
	    s.base_width = x->topextra->basew;
	    s.base_height = x->topextra->baseh;
	}

	if ( x->topextra && x->topextra->uspos) {
	    s.flags |= USPosition;
	    s.flags |= PPosition;
	}
	if ( x->topextra && x->topextra->ussize) {
	    s.flags |= USSize;
	    s.flags |= PSize;
	}
    }
    s.flags |= PWinGravity;
    s.win_gravity = qwidget_tlw_gravity;	// usually NorthWest
    qwidget_tlw_gravity = NorthWestGravity;	// reset in case it was set
    XSetWMNormalHints( widget->x11Display(), widget->winId(), &s );
}


void QWidget::internalSetGeometry( int x, int y, int w, int h, bool isMove )
{
    Display *dpy = x11Display();

    if ( testWFlags(WType_Desktop) )
	return;
    clearWState(WState_Maximized);
    if ( extra ) {				// any size restrictions?
	w = QMIN(w,extra->maxw);
	h = QMIN(h,extra->maxh);
	w = QMAX(w,extra->minw);
	h = QMAX(h,extra->minh);
    }
    if ( w < 1 )				// invalid size
	w = 1;
    if ( h < 1 )
	h = 1;
    QPoint oldPos( pos() );
    QSize oldSize( size() );
    QRect oldGeom( crect );
    QRect  r( x, y, w, h );

    // We only care about stuff that changes the geometry, or may
    // cause the window manager to change its state
    if ( !isTopLevel() && oldGeom == r )
	return;

    setCRect( r );

    bool isResize = size() != oldSize;

    if ( isTopLevel() ) {
	if ( isMove )
	    topData()->uspos = 1;
	if ( isResize )
	    topData()->ussize = 1;
	do_size_hints( this, extra );
    }

    if ( isMove )
	XMoveResizeWindow( dpy, winid, pos().x(), pos().y(), w, h ); // pos() is right according to ICCCM 4.1.5
    else if ( isResize )
	XResizeWindow( dpy, winid, w, h );

    if ( isVisible() ) {
	if ( isMove && pos() != oldPos ) {
	    QMoveEvent e( pos(), oldPos );
	    QApplication::sendEvent( this, &e );
	}
	if ( isResize ) {

	    // set config pending only on resize, see qapplication_x11.cpp, translateConfigEvent()
	    setWState( WState_ConfigPending );

	    QResizeEvent e( size(), oldSize );
	    QApplication::sendEvent( this, &e );
	}
    } else {
	if ( isMove && pos() != oldPos )
	    QApplication::postEvent( this,
				     new QMoveEvent( pos(), oldPos ) );
	if ( isResize )
	    QApplication::postEvent( this,
				     new QResizeEvent( size(), oldSize ) );
    }
}


/*!
  \overload void QWidget::setMinimumSize( const QSize &size )
*/

/*!
  Sets the minimum size of the widget to \e w by \e h pixels.

  The widget cannot be resized to a smaller size than the minimum widget
  size. The widget's size is forced to the minimum size if the current
  size is smaller.

  If you use a layout inside the widget, the minimum size will be set by the layout and
  not by setMinimumSize, unless you set the layouts resize mode to QLayout::FreeResize.

  \sa minimumSize(), setMaximumSize(), setSizeIncrement(), resize(), size(), QLayout::setResizeMode()
*/

void QWidget::setMinimumSize( int minw, int minh )
{
#if defined(CHECK_RANGE)
    if ( minw < 0 || minh < 0 )
	qWarning("QWidget::setMinimumSize: The smallest allowed size is (0,0)");
#endif
    createExtra();
    if ( extra->minw == minw && extra->minh == minh )
	return;
    extra->minw = minw;
    extra->minh = minh;
    if ( minw > width() || minh > height() ) {
	bool resized = testWState( WState_Resized );
	resize( QMAX(minw,width()), QMAX(minh,height()) );
	if ( !resized )
	    clearWState( WState_Resized ); //not a user resize
    }
    if ( testWFlags(WType_TopLevel) )
	do_size_hints( this, extra );
    updateGeometry();
}

/*!
  \overload void QWidget::setMaximumSize( const QSize &size )
*/

/*!
  Sets the maximum size of the widget to \e w by \e h pixels.

  The widget cannot be resized to a larger size than the maximum widget
  size. The widget's size is forced to the maximum size if the current
  size is greater.

  \sa maximumSize(), setMinimumSize(), setSizeIncrement(), resize(), size()
*/

void QWidget::setMaximumSize( int maxw, int maxh )
{
#if defined(CHECK_RANGE)
    if ( maxw > QWIDGETSIZE_MAX || maxh > QWIDGETSIZE_MAX ) {
	qWarning("QWidget::setMaximumSize: (%s/%s) "
		"The largest allowed size is (%d,%d)",
		 name( "unnamed" ), className(), QWIDGETSIZE_MAX,
		QWIDGETSIZE_MAX );
	maxw = QMIN( maxw, QWIDGETSIZE_MAX );
	maxh = QMIN( maxh, QWIDGETSIZE_MAX );
    }
    if ( maxw < 0 || maxh < 0 ) {
	qWarning("QWidget::setMaximumSize: (%s/%s) Negative sizes (%d,%d) "
		"are not possible",
		name( "unnamed" ), className(), maxw, maxh );
	maxw = QMAX( maxw, 0 );
	maxh = QMAX( maxh, 0 );
    }
#endif
    createExtra();
    if ( extra->maxw == maxw && extra->maxh == maxh )
	return;
    extra->maxw = maxw;
    extra->maxh = maxh;
    if ( maxw < width() || maxh < height() ) {
	bool resized = testWState( WState_Resized );
	resize( QMIN(maxw,width()), QMIN(maxh,height()) );
	if ( !resized )
	    clearWState( WState_Resized ); //not a user resize
    }
    if ( testWFlags(WType_TopLevel) )
	do_size_hints( this, extra );
    updateGeometry();
}

/*!
  Sets the size increment of the widget.  When the user resizes the
  window, the size will move in steps of \e w pixels horizontally and
  \e h pixels vertically, with baseSize() as basis. Preferred widget sizes are therefore for
  non-negative integers \e i and \e j:
  \code
  width = baseSize().width() + i * sizeIncrement().width();
  height = baseSize().height() + j * sizeIncrement().height();
  \endcode

  Note that while you can set the size increment for all widgets, it
  has no effect except for top-level widgets.

  \warning The size increment has no effect under Windows, and may be
  disregarded by the window manager on X.

  \sa sizeIncrement(), setMinimumSize(), setMaximumSize(), resize(), size()
*/

void QWidget::setSizeIncrement( int w, int h )
{
    QTLWExtra* x = topData();
    if ( x->incw == w && x->inch == h )
	return;
    x->incw = w;
    x->inch = h;
    if ( testWFlags(WType_TopLevel) )
	do_size_hints( this, extra );
}
/*!
  \overload void QWidget::setSizeIncrement( const QSize& )
*/


/*!
  Sets the base size of the widget.  The base size is important only
  in combination with size increments. See setSizeIncrement() for details.

  \sa baseSize()
*/

void QWidget::setBaseSize( int basew, int baseh )
{
    createTLExtra();
    QTLWExtra* x = topData();
    if ( x->basew == basew && x->baseh == baseh )
	return;
    x->basew = basew;
    x->baseh = baseh;
    if ( testWFlags(WType_TopLevel) )
	do_size_hints( this, extra );
}



/*!
  \overload void QWidget::setBaseSize( const QSize& )
*/

/*!
  \overload void QWidget::erase()
  This version erases the entire widget.
*/

/*!
  \overload void QWidget::erase( const QRect &r )
*/

/*!
  Erases the specified area \e (x,y,w,h) in the widget without generating
  a \link paintEvent() paint event\endlink.

  If \e w is negative, it is replaced with <code>width() - x</code>.
  If \e h is negative, it is replaced width <code>height() - y</code>.

  Child widgets are not affected.

  \sa repaint()
*/

void QWidget::erase( int x, int y, int w, int h )
{
    extern void qt_erase_rect( QWidget*, const QRect& ); // in qpainer_x11.cpp
    if ( w < 0 )
	w = crect.width()  - x;
    if ( h < 0 )
	h = crect.height() - y;
    if ( w != 0 && h != 0 )
	qt_erase_rect( this, QRect(x, y, w, h ) );
}

/*!
  Erases the area defined by \a reg, without generating a
  \link paintEvent() paint event\endlink.

  Child widgets are not affected.
*/

void QWidget::erase( const QRegion& reg )
{
    extern void qt_erase_region( QWidget*, const QRegion& ); // in qpainer_x11.cpp
    qt_erase_region( this, reg );
}


/*! \overload

  This version of the function scrolls the entire widget and moves the
  widget's children along with the scroll.

  \sa bitBlt() QScrollView
*/

void QWidget::scroll( int dx, int dy )
{
    scroll( dx, dy, QRect() );
}

/*! Scrolls \a r \a dx pixels to the right and \a dy downwards.  Both
  \a dx and \a dy may be negative.

  If \a r is empty or invalid, the result is undefined.

  After scrolling, scroll() sends a paint event for the the part of \a r
  that is read but not written.  For example, when scrolling 10 pixels
  rightwards, the leftmost ten pixels of \a r need repainting. The paint
  event may be delivered immediately or later, depending on some heuristics.

  This version of scroll() does not move the children of this widget.

  \sa QScrollView erase() bitBlt()
*/
void QWidget::scroll( int dx, int dy, const QRect& r )
{
    if ( testWState( WState_BlockUpdates ) )
	return;
    bool valid_rect = r.isValid();
    bool just_update = QABS( dx ) > width() || QABS( dy ) > height();
    if ( just_update )
	update();
    QRect sr = valid_rect?r:visibleRect();
    int x1, y1, x2, y2, w=sr.width(), h=sr.height();
    if ( dx > 0 ) {
	x1 = sr.x();
	x2 = x1+dx;
	w -= dx;
    } else {
	x2 = sr.x();
	x1 = x2-dx;
	w += dx;
    }
    if ( dy > 0 ) {
	y1 = sr.y();
	y2 = y1+dy;
	h -= dy;
    } else {
	y2 = sr.y();
	y1 = y2-dy;
	h += dy;
    }

    if ( dx == 0 && dy == 0 )
	return;

    Display *dpy = x11Display();
    GC gc = qt_xget_readonly_gc();
    // Want expose events
    if ( w > 0 && h > 0 && !just_update ) {
	XSetGraphicsExposures( dpy, gc, TRUE );
	XCopyArea( dpy, winId(), winId(), gc, x1, y1, w, h, x2, y2);
	XSetGraphicsExposures( dpy, gc, FALSE );
    }

    if ( !valid_rect && children() ) {	// scroll children
	QPoint pd( dx, dy );
	QObjectListIt it(*children());
	register QObject *object;
	while ( it ) {				// move all children
	    object = it.current();
	    if ( object->isWidgetType() ) {
		QWidget *w = (QWidget *)object;
		w->move( w->pos() + pd );
	    }
	    ++it;
	}
    }

    if ( just_update )
	return;

    // Don't let the server be bogged-down with repaint events
    bool repaint_immediately = qt_sip_count( this ) < 3;

    if ( dx ) {
	int x = x2 == sr.x() ? sr.x()+w : sr.x();
	if ( repaint_immediately )
	    repaint( x, sr.y(), QABS(dx), sr.height(), !testWFlags(WRepaintNoErase) );
	else
	    XClearArea( dpy, winid, x, sr.y(), QABS(dx), sr.height(), TRUE );
    }
    if ( dy ) {
	int y = y2 == sr.y() ? sr.y()+h : sr.y();
	if ( repaint_immediately )
	    repaint( sr.x(), y, sr.width(), QABS(dy), !testWFlags(WRepaintNoErase) );
	else
	    XClearArea( dpy, winid, sr.x(), y, sr.width(), QABS(dy), TRUE );
    }

    qt_insert_sip( this, dx, dy ); // #### ignores r
}


/*!
  \overload void QWidget::drawText( const QPoint &pos, const QString& str )
*/

/*!
  Writes \e str at position \e x,y.

  The \e y position is the base line position of the text.  The text is
  drawn using the default font and the default foreground color.

  This function is provided for convenience.  You will generally get
  more flexible results and often higher speed by using a a \link
  QPainter painter\endlink instead.

  \sa setFont(), foregroundColor(), QPainter::drawText()
*/

void QWidget::drawText( int x, int y, const QString &str )
{
    if ( testWState(WState_Visible) ) {
	QPainter paint;
	paint.begin( this );
	paint.drawText( x, y, str );
	paint.end();
    }
}


/*!
  Internal implementation of the virtual QPaintDevice::metric() function.

  Use the QPaintDeviceMetrics class instead.
*/

int QWidget::metric( int m ) const
{
    int val;
    if ( m == QPaintDeviceMetrics::PdmWidth ) {
	val = crect.width();
    } else if ( m == QPaintDeviceMetrics::PdmHeight ) {
	val = crect.height();
    } else {
	Display *dpy = x11Display();
	int scr = x11Screen();
	switch ( m ) {
	    case QPaintDeviceMetrics::PdmDpiX:
		val = QPaintDevice::x11AppDpiX();
		break;
	    case QPaintDeviceMetrics::PdmDpiY:
		val = QPaintDevice::x11AppDpiY();
		break;
	    case QPaintDeviceMetrics::PdmWidthMM:
		val = (DisplayWidthMM(dpy,scr)*crect.width())/
		      DisplayWidth(dpy,scr);
		break;
	    case QPaintDeviceMetrics::PdmHeightMM:
		val = (DisplayHeightMM(dpy,scr)*crect.height())/
		      DisplayHeight(dpy,scr);
		break;
	    case QPaintDeviceMetrics::PdmNumColors:
		val = x11Cells();
		break;
	    case QPaintDeviceMetrics::PdmDepth:
		val = x11Depth();
		break;
	    default:
		val = 0;
#if defined(CHECK_RANGE)
		qWarning( "QWidget::metric: Invalid metric command" );
#endif
	}
    }
    return val;
}

void QWidget::createSysExtra()
{
    extra->xDndProxy = 0;
}

void QWidget::deleteSysExtra()
{
}

void QWidget::createTLSysExtra()
{
#ifndef NO_XIM
    if ( qt_xim ) {
	XPoint spot; spot.x = 1; spot.y = 1; // dummmy

	XFontSet fontset = xic_fontset(fontMetrics().fontSet(), font().pointSize());

	XVaNestedList preedit_att = XVaCreateNestedList(0,
							XNSpotLocation, &spot,
							XNFontSet, fontset,
							NULL);
	XVaNestedList status_att = XVaCreateNestedList(0,
						       XNFontSet, fontset,
						       NULL);

	extra->topextra->xic = (void*)XCreateIC( qt_xim,
						 XNInputStyle, qt_xim_style,
						 XNClientWindow, winId(),
						 XNFocusWindow, winId(),
						 XNPreeditAttributes, preedit_att,
						 XNStatusAttributes, status_att,
						 0 );

	XFree(preedit_att);
	XFree(status_att);

    } else {
	extra->topextra->xic = 0;
    }
#else
    extra->topextra->xic = 0;
#endif
}

void QWidget::deleteTLSysExtra()
{
#ifndef NO_XIM
    if (extra->topextra->xic) {
	XUnsetICFocus( (XIC) extra->topextra->xic );
	XDestroyIC( (XIC) extra->topextra->xic );
	extra->topextra->xic = 0;
    }
#endif
}


/*!
  Returns TRUE if drop events are enabled for this widget.

  \sa setAcceptDrops()
*/

bool QWidget::acceptDrops() const
{
    return testWState(WState_DND);
}

/*!
  Announces to the system that this widget \e may be able to
  accept drop events.

  If the widgets is \link QWidget::isDesktop() the desktop\endlink,
  this may fail if another application is using the desktop - you
  can call acceptDrops() to test if this occurs.

  \sa acceptDrops()
*/

void QWidget::setAcceptDrops( bool on )
{
    if ( testWState(WState_DND) != on ) {
	if ( qt_dnd_enable( this, on ) ) {
	    if ( on )
		setWState(WState_DND);
	    else
		clearWState(WState_DND);
	}
    }
}

/*!
  Causes only the parts of the widget which overlap \a region
  to be visible.  If the region includes pixels outside the
  rect() of the widget, window system controls in that area
  may or may not be visible, depending on the platform.

  Note that this effect can be slow if the region is particularly
  complex.

  \sa setMask(QBitmap), clearMask()
*/

void QWidget::setMask( const QRegion& region )
{
    XShapeCombineRegion( x11Display(), winId(), ShapeBounding, 0, 0,
			 region.handle(), ShapeSet );
}

/*!
  Causes only the pixels of the widget for which \a bitmap
  has a corresponding 1 bit
  to be visible.  If the region includes pixels outside the
  rect() of the widget, window system controls in that area
  may or may not be visible, depending on the platform.

  Note that this effect can be slow if the region is particularly
  complex.

  \sa setMask(const QRegion&), clearMask()
*/

void QWidget::setMask( const QBitmap &bitmap )
{
    XShapeCombineMask( x11Display(), winId(), ShapeBounding, 0, 0,
		       bitmap.handle(), ShapeSet );
}

/*!
  Removes any mask set by setMask().

  \sa setMask()
*/

void QWidget::clearMask()
{
    XShapeCombineMask( x11Display(), winId(), ShapeBounding, 0, 0,
		       None, ShapeSet );
}

/*!\reimp
 */
void QWidget::setName( const char *name )
{
    QObject::setName( name );
    if ( isTopLevel() ) {
	XChangeProperty(qt_xdisplay(), winId(),
			qt_window_role, XA_STRING, 8, PropModeReplace,
			(unsigned char *)name, qstrlen( name ) );
    }
}
