/****************************************************************************
** $Id$
**
** Implementation of QPainter class for X11
**
** Created : 940112
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

#include "qpainter.h"
#include "qwidget.h"
#include "qbitmap.h"
#include "qpixmapcache.h"
#include "qlist.h"
#include "qintdict.h"
#include "qfontdata_p.h"
#include "qtextcodec.h"
#include <ctype.h>
#include <stdlib.h>
#include "qt_x11.h"
#include "qpaintdevicemetrics.h"

#include <netinet/in.h>

// REVISED: arnt

/* paintevent magic to provide Windows semantics on X11
 */
static QRegion* paintEventClipRegion = 0;
static QPaintDevice* paintEventDevice = 0;

void qt_set_paintevent_clipping( QPaintDevice* dev, const QRegion& region)
{
    if ( !paintEventClipRegion )
	paintEventClipRegion = new QRegion( region );
    else
	*paintEventClipRegion = region;
    paintEventDevice = dev;
}

void qt_clear_paintevent_clipping()
{
    delete paintEventClipRegion;
    paintEventClipRegion = 0;
    paintEventDevice = 0;
}

void qt_erase_region( QWidget* w, const QRegion& region)
{
    QRegion reg = region;

    if ( !w->paintingActive() && !w->isTopLevel() && w->backgroundPixmap()
	 && w->backgroundOrigin() == QWidget::ParentOrigin ) {
	QPainter p( w );
	p.setClipRegion( region ); // automatically includes paintEventDevice if required
	p.drawTiledPixmap( 0, 0, w->width(), w->height(),
			   *w->backgroundPixmap(),
			   w->x(), w->y() );
	return;
    }

    if ( w == paintEventDevice )
	reg = paintEventClipRegion->intersect( reg );

    QArray<QRect> r = reg.rects();
    for (uint i=0; i<r.size(); i++) {
	const QRect& rr = r[(int)i];
	XClearArea( w->x11Display(), w->winId(),
		    rr.x(), rr.y(), rr.width(), rr.height(), FALSE );
    }
}

void qt_erase_rect( QWidget* w, const QRect& r)
{
    if ( w == paintEventDevice || w->backgroundOrigin() == QWidget::ParentOrigin )
	qt_erase_region( w, r );
    else
	XClearArea( w->x11Display(), w->winId(), r.x(), r.y(), r.width(), r.height(), FALSE );

}




/*****************************************************************************
  Trigonometric function for QPainter

  We have implemented simple sine and cosine function that are called from
  QPainter::drawPie() and QPainter::drawChord() when drawing the outline of
  pies and chords.
  These functions are slower and less accurate than math.h sin() and cos(),
  but with still around 1/70000th sec. execution time (on a 486DX2-66) and
  8 digits accuracy, it should not be the bottleneck in drawing these shapes.
  The advantage is that you don't have to link in the math library.
 *****************************************************************************/

const double Q_PI   = 3.14159265358979323846;	// pi
const double Q_2PI  = 6.28318530717958647693;	// 2*pi
const double Q_PI2  = 1.57079632679489661923;	// pi/2


#if defined(_CC_GNU_) && defined(_OS_AIX_)
// AIX 4.2 gcc 2.7.2.3 gets internal error.
static int qRoundAIX( double d )
{
    return qRound(d);
}
#define qRound qRoundAIX
#endif


#if defined(_CC_GNU_) && defined(__i386__)

inline double qcos( double a )
{
    double r;
    __asm__ (
	"fcos"
	: "=t" (r) : "0" (a) );
    return(r);
}

inline double qsin( double a )
{
    double r;
    __asm__ (
	"fsin"
	: "=t" (r) : "0" (a) );
    return(r);
}

double qsincos( double a, bool calcCos=FALSE )
{
    return calcCos ? qcos(a) : qsin(a);
}

#else

double qsincos( double a, bool calcCos=FALSE )
{
    if ( calcCos )				// calculate cosine
	a -= Q_PI2;
    if ( a >= Q_2PI || a <= -Q_2PI ) {		// fix range: -2*pi < a < 2*pi
	int m = (int)(a/Q_2PI);
	a -= Q_2PI*m;
    }
    if ( a < 0.0 )				// 0 <= a < 2*pi
	a += Q_2PI;
    int sign = a > Q_PI ? -1 : 1;
    if ( a >= Q_PI )
	a = Q_2PI - a;
    if ( a >= Q_PI2 )
	a = Q_PI - a;
    if ( calcCos )
	sign = -sign;
    double a2  = a*a;				// here: 0 <= a < pi/4
    double a3  = a2*a;				// make taylor sin sum
    double a5  = a3*a2;
    double a7  = a5*a2;
    double a9  = a7*a2;
    double a11 = a9*a2;
    return (a-a3/6+a5/120-a7/5040+a9/362880-a11/39916800)*sign;
}

inline double qsin( double a ) { return qsincos(a,FALSE); }
inline double qcos( double a ) { return qsincos(a,TRUE); }

#endif


/*****************************************************************************
  QPainter internal GC (Graphics Context) allocator.

  The GC allocator offers two functions; alloc_gc() and free_gc() that
  reuse GC objects instead of calling XCreateGC() and XFreeGC(), which
  are a whole lot slower.
 *****************************************************************************/

struct QGC
{
    GC	 gc;
    char in_use;
    bool mono;
};

const  int  gc_array_size = 256;
static QGC  gc_array[gc_array_size];		// array of GCs
static bool gc_array_init = FALSE;


static void init_gc_array()
{
    if ( !gc_array_init ) {
	memset( gc_array, 0, gc_array_size*sizeof(QGC) );
	gc_array_init = TRUE;
    }
}

static void cleanup_gc_array( Display *dpy )
{
    register QGC *p = gc_array;
    int i = gc_array_size;
    if ( gc_array_init ) {
	while ( i-- ) {
	    if ( p->gc )			// destroy GC
		XFreeGC( dpy, p->gc );
	    p++;
	}
	gc_array_init = FALSE;
    }
}

// #define DONT_USE_GC_ARRAY

static GC alloc_gc( Display *dpy, Drawable hd, bool monochrome=FALSE,
		    bool privateGC = FALSE )
{
#if defined(DONT_USE_GC_ARRAY)
    privateGC = TRUE;				// will be slower
#endif
    if ( privateGC ) {
	GC gc = XCreateGC( dpy, hd, 0, 0 );
	XSetGraphicsExposures( dpy, gc, FALSE );
	return gc;
    }
    register QGC *p = gc_array;
    int i = gc_array_size;
    if ( !gc_array_init )			// not initialized
	init_gc_array();
    while ( i-- ) {
	if ( !p->gc ) {				// create GC (once)
	    p->gc = XCreateGC( dpy, hd, 0, 0 );
	    XSetGraphicsExposures( dpy, p->gc, FALSE );
	    p->in_use = FALSE;
	    p->mono   = monochrome;
	}
	if ( !p->in_use && (bool)p->mono == monochrome ) {
	    p->in_use = TRUE;			// available/compatible GC
	    return p->gc;
	}
	p++;
    }
#if defined(CHECK_NULL)
    qWarning( "QPainter: Internal error; no available GC" );
#endif
    GC gc = XCreateGC( dpy, hd, 0, 0 );
    XSetGraphicsExposures( dpy, gc, FALSE );
    return gc;
}

static void free_gc( Display *dpy, GC gc, bool privateGC = FALSE )
{
#if defined(DONT_USE_GC_ARRAY)
    privateGC = TRUE;				// will be slower
#endif
    if ( privateGC ) {
	ASSERT( dpy != 0 );
	XFreeGC( dpy, gc );
	return;
    }
    register QGC *p = gc_array;
    int i = gc_array_size;
    if ( gc_array_init ) {
	while ( i-- ) {
	    if ( p->gc == gc ) {
		p->in_use = FALSE;		// set available
		XSetClipMask( dpy, gc, None );	// make it reusable
		XSetFunction( dpy, gc, GXcopy );
		XSetFillStyle( dpy, gc, FillSolid );
		XSetTSOrigin( dpy, gc, 0, 0 );
		return;
	    }
	    p++;
	}
    }
}


/*****************************************************************************
  QPainter internal GC (Graphics Context) cache for solid pens and
  brushes.

  The GC cache makes a significant contribution to speeding up
  drawing.  Setting new pen and brush colors will make the painter
  look for another GC with the same color instead of changing the
  color value of the GC currently in use. The cache structure is
  optimized for fast lookup.  Only solid line pens with line width 0
  and solid brushes are cached.

  In addition, stored GCs may have an implicit clipping region
  set. This prevents any drawing outside paint events. Both
  updatePen() and updateBrush() keep track of the validity of this
  clipping region by storing the clip_serial number in the cache.

*****************************************************************************/

struct QGCC					// cached GC
{
    GC gc;
    uint pix;
    int count;
    int hits;
    int clip_serial;
};

const  int   gc_cache_size = 29;		// multiply by 4
static QGCC *gc_cache_buf;
static QGCC *gc_cache[4*gc_cache_size];
static bool  gc_cache_init = FALSE;
static int gc_cache_clip_serial = 0;


static void init_gc_cache()
{
    if ( !gc_cache_init ) {
	gc_cache_init = TRUE;
	QGCC *g = gc_cache_buf = new QGCC[4*gc_cache_size];
	memset( g, 0, 4*gc_cache_size*sizeof(QGCC) );
	for ( int i=0; i<4*gc_cache_size; i++ )
	    gc_cache[i] = g++;
    }
}


// #define GC_CACHE_STAT
#if defined(GC_CACHE_STAT)
#include "qtextstream.h"
#include "qbuffer.h"

static int g_numhits	= 0;
static int g_numcreates = 0;
static int g_numfaults	= 0;
#endif


static void cleanup_gc_cache()
{
    if ( !gc_cache_init )
	return;
#if defined(GC_CACHE_STAT)
    qDebug( "Number of cache hits = %d", g_numhits );
    qDebug( "Number of cache creates = %d", g_numcreates );
    qDebug( "Number of cache faults = %d", g_numfaults );
    for ( int i=0; i<gc_cache_size; i++ ) {
	QString	    str;
	QBuffer	    buf( str );
	buf.open(IO_ReadWrite);
	QTextStream s(&buf);
	s << i << ": ";
	for ( int j=0; j<4; j++ ) {
	    QGCC *g = gc_cache[i*4+j];
	    s << (g->gc ? 'X' : '-') << ',' << g->hits << ','
	      << g->count << '\t';
	}
	s << '\0';
	qDebug( str );
	buf.close();
    }
#endif
    delete [] gc_cache_buf;
    gc_cache_init = FALSE;
}


static bool obtain_gc( void **ref, GC *gc, uint pix, Display *dpy, HANDLE hd )
{
    if ( !gc_cache_init )
	init_gc_cache();

    int	  k = (pix % gc_cache_size) * 4;
    QGCC *g = gc_cache[k];
    QGCC *prev = 0;

#define NOMATCH (g->gc && g->pix != pix)

    if ( NOMATCH ) {
	prev = g;
	g = gc_cache[++k];
	if ( NOMATCH ) {
	    prev = g;
	    g = gc_cache[++k];
	    if ( NOMATCH ) {
		prev = g;
		g = gc_cache[++k];
		if ( NOMATCH ) {
		    if ( g->count == 0 ) {	// steal this GC
			g->pix	 = pix;
			g->count = 1;
			g->hits	 = 1;
			XSetForeground( dpy, g->gc, pix );
			gc_cache[k]   = prev;
			gc_cache[k-1] = g;
			*ref = (void *)g;
			*gc = g->gc;
			return TRUE;
		    } else {			// all GCs in use
#if defined(GC_CACHE_STAT)
			g_numfaults++;
#endif
			*ref = 0;
			return FALSE;
		    }
		}
	    }
	}
    }

#undef NOMATCH

    *ref = (void *)g;

    if ( g->gc ) {				// reuse existing GC
#if defined(GC_CACHE_STAT)
	g_numhits++;
#endif
	*gc = g->gc;
	g->count++;
	g->hits++;
	if ( prev && g->hits > prev->hits ) {	// maintain LRU order
	    gc_cache[k]   = prev;
	    gc_cache[k-1] = g;
	}
	return TRUE;
    } else {					// create new GC
#if defined(GC_CACHE_STAT)
	g_numcreates++;
#endif
	g->gc	 = alloc_gc( dpy, hd, FALSE );
	g->pix	 = pix;
	g->count = 1;
	g->hits	 = 1;
	g->clip_serial = 0;
	*gc = g->gc;
	return FALSE;
    }
}

