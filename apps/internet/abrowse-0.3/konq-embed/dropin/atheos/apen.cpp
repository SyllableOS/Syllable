#include <stdio.h>

#include "apen.h"

QPen::QPen()
{
//    printf( "QPen::QPen:1() not implemented\n" );
}

QPen::QPen( const QColor& color, uint /*width*/=0, PenStyle /*style*/=SolidLine )
{
    m_color = color;
//    printf( "QPen::QPen:2() not implemented\n" );
}

const QColor& QPen::color() const
{
//    printf( "QPen::color() not implemented\n" );
    return m_color;
}
