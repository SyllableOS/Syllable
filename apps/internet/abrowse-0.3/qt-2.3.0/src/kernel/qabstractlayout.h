/****************************************************************************
** $Id$
**
** Definition of the abstract layout base class
**
** Created : 960416
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

#ifndef QABSTRACTLAYOUT_H
#define QABSTRACTLAYOUT_H

#ifndef QT_H
#include "qobject.h"
#include "qsizepolicy.h"
#include "qwidget.h"
#endif // QT_H

#ifndef QT_NO_LAYOUT

class QMenuBar;
class QWidget;
struct QLayoutData;
class QLayoutItem;
class QLayout;
class QSpacerItem;
class QDomElement;
class QConfigureLayoutEvent;

class Q_EXPORT QGLayoutIterator : public QShared
{
public:
    virtual ~QGLayoutIterator();
    virtual QLayoutItem *next() = 0;
    virtual QLayoutItem *current() = 0;
    virtual QLayoutItem *takeCurrent() = 0;
};

class Q_EXPORT QLayoutIterator
{
public:
    QLayoutIterator( QGLayoutIterator *i ) :it(i) {}
    QLayoutIterator( const QLayoutIterator &i ) :it( i.it )
    { if ( it ) it->ref(); }
    ~QLayoutIterator() { if ( it && it->deref() ) delete it; }
    QLayoutIterator &operator=( const QLayoutIterator &i )
    {
	if ( i.it ) i.it->ref();
	if ( it && it->deref() ) delete it;
	it = i.it;
	return *this;
    }
    QLayoutItem *operator++() { return it ? it->next() : 0; }
    QLayoutItem *current() { return it ? it->current() : 0; }
    QLayoutItem *takeCurrent() { return it ? it->takeCurrent() : 0; }
    void deleteCurrent();
private:
    QGLayoutIterator *it;
};


class Q_EXPORT QLayoutItem
{
public:
    QLayoutItem( int alignment = 0 ) :align(alignment) {}
    virtual ~QLayoutItem();
    virtual QSize sizeHint() const = 0;
    virtual QSize minimumSize() const = 0;
    virtual QSize maximumSize() const = 0;
    virtual QSizePolicy::ExpandData expanding() const =0;
    virtual void setGeometry( const QRect& ) = 0;
    virtual QRect geometry() const = 0;
    virtual bool isEmpty() const = 0;
    virtual bool hasHeightForWidth() const;
    virtual int heightForWidth( int ) const;
    virtual void invalidate();

    virtual QWidget *widget();
    virtual QLayoutIterator iterator();
    virtual QLayout *layout();
    virtual QSpacerItem *spacerItem();

    int alignment() const { return align; }
    virtual void setAlignment( int a );
protected:
    int align;
};


class Q_EXPORT QSpacerItem : public QLayoutItem
{
 public:
    QSpacerItem( int w, int h,
		 QSizePolicy::SizeType hData=QSizePolicy::Minimum,
		 QSizePolicy::SizeType vData= QSizePolicy::Minimum )
	:width(w), height(h), sizeP(hData, vData )
	{}
    void changeSize( int w, int h,
		QSizePolicy::SizeType hData=QSizePolicy::Minimum,
		QSizePolicy::SizeType vData=QSizePolicy::Minimum );
    QSize sizeHint() const ;
    QSize minimumSize() const ;
    QSize maximumSize() const ;
    QSizePolicy::ExpandData expanding() const;
    bool isEmpty() const;
    void setGeometry( const QRect& );
    QRect geometry() const;
    QSpacerItem *spacerItem();
private:
    int width, height;
    QSizePolicy sizeP;
    QRect rect;
};


class Q_EXPORT QWidgetItem : public QLayoutItem
{
public:
    QWidgetItem( QWidget *w ) : wid(w) {}
    QSize sizeHint() const ;
    QSize minimumSize() const ;
    QSize maximumSize() const ;
    QSizePolicy::ExpandData expanding() const;
    bool isEmpty() const;
    void setGeometry( const QRect& ) ;
    QRect geometry() const;
    //void invalidate();
    virtual QWidget *widget();

    bool hasHeightForWidth() const;
    int heightForWidth( int ) const;

    const QSize &widgetSizeHint() const;

private:
    //QSize cachedSizeHint;
    QWidget *wid;
};


class Q_EXPORT QLayout : public QObject, public QLayoutItem
{
    Q_OBJECT
    Q_ENUMS( ResizeMode )
    Q_PROPERTY( int margin READ margin WRITE setMargin )
    Q_PROPERTY( int spacing READ spacing WRITE setSpacing )
    Q_PROPERTY( ResizeMode resizeMode READ resizeMode WRITE setResizeMode )

public:
    QLayout( QWidget *parent, int margin=0, int space=-1,
	     const char *name=0 );
    QLayout( QLayout *parentLayout, int space=-1, const char *name=0 );
    QLayout( int space=-1, const char *name=0 );

    ~QLayout();

    int margin() const { return outsideBorder; }
    int spacing() const { return insideSpacing; }

    virtual void setMargin( int );
    virtual void setSpacing( int );

    enum { unlimited = QWIDGETSIZE_MAX };
#if 1 //OBSOLETE
    int defaultBorder() const { return insideSpacing; }
    void freeze( int w, int h );
    void freeze() { setResizeMode( Fixed ); }
#endif

    enum ResizeMode { FreeResize, Minimum, Fixed };
    void setResizeMode( ResizeMode );
    ResizeMode resizeMode() const;

#ifndef QT_NO_MENUBAR
    virtual void  setMenuBar( QMenuBar *w );
    QMenuBar *menuBar() const { return menubar; }
#endif

    QWidget *mainWidget();
    bool isTopLevel() const { return topLevel; }

    virtual void setAutoAdd( bool );
    bool autoAdd() const { return autoNewChild; }

    void invalidate();
    QRect geometry() const;
    bool activate();

    void add( QWidget *w ) { addItem( new QWidgetItem( w ) ); }
    virtual void addItem ( QLayoutItem * ) = 0;

    QSizePolicy::ExpandData expanding() const;
    QSize minimumSize() const;
    QSize maximumSize() const;
    void setGeometry( const QRect& )=0;
    QLayoutIterator iterator()=0;
    bool isEmpty() const;

    int totalHeightForWidth( int w ) const;
    QSize totalMinimumSize() const;
    QSize totalMaximumSize() const;
    QSize totalSizeHint() const;
    QLayout *layout();

    bool supportsMargin() const { return marginImpl; }
    
    void setEnabled( bool );
    bool isEnabled() const;

protected:
    bool  eventFilter( QObject *, QEvent * );
    void addChildLayout( QLayout *l );
    void deleteAllItems();

    void setSupportsMargin( bool );
    QRect alignmentRect( const QRect& ) const;
private:
    void setWidgetLayout( QWidget *, QLayout * );
    void init();
    int insideSpacing;
    int outsideBorder;
    uint topLevel : 1;
    uint autoMinimum : 1;
    uint autoNewChild : 1;
    uint frozen : 1;
    uint activated : 1;
    uint marginImpl : 1;
    uint enabled : 1;
    QRect rect;
    QLayoutData *extraData;
#ifndef QT_NO_MENUBAR
    QMenuBar *menubar;
#endif
private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QLayout( const QLayout & );
    QLayout &operator=( const QLayout & );
#endif

};

inline void QLayoutIterator::deleteCurrent()
{
    delete takeCurrent();
}
#endif //QT_NO_LAYOUT
#endif
