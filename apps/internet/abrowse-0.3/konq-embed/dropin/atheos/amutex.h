#ifndef __F_ATHEOS_AMUTEX_H__
#define __F_ATHEOS_AMUTEX_H__

namespace os
{
    class Looper;
    class Locker;
}

class GlobalMutex
{
public:
    static void Lock();
    static void Unlock();
    static os::Locker* GetMutex();
    
    static void PushLooper( os::Looper* pcLooper );
    static void PopLooper();
    static os::Looper* GetCurLooper();    
};




#endif // __F_ATHEOS_AMUTEX_H__
