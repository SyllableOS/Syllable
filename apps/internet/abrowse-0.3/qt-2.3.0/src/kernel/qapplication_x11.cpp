/****************************************************************************
** $Id$
**
** Implementation of X11 startup routines and event handling
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

// NOT REVISED

#define select          _qt_hide_select
#define gettimeofday    _qt_hide_gettimeofday

#include "qglobal.h"

// XPG4v2 needed by FD_* macros.
#ifndef _XOPEN_SOURCE_EXTENDED
#  define _XOPEN_SOURCE_EXTENDED
#endif

// XPG5 would be needed to request the BSD b*() and strcasecmp() functions.
// However we keep XPG4v2 and request them "manually" on every OS so that
// all this works even when XPG5 is not available. Easier this way for now.

#if defined(_OS_WIN32_)
#undef select
#include <windows.h>
#define HANDLE QT_HANDLE
#endif
#include "qapplication.h"
#include "qwidget.h"
#include "qobjectlist.h"
#include "qobjectdict.h"
#include "qwidgetlist.h"
#include "qwidgetintdict.h"
#include "qbitarray.h"
#ifndef __ATHEOS__
#include "qpainter.h"
#endif
#include "qpixmapcache.h"
#include "qdatetime.h"
#include "qtextcodec.h"
#include "qdatastream.h"
#include "qbuffer.h"
#include "qsocketnotifier.h"
#ifndef __ATHEOS__
#include "qsessionmanager.h"
#endif // __ATHEOS__
#include "qvaluelist.h"
#include "qdict.h"
#include "qguardedptr.h"
#include "qclipboard.h"
#include "qwhatsthis.h" // ######## dependency
#include "qwindowsstyle.h" // ######## dependency
#include "qmotifplusstyle.h" // ######## dependency
#include <stdlib.h>
#ifndef QT_NO_SM_SUPPORT
#include <pwd.h>
#endif
#include <ctype.h>
#include <locale.h>
#include <errno.h>
#define	 GC GC_QQQ

#if defined(QT_THREAD_SUPPORT)
#include "qthread.h"
#endif

#if defined(DEBUG) && defined(_OS_LINUX_)
#include "qfile.h"
#include <unistd.h>
#endif

#ifdef __ATHEOS__
#define NO_XIM
#define QT_NO_STYLE
#define QT_NO_CURSOR
#include <sys/unistd.h>
#include <sys/time.h>
#endif


#if defined(_OS_WIN32_)
#undef gettimeofday
#endif

#if defined(_OS_UNIX_)

#if defined(_OS_SOLARIS_) || defined(_OS_UNIXWARE7_)
// Needed for FIONREAD.
// FIONREAD is #defined in <sys/filio.h>.
// Have <sys/ioctl.h> include <sys/filio.h>.
#  define BSD_COMP
#endif
#if defined(_OS_OSF_)
#  define _OSF_SOURCE // for FIONREAD on Tru64 4.0f
#endif
#if defined(_OS_HPUX_)
// needed for ioctl() and FIONREAD
#  define _HPUX_SOURCE
#endif
#include <sys/ioctl.h>
#if defined(_OS_SOLARIS_) || defined(_OS_UNIXWARE7_)
#  undef BSD_COMP
#endif
#if defined(_OS_OSF_)
#  undef _OSF_SOURCE
#endif
#if defined(_OS_HPUX_)
#  undef _HPUX_SOURCE
#endif

#if defined(_OS_SCO_)
#  include <sys/socket.h> // for FIONREAD on SCO OpenServer 5.0.x
#endif

#if defined(_OS_DYNIX_)
#  include <sys/sockio.h> // for FIONREAD on Dynix 4.x
#endif

static int qt_thread_pipe[2];
#endif


#if defined(_OS_LINUX_)
// This is one way to request strcasecmp().
// In the GNU C library another way would be to define __USE_BSD.
#include <strings.h>
#include <string.h>
#endif

#if defined(_OS_OSF_)
// This is one way to request BSD b*() string functions.
// On Tru64 another way would be to define _OSF_SOURCE.
#  include <strings.h>
#endif

#if defined(_OS_SCO_)
#  include <strings.h>
#endif

#if defined(_OS_IRIX_)
// Please add comments! Which version of Irix?
// Why <bstring.h> instead of <strings.h>?
#include <bstring.h>
#endif

#if defined(_OS_AIX_) || defined(_OS_UNIXWARE7_) || defined (_OS_HPUX_)
// Please add comments! Which version of AIX? Why?
// needed for strcasecmp() on HP-UX 10.xx
#include <strings.h>
#endif

#include "qt_x11.h"

#if !defined(X11R4) && !defined(__ATHEOS__)
#include <X11/Xlocale.h>
#endif


#if defined(_OS_AIX_) && defined(_CC_GNU_)
// Please add comments! Which version of AIX? Why?
// Adding some #defines like the IBM compiler does ought to be enough.
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#endif

#if defined(_OS_QNX_)
#include <sys/select.h>
#endif

#if defined(_CC_MSVC_)
#pragma warning(disable: 4018)
#undef open
#undef close
#endif

#if defined(_OS_WIN32_) && defined(gettimeofday)
#undef gettimeofday
#include <sys/timeb.h>
inline void gettimeofday( struct timeval *t, struct timezone * )
{
    struct _timeb tb;
    _ftime( &tb );
    t->tv_sec  = tb.time;
    t->tv_usec = tb.millitm * 1000;
}
#else
#undef gettimeofday
extern "C" int gettimeofday( struct timeval *, struct timezone * );
#endif // _OS_WIN32 etc.
#if !defined(_OS_WIN32_)
#undef select
extern "C" int select( int, void *, void *, void *, struct timeval * );
#endif

//#define X_NOT_BROKEN
#ifdef X_NOT_BROKEN
// Some X libraries are built with setlocale #defined to _Xsetlocale,
// even though library users are then built WITHOUT such a definition.
// This creates a problem - Qt might setlocale() one value, but then
// X looks and doesn't see the value Qt set.  The solution here is to
// implement _Xsetlocale just in case X calls it - redirecting it to
// the real libC version.
//
#ifndef setlocale
extern "C" char *_Xsetlocale(int category, const char *locale);
char *_Xsetlocale(int category, const char *locale)
{
    //qDebug("_Xsetlocale(%d,%s),category,locale");
    return setlocale(category,locale);
}
#endif
#endif

// resolve the conflict between X11's FocusIn and QEvent::FocusIn
#ifndef __ATHEOS__
const int XFocusOut = FocusOut;
const int XFocusIn = FocusIn;
#undef FocusOut
#undef FocusIn

const int XKeyPress = KeyPress;
const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease
#endif // __ATHEOS__

#if defined(_OS_AIX_) && !defined(bzero)
// For FD_ZERO, which the X11 libraries define to use bzero(), even
// though the system libraries don't have that function.
#define bzero( s, n ) memset( (s), 0, (n) )
#endif

// Fix old X libraries
#ifndef XK_KP_Home
#define XK_KP_Home              0xFF95
#endif
#ifndef XK_KP_Left
#define XK_KP_Left              0xFF96
#endif
#ifndef XK_KP_Up
#define XK_KP_Up                0xFF97
#endif
#ifndef XK_KP_Right
#define XK_KP_Right             0xFF98
#endif
#ifndef XK_KP_Down
#define XK_KP_Down              0xFF99
#endif
#ifndef XK_KP_Prior
#define XK_KP_Prior             0xFF9A
#endif
#ifndef XK_KP_Next
#define XK_KP_Next              0xFF9B
#endif
#ifndef XK_KP_End
#define XK_KP_End               0xFF9C
#endif
#ifndef XK_KP_Insert
#define XK_KP_Insert            0xFF9E
#endif
#ifndef XK_KP_Delete
#define XK_KP_Delete            0xFF9F
#endif

/*****************************************************************************
  Internal variables and functions
 *****************************************************************************/

static char    *appName;			// application name
static char    *appFont		= 0;		// application font
static char    *appBGCol	= 0;		// application bg color
static char    *appFGCol	= 0;		// application fg color
static char    *appBTNCol	= 0;		// application btn color
static char    *mwGeometry	= 0;		// main widget geometry
static char    *mwTitle		= 0;		// main widget title
//Ming-Che 10/10
static char    *ximServer	= 0;		// XIM Server will connect to
static bool	mwIconic	= FALSE;	// main widget iconified
//Ming-Che 10/10
#ifndef __ATHEOS__
static bool	noxim		= False;	// connect to xim or not
#endif
static Display *appDpy		= 0;		// X11 application display
static char    *appDpyName	= 0;		// X11 display name
static bool	appForeignDpy	= FALSE;        // we didn't create display
static bool	appSync		= FALSE;	// X11 synchronization
#if defined(DEBUG)
static bool	appNoGrab	= FALSE;	// X11 grabbing enabled
static bool	appDoGrab	= FALSE;	// X11 grabbing override (gdb)
#endif
static int	appScreen;			// X11 screen number
#ifndef __ATHEOS__
static Window	appRootWin;			// X11 root window
#endif
static bool	app_save_rootinfo = FALSE;	// save root info

static bool	app_do_modal	= FALSE;	// modal mode
#ifndef __ATHEOS__
static Window	curWin = 0;			// current window
static int	app_Xfd;			// X network socket
static fd_set	app_readfds;			// fd set for reading
static fd_set	app_writefds;			// fd set for writing
static fd_set	app_exceptfds;			// fd set for exceptions

static GC	app_gc_ro	= 0;		// read-only GC
static GC	app_gc_tmp	= 0;		// temporary GC
static GC	app_gc_ro_m	= 0;		// read-only GC (monochrome)
static GC	app_gc_tmp_m	= 0;		// temporary GC (monochrome)
Atom		qt_wm_protocols		= 0;	// window manager protocols
Atom		qt_wm_delete_window	= 0;	// delete window protocol
Atom 		qt_wm_take_focus	= 0;	// take focus window protocol
static Atom	qt_qt_scrolldone 	= 0;	// scroll synchronization
Atom		qt_net_wm_context_help	= 0;	// context help

static Atom	qt_xsetroot_id		= 0;
Atom		qt_selection_property	= 0;
Atom		qt_selection_sentinel	= 0;
Atom		qt_wm_state		= 0;
static Atom 	qt_desktop_properties	= 0;	// Qt desktop properties
static Atom 	qt_input_encoding 		= 0;	// Qt desktop properties
static Atom 	qt_resource_manager	= 0;	// X11 Resource manager
Atom 		qt_sizegrip		= 0;	// sizegrip
Atom 		qt_wm_client_leader	= 0;
Atom 		qt_window_role		= 0;
Atom 		qt_sm_client_id		= 0;
Atom 		qt_xa_motif_wm_hints	= 0;
Atom 		qt_kwin_running	= 0;
Atom 		qt_kwm_running	= 0;
Atom 		qt_gbackground_properties	= 0;
Atom 		qt_x_incr		= 0;

static Window	mouseActWindow	     = 0;	// window where mouse is
#endif // __ATHEOS__
static int	mouseButtonPressed   = 0;	// last mouse button pressed
static int	mouseButtonState     = 0;	// mouse button state
#ifndef __ATHEOS__
static Time	mouseButtonPressTime = 0;	// when was a button pressed
#endif
static short	mouseXPos, mouseYPos;		// mouse pres position in act window
static short	mouseGlobalXPos, mouseGlobalYPos; // global mouse press position

extern QWidgetList *qt_modal_stack;		// stack of modal widgets
static QWidget     *popupButtonFocus = 0;
static QWidget     *popupOfPopupButtonFocus = 0;
static bool	    popupCloseDownMode = FALSE;
static bool	    popupGrabOk;

static bool sm_blockUserInput = FALSE;		// session management

// one day in the future we will be able to have static objects in libraries....
static QGuardedPtr<QWidget>* activeBeforePopup = 0; // focus handling with popups

typedef void (*VFPTR)();
typedef QValueList<VFPTR> QVFuncList;
static QVFuncList *postRList = 0;		// list of post routines

typedef int (*QX11EventFilter) (XEvent*);
QX11EventFilter qt_set_x11_event_filter (QX11EventFilter filter);

static QX11EventFilter qt_x11_event_filter = 0;
QX11EventFilter qt_set_x11_event_filter (QX11EventFilter filter)
{
    QX11EventFilter old_filter = qt_x11_event_filter;
    qt_x11_event_filter = filter;
    return old_filter;
}
#ifndef __ATHEOS__
static bool qt_x11EventFilter( XEvent* ev )
{
    if ( qt_x11_event_filter  && qt_x11_event_filter( ev )  )
	return TRUE;
    return qApp->x11EventFilter( ev );
}
#endif // __ATHEOS__

void qt_install_preselect_handler( VFPTR );
void qt_remove_preselect_handler( VFPTR );
static QVFuncList *qt_preselect_handler = 0;
void qt_install_postselect_handler( VFPTR );
void qt_remove_postselect_handler( VFPTR );
static QVFuncList *qt_postselect_handler = 0;
void qt_install_preselect_handler( VFPTR handler )
{
    if ( !qt_preselect_handler )
	qt_preselect_handler = new QVFuncList;
    qt_preselect_handler->append( handler );
}
void qt_remove_preselect_handler( VFPTR handler )
{
    if ( qt_preselect_handler ) {
	QVFuncList::Iterator it = qt_preselect_handler->find( handler );
	if ( it != qt_preselect_handler->end() )
		qt_preselect_handler->remove( it );
    }
}
void qt_install_postselect_handler( VFPTR handler )
{
    if ( !qt_postselect_handler )
	qt_postselect_handler = new QVFuncList;
    qt_postselect_handler->prepend( handler );
}
void qt_remove_postselect_handler( VFPTR handler )
{
    if ( qt_postselect_handler ) {
	QVFuncList::Iterator it = qt_postselect_handler->find( handler );
	if ( it != qt_postselect_handler->end() )
		qt_postselect_handler->remove( it );
    }
}



static void	initTimers();
static void	cleanupTimers();
static timeval	watchtime;			// watch if time is turned back
timeval		*qt_wait_timer();
timeval 	*qt_wait_timer_max = 0;
int 		qt_activate_timers();

#if !defined(NO_XIM)
XIM	qt_xim = 0;
XIMStyle qt_xim_style = 0;
static XIMStyle xim_preferred_style = XIMPreeditPosition | XIMStatusNothing;
#endif
static int composingKeycode=0;
static QTextCodec * input_mapper = 0;

QObject	       *qt_clipboard = 0;
#ifndef __ATHEOS__
Time		qt_x_time = CurrentTime;
#endif
extern bool	qt_check_selection_sentinel( XEvent* ); //def in qclipboard_x11

#ifndef __ATHEOS__
static void	qt_save_rootinfo();
#endif // __ATHEOS__
static bool	qt_try_modal( QWidget *, XEvent * );
void		qt_reset_color_avail();		// defined in qcolor_x11.cpp

int		qt_ncols_option  = 216;		// used in qcolor_x11.cpp
int		qt_visual_option = -1;
bool		qt_cmap_option	 = FALSE;
QWidget	       *qt_button_down	 = 0;		// widget got last button-down

extern bool qt_is_gui_used; // qapplication.cpp

extern void qt_dispatchEnterLeave( QWidget*, QWidget* ); // qapplication.cpp

struct QScrollInProgress {
    static long serial;
    QScrollInProgress( QWidget* w, int x, int y ) :
    id( serial++ ), scrolled_widget( w ), dx( x ), dy( y ) {}
    long id;
    QWidget* scrolled_widget;
    int dx, dy;
};
long QScrollInProgress::serial=0;
static QList<QScrollInProgress> *sip_list = 0;


// stuff in qt_xdnd.cpp
// setup
extern void qt_xdnd_setup();
// x event handling
extern void qt_handle_xdnd_enter( QWidget *, const XEvent *, bool );
extern void qt_handle_xdnd_position( QWidget *, const XEvent *, bool );
extern void qt_handle_xdnd_status( QWidget *, const XEvent *, bool );
extern void qt_handle_xdnd_leave( QWidget *, const XEvent *, bool );
extern void qt_handle_xdnd_drop( QWidget *, const XEvent *, bool );
extern void qt_handle_xdnd_finished( QWidget *, const XEvent *, bool );
#ifndef __ATHEOS__
extern void qt_xdnd_handle_selection_request( const XSelectionRequestEvent * );
#endif
extern bool qt_xdnd_handle_badwindow();

extern void qt_motifdnd_handle_msg( QWidget *, const XEvent *, bool );
extern void qt_x11_motifdnd_init();

// client message atoms
#ifndef __ATHEOS__
extern Atom qt_xdnd_enter;
extern Atom qt_xdnd_position;
extern Atom qt_xdnd_status;
extern Atom qt_xdnd_leave;
extern Atom qt_xdnd_drop;
extern Atom qt_xdnd_finished;
// xdnd selection atom
extern Atom qt_xdnd_selection;
#endif // __ATHEOS__


// Paint event clipping magic
extern void qt_set_paintevent_clipping( QPaintDevice* dev, const QRegion& region);
extern void qt_clear_paintevent_clipping();


// Palette handling
extern QPalette *qt_std_pal;
extern void qt_create_std_palette();

#ifndef __ATHEOS__
void qt_x11_intern_atom( const char *, Atom * );
#endif // __ATHEOS__

static QList<QWidget>* deferred_map_list = 0;
static void qt_deferred_map_cleanup()
{
    delete deferred_map_list;
    deferred_map_list = 0;
}
void qt_deferred_map_add( QWidget* w)
{
    if ( !deferred_map_list ) {
	deferred_map_list = new QList<QWidget>;
	qAddPostRoutine( qt_deferred_map_cleanup );
    }
    deferred_map_list->append( w );
};
void qt_deferred_map_take( QWidget* w )
{
    if (deferred_map_list ) {
	deferred_map_list->remove( w );
    }
}
static bool qt_deferred_map_contains( QWidget* w )
{
    if (!deferred_map_list)
	return FALSE;
    else
	return deferred_map_list->contains( w );
}


#ifndef __ATHEOS__
class QETWidget : public QWidget		// event translator widget
{
public:
    void setWState( WFlags f )		{ QWidget::setWState(f); }
    void clearWState( WFlags f )	{ QWidget::clearWState(f); }
    void setWFlags( WFlags f )		{ QWidget::setWFlags(f); }
    void clearWFlags( WFlags f )	{ QWidget::clearWFlags(f); }
    bool translateMouseEvent( const XEvent * );
    bool translateKeyEventInternal( const XEvent *, int& count, QString& text, int& state, char& ascii, int &code );
    bool translateKeyEvent( const XEvent *, bool grab );
    bool translatePaintEvent( const XEvent * );
    bool translateConfigEvent( const XEvent * );
    bool translateCloseEvent( const XEvent * );
    bool translateScrollDoneEvent( const XEvent * );
    bool translateWheelEvent( int global_x, int global_y, int delta, int state );
};
#endif

/*!
  \internal
*/
//Ming-Che 04/10
void QApplication::close_xim()
{
#if !defined(NO_XIM)
    // Calling XCloseIM gives a Purify FMR error
    // XCloseIM( qt_xim );
    // We prefer a less serious memory leak
    qt_xim = 0;
    QWidgetList *list= qApp->topLevelWidgets();
    QWidgetListIt it(*list);
    while(it.current()) {
	it.current()->topData()->xic=0;
	++it;
    }
    delete list;
#endif
}


/*****************************************************************************
  Default X error handlers
 *****************************************************************************/

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static bool x11_ignore_badwindow;
static bool x11_badwindow;

    // starts to ignore bad window errors from X
void qt_ignore_badwindow()
{
    x11_ignore_badwindow = TRUE;
    x11_badwindow = FALSE;
}

    // ends ignoring bad window errors and returns whether an error
    // had happen.
bool qt_badwindow()
{
    x11_ignore_badwindow = FALSE;
    return x11_badwindow;
}
#ifndef __ATHEOS__
static int (*original_x_errhandler)(Display*dpy,XErrorEvent*);
static int (*original_xio_errhandler)(Display*dpy);

static int qt_x_errhandler( Display *dpy, XErrorEvent *err )
{
    if ( err->error_code == BadWindow ) {
	x11_badwindow = TRUE;
	if ( err->request_code == 25 && qt_xdnd_handle_badwindow() )
	    return 0;
	if ( x11_ignore_badwindow )
	    return 0;
    }
    else if ( err->error_code == BadMatch
	      && err->request_code == 42 /* X_SetInputFocus */ ) {
	return 0;
    }

    char errstr[256];
    XGetErrorText( dpy, err->error_code, errstr, 256 );
    qWarning( "X Error: %s %d\n  Major opcode:  %d", errstr, err->error_code, err->request_code );
    //### we really should distinguish between severe, non-severe and
    //### application specific errors
    return 0;
}


static int qt_xio_errhandler( Display * )
{
    qWarning( "%s: Fatal IO error: client killed", appName );
    exit( 1 );
    //### give the application a chance for a proper shutdown instead,
    //### exit(1) doesn't help.
    return 0;
}
#endif // __ATHEOS__

#if defined(Q_C_CALLBACKS)
}
#endif


// Memory leak: if the app exits before qt_init_internal(), this dict
// isn't released correctly.
#ifndef __ATHEOS__
static QAsciiDict<Atom> *atoms_to_be_created = 0;
static bool create_atoms_now = 0;
#endif // __ATHEOS__

/*****************************************************************************
  qt_x11_intern_atom() - efficiently interns an atom, now or later.

  If the application is being initialized, this function stores the
  adddress of the atom and qt_init_internal will do the actual work
  quickly.  If the application is running, the atom is created here.

  Neither argument may point to temporary variables.
 *****************************************************************************/

#ifndef __ATHEOS__

void qt_x11_intern_atom( const char *name, Atom *result)
{
    if ( !name || !result || *result )
	return;

    if ( create_atoms_now ) {
	*result = XInternAtom(appDpy, name, FALSE );
    } else {
	if ( !atoms_to_be_created ) {
	    atoms_to_be_created = new QAsciiDict<Atom>;
	    atoms_to_be_created->setAutoDelete( FALSE );
	}
	atoms_to_be_created->insert( name, result );
	*result = 0;
    }
}


