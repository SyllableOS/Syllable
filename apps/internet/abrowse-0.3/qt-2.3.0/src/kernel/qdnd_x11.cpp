/****************************************************************************
** $Id$
**
** XDND implementation for Qt.  See http://www.cco.caltech.edu/~jafl/xdnd/
**
** Created : 980320
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

#ifndef QT_NO_DRAGANDDROP

#include "qwidget.h"
#include "qintdict.h"
#include "qdatetime.h"
#include "qdict.h"
#include "qdragobject.h"
#include "qobjectlist.h"
#include "qbitmap.h"

#include "qt_x11.h"


// conflict resolution

// unused, may be used again later: const int XKeyPress = KeyPress;
// unused, may be used again later: const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease

// this stuff is copied from qapp_x11.cpp

extern void qt_x11_intern_atom( const char *, Atom * );

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

extern void qt_ignore_badwindow();
extern bool qt_badwindow();
extern void qt_enter_modal( QWidget *widget );
extern void qt_leave_modal( QWidget *widget );

#if defined(Q_C_CALLBACKS)
}
#endif

extern Window qt_x11_findClientWindow( Window, Atom, bool );
extern Atom qt_wm_state;
extern Time qt_x_time;

// this stuff is copied from qclb_x11.cpp

extern bool qt_xclb_wait_for_event( Display *dpy, Window win, int type,
				    XEvent *event, int timeout );
extern bool qt_xclb_read_property( Display *dpy, Window win, Atom property,
				   bool deleteProperty,
				   QByteArray *buffer, int *size, Atom *type,
				   int *format, bool nullterm );
extern QByteArray qt_xclb_read_incremental_property( Display *dpy, Window win,
						     Atom property,
						     int nbytes, bool nullterm );
// and all this stuff is copied -into- qapp_x11.cpp

void qt_xdnd_setup();
void qt_handle_xdnd_enter( QWidget *, const XEvent *, bool );
void qt_handle_xdnd_position( QWidget *, const XEvent *, bool );
void qt_handle_xdnd_status( QWidget *, const XEvent *, bool );
void qt_handle_xdnd_leave( QWidget *, const XEvent *, bool );
void qt_handle_xdnd_drop( QWidget *, const XEvent *, bool );
void qt_handle_xdnd_finished( QWidget *, const XEvent *, bool );
void qt_xdnd_handle_selection_request( const XSelectionRequestEvent * );
bool qt_xdnd_handle_badwindow();
// client messages
Atom qt_xdnd_enter;
Atom qt_xdnd_position;
Atom qt_xdnd_status;
Atom qt_xdnd_leave;
Atom qt_xdnd_drop;
Atom qt_xdnd_finished;
Atom qt_xdnd_type_list;
int qt_xdnd_version = 4;

// Actions
//
// The Xdnd spec allows for user-defined actions.  This could be implemented
// with a registration process in Qt.  WE SHOULD do that later.
//
Atom qt_xdnd_action_copy;
Atom qt_xdnd_action_link;
Atom qt_xdnd_action_move;
Atom qt_xdnd_action_private;
static
QDropEvent::Action xdndaction_to_qtaction(Atom atom)
{
    if ( atom == qt_xdnd_action_copy || atom == 0 )
	return QDropEvent::Copy;
    if ( atom == qt_xdnd_action_link )
	return QDropEvent::Link;
    if ( atom == qt_xdnd_action_move )
	return QDropEvent::Move;
    return QDropEvent::Private;
}
static
int qtaction_to_xdndaction(QDropEvent::Action a)
{
    switch ( a ) {
      case QDropEvent::Copy:
	return qt_xdnd_action_copy;
      case QDropEvent::Link:
	return qt_xdnd_action_link;
      case QDropEvent::Move:
	return qt_xdnd_action_move;
      case QDropEvent::Private:
	return qt_xdnd_action_private;
      default:
	return qt_xdnd_action_copy;
    }
}

// clean up the stuff used.
static void qt_xdnd_cleanup();

static void qt_xdnd_send_leave();

// XDND selection
Atom qt_xdnd_selection;
// other selection
static Atom qt_selection_property;
// INCR
static Atom qt_incr_atom;

// properties for XDND drop sites
Atom qt_xdnd_aware;
Atom qt_xdnd_proxy;

// real variables:
// xid of current drag source
static Atom qt_xdnd_dragsource_xid = 0;

// the types in this drop.  100 is no good, but at least it's big.
const int qt_xdnd_max_type = 100;
static Atom qt_xdnd_types[qt_xdnd_max_type];

static QIntDict<QCString> * qt_xdnd_drag_types = 0;
static QDict<Atom> * qt_xdnd_atom_numbers = 0;

// timer used when target wants "continuous" move messages (eg. scroll)
static int heartbeat = 0;
// rectangle in which the answer will be the same
static QRect qt_xdnd_source_sameanswer;
//static QRect qt_xdnd_target_sameanswer;
static bool qt_xdnd_target_answerwas;
// top-level window we sent position to last.
static Window qt_xdnd_current_target;
// window to send events to (always valid if qt_xdnd_current_target)
static Window qt_xdnd_current_proxy_target;
// widget we forwarded position to last, and local position
static QWidget * qt_xdnd_current_widget;
static QPoint qt_xdnd_current_position;
// time of this drop, as type Atom to save on casts
static Atom qt_xdnd_source_current_time;
//NOTUSED static Atom qt_xdnd_target_current_time;

// dict of payload data, sorted by type atom
QIntDict<QByteArray> * qt_xdnd_target_data = 0;

// first drag object, or 0
QDragObject * qt_xdnd_source_object = 0;

// Motif dnd
extern void qt_motifdnd_enable( QWidget *, bool );
extern QByteArray qt_motifdnd_obtain_data();

bool qt_motifdnd_active = FALSE;


// Shift/Ctrl handling, and final drop status
static QDragObject::DragMode drag_mode;
static QDropEvent::Action global_requested_action = QDropEvent::Copy;
static QDropEvent::Action global_accepted_action = QDropEvent::Copy;

// for embedding only
static QWidget* current_embedding_widget  = 0;
static XEvent last_enter_event;

// cursors
static QCursor *noDropCursor = 0;
static QCursor *moveCursor = 0;
static QCursor *copyCursor = 0;
static QCursor *linkCursor = 0;

static QPixmap *defaultPm = 0;

static const int default_pm_hotx = -2;
static const int default_pm_hoty = -16;
static const char* default_pm[] = {
"13 9 3 1",
".      c None",
"       c #000000",
"X      c #FFFFFF",
"X X X X X X X",
" X X X X X X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X X X X X X ",
"X X X X X X X",
};

class QShapedPixmapWidget : public QWidget {
    QPixmap pixmap;
public:
    QShapedPixmapWidget() :
	QWidget(0,0,WStyle_Customize | WStyle_Tool | WStyle_NoBorder | WX11BypassWM )
    {
    }

    void setPixmap(QPixmap pm)
    {
	pixmap = pm;
	if ( pixmap.mask() ) {
	    setMask( *pixmap.mask() );
	} else {
	    clearMask();
	}
	resize(pm.width(),pm.height());
    }

    void paintEvent(QPaintEvent*)
    {
	bitBlt(this,0,0,&pixmap);
    }
};

QShapedPixmapWidget * qt_xdnd_deco = 0;

static QWidget* desktop_proxy = 0;

class QExtraWidget : public QWidget
{
public:
    QWExtra* extraData() { return QWidget::extraData(); }
    QTLWExtra* topData() { return QWidget::topData(); }
};


static bool qt_xdnd_enable( QWidget* w, bool on )
{
    if ( on ) {
	QWidget * xdnd_widget = 0;
	if ( w->isDesktop() ) {
	    if ( desktop_proxy ) // *WE* already have one.
		return FALSE;

	    // As per Xdnd4, use XdndProxy
	    XGrabServer( w->x11Display() );
	    Atom type = None;
	    int f;
	    unsigned long n, a;
	    WId *proxy_id_ptr;
	    XGetWindowProperty( w->x11Display(), w->winId(),
		qt_xdnd_proxy, 0, 1, False,
		XA_WINDOW, &type, &f,&n,&a,(uchar**)&proxy_id_ptr );
	    WId proxy_id = 0;
	    if ( type == XA_WINDOW && proxy_id_ptr ) {
		proxy_id = *proxy_id_ptr;
		XFree(proxy_id_ptr);
		proxy_id_ptr = 0;
		// Already exists.  Real?
		qt_ignore_badwindow();
		XGetWindowProperty( w->x11Display(), proxy_id,
		    qt_xdnd_proxy, 0, 1, False,
		    XA_WINDOW, &type, &f,&n,&a,(uchar**)&proxy_id_ptr );
		if ( qt_badwindow() || type != XA_WINDOW || !proxy_id_ptr || *proxy_id_ptr != proxy_id ) {
		    // Bogus - we will overwrite.
		    proxy_id = 0;
		}
	    }
	    if ( proxy_id_ptr )
		XFree(proxy_id_ptr);

	    if ( !proxy_id ) {
		xdnd_widget = desktop_proxy = new QWidget;
		proxy_id = desktop_proxy->winId();
		XChangeProperty ( w->x11Display(),
		    w->winId(), qt_xdnd_proxy,
		    XA_WINDOW, 32, PropModeReplace,
		    (unsigned char *)&proxy_id, 1 );
		XChangeProperty ( w->x11Display(),
		    proxy_id, qt_xdnd_proxy,
		    XA_WINDOW, 32, PropModeReplace,
		    (unsigned char *)&proxy_id, 1 );
	    }

	    XUngrabServer( w->x11Display() );
	} else {
	    xdnd_widget = w->topLevelWidget();
	}
	if ( xdnd_widget ) {
	    Atom atm = (Atom)qt_xdnd_version;
	    XChangeProperty ( xdnd_widget->x11Display(), xdnd_widget->winId(),
			      qt_xdnd_aware, XA_ATOM, 32, PropModeReplace,
			      (unsigned char *)&atm, 1 );
	    return TRUE;
	} else {
	    return FALSE;
	}
    } else {
	if ( w->isDesktop() ) {
	    XDeleteProperty( w->x11Display(), w->winId(),
			     qt_xdnd_proxy );
	    delete desktop_proxy;
	    desktop_proxy = 0;
	}
	return TRUE;
    }
}

const char* qt_xdnd_atom_to_str( Atom a )
{
    if ( !a ) return 0;

    if ( a == XA_STRING )
	return "text/plain"; // some Xdnd clients are dumb

    if ( !qt_xdnd_drag_types ) {
	qt_xdnd_drag_types = new QIntDict<QCString>( 17 );
	qt_xdnd_drag_types->setAutoDelete( TRUE );
    }
    QCString* result;
    if ( !(result=qt_xdnd_drag_types->find( a )) ) {
	const char* mimeType = XGetAtomName( qt_xdisplay(), a );
	if ( !mimeType )
	    return 0; // only happens on protocol error
	result = new QCString( mimeType );
	qt_xdnd_drag_types->insert( (long)a, result );
	XFree((void*)mimeType);
    }
    return *result;
}

Atom* qt_xdnd_str_to_atom( const char *mimeType )
{
    if ( !mimeType || !*mimeType )
	return 0;
    if ( !qt_xdnd_atom_numbers ) {
	qt_xdnd_atom_numbers = new QDict<Atom>( 17 );
	qt_xdnd_atom_numbers->setAutoDelete( TRUE );
    }

    Atom * result;
    if ( (result = qt_xdnd_atom_numbers->find( mimeType )) )
	return result;

    result = new Atom;
    *result = 0;
    qt_x11_intern_atom( mimeType, result );
    qt_xdnd_atom_numbers->insert( mimeType, result );
    qt_xdnd_atom_to_str( *result );

    return result;
}


void qt_xdnd_setup() {
    // set up protocol atoms
    qt_x11_intern_atom( "XdndEnter", &qt_xdnd_enter );
    qt_x11_intern_atom( "XdndPosition", &qt_xdnd_position );
    qt_x11_intern_atom( "XdndStatus", &qt_xdnd_status );
    qt_x11_intern_atom( "XdndLeave", &qt_xdnd_leave );
    qt_x11_intern_atom( "XdndDrop", &qt_xdnd_drop );
    qt_x11_intern_atom( "XdndFinished", &qt_xdnd_finished );
    qt_x11_intern_atom( "XdndTypeList", &qt_xdnd_type_list );

    qt_x11_intern_atom( "XdndSelection", &qt_xdnd_selection );

    qt_x11_intern_atom( "XdndAware", &qt_xdnd_aware );
    qt_x11_intern_atom( "XdndProxy", &qt_xdnd_proxy );


    qt_x11_intern_atom( "XdndActionCopy", &qt_xdnd_action_copy );
    qt_x11_intern_atom( "XdndActionLink", &qt_xdnd_action_link );
    qt_x11_intern_atom( "XdndActionMove", &qt_xdnd_action_move );
    qt_x11_intern_atom( "XdndActionPrivate", &qt_xdnd_action_private );

    qt_x11_intern_atom( "QT_SELECTION", &qt_selection_property );
    qt_x11_intern_atom( "INCR", &qt_incr_atom );

    qAddPostRoutine( qt_xdnd_cleanup );
}


void qt_xdnd_cleanup()
{
    delete qt_xdnd_drag_types;
    qt_xdnd_drag_types = 0;
    delete qt_xdnd_atom_numbers;
    qt_xdnd_atom_numbers = 0;
    delete qt_xdnd_target_data;
    qt_xdnd_target_data = 0;
    noDropCursor = 0;
    delete copyCursor;
    copyCursor = 0;
    delete moveCursor;
    moveCursor = 0;
    delete linkCursor;
    linkCursor = 0;
    delete defaultPm;
    defaultPm = 0;
    delete desktop_proxy;
    desktop_proxy = 0;
}


static QWidget * find_child( QWidget * tlw, QPoint & p )
{
    QWidget * w = tlw;

    p = w->mapFromGlobal( p );
    bool done = FALSE;
    while ( !done ) {
	done = TRUE;
	if ( w->children() ) {
	    QObjectListIt it( *w->children() );
	    it.toLast();
	    QObject * o;
	    while( (o=it.current()) ) {
		--it;
		if ( o->isWidgetType() &&
		     ((QWidget*)o)->isVisible() &&
		     ((QWidget*)o)->geometry().contains( p ) ) {
		    w = (QWidget *)o;
		    done = FALSE;
		    p = w->mapFromParent( p );
		    break;
		}
	    }
	}
    }
    return w;
}


static bool checkEmbedded(QWidget* w, const XEvent* xe)
{
    if (!w)
	return FALSE;

    if (current_embedding_widget != 0 && current_embedding_widget != w) {
	qt_xdnd_current_target = ((QExtraWidget*)current_embedding_widget)->extraData()->xDndProxy;
	qt_xdnd_current_proxy_target = qt_xdnd_current_target;
	qt_xdnd_send_leave();
	qt_xdnd_current_target = 0;
	qt_xdnd_current_proxy_target = 0;
	current_embedding_widget = 0;
    }

    QWExtra* extra = ((QExtraWidget*)w)->extraData();
    if ( extra && extra->xDndProxy != 0 ) {

	if (current_embedding_widget != w) {

 	    last_enter_event.xany.window = extra->xDndProxy;
 	    XSendEvent( qt_xdisplay(), extra->xDndProxy, FALSE, NoEventMask,
 			&last_enter_event );
	    current_embedding_widget = w;
	}

	((XEvent*)xe)->xany.window = extra->xDndProxy;
	XSendEvent( qt_xdisplay(), extra->xDndProxy, FALSE, NoEventMask,
		    (XEvent*)xe );
	qt_xdnd_current_widget = w;
	return TRUE;
    }
    current_embedding_widget = 0;
    return FALSE;
}

void qt_handle_xdnd_enter( QWidget *, const XEvent * xe, bool /*passive*/ )
{
    //if ( !w->neveHadAChildWithDropEventsOn() )
	//return; // haven't been set up for dnd

    qt_motifdnd_active = FALSE;

    last_enter_event.xclient = xe->xclient;

    qt_xdnd_target_answerwas = FALSE;

    const long *l = xe->xclient.data.l;
    int version = (int)(((unsigned long)(l[1])) >> 24);

    if ( version > qt_xdnd_version )
	return;

    qt_xdnd_dragsource_xid = l[0];

    int j = 0;
    if ( l[1] & 1 ) {
	// get the types from XdndTypeList
	Atom   type = None;
	int f;
	unsigned long n, a;
	Atom *data;
	XGetWindowProperty( qt_xdisplay(), qt_xdnd_dragsource_xid,
		    qt_xdnd_type_list, 0,
		    qt_xdnd_max_type, False, XA_ATOM, &type, &f,&n,&a,(uchar**)&data );
	for ( ; j<qt_xdnd_max_type && j < (int)n; j++ ) {
	    qt_xdnd_types[j] = data[j];
	}
    } else {
	// get the types from the message
	int i;
	for( i=2; i < 5; i++ ) {
	    qt_xdnd_types[j++] = l[i];
	}
    }
    qt_xdnd_types[j] = 0;
}



