/****************************************************************************
** $Id$
**
** Definition of QIconSet class
**
** Created : 980318
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

#ifndef QICONSET_H
#define QICONSET_H

#ifndef QT_H
#include "qpixmap.h"
#endif // QT_H

#ifndef QT_NO_ICONSET
struct QIconSetPrivate;


class Q_EXPORT QIconSet
{
public:
    enum Size { Automatic, Small, Large };

    enum Mode { Normal, Disabled, Active };

    QIconSet();
    QIconSet( const QPixmap &, Size = Automatic );
    QIconSet( const QPixmap &smallPix, const QPixmap &largePix );
    QIconSet( const QIconSet & );
    virtual ~QIconSet();

    void reset( const QPixmap &, Size );

    virtual void setPixmap( const QPixmap &, Size, Mode = Normal );
    virtual void setPixmap( const QString &, Size, Mode = Normal );
    QPixmap pixmap( Size, Mode ) const;
    QPixmap pixmap( Size s, bool enabled ) const;
    QPixmap pixmap() const;
    bool isGenerated( Size, Mode ) const;

    bool isNull() const;

    void detach();

    QIconSet &operator=( const QIconSet & );

private:
    QIconSetPrivate * d;
};

#endif // QT_NO_ICONSET
#endif