static inline void release_gc( void *ref )
{
    ((QGCC*)ref)->count--;
}



#ifdef QT_XFT
int qt_use_xft (void)
{
    static int	checked_env=0;
    static int	use_xft=0;

    if (!checked_env) {
	if (XftDefaultHasRender(qt_xdisplay())) {
	    char *e = getenv ("QT_XFT");
	    if ( e && (*e == '1' ||
		       *e == 'y' || *e == 'Y' ||
		       *e == 't' || *e == 'T' ))
		use_xft = 1;
	    else
		use_xft = 0;
	}
	checked_env = 1;
    }
    return use_xft;
}

typedef struct _qt_ft_draw {
    struct _qt_ft_draw	*next;
    Drawable		draw;
    Display		*dpy;
    Visual		*visual;
    Colormap		cmap;
    XftDraw		*ftdraw;
    int			clip_serial;
    bool		clip_bad;
} qt_ft_draw;

static qt_ft_draw   *qt_ft_head;

XftDraw *
qt_lookup_ft_draw (Drawable draw, QPaintDevice *pdev, QRegion *crgn)
{
    qt_ft_draw	**prev, *qt_ft;

    for (prev = &qt_ft_head; (qt_ft = *prev); prev = &qt_ft->next)
	if (qt_ft->draw == draw)
	{
	    if (!qt_ft->ftdraw)
	    {
		if (qt_ft->visual)
		    qt_ft->ftdraw = XftDrawCreate (qt_ft->dpy,
						   qt_ft->draw,
						   qt_ft->visual,
						   qt_ft->cmap);
		else
		    qt_ft->ftdraw = XftDrawCreateBitmap (qt_ft->dpy,
							 (Pixmap) qt_ft->draw);
		if (!qt_ft->ftdraw)
		{
		    *prev = qt_ft->next;
		    free (qt_ft);
		    return 0;
		}
	    }

	    if (prev != &qt_ft_head)
	    {
		*prev = qt_ft->next;
		qt_ft->next = qt_ft_head;
		qt_ft_head = qt_ft;
	    }
	    if (crgn != 0)
	    {
		if (pdev == paintEventDevice)
		{
		    QRegion rgn;
		    if (qt_ft->clip_serial < gc_cache_clip_serial ||
			qt_ft->clip_bad)
		    {
			rgn = crgn->intersect (*paintEventClipRegion);
			XftDrawSetClip (qt_ft->ftdraw, rgn.handle());
			qt_ft->clip_serial = gc_cache_clip_serial;
			qt_ft->clip_bad = FALSE;
		    }
		}
		else
		{
		    if (qt_ft->clip_serial || qt_ft->clip_bad)
		    {
			XftDrawSetClip (qt_ft->ftdraw, crgn->handle());
			qt_ft->clip_serial = 0;
			qt_ft->clip_bad = FALSE;
		    }
		}
	    }
	    else
	    {
		if (pdev == paintEventDevice)
		{
		    if (qt_ft->clip_serial < gc_cache_clip_serial ||
			qt_ft->clip_bad)
		    {
			XftDrawSetClip (qt_ft->ftdraw,
					paintEventClipRegion->handle());
			qt_ft->clip_serial = gc_cache_clip_serial;
			qt_ft->clip_bad = FALSE;
		    }
		}
		else
		{
		    if (qt_ft->clip_serial || qt_ft->clip_bad)
		    {
			XftDrawSetClip (qt_ft->ftdraw, 0);
			qt_ft->clip_serial = 0;
			qt_ft->clip_bad = FALSE;
		    }
		}
	    }
	    return qt_ft->ftdraw;
	}
    return 0;
}

int
qt_create_ft_draw (Display *dpy, Drawable draw, Visual *visual, Colormap cmap)
{
    qt_ft_draw	*qt_ft;

    if (!qt_use_xft ())
	return 1;
    qt_ft = (qt_ft_draw *) malloc (sizeof (qt_ft_draw));
    if (!qt_ft)
	return 0;
    qt_ft->next = qt_ft_head;
    qt_ft_head = qt_ft;

    qt_ft->draw = draw;
    qt_ft->ftdraw = 0;
    qt_ft->dpy = dpy;
    qt_ft->visual = visual;
    qt_ft->cmap = cmap;
    qt_ft->clip_serial = 0;
    qt_ft->clip_bad = FALSE;

    return 1;
}

void
qt_destroy_ft_draw (Display */*dpy*/, Drawable draw)
{
    qt_ft_draw	**prev, *qt_ft;

    for (prev = &qt_ft_head; (qt_ft = *prev); prev = &qt_ft->next)
	if (qt_ft->draw == draw)
	{
	    *prev = qt_ft->next;
	    if (qt_ft->ftdraw)
		XftDrawDestroy (qt_ft->ftdraw);
	    free (qt_ft);
	    break;
	}
}

void
qt_invalidate_ft_clip (Drawable draw)
{
    qt_ft_draw	*qt_ft;

    for (qt_ft = qt_ft_head; qt_ft; qt_ft = qt_ft->next)
	if (qt_ft->draw == draw)
	    qt_ft->clip_bad = TRUE;
}
#endif

/*****************************************************************************
  QPainter member functions
 *****************************************************************************/

const int TxNone      = 0;			// transformation codes
const int TxTranslate = 1;			// also in qpainter.cpp
const int TxScale     = 2;
const int TxRotShear  = 3;


/*!
  Internal function that initializes the painter.
*/

void QPainter::initialize()
{
    init_gc_array();
    init_gc_cache();
}

/*!
  Internal function that cleans up the painter.
*/

void QPainter::cleanup()
{
    cleanup_gc_cache();
    cleanup_gc_array( qt_xdisplay() );
    QPointArray::cleanBuffers();
}


typedef QIntDict<QPaintDevice> QPaintDeviceDict;
static QPaintDeviceDict *pdev_dict = 0;

/*!
  Redirects all paint command for a paint device \a pdev to another
  paint device \a replacement, unless \a replacement is 0.  If \a
  replacement is 0, the redirection for \a pdev is removed.

  Mostly, you can get better results with less work by calling
  QPixmap::grabWidget() or QPixmap::grapWindow().
*/

void QPainter::redirect( QPaintDevice *pdev, QPaintDevice *replacement )
{
    if ( pdev_dict == 0 ) {
	if ( replacement == 0 )
	    return;
	pdev_dict = new QPaintDeviceDict;
	CHECK_PTR( pdev_dict );
    }
#if defined(CHECK_NULL)
    if ( pdev == 0 )
	qWarning( "QPainter::redirect: The pdev argument cannot be 0" );
#endif
    if ( replacement ) {
	pdev_dict->insert( (long)pdev, replacement );
    } else {
	pdev_dict->remove( (long)pdev );
	if ( pdev_dict->count() == 0 ) {
	    delete pdev_dict;
	    pdev_dict = 0;
	}
    }
}


void QPainter::init()
{
    flags = IsStartingUp;
    bg_col = white;				// default background color
    bg_mode = TransparentMode;			// default background mode
    rop = CopyROP;				// default ROP
    tabstops = 0;				// default tabbing
    tabarray = 0;
    tabarraylen = 0;
    ps_stack = 0;
    wm_stack = 0;
    gc = gc_brush = 0;
    pdev = 0;
    dpy  = 0;
    txop = txinv = 0;
    penRef = brushRef = 0;
}


/*!
  \fn const QFont &QPainter::font() const

  Returns the currently set painter font.
  \sa setFont(), QFont
*/

/*!
  Sets a new painter font.

  This font is used by subsequent drawText() functions.  The text
  color is the same as the pen color.

  \sa font(), drawText()
*/

void QPainter::setFont( const QFont &font )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setFont: Will be reset by begin()" );
#endif
    if ( cfont.d != font.d ) {
	cfont = font;
	setf(DirtyFont);
    }
}


void QPainter::updateFont()
{
    clearf(DirtyFont);
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].font = &cfont;
	if ( !pdev->cmd( QPaintDevice::PdcSetFont, this, param ) || !hd )
	    return;
    }
    setf(NoCache);
    if ( penRef )
	updatePen();				// force a non-cached GC
    HANDLE h = cfont.handle();
    if ( h && h != 1 ) {
	XSetFont( dpy, gc, cfont.handle() );
    }
}


void QPainter::updatePen()
{
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].pen = &cpen;
	if ( !pdev->cmd( QPaintDevice::PdcSetPen, this, param ) || !hd )
	    return;
    }

    int	ps = cpen.style();
    bool cacheIt = !testf(ClipOn|MonoDev|NoCache) &&
		   (ps == NoPen || ps == SolidLine) &&
		   cpen.width() == 0 && rop == CopyROP;

    bool obtained = FALSE;
    bool internclipok = hasClipping();
    if ( cacheIt ) {
	if ( gc ) {
	    if ( penRef )
		release_gc( penRef );
	    else
		free_gc( dpy, gc );
	}
	obtained = obtain_gc(&penRef, &gc, cpen.color().pixel(), dpy, hd);
	if ( !obtained && !penRef )
	    gc = alloc_gc( dpy, hd, FALSE );
    } else {
	if ( gc ) {
	    if ( penRef ) {
		release_gc( penRef );
		penRef = 0;
		gc = alloc_gc( dpy, hd, testf(MonoDev) );
	    } else {
		internclipok = TRUE;
	    }
	} else {
	    gc = alloc_gc( dpy, hd, testf(MonoDev), testf(UsePrivateCx) );
	}
    }

    if ( !internclipok ) {
	if ( pdev == paintEventDevice ) {
	    if ( penRef &&((QGCC*)penRef)->clip_serial < gc_cache_clip_serial ) {
		XSetRegion( dpy, gc, paintEventClipRegion->handle() );
		((QGCC*)penRef)->clip_serial = gc_cache_clip_serial;
	    } else if ( !penRef ) {
		XSetRegion( dpy, gc, paintEventClipRegion->handle() );
	    }
	} else if (penRef && ((QGCC*)penRef)->clip_serial ) {
	    XSetClipMask( dpy, gc, None );
	    ((QGCC*)penRef)->clip_serial = 0;
	}
    }

    if ( obtained )
	return;

    char dashes[10];				// custom pen dashes
    int dash_len = 0;				// length of dash list
    int s = LineSolid;
    int cp = CapButt;
    int jn = JoinMiter;

    /*
      We are emulating Windows here.  Windows treats cpen.width() == 1
      as a very special case.  The fudge variable unifies this case
      with the general case.
    */
    int dot = cpen.width();			// width of a dot
    int fudge = 1;
    if ( dot <= 1 ) {
	dot = 3;
	fudge = 2;
    }

    switch( ps ) {
	case NoPen:
	case SolidLine:
	    s = LineSolid;
	    break;
	case DashLine:
	    dashes[0] = fudge * 3 * dot;
	    dashes[1] = fudge * dot;
	    dash_len = 2;
	    break;
	case DotLine:
	    dashes[0] = dot;
	    dashes[1] = dot;
	    dash_len = 2;
	    break;
	case DashDotLine:
	    dashes[0] = 3 * dot;
	    dashes[1] = fudge * dot;
	    dashes[2] = dot;
	    dashes[3] = fudge * dot;
	    dash_len = 4;
	    break;
	case DashDotDotLine:
	    dashes[0] = 3 * dot;
	    dashes[1] = dot;
	    dashes[2] = dot;
	    dashes[3] = dot;
	    dashes[4] = dot;
	    dashes[5] = dot;
	    dash_len = 6;
    }
    ASSERT( dash_len <= (int) sizeof(dashes) );

    switch ( cpen.capStyle() ) {
    case SquareCap:
	cp = CapProjecting;
	break;
    case RoundCap:
	cp = CapRound;
	break;
    case FlatCap:
    default:
	cp = CapButt;
	break;
    }
    switch ( cpen.joinStyle() ) {
    case BevelJoin:
	jn = JoinBevel;
	break;
    case RoundJoin:
	jn = JoinRound;
	break;
    case MiterJoin:
    default:
	jn = JoinMiter;
	break;
    }

    XSetForeground( dpy, gc, cpen.color().pixel() );
    XSetBackground( dpy, gc, bg_col.pixel() );

    if ( dash_len ) {				// make dash list
	XSetDashes( dpy, gc, 0, dashes, dash_len );
	s = bg_mode == TransparentMode ? LineOnOffDash : LineDoubleDash;
    }
    XSetLineAttributes( dpy, gc, cpen.width(), s, cp, jn );
}