void qt_handle_xdnd_position( QWidget *w, const XEvent * xe, bool passive )
{
    const unsigned long *l = (const unsigned long *)xe->xclient.data.l;

    QPoint p( (l[2] & 0xffff0000) >> 16, l[2] & 0x0000ffff );
    QWidget * c = find_child( w, p ); // changes p to to c-local coordinates

    if (!passive && checkEmbedded(c, xe))
	return;

    if ( !c || !c->acceptDrops() && c->isDesktop() ) {
	return;
    }

    if ( l[0] != qt_xdnd_dragsource_xid ) {
	//qDebug( "xdnd drag position from unexpected source (%08lx not %08lx)",
	//     l[0], qt_xdnd_dragsource_xid );
	return;
    }

    XClientMessageEvent response;
    response.type = ClientMessage;
    response.window = qt_xdnd_dragsource_xid;
    response.format = 32;
    response.message_type = qt_xdnd_status;
    response.data.l[0] = w->winId();
    response.data.l[1] = 0; // flags
    response.data.l[2] = 0; // x, y
    response.data.l[3] = 0; // w, h
    response.data.l[4] = 0; // action

    if ( !passive ) { // otherwise just reject
	while ( c && !c->acceptDrops() && !c->isTopLevel() ) {
	    p = c->mapToParent( p );
	    c = c->parentWidget();
	}

	QRect answerRect( c->mapToGlobal( p ), QSize( 1,1 ) );

	QDragMoveEvent me( p );
	QDropEvent::Action accepted_action = xdndaction_to_qtaction(l[4]);
	me.setAction(accepted_action);

	if ( qt_xdnd_current_widget != c ) {
	    qt_xdnd_target_answerwas = FALSE;
	    if ( qt_xdnd_current_widget ) {
		QDragLeaveEvent e;
		QApplication::sendEvent( qt_xdnd_current_widget, &e );
	    }
	    if ( c->acceptDrops() ) {
		qt_xdnd_current_widget = c;
		qt_xdnd_current_position = p;
		//NOTUSED qt_xdnd_target_current_time = l[3]; // will be 0 for xdnd1

		QDragEnterEvent de( p );
		QApplication::sendEvent( c, &de );
		if ( de.isAccepted() ) {
		    me.accept( de.answerRect() );
		    if ( !me.isActionAccepted() ) // only as a copy (move if we del)
			accepted_action = QDropEvent::Copy;
		} else {
		    me.ignore( de.answerRect() );
		}
	    }
	} else {
	    if ( qt_xdnd_target_answerwas )
		me.accept();
	}

	if ( !c->acceptDrops() ) {
	    qt_xdnd_current_widget = 0;
	    answerRect = QRect( p, QSize( 1, 1 ) );
	} else if ( xdndaction_to_qtaction(l[4]) < QDropEvent::Private ) {
	    qt_xdnd_current_widget = c;
	    qt_xdnd_current_position = p;
	    //NOTUSED qt_xdnd_target_current_time = l[3]; // will be 0 for xdnd1

	    QApplication::sendEvent( c, &me );
	    qt_xdnd_target_answerwas = me.isAccepted();
	    if ( me.isAccepted() ) {
		response.data.l[1] = 1; // yes
		if ( !me.isActionAccepted() ) // only as a copy (move if we del)
		    accepted_action = QDropEvent::Copy;
	    } else {
		response.data.l[0] = 0;
	    }
	    answerRect = me.answerRect().intersect( c->rect() );
	} else {
	    response.data.l[0] = 0;
	    answerRect = QRect( p, QSize( 1, 1 ) );
	}
	answerRect = QRect( c->mapToGlobal( answerRect.topLeft() ),
			    answerRect.size() );

	if ( answerRect.width() < 0 )
	    answerRect.setWidth( 0 );
	if ( answerRect.height() < 0 )
	    answerRect.setHeight( 0 );
	if ( answerRect.left() < 0 )
	    answerRect.setLeft( 0 );
	if ( answerRect.right() > 4096 )
	    answerRect.setRight( 4096 );
	if ( answerRect.top() < 0 )
	    answerRect.setTop( 0 );
	if ( answerRect.bottom() > 4096 )
	    answerRect.setBottom( 4096 );

	response.data.l[2] = (answerRect.x() << 16) + answerRect.y();
	response.data.l[3] = (answerRect.width() << 16) + answerRect.height();
	response.data.l[4] = qtaction_to_xdndaction(accepted_action);
	global_accepted_action = accepted_action;
    }

    QWidget * source = QWidget::find( qt_xdnd_dragsource_xid );

    int emask = NoEventMask;
    if ( source && source->isDesktop() && !source->acceptDrops() ) {
	emask = EnterWindowMask;
	source = 0;
    }

    if ( source )
	qt_handle_xdnd_status( source, (const XEvent *)&response, passive );
    else
	XSendEvent( qt_xdisplay(), qt_xdnd_dragsource_xid, FALSE,
		    emask, (XEvent*)&response );
}


