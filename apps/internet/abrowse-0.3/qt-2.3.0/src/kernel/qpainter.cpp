/****************************************************************************
** $Id$
**
** Implementation of QPainter, QPen and QBrush classes
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

#include "qpainter.h"
#include "qpainter_p.h"
#include "qbitmap.h"
#include "qstack.h"
#include "qdatastream.h"
#include "qwidget.h"
#include "qimage.h"
#include "q1xcompatibility.h"
#include "qpaintdevicemetrics.h"
#ifdef _WS_QWS_
#include "qgfx_qws.h"
#endif
#include <stdlib.h>

#ifndef QT_NO_TRANSFORMATIONS
typedef QStack<QWMatrix> QWMatrixStack;
#endif

// REVISED: arnt
/*!
  \class QPainter qpainter.h
  \brief The QPainter class paints on paint devices.

  \ingroup drawing

  The painter provides efficient graphics rendering on any
  QPaintDevice object. QPainter can draw everything from simple lines
  to complex shapes like pies and chords. It can also draw aligned
  text and pixmaps.  Normally, it draws in a "natural" coordinate
  system, but it can also do view and world transformation.

  The typical use of a painter is:

  <ol>
  <li> Construct a painter.
  <li> Set a pen, a brush etc.
  <li> Draw.
  <li> Destroy the painter.
  </ol>

  Mostly, all this is done inside a paint event.  (In fact, 99% of all
  QPainter use is in a reimplementation of QWidget::paintEvent()).
  Here's one very simple example:

  \code
    void SimpleExampleWidget::paintEvent()
    {
	QPainter paint( this );
	paint.setPen( Qt::blue );
	paint.drawText( rect(), AlignCenter, "The Text" );
    }
  \endcode

  Simple. However, there are many settings you may use: <ul>

  <li> font() is the currently set font.  If you set a font that isn't
  available, Qt finds a close match.  In that case font() returns what
  you set using setFont() and fontInfo() returns the font actually
  being used.

  <li> brush() is the currently set brush; the color or pattern
  that's used for filling e.g. circles.

  <li> pen() is the currently set pen; the color or stipple that's
  used for drawing lines or boundaries.

  <li> backgroundMode() is \c Opaque or \c Transparent, ie. whether
  backgroundColor() is used or not.

  <li> backgroundColor() only applies when backgroundMode() is Opaque
  and pen() is a stipple.  In that case, it describes the color of the
  background pixels in the stipple.

  <li> rasterOp() is how pixels drawn interact with the data already
  there.

  <li> brushOrigin() is the origin of the tiled brushes, normally the
  origin of the window.

  <li> viewport(), window(), worldMatrix() and many more make up the
  painter's coordinate transformation system.  See \link coordsys.html
  The Coordinate System \endlink for an explanation of this, or a
  paragraph below for a quick overview of the functions.

  <li> clipping() is whether the painter clips at all. (The paint
  device clips, too.)  If the painter clips, it clips to clipRegion().

  <li> pos() is the current position, set by moveTo() and used by
  lineTo().

  </ul>

  Note that some of these settings mirror settings in some paint
  devices, e.g. QWidget::font(). QPainter::begin() (or the QPainter
  constructor) copies these attributes from the paint device, changing
  calling e.g. QWidget::setFont() doesn't take effect until the next
  time a painter begins painting on it.

  save() saves all of these settings on an internal stack, restore()
  pops them back.

  The core functionality of QPainter is drawing, and there are
  functions to draw most primitives: drawPoint(), drawPoints(),
  drawLine(), drawRect(), drawWinFocusRect(), drawRoundRect(),
  drawEllipse(), drawArc(), drawPie(), drawChord(),
  drawLineSegments(), drawPolyline(), drawPolygon(), and
  drawQuadBezier().

  There are functions to draw pixmaps/images, namely drawPixmap(),
  drawImage() and drawTiledPixmap().  drawPixmap() and drawImage()
  produce the same result, except that drawPixmap() is faster
  on-screen and drawImage() faster and sometimes better on QPrinter
  and QPicture.

  Text drawing is done using drawText(), and when you need
  fine-grained positioning, boundingRect() tells you where a given
  drawText() command would draw.

  There is a drawPicture() that draws the contents of an entire
  QPicture using this painter.  drawPicture() is the only function
  that disregards all the painter's settings: The QPicture has its own
  settings.

  Normally, the QPainter operates on the device's own coordinate
  system (usually pixels), but QPainter has good support for
  coordinate transformation.  See \link coordsys.html The Coordinate
  System \endlink for a more general overview and a walkthrough of a
  simple example.

  The most common functions used are scale(), rotate(), translate()
  and shear(), all of which operate on the worldMatrix().
  setWorldMatrix() can replace or add to the currently set matrix().

  setViewport() sets the rectangle on which QPainter operates.  The
  default is the entire device, which is usually fine, except on
  printers.  setWindow() sets the coordinate system, that is, the
  rectangle that maps to viewport().  What's draws inside the window()
  ends up being inside the viewport().  The window's default is the
  same as the viewport, and if you don't use the transformations, they
  are optimized away, gaining a little speed.

  After all the coordinate transformation is done, QPainter can clip
  the drawing to and arbitrary rectangle or region.  hasClipping() is
  TRUE if QPainter clips, and clipRegion() returns the clip region.
  You can set it using either setClipRegion() or setClipRect().  Note
  that the clipping can be slow.  It's all system-dependent, but as a
  rule of thumb, you can assume that drawing speed is inversely
  proportional to the number of rectangles in the clip region.

  After QPainter's clipping, the paint device too will clip a bit.
  For example, most widgets clip away the pixels used by child
  widgets, and most printers clip away an area near the edges of the
  paper.  This additional clipping is \e not reflected by the return
  value of clipRegion() or hasClipping().

  Finally, QPainter includes some little-used functions that are very
  handy the few times you need them.

  isActive() indicates whether the painter is active.  begin() (and
  the most usual constructor) makes it active.  end() (and the
  destructor) deactivates it.  If the painter is active, device()
  returns the paint device on which the painter paints.

  Sometimes it is desirable to make someone else paint on an unusual
  QPaintDevice.  QPainter supports a static function to do this,
  redirect().  We recommend not using it, but for some hacks it's
  perfect.

  setTabStops() and setTabArray() can change where the
  tab stops are, but these are very seldomly used.

  \warning Note that QPainter does not attempt to work around
  coordinate limitations in the underlying window system.  Some
  platforms may behave incorrectly with coordinates as small as +/-
  4000.

  \header qdrawutil.h

  \sa QPaintDevice QWidget QPixmap QPrinter QPicture
  \link simple-application.html Application Walkthrough \endlink
  \link coordsys.html Coordinate System Overview \endlink
*/

/*! \enum Qt::RasterOp

  This enum type is used to describe the way things are written to the
  paint device. Each bit of the \e src (what you write) interacts with
  the corresponding bit of the \e dst pixel.  The currently defined
  values are: <ul>

  <li> \c CopyROP - dst = src
  <li> \c OrROP -  dst = src OR dst
  <li> \c XorROP -  dst = src XOR dst
  <li> \c NotAndROP - dst = (NOT src) AND dst
  <li> \c EraseROP - an alias for \c NotAndROP
  <li> \c NotCopyROP - dst = NOT src
  <li> \c NotOrROP - dst = (NOT src) OR dst
  <li> \c NotXorROP - dst = (NOT src) XOR dst
  <li> \c AndROP - dst = src AND dst
  <li> \c NotEraseROP - an alias for \c AndROP
  <li> \c NotROP - dst = NOT dst
  <li> \c ClearROP - dst = 0
  <li> \c SetROP - dst = 1
  <li> \c NopROP - dst = dst
  <li> \c AndNotROP - dst = src AND (NOT dst)
  <li> \c OrNotROP - dst = src OR (NOT dst)
  <li> \c NandROP - dst = NOT (src AND dst)
  <li> \c NorROP - dst = NOT (src OR dst)

  </ul>

  By far the most useful ones are \c CopyROP and \c XorROP.
*/

/*! \enum Qt::AlignmentFlags

  This enum type is used to describe alignment.  It contains four sets
  of flags: Horizontal, vertical and modifying flags.  The horizontal
  flags are: <ul>

  <li> \c AlignLeft - Align with the left edge.
  <li> \c AlignRight - Align with the right edge.
  <li> \c AlignHCenter - Center horizontally in the available space.

  </ul> The vertical flags are: <ul>

  <li> \c AlignTop - Align with the top.
  <li> \c AlignBottom - Align with the bottom.
  <li> \c AlignVCenter - Center vertically in the available space.

  </ul> You can only use one of the horizontal flags at a time.  There
  is one two-dimensional flag: <ul>

  <li> \c AlignCenter - Center in both dimensions.

  </ul> This counts both as a horizontal and vertical flag: It cannot
  be combined with any other horizontal or vertical flags.

  There are also some modifier flags.  All of them apply only to
  printing: <ul>

  <li> \c SingleLine - Treat all white-space as space and print just
  one line.

  <li> \c DontClip - If it's impossible to stay within the given
  bounds, print outside.

  <li> \c ExpandTabs - Make the U+0009 (ascii tab) character move to
  the next tab stop.

  <li> \c ShowPrefix - Display the string "\&P" as an underlined P
  (see QButton for an example).  To get an ampersand, use "\&\&".

  <li> \c WordBreak - Do line breaking at at appropriate points.

  </ul>

  You can only use one of the horizontal flags at a time, and one of
  the vertical flags.  \c AlignCenter counts as both horizontal and
  vertical.  You can use as many modifier flags as you want, except
  that \c SingleLine and \c WordBreak cannot be combined.

  Flags that are inappropriate for a given use (e.g. ShowPrefix to
  QGridLayout::addWidget()) are ignored.

  Conflicting combinations of flags have undefined meanings.
*/


/*! \enum Qt::PenStyle

  This enum type defines the pen styles supported by Qt; ie. what
  sorts of lines that can be drawn using QPainter. The current styles
  are: <ul>

  <li> \c NoPen - no line at all.  For example, QPainter::drawRect()
  fills but does not draw any explicit boundary line.

  <li> \c SolidLine - a simple line.

  <li> \c DashLine - dashes, separated by a few pixels.

  <li> \c DotLine - dots, separated by a few pixels.

  <li> \c DashDotLine - alternately dots and dashes.

  <li> \c DashDotDotLine - one dash, two dots, one dash, two dots...

  </ul>
*/

/*! \enum Qt::PenCapStyle

  This enum type defines the pen cap styles supported by Qt; ie. what
  sorts of line end caps that can be drawn using QPainter. The
  available styles are: <ul>

  <li> \c FlatCap - A square line end that does not cover the end
  point of the line.

  <li> \c SquareCap - A square line end that covers the end point and
  extends beyond it with half the line width.

  <li> \c RoundCap - A rounded line end.

  </ul>
*/

/*! \enum Qt::PenJoinStyle

  This enum type defines the pen join styles supported by Qt; ie. what
  sorts of joins between two connected lines that can be drawn using
  QPainter. The available styles are: <ul>

  <li> \c MiterJoin - The outer edges of the lines are extended to
  meet at an angle, and this area is filled.

  <li> \c BevelJoin - The triangular notch between the two lines is filled.

  <li> \c RoundJoin - A circular arc between the two lines is filled.

  </ul>
*/

/*!
  Constructs a painter.

  Notice that all painter settings (setPen,setBrush etc.) are reset to
  default values when begin() is called.

  \sa begin(), end()
*/

QPainter::QPainter()
{
    init();
}


/*!
  Constructs a painter that begins painting the paint device \a pd
  immediately.

  This constructor is convenient for short-lived painters, e.g. in
  a \link QWidget::paintEvent() paint event\endlink and should be
  used only once. The constructor calls begin() for you and the QPainter
  destructor automatically calls end().

  Here's an example using begin() and end():
  \code
    void MyWidget::paintEvent( QPaintEvent * )
    {
	QPainter p;
	p.begin( this );
	p.drawLine( ... );	// drawing code
	p.end();
    }
  \endcode

  The same example using this constructor:
  \code
    void MyWidget::paintEvent( QPaintEvent * )
    {
	QPainter p( this );
	p.drawLine( ... );	// drawing code
    }
  \endcode

  \sa begin(), end()
*/