void QPainter::updateBrush()
{
static uchar dense1_pat[] = { 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff };
static uchar dense2_pat[] = { 0x77, 0xff, 0xdd, 0xff, 0x77, 0xff, 0xdd, 0xff };
static uchar dense3_pat[] = { 0x55, 0xbb, 0x55, 0xee, 0x55, 0xbb, 0x55, 0xee };
static uchar dense4_pat[] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
static uchar dense5_pat[] = { 0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa, 0x11 };
static uchar dense6_pat[] = { 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 };
static uchar dense7_pat[] = { 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00 };
static uchar hor_pat[] = {			// horizontal pattern
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uchar ver_pat[] = {			// vertical pattern
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20 };
static uchar cross_pat[] = {			// cross pattern
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20 };
static uchar bdiag_pat[] = {			// backward diagonal pattern
    0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04, 0x02, 0x02, 0x01, 0x01,
    0x80, 0x80, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04,
    0x02, 0x02, 0x01, 0x01, 0x80, 0x80, 0x40, 0x40 };
static uchar fdiag_pat[] = {			// forward diagonal pattern
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x40,
    0x80, 0x80, 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10,
    0x20, 0x20, 0x40, 0x40, 0x80, 0x80, 0x01, 0x01 };
static uchar dcross_pat[] = {			// diagonal cross pattern
    0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x41,
    0x80, 0x80, 0x41, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14,
    0x22, 0x22, 0x41, 0x41, 0x80, 0x80, 0x41, 0x41 };
static uchar *pat_tbl[] = {
    dense1_pat, dense2_pat, dense3_pat, dense4_pat, dense5_pat,
    dense6_pat, dense7_pat,
    hor_pat, ver_pat, cross_pat, bdiag_pat, fdiag_pat, dcross_pat };

    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].brush = &cbrush;
	if ( !pdev->cmd( QPaintDevice::PdcSetBrush, this, param ) || !hd )
	    return;
    }

    int	 bs	 = cbrush.style();
    bool cacheIt = !testf(ClipOn|MonoDev|NoCache) &&
		   (bs == NoBrush || bs == SolidPattern) &&
		   bro.x() == 0 && bro.y() == 0 && rop == CopyROP;

    bool obtained = FALSE;
    bool internclipok = hasClipping();
    if ( cacheIt ) {
	if ( gc_brush ) {
	    if ( brushRef )
		release_gc( brushRef );
	    else
		free_gc( dpy, gc_brush );
	}
	obtained = obtain_gc(&brushRef, &gc_brush, cbrush.color().pixel(), dpy, hd);
	if ( !obtained && !brushRef )
	    gc_brush = alloc_gc( dpy, hd, FALSE );
    } else {
	if ( gc_brush ) {
	    if ( brushRef ) {
		release_gc( brushRef );
		brushRef = 0;
		gc_brush = alloc_gc( dpy, hd, testf(MonoDev) );
	    } else {
		internclipok = TRUE;
	    }
	} else {
	    gc_brush = alloc_gc( dpy, hd, testf(MonoDev), testf(UsePrivateCx));
	}
    }

    if ( !internclipok ) {
	if ( pdev == paintEventDevice ) {
	    if ( brushRef &&((QGCC*)brushRef)->clip_serial < gc_cache_clip_serial ) {
		XSetRegion( dpy, gc_brush, paintEventClipRegion->handle() );
		((QGCC*)brushRef)->clip_serial = gc_cache_clip_serial;
	    } else if ( !brushRef ){
		XSetRegion( dpy, gc_brush, paintEventClipRegion->handle() );
	    }
	} else if (brushRef && ((QGCC*)brushRef)->clip_serial ) {
	    XSetClipMask( dpy, gc_brush, None );
	    ((QGCC*)brushRef)->clip_serial = 0;
	}
    }

    if ( obtained )
	return;

    uchar *pat = 0;				// pattern
    int d = 0;					// defalt pattern size: d*d
    int s  = FillSolid;
    if ( bs >= Dense1Pattern && bs <= DiagCrossPattern ) {
	pat = pat_tbl[ bs-Dense1Pattern ];
	if ( bs <= Dense7Pattern )
	    d = 8;
	else if ( bs <= CrossPattern )
	    d = 24;
	else
	    d = 16;
    }

    XSetLineAttributes( dpy, gc_brush, 0, LineSolid, CapButt, JoinMiter );
    XSetForeground( dpy, gc_brush, cbrush.color().pixel() );
    XSetBackground( dpy, gc_brush, bg_col.pixel() );

    if ( bs == CustomPattern || pat ) {
	QPixmap *pm;
	if ( pat ) {
	    QString key;
	    key.sprintf( "$qt-brush$%d", bs );
	    pm = QPixmapCache::find( key );
	    bool del = FALSE;
	    if ( !pm ) {			// not already in pm dict
		pm = new QBitmap( d, d, pat, TRUE );
		CHECK_PTR( pm );
		del = !QPixmapCache::insert( key, pm );
	    }
	    if ( cbrush.data->pixmap )
		delete cbrush.data->pixmap;
	    cbrush.data->pixmap = new QPixmap( *pm );
	    if (del) delete pm;
	}
	pm = cbrush.data->pixmap;
	if ( pm->depth() == 1 ) {
	    XSetStipple( dpy, gc_brush, pm->handle() );
	    s = bg_mode == TransparentMode ? FillStippled : FillOpaqueStippled;
	} else {
	    XSetTile( dpy, gc_brush, pm->handle() );
	    s = FillTiled;
	}
    }
    XSetFillStyle( dpy, gc_brush, s );
}


/*!
  Begins painting the paint device \a pd and returns TRUE if successful,
  or FALSE if an error occurs.

  The errors that can occur are serious problems, such as these:

  \code
    p->begin( 0 ); // impossible - paint device cannot be 0

    QPixmap pm( 0, 0 );
    p->begin( pm ); // impossible - pm.isNull();

    p->begin( myWidget );
    p2->begin( myWidget ); // impossible - only one painter at a time
  \endcode

  Note that most of the time, you can use one of the constructors
  instead of begin(), and that end() is automatically done at
  destruction.

  \warning A paint device can only be painted by one painter at a time.

  \sa end(), flush()
*/

bool QPainter::begin( const QPaintDevice *pd )
{
    if ( isActive() ) {				// already active painting
#if defined(CHECK_STATE)
	qWarning( "QPainter::begin: Painter is already active."
		  "\n\tYou must end() the painter before a second begin()" );
#endif
	return FALSE;
    }
    if ( pd == 0 ) {
#if defined(CHECK_NULL)
	qWarning( "QPainter::begin: Paint device cannot be null" );
#endif
	return FALSE;
    }

    const QWidget *copyFrom = 0;
    if ( pdev_dict ) {				// redirected paint device?
	pdev = pdev_dict->find( (long)pd );
	if ( pdev ) {
	    if ( pd->devType() == QInternal::Widget )
		copyFrom = (const QWidget *)pd;	// copy widget settings
	} else {
	    pdev = (QPaintDevice *)pd;
	}
    } else {
	pdev = (QPaintDevice *)pd;
    }

    if ( pdev->isExtDev() && pdev->paintingActive() ) {
	// somebody else is already painting
#if defined(CHECK_STATE)
	qWarning( "QPainter::begin: Another QPainter is already painting "
		  "this device;\n\tAn extended paint device can only be "
		  "painted by one QPainter at a time." );
#endif
	return FALSE;
    }

    bool reinit = flags != IsStartingUp;	// 2nd or 3rd etc. time called
    flags = IsActive | DirtyFont;		// init flags
    int dt = pdev->devType();			// get the device type

    if ( (pdev->devFlags & QInternal::ExternalDevice) != 0 )
	setf(ExtDev);
    else if ( dt == QInternal::Pixmap )		// device is a pixmap
	((QPixmap*)pdev)->detach();		// will modify it

    dpy = pdev->x11Display();			// get display variable
    hd	= pdev->handle();			// get handle to drawable

    if ( testf(ExtDev) ) {			// external device
	if ( !pdev->cmd( QPaintDevice::PdcBegin, this, 0 ) ) {
	    // could not begin painting
	    if ( reinit )
		clearf( IsActive | DirtyFont );
	    else
		flags = IsStartingUp;
	    pdev = 0;
	    return FALSE;
	}
	if ( tabstops )				// update tabstops for device
	    setTabStops( tabstops );
	if ( tabarray )				// update tabarray for device
	    setTabArray( tabarray );
    }

    if ( pdev->x11Depth() != pdev->x11AppDepth() ) {	// non-standard depth
	setf(NoCache);
	setf(UsePrivateCx);
    }

    pdev->painters++;				// also tell paint device
    bro = curPt = QPoint( 0, 0 );
    if ( reinit ) {
	bg_mode = TransparentMode;		// default background mode
	rop = CopyROP;				// default ROP
	wxmat.reset();				// reset world xform matrix
	txop = txinv = 0;
	if ( dt != QInternal::Widget ) {
	    QFont  defaultFont;			// default drawing tools
	    QPen   defaultPen;
	    QBrush defaultBrush;
	    cfont  = defaultFont;		// set these drawing tools
	    cpen   = defaultPen;
	    cbrush = defaultBrush;
	    bg_col = white;			// default background color
	}
    }
    wx = wy = vx = vy = 0;			// default view origins

    if ( dt == QInternal::Widget ) {			// device is a widget
	QWidget *w = (QWidget*)pdev;
	cfont = w->font();			// use widget font
	cpen = QPen( w->foregroundColor() );	// use widget fg color
	if ( reinit ) {
	    QBrush defaultBrush;
	    cbrush = defaultBrush;
	}
	bg_col = w->backgroundColor();		// use widget bg color
	ww = vw = w->width();			// default view size
	wh = vh = w->height();
	if ( w->testWFlags(WPaintUnclipped) ) { // paint direct on device
	    setf( NoCache );
	    setf(UsePrivateCx);
	    updatePen();
	    updateBrush();
	    XSetSubwindowMode( dpy, gc, IncludeInferiors );
	    XSetSubwindowMode( dpy, gc_brush, IncludeInferiors );
	}
    } else if ( dt == QInternal::Pixmap ) {		// device is a pixmap
	QPixmap *pm = (QPixmap*)pdev;
	if ( pm->isNull() ) {
#if defined(CHECK_NULL)
	    qWarning( "QPainter::begin: Cannot paint null pixmap" );
#endif
	    end();
	    return FALSE;
	}
	bool mono = pm->depth() == 1;		// monochrome bitmap
	if ( mono ) {
	    setf( MonoDev );
	    bg_col = color0;
	    cpen.setColor( color1 );
	}
	ww = vw = pm->width();			// default view size
	wh = vh = pm->height();
    } else if ( testf(ExtDev) ) {		// external device
	ww = vw = pdev->metric( QPaintDeviceMetrics::PdmWidth );
	wh = vh = pdev->metric( QPaintDeviceMetrics::PdmHeight );
    }
    if ( ww == 0 )
	ww = wh = vw = vh = 1024;
    if ( copyFrom ) {				// copy redirected widget
	cfont = copyFrom->font();
	cpen = QPen( copyFrom->foregroundColor() );
	bg_col = copyFrom->backgroundColor();
    }
    if ( testf(ExtDev) ) {			// external device
	setBackgroundColor( bg_col );		// default background color
	setBackgroundMode( TransparentMode );	// default background mode
	setRasterOp( CopyROP );			// default raster operation
    }
    gc_cache_clip_serial++;
    updateBrush();
    updatePen();
    return TRUE;
}

