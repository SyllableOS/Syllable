/*  This file is part of the KDE project
    Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 2000 David Faure <faure@kde.org>

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

#include "part.h"

#include <kdebug.h>


using namespace KParts;

Part::Part( QObject *parent, const char *name )
    : QObject( parent, name )
{
    m_manager = 0;
    m_selectable = false;
    m_widget = 0;
    m_instance = 0;
}

Part::~Part()
{
}

Part *Part::hitTest( QWidget *widget, const QPoint & )
{
    if ( widget != m_widget )
        return 0;

    return this;
}

QWidget *Part::widget()
{
    return m_widget;
}

void Part::setWidget( QWidget *widget )
{
    ASSERT( !m_widget );
    m_widget = widget;

    connect( m_widget, SIGNAL( destroyed() ),
             this, SLOT( slotWidgetDestroyed() ) );
}

void Part::slotWidgetDestroyed()
{
    kdDebug() << "Part::slotWidgetDestroyed()" << endl;
    m_widget = 0;
    delete this;
}

#ifndef __ATHEOS__
bool Part::event( QEvent *event )
{
  if ( QObject::event( event ) )
    return true;

  if ( PartActivateEvent::test( event ) )
  {
    partActivateEvent( (PartActivateEvent *)event );
    return true;
  }

  if ( PartSelectEvent::test( event ) )
  {
    partSelectEvent( (PartSelectEvent *)event );
    return true;
  }

  if ( GUIActivateEvent::test( event ) )
  {
    guiActivateEvent( (GUIActivateEvent *)event );
    return true;
  }

  return false;
}

void Part::partActivateEvent( PartActivateEvent * )
{
}

void Part::partSelectEvent( PartSelectEvent * )
{
}

void Part::guiActivateEvent( GUIActivateEvent * )
{
}
#endif // __ATHEOS__


ReadOnlyPart::ReadOnlyPart( QObject *parent, const char *name )
    : Part( parent, name )
{
}

ReadOnlyPart::~ReadOnlyPart()
{
}



#ifndef __ATHEOS__
void ReadOnlyPart::guiActivateEvent( GUIActivateEvent *event )
{
  if (event->activated())
  {
    if (!m_url.isEmpty())
    {
      emit setWindowCaption( m_url.prettyURL() );
    } else emit setWindowCaption( "" );
  }
}
#endif // __ATHEOS__

#include "part.moc"
