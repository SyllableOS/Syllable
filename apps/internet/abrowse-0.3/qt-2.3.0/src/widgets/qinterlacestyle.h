/****************************************************************************
** $Id$
**
** Implementation of QInterlaceStyle widget class
**
** Created : 22 January 2001
**
** Copyright (C) 1992-2001 Trolltech AS.  All rights reserved.
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

#ifndef QINTERLACESTYLE_H
#define QINTERLACESTYLE_H

#ifndef QT_H
#include "qmotifstyle.h"
#endif // QT_H

#ifndef QT_NO_STYLE_INTERLACE

#include <qpalette.h>

class QInterlaceStyle : public QMotifStyle
{
public:
    QInterlaceStyle();
    void polish( QApplication*);
    void unPolish( QApplication*);
    void polish( QWidget* );
    void unPolish( QWidget* );

    int defaultFrameWidth() const;
    QRect pushButtonContentsRect( QPushButton *btn );

    void drawFocusRect ( QPainter *, const QRect &, const QColorGroup &, const QColor * bg = 0, bool = FALSE );
    void drawButton( QPainter *p, int x, int y, int w, int h,
			     const QColorGroup &g, bool sunken = FALSE,
			     const QBrush *fill = 0 );
    void drawButtonMask ( QPainter * p, int x, int y, int w, int h );
    void drawBevelButton( QPainter *p, int x, int y, int w, int h,
			  const QColorGroup &g, bool sunken = FALSE,
			  const QBrush *fill = 0 );

    void drawPushButton( QPushButton* btn, QPainter *p);
    QSize indicatorSize () const;
    void drawIndicator ( QPainter * p, int x, int y, int w, int h, const QColorGroup & g, int state, bool down = FALSE, bool enabled = TRUE );
    void drawIndicatorMask( QPainter *p, int x, int y, int w, int h, int );
    QSize exclusiveIndicatorSize () const;
    void drawExclusiveIndicator( QPainter * p, int x, int y, int w, int h, const QColorGroup & g, bool on, bool down = FALSE, bool enabled = TRUE );
    void drawExclusiveIndicatorMask( QPainter * p, int x, int y, int w, int h, bool );
    QRect comboButtonRect ( int x, int y, int w, int h );
    void drawComboButton( QPainter *p, int x, int y, int w, int h, const QColorGroup &g, bool sunken, bool editable, bool enabled, const QBrush *fb );
    void drawPushButtonLabel( QPushButton* btn, QPainter *p);
    void drawPanel( QPainter *p, int x, int y, int w, int h,
		    const QColorGroup &, bool sunken,
		    int lineWidth, const QBrush *fill );

    void scrollBarMetrics( const QScrollBar* sb, int &sliderMin, int &sliderMax, int &sliderLength, int &buttonDim );
    void drawScrollBarControls( QPainter* p, const QScrollBar* sb, int sliderStart, uint controls, uint activeControl );
    void drawSlider( QPainter * p, int x, int y, int w, int h, const QColorGroup & g, Orientation, bool tickAbove, bool tickBelow );
    void drawSliderMask( QPainter * p, int x, int y, int w, int h, Orientation, bool tickAbove, bool tickBelow );
    void drawSliderGroove( QPainter * p, int x, int y, int w, int h, const QColorGroup & g, QCOORD c, Orientation );
    void drawSliderGrooveMask( QPainter * p, int x, int y, int w, int h, QCOORD c, Orientation );
    int splitterWidth() const;
    void drawSplitter( QPainter *p, int x, int y, int w, int h,
		      const QColorGroup &g, Orientation orient);

private:
    QPalette oldPalette;
};

#endif // QT_NO_STYLE_INTERLACE

#endif
