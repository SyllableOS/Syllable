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
#include <kparts/partmanager.h>
#include <kparts/part.h>
#include <kglobal.h>
#include <kdebug.h>

#ifndef __ATHEOS__
#include <qapplication.h>
#endif

using namespace KParts;

template class QList<Part>;

namespace KParts {

class PartManagerPrivate
{
public:
  PartManagerPrivate()
  {
    m_activeWidget = 0;
    m_activePart = 0;
    m_selectedPart = 0;
    m_selectedWidget = 0;
    m_bAllowNestedParts = false;
    m_bIgnoreScrollBars = false;
  }
  ~PartManagerPrivate()
  {
  }

  Part * m_activePart;
  QWidget *m_activeWidget;

  QList<Part> m_parts;

  PartManager::SelectionPolicy m_policy;

  Part *m_selectedPart;
  QWidget *m_selectedWidget;

  bool m_bAllowNestedParts;

  QList<QWidget> m_managedTopLevelWidgets;

  bool m_bIgnoreScrollBars;
};

};

PartManager::PartManager( QWidget * parent, const char * name )
 : QObject( parent, name )
{
  d = new PartManagerPrivate;

#ifdef __ATHEOS__
  printf( "Warning: PartManager::PartManager() qApp->installEventFilter() not called\n" );
#else
  qApp->installEventFilter( this );
#endif
  d->m_policy = Direct;

  addManagedTopLevelWidget( parent );
}

PartManager::PartManager( QWidget *topLevel, QObject *parent, const char *name )
 : QObject( parent, name )
{
  d = new PartManagerPrivate;

#ifdef __ATHEOS__
  printf( "Warning: PartManager::PartManager() qApp->installEventFilter() not called\n" );
#else
  qApp->installEventFilter( this );
#endif
  
  d->m_policy = Direct;

  addManagedTopLevelWidget( topLevel );
}

PartManager::~PartManager()
{
  QListIterator<QWidget> it( d->m_managedTopLevelWidgets );
  for (; it.current(); ++it )
    disconnect( it.current(), SIGNAL( destroyed() ),
                this, SLOT( slotManagedTopLevelWidgetDestroyed() ) );

  // core dumps ... setActivePart( 0L );
#ifdef __ATHEOS__
  printf( "Warning: PartManager::~PartManager() qApp->removeEventFilter() not called\n" );
#else
  qApp->removeEventFilter( this );
#endif
  delete d;
}

void PartManager::setSelectionPolicy( SelectionPolicy policy )
{
  d->m_policy = policy;
}

PartManager::SelectionPolicy PartManager::selectionPolicy() const
{
  return d->m_policy;
}

void PartManager::setAllowNestedParts( bool allow )
{
  d->m_bAllowNestedParts = allow;
}

bool PartManager::allowNestedParts() const
{
  return d->m_bAllowNestedParts;
}

void PartManager::setIgnoreScrollBars( bool ignore )
{
  d->m_bIgnoreScrollBars = ignore;
}

bool PartManager::ignoreScrollBars() const
{
  return d->m_bIgnoreScrollBars;
}

