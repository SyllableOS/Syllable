/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

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


#include "khtml_events.h"
#include "xml/dom_nodeimpl.h"

using namespace khtml;

class MouseEvent::MouseEventPrivate
{
public:
  MouseEventPrivate()
  {
      urlHandling = true;
  }
  ~MouseEventPrivate()
  {
  }

    bool urlHandling; // ### make member KDE 3.0
    int nodeAbsX, nodeAbsY;
};

MouseEvent::MouseEvent( const char *name, QMouseEvent *qmouseEvent, int x, int y, const DOM::DOMString &url,
	                const DOM::Node &innerNode, long )
: KParts::Event( name ), m_qmouseEvent( qmouseEvent ), m_x( x ), m_y( y ),
  m_url( url ), m_innerNode( innerNode )
{
  d = new MouseEventPrivate();
}

MouseEvent::~MouseEvent()
{
  delete d;
}

int MouseEvent::nodeAbsX() const
{
    return d->nodeAbsX;
}

int MouseEvent::nodeAbsY() const
{
    return d->nodeAbsY;
}

void MouseEvent::setNodePos( int x, int y)
{
    d->nodeAbsX = x;
    d->nodeAbsY = y;
}

bool MouseEvent::isURLHandlingEnabled() const
{
    return d->urlHandling;
}

void MouseEvent::setURLHandlingEnabled( bool enable )
{
    d->urlHandling = enable;
}

long MouseEvent::offset() const
{
    int offset = 0;
    DOM::Node tempNode = 0;
    innerNode().handle()->findSelectionNode( x(), y(), nodeAbsX(), nodeAbsY(), tempNode, offset );
    return offset;
}

const char *MousePressEvent::s_strMousePressEvent = "khtml/Events/MousePressEvent";

const char *MouseDoubleClickEvent::s_strMouseDoubleClickEvent = "khtml/Events/MouseDoubleClickEvent";

const char *MouseMoveEvent::s_strMouseMoveEvent = "khtml/Events/MouseMoveEvent";

const char *MouseReleaseEvent::s_strMouseReleaseEvent = "khtml/Events/MouseReleaseEvent";

#ifndef __ATHEOS__


const char *DrawContentsEvent::s_strDrawContentsEvent = "khtml/Events/DrawContentsEvent";

class DrawContentsEvent::DrawContentsEventPrivate
{
public:
  DrawContentsEventPrivate()
  {
  }
  ~DrawContentsEventPrivate()
  {
  }
};

DrawContentsEvent::DrawContentsEvent( QPainter *painter, int clipx, int clipy, int clipw, int cliph )
  : KParts::Event( s_strDrawContentsEvent ), m_painter( painter ), m_clipx( clipx ), m_clipy( clipy ),
    m_clipw( clipw ), m_cliph( cliph )
{
  d = new DrawContentsEventPrivate;
}

DrawContentsEvent::~DrawContentsEvent()
{
  delete d;
}


#endif // __ATHEOS__
