/****************************************************************************
** $Id$
**
** Definition of QHeader widget class (table header)
**
** Created : 961105
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

#ifndef QHEADER_H
#define QHEADER_H

#ifndef QT_H
#include "qwidget.h"
#include "qstring.h"
#include "qiconset.h"
#endif // QT_H

#ifndef QT_NO_HEADER

struct QHeaderData;
class QShowEvent;

class Q_EXPORT QHeader : public QWidget
{
    Q_OBJECT
    Q_PROPERTY( Orientation orientation READ orientation WRITE setOrientation )
    Q_PROPERTY( bool tracking READ tracking WRITE setTracking )
    Q_PROPERTY( int count READ count )
    Q_PROPERTY( int offset READ offset WRITE setOffset )
    Q_PROPERTY( bool moving READ isMovingEnabled WRITE setMovingEnabled )

public:
    QHeader( QWidget *parent=0, const char *name=0 );
    QHeader( int, QWidget *parent=0, const char *name=0 );
    ~QHeader();

    int		addLabel( const QString &, int size = -1 );
    int		addLabel( const QIconSet&, const QString &, int size = -1 );
    void 	removeLabel( int section );
    virtual void setLabel( int, const QString &, int size = -1 );
    virtual void setLabel( int, const QIconSet&, const QString &, int size = -1 );
    QString 	label( int section ) const;
    QIconSet* 	iconSet( int section ) const;

    virtual void setOrientation( Orientation );
    Orientation orientation() const;
    virtual void setTracking( bool enable );
    bool	tracking() const;

    virtual void setClickEnabled( bool, int section = -1 );
    virtual void setResizeEnabled( bool, int section = -1 );
    virtual void setMovingEnabled( bool );
    bool isClickEnabled( int section = -1 ) const;
    bool isResizeEnabled( int section = -1 ) const;
    bool isMovingEnabled() const;

    void 	resizeSection( int section, int s );
    int		sectionSize( int section ) const;
    int		sectionPos( int section ) const;
    int		sectionAt( int pos ) const;
    int		count() const;

    virtual void setCellSize( int , int ); // obsolete, do not use
    int		cellSize( int ) const; // obsolete, do not use
    int		cellPos( int ) const; // obsolete, do not use
    int		cellAt( int ) const; // obsolete, do not use

    int 	offset() const;

    QSize	sizeHint() const;
    QSizePolicy sizePolicy() const;

    int		mapToSection( int index ) const;
    int		mapToIndex( int section ) const;
    int		mapToLogical( int ) const; // obsolete, do not use
    int		mapToActual( int ) const; // obsolete, do not use

    void 	moveSection( int section, int toIndex );
    virtual void moveCell( int, int); // obsolete, do not use

    void 	setSortIndicator( int section, bool increasing = TRUE );

public slots:
    void setUpdatesEnabled( bool enable );
    virtual void setOffset( int pos );

signals:
    void	clicked( int section );
    void	pressed( int section );
    void	released( int section );
    void	sizeChange( int section, int oldSize, int newSize );
    void	indexChange( int section, int fromIndex, int toIndex );
    void	sectionClicked( int ); // obsolete, do not use
    void	moved( int, int ); // obsolete, do not use

protected:
    void	paintEvent( QPaintEvent * );
    void	showEvent( QShowEvent *e );
    QRect	sRect( int index );

    void	paintSection( QPainter *p, int index, QRect fr); // ### const QRect& ## virtual 3.0
    void	paintSectionLabel( QPainter* p, int index, const QRect& fr ); //## virtual 3.0

    void	mousePressEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );

private:
    void	init( int );

    void	paintRect( int p, int s );
    void	markLine( int idx );
    void	unMarkLine( int idx );
    int		pPos( int i ) const;
    int		pSize( int i ) const;
    int		pHeight( int i ) const;
    void	setPHeight( int i, int h );
    int 	findLine( int );

    void	handleColumnResize(int, int, bool);

    int		offs;
    int		handleIdx;
    int		oldHIdxSize;
    int		moveToIdx;
    enum State { Idle, Sliding, Pressed, Moving, Blocked };
    State	state;
    QCOORD	clickPos;
    bool	trackingIsOn;
    int       cachedIdx; // not used
    int	cachedPos; // not used
    Orientation orient;

    QHeaderData *d;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QHeader( const QHeader & );
    QHeader &operator=( const QHeader & );
#endif
};


inline QHeader::Orientation QHeader::orientation() const
{
    return orient;
}

inline void QHeader::setTracking( bool enable ) { trackingIsOn = enable; }
inline bool QHeader::tracking() const { return trackingIsOn; }

#endif // QT_NO_HEADER

#endif // QHEADER_H
