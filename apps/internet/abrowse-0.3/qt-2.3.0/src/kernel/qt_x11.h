/****************************************************************************
** $Id$
**
** Includes X11 system header files.
**
** Created : 981123
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

#ifdef __ATHEOS__
#define QT_X11_H
#endif

#ifndef QT_X11_H
#define QT_X11_H


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of q*_x11.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
//


#include "qwindowdefs.h"
#define	 GC GC_QQQ

#if defined(_XLIB_H_) // crude hack, but...
#error "cannot include X11/Xlib.h before this file"
#endif

// the following is necessary to work around breakage in many
// still-used versions of XFree86's Xlib.h.  *sigh*
#define XRegisterIMInstantiateCallback qt_XRegisterIMInstantiateCallback
#define XUnregisterIMInstantiateCallback qt_XUnregisterIMInstantiateCallback
#define XSetIMValues qt_XSetIMValues
#include <X11/Xlib.h>
// we trust the include guard, and undef the symbols again ASAP.
#undef XRegisterIMInstantiateCallback
#undef XUnregisterIMInstantiateCallback
#undef XSetIMValues

#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

#ifdef QT_XFT
#include <X11/Xft/Xft.h>
#endif

//#define QT_NO_SHAPE
#ifdef QT_NO_SHAPE
#define XShapeCombineRegion(a,b,c,d,e,f,g)
#define XShapeCombineMask(a,b,c,d,e,f,g)
#else
#include <X11/extensions/shape.h>
#endif

#if !defined(XlibSpecificationRelease)
#define X11R4
typedef char *XPointer;
#else
#undef  X11R4
#endif


#if defined(X11R4)
// X11R4 does not have XIM
#define NO_XIM
#elif defined(_OS_OSF_) && (XlibSpecificationRelease < 6)
// broken in Xlib up to OSF/1 3.2
#define NO_XIM
#elif defined(_OS_AIX_)
// broken in Xlib up to what version of AIX?
#define NO_XIM
#elif defined(_OS_SOLARIS_)
// XRegisterIMInstantiateCallback broken under "C" locale on Solaris
#define NO_XIM
#elif defined(NO_DEBUG) && defined(_OS_IRIX_) && defined(_CC_EDG_)
// XCreateIC broken when compiling -64 on IRIX 6.5.2
#define NO_XIM
#elif defined(_OS_HPUX_) && defined(__LP64__)
// XCreateIC broken when compiling 64-bit ELF on HP-UX 11.0
#define NO_XIM
#endif



#if !defined(NO_XIM) && (XlibSpecificationRelease >= 6 )
#define USE_X11R6_XIM

//######### XFree86 has wrong declarations for XRegisterIMInstantiateCallback
//######### and XUnregisterIMInstantiateCallback in at least version 3.3.2.
//######### Many old X11R6 header files lack XSetIMValues.
//######### Therefore, we have to declare these functions ourselves.

extern "C" Bool XRegisterIMInstantiateCallback(
    Display*,
    struct _XrmHashBucketRec*,
    char*,
    char*,
    XIMProc, //XFree86 has XIDProc, which has to be wrong
    XPointer
);

extern "C" Bool XUnregisterIMInstantiateCallback(
    Display*,
    struct _XrmHashBucketRec*,
    char*,
    char*,
    XIMProc, //XFree86 has XIDProc, which has to be wrong
    XPointer
);

extern "C" char *XSetIMValues( XIM /* im */, ... );

#ifdef QT_XFT
// These methods are internal. They are likely to change or disappear in the next versions of Qt.
class QFont;
void qt_destroy_ft_draw (Display *dpy, Drawable draw);
int qt_create_ft_draw (Display *dpy, Drawable draw, Visual *visual, Colormap cmap);
int qt_use_xft (void);
void *qt_ft_font(const QFont *);
#endif

#endif


#endif // QT_X11_H