QPainter::QPainter( const QPaintDevice *pd )
{
    init();
    if ( begin( pd ) )
	flags |= CtorBegin;
}


/*!
  Constructs a painter that begins painting the paint device \a pd
  immediately, with the default arguments taken from \a copyAttributes.

  \sa begin()
*/

QPainter::QPainter( const QPaintDevice *pd,
		    const QWidget *copyAttributes )
{
    init();
    if ( begin( pd, copyAttributes ) )
	flags |= CtorBegin;
}


/*!
  Destructs the painter.
*/

QPainter::~QPainter()
{
    if ( isActive() )
	end();
    if ( tabarray )				// delete tab array
	delete [] tabarray;
#ifndef QT_NO_TRANSFORMATIONS
    if ( wm_stack )
	delete (QWMatrixStack *)wm_stack;
#endif
}


/*!
  \overload bool QPainter::begin( const QPaintDevice *pd, const QWidget *copyAttributes )

  This version opens the painter on a paint device \a pd and sets the initial
  pen, background color and font from \a copyAttributes.  This is equivalent
  with:
  \code
    QPainter p;
    p.begin( pd );
    p.setPen( copyAttributes->foregroundColor() );
    p.setBackgroundColor( copyAttributes->backgroundColor() );
    p.setFont( copyAttributes->font() );
  \endcode

  This begin function is convenient for double buffering.  When you
  draw in a pixmap instead of directly in a widget (to later bitBlt
  the pixmap into the widget) you will need to set the widgets's
  font etc.  This function does exactly that.

  Example:
  \code
    void MyWidget::paintEvent( QPaintEvent * )
    {
	QPixmap pm(size());
	QPainter p;
	p.begin(&pm, this);
	// ... potential flickering paint operation ...
	p.end();
	bitBlt(this, 0, 0, &pm);
    }
  \endcode

  \sa end()
*/

bool QPainter::begin( const QPaintDevice *pd, const QWidget *copyAttributes )
{
    if ( copyAttributes == 0 ) {
#if defined(CHECK_NULL)
	qWarning( "QPainter::begin: The widget to copy attributes from cannot "
		 "be null" );
#endif
	return FALSE;
    }
    if ( begin(pd) ) {
	setPen( copyAttributes->foregroundColor() );
	setBackgroundColor( copyAttributes->backgroundColor() );
	setFont( copyAttributes->font() );
	return TRUE;
    }
    return FALSE;
}


/*!
  \internal
  Sets or clears a pointer flag.
*/

void QPainter::setf( uint b, bool v )
{
    if ( v )
	setf( b );
    else
	clearf( b );
}


/*!
  \fn bool QPainter::isActive() const

  Returns TRUE if the painter is active painting, i.e. begin() has
  been called and end() has not yet been called.

  \sa QPaintDevice::paintingActive()
*/

/*!
  \fn QPaintDevice *QPainter::device() const

  Returns the paint device on which this painter is currently
  painting, or null if the painter is not active.

  \sa QPaintDevice::paintingActive()
*/


struct QPState {				// painter state
    QFont	font;
    QPen	pen;
    QBrush	brush;
    QColor	bgc;
    uchar	bgm;
    uchar	pu;
    uchar	rop;
    QPoint	bro;
    QRect	wr, vr;
#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix	wm;
#else
    int		xlatex;
    int		xlatey;
#endif
    bool	vxf;
    bool	wxf;
    QRegion	rgn;
    bool	clip;
    int		ts;
    int	       *ta;
    void* wm_stack;
};

//TODO lose the worldmatrix stack

typedef QStack<QPState> QPStateStack;


void QPainter::killPStack()
{
    delete (QPStateStack *)ps_stack;
    ps_stack = 0;
}

/*!
  Saves the current painter state (pushes the state onto a stack).  A
  save() must be followed by a corresponding restore().  end() unwinds
  the stack().

  \sa restore()
*/

void QPainter::save()
{
    if ( testf(ExtDev) ) {
	if ( testf(DirtyFont) )
	    updateFont();
	if ( testf(DirtyPen) )
	    updatePen();
	if ( testf(DirtyBrush) )
	    updateBrush();
	pdev->cmd( QPaintDevice::PdcSave, this, 0 );
    }
    QPStateStack *pss = (QPStateStack *)ps_stack;
    if ( pss == 0 ) {
	pss = new QStack<QPState>;
	CHECK_PTR( pss );
	pss->setAutoDelete( TRUE );
	ps_stack = pss;
    }
    QPState *ps = new QPState;
    CHECK_PTR( ps );
    ps->font  = cfont;
    ps->pen   = cpen;
    ps->brush = cbrush;
    ps->bgc   = bg_col;
    ps->bgm   = bg_mode;
    ps->rop   = rop;
    ps->bro   = bro;
#if 0
    ps->pu    = pu;				// !!!not used
#endif
#ifndef QT_NO_TRANSFORMATIONS
    ps->wr    = QRect( wx, wy, ww, wh );
    ps->vr    = QRect( vx, vy, vw, vh );
    ps->wm    = wxmat;
    ps->vxf   = testf(VxF);
    ps->wxf   = testf(WxF);
#else
    ps->xlatex = xlatex;
    ps->xlatey = xlatey;
#endif
    ps->rgn   = crgn;
    ps->clip  = testf(ClipOn);
    ps->ts    = tabstops;
    ps->ta    = tabarray;
    ps->wm_stack = wm_stack;
    wm_stack = 0;
    pss->push( ps );
}

/*!
  Restores the current painter state (pops a saved state off the stack).
  \sa save()
*/

void QPainter::restore()
{
    if ( testf(ExtDev) ) {
	pdev->cmd( QPaintDevice::PdcRestore, this, 0 );
    }
    QPStateStack *pss = (QPStateStack *)ps_stack;
    if ( pss == 0 || pss->isEmpty() ) {
#if defined(CHECK_STATE)
	qWarning( "QPainter::restore: Empty stack error" );
#endif
	return;
    }
    QPState *ps = pss->pop();
    bool hardRestore = testf(VolatileDC);

    if ( ps->font != cfont || hardRestore )
	setFont( ps->font );
    if ( ps->pen != cpen || hardRestore )
	setPen( ps->pen );
    if ( ps->brush != cbrush || hardRestore )
	setBrush( ps->brush );
    if ( ps->bgc != bg_col || hardRestore )
	setBackgroundColor( ps->bgc );
    if ( ps->bgm != bg_mode || hardRestore )
	setBackgroundMode( (BGMode)ps->bgm );
    if ( ps->rop != rop || hardRestore )
	setRasterOp( (RasterOp)ps->rop );
#if 0
    if ( ps->pu != pu )				// !!!not used
	pu = ps->pu;
#endif
#ifndef QT_NO_TRANSFORMATIONS
    QRect wr( wx, wy, ww, wh );
    QRect vr( vx, vy, vw, vh );
    if ( ps->wr != wr || hardRestore )
	setWindow( ps->wr );
    if ( ps->vr != vr || hardRestore )
	setViewport( ps->vr );
    if ( ps->wm != wxmat || hardRestore )
	setWorldMatrix( ps->wm );
    if ( ps->vxf != testf(VxF) || hardRestore )
	setViewXForm( ps->vxf );
    if ( ps->wxf != testf(WxF) || hardRestore )
	setWorldXForm( ps->wxf );
#else
    xlatex = ps->xlatex;
    xlatey = ps->xlatey;
#endif
    if ( ps->rgn != crgn || hardRestore )
	setClipRegion( ps->rgn );
    if ( ps->clip != testf(ClipOn) || hardRestore )
	setClipping( ps->clip );
    tabstops = ps->ts;
    tabarray = ps->ta;

#ifndef QT_NO_TRANSFORMATIONS
    if (wm_stack )
	delete (QWMatrixStack *)wm_stack;
    wm_stack = ps->wm_stack;
#endif
    delete ps;
}


/*!
  Returns the font metrics for the painter, if the painter is active.
  It is not possible to obtain metrics for an inactive painter, so the
  return value is undefined if the painter is not active.

  \sa fontInfo(), isActive()
*/

QFontMetrics QPainter::fontMetrics() const
{
    if ( pdev && pdev->devType() == QInternal::Picture )
	return QFontMetrics( cfont );

    return QFontMetrics(this);
}

/*!
  Returns the font info for the painter, if the painter is active.  It
  is not possible to obtain font information for an inactive painter,
  so the return value is undefined if the painter is not active.

  \sa fontMetrics(), isActive()
*/

QFontInfo QPainter::fontInfo() const
{
    if ( pdev && pdev->devType() == QInternal::Picture )
	return QFontInfo( cfont );

    return QFontInfo(this);
}


/*!
  \fn const QPen &QPainter::pen() const

  Returns the current pen for the painter.

  \sa setPen()
*/

/*!
  Sets a new painter pen.

  The pen defines how to draw lines and outlines, and it also defines
  the text color.

  \sa pen()
*/

void QPainter::setPen( const QPen &pen )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setPen: Will be reset by begin()" );
#endif
    cpen = pen;
    updatePen();
}

/*!
  Sets a new painter pen with style \c style, width 0 and black color.
  \sa pen(), QPen
*/

void QPainter::setPen( PenStyle style )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setPen: Will be reset by begin()" );
#endif
    QPen::QPenData *d = cpen.data;	// low level access
    if ( d->count != 1 ) {
	cpen.detach();
	d = cpen.data;
    }
    d->style = style;
    d->width = 0;
    d->color = Qt::black;
    d->linest = style;
    updatePen();
}

/*!
  Sets a new painter pen with style \c SolidLine, width 0 and the specified
  \a color.
  \sa pen(), QPen
*/

void QPainter::setPen( const QColor &color )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setPen: Will be reset by begin()" );
#endif
    QPen::QPenData *d = cpen.data;	// low level access
    if ( d->count != 1 ) {
	cpen.detach();
	d = cpen.data;
    }
    d->style = SolidLine;
    d->width = 0;
    d->color = color;
    d->linest = SolidLine;
    updatePen();
}

/*!
  \fn const QBrush &QPainter::brush() const
  Returns the current painter brush.
  \sa QPainter::setBrush()
*/

/*!
  Sets a new painter brush.

  The brush defines how to fill shapes.

  \sa brush()
*/

void QPainter::setBrush( const QBrush &brush )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setBrush: Will be reset by begin()" );
#endif
    cbrush = brush;
    updateBrush();
}

/*!
  Sets a new painter brush with black color and the specified \a style.
  \sa brush(), QBrush
*/

void QPainter::setBrush( BrushStyle style )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setBrush: Will be reset by begin()" );
#endif
    QBrush::QBrushData *d = cbrush.data; // low level access
    if ( d->count != 1 ) {
	cbrush.detach();
	d = cbrush.data;
    }
    d->style = style;
    d->color = Qt::black;
    if ( d->pixmap ) {
	delete d->pixmap;
	d->pixmap = 0;
    }
    updateBrush();
}

/*!
  Sets a new painter brush with the style \c SolidPattern and the specified
  \a color.
  \sa brush(), QBrush
*/

void QPainter::setBrush( const QColor &color )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setBrush: Will be reset by begin()" );
#endif
    QBrush::QBrushData *d = cbrush.data; // low level access
    if ( d->count != 1 ) {
	cbrush.detach();
	d = cbrush.data;
    }
    d->style = SolidPattern;
    d->color = color;
    if ( d->pixmap ) {
	delete d->pixmap;
	d->pixmap = 0;
    }
    updateBrush();
}


/*!
  \fn const QColor &QPainter::backgroundColor() const
  Returns the current background color.
  \sa setBackgroundColor() QColor
*/

/*!
  \fn BGMode QPainter::backgroundMode() const
  Returns the current background mode.
  \sa setBackgroundMode() BGMode
*/

