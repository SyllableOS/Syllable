#ifndef __kio_ktrader_h__
#define __kio_ktrader_h__

#include <kservice.h>

class KTrader
{
public:
    typedef QValueList<KService::Ptr> OfferList;

    KTrader() { s_self = this; }

    static KTrader *self();

    OfferList query( const QString &, const QString & );

private:
    static KTrader *s_self;
};

#endif