bool PartManager::eventFilter( QObject *obj, QEvent *ev )
{
#ifdef __ATHEOS__
    if ( obj == NULL ) obj = NULL;
    if ( ev == NULL ) ev = NULL;
    printf( "Warning: PartManager::eventFilter() called. Stripped a lot\n" );
#else
  if ( ev->type() != QEvent::MouseButtonPress &&
       ev->type() != QEvent::MouseButtonDblClick &&
       ev->type() != QEvent::FocusIn )
    return false;

  if ( !obj->isWidgetType() )
    return false;

  QWidget *w = static_cast<QWidget *>( obj );

  if ( ( w->testWFlags( WStyle_Dialog ) && w->isModal() ) ||
       w->testWFlags( WType_Popup ) || w->testWFlags( WStyle_Tool ) )
    return false;

  Part * part;
  while ( w )
  {
    QPoint pos;

    if ( !d->m_managedTopLevelWidgets.containsRef( w->topLevelWidget() ) )
      return false;
#ifdef __ATHEOS__
    if ( d->m_bIgnoreScrollBars && dynamic_cast<QScrollBar*>(w) != NULL ) {
#else    
    if ( d->m_bIgnoreScrollBars && w->inherits( "QScrollBar" ) ) {
#endif	
      return false;
    }

    if ( ev->type() == QEvent::MouseButtonPress || ev->type() == QEvent::MouseButtonDblClick )
    {
      pos = static_cast<QMouseEvent *>( ev )->globalPos();
      part = findPartFromWidget( w, pos );
    } else
      part = findPartFromWidget( w );

    if ( part ) // We found a part whose widget is w
    {
      if ( d->m_policy == PartManager::TriState )
      {
        if ( ev->type() == QEvent::MouseButtonDblClick )
        {
          if ( part == d->m_activePart && w == d->m_activeWidget )
            return false;

          setActivePart( part, w );
          return true;
        }

        if ( ( d->m_selectedWidget != w || d->m_selectedPart != part ) &&
             ( d->m_activeWidget != w || d->m_activePart != part ) )
        {
          if ( part->isSelectable() )
            setSelectedPart( part, w );
          else
            setActivePart( part, w );
          return true;
        }
        else if ( d->m_selectedWidget == w && d->m_selectedPart == part )
        {
          setActivePart( part, w );
          return true;
        }
        else if ( d->m_activeWidget == w && d->m_activePart == part )
        {
          setSelectedPart( 0L );
          return false;
        }

        return false;
      }
      else if ( part != d->m_activePart )
      {
        //kdDebug(1000) << QString("Part %1 made active because %2 got event").arg(part->name()).arg(w->className()) << endl;

        setActivePart( part, w );
      }

      return false;
    }

    w = w->parentWidget();

    if ( w && ( ( w->testWFlags( WStyle_Dialog ) && w->isModal() ) ||
                w->testWFlags( WType_Popup ) || w->testWFlags( WStyle_Tool ) ) )
    {
      //kdDebug(1000) << QString("No part made active although %1/%2 got event - loop aborted").arg(obj->name()).arg(obj->className()) << endl;
      return false;
    }

  }
#endif // __ATHEOS__

  //kdDebug(1000) << QString("No part made active although %1/%2 got event").arg(obj->name()).arg(obj->className()) << endl;
  return false;
}

Part * PartManager::findPartFromWidget( QWidget * widget, const QPoint &pos )
{
  QListIterator<Part> it ( d->m_parts );
  for ( ; it.current() ; ++it )
  {
    Part *part = it.current()->hitTest( widget, pos );
    if ( part && d->m_parts.findRef( part ) != -1 )
      return part;
  }
  return 0L;
}

Part * PartManager::findPartFromWidget( QWidget * widget )
{
  QListIterator<Part> it ( d->m_parts );
  for ( ; it.current() ; ++it )
  {
    if ( widget == it.current()->widget() )
      return it.current();
  }
  return 0L;
}

void PartManager::addPart( Part *part, bool setActive )
{
    if ( d->m_parts.findRef( part ) != -1 ) // don't add parts more than once :)
      return;

  connect( part, SIGNAL( destroyed() ), this, SLOT( slotObjectDestroyed() ) );

  d->m_parts.append( part );

  part->setManager( this );

  if ( setActive )
  {
    setActivePart( part );
    if ( part->widget() )
      part->widget()->setFocus();
  }

#ifndef __ATHEOS__
  // Prevent focus problems
  if ( part->widget() && part->widget()->focusPolicy() == QWidget::NoFocus )
  {
    kdWarning(1000) << "Part '" << part->name() << "' has a widget " << part->widget()->name() << " with a focus policy of NoFocus. It should have at least a ClickFocus policy, for part activation to work well." << endl;
  }
  if ( part->widget() && part->widget()->focusPolicy() == QWidget::TabFocus )
  {
    kdWarning(1000) << "Part '" << part->name() << "' has a widget " << part->widget()->name() << " with a focus policy of TabFocus. It should have at least a ClickFocus policy, for part activation to work well." << endl;
  }
#endif // __ATHEOS__

  if ( part->widget() )
    part->widget()->show();
  emit partAdded( part );
}

void PartManager::removePart( Part *part )
{
  if ( d->m_parts.findRef( part ) == -1 )
  {
    kdFatal(1000) << QString("Can't remove part %1, not in KPartManager's list.").arg(part->name()) << endl;
    return;
  }
  disconnect( part, SIGNAL( destroyed() ), this, SLOT( slotObjectDestroyed() ) );

  //Warning. The part could be already deleted
  //kdDebug(1000) << QString("Part %1 removed").arg(part->name()) << endl;
  d->m_parts.removeRef( part );

  emit partRemoved( part );

  if ( part == d->m_activePart )
    setActivePart( 0 );
}

void PartManager::replacePart( Part * oldPart, Part * newPart, bool setActive )
{
  kdDebug(1000) << QString("replacePart %1 -> %2").arg(oldPart->name()).arg(newPart->name()) << endl;
  // This methods does exactly removePart + addPart but without calling setActivePart(0) in between
  if ( d->m_parts.findRef( oldPart ) == -1 )
  {
    kdFatal(1000) << QString("Can't remove part %1, not in KPartManager's list.").arg(oldPart->name()) << endl;
    return;
  }
  disconnect( oldPart, SIGNAL( destroyed() ), this, SLOT( slotObjectDestroyed() ) );

  d->m_parts.removeRef( oldPart );
  emit partRemoved( oldPart );

  addPart( newPart, setActive );
}

void PartManager::setActivePart( Part *part, QWidget *widget )
{
  if ( part && d->m_parts.findRef( part ) == -1 )
  {
      kdWarning( 1000 ) << "PartManager::setActivePart : trying to active a non-registered part! " << part->name() << endl;
      return; // don't allow someone call setActivePart with a part we don't know about
  }

  //check whether nested parts are disallowed and activate the top parent part then, by traversing the
  //tree recursively (Simon)
  if ( part && !d->m_bAllowNestedParts )
  {
    QObject *parentPart = part->parent(); // ### this relies on people using KParts::Factory!
#ifdef __ATHEOS__
    if ( dynamic_cast<KParts::Part*>(parentPart) != NULL )
#else    
    if ( parentPart && parentPart->inherits( "KParts::Part" ) )
#endif
    {
      setActivePart( static_cast<KParts::Part *>( parentPart ) );
      return;
    }
  }

  KParts::Part *oldActivePart = d->m_activePart;
  QWidget *oldActiveWidget = d->m_activeWidget;

  setSelectedPart( 0L );

  d->m_activePart = part;
  d->m_activeWidget = widget;

  if ( oldActivePart )
  {
    KParts::Part *savedActivePart = part;
    QWidget *savedActiveWidget = widget;

#ifdef __ATHEOS__
  printf( "Warning: PartManager::setActivePart() QApplication::sendEvent() not called\n" );
#else
    PartActivateEvent ev( false, oldActivePart, oldActiveWidget );
    QApplication::sendEvent( oldActivePart, &ev );
    if ( oldActiveWidget )
    {
      disconnect( oldActiveWidget, SIGNAL( destroyed() ),
                  this, SLOT( slotWidgetDestroyed() ) );
      QApplication::sendEvent( oldActiveWidget, &ev );
    }
#endif

    d->m_activePart = savedActivePart;
    d->m_activeWidget = savedActiveWidget;
  }

  if ( d->m_activePart )
  {
    if ( !widget )
      d->m_activeWidget = part->widget();

#ifdef __ATHEOS__
  printf( "Warning: PartManager::setActivePart() QApplication::sendEvent() not called\n" );
#else
    PartActivateEvent ev( true, d->m_activePart, d->m_activeWidget );
    QApplication::sendEvent( d->m_activePart, &ev );
    if ( d->m_activeWidget )
    {
      connect( d->m_activeWidget, SIGNAL( destroyed() ),
               this, SLOT( slotWidgetDestroyed() ) );
      QApplication::sendEvent( d->m_activeWidget, &ev );
    }
#endif // __ATHEOS__
  }
  // Set the new active instance in KGlobal
  setActiveInstance( d->m_activePart ? d->m_activePart->instance() : 0L );

  kdDebug(1000) << this << " emitting activePartChanged " << d->m_activePart << endl;
  emit activePartChanged( d->m_activePart );
}

void PartManager::setActiveInstance( KInstance * instance )
{
  // It's a separate method to allow redefining this behaviour
  KGlobal::_activeInstance = instance;
}

Part *PartManager::activePart() const
{
  return d->m_activePart;
}

QWidget *PartManager::activeWidget() const
{
  return  d->m_activeWidget;
}

void PartManager::setSelectedPart( Part *part, QWidget *widget )
{
  if ( part == d->m_selectedPart && widget == d->m_selectedWidget )
    return;

#ifndef __ATHEOS__
  Part *oldPart = d->m_selectedPart;
  QWidget *oldWidget = d->m_selectedWidget;
#endif
  
  d->m_selectedPart = part;
  d->m_selectedWidget = widget;

  if ( part && !widget )
    d->m_selectedWidget = part->widget();

#ifdef __ATHEOS__
  printf( "Warning: PartManager::setSelectedPart() QApplication::sendEvent() not called\n" );
#else
  if ( oldPart )
  {
    PartSelectEvent ev( false, oldPart, oldWidget );
    QApplication::sendEvent( oldPart, &ev );
    QApplication::sendEvent( oldWidget, &ev );
  }

  if ( d->m_selectedPart )
  {
    PartSelectEvent ev( true, d->m_selectedPart, d->m_selectedWidget );
    QApplication::sendEvent( d->m_selectedPart, &ev );
    QApplication::sendEvent( d->m_selectedWidget, &ev );
  }
#endif // __ATHEOS__
}

Part *PartManager::selectedPart() const
{
  return d->m_selectedPart;
}

QWidget *PartManager::selectedWidget() const
{
  return d->m_selectedWidget;
}

void PartManager::slotObjectDestroyed()
{
  kdDebug(1000) << "KPartManager::slotObjectDestroyed()" << endl;
  removePart( const_cast<Part *>( static_cast<const Part *>( sender() ) ) );
}

void PartManager::slotWidgetDestroyed()
{
  kdDebug(1000) << "KPartsManager::slotWidgetDestroyed()" << endl;
  if ( static_cast<const QWidget *>( sender() ) == d->m_activeWidget )
    setActivePart( 0L ); //do not remove the part because if the part's widget dies, then the
                         //part will delete itself anyway (which ends up in a slotObjectDestroyed() call
}

const QList<Part> *PartManager::parts() const
{
  return &d->m_parts;
}

void PartManager::addManagedTopLevelWidget( const QWidget *topLevel )
{
  if ( !topLevel->isTopLevel() )
    return;

  if ( d->m_managedTopLevelWidgets.containsRef( topLevel ) )
    return;

  d->m_managedTopLevelWidgets.append( topLevel );
  connect( topLevel, SIGNAL( destroyed() ),
           this, SLOT( slotManagedTopLevelWidgetDestroyed() ) );
}

void PartManager::removeManagedTopLevelWidget( const QWidget *topLevel )
{
  if ( !topLevel->isTopLevel() )
    return;

  if ( d->m_managedTopLevelWidgets.findRef( topLevel ) == -1 )
    return;

  d->m_managedTopLevelWidgets.remove();
}

void PartManager::slotManagedTopLevelWidgetDestroyed()
{
  const QWidget *widget = static_cast<const QWidget *>( sender() );
  removeManagedTopLevelWidget( widget );
}

#include "partmanager.moc"
