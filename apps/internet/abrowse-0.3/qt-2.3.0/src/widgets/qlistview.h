/****************************************************************************
** $Id$
**
** Definition of QListView widget class
**
** Created : 970809
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

#ifndef QLISTVIEW_H
#define QLISTVIEW_H

class QPixmap;
class QFont;
class QHeader;
class QIconSet;

class QListView;
struct QListViewPrivate;
class QListViewItemIterator;

#ifndef QT_H
#include "qscrollview.h"
#endif // QT_H

#ifndef QT_NO_LISTVIEW


class Q_EXPORT QListViewItem: public Qt
{
    friend class QListViewItemIterator;
#if defined(_CC_MSVC_)
    friend class QListViewItem;
#endif

public:
    QListViewItem( QListView * parent );
    QListViewItem( QListViewItem * parent );
    QListViewItem( QListView * parent, QListViewItem * after );
    QListViewItem( QListViewItem * parent, QListViewItem * after );

    QListViewItem( QListView * parent,
		   QString,     QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null );
    QListViewItem( QListViewItem * parent,
		   QString,     QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null );

    QListViewItem( QListView * parent, QListViewItem * after,
		   QString,     QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null );
    QListViewItem( QListViewItem * parent, QListViewItem * after,
		   QString,     QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null );
    virtual ~QListViewItem();

    virtual void insertItem( QListViewItem * );
    virtual void takeItem( QListViewItem * );
    virtual void removeItem( QListViewItem * ); //obsolete, use takeItem instead

    int height() const;
    virtual void invalidateHeight();
    int totalHeight() const;
    virtual int width( const QFontMetrics&,
		       const QListView*, int column) const;
    void widthChanged(int column=-1) const;
    int depth() const;

    virtual void setText( int, const QString &);
    virtual QString text( int ) const;

    virtual void setPixmap( int, const QPixmap & );
    virtual const QPixmap * pixmap( int ) const;

    virtual QString key( int, bool ) const;
    virtual void sortChildItems( int, bool );

    int childCount() const { return nChildren; }

    bool isOpen() const { return open; }
    virtual void setOpen( bool );
    virtual void setup();

    virtual void setSelected( bool );
    bool isSelected() const { return selected; }

    virtual void paintCell( QPainter *, const QColorGroup & cg,
			    int column, int width, int alignment );
    virtual void paintBranches( QPainter * p, const QColorGroup & cg,
				int w, int y, int h, GUIStyle s );
    virtual void paintFocus( QPainter *, const QColorGroup & cg,
			     const QRect & r );

    QListViewItem * firstChild() const;
    QListViewItem * nextSibling() const { return siblingItem; }
    QListViewItem * parent() const;

    QListViewItem * itemAbove();
    QListViewItem * itemBelow();

    int itemPos() const;

    QListView *listView() const;

    virtual void setSelectable( bool enable );
    bool isSelectable() const { return selectable; }

    virtual void setExpandable( bool );
    bool isExpandable() const { return expandable; }

    void repaint() const;

    void sort(); // ######## make virtual in next major release
    void moveItem( QListViewItem *after );

protected:
    virtual void enforceSortOrder() const;
    virtual void setHeight( int );
    virtual void activate();

    bool activatedPos( QPoint & );
private:
    void init();
    void moveToJustAfter( QListViewItem * );
    int ownHeight;
    int maybeTotalHeight;
    int nChildren;

    uint lsc: 14;
    uint lso: 1;
    uint open : 1;
    uint selected : 1;
    uint selectable: 1;
    uint configured: 1;
    uint expandable: 1;
    uint is_root: 1;

    QListViewItem * parentItem;
    QListViewItem * siblingItem;
    QListViewItem * childItem;

    void * columns;

    friend class QListView;
};

class QCheckListItem;

class Q_EXPORT QListView: public QScrollView
{
    friend class QListViewItemIterator;
    friend class QListViewItem;
    friend class QCheckListItem;
    
    Q_OBJECT
    Q_ENUMS( SelectionMode )
    Q_PROPERTY( int columns READ columns )
    Q_PROPERTY( bool multiSelection READ isMultiSelection WRITE setMultiSelection DESIGNABLE false )
    Q_PROPERTY( SelectionMode selectionMode READ selectionMode WRITE setSelectionMode )
    Q_PROPERTY( int childCount READ childCount )
    Q_PROPERTY( bool allColumnsShowFocus READ allColumnsShowFocus WRITE setAllColumnsShowFocus )
    Q_PROPERTY( bool showSortIndicator READ showSortIndicator WRITE setShowSortIndicator )
    Q_PROPERTY( int itemMargin READ itemMargin WRITE setItemMargin )
    Q_PROPERTY( bool rootIsDecorated READ rootIsDecorated WRITE setRootIsDecorated )

public:
    QListView( QWidget * parent, const char *name, WFlags f );
    QListView( QWidget * parent = 0, const char *name = 0 ); // ##### merge with above in 3.0
    ~QListView();

    int treeStepSize() const;
    virtual void setTreeStepSize( int );

    virtual void insertItem( QListViewItem * );
    virtual void takeItem( QListViewItem * );
    virtual void removeItem( QListViewItem * ); // obsolete, use takeItem instead

    virtual void clear();

    QHeader * header() const;

    virtual int addColumn( const QString &label, int size = -1);
    virtual int addColumn( const QIconSet& iconset, const QString &label, int size = -1);
    void removeColumn( int index ); // #### make virtual in next major release!
    virtual void setColumnText( int column, const QString &label );
    virtual void setColumnText( int column, const QIconSet& iconset, const QString &label );
    QString columnText( int column ) const;
    virtual void setColumnWidth( int column, int width );
    int columnWidth( int column ) const;
    enum WidthMode { Manual, Maximum };
    virtual void setColumnWidthMode( int column, WidthMode );
    WidthMode columnWidthMode( int column ) const;
    int columns() const;

    virtual void setColumnAlignment( int, int );
    int columnAlignment( int ) const;

    void show();

    QListViewItem * itemAt( const QPoint & screenPos ) const;
    QRect itemRect( const QListViewItem * ) const;
    int itemPos( const QListViewItem * );

    void ensureItemVisible( const QListViewItem * );

    void repaintItem( const QListViewItem * ) const;

    virtual void setMultiSelection( bool enable );
    bool isMultiSelection() const;

    enum SelectionMode { Single, Multi, Extended, NoSelection  };
    void setSelectionMode( SelectionMode mode );
    SelectionMode selectionMode() const;

    virtual void clearSelection();
    virtual void setSelected( QListViewItem *, bool );
    bool isSelected( const QListViewItem * ) const;
    QListViewItem * selectedItem() const;
    virtual void setOpen( QListViewItem *, bool );
    bool isOpen( const QListViewItem * ) const;

    virtual void setCurrentItem( QListViewItem * );
    QListViewItem * currentItem() const;

    QListViewItem * firstChild() const;

    int childCount() const;

    virtual void setAllColumnsShowFocus( bool );
    bool allColumnsShowFocus() const;

    virtual void setItemMargin( int );
    int itemMargin() const;

    virtual void setRootIsDecorated( bool );
    bool rootIsDecorated() const;

    virtual void setSorting( int column, bool increasing = TRUE );
    void sort(); // #### make virtual in next major release

    virtual void setFont( const QFont & );
    virtual void setPalette( const QPalette & );

    bool eventFilter( QObject * o, QEvent * );

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    void setShowSortIndicator( bool show );
    bool showSortIndicator() const;

public slots:
    void invertSelection(); // ###### make virtual
    void selectAll( bool select ); // make virtual
    void triggerUpdate();
    void setContentsPos( int x, int y );

signals:
    void selectionChanged();
    void selectionChanged( QListViewItem * );
    void currentChanged( QListViewItem * );
    void clicked( QListViewItem * );
    void clicked( QListViewItem *, const QPoint &, int );
    void pressed( QListViewItem * );
    void pressed( QListViewItem *, const QPoint &, int );

    void doubleClicked( QListViewItem * );
    void returnPressed( QListViewItem * );
    void rightButtonClicked( QListViewItem *, const QPoint&, int );
    void rightButtonPressed( QListViewItem *, const QPoint&, int );
    void mouseButtonPressed( int, QListViewItem *, const QPoint& , int );
    void mouseButtonClicked( int, QListViewItem *,  const QPoint&, int );

    void onItem( QListViewItem *item );
    void onViewport();

    void expanded( QListViewItem *item );
    void collapsed( QListViewItem *item );

protected:
    void contentsMousePressEvent( QMouseEvent * e );
    void contentsMouseReleaseEvent( QMouseEvent * e );
    void contentsMouseMoveEvent( QMouseEvent * e );
    void contentsMouseDoubleClickEvent( QMouseEvent * e );

    void focusInEvent( QFocusEvent * e );
    void focusOutEvent( QFocusEvent * e );

    void keyPressEvent( QKeyEvent *e );

    void resizeEvent( QResizeEvent *e );

    void showEvent( QShowEvent * );

    void drawContentsOffset( QPainter *, int ox, int oy,
			     int cx, int cy, int cw, int ch );

    virtual void paintEmptyArea( QPainter *, const QRect & );
    void enabledChange( bool );
    void styleChange( QStyle& );

protected slots:
    void updateContents();
    void doAutoScroll();

private slots:
    void changeSortColumn( int );
    void updateDirtyItems();
    void makeVisible();
    void handleSizeChange( int, int, int );

private:
    void init();
    void updateGeometries();
    void buildDrawableList() const;
    void reconfigureItems();
    void widthChanged(const QListViewItem*, int c);
    void handleItemChange( QListViewItem *old, bool shift, bool control );
    void selectRange( QListViewItem *from, QListViewItem *to, bool invert, bool includeFirst, bool clearSel = FALSE );

    QListViewPrivate * d;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QListView( const QWidget & );
    QListView &operator=( const QWidget & );
#endif
};


class Q_EXPORT QCheckListItem : public QListViewItem
{
public:
    enum Type { RadioButton, CheckBox, Controller };

    QCheckListItem( QCheckListItem *parent, const QString &text,
		    Type = Controller );
    QCheckListItem( QListViewItem *parent, const QString &text,
		    Type = Controller );
    QCheckListItem( QListView *parent, const QString &text,
		    Type = Controller );
    QCheckListItem( QListViewItem *parent, const QString &text,
		    const QPixmap & );
    QCheckListItem( QListView *parent, const QString &text,
		    const QPixmap & );
    ~QCheckListItem();

    void paintCell( QPainter *,  const QColorGroup & cg,
		    int column, int width, int alignment );
    virtual void paintFocus( QPainter *, const QColorGroup & cg,
			     const QRect & r );
    int width( const QFontMetrics&, const QListView*, int column) const;
    void setup();

    virtual void setOn( bool );
    bool isOn() const { return on; }
    Type type() const { return myType; }
    QString text() const { return QListViewItem::text( 0 ); }
    QString text( int n ) const { return QListViewItem::text( n ); }

    void setEnabled( bool b );
    bool isEnabled() const;

protected:
    void paintBranches( QPainter * p, const QColorGroup & cg,
			int w, int y, int h, GUIStyle s );

    void activate();
    void turnOffChild();
    virtual void stateChange( bool );

private:
    void init();
    Type myType;
    bool on;
    QCheckListItem *exclusive;

    void *reserved;
};

class Q_EXPORT QListViewItemIterator
{
    friend struct QListViewPrivate;
    friend class QListView;
    friend class QListViewItem;

public:
    QListViewItemIterator();
    QListViewItemIterator( QListViewItem *item );
    QListViewItemIterator( const QListViewItemIterator &it );
    QListViewItemIterator( QListView *lv );

    QListViewItemIterator &operator=( const QListViewItemIterator &it );

    ~QListViewItemIterator();

    QListViewItemIterator &operator++();
    const QListViewItemIterator operator++( int );
    QListViewItemIterator &operator+=( int j );

    QListViewItemIterator &operator--();
    const QListViewItemIterator operator--( int );
    QListViewItemIterator &operator-=( int j );

    QListViewItem *current() const;

protected:
    QListViewItem *curr;
    QListView *listView;

private:
    void addToListView();
    void currentRemoved();

};

#endif // QT_NO_LISTVIEW

#endif // QLISTVIEW_H