/*!
  Ends painting.  Any resources used while painting are released.

  Note that while you mostly don't need to call end(), the destructor
  will do it, there is at least one common case, namely double
  buffering.

  \code
    QPainter p( myPixmap, this )
    // ...
    p.end(); // stops drawing on myPixmap
    p.begin( this );
    p.drawPixmap( myPixmap );
  \endcode

  Since you can't draw a QPixmap while it is being painted, it is
  necessary to close the active painter.

  \sa begin(), isActive()
*/

bool QPainter::end()				// end painting
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	qWarning( "QPainter::end: Missing begin() or begin() failed" );
#endif
	return FALSE;
    }
    killPStack();
    if ( testf(FontMet) )			// remove references to this
	QFontMetrics::reset( this );
    if ( testf(FontInf) )			// remove references to this
	QFontInfo::reset( this );

    //#### This should not be necessary:
    if ( pdev->devType() == QInternal::Widget  &&
	 ((QWidget*)pdev)->testWFlags(WPaintUnclipped) ) {
	if ( gc )
	    XSetSubwindowMode( dpy, gc, ClipByChildren );
	if ( gc_brush )
	    XSetSubwindowMode( dpy, gc_brush, ClipByChildren );
    }

    if ( gc_brush ) {				// restore brush gc
	if ( brushRef ) {
	    release_gc( brushRef );
	    brushRef = 0;
	} else {
	    free_gc( dpy, gc_brush, testf(UsePrivateCx) );
	}
	gc_brush = 0;

    }
    if ( gc ) {					// restore pen gc
	if ( penRef ) {
	    release_gc( penRef );
	    penRef = 0;
	} else {
	    free_gc( dpy, gc, testf(UsePrivateCx) );
	}
	gc = 0;
    }

    if ( testf(ExtDev) )
	pdev->cmd( QPaintDevice::PdcEnd, this, 0 );

    flags = 0;
    pdev->painters--;
    pdev = 0;
    dpy  = 0;
    return TRUE;
}


/*!
  Flushes any buffered drawing operations.
*/

void QPainter::flush()
{
    if ( isActive() && dpy )
	XFlush( dpy );
}


/*!
  Sets the background color of the painter to \a c.

  The background color is the color that is filled in when drawing
  opaque text, stippled lines and bitmaps.  The background color has
  no effect in transparent background mode (which is the default).

  \sa backgroundColor() setBackgroundMode() BackgroundMode
*/

void QPainter::setBackgroundColor( const QColor &c )
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	qWarning( "QPainter::setBackgroundColor: Call begin() first" );
#endif
	return;
    }
    bg_col = c;
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].color = &bg_col;
	if ( !pdev->cmd( QPaintDevice::PdcSetBkColor, this, param ) || !hd )
	    return;
    }
    if ( !penRef )
	updatePen();				// update pen setting
    if ( !brushRef )
	updateBrush();				// update brush setting
}

/*!
  Sets the background mode of the painter to \a m, which must be one
  of \c TransparentMode (the default) and \c OpaqueMode.

  Transparent mode draws stippled lines and text without setting the
  background pixels. Opaque mode fills these space with the current
  background color.

  Note that in order to draw a bitmap or pixmap transparently, you must use
  QPixmap::setMask().

  \sa backgroundMode(), setBackgroundColor() */

void QPainter::setBackgroundMode( BGMode m )
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	qWarning( "QPainter::setBackgroundMode: Call begin() first" );
#endif
	return;
    }
    if ( m != TransparentMode && m != OpaqueMode ) {
#if defined(CHECK_RANGE)
	qWarning( "QPainter::setBackgroundMode: Invalid mode" );
#endif
	return;
    }
    bg_mode = m;
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = m;
	if ( !pdev->cmd( QPaintDevice::PdcSetBkMode, this, param ) || !hd )
	    return;
    }
    if ( !penRef )
	updatePen();				// update pen setting
    if ( !brushRef )
	updateBrush();				// update brush setting
}

static short ropCodes[] = {			// ROP translation table
    GXcopy, GXor, GXxor, GXandInverted,
    GXcopyInverted, GXorInverted, GXequiv, GXand,
    GXinvert, GXclear, GXset, GXnoop,
    GXandReverse, GXorReverse, GXnand, GXnor
};

/*!
  Sets the raster operation to \a r.  The default is \c CopyROP.
  \sa rasterOp()
*/

void QPainter::setRasterOp( RasterOp r )
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	qWarning( "QPainter::setRasterOp: Call begin() first" );
#endif
	return;
    }
    if ( (uint)r > LastROP ) {
#if defined(CHECK_RANGE)
	qWarning( "QPainter::setRasterOp: Invalid ROP code" );
#endif
	return;
    }
    rop = r;
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = r;
	if ( !pdev->cmd( QPaintDevice::PdcSetROP, this, param ) || !hd )
	    return;
    }
    if ( penRef )
	updatePen();				// get non-cached pen GC
    if ( brushRef )
	updateBrush();				// get non-cached brush GC
    XSetFunction( dpy, gc, ropCodes[rop] );
    XSetFunction( dpy, gc_brush, ropCodes[rop] );
}

// ### matthias - true?

/*!
  Sets the brush origin to \a (x,y).

  The brush origin specifies the (0,0) coordinate of the painter's
  brush.  This setting only applies to pattern brushes and pixmap
  brushes.

  \sa brushOrigin()
*/

void QPainter::setBrushOrigin( int x, int y )
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	qWarning( "QPainter::setBrushOrigin: Call begin() first" );
#endif
	return;
    }
    bro = QPoint(x,y);
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].point = &bro;
	if ( !pdev->cmd( QPaintDevice::PdcSetBrushOrigin, this, param ) ||
	     !hd )
	    return;
    }
    if ( brushRef )
	updateBrush();				// get non-cached brush GC
    XSetTSOrigin( dpy, gc_brush, x, y );
}


/*!
  Enables clipping if \a enable is TRUE, or disables clipping if \a enable
  is FALSE.
  \sa hasClipping(), setClipRect(), setClipRegion()
*/

void QPainter::setClipping( bool enable )
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	qWarning( "QPainter::setClipping: Will be reset by begin()" );
#endif
	return;
    }

    if ( !isActive() || enable == testf(ClipOn) )
	return;

    setf( ClipOn, enable );
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = enable;
	if ( !pdev->cmd( QPaintDevice::PdcSetClip, this, param ) || !hd )
	    return;
    }
    if ( enable ) {
	QRegion rgn = crgn;
	if ( pdev == paintEventDevice )
	    rgn = rgn.intersect( *paintEventClipRegion );
	if ( penRef )
	    updatePen();
	XSetRegion( dpy, gc, rgn.handle() );
	if ( brushRef )
	    updateBrush();
	XSetRegion( dpy, gc_brush, rgn.handle() );
    } else {
	if ( pdev == paintEventDevice ) {
	    XSetRegion( dpy, gc, paintEventClipRegion->handle() );
	    XSetRegion( dpy, gc_brush, paintEventClipRegion->handle() );
	} else {
	    XSetClipMask( dpy, gc, None );
	    XSetClipMask( dpy, gc_brush, None );
	}
    }
#ifdef QT_XFT
    qt_invalidate_ft_clip (hd);
#endif
}


/*!
  \overload void QPainter::setClipRect( const QRect &r )

  If the rectangle is invalid, the rectangle will be normalized() before the clipping region is set. This semantics will change in Qt-3
  and an invalid rectangle will clip the painter to an empty rectangle.

  \sa QRect::isValid() QRect::normalize()
*/

void QPainter::setClipRect( const QRect &r )
{
    QRegion rgn( r );
    setClipRegion( rgn );
}

/*!
  Sets the clip region to \a rgn and enables clipping.

  Note that the clip region is given in physical device coordinates and
  \e not subject to any \link coordsys.html coordinate
  transformation.\endlink

  \sa setClipRect(), clipRegion(), setClipping()
*/

void QPainter::setClipRegion( const QRegion &rgn )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setClipRegion: Will be reset by begin()" );
#endif
    crgn = rgn;

    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].rgn = &crgn;
	if ( !pdev->cmd( QPaintDevice::PdcSetClipRegion, this, param ) )
	    return; // device cannot clip
    }
    clearf( ClipOn );				// be sure to update clip rgn
    setClipping( TRUE );
}


/*!
  Internal function for drawing a polygon.
*/

void QPainter::drawPolyInternal( const QPointArray &a, bool close )
{
    if ( a.size() < 2 )
	return;

    int x1, y1, x2, y2;				// connect last to first point
    a.point( a.size()-1, &x1, &y1 );
    a.point( 0, &x2, &y2 );
    bool do_close = close && !(x1 == x2 && y1 == y2);

    if ( close && cbrush.style() != NoBrush ) {	// draw filled polygon
	XFillPolygon( dpy, hd, gc_brush, (XPoint*)a.shortPoints(), a.size(),
		      Nonconvex, CoordModeOrigin );
	if ( cpen.style() == NoPen ) {		// draw fake outline
	    XDrawLines( dpy, hd, gc_brush, (XPoint*)a.shortPoints(), a.size(),
			CoordModeOrigin );
	    if ( do_close )
		XDrawLine( dpy, hd, gc_brush, x1, y1, x2, y2 );
	}
    }
    if ( cpen.style() != NoPen ) {		// draw outline
	XDrawLines( dpy, hd, gc, (XPoint*)a.shortPoints(), a.size(),
		    CoordModeOrigin);
	if ( do_close )
	    XDrawLine( dpy, hd, gc, x1, y1, x2, y2 );
    }
}


/*!
  Draws/plots a single point at \a (x,y) using the current pen.

  \sa QPen
*/

void QPainter::drawPoint( int x, int y )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QPoint p( x, y );
	    param[0].point = &p;
	    if ( !pdev->cmd( QPaintDevice::PdcDrawPoint, this, param ) ||
		 !hd )
		return;
	}
	map( x, y, &x, &y );
    }
    if ( cpen.style() != NoPen )
	XDrawPoint( dpy, hd, gc, x, y );
}


/*!
  Draws/plots an array of points using the current pen.

  If \a index is non-zero (the default is zero) only points from \a
  index are drawn.  If \a npoints is negative (the default) the rest
  of the points from \a index are drawn.  If is is zero or greater, \a
  npoints points are drawn.
*/

void QPainter::drawPoints( const QPointArray& a, int index, int npoints )
{
    if ( npoints < 0 )
	npoints = a.size() - index;
    if ( index + npoints > (int)a.size() )
	npoints = a.size() - index;
    if ( !isActive() || npoints < 1 || index < 0 )
	return;
    QPointArray pa = a;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    for (int i=0; i<npoints; i++) {
		QPoint p( pa[index+i].x(), pa[index+i].y() );
		param[0].point = &p;
		if ( !pdev->cmd( QPaintDevice::PdcDrawPoint, this, param ))
		    return;
	    }
	    if ( !hd ) return;
	}
	if ( txop != TxNone ) {
	    pa = xForm( a, index, npoints );
	    if ( pa.size() != a.size() ) {
		index = 0;
		npoints = pa.size();
	    }
	}
    }
    if ( cpen.style() != NoPen )
	XDrawPoints( dpy, hd, gc, (XPoint*)(pa.shortPoints( index, npoints )),
		     npoints, CoordModeOrigin );
}


/*!
  Sets the current pen position to \a (x,y)
  \sa lineTo(), pos()
*/

