#ifndef __kglobalsettings_h__
#define __kglobalsettings_h__

#include <qfont.h>

#define KDE_DEFAULT_CHANGECURSOR true

class KGlobalSettings
{
public:

    static QFont generalFont();
    static QFont fixedFont();

    static int dndEventDelay() { return 5; } // ###

private:
    static QFont *s_generalFont;
    static QFont *s_fixedFont;
};

#endif
