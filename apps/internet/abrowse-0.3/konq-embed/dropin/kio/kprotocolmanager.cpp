/*  This file is part of the KDE project
    Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 1999 Torben Weis <weis@kde.org>

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

#include "kprotocolmanager.h"

#include <stdlib.h>

// CACHE SETTINGS
#define DEFAULT_MAX_CACHE_SIZE          5120          //  5 MB
#define DEFAULT_MAX_CACHE_AGE           60*60*24*14   // 14 DAYS
#define DEFAULT_EXPIRE_TIME             60*30         // 1/2 hour

// MAXIMUM VALUE ALLOWED WHEN CONFIGURING
// REMOTE AND PROXY SERVERS CONNECTION AND
// RESPONSE TIMEOUTS.
#define MAX_RESPONSE_TIMEOUT            360           //  6 MIN
#define MAX_CONNECT_TIMEOUT             360           //  6 MIN
#define MAX_PROXY_CONNECT_TIMEOUT       120           //  2 MIN

// DEFAULT TIMEOUT VALUE FOR REMOTE AND PROXY CONNECTION
// AND RESPONSE WAIT PERIOD.  NOTE: CHANGING THESE VALUES
// ALSO CHANGES THE DEFAULT ESTABLISHED INITIALLY.
#define DEFAULT_RESPONSE_TIMEOUT         60           //  1 MIN
#define DEFAULT_CONNECT_TIMEOUT          20           // 20 SEC
#define DEFAULT_PROXY_CONNECT_TIMEOUT    10           // 10 SEC

// MINIMUM TIMEOUT VALUE ALLOWED
#define MIN_TIMEOUT_VALUE                 5           //  5 SEC

#define DEFAULT_USERAGENT_STRING    QString("Mozilla/5.0 (compatible; Konqueror/2.0.9; X11)")

QString KProtocolManager::userAgentForHost( const QString &/*host*/ )
{
    QString agent = QString::fromLatin1( getenv( "HTTP_USER_AGENT" ) );

    if ( agent.isEmpty() )
        agent = DEFAULT_USERAGENT_STRING;

    return agent;
}

bool KProtocolManager::useProxy()
{
    return !httpProxy().isEmpty();
}

QString KProtocolManager::httpProxy()
{
    return QString::fromLatin1( getenv( "HTTP_PROXY" ) );
}

QString KProtocolManager::ftpProxy()
{
    // ###
    return QString::null;
}

QString KProtocolManager::noProxyFor()
{
    return QString::fromLatin1( getenv( "NO_PROXY_FOR" ) );
}

QString KProtocolManager::proxyFor( const QString &protocol )
{
    if ( protocol.left( 4 ) == "http" )
        return httpProxy();
    else
        return QString::null;
}

bool KProtocolManager::useCache()
{
    return getenv( "KIO_HTTP_USECACHE" ) != 0;
}

int KProtocolManager::maxCacheAge()
{
    return configValue( "KIO_HTTP_MAXCACHEAGE", DEFAULT_MAX_CACHE_AGE );
}

int KProtocolManager::maxCacheSize()
{
    return configValue( "KIO_HTTP_MAXCACHESIZE", DEFAULT_MAX_CACHE_SIZE );
}

int KProtocolManager::proxyConnectTimeout()
{
    return configValue( "KIO_HTTP_PROXY_CONNECT_TIMEOUT", DEFAULT_PROXY_CONNECT_TIMEOUT );
}

int KProtocolManager::connectTimeout()
{
    return configValue( "KIO_HTTP_CONNECT_TIMEOUT", DEFAULT_CONNECT_TIMEOUT );
}

int KProtocolManager::responseTimeout()
{
    return configValue( "KIO_HTTP_RESPONSE_TIMEOUT", DEFAULT_RESPONSE_TIMEOUT );
}

int KProtocolManager::configValue( const char *envVarName, int defaultVal )
{
    QString valStr = QString::fromLatin1( getenv( envVarName ) );
    bool ok = false;

    int val = valStr.toInt( &ok );

    if ( !valStr.isEmpty() && ok )
        return val;

    return defaultVal;
}