void QPainter::moveTo( int x, int y )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QPoint p( x, y );
	    param[0].point = &p;
	    if ( !pdev->cmd( QPaintDevice::PdcMoveTo, this, param ) || !hd )
		return;
	}
	map( x, y, &x, &y );
    }
    curPt = QPoint( x, y );
}

/*!
  Draws a line from the current pen position to \a (x,y) and sets \a
  (x,y) to be the new current pen position.

  \sa QPen moveTo(), drawLine(), pos()
*/

void QPainter::lineTo( int x, int y )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QPoint p( x, y );
	    param[0].point = &p;
	    if ( !pdev->cmd( QPaintDevice::PdcLineTo, this, param ) || !hd )
		return;
	}
	map( x, y, &x, &y );
    }
    if ( cpen.style() != NoPen )
	XDrawLine( dpy, hd, gc, curPt.x(), curPt.y(), x, y );
    curPt = QPoint( x, y );
}

/*!
  Draws a line from \a (x1,y2) to \a (x2,y2) and sets \a (x2,y2) to be
  the new current pen position.

  \sa QPen
*/

void QPainter::drawLine( int x1, int y1, int x2, int y2 )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[2];
	    QPoint p1(x1, y1), p2(x2, y2);
	    param[0].point = &p1;
	    param[1].point = &p2;
	    if ( !pdev->cmd( QPaintDevice::PdcDrawLine, this, param ) || !hd )
		return;
	}
	map( x1, y1, &x1, &y1 );
	map( x2, y2, &x2, &y2 );
    }
    if ( cpen.style() != NoPen )
	XDrawLine( dpy, hd, gc, x1, y1, x2, y2 );
    curPt = QPoint( x2, y2 );
}



/*!
  Draws a rectangle with upper left corner at \a (x,y) and with
  width \a w and height \a h.

  \sa QPen, drawRoundRect()
*/

void QPainter::drawRect( int x, int y, int w, int h )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    if ( !pdev->cmd( QPaintDevice::PdcDrawRect, this, param ) || !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear polygon
	    QPointArray a( QRect(x,y,w,h), TRUE );
	    drawPolyInternal( xForm(a) );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    if ( cbrush.style() != NoBrush ) {
	if ( cpen.style() == NoPen ) {
	    XFillRectangle( dpy, hd, gc_brush, x, y, w, h );
	    return;
	}
	if ( w > 2 && h > 2 )
	    XFillRectangle( dpy, hd, gc_brush, x+1, y+1, w-2, h-2 );
    }
    if ( cpen.style() != NoPen )
	XDrawRectangle( dpy, hd, gc, x, y, w-1, h-1 );
}

/*!
  Draws a Windows focus rectangle with upper left corner at \a (x,y) and with
  width \a w and height \a h.

  This function draws a stippled XOR rectangle that is used to indicate
  keyboard focus (when QApplication::style() is \c WindowStyle).

  \warning This function draws nothing if the coordinate system has been
  \link rotate() rotated\endlink or \link shear() sheared\endlink.

  \sa drawRect(), QApplication::style()
*/

void QPainter::drawWinFocusRect( int x, int y, int w, int h )
{
    drawWinFocusRect( x, y, w, h, TRUE, color0 );
}

/*!
  Draws a Windows focus rectangle with upper left corner at \a (x,y) and with
  width \a w and height \a h using a pen color that contrasts with \a bgColor.

  This function draws a stippled rectangle (XOR is not used) that is
  used to indicate keyboard focus (when the QApplication::style() is
  \c WindowStyle).

  The pen color used to draw the rectangle is either white or black
  depending on the color of \a bgColor (see QColor::gray()).

  \warning This function draws nothing if the coordinate system has been
  \link rotate() rotated\endlink or \link shear() sheared\endlink.

  \sa drawRect(), QApplication::style()
*/

void QPainter::drawWinFocusRect( int x, int y, int w, int h,
				 const QColor &bgColor )
{
    drawWinFocusRect( x, y, w, h, FALSE, bgColor );
}


/*!
  \internal
*/

void QPainter::drawWinFocusRect( int x, int y, int w, int h,
				 bool xorPaint, const QColor &bgColor )
{
    if ( !isActive() || txop == TxRotShear )
	return;
    static char winfocus_line[] = { 1, 1 };

    QPen     old_pen = cpen;
    RasterOp old_rop = (RasterOp)rop;

    if ( xorPaint ) {
	if ( QColor::numBitPlanes() <= 8 )
	    setPen( color1 );
	else
	    setPen( white );
	setRasterOp( XorROP );
    } else {
	if ( qGray( bgColor.rgb() ) < 128 )
	    setPen( white );
	else
	    setPen( black );
    }

    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    if ( !pdev->cmd( QPaintDevice::PdcDrawRect, this, param ) || !hd) {
		setRasterOp( old_rop );
		setPen( old_pen );
		return;
	    }
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    XSetDashes( dpy, gc, 0, winfocus_line, 2 );
    XSetLineAttributes( dpy, gc, 0, LineOnOffDash, CapButt, JoinMiter );

    XDrawRectangle( dpy, hd, gc, x, y, w-1, h-1 );
    XSetLineAttributes( dpy, gc, 0, LineSolid, CapButt, JoinMiter );
    setRasterOp( old_rop );
    setPen( old_pen );
}


/*! \overload void QPainter::drawRoundRect( int x, int y, int w, int h )

  As the main version of the function, but with the roundness
  arguments fixed at 25.
*/


/*! \overload void QPainter::drawRoundRect( const QRect & )

  As the main version of the function, but with the roundness
  arguments fixed at 25.
*/


/*!
  Draws a rectangle with round corners at \a (x,y), with width \a w
  and height \a h.

  The \a xRnd and \a yRnd arguments specify how rounded the corners
  should be.  0 is angled corners, 99 is maximum roundedness.

  The width and height include all of the drawn lines.

  \sa drawRect(), QPen
*/

void QPainter::drawRoundRect( int x, int y, int w, int h, int xRnd, int yRnd )
{
    if ( !isActive() )
	return;
    if ( xRnd <= 0 || yRnd <= 0 ) {
	drawRect( x, y, w, h );			// draw normal rectangle
	return;
    }
    if ( xRnd >= 100 )				// fix ranges
	xRnd = 99;
    if ( yRnd >= 100 )
	yRnd = 99;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    param[1].ival = xRnd;
	    param[2].ival = yRnd;
	    if ( !pdev->cmd( QPaintDevice::PdcDrawRoundRect, this, param ) ||
		 !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear polygon
	    if ( w <= 0 || h <= 0 )
		fix_neg_rect( &x, &y, &w, &h );
	    w--;
	    h--;
	    int rxx = w*xRnd/200;
	    int ryy = h*yRnd/200;
	    // were there overflows?
	    if ( rxx < 0 )
		rxx = w/200*xRnd;
	    if ( ryy < 0 )
		ryy = h/200*yRnd;
	    int rxx2 = 2*rxx;
	    int ryy2 = 2*ryy;
	    QPointArray a[4];
	    a[0].makeArc( x, y, rxx2, ryy2, 1*16*90, 16*90, xmat );
	    a[1].makeArc( x, y+h-ryy2, rxx2, ryy2, 2*16*90, 16*90, xmat );
	    a[2].makeArc( x+w-rxx2, y+h-ryy2, rxx2, ryy2, 3*16*90, 16*90, xmat );
	    a[3].makeArc( x+w-rxx2, y, rxx2, ryy2, 0*16*90, 16*90, xmat );
	    // ### is there a better way to join QPointArrays?
	    QPointArray aa;
	    aa.resize( a[0].size() + a[1].size() + a[2].size() + a[3].size() );
	    uint j = 0;
	    for ( int k=0; k<4; k++ ) {
		for ( uint i=0; i<a[k].size(); i++ ) {
		    aa.setPoint( j, a[k].point(i) );
		    j++;
		}
	    }
	    drawPolyInternal( aa );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    int rx = (w*xRnd)/200;
    int ry = (h*yRnd)/200;
    int rx2 = 2*rx;
    int ry2 = 2*ry;
    if ( cbrush.style() != NoBrush ) {		// draw filled round rect
	int dp, ds;
	if ( cpen.style() == NoPen ) {
	    dp = 0;
	    ds = 1;
	}
	else {
	    dp = 1;
	    ds = 0;
	}
#define SET_ARC(px,py,w,h,a1,a2) \
    a->x=px; a->y=py; a->width=w; a->height=h; a->angle1=a1; a->angle2=a2; a++
	XArc arcs[4];
	XArc *a = arcs;
	SET_ARC( x+w-rx2, y, rx2, ry2, 0, 90*64 );
	SET_ARC( x, y, rx2, ry2, 90*64, 90*64 );
	SET_ARC( x, y+h-ry2, rx2, ry2, 180*64, 90*64 );
	SET_ARC( x+w-rx2, y+h-ry2, rx2, ry2, 270*64, 90*64 );
	XFillArcs( dpy, hd, gc_brush, arcs, 4 );
#undef SET_ARC
#define SET_RCT(px,py,w,h) \
    r->x=px; r->y=py; r->width=w; r->height=h; r++
	XRectangle rects[3];
	XRectangle *r = rects;
	SET_RCT( x+rx, y+dp, w-rx2, ry );
	SET_RCT( x+dp, y+ry, w+ds, h-ry2 );
	SET_RCT( x+rx, y+h-ry, w-rx2, ry+ds );
	XFillRectangles( dpy, hd, gc_brush, rects, 3 );
#undef SET_RCT
    }
    if ( cpen.style() != NoPen ) {		// draw outline
#define SET_ARC(px,py,w,h,a1,a2) \
    a->x=px; a->y=py; a->width=w; a->height=h; a->angle1=a1; a->angle2=a2; a++
	XArc arcs[4];
	XArc *a = arcs;
	SET_ARC( x+w-rx2, y, rx2, ry2, 0, 90*64 );
	SET_ARC( x, y, rx2, ry2, 90*64, 90*64 );
	SET_ARC( x, y+h-ry2, rx2, ry2, 180*64, 90*64 );
	SET_ARC( x+w-rx2, y+h-ry2, rx2, ry2, 270*64, 90*64 );
	XDrawArcs( dpy, hd, gc, arcs, 4 );
#undef SET_ARC
#define SET_SEG(xp1,yp1,xp2,yp2) \
    s->x1=xp1; s->y1=yp1; s->x2=xp2; s->y2=yp2; s++
	XSegment segs[4];
	XSegment *s = segs;
	SET_SEG( x+rx, y, x+w-rx, y );
	SET_SEG( x+rx, y+h, x+w-rx, y+h );
	SET_SEG( x, y+ry, x, y+h-ry );
	SET_SEG( x+w, y+ry, x+w, y+h-ry );
	XDrawSegments( dpy, hd, gc, segs, 4 );
#undef SET_SET
    }
}

/*!
  Draws an ellipse with center at \a (x+w/2,y+h/2) and size \a (w,h).
*/

void QPainter::drawEllipse( int x, int y, int w, int h )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    if ( !pdev->cmd( QPaintDevice::PdcDrawEllipse, this, param ) ||
		 !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear polygon
	    QPointArray a;
	    a.makeArc( x, y, w, h, 0, 360*16, xmat );
	    drawPolyInternal( a );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    if ( cbrush.style() != NoBrush ) {		// draw filled ellipse
	XFillArc( dpy, hd, gc_brush, x, y, w, h, 0, 360*64 );
	if ( cpen.style() == NoPen ) {
	    XDrawArc( dpy, hd, gc_brush, x, y, w, h, 0, 360*64 );
	    return;
	}
    }
    if ( cpen.style() != NoPen )		// draw outline
	XDrawArc( dpy, hd, gc, x, y, w, h, 0, 360*64 );
}


/*!
  Draws an arc defined by the rectangle \a (x,y,w,h), the start
  angle \a a and the arc length \a alen.

  The angles \a a and \a alen are 1/16th of a degree, i.e. a full
  circle equals 5760 (16*360). Positive values of \a a and \a alen mean
  counter-clockwise while negative values mean clockwise direction.
  Zero degrees is at the 3'o clock position.

  Example:
  \code
    QPainter p( myWidget );
    p.drawArc( 10,10, 70,100, 100*16, 160*16 ); // draws a "(" arc
  \endcode

  \sa drawPie(), drawChord()
*/

void QPainter::drawArc( int x, int y, int w, int h, int a, int alen )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    param[1].ival = a;
	    param[2].ival = alen;
	    if ( !pdev->cmd( QPaintDevice::PdcDrawArc, this, param ) ||
		 !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear
	    QPointArray pa;
	    pa.makeArc( x, y, w, h, a, alen, xmat ); // arc polyline
	    drawPolyInternal( pa, FALSE );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    if ( cpen.style() != NoPen )
	XDrawArc( dpy, hd, gc, x, y, w, h, a*4, alen*4 );
}


/*!
  Draws a pie defined by the rectangle \a (x,y,w,h), the start
  angle \a a and the arc length \a alen.

  The pie is filled with the current brush().

  The angles \a a and \a alen are 1/16th of a degree, i.e. a full
  circle equals 5760 (16*360). Positive values of \a a and \a alen mean
  counter-clockwise while negative values mean clockwise direction.
  Zero degrees is at the 3'o clock position.

  \sa drawArc(), drawChord()
*/

void QPainter::drawPie( int x, int y, int w, int h, int a, int alen )
{
    // Make sure "a" is 0..360*16, as otherwise a*4 may overflow 16 bits.
    if ( a > (360*16) ) {
	a = a % (360*16);
    } else if ( a < 0 ) {
	a = a % (360*16);
	if ( a < 0 ) a += (360*16);
    }

    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    param[1].ival = a;
	    param[2].ival = alen;
	    if ( !pdev->cmd( QPaintDevice::PdcDrawPie, this, param ) || !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear
	    QPointArray pa;
	    pa.makeArc( x, y, w, h, a, alen, xmat ); // arc polyline
	    int n = pa.size();
	    int cx, cy;
	    xmat.map(x+w/2, y+h/2, &cx, &cy);
	    pa.resize( n+2 );
	    pa.setPoint( n, cx, cy );	// add legs
	    pa.setPoint( n+1, pa.at(0) );
	    drawPolyInternal( pa );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    XSetArcMode( dpy, gc_brush, ArcPieSlice );
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }

    GC g = gc;
    bool nopen = cpen.style() == NoPen;

    if ( cbrush.style() != NoBrush ) {		// draw filled pie
	XFillArc( dpy, hd, gc_brush, x, y, w, h, a*4, alen*4 );
	if ( nopen ) {
	    g = gc_brush;
	    nopen = FALSE;
	}
    }
    if ( !nopen ) {				// draw pie outline
	double w2 = 0.5*w;			// with, height in ellipsis
	double h2 = 0.5*h;
	double xc = (double)x+w2;
	double yc = (double)y+h2;
	double ra1 = Q_PI/2880.0*a;		// convert a,alen to radians
	double ra2 = ra1 + Q_PI/2880.0*alen;
	int xic = qRound(xc);
	int yic = qRound(yc);
	XDrawLine( dpy, hd, g, xic, yic,
		   qRound(xc + qcos(ra1)*w2), qRound(yc - qsin(ra1)*h2));
	XDrawLine( dpy, hd, g, xic, yic,
		   qRound(xc + qcos(ra2)*w2), qRound(yc - qsin(ra2)*h2));
	XDrawArc( dpy, hd, g, x, y, w, h, a*4, alen*4 );
    }
}


/*!
  Draws a chord defined by the rectangle \a (x,y,w,h), the start
  angle \a a and the arc length \a alen.

  The chord is filled with the current brush().

  The angles \a a and \a alen are 1/16th of a degree, i.e. a full
  circle equals 5760 (16*360). Positive values of \a a and \a alen mean
  counter-clockwise while negative values mean clockwise direction.
  Zero degrees is at the 3'o clock position.

  \sa drawArc(), drawPie()
*/

void QPainter::drawChord( int x, int y, int w, int h, int a, int alen )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    param[1].ival = a;
	    param[2].ival = alen;
	    if ( !pdev->cmd(QPaintDevice::PdcDrawChord,this,param) || !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear
	    QPointArray pa;
	    pa.makeArc( x, y, w-1, h-1, a, alen, xmat ); // arc polygon
	    int n = pa.size();
	    pa.resize( n+1 );
	    pa.setPoint( n, pa.at(0) );		// connect endpoints
	    drawPolyInternal( pa );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    XSetArcMode( dpy, gc_brush, ArcChord );
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }

    GC g = gc;
    bool nopen = cpen.style() == NoPen;

    if ( cbrush.style() != NoBrush ) {		// draw filled chord
	XFillArc( dpy, hd, gc_brush, x, y, w, h, a*4, alen*4 );
	if ( nopen ) {
	    g = gc_brush;
	    nopen = FALSE;
	}
    }
    if ( !nopen ) {				// draw chord outline
	double w2 = 0.5*w;			// with, height in ellipsis
	double h2 = 0.5*h;
	double xc = (double)x+w2;
	double yc = (double)y+h2;
	double ra1 = Q_PI/2880.0*a;		// convert a,alen to radians
	double ra2 = ra1 + Q_PI/2880.0*alen;
	XDrawLine( dpy, hd, g,
		   qRound(xc + qcos(ra1)*w2), qRound(yc - qsin(ra1)*h2),
		   qRound(xc + qcos(ra2)*w2), qRound(yc - qsin(ra2)*h2));
	XDrawArc( dpy, hd, g, x, y, w, h, a*4, alen*4 );
    }
    XSetArcMode( dpy, gc_brush, ArcPieSlice );
}


