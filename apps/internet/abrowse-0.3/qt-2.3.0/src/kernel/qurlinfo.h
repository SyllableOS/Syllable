/****************************************************************************
** $Id$
**
** Definition of QUrlInfo class
**
** Created : 950429
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

#ifndef QURLINFO_H
#define QURLINFO_H

#ifndef QT_H
#include "qdatetime.h"
#include "qstring.h"
#endif // QT_H

class QUrlOperator;
struct QUrlInfoPrivate;
class QUrl;

class Q_EXPORT QUrlInfo
{
public:
    QUrlInfo();
    QUrlInfo( const QUrlOperator &path, const QString &file );
    QUrlInfo( const QUrlInfo &ui );
    QUrlInfo( const QString &name, int permissions, const QString &owner,
	      const QString &group, uint size, const QDateTime &lastModified,
	      const QDateTime &lastRead, bool isDir, bool isFile, bool isSymLink,
	      bool isWritable, bool isReadable, bool isExecutable );
    QUrlInfo( const QUrl &url, int permissions, const QString &owner,
	      const QString &group, uint size, const QDateTime &lastModified,
	      const QDateTime &lastRead, bool isDir, bool isFile, bool isSymLink,
	      bool isWritable, bool isReadable, bool isExecutable );
    QUrlInfo &operator=( const QUrlInfo &ui );
    virtual ~QUrlInfo();

    virtual void setName( const QString &name );
    virtual void setDir( bool b );
    virtual void setFile( bool b );
    virtual void setSymLink( bool b );
    virtual void setOwner( const QString &s );
    virtual void setGroup( const QString &s );
    virtual void setSize( uint s );
    virtual void setWritable( bool b );
    virtual void setReadable( bool b );
    virtual void setPermissions( int p );
    virtual void setLastModified( const QDateTime &dt );

    QString name() const;
    int permissions() const;
    QString owner() const;
    QString group() const;
    uint size() const;
    QDateTime lastModified() const;
    QDateTime lastRead() const;
    bool isDir() const;
    bool isFile() const;
    bool isSymLink() const;
    bool isWritable() const;
    bool isReadable() const;
    bool isExecutable() const;

    static bool greaterThan( const QUrlInfo &i1, const QUrlInfo &i2,
			     int sortBy );
    static bool lessThan( const QUrlInfo &i1, const QUrlInfo &i2,
			  int sortBy );
    static bool equal( const QUrlInfo &i1, const QUrlInfo &i2,
		       int sortBy );

    bool operator==( const QUrlInfo &i ) const;
private:
    QUrlInfoPrivate *d;

};

#endif
