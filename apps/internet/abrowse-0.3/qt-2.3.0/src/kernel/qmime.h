/****************************************************************************
** $Id$
**
** Definition of mime classes
**
** Created : 981204
**
** Copyright (C) 1998-2000 Trolltech AS.  All rights reserved.
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

#ifndef QMIME_H
#define QMIME_H

#ifndef QT_H
#include "qwindowdefs.h"
#endif // QT_H

#ifndef QT_NO_MIME

class Q_EXPORT QMimeSource {
public:
    virtual ~QMimeSource();
    virtual const char* format( int n = 0 ) const = 0;
    virtual bool provides( const char* ) const;
    virtual QByteArray encodedData( const char* ) const = 0;
};

class QMimeSourceFactoryData;
class QStringList;

class Q_EXPORT QMimeSourceFactory {
public:
    QMimeSourceFactory();
    virtual ~QMimeSourceFactory();

    static QMimeSourceFactory* defaultFactory();
    static void setDefaultFactory( QMimeSourceFactory* );

    virtual const QMimeSource* data(const QString& abs_name) const;
    virtual QString makeAbsolute(const QString& abs_or_rel_name, const QString& context) const;
    const QMimeSource* data(const QString& abs_or_rel_name, const QString& context) const;

    virtual void setText( const QString& abs_name, const QString& text );
    virtual void setImage( const QString& abs_name, const QImage& im );
    virtual void setPixmap( const QString& abs_name, const QPixmap& pm );
    virtual void setData( const QString& abs_name, QMimeSource* data );
    virtual void setFilePath( const QStringList& );
    virtual QStringList filePath() const;
    void addFilePath( const QString& );
    virtual void setExtensionType( const QString& ext, const char* mimetype );

private:
    QMimeSourceFactoryData* d;
};

#ifdef _WS_WIN_

// This header file is down here for GCC 2.7.* compatibility
#ifndef QT_H
#include "qlist.h"
#endif // QT_H

/*
  Encapsulation of conversion between MIME and Windows CLIPFORMAT.
  Not need on X11, as the underlying protocol uses the MIME standard
  directly.
*/

class Q_EXPORT QWindowsMime {
public:
    QWindowsMime();
    virtual ~QWindowsMime();

    static void initialize();

    static QList<QWindowsMime> all();
    static QWindowsMime* convertor( const char* mime, int cf );
    static const char* cfToMime(int cf);

    static int registerMimeType(const char *mime);

    virtual const char* convertorName()=0;
    virtual int countCf()=0;
    virtual int cf(int index)=0;
    virtual bool canConvert( const char* mime, int cf )=0;
    virtual const char* mimeFor(int cf)=0;
    virtual int cfFor(const char* )=0;
    virtual QByteArray convertToMime( QByteArray data, const char* mime, int cf )=0;
    virtual QByteArray convertFromMime( QByteArray data, const char* mime, int cf )=0;
};

#endif // _WS_WIN_

#endif // QT_NO_MIME

#endif // QMIME_H
