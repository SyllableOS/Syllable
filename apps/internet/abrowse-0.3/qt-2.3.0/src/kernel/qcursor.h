/****************************************************************************
** $Id$
**
** Definition of QCursor class
**
** Created : 940219
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

#ifndef QCURSOR_H
#define QCURSOR_H

#ifdef __ATHEOS__
#include "../../../konq-embed/dropin/atheos/acursor.h"
#else

#ifndef QT_H
#include "qpoint.h"
#include "qshared.h"
#include "qnamespace.h"
#endif // QT_H

/*
  ### The fake cursor has to go first with old qdoc.
*/
#ifdef QT_NO_CURSOR

class Q_EXPORT QCursor
{
public:
    static QPoint pos();
    static void	  setPos( int x, int y );
    static void	  setPos( const QPoint & );
private:
    QCursor();
};

#endif // QT_NO_CURSOR

#ifndef QT_NO_CURSOR

struct QCursorData;


class Q_EXPORT QCursor
{
public:
    QCursor();					// create default arrow cursor
    QCursor( int shape );
    QCursor( const QBitmap &bitmap, const QBitmap &mask,
	     int hotX=-1, int hotY=-1 );
    QCursor( const QPixmap &pixmap,
	     int hotX=-1, int hotY=-1 );
    QCursor( const QCursor & );
   ~QCursor();
    QCursor &operator=( const QCursor & );

    int		  shape()   const;
    void	  setShape( int );

    const QBitmap *bitmap() const;
    const QBitmap *mask()   const;
    QPoint	  hotSpot() const;

#if defined(_WS_WIN_)
    HCURSOR	  handle()  const;
#elif defined(_WS_X11_)
    HANDLE	  handle()  const;
#elif defined(_WS_MAC_)
    void * handle() const;
#elif defined(_WS_QWS_)
    HANDLE	  handle()  const;
#endif

    static QPoint pos();
    static void	  setPos( int x, int y );
    static void	  setPos( const QPoint & );

    static void	  initialize();
    static void	  cleanup();

private:
    void	  setBitmap( const QBitmap &bitmap, const QBitmap &mask,
				 int hotX, int hotY );
    void	  update() const;
    QCursorData	 *data;
    QCursor	 *find_cur(int);
};



/*****************************************************************************
  Cursor shape identifiers (correspond to global cursor objects)
 *****************************************************************************/

// ############ Should be moved to QNamespace in 3.0!!!!!!
enum QCursorShape {
    ArrowCursor, UpArrowCursor, CrossCursor, WaitCursor, IbeamCursor,
    SizeVerCursor, SizeHorCursor, SizeBDiagCursor, SizeFDiagCursor,
    SizeAllCursor, BlankCursor, SplitVCursor, SplitHCursor, PointingHandCursor,
    ForbiddenCursor, LastCursor = ForbiddenCursor, BitmapCursor=24 };


/*****************************************************************************
  QCursor stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_EXPORT QDataStream &operator<<( QDataStream &, const QCursor & );
Q_EXPORT QDataStream &operator>>( QDataStream &, QCursor & );
#endif
#endif // QT_NO_CURSOR


inline void QCursor::setPos( const QPoint &p )
{
    setPos( p.x(), p.y() );
}

#endif // __ATHEOS__

#endif // QCURSOR_H
