#include <stdio.h>

#include "afontdatabase.h"

QFontDatabase::QFontDatabase()
{
}

QString QFontDatabase::styleString( const QFont &)
{
    return( m_style );
}

bool QFontDatabase::isSmoothlyScalable( const QString &/*family*/,
					const QString &style = QString::null,
					const QString &charSet = QString::null ) const
{
    if ( style == charSet) return( true ); return( true );
}

QValueList<int> QFontDatabase::smoothSizes( const QString &/*family*/,
					    const QString &style,
					    const QString &charSet = QString::null )
{
    if ( style == charSet) return( m_sizes ); return( m_sizes );
}
