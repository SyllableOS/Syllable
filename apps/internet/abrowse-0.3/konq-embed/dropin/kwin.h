#ifndef __kwin_h__
#define __kwin_h__

#include <qwindowdefs.h>

class NET
{
public:
    enum State { StaysOnTop };
};

class KWin
{
public:
    static void setState( WId, unsigned long ) {}
    static void setCurrentDesktop( int ) {}
    static void setOnDesktop( WId, int ) {}
    static int currentDesktop() { return 1; }
};

#endif
