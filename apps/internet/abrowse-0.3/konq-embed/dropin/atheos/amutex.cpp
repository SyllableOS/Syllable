#include <assert.h>
#include <unistd.h>

#include <util/locker.h>
#include "amutex.h"

#include <stack>

os::Locker  g_cLock( "khtml_gmutex" );
//static os::Looper* g_pcCurLooper = NULL;
static std::stack<os::Looper*>	g_cLooperStack;


void GlobalMutex::Lock()
{
    g_cLock.Lock();
}

void GlobalMutex::Unlock()
{
    assert( g_cLock.GetOwner() == getpid() );
    g_cLock.Unlock();
}

void GlobalMutex::PushLooper( os::Looper* pcLooper )
{
    g_cLooperStack.push( pcLooper );
//    g_pcCurLooper = pcLooper;
}

void GlobalMutex::PopLooper()
{
    g_cLooperStack.pop();
}

os::Looper* GlobalMutex::GetCurLooper()
{
    return( g_cLooperStack.top() );
}

os::Locker* GlobalMutex::GetMutex()
{
    return( &g_cLock );
}
