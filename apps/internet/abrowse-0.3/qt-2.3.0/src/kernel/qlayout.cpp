/****************************************************************************
** $Id$
**
** Implementation of layout classes
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

#include "qlayout.h"

#ifndef QT_NO_LAYOUT

#include "qapplication.h" //for postEvent() in QBoxLayout::setDirection
#include "qwidget.h"
#include "qlist.h"
#include "qsizepolicy.h"

#include "qlayoutengine_p.h"

// This cannot be a subclass of QLayoutItem, since it can contain different item classes.
class QLayoutBox
{
public:
    QLayoutBox( QLayoutItem *lit ) { item_ = lit; }

    QLayoutBox( QWidget *wid ) { item_ = new QWidgetItem( wid ); }
    QLayoutBox( int w, int h, QSizePolicy::SizeType hData=QSizePolicy::Minimum,
		QSizePolicy::SizeType vData= QSizePolicy::Minimum )
	{ item_ = new QSpacerItem( w, h, hData, vData ); }
    ~QLayoutBox() { delete item_; }

    QSize sizeHint() const { return item_->sizeHint(); }
    QSize minimumSize() const { return item_->minimumSize(); }
    QSize maximumSize() const { return item_->maximumSize(); }
    QSizePolicy::ExpandData expanding() const { return item_->expanding(); }
    bool isEmpty() const { return item_->isEmpty(); }

    bool hasHeightForWidth() const { return item_->hasHeightForWidth(); }
    int heightForWidth( int w ) const { return item_->heightForWidth(w); }

    void setAlignment( int a ) { item_->setAlignment( a ); }
    void setGeometry( const QRect &r ) { item_->setGeometry( r ); }
    int alignment() const { return item_->alignment(); }
    QLayoutItem *item() { return item_; }
    QLayoutItem *takeItem() { QLayoutItem *i=item_; item_=0; return i; }

private:
    friend class QLayoutArray;
    QLayoutItem *item_;
    int row, col;
};


class QMultiBox
{
public:
    QMultiBox( QLayoutBox *box, int toRow, int toCol )
	:box_(box), torow(toRow), tocol(toCol) {}
    ~QMultiBox() { delete box_; }
    QLayoutBox *box() { return box_; }
    QLayoutItem *takeItem() { return box_->takeItem(); }
private:
    friend class QLayoutArray;
    QLayoutBox *box_;
    int torow, tocol;
};

class QLayoutArray
{
public:
    QLayoutArray();
    QLayoutArray( int nRows, int nCols );
    ~QLayoutArray();

    void add( QLayoutBox*, int row, int col );
    void add( QLayoutBox*, int row1, int row2, int col1, int col2  );
    QSize sizeHint( int ) const;
    QSize minimumSize( int ) const;
    QSize maximumSize( int ) const;

    QSizePolicy::ExpandData expanding( int spacing);

    void distribute( QRect, int );
    int numRows() const { return rr; }
    int numCols() const { return cc; }
    void expand( int rows, int cols )
	{ setSize( QMAX(rows,rr), QMAX(cols,cc) ); }
    void setRowStretch( int r, int s ) { expand(r+1,0); rowData[r].stretch=s; }
    void setColStretch( int c, int s ) { expand(0,c+1); colData[c].stretch=s; }
    int rowStretch( int r ) const { return rowData[r].stretch; }
    int colStretch( int c ) const { return colData[c].stretch; }



    void setReversed( bool r, bool c ) { hReversed = c; vReversed = r; }
    void setDirty() { needRecalc = TRUE; hfw_width = -1; }
    bool isDirty() const { return needRecalc; }
    bool hasHeightForWidth( int space );
    int heightForWidth( int, int defB );

    bool findWidget( QWidget* w, int *row, int *col );

    void getNextPos( int &row, int &col ) { row = nextR; col = nextC; }
    uint count() const { return things.count() + (multi?multi->count():0); }
    QRect cellGeometry( int row, int col ) const;
private:
    void setNextPosAfter( int r, int c );
    void recalcHFW( int w, int s );
    void addHfwData ( QLayoutBox *box, int width );
    void init();
    QSize findSize( QCOORD QLayoutStruct::*, int ) const;
    void addData ( QLayoutBox *b, bool r = TRUE, bool c = TRUE );
    void setSize( int rows, int cols );
    void setupLayoutData( int space );
    void setupHfwLayoutData( int space );
    int rr;
    int cc;
    bool hReversed;
    bool vReversed;
    QArray<QLayoutStruct> rowData;
    QArray<QLayoutStruct> colData;
    QArray<QLayoutStruct> *hfwData;
    QList<QLayoutBox> things;
    QList<QMultiBox> *multi;
    bool needRecalc;
    bool has_hfw;
    int hfw_width;
    int hfw_height;
    int nextR;
    int nextC;
    bool addVertical;
    friend class QLayoutArrayIterator;
};

QLayoutArray::QLayoutArray()
{
    init();
}

QLayoutArray::QLayoutArray( int nRows, int nCols )
    :rowData(0), colData(0)
{
    init();
    if ( nRows  < 0 ) {
	nRows = 1;
	addVertical = FALSE;
    }
    if ( nCols  < 0 ) {
	nCols = 1;
	addVertical = TRUE;
    }
    setSize( nRows, nCols );
}

QLayoutArray::~QLayoutArray()
{
    delete multi;
    delete hfwData;
}


void QLayoutArray::init()
{
    addVertical = FALSE;
    setDirty();
    multi = 0;
    rr = cc = 0;
    nextR = nextC = 0;
    hfwData = 0;
    things.setAutoDelete( TRUE );
    hReversed = vReversed = FALSE;
}

bool QLayoutArray::hasHeightForWidth( int spacing )
{
    setupLayoutData( spacing );
    return has_hfw;
}

/* Assumes that setupLayoutData has been called.
   And that qGeomCalc has filled in colData with appropriate values
*/
void QLayoutArray::recalcHFW( int w, int spacing )
{
    //go through all children, using colData and heightForWidth()
    //and put the results in hfw_rowData
    if ( !hfwData )
	hfwData = new QArray<QLayoutStruct>( rr );
    setupHfwLayoutData( spacing );
    QArray<QLayoutStruct> &rData = *hfwData;

    int h = 0;
    int n = 0;
    for ( int r = 0; r < rr; r++ ) {
	h = h + rData[r].sizeHint;
	if ( !rData[r].empty )
	    n++;
    }
    if ( n )
	h += (n-1)*spacing;
    h = QMIN( QWIDGETSIZE_MAX, h );

    hfw_height = h;
    hfw_width = w;
}

int QLayoutArray::heightForWidth( int w, int spacing )
{
    setupLayoutData( spacing );
    if ( has_hfw && w != hfw_width ) {
	qGeomCalc( colData, 0, cc, 0, w, spacing );
	recalcHFW( w, spacing );
    }
    return hfw_height;
}



bool QLayoutArray::findWidget( QWidget* w, int *row, int *col )
{
    QListIterator<QLayoutBox> it( things );
    QLayoutBox * box;
    while ( (box=it.current()) != 0 ) {
	++it;
	if ( box->item()->widget() == w ) {
	    if ( row )
		*row = box->row;
	    if ( col )
		*col = box->col;
	    return TRUE;
	}
    }
    if ( multi ) {
	QListIterator<QMultiBox> it( *multi );
	QMultiBox * mbox;
	while ( (mbox=it.current()) != 0 ) {
	    ++it;
	    box = mbox->box();
	    if ( box->item()->widget() == w ) {
		if ( row )
		    *row = box->row;
		if ( col )
		    *col = box->col;
		return TRUE;
	    }

	}
    }
    return FALSE;
}


QSize QLayoutArray::findSize( QCOORD QLayoutStruct::*size, int spacer ) const
{
    QLayoutArray *This = (QLayoutArray*)this;
    This->setupLayoutData( spacer );
	    //###A very clever optimizer could cause trouble
    int w = 0;
    int h = 0;
    int n = 0;
    for ( int r = 0; r < rr; r++ ) {
	h = h + rowData[r].*size;
	if ( !rowData[r].empty )
	    n++;
    }
    if ( n )
	h += (n-1)*spacer;
    n = 0;
    for ( int c = 0; c < cc; c++ ) {
	w = w + colData[c].*size;
	if ( !colData[c].empty )
	    n++;
    }
    if ( n )
	w += (n-1)*spacer;
    w = QMIN( QWIDGETSIZE_MAX, w );
    h = QMIN( QWIDGETSIZE_MAX, h );

    return QSize(w,h);
}

QSizePolicy::ExpandData QLayoutArray::expanding( int spacing )
{
    setupLayoutData( spacing );
    bool hExp = FALSE;
    bool vExp = FALSE;

    for ( int r = 0; r < rr; r++ ) {
       vExp = vExp || rowData[r].expansive;
    }
    for ( int c = 0; c < cc; c++ ) {
	hExp = hExp || colData[c].expansive;
    }

    return (QSizePolicy::ExpandData) (( hExp ? QSizePolicy::Horizontal : 0 )
		  | ( vExp ? QSizePolicy::Vertical : 0 ) );
}

QSize QLayoutArray::sizeHint( int spacer ) const
{
    return findSize( &QLayoutStruct::sizeHint, spacer );
}