static void qt_x11_process_intern_atoms()
{
    if ( atoms_to_be_created ) {
#if defined(XlibSpecificationRelease) && (XlibSpecificationRelease >= 6)
	int i = atoms_to_be_created->count();
	Atom * res = (Atom *)malloc( i * sizeof( Atom ) );
	Atom ** resp = (Atom **)malloc( i * sizeof( Atom* ) );
	char ** names = (char **)malloc( i * sizeof(const char*));

	i = 0;
	QAsciiDictIterator<Atom> it( *atoms_to_be_created );
	while( it.current() ) {
	    res[i] = 0;
	    resp[i] = it.current();
	    names[i] = qstrdup(it.currentKey());
	    i++;
	    ++it;
	}
	XInternAtoms( appDpy, names, i, FALSE, res );
	while( i ) {
	    i--;
	    delete [] names[i];
	    if ( res[i] && resp[i] )
		*(resp[i]) = res[i];
	}
	free( res );
	free( resp );
	free( names );
#else
	QAsciiDictIterator<Atom> it( *atoms_to_be_created );
	Atom * result;
	const char * name;
	while( (result = it.current()) != 0 ) {
	    name = it.currentKey();
	    ++it;
	    *result = XInternAtom(appDpy, name, FALSE );
	}
#endif
	delete atoms_to_be_created;
	atoms_to_be_created = 0;
	create_atoms_now = TRUE;
    }
}

static bool seems_like_KDE_is_running = FALSE;

// read the _QT_DESKTOP_PROPERTIES property and apply the settings to
// the application
static bool qt_set_desktop_properties()
{

    if ( !qt_std_pal )
	qt_create_std_palette();

    Atom type;
    int format;
    ulong  nitems, after = 1;
    long offset = 0;
    const char *data;

    int e = XGetWindowProperty( appDpy, appRootWin, qt_desktop_properties, 0, 1,
				FALSE, AnyPropertyType, &type, &format, &nitems,
				&after,  (unsigned char**)&data );
    if ( data )
	XFree(  (unsigned char*)data );
    if ( e != Success || !nitems )
	return FALSE;

    QBuffer  properties;
    properties.open( IO_WriteOnly );
    while (after > 0) {
	XGetWindowProperty( appDpy, appRootWin, qt_desktop_properties,
			    offset, 1024, FALSE, AnyPropertyType,
			    &type, &format, &nitems, &after, (unsigned char**) &data );
	if (format == 8) {
	    properties.writeBlock(data, nitems);
	    offset += nitems / 4;
	}

	XFree( (unsigned char *) data );
    }

    QDataStream d( properties.buffer(), IO_ReadOnly );

    QPalette pal;
    QFont font;
    d >> pal >> font;

    // check the palette to see if the disabled foreground and background color
    // are the same, if they are, we are going to do a hack and set the disabled
    // foreground color to something sensible - this works around a bug in KDE that
    // sets the disabled foreground color to the active foregroundcolor
    QColor actfg(pal.color(QPalette::Active, QColorGroup::Foreground)),
	disfg(pal.color(QPalette::Disabled, QColorGroup::Foreground));
    if (actfg == disfg) {
	int h, s, v;
	disfg.hsv( &h, &s, &v );
	if (v > 128)
	    // dark bg, light fg - need a darker disabled fg
	    disfg = disfg.dark();
	else if (disfg != Qt::black)
	    // light bg, dark fg - need a lighter disabled fg - but only if !black
	    disfg = disfg.light();
	else
	    // black fg - use darkgrey disabled fg
	    disfg = Qt::darkGray;

	pal.setColor(QPalette::Disabled, QColorGroup::Foreground, disfg);
    }

    if ( pal != *qt_std_pal && pal != QApplication::palette() )
	QApplication::setPalette( pal, TRUE );
    *qt_std_pal = pal;
    font.setCharSet(QFont::charSetForLocale());
    if ( font != QApplication::font() ) {
	QApplication::setFont( font, TRUE );
    }

    seems_like_KDE_is_running = TRUE;

    return TRUE;
}

// read the _QT_INPUT_ENCODING property and apply the settings to
// the application
static void qt_set_input_encoding()
{
    Atom type;
    int format;
    ulong  nitems, after = 1;
    const char *data;

    int e = XGetWindowProperty( appDpy, appRootWin, qt_input_encoding, 0, 1024,
				FALSE, XA_STRING, &type, &format, &nitems,
				&after,  (unsigned char**)&data );
    if ( e != Success || !nitems || type == None ) {
	// Always use the locale codec, since we have no examples of non-local
	// XIMs, and since we cannot get a sensible answer about the encoding
	// from the XIM.
	input_mapper = QTextCodec::codecForLocale();

    } else {
#if defined(_OS_QNX_)
	QString str( data );
	str = str.lower();
	if ( !str.compare( "locale" ) )
#else
	if ( !strcasecmp( data, "locale" ) )
#endif
	    input_mapper = QTextCodec::codecForLocale();
	else
	    input_mapper = QTextCodec::codecForName( data );
	// make sure we have an input codec
	if( !input_mapper )
	    input_mapper = QTextCodec::codecForName( "ISO 8859-1" );
    }
    if ( input_mapper->mibEnum() == 11 ) // 8859-8
	input_mapper = QTextCodec::codecForName( "ISO 8859-8-I");
    if( data )
	XFree( (unsigned char *) data );

}

// set font, foreground and background from x11 resources. The
// arguments may override the resource settings.
static void qt_set_x11_resources( const char* font = 0, const char* fg = 0,
				  const char* bg = 0, const char* button = 0 )
{
    if ( !qt_std_pal )
	qt_create_std_palette();

    QCString resFont, resFG, resBG, resEF;

    if ( QApplication::desktopSettingsAware() && !qt_set_desktop_properties() ) {
	int format;
	ulong  nitems, after = 1;
	QCString res;
	long offset = 0;
	Atom type = None;

	while (after > 0) {
	    char *data;
	    XGetWindowProperty( appDpy, appRootWin, qt_resource_manager,
				offset, 8192, FALSE, AnyPropertyType,
				&type, &format, &nitems, &after,
				(unsigned char**) &data );
	    res += data;
	    offset += 2048; // offset is in 32bit quantities... 8192/4 == 2048
	    if ( data )
		XFree(data);
	}

	QCString key, value;
	int l = 0, r;
	QCString apn = appName;
	int apnl = apn.length();
	int resl = res.length();

	while (l < resl) {
	    r = res.find( '\n', l );
	    if ( r < 0 )
		r = resl;
	    while ( isspace(res[l]) )
		l++;
	    bool mine = FALSE;
	    if ( res[l] == '*'
	      && (res[l+1] == 'f' || res[l+1] == 'b' || res[l+1] == 'g') )
	    {
		// OPTIMIZED, since we only want "*[fbg].."

		QCString item = res.mid( l, r - l ).simplifyWhiteSpace();
		int i = item.find( ":" );
		key = item.left( i ).stripWhiteSpace().mid(1);
		value = item.right( item.length() - i - 1 ).stripWhiteSpace();
		mine = TRUE;
	    } else if ( res[l] == appName[0] ) {
		if ( res.mid(l,apnl) == apn &&
			(res[l+apnl] == '.' || res[l+apnl] == '*' ) )
		{
		    QCString item = res.mid( l, r - l ).simplifyWhiteSpace();
		    int i = item.find( ":" );
		    key = item.left( i ).stripWhiteSpace().mid(apnl+1);
		    value = item.right( item.length() - i - 1 ).stripWhiteSpace();
		    mine = TRUE;
		}
	    }

	    if ( mine ) {
		if ( !font && key == "font")
		    resFont = value.copy();
		else if  ( !fg &&  key == "foreground" )
		    resFG = value.copy();
		else if ( !bg && key == "background")
		    resBG = value.copy();
		else if ( key == "guieffects")
		    resEF = value.copy();
		// NOTE: if you add more, change the [fbg] stuff above
	    }

	    l = r + 1;
	}
    }

    if ( resFont.isEmpty() )
	resFont = font;
    if ( resFG.isEmpty() )
	resFG = fg;
    if ( resBG.isEmpty() )
	resBG = bg;

    if ( !resFont.isEmpty() ) {				// set application font
	QFont fnt;
	fnt.setRawName( resFont );

	// override requested charset, unless given on the command-line
	if ( !font )
	    fnt.setCharSet( QFont::charSetForLocale() );

	if ( fnt != QApplication::font() )
	    QApplication::setFont( fnt, TRUE );
    }
    if ( button || !resBG.isEmpty() || !resFG.isEmpty() ) {// set app colors
	QColor btn;
	QColor bg;
	QColor fg;
	if ( !resBG.isEmpty() )
	    bg = QColor(QString(resBG));
	else
	    bg = qt_std_pal->normal().background();
	if ( !resFG.isEmpty() )
	    fg = QColor(QString(resFG));
	else
	    fg = qt_std_pal->normal().foreground();
	if ( button )
	    btn = QColor( button );
	else if ( !resBG.isEmpty() )
	    btn = bg;
	else
	    btn = qt_std_pal->normal().button();

	int h,s,v;
	fg.hsv(&h,&s,&v);
	QColor base = Qt::white;
	bool bright_mode = FALSE;
	if (v >= 255-50) {
	    base = btn.dark(150);
	    bright_mode = TRUE;
	}

	QColorGroup cg( fg, btn, btn.light(),
			btn.dark(), btn.dark(150), fg, Qt::white, base, bg );
	if (bright_mode) {
	    cg.setColor( QColorGroup::HighlightedText, base );
	    cg.setColor( QColorGroup::Highlight, Qt::white );
	} else {
	    cg.setColor( QColorGroup::HighlightedText, Qt::white );
	    cg.setColor( QColorGroup::Highlight, Qt::darkBlue );
	}
	QColor disabled( (fg.red()+btn.red())/2,
			 (fg.green()+btn.green())/2,
			 (fg.blue()+btn.blue())/2);
	QColorGroup dcg( disabled, btn, btn.light( 125 ), btn.dark(), btn.dark(150),
			 disabled, Qt::white, Qt::white, bg );
	if (bright_mode) {
	    dcg.setColor( QColorGroup::HighlightedText, base );
	    dcg.setColor( QColorGroup::Highlight, Qt::white );
	} else {
	    dcg.setColor( QColorGroup::HighlightedText, Qt::white );
	    dcg.setColor( QColorGroup::Highlight, Qt::darkBlue );
	}
	QPalette pal( cg, dcg, cg );
	if ( pal != *qt_std_pal && pal != QApplication::palette() )
	    QApplication::setPalette( pal, TRUE );
	*qt_std_pal = pal;
    }

    if ( !resEF.isEmpty() ) {
	QStringList effects = QStringList::split(" ",resEF);
	if ( effects.contains("general") )
	    QApplication::setEffectEnabled( Qt::UI_General, TRUE );
	if ( effects.contains("animatemenu") )
	    QApplication::setEffectEnabled( Qt::UI_AnimateMenu, TRUE );
	if ( effects.contains("fademenu") )
	    QApplication::setEffectEnabled( Qt::UI_FadeMenu, TRUE );
	if ( effects.contains("animatecombo") )
	    QApplication::setEffectEnabled( Qt::UI_AnimateCombo, TRUE );
	if ( effects.contains("animatetooltip") )
	    QApplication::setEffectEnabled( Qt::UI_AnimateTooltip, TRUE );
	if ( effects.contains("fadetooltip") )
	    QApplication::setEffectEnabled( Qt::UI_FadeTooltip, TRUE );
    }
}


/*
  Returns a truecolor visual (if there is one). 8-bit TrueColor visuals
  are ignored, unless the user has explicitly requested -visual TrueColor.
  The SGI X server usually has an 8 bit default visual, but the application
  can also ask for a truecolor visual. This is what we do if
  QApplication::colorSpec() is QApplication::ManyColor.
*/

static Visual *find_truecolor_visual( Display *dpy, int *depth, int *ncols )
{
    XVisualInfo *vi, rvi;
    int best=0, n, i;
    int scr = DefaultScreen(dpy);
    rvi.c_class = TrueColor;
    rvi.screen  = scr;
    vi = XGetVisualInfo( dpy, VisualClassMask | VisualScreenMask,
			 &rvi, &n );
    if ( vi ) {
	for ( i=0; i<n; i++ ) {
	    if ( vi[i].depth > vi[best].depth )
		best = i;
	}
    }
    Visual *v = DefaultVisual(dpy,scr);
    if ( !vi || (vi[best].visualid == XVisualIDFromVisual(v)) ||
	 (vi[best].depth <= 8 && qt_visual_option != TrueColor) )
    {
	*depth = DefaultDepth(dpy,scr);
	*ncols = DisplayCells(dpy,scr);
    } else {
	v = vi[best].visual;
	*depth = vi[best].depth;
	*ncols = vi[best].colormap_size;
    }
    if ( vi )
	XFree( (char *)vi );
    return v;
}

#endif // __ATHEOS__

/*****************************************************************************
  qt_init() - initializes Qt for X11
 *****************************************************************************/
//Ming-Che 05/10




#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

#ifdef USE_X11R6_XIM
static void xim_create_callback(XIM /*im*/,XPointer /*client_data*/,XPointer /*call_data*/)
{
    QApplication::create_xim();
}

static void xim_destroy_callback(XIM /*im*/,XPointer /*client_data*/,XPointer /*call_data*/)
{
    QApplication::close_xim();

    XRegisterIMInstantiateCallback(appDpy,0,0,0,(XIMProc)xim_create_callback,0);

}
#endif

#if defined(Q_C_CALLBACKS)
}
#endif


#ifndef __ATHEOS__
/*! \internal */
void QApplication::create_xim()
{
#if !defined(NO_XIM)
    qt_xim = XOpenIM( appDpy, 0, 0, 0 );
    if ( qt_xim ) {
#ifdef USE_X11R6_XIM
	XIMCallback destroy;
	destroy.callback = (XIMProc)xim_destroy_callback;
	destroy.client_data = 0;
	if ( XSetIMValues( qt_xim, XNDestroyCallback, &destroy, 0 ) != 0 )
	    qWarning( "Xlib dosn't support destroy callback");
#endif
	XIMStyles *styles=0;
	XGetIMValues(qt_xim, XNQueryInputStyle, &styles, 0, 0);
	if ( styles ) {
	    int i;
	    for ( i = 0; !qt_xim_style && i < styles->count_styles; i++ )
		if ( styles->supported_styles[i] == xim_preferred_style )
		    qt_xim_style = xim_preferred_style;
	    // if the preferred input style couldn't be found, look for
	    // Nothing
	    for ( i = 0; !qt_xim_style && i < styles->count_styles; i++ )
		if ( styles->supported_styles[i] == (XIMPreeditNothing |
						     XIMStatusNothing) )
		    qt_xim_style = XIMPreeditNothing | XIMStatusNothing;
	    // ... and failing that, None.
	    for ( i = 0; !qt_xim_style && i < styles->count_styles; i++ )
		if ( styles->supported_styles[i] == (XIMPreeditNone |
						     XIMStatusNone) )
		    qt_xim_style = XIMPreeditNone | XIMStatusNone;
	    XFree( styles );
	}
	if ( qt_xim_style ) {
#ifdef USE_X11R6_XIM
	    XUnregisterIMInstantiateCallback(appDpy,0,0,0,
					     (XIMProc )create_xim,0);
#endif
	    QWidgetList *list= qApp->topLevelWidgets();
	    QWidgetListIt it(*list);
	    QWidget * w;
	    while( (w=it.current()) != 0 ) {
		++it;
		w->createTLSysExtra();
	    }
	    delete list;
	} else {
	    // Give up
	    qWarning( "No supported input style found."
		      "  See InputMethod documentation.");
	    close_xim();
	}
    }
#endif
}
#endif // __ATHEOS__

void qt_init_internal( int *argcptr, char **argv, Display *display )
{
    if ( display ) {
	// Qt part of other application

	appForeignDpy = TRUE;
	appName = qstrdup( "Qt-subapplication" );
	appDpy  = display;
#ifndef __ATHEOS__
	app_Xfd = XConnectionNumber( appDpy );
#endif
	// Install default error handlers

#ifndef __ATHEOS__
	original_x_errhandler = XSetErrorHandler( qt_x_errhandler );
	original_xio_errhandler = XSetIOErrorHandler( qt_xio_errhandler );
#endif // __ATHEOS__
    } else {
	// Qt controls everything (default)

	char *p;
	int argc = *argcptr;
	int j;

	// Install default error handlers

#ifndef __ATHEOS__
	original_x_errhandler = XSetErrorHandler( qt_x_errhandler );
	original_xio_errhandler = XSetIOErrorHandler( qt_xio_errhandler );
#endif // __ATHEOS__

	// Set application name

	p = strrchr( argv[0], '/' );
	appName = p ? p + 1 : argv[0];

	// Get command line params

	j = 1;
	for ( int i=1; i<argc; i++ ) {
	    if ( argv[i] && *argv[i] != '-' ) {
		argv[j++] = argv[i];
		continue;
	    }
	    QCString arg = argv[i];
	    if ( arg == "-display" ) {
		if ( ++i < argc )
		    appDpyName = argv[i];
	    } else if ( arg == "-fn" || arg == "-font" ) {
		if ( ++i < argc )
		    appFont = argv[i];
	    } else if ( arg == "-bg" || arg == "-background" ) {
		if ( ++i < argc )
		    appBGCol = argv[i];
	    } else if ( arg == "-btn" || arg == "-button" ) {
		if ( ++i < argc )
		    appBTNCol = argv[i];
	    } else if ( arg == "-fg" || arg == "-foreground" ) {
		if ( ++i < argc )
		    appFGCol = argv[i];
	    } else if ( arg == "-name" ) {
		if ( ++i < argc )
		    appName = argv[i];
	    } else if ( arg == "-title" ) {
		if ( ++i < argc )
		    mwTitle = argv[i];
	    } else if ( arg == "-geometry" ) {
		if ( ++i < argc )
		    mwGeometry = argv[i];
		//Ming-Che 10/10
	    } else if ( arg == "-im" ) {
		if ( ++i < argc )
		    ximServer = argv[i];
	    } else if ( arg == "-noxim" ) {
#ifndef __ATHEOS__
		noxim=TRUE;
#endif // __ATHEOS__
		//
	    } else if ( arg == "-iconic" ) {
		mwIconic = !mwIconic;
	    } else if ( arg == "-ncols" ) {   // xv and netscape use this name
		if ( ++i < argc )
		    qt_ncols_option = QMAX(0,atoi(argv[i]));
	    } else if ( arg == "-visual" ) {  // xv and netscape use this name
		if ( ++i < argc ) {
		    QCString s = QCString(argv[i]).lower();
#ifndef __ATHEOS__
		    if ( s == "truecolor" ) {
			qt_visual_option = TrueColor;
		    } else {
			// ### Should we honor any others?
		    }
#endif // __ATHEOS__
		}
#if !defined(NO_XIM)
	    } else if ( arg == "-inputstyle" ) {
		if ( ++i < argc ) {
		    QCString s = QCString(argv[i]).lower();
		    if ( s == "overthespot" )
			xim_preferred_style = XIMPreeditPosition |
					      XIMStatusNothing;
		    else if ( s == "offthespot" )
			xim_preferred_style = XIMPreeditArea |
					      XIMStatusArea;
		    else if ( s == "root" )
			xim_preferred_style = XIMPreeditNothing |
					      XIMStatusNothing;
		}
#endif
	    } else if ( arg == "-cmap" ) {    // xv uses this name
		qt_cmap_option = TRUE;
	    }
#if defined(DEBUG)
	    else if ( arg == "-sync" )
		appSync = !appSync;
	    else if ( arg == "-nograb" )
		appNoGrab = !appNoGrab;
	    else if ( arg == "-dograb" )
		appDoGrab = !appDoGrab;
#endif
	    else
		argv[j++] = argv[i];
	}

	*argcptr = j;

#if defined(DEBUG) && defined(_OS_LINUX_)
	if ( !appNoGrab && !appDoGrab ) {
	    QCString s;
	    s.sprintf( "/proc/%d/cmdline", getppid() );
	    QFile f( s );
	    if ( f.open( IO_ReadOnly ) ) {
		s.truncate( 0 );
		int c;
		while ( (c = f.getch()) > 0 ) {
		    if ( c == '/' )
			s.truncate( 0 );
		    else
			s += (char)c;
		}
		if ( s == "gdb" ) {
		    appNoGrab = TRUE;
		    qDebug( "Qt: gdb: -nograb added to command-line options.\n"
			    "\t Use the -dograb option to enforce grabbing." );
		}
		f.close();
	    }
	}
#endif
	// Connect to X server

#ifndef __ATHEOS__
	if( qt_is_gui_used ) {
	    if ( ( appDpy = XOpenDisplay(appDpyName) ) == 0 ) {
		qWarning( "%s: cannot connect to X server %s", appName,
			  XDisplayName(appDpyName) );
		exit( 1 );
	    }
	    app_Xfd = XConnectionNumber( appDpy );	// set X network socket

	    if ( appSync )				// if "-sync" argument
		XSynchronize( appDpy, TRUE );
	}
#endif // __ATHEOS__
    }
    // Common code, regardless of whether display is foreign.

    // Get X parameters

#ifndef __ATHEOS__
    if( qt_is_gui_used ) {
	appScreen  = DefaultScreen(appDpy);
	appRootWin = RootWindow(appDpy,appScreen);

	// Set X paintdevice parameters

	Visual *vis = DefaultVisual(appDpy,appScreen);
	QPaintDevice::x_appdisplay     = appDpy;
	QPaintDevice::x_appscreen      = appScreen;
	QPaintDevice::x_appdepth       = DefaultDepth(appDpy,appScreen);
	QPaintDevice::x_appcells       = DisplayCells(appDpy,appScreen);
	QPaintDevice::x_appvisual      = vis;
	QPaintDevice::x_appdefvisual   = TRUE;

	if ( qt_visual_option == TrueColor ||	// find custom visual
	     QApplication::colorSpec() == QApplication::ManyColor ) {
	    vis = find_truecolor_visual( appDpy, &QPaintDevice::x_appdepth,
					 &QPaintDevice::x_appcells );
	    QPaintDevice::x_appdefvisual =
		(XVisualIDFromVisual(vis) ==
		 XVisualIDFromVisual(DefaultVisual(appDpy,appScreen)));
	    QPaintDevice::x_appvisual = vis;
	}

	if ( vis->c_class == TrueColor ) {
	    QPaintDevice::x_appdefcolormap = QPaintDevice::x_appdefvisual;
	} else {
	    QPaintDevice::x_appdefcolormap = !qt_cmap_option;
	}
	if ( QPaintDevice::x_appdefcolormap ) {
	    QPaintDevice::x_appcolormap = DefaultColormap(appDpy,appScreen);
	} else {
	    QPaintDevice::x_appcolormap = XCreateColormap(appDpy, appRootWin,
							  vis, AllocNone);
	}

	// Support protocols

	qt_x11_intern_atom( "WM_PROTOCOLS", &qt_wm_protocols );
	qt_x11_intern_atom( "WM_DELETE_WINDOW", &qt_wm_delete_window );
	qt_x11_intern_atom( "_XSETROOT_ID", &qt_xsetroot_id );
	qt_x11_intern_atom( "_QT_SCROLL_DONE", &qt_qt_scrolldone );
	qt_x11_intern_atom( "_QT_SELECTION", &qt_selection_property );
	qt_x11_intern_atom( "_QT_SELECTION_SENTINEL", &qt_selection_sentinel );
	qt_x11_intern_atom( "WM_STATE", &qt_wm_state );
	qt_x11_intern_atom( "WM_TAKE_FOCUS", &qt_wm_take_focus );
	qt_x11_intern_atom( "_NET_WM_CONTEXT_HELP", &qt_net_wm_context_help );
	qt_x11_intern_atom( "RESOURCE_MANAGER", &qt_resource_manager );
	qt_x11_intern_atom( "_QT_DESKTOP_PROPERTIES", &qt_desktop_properties );
	qt_x11_intern_atom( "_QT_INPUT_ENCODING", &qt_input_encoding );
	qt_x11_intern_atom( "_QT_SIZEGRIP", &qt_sizegrip );
	qt_x11_intern_atom( "WM_CLIENT_LEADER", &qt_wm_client_leader);
	qt_x11_intern_atom( "WINDOW_ROLE", &qt_window_role);
	qt_x11_intern_atom( "SM_CLIENT_ID", &qt_sm_client_id);
	qt_x11_intern_atom( "_MOTIF_WM_HINTS", &qt_xa_motif_wm_hints );
	qt_x11_intern_atom( "KWIN_RUNNING", &qt_kwin_running );
	qt_x11_intern_atom( "KWM_RUNNING", &qt_kwm_running );
	qt_x11_intern_atom( "GNOME_BACKGROUND_PROPERTIES", &qt_gbackground_properties );
	qt_x11_intern_atom( "INCR", &qt_x_incr );

	qt_xdnd_setup();
	qt_x11_motifdnd_init();

	// Finally create all atoms
	qt_x11_process_intern_atoms();

	// Misc. initialization

	QColor::initialize();
	QFont::initialize();
	QCursor::initialize();
	QPainter::initialize();
    }
#endif // __ATHEOS__
    gettimeofday( &watchtime, 0 );

#ifndef __ATHEOS__
    if( qt_is_gui_used ) {
	qApp->setName( appName );

	XSelectInput( appDpy, appRootWin,
		      KeyPressMask | KeyReleaseMask |
		      KeymapStateMask |
		      EnterWindowMask | LeaveWindowMask |
		      FocusChangeMask | PropertyChangeMask
		      );
    }
#endif // __ATHEOS__
    // XIM segfaults on Solaris with "C" locale!
    // The idea was that the "en_US" locale is maybe installed on all systems
    // with a "C" locale and could be used as a fallback instead of "C". This
    // is not the case.
    // We'll have to take a XIM / no XIM decision at run-time.
    // Taking a decision at compile-time using NO_XIM is not enough.
#if defined (_OS_SOLARIS_) && !defined(NO_XIM)
    const char* locale = ::setlocale( LC_ALL, "" );
    if ( !locale || qstrcmp( locale, "C" ) == 0 ) {
	locale = ::setlocale( LC_ALL, "en_US" );
	ASSERT( qstrcmp( locale, "en_US" ) == 0 );
    }
#else
    setlocale( LC_ALL, "" );		// use correct char set mapping
#endif
    setlocale( LC_NUMERIC, "C" );	// make sprintf()/scanf() work


#ifndef __ATHEOS__
    if ( qt_is_gui_used ) {
#if !defined(NO_XIM)
	qt_xim = 0;
	QString ximServerName(ximServer);
	if (ximServer)
	    ximServerName.prepend("@im=");
	else
	    ximServerName = "";

	if ( !XSupportsLocale() )
	    qWarning("Qt: Locales not supported on X server");
#ifdef USE_X11R6_XIM
	else if ( XSetLocaleModifiers (ximServerName.ascii()) == 0 )
	    qWarning( "Qt: Cannot set locale modifiers: %s",
		      ximServerName.ascii());
	else if ( !noxim )
	    XRegisterIMInstantiateCallback( appDpy, 0 , 0, 0,
					    (XIMProc)QApplication::create_xim,
					    0);
#else
	else if ( XSetLocaleModifiers ("") == 0 )
	    qWarning("Qt: Cannot set locale modifiers");
	else if ( !noxim )
	    QApplication::create_xim();
#endif
#endif

	qt_set_input_encoding();

	// pick default character set (now that we have done setlocale stuff)
	QFont::locale_init();
	QFont f;
	f = QFont( "Helvetica", (QPaintDevice::x11AppDpiX() < 95) ? 12 : 11 );
	f.setCharSet( QFont::charSetForLocale() ); // must come after locale_init()
	QApplication::setFont( f );

	qt_set_x11_resources( appFont, appFGCol, appBGCol, appBTNCol);
    }
#endif // __ATHEOS__

#if defined(_OS_UNIX_)
    pipe( qt_thread_pipe );
#endif

}


