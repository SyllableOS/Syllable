/****************************************************************************
** $Id$
**
** Definition of the QFontDatabase class
**
** Created : 981126
**
** Copyright (C) 1999-2000 Trolltech AS.  All rights reserved.
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

#ifndef QFONTDATABASE_H
#define QFONTDATABASE_H


#ifdef __ATHEOS__
#include "../../konq-embed/dropin/atheos/afontdatabase.h"
#else

#ifndef QT_H
#include "qwindowdefs.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qfont.h"
#include "qlist.h"
#include "qvaluelist.h"
#endif // QT_H

#ifndef QT_NO_FONTDATABASE

class QStringList;
class QFontStylePrivate;
class QtFontStyle;
class QtFontCharSet;
class QtFontFamily;
class QtFontFoundry;

class QFontDatabasePrivate;

class Q_EXPORT QFontDatabase
{
public:
    QFontDatabase();

    QStringList families( bool onlyForLocale = TRUE ) const;
    QValueList<int> pointSizes( const QString &family,
				const QString &style = QString::null,
				const QString &charSet = QString::null );
    QStringList styles( const QString &family,
			const QString &charSet = QString::null ) const;
    QStringList charSets( const QString &familyName,
			  bool onlyForLocale = TRUE ) const;

    QFont font( const QString familyName, const QString &style,
		int pointSize, const QString charSetName = QString::null );

    bool  isBitmapScalable( const QString &family,
			    const QString &style   = QString::null,
			    const QString &charSet = QString::null ) const;
    bool  isSmoothlyScalable( const QString &family,
			      const QString &style   = QString::null,
			      const QString &charSet = QString::null ) const;
    bool  isScalable( const QString &family,
		      const QString &style   = QString::null,
		      const QString &charSet = QString::null ) const;
    bool  isFixedPitch( const QString &family,
			const QString &style   = QString::null,
			const QString &charset = QString::null) const;

    QValueList<int> smoothSizes( const QString &family,
				 const QString &style,
				 const QString &charSet = QString::null );

    static QValueList<int> standardSizes();

    bool italic( const QString &family,
		 const QString &style,
		 const QString &charSet = QString::null ) const;

    bool bold( const QString &family,
	       const QString &style,
	       const QString &charSet = QString::null ) const;

    int weight( const QString &family,
		const QString &style,
		const QString &charSet = QString::null ) const;


#if 0
    QValueList<QFont::CharSet> charSets( const QString &familyName ) const;
    bool  supportsCharSet( const QString &familyName,
			   const QString &charSet ) const;
    bool  supportsCharSet( const QString &familyName,
			   QFont::CharSet charSet ) const;
#endif

    QString styleString( const QFont &);

    static QString verboseCharSetName( const QString & charSetName );
    static QString charSetSample( const QString & charSetName );

private:
    static void createDatabase();

    friend class QtFontStyle;
    friend class QtFontCharSet;
    friend class QtFontFamily;
    friend class QtFontFoundry;
    friend class QFontDatabasePrivate;

    QFontDatabasePrivate *d;

};

#endif // QT_NO_FONTDATABASE

#endif // __ATHEOS__

#endif // QFONTDATABASE_H