void qt_handle_xdnd_status( QWidget * w, const XEvent * xe, bool /*passive*/ )
{
    const unsigned long *l = (const unsigned long *)xe->xclient.data.l;
    // Messy:  QDragResponseEvent is just a call to QDragManager function
    global_accepted_action = xdndaction_to_qtaction(l[4]);
    QDragResponseEvent e( (int)(l[1] & 1) );
    QApplication::sendEvent( w, &e );

    if ( (int)(l[1] & 2) == 0 ) {
	QPoint p( (l[2] & 0xffff0000) >> 16, l[2] & 0x0000ffff );
	QSize s( (l[3] & 0xffff0000) >> 16, l[3] & 0x0000ffff );
	qt_xdnd_source_sameanswer = QRect( p, s );
	if ( qt_xdnd_source_sameanswer.isNull() ) {
	    // Application wants "coninutous" move events
	}
    } else {
	qt_xdnd_source_sameanswer = QRect();
    }
}


void qt_handle_xdnd_leave( QWidget *w, const XEvent * xe, bool /*passive*/ )
{
    //qDebug( "xdnd leave" );
    if ( !qt_xdnd_current_widget ||
	 w->topLevelWidget() != qt_xdnd_current_widget->topLevelWidget() ) {
	return; // sanity
    }

    if (checkEmbedded(current_embedding_widget, xe)) {
	current_embedding_widget = 0;
	qt_xdnd_current_widget = 0;
	return;
    }

    const unsigned long *l = (const unsigned long *)xe->xclient.data.l;

    QDragLeaveEvent e;
    QApplication::sendEvent( qt_xdnd_current_widget, &e );

    if ( l[0] != qt_xdnd_dragsource_xid ) {
	// This often happens - leave other-process window quickly
	//qDebug( "xdnd drag leave from unexpected source (%08lx not %08lx",
	       //l[0], qt_xdnd_dragsource_xid );
	qt_xdnd_current_widget = 0;
	return;
    }

    qt_xdnd_dragsource_xid = 0;
    qt_xdnd_types[0] = 0;
    qt_xdnd_current_widget = 0;
}


