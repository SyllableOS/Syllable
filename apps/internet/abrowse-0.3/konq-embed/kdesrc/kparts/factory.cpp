/* This file is part of the KDE project
   Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
             (C) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/


#include "factory.h"
#include "part.h"

#include <qwidget.h>

#include <klocale.h>
#include <kglobal.h>
#include <kinstance.h>
#include <assert.h>

using namespace KParts;

Factory::Factory( QObject *parent, const char *name )
: KLibFactory( parent, name )
{
}

Factory::~Factory()
{
}

Part *Factory::createPart( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *classname, const QStringList &args )
{
    Part* part = createPartObject( parentWidget, widgetName, parent, name, classname, args );
    if ( part )
	emit objectCreated( part );
    return part;
}

Part *Factory::createPartObject( QWidget *, const char *, QObject *, const char *, const char *, const QStringList & )
{
    return 0;
}

QObject *Factory::createObject( QObject *parent, const char *name, const char *classname, const QStringList &args )
{
#ifdef __ATHEOS__    
  assert( dynamic_cast<QWidget *>( parent ) == NULL );
#else
  assert( parent->isWidgetType() );
#endif
  return createPart( static_cast<QWidget *>( parent ), name, parent, name, classname, args );
}

#include "factory.moc"
