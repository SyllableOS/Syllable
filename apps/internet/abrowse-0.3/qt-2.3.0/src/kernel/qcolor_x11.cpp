/****************************************************************************
** $Id$
**
** Implementation of QColor class for X11
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

#include "qcolor.h"
#include "string.h"
#include "qpaintdevice.h"
#include "qapplication.h"
#include "qt_x11.h"

// NOT REVISED

/*****************************************************************************
  The color dictionary speeds up color allocation significantly for X11.
  When there are no more colors, QColor::alloc() will set the colors_avail
  flag to FALSE and try to find the nearest color.
  NOTE: From deep within the event loop, the colors_avail flag is reset to
  TRUE (calls the function qt_reset_color_avail()), because some other
  application might free its colors, thereby making them available for
  this Qt application.
 *****************************************************************************/

#include "qintdict.h"

struct QColorData {
    uint pix;					// allocated pixel value
    int	 context;				// allocation context
};

typedef QIntDict<QColorData> QColorDict;
typedef QIntDictIterator<QColorData> QColorDictIt;
static QColorDict *colorDict  = 0;		// dict of allocated colors

static bool	colors_avail  = TRUE;		// X colors available
static int	current_alloc_context = 0;	// current color alloc context
static bool	g_truecolor;			// truecolor visual
static Visual  *g_vis	= 0;			// visual
static XColor  *g_carr	= 0;			// color array
static bool	g_carr_fetch = TRUE;		// perform XQueryColors?
static int	g_cells = 0;			// number of entries in g_carr
static bool    *g_our_alloc = 0;		// our allocated colors
static uint	red_mask , green_mask , blue_mask;
static int	red_shift, green_shift, blue_shift;
static const uint col_std_dict = 419;
static const uint col_large_dict = 18397;

static bool	color_reduce = FALSE;
static int	col_div_r;
static int	col_div_g;
static int	col_div_b;

extern int	qt_ncols_option;		// qapplication_x11.cpp


/*
  This function is called from the event loop. It resets the colors_avail
  flag so that the application can retry to allocate read-only colors
  that other applications may have deallocated lately.

  The g_our_alloc and g_carr are global arrays that optimize color
  approximation when there are no more colors left to allocate.
*/

void qt_reset_color_avail()
{
    colors_avail = TRUE;
    g_carr_fetch = TRUE;		// do XQueryColors if !colors_avail
}


/*
  Finds the nearest color.
*/

static int find_nearest_color( int r, int g, int b, int* mindist_out )
{
    int mincol = -1;
    int mindist = 200000;
    int rx, gx, bx, dist;
    XColor *xc = &g_carr[0];
    for ( int i=0; i<g_cells; i++ ) {
	rx = r - (xc->red >> 8);
	gx = g - (xc->green >> 8);
	bx = b - (xc->blue>> 8);
	dist = rx*rx + gx*gx + bx*bx;		// calculate distance
	if ( dist < mindist ) {			// minimal?
	    mindist = dist;
	    mincol = i;
	}
	xc++;
    }
    *mindist_out = mindist;
    return mincol;
}


/*****************************************************************************
  QColor misc internal functions
 *****************************************************************************/

static int highest_bit( uint v )
{
    int i;
    uint b = (uint)1 << 31;			// get pos of highest bit in v
    for ( i=31; ((b & v) == 0) && i>=0;	 i-- )
	b >>= 1;
    return i;
}


/*****************************************************************************
  QColor static member functions
 *****************************************************************************/

/*!
  Returns the maximum number of colors supported by the underlying window
  system.
*/

int QColor::maxColors()
{
    return QPaintDevice::x11AppCells();
}

/*!
  Returns the number of color bit planes for the underlying window system.

  The returned values is equal to the default pixmap depth;

  \sa QPixmap::defaultDepth()
*/

int QColor::numBitPlanes()
{
    return QPaintDevice::x11AppDepth();
}


/*!
  Internal initialization required for QColor.
  This function is called from the QApplication constructor.
  \sa cleanup()
*/

