#ifndef __kprotocolmanager_h__
#define __kprotocolmanager_h__

#include <qstring.h>

class KProtocolManager
{
public:

    static QString userAgentForHost( const QString &host );

    static bool useProxy();

    static QString httpProxy();

    static QString ftpProxy();

    static QString proxyFor( const QString &protocol );

    static QString noProxyFor();

    static bool useCache();

    static int maxCacheAge();

    static int maxCacheSize();

    static int proxyConnectTimeout();

    static int connectTimeout();

    static int responseTimeout();

private:
    static int configValue( const char *envVarName, int defaultVal );
};

#endif