void qt_xdnd_send_leave()
{
    if ( !qt_xdnd_current_target )
	return;

    XClientMessageEvent leave;
    leave.type = ClientMessage;
    leave.window = qt_xdnd_current_target;
    leave.format = 32;
    leave.message_type = qt_xdnd_leave;
    leave.data.l[0] = qt_xdnd_dragsource_xid;
    leave.data.l[1] = 0; // flags
    leave.data.l[2] = 0; // x, y
    leave.data.l[3] = 0; // w, h
    leave.data.l[4] = 0; // just null

    QWidget * w = QWidget::find( qt_xdnd_current_proxy_target );

    int emask = NoEventMask;
    if ( w && w->isDesktop() && !w->acceptDrops() ) {
	emask = EnterWindowMask;
	w = 0;
    }

    if ( w )
	qt_handle_xdnd_leave( w, (const XEvent *)&leave, FALSE );
    else
	XSendEvent( qt_xdisplay(), qt_xdnd_current_proxy_target, FALSE,
		    emask, (XEvent*)&leave );
    qt_xdnd_current_target = 0;
    qt_xdnd_current_proxy_target = 0;
}



void qt_handle_xdnd_drop( QWidget *, const XEvent * xe, bool passive )
{
    if ( !qt_xdnd_current_widget ) {
	qt_xdnd_dragsource_xid = 0;
	return; // sanity
    }

    if (!passive && checkEmbedded(qt_xdnd_current_widget, xe)){
	current_embedding_widget = 0;
	qt_xdnd_dragsource_xid = 0;
	qt_xdnd_current_widget = 0;
	return;
    }
    const unsigned long *l = (const unsigned long *)xe->xclient.data.l;

    //qDebug( "xdnd drop" );

    if ( l[0] != qt_xdnd_dragsource_xid ) {
	//qDebug( "xdnd drop from unexpected source (%08lx not %08lx",
	//       l[0], qt_xdnd_dragsource_xid );
	return;
    }
    if ( qt_xdnd_source_object )
	qt_xdnd_source_object->setTarget( qt_xdnd_current_widget );

    if ( !passive ) {
	QDropEvent de( qt_xdnd_current_position );
	de.setAction( global_accepted_action );
	QApplication::sendEvent( qt_xdnd_current_widget, &de );
	if ( !de.isAccepted() ) {
	    // Ignore a failed move
	    global_accepted_action = QDropEvent::Copy;
	}
	XClientMessageEvent finished;
	finished.type = ClientMessage;
	finished.window = qt_xdnd_dragsource_xid;
	finished.format = 32;
	finished.message_type = qt_xdnd_finished;
	finished.data.l[0] = qt_xdnd_current_widget?qt_xdnd_current_widget->topLevelWidget()->winId():0;
	finished.data.l[1] = 0; // flags
	XSendEvent( qt_xdisplay(), qt_xdnd_dragsource_xid, FALSE,
		    NoEventMask, (XEvent*)&finished );
    } else {
	QDragLeaveEvent e;
	QApplication::sendEvent( qt_xdnd_current_widget, &e );
    }
    qt_xdnd_dragsource_xid = 0;
    qt_xdnd_current_widget = 0;
}


