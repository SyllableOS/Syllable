#ifndef __kprocess_h__
#define __kprocess_h__

// ugly dummy for KSSL and kio/connection

#include <qlist.h>
#include <qsocketnotifier.h>
#include <signal.h>

class KProcess
{
public:
    enum ProcMode { DontCare };

    void start( ProcMode ) {}

};

class KShellProcess : public KProcess
{
public:

    KShellProcess() {}

    KShellProcess &operator<<(const QString & )
        { return *this; }

};

#endif
