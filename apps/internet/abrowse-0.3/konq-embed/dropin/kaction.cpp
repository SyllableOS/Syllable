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

#include "kaction.h"

#include <qobjectlist.h>

KAction *KActionCollection::action( const char *name )
{
    QObjectListIt it( *children() );
    for (; it.current(); ++it )
        if ( it.current()->inherits( "QAction" ) &&
             strcmp( it.current()->name(), name ) == 0 )
            return static_cast<KAction *>( it.current() );

    return 0;
}

KAction::KAction( const QString &text, int accel, const QObject *receiver, const char *slot,
                  QObject *parent, const char *name )
    : QAction( text, QIconSet(), text, accel, parent, name )
{
    connect( this, SIGNAL( activated() ),
             receiver, slot );
}

KAction::KAction( const QString &text, const QString &icon, int accel, const QObject *receiver, const char *slot,
                  QObject *parent, const char *name )
    : QAction( text, QIconSet(), text, accel, parent, name )
{
    connect( this, SIGNAL( activated() ),
             receiver, slot );
}

void KAction::setIcon( const QString &name )
{
    setIconSet( m_icons[ name ] );
}

void KAction::assignIconSet( const QString &name, const QIconSet &iconSet )
{
    if ( m_icons.contains( name ) )
        m_icons.replace( name, iconSet );
    else
        m_icons.insert( name, iconSet );
}

KSelectAction::KSelectAction( const QString &text, int accel, const QObject *receiver, const char *slot,
                              QObject *parent, const char *name )
        : QAction( parent, name )
{
    // ###
}

#endif // __ATHEOS__