QSize QLayoutArray::maximumSize( int spacer ) const
{
    return findSize( &QLayoutStruct::maximumSize, spacer );
}

QSize QLayoutArray::minimumSize( int spacer ) const
{
    return findSize( &QLayoutStruct::minimumSize, spacer );
}

void QLayoutArray::setSize( int r, int c )
{
    if ( (int)rowData.size() < r ) {
	int newR = QMAX(r,rr*2);
	rowData.resize( newR );
	for ( int i = rr; i < newR; i++ )
	    rowData[i].init();
    }
    if ( (int)colData.size() < c ) {
	int newC = QMAX(c,cc*2);
	colData.resize( newC );
	for ( int i = cc; i < newC; i++ )
	    colData[i].init();
    }
    rr = r;
    cc = c;
}

void QLayoutArray::setNextPosAfter( int row, int col )
{
    if ( addVertical ) {
	if ( col > nextC || col == nextC && row >= nextR ) {
	    nextR = row + 1;
	    nextC = col;
	    if ( nextR >= rr ) {
		nextR = 0;
		nextC++;
	    }
	}
    } else {
	if ( row > nextR || row == nextR && col >= nextC ) {
	    nextR = row;
	    nextC = col + 1;
	    if ( nextC >= cc ) {
		nextC = 0;
		nextR++;
	    }
	}
    }
}

void QLayoutArray::add( QLayoutBox *box, int row, int col )
{
    expand( row+1, col+1 );
    box->row = row;
    box->col = col;
    things.append( box );
    setDirty();
    setNextPosAfter( row, col );
}


void QLayoutArray::add( QLayoutBox *box,  int row1, int row2,
			int col1, int col2  )
{
#ifdef CHECK_RANGE
    if ( row2 >= 0 && row2 < row1 )
	qWarning( "QGridLayout: multicell fromRow greater than toRow" );
    if ( col2 >= 0 && col2 < col1 )
	qWarning( "QGridLayout: multicell fromCol greater than toCol" );
#endif
    if ( row1 == row2 && col1 == col2 ) {
	add( box, row1, col1 );
	return;
    }
    expand( row2+1, col2+1 );
    box->row = row1;
    box->col = col1;
    QMultiBox *mbox = new QMultiBox( box, row2, col2 );
    if ( !multi ) {
	multi = new QList<QMultiBox>;
	multi->setAutoDelete(TRUE);
    }
    multi->append( mbox );
    setDirty();
    if ( col2 < 0 )
	col2 = cc - 1;

    setNextPosAfter( row2, col2 );
}


void QLayoutArray::addData ( QLayoutBox *box, bool r, bool c )
{
    QSize hint = box->sizeHint();
    QSize minS = box->minimumSize();
    QSize maxS = box->maximumSize();

    if ( c ) {
    colData[box->col].sizeHint = QMAX( hint.width(),
				      colData[box->col].sizeHint );
    colData[box->col].minimumSize = QMAX( minS.width(),
				      colData[box->col].minimumSize );

    qMaxExpCalc( colData[box->col].maximumSize, colData[box->col].expansive,
		maxS.width(), box->expanding() & QSizePolicy::Horizontal);

    }
    if ( r ) {
    rowData[box->row].sizeHint = QMAX( hint.height(),
				      rowData[box->row].sizeHint );
    rowData[box->row].minimumSize = QMAX( minS.height(),
				      rowData[box->row].minimumSize );

    qMaxExpCalc( rowData[box->row].maximumSize, rowData[box->row].expansive,
		maxS.height(), box->expanding() & QSizePolicy::Vertical);
    }
    if ( !box->isEmpty() ) {
	//empty boxes ( i.e. spacers) do not get borders. This is hacky, but compatible.
	if ( c )
	    colData[box->col].empty = FALSE;
	if ( r )
	    rowData[box->row].empty = FALSE;
    }

}


static void distributeMultiBox( QArray<QLayoutStruct> &chain, int spacing,
				int start, int end,
				int minSize, int sizeHint )
{
    //distribute the sizes somehow.

    int i;
    int w = 0;
    int wh = 0;
    int max = 0;
    bool exp = FALSE;
    bool stretch = FALSE;
    for ( i = start; i <= end; i++ ) {
	w += chain[i].minimumSize;
	wh += chain[i].sizeHint;
	max += chain[i].maximumSize;
	exp = exp || chain[i].expansive;
	stretch = stretch || chain[i].stretch == 0;
	chain[i].empty = FALSE;
    }
    w += spacing * (end-start);
    wh += spacing * (end-start);
    max += spacing * (end-start);

    if ( max < minSize ) { //implies w<minSize
	//we must increase the maximum size of at least one of the
	//items. qGeomCalc() will put the extra space in between
	//the items. We must recover that extra space and put it somewhere.
	//It does not really matter where, since the user can always
	//specify stretch factors and avoid this code.

	//qDebug( "Multicell bigger than maximumSize" );
	qGeomCalc( chain, start, end-start+1, 0, minSize, spacing );
	int pos = 0;
	for ( i = start; i <= end; i++ ) {
	    int nextPos = (i==end) ? minSize-1 : chain[i+1].pos;
	    int realSize = nextPos - pos;
	    if ( i != end )
		realSize -= spacing;
	    if ( chain[i].minimumSize < realSize )
		chain[i].minimumSize = realSize;
	    if ( chain[i].maximumSize < chain[i].minimumSize )
		chain[i].maximumSize = chain[i].minimumSize;
	    pos = nextPos;
 	}

    } else if ( w < minSize ) {
	//qDebug( "Big multicell" );
	qGeomCalc( chain, start, end-start+1, 0, minSize, spacing );
	for ( i = start; i <= end; i++ ) {
	    if ( chain[i].minimumSize < chain[i].size )
		chain[i].minimumSize = chain[i].size;
	}
    }

    if ( wh < sizeHint ) {
	qGeomCalc( chain, start, end-start+1, 0, sizeHint, spacing );
	for ( i = start; i <= end; i++ ) {
	    if ( chain[i].sizeHint < chain[i].size )
		chain[i].sizeHint = chain[i].size;
	}
    }
}


//#define QT_LAYOUT_DISABLE_CACHING

void QLayoutArray::setupLayoutData( int spacing )
{
#ifndef QT_LAYOUT_DISABLE_CACHING
    if ( !needRecalc )
	return;
#endif
    has_hfw = FALSE;
    int i;
    for ( i = 0; i < rr; i++ ) {
	rowData[i].initParameters();
    }
    for ( i = 0; i < cc; i++ ) {
	colData[i].initParameters();
    }
    QListIterator<QLayoutBox> it( things );
    QLayoutBox * box;
    while ( (box=it.current()) != 0 ) {
	++it;
	addData( box );
	has_hfw = has_hfw || box->item()->hasHeightForWidth();
    }

    if ( multi ) {
	QListIterator<QMultiBox> it( *multi );
	QMultiBox * mbox;
	while ( (mbox=it.current()) != 0 ) {
	    ++it;
	    QLayoutBox *box = mbox->box();
	    int r1 = box->row;
	    int c1 = box->col;
	    int r2 = mbox->torow;
	    int c2 = mbox->tocol;
	    if ( r2 < 0 )
		r2 = rr-1;
	    if ( c2 < 0 )
		c2 = cc-1;
	    QSize hint = box->sizeHint();
	    QSize min = box->minimumSize();
	    if ( box->hasHeightForWidth() ) {
		has_hfw = TRUE;
	    }
	    if ( r1 == r2 ) {
		addData( box, TRUE, FALSE );
	    } else {
		distributeMultiBox( rowData, spacing, r1, r2,
				    min.height(), hint.height() );
	    }
	    if ( c1 == c2 ) {
		addData( box, FALSE, TRUE );
	    } else {
		distributeMultiBox( colData, spacing, c1, c2,
				    min.width(), hint.width() );
	    }
	}
    }
    for ( i = 0; i < rr; i++ ) {
	rowData[i].expansive = rowData[i].expansive || rowData[i].stretch > 0;
    }
    for ( i = 0; i < cc; i++ ) {
	colData[i].expansive = colData[i].expansive || colData[i].stretch > 0;
    }


    needRecalc = FALSE;
}





void QLayoutArray::addHfwData ( QLayoutBox *box, int width )
{
    QArray<QLayoutStruct> &rData = *hfwData;
    if ( box->hasHeightForWidth() ) {
	int hint = box->heightForWidth( width );
	rData[box->row].sizeHint = QMAX( hint,
					 rData[box->row].sizeHint );
	rData[box->row].minimumSize = QMAX( hint,
					    rData[box->row].minimumSize );
    } else {

	QSize hint = box->sizeHint();
	QSize minS = box->minimumSize();
	rData[box->row].sizeHint = QMAX( hint.height(),
				     rData[box->row].sizeHint );
	rData[box->row].minimumSize = QMAX( minS.height(),
					rData[box->row].minimumSize );
    }
}


/*
  similar to setupLayoutData, but uses
  heightForWidth( colData ) instead of sizeHint
  assumes that setupLayoutData and qGeomCalc( colData ) has been called
 */
