/****************************************************************************
** $Id$
**
** Definition of QClipboard class
**
** Created : 960430
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

#ifndef QCLIPBOARD_H
#define QCLIPBOARD_H

#ifndef QT_H
#include "qwindowdefs.h"
#include "qobject.h"
#endif // QT_H

#ifndef QT_NO_CLIPBOARD

class QMimeSource;

class Q_EXPORT QClipboard : public QObject
{
    Q_OBJECT
private:
    QClipboard( QObject *parent=0, const char *name=0 );
    ~QClipboard();

public:
    void	clear();

#if defined(_WS_X11_)
    bool ownsSelection() const;
#endif
    
#ifndef QT_NO_MIMECLIPBOARD
    QMimeSource* data() const;
    void  setData( QMimeSource* );
#endif
    QString     text()	 const;
    QString     text(QCString& subtype) const;
    void	setText( const QString &);
#ifndef QT_NO_MIMECLIPBOARD
    QImage	image() const;
    QPixmap	pixmap() const;
    void	setImage( const QImage & );
    void	setPixmap( const QPixmap & );
#endif
signals:
    void	dataChanged();

private slots:
    void	ownerDestroyed();

protected:
    void	connectNotify( const char * );
    bool	event( QEvent * );

    friend class QApplication;
    friend class QBaseApplication;
    friend class QDragManager;
    friend class QMimeSource;

private:	// Disabled copy constructor and operator=
#if defined(_WS_X11_)
    void clobber();
#endif

#if defined(Q_DISABLE_COPY)
    QClipboard( const QClipboard & );
    QClipboard &operator=( const QClipboard & );
#endif
};

#endif // QT_NO_CLIPBOARD

#endif // QCLIPBOARD_H
