/****************************************************************************
** $Id$
**
** Definition of QPaintDevice class
**
** Created : 940721
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

#ifndef QPAINTDEVICE_H
#define QPAINTDEVICE_H

#ifdef __ATHEOS__
#include "../../konq-embed/dropin/atheos/apaintdevice.h"
#else

#ifndef QT_H
#include "qwindowdefs.h"
#include "qrect.h"
#endif // QT_H

#if defined(_WS_QWS_)
class QWSDisplay;
class QGfx;
#endif

class QIODevice;
class QString;


#if defined(_WS_X11_)
struct QPaintDeviceX11Data;
#endif

union QPDevCmdParam {
    int			 ival;
    int			*ivec;
    QString	        *str;
    const QPoint	*point;
    const QRect		*rect;
    const QPointArray	*ptarr;
    const QPixmap	*pixmap;
    const QImage	*image;
    const QColor	*color;
    const QFont		*font;
    const QPen		*pen;
    const QBrush	*brush;
    const QRegion	*rgn;
    const QWMatrix	*matrix;
    QIODevice		*device;
};



class Q_EXPORT QPaintDevice				// device for QPainter
{
public:
    virtual ~QPaintDevice();

    int		devType() const;
    bool	isExtDev() const;
    bool	paintingActive() const;

    // Windows:	  get device context
    // X-Windows: get drawable
#if defined(_WS_WIN_)
    HDC		handle() const;
#elif defined(_WS_X11_)
    HANDLE	handle() const;
#elif defined(_WS_MAC_)
    HANDLE      handle() const;
#elif defined(_WS_QWS_)
    HANDLE      handle() const;
#endif

#if defined(_WS_X11_)
    Display 	   *x11Display() const;
    int		    x11Screen() const;
    int		    x11Depth() const;
    int		    x11Cells() const;
    HANDLE	    x11Colormap() const;
    bool	    x11DefaultColormap() const;
    void	   *x11Visual() const;
    bool	    x11DefaultVisual() const;

    static Display *x11AppDisplay();
    static int	    x11AppScreen();
    static int	    x11AppDepth();
    static int	    x11AppCells();
    static int	    x11AppDpiX();
    static int	    x11AppDpiY();
    static HANDLE   x11AppColormap();
    static bool     x11AppDefaultColormap();
    static void    *x11AppVisual();
    static bool	    x11AppDefaultVisual();
    static void	    x11SetAppDpiX(int);
    static void	    x11SetAppDpiY(int);
#endif

#if defined(_WS_QWS_)
    static QWSDisplay *qwsDisplay();
    virtual unsigned char * scanLine(int) const;
    virtual int bytesPerLine() const;
    virtual QGfx * graphicsContext(bool clip_children=TRUE) const;
#endif

    enum PDevCmd {
	PdcNOP = 0, //  <void>
	PdcDrawPoint = 1, // point
	PdcDrawFirst = PdcDrawPoint,
	PdcMoveTo = 2, // point
	PdcLineTo = 3, // point
	PdcDrawLine = 4, // point,point
	PdcDrawRect = 5, // rect
	PdcDrawRoundRect = 6, // rect,ival,ival
	PdcDrawEllipse = 7, // rect
	PdcDrawArc = 8, // rect,ival,ival
	PdcDrawPie = 9, // rect,ival,ival
	PdcDrawChord = 10, // rect,ival,ival
	PdcDrawLineSegments = 11, // ptarr
	PdcDrawPolyline = 12, // ptarr
	PdcDrawPolygon = 13, // ptarr,ival
	PdcDrawQuadBezier = 14, // ptarr
	PdcDrawText = 15, // point,str
	PdcDrawTextFormatted = 16, // rect,ival,str
	PdcDrawPixmap = 17, // point,pixmap
	PdcDrawImage = 18, // point,image
	PdcDrawText2 = 19, // point,str
	PdcDrawText2Formatted = 20, // rect,ival,str
	PdcDrawLast = PdcDrawText2Formatted,

	// no painting commands below PdcDrawLast.

	PdcBegin = 30, //  <void>
	PdcEnd = 31, //  <void>
	PdcSave = 32, //  <void>
	PdcRestore = 33, //  <void>
	PdcSetdev = 34, // device - PRIVATE
	PdcSetBkColor = 40, // color
	PdcSetBkMode = 41, // ival
	PdcSetROP = 42, // ival
	PdcSetBrushOrigin = 43, // point
	PdcSetFont = 45, // font
	PdcSetPen = 46, // pen
	PdcSetBrush = 47, // brush
	PdcSetTabStops = 48, // ival
	PdcSetTabArray = 49, // ival,ivec
	PdcSetUnit = 50, // ival
	PdcSetVXform = 51, // ival
	PdcSetWindow = 52, // rect
	PdcSetViewport = 53, // rect
	PdcSetWXform = 54, // ival
	PdcSetWMatrix = 55, // matrix,ival
	PdcSaveWMatrix = 56,
	PdcRestoreWMatrix = 57,
	PdcSetClip = 60, // ival
	PdcSetClipRegion = 61, // rgn

	PdcReservedStart = 0, // codes 0-199 are reserved
	PdcReservedStop = 199 //   for Qt
    };

protected:
    QPaintDevice( uint devflags );

#if defined(_WS_WIN_)
    HDC		hdc;				// device context
#elif defined(_WS_X11_)
    HANDLE	hd;				// handle to drawable
    void		 copyX11Data( const QPaintDevice * );
    virtual void	 setX11Data( const QPaintDeviceX11Data* );
    QPaintDeviceX11Data* getX11Data( bool def=FALSE ) const;
#elif defined(_WS_MAC_)
    void * hd;
    virtual void fixport();
#elif defined(_WS_QWS_)
    HANDLE hd;
#endif

    virtual bool cmd( int, QPainter *, QPDevCmdParam * );
    virtual int	 metric( int ) const;
    virtual int	 fontMet( QFont *, int, const char * = 0, int = 0 ) const;
    virtual int	 fontInf( QFont *, int ) const;

    ushort	devFlags;			// device flags
    ushort	painters;			// refcount

    friend class QPainter;
    friend class QPaintDeviceMetrics;
    friend Q_EXPORT void bitBlt( QPaintDevice *, int, int,
				 const QPaintDevice *,
				 int, int, int, int, Qt::RasterOp, bool );
#if defined(_WS_X11_)
    friend void qt_init_internal( int *, char **, Display * );
#endif

private:
#if defined(_WS_X11_)
    static Display *x_appdisplay;
    static int	    x_appscreen;
    static int	    x_appdepth;
    static int	    x_appcells;
    static HANDLE   x_appcolormap;
    static bool	    x_appdefcolormap;
    static void	   *x_appvisual;
    static bool     x_appdefvisual;

    QPaintDeviceX11Data* x11Data;
#endif

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QPaintDevice( const QPaintDevice & );
    QPaintDevice &operator=( const QPaintDevice & );
#endif
};


Q_EXPORT
void bitBlt( QPaintDevice *dst, int dx, int dy,
	     const QPaintDevice *src, int sx=0, int sy=0, int sw=-1, int sh=-1,
	     Qt::RasterOp = Qt::CopyROP, bool ignoreMask=FALSE );

Q_EXPORT
void bitBlt( QPaintDevice *dst, int dx, int dy,
	     const QImage *src, int sx=0, int sy=0, int sw=-1, int sh=-1,
	     int conversion_flags=0 );


#if defined(_WS_X11_)

struct Q_EXPORT QPaintDeviceX11Data {
    Display*	x_display;
    int		x_screen;
    int		x_depth;
    int		x_cells;
    HANDLE	x_colormap;
    bool	x_defcolormap;
    void*	x_visual;
    bool	x_defvisual;
};

#endif

/*****************************************************************************
  Inline functions
 *****************************************************************************/