/*!
  \fn RasterOp QPainter::rasterOp() const
  Returns the current raster operation.
  \sa setRasterOp() RasterOp
*/

/*!
  \fn const QPoint &QPainter::brushOrigin() const
  Returns the brush origin currently set.
  \sa setBrushOrigin()
*/


/*!
  \fn int QPainter::tabStops() const
  Returns the tab stop setting.
  \sa setTabStops()
*/

/*!
  Set the tab stop width to \a ts, ie. locates tab stops at ts, 2*ts,
  3*ts and so on.

  Tab stops are used when drawing formatted text with \c ExpandTabs
  set.  This fixed tab stop value is used only if no tab array is set
  (which is the default case).

  \sa tabStops(), setTabArray(), drawText(), fontMetrics()
*/

void QPainter::setTabStops( int ts )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setTabStops: Will be reset by begin()" );
#endif
    tabstops = ts;
    if ( isActive() && testf(ExtDev) ) {	// tell extended device
	QPDevCmdParam param[1];
	param[0].ival = ts;
	pdev->cmd( QPaintDevice::PdcSetTabStops, this, param );
    }
}

/*!
  \fn int *QPainter::tabArray() const
  Returns the currently set tab stop array.
  \sa setTabArray()
*/

/*!
  Sets the tab stop array to \a ta.  This puts tab stops at \a ta[0],
  ta[1] and so on.  The array is null-terminated.

  If both a tab array and a tab top size is set, the tab array wins.

  \sa tabArray(), setTabStops(), drawText(), fontMetrics()
*/

void QPainter::setTabArray( int *ta )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setTabArray: Will be reset by begin()" );
#endif
    if ( ta != tabarray ) {
	tabarraylen = 0;
	if ( tabarray )				// Avoid purify complaint
	    delete [] tabarray;			// delete old array
	if ( ta ) {				// tabarray = copy of 'ta'
	    while ( ta[tabarraylen] )
		tabarraylen++;
	    tabarraylen++; // and 0 terminator
	    tabarray = new int[tabarraylen];	// duplicate ta
	    memcpy( tabarray, ta, sizeof(int)*tabarraylen );
	} else {
	    tabarray = 0;
	}
    }
    if ( isActive() && testf(ExtDev) ) {	// tell extended device
	QPDevCmdParam param[2];
	param[0].ival = tabarraylen;
	param[1].ivec = tabarray;
	pdev->cmd( QPaintDevice::PdcSetTabArray, this, param );
    }
}


/*!
  \fn HANDLE QPainter::handle() const

  Returns the platform-dependent handle used for drawing.
*/


/*****************************************************************************
  QPainter xform settings
 *****************************************************************************/

#ifndef QT_NO_TRANSFORMATIONS

/*! Enables view transformations if \a enable is TRUE, or disables
view transformations if \a enable is FALSE.

  \sa hasViewXForm(), setWindow(), setViewport(), setWorldMatrix(),
  setWorldXForm(), xForm()
*/

void QPainter::setViewXForm( bool enable )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setViewXForm: Will be reset by begin()" );
#endif
    if ( !isActive() || enable == testf(VxF) )
	return;
    setf( VxF, enable );
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = enable;
	pdev->cmd( QPaintDevice::PdcSetVXform, this, param );
    }
    updateXForm();
}

/*!
  \fn bool QPainter::hasViewXForm() const
  Returns TRUE if view transformation is enabled, otherwise FALSE.
  \sa setViewXForm(), xForm()
*/

/*!
  Returns the window rectangle.
  \sa setWindow(), setViewXForm()
*/

QRect QPainter::window() const
{
    return QRect( wx, wy, ww, wh );
}

/*!
  Sets the window rectangle view transformation for the painter and
  enables view transformation.

  The window rectangle is part of the view transformation.  The window
  specifies the logical coordinate system.  Its sister, the
  viewport(), specifies the device coordinate system.

  The default window rectangle is the same as the device's rectangle.
  See the \link coordsys.html Coordinate System Overview \endlink for
  an overview of coordinate transformation.

  \sa window(), setViewport(), setViewXForm(), setWorldMatrix(),
  setWorldXForm()
*/

void QPainter::setWindow( int x, int y, int w, int h )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setWindow: Will be reset by begin()" );
#endif
    wx = x;
    wy = y;
    ww = w;
    wh = h;
    if ( testf(ExtDev) ) {
	QRect r( x, y, w, h );
	QPDevCmdParam param[1];
	param[0].rect = (QRect*)&r;
	pdev->cmd( QPaintDevice::PdcSetWindow, this, param );
    }
    if ( testf(VxF) )
	updateXForm();
    else
	setViewXForm( TRUE );
}

/*!
  Returns the viewport rectangle.
  \sa setViewport(), setViewXForm()
*/

QRect QPainter::viewport() const		// get viewport
{
    return QRect( vx, vy, vw, vh );
}

/*!
  Sets the viewport rectangle view transformation for the painter and
  enables view transformation.

  The viewport rectangle is part of the view transformation.  The
  viewport specifies the device coordinate system.  Its sister, the
  window(), specifies the logical coordinate system.

  The default viewport rectangle is the same as the device's rectangle.
  See the \link coordsys.html Coordinate System Overview \endlink for
  an overview of coordinate transformation.

  \sa viewport(), setWindow(), setViewXForm(), setWorldMatrix(),
  setWorldXForm(), xForm()
*/

void QPainter::setViewport( int x, int y, int w, int h )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setViewport: Will be reset by begin()" );
#endif
    vx = x;
    vy = y;
    vw = w;
    vh = h;
    if ( testf(ExtDev) ) {
	QRect r( x, y, w, h );
	QPDevCmdParam param[1];
	param[0].rect = (QRect*)&r;
	pdev->cmd( QPaintDevice::PdcSetViewport, this, param );
    }
    if ( testf(VxF) )
	updateXForm();
    else
	setViewXForm( TRUE );
}


/*!
  Enables world transformations if \a enable is TRUE, or disables
  world transformations if \a enable is FALSE. The world
  transformation matrix is not changed.

  \sa setWorldMatrix(), setWindow(), setViewport(), setViewXForm(), xForm()
*/

void QPainter::setWorldXForm( bool enable )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setWorldXForm: Will be reset by begin()" );
#endif
    if ( !isActive() || enable == testf(WxF) )
	return;
    setf( WxF, enable );
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = enable;
	pdev->cmd( QPaintDevice::PdcSetWXform, this, param );
    }
    updateXForm();
}

/*!
  \fn bool QPainter::hasWorldXForm() const
  Returns TRUE if world transformation is enabled, otherwise FALSE.
  \sa setWorldXForm()
*/

/*!
  Returns the world transformation matrix.
  \sa setWorldMatrix()
*/

const QWMatrix &QPainter::worldMatrix() const
{
    return wxmat;
}

/*!
  Sets the world transformation matrix to \a m and enables world
  transformation.

  If \a combine is TRUE, then \a m is combined with the current
  transformation matrix, otherwise \a m replaces the current
  transformation matrix.

  If \a m the identity matrix and \a combine is FALSE, this function calls
  setWorldXForm(FALSE).  (The identity matrix is the matrix where
  QWMatrix::m11() and QWMatrix::m22() are 1.0 and the rest are 0.0.)

  World transformations are applied after the view transformations
  (i.e. \link setWindow window\endlink and \link setViewport viewport\endlink).

  The following functions can transform the coordinate system without using
  a QWMatrix:
  <ul>
  <li>translate()
  <li>scale()
  <li>shear()
  <li>rotate()
  </ul>

  They operate on the painter's worldMatrix() and are implemented like this:

  \code
    void QPainter::rotate( double a )
    {
	QWMatrix m;
	m.rotate( a );
	setWorldMatrix( m, TRUE );
    }
  \endcode

  Note that you should always use \a combine when you are drawing into
  a QPicture. Otherwise it may not be possible to replay the picture
  with additional transformations.  Using translate(), scale(),
  etc. is safe.

  For a brief overview of coordinate transformation, see the \link
  coordsys.html Coordinate System Overview. \endlink

  \sa worldMatrix() setWorldXForm() setWindow() setViewport()
  setViewXForm() xForm() QWMatrix
*/

void QPainter::setWorldMatrix( const QWMatrix &m, bool combine )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	qWarning( "QPainter::setWorldMatrix: Will be reset by begin()" );
#endif
    if ( combine )
	wxmat = m * wxmat;			// combines
    else
	wxmat = m;				// set new matrix
    bool identity = wxmat.m11() == 1.0F && wxmat.m22() == 1.0F &&
		    wxmat.m12() == 0.0F && wxmat.m21() == 0.0F &&
		    wxmat.dx()	== 0.0F && wxmat.dy()  == 0.0F;
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[2];
	param[0].matrix = &m;
	param[1].ival = combine;
	pdev->cmd( QPaintDevice::PdcSetWMatrix, this, param );
    }
    if ( identity )
	setWorldXForm( FALSE );
    else if ( !testf(WxF) )
	setWorldXForm( TRUE );
    else
	updateXForm();
}

/*! \obsolete

  We recommend using save() instead.
*/

void QPainter::saveWorldMatrix()
{
    QWMatrixStack *stack = (QWMatrixStack *)wm_stack;
    if ( stack == 0 ) {
	stack  = new QStack<QWMatrix>;
	CHECK_PTR( stack );
	stack->setAutoDelete( TRUE );
	wm_stack = stack;
    }

    stack->push( new QWMatrix( wxmat ) );

}

/*! \obsolete
  We recommend using save() instead.
*/

void QPainter::restoreWorldMatrix()
{
    QWMatrixStack *stack = (QWMatrixStack *)wm_stack;
    if ( stack == 0 || stack->isEmpty() ) {
#if defined(CHECK_STATE)
	qWarning( "QPainter::restoreWorldMatrix: Empty stack error" );
#endif
	return;
    }
    QWMatrix* m = stack->pop();
    setWorldMatrix( *m );
    delete m;
}

#endif // QT_NO_TRANSFORMATIONS

/*!
  Translates the coordinate system by \a (dx,dy).

  For example, the following code draws a single vertical line 20 pixels high.
  \code
    void MyWidget::paintEvent()
    {
	QPainter paint( this );
	paint.drawLine(10,0,10,20);
	paint.translate(100.0,100.0);
	paint.drawLine(-90,-80,-90,-70);
    }
  \endcode

  \sa scale(), shear(), rotate(), resetXForm(), setWorldMatrix(), xForm()
*/

void QPainter::translate( double dx, double dy )
{
#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix m;
    m.translate( dx, dy );
    setWorldMatrix( m, TRUE );
#else
    xlatex += (int)dx;
    xlatey += (int)dy;
    setf( VxF );
#endif
}


#ifndef QT_NO_TRANSFORMATIONS
/*!
  Scales the coordinate system by \a (sx,sy).
  \sa translate(), shear(), rotate(), resetXForm(), setWorldMatrix(),
  xForm()
*/

void QPainter::scale( double sx, double sy )
{
    QWMatrix m;
    m.scale( sx, sy );
    setWorldMatrix( m, TRUE );
}

/*!
  Shears the coordinate system \a (sh,sv).
  \sa translate(), scale(), rotate(), resetXForm(), setWorldMatrix(),
  xForm()
*/

void QPainter::shear( double sh, double sv )
{
    QWMatrix m;
    m.shear( sv, sh );
    setWorldMatrix( m, TRUE );
}

/*!
  Rotates the coordinate system \a a degrees.
  \sa translate(), scale(), shear(), resetXForm(), setWorldMatrix(),
  xForm()
*/

void QPainter::rotate( double a )
{
    QWMatrix m;
    m.rotate( a );
    setWorldMatrix( m, TRUE );
}


/*!
  Resets any transformations that were made using translate(), scale(),
  shear(), rotate(), setWorldMatrix(), setViewport() and setWindow()
  \sa worldMatrix(), viewport(), window()
*/

