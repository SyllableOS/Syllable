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
#ifndef __ATHEOS__
#include "job.h"

#include <slaveinterface.h>

using namespace KIO;

#define KIO_ARGS QByteArray packedArgs; QDataStream stream( packedArgs, IO_WriteOnly ); stream

TransferJob *KIO::get( const KURL &url, bool reload, bool showProgressInfo )
{
    KIO_ARGS << url;

    TransferJob *job = new TransferJob( url, CMD_GET, packedArgs, QByteArray(), showProgressInfo );

    if ( reload )
        job->addMetaData( "cache", "reload" );

    return job;
}

TransferJob *KIO::http_post( const KURL &url, const QByteArray &data, bool showProgressInfo )
{
    KIO_ARGS << (int)1 << url;

    TransferJob *job = new TransferJob( url, CMD_SPECIAL, packedArgs, data, showProgressInfo );

    return job;
}

SimpleJob *KIO::http_update_cache( const KURL &url, bool no_cache, time_t expireDate )
{

    KIO_ARGS << (int)2 << url << no_cache << expireDate;

    SimpleJob *job = new SimpleJob( url, CMD_SPECIAL, packedArgs, false );

    return job;
}
#endif // __ATHEOS__