#ifndef QT_NO_STYLE
 // run-time search for default style
void QApplication::x11_initialize_style()
{
    Atom type;
    int format;
    unsigned long length, after;
    unsigned char *data;
    if ( app_style )
	return;
    if ( !seems_like_KDE_is_running ) {
	if ( XGetWindowProperty( appDpy, appRootWin, qt_kwin_running, 0, 1,
				 FALSE, AnyPropertyType, &type, &format,
				 &length, &after, &data ) == Success
	     && length ) {
	    seems_like_KDE_is_running = TRUE;
	    if ( data )
		XFree( data );
	}
    }
    if ( !seems_like_KDE_is_running ) {
	if ( XGetWindowProperty( appDpy, appRootWin, qt_kwm_running, 0, 1,
				 FALSE, AnyPropertyType, &type, &format,
				 &length, &after, &data ) == Success
	     && length ) {
	    seems_like_KDE_is_running = TRUE; // KDE1, to be precise
	    if ( data )
		XFree( data );
	}
    }
    if ( seems_like_KDE_is_running ) {
#if !defined(QT_NO_STYLE_WINDOWS)
	app_style = new QWindowsStyle; // default to windowsstyle on KDE
#endif
    } else { // maybe another desktop?
	if ( XGetWindowProperty( appDpy, appRootWin, qt_gbackground_properties, 0, 1,
				 FALSE, AnyPropertyType, &type, &format,
				 &length, &after, &data ) == Success
	     && length ) {
#ifndef QT_NO_STYLE_CDE
	    app_style = new QMotifPlusStyle( TRUE ); // default to MotifPlus with hovering
#endif
	    if ( data )
		XFree( data );
	}
    }
}
#endif

void qt_init( int *argcptr, char **argv, QApplication::Type )
{
    qt_init_internal( argcptr, argv, 0 );
}

void qt_init( Display *display )
{
    qt_init_internal( 0, 0, display );
}


/*****************************************************************************
  qt_cleanup() - cleans up when the application is finished
 *****************************************************************************/

void qt_cleanup()
{
    if ( postRList ) {
	QVFuncList::Iterator it = postRList->begin();
	while ( it != postRList->end() ) {	// call post routines
	    (**it)();
	    postRList->remove( it );
	    it = postRList->begin();
	}
	delete postRList;
	postRList = 0;
    }

#ifndef __ATHEOS__
    if ( app_save_rootinfo )			// root window must keep state
	qt_save_rootinfo();
#endif // __ATHEOS__
    cleanupTimers();
#ifndef __ATHEOS__
    QPixmapCache::clear();
    QPainter::cleanup();
    QFont::cleanup();
    QColor::cleanup();
    
#if !defined(NO_XIM)
    if ( qt_xim )
	QApplication::close_xim();
#endif

    if ( qt_is_gui_used && !QPaintDevice::x11AppDefaultColormap() )
	XFreeColormap( QPaintDevice::x11AppDisplay(),
		       QPaintDevice::x11AppColormap() );

#define CLEANUP_GC(g) if (g) { XFreeGC(appDpy,g); g = 0; }
    CLEANUP_GC(app_gc_ro);
    CLEANUP_GC(app_gc_ro_m);
    CLEANUP_GC(app_gc_tmp);
    CLEANUP_GC(app_gc_tmp_m);
#undef CLEANUP_GC

    delete sip_list;
    sip_list = 0;

    // Reset the error handlers
    XSetErrorHandler( original_x_errhandler );
    XSetIOErrorHandler( original_xio_errhandler );

    if ( qt_is_gui_used && !appForeignDpy )
	XCloseDisplay( appDpy );		// close X display
    appDpy = 0;

    if ( appForeignDpy ) {
	delete [] appName;
	appName = 0;
    }

    delete activeBeforePopup;
    activeBeforePopup = 0;
#endif // __ATHEOS__

#ifdef _OS_UNIX_
    close(qt_thread_pipe[0]);
    close(qt_thread_pipe[1]);
#endif // _OS_UNIX_
}


/*****************************************************************************
  Platform specific global and internal functions
 *****************************************************************************/

#ifndef __ATHEOS__
void qt_save_rootinfo()				// save new root info
{
    Atom type;
    int format;
    unsigned long length, after;
    unsigned char *data;

    if ( qt_xsetroot_id ) {			// kill old pixmap
	if ( XGetWindowProperty( appDpy, appRootWin, qt_xsetroot_id, 0, 1,
				 TRUE, AnyPropertyType, &type, &format,
				 &length, &after, &data ) == Success ) {
	    if ( type == XA_PIXMAP && format == 32 && length == 1 &&
		 after == 0 && data ) {
		XKillClient( appDpy, *((Pixmap*)data) );
		XFree( (char *)data );
	    }
	    Pixmap dummy = XCreatePixmap( appDpy, appRootWin, 1, 1, 1 );
	    XChangeProperty( appDpy, appRootWin, qt_xsetroot_id, XA_PIXMAP, 32,
			     PropModeReplace, (uchar *)&dummy, 1 );
	    XSetCloseDownMode( appDpy, RetainPermanent );
	}
    }
}

void qt_updated_rootinfo()
{
    app_save_rootinfo = TRUE;
}

bool qt_wstate_iconified( WId winid )
{
    Atom type;
    int format;
    unsigned long length, after;
    unsigned char *data;
    int r = XGetWindowProperty( appDpy, winid, qt_wm_state, 0, 2,
				 FALSE, AnyPropertyType, &type, &format,
				 &length, &after, &data );
    bool iconic = FALSE;
    if ( r == Success && data && format == 32 ) {
	// Q_UINT32 *wstate = (Q_UINT32*)data;
	unsigned long *wstate = (unsigned long *) data;
	iconic = (*wstate == IconicState );
	XFree( (char *)data );
    }
    return iconic;
}
#endif // __ATHEOS__

/*!
  \relates QApplication

  Adds a global routine that will be called from the QApplication
  destructor.  This function is normally used to add cleanup routines
  for program-wide functionality.

  The function given by \a p should take no arguments and return
  nothing, like this:
  \code
    static int *global_ptr = 0;

    static void cleanup_ptr()
    {
	delete [] global_ptr;
	global_ptr = 0;
    }

    void init_ptr()
    {
	global_ptr = new int[100];	// allocate data
	qAddPostRoutine( cleanup_ptr );	// delete later
    }
  \endcode

  Note that for an application- or module-wide cleanup,
  qAddPostRoutine() is often not suitable.  People have a tendency to
  make such modules dynamically loaded, and then unload those modules
  long before the QApplication destructor is called, for example.

  For modules and libraries, using a reference-counted initialization
  manager or Qt' parent-child delete mechanism may be better.  Here is
  an example of a private class which uses the parent-child mechanism
  to call a cleanup function at the right time:

  \code
    class MyPrivateInitStuff: public QObject {
    private:
        MyPrivateInitStuff( QObject * parent ): QObject( parent) {
	    // initialization goes here
	}
	MyPrivateInitStuff * p;

    public:
        static MyPrivateInitStuff * initStuff( QObject * parent ) {
	    if ( !p )
	        p = new MyPrivateInitStuff( parent );
	    return p;
	}

        ~MyPrivateInitStuff() {
	    // cleanup (the "post routine") goes here
	}
    }
  \endcode

  By selecting the right parent widget/object, this can often be made
  to clean up the module's data at the exact right moment.
*/

void qAddPostRoutine( Q_CleanUpFunction p )
{
    if ( !postRList ) {
	postRList = new QVFuncList;
	CHECK_PTR( postRList );
    }
    postRList->prepend( p );
}


void qRemovePostRoutine( Q_CleanUpFunction p )
{
    if ( !postRList ) return;

    QVFuncList::Iterator it = postRList->begin();

    while ( it != postRList->end() ) {
	if ( *it == p ) {
	    postRList->remove( it );
	    it = postRList->begin();
	}
    }
}


// HACK HACK
/*
void clean_post_routines(void *obj, unsigned long len, const char *name) {
    QListIterator<void> post_it(*postRList);
    void *post;

    unsigned long objl = (unsigned long) obj, ptrl;

    while ((post = post_it.current()) != 0) {
    	++post_it;

 	ptrl = (unsigned long) post;

	if (ptrl >= objl && ptrl <= (objl + len)) {
	    qDebug("clean_post_routines: cleaning post routine for '%s'", name);

	    postRList->remove(post);

	    //	    break;
	}
    }
}
*/
// </HACK>


char *qAppName()				// get application name
{
    return appName;
}

Display *qt_xdisplay()				// get current X display
{
    return appDpy;
}

int qt_xscreen()				// get current X screen
{
    return appScreen;
}

#ifndef __ATHEOS__
WId qt_xrootwin()				// get X root window
{
    return appRootWin;
}
#endif __ATHEOS__

bool qt_nograb()				// application no-grab option
{
#if defined(DEBUG)
    return appNoGrab;
#else
    return FALSE;
#endif
}

#ifndef __ATHEOS__
static GC create_gc( bool monochrome )
{
    GC gc;
    if ( monochrome ) {
	Pixmap pm = XCreatePixmap( appDpy, appRootWin, 8, 8, 1 );
	gc = XCreateGC( appDpy, pm, 0, 0 );
	XFreePixmap( appDpy, pm );
    } else {
	if ( QPaintDevice::x11AppDefaultVisual() ) {
	    gc = XCreateGC( appDpy, appRootWin, 0, 0 );
	} else {
	    Window w;
	    XSetWindowAttributes a;
	    a.background_pixel = Qt::black.pixel();
	    a.border_pixel = Qt::black.pixel();
	    a.colormap = QPaintDevice::x11AppColormap();
	    w = XCreateWindow( appDpy, appRootWin, 0, 0, 100, 100,
			       0, QPaintDevice::x11AppDepth(), InputOutput,
			       (Visual*)QPaintDevice::x11AppVisual(),
			       CWBackPixel|CWBorderPixel|CWColormap, &a );
	    gc = XCreateGC( appDpy, w, 0, 0 );
	    XDestroyWindow( appDpy, w );
	}
    }
    XSetGraphicsExposures( appDpy, gc, FALSE );
    return gc;
}

GC qt_xget_readonly_gc( bool monochrome )	// get read-only GC
{
    GC gc;
    if ( monochrome ) {
	if ( !app_gc_ro_m )			// create GC for bitmap
	    app_gc_ro_m = create_gc(TRUE);
	gc = app_gc_ro_m;
    } else {					// create standard GC
	if ( !app_gc_ro )			// create GC for bitmap
	    app_gc_ro = create_gc(FALSE);
	gc = app_gc_ro;
    }
    return gc;
}

GC qt_xget_temp_gc( bool monochrome )		// get temporary GC
{
    GC gc;
    if ( monochrome ) {
	if ( !app_gc_tmp_m )			// create GC for bitmap
	    app_gc_tmp_m = create_gc(TRUE);
	gc = app_gc_tmp_m;
    } else {					// create standard GC
	if ( !app_gc_tmp )			// create GC for bitmap
	    app_gc_tmp = create_gc(FALSE);
	gc = app_gc_tmp;
    }
    return gc;
}


/*****************************************************************************
  Platform specific QApplication members
 *****************************************************************************/

/*!
  \fn QWidget *QApplication::mainWidget() const

  Returns the main application widget, or a null pointer if there is
  not a defined main widget.

  \sa setMainWidget()
*/

/*!
  Sets the main widget of the application.

  The main widget is like any other, in most respects except that if
  it is deleted, the application exits.

  You need not have a main widget; connecting lastWindowClosed() to quit() is
  another alternative.

  For X11, this function also resizes and moves the main widget
  according to the \e -geometry command-line option, so you should set
  the default geometry (using \l QWidget::setGeometry()) before
  calling setMainWidget().

  \sa mainWidget(), exec(), quit()
*/

void QApplication::setMainWidget( QWidget *mainWidget )
{
#if QT_VERSION >= 300
    ASSERT(!mainWidget->parentWidget()); // catch silly error
#endif
    extern int qwidget_tlw_gravity;		// in qwidget_x11.cpp
    main_widget = mainWidget;
    if ( main_widget ) {			// give WM command line
	XSetWMProperties( main_widget->x11Display(), main_widget->winId(),
			  0, 0, app_argv, app_argc, 0, 0, 0 );
	if ( mwTitle )
	    XStoreName( main_widget->x11Display(), main_widget->winId(), mwTitle );
	if ( mwGeometry ) {			// parse geometry
	    int x, y;
	    int w, h;
	    int m = XParseGeometry( mwGeometry, &x, &y, (uint*)&w, (uint*)&h );
	    QSize minSize = main_widget->minimumSize();
	    QSize maxSize = main_widget->maximumSize();
	    if ( (m & XValue) == 0 )
		x = main_widget->geometry().x();
	    if ( (m & YValue) == 0 )
		y = main_widget->geometry().y();
	    if ( (m & WidthValue) == 0 )
		w = main_widget->width();
	    if ( (m & HeightValue) == 0 )
		h = main_widget->height();
	    w = QMIN(w,maxSize.width());
	    h = QMIN(h,maxSize.height());
	    w = QMAX(w,minSize.width());
	    h = QMAX(h,minSize.height());
	    if ( (m & XNegative) ) {
		x = desktop()->width()  + x - w;
		qwidget_tlw_gravity = 3;
	    }
	    if ( (m & YNegative) ) {
		y = desktop()->height() + y - h;
		qwidget_tlw_gravity = (m & XNegative) ? 9 : 7;
	    }
	    main_widget->setGeometry( x, y, w, h );
	}
    }
}
#endif // __ATHEOS__

#ifndef QT_NO_CURSOR

/*****************************************************************************
  QApplication cursor stack
 *****************************************************************************/

typedef QList<QCursor> QCursorList;

static QCursorList *cursorStack = 0;

/*!
  \fn QCursor *QApplication::overrideCursor()

  Returns the active application override cursor.

  This function returns 0 if no application cursor has been defined
  (i.e. the internal cursor stack is empty).

  \sa setOverrideCursor(), restoreOverrideCursor()
*/

/*!
  Sets the application override cursor to \a cursor.

  Application override cursors are intended for showing the user that
  the application is in a special state, for example during an
  operation that might take some time.

  This cursor will be displayed in all the widgets of the application
  until restoreOverrideCursor() or another setOverrideCursor() is
  called.

  Application cursors are stored on an internal stack.
  setOverrideCursor() pushes the cursor onto the stack, and
  restoreOverrideCursor() pops the active cursor off the stack. Every
  setOverrideCursor() must eventually be followed by a corresponding
  restoreOverrideCursor(), otherwise the stack will never be emptied.

  If \a replace is TRUE, the new cursor will replace the last override
  cursor (the stack keeps its depth). If \a replace is FALSE, the new
  stack is pushed onto the top of the stack.

  Example:
  \code
    QApplication::setOverrideCursor( Qt::waitCursor );
    calculateHugeMandelbrot();			// lunch time...
    QApplication::restoreOverrideCursor();
  \endcode

  \sa overrideCursor(), restoreOverrideCursor(), QWidget::setCursor()
*/

void QApplication::setOverrideCursor( const QCursor &cursor, bool replace )
{
    if ( !cursorStack ) {
	cursorStack = new QCursorList;
	CHECK_PTR( cursorStack );
	cursorStack->setAutoDelete( TRUE );
    }
    app_cursor = new QCursor( cursor );
    CHECK_PTR( app_cursor );
    if ( replace )
	cursorStack->removeLast();
    cursorStack->append( app_cursor );

    QWidget* amw = activeModalWidget();
    QWidgetIntDictIt it( *((QWidgetIntDict*)QWidget::mapper) );
    register QWidget *w;
    while ( (w=it.current()) ) {		// for all widgets that have
	if ( w->testWState(WState_OwnCursor) &&
	     ( !amw || !w->isVisible() || w->topLevelWidget() == amw ) )	//   set a cursor
	    XDefineCursor( w->x11Display(), w->winId(), app_cursor->handle() );
	++it;
    }
    XFlush( appDpy );				// make X execute it NOW
}

/*!
  Undoes the last setOverrideCursor().

  If setOverrideCursor() has been called twice, calling
  restoreOverrideCursor() will activate the first cursor set. Calling
  this function a second time restores the original widgets cursors.

  \sa setOverrideCursor(), overrideCursor().
*/

void QApplication::restoreOverrideCursor()
{
    if ( !cursorStack )				// no cursor stack
	return;
    cursorStack->removeLast();
    app_cursor = cursorStack->last();
    if ( QWidget::mapper != 0 && !closingDown() ) {
	QWidgetIntDictIt it( *((QWidgetIntDict*)QWidget::mapper) );
	register QWidget *w;
	while ( (w=it.current()) ) {		// set back to original cursors
	    if ( w->testWState(WState_OwnCursor) )
		XDefineCursor( w->x11Display(), w->winId(),
			       app_cursor ? app_cursor->handle()
			       : w->cursor().handle() );
	    ++it;
	}
	XFlush( appDpy );
    }
    if ( !app_cursor ) {
	delete cursorStack;
	cursorStack = 0;
    }
}

#endif

/*!
  \fn bool QApplication::hasGlobalMouseTracking()

  Returns TRUE if global mouse tracking is enabled, otherwise FALSE.

  \sa setGlobalMouseTracking()
*/

