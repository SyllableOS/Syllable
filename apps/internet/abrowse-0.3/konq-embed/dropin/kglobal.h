#ifndef __kglobal_h__
#define __kglobal_h__

#include <qstring.h>

#include <kinstance.h>
#include <kapp.h>

// ### compat
#include <sys/stat.h>

class KStringDict;
class KLocale;
class KStandardDirs;
class KCharsets;
class KConfig;

class KGlobal
{
public:

    static const QString &staticQString( const char * );
    static const QString &staticQString( const QString & );

    static KLocale *locale();
#ifndef __ATHEOS__
    static KStandardDirs *dirs() { return instance()->dirs(); }
#endif
    static KConfig *config() { return instance()->config(); }

    static KCharsets *charsets();
    static KInstance *instance() { return kapp; }

    static KInstance *_activeInstance;
private:
    static KStringDict *s_stringDict;
    static KLocale *s_locale;
    static KCharsets *s_charsets;
};

#endif
