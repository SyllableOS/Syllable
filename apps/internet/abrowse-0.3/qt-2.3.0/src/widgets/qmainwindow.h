/****************************************************************************
** $Id$
**
** Definition of QMainWindow class
**
** Created : 980316
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

#ifndef QMAINWINDOW_H
#define QMAINWINDOW_H

#ifndef QT_H
#include "qwidget.h"
#endif // QT_H

#ifndef QT_NO_MAINWINDOW

class QMenuBar;
class QStatusBar;
class QToolTipGroup;
class QToolBar;
template <class type> class QList;

class QMainWindowPrivate;

class Q_EXPORT QMainWindow: public QWidget
{
    Q_OBJECT
    Q_PROPERTY( bool rightJustification READ rightJustification WRITE setRightJustification )
    Q_PROPERTY( bool usesBigPixmaps READ usesBigPixmaps WRITE setUsesBigPixmaps )
    Q_PROPERTY( bool usesTextLabel READ usesTextLabel WRITE setUsesTextLabel )
    Q_PROPERTY( bool toolBarsMovable READ toolBarsMovable WRITE setToolBarsMovable )
    Q_PROPERTY( bool opaqueMoving READ opaqueMoving WRITE setOpaqueMoving )

public:
    QMainWindow( QWidget * parent = 0, const char * name = 0, WFlags f = WType_TopLevel );
    ~QMainWindow();

#ifndef QT_NO_MENUBAR
    QMenuBar * menuBar() const;
#endif
    QStatusBar * statusBar() const;
    QToolTipGroup * toolTipGroup() const;

    virtual void setCentralWidget( QWidget * );
    QWidget * centralWidget() const;

    // ##### In moved to Qt:: in 3.0 and qtoolbar doesn't #include qmainwindow,
    // ##### then qmainwindow can include qtoolbar, so toolBars() below can
    // ##### work in bad compilers.
    enum ToolBarDock {
	Unmanaged, TornOff, Top, Bottom, Right, Left, Minimized
    };

    virtual void setDockEnabled( ToolBarDock dock, bool enable );
    bool isDockEnabled( ToolBarDock dock ) const;
    void setDockEnabled( QToolBar *tb, ToolBarDock dock, bool enable ); // ########### make virtual
    bool isDockEnabled( QToolBar *tb, ToolBarDock dock ) const;

    void addToolBar( QToolBar *, ToolBarDock = Top, bool newLine = FALSE );
    void addToolBar( QToolBar *, const QString &label,
		     ToolBarDock = Top, bool newLine = FALSE );
    void moveToolBar( QToolBar *, ToolBarDock = Top );
    void moveToolBar( QToolBar *, ToolBarDock, bool nl, int index, int extraOffset = -1 );

    void removeToolBar( QToolBar * );


    void show();
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    bool rightJustification() const;
    bool usesBigPixmaps() const;
    bool usesTextLabel() const;
    bool toolBarsMovable() const;
    bool opaqueMoving() const;

    bool eventFilter( QObject*, QEvent* );

    bool getLocation( QToolBar *tb, ToolBarDock &dock, int &index, bool &nl, int &extraOffset ) const;

// WARNING: compilers requiring Q_TEMPLATE_NEEDS_CLASS_DECLARATION
//	    may not be supported in future Qt versions.
#ifndef Q_TEMPLATE_NEEDS_CLASS_DECLARATION
    QList<QToolBar> toolBars( ToolBarDock dock ) const;
#endif
    void lineUpToolBars( bool keepNewLines = FALSE );

    bool isDockMenuEnabled() const;

public slots:
    virtual void setRightJustification( bool );
    virtual void setUsesBigPixmaps( bool );
    void setUsesTextLabel( bool ); // virtual 3.0
    void setToolBarsMovable( bool ); // virtual 3.0
    void setOpaqueMoving( bool ); // virtual 3.0
    void setDockMenuEnabled( bool );

    void whatsThis();

signals:
    void pixmapSizeChanged( bool );
    void usesTextLabelChanged( bool );
    void startMovingToolBar( QToolBar * );
    void endMovingToolBar( QToolBar * );
    void toolBarPositionChanged( QToolBar * );

protected slots:
    virtual void setUpLayout();

protected:
    void paintEvent( QPaintEvent * );
    void resizeEvent( QResizeEvent * );
    void childEvent( QChildEvent * );
    bool event( QEvent * );
    void styleChange( QStyle& );

private:
    QMainWindowPrivate * d;
    void triggerLayout( bool deleteLayout = TRUE);
    void moveToolBar( QToolBar *, QMouseEvent * );
    void rightMouseButtonMenu( const QPoint &p );

#ifndef QT_NO_MENUBAR
    virtual void setMenuBar( QMenuBar * );
#endif
    virtual void setStatusBar( QStatusBar * );
    virtual void setToolTipGroup( QToolTipGroup * );
    ToolBarDock findDockArea( const QPoint &pos, QRect &rect, QToolBar *tb, QRect *dockRect = 0 );
    void moveToolBar( QToolBar *, ToolBarDock, QToolBar *relative, int ipos );

    friend class QToolBar;
    friend class QMenuBar;
    friend class QHideDock;
private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QMainWindow( const QMainWindow & );
    QMainWindow& operator=( const QMainWindow & );
#endif
};

#endif // QT_NO_MAINWINDOW

#endif // QMAINWINDOW_H
