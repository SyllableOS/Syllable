#ifndef __kurldrag_h__
#define __kurldrag_h__

#include <kurl.h>

class QWidget;
class QUriDrag;

class KURLDrag
{
public:

    static QUriDrag *newDrag( const KURL::List &, QWidget *, const char * = 0 )
        { return 0; }

};

#endif