void QLayoutArray::setupHfwLayoutData( int spacing )
{
    QArray<QLayoutStruct> &rData = *hfwData;
    int i;
    for ( i = 0; i < rr; i++ ) {
	rData[i] = rowData[i];
	rData[i].minimumSize = rData[i].sizeHint = 0;
    }
    QListIterator<QLayoutBox> it( things );
    QLayoutBox * box;
    while ( (box=it.current()) != 0 ) {
	++it;
	addHfwData( box, colData[box->col].size );
    }
    if ( multi ) {
	QListIterator<QMultiBox> it( *multi );
	QMultiBox * mbox;
	while ( (mbox=it.current()) != 0 ) {
	    ++it;
	    QLayoutBox *box = mbox->box();
	    int r1 = box->row;
	    int c1 = box->col;
	    int r2 = mbox->torow;
	    int c2 = mbox->tocol;
	    if ( r2 < 0 )
		r2 = rr-1;
	    if ( c2 < 0 )
		c2 = cc-1;
	    int w = colData[c2].pos + colData[c2].size - colData[c1].pos;
	    if ( r1 == r2 ) {
		addHfwData( box, w );
	    } else {
		QSize hint = box->sizeHint();
		QSize min = box->minimumSize();
		if ( box->hasHeightForWidth() ) {
		    int hfwh = box->heightForWidth( w );
		    if ( hfwh > hint.height() )
			hint.setHeight( hfwh );
		    if ( hfwh > min.height() )
			min.setHeight( hfwh );
		}
		distributeMultiBox( rData, spacing, r1, r2,
				    min.height(), hint.height() );
	    }
	}
    }
    for ( i = 0; i < rr; i++ ) {
	rData[i].expansive = rData[i].expansive || rData[i].stretch > 0;
    }
}

void QLayoutArray::distribute( QRect r, int spacing )
{
    setupLayoutData( spacing );

    qGeomCalc( colData, 0, cc, r.x(), r.width(), spacing );
    QArray<QLayoutStruct> *rDataPtr;
    if ( has_hfw ) {
	recalcHFW( r.width(), spacing );
	qGeomCalc( *hfwData, 0, rr, r.y(), r.height(), spacing );
	rDataPtr = hfwData;
    } else {
	qGeomCalc( rowData, 0, rr, r.y(), r.height(), spacing );
	rDataPtr = &rowData;
    }
    QArray<QLayoutStruct> &rData = *rDataPtr; //cannot assign to reference

    QListIterator<QLayoutBox> it( things );
    QLayoutBox * box;
    while ( (box=it.current()) != 0 ) {
	++it;
	int x = colData[box->col].pos;
	int y = rData[box->row].pos;
	int w = colData[box->col].size;
	int h = rData[box->row].size;
	if ( hReversed )
	    x = r.left() + r.right() - x - w;
	if ( vReversed )
	    y = r.top() + r.bottom() - y - h;
	box->setGeometry( QRect( x, y, w, h ) );
    }
    if ( multi ) {
	QListIterator<QMultiBox> it( *multi );
	QMultiBox * mbox;
	while ( (mbox=it.current()) != 0 ) {
	    ++it;
	    QLayoutBox *box = mbox->box();
	    int r2 = mbox->torow;
	    int c2 = mbox->tocol;
	    if ( r2 < 0 )
		r2 = rr-1;
	    if ( c2 < 0 )
		c2 = cc-1;

	    int x = colData[box->col].pos;
	    int y = rData[box->row].pos;
	    int x2p = colData[c2].pos + colData[c2].size; // x2+1
	    int y2p = rData[r2].pos + rData[r2].size;    // y2+1
	    int w = x2p - x;
	    int h = y2p - y;
	    // this code is copied from above:
	    if ( hReversed )
		x = r.left() + r.right() - x - w;
	    if ( vReversed )
		y = r.top() + r.bottom() - y - h;
	    box->setGeometry( QRect( x, y, w, h ) );
	    //end copying
	}
    }
}

QRect QLayoutArray::cellGeometry( int row, int col ) const
{
    if ( row < 0 || row >= rr || col < 0 || col >= cc )
	return QRect();
    return QRect( colData[col].pos, rowData[row].pos,
		  colData[col].size, rowData[row].size );
}



class QLayoutArrayIterator : public QGLayoutIterator
{
public:
    QLayoutArrayIterator( QLayoutArray *a ) :array(a) { toFirst(); }
    uint count() const { return array->count(); }
    QLayoutItem *current() {
	if ( multi ) {
	    if ( !array->multi || idx >= int(array->multi->count()) )
		return 0;
	    return array->multi->at(idx)->box()->item();
	} else {
	    if ( idx >= int(array->things.count()) )
		return 0;
	    return array->things.at(idx)->item();
	}
    }
    void toFirst() { multi = FALSE; idx = 0; }
    QLayoutItem *next() {
	idx++;
	if ( !multi && idx >= int(array->things.count()) ) {
	    multi = TRUE; idx = 0;
	}
	return current();
    }
    QLayoutItem *takeCurrent() {
	QLayoutItem *item = 0;
	if ( multi ) {
	    QMultiBox *b = array->multi->take( idx );
	    item = b ? b->takeItem() : 0;
	    delete b;
	} else {
	    QLayoutBox *b = array->things.take( idx );
	    item = b? b->takeItem() : 0;
	    delete b;
	}
	return item;
    }
private:
    QLayoutArray *array;
    bool multi;
    int idx;
};


// BEING REVISED: reggie
/*!
  \class QGridLayout qlayout.h

  \brief The QGridLayout class lays out widgets in a grid.

  \ingroup geomanagement

  QGridLayout takes the space it gets (from its parent layout or from
  the mainWidget()), divides it up into rows and columns, and puts
  each of the widgets it manages into the correct cell(s).

  Columns and rows behave identically; we will discuss columns but
  there are equivalent functions for rows.

  Each column has a minimum width and a stretch factor.  The minimum
  width is the greatest of that set using addColSpacing() and the
  minimum width of each widget in that column.  The stretch factor is
  set using setColStretch() and determines how much of the available
  space the column will get, over and above its necessary minimum.

  Normally, each managed widget or layout is put into a cell of its
  own using addWidget(), addLayout(), or by the \link QLayout::setAutoAdd()
  auto-add facility\endlink, but you can also put widget
  into multiple cells using addMultiCellWidget().  If you do that,
  QGridLayout will make a guess at how to distribute the size over the
  columns/rows (based on the stretch factors). You can adjust the
  minimum width of each column/row using addColSpacing()/addRowSpacing().

  This illustration shows a fragment of a dialog with a five-column,
  three-row grid (the grid is shown overlaid in magenta):

  <img src="gridlayout.png" width="425" height="150">

  Columns 0, 2 and 4 in this dialog fragment are made up of a QLabel,
  a QLineEdit and a QListBox.  Columns 1 and 2 are placeholders, made
  with addColSpacing().	 Row 0 consists of three QLabel objects, row 1
  of three QLineEdit objects and row 2 of three QListBox objects.

  Since we did not want any space between the rows, we had to use
  placeholder columns to get the right amount of space between the
  columns.

  Note that the columns and rows are not equally wide/tall: If you
  want two columns to be equally wide, you must set the columns'
  minimum widths and stretch factors to be the same yourself.  You do
  this using addColSpacing() and setStretch().

  If the QGridLayout is not the top-level layout (ie. is not managing
  all of the widget's area and children), you must add it to its
  parent layout when you have created it, but before you can do
  anything with it.  The normal way to add a layout is by calling
  parentLayout->addLayout().

  Once you have done that, you can start putting widgets and other
  layouts in the cells of your grid layout using addWidget(),
  addLayout() and addMultiCellWidget().

  QGridLayout also includes two margin widths: The border width and
  the inter-box width.	The border width is the width of the reserved
  space along each of the QGridLayout's four sides.  The intra-widget
  width is the width of the automatically allocated spacing between
  neighbouring boxes.

  The border width defaults to 0, and the intra-widget width defaults
  to the same as the border width.  Both are set using arguments to
  the constructor.

  See also the \link layout.html Layout Overview \endlink documentation.
*/