/*!
  Draws \a nlines separate lines from points defined in \a a, starting
  at a[\a index] (\a index defaults to 0). If \a nlines is -1 (the
  defauls) all points until the end of the array are used
  (i.e. (a.size()-index)/2 lines are drawn).

  Draws the 1st line from \a a[index] to \a a[index+1].
  Draws the 2nd line from \a a[index+2] to \a a[index+3] etc.

  \sa drawPolyline(), drawPolygon(), QPen
*/

void QPainter::drawLineSegments( const QPointArray &a, int index, int nlines )
{
    if ( nlines < 0 )
	nlines = a.size()/2 - index/2;
    if ( index + nlines*2 > (int)a.size() )
	nlines = (a.size() - index)/2;
    if ( !isActive() || nlines < 1 || index < 0 )
	return;
    QPointArray pa = a;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    if ( nlines != (int)pa.size()/2 ) {
		pa = QPointArray( nlines*2 );
		for ( int i=0; i<nlines*2; i++ )
		    pa.setPoint( i, a.point(index+i) );
		index = 0;
	    }
	    QPDevCmdParam param[1];
	    param[0].ptarr = (QPointArray*)&pa;
	    if ( !pdev->cmd(QPaintDevice::PdcDrawLineSegments,this,param) || !hd )
		return;
	}
	if ( txop != TxNone ) {
	    pa = xForm( a, index, nlines*2 );
	    if ( pa.size() != a.size() ) {
		index  = 0;
		nlines = pa.size()/2;
	    }
	}
    }
    if ( cpen.style() != NoPen )
	XDrawSegments( dpy, hd, gc,
		       (XSegment*)(pa.shortPoints( index, nlines*2 )),nlines );
}


/*!
  Draws the polyline defined by the \a npoints points in \a a starting
  at \a a[index].  (\a index defaults to 0.)

  If \a npoints is -1 (the default) all points until the end of the
  array are used (i.e. a.size()-index-1 line segments are drawn).

  \sa drawLineSegments(), drawPolygon(), QPen
*/

void QPainter::drawPolyline( const QPointArray &a, int index, int npoints )
{
    if ( npoints < 0 )
	npoints = a.size() - index;
    if ( index + npoints > (int)a.size() )
	npoints = a.size() - index;
    if ( !isActive() || npoints < 2 || index < 0 )
	return;
    QPointArray pa = a;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    if ( npoints != (int)pa.size() ) {
		pa = QPointArray( npoints );
		for ( int i=0; i<npoints; i++ )
		    pa.setPoint( i, a.point(index+i) );
		index = 0;
	    }
	    QPDevCmdParam param[1];
	    param[0].ptarr = (QPointArray*)&pa;
	    if ( !pdev->cmd(QPaintDevice::PdcDrawPolyline,this,param) || !hd )
		return;
	}
	if ( txop != TxNone ) {
	    pa = xForm( a, index, npoints );
	    if ( pa.size() != a.size() ) {
		index   = 0;
		npoints = pa.size();
	    }
	}
    }
    if ( cpen.style() != NoPen ) {
	while(npoints>65535) {
	    XDrawLines( dpy, hd, gc, (XPoint*)(pa.shortPoints( index, 65535 )),
			65535, CoordModeOrigin );
	    npoints-=65535;
	    index+=65535;
	}
	XDrawLines( dpy, hd, gc, (XPoint*)(pa.shortPoints( index, npoints )),
		    npoints, CoordModeOrigin );
    }
}


/*!
  Draws the polygon defined by the \a npoints points in \a a starting at
  \a a[index].  (\a index defaults to 0.)

  If \a npoints is -1 (the default) all points until the end of the
  array are used (i.e. a.size()-index line segments define the
  polygon).

  The first point is always connected to the last point.

  The polygon is filled with the current brush().
  If \a winding is TRUE, the polygon is filled using the winding
  fill algorithm. If \a winding is FALSE, the polygon is filled using the
  even-odd (alternative) fill algorithm.

  \sa drawLineSegments(), drawPolyline(), QPen
*/

void QPainter::drawPolygon( const QPointArray &a, bool winding,
			    int index, int npoints )
{
    if ( npoints < 0 )
	npoints = a.size() - index;
    if ( index + npoints > (int)a.size() )
	npoints = a.size() - index;
    if ( !isActive() || npoints < 2 || index < 0 )
	return;
    QPointArray pa = a;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    if ( npoints != (int)a.size() ) {
		pa = QPointArray( npoints );
		for ( int i=0; i<npoints; i++ )
		    pa.setPoint( i, a.point(index+i) );
		index = 0;
	    }
	    QPDevCmdParam param[2];
	    param[0].ptarr = (QPointArray*)&pa;
	    param[1].ival = winding;
	    if ( !pdev->cmd(QPaintDevice::PdcDrawPolygon,this,param) || !hd )
		return;
	}
	if ( txop != TxNone ) {
	    pa = xForm( a, index, npoints );
	    if ( pa.size() != a.size() ) {
		index   = 0;
		npoints = pa.size();
	    }
	}
    }
    if ( winding )				// set to winding fill rule
	XSetFillRule( dpy, gc_brush, WindingRule );

    int x1, y1, x2, y2;				// connect last to first point
    pa.point( index+npoints-1, &x1, &y1 );
    pa.point( index, &x2, &y2 );
    bool closed = x1 == x2 && y1 == y2;

    if ( cbrush.style() != NoBrush ) {		// draw filled polygon
	XFillPolygon( dpy, hd, gc_brush,
		      (XPoint*)(pa.shortPoints( index, npoints )),
		      npoints, Complex, CoordModeOrigin );
    }
    if ( cpen.style() != NoPen ) {		// draw outline
	XDrawLines( dpy, hd, gc, (XPoint*)(pa.shortPoints( index, npoints )),
		    npoints, CoordModeOrigin );
	if ( !closed )
	    XDrawLine( dpy, hd, gc, x1, y1, x2, y2 );
    }
    if ( winding )				// set to normal fill rule
	XSetFillRule( dpy, gc_brush, EvenOddRule );
}


/*!
  Draws a cubic Bezier curve defined by the control points in \a a,
  starting at \a a[index].  (\a index defaults to 0.)

  Control points after \a a[index+3] are ignored.  Nothing happens if
  there aren't enough control points.
*/