void QPainter::resetXForm()
{
    if ( !isActive() )
	return;
    wx = wy = vx = vy = 0;			// default view origins
    ww = vw = pdev->metric( QPaintDeviceMetrics::PdmWidth );
    wh = vh = pdev->metric( QPaintDeviceMetrics::PdmHeight );
    wxmat = QWMatrix();
    setWorldXForm( FALSE );
}

const int TxNone      = 0;			// transformation codes
const int TxTranslate = 1;			// copy in qptr_xyz.cpp
const int TxScale     = 2;
const int TxRotShear  = 3;

/*!
  \internal
  Updates an internal integer transformation matrix.
*/

void QPainter::updateXForm()
{
    QWMatrix m;
    if ( testf(VxF) ) {
	m.translate( vx, vy );
	m.scale( 1.0*vw/ww, 1.0*vh/wh );
	m.translate( -wx, -wy );
    }
    if ( testf(WxF) ) {
	if ( testf(VxF) )
	    m = wxmat * m;
	else
	    m = wxmat;
    }
    xmat = m;

    txinv = FALSE;				// no inverted matrix
    txop  = TxNone;
    const double eps = 0.0; // ##### can we get away with this?
//#define FZ(x) ((x)<eps&&(x)>-eps) ###nonsense if eps==0.0!!!
#define FZ(x) ((x) == eps)
    //#define FEQ(x,y) (((x)-(y))<eps&&((x)-(y))>-eps) ###nonsense if eps==0.0!
#define FEQ(x,y) ((x)==(y))
    if ( FZ(m12()) && FZ(m21()) && m11() >= -eps && m22() >= -eps ) {
	if ( FEQ(m11(),1.0) && FEQ(m22(),1.0) ) {
	    if ( !FZ(dx()) || !FZ(dy()) )
		txop = TxTranslate;
	} else {
	    txop = TxScale;
#if defined(_WS_WIN_)
	    setf(DirtyFont);
#endif
	}
    } else {
	txop = TxRotShear;
#if defined(_WS_WIN_)
	setf(DirtyFont);
#endif
    }
#undef FZ
#undef FEQ
}


/*!
  \internal
  Updates an internal integer inverse transformation matrix.
*/

void QPainter::updateInvXForm()
{
#if defined(CHECK_STATE)
    ASSERT( txinv == FALSE );
#endif
    txinv = TRUE;				// creating inverted matrix
    bool invertible;
    QWMatrix m;
    if ( testf(VxF) ) {
	m.translate( vx, vy );
	m.scale( 1.0*vw/ww, 1.0*vh/wh );
	m.translate( -wx, -wy );
    }
    if ( testf(WxF) ) {
	if ( testf(VxF) )
	    m = wxmat * m;
	else
	    m = wxmat;
    }
    ixmat = m.invert( &invertible );		// invert matrix
}

#else
void QPainter::resetXForm()
{
    xlatex = 0;
    xlatey = 0;
}
#endif // QT_NO_TRANSFORMATIONS

/*!
  \internal
  Maps a point from logical coordinates to device coordinates.
*/

void QPainter::map( int x, int y, int *rx, int *ry ) const
{
#ifndef QT_NO_TRANSFORMATIONS
     switch ( txop ) {
	case TxNone:
	    *rx = x;  *ry = y;
	    break;
	case TxTranslate:
	    // #### "Why no rounding here?", Warwick asked of Haavard.
	    *rx = int(x + dx());
	    *ry = int(y + dy());
	    break;
	case TxScale: {
	    double tx = m11()*x + dx();
	    double ty = m22()*y + dy();
	    *rx = tx >= 0 ? int(tx + 0.5) : int(tx - 0.5);
	    *ry = ty >= 0 ? int(ty + 0.5) : int(ty - 0.5);
	    } break;
	default: {
	    double tx = m11()*x + m21()*y+dx();
	    double ty = m12()*x + m22()*y+dy();
	    *rx = tx >= 0 ? int(tx + 0.5) : int(tx - 0.5);
	    *ry = ty >= 0 ? int(ty + 0.5) : int(ty - 0.5);
	    } break;
    }
#else
    *rx = x + xlatex;
    *ry = y + xlatey;
#endif
}

/*!
  \internal
  Maps a rectangle from logical coordinates to device coordinates.
  This internal function does not handle rotation and/or shear.
*/

void QPainter::map( int x, int y, int w, int h,
		    int *rx, int *ry, int *rw, int *rh ) const
{
#ifndef QT_NO_TRANSFORMATIONS
     switch ( txop ) {
	case TxNone:
	    *rx = x;  *ry = y;
	    *rw = w;  *rh = h;
	    break;
	case TxTranslate:
	    // #### "Why no rounding here?", Warwick asked of Haavard.
	    *rx = int(x + dx());
	    *ry = int(y + dy());
	    *rw = w;  *rh = h;
	    break;
	case TxScale: {
	    double tx = m11()*x + dx();
	    double ty = m22()*y + dy();
	    double tw = m11()*w;
	    double th = m22()*h;
	    *rx = tx >= 0 ? int(tx + 0.5) : int(tx - 0.5);
	    *ry = ty >= 0 ? int(ty + 0.5) : int(ty - 0.5);
	    *rw = tw >= 0 ? int(tw + 0.5) : int(tw - 0.5);
	    *rh = th >= 0 ? int(th + 0.5) : int(th - 0.5);
	    } break;
	default:
#if defined(CHECK_STATE)
	    qWarning( "QPainter::map: Internal error" );
#endif
	    break;
    }

#else
    *rx = x + xlatex;
    *ry = y + xlatey;
    *rw = w;  *rh = h;
#endif
}

/*!
  \internal
  Maps a point from device coordinates to logical coordinates.
*/

void QPainter::mapInv( int x, int y, int *rx, int *ry ) const
{
#ifndef QT_NO_TRANSFORMATIONS
#if defined(CHECK_STATE)
    if ( !txinv )
	qWarning( "QPainter::mapInv: Internal error" );
#endif
    double tx = im11()*x + im21()*y+idx();
    double ty = im12()*x + im22()*y+idy();
    *rx = tx >= 0 ? int(tx + 0.5) : int(tx - 0.5);
    *ry = ty >= 0 ? int(ty + 0.5) : int(ty - 0.5);

#else
    *rx = x - xlatex;
    *ry = y - xlatey;
#endif
}

/*!
  \internal
  Maps a rectangle from device coordinates to logical coordinates.
  Cannot handle rotation and/or shear.
*/

void QPainter::mapInv( int x, int y, int w, int h,
		       int *rx, int *ry, int *rw, int *rh ) const
{
#ifndef QT_NO_TRANSFORMATIONS
#if defined(CHECK_STATE)
    if ( !txinv || txop == TxRotShear )
	qWarning( "QPainter::mapInv: Internal error" );
#endif
    double tx = im11()*x + idx();
    double ty = im22()*y + idy();
    double tw = im11()*w;
    double th = im22()*h;
    *rx = tx >= 0 ? int(tx + 0.5) : int(tx - 0.5);
    *ry = ty >= 0 ? int(ty + 0.5) : int(ty - 0.5);
    *rw = tw >= 0 ? int(tw + 0.5) : int(tw - 0.5);
    *rh = th >= 0 ? int(th + 0.5) : int(th - 0.5);

#else
    *rx = x - xlatex;
    *ry = y - xlatey;
    *rw = w;
    *rh = h;
#endif
}


/*!
  Returns the point \a pv transformed from model coordinates to device
  coordinates.

  \sa xFormDev(), QWMatrix::map()
*/

QPoint QPainter::xForm( const QPoint &pv ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return pv;
#endif
    int x=pv.x(), y=pv.y();
    map( x, y, &x, &y );
    return QPoint( x, y );
}

/*! \overload
  Returns the rectangle \a rv transformed from model coordinates to device
  coordinates.

  If world transformation is enabled and rotation or shearing has been
  specified, then the bounding rectangle is returned.

  \sa xFormDev(), QWMatrix::map()
*/

QRect QPainter::xForm( const QRect &rv ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return rv;

    if ( txop == TxRotShear ) {			// rotation/shear
	QPointArray a( rv );
	a = xForm( a );
	return a.boundingRect();
    }
#endif

    // Just translation/scale
    int x, y, w, h;
    rv.rect( &x, &y, &w, &h );
    map( x, y, w, h, &x, &y, &w, &h );
    return QRect( x, y, w, h );
}

/*! \overload

  Returns the point array \a av transformed from model coordinates to device
  coordinates.
  \sa xFormDev(), QWMatrix::map()
*/

QPointArray QPainter::xForm( const QPointArray &av ) const
{
    QPointArray a = av;
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop != TxNone )
#else
    if ( xlatex || xlatey )
#endif
    {
	a = a.copy();
	int x, y, i;
	for ( i=0; i<(int)a.size(); i++ ) {
	    a.point( i, &x, &y );
	    map( x, y, &x, &y );
	    a.setPoint( i, x, y );
	}
    }
    return a;
}

/*! \overload

  Returns the point array \a av transformed from model coordinates to device
  coordinates.  The \a index is the first point in the array and \a npoints
  denotes the number of points to be transformed.  If \a npoints is negative,
  all points from \a av[index] until the last point in the array are
  transformed.

  The returned point array consists of the number of points that were
  transformed.

  Example:
  \code
    QPointArray a(10);
    QPointArray b;
    b = painter.xForm(a,2,4);	// b.size() == 4
    b = painter.xForm(a,2,-1);	// b.size() == 8
  \endcode

  \sa xFormDev(), QWMatrix::map()
*/

QPointArray QPainter::xForm( const QPointArray &av, int index,
			     int npoints ) const
{
    int lastPoint = npoints < 0 ? av.size() : index+npoints;
    QPointArray a( lastPoint-index );
    int x, y, i=index, j=0;
    while ( i<lastPoint ) {
	av.point( i++, &x, &y );
	map( x, y, &x, &y );
	a.setPoint( j++, x, y );
    }
    return a;
}

/*!
  Returns the point \a pv transformed from device coordinates to model
  coordinates.
  \sa xForm(), QWMatrix::map()
*/

QPoint QPainter::xFormDev( const QPoint &pd ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return pd;
    if ( !txinv ) {
	QPainter *that = (QPainter*)this;	// mutable
	that->updateInvXForm();
    }
#endif
    int x=pd.x(), y=pd.y();
    mapInv( x, y, &x, &y );
    return QPoint( x, y );
}

/*!
  Returns the rectangle \a rv transformed from device coordinates to model
  coordinates.

  If world transformation is enabled and rotation or shearing is used,
  then the bounding rectangle is returned.

  \sa xForm(), QWMatrix::map()
*/

QRect QPainter::xFormDev( const QRect &rd ) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop == TxNone )
	return rd;
    if ( !txinv ) {
	QPainter *that = (QPainter*)this;	// mutable
	that->updateInvXForm();
    }
    if ( txop == TxRotShear ) {			// rotation/shear
	QPointArray a( rd );
	a = xFormDev( a );
	return a.boundingRect();
    }
#endif

    // Just translation/scale
    int x, y, w, h;
    rd.rect( &x, &y, &w, &h );
    mapInv( x, y, w, h, &x, &y, &w, &h );
    return QRect( x, y, w, h );
}

/*! Returns the point array \a av transformed from device coordinates
  to model coordinates.

  \sa xForm(), QWMatrix::map()
*/

QPointArray QPainter::xFormDev( const QPointArray &ad ) const
{
    QPointArray a = ad;
#ifndef QT_NO_TRANSFORMATIONS
    if ( txop != TxNone )
#else
    if ( xlatex || xlatey )
#endif
    {
	a = a.copy();
	int x, y, i;
	for ( i=0; i<(int)a.size(); i++ ) {
	    a.point( i, &x, &y );
	    mapInv( x, y, &x, &y );
	    a.setPoint( i, x, y );
	}
    }
    return a;
}

