// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
#ifndef DAMNREDNEX_CPIACAMLOOPER_H
#define DAMNREDNEX_CPIACAMLOOPER_H
//-----------------------------------------------------------------------------

#include <app/looper.h>

//-----------------------------------------------------------------------------

/* Message format
 what = 'set'

 

  
*/

class CPiACamLooper : public Looper
{
public:
    CPiACamLooper( const char *device );
    ~CPiACamLooper();

    
    
private:
};