/*!
  Enables global mouse tracking if \a enable is TRUE or disables it
  if \a enable is FALSE.

  Enabling global mouse tracking makes it possible for widget event
  filters or application event filters to get all mouse move events, even
  when no button is depressed.  This is useful for special GUI elements,
  e.g. tool tips.

  Global mouse tracking does not affect widgets and their
  mouseMoveEvent().  For a widget to get mouse move events when no button
  is depressed, it must do QWidget::setMouseTracking(TRUE).

  This function uses an internal counter.  Each
  setGlobalMouseTracking(TRUE) must have a corresponding
  setGlobalMouseTracking(FALSE):
  \code
    // at this point global mouse tracking is off
    QApplication::setGlobalMouseTracking( TRUE );
    QApplication::setGlobalMouseTracking( TRUE );
    QApplication::setGlobalMouseTracking( FALSE );
    // at this point it's still on
    QApplication::setGlobalMouseTracking( FALSE );
    // but now it's off
  \endcode

  \sa hasGlobalMouseTracking(), QWidget::hasMouseTracking()
*/

#ifndef __ATHEOS__
void QApplication::setGlobalMouseTracking( bool enable )
{
    bool tellAllWidgets;
    if ( enable ) {
	tellAllWidgets = (++app_tracking == 1);
    } else {
	tellAllWidgets = (--app_tracking == 0);
    }
    if ( tellAllWidgets ) {
	QWidgetIntDictIt it( *((QWidgetIntDict*)QWidget::mapper) );
	register QWidget *w;
	while ( (w=it.current()) ) {
	    if ( app_tracking > 0 ) {		// switch on
		if ( !w->testWState(WState_MouseTracking) ) {
		    w->setMouseTracking( TRUE );
		    w->clearWState(WState_MouseTracking);
		}
	    } else {				// switch off
		if ( !w->testWState(WState_MouseTracking) ) {
		    w->setWState(WState_MouseTracking);
		    w->setMouseTracking( FALSE );
		}
	    }
	    ++it;
	}
    }
}


/*****************************************************************************
  Routines to find a Qt widget from a screen position
 *****************************************************************************/

static QWidget *findChildWidget( const QWidget *p, const QPoint &pos )
{
    if ( p->children() ) {
	QWidget *w;
	QObjectListIt it( *p->children() );
	it.toLast();
	while ( it.current() ) {
	    if ( it.current()->isWidgetType() ) {
		w = (QWidget*)it.current();
		if ( w->isVisible() && w->geometry().contains(pos) ) {
		    QWidget *c = findChildWidget( w, w->mapFromParent(pos) );
		    return c ? c : w;
		}
	    }
	    --it;
	}
    }
    return 0;
}

Window qt_x11_findClientWindow( Window win, Atom property, bool leaf )
{
    Atom   type = None;
    int	   format, i;
    ulong  nitems, after;
    uchar *data;
    Window root, parent, target=0, *children=0;
    uint   nchildren;
    if ( XGetWindowProperty( appDpy, win, property, 0, 0, FALSE, AnyPropertyType,
			     &type, &format, &nitems, &after, &data ) == Success ) {
	if ( data )
	    XFree( (char *)data );
	if ( type )
	    return win;
    }
    if ( !XQueryTree(appDpy,win,&root,&parent,&children,&nchildren) ) {
	if ( children )
	    XFree( (char *)children );
	return 0;
    }
    for ( i=nchildren-1; !target && i >= 0; i-- )
	target = qt_x11_findClientWindow( children[i], property, leaf );
    if ( children )
	XFree( (char *)children );
    return target;
}


/*!
  Returns a pointer to the widget at global screen position \a (x,y), or a
  null pointer if there is no Qt widget there.

  If \a child is FALSE and there is a child widget at position \a
  (x,y), the top-level widget containing it is returned. If \a child
  is TRUE the child widget at position \a (x,y) is returned.

  This function is normally rather slow.

  \sa QCursor::pos(), QWidget::grabMouse(), QWidget::grabKeyboard()
*/

QWidget *QApplication::widgetAt( int x, int y, bool child )
{
    int lx, ly;

    Window target;
    if ( !XTranslateCoordinates(appDpy, appRootWin, appRootWin,
				x, y, &lx, &ly, &target) )
	return 0;
    if ( !target || target == appRootWin )
	return 0;
    QWidget *w, *c;
    w = QWidget::find( (WId)target );

    if ( !w ) {
	qt_ignore_badwindow();
	target = qt_x11_findClientWindow( target, qt_wm_state, TRUE );
	if (qt_badwindow() )
	    return 0;
	w = QWidget::find( (WId)target );
	if ( FALSE && !w ) {
	    // Perhaps the widgets at (x,y) is inside a foreign application?
	    // Search all toplevel widgets to see if one is within target
	    QWidgetList *list   = topLevelWidgets();
	    QWidget     *widget = list->first();
	    while ( widget && !w ) {
		Window	ctarget = target;
		if ( widget->isVisible() && !widget->isDesktop() ) {
		    Window wid = widget->winId();
		    while ( ctarget && !w ) {
			XTranslateCoordinates(appDpy, appRootWin, ctarget,
					      x, y, &lx, &ly, &ctarget);
			if ( ctarget == wid ) {
			    // Found
			    w = widget;
			    XTranslateCoordinates(appDpy, appRootWin, ctarget,
						  x, y, &lx, &ly, &ctarget);
			}
		    }
		}
		widget = list->next();
	    }
	    delete list;
	}
    }
    if ( child && w ) {
	if ( (c = findChildWidget( w, w->mapFromGlobal(QPoint(x, y ) ) ) ) )
	    return c;
    }
    return w;
}

/*!
  \overload QWidget *QApplication::widgetAt( const QPoint &pos, bool child )
*/


/*!
  Flushes the X event queue in the X11 implementation. This normally
  returns almost immediately. Does nothing on other platforms.

  \sa syncX()
*/

void QApplication::flushX()
{
    if ( appDpy )
	XFlush( appDpy );
}

/*!
  Synchronizes with the X server in the X11 implementation. This
  normally takes some time. Does nothing on other platforms.

  \sa flushX()
*/

void QApplication::syncX()
{
    if ( appDpy )
	XSync( appDpy, FALSE );			// don't discard events
}


/*!
  Sounds the bell, using the default volume and sound.
*/

void QApplication::beep()
{
    if ( appDpy )
	XBell( appDpy, 0 );
}



/*****************************************************************************
  Special lookup functions for windows that have been reparented recently
 *****************************************************************************/

static QWidgetIntDict *wPRmapper = 0;		// alternative widget mapper

void qPRCreate( const QWidget *widget, Window oldwin )
{						// QWidget::reparent mechanism
    if ( !wPRmapper ) {
	wPRmapper = new QWidgetIntDict;
	CHECK_PTR( wPRmapper );
    }
    wPRmapper->insert( (long)oldwin, widget );	// add old window to mapper
    QETWidget *w = (QETWidget *)widget;
    w->setWState( Qt::WState_Reparented );	// set reparented flag
}

void qPRCleanup( QWidget *widget )
{
    QETWidget *etw = (QETWidget *)widget;
    if ( !(wPRmapper && etw->testWState(Qt::WState_Reparented)) )
	return;					// not a reparented widget
    QWidgetIntDictIt it(*wPRmapper);
    QWidget *w;
    while ( (w=it.current()) ) {
	int key = it.currentKey();
	++it;
	if ( w == etw ) {                       // found widget
	    etw->clearWState( Qt::WState_Reparented ); // clear flag
	    wPRmapper->remove( key );// old window no longer needed
 	    if ( wPRmapper->count() == 0 ) {	// became empty
		delete wPRmapper;		// then reset alt mapper
		wPRmapper = 0;
		return;
	    }
	}
    }
}

QETWidget *qPRFindWidget( Window oldwin )
{
    return wPRmapper ? (QETWidget*)wPRmapper->find((long)oldwin) : 0;
}
#endif __ATHEOS__


/*****************************************************************************
  Socket notifier (type: 0=read, 1=write, 2=exception)

  The QSocketNotifier class (qsocketnotifier.h) provides installable callbacks
  for select() through the internal function qt_set_socket_handler().
 *****************************************************************************/

struct QSockNot {
    QObject *obj;
    int	     fd;
    fd_set  *queue;
};

typedef QList<QSockNot> QSNList;
typedef QListIterator<QSockNot> QSNListIt;

static int	sn_highest = -1;
static QSNList *sn_read	   = 0;
static QSNList *sn_write   = 0;
static QSNList *sn_except  = 0;

static fd_set	sn_readfds;			// fd set for reading
static fd_set	sn_writefds;			// fd set for writing
static fd_set	sn_exceptfds;			// fd set for exceptions
static fd_set	sn_queued_read;
static fd_set	sn_queued_write;
static fd_set	sn_queued_except;

#ifndef __ATHEOS__
static struct SN_Type {
    QSNList **list;
    fd_set   *fdspec;
    fd_set   *fdres;
    fd_set   *queue;
} sn_vec[3] = {
    { &sn_read,	  &sn_readfds,	 &app_readfds,   &sn_queued_read },
    { &sn_write,  &sn_writefds,	 &app_writefds,  &sn_queued_write },
    { &sn_except, &sn_exceptfds, &app_exceptfds, &sn_queued_except } };


static QSNList *sn_act_list = 0;


static void sn_cleanup()
{
    delete sn_act_list;
    sn_act_list = 0;
    for ( int i=0; i<3; i++ ) {
	delete *sn_vec[i].list;
	*sn_vec[i].list = 0;
    }
}


static void sn_init()
{
    if ( !sn_act_list ) {
	sn_act_list = new QSNList;
	CHECK_PTR( sn_act_list );
	qAddPostRoutine( sn_cleanup );
    }
}


bool qt_set_socket_handler( int sockfd, int type, QObject *obj, bool enable )
{
    if ( sockfd < 0 || type < 0 || type > 2 || obj == 0 ) {
#if defined(CHECK_RANGE)
	qWarning( "QSocketNotifier: Internal error" );
#endif
	return FALSE;
    }

    QSNList  *list = *sn_vec[type].list;
    fd_set   *fds  =  sn_vec[type].fdspec;
    QSockNot *sn;

    if ( enable ) {				// enable notifier
	if ( !list ) {
	    sn_init();
	    list = new QSNList;			// create new list
	    CHECK_PTR( list );
	    list->setAutoDelete( TRUE );
	    *sn_vec[type].list = list;
	    FD_ZERO( fds );
	    FD_ZERO( sn_vec[type].queue );
	}
	sn = new QSockNot;
	CHECK_PTR( sn );
	sn->obj = obj;
	sn->fd	= sockfd;
	sn->queue = sn_vec[type].queue;
	if ( list->isEmpty() ) {
	    list->insert( 0, sn );
	} else {				// sort list by fd, decreasing
	    QSockNot *p = list->first();
	    while ( p && p->fd > sockfd )
		p = list->next();
#if defined(CHECK_STATE)
	    if ( p && p->fd == sockfd ) {
		static const char *t[] = { "read", "write", "exception" };
		qWarning( "QSocketNotifier: Multiple socket notifiers for "
			 "same socket %d and type %s", sockfd, t[type] );
	    }
#endif
	    if ( p )
		list->insert( list->at(), sn );
	    else
		list->append( sn );
	}
	FD_SET( sockfd, fds );
	sn_highest = QMAX(sn_highest,sockfd);

    } else {					// disable notifier

	if ( list == 0 )
	    return FALSE;			// no such fd set
	QSockNot *sn = list->first();
	while ( sn && !(sn->obj == obj && sn->fd == sockfd) )
	    sn = list->next();
	if ( !sn )				// not found
	    return FALSE;
	FD_CLR( sockfd, fds );			// clear fd bit
	FD_CLR( sockfd, sn->queue );
	if ( sn_act_list )
	    sn_act_list->removeRef( sn );	// remove from activation list
	list->remove();				// remove notifier found above
	if ( sn_highest == sockfd ) {		// find highest fd
	    sn_highest = -1;
	    for ( int i=0; i<3; i++ ) {
		if ( *sn_vec[i].list && (*sn_vec[i].list)->count() )
		    sn_highest = QMAX(sn_highest,  // list is fd-sorted
				      (*sn_vec[i].list)->getFirst()->fd);
	    }
	}
    }

    return TRUE;
}


//
// We choose a random activation order to be more fair under high load.
// If a constant order is used and a peer early in the list can
// saturate the IO, it might grab our attention completely.
// Also, if we're using a straight list, the callback routines may
// delete other entries from the list before those other entries are
// processed.
//

static int sn_activate()
{
    if ( !sn_act_list )
	sn_init();
    int i, n_act = 0;
    for ( i=0; i<3; i++ ) {			// for each list...
	if ( *sn_vec[i].list ) {		// any entries?
	    QSNList  *list = *sn_vec[i].list;
	    fd_set   *fds  = sn_vec[i].fdres;
	    QSockNot *sn   = list->first();
	    while ( sn ) {
		if ( FD_ISSET( sn->fd, fds ) &&	// store away for activation
		     !FD_ISSET( sn->fd, sn->queue ) ) {
		    sn_act_list->insert( (rand() & 0xff) %
					 (sn_act_list->count()+1),
					 sn );
		    FD_SET( sn->fd, sn->queue );
		}
		sn = list->next();
	    }
	}
    }
    if ( sn_act_list->count() > 0 ) {		// activate entries
	QEvent event( QEvent::SockAct );
	QSNListIt it( *sn_act_list );
	QSockNot *sn;
	while ( (sn=it.current()) ) {
	    ++it;
	    sn_act_list->removeRef( sn );
	    if ( FD_ISSET(sn->fd, sn->queue) ) {
		FD_CLR( sn->fd, sn->queue );
		QApplication::sendEvent( sn->obj, &event );
		n_act++;
	    }
	}
    }
    return n_act;
}
#endif // __ATHEOS__


/*****************************************************************************
  Main event loop
 *****************************************************************************/

/*!
  Enters the main event loop and waits until exit() is called or the
  main widget is destroyed, and Returns the value that was set via to
  exit() (which is 0 if exit() is called via quit()).

  It is necessary to call this function to start event handling. The
  main event loop receives events from the window system and
  dispatches these to the application widgets.

  Generally speaking, no user interaction can take place before
  calling exec(). As a special case, modal widgets like QMessageBox
  can be used before calling exec(), because modal widgets call exec()
  to start a local event loop.

  To make your application perform idle processing, i.e. executing a
  special function whenever there are no pending events, use a QTimer
  with 0 timeout. More advanced idle processing schemes can be
  achieved by using processEvents() and processOneEvent().

  \sa quit(), exit(), processEvents(), setMainWidget()
*/

#ifndef __ATHEOS__
int QApplication::exec()
{
    quit_now = FALSE;
    quit_code = 0;

#if defined(QT_THREAD_SUPPORT)
    qApp->unlock(FALSE);
#endif

    enter_loop();

    return quit_code;
}


/*!
  Processes the next event and returns TRUE if there was an event
  (excluding posted events or zero-timer events) to process.

  This function returns immediately if \a canWait is FALSE. It might go
  into a sleep/wait state if \a canWait is TRUE.

  \sa processEvents()
*/

bool QApplication::processNextEvent( bool canWait )
{
    XEvent event;
    int	   nevents = 0;

#if defined(QT_THREAD_SUPPORT)
    qApp->lock();
#endif

    emit guiThreadAwake();

    if (qt_is_gui_used ) {
	sendPostedEvents();

       	// Two loops so that posted events accumulate
	while ( XPending(appDpy) ) {
	    while ( XPending(appDpy) ) {	// also flushes output buffer
		if ( app_exit_loop ) {          // quit between events
#if defined(QT_THREAD_SUPPORT)
		    qApp->unlock(FALSE);
#endif

		    return FALSE;
		}

		XNextEvent( appDpy, &event );	// get next event
		nevents++;

		if ( x11ProcessEvent( &event ) == 1 ) {
#if defined(QT_THREAD_SUPPORT)
		    qApp->unlock(FALSE);
#endif

		    return TRUE;
		}
	    }

	    sendPostedEvents();
	}
    }

    if ( app_exit_loop ) {			// break immediately
#if defined(QT_THREAD_SUPPORT)
	qApp->unlock(FALSE);
#endif

	return FALSE;
    }

    sendPostedEvents();

    static timeval zerotm;
    timeval *tm = qt_wait_timer();		// wait for timer or X event
    if ( !canWait ) {
	if ( !tm )
	    tm = &zerotm;
	tm->tv_sec  = 0;			// no time to wait
	tm->tv_usec = 0;
    }
    if ( sn_highest >= 0 ) {			// has socket notifier(s)
	if ( sn_read )
	    app_readfds = sn_readfds;
	else
	    FD_ZERO( &app_readfds );
	if ( sn_write )
	    app_writefds = sn_writefds;
	if ( sn_except )
	    app_exceptfds = sn_exceptfds;
    } else {
	FD_ZERO( &app_readfds );
    }

    int highest = sn_highest;
    if ( qt_is_gui_used ) {
	FD_SET( app_Xfd, &app_readfds );
	highest = QMAX( highest, app_Xfd );
	XFlush( appDpy );
    }
    int nsel;

#if defined(_OS_UNIX_)
    FD_SET( qt_thread_pipe[0], &app_readfds );
    highest = QMAX( highest, qt_thread_pipe[0] );
#endif

    if ( qt_preselect_handler ) {
	QVFuncList::Iterator end = qt_preselect_handler->end();
	for ( QVFuncList::Iterator it = qt_preselect_handler->begin(); it != end; ++it )
	    (**it)();
    }

#if defined(_OS_WIN32_)
#define FDCAST (fd_set*)
#else
#define FDCAST (void*)
#endif

#if defined(QT_THREAD_SUPPORT)
    qApp->unlock(FALSE);
#endif

    nsel = select( highest + 1,
		   FDCAST (&app_readfds),
		   FDCAST (sn_write  ? &app_writefds  : 0),
		   FDCAST (sn_except ? &app_exceptfds : 0),
		   tm );

#undef FDCAST

#if defined(QT_THREAD_SUPPORT)
    qApp->lock();
#endif

#if defined(_OS_UNIX_)
    if ( nsel > 0 && FD_ISSET( qt_thread_pipe[0], &app_readfds ) ) {
	char c;
	::read(qt_thread_pipe[0], &c, 1);
    }
#endif

    if ( qt_postselect_handler ) {
	QVFuncList::Iterator end = qt_postselect_handler->end();
	for ( QVFuncList::Iterator it = qt_postselect_handler->begin(); it != end; ++it )
	    (**it)();
    }

    if ( nsel == -1 ) {
	if ( errno == EINTR || errno == EAGAIN ) {
	    errno = 0;

#if defined(QT_THREAD_SUPPORT)
	    qApp->unlock(FALSE);
#endif

	    return (nevents > 0);
	} else {
	    ; // select error
	}
    } else if ( nsel > 0 && sn_highest >= 0 ) {
	nevents += sn_activate();
    }

    nevents += qt_activate_timers();		// activate timers
    qt_reset_color_avail();			// color approx. optimization

#if defined(QT_THREAD_SUPPORT)
    qApp->unlock(FALSE);
#endif

    return (nevents > 0);
}


void QApplication::wakeUpGuiThread()
{
#  if defined(_OS_UNIX_)
    char c = 0;
    int nbytes;
    if ( ::ioctl(qt_thread_pipe[0], FIONREAD, (char*)&nbytes) >= 0 && nbytes == 0 ) {
	::write(  qt_thread_pipe[1], &c, 1  );
    }
#  endif
}

int QApplication::x11ClientMessage(QWidget* w, XEvent* event, bool passive_only)
{
    QETWidget *widget = (QETWidget*)w;
    if ( event->xclient.format == 32 && event->xclient.message_type ) {
	if ( event->xclient.message_type == qt_wm_protocols ) {
	    Atom a = event->xclient.data.l[0];
	    if ( a == qt_wm_delete_window ) {
		if ( passive_only ) return 0;
		widget->translateCloseEvent(event);
	    }
	    else if ( a == qt_wm_take_focus ) {
		QWidget * amw = activeModalWidget();
		if ( (ulong) event->xclient.data.l[1] > qt_x_time )
		    qt_x_time = event->xclient.data.l[1];
		if ( amw && amw != widget ) {
		    QWidget* groupLeader = widget;
		    while ( groupLeader && !groupLeader->testWFlags( Qt::WGroupLeader ) )
			groupLeader = groupLeader->parentWidget();
		    if ( !groupLeader ) {
			amw->raise(); //  help broken window managers
			XSetInputFocus( appDpy, amw->winId(),
					RevertToParent, qt_x_time );
		    }
		}
	    } else if ( a == qt_net_wm_context_help ) {
		QWhatsThis::enterWhatsThisMode();
	    }
	} else if ( event->xclient.message_type == qt_qt_scrolldone ) {
	    widget->translateScrollDoneEvent(event);
	} else if ( event->xclient.message_type == qt_xdnd_position ) {
	    qt_handle_xdnd_position( widget, event, passive_only );
	} else if ( event->xclient.message_type == qt_xdnd_enter ) {
	    qt_handle_xdnd_enter( widget, event, passive_only );
	} else if ( event->xclient.message_type == qt_xdnd_status ) {
	    qt_handle_xdnd_status( widget, event, passive_only );
	} else if ( event->xclient.message_type == qt_xdnd_leave ) {
	    qt_handle_xdnd_leave( widget, event, passive_only );
	} else if ( event->xclient.message_type == qt_xdnd_drop ) {
	    qt_handle_xdnd_drop( widget, event, passive_only );
	} else if ( event->xclient.message_type == qt_xdnd_finished ) {
	    qt_handle_xdnd_finished( widget, event, passive_only );
	} else {
	    if ( passive_only ) return 0;
	    // All other are interactions
	}
    } else {
	qt_motifdnd_handle_msg( widget, event, passive_only );
    }

    return 0;
}





