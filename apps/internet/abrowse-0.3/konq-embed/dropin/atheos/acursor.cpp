#include <stdio.h>

#include "acursor.h"

#include <util/application.h>

QCursor::QCursor()
{
//    printf( "Warning: QCursor::QCursor:1() not implemented\n" );
}

QCursor::QCursor( QCursorShape )
{
//    printf( "Warning: QCursor::QCursor:2() not implemented\n" );
}

QPoint QCursor::pos()
{
    printf( "Warning: QCursor::pos() not implemented\n" );
    return( QPoint( 0, 0 ) );
}