/*! \overload

  Returns the point array \a ad transformed from device coordinates to model
  coordinates.  The \a index is the first point in the array and \a npoints
  denotes the number of points to be transformed.  If \a npoints is negative,
  all points from \a av[index] until the last point in the array are
  transformed.

  The returned point array consists of the number of points that were
  transformed.

  Example:
  \code
    QPointArray a(10);
    QPointArray b;
    b = painter.xFormDev(a,1,3);	// b.size() == 3
    b = painter.xFormDev(a,1,-1);	// b.size() == 9
  \endcode

  \sa xForm(), QWMatrix::map()
*/

QPointArray QPainter::xFormDev( const QPointArray &ad, int index,
				int npoints ) const
{
    int lastPoint = npoints < 0 ? ad.size() : index+npoints;
    QPointArray a( lastPoint-index );
    int x, y, i=index, j=0;
    while ( i<lastPoint ) {
	ad.point( i++, &x, &y );
	map( x, y, &x, &y );
	a.setPoint( j++, x, y );
    }
    return a;
}


/*!
  Fills the rectangle \a (x,y,w,h) with the \a brush.

  You can specify a QColor as \a brush, since there is a QBrush constructor
  that takes a QColor argument and creates a solid pattern brush.

  \sa drawRect()
*/

void QPainter::fillRect( int x, int y, int w, int h, const QBrush &brush )
{
    QPen   oldPen   = pen();			// save pen
    QBrush oldBrush = this->brush();		// save brush
    setPen( NoPen );
    setBrush( brush );
    drawRect( x, y, w, h );			// draw filled rect
    setBrush( oldBrush );			// restore brush
    setPen( oldPen );				// restore pen
}


/*!
  \overload void QPainter::setBrushOrigin( const QPoint &p )
*/

/*!
  \overload void QPainter::setWindow( const QRect &r )
*/


/*!
  \overload void QPainter::setViewport( const QRect &r )
*/


/*!
  \fn bool QPainter::hasClipping() const
  Returns TRUE if clipping has been set, otherwise FALSE.
  \sa setClipping()
*/

/*!
  \fn const QRegion &QPainter::clipRegion() const

  Returns the currently set clip region.  Note that the clip region is
  given in physical device coordinates and \e not subject to any
  \link coordsys.html coordinate transformation. \endlink

  \sa setClipRegion(), setClipRect(), setClipping()
*/

/*!
  \fn void QPainter::setClipRect( int x, int y, int w, int h )

  Sets the clip region to the the rectangle \a (x,y,w,h) and enables clipping.

  Note that the clip region is given in physical device coordinates and
  \e not subject to any \link coordsys.html coordinate
  transformation.\endlink

  \sa setClipRegion(), clipRegion(), setClipping()
*/

/*!
  \overload void QPainter::drawPoint( const QPoint &p )
*/


/*!
  \overload void QPainter::moveTo( const QPoint &p )
*/

/*!
  \overload void QPainter::lineTo( const QPoint &p )
*/

/*!
  \overload void QPainter::drawLine( const QPoint &p1, const QPoint &p2 )
*/

/*!
  \overload void QPainter::drawRect( const QRect &r )
*/

/*!
  \overload void QPainter::drawWinFocusRect( const QRect &r )
*/

/*!
  \overload void QPainter::drawWinFocusRect( const QRect &r, const QColor &bgColor )
*/


#if !defined(_WS_X11_) && !defined(_WS_QWS_)
// The doc and X implementation of this functions is in qpainter_x11.cpp
void QPainter::drawWinFocusRect( int, int, int, int,
				 bool, const QColor & )
{
    // do nothing, only called from X11 specific functions
}
#endif


/*!
  \overload void QPainter::drawRoundRect( const QRect &r, int xRnd, int yRnd )
*/

/*!
  \overload void QPainter::drawEllipse( const QRect &r )
*/

/*!
  \overload void QPainter::drawArc( const QRect &r, int a, int alen )
*/

/*!
  \overload void QPainter::drawPie( const QRect &r, int a, int alen )
*/

/*!
  \overload void QPainter::drawChord( const QRect &r, int a, int alen )
*/

/*!
  \overload void QPainter::drawPixmap( const QPoint &p, const QPixmap &pm, const QRect &sr )
*/

/*!
  \overload void QPainter::drawPixmap( const QPoint &p, const QPixmap &pm )

  This version of the call draws the entire pixmap.
*/

void QPainter::drawPixmap( const QPoint &p, const QPixmap &pm )
{
    drawPixmap( p.x(), p.y(), pm, 0, 0, pm.width(), pm.height() );
}

#if QT_VERSION >= 300
#error "Merge QPainter::drawImage()'s by default conversion_flags=0"
#endif

/*!
  Draws at (\a x, \a y) the \a sw by \a sh area of pixels
  from (\a sx, \a sy) in \a image, using \a conversion_flags if
  the image needs to be converted to a pixmap.

  This function may convert \a image to a pixmap and then draw it, if
  device() is a QPixmap or a QWidget, or else draw it directly, if
  device() is a QPrinter or QPicture.

  \sa drawPixmap() QPixmap::convertFromImage()
*/
void QPainter::drawImage( int x, int y, const QImage & image,
			  int sx, int sy, int sw, int sh,
			  int conversion_flags )
{
#ifdef _WS_QWS_
    //### Hackish
# ifndef QT_NO_TRANSFORMATIONS
    if ( !image.isNull() && gfx &&
	(txop==TxNone||txop==TxTranslate) && !testf(ExtDev) )
# else
    if ( !image.isNull() && gfx && !testf(ExtDev) )
# endif
    {
        if(sw<0)
	    sw=image.width();
        if(sh<0)
	    sh=image.height();

	QImage image2 = qt_screen->mapToDevice( image );

	// This is a bit dubious
	if(image2.depth()==1) {
	    image2.setNumColors( 2 );
	    image2.setColor( 0, qRgb(255,255,255) );
	    image2.setColor( 1, qRgb(0,0,0) );
	}
	if ( image2.hasAlphaBuffer() )
	    gfx->setAlphaType(QGfx::InlineAlpha);
	else
	    gfx->setAlphaType(QGfx::IgnoreAlpha);
	gfx->setSource(&image2);
	if ( testf(VxF|WxF) ) {
	    map( x, y, &x, &y );
	}
	gfx->blt(x,y,sw,sh,sx,sy);
    }
    return;
#endif

    if ( !isActive() || image.isNull() )
	return;

    // right/bottom
    if ( sw < 0 )
	sw = image.width()  - sx;
    if ( sh < 0 )
	sh = image.height() - sy;

    // Sanity-check clipping
    if ( sx < 0 ) {
	x -= sx;
	sw += sx;
	sx = 0;
    }
    if ( sw + sx > image.width() )
	sw = image.width() - sx;
    if ( sy < 0 ) {
	y -= sy;
	sh += sy;
	sy = 0;
    }
    if ( sh + sy > image.height() )
	sh = image.height() - sy;

    if ( sw <= 0 || sh <= 0 )
	return;

    bool all = image.rect().intersect(QRect(sx,sy,sw,sh)) == image.rect();
    QImage subimage = all ? image : image.copy(sx,sy,sw,sh);

    if ( testf(ExtDev) ) {
	QPDevCmdParam param[2];
	QPoint p(x,y);
	param[0].point = &p;
	param[1].image = &subimage;
#if defined(_WS_WIN_)
	if ( !pdev->cmd( QPaintDevice::PdcDrawImage, this, param ) || !hdc )
	    return;
#elif defined(_WS_QWS_)
	pdev->cmd( QPaintDevice::PdcDrawImage, this, param );
	return;
#else
	if ( !pdev->cmd( QPaintDevice::PdcDrawImage, this, param ) || !hd )
	    return;
#endif
    }

    QPixmap pm;
    pm.convertFromImage( subimage, conversion_flags );
    drawPixmap( x, y, pm );
}

/*!
  \overload void QPainter::drawImage( int x, int y, const QImage & image,
			  int sx, int sy, int sw, int sh )
*/
void QPainter::drawImage( int x, int y, const QImage & image,
			  int sx, int sy, int sw, int sh )
{
    drawImage(x,y,image,sx,sy,sw,sh,0);
}

/*!
  \overload void QPainter::drawImage( const QPoint &, const QImage &, const QRect &sr )
*/
/*!
  \overload void QPainter::drawImage( const QPoint &, const QImage &, const QRect &sr, int conversion_flags )
*/
void QPainter::drawImage( const QPoint &p, const QImage &i, const QRect &sr, int conversion_flags )
{
    drawImage(p.x(),p.y(),i,sr.x(),sr.y(),sr.width(),sr.height(),conversion_flags);
}

/*!
  \overload void QPainter::drawImage( const QPoint &, const QImage & )
*/
void QPainter::drawImage( const QPoint & p, const QImage & i )
{
    drawImage(p, i, i.rect());
}

/*!
  \overload void QPainter::drawImage( const QPoint &, const QImage &, int conversion_flags )
*/
void QPainter::drawImage( const QPoint & p, const QImage & i, int conversion_flags )
{
    drawImage(p, i, i.rect(), conversion_flags);
}


void bitBlt( QPaintDevice *dst, int dx, int dy,
	     const QImage *src, int sx, int sy, int sw, int sh,
	     int conversion_flags )
{
    QPixmap tmp;
    if ( sx == 0 && sy == 0
	&& (sw<0 || sw==src->width()) && (sh<0 || sh==src->height()) )
    {
	tmp.convertFromImage( *src, conversion_flags );
    } else {
	tmp.convertFromImage( src->copy( sx, sy, sw, sh, conversion_flags),
			      conversion_flags );
    }
    bitBlt( dst, dx, dy, &tmp );
}


/*!
  \overload void QPainter::drawTiledPixmap( const QRect &r, const QPixmap &pm, const QPoint &sp )
*/

/*!
  \overload void QPainter::drawTiledPixmap( const QRect &r, const QPixmap &pm )
*/

/*!
  \overload void QPainter::fillRect( const QRect &r, const QBrush &brush )
*/

/*!
  \fn void QPainter::eraseRect( int x, int y, int w, int h )

  Erases the area inside \a (x,y,w,h).
  Equivalent to <code>fillRect( x, y, w, h, backgroundColor() )</code>.
*/

/*!
  \overload void QPainter::eraseRect( const QRect &r )
*/

/*!
  \overload void QPainter::drawText( const QPoint &p, const QString&, int len )
*/

/*!
  \overload void QPainter::drawText( const QRect &r, int tf, const QString&, int len, QRect *br, char **i )
*/

/*!
  \overload QRect QPainter::boundingRect( const QRect &r, int tf, const QString&, int len, char **i )
*/


static inline void fix_neg_rect( int *x, int *y, int *w, int *h )
{
    if ( *w < 0 ) {
	*w = -*w;
	*x -= *w - 1;
    }
    if ( *h < 0 ) {
	*h = -*h;
	*y -= *h - 1;
    }
}
void QPainter::fix_neg_rect( int *x, int *y, int *w, int *h )
{
    ::fix_neg_rect(x,y,w,h);
}

//
// The drawText function takes two special parameters; 'internal' and 'brect'.
//
// The 'internal' parameter contains a pointer to an array of encoded
// information that keeps internal geometry data.
// If the drawText function is called repeatedly to display the same text,
// it makes sense to calculate text width and linebreaks the first time,
// and use these parameters later to print the text because we save a lot of
// CPU time.
// The 'internal' parameter will not be used if it is a null pointer.
// The 'internal' parameter will be generated if it is not null, but points
// to a null pointer, i.e. internal != 0 && *internal == 0.
// The 'internal' parameter will be used if it contains a non-null pointer.
//
// If the 'brect parameter is a non-null pointer, then the bounding rectangle
// of the text will be returned in 'brect'.
//

/*!
  Draws at most \a len characters from \a str in the rectangle \a (x,y,w,h).

  Note that the meaning of \a y is not the same for the two drawText()
  varieties.

  This function draws formatted text.  The \a tf text formatting is
  really of type Qt::AlignmentFlags.

  Horizontal alignment defaults to AlignLeft and vertical alignment
  defaults to AlignTop.

  \a brect (if non-null) is set to the actual bounding rectangle of
  the output.  \a internal is, yes, internal.

  \sa boundingRect()
*/

