/****************************************************************************
** $Id$
**
** Definition of QMotifPlusStyle class
**
** Created : 2000.07.27
**
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
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

#ifndef QMOTIFPLUSSTYLE_H
#define QMOTIFPLUSSTYLE_H


#ifndef QT_H
#include <qmotifstyle.h>
#endif // QT_H

#ifndef QT_NO_STYLE_MOTIFPLUS

class Q_EXPORT QMotifPlusStyle : public QMotifStyle {
    Q_OBJECT

public:
    QMotifPlusStyle(bool hoveringHighlight = FALSE);
    virtual ~QMotifPlusStyle();

    void polish(QPalette &pal);
    void polish(QWidget *widget);
    void unPolish(QWidget*widget);

    void polish(QApplication *app);
    void unPolish(QApplication *app);

    void polishPopupMenu(QPopupMenu *menu);

    int defaultFrameWidth() const;

    void drawArrow(QPainter *p, ArrowType type, bool down,
		   int x, int y, int w, int h,
		   const QColorGroup &g, bool,
		   const QBrush * = 0);

    void drawMenuBarItem(QPainter *p, int x, int y, int w, int h, QMenuItem *mi,
			 QColorGroup &g, bool enabled, bool activated);
    void drawPopupMenuItem(QPainter *p, bool checkable, int maxpmw, int tab,
			   QMenuItem *mi, const QPalette &pl, bool act,
			   bool enabled, int x, int y, int w, int h);

    void drawPushButton(QPushButton *button, QPainter *p);
    void drawButton(QPainter *p, int x, int y, int w, int h,
		    const QColorGroup &g, bool sunken = FALSE,
		    const QBrush *fill = 0);
    void drawBevelButton(QPainter *p, int x, int y, int w, int h,
			 const QColorGroup &g, bool sunken = FALSE,
			 const QBrush *fill = 0);
    void getButtonShift(int &x, int &y);

    void drawComboButton(QPainter *p, int x, int y, int w, int h,
			 const QColorGroup &g, bool sunken = FALSE,
			 bool editable = FALSE, bool = TRUE,
			 const QBrush *fill = 0);
    QRect comboButtonRect(int x, int y, int w, int h);
    QRect comboButtonFocusRect(int x, int y, int w, int h);

    void drawIndicator(QPainter *p, int x, int y ,int w, int h,
		       const QColorGroup &g, int state,
		       bool = FALSE, bool = TRUE);
    QSize indicatorSize() const;

    void drawExclusiveIndicator(QPainter *p, int x, int y ,int w, int h,
				const QColorGroup &g, bool on,
				bool = FALSE, bool = TRUE);
    QSize exclusiveIndicatorSize() const;

    void drawPanel(QPainter * p, int x, int y, int w, int h,
		   const QColorGroup &g, bool sunken = FALSE,
		   int = 1, const QBrush * = 0);

    void scrollBarMetrics(const QScrollBar *, int &, int &, int &, int &);
    void drawScrollBarControls(QPainter* p, const QScrollBar* sb,
			       int sliderStart, uint controls,
			       uint activeControl);

    void drawTab(QPainter *p, const QTabBar *tabbar, QTab *tab, bool selected);

    void drawSlider(QPainter *p, int x, int y, int w, int h,
		    const QColorGroup &g, Orientation orientation,
		    bool, bool);
    void drawSliderGroove(QPainter *p, int x, int y, int w, int h,
			  const QColorGroup& g, QCOORD,
			  Orientation );


protected:
    bool eventFilter(QObject *, QEvent *);


private:
    bool useHoveringHighlight;
};


#endif // QT_NO_STYLE_MOTIFPLUS

#endif // QMOTIFPLUSSTYLE_H