inline int QPaintDevice::devType() const
{ return devFlags & QInternal::DeviceTypeMask; }

inline bool QPaintDevice::isExtDev() const
{ return (devFlags & QInternal::ExternalDevice) != 0; }

inline bool QPaintDevice::paintingActive() const
{ return painters != 0; }

#if defined(_WS_WIN_)
inline HDC    QPaintDevice::handle() const { return hdc; }
#elif defined(_WS_X11_)
inline HANDLE QPaintDevice::handle() const { return hd; }
#endif

#if defined(_WS_X11_)
inline Display *QPaintDevice::x11Display() const
{ return x11Data ? x11Data->x_display : x_appdisplay; }

inline int QPaintDevice::x11Screen() const
{ return x11Data ? x11Data->x_screen : x_appscreen; }

inline int QPaintDevice::x11Depth() const
{ return x11Data ? x11Data->x_depth : x_appdepth; }

inline int QPaintDevice::x11Cells() const
{ return x11Data ? x11Data->x_cells : x_appcells; }

inline HANDLE QPaintDevice::x11Colormap() const
{ return x11Data ? x11Data->x_colormap : x_appcolormap; }

inline bool QPaintDevice::x11DefaultColormap() const
{ return x11Data ? x11Data->x_defcolormap : x_appdefcolormap; }

inline void *QPaintDevice::x11Visual() const
{ return x11Data ? x11Data->x_visual : x_appvisual; }

inline bool QPaintDevice::x11DefaultVisual() const
{ return x11Data ? x11Data->x_defvisual : x_appdefvisual; }

inline Display *QPaintDevice::x11AppDisplay()
{ return x_appdisplay; }

inline int QPaintDevice::x11AppScreen()
{ return x_appscreen; }

inline int QPaintDevice::x11AppDepth()
{ return x_appdepth; }

inline int QPaintDevice::x11AppCells()
{ return x_appcells; }

inline HANDLE QPaintDevice::x11AppColormap()
{ return x_appcolormap; }

inline bool QPaintDevice::x11AppDefaultColormap()
{ return x_appdefcolormap; }

inline void *QPaintDevice::x11AppVisual()
{ return x_appvisual; }

inline bool QPaintDevice::x11AppDefaultVisual()
{ return x_appdefvisual; }
#endif // _WS_X11_


Q_EXPORT
inline void bitBlt( QPaintDevice *dst, const QPoint &dp,
		    const QPaintDevice *src, const QRect &sr =QRect(0,0,-1,-1),
		    Qt::RasterOp rop=Qt::CopyROP, bool ignoreMask=FALSE )
{
    bitBlt( dst, dp.x(), dp.y(), src, sr.x(), sr.y(), sr.width(), sr.height(),
	    rop, ignoreMask );
}


#endif // __ATHEOS__

#endif // QPAINTDEVICE_H