void QPainter::drawQuadBezier( const QPointArray &a, int index )
{
    if ( !isActive() )
	return;
    if ( a.size() - index < 4 ) {
#if defined(CHECK_RANGE)
	qWarning( "QPainter::drawQuadBezier: Cubic Bezier needs 4 control "
		 "points" );
#endif
	return;
    }
    QPointArray pa( a );
    if ( index != 0 || a.size() > 4 ) {
	pa = QPointArray( 4 );
	for ( int i=0; i<4; i++ )
	    pa.setPoint( i, a.point(index+i) );
    }
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    param[0].ptarr = (QPointArray*)&pa;
	    if ( !pdev->cmd(QPaintDevice::PdcDrawQuadBezier,this,param) || !hd )
		return;
	}
	if ( txop != TxNone )
	    pa = xForm( pa );
    }
    if ( cpen.style() != NoPen ) {
	pa = pa.quadBezier();
	XDrawLines( dpy, hd, gc, (XPoint*)pa.shortPoints(), pa.size(),
		    CoordModeOrigin);
    }
}


/*!
  Draws a pixmap at \a (x,y) by copying a part of \a pixmap into the
  paint device.

  \a (x,y) specify the top-left point in the paint device that is to
  be drawn onto.  \a (sx,sy) specify the top-left point in \a pixmap
  that is to be drawn (the default is (0,0).  \a (sw,sh) specify the
  size of the pixmap that is to be drawn (the default, (-1,-1), means
  all the way to the right/bottom of the pixmap).

  \sa bitBlt(), QPixmap::setMask()
*/

void QPainter::drawPixmap( int x, int y, const QPixmap &pixmap,
			   int sx, int sy, int sw, int sh )
{
    if ( !isActive() || pixmap.isNull() )
	return;

    // right/bottom
    if ( sw < 0 )
	sw = pixmap.width()  - sx;
    if ( sh < 0 )
	sh = pixmap.height() - sy;

    // Sanity-check clipping
    if ( sx < 0 ) {
	x -= sx;
	sw += sx;
	sx = 0;
    }
    if ( sw + sx > pixmap.width() )
	sw = pixmap.width() - sx;
    if ( sy < 0 ) {
	y -= sy;
	sh += sy;
	sy = 0;
    }
    if ( sh + sy > pixmap.height() )
	sh = pixmap.height() - sy;

    if ( sw <= 0 || sh <= 0 )
	return;

    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) || txop == TxScale || txop == TxRotShear ) {
	    if ( sx != 0 || sy != 0 ||
		 sw != pixmap.width() || sh != pixmap.height() ) {
		QPixmap tmp( sw, sh, pixmap.depth() );
		bitBlt( &tmp, 0, 0, &pixmap, sx, sy, sw, sh, CopyROP, TRUE );
		if ( pixmap.mask() ) {
		    QBitmap mask( sw, sh );
		    bitBlt( &mask, 0, 0, pixmap.mask(), sx, sy, sw, sh,
			    CopyROP, TRUE );
		    tmp.setMask( mask );
		}
		drawPixmap( x, y, tmp );
		return;
	    }
	    if ( testf(ExtDev) ) {
		QPDevCmdParam param[2];
		QPoint p(x,y);
		param[0].point	= &p;
		param[1].pixmap = &pixmap;
		if ( !pdev->cmd(QPaintDevice::PdcDrawPixmap,this,param) || !hd )
		    return;
	    }
	    if ( txop == TxScale || txop == TxRotShear ) {
		QWMatrix mat( m11(), m12(),
			      m21(), m22(),
			      dx(),  dy() );
		mat = QPixmap::trueMatrix( mat, sw, sh );
		QPixmap pm = pixmap.xForm( mat );
		if ( !pm.mask() && txop == TxRotShear ) {
		    QBitmap bm_clip( sw, sh, 1 );
		    bm_clip.fill( color1 );
		    pm.setMask( bm_clip.xForm(mat) );
		}
		map( x, y, &x, &y );		// compute position of pixmap
		int dx, dy;
		mat.map( 0, 0, &dx, &dy );
		uint save_flags = flags;
		flags = IsActive | (save_flags & ClipOn);
		drawPixmap( x-dx, y-dy, pm );
		flags = save_flags;
		return;
	    }
	}
	map( x, y, &x, &y );
    }

    QBitmap *mask = (QBitmap *)pixmap.mask();
    bool mono = pixmap.depth() == 1;

    if ( mask && !hasClipping() && pdev != paintEventDevice ) {
	if ( mono ) {				// needs GCs pen color
	    bool selfmask = pixmap.data->selfmask;
	    if ( selfmask ) {
		XSetFillStyle( dpy, gc, FillStippled );
		XSetStipple( dpy, gc, pixmap.handle() );
	    } else {
		XSetFillStyle( dpy, gc, FillOpaqueStippled );
		XSetStipple( dpy, gc, pixmap.handle() );
		XSetClipMask( dpy, gc, mask->handle() );
		XSetClipOrigin( dpy, gc, x-sx, y-sy );
	    }
	    XSetTSOrigin( dpy, gc, x-sx, y-sy );
	    XFillRectangle( dpy, hd, gc, x, y, sw, sh );
	    XSetTSOrigin( dpy, gc, 0, 0 );
	    XSetFillStyle( dpy, gc, FillSolid );
	    if ( !selfmask ) {
		XSetClipOrigin( dpy, gc, 0, 0 );
		if ( pdev == paintEventDevice )
		    XSetRegion( dpy, gc, paintEventClipRegion->handle() );
		else
		    XSetClipMask( dpy, gc, None );
	    }
	} else {
	    bitBlt( pdev, x, y, &pixmap, sx, sy, sw, sh, (RasterOp)rop );
	}
	return;
    }

    QRegion rgn = crgn;

    if ( mask ) {				// pixmap has clip mask
	// Implies that clipping is on, either explicit or implicit
	// Create a new mask that combines the mask with the clip region

	if ( pdev == paintEventDevice ) {
	    if ( hasClipping() )
		rgn = rgn.intersect( *paintEventClipRegion );
	    else
		rgn = *paintEventClipRegion;
	}

	QBitmap *comb = new QBitmap( sw, sh );
	comb->detach();
	GC cgc = qt_xget_temp_gc( TRUE );	// get temporary mono GC
	XSetForeground( dpy, cgc, 0 );
	XFillRectangle( dpy, comb->handle(), cgc, 0, 0, sw, sh );
	XSetBackground( dpy, cgc, 0 );
	XSetForeground( dpy, cgc, 1 );
	XSetRegion( dpy, cgc, rgn.handle() );
	XSetClipOrigin( dpy, cgc, -x, -y );
	XSetFillStyle( dpy, cgc, FillOpaqueStippled );
	XSetStipple( dpy, cgc, mask->handle() );
	XSetTSOrigin( dpy, cgc, -sx, -sy );
	XFillRectangle( dpy, comb->handle(), cgc, 0, 0, sw, sh );
	XSetTSOrigin( dpy, cgc, 0, 0 );		// restore cgc
	XSetFillStyle( dpy, cgc, FillSolid );
	XSetClipOrigin( dpy, cgc, 0, 0 );
	XSetClipMask( dpy, cgc, None );
	mask = comb;				// it's deleted below

	XSetClipMask( dpy, gc, mask->handle() );
	XSetClipOrigin( dpy, gc, x, y );
    }

    if ( mono ) {
	XSetBackground( dpy, gc, bg_col.pixel() );
	XSetFillStyle( dpy, gc, FillOpaqueStippled );
	XSetStipple( dpy, gc, pixmap.handle() );
	XSetTSOrigin( dpy, gc, x-sx, y-sy );
	XFillRectangle( dpy, hd, gc, x, y, sw, sh );
	XSetTSOrigin( dpy, gc, 0, 0 );
	XSetFillStyle( dpy, gc, FillSolid );
    } else {
	XCopyArea( dpy, pixmap.handle(), hd, gc, sx, sy, sw, sh, x, y );
    }

    if ( mask ) {				// restore clipping
	XSetClipOrigin( dpy, gc, 0, 0 );
	XSetRegion( dpy, gc, rgn.handle() );
	delete mask;				// delete comb, created above
    }
}


/* Internal, used by drawTiledPixmap */

static void drawTile( QPainter *p, int x, int y, int w, int h,
		      const QPixmap &pixmap, int xOffset, int yOffset )
{
    int yPos, xPos, drawH, drawW, yOff, xOff;
    yPos = y;
    yOff = yOffset;
    while( yPos < y + h ) {
	drawH = pixmap.height() - yOff;    // Cropping first row
	if ( yPos + drawH > y + h )	   // Cropping last row
	    drawH = y + h - yPos;
	xPos = x;
	xOff = xOffset;
	while( xPos < x + w ) {
	    drawW = pixmap.width() - xOff; // Cropping first column
	    if ( xPos + drawW > x + w )	   // Cropping last column
		drawW = x + w - xPos;
	    p->drawPixmap( xPos, yPos, pixmap, xOff, yOff, drawW, drawH );
	    xPos += drawW;
	    xOff = 0;
	}
	yPos += drawH;
	yOff = 0;
    }
}

#if 0 // see comment in drawTiledPixmap
/* Internal, used by drawTiledPixmap */

static void fillTile(  QPixmap *tile, const QPixmap &pixmap )
{
    bitBlt( tile, 0, 0, &pixmap, 0, 0, -1, -1, Qt::CopyROP, TRUE );
    int x = pixmap.width();
    while ( x < tile->width() ) {
	bitBlt( tile, x,0, tile, 0,0, x,pixmap.height(), Qt::CopyROP, TRUE );
	x *= 2;
    }
    int y = pixmap.height();
    while ( y < tile->height() ) {
	bitBlt( tile, 0,y, tile, 0,0, tile->width(),y, Qt::CopyROP, TRUE );
	y *= 2;
    }
}
#endif

/*!
  Draws a tiled \a pixmap in the specified rectangle.

  \a (x,y) specify the top-left point in the paint device that is to
  be drawn onto.  \a (sx,sy) specify the top-left point in \a pixmap
  that is to be drawn (the default is (0,0).

  Calling drawTiledPixmap() is similar to calling drawPixmap() several
  times to fill (tile) an area with a pixmap, but is potentially
  much more efficient depending on the underlying window system.

  \sa drawPixmap()
*/

void QPainter::drawTiledPixmap( int x, int y, int w, int h,
				const QPixmap &pixmap, int sx, int sy )
{
    int sw = pixmap.width();
    int sh = pixmap.height();
    if (!sw || !sh )
	return;
    if ( sx < 0 )
	sx = sw - -sx % sw;
    else
	sx = sx % sw;
    if ( sy < 0 )
	sy = sh - -sy % sh;
    else
	sy = sy % sh;
    /*
      Requirements for optimizing tiled pixmaps:
      - not an external device
      - not scale or rotshear
      - not mono pixmap
      - no mask
    */
    QBitmap *mask = (QBitmap *)pixmap.mask();
    if ( !testf(ExtDev) && txop <= TxTranslate && pixmap.depth() > 1 &&
	 mask == 0 ) {
	if ( txop == TxTranslate )
	    map( x, y, &x, &y );
	XSetTile( dpy, gc, pixmap.handle() );
	XSetFillStyle( dpy, gc, FillTiled );
	XSetTSOrigin( dpy, gc, x-sx, y-sy );
	XFillRectangle( dpy, hd, gc, x, y, w, h );
	XSetTSOrigin( dpy, gc, 0, 0 );
	XSetFillStyle( dpy, gc, FillSolid );
	return;
    }

#if 0
    // maybe there'll be point in this again, but for the time all it
    // does is make trouble for the postscript code.
    if ( sw*sh < 8192 && sw*sh < 16*w*h ) {
	int tw = sw;
	int th = sh;
	while( th * tw < 4096 && ( th < h || tw < w ) ) {
	    if ( h/th > w/tw )
		th *= 2;
	    else
		tw *= 2;
	}
	QPixmap tile( tw, th, pixmap.depth(), QPixmap::NormalOptim );
	fillTile( &tile, pixmap );
	if ( mask ) {
	    QBitmap tilemask( tw, th, QPixmap::NormalOptim );
	    fillTile( &tilemask, *mask );
	    tile.setMask( tilemask );
	}
	drawTile( this, x, y, w, h, tile, sx, sy );
    } else {
	drawTile( this, x, y, w, h, pixmap, sx, sy );
    }
#else
    // for now we'll just output the original and let the postscript
    // code make what it can of it.  qpicture will be unhappy.
    drawTile( this, x, y, w, h, pixmap, sx, sy );
#endif
}