void QPainter::drawText( int x, int y, int w, int h, int tf,
			 const QString& str, int len, QRect *brect,
			 char **internal )
{
    if ( !isActive() )
	return;
    if ( len < 0 )
	len = str.length();
    if ( len == 0 )				// empty string
	return;

    if ( testf(DirtyFont|ExtDev) ) {
	if ( testf(DirtyFont) )
	    updateFont();
	if ( testf(ExtDev) && (tf & DontPrint) == 0 ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    QString newstr = str;
	    newstr.truncate( len );
	    param[0].rect = &r;
	    param[1].ival = tf;
	    param[2].str = &newstr;
	    if ( pdev->devType() != QInternal::Printer ) {
#if defined(_WS_WIN_)
		if ( !pdev->cmd( QPaintDevice::PdcDrawText2Formatted,
				 this, param) ||
		     !hdc )
		    return;			// QPrinter wants PdcDrawText2
#elif defined(_WS_QWS_)
		pdev->cmd( QPaintDevice::PdcDrawText2Formatted, this, param);
		return;
#else
		if ( !pdev->cmd( QPaintDevice::PdcDrawText2Formatted,
				 this, param) ||
		     !hd )
		    return;			// QPrinter wants PdcDrawText2
#endif
	    }
	}
    }

    const QFontMetrics & fm = fontMetrics();		// get font metrics

    qt_format_text(fm, x, y, w, h, tf, str, len, brect,
		   tabstops, tabarray, tabarraylen, internal, this);
}


void qt_format_text( const QFontMetrics& fm, int x, int y, int w, int h,
		     int tf, const QString& str, int len, QRect *brect,
		     int tabstops, int* tabarray, int tabarraylen,
		     char **internal, QPainter* painter )
{
    if ( w <= 0 || h <= 0 )
	fix_neg_rect( &x, &y, &w, &h );

    struct text_info {				// internal text info
	char  tag[4];				// contains "qptr"
	int   w;				// width
	int   h;				// height
	int   tf;				// flags (alignment etc.)
	int   len;				// text length
	int   maxwidth;				// max text width
	Q_INT16 mlb;				// min left bearing
	Q_INT16 mrb;				// min right bearing
	int   nlines;				// number of lines
	int   codelen;				// length of encoding
    };

    uint codearray[200];
    int	   codelen    = 200;
    bool   code_alloc = FALSE;
    uint *codes     = codearray;
    uint cc;					// character code
    bool   decode     = internal && *internal;	// decode from internal data
    bool   encode     = internal && !*internal; // build internal data

    if ( len > 150 && !decode ) {		// need to alloc code array
	codelen = len + len/2; // ### enough? 200 != 150*1.5 -- WWA
	codes	= (uint *)malloc( codelen*sizeof(uint) );
	code_alloc = TRUE;
    }

    const uint BEGLINE  = 0x80000000;	// encoding 0x8000zzzz, z=width
    const uint TABSTOP  = 0x40000000;	// encoding 0x4000zzzz, z=tab pos
    const uint PREFIX   = 0x20000000;	// encoding 0x2000hilo
    const uint HI       = 0x0000ff00;	//  hi,lo=QChar
    const uint LO       = 0x000000ff;
    const int HI_SHIFT = 8;
    const int LO_SHIFT = 0;
    // An advanced display function might provide for different fonts, etc.
    const int WIDTHBITS= 0x1fffffff;	// bits for width encoding
    const int MAXWIDTH = 0x1fffffff;	// max width value

    const QChar *p = str.unicode();
    int nlines;					// number of lines
    int index;					// index for codes
    int begline;				// index at beginning of line
    int breakindex;				// index where to break
    int breakwidth;				// width of text at breakindex
    int maxwidth;				// maximum width of a line
    int tabindex;				// tab array index
    int cw;					// character width
    int k;					// index for p
    int tw;					// text width
    int minleftbearing = 0;
    int minrightbearing = 0;

#define ENCCHAR(x) (((x).cell() << LO_SHIFT) | ((x).row() << HI_SHIFT))
#define DECCHAR(x) QChar(((x)&LO)>>LO_SHIFT,((x)&HI)>>HI_SHIFT)
#define CWIDTH(x) fm.width(DECCHAR(x)) // Could cache, but put that it in fm
#define ISPRINT(x) ((x).row() || (x).cell()>' ')
    // ##### should use (unicode) QChar::isPrint() -- WWA to AG

    bool wordbreak  = (tf & Qt::WordBreak)  == Qt::WordBreak;
    bool expandtabs = (tf & Qt::ExpandTabs) == Qt::ExpandTabs;
    bool singleline = (tf & Qt::SingleLine) == Qt::SingleLine;
    bool showprefix = (tf & Qt::ShowPrefix) == Qt::ShowPrefix;

    int	 spacewidth = CWIDTH( QChar(' ') );	// width of space char

    nlines = 0;
    index  = 1;					// first index contains BEGLINE
    begline = breakindex = breakwidth = maxwidth = tabindex = 0;
    k = tw = 0;

    if ( decode )				// skip encoding
	k = len;

    int localTabStops = 0;	       		// tab stops
    if ( tabstops )
	localTabStops = tabstops;
    else {
	localTabStops = fm.width(QChar('x'))*8;       	// default to 8 times x
	if ( localTabStops == 0 )
	    localTabStops = fm.maxWidth() * 8;
    }
    QString word;

    bool fakeBreak = FALSE;
    bool breakwithinwords = FALSE;
    //qDebug("painting string %s pointSize=%d width=%d", str.latin1(), fm.height(), w);
    while ( k <= len ) {				// convert string to codes
	//qDebug("at position %d fakeBreak=%d", k, fakeBreak);
	if ( !fakeBreak && k < len && ISPRINT(*p) ) {			// printable character
	    if ( *p == '&' && showprefix ) {
		cc = '&';			// assume ampersand
		if ( k < len-1 ) {
		    k++;
		    p++;
#ifndef QT_NO_ACCEL
		    if ( *p != '&' && ISPRINT(*p) )
			cc = PREFIX | ENCCHAR(*p);// use prefix char
#else
		    if ( ISPRINT(*p) )
			cc = ENCCHAR(*p);// don't underline if no accel
#endif
		}
	    } else {
		cc = ENCCHAR(*p);
	    }

	    cw = 0;
 	    if ( breakwithinwords ) {
 		breakwidth += CWIDTH(cc);
 		if ( !word.isEmpty() && breakwidth > w ) {
 		    fakeBreak = TRUE;
 		    continue;
 		}
 	    }
	    word += *p;
	} else {				// not printable (except ' ')
	    // somehow the assertion fm.width(word) == breakwidth seems not to hold in 100%
	    // of the cases (maybe an X11 bug). This led to an endless loop in a very special case.
	    if ( !breakwithinwords || fakeBreak ) {
		cw = fm.width(word);
		//qDebug("%s: word = %s, width=%d", str.latin1(), word.latin1(), cw);
	    } else {
		cw = breakwidth - CWIDTH(cc);
		//qDebug( "cw gets set to %d - would have been %d", cw, fm.width(word) );
	    }
	    if ( !fakeBreak && wordbreak ) {
		if ( tw+cw > w ) {
		    if ( breakindex > 0 ) {
			breakwithinwords = FALSE;
			codes[begline] = BEGLINE | QMIN(tw,MAXWIDTH);
			maxwidth = QMAX(maxwidth,tw);
			begline = breakindex;
			tw = cw;
			breakindex = tabindex = 0;
			cw = 0;
			nlines++;
		    }
		    if ( /* do not add !breakwithinwords &&*/
			tw+cw > w && word.length() > 1) {
			breakwithinwords = TRUE;
			breakwidth = 0;
			p -= word.length();
			k -= word.length();
			index = begline+1;
			tw = 0;
			word = "";
			continue;
		    }
		}
	    }
	    word = "";

 	    if ( fakeBreak ) {
 		cc = BEGLINE;
 		fakeBreak = FALSE;
  		--k;
  		--p;
 	    }
 	    else
	    if ( k == len ) {
		// end (*p not valid)
		cc = 0;
	    } else if ( *p == ' ' ) {			// the space character
		cc = ' ';
		cw += spacewidth;
	    } else if ( *p == '\n' ) {		// newline
		if ( singleline ) {
		    cc = ' ';			// convert newline to space
		    cw += spacewidth;
		} else {
		    cc = BEGLINE;
		}
	    } else if ( *p == '\r' ) {		// ignore carriage return for now (convert to space)
		cc = ' ';
		cw += spacewidth;
	    } else if ( *p == '\t' ) {		// TAB character
		if ( expandtabs ) {
		    int ccw = 0;
		    if ( tabarray ) {		// use tab array
			while ( tabindex < tabarraylen ) {
			    if ( tabarray[tabindex] > (tw+cw) ) {
				ccw = tabarray[tabindex] - (tw+cw);
				tabindex++;
				break;
			    }
			    tabindex++;
			}
		    }
		    if ( ccw == 0 )		// use fixed tab stops
			ccw = localTabStops - (tw+cw)%localTabStops;
		    cw += ccw;
		    cc = TABSTOP | QMIN(tw+cw,MAXWIDTH);
		} else {			// convert TAB to space
		    cc = ' ';
		    cw += spacewidth;
		}
	    } else {				// ignore character
		k++;
		p++;
		continue;
	    }
	    breakindex = index;
	    breakwidth = 0;
	}

	tw += cw;				// increment text width

	if ( cc == BEGLINE ) {
	    breakwithinwords = FALSE;
	    codes[begline] = BEGLINE | QMIN(tw,MAXWIDTH);
	    maxwidth = QMAX(maxwidth,tw);
	    begline = index;
	    nlines++;
	    tw = 0;
	    breakindex = tabindex = 0;
	}
	codes[index++] = cc;
	if ( index >= codelen - 1 ) {		// grow code array
	    codelen *= 2;
	    if ( code_alloc ) {
		codes = (uint *)realloc( codes, sizeof(uint)*codelen );
	    } else {
		codes = (uint *)malloc( sizeof(uint)*codelen );
		code_alloc = TRUE;
	    }
	}
	k++;
	p++;
    }

    if ( !decode ) {
	codes[begline] = BEGLINE | QMIN(tw,MAXWIDTH);
	maxwidth = QMAX(maxwidth,tw);
	nlines++;
	codes[index++] = 0;
	codelen = index;

	uint* cptr = codes;
	while ( *cptr ) { 			// determine bearings
	    int lw = *cptr++ & WIDTHBITS;
	    if ( !lw ) {			// ignore empty line
		while ( *cptr && (*cptr & BEGLINE) != BEGLINE )
		    cptr++;
		continue;
	    }
	    if ( *cptr && (*cptr & (BEGLINE|TABSTOP)) == 0 ) {
		int lb = fm.leftBearing( DECCHAR(*cptr) );
		minleftbearing = QMIN( minleftbearing, lb );
	    }
	    while ( *cptr && (*cptr & BEGLINE) != BEGLINE )
		cptr++;
	    cptr--;
	    if ( *cptr && (*cptr & (BEGLINE|TABSTOP)) == 0 ) {
		int rb = fm.rightBearing( DECCHAR(*cptr) );
		minrightbearing = QMIN( minrightbearing, rb );
	    }
	    cptr++;
	}
    }

    if ( decode ) {				// decode from internal data
	char	  *data = *internal;
	text_info *ti	= (text_info*)data;
	if ( qstrncmp(ti->tag,"qptr",4)!=0 || ti->w != w || ti->h != h ||
	     ti->tf != tf || ti->len != len ) {
#if defined(CHECK_STATE)
	    qWarning( "QPainter::drawText: Internal text info is invalid" );
#endif
	    return;
	}
	maxwidth = ti->maxwidth;		// get internal values
	minleftbearing = ti->mlb;
	minrightbearing = ti->mrb;
	nlines	 = ti->nlines;
	codelen	 = ti->codelen;
	codes	 = (uint *)(data + sizeof(text_info));
    }

    if ( encode ) {				// build internal data
	char	  *data = new char[sizeof(text_info)+codelen*sizeof(uint)];
	text_info *ti	= (text_info*)data;
	strncpy( ti->tag, "qptr", 4 );		// set tag
	ti->w	     = w;			// save parameters
	ti->h	     = h;
	ti->tf	     = tf;
	ti->len	     = len;
	ti->maxwidth = maxwidth;
	ti->mlb	     = minleftbearing;
	ti->mrb	     = minrightbearing;
	ti->nlines   = nlines;
	ti->codelen  = codelen;
	memcpy( data+sizeof(text_info), codes, codelen*sizeof(uint) );
	*internal = data;
    }

    int	    fascent  = fm.ascent();		// get font measurements
    int	    fheight  = fm.height();
    int	    xp, yp;
    int	    xc;					// character xp

    int overflow = -minleftbearing - minrightbearing;
    maxwidth += overflow;
    if ( (tf & Qt::AlignVCenter) == Qt::AlignVCenter )	// vertically centered text
	yp = h/2 - nlines*fheight/2;
    else if ( (tf & Qt::AlignBottom) == Qt::AlignBottom)// bottom aligned
	yp = h - nlines*fheight;
    else					// top aligned
	yp = 0;
    if ( (tf & Qt::AlignRight) == Qt::AlignRight ) {
	xp = w - maxwidth;			// right aligned
    } else if ( (tf & Qt::AlignHCenter) == Qt::AlignHCenter ) {
	xp = w/2 - maxwidth/2;			// centered text
    } else {
	xp = 0;				// left aligned
    }

#if defined(CHECK_RANGE)
    int hAlignFlags = 0;
    if ( (tf & Qt::AlignRight) == Qt::AlignRight )
	hAlignFlags++;
    if ( (tf & Qt::AlignHCenter) == Qt::AlignHCenter )
	hAlignFlags++;
    if ( (tf & Qt::AlignLeft ) == Qt::AlignLeft )
	hAlignFlags++;

    if ( hAlignFlags > 1 )
	qWarning("QPainter::drawText: More than one of AlignRight, AlignLeft\n"
		 "\t\t    and AlignHCenter set in the tf parameter.");

    int vAlignFlags = 0;
    if ( (tf & Qt::AlignTop) == Qt::AlignTop )
	vAlignFlags++;
    if ( (tf & Qt::AlignVCenter) == Qt::AlignVCenter )
	vAlignFlags++;
    if ( (tf & Qt::AlignBottom ) == Qt::AlignBottom )
	vAlignFlags++;

    if ( vAlignFlags > 1 )
	qWarning("QPainter::drawText: More than one of AlignTop, AlignBottom\n"
		 "\t\t    and AlignVCenter set in the tf parameter.");
#endif // CHECK_RANGE

    //qDebug("%s: nlines = %d height=%d width needed=%d", str.latin1(), nlines, fheight, maxwidth);
    QRect br( x+xp, y+yp, maxwidth, nlines*fheight );
    if ( brect )				// set bounding rect
	*brect = br;

    if ( !painter || (tf & Qt::DontPrint) != 0 ) {// can't/don't print any text
	if ( code_alloc )
	    free( codes );
	return;
    }

    // From here, we have a painter.

    QRegion save_rgn = painter->crgn;		// save the current region
    bool    clip_on  = painter->testf(QPainter::ClipOn);

    if ( br.x() >= x && br.y() >= y && br.width() < w && br.height() < h )
	tf |= Qt::DontClip;				// no need to clip

    if ( (tf & Qt::DontClip) == 0 ) {		// clip text
	QRegion new_rgn;
	QRect r( x, y, w, h );
#ifndef QT_NO_TRANSFORMATIONS
	if ( painter->txop == TxRotShear ) {		// world xform active
	    QPointArray a( r );			// complex region
	    a = painter->xForm( a );
	    new_rgn = QRegion( a );
	} else {
#endif
	    r = painter->xForm( r );
	    new_rgn = QRegion( r );
#ifndef QT_NO_TRANSFORMATIONS
	}
#endif
	if ( clip_on )				// combine with existing region
	    new_rgn = new_rgn.intersect( painter->crgn );
	painter->setClipRegion( new_rgn );
    }

    yp += fascent;

    uint *cp = codes;

#if 0
    int i = 0;
    while ( *cp ) {
	qDebug("code[%d] = %x", i, *cp);
	cp++;
	i++;
    }
    cp = codes;
#endif

    while ( *cp ) {				// finally, draw the text
	tw = *cp++ & WIDTHBITS;			// text width

	if ( tw == 0 ) {			// ignore empty line
	    while ( *cp && (*cp & BEGLINE) != BEGLINE )
		cp++;
	    yp += fheight;
	    continue;
	}
	
	if ( (tf & Qt::AlignRight) == Qt::AlignRight ) {
	    xc = w - tw + minrightbearing;
	} else if ( (tf & Qt::AlignHCenter) == Qt::AlignHCenter ) {
	    xc = w/2 - (tw-minleftbearing-minrightbearing)/2 - minleftbearing;
	} else {
	    xc = -minleftbearing;
	}

	int bxc = xc;				// base x position (chars)
	while ( TRUE ) {
	    QString chunk;
	    while ( *cp && (*cp & (BEGLINE|TABSTOP)) == 0 ) {
		if ( (*cp & PREFIX) == PREFIX ) {
		    int xcpos = fm.width( chunk );
		    painter->fillRect( x+xc+xcpos, y+yp+fm.underlinePos(),
				       CWIDTH(*cp), fm.lineWidth(),
				       painter->cpen.color() );
		}
		chunk += DECCHAR(*cp);
		++cp;
	    }
	    painter->drawText( x+xc, y+yp, chunk );// draw the text
	    if ( (*cp & TABSTOP) == TABSTOP ) {
		int w = (*cp++ & WIDTHBITS);
		xc = bxc + w;
	    } else {				// *cp == 0 || *cp == BEGLINE
		break;
	    }
	}
	yp += fheight;
    }

    if ( (tf & Qt::DontClip) == 0 ) {		// restore clipping
	if ( clip_on ) {
	    painter->setClipRegion( save_rgn );
	} else {
	    painter->setClipping( FALSE );
	}
    }

    if ( code_alloc )
	free( codes );
}


