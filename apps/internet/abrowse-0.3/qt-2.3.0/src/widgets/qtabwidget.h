/****************************************************************************
** $Id$
**
** Definition of QTabWidget class
**
** Created : 990318
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

#ifndef QTABWIDGET_H
#define QTABWIDGET_H

#ifndef QT_H
#include "qwidget.h"
#include "qiconset.h"
#endif // QT_H

#ifndef QT_NO_TABWIDGET

class QTabBar;
class QTabWidgetData;
class QTab;
class QWidgetStack;


class Q_EXPORT QTabWidget : public QWidget
{
    Q_OBJECT
    Q_ENUMS( TabPosition )
    Q_ENUMS( TabShape )
    Q_PROPERTY( TabPosition tabPosition READ tabPosition WRITE setTabPosition )
    Q_PROPERTY( TabShape tabShape READ tabShape WRITE setTabShape )
    Q_PROPERTY( int margin READ margin WRITE setMargin )
    Q_PROPERTY( int currentPage READ currentPageIndex WRITE setCurrentPage )
    
public:
    
    QTabWidget( QWidget *parent, const char *name, WFlags f);
    QTabWidget( QWidget *parent=0, const char *name=0);
   ~QTabWidget();

    void addTab( QWidget *, const QString &);
    void addTab( QWidget *child, const QIconSet& iconset, const QString &label);
    void addTab( QWidget *, QTab* );

    void insertTab( QWidget *, const QString &, int index = -1);
    void insertTab( QWidget *child, const QIconSet& iconset, const QString &label, int index = -1);
    void insertTab( QWidget *, QTab*, int index = -1 );

    void changeTab( QWidget *, const QString &);
    void changeTab( QWidget *child, const QIconSet& iconset, const QString &label);

    bool isTabEnabled(  QWidget * ) const;
    void setTabEnabled( QWidget *, bool );

    void showPage( QWidget * );
    void removePage( QWidget * );
    QString tabLabel( QWidget * ); //### const in 3.0

    QWidget * currentPage() const;
    int currentPageIndex() const;
    void setCurrentPage( int );

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    enum TabPosition { Top, Bottom };
    TabPosition tabPosition() const;
    void setTabPosition( TabPosition );

    enum TabShape { Rounded, Triangular };
    TabShape tabShape() const;
    void setTabShape( TabShape s );
    
    int margin() const;
    void setMargin( int );

    QSizePolicy sizePolicy() const;

protected:
    void showEvent( QShowEvent * );
    void resizeEvent( QResizeEvent * );
    void setTabBar( QTabBar* );
    QTabBar* tabBar() const;
    void styleChange( QStyle& );
    void updateMask();
    bool eventFilter( QObject *, QEvent * );

signals:
    void currentChanged( QWidget * );
    void selected( const QString& ); // obsolete

private slots:
    void showTab( int i );

private:
    QTabWidgetData *d;
    void setUpLayout(bool = FALSE);
    void init();

#if 1 //def TOTAL_LOSER_COMPILER
    friend class QTabDialog;
#else
    friend void QTabDialog::setTabBar( QTabBar* );
    friend void QTabBar* QTabDialog::tabBar() const;
#endif

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QTabWidget( const QTabWidget & );
    QTabWidget& operator=( const QTabWidget & );
#endif
};

#endif // QT_NO_TABWIDGET

#endif // QTABWIDGET_H