void QColor::initialize()
{
    static const int blackIdx = 2;
    static const int whiteIdx = 3;

    if ( color_init )				// already initialized
	return;
    color_init = TRUE;

    Display *dpy  = QPaintDevice::x11AppDisplay();
    int      scr  = QPaintDevice::x11AppScreen();
    int	     ncols= QPaintDevice::x11AppCells();
    int	     spec = QApplication::colorSpec();

    g_vis = (Visual *)QPaintDevice::x11AppVisual();
    g_truecolor = g_vis->c_class == TrueColor;

    if ( !g_truecolor ) {
	// Create the g_our_alloc array, which remembers which color pixels
	// we allocated.
	g_cells = QMIN(ncols,256);
	g_carr  = new XColor[g_cells];
	CHECK_PTR( g_carr );
	memset( g_carr, 0, g_cells*sizeof(XColor) );
	g_carr_fetch = TRUE;		// run XQueryColors on demand
	g_our_alloc = new bool[g_cells];	
	CHECK_PTR( g_our_alloc );
	memset( g_our_alloc, FALSE, g_cells*sizeof(bool) );
	XColor *xc = &g_carr[0];
	for ( int i=0; i<g_cells; i++ ) {
	    xc->pixel = i;		// g_carr[i] = color i
	    xc++;
	}
    }

    int dictsize;
    if ( g_truecolor ) {			// truecolor
	dictsize    = 1;			// will not need color dict
	red_mask    = (uint)g_vis->red_mask;
	green_mask  = (uint)g_vis->green_mask;
	blue_mask   = (uint)g_vis->blue_mask;
	red_shift   = highest_bit( red_mask )	- 7;
	green_shift = highest_bit( green_mask ) - 7;
	blue_shift  = highest_bit( blue_mask )	- 7;
    } else {
	dictsize = col_std_dict;
    }
    colorDict = new QColorDict(dictsize);	// create dictionary
    CHECK_PTR( colorDict );

  // Initialize global color objects

    globalColors()[blackIdx].rgbVal = qRgb( 0, 0, 0 ) & RGB_MASK;
    globalColors()[whiteIdx].rgbVal = qRgb( 255, 255, 255 ) & RGB_MASK;
    if ( QPaintDevice::x11AppDefaultVisual() &&
	 QPaintDevice::x11AppDefaultColormap() ) {
	globalColors()[blackIdx].pix = (uint)BlackPixel( dpy, scr );
	globalColors()[whiteIdx].pix = (uint)WhitePixel( dpy, scr );
    } else {
	globalColors()[blackIdx].alloc();
	globalColors()[whiteIdx].alloc();
    }

    if ( spec == (int)QApplication::ManyColor ) {
	color_reduce = TRUE;

	switch ( qt_ncols_option ) {
	  case 216:
	    // 6:6:6
	    col_div_r = col_div_g = col_div_b = (255/(6-1));
	    break;
	  default: {
	    // 2:3:1 proportions, solved numerically
	    if ( qt_ncols_option > 255 ) qt_ncols_option = 255;
	    if ( qt_ncols_option < 1 ) qt_ncols_option = 1;
	    int nr = 2;
	    int ng = 2;
	    int nb = 2;
	    while ( 1 ) {
		if ( nb*2 < nr && (nb+1)*nr*ng < qt_ncols_option )
		    nb++;
		else if ( nr*3 < ng*2 && nb*(nr+1)*ng < qt_ncols_option )
		    nr++;
		else if ( nb*nr*(ng+1) < qt_ncols_option )
		    ng++;
		else break;
	    }
	    qt_ncols_option = nr*ng*nb;
	    col_div_r = (255/(nr-1));
	    col_div_g = (255/(ng-1));
	    col_div_b = (255/(nb-1));
	  }
	}
    }

#if 0 /* 0 == allocate colors on demand */
    setLazyAlloc( FALSE );			// allocate global colors
    ((QColor*)(&darkGray))->	alloc();
    ((QColor*)(&gray))->	alloc();
    ((QColor*)(&lightGray))->	alloc();
    ((QColor*)(&::red))->	alloc();
    ((QColor*)(&::green))->	alloc();
    ((QColor*)(&::blue))->	alloc();
    ((QColor*)(&cyan))->	alloc();
    ((QColor*)(&magenta))->	alloc();
    ((QColor*)(&yellow))->	alloc();
    ((QColor*)(&darkRed))->	alloc();
    ((QColor*)(&darkGreen))->	alloc();
    ((QColor*)(&darkBlue))->	alloc();
    ((QColor*)(&darkCyan))->	alloc();
    ((QColor*)(&darkMagenta))-> alloc();
    ((QColor*)(&darkYellow))->	alloc();
    setLazyAlloc( TRUE );
#endif
}