/*! \base64 gridlayout.png

iVBORw0KGgoAAAANSUhEUgAAAakAAACWBAMAAACWWWqkAAAAFVBMVEUAAADc3NzAwMD/AP+A
gID///+goKQFrhiJAAAGTUlEQVR4nO2ca47kKAyAUSQOsD/qf5SZE/ReYFeq6d9o1Byh9/5H
WF5JeMeASaUyWD2pnhax/WEg5pEiyw1lJq/2oIcMqveRQfU+MqjeRwbV+8iNqX78fSP52Kj6
1pxW/4Gj5tDX21PNhEzmr3Or9YQ7HxEj03YBqxFXQnJO2lT7XztSBUam7QJWo3zN3eJTiZqc
yEJyt1SJQ7UbmaeZzOKPE9CkRSX+yYBFx2+PSlogy9Q3VpYRcZkU1QxrH3sLVLGW1TFFirn9
atH11pPKNTItlVQL0a3wmGpZTqByjczqU14IKBWwxkBdESTacl9MpX+DW7RHizk9ZryYatqp
ilrgbG6FtUA1PIkbYkWbJBgDtRHxz3SQsjFQP68gY2BXyeUWBS2jPLcgBNZjqyRDReI9PqPm
0NcLxKqkExfH6tWzB0y5QKzK1RTE6tlTfhkqHDVHvv7aqf4hHeVhqBqNGDVHvj4sKt5RNioU
NUe+DioEdwZVRs2g4mL0oLTN7IE7gooR0qxGUjHxk9LkxorV24O5I6k4r6+5nUoS0ZSmgIoy
JsrKexqqNOnOSkWEQ1T8Jj9L7LixKqGiRHJR1lClSXcMFRNXwqj+LLFTT6WBKG5rdPuVpNHV
xtKuZdRUUolho18LZDsVIWdSqdJ9W6A2dGqsTuhXL6A6YwxULZDSBirg86qfjIzpWM2gajMI
cmdQZdQUUP37V0dZqRqNrFQHah53Xw9sXNRKrGG5S161K2eempSvW7HOK2ePz/+UfLetnHlq
Ur5uxU6g+v5EobLU5KhUsUFV6Q5/olBZanJUqphNJdNMWus/yB1JJbPSRjWKSk0/E8XcWJXb
O3bn+/fPT5uKkfKa89RIX7nUQxPFfCpKGKP6HgzEBJWag8jf5CfAToSKlFHJZSZZnmE0RmGH
L8vTp2JCu5hf6U+AHU9NhkoXi8RK/VCc1hihkv2KGStEmzqLSi7V4bTA79/L8vPTidVORTmU
ylGTpjLFUrESgtMCI1R0o2LgWOFQofUrrjKzZipHTZrKFItTIY6BzxiVHPt0C6RAqmeEKvK8
esao0EXUnpSv5tzCUZPOLUyxc6ieOFRPGNUZsZJuIFDZajJUutjtqdoWShLrIj4VipqUr4Oq
Ra5ARbm7x9O+LxKh0vt9ZapjVCqrq6NqlQxVgxrpq8whfS0ZKr0Lo3Z75EEFQtM7KnVUMmvh
ep+ngUpVPZjK7JitO3PFG2fHVLbqRipQCxRDfkjFw7tbqbiluoXK15CJld6BljvRq2kGbyow
KkZwqHy3ci2Qayp9bsD8F5eK4sQqUHDQr8xe8sX7VQGVGgPNXjJbx8DqIT5KJVo5wxgDg4Ne
f1JugS+DCq5mUNX7DnZnUGXUDCouE8G9CMqxxxgVIXw98Fippiy7bTrpC3HnwzoCV1Jr8fkV
ZwHVV7DKaZBUbkEZUzMs1hixWG6hwmRmWDodO0wyUtltQBWu3a4TMZ2nmXStMXYpKrIeheUU
khCCqcKTCS7V+tGHykmda6mCngmiUjOsM6gOX0eIUwVzCWCsmgdCYKwK1RgqEubsMar9pG/H
FqgP3u79itX3K1is1KNEbWuuq0wdqPTzahsDzRyuJxW6HOUWwFoD5xaDql7+TKo6NYOq3new
O4Mqo6aMiugHR8UGE9AdTKro/lU8Y9r3lnDeletHFd+/SlGphELvXTHWtHfVlyq+Ip2iotZC
OG1aY+9PFc6F0/3K2mC6OJWffGdmIsTaYLo4FbRfcacFDqprU6m9JXnZ5liXpQI+r/DlCrkF
vgwquJpBVe872J1BlVEzqLhaqms/5ePa+XLPs6OoKXteMfvQD9Ku3BOH6ulTlcyvrLNL+nSM
nGI1RO2hT5jz9hZoq0lnTKZYiopRvm1jNUTt4Z/SR1GTOaGqi3lU+xE3ZqZabesXwTs9KGrS
sQrf6Vk7lYmNmWWxxhZ4Kaptss+aFmaCd+VQ1GRmIrqYNwayCFVbv3o9lVpY2p9XeqrV2AK9
d1BR1GSeV8E7qPW+g93BpMoV60/lvtuNoiaXMfnvdtf7DnbnRlTOdyagqMlR+d+ZUO872J3z
qbp8G47/XSSVRvzvIkmoiWS39QGBiKHCUXPo692/HfvV3zaEKRvV7WRQvY8MqveRQfU+Mqje
RwbV+8hNqf4Hob4MkPD3BvwAAAAASUVORK5CYII=

*/

/*!
  Constructs a new QGridLayout with \a nRows rows, \a nCols columns
   and main widget \a  parent.	\a parent may not be 0.

  \a border is the number of pixels between the edge of the widget and
  the managed children.	 \a space is the default number of pixels
  between cells.  If \a space is -1 the value
  of \a border is used.

  \a name is the internal object name.
*/

QGridLayout::QGridLayout( QWidget *parent, int nRows, int nCols, int border ,
			  int space , const char *name )
    : QLayout( parent, border, space, name )
{
    init( nRows, nCols );
}



/*!
  Constructs a new grid that is placed inside \a parentLayout,
  with \a nRows rows and \a  nCols columns,
  If \a space is -1, this QGridLayout will inherits its parent's
  spacing(), otherwise \a space is used.

  This grid is placed according to  \a parentLayout's default placement
  rules.
*/

QGridLayout::QGridLayout( QLayout *parentLayout, int nRows, int nCols,
  int space, const char *name )
    : QLayout( parentLayout, space, name )
{
    init( nRows, nCols );
}


/*!
  Constructs a new grid with \a nRows rows and \a  nCols columns,
  If \a space is -1, this QGridLayout will inherits its parent's
  spacing(), otherwise \a space is used.

  You have to insert this grid into another layout. You can insert
  widgets and layouts in this layout at any time, but layout will not
  be performed before it is inserted.
*/

QGridLayout::QGridLayout( int nRows, int nCols,
			  int space, const char *name )
     : QLayout( space, name )
{
    init( nRows, nCols );
}


/*!
  Destructs the grid layout. Geometry management is terminated if
  this is a top-level grid.
*/

QGridLayout::~QGridLayout()
{
    delete array;
}

/*!
  Returns the number of rows in this grid.
  */
int QGridLayout::numRows() const
{
    return array->numRows();
}

/*!
  Returns the number of columns in this grid.
  */
int QGridLayout::numCols() const
{
    return array->numCols();
}


/*!
  Returns the preferred size of this grid.
*/

QSize QGridLayout::sizeHint() const
{
    return array->sizeHint( spacing() ) + QSize(2*margin(),2*margin());
}
/*!
  Returns the minimum size needed by this grid.
*/

QSize QGridLayout::minimumSize() const
{
    return array->minimumSize( spacing() ) + QSize(2*margin(),2*margin());
}


static const int HorAlign = Qt::AlignHCenter | Qt::AlignRight | Qt::AlignLeft;
static const int VerAlign = Qt::AlignVCenter | Qt::AlignBottom | Qt::AlignTop;


/*!
  Returns the maximum size needed by this grid.
*/

