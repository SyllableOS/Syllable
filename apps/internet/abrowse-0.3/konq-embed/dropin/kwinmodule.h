#ifndef __kwinmodule_h__
#define __kwinmodule_h__

#include <qapplication.h>

class KWinModule
{
public:
    KWinModule() {}

    QRect workArea()
        { return QApplication::desktop()->rect(); }
};

#endif
