/****************************************************************************
** $Id$
**
** Definition of the dial widget
**
** Created : 990104
**
** Copyright (C) 1999-2000 Trolltech AS.  All rights reserved.
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


#ifndef QDIAL_H
#define QDIAL_H

#ifndef QT_H
#include "qwidget.h"
#include "qrangecontrol.h"
#endif // QT_H

#ifndef QT_NO_DIAL

//class QTimer;
class QDialPrivate;

class Q_EXPORT QDial: public QWidget, public QRangeControl
{
    Q_OBJECT
    Q_PROPERTY( bool tracking READ tracking WRITE setTracking )
    Q_PROPERTY( bool wrapping READ wrapping WRITE setWrapping )
    Q_PROPERTY( int notchSize READ notchSize )
    Q_PROPERTY( double notchTarget READ notchTarget WRITE setNotchTarget )
    Q_PROPERTY( bool notchesVisible READ notchesVisible WRITE setNotchesVisible )
    Q_PROPERTY( int minValue READ minValue WRITE setMinValue )
    Q_PROPERTY( int maxValue READ maxValue WRITE setMaxValue )
    Q_PROPERTY( int lineStep READ lineStep WRITE setLineStep )
    Q_PROPERTY( int pageStep READ pageStep WRITE setPageStep )
    Q_PROPERTY( int value READ value WRITE setValue )
	
public:
    QDial( QWidget *parent=0, const char *name=0 );
    QDial( int minValue, int maxValue, int pageStep, int value,
	   QWidget *parent=0, const char *name=0 );
    ~QDial();

    bool tracking() const;

    bool wrapping() const;

    int notchSize() const;

    virtual void setNotchTarget( double );
    double notchTarget() const;

    bool notchesVisible() const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    int	 minValue() const;
    int	 maxValue() const;
    void setMinValue( int );
    void setMaxValue( int );
    int	 lineStep() const;
    int	 pageStep() const;
    void setLineStep( int );
    void setPageStep( int );
    int  value() const;

public slots:
    virtual void setValue( int );
    void addLine();
    void subtractLine();
    void addPage();
    void subtractPage();
    virtual void setNotchesVisible( bool b );
    virtual void setWrapping( bool on );
    virtual void setTracking( bool enable );

signals:
    void valueChanged( int value );
    void dialPressed();
    void dialMoved( int value );
    void dialReleased();

protected:
    void resizeEvent( QResizeEvent * );
    void paintEvent( QPaintEvent * );

    void keyPressEvent( QKeyEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );
    void wheelEvent( QWheelEvent * );
    void focusInEvent( QFocusEvent * );
    void focusOutEvent( QFocusEvent * );

    void valueChange();
    void rangeChange();

    virtual void repaintScreen( const QRect *cr = 0 );

private:
    QDialPrivate * d;

    int valueFromPoint( const QPoint & ) const;
    double angle( const QPoint &, const QPoint & ) const;
    QPointArray calcArrow( double &a ) const;
    QRect calcDial() const;
    int calcBigLineSize() const;
    void calcLines();

private: // Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QDial( const QDial & );
    QDial &operator=( const QDial & );
#endif

};

#endif  // QT_NO_DIAL

#endif
