/****************************************************************************
** $Id$
**
** Definition of the CDE-like style class
**
** Created : 990513
**
** Copyright (C) 1999 by Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
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


#ifndef QCDESTYLE_H
#define QCDESTYLE_H

#ifndef QT_H
#include "qmotifstyle.h"
#endif // QT_H

#ifndef QT_NO_STYLE_CDE

class Q_EXPORT QCDEStyle : public QMotifStyle
{
    Q_OBJECT
public:
    QCDEStyle( bool useHighlightCols = FALSE );
    virtual ~QCDEStyle();
    int defaultFrameWidth() const;

    void drawArrow( QPainter *p, ArrowType type, bool down,
		    int x, int y, int w, int h,
		    const QColorGroup &g, bool enabled, const QBrush *fill = 0 );

    void drawIndicator( QPainter* p, int x, int y, int w, int h,  const QColorGroup &g,
		       int state, bool down = FALSE, bool enabled = TRUE );

    void drawExclusiveIndicator( QPainter* p,  int x, int y, int w, int h, const QColorGroup &g,
				 bool on, bool down = FALSE, bool enabled = TRUE );
    
};

#endif // QT_NO_STYLE_CDE

#endif // QCDESTYLE_H
