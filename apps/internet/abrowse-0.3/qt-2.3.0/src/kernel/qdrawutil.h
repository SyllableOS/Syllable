/****************************************************************************
** $Id$
**
** Definition of draw utilities
**
** Created : 950920
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

#ifndef QDRAWUTIL_H
#define QDRAWUTIL_H

#ifndef QT_H
#include "qpainter.h"
#include "qpalette.h"
#include "qnamespace.h"
#endif // QT_H

#ifndef QT_NO_DRAWUTIL
//
// Standard shade drawing
//

Q_EXPORT
void qDrawShadeLine( QPainter *p, int x1, int y1, int x2, int y2,
		     const QColorGroup &g, bool sunken = TRUE,
		     int lineWidth = 1, int midLineWidth = 0 );

Q_EXPORT
void qDrawShadeLine( QPainter *p, const QPoint &p1, const QPoint &p2,
		     const QColorGroup &g, bool sunken = TRUE,
		     int lineWidth = 1, int midLineWidth = 0 );

Q_EXPORT
void qDrawShadeRect( QPainter *p, int x, int y, int w, int h,
		     const QColorGroup &, bool sunken=FALSE,
		     int lineWidth = 1, int midLineWidth = 0,
		     const QBrush *fill = 0 );

Q_EXPORT
void qDrawShadeRect( QPainter *p, const QRect &r,
		     const QColorGroup &, bool sunken=FALSE,
		     int lineWidth = 1, int midLineWidth = 0,
		     const QBrush *fill = 0 );

Q_EXPORT
void qDrawShadePanel( QPainter *p, int x, int y, int w, int h,
		      const QColorGroup &, bool sunken=FALSE,
		      int lineWidth = 1, const QBrush *fill = 0 );

Q_EXPORT
void qDrawShadePanel( QPainter *p, const QRect &r,
		      const QColorGroup &, bool sunken=FALSE,
		      int lineWidth = 1, const QBrush *fill = 0 );

Q_EXPORT
void qDrawWinButton( QPainter *p, int x, int y, int w, int h,
		     const QColorGroup &g, bool sunken = FALSE,
		     const QBrush *fill = 0 );

Q_EXPORT
void qDrawWinButton( QPainter *p, const QRect &r,
		     const QColorGroup &g, bool sunken = FALSE,
		     const QBrush *fill = 0 );

Q_EXPORT
void qDrawWinPanel( QPainter *p, int x, int y, int w, int h,
		    const QColorGroup &, bool sunken=FALSE,
		    const QBrush *fill = 0 );

Q_EXPORT
void qDrawWinPanel( QPainter *p, const QRect &r,
		    const QColorGroup &, bool sunken=FALSE,
		    const QBrush *fill = 0 );

Q_EXPORT
void qDrawPlainRect( QPainter *p, int x, int y, int w, int h, const QColor &,
		     int lineWidth = 1, const QBrush *fill = 0 );

Q_EXPORT
void qDrawPlainRect( QPainter *p, const QRect &r, const QColor &,
		     int lineWidth = 1, const QBrush *fill = 0 );


//
// Other obsolete drawing functions. 
// Use QStyle::itemRect(), QStyle::drawItem() and QStyle::drawArrow() instead.
//

Q_EXPORT
QRect qItemRect( QPainter *p, Qt::GUIStyle gs, int x, int y, int w, int h,
		int flags, bool enabled,
		const QPixmap *pixmap, const QString& text, int len=-1 );

Q_EXPORT
void qDrawItem( QPainter *p, Qt::GUIStyle gs, int x, int y, int w, int h,
		int flags, const QColorGroup &g, bool enabled,
		const QPixmap *pixmap, const QString& text,
		int len=-1, const QColor* penColor = 0 );

Q_EXPORT
void qDrawArrow( QPainter *p, Qt::ArrowType type, Qt::GUIStyle style, bool down,
		 int x, int y, int w, int h,
		 const QColorGroup &g, bool enabled );

#endif // QT_NO_DRAWUTIL
#endif // QDRAWUTIL_H
