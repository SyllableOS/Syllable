#include <stdio.h>

#include "aframe.h"

QFrame::QFrame( QWidget* pcParent, const char* pzName ) : QWidget( pcParent, pzName )
{
}

QFrame::~QFrame()
{
}

void QFrame::setFrameStyle( int )
{
}

int QFrame::frameStyle() const
{
    return( NoFrame );
}

#include "aframe.moc"