QSize QGridLayout::maximumSize() const
{
    QSize s =   array->maximumSize( spacing() ) + QSize(2*margin(),2*margin())
	.boundedTo(QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
    if ( alignment() & HorAlign )
	s.setWidth( QWIDGETSIZE_MAX );
    if ( alignment() & VerAlign )
	s.setHeight( QWIDGETSIZE_MAX );

    return s;
}




/*!
  Returns whether this layout's preferred height depends on its width.
*/

bool QGridLayout::hasHeightForWidth() const
{
    return ((QGridLayout*)this)->array->hasHeightForWidth( spacing() );
}


/*!
  Returns the layout's preferred height when it is \a w pixels wide.
*/

int QGridLayout::heightForWidth( int w ) const
{
    QGridLayout *that = ((QGridLayout*)this);
    return that->array->heightForWidth( w - 2*margin(), spacing() )
	+ 2*margin();

}




/*!
  Searches for \a w in this layout (not including child layouts).  If
  \a w is found, it sets \a row and \a col to the row and column and
  returns TRUE. If \a w is not found, FALSE is returned.

  \warning If a widget spans  multiple rows/columns, the top-left cell is returned.
*/

bool QGridLayout::findWidget( QWidget* w, int *row, int *col )
{
    return array->findWidget( w, row, col );
}

/*!
  Resizes managed widgets within the rectangle \a r.
 */
void QGridLayout::setGeometry( const QRect &r )
{
    if ( array->isDirty() || r != geometry() ) {
	QLayout::setGeometry( r );
	QRect cr = alignment() ? alignmentRect(r) : r;
	QRect s( cr.x()+margin(), cr.y()+margin(),
		 cr.width()-2*margin(), cr.height()-2*margin() );
	array->distribute( s, spacing() );
    }
}


/*!
  Returns the geometry of the cell with row \a row, and column
  \a col in the grid. Returns an invalid rectangle if \a row or
  \a col is outside the grid.

  \warning in the current version of Qt, this function does not return
  valid results until setGeometry() has been called, ie. after the
  mainWidget() is visible.

*/
QRect QGridLayout::cellGeometry( int row, int col ) const
{
    return array->cellGeometry( row, col );
}



/*!
  Expands this grid so that it will have \a nRows rows and \a nCols columns.
  Will not shrink the grid. You should not need to call this function, as
  QGridLayout expands automatically as new items are inserted.
 */
void QGridLayout::expand( int nRows, int nCols )
{
    array->expand( nRows, nCols );
}

/*!
  Sets up the table and other internal stuff
*/

void QGridLayout::init( int nRows, int nCols )
{
    setSupportsMargin( TRUE );
    array = new QLayoutArray( nRows, nCols );
}

/*!
  Adds \a item to the next free position of this layout.
*/

void QGridLayout::addItem( QLayoutItem *item )
{
    int r,c;
    array->getNextPos( r, c );
    add( item, r, c );
}

/*!
  Adds \a item at position \a row, \a col. The layout takes over ownership
  of \a item.
*/

void QGridLayout::addItem( QLayoutItem *item, int row, int col )
{
    add( item, row, col );
}


/*!
  Adds \a item at position \a row, \a col. The layout takes over ownership
  of \a item.
*/

void QGridLayout::add( QLayoutItem *item, int row, int col )
{
    QLayoutBox *box = new QLayoutBox( item );
    array->add( box, row, col );
}



/*!
  Adds the \a item to the cell grid, spanning multiple rows/columns.

  Alignment is specified by \a alignment which is a bitwise OR of
  Qt::AlignmentFlags values.
  The default alignment is 0, which means
  that the widget fills the entire cell.

*/

void QGridLayout::addMultiCell( QLayoutItem *item, int fromRow, int toRow,
  int fromCol, int toCol, int alignment )
{
    QLayoutBox *b = new QLayoutBox( item );
    b->setAlignment( alignment );
    array->add( b, fromRow, toRow, fromCol, toCol );
}

/*
  Returns TRUE if w can be added to l
*/
static bool checkWidget( QLayout *l, QWidget *w )
{
    if ( !w ) {
#if defined(CHECK_STATE)
	qWarning( "cannot add null widget to %s/%s", l->className(),
		  l->name() );
#endif
	return FALSE;
    }
#if defined(CHECK_STATE)
    if ( w->parentWidget() != l->mainWidget() ) {
	if ( w->parentWidget() && l->mainWidget() )
	    qWarning( "Warning: adding %s/%s (child of %s/%s) to layout for %s/%s",
		      w->className(), w->name(),
		      w->parentWidget()->className(), w->parentWidget()->name(),
		      l->mainWidget()->className(), l->mainWidget()->name() );
	else if (  l->mainWidget() )
	    qWarning( "Warning: adding %s/%s (top-level widget) to layout for %s/%s",
		      w->className(), w->name(),
		      l->mainWidget()->className(), l->mainWidget()->name() );
    }
#endif
    return TRUE;
}


/*!
  Adds the widget \a w to the cell grid at \a row, \a col.
  The top left position is (0,0)

  Alignment is specified by \a alignment which is a bitwise OR of
  Qt::AlignmentFlags values.
  The default alignment is 0, which means
  that the widget fills the entire cell.

  Note 1: You should not call this if you have enabled the
  \link QLayout::setAutoAdd() auto-add facility of the layout\endlink.

  Note 2: The \a alignment parameter is interpreted more aggressively
  than in previous versions of Qt.  A non-default alignment now
  indicates that the widget should not grow to fill the available
  space, but should be sized according to sizeHint().

*/

void QGridLayout::addWidget( QWidget *w, int row, int col, int alignment )
{
    if ( !checkWidget( this, w ) )
	return;
    if ( row < 0 || col < 0 ) {
#if defined(CHECK_STATE)
	qWarning( "cannot add %s/%s to %s/%s at row %d col %d",
		 w->className(), w->name(),
		 className(), name(),
		 row, col );
#endif
	return;
    }
    QWidgetItem *b = new QWidgetItem( w );
    b->setAlignment( alignment );
    add( b, row, col );
}

/*!
  Adds the widget \a w to the cell grid, spanning multiple rows/columns.

  Alignment is specified by \a alignment which is a bitwise OR of
  Qt::AlignmentFlags values.
  The default alignment is 0, which means
  that the widget fills the entire cell.

  A non-zero alignment indicates that the widget should not grow to
  fill the available space, but should be sized according to
  sizeHint().

*/

void QGridLayout::addMultiCellWidget( QWidget *w, int fromRow, int toRow,
				      int fromCol, int toCol, int alignment )
{
    QLayoutBox *b = new QLayoutBox( w );
    b->setAlignment( alignment );
    array->add( b, fromRow, toRow, fromCol, toCol );
}


/*!
  Places another layout at position (\a row, \a col) in the grid.
  The top left position is (0,0)
*/

void QGridLayout::addLayout( QLayout *layout, int row, int col)
{
    addChildLayout( layout );
    add( layout, row, col );
}


/*!
  Adds the layout \a w to the cell grid, spanning multiple rows/columns.

  Alignment is specified by \a alignment which is a bitwise OR of
  Qt::AlignmentFlags values.
  The default alignment is 0, which means
  that the widget fills the entire cell.

  A non-zero alignment indicates that the layout should not grow to
  fill the available space, but should be sized according to
  sizeHint().

*/

void QGridLayout::addMultiCellLayout( QLayout *layout, int fromRow, int toRow,
				      int fromCol, int toCol, int alignment )
{
    QLayoutBox *b = new QLayoutBox( layout );
    b->setAlignment( alignment );
    array->add( b, fromRow, toRow, fromCol, toCol );
}



/*!
  Sets the stretch factor of row \a row to \a stretch.
  The first row is number 0.

  The stretch factor  is relative to the other rows in this grid.
  Rows with higher stretch factor take more of the available space.

  The default stretch factor is 0.
  If the stretch factor is 0 and no other row in this table can
  grow at all, the row may still grow.

  \sa rowStretch(), addRowSpacing(), setColStretch()
*/

void QGridLayout::setRowStretch( int row, int stretch )
{
    array->setRowStretch( row, stretch );
}


/*!
  Returns the stretch factor for row \a row.
  \sa setRowStretch()
*/

int QGridLayout::rowStretch( int row ) const
{
    return array->rowStretch( row );
}


/*!
  Returns the stretch factor for column \a col.
  \sa setColStretch()
*/

int QGridLayout::colStretch( int col ) const
{
    return array->colStretch( col );
}

/*!
  Sets the stretch factor of column \a col to \a stretch.
  The first column is number 0.

  The stretch factor  is relative to the other columns in this grid.
  Columns with higher stretch factor take more of the available space.

  The default stretch factor is 0.
  If the stretch factor is 0 and no other column in this table can
  grow at all, the column may still grow.

  \sa colStretch(), addColSpacing(), setRowStretch()
*/

void QGridLayout::setColStretch( int col, int stretch )
{
    array->setColStretch( col, stretch );
}


/*!
  Sets the minimum height of \a row to \a minsize pixels.
 */
void QGridLayout::addRowSpacing( int row, int minsize )
{
    QLayoutItem *b = new QSpacerItem( 0, minsize );
    //b.setAlignment( align );
    add( b, row, 0 );
}

/*!
  Sets the minimum width of \a col to \a minsize pixels.
 */
void QGridLayout::addColSpacing( int col, int minsize )
{
    QLayoutItem *b = new QSpacerItem( minsize, 0 );
    //b.setAlignment( align );
    add( b, 0, col );
}



/*!
  Returns the expansiveness of this layout
*/

QSizePolicy::ExpandData QGridLayout::expanding() const
{
    return array->expanding( spacing() );
}

/*!
  Sets which of the four corners of the grid corresponds to (0,0).
*/

void QGridLayout::setOrigin( Corner c )
{
    array->setReversed( c == BottomLeft || c == BottomRight,
			c == TopRight || c == BottomRight );
}
/*!
  Resets cached information.
*/

void QGridLayout::invalidate()
{
    QLayout::invalidate();
    QLayout::setGeometry( QRect() ); //###binary compatibility
    array->setDirty();
}


/*!
  \reimp
*/

QLayoutIterator QGridLayout::iterator()
{
    return QLayoutIterator( new QLayoutArrayIterator( array ) );
}



struct QBoxLayoutItem
{
    QBoxLayoutItem( QLayoutItem *it, int stretch_ = 0)
	: item(it), stretch(stretch_), magic(FALSE) {}
    ~QBoxLayoutItem() { delete item; }
    int hfw( int w ) {
	if ( item->hasHeightForWidth() )
	    return item->heightForWidth( w );
	else
	    return item->sizeHint().height();
    }
    QLayoutItem *item;
    int stretch;
    bool magic;
};

class QBoxLayoutData
{
public:
    QBoxLayoutData() :geomArray(0), hfwWidth(-1),dirty(TRUE) {
	list.setAutoDelete( TRUE );
    }
    ~QBoxLayoutData() { delete geomArray; }
    void setDirty() {
	delete geomArray;
	geomArray=0;
	hfwWidth=hfwHeight=-1;
	dirty = TRUE;
    }

    QList<QBoxLayoutItem> list;
    QArray<QLayoutStruct> *geomArray;
    int hfwWidth;
    int hfwHeight;
    QSize sizeHint;
    QSize minSize;
    QSize maxSize;
    QSizePolicy::ExpandData expanding;
    bool hasHfw;
    bool dirty;
};



class QBoxLayoutIterator : public QGLayoutIterator
{
public:
    QBoxLayoutIterator( QBoxLayoutData *d ) :data(d),idx(0) {}
    QLayoutItem *current() {
	if ( idx >= int(data->list.count()) )
	    return 0;
	return data->list.at(idx)->item;
    }
    QLayoutItem *next() {
	idx++;
	return current();
    }
    QLayoutItem *takeCurrent() {
	QLayoutItem *item = 0;

	QBoxLayoutItem *b = data->list.take( idx );
	if ( b ) {
	    item = b->item;
	    b->item = 0;
	    delete b;
	}
	return item;
    }

private:
    QBoxLayoutData *data;
    int idx;
};




/*!
  \class QBoxLayout qlayout.h

  \brief The QBoxLayout class lines up child widgets horizontally or
  vertically.

  \ingroup geomanagement

  QBoxLayout takes the space it gets (from its parent layout or from
  the mainWidget()), divides it up into a row of boxes and makes each
  managed widget fill one box.

  If the QBoxLayout is \c Horizontal, the boxes are beside each other,
  with suitable sizes.	Each widget (or other box) will get at least
  its minimum sizes and at most its maximum size, and any excess space
  is shared according to the stretch factors (more about that below).

  If the QBoxLayout is \c Vertical, the boxes are above and below each
  other, again with suitable sizes.

  The easiest way to create a QBoxLayout is to use one of the
  convenience classes QHBoxLayout (for \c Horizontal boxes) or
  QVBoxLayout (for \c Vertical boxes). You can also use the QBoxLayout
  constructor directly, specifying its direction as \c LeftToRight, \c
  Down, \c RightToLeft or \c Up.

  If the QBoxLayout is not the top-level layout (ie. is not managing
  all of the widget's area and children), you must add it to its
  parent layout before you can do anything with it.  The normal way to
  add a layout is by calling parentLayout->addLayout().

  Once you have done that, you can add boxes to the QBoxLayout using
  one of four functions: <ul>

  <li> addWidget() to add a widget to the QBoxLayout and set the
  widget's stretch factor.  (The stretch factor is along the row of
  boxes.)

  <li> addSpacing() to create an empty box; this is one of the
  functions you use to create nice and spacious dialogs.  See below
  for ways to set margins.

  <li> addStretch() to create an empty, stretchable box.

  <li> addLayout() to add a box containing another QLayout to the row
  and set that layout's stretch factor.

  </ul>

  Use insertWidget(), insertSpacing(), insertStretch() or insertLayout()
  to insert a box at a specified position in the layout.

  QBoxLayout also includes two margin widths: <ul>

  <li> setMargin() sets the width of the outer border. This is the width
  of the reserved space along each of the QBoxLayout's four sides.

  <li> setSpacing() sets the inter-box width. This is the width of the
  automatically allocated spacing between neighbouring boxes.  (You
  can use addSpacing() to get more space at a .)

  </ul>

  The outer border width defaults to 0, and the intra-widget width defaults
  to the same as the border width for a top-level layout, or to the
  same as the parent layout otherwise.  Both can be set using
  arguments to the constructor.

  You will almost always want to use the convenience classes for
  QBoxLayout: QVBoxLayout and QHBoxLayout, because of their simpler
  constructors.

  See also the \link layout.html Layout overview documentation \endlink.
*/




/*! \enum QBoxLayout::Direction

  This type is used to determine the direction of
  a box layout. The possible values are:

  <ul>
  <li>\c LeftToRight - Horizontal, from left to right
  <li>\c RightToLeft - Horizontal, from right to left
  <li>\c TopToBottom - Vertical, from top to bottom
  <li>\c Down - An alias for \c TopToBottom
  <li>\c BottomToTop - Vertical, from bottom to top
  <li>\c Up - An alias for \c BottomToTop
  </ul>
 */



static inline bool horz( QBoxLayout::Direction dir )
{
    return dir == QBoxLayout::RightToLeft || dir == QBoxLayout::LeftToRight;
}

/*!
  Constructs a new QBoxLayout with direction \a d and main widget \a
  parent.  \a parent may not be 0.

  \a border is the number of pixels between the edge of the widget and
  the managed children.	 \a space is the default number of pixels
  between neighbouring children.  If \a space is -1 the value
  of \a border is used.

  \a name is the internal object name

  \sa direction()
*/

QBoxLayout::QBoxLayout( QWidget *parent, Direction d,
			int border, int space, const char *name )
    : QLayout( parent, border, space, name )
{
    data = new QBoxLayoutData;
    dir = d;
    setSupportsMargin( TRUE );
}



/*!
  Constructs a new QBoxLayout with direction \a d and inserts it into
  \a parentLayout.

*/

QBoxLayout::QBoxLayout( QLayout *parentLayout, Direction d, int space,
 const char *name )
	: QLayout( parentLayout, space, name )
{
    data = new QBoxLayoutData;
    dir = d;
    setSupportsMargin( TRUE );
}



/*!
  If \a space is -1, this QBoxLayout will inherit its parent's
  spacing(), otherwise \a space is used.

  You have to insert this box into another layout.
*/

QBoxLayout::QBoxLayout( Direction d,
			int space, const char *name )
    : QLayout( space, name )
{
    data = new QBoxLayoutData;
    dir = d;
    setSupportsMargin( TRUE );
}

/*!
  Destructs this box.
*/

QBoxLayout::~QBoxLayout()
{
    delete data;
}

/*!
  Returns the preferred size of this grid.
*/

QSize QBoxLayout::sizeHint() const
{
    if ( data->dirty ) {
	QBoxLayout *that = (QBoxLayout*)this;
	that->setupGeom();
    }
    return data->sizeHint + QSize(2*margin(),2*margin());
}
/*!
  Returns the minimum size needed by this box.
*/

QSize QBoxLayout::minimumSize() const
{
    if ( data->dirty ) {
	QBoxLayout *that = (QBoxLayout*)this;
	that->setupGeom();
    }
    return data->minSize + QSize(2*margin(),2*margin());
}
/*!
  Returns the maximum size needed by this box.
*/

QSize QBoxLayout::maximumSize() const
{
    if ( data->dirty ) {
	QBoxLayout *that = (QBoxLayout*)this;
	that->setupGeom();
    }
    QSize s =  (data->maxSize + QSize(2*margin(),2*margin()))
	       .boundedTo(QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
    if ( alignment() & HorAlign )
	s.setWidth( QWIDGETSIZE_MAX );
    if ( alignment() & VerAlign )
	s.setHeight( QWIDGETSIZE_MAX );

    return s;
}




/*!
  Returns whether this layout's preferred height depends on its width.
*/

bool QBoxLayout::hasHeightForWidth() const
{
    if ( data->dirty ) {
	QBoxLayout *that = (QBoxLayout*)this;
	that->setupGeom();
    }
    return data->hasHfw;
}


/*!
  Returns the layout's preferred height when it is \a w pixels wide.
*/

int QBoxLayout::heightForWidth( int w ) const
{
    w -= 2*margin();
    if ( data->dirty || w != data->hfwWidth ) {
	QBoxLayout *that = (QBoxLayout*)this;
	if ( data->dirty )
	    that->setupGeom();
	return that->calcHfw( w ) + 2*margin();
    }
    return data->hfwHeight + 2*margin();
}

/*!
  Resets cached information.
*/

void QBoxLayout::invalidate()
{
    QLayout::invalidate();
    QLayout::setGeometry( QRect() ); //###binary compatibility
    data->setDirty();
}


/*!
  \reimp
*/

QLayoutIterator QBoxLayout::iterator()
{
    return QLayoutIterator( new QBoxLayoutIterator( data ) );
}


/*!
  Returns the expansiveness of this layout
*/

QSizePolicy::ExpandData QBoxLayout::expanding() const
{
    if ( data->dirty ) {
	QBoxLayout *that = (QBoxLayout*)this;
	that->setupGeom();
    }
    return data->expanding;
}

/*!
  Resizes managed widgets within the rectangle \a r.
 */
void QBoxLayout::setGeometry( const QRect &r )
{
    if ( !data->geomArray || r != geometry() ) {
	QLayout::setGeometry( r );
	if ( !data->geomArray )
	    setupGeom();
	QRect cr = alignment() ? alignmentRect(r) : r;
	QRect s( cr.x()+margin(), cr.y()+margin(),
		 cr.width()-2*margin(), cr.height()-2*margin() );

	QArray<QLayoutStruct> a = *data->geomArray;
	int pos = horz(dir) ? s.x() : s.y();
	int space = horz(dir) ? s.width() : s.height();
	int n = a.count();
	if ( data->hasHfw && !horz(dir) ) {
	    for ( int i = 0; i < n; i++ ) {
		QBoxLayoutItem *box = data->list.at(i);
		if ( box->item->hasHeightForWidth() )
		    a[i].sizeHint = a[i].minimumSize =
				    box->item->heightForWidth( s.width() );
	    }
	}

	qGeomCalc( a, 0, n, pos, space, spacing() );
	for ( int i = 0; i < n; i++ ) {
	    QBoxLayoutItem *box = data->list.at(i);

	    switch ( dir ) {
	    case LeftToRight:
		box->item->setGeometry( QRect( a[i].pos, s.y(),
					       a[i].size, s.height() ));
		break;
	    case RightToLeft:
		box->item->setGeometry( QRect( s.left() + s.right()
					       - a[i].pos - a[i].size, s.y(),
					       a[i].size, s.height() ));
		break;

	    case TopToBottom:
		box->item->setGeometry( QRect( s.x(), a[i].pos,
					       s.width(), a[i].size ));
		break;
	    case BottomToTop:
		box->item->setGeometry( QRect( s.x(), s.top() + s.bottom()
					       - a[i].pos - a[i].size,
					       s.width(), a[i].size ));
		break;
	    }
	}
    }
}

/*!
  Adds \a item to the end of this box layout.
*/

void QBoxLayout::addItem( QLayoutItem *item )
{
    QBoxLayoutItem *it = new QBoxLayoutItem( item );
    data->list.append( it );
    invalidate();
}



/*!
  Inserts \a item in this box layout at index \a index.  If \a index
  is negative, the item is added at the end.

  \warning does not call QLayout::insertChildLayout() if \a item is
  a QLayout.

  \sa addItem(), findWidget()
*/

void QBoxLayout::insertItem( int index, QLayoutItem *item )
{
    if ( index < 0 )				// append
	index = data->list.count();

    QBoxLayoutItem *it = new QBoxLayoutItem( item );
    data->list.insert( index, it );
    invalidate();
}



/*!
  Inserts a non-stretchable space at index \a index with size \a size.
  If \a index is negative, the space is added at the end.

  QBoxLayout gives default border and spacing. This function adds
  additional space.

  \sa insertStretch()
*/
void QBoxLayout::insertSpacing( int index, int size )
{
    if ( index < 0 )				// append
	index = data->list.count();

    //hack in QLayoutArray: spacers do not get insideSpacing

    QLayoutItem *b;
    if ( horz( dir ) )
	b = new QSpacerItem( size, 0, QSizePolicy::Fixed,
			     QSizePolicy::Minimum );
    else
	b = new QSpacerItem( 0, size, QSizePolicy::Minimum,
			     QSizePolicy::Fixed );

    QBoxLayoutItem *it = new QBoxLayoutItem( b );
    it->magic = TRUE;
    data->list.insert( index, it );
    invalidate();
}


/*!
  Inserts a stretchable space at index \a index with zero minimum size
  and stretch factor \a stretch. If \a index is negative, the space
  is added at the end.

  \sa insertSpacing()
*/

void QBoxLayout::insertStretch( int index, int stretch )
{
    if ( index < 0 )				// append
	index = data->list.count();

    //hack in QGridLayout: spacers do not get insideSpacing
    QLayoutItem *b;
    if ( horz( dir ) )
	b = new QSpacerItem( 0, 0, QSizePolicy::Expanding,
			     QSizePolicy::Minimum );
    else
	b = new QSpacerItem( 0, 0,  QSizePolicy::Minimum,
			     QSizePolicy::Expanding );

    QBoxLayoutItem *it = new QBoxLayoutItem( b, stretch );
    it->magic = TRUE;
    data->list.insert( index, it );
    invalidate();
}


/*!
  Inserts \a layout at index \a index, with serial stretch
  factor \a stretch.  If \a index is negative, the layout is added at
  the end.

  \sa setAutoAdd(), insertWidget(), insertSpacing()
*/

void QBoxLayout::insertLayout( int index, QLayout *layout, int stretch )
{
    if ( index < 0 )				// append
	index = data->list.count();

    addChildLayout( layout );
    QBoxLayoutItem *it = new QBoxLayoutItem( layout, stretch );
    data->list.insert( index, it );
    invalidate();
}

/*!
  Inserts \a widget at index \a index, with a \a stretch
  factor and \a alignment.  If \a index is negative, the widget is
  added at the end.

  The stretch factor applies only in the \link direction() direction
  \endlink of the QBoxLayout, and is relative to the other boxes and
  widgets in this QBoxLayout.  Widgets and boxes with higher stretch
  factor grow more.

  If the stretch factor is 0 and nothing else in the QBoxLayout has a
  stretch factor greater than zero, the space is distributed according
  to the QWidget:sizePolicy() of each widget that's involved.

  Alignment is specified by \a alignment which is a bitwise OR of
  Qt::AlignmentFlags values.
  The default alignment is 0, which means
  that the widget fills the entire cell.

  Note: The alignment parameter is interpreted more aggressively
  than in previous versions of Qt.  A non-default alignment now
  indicates that the widget should not grow to fill the available
  space, but should be sized according to sizeHint().

  \sa setAutoAdd(), insertLayout(), insertSpacing()
*/

void QBoxLayout::insertWidget( int index, QWidget *widget, int stretch, int
alignment )
{
    if ( !checkWidget( this, widget ) )
	 return;

    if ( index < 0 )				// append
	index = data->list.count();

    QWidgetItem *b = new QWidgetItem( widget );
    b->setAlignment( alignment );
    QBoxLayoutItem *it = new QBoxLayoutItem( b, stretch );
    data->list.insert( index, it );
    invalidate();
}



/*!
  Adds a non-stretchable space with size \a size to the end of this
  box layout. QBoxLayout gives default border and spacing. This
  function adds additional space.

  \sa insertSpacing(), addStretch()
*/
void QBoxLayout::addSpacing( int size )
{
    insertSpacing( -1, size );
}

/*!
  Adds a stretchable space with zero minimum size and stretch factor
  \a stretch to the end of this box layout.

  \sa addSpacing()
*/
void QBoxLayout::addStretch( int stretch )
{
    insertStretch( -1, stretch );
}

/*!
  Adds \a widget to the end of this box layout, with a stretch factor
  \a stretch and alignment \a alignment.

  The stretch factor applies only in the \link direction() direction
  \endlink of the QBoxLayout, and is relative to the other boxes and
  widgets in this QBoxLayout.  Widgets and boxes with higher stretch
  factor grow more.

  If the stretch factor is 0 and nothing else in the QBoxLayout has a
  stretch factor greater than zero, the space is distributed according
  to the QWidget:sizePolicy() of each widget that's involved.

  Alignment is specified by \a alignment which is a bitwise OR of
  Qt::AlignmentFlags values.
  The default alignment is 0, which means
  that the widget fills the entire cell.

  Note: The alignment parameter is interpreted more aggressively
  than in previous versions of Qt.  A non-default alignment now
  indicates that the widget should not grow to fill the available
  space, but should be sized according to sizeHint().

  \sa insertWidget(), setAutoAdd(), addLayout(), addSpacing()
*/

void QBoxLayout::addWidget( QWidget *widget, int stretch, int
alignment )
{
    insertWidget( -1, widget, stretch, alignment );
}

/*!
  Adds \a layout to the end of the box, with serial stretch factor \a stretch.

  \sa insertLayout(), setAutoAdd(), addWidget(), addSpacing()
*/
void QBoxLayout::addLayout( QLayout *layout, int stretch )
{
    insertLayout( -1, layout, stretch );
}



/*!
  Limits the perpendicular dimension of the box (e.g. height if the
  box is LeftToRight) to a minimum of \a size. Other constraints may
  increase the limit.
*/

void QBoxLayout::addStrut( int size )
{
    QLayoutItem *b;
    if ( horz( dir ) )
	b = new QSpacerItem( 0, size, QSizePolicy::Fixed,
			     QSizePolicy::Minimum );
    else
	b = new QSpacerItem( size, 0, QSizePolicy::Minimum,
			     QSizePolicy::Fixed );

    QBoxLayoutItem *it = new QBoxLayoutItem( b );
    it->magic = TRUE;
    data->list.append( it );
    invalidate();
}


/*!
  Searches for \a w in this layout (not including child layouts).

  Returns the index of \a w, or -1 if \a w is not found.
*/

int QBoxLayout::findWidget( QWidget* w )
{
    const int n = data->list.count();
    for ( int i=0; i < n; i++ ) {
	if ( data->list.at(i)->item->widget() == w )
	    return i;
    }
    return -1;
}



/*!
  Sets the stretch factor for widget \a w to \a stretch and returns
  TRUE, if \a w is found in this layout (not including child layouts).

  Returns FALSE if \a w is not found.
*/

bool QBoxLayout::setStretchFactor( QWidget *w, int stretch )
{
    QListIterator<QBoxLayoutItem> it( data->list );
    QBoxLayoutItem *box;
    while ( (box=it.current()) != 0 ) {
	++it;
	if ( box->item->widget() == w ) {
	    box->stretch = stretch;
	    invalidate();
	    return TRUE;
	}
    }
    return FALSE;
}


/*!
  Sets the stretch factor for the layout \a l to \a stretch and returns
  TRUE, if \a l is found in this layout (not including child layouts).

  Returns FALSE if \a l is not found.
*/

bool QBoxLayout::setStretchFactor( QLayout *l, int stretch )
{
    QListIterator<QBoxLayoutItem> it( data->list );
    QBoxLayoutItem *box;
    while ( (box=it.current()) != 0 ) {
	++it;
	if ( box->item->layout() == l ) {
	    box->stretch = stretch;
	    invalidate();
	    return TRUE;
	}
    }
    return FALSE;
}

/*!
  Sets the direction of this layout to \a direction.


*/

void QBoxLayout::setDirection( Direction direction )
{
    if ( dir == direction )
	return;
    if ( horz(dir) != horz(direction) ) {
	//swap around the spacers (the "magic" bits)
	//#### a bit yucky, knows too much.
	//#### probably best to add access functions to spacerItem
	//#### or even a QSpacerItem::flip()
	QListIterator<QBoxLayoutItem> it( data->list );
	QBoxLayoutItem *box;
	while ( (box=it.current()) != 0 ) {
	    ++it;
	    if ( box->magic ) {
		QSpacerItem *sp = box->item->spacerItem();
		if ( sp ) {
		    if ( sp->expanding() == QSizePolicy::NoDirection ) {
			//spacing or strut
			QSize s = sp->sizeHint();
			sp->changeSize( s.height(), s.width(),
		horz(direction) ? QSizePolicy::Fixed:QSizePolicy::Minimum,
		horz(direction) ? QSizePolicy::Minimum:QSizePolicy::Fixed );

		    } else {
			//stretch
			if ( horz(direction) )
			    sp->changeSize( 0, 0, QSizePolicy::Expanding,
					    QSizePolicy::Minimum );
			else
			    sp->changeSize( 0, 0, QSizePolicy::Minimum,
					    QSizePolicy::Expanding );
		    }
		}

	    }
	}
    }
    dir = direction;
    invalidate();
    if ( mainWidget() ) {
	QEvent *lh = new QEvent( QEvent::LayoutHint );
	QApplication::postEvent( mainWidget(), lh );
    }

}


/*!
  Initializes the data structure needed by qGeomCalc and
  recalculates max/min and size hint.
*/

void QBoxLayout::setupGeom()
{
    if ( !data->dirty )
	return;

    int maxw = horz(dir) ? 0 : QWIDGETSIZE_MAX;
    int maxh = horz(dir) ? QWIDGETSIZE_MAX : 0;
    int minw = 0;
    int minh = 0;
    int hintw = 0;
    int hinth = 0;

    bool horexp = FALSE;
    bool verexp = FALSE;

    data->hasHfw = FALSE;

    delete data->geomArray;
    int n = data->list.count();
    data->geomArray = new QArray<QLayoutStruct>( n );
    QArray<QLayoutStruct> &a = *data->geomArray;

    bool first = TRUE;
    for ( int i = 0; i < n; i++ ) {
	QBoxLayoutItem *box = data->list.at(i);
	QSize max = box->item->maximumSize();
	QSize min = box->item->minimumSize();
	QSize hint = box->item->sizeHint();
	QSizePolicy::ExpandData exp = box->item->expanding();
	bool empty = box->item->isEmpty();
	// space before non-empties, except the first:
	int space = (empty||first) ? 0 : spacing();
	bool ignore =  empty && box->item->widget(); // ignore hidden widgets
	if ( horz( dir ) ) {
	    bool expand = exp & QSizePolicy::Horizontal || box->stretch > 0;
	    horexp = horexp || expand;
	    maxw += max.width() + space;
	    minw += min.width() + space;
	    hintw += hint.width() + space;
	    if ( !ignore )
		qMaxExpCalc( maxh, verexp,
			     max.height(), exp & QSizePolicy::Vertical );
	    minh = QMAX( minh, min.height() );
	    hinth = QMAX( hinth, hint.height() );

	    a[i].sizeHint = hint.width();
	    a[i].maximumSize = max.width();
	    a[i].minimumSize = min.width();
	    a[i].expansive = expand;
	} else {
	    bool expand = exp & QSizePolicy::Vertical || box->stretch > 0;
	    verexp = verexp || expand;
	    maxh += max.height() + space;
	    minh += min.height() + space;
	    hinth += hint.height() + space;
	    if ( !ignore )
		qMaxExpCalc( maxw, horexp,
			     max.width(), exp & QSizePolicy::Horizontal );
	    minw = QMAX( minw, min.width() );
	    hintw = QMAX( hintw, hint.width() );

	    a[i].sizeHint = hint.height();
	    a[i].maximumSize = max.height();
	    a[i].minimumSize = min.height();
	    a[i].expansive = expand;
	}
	a[i].empty = empty;
	a[i].stretch = box->stretch;
	data->hasHfw = data->hasHfw || box->item->hasHeightForWidth();
	first = first && empty;
    }

    data->expanding =  (QSizePolicy::ExpandData)
		       (( horexp ? QSizePolicy::Horizontal : 0 )
			| ( verexp ? QSizePolicy::Vertical : 0 ) );


    data->minSize = QSize(minw,minh);
    data->maxSize = QSize(maxw,maxh);
    data->sizeHint = QSize(hintw,hinth);

    data->maxSize = data->maxSize.expandedTo( data->minSize );
    data->sizeHint = data->sizeHint.expandedTo( data->minSize )
		     .boundedTo( data->maxSize );

    data->dirty = FALSE;
}


/*!
  Calculates and stores the preferred height given the width \a w.
*/

int QBoxLayout::calcHfw( int w )
{
    int h = 0;
    if ( horz(dir) ) {
	QArray<QLayoutStruct> &a = *data->geomArray;
	int n = a.count();
	qGeomCalc( a, 0, n, 0, w, spacing() );
	for ( int i = 0; i < n; i++ ) {
	    QBoxLayoutItem *box = data->list.at(i);
	    h = QMAX( h, box->hfw( a[i].size ) );
	}
    } else {
	QListIterator<QBoxLayoutItem> it( data->list );
	QBoxLayoutItem *box;
	bool first = TRUE;
	while ( (box=it.current()) != 0 ) {
	    ++it;
	    bool empty = box->item->isEmpty();
	    h += box->hfw( w );
	    if ( !first && !empty )
		h += spacing();
	    first = first && empty;
	}
    }
    data->hfwHeight = h;
    data->hfwWidth = w;
    return h;
}


/*!
  \fn QBoxLayout::Direction QBoxLayout::direction() const

  Returns the (serial) direction of the box. addWidget() and addSpacing()
  work in this direction; the stretch stretches in this direction.

  The directions are \c LeftToRight, \c RightToLeft, \c TopToBottom
  and \c BottomToTop. For the last two, the shorter aliases \c Down and
  \c Up are also available.

  \sa addWidget(), addSpacing()
*/





/*!
  \class QHBoxLayout qlayout.h

  \brief The QHBoxLayout class lines up widgets horizontally.

  \ingroup geomanagement

  This class provides an easier way to construct horizontal box layout
  objects.  See \l QBoxLayout for more details.

  The simplest ways to use this class are:

  \code
     QBoxLayout * l = new QHBoxLayout( widget );
     l->setAutoAdd( TRUE );
     new QSomeWidget( widget );
     new QSomeOtherWidget( widget );
     new QAnotherWidget( widget );
  \endcode

  or

  \code
     QBoxLayout * l = new QHBoxLayout( widget );
     l->addWidget( existingChildOfWidget );
     l->addWidget( anotherChildOfWidget );
  \endcode

  \sa QVBoxLayout QGridLayout <a href="layout.html">Layout overview documentation</a>

*/


/*!
  Constructs a new top-level horizontal box.
 */
QHBoxLayout::QHBoxLayout( QWidget *parent, int border,
			  int space, const char *name )
    : QBoxLayout( parent, LeftToRight, border, space, name )
{

}



/*!
  Constructs a new horizontal box and adds it to \a parentLayout.
*/

QHBoxLayout::QHBoxLayout( QLayout *parentLayout, int space,
			  const char *name )
    :QBoxLayout( parentLayout, LeftToRight, space, name )
{

}


/*!
  Constructs a new horizontal box. You have to add it to another
  layout.
 */
QHBoxLayout::QHBoxLayout( int space, const char *name )
    :QBoxLayout( LeftToRight, space, name )
{
}


/*!
  Destructs this box.
*/

QHBoxLayout::~QHBoxLayout()
{
}


/*!
  \class QVBoxLayout qlayout.h

  \brief The QVBoxLayout class lines up widgets vertically.

  \ingroup geomanagement

  This class provides an easier way to construct vertical box layout
  objects.  See \l QBoxLayout for more details.

  The simplest way to use this class is:

  \code
     QBoxLayout * l = new QVBoxLayout( widget );
     l->addWidget( aWidget );
     l->addWidget( anotherWidget );
  \endcode

  \sa QHBoxLayout QGridLayout <a href="layout.html">Layout overview documentation</a>
*/

/*!
  Constructs a new top-level vertical box.
 */
QVBoxLayout::QVBoxLayout( QWidget *parent, int border,
			  int space, const char *name )
    : QBoxLayout( parent, TopToBottom, border, space, name )
{

}


/*!
  Constructs a new vertical box and adds it to \a parentLayout.
*/

QVBoxLayout::QVBoxLayout( QLayout *parentLayout, int space,
			  const char *name )
    :QBoxLayout( parentLayout, TopToBottom, space, name )
{

}

/*!
  Constructs a new vertical box. You have to add it to another
  layout.
 */
QVBoxLayout::QVBoxLayout( int space, const char *name )
    :QBoxLayout( TopToBottom, space, name )
{
}

/*!
  Destructs this box.
*/

QVBoxLayout::~QVBoxLayout()
{
}



#endif //QT_NO_LAYOUT