void qt_handle_xdnd_finished( QWidget *, const XEvent * xe, bool passive )
{
    const unsigned long *l = (const unsigned long *)xe->xclient.data.l;

    if ( l[0] && (l[0] == qt_xdnd_current_target
	    || l[0] == qt_xdnd_current_proxy_target) ) {
	//
	if ( !passive )
	    (void ) checkEmbedded( qt_xdnd_current_widget, xe);
	current_embedding_widget = 0;
	qt_xdnd_current_target = 0;
	qt_xdnd_current_proxy_target = 0;
	delete qt_xdnd_source_object;
	qt_xdnd_source_object = 0;
    }
}


void QDragManager::timerEvent( QTimerEvent* e )
{
    if ( e->timerId() == heartbeat && qt_xdnd_source_sameanswer.isNull() )
	move( QCursor::pos() );
}

bool QDragManager::eventFilter( QObject * o, QEvent * e)
{
    if ( beingCancelled ) {
	if ( e->type() == QEvent::KeyRelease &&
	     ((QKeyEvent*)e)->key() == Key_Escape ) {
	    qApp->removeEventFilter( this );
	    object = 0;
	    dragSource = 0;
	    beingCancelled = FALSE;
	    qApp->exit_loop();
	    return TRUE; // block the key release
	}
	return FALSE;
    }

    ASSERT( object != 0 );

    if ( !o->isWidgetType() )
	return FALSE;

    QWidget* w = (QWidget*)o;

    if ( e->type() == QEvent::MouseMove ) {
	QMouseEvent* me = (QMouseEvent *)e;
	updateMode(me->stateAfter());
	move( w->mapToGlobal( me->pos() ) );
	return TRUE;
    } else if ( e->type() == QEvent::MouseButtonRelease ) {
	qApp->removeEventFilter( this );
	if ( willDrop )
	    drop();
	else
	    cancel();
	object = 0;
	dragSource = 0;
	beingCancelled = FALSE;
	qApp->exit_loop();
	return TRUE;
    } else if ( e->type() == QEvent::DragResponse ) {
	if ( ((QDragResponseEvent *)e)->dragAccepted() ) {
	    if ( !willDrop ) {
		willDrop = TRUE;
	    }
	} else {
	    if ( willDrop ) {
		willDrop = FALSE;
	    }
	}
	updateCursor();
	return TRUE;
    }

    if ( e->type() == QEvent::KeyPress
      || e->type() == QEvent::KeyRelease )
    {
	QKeyEvent *ke = ((QKeyEvent*)e);
	if ( ke->key() == Key_Escape && e->type() == QEvent::KeyPress ) {
	    cancel();
	    qApp->removeEventFilter( this );
	    object = 0;
	    dragSource = 0;
	    beingCancelled = FALSE;
	    qApp->exit_loop();
	} else {
	    updateMode(ke->stateAfter());
	    qt_xdnd_source_sameanswer = QRect(); // force move
	    move( QCursor::pos() );
	}
	return TRUE; // Eat all key events
    }

    // ### We bind modality to widgets, so we have to do this
    // ###  "manually".
    // DnD is modal - eat all other interactive events
    switch ( e->type() ) {
      case QEvent::MouseButtonPress:
      case QEvent::MouseButtonRelease:
      case QEvent::MouseButtonDblClick:
      case QEvent::MouseMove:
      case QEvent::KeyPress:
      case QEvent::KeyRelease:
      case QEvent::Wheel:
      case QEvent::Accel:
      case QEvent::AccelAvailable:
	return TRUE;
      default:
	return FALSE;
    }
}


static Qt::ButtonState oldstate;
void QDragManager::updateMode( ButtonState newstate )
{
    if ( newstate == oldstate )
	return;
    const int both = ShiftButton|ControlButton;
    if ( (newstate & both) == both ) {
	global_requested_action = QDropEvent::Link;
    } else {
	bool local = qt_xdnd_source_object != 0;
	if ( drag_mode == QDragObject::DragMove )
	    global_requested_action = QDropEvent::Move;
	else if ( drag_mode == QDragObject::DragCopy )
	    global_requested_action = QDropEvent::Copy;
	else {
	    if ( drag_mode == QDragObject::DragDefault && local )
		global_requested_action = QDropEvent::Move;
	    else
		global_requested_action = QDropEvent::Copy;
	    if ( newstate & ShiftButton )
		global_requested_action = QDropEvent::Move;
	    else if ( newstate & ControlButton )
		global_requested_action = QDropEvent::Copy;
	}
    }
    oldstate = newstate;
}


void QDragManager::updateCursor()
{
    if ( !noDropCursor ) {
	noDropCursor = new QCursor( ForbiddenCursor );
	if ( !pm_cursor[0].isNull() )
	    moveCursor = new QCursor(pm_cursor[0], 0,0);
	if ( !pm_cursor[1].isNull() )
	    copyCursor = new QCursor(pm_cursor[1], 0,0);
	if ( !pm_cursor[2].isNull() )
	    linkCursor = new QCursor(pm_cursor[2], 0,0);
    }

    QCursor *c;
    if ( willDrop ) {
	if ( global_accepted_action == QDropEvent::Copy ) {
	    if ( global_requested_action == QDropEvent::Move )
		c = moveCursor; // (source can delete)
	    else
		c = copyCursor;
	} else if ( global_accepted_action == QDropEvent::Link ) {
	    c = linkCursor;
	} else {
	    c = moveCursor;
	}
	if ( qt_xdnd_deco )
	    qt_xdnd_deco->show();
    } else {
	c = noDropCursor;
	if ( qt_xdnd_deco )
	    qt_xdnd_deco->hide();
    }
#ifndef QT_NO_CURSOR
    if ( c )
	qApp->setOverrideCursor( *c, TRUE );
#endif
}


