

#include "qrect.h"
#include <qpointarray.h>



class QRegion
{
public:
    enum RegionType { Rectangle, Ellipse   };

    QRegion();
    QRegion( int x, int y, int w, int h, RegionType = Rectangle );
//    QRegion( const QRect &, RegionType = Rectangle );
    QRegion( const QPointArray &, bool winding=FALSE );
//    QRegion( const QRegion & );
//    QRegion( const QBitmap & );
   ~QRegion();
    QRegion &operator=( const QRegion & );

    bool    contains( const QPoint &p ) const;
private:
    void* d;
};
