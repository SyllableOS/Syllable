
#include <stdio.h>
#include "aregion.h"

QRegion::QRegion()
{
}

QRegion::QRegion( int x, int y, int w, int h, RegionType = Rectangle )
{
}

QRegion::QRegion( const QPointArray &, bool winding=FALSE )
{
}

QRegion::~QRegion()
{
}

QRegion& QRegion::operator=( const QRegion & )
{
    return( *this );
}

bool QRegion::contains( const QPoint &p ) const
{
    printf( "Warning: QRegion::contains() not implemented\n" );
    return( false );
}
