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

#include "krun.h"

KRun::KRun( const KURL &url, int, bool, bool )
{
    m_strURL = url;
//    QTimer::singleShot( 0, this, SLOT( slotStart() ) );
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( slotStart() ) );
    m_timer.start( 0, true );
}

void KRun::foundMimeType( const QString & )
{
}

void KRun::slotStart()
{
    // ###
    foundMimeType( QString::fromLatin1( "text/html" ) );

    // check this again
    delete this;
}

#include "krun.moc"
