/****************************************************************************
** $Id$
**
** Definition of QScrollView class
**
** Created : 970523
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
#ifndef QSCROLLVIEW_H
#define QSCROLLVIEW_H

#ifdef __ATHEOS__
#include "../../../konq-embed/dropin/atheos/ascrollview.h"
#else

#ifndef QT_H
#include "qframe.h"
#include "qscrollbar.h"
#endif // QT_H

#ifndef QT_NO_SCROLLVIEW

struct QScrollViewData;

class Q_EXPORT QScrollView : public QFrame
{
    Q_OBJECT
    Q_ENUMS( ResizePolicy ScrollBarMode )
    Q_PROPERTY( ResizePolicy resizePolicy READ resizePolicy WRITE setResizePolicy )
    Q_PROPERTY( ScrollBarMode vScrollBarMode READ vScrollBarMode WRITE setVScrollBarMode )
    Q_PROPERTY( ScrollBarMode hScrollBarMode READ hScrollBarMode WRITE setHScrollBarMode )
    Q_PROPERTY( int visibleWidth READ visibleWidth )
    Q_PROPERTY( int visibleHeight READ visibleHeight )
    Q_PROPERTY( int contentsWidth READ contentsWidth )
    Q_PROPERTY( int contentsHeight READ contentsHeight )
    Q_PROPERTY( int contentsX READ contentsX )
    Q_PROPERTY( int contentsY READ contentsY )
#ifndef QT_NO_DRAGANDDROP
    Q_PROPERTY( bool dragAutoScroll READ dragAutoScroll WRITE setDragAutoScroll )
#endif

public:
    QScrollView(QWidget *parent=0, const char *name=0, WFlags f=0);
    ~QScrollView();

    enum ResizePolicy { Default, Manual, AutoOne, AutoOneFit  };
    virtual void setResizePolicy( ResizePolicy );
    ResizePolicy resizePolicy() const;

    void styleChange(QStyle&);
    void removeChild(QWidget* child);
    virtual void addChild( QWidget* child, int x=0, int y=0 );
    virtual void moveChild( QWidget* child, int x, int y );
    int childX(QWidget* child);
    int childY(QWidget* child);
    bool childIsVisible(QWidget* child);
    void showChild(QWidget* child, bool yes=TRUE);

    enum ScrollBarMode { Auto, AlwaysOff, AlwaysOn };

    ScrollBarMode vScrollBarMode() const;
    virtual void  setVScrollBarMode( ScrollBarMode );

    ScrollBarMode hScrollBarMode() const;
    virtual void  setHScrollBarMode( ScrollBarMode );

    QWidget*     cornerWidget() const;
    virtual void setCornerWidget(QWidget*);

    QScrollBar*  horizontalScrollBar() const;
    QScrollBar*  verticalScrollBar() const;
    QWidget*	 viewport() const;
    QWidget*	 clipper() const;

    int		visibleWidth() const;
    int		visibleHeight() const;

    int		contentsWidth() const;
    int		contentsHeight() const;
    int		contentsX() const;
    int		contentsY() const;

    void	resize( int w, int h );
    void	resize( const QSize& );
    void	show();

    void	updateContents( int x, int y, int w, int h );
    void	updateContents( const QRect& r );
    void	repaintContents( int x, int y, int w, int h, bool erase=TRUE );
    void	repaintContents( const QRect& r, bool erase=TRUE );
//### make this const in 3.0
    void	contentsToViewport(int x, int y, int& vx, int& vy);
//### make this const in 3.0
    void	viewportToContents(int vx, int vy, int& x, int& y);
    QPoint	contentsToViewport(const QPoint&);
    QPoint	viewportToContents(const QPoint&);
    void	enableClipper(bool y);

    void     setStaticBackground(bool y);
    bool     hasStaticBackground() const;

    QSize	viewportSize( int, int ) const;
    QSizePolicy sizePolicy() const;
    QSize	sizeHint() const;
    QSize	minimumSizeHint() const;

    void	removeChild(QObject* child);

#ifndef QT_NO_DRAGANDDROP
    void	setDragAutoScroll( bool b ); // #### virtual in 3.0
    bool	dragAutoScroll() const;
#endif

signals:
    void	contentsMoving(int x, int y);

public slots:
    virtual void resizeContents( int w, int h );
    void	scrollBy( int dx, int dy );
    virtual void        setContentsPos( int x, int y );
    void	ensureVisible(int x, int y);
    void	ensureVisible(int x, int y, int xmargin, int ymargin);
    void	center(int x, int y);
    void	center(int x, int y, float xmargin, float ymargin);

    void	updateScrollBars();
    void	setEnabled( bool enable );

protected:
    void	resizeEvent(QResizeEvent*);
    void 	wheelEvent( QWheelEvent * );
    bool	eventFilter( QObject *, QEvent *e );

    virtual void contentsMousePressEvent( QMouseEvent* );
    virtual void contentsMouseReleaseEvent( QMouseEvent* );
    virtual void contentsMouseDoubleClickEvent( QMouseEvent* );
    virtual void contentsMouseMoveEvent( QMouseEvent* );
#ifndef QT_NO_DRAGANDDROP
    virtual void contentsDragEnterEvent( QDragEnterEvent * );
    virtual void contentsDragMoveEvent( QDragMoveEvent * );
    virtual void contentsDragLeaveEvent( QDragLeaveEvent * );
    virtual void contentsDropEvent( QDropEvent * );
#endif
    virtual void contentsWheelEvent( QWheelEvent * );

    virtual void viewportPaintEvent( QPaintEvent* );
    virtual void viewportResizeEvent( QResizeEvent* );
    virtual void viewportMousePressEvent( QMouseEvent* );
    virtual void viewportMouseReleaseEvent( QMouseEvent* );
    virtual void viewportMouseDoubleClickEvent( QMouseEvent* );
    virtual void viewportMouseMoveEvent( QMouseEvent* );
#ifndef QT_NO_DRAGANDDROP
    virtual void viewportDragEnterEvent( QDragEnterEvent * );
    virtual void viewportDragMoveEvent( QDragMoveEvent * );
    virtual void viewportDragLeaveEvent( QDragLeaveEvent * );
    virtual void viewportDropEvent( QDropEvent * );
#endif
    virtual void viewportWheelEvent( QWheelEvent * );

    virtual void drawContentsOffset(QPainter*, int ox, int oy,
		    int cx, int cy, int cw, int ch);
    virtual void drawContents(QPainter*, int cx, int cy, int cw, int ch);
    void	frameChanged();

    virtual void setMargins(int left, int top, int right, int bottom);
    int leftMargin() const;
    int topMargin() const;
    int rightMargin() const;
    int bottomMargin() const;

    bool focusNextPrevChild( bool next );

    virtual void setHBarGeometry(QScrollBar& hbar, int x, int y, int w, int h);
    virtual void setVBarGeometry(QScrollBar& vbar, int x, int y, int w, int h);

private:
    virtual void drawContents( QPainter* );
    void moveContents(int x, int y);

    QScrollViewData* d;

private slots:
    void hslide(int);
    void vslide(int);
#ifndef QT_NO_DRAGANDDROP
    void doDragAutoScroll();
    void startDragAutoScroll();
    void stopDragAutoScroll();
#endif

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QScrollView( const QScrollView & );
    QScrollView &operator=( const QScrollView & );
#endif
    void changeFrameRect(const QRect&);
};

#endif // QT_NO_SCROLLVIEW

#endif // __ATHEOS__

#endif // QSCROLLVIEW_H
