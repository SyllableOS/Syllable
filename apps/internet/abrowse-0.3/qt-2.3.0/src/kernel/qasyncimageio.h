/****************************************************************************
** $Id$
**
** Definition of asynchronous image/movie loading classes
**
** Created : 970617
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

#ifndef QASYNCIMAGEIO_H
#define QASYNCIMAGEIO_H

#ifndef QT_H
#include "qimage.h"
#endif // QT_H

#ifndef QT_NO_ASYNC_IMAGE_IO


class Q_EXPORT QImageConsumer {
public:
    virtual void end()=0;

    // Change transfer type 1.
    virtual void changed( const QRect& ) = 0;
    virtual void frameDone() = 0;

    // Change transfer type 2.
    virtual void frameDone( const QPoint&, const QRect& ) = 0;

    virtual void setLooping( int ) = 0;
    virtual void setFramePeriod( int ) = 0;
    virtual void setSize( int, int ) = 0;
};

class Q_EXPORT QImageFormat {
public:
    virtual ~QImageFormat();
    virtual int decode( QImage& img, QImageConsumer* consumer,
			const uchar* buffer, int length ) = 0;
};

class Q_EXPORT QImageFormatType {
public:
    virtual ~QImageFormatType();
    virtual QImageFormat* decoderFor( const uchar* buffer, int length ) = 0;
    virtual const char* formatName() const = 0;
protected:
    QImageFormatType();
};

struct QImageDecoderPrivate;

class Q_EXPORT QImageDecoder {
public:
    QImageDecoder( QImageConsumer* c );
    ~QImageDecoder();

    const QImage& image() { return img; }
    int decode( const uchar* buffer, int length );

    static const char* formatName( const uchar* buffer, int length );
    static QImageFormatType* format( const char* name ); // direct use - no decode()

    static QStrList inputFormats();
    static void registerDecoderFactory( QImageFormatType* );
    static void unregisterDecoderFactory( QImageFormatType* );

private:
    QImageFormat* actual_decoder;
    QImageConsumer* consumer;
    QImage img;
    QImageDecoderPrivate *d;
};

#endif // QT_NO_ASYNC_IMAGE_IO

#endif // QASYNCIMAGEIO_H
