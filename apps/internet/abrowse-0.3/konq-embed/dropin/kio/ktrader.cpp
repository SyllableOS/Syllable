/*  This file is part of the KDE project
    Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ktrader.h"

KTrader *KTrader::s_self = 0;

KTrader *KTrader::self()
{
    if ( !s_self )
        s_self = new KTrader;
    return s_self;
}

KTrader::OfferList KTrader::query( const QString &, const QString & )
{
    // ### we only return KHTLPart as offer for "anything" :)

    KTrader::OfferList res;

    QStringList serviceTypes;
    serviceTypes << QString::fromLatin1( "text/html" )
                 << QString::fromLatin1( "text/xml" );

    res.append( KService::Ptr( new KService( "KHTMLPart", "libkhtml", serviceTypes ) ) );

    return res;
}