/*!
  Internal clean up required for QColor.
  This function is called from the QApplication destructor.
  \sa initialize()
*/

void QColor::cleanup()
{
    if ( !color_init )
	return;
    color_init = FALSE;
    if ( g_carr ) {
	delete [] g_carr;
	g_carr = 0;
    }
    if ( g_our_alloc ) {
	delete [] g_our_alloc;
	g_our_alloc = 0;
    }
    if ( colorDict ) {
	colorDict->setAutoDelete( TRUE );
	colorDict->clear();
	delete colorDict;
	colorDict = 0;
    }
}


/*****************************************************************************
  QColor member functions
 *****************************************************************************/

/*!
  Allocates the RGB color and returns the pixel value.

  Allocating a color means to obtain a pixel value from the RGB
  specification.  The pixel value is an index into the global color
  table, but should be considered an arbitrary platform-dependent value.

  The pixel() function calls alloc() if necessary, so in general you
  don't need to call this function.

  \sa setLazyAlloc(), enterAllocContext()
*/

uint QColor::alloc()
{
    Display *dpy = QPaintDevice::x11AppDisplay();
    int      scr = QPaintDevice::x11AppScreen();
    if ( (rgbVal & RGB_INVALID) || !color_init ) {
	rgbVal = 0;				// invalid color or state
	pix = dpy ? (uint)BlackPixel(dpy, scr) : 0;
	return pix;
    }
    int r = qRed(rgbVal);
    int g = qGreen(rgbVal);
    int b = qBlue(rgbVal);
    if ( g_truecolor ) {			// truecolor: map to pixel
	r = red_shift	> 0 ? r << red_shift   : r >> -red_shift;
	g = green_shift > 0 ? g << green_shift : g >> -green_shift;
	b = blue_shift	> 0 ? b << blue_shift  : b >> -blue_shift;
	pix = (b & blue_mask) | (g & green_mask) | (r & red_mask);
	rgbVal &= RGB_MASK;
	return pix;
    }
    QColorData *c = colorDict->find( (long)(rgbVal&RGB_MASK) );
    if ( c ) {					// found color in dictionary
	rgbVal &= RGB_MASK;			// color ok
	pix = c->pix;				// use same pixel value
	if ( c->context != current_alloc_context ) {
	    c->context = 0;			// convert to default context
	    g_our_alloc[pix] = TRUE;		// reuse without XAllocColor
	}
	return pix;
    }

    XColor col;
    col.red   = r << 8;
    col.green = g << 8;
    col.blue  = b << 8;

    bool try_again = FALSE;
    bool try_alloc = !color_reduce;
    int  try_count = 0;

    do {
	// This loop is run until we manage to either allocate or
	// find an approximate color, it stops after a few iterations.

	try_again = FALSE;

	if ( try_alloc && colors_avail &&
	     XAllocColor(dpy,QPaintDevice::x11AppColormap(),&col) ) {

	    // We could allocate the color
	    pix = (uint)col.pixel;
	    rgbVal &= RGB_MASK;
	    g_carr[pix] = col;			// update color array
	    if ( current_alloc_context == 0 )
		g_our_alloc[pix] = TRUE;	// reuse without XAllocColor

	} else {
	    // No available colors, or we did not want to allocate one
	    int i;
	    colors_avail = FALSE;		// no more available colors
	    if ( g_carr_fetch ) {		// refetch color array
		g_carr_fetch = FALSE;
		XQueryColors( dpy, QPaintDevice::x11AppColormap(), g_carr,
			      g_cells );
	    }
	    int mindist;
	    i = find_nearest_color( r, g, b, &mindist );

	    if ( mindist != 0 && !try_alloc ) {
		// Not an exact match with an existing color
		int rr = ((r+col_div_r/2)/col_div_r)*col_div_r;
		int rg = ((g+col_div_g/2)/col_div_g)*col_div_g;
		int rb = ((b+col_div_b/2)/col_div_b)*col_div_b;
		int rx = rr - r;
		int gx = rg - g;
		int bx = rb - b;
		int dist = rx*rx + gx*gx + bx*bx; // calculate distance
		if ( dist < mindist ) {
		    // reduced color is closer - try to alloc it
		    r = rr;
		    g = rg;
		    b = rb;
		    col.red   = r << 8;
		    col.green = g << 8;
		    col.blue  = b << 8;
		    try_alloc = TRUE;
		    try_again = TRUE;
		    colors_avail = TRUE;
		    continue; // Try alloc reduced color
		}
	    }

	    if ( i == -1 ) {			// no nearest color?!
		rgbVal |= RGB_INVALID;
		pix = (uint)BlackPixel( dpy, scr );
		return pix;
	    }
	    if ( g_our_alloc[i] ) {		// we've already allocated it
		; // i == g_carr[i].pixel
	    } else {
		// Try to allocate existing color
		col = g_carr[i];
		if ( XAllocColor(dpy,QPaintDevice::x11AppColormap(), &col) ) {
		    i = (uint)col.pixel;
		    g_carr[i] = col;		// update color array
		    if ( current_alloc_context == 0 )
			g_our_alloc[i] = TRUE;	// only in the default context
		} else {
		    // Oops, it's gone again
		    try_count++;
		    try_again    = TRUE;
		    colors_avail = TRUE;
		    g_carr_fetch = TRUE;
		}
	    }
	    if ( !try_again ) {			// got it
		pix = (uint)g_carr[i].pixel;	// allocated X11 color
		rgbVal &= RGB_MASK;
	    }
	}

    } while ( try_again && try_count < 2 );

    if ( try_again ) {				// no hope of allocating color
	rgbVal |= RGB_INVALID;
	pix = (uint)BlackPixel( dpy, scr );
	return pix;
    }
    // All colors outside context 0 must go into the dictionary
    bool many = colorDict->count() >= colorDict->size() * 8;
    if ( many && colorDict->size() == col_std_dict ) {
	colorDict->resize( col_large_dict );
    }
    if ( !many || current_alloc_context != 0 ) {
	c = new QColorData;			// insert into color dict
	CHECK_PTR( c );
	c->pix	   = pix;
	c->context = current_alloc_context;
	colorDict->insert( (long)rgbVal, c );	// store color in dict
    }
    return pix;
}


