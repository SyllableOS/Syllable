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

#include "klibloader.h"

#include <assert.h>

KLibrary::KLibrary( const QCString &name )
{
    m_name = name;
    m_factory = 0;
}

KLibrary::~KLibrary()
{
    // noone should ever destruct a KLibrary! (khtml_part.cpp tries to, but
    // it mustn't
    assert( false );
}

void KLibrary::registerSymbol( const char *key, void *symbol )
{
    if ( m_symbols.find( key ) == 0 )
        m_symbols.replace( key, symbol );

    m_symbols.insert( key, symbol );
}

void *KLibrary::symbol( const char *name )
{
    void *res = m_symbols[ name ];
    assert( res );
    return res;
}

KLibFactory *KLibrary::factory()
{
    if ( m_factory )
        return m_factory;

    void *sym = symbol( "init_" + m_name );

    if ( !sym )
        return 0;

    typedef KLibFactory * (*initFunc)();
    initFunc func = (initFunc)sym;

    m_factory = (*func)();

    return m_factory;
}

KLibrary *KLibLoader::library( const char *name )
{
    if ( m_libs.find( name ) == 0 )
        m_libs.insert( name, new KLibrary( name ) );

    return m_libs[ name ];
}

KLibLoader *KLibLoader::s_self = 0;

KLibLoader *KLibLoader::self()
{
    if ( !s_self )
        s_self = new KLibLoader;
    return s_self;
}

#include "klibloader.moc"