/*! This virtual does the core processing of individual X events,
normally by dispatching Qt events to the right destination.

It returns 1 if the event was consumed by special handling, 0 if the
event was consumed by normal handling, and -1 if the event was for an
unrecognized widget.

\sa x11EventFilter()
*/
int QApplication::x11ProcessEvent( XEvent* event )
{
    if ( qt_x11EventFilter(event) )		// send through app filter
	return 1;

    QETWidget *widget = (QETWidget*)QWidget::find( (WId)event->xany.window );

    if ( wPRmapper ) {				// just did a widget reparent?
	if ( widget == 0 ) {			// not in std widget mapper
	    switch ( event->type ) {		// only for mouse/key events
	    case ButtonPress:
	    case ButtonRelease:
	    case MotionNotify:
	    case XKeyPress:
	    case XKeyRelease:
		widget = qPRFindWidget( event->xany.window );
		break;
	    }
	}
	else if ( widget->testWState(WState_Reparented) )
	    qPRCleanup( widget );		// remove from alt mapper
    }

    QETWidget *keywidget=0;
    bool grabbed=FALSE;
    if ( event->type==XKeyPress || event->type==XKeyRelease ) {
	keywidget = (QETWidget*)QWidget::keyboardGrabber();
	if ( keywidget ) {
	    grabbed = TRUE;
	} else {
	    if ( focus_widget )
		keywidget = (QETWidget*)focus_widget;
	    else if ( inPopupMode() )
 		widget = (QETWidget*) activePopupWidget();
	    if ( !keywidget && widget )
		keywidget = widget->focusWidget()?(QETWidget*)widget->focusWidget():widget;
	}
    }


    int xkey_keycode = event->xkey.keycode;
    if ( XFilterEvent( event, keywidget ? keywidget->topLevelWidget()->winId() : None ) ) {
	if ( keywidget )
	    composingKeycode = xkey_keycode; // ### not documented in xlib
	return 1;
    }

    if ( event->type == MappingNotify ) {	// keyboard mapping changed
	XRefreshKeyboardMapping( &event->xmapping );
	return 0;
    }

    if ( event->type == PropertyNotify ) {	// some properties changed
	qt_x_time = event->xproperty.time;
	if ( event->xproperty.window == appRootWin ) { // root properties
	    if ( event->xproperty.atom == qt_selection_sentinel ) {
		if (clipboard()->receivers(SIGNAL(dataChanged())) &&
		    qt_check_selection_sentinel( event ) )
		    emit clipboard()->dataChanged();
	    } else if ( event->xproperty.atom == qt_input_encoding ) {
		qt_set_input_encoding();
	    } else if ( obey_desktop_settings ) {
		if ( event->xproperty.atom == qt_resource_manager )
		    qt_set_x11_resources();
		else if ( event->xproperty.atom == qt_desktop_properties )
		    qt_set_desktop_properties();
	    }
	} else if ( widget ) { // widget properties
	    // nothing yet
	    if (widget->isTopLevel() &&
		widget->winId() == event->xproperty.window &&
		event->xproperty.atom == qt_wm_state) {
		if (event->xproperty.state == PropertyDelete) {
		    // the window manager has removed the WM State property, so it
		    // is now in the withdrawn state (ICCCM 4.1.3.1)
		    // we are free to reuse this window
		    widget->topData()->parentWinId = 0;
		    if ( qt_deferred_map_contains( widget ) ) {
			qt_deferred_map_take( widget );
			XMapWindow( appDpy, widget->winId() );
		    }
		} else if (widget->topData()->parentWinId != appRootWin) {
                    // the window manager has changed the WM State property... we are
		    // wanting to see if we are withdrawn so that we can reuse this
		    // window... we only do this check *IF* we haven't been reparented
		    // to root (the parentWinId != appRootWin) check above

                    Atom type;
                    int format;
                    ulong  nitems, after;
                    unsigned char *data;
                    if (XGetWindowProperty(appDpy, widget->winId(),
					   qt_wm_state,
                                           0, 2, FALSE, qt_wm_state, &type,
                                           &format, &nitems, &after,
                                           &data ) == Success &&
                        type == qt_wm_state && format == 32 && nitems > 0) {
                        long *state = (long *) data;
                        switch (state[0]) {
			case WithdrawnState:
			    // if we are in the withdrawn state, we are free to reuse
			    // this window provided we remove the WM_STATE property
			    // (ICCCM 4.1.3.1)
			    XDeleteProperty(appDpy, widget->winId(), qt_wm_state);

			    // set the parent id to zero, so that show() will work again
			    widget->topData()->parentWinId = 0;
			    if ( qt_deferred_map_contains( widget ) ) {
				qt_deferred_map_take( widget );
				XMapWindow( appDpy, widget->winId() );
			    }
                            break;
                        default:
                            break;
                        }
                    }

                    if (data)
                        XFree(data);
                }
            }
        }
	return 0;
    }

    if ( !widget ) {				// don't know this windows
	QWidget* popup = QApplication::activePopupWidget();
	if ( popup ) {

	    /*
	      That is more than suboptimal. The real solution should
	      do some keyevent and buttonevent translation, so that
	      the popup still continues to work as the user expects.
	      Unfortunately this translation is currently only
	      possible with a known widget. I'll change that soon
	      (Matthias).
	    */

	    // Danger - make sure we don't lock the server
	    switch ( event->type ) {
	    case ButtonPress:
	    case ButtonRelease:
	    case XKeyPress:
	    case XKeyRelease:
		do {
		    popup->close();
		} while ( (popup = qApp->activePopupWidget()) );
		return 1;
	    }
	} else {
	    void qt_np_process_foreign_event(XEvent*); // in qnpsupport.cpp
 	    if ( !activeModalWidget() ||
 		 ( event->type != ButtonRelease &&
		   event->type != ButtonPress &&
		   event->type != MotionNotify &&
		   event->type != XKeyPress &&
		   event->type != XKeyRelease ) )
		qt_np_process_foreign_event( event );
	}
	return -1;
    }

    if ( app_do_modal )				// modal event handling
	if ( !qt_try_modal(widget, event) ) {
	    if ( event->type == ClientMessage )
		x11ClientMessage( widget, event, TRUE );
	    return 1;
	}


    if ( widget->x11Event(event) )		// send through widget filter
	return 1;

    switch ( event->type ) {

    case ButtonPress:			// mouse event
    case ButtonRelease:
    case MotionNotify:
	qt_x_time = (event->type == MotionNotify) ?
		    event->xmotion.time : event->xbutton.time;
	widget->translateMouseEvent( event );
	break;

    case XKeyPress:				// keyboard event
    case XKeyRelease: {
	qt_x_time = event->xkey.time;
	if ( keywidget ) // should always exist
	    keywidget->translateKeyEvent( event, grabbed );
    }
	break;

    case GraphicsExpose:
    case Expose:				// paint event
	widget->translatePaintEvent( event );
	break;

    case ConfigureNotify:			// window move/resize event
	if ( event->xconfigure.event == event->xconfigure.window )
	    widget->translateConfigEvent( event );
	break;

    case XFocusIn: {				// got focus
	if ( widget == desktop() )
	    break;
 	if ( inPopupMode() ) // some delayed focus event to ignore
 	    break;
 	if ( !widget->isTopLevel() )
	    break;
	if ( event->xfocus.mode != NotifyNormal )
	    break;
	if ( event->xfocus.detail != NotifyAncestor &&
	     event->xfocus.detail != NotifyInferior &&
	     event->xfocus.detail != NotifyNonlinear )
	    break;
	setActiveWindow( widget );
    }
	break;

    case XFocusOut:				// lost focus
	if ( widget == desktop() )
	    break;
 	if ( !widget->isTopLevel() )
	    break;
	if (event->xfocus.mode == NotifyWhileGrabbed && inPopupMode()) {
	    QWidget* popup = QApplication::activePopupWidget();
	    if ( popup ) {
		// Danger - make sure we don't lock the server
		do {
		    popup->close();
		} while ( (popup = qApp->activePopupWidget()) );
	    }
	}
	if ( event->xfocus.mode != NotifyNormal )
	    break;
	if ( event->xfocus.detail != NotifyAncestor &&
	     event->xfocus.detail != NotifyNonlinearVirtual &&
	     event->xfocus.detail != NotifyNonlinear )
	    break;
	if ( !inPopupMode() && widget == active_window )
	    setActiveWindow( 0 );
	break;

    case EnterNotify: {			// enter window
	qt_x_time = event->xcrossing.time;

	// 	// check PointerRoot focus
	// 	if ( event->xcrossing.focus &&
	// 	     widget->isTopLevel() && !widget->isDesktop () &&
	// 	     event->xcrossing.detail != NotifyInferior &&!inPopupMode() )
	// 	    setActiveWindow (widget);

	if ( QWidget::mouseGrabber()  && widget != QWidget::mouseGrabber() )
	    break;
 	if ( inPopupMode() && widget->topLevelWidget() != activePopupWidget() )
 	    break;
 	if ( event->xcrossing.mode != NotifyNormal ||
 	     event->xcrossing.detail == NotifyVirtual  ||
 	     event->xcrossing.detail == NotifyNonlinearVirtual )
 	    break;
	qt_dispatchEnterLeave( widget, QWidget::find( curWin ) );
	curWin = widget->winId();
	widget->translateMouseEvent( event ); //we don't get MotionNotify, emulate it
    }
	break;

    case LeaveNotify: {			// leave window
	qt_x_time = event->xcrossing.time;

	// 	// check PointerRoot focus
	// 	if ( event->xcrossing.focus &&
	// 	     widget->isTopLevel() && !widget->isDesktop () &&
	// 	     event->xcrossing.detail != NotifyInferior &&!inPopupMode() )
	// 	    setActiveWindow ( 0 );

	if ( QWidget::mouseGrabber()  && widget != QWidget::mouseGrabber() )
	    break;
 	if ( curWin && widget->winId() != curWin )
 	    break;
 	if ( event->xcrossing.mode != NotifyNormal )
 	    break;
	if ( !widget->isDesktop() )
	    widget->translateMouseEvent( event ); //we don't get MotionNotify, emulate it

	QWidget* enter = 0;
	XEvent ev;
	while ( XCheckMaskEvent( widget->x11Display(), EnterWindowMask | LeaveWindowMask , &ev )
		&& !qt_x11EventFilter( &ev ) ) {
	    if ( ev.type == LeaveNotify ) {
		XPutBackEvent( widget->x11Display(), &ev );
		break;
	    }
 	    if (  ev.xcrossing.mode != NotifyNormal ||
		  ev.xcrossing.detail == NotifyVirtual  ||
		  ev.xcrossing.detail == NotifyNonlinearVirtual )
 		continue;
	    enter = QWidget::find( ev.xcrossing.window );
	    break;
	}

	if ( !curWin )
	    qt_dispatchEnterLeave( widget, 0 );
	qt_dispatchEnterLeave( enter, widget );
	curWin = enter ? enter->winId() : 0;
    }
	break;

    case UnmapNotify:			// window hidden
	if ( widget->isTopLevel() && widget->isVisible() && !widget->isPopup() ) {
	    widget->clearWState( WState_Visible );
	    QHideEvent e( TRUE );
	    QApplication::sendEvent( widget, &e );
	    widget->sendHideEventsToChildren( TRUE );
	}
	break;

    case MapNotify:				// window shown
	if ( widget->isTopLevel() && !widget->isVisible()
	     && !widget->isHidden() ) {
	    widget->setWState( WState_Visible );
	    widget->sendShowEventsToChildren( TRUE );
	    QShowEvent e( TRUE );
	    QApplication::sendEvent( widget, &e );
	}
	break;

    case ClientMessage:			// client message
	return x11ClientMessage(widget,event,FALSE);

    case ReparentNotify:			// window manager reparents
	while ( XCheckTypedWindowEvent( widget->x11Display(),
					widget->winId(),
					ReparentNotify,
					event ) )
	    ;	// skip old reparent events
	if ( event->xreparent.parent == appRootWin ) {
	    if ( widget->isTopLevel() ) {
		widget->topData()->parentWinId = event->xreparent.parent;
		if ( qt_deferred_map_contains( widget ) ) {
		    qt_deferred_map_take( widget );
		    XMapWindow( appDpy, widget->winId() );
		}
	    }
	}
	else if (!QWidget::find((WId)event->xreparent.parent) )
	    {
		Window parent = event->xreparent.parent;
		int x = event->xreparent.x;
		int y = event->xreparent.y;
		XWindowAttributes a;
		qt_ignore_badwindow();
		XGetWindowAttributes( widget->x11Display(), parent, &a );
		if (qt_badwindow())
		    break;

		QRect& r = widget->crect;
		QRect frect ( r );

 		// help OL(V)WM to get things right: they sometimes
 		// need a little bit longer, i.e. both a and x/y may
 		// contain bogus values at this point in time.
 		int count = 0;
 		while ( count < 10 && a.x == 0 && a.y == 0 && ( a.width < r.width() || a.height < r.height() ) ) {
 		    count++;
 		    QApplication::syncX();
 		    qt_ignore_badwindow();
 		    XGetWindowAttributes( widget->x11Display(), parent, &a );
 		    if (qt_badwindow())
 			break;
		}

		if ( x <= 4 && y <= 4 && a.width <= r.width()+8 && a.height <= r.height()+8 ) {
		    // multi reparenting window manager, parent is
		    // just a shell. The 4 is for safety to support
		    // BlackBox...
		    Window root_return, parent_return, *children_return;
		    unsigned int nchildren;
		    if ( XQueryTree( widget->x11Display(), parent,
				     &root_return, &parent_return,
				     &children_return, &nchildren) ) {
			if ( children_return )
			    XFree( (void*) children_return );
			XWindowAttributes a2;
			qt_ignore_badwindow();
			XGetWindowAttributes( widget->x11Display(), parent_return,
					      &a2 );
			if (qt_badwindow())
			    break;
			x += a.x;
			y += a.y;
			frect.setRect(r.left() - x , r.top() - y, a2.width, a2.height );
		    }
		} else {
		    if ( x == y && x == 0 && a.x == r.x() && a.y == r.y()  ) {
			// we do not believe this, OL(V)M tries to lie to us
			XWindowAttributes aw;
			qt_ignore_badwindow();
			XGetWindowAttributes( widget->x11Display(), widget->winId(), &aw );
			if (qt_badwindow())
			    break;
			x = aw.x;
			y = aw.y;
		    }
		    // single reparenting window manager
		    frect.setRect(r.left() - x, r.top() - y, a.width, a.height );
		}

		widget->fpos = frect.topLeft();
		widget->topData()->fsize = frect.size();

		// store the parent. Useful for many things, embedding for instance.
		widget->topData()->parentWinId = parent;
	    }
	break;

    case SelectionRequest:
	if ( qt_xdnd_selection ) {
	    XSelectionRequestEvent *req = &event->xselectionrequest;
	    if ( req && req->selection == qt_xdnd_selection ) {
		qt_xdnd_handle_selection_request( req );
		break;
	    }
	}
	// FALL THROUGH
    case SelectionClear:
    case SelectionNotify:
	if ( qt_clipboard ) {
	    QCustomEvent e( QEvent::Clipboard, event );
	    QApplication::sendEvent( qt_clipboard, &e );
	}
	break;

    default:
	break;
    }

    return 0;
}


/*!
  Processes pending events for \a maxtime milliseconds or until there
  are no more events to process, whichever is shorter.

  You can call this function occasionally when you program is busy doing a
  long operation (e.g. copying a file).

  \sa processOneEvent(), exec(), QTimer
*/
void QApplication::processEvents( int maxtime )
{
    QTime start = QTime::currentTime();
    QTime now;
    while ( !app_exit_loop && processNextEvent(FALSE) ) {
	now = QTime::currentTime();
	if ( start.msecsTo(now) > maxtime )
	    break;
    }
}
#endif // __ATHEOS__


/*!
  This virtual function is only implemented under X11.

  If you create an application that inherits QApplication and
  reimplement this function, you get direct access to all X events
  that the are received from the X server.

  Return TRUE if you want to stop the event from being processed, or
  return FALSE for normal event dispatching.

  \sa x11ProcessEvent()
*/

bool QApplication::x11EventFilter( XEvent * )
{
    return FALSE;
}



/*****************************************************************************
  Modal widgets; Since Xlib has little support for this we roll our own
  modal widget mechanism.
  A modal widget without a parent becomes application-modal.
  A modal widget with a parent becomes modal to its parent and grandparents..

  qt_enter_modal()
	Enters modal state
	Arguments:
	    QWidget *widget	A modal widget

  qt_leave_modal()
	Leaves modal state for a widget
	Arguments:
	    QWidget *widget	A modal widget
 *****************************************************************************/

bool qt_modal_state()
{
    return app_do_modal;
}

#ifndef __ATHEOS__
void qt_enter_modal( QWidget *widget )
{
    if ( !qt_modal_stack ) {			// create modal stack
	qt_modal_stack = new QWidgetList;
	CHECK_PTR( qt_modal_stack );
    }
    qt_modal_stack->insert( 0, widget );
    app_do_modal = TRUE;
    QWidget *w = QWidget::find( (WId)curWin );
    if ( w ) { // send synthetic leave event
	QEvent e( QEvent::Leave );
	QApplication::sendEvent( w, &e );
    }
}


void qt_leave_modal( QWidget *widget )
{
    if ( qt_modal_stack && qt_modal_stack->removeRef(widget) ) {
	if ( qt_modal_stack->isEmpty() ) {
	    delete qt_modal_stack;
	    qt_modal_stack = 0;
	    QPoint p( QCursor::pos() );
	    QWidget* w = QApplication::widgetAt( p.x(), p.y(), TRUE );
	    qt_dispatchEnterLeave( w, QWidget::find( curWin ) ); // send synthetic enter event
	    curWin = w? w->winId() : 0;
	}
    }
    app_do_modal = qt_modal_stack != 0;
}


static bool qt_try_modal( QWidget *widget, XEvent *event )
{
    if ( qApp->activePopupWidget() )
	return TRUE;
    if ( widget->testWFlags(Qt::WStyle_Tool) )	// allow tool windows
	return TRUE;

    QWidget *modal=0, *top=QApplication::activeModalWidget();

    QWidget* groupLeader = widget;
    widget = widget->topLevelWidget();

    if ( widget->testWFlags(Qt::WType_Modal) )	// widget is modal
	modal = widget;
    if ( !top || modal == top )			// don't block event
	return TRUE;

    while ( groupLeader && !groupLeader->testWFlags( Qt::WGroupLeader ) )
	groupLeader = groupLeader->parentWidget();

    if ( groupLeader ) {
	// Does groupLeader have a child in qt_modal_stack?
	bool unrelated = TRUE;
	modal = qt_modal_stack->first();
	while (modal && unrelated) {
	    QWidget* p = modal->parentWidget();
	    while ( p && p != groupLeader && !p->testWFlags( Qt::WGroupLeader) ) {
		p = p->parentWidget();
	    }
	    modal = qt_modal_stack->next();
	    if ( p == groupLeader ) unrelated = FALSE;
	}

	if ( unrelated )
	    return TRUE;		// don't block event
    }

    bool block_event  = FALSE;
    bool expose_event = FALSE;

    switch ( event->type ) {
	case ButtonPress:			// disallow mouse/key events
	case ButtonRelease:
	case MotionNotify:
	case XKeyPress:
	case XKeyRelease:
	case XFocusIn:
	case XFocusOut:
	case ClientMessage:
	case EnterNotify:
	case LeaveNotify:
	    block_event	 = TRUE;
	    break;
	case Expose:
	    expose_event = TRUE;
	    break;
    }

    return !block_event;
}


/*****************************************************************************
  Popup widget mechanism

  openPopup()
	Adds a widget to the list of popup widgets
	Arguments:
	    QWidget *widget	The popup widget to be added

  closePopup()
	Removes a widget from the list of popup widgets
	Arguments:
	    QWidget *widget	The popup widget to be removed
 *****************************************************************************/

void QApplication::openPopup( QWidget *popup )
{
    if ( !popupWidgets ) {			// create list
	popupWidgets = new QWidgetList;
	CHECK_PTR( popupWidgets );
	if ( !activeBeforePopup )
	    activeBeforePopup = new QGuardedPtr<QWidget>;
	(*activeBeforePopup) = active_window;
    }
    popupWidgets->append( popup );		// add to end of list
    if ( popupWidgets->count() == 1 && !qt_nograb() ){ // grab mouse
	int r = XGrabPointer( popup->x11Display(), popup->winId(), TRUE,
			      (uint)(ButtonPressMask | ButtonReleaseMask |
				     ButtonMotionMask | EnterWindowMask |
				     LeaveWindowMask | PointerMotionMask),
			      GrabModeSync, GrabModeAsync,
			      None, None, CurrentTime );
	if ( (popupGrabOk = (r == GrabSuccess)) ) {
	    XAllowEvents( popup->x11Display(), SyncPointer, CurrentTime );
	}
	if ( !active_window ) {
	    QWidget *tlw = popup->topLevelWidget();
	    XSetInputFocus( tlw->x11Display(), tlw->winId(), RevertToNone, CurrentTime );
	} else {
	    XSetInputFocus( active_window->x11Display(), active_window->winId(), RevertToNone, CurrentTime );
	}
    } else if ( popupGrabOk ) {
	XAllowEvents(  popup->x11Display(), SyncPointer, CurrentTime );
    }

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    QFocusEvent::setReason( QFocusEvent::Popup );
    if ( popup->focusWidget())
	popup->focusWidget()->setFocus();
    else
	popup->setFocus();
    QFocusEvent::resetReason();
}

void QApplication::closePopup( QWidget *popup )
{
    if ( !popupWidgets )
	return;
    popupWidgets->removeRef( popup );
    if (popup == popupOfPopupButtonFocus) {
	popupButtonFocus = 0;
	popupOfPopupButtonFocus = 0;
    }
    if ( popupWidgets->count() == 0 ) {		// this was the last popup
	popupCloseDownMode = TRUE;		// control mouse events
	delete popupWidgets;
	popupWidgets = 0;
	if ( !qt_nograb() && popupGrabOk ) {	// grabbing not disabled
	    if ( mouseButtonState != 0
		 || popup->geometry(). contains(QPoint(mouseGlobalXPos, mouseGlobalYPos) ) )
		{	// mouse release event or inside
		    XAllowEvents( popup->x11Display(), AsyncPointer,
				  CurrentTime );
	    } else {				// mouse press event
		mouseButtonPressTime -= 10000;	// avoid double click
		XAllowEvents( popup->x11Display(), ReplayPointer,CurrentTime );
	    }
	    XUngrabPointer( popup->x11Display(), CurrentTime );
	    XFlush( popup->x11Display() );
	}
	active_window = (*activeBeforePopup);
	// restore the former active window immediately, although
	// we'll get a focusIn later from X
	if ( active_window ) {
	    QFocusEvent::setReason( QFocusEvent::Popup );
	    if (active_window->focusWidget())
		active_window->focusWidget()->setFocus();
	    else
		active_window->setFocus();
	    QFocusEvent::resetReason();
	}
    }
     else {
	// popups are not focus-handled by the window system (the
	// first popup grabbed the keyboard), so we have to do that
	// manually: A popup was closed, so the previous popup gets
	// the focus.
	 QFocusEvent::setReason( QFocusEvent::Popup );
	 QWidget* aw = popupWidgets->getLast();
	 if (aw->focusWidget())
	     aw->focusWidget()->setFocus();
	 else
	     aw->setFocus();
	 QFocusEvent::resetReason();
     }
}
#endif // __ATHEOS__