void QColor::setSystemNamedColor( const QString& name )
{
    if ( !color_init ) {
#if defined(CHECK_STATE)
	qWarning( "QColor::setSystemNamedColor: Cannot perform this operation "
		 "because QApplication does not exist" );
#endif
	alloc();				// makes the color black
	return;
    }
    XColor col, hw_col;
    if ( XLookupColor(QPaintDevice::x11AppDisplay(),
		      QPaintDevice::x11AppColormap(), name.latin1(),
		      &col, &hw_col) ) {
	setRgb( col.red>>8, col.green>>8, col.blue>>8 );
	return;					// success
    }
    // The name lookup failed if we got here
    if ( lazy_alloc ) {
	rgbVal = RGB_INVALID | RGB_DIRTY;
	pix = 0;
    } else {
	rgbVal = RGB_INVALID;
	alloc();
    }
}


#define MAX_CONTEXTS 16
static int  context_stack[MAX_CONTEXTS];
static int  context_ptr = 0;

static void init_context_stack()
{
    static bool did_init = FALSE;
    if ( !did_init ) {
	did_init = TRUE;
	context_stack[0] = current_alloc_context = 0;
    }
}


/*!
  Enters a color allocation context and returns a nonzero unique identifier.

  Color allocation contexts are useful for programs that need to
  allocate many colors and throw them away later, like image viewers.
  The allocation context functions work for true color displays as
  well as colormap display, except that QColor::destroyAllocContext()
  does nothing for true color.

  Example:
  \code
    QPixmap loadPixmap( QString fileName )
    {
	static int alloc_context = 0;
	if ( alloc_context )
	    QColor::destroyAllocContext( alloc_context );
	alloc_context = QColor::enterAllocContext();
	QPixmap pm( fileName );
	QColor::leaveAllocContext();
	return pm;
    }
  \endcode

  The example code loads a pixmap from file. It frees up all colors
  that were allocated the last time loadPixmap() was called.

  The initial/default context is 0. Qt keeps a list of colors
  associated with their allocation contexts. You can call
  destroyAllocContext() to get rid of all colors that were allocated
  in a specific context.

  Calling enterAllocContext() enters an allocation context. The
  allocation context lasts until you call leaveAllocContext(). QColor
  has an internal stack of allocation contexts. Each call to
  enterAllocContex() must have a corresponding leaveAllocContext().

  \code
      // context 0 active
    int c1 = QColor::enterAllocContext();	// enter context c1
      // context c1 active
    int c2 = QColor::enterAllocContext();	// enter context c2
      // context c2 active
    QColor::leaveAllocContext();		// leave context c2
      // context c1 active
    QColor::leaveAllocContext();		// leave context c1
      // context 0 active
      // Now, free all colors that were allocated in context c2
    QColor::destroyAllocContext( c2 );
  \endcode

  You may also want to set the application's color specification.
  See QApplication::setColorSpec() for more information.

  \sa leaveAllocContext(), currentAllocContext(), destroyAllocContext(),
  QApplication::setColorSpec()
*/

