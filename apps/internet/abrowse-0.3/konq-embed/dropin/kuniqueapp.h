#ifndef __kuniqueapp_h__
#define __kuniqueapp_h__

// hacky-wacky dummy-app object, for cookie-jar server

#ifdef __ATHEOS__
#include <stdio.h>
#endif

#include <dcopobject.h>

// for compatibility
#include <kapp.h>

class DCOPClient;

class KUniqueApplication : public DCOPObject
{
    Q_OBJECT
public:
    KUniqueApplication() {}
    ~KUniqueApplication() {}

#ifdef __ATHEOS__
    void quit() { printf( "Warning: KUniqueApplication::quit() not implemented\n" ); }
    DCOPClient *dcopClient() const { printf( "Warning: KUniqueApplication::dcopClient() not implemented\n" ); return 0; }
#else
    void quit() { kapp->quit(); }
    DCOPClient *dcopClient() const { return kapp->dcopClient(); }
#endif
};
#endif
