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
#include <kstddirs.h>
#endif
#include <kconfig.h>
#include "kinstance.h"
#include "kaboutdata.h"

KInstance::KInstance( const char *name )
{
#ifndef __ATHEOS__    
    m_dirs = new KStandardDirs();
    m_dirs->addKDEDefaults();
#endif    
    m_config = 0;
    m_name = name;
}

KInstance::KInstance( KAboutData *data )
{
#ifndef __ATHEOS__    
    m_dirs = new KStandardDirs();
    m_dirs->addKDEDefaults();
#endif    
    m_config = 0;
    m_name = data->appName();
}

KConfig *KInstance::config() const
{
    if ( !m_config )
        m_config = new KConfig( QString::fromLatin1( m_name ) + "rc" );
    return m_config;
}