void QDragManager::cancel( bool deleteSource )
{
    if ( object ) {
	beingCancelled = TRUE;
	object = 0;
    }

    if ( qt_xdnd_current_target ) {
	qt_xdnd_send_leave();
    }

#ifndef QT_NO_CURSOR
    if ( restoreCursor ) {
	QApplication::restoreOverrideCursor();
	restoreCursor = FALSE;
    }
#endif

    if ( deleteSource )
	delete qt_xdnd_source_object;
    qt_xdnd_source_object = 0;
    delete qt_xdnd_deco;
    qt_xdnd_deco = 0;
}

static
Window findRealWindow( const QPoint & pos, Window w, int md )
{
    if ( qt_xdnd_deco && w == qt_xdnd_deco->winId() )
	return 0;

    if ( md ) {
	XWindowAttributes attr;
	XGetWindowAttributes( qt_xdisplay(), w, &attr );

	if ( attr.map_state != IsUnmapped
	    && QRect(attr.x,attr.y,attr.width,attr.height)
		.contains(pos) )
	{
	    {
		Atom   type = None;
		int f;
		unsigned long n, a;
		unsigned char *data;

		XGetWindowProperty( qt_xdisplay(), w, qt_wm_state, 0,
		    0, False, AnyPropertyType, &type, &f,&n,&a,&data );
		if ( data ) XFree(data);
		if ( type ) return w;
	    }

	    Window r, p;
	    Window* c;
	    uint nc;
	    if ( XQueryTree( qt_xdisplay(), w, &r, &p, &c, &nc ) ) {
		r=0;
		for (uint i=nc; !r && i--; ) {
		    r = findRealWindow( pos-QPoint(attr.x,attr.y),
					c[i], md-1 );
		}
		XFree(c);
		if ( r )
		    return r;

		// We didn't find a client window!  Just use the
		// innermost window.
	    }

	    // No children!
	    return w;
	}
    }
    return 0;
}

void QDragManager::move( const QPoint & globalPos )
{
    updatePixmap();

    if ( qt_xdnd_source_sameanswer.contains( globalPos ) &&
	 qt_xdnd_source_sameanswer.isValid() ) {
	return;
    }

    Window target = 0;
    int lx = 0, ly = 0;
    if ( !XTranslateCoordinates( qt_xdisplay(), qt_xrootwin(), qt_xrootwin(),
				 globalPos.x(), globalPos.y(),
				 &lx, &ly, &target) ) {
	// somehow got to a different screen?  ignore for now
	return;
    }

    if ( target == qt_xrootwin() ) {
	// Ok.
    } else if ( target ) {
	//me
	Window targetW = qt_x11_findClientWindow( target, qt_wm_state, TRUE );
	if (targetW)
	    target = targetW;
 	if ( qt_xdnd_deco && (!target || target == qt_xdnd_deco->winId()) ) {
 	    target = findRealWindow(globalPos,qt_xrootwin(),6);
 	}
    }

    int emask = NoEventMask;
    QWidget* w;
    if ( target ) {
	w = QWidget::find( (WId)target );
	if ( w && w->isDesktop() && !w->acceptDrops() ) {
	    emask = EnterWindowMask;
	    w = 0;
	}
    } else {
	w = 0;
	target = qt_xrootwin();
    }

    WId proxy_target = target;
    int target_version = 1;

    {
	Atom   type = None;
	int f;
	unsigned long n, a;
	WId *proxy_id;
	qt_ignore_badwindow();
	XGetWindowProperty( qt_xdisplay(), target, qt_xdnd_proxy, 0,
	    1, False, XA_WINDOW, &type, &f,&n,&a,(uchar**)&proxy_id );
	if ( qt_badwindow() ) {
	    proxy_target = target = 0;
	} else if ( type == XA_WINDOW && proxy_id ) {
	    proxy_target = *proxy_id;
	    XFree(proxy_id);
	    proxy_id = 0;
	    qt_ignore_badwindow();
	    XGetWindowProperty( qt_xdisplay(), proxy_target, qt_xdnd_proxy, 0,
		1, False, XA_WINDOW, &type, &f,&n,&a,(uchar**)&proxy_id );
	    if ( qt_badwindow() || !type || !proxy_id || *proxy_id != proxy_target ) {
		// Bogus
		proxy_target = 0;
		target = 0;
	    }
	    if ( proxy_id )
		XFree(proxy_id);
	}
	if ( proxy_target ) {
	    int *tv;
	    qt_ignore_badwindow();
	    XGetWindowProperty( qt_xdisplay(), proxy_target, qt_xdnd_aware, 0,
		1, False, AnyPropertyType, &type, &f,&n,&a,(uchar**)&tv );
	    target_version = QMIN(qt_xdnd_version,tv ? *tv : 1);
	    if ( tv ) XFree(tv);
	    if ( !qt_badwindow() && type )
		emask = EnterWindowMask;
	    else
		target = 0;
	}
    }

    if ( target != qt_xdnd_current_target ) {
	if ( qt_xdnd_current_target )
	    qt_xdnd_send_leave();

	qt_xdnd_current_target = target;
	qt_xdnd_current_proxy_target = proxy_target;
	if ( target ) {
	    QArray<Atom> type;
	    int flags = target_version << 24;
	    const char* fmt;
	    int nfmt=0;
	    for (nfmt=0; (fmt=object->format(nfmt)); nfmt++) {
		type.resize(nfmt+1);
		type[nfmt] = *qt_xdnd_str_to_atom( fmt );
	    }
	    if ( nfmt >= 3 ) {
		XChangeProperty( qt_xdisplay(),
		    object->source()->winId(), qt_xdnd_type_list,
		    XA_ATOM, 32, PropModeReplace,
		    (unsigned char *)type.data(),
		    type.size() );
		flags |= 0x0001;
	    }
	    XClientMessageEvent enter;
	    enter.type = ClientMessage;
	    enter.window = target;
	    enter.format = 32;
	    enter.message_type = qt_xdnd_enter;
	    enter.data.l[0] = object->source()->winId();
	    enter.data.l[1] = flags;
	    enter.data.l[2] = type.size()>0 ? type[0] : 0;
	    enter.data.l[3] = type.size()>1 ? type[1] : 0;
	    enter.data.l[4] = type.size()>2 ? type[2] : 0;
	    // provisionally set the rectangle to 5x5 pixels...
	    qt_xdnd_source_sameanswer = QRect( globalPos.x() - 2,
					       globalPos.y() -2 , 5, 5 );

	    if ( w ) {
		qt_handle_xdnd_enter( w, (const XEvent *)&enter, FALSE );
	    } else if ( target ) {
		XSendEvent( qt_xdisplay(), proxy_target, FALSE, emask,
			    (XEvent*)&enter );
	    }
	}
    }

    if ( target ) {
	XClientMessageEvent move;
	move.type = ClientMessage;
	move.window = target;
	move.format = 32;
	move.message_type = qt_xdnd_position;
	move.window = target;
	move.data.l[0] = object->source()->winId();
	move.data.l[1] = 0; // flags
	move.data.l[2] = (globalPos.x() << 16) + globalPos.y();
	move.data.l[3] = qt_x_time;
	move.data.l[4] = qtaction_to_xdndaction( global_requested_action );

	if ( w )
	    qt_handle_xdnd_position( w, (const XEvent *)&move, FALSE );
	else
	    XSendEvent( qt_xdisplay(), proxy_target, FALSE, emask,
			(XEvent*)&move );
    } else {
	if ( willDrop ) {
	    willDrop = FALSE;
	    updateCursor();
	}
    }
}


