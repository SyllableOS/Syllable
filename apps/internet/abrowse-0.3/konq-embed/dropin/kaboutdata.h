#ifndef __kaboutdata_h__
#define __kaboutdata_h__

#include <qcstring.h>

// ###

class KAboutData
{
public:
    enum { License_LGPL };

    KAboutData( const char *appname, const char */*progname*/, const char */*version*/,
                const char */*description*/, int /*license*/ )
        { m_appname = appname; }

    void addAuthor( const char */*name*/,
                    const char */*task*/,
                    const char */*email*/ )
        {}

    QCString appName() const { return m_appname; }

private:
    QCString m_appname;
};

#endif