/*****************************************************************************
  Timer handling; Xlib has no application timer support so we'll have to
  make our own from scratch.

  NOTE: These functions are for internal use. QObject::startTimer() and
	QObject::killTimer() are for public use.
	The QTimer class provides a high-level interface which translates
	timer events into signals.

  qStartTimer( interval, obj )
	Starts a timer which will run until it is killed with qKillTimer()
	Arguments:
	    int interval	timer interval in milliseconds
	    QObject *obj	where to send the timer event
	Returns:
	    int			timer identifier, or zero if not successful

  qKillTimer( timerId )
	Stops a timer specified by a timer identifier.
	Arguments:
	    int timerId		timer identifier
	Returns:
	    bool		TRUE if successful

  qKillTimer( obj )
	Stops all timers that are sent to the specified object.
	Arguments:
	    QObject *obj	object receiving timer events
	Returns:
	    bool		TRUE if successful
 *****************************************************************************/

//
// Internal data structure for timers
//

struct TimerInfo {				// internal timer info
    int	     id;				// - timer identifier
    timeval  interval;				// - timer interval
    timeval  timeout;				// - when to sent event
    QObject *obj;				// - object to receive event
};

typedef QList<TimerInfo> TimerList;	// list of TimerInfo structs

static QBitArray *timerBitVec;			// timer bit vector
static TimerList *timerList	= 0;		// timer list


//
// Internal operator functions for timevals
//

static inline bool operator<( const timeval &t1, const timeval &t2 )
{
    return t1.tv_sec < t2.tv_sec ||
	  (t1.tv_sec == t2.tv_sec && t1.tv_usec < t2.tv_usec);
}

static inline timeval &operator+=( timeval &t1, const timeval &t2 )
{
    t1.tv_sec += t2.tv_sec;
    if ( (t1.tv_usec += t2.tv_usec) >= 1000000 ) {
	t1.tv_sec++;
	t1.tv_usec -= 1000000;
    }
    return t1;
}

static inline timeval operator+( const timeval &t1, const timeval &t2 )
{
    timeval tmp;
    tmp.tv_sec = t1.tv_sec + t2.tv_sec;
    if ( (tmp.tv_usec = t1.tv_usec + t2.tv_usec) >= 1000000 ) {
	tmp.tv_sec++;
	tmp.tv_usec -= 1000000;
    }
    return tmp;
}

static inline timeval operator-( const timeval &t1, const timeval &t2 )
{
    timeval tmp;
    tmp.tv_sec = t1.tv_sec - t2.tv_sec;
    if ( (tmp.tv_usec = t1.tv_usec - t2.tv_usec) < 0 ) {
	tmp.tv_sec--;
	tmp.tv_usec += 1000000;
    }
    return tmp;
}


//
// Internal functions for manipulating timer data structures.
// The timerBitVec array is used for keeping track of timer identifiers.
//

static int allocTimerId()			// find avail timer identifier
{
    int i = timerBitVec->size()-1;
    while ( i >= 0 && (*timerBitVec)[i] )
	i--;
    if ( i < 0 ) {
	i = timerBitVec->size();
	timerBitVec->resize( 4 * i );
	for( int j=timerBitVec->size()-1; j > i; j-- )
	    timerBitVec->clearBit( j );
    }
    timerBitVec->setBit( i );
    return i+1;
}

static void insertTimer( const TimerInfo *ti )	// insert timer info into list
{
    TimerInfo *t = timerList->first();
    int index = 0;
#if defined(DEBUG)
    int dangerCount = 0;
#endif
    while ( t && t->timeout < ti->timeout ) {	// list is sorted by timeout
#if defined(DEBUG)
	if ( t->obj == ti->obj )
	    dangerCount++;
#endif
	t = timerList->next();
	index++;
    }
    timerList->insert( index, ti );		// inserts sorted
#if defined(DEBUG)
    if ( dangerCount > 16 )
	qDebug( "QObject: %d timers now exist for object %s::%s",
	       dangerCount, ti->obj->className(), ti->obj->name() );
#endif
}

static inline void getTime( timeval &t )	// get time of day
{
    gettimeofday( &t, 0 );
    while ( t.tv_usec >= 1000000 ) {		// NTP-related fix
	t.tv_usec -= 1000000;
	t.tv_sec++;
    }
    while ( t.tv_usec < 0 ) {
	if ( t.tv_sec > 0 ) {
	    t.tv_usec += 1000000;
	    t.tv_sec--;
	} else {
	    t.tv_usec = 0;
	    break;
	}
    }
}

static void repairTimer( const timeval &time )	// repair broken timer
{
    if ( !timerList )				// not initialized
	return;
    timeval diff = watchtime - time;
    register TimerInfo *t = timerList->first();
    while ( t ) {				// repair all timers
	t->timeout = t->timeout - diff;
	t = timerList->next();
    }
}


//
// Timer activation functions (called from the event loop)
//

/*
  Returns the time to wait for the next timer, or null if no timers are
  waiting.

  The result is bounded to qt_wait_timer_max if this exists.
*/

timeval *qt_wait_timer()
{
    static timeval tm;
    bool first = TRUE;
    timeval currentTime;
    if ( timerList && timerList->count() ) {	// there are waiting timers
	getTime( currentTime );
	if ( first ) {
	    if ( currentTime < watchtime )	// clock was turned back
		repairTimer( currentTime );
	    first = FALSE;
	    watchtime = currentTime;
	}
	TimerInfo *t = timerList->first();	// first waiting timer
	if ( currentTime < t->timeout ) {	// time to wait
	    tm = t->timeout - currentTime;
	} else {
	    tm.tv_sec  = 0;			// no time to wait
	    tm.tv_usec = 0;
	}
	if ( qt_wait_timer_max && *qt_wait_timer_max < tm )
	    tm = *qt_wait_timer_max;
	return &tm;
    }
    if ( qt_wait_timer_max ) {
	tm = *qt_wait_timer_max;
	return &tm;
    }
    return 0;					// no timers
}

/*
  Activates the timer events that have expired. Returns the number of timers
  (not 0-timer) that were activated.
*/

int qt_activate_timers()
{
    if ( !timerList || !timerList->count() )	// no timers
	return 0;
    bool first = TRUE;
    timeval currentTime;
    int maxcount = timerList->count();
    int n_act = 0;
    register TimerInfo *t;
    while ( maxcount-- ) {			// avoid starvation
	getTime( currentTime );			// get current time
	if ( first ) {
	    if ( currentTime < watchtime )	// clock was turned back
		repairTimer( currentTime );
	    first = FALSE;
	    watchtime = currentTime;
	}
	t = timerList->first();
	if ( !t || currentTime < t->timeout )	// no timer has expired
	    break;
	timerList->take();			// unlink from list
	t->timeout += t->interval;
	if ( t->timeout < currentTime )
	    t->timeout = currentTime + t->interval;
	insertTimer( t );			// relink timer
	if ( t->interval.tv_usec > 0 || t->interval.tv_sec > 0 )
	    n_act++;
#ifdef __ATHEOS__
	printf( "Warning: qt_activate_timers() timer event not sendt\n" );
#else
	QTimerEvent e( t->id );
	QApplication::sendEvent( t->obj, &e );	// send event
#endif
    }
    return n_act;
}


//
// Timer initialization and cleanup routines
//

static void initTimers()			// initialize timers
{
    timerBitVec = new QBitArray( 128 );
    CHECK_PTR( timerBitVec );
    int i = timerBitVec->size();
    while( i-- > 0 )
	timerBitVec->clearBit( i );
    timerList = new TimerList;
    CHECK_PTR( timerList );
    timerList->setAutoDelete( TRUE );
}

static void cleanupTimers()			// cleanup timer data structure
{
    if ( timerList ) {
	delete timerList;
	timerList = 0;
	delete timerBitVec;
	timerBitVec = 0;
    }
}


//
// Main timer functions for starting and killing timers
//

int qStartTimer( int interval, QObject *obj )
{
    if ( !timerList )				// initialize timer data
	initTimers();
    int id = allocTimerId();			// get free timer id
    if ( id <= 0 ||
	 id > (int)timerBitVec->size() || !obj )// cannot create timer
	return 0;
    timerBitVec->setBit( id-1 );		// set timer active
    TimerInfo *t = new TimerInfo;		// create timer
    CHECK_PTR( t );
    t->id = id;
    t->interval.tv_sec  = interval/1000;
    t->interval.tv_usec = (interval%1000)*1000;
    timeval currentTime;
    getTime( currentTime );
    t->timeout = currentTime + t->interval;
    t->obj = obj;
    insertTimer( t );				// put timer in list
#ifdef __ATHEOS__
    printf( "Error: qStartTimer() qt_np_enable_timers() not called\n" );
#else
    void qt_np_enable_timers();			// in qnpsupport.cpp
    qt_np_enable_timers();
#endif
    return id;
}

bool qKillTimer( int id )
{
    register TimerInfo *t;
    if ( !timerList || id <= 0 ||
	 id > (int)timerBitVec->size() || !timerBitVec->testBit( id-1 ) )
	return FALSE;				// not init'd or invalid timer
    t = timerList->first();
    while ( t && t->id != id )			// find timer info in list
	t = timerList->next();
    if ( t ) {					// id found
	timerBitVec->clearBit( id-1 );		// set timer inactive
	return timerList->remove();
    }
    else					// id not found
	return FALSE;
}

bool qKillTimer( QObject *obj )
{
    register TimerInfo *t;
    if ( !timerList )				// not initialized
	return FALSE;
    t = timerList->first();
    while ( t ) {				// check all timers
	if ( t->obj == obj ) {			// object found
	    timerBitVec->clearBit( t->id-1 );
	    timerList->remove();
	    t = timerList->current();
	} else {
	    t = timerList->next();
	}
    }
    return TRUE;
}


/*****************************************************************************
  Event translation; translates X11 events to Qt events
 *****************************************************************************/

//
// Mouse event translation
//
// Xlib doesn't give mouse double click events, so we generate them by
// comparing window, time and position between two mouse press events.
//

#ifndef __ATHEOS__
static int translateButtonState( int s )
{
    int bst = 0;
    if ( s & Button1Mask )
	bst |= Qt::LeftButton;
    if ( s & Button2Mask )
	bst |= Qt::MidButton;
    if ( s & Button3Mask )
	bst |= Qt::RightButton;
    if ( s & ShiftMask )
	bst |= Qt::ShiftButton;
    if ( s & ControlMask )
	bst |= Qt::ControlButton;
    if ( s & Mod1Mask )
	bst |= Qt::AltButton;
    return bst;
}

bool QETWidget::translateMouseEvent( const XEvent *event )
{
    static bool manualGrab = FALSE;
    QEvent::Type type;				// event parameters
    QPoint pos;
    QPoint globalPos;
    int button = 0;
    int state;
    XEvent nextEvent;

    if ( sm_blockUserInput ) // block user interaction during session management
	return TRUE;

    if ( event->type == MotionNotify ) { // mouse move
	XMotionEvent lastMotion = event->xmotion;
	while( XPending( appDpy ) )  { // compres mouse moves
	    XNextEvent( appDpy, &nextEvent );
	    if ( nextEvent.type == ConfigureNotify
		 || nextEvent.type == PropertyNotify
		 || nextEvent.type == Expose) {
		qApp->x11ProcessEvent( &nextEvent );
		continue;
	    } else if ( nextEvent.type != MotionNotify ||
			nextEvent.xmotion.window != event->xmotion.window ||
			nextEvent.xmotion.state != event->xmotion.state ) {
		XPutBackEvent( appDpy, &nextEvent );
		break;
	    }
	    if ( !qt_x11EventFilter(&nextEvent)
		 && !x11Event( &nextEvent ) ) // send event through filter
		lastMotion = nextEvent.xmotion;
	    else
		break;
	}
	type = QEvent::MouseMove;
	pos.rx() = lastMotion.x;
	pos.ry() = lastMotion.y;
	globalPos.rx() = lastMotion.x_root;
	globalPos.ry() = lastMotion.y_root;
	state = translateButtonState( lastMotion.state );
	if ( qt_button_down && (state & (LeftButton |
					 MidButton |
					 RightButton ) ) == 0 )
	    qt_button_down = 0;
    } else if ( event->type == EnterNotify || event->type == LeaveNotify) {
	XEvent *xevent = (XEvent *)event;
	//unsigned int xstate = event->xcrossing.state;
	type = QEvent::MouseMove;
	pos.rx() = xevent->xcrossing.x;
	pos.ry() = xevent->xcrossing.y;
	globalPos.rx() = xevent->xcrossing.x_root;
	globalPos.ry() = xevent->xcrossing.y_root;
	state = translateButtonState( xevent->xcrossing.state );
	if ( qt_button_down && (state & (LeftButton |
					 MidButton |
					 RightButton ) ) == 0 )
	    qt_button_down = 0;
	if ( !qt_button_down )
	    state = state & ~(LeftButton | MidButton | RightButton );
    } else {					// button press or release
	pos.rx() = event->xbutton.x;
	pos.ry() = event->xbutton.y;
	globalPos.rx() = event->xbutton.x_root;
	globalPos.ry() = event->xbutton.y_root;
	state = translateButtonState( event->xbutton.state );
	switch ( event->xbutton.button ) {
	    case Button1: button = LeftButton;   goto DoFocus;
	    case Button2: button = MidButton;    goto DoFocus;
	    case Button3: button = RightButton;       DoFocus:
		if ( isEnabled() && event->type == ButtonPress ) {
		    QWidget* w = this;
		    while ( w->focusProxy() )
			w = w->focusProxy();
		    if ( w->focusPolicy() & QWidget::ClickFocus ) {
			QFocusEvent::setReason( QFocusEvent::Mouse);
			w->setFocus();
			QFocusEvent::resetReason();
		    }
		}
		break;
	    case Button4:
	    case Button5:
		// the fancy mouse wheel.

		// take care about grabbing.  We do this here since it
		// is clear that we return anyway
		if ( qApp->inPopupMode() && popupGrabOk )
		    XAllowEvents( x11Display(), SyncPointer, CurrentTime );

		// We are only interested in ButtonPress.
		if (event->type == ButtonPress ){

		    // compress wheel events (the X Server will simply
		    // send a button press for each single notch,
		    // regardless whether the application can catch up
		    // or not)
		    int delta = 1;
		    XEvent xevent;
		    while ( XCheckTypedWindowEvent(x11Display(),winId(),
						   ButtonPress,&xevent) ){
			if (xevent.xbutton.button != event->xbutton.button){
			    XPutBackEvent(x11Display(), &xevent);
			    break;
			}
			delta++;
		    }

		    // the delta is defined as multiples of
		    // WHEEL_DELTA, which is set to 120. Future wheels
		    // may offer a finer-resolution.  A positive delta
		    // indicates forward rotation, a negative one
		    // backward rotation respectively.
		    delta *= 120*(event->xbutton.button == Button4?1:-1);

		    translateWheelEvent( globalPos.x(), globalPos.y(), delta, state );
		}
		return TRUE;
	}
	if ( event->type == ButtonPress ) {	// mouse button pressed
	    qt_button_down = findChildWidget( this, pos );	//magic for masked widgets
	    if ( !qt_button_down || !qt_button_down->testWFlags(WMouseNoMask) )
		qt_button_down = this;
	    if ( mouseActWindow == event->xbutton.window &&
		 mouseButtonPressed == button &&
		 (long)event->xbutton.time -(long)mouseButtonPressTime
		       < QApplication::doubleClickInterval() &&
		 QABS(event->xbutton.x - mouseXPos) < 5 &&
		 QABS(event->xbutton.y - mouseYPos) < 5 ) {
		type = QEvent::MouseButtonDblClick;
		mouseButtonPressTime -= 2000;	// no double-click next time
	    } else {
		type = QEvent::MouseButtonPress;
		mouseButtonPressTime = event->xbutton.time;
	    }
	    mouseButtonPressed = button; 	// save event params for
	    mouseXPos = pos.x();		// future double click tests
	    mouseYPos = pos.y();
	    mouseGlobalXPos = globalPos.x();
	    mouseGlobalYPos = globalPos.y();
	} else {				// mouse button released
	    if ( manualGrab ) {			// release manual grab
		manualGrab = FALSE;
		XUngrabPointer( x11Display(), CurrentTime );
		XFlush( x11Display() );
	    }

	    type = QEvent::MouseButtonRelease;
	}
    }
    mouseActWindow = winId();			// save some event params
    mouseButtonState = state;
    if ( type == 0 )				// don't send event
	return FALSE;

    if ( qApp->inPopupMode() ) {			// in popup mode
	QWidget *popup = qApp->activePopupWidget();
	if ( popup != this ) {
	    if ( testWFlags(WType_Popup) && rect().contains(pos) )
		popup = this;
	    else				// send to last popup
		pos = popup->mapFromGlobal( globalPos );
	}
	bool releaseAfter = FALSE;
	QWidget *popupChild  = findChildWidget( popup, pos );
	QWidget *popupTarget = popupChild ? popupChild : popup;

	if (popup != popupOfPopupButtonFocus){
	    popupButtonFocus = 0;
	    popupOfPopupButtonFocus = 0;
	}

	if ( !popupTarget->isEnabled() ) {
	    if ( popupGrabOk )
		XAllowEvents( x11Display(), SyncPointer, CurrentTime );
	    return FALSE;
	}

	switch ( type ) {
	    case QEvent::MouseButtonPress:
	    case QEvent::MouseButtonDblClick:
		popupButtonFocus = popupChild;
		popupOfPopupButtonFocus = popup;
		break;
	    case QEvent::MouseButtonRelease:
		releaseAfter = TRUE;
		break;
	    default:
		break;				// nothing for mouse move
	}

	Display* dpy = x11Display(); // store display, send() may destroy us

	if ( popupButtonFocus ) {
	    QMouseEvent e( type, popupButtonFocus->mapFromGlobal(globalPos),
			   globalPos, button, state );
	    QApplication::sendEvent( popupButtonFocus, &e );
	    if ( releaseAfter ) {
		popupButtonFocus = 0;
		popupOfPopupButtonFocus = 0;
	    }
	} else if ( popupChild ) {
	    QMouseEvent e( type, popupChild->mapFromGlobal(globalPos),
			   globalPos, button, state );
	    QApplication::sendEvent( popupChild, &e );
	} else {
	    QMouseEvent e( type, pos, globalPos, button, state );
	    QApplication::sendEvent( popupChild ? popupChild : popup, &e );
	}

	if ( releaseAfter )
	    qt_button_down = 0;

	if ( qApp->inPopupMode() ) { // still in popup mode
	    if ( popupGrabOk )
		XAllowEvents( dpy, SyncPointer, CurrentTime );
	} else {
	    if ( type != QEvent::MouseButtonRelease && state != 0 &&
		 QWidget::find((WId)mouseActWindow) ) {
		manualGrab = TRUE;		// need to manually grab
		XGrabPointer( dpy, mouseActWindow, FALSE,
			      (uint)(ButtonPressMask | ButtonReleaseMask |
			      ButtonMotionMask |
			      EnterWindowMask | LeaveWindowMask),
			      GrabModeAsync, GrabModeAsync,
			      None, None, CurrentTime );
	    }
	}

    } else {
	QWidget *widget = this;
	QWidget *w = QWidget::mouseGrabber();
	if ( !w )
	    w = qt_button_down;
	if ( w && w != this ) {
	    widget = w;
	    pos = w->mapFromGlobal( globalPos );
	}

	if ( popupCloseDownMode ) {
	    popupCloseDownMode = FALSE;
	    if ( testWFlags(WType_Popup) )	// ignore replayed event
		return TRUE;
	}

	if ( type == QEvent::MouseButtonRelease &&
	     (state & (~button) & ( LeftButton |
				    MidButton |
				    RightButton)) == 0 ) {
	    qt_button_down = 0;
	}

	QMouseEvent e( type, pos, globalPos, button, state );
	QApplication::sendEvent( widget, &e );
    }
    return TRUE;
}


//
// Wheel event translation
//
bool QETWidget::translateWheelEvent( int global_x, int global_y, int delta, int state )
{
    QWidget* w = this;

    while ( w->focusProxy() )
	w = w->focusProxy();
    if ( w->focusPolicy() == QWidget::WheelFocus ) {
	QFocusEvent::setReason( QFocusEvent::Mouse);
	w->setFocus();
	QFocusEvent::resetReason();
    }

    // send the event to the widget or its ancestors
    if (w){
	QWidget* popup = qApp->activePopupWidget();
	if ( popup && w != popup )
	    popup->hide();
	do {
	    QWheelEvent e( w->mapFromGlobal(QPoint( global_x, global_y)),
			   QPoint(global_x, global_y), delta, state );
	    e.ignore();
	    QApplication::sendEvent( w, &e );
	    if ( e.isAccepted() )
		return TRUE;
	    w = w->isTopLevel()?0:w->parentWidget();
	} while (w);
    }

    // send the event to the widget that has the focus or its ancestors
    w = qApp->focusWidget();
    if (w){
	do {
	    QWheelEvent e( w->mapFromGlobal(QPoint( global_x, global_y)),
			   QPoint(global_x, global_y), delta, state );
	    e.ignore();
	    QApplication::sendEvent( w, &e );
	    if ( e.isAccepted() )
		return TRUE;
	    w = w->isTopLevel()?0:w->parentWidget();
	} while (w);
    }
    return FALSE;
}


//
// Keyboard event translation
//