void QDragManager::drop()
{
    if ( !qt_xdnd_current_target )
	return;

    delete qt_xdnd_deco;
    qt_xdnd_deco = 0;

    XClientMessageEvent drop;
    drop.type = ClientMessage;
    drop.window = qt_xdnd_current_target;
    drop.format = 32;
    drop.message_type = qt_xdnd_drop;
    drop.data.l[0] = object->source()->winId();
    drop.data.l[1] = 1 << 24; // flags
    drop.data.l[2] = 0; // ###
    drop.data.l[3] = qt_x_time;
    drop.data.l[4] = 0;

    QWidget * w = QWidget::find( qt_xdnd_current_proxy_target );

    int emask = NoEventMask;
    if ( w && w->isDesktop() && !w->acceptDrops() ) {
	emask = EnterWindowMask;
	w = 0;
    }

    if ( w )
	qt_handle_xdnd_drop( w, (const XEvent *)&drop, FALSE );
    else
	XSendEvent( qt_xdisplay(), qt_xdnd_current_proxy_target, FALSE, emask,
		    (XEvent*)&drop );

#ifndef QT_NO_CURSOR
    if ( restoreCursor ) {
	QApplication::restoreOverrideCursor();
	restoreCursor = FALSE;
    }
#endif
}



bool qt_xdnd_handle_badwindow()
{
    if ( qt_xdnd_source_object && qt_xdnd_current_target ) {
	qt_xdnd_current_target = 0;
	qt_xdnd_current_proxy_target = 0;
	delete qt_xdnd_source_object;
	qt_xdnd_source_object = 0;
	delete qt_xdnd_deco;
	qt_xdnd_deco = 0;
	return TRUE;
    }
    if ( qt_xdnd_dragsource_xid ) {
	qt_xdnd_dragsource_xid = 0;
	if ( qt_xdnd_current_widget ) {
	    QDragLeaveEvent e;
	    QApplication::sendEvent( qt_xdnd_current_widget, &e );
	    qt_xdnd_current_widget = 0;
	}
	return TRUE;
    }
    return FALSE;
}


// NOT REVISED
/*!
  \class QDragMoveEvent qevent.h
  \brief Event sent as a drag-and-drop is in progress.

  When a widget \link QWidget::setAcceptDrops() accepts drop events\endlink,
  it will receive this event repeatedly while the the drag is inside that
  widget.  The widget should examine the event, especially
  seeing what data it \link QDragMoveEvent::provides provides\endlink,
  and accept() the drop if appropriate.

  Note that this class inherits most of its functionality from QDropEvent.
*/


/*!  Returns TRUE if this event provides format \a mimeType or
  FALSE if it does not.

  \sa data()
*/

bool QDropEvent::provides( const char *mimeType ) const
{
    if ( qt_motifdnd_active ) {
	if ( 0 == qstrnicmp( mimeType, "text/", 5 ) )
	    return TRUE;
	else
	    return FALSE;
    }

    int n=0;
    const char* f;
    do {
	f = format( n );
	if ( !f )
	    return FALSE;
	n++;
    } while( qstricmp( mimeType, f ) );
    return TRUE;
}

void qt_xdnd_handle_selection_request( const XSelectionRequestEvent * req )
{
    if ( !req )
	return;
    XEvent evt;
    evt.xselection.type = SelectionNotify;
    evt.xselection.display = req->display;
    evt.xselection.requestor = req->requestor;
    evt.xselection.selection = req->selection;
    evt.xselection.target = req->target;
    evt.xselection.property = None;
    evt.xselection.time = req->time;
    const char* format = qt_xdnd_atom_to_str( req->target );
    if ( format && qt_xdnd_source_object &&
	 qt_xdnd_source_object->provides( format ) ) {
	QByteArray a = qt_xdnd_source_object->encodedData(format);
	XChangeProperty ( qt_xdisplay(), req->requestor, req->property,
			  req->target, 8, PropModeReplace,
			  (unsigned char *)a.data(), a.size() );
	evt.xselection.property = req->property;
    }
    // ### this can die if req->requestor crashes at the wrong
    // ### moment
    XSendEvent( qt_xdisplay(), req->requestor, False, 0, &evt );
}

/*
	XChangeProperty ( qt_xdisplay(), req->requestor, req->property,
			  XA_STRING, 8,
			  PropModeReplace,
			  (uchar *)d->text(), strlen(d->text()) );
	evt.xselection.property = req->property;
*/