/*!

  Returns the bounding rectangle of the aligned text that would be
  printed with the corresponding drawText() function (the first \a len
  characters from \a str).  The drawing, and hence the bounding
  rectangle, is constrained to the rectangle \a (x,y,w,h).

  If \a len is negative (default value), the whole string is used.

  The \a tf argument is
  the bitwise OR of the following flags:  <ul>
  <li> \c AlignLeft aligns to the left border.
  <li> \c AlignRight aligns to the right border.
  <li> \c AlignHCenter aligns horizontally centered.
  <li> \c AlignTop aligns to the top border.
  <li> \c AlignBottom aligns to the bottom border.
  <li> \c AlignVCenter aligns vertically centered
  <li> \c AlignCenter (= \c AlignHCenter | AlignVCenter)
  <li> \c SingleLine ignores newline characters in the text.
  <li> \c ExpandTabs expands tabulators.
  <li> \c ShowPrefix interprets "&x" as "x" underlined.
  <li> \c WordBreak breaks the text to fit the rectangle.
  </ul>

  Horizontal alignment defaults to AlignLeft and vertical alignment
  defaults to AlignTop.

  If several of the horizontal or several of the vertical alignment flags
  are set, the resulting alignment is undefined.

  These flags are defined in qnamespace.h.

  \sa drawText(), fontMetrics(), QFontMetrics::boundingRect(), Qt::AlignmentFlags
*/

QRect QPainter::boundingRect( int x, int y, int w, int h, int tf,
			      const QString& str, int len, char **internal )
{
    QRect brect;
    if ( str.isEmpty() )
	brect.setRect( x,y, 0,0 );
    else
	drawText( x, y, w, h, tf | DontPrint, str, len, &brect, internal );
    return brect;
}

// NOT REVISED BELOW THIS POINT

/*****************************************************************************
  QPen member functions
 *****************************************************************************/

/*!
  \class QPen qpen.h
  \brief The QPen class defines how a QPainter should draw lines and outlines
  of shapes.
  \ingroup drawing
  \ingroup shared

  A pen has a style, a width, a color, a cap style and a join style.

  The pen style defines the line type. The default pen style is \c
  Qt::SolidLine. Setting the style to \c NoPen tells the painter to
  not draw lines or outlines.

  The pen width defines the line width. The default line width is 0,
  which draws a 1-pixel line very fast, but with lower precision than
  with a line width of 1. Setting the line width to 1 or more draws
  lines that are precise, but drawing is slower.

  The pen color defines the color of lines and text. The default line
  color is black.  The QColor documentation lists predefined colors.

  The cap style defines how the end points of lines are drawn. The
  join style defines how the joins between two lines drawn when
  multiple, connected lines are drawn (QPainter::drawPolyLine() etc.).
  The cap and join styles apply only to wide lines, i.e. when the
  width is 1 or greater.

  Use the QBrush class for specifying fill styles.

  Example:
  \code
    QPainter painter;
    QPen     pen( red, 2 );		// red solid line, 2 pixel width
    painter.begin( &anyPaintDevice );	// paint something
    painter.setPen( pen );		// set the red, fat pen
    painter.drawRect( 40,30, 200,100 ); // draw rectangle
    painter.setPen( blue );		// set blue pen, 0 pixel width
    painter.drawLine( 40,30, 240,130 ); // draw diagonal in rectangle
    painter.end();			// painting done
  \endcode

  See the setStyle() function for a complete list of pen styles.

  About the end point of lines: For wide (non-0-width) pens, it
  depends on the cap style whether the end point is drawn or not. For
  0-width pens, QPainter will try to make sure that the end point is
  drawn, but this cannot be absolutely guaranteed, since the underlying
  drawing engine is free to use any (typically accellerated) algorithm
  for drawing 0-width lines. On all tested systems, however, the
  endpoint of at least all non-diagonal lines are drawn.

  \sa QPainter, QPainter::setPen()
*/


/*!
  \internal
  Initializes the pen.
*/

void QPen::init( const QColor &color, uint width, uint linestyle )
{
    data = new QPenData;
    CHECK_PTR( data );
    data->style = (PenStyle)(linestyle & MPenStyle);
    data->width = width;
    data->color = color;
    data->linest = linestyle;
}

/*!
  Constructs a default black solid line pen with 0 width.
*/

QPen::QPen()
{
    init( Qt::black, 0, SolidLine );		// default pen
}

/*!
  Constructs a	pen black with 0 width and a specified style.
  \sa setStyle()
*/

QPen::QPen( PenStyle style )
{
    init( Qt::black, 0, style );
}

/*!
  Constructs a pen with a specified color, width and style.
  \sa setWidth(), setStyle(), setColor()
*/

QPen::QPen( const QColor &color, uint width, PenStyle style )
{
    init( color, width, style );
}

/*!
  Constructs a pen with a specified color, width and styles.
  \sa setWidth(), setStyle(), setColor()
*/

QPen::QPen( const QColor &cl, uint w, PenStyle s, PenCapStyle c,
	    PenJoinStyle j )
{
    init( cl, w, s | c | j );
}

/*!
  Constructs a pen which is a copy of \a p.
*/

QPen::QPen( const QPen &p )
{
    data = p.data;
    data->ref();
}

/*!
  Destructs the pen.
*/

QPen::~QPen()
{
    if ( data->deref() )
	delete data;
}


/*!
  Detaches from shared pen data to makes sure that this pen is the only
  one referring the data.

  If multiple pens share common data, this pen dereferences the data
  and gets a copy of the data. Nothing is done if there is just a
  single reference.
*/

void QPen::detach()
{
    if ( data->count != 1 )
	*this = copy();
}


/*!
  Assigns \a c to this pen and returns a reference to this pen.
*/

QPen &QPen::operator=( const QPen &p )
{
    p.data->ref();				// beware of p = p
    if ( data->deref() )
	delete data;
    data = p.data;
    return *this;
}


