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
#include <kparts/event.h>

using namespace KParts;

//the answer!
#define KPARTS_EVENT_MAGIC 42

Event::Event( const char *eventName )
 : QCustomEvent( (QEvent::Type)(QEvent::User + KPARTS_EVENT_MAGIC), (void *)eventName )
{
}

const char *Event::eventName() const
{
  if ( !test( this ) )
    return 0L;
  
  return (const char *)data();
} 

bool Event::test( const QEvent *event )
{
  if ( !event )
    return false;
  
  return ( event->type() == (QEvent::Type)(QEvent::User + KPARTS_EVENT_MAGIC ) );
} 

bool Event::test( const QEvent *event, const char *name )
{
  if ( !test( event ) )
    return false;
  
  return ( strcmp( name, (const char *)((QCustomEvent *)event)->data() ) == 0 );
} 

const char *GUIActivateEvent::s_strGUIActivateEvent = "KParts/GUIActivate";
const char *PartActivateEvent::s_strPartActivateEvent = "KParts/PartActivateEvent";
const char *PartSelectEvent::s_strPartSelectEvent = "KParts/PartSelectEvent";
