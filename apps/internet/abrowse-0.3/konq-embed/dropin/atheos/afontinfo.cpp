#include <stdio.h>

#include "afontinfo.h"
#include <qfont.h>


class QFontInfoInternal
{
public:
    QFont m_cFont;
};

QFontInfo::QFontInfo( const QFont& cFont )
{
    d = new QFontInfoInternal;
    d->m_cFont = cFont;
}

QFontInfo::~QFontInfo()
{
    delete d;
}

int QFontInfo::pointSize() const
{
    return( d->m_cFont.pointSize() );
}

bool QFontInfo::fixedPitch() const
{
    return( false );
}
