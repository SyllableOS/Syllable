#ifndef __kiconeffect_h__
#define __kiconeffect_h__

#include <qrect.h>

class QWidget;

class KIcon
{
public:
    enum Sz { SizeMedium };
    enum Tp { Desktop, FileSystem };
    enum St { DisabledState };
};

class KIconEffect
{
public:
    static void visualActivate( QWidget *, const QRect & ) {}
};

#endif