/*!
  Returns a
  \link shclass.html deep copy\endlink of the pen.
*/

QPen QPen::copy() const
{
    QPen p( data->color, data->width, data->style, capStyle(), joinStyle() );
    return p;
}


/*!
  \fn PenStyle QPen::style() const
  Returns the pen style.
  \sa setStyle()
*/

/*!
  Sets the pen style to \a s.

  \warning On Windows 95/98, the style setting (other than NoPen and
  SolidLine) has no effect for lines with width greater than 1.

  \sa style()
*/

void QPen::setStyle( PenStyle s )
{
    if ( data->style == s )
	return;
    detach();
    data->style = s;
    data->linest = (data->linest & ~MPenStyle) | s;
}


/*!
  \fn uint QPen::width() const
  Returns the pen width.
  \sa setWidth()
*/

/*!
  Sets the pen width to \a w.
  \sa width()
*/

void QPen::setWidth( uint w )
{
    if ( data->width == w )
	return;
    detach();
    data->width = w;
}


/*!
  Returns the pen's cap style.

  \sa setCapStyle()
*/
Qt::PenCapStyle QPen::capStyle() const
{
    return (PenCapStyle)(data->linest & MPenCapStyle);
}

/*!
  Sets the pen's cap style to \a c.

  The default value is FlatCap. The cap style has no effect on 0-width pens.

  \warning On Windows 95/98, the cap style setting has no effect. Wide
  lines are rendered as if the cap style was SquareCap.

  \sa capStyle()
*/

void QPen::setCapStyle( PenCapStyle c )
{
    if ( (data->linest & MPenCapStyle) == c )
	return;
    detach();
    data->linest = (data->linest & ~MPenCapStyle) | c;
}

/*!
  Returns the pen's join style.

  \sa setJoinStyle()
*/
Qt::PenJoinStyle QPen::joinStyle() const
{
    return (PenJoinStyle)(data->linest & MPenJoinStyle);
}

/*!
  Sets the pen's join style to \a j.

  The default value is MiterJoin. The join style has no effect on 0-width pens.

  \warning On Windows 95/98, the join style setting has no effect. Wide
  lines are rendered as if the join style was BevelJoin.

  \sa joinStyle()
*/

void QPen::setJoinStyle( PenJoinStyle j )
{
    if ( (data->linest & MPenJoinStyle) == j )
	return;
    detach();
    data->linest = (data->linest & ~MPenJoinStyle) | j;
}

/*!
  \fn const QColor &QPen::color() const
  Returns the pen color.
  \sa setColor()
*/

/*!
  Sets the pen color to \a c.
  \sa color()
*/

void QPen::setColor( const QColor &c )
{
    detach();
    data->color = c;
}


/*!
  \fn bool QPen::operator!=( const QPen &p ) const

  Returns TRUE if the pen is different from \a p, or FALSE if the pens
  are equal.

  Two pens are different if they have different styles, widths or colors.

  \sa operator==()
*/

/*!
  Returns TRUE if the pen is equal to \a p, or FALSE if the pens are
  different.

  Two pens are equal if they have equal styles, widths and colors.

  \sa operator!=()
*/

bool QPen::operator==( const QPen &p ) const
{
    return (p.data == data) || (p.data->linest == data->linest &&
	    p.data->width == data->width && p.data->color == data->color);
}


/*****************************************************************************
  QPen stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
  \relates QPen
  Writes a pen to the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<( QDataStream &s, const QPen &p )
{
    if ( s.version() < 3 )
	return s << (Q_UINT8)p.style() << (Q_UINT8)p.width() << p.color();
    else
	return s << (Q_UINT8)( p.style() | p.capStyle() | p.joinStyle() )
		 << (Q_UINT8)p.width() << p.color();
}

/*!
  \relates QPen
  Reads a pen from the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>( QDataStream &s, QPen &p )
{
    Q_UINT8 style, width;
    QColor color;
    s >> style;
    s >> width;
    s >> color;
    p = QPen( color, (uint)width, (Qt::PenStyle)style );	// owl
    return s;
}
#endif //QT_NO_DATASTREAM

/*****************************************************************************
  QBrush member functions
 *****************************************************************************/

/*!
  \class QBrush qbrush.h

  \brief The QBrush class defines the fill pattern of shapes drawn by a QPainter.

  \ingroup drawing
  \ingroup shared

  A brush has a style and a color.  One of the brush styles is a custom
  pattern, which is defined by a QPixmap.

  The brush style defines the fill pattern. The default brush style is \c
  NoBrush (depends on how you construct a brush).  This style tells the
  painter to not fill shapes. The standard style for filling is called \c
  SolidPattern.

  The brush color defines the color of the fill pattern.
  The QColor documentation lists the predefined colors.

  Use the QPen class for specifying line/outline styles.

  Example:
  \code
    QPainter painter;
    QBrush   brush( yellow );		// yellow solid pattern
    painter.begin( &anyPaintDevice );	// paint something
    painter.setBrush( brush );		// set the yellow brush
    painter.setPen( NoPen );		// do not draw outline
    painter.drawRect( 40,30, 200,100 ); // draw filled rectangle
    painter.setBrush( NoBrush );	// do not fill
    painter.setPen( black );		// set black pen, 0 pixel width
    painter.drawRect( 10,10, 30,20 );	// draw rectangle outline
    painter.end();			// painting done
  \endcode

  See the setStyle() function for a complete list of brush styles.

  \sa QPainter, QPainter::setBrush(), QPainter::setBrushOrigin()
*/


/*!
  \internal
  Initializes the brush.
*/

void QBrush::init( const QColor &color, BrushStyle style )
{
    data = new QBrushData;
    CHECK_PTR( data );
    data->style	 = style;
    data->color	 = color;
    data->pixmap = 0;
}

/*!
  Constructs a default black brush with the style \c NoBrush (will not fill
  shapes).
*/

QBrush::QBrush()
{
    init( Qt::black, NoBrush );
}

/*!
  Constructs a black brush with the specified style.
  \sa setStyle()
*/

QBrush::QBrush( BrushStyle style )
{
    init( Qt::black, style );
}

/*!
  Constructs a brush with a specified color and style.
  \sa setColor(), setStyle()
*/

QBrush::QBrush( const QColor &color, BrushStyle style )
{
    init( color, style );
}

/*!
  Constructs a brush with a specified color and a custom pattern.

  The color will only have an effect for monochrome pixmaps, i.e.
  QPixmap::depth() == 1.

  \sa setColor(), setPixmap()
*/

QBrush::QBrush( const QColor &color, const QPixmap &pixmap )
{
    init( color, CustomPattern );
    setPixmap( pixmap );
}

/*!
  Constructs a brush which is a
  \link shclass.html shallow copy\endlink of \a b.
*/

QBrush::QBrush( const QBrush &b )
{
    data = b.data;
    data->ref();
}

/*!
  Destructs the brush.
*/

QBrush::~QBrush()
{
    if ( data->deref() ) {
	delete data->pixmap;
	delete data;
    }
}


/*!
  Detaches from shared brush data to makes sure that this brush is the only
  one referring the data.

  If multiple brushes share common data, this pen dereferences the data
  and gets a copy of the data. Nothing is done if there is just a single
  reference.
*/

void QBrush::detach()
{
    if ( data->count != 1 )
	*this = copy();
}


/*!
  Assigns \a b to this brush and returns a reference to this brush.
*/

QBrush &QBrush::operator=( const QBrush &b )
{
    b.data->ref();				// beware of b = b
    if ( data->deref() ) {
	delete data->pixmap;
	delete data;
    }
    data = b.data;
    return *this;
}


/*!
  Returns a
  \link shclass.html deep copy\endlink of the brush.
*/

QBrush QBrush::copy() const
{
    if ( data->style == CustomPattern ) {     // brush has pixmap
	QBrush b( data->color, *data->pixmap );
	return b;
    } else {				      // brush has std pattern
	QBrush b( data->color, data->style );
	return b;
    }
}


/*!
  \fn BrushStyle QBrush::style() const
  Returns the brush style.
  \sa setStyle()
*/

/*!
  Sets the brush style to \a s.

  The brush styles are:
  <ul>
  <li> \c NoBrush  will not fill shapes (default).
  <li> \c SolidPattern  solid (100%) fill pattern.
  <li> \c Dense1Pattern  94% fill pattern.
  <li> \c Dense2Pattern  88% fill pattern.
  <li> \c Dense3Pattern  63% fill pattern.
  <li> \c Dense4Pattern  50% fill pattern.
  <li> \c Dense5Pattern  37% fill pattern.
  <li> \c Dense6Pattern  12% fill pattern.
  <li> \c Dense7Pattern  6% fill pattern.
  <li> \c HorPattern  horizontal lines pattern.
  <li> \c VerPattern  vertical lines pattern.
  <li> \c CrossPattern  crossing lines pattern.
  <li> \c BDiagPattern  diagonal lines (directed / ) pattern.
  <li> \c FDiagPattern  diagonal lines (directed \ ) pattern.
  <li> \c DiagCrossPattern  diagonal crossing lines pattern.
  <li> \c CustomPattern  set when a pixmap pattern is being used.
  </ul>

  \sa style()
*/

void QBrush::setStyle( BrushStyle s )		// set brush style
{
    if ( data->style == s )
	return;
#if defined(CHECK_RANGE)
    if ( s == CustomPattern )
	qWarning( "QBrush::setStyle: CustomPattern is for internal use" );
#endif
    detach();
    data->style = s;
}


/*!
  \fn const QColor &QBrush::color() const
  Returns the brush color.
  \sa setColor()
*/

/*!
  Sets the brush color to \a c.
  \sa color(), setStyle()
*/

void QBrush::setColor( const QColor &c )
{
    detach();
    data->color = c;
}


/*!
  \fn QPixmap *QBrush::pixmap() const
  Returns a pointer to the custom brush pattern.

  A null pointer is returned if no custom brush pattern has been set.

  \sa setPixmap()
*/

/*!
  Sets the brush pixmap.  The style is set to \c CustomPattern.

  The current brush color will only have an effect for monochrome pixmaps,
  i.e.	QPixmap::depth() == 1.

  \sa pixmap(), color()
*/

void QBrush::setPixmap( const QPixmap &pixmap )
{
    detach();
    if ( data->pixmap )
	delete data->pixmap;
    if ( pixmap.isNull() ) {
	data->style  = NoBrush;
	data->pixmap = 0;
    } else {
	data->style = CustomPattern;
	data->pixmap = new QPixmap( pixmap );
	if ( data->pixmap->optimization() == QPixmap::MemoryOptim )
	    data->pixmap->setOptimization( QPixmap::NormalOptim );
    }
}


/*!
  \fn bool QBrush::operator!=( const QBrush &b ) const
  Returns TRUE if the brush is different from \a b, or FALSE if the brushes are
  equal.

  Two brushes are different if they have different styles, colors or pixmaps.

  \sa operator==()
*/

/*!
  Returns TRUE if the brush is equal to \a b, or FALSE if the brushes are
  different.

  Two brushes are equal if they have equal styles, colors and pixmaps.

  \sa operator!=()
*/

bool QBrush::operator==( const QBrush &b ) const
{
    return (b.data == data) || (b.data->style == data->style &&
	    b.data->color  == data->color &&
	    b.data->pixmap == data->pixmap);
}


/*****************************************************************************
  QBrush stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
  \relates QBrush
  Writes a brush to the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<( QDataStream &s, const QBrush &b )
{
    s << (Q_UINT8)b.style() << b.color();
    if ( b.style() == Qt::CustomPattern )
	s << *b.pixmap();
    return s;
}

/*!
  \relates QBrush
  Reads a brush from the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>( QDataStream &s, QBrush &b )
{
    Q_UINT8 style;
    QColor color;
    s >> style;
    s >> color;
    if ( style == Qt::CustomPattern ) {
	QPixmap pm;
	s >> pm;
	b = QBrush( color, pm );
    }
    else
	b = QBrush( color, (Qt::BrushStyle)style );
    return s;
}
#endif // QT_NO_DATASTREAM