//
// Generate a string that describes a transformed bitmap. This string is used
// to insert and find bitmaps in the global pixmap cache.
//

static QString gen_text_bitmap_key( const QWMatrix &m, const QFont &font,
				    const QString &str, int len )
{
    QString fk = font.key();
    int sz = 4*2 + len*2 + fk.length()*2 + sizeof(double)*6;
    QByteArray buf(sz);
    uchar *p = (uchar *)buf.data();
    *((double*)p)=m.m11();  p+=sizeof(double);
    *((double*)p)=m.m12();  p+=sizeof(double);
    *((double*)p)=m.m21();  p+=sizeof(double);
    *((double*)p)=m.m22();  p+=sizeof(double);
    *((double*)p)=m.dx();   p+=sizeof(double);
    *((double*)p)=m.dy();   p+=sizeof(double);
    QChar h1( '$' );
    QChar h2( 'q' );
    QChar h3( 't' );
    QChar h4( '$' );
    *((QChar*)p)=h1;  p+=2;
    *((QChar*)p)=h2;  p+=2;
    *((QChar*)p)=h3;  p+=2;
    *((QChar*)p)=h4;  p+=2;
    memcpy( (char*)p, (char*)str.unicode(), len*2 );  p += len*2;
    memcpy( (char*)p, (char*)fk.unicode(), fk.length()*2 ); p += fk.length()*2;
    return QString( (QChar*)buf.data(), buf.size()/2 );
}

static QBitmap *get_text_bitmap( const QString &key )
{
    return (QBitmap*)QPixmapCache::find( key );
}

static void ins_text_bitmap( const QString &key, QBitmap *bm )
{
    if ( !QPixmapCache::insert(key,bm) )	// cannot insert pixmap
	delete bm;
}


/*!
  Draws at most \a len characters from \a str at position \a (x,y).

  \a (x,y) is the base line position.  Note that the meaning of \a y
  is not the same for the two drawText() varieties.
*/

void QPainter::drawText( int x, int y, const QString &str, int len )
{
    if ( !isActive() )
	return;
    if ( len < 0 )
	len = str.length();
    if ( len == 0 )				// empty string
	return;

    if ( testf(DirtyFont|ExtDev|VxF|WxF) ) {
	if ( testf(DirtyFont) )
	    updateFont();

	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[2];
	    QPoint p( x, y );
	    QString newstr = str.left(len);
	    param[0].point = &p;
	    param[1].str = &newstr;
	    if ( !pdev->cmd(QPaintDevice::PdcDrawText2,this,param) || !hd )
		return;
	}

	if ( txop >= TxScale ) {
	    QFontMetrics fm = fontMetrics();
	    QFontInfo	 fi = fontInfo();
	    QRect bbox = fm.boundingRect( str, len );
	    int w=bbox.width(), h=bbox.height();
	    int aw, ah;
	    int tx=-bbox.x(),  ty=-bbox.y();	// text position
	    QWMatrix mat1( m11(), m12(), m21(), m22(), dx(),  dy() );
	    QFont dfont( cfont );
	    QWMatrix mat2;
	    if ( txop == TxScale ) {
		double newSize = m22() * cfont.pointSizeFloat();
		newSize = QMAX( 6.0, QMIN( newSize, 72.0 ) ); // empirical values
		dfont.setPointSizeFloat( newSize );
		QFontMetrics fm2( dfont );
		QRect abbox = fm2.boundingRect( str, len );
		aw = abbox.width();
		ah = abbox.height();
		tx = -abbox.x();
		ty = -abbox.y();	// text position - off-by-one?
		if ( aw == 0 || ah == 0 )
		    return;
// ###		double rx = mat1.m11() * (double)w / (double)aw;
//		double ry = mat1.m22() * (double)h / (double)ah;
		double rx = mat1.m11() * cfont.pointSizeFloat() / newSize;
		double ry = mat1.m22() * cfont.pointSizeFloat() / newSize;
		mat2 = QWMatrix( rx, 0, 0, ry, 0, 0 );
	    } else {
		mat2 = QPixmap::trueMatrix( mat1, w, h );
		aw = w;
		ah = h;
	    }
	    bool empty = aw == 0 || ah == 0;
	    QString bm_key = gen_text_bitmap_key( mat2, dfont, str, len );
	    QBitmap *wx_bm = get_text_bitmap( bm_key );
	    bool create_new_bm = wx_bm == 0;
	    if ( create_new_bm && !empty ) {	// no such cached bitmap
		QBitmap bm( aw, ah, TRUE );	// create bitmap
		QPainter paint;
		paint.begin( &bm );		// draw text in bitmap
		paint.setFont( dfont );
		paint.drawText( tx, ty, str, len );
		paint.end();
		wx_bm = new QBitmap( bm.xForm(mat2) ); // transform bitmap
		if ( wx_bm->isNull() ) {
		    delete wx_bm;		// nothing to draw
		    return;
		}
	    }
	    if ( bg_mode == OpaqueMode ) {	// opaque fill
		int fx = x;
		int fy = y - fm.ascent();
		int fw = fm.width(str,len);
		int fh = fm.ascent() + fm.descent();
		int m, n;
		QPointArray a(5);
		mat1.map( fx,	 fy,	&m, &n );  a.setPoint( 0, m, n );
						   a.setPoint( 4, m, n );
		mat1.map( fx+fw, fy,	&m, &n );  a.setPoint( 1, m, n );
		mat1.map( fx+fw, fy+fh, &m, &n );  a.setPoint( 2, m, n );
		mat1.map( fx,	 fy+fh, &m, &n );  a.setPoint( 3, m, n );
		QBrush oldBrush = cbrush;
		setBrush( backgroundColor() );
		updateBrush();
		XFillPolygon( dpy, hd, gc_brush, (XPoint*)a.shortPoints(), 4,
			      Nonconvex, CoordModeOrigin );
		XDrawLines( dpy, hd, gc_brush, (XPoint*)a.shortPoints(), 5,
			    CoordModeOrigin );
		setBrush( oldBrush );
	    }
	    if ( empty )
		return;
	    double fx=x, fy=y, nfx, nfy;
	    mat1.map( fx,fy, &nfx,&nfy );
	    double tfx=tx, tfy=ty, dx, dy;
	    mat2.map( tfx, tfy, &dx, &dy );	// compute position of bitmap
	    x = qRound(nfx-dx);
	    y = qRound(nfy-dy);
	    XSetFillStyle( dpy, gc, FillStippled );
	    XSetStipple( dpy, gc, wx_bm->handle() );
	    XSetTSOrigin( dpy, gc, x, y );
	    XFillRectangle( dpy, hd, gc, x, y,wx_bm->width(),wx_bm->height() );
	    XSetTSOrigin( dpy, gc, 0, 0 );
	    XSetFillStyle( dpy, gc, FillSolid );
	    if ( create_new_bm )
		ins_text_bitmap( bm_key, wx_bm );
	    return;
	}
	if ( txop == TxTranslate )
	    map( x, y, &x, &y );
    }

    QCString mapped;

    const QTextCodec* mapper = cfont.d->mapper();
    if ( mapper ) {
	// translate from Unicode to font charset encoding here
	mapped = mapper->fromUnicode(str,len);
    }
// ### not pretty -- using codec to produce 16-bit encoding
    if ( !cfont.handle() ) {
	if ( mapped.isNull() )
	    qWarning("Fontsets only apply to mapped encodings");
	else {
	    XFontSet set = (XFontSet)cfont.d->fontSet();
	    if ( bg_mode == TransparentMode )
		XmbDrawString( dpy, hd, set, gc, x, y, mapped, len );
	    else
		XmbDrawImageString( dpy, hd, set, gc, x, y, mapped, len );
	}
    } else {
	if ( !mapped.isNull()
#ifdef QT_XFT
	     // XFT always uses unicode mappings
	     && !qt_use_xft()
#endif
	    ) {
	    if ( cfont.charSet() < QFont::Enc16 )
		if ( bg_mode == TransparentMode )
		    XDrawString( dpy, hd, gc, x, y, mapped, len );
		else
		    XDrawImageString( dpy, hd, gc, x, y, mapped, len );
	    else
		if ( bg_mode == TransparentMode )
		    XDrawString16( dpy, hd, gc, x, y, (XChar2b*)mapped.data(), len/2 );
		else
		    XDrawImageString16( dpy, hd, gc, x, y, (XChar2b*)mapped.data(), len/2 );
	} else {
	    // Unicode font

	    QString v = str;
#ifdef QT_BIDI
	    v.compose();  // apply ligatures (for arabic, etc...)
	    v = v.visual(); // visual ordering
	    len = v.length();
#endif
#ifdef QT_XFT
	    if (qt_use_xft ())
	    {
		XftFont	    *ft;
		XftDraw	    *draw;
		XftColor    color;

		ft = (XftFont *)qt_ft_font (&cfont);
		if (ft && (draw = qt_lookup_ft_draw (hd, pdev,
						     testf(ClipOn) ? &crgn :
						     0)))
		{
		    unsigned short   *tstr;
		    unsigned short  *tshort, c;
		    int i;

		    tshort = new unsigned short[len];
		    tstr = (unsigned short *) v.unicode();
		    for (i = 0; i < len; i++)
		    {
			c = tstr[i];
			tshort[i] = htons( c );
		    }
		    if ( bg_mode != TransparentMode )
		    {
			XGlyphInfo	extents;
			XftTextExtents16 (dpy, ft, tshort, len, &extents);
			color.color.red = backgroundColor().red() | backgroundColor().red() << 8;
			color.color.green = backgroundColor().green() | backgroundColor().green() << 8;
			color.color.blue = backgroundColor().blue() | backgroundColor().blue() << 8;
			color.color.alpha = 0xffff;
			color.pixel = backgroundColor().pixel();
			XftDrawRect (draw, &color, x, y - ft->ascent,
				     extents.xOff, ft->ascent + ft->descent);
		    }
		    color.color.red = cpen.color().red() | cpen.color().red() << 8;
		    color.color.green = cpen.color().green() | cpen.color().green() << 8;
		    color.color.blue = cpen.color().blue() | cpen.color().blue() << 8;
		    color.color.alpha = 0xffff;
		    color.pixel = cpen.color().pixel();
		    XftDrawString16 (draw, &color, ft, x, y, tshort, len);
		    delete [] tshort;
		}
	    }
	    else
#endif
	    {
	    if ( bg_mode == TransparentMode )
		XDrawString16( dpy, hd, gc, x, y, (XChar2b*)v.unicode(), len );
	    else
		XDrawImageString16( dpy, hd, gc, x, y, (XChar2b*)v.unicode(), len );
	}
    }
    }

    if ( cfont.underline() || cfont.strikeOut() ) {
	QFontMetrics fm = fontMetrics();
	int lw = fm.lineWidth();
	int tw = fm.width( str, len );
	if ( cfont.underline() )		// draw underline effect
	    XFillRectangle( dpy, hd, gc, x, y+fm.underlinePos(),
			    tw, lw );
	if ( cfont.strikeOut() )		// draw strikeout effect
	    XFillRectangle( dpy, hd, gc, x, y-fm.strikeOutPos(),
			    tw, lw );
    }
}

/*!
  Returns the current position of the  pen.

  \sa moveTo()
 */
QPoint QPainter::pos() const
{
    return curPt;
}