static QByteArray qt_xdnd_obtain_data( const char *format )
{
    QByteArray result;

    QWidget* w;
    if ( qt_xdnd_dragsource_xid && qt_xdnd_source_object &&
	 (w=QWidget::find( qt_xdnd_dragsource_xid ))
	   && (!w->isDesktop() || w->acceptDrops()) )
    {
	QDragObject * o = qt_xdnd_source_object;
	if ( o->provides( format ) )
	    result = o->encodedData(format);
	return result;
    }

    Atom * a = qt_xdnd_str_to_atom( format );
    if ( !a || !*a )
	return result;

    if ( !qt_xdnd_target_data )
	qt_xdnd_target_data = new QIntDict<QByteArray>( 17 );

    if ( qt_xdnd_target_data->find( (int)*a ) ) {
	result = *(qt_xdnd_target_data->find( (int)*a ));
    } else {
	if ( XGetSelectionOwner( qt_xdisplay(),
				 qt_xdnd_selection ) == None )
	    return result; // should never happen?

	QWidget* tw = qt_xdnd_current_widget;
	if ( qt_xdnd_current_widget->isDesktop() ) {
	    tw = new QWidget;
	}
	XConvertSelection( qt_xdisplay(),
			   qt_xdnd_selection, *a,
			   qt_xdnd_selection,
			   tw->winId(), CurrentTime );
	XFlush( qt_xdisplay() );

	XEvent xevent;
	bool got=qt_xclb_wait_for_event( qt_xdisplay(),
				      tw->winId(),
				      SelectionNotify, &xevent, 5000);
	if ( got ) {
	    Atom type;

	    if ( qt_xclb_read_property( qt_xdisplay(),
					tw->winId(),
					qt_xdnd_selection, TRUE,
					&result, 0, &type, 0, FALSE ) ) {
		if ( type == qt_incr_atom ) {
		    int nbytes = result.size() >= 4 ? *((int*)result.data()) : 0;
		    result = qt_xclb_read_incremental_property( qt_xdisplay(),
								tw->winId(),
								qt_xdnd_selection,
								nbytes, FALSE );
		} else if ( type != *a ) {
		    // (includes None) qDebug( "Qt clipboard: unknown atom %ld", type);
		}
#if 0
		// this needs to be matched by a qt_xdnd_target_data->clear()
		// when each drag is finished.  for 2.0, we do the safe thing
		// and disable the entire caching.
		if ( type != None )
		    qt_xdnd_target_data->insert( (int)((long)a), new QByteArray(result) );
#endif
	    }
	}
	if ( qt_xdnd_current_widget->isDesktop() ) {
	    delete tw;
	}
    }

    return result;
}


/*
  Enable drag and drop for widget w by installing the proper
  properties on w's toplevel widget.
*/
bool qt_dnd_enable( QWidget* w, bool on )
{
    w = w->topLevelWidget();

    if ( on ) {
	if ( ( (QExtraWidget*)w)->topData()->dnd )
	    return TRUE; // been there, done that
	((QExtraWidget*)w)->topData()->dnd  = 1;
    }

    qt_motifdnd_enable( w, on );
    return qt_xdnd_enable( w, on );
}


/*!
  \class QDropEvent qevent.h

  \brief Event sent when a drag-and-drop is completed.

  When a widget \link QWidget::setAcceptDrops() accepts drop events\endlink,
  it will receive this event if it has accepted the most recent
  QDragEnterEvent or QDragMoveEvent sent to it.

  The widget should use data() to extract data in an
  appropriate format.
*/


/*! \fn QDropEvent::QDropEvent (const QPoint & pos, Type typ)

  Constructs a drop event that drops a drop of type \a typ on point \a
  pos.
*/ // ### pos is in which coordinate system?


/*!  Returns a byte array containing the payload data of this drag, in
  \a format.

  data() normally needs to get the data from the drag source, which is
  potentially very slow, so it's advisable to call this function only
  if you're sure that you will need the data in \a format.

  The resulting data will have a size of 0 if the format was not
  available.

  \sa format() QByteArray::size()
*/

QByteArray QDropEvent::encodedData( const char *format ) const
{
    if ( qt_motifdnd_active )
	return qt_motifdnd_obtain_data();
    return qt_xdnd_obtain_data( format );
}

/*!  Returns a string describing one of the available data types for
  this drag.  Common examples are "text/plain" and "image/gif".  If \a
  n is less than zero or greater than the number of available data
  types, format() returns 0.

  This function is provided mainly for debugging.  Most drop targets
  will use provides().

  \sa data() provides()
*/

const char* QDropEvent::format( int n ) const
{
    if ( qt_motifdnd_active ) {
	if ( n == 0 )
	    return "text/plain";
	else if ( n == 1 )
	    return "text/uri-list";
	else
	    return 0;
    }

    int i = 0;
    while( i<n && qt_xdnd_types[i] )
	i++;
    if ( i < n )
	return 0;

    const char* name = qt_xdnd_atom_to_str( qt_xdnd_types[i] );
    if ( !name )
	return 0; // should never happen

    return name;
}

bool QDragManager::drag( QDragObject * o, QDragObject::DragMode mode )
{
    if ( object == o || !o || !o->parent() )
	return FALSE;

    if ( object ) {
	cancel();
	qApp->removeEventFilter( this );
	beingCancelled = FALSE;
    }

    qt_xdnd_source_object = o;
    qt_xdnd_deco = new QShapedPixmapWidget();

    willDrop = FALSE;

    object = o;
    updatePixmap();

    dragSource = (QWidget *)(object->parent());

    qApp->installEventFilter( this );
    qt_xdnd_source_current_time = qt_x_time;
    XSetSelectionOwner( qt_xdisplay(), qt_xdnd_selection,
			dragSource->topLevelWidget()->winId(),
			qt_xdnd_source_current_time );
    oldstate = ButtonState(-1); // #### Should use state that caused the drag
    drag_mode = mode;
    global_accepted_action = QDropEvent::Copy; // #####
    updateMode(ButtonState(0));
    qt_xdnd_source_sameanswer = QRect();
    move(QCursor::pos());
    heartbeat = startTimer(200);

#ifndef QT_NO_CURSOR
    qApp->setOverrideCursor( arrowCursor );
    restoreCursor = TRUE;
    updateCursor();
#endif

    qApp->enter_loop(); // Do the DND.

#ifndef QT_NO_CURSOR
    qApp->restoreOverrideCursor();
#endif

    delete qt_xdnd_deco;
    qt_xdnd_deco = 0;
    killTimer(heartbeat);
    heartbeat = 0;

    return global_accepted_action == QDropEvent::Copy
	    && global_requested_action == QDropEvent::Move; // source del?

    // qt_xdnd_source_object persists for a while...
}

void QDragManager::updatePixmap()
{
    if ( qt_xdnd_deco ) {
	QPixmap pm;
	QPoint pm_hot(default_pm_hotx,default_pm_hoty);
	if ( object ) {
	    pm = object->pixmap();
	    if ( !pm.isNull() )
		pm_hot = object->pixmapHotSpot();
	}
	if ( pm.isNull() ) {
	    if ( !defaultPm )
		defaultPm = new QPixmap(default_pm);
	    pm = *defaultPm;
	}
	qt_xdnd_deco->setPixmap(pm);
	qt_xdnd_deco->move(QCursor::pos()-pm_hot);
	//qt_xdnd_deco->repaint(FALSE);
	if ( willDrop ) {
	    qt_xdnd_deco->show();
	} else {
	    qt_xdnd_deco->hide();
	}
    }
}

#endif // QT_NO_DRAGANDDROP
