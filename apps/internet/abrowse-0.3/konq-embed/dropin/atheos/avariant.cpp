
#include <qvariant.h>

class QVariant::Private
{
public:
    QVariant::Type m_eType;
    bool	   m_bBool;
};

QVariant::QVariant()
{
    d = new Private;
    d->m_eType = Invalid;
}

QVariant::QVariant( const QString& )
{
    d = new Private;
    d->m_eType = String;
}

QVariant::QVariant( bool bValue, int )
{
    d = new Private;
    d->m_eType = Bool;
    d->m_bBool = bValue;
}

QVariant::QVariant( double )
{
    d = new Private;
    d->m_eType = QVariant::Double;
}

QVariant::Type QVariant::type() const
{
    return( d->m_eType );
}

bool QVariant::toBool() const
{
    return( d->m_eType );
}
