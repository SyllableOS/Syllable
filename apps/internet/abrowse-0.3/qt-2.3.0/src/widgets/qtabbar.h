/****************************************************************************
** $Id$
**
** Definition of QTab and QTabBar classes
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
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

#ifndef QTABBAR_H
#define QTABBAR_H

#ifndef QT_H
#include "qwidget.h"
#include "qpainter.h"
#include "qlist.h"
#include "qiconset.h"
#endif // QT_H

#ifndef QT_NO_TABBAR

class Q_EXPORT QTab
{
public:
    QTab(): enabled( TRUE ), id( 0 ), iconset( 0 ) {}
    virtual ~QTab();
    QTab( const QString& text )
	: label( text ), enabled( TRUE ), id( 0 ), iconset( 0 ) {}
    QTab( const QIconSet& icon, const QString& text = QString::null )
	: label( text ), enabled( TRUE ), id( 0 ), iconset( new QIconSet(icon) ) {}
#if 1
    void setText( const QString& text) { label = text; }
    QString text() const { return label; }
    void setIconSet( const QIconSet& icon ) { iconset = new QIconSet( icon ); }
    QIconSet* iconSet() const { return iconset; }
    void setRect( const QRect& rect ) { r = rect; }
    QRect rect() const { return r; }
    void setEnabled( bool enable ) { enabled = enable; }
    bool isEnabled() const { return enabled; }
    void setIdentifier( int i ) { id = i; }
    int identitifer() const { return id; }
#endif
// private: (public for compatibility, ### change 3.0)
    QString label;
    QRect r;    // the bounding rectangle of this - may overlap with others
    bool enabled;
    int id;
    QIconSet* iconset;     // optional iconset
};


struct QTabPrivate;


class Q_EXPORT QTabBar: public QWidget
{
    Q_OBJECT
    Q_ENUMS( Shape )
    Q_PROPERTY( Shape shape READ shape WRITE setShape )
    Q_PROPERTY( int currentTab READ currentTab WRITE setCurrentTab )
    Q_PROPERTY( int count READ count )
    Q_PROPERTY( int keyboardFocusTab READ keyboardFocusTab )

public:
    QTabBar( QWidget *parent = 0, const char *name = 0 );
   ~QTabBar();

    enum Shape { RoundedAbove, RoundedBelow,
		 TriangularAbove, TriangularBelow };

    Shape shape() const;
    virtual void setShape( Shape );

    void show();

    virtual int addTab( QTab * );
    virtual int insertTab( QTab *, int index = -1 );
    virtual void removeTab( QTab * );

    virtual void setTabEnabled( int, bool );
    bool isTabEnabled( int ) const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    QSizePolicy sizePolicy() const;

    int currentTab() const;
    int keyboardFocusTab() const;

    QTab * tab( int );
    int count() const;

    virtual void layoutTabs();

public slots:
    virtual void setCurrentTab( int );
    virtual void setCurrentTab( QTab * );

signals:
    void  selected( int );

protected:
    virtual void paint( QPainter *, QTab *, bool ) const; // ### not const
    virtual void paintLabel( QPainter*, const QRect&, QTab*, bool ) const;

    void focusInEvent( QFocusEvent *e );
    void focusOutEvent( QFocusEvent *e );

    virtual QTab * selectTab( const QPoint & p ) const;
    void updateMask();

    void resizeEvent( QResizeEvent * );
    void paintEvent( QPaintEvent * );
    void mousePressEvent ( QMouseEvent * );
    void mouseReleaseEvent ( QMouseEvent * );
    void keyPressEvent( QKeyEvent * );
    void styleChange( QStyle& );

    QList<QTab> * tabList();

private slots:
    void scrollTabs();

private:
    QList<QTab> * l;
    QList<QTab> * lstatic;
    void makeVisible( QTab* t  );
    void updateArrowButtons();
    QTabPrivate * d;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QTabBar( const QTabBar & );
    QTabBar& operator=( const QTabBar & );
#endif
};


#endif // QT_NO_TABBAR

#endif // QTABBAR_H