#define XK_MISCELLANY
#define XK_LATIN1
#ifndef __ATHEOS__
#include <X11/keysymdef.h>
#endif
#ifndef XK_ISO_Left_Tab
#define	XK_ISO_Left_Tab					0xFE20
#endif
static const KeySym KeyTbl[] = {		// keyboard mapping table
    XK_Escape,		Qt::Key_Escape,		// misc keys
    XK_Tab,		Qt::Key_Tab,
    XK_ISO_Left_Tab,    Qt::Key_Backtab,
    XK_BackSpace,	Qt::Key_Backspace,
    XK_Return,		Qt::Key_Return,
    XK_Insert,		Qt::Key_Insert,
    XK_KP_Insert,	Qt::Key_Insert,
    XK_Delete,		Qt::Key_Delete,
    XK_KP_Delete,	Qt::Key_Delete,
    XK_Clear,		Qt::Key_Delete,
    XK_Pause,		Qt::Key_Pause,
    XK_Print,		Qt::Key_Print,
    0x1005FF60,		Qt::Key_SysReq,		// hardcoded Sun SysReq
    0x1007ff00,		Qt::Key_SysReq,		// hardcoded X386 SysReq
    XK_Home,		Qt::Key_Home,		// cursor movement
    XK_End,		Qt::Key_End,
    XK_Left,		Qt::Key_Left,
    XK_Up,		Qt::Key_Up,
    XK_Right,		Qt::Key_Right,
    XK_Down,		Qt::Key_Down,
    XK_Prior,		Qt::Key_Prior,
    XK_Next,		Qt::Key_Next,
    XK_KP_Home,		Qt::Key_Home,
    XK_KP_End,		Qt::Key_End,
    XK_KP_Left,		Qt::Key_Left,
    XK_KP_Up,		Qt::Key_Up,
    XK_KP_Right,	Qt::Key_Right,
    XK_KP_Down,		Qt::Key_Down,
    XK_KP_Prior,	Qt::Key_Prior,
    XK_KP_Next,		Qt::Key_Next,
    XK_Shift_L,		Qt::Key_Shift,		// modifiers
    XK_Shift_R,		Qt::Key_Shift,
    XK_Shift_Lock,	Qt::Key_Shift,
    XK_Control_L,	Qt::Key_Control,
    XK_Control_R,	Qt::Key_Control,
    XK_Meta_L,		Qt::Key_Meta,
    XK_Meta_R,		Qt::Key_Meta,
    XK_Alt_L,		Qt::Key_Alt,
    XK_Alt_R,		Qt::Key_Alt,
    XK_Caps_Lock,	Qt::Key_CapsLock,
    XK_Num_Lock,	Qt::Key_NumLock,
    XK_Scroll_Lock,	Qt::Key_ScrollLock,
    XK_KP_Space,	Qt::Key_Space,		// numeric keypad
    XK_KP_Tab,		Qt::Key_Tab,
    XK_KP_Enter,	Qt::Key_Enter,
    XK_KP_Equal,	Qt::Key_Equal,
    XK_KP_Multiply,	Qt::Key_Asterisk,
    XK_KP_Add,		Qt::Key_Plus,
    XK_KP_Separator,	Qt::Key_Comma,
    XK_KP_Subtract,	Qt::Key_Minus,
    XK_KP_Decimal,	Qt::Key_Period,
    XK_KP_Divide,	Qt::Key_Slash,
    XK_Super_L,		Qt::Key_Super_L,
    XK_Super_R,		Qt::Key_Super_R,
    XK_Menu,		Qt::Key_Menu,
    XK_Hyper_L,		Qt::Key_Hyper_L,
    XK_Hyper_R,		Qt::Key_Hyper_R,
    XK_Help,		Qt::Key_Help,
    0,			0
};
#endif // __ATHEOS__


static QIntDict<void>    *keyDict  = 0;
static QIntDict<void>    *textDict = 0;

static void deleteKeyDicts()
{
    if ( keyDict )
	delete keyDict;
    keyDict = 0;
    if ( textDict )
	delete textDict;
    textDict = 0;
}

static const unsigned short cyrillicKeysymsToUnicode[] = {
    0x0000, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457,
    0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x0000, 0x045e, 0x045f,
    0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407,
    0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x0000, 0x040e, 0x040f,
    0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
    0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
    0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,
    0x044c, 0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a,
    0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
    0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e,
    0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,
    0x042c, 0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a
};

