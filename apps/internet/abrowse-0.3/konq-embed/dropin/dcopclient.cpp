/*  This file is part of the KDE project
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

#ifndef __ATHEOS__

#include "dcopclient.h"
#include "dcopobject.h"
#include "kdebug.h"

#include <kio/connection.h>

#include <assert.h>
#include <ctype.h>

static inline bool isIdentChar( char x )
{                                               // Avoid bug in isalnum
    return x == '_' || (x >= '0' && x <= '9') ||
         (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z');
}

QCString normalizeFunctionSignature( const QCString& fun ) {
    if ( fun.isEmpty() )                                // nothing to do
        return fun.copy();
    QCString result( fun.size() );
    char *from  = fun.data();
    char *to    = result.data();
    char *first = to;
    char last = 0;
    while ( true ) {
        while ( *from && isspace(*from) )
            from++;
        if ( last && isIdentChar( last ) && isIdentChar( *from ) )
            *to++ = 0x20;
        while ( *from && !isspace(*from) ) {
            last = *from++;
            *to++ = last;
        }
        if ( !*from )
            break;
    }
    if ( to > first && *(to-1) == 0x20 )
        to--;
    *to = '\0';
    result.resize( (int)((long)to - (long)result.data()) + 1 );
    return result;
}

KIO::Connection *DCOPClient::s_connection = 0;

DCOPClient::DCOPClient()
{
}

DCOPClient::~DCOPClient()
{
}

bool DCOPClient::send( const QCString &remApp, const QCString &remObj, const QCString &remFun,
                       const QByteArray &data )
{
    kdDebug() << "dcopclient::send" << endl;
    QCString replyType;
    QByteArray replyData;

    //   return DCOPObject::self()->process( normalizeFunctionSignature( remFun ), data, replyType, replyData );

    QByteArray realData;
    QDataStream stream( realData, IO_WriteOnly );
    stream << remApp << remObj << normalizeFunctionSignature( remFun ) << data;

    globalConnection()->send( static_cast<int>( DCOPClient::Send ), realData );

    return true;
}

bool DCOPClient::call( const QCString &remApp, const QCString &remObj, const QCString &remFun,
                       const QByteArray &data,
                       QCString &replyType, QByteArray &replyData )
{
    kdDebug() << "dcopclient::call" << endl;

//    return DCOPObject::self()->process( normalizeFunctionSignature( remFun ), data, replyType, replyData );

    QByteArray realData;
    QDataStream stream( realData, IO_WriteOnly );
    stream << remApp << remObj << normalizeFunctionSignature( remFun ) << data;

    globalConnection()->send( static_cast<int>( DCOPClient::Call ), realData );

    QByteArray reply;
    int cmd = 0;
    int res = globalConnection()->read( &cmd, reply );

    assert( res != -1 );

    QDataStream replyStream( reply, IO_ReadOnly );
    replyStream >> replyType >> replyData;

    return true; // ###
}

#endif // __ATHEOS__