int QColor::enterAllocContext()
{
    static int context_seq_no = 0;
    init_context_stack();
    if ( context_ptr+1 == MAX_CONTEXTS ) {
#if defined(CHECK_STATE)
	qWarning( "QColor::enterAllocContext: Context stack overflow" );
#endif
	return 0;
    }
    current_alloc_context = context_stack[++context_ptr] = ++context_seq_no;
    return current_alloc_context;
}


/*!
  Leaves a color allocation context.

  See enterAllocContext() for a detailed explanation.

  \sa enterAllocContext(), currentAllocContext()
*/

void QColor::leaveAllocContext()
{
    init_context_stack();
    if ( context_ptr == 0 ) {
#if defined(CHECK_STATE)
	qWarning( "QColor::leaveAllocContext: Context stack underflow" );
#endif
	return;
    }
    current_alloc_context = context_stack[--context_ptr];
}


/*!
  Returns the current color allocation context.

  The default context is 0.

  \sa enterAllocContext(), leaveAllocContext()
*/

int QColor::currentAllocContext()
{
    return current_alloc_context;
}


/*!
  Destroys a color allocation context, \e context.

  This function deallocates all colors that were allocated in the
  specified \a context.
  If \a context == -1, it frees up all colors
  that the application has allocated.
  If \a context == -2, it frees up all colors
  that the application has allocated, except those in the
  default context.

  The function does nothing for true color displays.

  \sa enterAllocContext(), alloc()
*/

void QColor::destroyAllocContext( int context )
{
    init_context_stack();
    if ( !color_init || g_truecolor )
	return;
    ulong pixels[256];
    bool freeing[256];
    memset( freeing, FALSE, g_cells*sizeof(bool) );
    QColorData   *d;
    QColorDictIt it( *colorDict );
    int i = 0;
    uint rgbVal;
    while ( (d=it.current()) ) {
	rgbVal = (uint)it.currentKey();
	if ( (d->context || context==-1) &&
	     (d->context == context || context < 0) )
	{
	    if ( !g_our_alloc[d->pix] && !freeing[d->pix] )
	    {
		// will free this color
		pixels[i++] = d->pix;
		freeing[d->pix] = TRUE;
	    }
	    colorDict->remove( (long)rgbVal );	// remove from dict
	}
	++it;
    }
    if ( i )
	XFreeColors( QPaintDevice::x11AppDisplay(),
		     QPaintDevice::x11AppColormap(),
		     pixels, i, 0 );
}