static const unsigned short greekKeysymsToUnicode[] = {
    0x0000, 0x0386, 0x0388, 0x0389, 0x038a, 0x03aa, 0x0000, 0x038c,
    0x038e, 0x03ab, 0x0000, 0x038f, 0x0000, 0x0000, 0x0385, 0x2015,
    0x0000, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03ca, 0x0390, 0x03cc,
    0x03cd, 0x03cb, 0x03b0, 0x03ce, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
    0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f,
    0x03a0, 0x03a1, 0x03a3, 0x0000, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
    0x03a8, 0x03a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
    0x03c0, 0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7,
    0x03c8, 0x03c9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

// ### add keysym conversion for the missing ranges.
static QChar keysymToUnicode(unsigned char byte3, unsigned char byte4)
{
    if ( byte3 == 0x06 ) {
	// russian, use lookup table
	if ( byte4 > 0xa0 )
	    return QChar( cyrillicKeysymsToUnicode[byte4 - 0xa0] );
    }
    if ( byte3 == 0x07 ) {
	// greek
	if ( byte4 > 0xa0 )
	    return QChar( cyrillicKeysymsToUnicode[byte4 - 0xa0] );
    }
    return QChar(0x0);
}

#ifndef __ATHEOS__

bool QETWidget::translateKeyEventInternal( const XEvent *event, int& count,
					   QString& text,
					   int& state,
					   char& ascii, int& code )
{
    QTextCodec *mapper = input_mapper;
    QCString chars(64);
    QChar converted;
    KeySym key = 0;

    if ( !keyDict ) {
	keyDict = new QIntDict<void>( 13 );
	keyDict->setAutoDelete( FALSE );
	textDict = new QIntDict<void>( 13 );
	textDict->setAutoDelete( FALSE );
	qAddPostRoutine( deleteKeyDicts );
    }

    QWidget* tlw = topLevelWidget();

#if defined(NO_XIM)

    count = XLookupString( &((XEvent*)event)->xkey,
			   chars.data(), chars.size(), &key, 0 );

    if ( count == 1 )
	ascii = chars[0];

#else
    QEvent::Type type = (event->type == XKeyPress)
			? QEvent::KeyPress : QEvent::KeyRelease;
    // Implementation for X11R5 and newer, using XIM

    int	       keycode = event->xkey.keycode;
    Status     status;

    if ( type == QEvent::KeyPress ) {
	bool mb=FALSE;
	if ( qt_xim ) {
	    QTLWExtra*  xd = tlw->topData();
	    if ( xd->xic ) {
		mb=TRUE;
		count = XmbLookupString( (XIC)(xd->xic), &((XEvent*)event)->xkey,
					 chars.data(), chars.size(), &key, &status );
		if ( status == XBufferOverflow ) {
		    chars.resize(count+1);
		    count = XmbLookupString( (XIC)(xd->xic), &((XEvent*)event)->xkey,
					 chars.data(), chars.size(), &key, &status );
		}
	    }
	}
	if ( !mb ) {
	    count = XLookupString( &((XEvent*)event)->xkey,
				   chars.data(), chars.size(), &key, 0 );
	}
	if ( count && !keycode ) {
	    keycode = composingKeycode;
	    composingKeycode = 0;
	}
	if ( key )
	    keyDict->replace( keycode, (void*)key );
	if ( count == 0 && key < 0xff00 ) { // all keysyms smaller than that are actally keys that can be mapped to unicode chars
	    unsigned char byte3 = (unsigned char )(key >> 8);
	    int mib = -1;
	    switch( byte3 ) {
		case 0: // Latin 1
		case 1: // Latin 2
		case 2: //latin 3
		case 3: // latin4
		    mib = byte3 + 4; break;
		case 4: // kana
		    break;
		case 5: // arabic
		    mib = 82; break;
		case 6: // Cyrillic
		case 7: //greek
		    mib = -1; // manual conversion
		    mapper = 0;
		    converted = keysymToUnicode( byte3, key & 0xff );
		case 8: // technical, no mapping here at the moment
		case 9: // Special
		case 10: // Publishing
		case 11: // APL
		    break;
		case 12: // Hebrew
		    mib = 85; break;
		case 13: // Thai
		    mib = 2259; break;
		case 14: // Korean, no mapping
		default:
		    break;
	    }
	    if ( mib != -1 ) {
		mapper = QTextCodec::codecForMib( mib );
		chars[0] = (unsigned char) (key & 0xff );
		count++;
	    }
	}
	if ( count < (int)chars.size()-1 )
	    chars[count] = '\0';
	if ( count == 1 ) {
	    ascii = chars[0];
	    // +256 so we can store all eight-bit codes, including ascii 0,
	    // and independent of whether char is signed or not.
	    textDict->replace( keycode, (void*)(256+ascii) );
	}
	tlw = 0;
    } else {
	key = (int)(long)keyDict->find( keycode );
	if ( key )
	    keyDict->take( keycode );
	long s = (long)textDict->find( keycode );
	if ( s ) {
	    textDict->take( keycode );
	    ascii = (char)(s-256);
	}
    }
#endif // !NO_XIM

    state = translateButtonState( event->xkey.state );

    // Commentary in X11/keysymdef says that X codes match ASCII, so it
    // is safe to use the locale functions to process X codes in ISO8859-1.
    //
    // This is mainly for compatibility - applications should not use the
    // Qt keycodes between 128 and 255, but should rather use the
    // QKeyEvent::text().
    //
    if ( key < 128 || (key < 256 && (!input_mapper || input_mapper->mibEnum()==4)) ) {
	code = isprint((int)key) ? toupper((int)key) : 0; // upper-case key, if known
    } else if ( key >= XK_F1 && key <= XK_F35 ) {
	code = Key_F1 + ((int)key - XK_F1);	// function keys
    } else if ( key >= XK_KP_0 && key <= XK_KP_9) {
	code = Key_0 + ((int)key - XK_KP_0);	// numeric keypad keys
	state |= Keypad;
    } else {
	int i = 0;				// any other keys
	while ( KeyTbl[i] ) {
	    if ( key == KeyTbl[i] ) {
		code = (int)KeyTbl[i+1];
		break;
	    }
	    i += 2;
	}
	switch ( key ) {
	case XK_KP_Insert:
	case XK_KP_Delete:
	case XK_KP_Home:
	case XK_KP_End:
	case XK_KP_Left:
	case XK_KP_Up:
	case XK_KP_Right:
	case XK_KP_Down:
	case XK_KP_Prior:
	case XK_KP_Next:
	case XK_KP_Space:
	case XK_KP_Tab:
	case XK_KP_Enter:
	case XK_KP_Equal:
	case XK_KP_Multiply:
	case XK_KP_Add:
	case XK_KP_Separator:
	case XK_KP_Subtract:
	case XK_KP_Decimal:
	case XK_KP_Divide:
	    state |= Keypad;
	    break;
	default:
	    break;
	}
	if ( code == Key_Tab &&
	     (state & ShiftButton) == ShiftButton ) {
	    code = Key_Backtab;
	    chars[0] = 0;
	}
    }

#if 0
#ifndef Q_EE
    static int c  = 0;
    extern void qt_dialog_default_key();
#define Q_EE(x) c = (c == x || (!c && x == 0x1000) )? x+1 : 0
    if ( tlw && state == '0' ) {
	switch ( code ) {
	case 0x4f: Q_EE(Key_Backtab); break;
	case 0x52: Q_EE(Key_Tab); break;
	case 0x54: Q_EE(Key_Escape); break;
	case 0x4c:
	    if (c == Key_Return )
		qt_dialog_default_key();
	    else
		Q_EE(Key_Backspace);
	    break;
	}
    }
#undef Q_EE
#endif
#endif

    // convert chars (8bit) to text (unicode).
    if ( mapper )
	text = mapper->toUnicode(chars,count);
    else if ( !mapper && converted.unicode() != 0x0 )
	text = converted;
    else
	text = chars;
    return TRUE;
}


bool QETWidget::translateKeyEvent( const XEvent *event, bool grab )
{
    int	   code = -1;
    int	   count = 0;
    int	   state;
    char   ascii = 0;

    if ( sm_blockUserInput ) // block user interaction during session management
	return TRUE;

    Display *dpy = x11Display();

    if ( !isEnabled() )
	return TRUE;

    QEvent::Type type = (event->type == XKeyPress) ?
			QEvent::KeyPress : QEvent::KeyRelease;
    bool    autor = FALSE;
    QString text;

    translateKeyEventInternal( event, count, text, state, ascii, code );
    bool isAccel = FALSE;
    if ( !grab ) { // test for accel if the keyboard is not grabbed
	QKeyEvent a( QEvent::AccelAvailable, code, ascii, state, text, FALSE,
		     QMAX(count, int(text.length())) );
	a.ignore();
	QApplication::sendEvent( topLevelWidget(), &a );
	isAccel = a.isAccepted();
    }

    if ( !isAccel && !text.isEmpty() && testWState(WState_CompressKeys) ) {
	// the widget wants key compression so it gets it
	int	codeIntern = -1;
	int	countIntern = 0;
	int	stateIntern;
	char	asciiIntern = 0;
	XEvent	evRelease;
	XEvent	evPress;
	while (1) {
	    QString textIntern;
	    if ( !XCheckTypedWindowEvent(dpy,event->xkey.window,
					 XKeyRelease,&evRelease) )
		break;
	    if ( !XCheckTypedWindowEvent(dpy,event->xkey.window,
					 XKeyPress,&evPress) ) {
		XPutBackEvent(dpy, &evRelease);
		break;
	    }
	    translateKeyEventInternal( &evPress, countIntern, textIntern,
				       stateIntern, asciiIntern, codeIntern);
	    if ( stateIntern == state && !textIntern.isEmpty() ) {
		if (!grab) { // test for accel if the keyboard is not grabbed
		    QKeyEvent a( QEvent::AccelAvailable, codeIntern,
				 asciiIntern, stateIntern, textIntern, FALSE,
				 QMAX(countIntern, int(textIntern.length())) );
		    a.ignore();
		    QApplication::sendEvent( topLevelWidget(), &a );
		    if ( a.isAccepted() ) {
			XPutBackEvent(dpy, &evRelease);
			XPutBackEvent(dpy, &evPress);
			break;
		    }
		}
		text += textIntern;
		count += countIntern;
	    } else {
		XPutBackEvent(dpy, &evRelease);
		XPutBackEvent(dpy, &evPress);
		break;
	    }
	}
    }


    // was this the last auto-repeater?
    static uint curr_autorep = 0;
    if ( event->type == XKeyPress ) {
	if ( curr_autorep == event->xkey.keycode ) {
	    autor = TRUE;
	    curr_autorep = 0;
	}
    } else {
	// look ahead for auto-repeat
	XEvent nextpress;
	if ( XCheckTypedWindowEvent(dpy,event->xkey.window,
				    XKeyPress,&nextpress) ) {
	    autor = nextpress.xkey.time == event->xkey.time;
	    // Put it back... we COULD send the event now and not need
	    // the static curr_autorep variable.
	    XPutBackEvent(dpy,&nextpress);
	}
	curr_autorep = autor ? event->xkey.keycode : 0;
    }

    // autorepeat compression makes sense for all widgets (Windows
    // does it automatically .... )
    if ( event->type == XKeyPress && text.length() <= 1 ) {
	XEvent evPress = *event;
	XEvent evRelease;
	while (1) {
	    if (!XCheckTypedWindowEvent(dpy,event->xkey.window,XKeyRelease,
					&evRelease) )
		break;
	    if (evRelease.xkey.keycode != event->xkey.keycode ) {
		XPutBackEvent(dpy, &evRelease);
		break;
	    }
	    if (!XCheckTypedWindowEvent(dpy,event->xkey.window,XKeyPress,
					&evPress)) {
		XPutBackEvent(dpy, &evRelease);
		break;
	    }
	    if ( evPress.xkey.keycode != event->xkey.keycode ||
		 evRelease.xkey.time != evPress.xkey.time){
		XPutBackEvent(dpy, &evRelease);
		XPutBackEvent(dpy, &evPress);
		break;
	    }
	    count++;
	    if (!text.isEmpty())
		text += text[0];
	}
    }

    if (code == 0 && ascii == '\n') {
	code = Key_Return;
	ascii = '\r';
	text = "\r";
    }

    // process acceleraters before popups
    QKeyEvent e( type, code, ascii, state, text, autor,
		 QMAX(count, int(text.length())) );
    if ( type == QEvent::KeyPress && !grab ) {
	// send accel events if the keyboard is not grabbed
	QKeyEvent aa( QEvent::AccelOverride, code, ascii, state, text, autor,
		      QMAX(count, int(text.length())) );
	aa.ignore();
	QApplication::sendEvent( this, &aa );
	if ( !aa.isAccepted() ) {
	    QKeyEvent a( QEvent::Accel, code, ascii, state, text, autor,
			 QMAX(count, int(text.length())) );
	    a.ignore();
	    QApplication::sendEvent( topLevelWidget(), &a );
	    if ( a.isAccepted() )
		return TRUE;
	}
    }
    return QApplication::sendEvent( this, &e );
}


//
// Paint event translation
//
// When receiving many expose events, we compress them (union of all expose
// rectangles) into one event which is sent to the widget.

struct PaintEventInfo {
    Window window;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static Bool isPaintOrScrollDoneEvent( Display *, XEvent *ev, XPointer a )
{
    PaintEventInfo *info = (PaintEventInfo *)a;
    if ( ev->type == Expose || ev->type == GraphicsExpose
      ||    ev->type == ClientMessage
	 && ev->xclient.message_type == qt_qt_scrolldone )
    {
	if ( ev->xexpose.window == info->window )
	    return TRUE;
    }
    return FALSE;
}

#if defined(Q_C_CALLBACKS)
}
#endif


// declared above: static QList<QScrollInProgress> *sip_list = 0;

void qt_insert_sip( QWidget* scrolled_widget, int dx, int dy )
{
    if ( !sip_list ) {
	sip_list = new QList<QScrollInProgress>;
	sip_list->setAutoDelete( TRUE );
    }

    QScrollInProgress* sip = new QScrollInProgress( scrolled_widget, dx, dy );
    sip_list->append( sip );

    XClientMessageEvent client_message;
    client_message.type = ClientMessage;
    client_message.window = scrolled_widget->winId();
    client_message.format = 32;
    client_message.message_type = qt_qt_scrolldone;
    client_message.data.l[0] = sip->id;

    XSendEvent( appDpy, scrolled_widget->winId(), FALSE, NoEventMask,
	(XEvent*)&client_message );
}

int qt_sip_count( QWidget* scrolled_widget )
{
    if ( !sip_list )
	return 0;

    int sips=0;

    for (QScrollInProgress* sip = sip_list->first();
	sip; sip=sip_list->next())
    {
	if ( sip->scrolled_widget == scrolled_widget )
	    sips++;
    }

    return sips;
}

static
bool translateBySips( QWidget* that, QRect& paintRect )
{
    if ( sip_list ) {
	int dx=0, dy=0;
	int sips=0;
	for (QScrollInProgress* sip = sip_list->first();
	    sip; sip=sip_list->next())
	{
	    if ( sip->scrolled_widget == that ) {
		if ( sips ) {
		    dx += sip->dx;
		    dy += sip->dy;
		}
		sips++;
	    }
	}
	if ( sips > 1 ) {
	    paintRect.moveBy( dx, dy );
	    return TRUE;
	}
    }
    return FALSE;
}

bool QETWidget::translatePaintEvent( const XEvent *event )
{
    setWState( WState_Exposed );
    QRect  paintRect( event->xexpose.x,	   event->xexpose.y,
		      event->xexpose.width, event->xexpose.height );
    bool   merging_okay = !testWFlags(WPaintClever);
    XEvent xevent;
    PaintEventInfo info;
    info.window = winId();
    bool should_clip = translateBySips( this, paintRect );

    QRegion paintRegion( paintRect );

    if ( merging_okay ) {
	// WARNING: this is O(number_of_events * number_of_matching_events)
	while ( XCheckIfEvent(x11Display(),&xevent,isPaintOrScrollDoneEvent,
			      (XPointer)&info) &&
		!qt_x11EventFilter(&xevent)  &&
		!x11Event( &xevent ) ) // send event through filter
	{
	    if ( xevent.type == Expose || xevent.type == GraphicsExpose ) {
		QRect exposure(xevent.xexpose.x,
			       xevent.xexpose.y,
			       xevent.xexpose.width,
			       xevent.xexpose.height);
		if ( translateBySips( this, exposure ) )
		    should_clip = TRUE;
		paintRegion = paintRegion.unite( exposure );
	    } else {
		translateScrollDoneEvent( &xevent );
	    }
	}
    }

    if ( should_clip ) {
	paintRegion = paintRegion.intersect( rect() );
	if ( paintRegion.isEmpty() )
	    return TRUE;
    }

    QPaintEvent e( paintRegion );
    setWState( WState_InPaintEvent );
    if ( !isTopLevel() && backgroundOrigin() == ParentOrigin )
	erase( paintRegion );
    qt_set_paintevent_clipping( this, paintRegion );
    QApplication::sendEvent( this, &e );
    qt_clear_paintevent_clipping();
    clearWState( WState_InPaintEvent );
    return TRUE;
}

//
// Scroll-done event translation.
//

bool QETWidget::translateScrollDoneEvent( const XEvent *event )
{
    if ( !sip_list ) return FALSE;

    long id = event->xclient.data.l[0];

    // Remove any scroll-in-progress record for the given id.
    for (QScrollInProgress* sip = sip_list->first(); sip; sip=sip_list->next()) {
	if ( sip->id == id ) {
	    sip_list->remove( sip_list->current() );
	    return TRUE;
	}
    }

    return FALSE;
}


//
// ConfigureNotify (window move and resize) event translation

bool QETWidget::translateConfigEvent( const XEvent *event )
{
    // config pending is only set on resize, see qwidget_x11.cpp, internalSetGeometry()
    bool was_resize = testWState( WState_ConfigPending );

    clearWState(WState_ConfigPending);

    if ( isTopLevel() ) {
	QPoint newCPos( geometry().topLeft() );
	QSize  newSize( event->xconfigure.width, event->xconfigure.height );

	bool trust =  topData()->parentWinId == None ||  topData()->parentWinId == appRootWin;
	if (event->xconfigure.send_event || trust ) {
	    /* if a ConfigureNotify comes from a real sendevent request, we can
	       trust its values. */
	    newCPos.rx() = event->xconfigure.x + event->xconfigure.border_width;
	    newCPos.ry() = event->xconfigure.y + event->xconfigure.border_width;
	}

	if ( isVisible() )
	    QApplication::syncX();

	XEvent otherEvent;
	while ( XCheckTypedWindowEvent( x11Display(),winId(),ConfigureNotify,&otherEvent ) ) {
	    if ( qt_x11EventFilter( &otherEvent ) )
		continue;
	    if (x11Event( &otherEvent ) )
		continue;
	    if ( otherEvent.xconfigure.event != otherEvent.xconfigure.window )
		continue;
	    newSize.setWidth( otherEvent.xconfigure.width );
	    newSize.setHeight( otherEvent.xconfigure.height );
	    if ( otherEvent.xconfigure.send_event || trust ) {
		newCPos.rx() = otherEvent.xconfigure.x + otherEvent.xconfigure.border_width;
		newCPos.ry() = otherEvent.xconfigure.y + otherEvent.xconfigure.border_width;
	    }
	}

	QRect cr ( geometry() );
	if  (newSize != cr.size() ) { // size changed
	    was_resize = TRUE;
	    QSize oldSize = size();
	    cr.setSize( newSize );
	    setCRect( cr );
	    if ( isVisible() ) {
		QResizeEvent e( newSize, oldSize );
		QApplication::sendEvent( this, &e );
	    } else {
		QResizeEvent * e = new QResizeEvent( newSize, oldSize );
		QApplication::postEvent( this, e );
	    }
	}

	if ( newCPos != cr.topLeft() ) { // compare with cpos (exluding frame)
	    QPoint oldPos = pos();
	    cr.moveTopLeft( newCPos );
	    setCRect( cr );
	    if ( isVisible() ) {
		QMoveEvent e( pos(), oldPos ); // pos (including frame), not cpos
		QApplication::sendEvent( this, &e );
	    } else {
		QMoveEvent * e = new QMoveEvent( pos(), oldPos );
		QApplication::postEvent( this, e );
	    }
	}
    } else {
	XEvent xevent;
	while ( XCheckTypedWindowEvent(x11Display(),winId(), ConfigureNotify,&xevent) &&
		!qt_x11EventFilter(&xevent)  &&
		!x11Event( &xevent ) ) // send event through filter
	    ;
    }

    bool transbg = backgroundOrigin() == ParentOrigin;
    if ( transbg || (!testWFlags( WNorthWestGravity ) && testWState( WState_Exposed ) && was_resize) ) {
	// remove unnecessary paint events from the queue
	XEvent xevent;
	while ( XCheckTypedWindowEvent(x11Display(),winId(), Expose,&xevent) &&
		!qt_x11EventFilter(&xevent)  &&
		!x11Event( &xevent ) ) // send event through filter
	    ;
	repaint( visibleRect(), !testWFlags(WResizeNoErase) || transbg );
    }

    return TRUE;
}
#endif // __ATHEOS__


//
// Close window event translation.
//
#ifndef __ATHEOS__
bool QETWidget::translateCloseEvent( const XEvent * )
{
    return close(FALSE);
}
#endif

/*!
  Sets the text cursor's flash time to \a msecs milliseconds.  The
  flash time is the time required to display, invert and restore the
  caret display: A full flash cycle.  Usually, the text cursor is
  displayed for \a msecs/2 milliseconds, then hidden for \a msecs/2
  milliseconds, but this may vary.

  Note that on Microsoft Windows, calling this function sets the
  cursor flash time for all windows.

  \sa cursorFlashTime()
 */
void  QApplication::setCursorFlashTime( int msecs )
{
#ifndef __ATHEOS__
    cursor_flash_time = msecs;
#endif __ATHEOS__
}


/*!
  Returns the text cursor's flash time in milliseconds. The flash time
  is the time required to display, invert and restore the caret
  display.

  The default value on X11 is 1000 milliseconds. On Windows, the
  control panel value is used.

  Widgets should not cache this value since it may vary any time the
  user changes the global desktop settings.

  \sa setCursorFlashTime()
 */
int QApplication::cursorFlashTime()
{
#ifndef __ATHEOS__
    return cursor_flash_time;
#endif // __ATHEOS__
}

/*!
  Sets the time limit that distinguishes a double click from two
  consecutive mouse clicks to \a ms milliseconds.

  Note that on Microsoft Windows, calling this function sets the
  double click interval for all windows.

  \sa doubleClickInterval()
*/

void QApplication::setDoubleClickInterval( int ms )
{
#ifndef __ATHEOS__
    mouse_double_click_time = ms;
#endif
}


/*!
  Returns the maximum duration for a double click.

  The default value on X11 is 400 milliseconds. On Windows, the control
  panel value is used.

  \sa setDoubleClickInterval()
*/

int QApplication::doubleClickInterval()
{
#ifdef __ATHEOS__
    return 400;
#else
    return mouse_double_click_time;
#endif
}


/*!
  Sets the number of lines to scroll when the mouse wheel is
  rotated.

  If this number exceeds the number of visible lines in a certain
  widget, the widget should interpret the scroll operation as a single
  page up / page down operation instead.

  \sa wheelScrollLines()
 */
void QApplication::setWheelScrollLines( int n )
{
#ifndef __ATHEOS__
    wheel_scroll_lines = n;
#endif
}

/*!
  Returns the number of lines to scroll when the mouse wheel is rotated.

  \sa setWheelScrollLines()
 */
int QApplication::wheelScrollLines()
{
#ifdef __ATHEOS__
    return 1;
#else
    return wheel_scroll_lines;
#endif
}

/*!
  Enables the UI effect \a effect if \a enable is TRUE, otherwise
  the effect will not be used.

  \sa isEffectEnabled(), Qt::UIEffect, setDesktopSettingsAware()
*/
void QApplication::setEffectEnabled( Qt::UIEffect effect, bool enable )
{
#ifndef __ATHEOS__
    switch (effect) {
    case UI_AnimateMenu:
	animate_menu = enable;
	break;
    case UI_FadeMenu:
	if ( enable )
	    animate_menu = TRUE;
	fade_menu = enable;
	break;
    case UI_AnimateCombo:
	animate_combo = enable;
	break;
    case UI_AnimateTooltip:
	animate_tooltip = enable;
	break;
    case UI_FadeTooltip:
	if ( enable )
	    animate_tooltip = TRUE;
	fade_tooltip = enable;
	break;
    default:
	animate_ui = enable;
	break;
    }
#endif
}

/*!
  Returns TRUE if \a effect is enabled, otherwise FALSE.

  By default, Qt will try to use the desktop settings, and
  setDesktopSettingsAware() must be called to prevent this.

  sa\ setEffectEnabled(), Qt::UIEffect
*/
#ifndef __ATHEOS__
bool QApplication::isEffectEnabled( Qt::UIEffect effect )
{
    if ( !animate_ui )
	return FALSE;

    switch( effect ) {
    case UI_AnimateMenu:
	return animate_menu;
    case UI_FadeMenu:
	return fade_menu;
    case UI_AnimateCombo:
	return animate_combo;
    case UI_AnimateTooltip:
	return animate_tooltip;
    case UI_FadeTooltip:
	return fade_tooltip;
    default:
	return animate_ui;
    }
}
#endif // __ATHEOS__

/*****************************************************************************
  Session management support
 *****************************************************************************/

#ifdef QT_NO_SM_SUPPORT

#ifndef __ATHEOS__
class QSessionManagerData
{
public:
    QStringList restartCommand;
    QStringList discardCommand;
    QString sessionId;
    QSessionManager::RestartHint restartHint;
};

QSessionManager::QSessionManager( QApplication * app, QString &session )
    : QObject( app, "qt_sessionmanager" )
{
    d = new QSessionManagerData;
    d->sessionId = session;
    d->restartHint = RestartIfRunning;
}

QSessionManager::~QSessionManager()
{
    delete d;
}

QString QSessionManager::sessionId() const
{
    return d->sessionId;
}

void* QSessionManager::handle() const
{
    return 0;
}

bool QSessionManager::allowsInteraction()
{
    return TRUE;
}

bool QSessionManager::allowsErrorInteraction()
{
    return TRUE;
}

void QSessionManager::release()
{
}

void QSessionManager::cancel()
{
}

void QSessionManager::setRestartHint( QSessionManager::RestartHint hint)
{
    d->restartHint = hint;
}

QSessionManager::RestartHint QSessionManager::restartHint() const
{
    return d->restartHint;
}

void QSessionManager::setRestartCommand( const QStringList& command)
{
    d->restartCommand = command;
}

QStringList QSessionManager::restartCommand() const
{
    return d->restartCommand;
}

void QSessionManager::setDiscardCommand( const QStringList& command)
{
    d->discardCommand = command;
}

QStringList QSessionManager::discardCommand() const
{
    return d->discardCommand;
}

void QSessionManager::setProperty( const QString&, const QString&)
{
}

void QSessionManager::setProperty( const QString&, const QStringList& )
{
}

bool QSessionManager::isPhase2() const
{
    return FALSE;
}

void QSessionManager::requestPhase2()
{
}
#endif // __ATHEOS__

#else // QT_NO_SM_SUPPORT


#include <X11/SM/SMlib.h>

class QSmSocketReceiver : public QObject
{
    Q_OBJECT
public:
    QSmSocketReceiver( int socket )
	: QObject(0,0)
	{
	    QSocketNotifier* sn = new QSocketNotifier( socket, QSocketNotifier::Read, this );
	    connect( sn, SIGNAL( activated(int) ), this, SLOT( socketActivated(int) ) );
	}

public slots:
     void socketActivated(int);
};


static SmcConn smcConnection = 0;
static bool sm_interactionActive;
static bool sm_smActive;
static int sm_interactStyle;
static int sm_saveType;
static bool sm_cancel;
static bool sm_waitingForPhase2;
static bool sm_waitingForInteraction;
static bool sm_isshutdown;
static bool sm_shouldbefast;
static bool sm_phase2;
static bool sm_in_phase2;

static QSmSocketReceiver* sm_receiver = 0;

static void resetSmState();
static void sm_setProperty( const char* name, const char* type,
			    int num_vals, SmPropValue* vals);
static void sm_saveYourselfCallback( SmcConn smcConn, SmPointer clientData,
				  int saveType, Bool shutdown , int interactStyle, Bool fast);
static void sm_saveYourselfPhase2Callback( SmcConn smcConn, SmPointer clientData ) ;
static void sm_dieCallback( SmcConn smcConn, SmPointer clientData ) ;
static void sm_shutdownCancelledCallback( SmcConn smcConn, SmPointer clientData );
static void sm_saveCompleteCallback( SmcConn smcConn, SmPointer clientData );
static void sm_interactCallback( SmcConn smcConn, SmPointer clientData );
static void sm_performSaveYourself( QSessionManager* );

static void resetSmState()
{
    sm_waitingForPhase2 = FALSE;
    sm_waitingForInteraction = FALSE;
    sm_interactionActive = FALSE;
    sm_interactStyle = SmInteractStyleNone;
    sm_smActive = FALSE;
    sm_blockUserInput = FALSE;
    sm_isshutdown = FALSE;
    sm_shouldbefast = FALSE;
    sm_phase2 = FALSE;
    sm_in_phase2 = FALSE;
}


// theoretically it's possible to set several properties at once. For
// simplicity, however, we do just one property at a time
static void sm_setProperty( const char* name, const char* type,
			    int num_vals, SmPropValue* vals)
{
    if (num_vals ) {
      SmProp prop;
      prop.name = (char*)name;
      prop.type = (char*)type;
      prop.num_vals = num_vals;
      prop.vals = vals;

      SmProp* props[1];
      props[0] = &prop;
      SmcSetProperties( smcConnection, 1, props );
    }
    else {
      char* names[1];
      names[0] = (char*) name;
      SmcDeleteProperties( smcConnection, 1, names );
    }
}

static void sm_setProperty( const QString& name, const QString& value)
{
    SmPropValue prop;
    prop.length = value.length();
    prop.value = (SmPointer) value.latin1();
    sm_setProperty( name.latin1(), SmARRAY8, 1, &prop );
}

static void sm_setProperty( const QString& name, const QStringList& value)
{
    SmPropValue *prop = new SmPropValue[ value.count() ];
    int count = 0;
    for ( QStringList::ConstIterator it = value.begin(); it != value.end(); ++it ) {
      prop[ count ].length = (*it).length();
      prop[ count ].value = (char*)(*it).latin1();
      ++count;
    }
    sm_setProperty( name.latin1(), SmLISTofARRAY8, count, prop );
    delete [] prop;
}


// workaround for broken libsm, see below
struct QT_smcConn {
    unsigned int save_yourself_in_progress : 1;
    unsigned int shutdown_in_progress : 1;
};

static void sm_saveYourselfCallback( SmcConn smcConn, SmPointer clientData,
				  int saveType, Bool shutdown , int interactStyle, Bool fast)
{
    if (smcConn != smcConnection )
	return;
    sm_cancel = FALSE;
    sm_smActive = TRUE;
    sm_isshutdown = shutdown;
    sm_saveType = saveType;
    sm_interactStyle = interactStyle;
    sm_shouldbefast = fast;

    // ugly workaround for broken libSM. libSM should do that _before_
    // actually invoking the callback in sm_process.c
    ( (QT_smcConn*)smcConn )->save_yourself_in_progress = TRUE;
    if ( sm_isshutdown )
	( (QT_smcConn*)smcConn )->shutdown_in_progress = TRUE;

    sm_performSaveYourself( (QSessionManager*) clientData );
    if ( !sm_isshutdown ) // we cannot expect a confirmation message in that case
	resetSmState();
}

static void sm_performSaveYourself( QSessionManager* sm )
{
    if ( sm_isshutdown )
	sm_blockUserInput = TRUE;

    // tell the session manager about our program in best POSIX style
    sm_setProperty( SmProgram, QString( qApp->argv()[0] ) );
    // tell the session manager about our user as well.
    struct passwd* entry = getpwuid( geteuid() );
    if ( entry )
	sm_setProperty( SmUserID, QString::fromLatin1( entry->pw_name ) );

    // generate a restart and discard command that makes sense
    QStringList restart;
    restart  << qApp->argv()[0] << "-session" << sm->sessionId();
    sm->setRestartCommand( restart );
    QStringList discard;
    sm->setDiscardCommand( discard );

    switch ( sm_saveType ) {
    case SmSaveBoth:
	qApp->commitData( *sm );
	if ( sm_isshutdown && sm_cancel)
	    break; // we cancelled the shutdown, no need to save state
    // fall through
    case SmSaveLocal:
	qApp->saveState( *sm );
	break;
    case SmSaveGlobal:
	qApp->commitData( *sm );
	break;
    default:
	break;
    }

    if ( sm_phase2 && !sm_in_phase2 ) {
	SmcRequestSaveYourselfPhase2( smcConnection, sm_saveYourselfPhase2Callback, (SmPointer*) sm );
	sm_blockUserInput = FALSE;
    }
    else {
	// close eventual interaction monitors and cancel the
	// shutdown, if required.  Note that we can only cancel when
	// performing a shutdown, it does not work for checkpoints
	if ( sm_interactionActive ) {
	    SmcInteractDone( smcConnection, sm_isshutdown && sm_cancel);
	    sm_interactionActive = FALSE;
	}
	else if ( sm_cancel && sm_isshutdown ) {
	    if ( sm->allowsErrorInteraction() ) {
		SmcInteractDone( smcConnection, TRUE );
		sm_interactionActive = FALSE;
	    }
	}

	// set restart and discard command in session manager
	sm_setProperty( SmRestartCommand, sm->restartCommand() );
	sm_setProperty( SmDiscardCommand, sm->discardCommand() );

	// set the restart hint
	SmPropValue prop;
	prop.length = sizeof( int );
	int value = sm->restartHint();
	prop.value = (SmPointer) &value;
	sm_setProperty( SmRestartStyleHint, SmCARD8, 1, &prop );

	// we are done
	SmcSaveYourselfDone( smcConnection, !sm_cancel );
    }
}

static void sm_dieCallback( SmcConn smcConn, SmPointer /* clientData  */)
{
    if (smcConn != smcConnection )
	return;
    resetSmState();
    qApp->quit();
}

static void sm_shutdownCancelledCallback( SmcConn smcConn, SmPointer /* clientData */)
{
    if (smcConn != smcConnection )
	return;
    if ( sm_waitingForInteraction )
	qApp->exit_loop();
    resetSmState();
}

static void sm_saveCompleteCallback( SmcConn smcConn, SmPointer /*clientData */)
{
    if (smcConn != smcConnection )
	return;
    resetSmState();
}

static void sm_interactCallback( SmcConn smcConn, SmPointer /* clientData */ )
{
    if (smcConn != smcConnection )
	return;
    if ( sm_waitingForInteraction )
	qApp->exit_loop();
}

static void sm_saveYourselfPhase2Callback( SmcConn smcConn, SmPointer clientData )
{
    if (smcConn != smcConnection )
	return;
    sm_in_phase2 = TRUE;
    sm_performSaveYourself( (QSessionManager*) clientData );
}


void QSmSocketReceiver::socketActivated(int)
{
    IceProcessMessages( SmcGetIceConnection( smcConnection ), 0, 0);
}

#include "qapplication_x11.moc"

class QSessionManagerData
{
public:
    QStringList restartCommand;
    QStringList discardCommand;
    QString sessionId;
    QSessionManager::RestartHint restartHint;
};

QSessionManager::QSessionManager( QApplication * app, QString &session )
    : QObject( app, "session manager" )
{
    d = new QSessionManagerData;
    d->sessionId = session;
    d->restartHint = RestartIfRunning;

    resetSmState();
    char cerror[256];
    char* myId = 0;
    char* prevId = (char*)session.latin1(); // we know what we are doing

    SmcCallbacks cb;
    cb.save_yourself.callback = sm_saveYourselfCallback;
    cb.save_yourself.client_data = (SmPointer) this;
    cb.die.callback = sm_dieCallback;
    cb.die.client_data = (SmPointer) this;
    cb.save_complete.callback = sm_saveCompleteCallback;
    cb.save_complete.client_data = (SmPointer) this;
    cb.shutdown_cancelled.callback = sm_shutdownCancelledCallback;
    cb.shutdown_cancelled.client_data = (SmPointer) this;

    char* session_manager = getenv("SESSION_MANAGER");

    // avoid showing a warning message below
    if ( !session_manager || !session_manager[0] )
	return;

    smcConnection = SmcOpenConnection( 0, 0, 1, 0,
				       SmcSaveYourselfProcMask |
				       SmcDieProcMask |
				       SmcSaveCompleteProcMask |
				       SmcShutdownCancelledProcMask,
				       &cb,
				       prevId,
				       &myId,
				       255,
				       cerror );

    d->sessionId = QString::fromLatin1( myId );
    ::free( myId ); // it was allocated by C
    session = d->sessionId;

    QString error = cerror;
    if (!smcConnection ) {
	qWarning("Session management error: %s", error.latin1() );
    }
    else {
	sm_receiver = new QSmSocketReceiver(  IceConnectionNumber( SmcGetIceConnection( smcConnection ) ) );
    }
}

QSessionManager::~QSessionManager()
{
    if ( smcConnection )
      SmcCloseConnection( smcConnection, 0, 0 );
    smcConnection = 0;
    delete sm_receiver;
    delete d;
}

QString QSessionManager::sessionId() const
{
    return d->sessionId;
}

void* QSessionManager::handle() const
{
    return (void*) smcConnection;
}


bool QSessionManager::allowsInteraction()
{
    if ( sm_interactionActive )
	return TRUE;

    if ( sm_waitingForInteraction )
	return FALSE;

    if ( sm_interactStyle == SmInteractStyleAny ) {
	sm_waitingForInteraction =  SmcInteractRequest( smcConnection, SmDialogNormal,
							sm_interactCallback, (SmPointer*) this );
    }
    if ( sm_waitingForInteraction ) {
	qApp->enter_loop();
	sm_waitingForInteraction = FALSE;
	if ( sm_smActive ) { // not cancelled
	    sm_interactionActive = TRUE;
	    sm_blockUserInput = FALSE;
	    return TRUE;
	}
    }
    return FALSE;
}

bool QSessionManager::allowsErrorInteraction()
{
    if ( sm_interactionActive )
	return TRUE;

    if ( sm_waitingForInteraction )
	return FALSE;

    if ( sm_interactStyle == SmInteractStyleAny || sm_interactStyle == SmInteractStyleErrors ) {
	sm_waitingForInteraction =  SmcInteractRequest( smcConnection, SmDialogError,
							sm_interactCallback, (SmPointer*) this );
    }
    if ( sm_waitingForInteraction ) {
	qApp->enter_loop();
	sm_waitingForInteraction = FALSE;
	if ( sm_smActive ) { // not cancelled
	    sm_interactionActive = TRUE;
	    sm_blockUserInput = FALSE;
	    return TRUE;
	}
    }
    return FALSE;
}

void QSessionManager::release()
{
    if ( sm_interactionActive ) {
	SmcInteractDone( smcConnection, FALSE );
	sm_interactionActive = FALSE;
	if ( sm_smActive && sm_isshutdown )
	    sm_blockUserInput = TRUE;
    }
}

void QSessionManager::cancel()
{
    sm_cancel = TRUE;
}

void QSessionManager::setRestartHint( QSessionManager::RestartHint hint)
{
    d->restartHint = hint;
}

QSessionManager::RestartHint QSessionManager::restartHint() const
{
    return d->restartHint;
}

void QSessionManager::setRestartCommand( const QStringList& command)
{
    d->restartCommand = command;
}

QStringList QSessionManager::restartCommand() const
{
    return d->restartCommand;
}

void QSessionManager::setDiscardCommand( const QStringList& command)
{
    d->discardCommand = command;
}

QStringList QSessionManager::discardCommand() const
{
    return d->discardCommand;
}

void QSessionManager::setProperty( const QString& name, const QString& value)
{
    SmPropValue prop;
    prop.length = value.length();
    prop.value = (SmPointer) value.utf8().data();
    sm_setProperty( name.latin1(), SmARRAY8, 1, &prop );
}

void QSessionManager::setProperty( const QString& name, const QStringList& value)
{
    SmPropValue *prop = new SmPropValue[ value.count() ];
    int count = 0;
    for ( QStringList::ConstIterator it = value.begin(); it != value.end(); ++it ) {
      prop[ count ].length = (*it).length();
      prop[ count ].value = (char*)(*it).utf8().data();
      ++count;
    }
    sm_setProperty( name.latin1(), SmLISTofARRAY8, count, prop );
    delete [] prop;
}

bool QSessionManager::isPhase2() const
{
    return sm_in_phase2;
}

void QSessionManager::requestPhase2()
{
    sm_phase2 = TRUE;
}


#endif // QT_NO_SM_SUPPORT
