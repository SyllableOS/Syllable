/*  This file is part of the KDE project
    Copyright (C) 1999 Sirtaj Singh Kanq <taj@kde.org>
    Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
#include "kglobal.h"
#include "klocale.h"
#ifndef __ATHEOS__
#include "kstddirs.h"
#endif
#include "kcharsets.h"
#include "kinstance.h"

#include <qdict.h>

class KStringDict : public QDict<QString>
{
public:
    KStringDict() : QDict<QString>() {};
};

KStringDict *KGlobal::s_stringDict = 0;

// we may leak :)
const QString &KGlobal::staticQString( const char *str )
{
    return staticQString( QString::fromLatin1( str ) );
}

// we may leak :)
const QString &KGlobal::staticQString( const QString &str )
{
    if ( !s_stringDict )
    {
        s_stringDict = new KStringDict;
        s_stringDict->setAutoDelete( true );
    }

    QString *result = s_stringDict->find( str );
    if ( !result )
    {
        result = new QString( str );
        s_stringDict->insert( str, result );
    }

    return *result;
}

KLocale *KGlobal::s_locale = 0;

KLocale *KGlobal::locale()
{
    if ( !s_locale )
        s_locale = new KLocale;
    return s_locale;
}

KCharsets *KGlobal::s_charsets = 0;

KCharsets *KGlobal::charsets()
{
    if ( !s_charsets )
        s_charsets = new KCharsets;
    return s_charsets;
}

KInstance *KGlobal::_activeInstance = 0;
