/****************************************************************************
** $Id$
**
** Implementation of QObject class
**
** Created : 930418
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qvariant.h"
#include "qapplication.h"
#include "qobject.h"
#include "qobjectlist.h"
#include "qobjectdict.h"
#include "qsignalslotimp.h"
#include "qregexp.h"
#include <ctype.h>

#ifndef __ATHEOS__
#include "qpixmap.h"
#endif
#include "qiconset.h"
#include "qimage.h"
#include "qregion.h"
#include "qbitmap.h"
#include "qpointarray.h"
#include "qcursor.h"

#ifdef __ATHEOS__
#include <stdio.h>
#endif

// NOT REVISED
/*! \class Qt qnamespace.h

  \brief The Qt class is a namespace for miscellaneous identifiers
  that need to be global-like.

  \ingroup misc

  Normally, you can ignore this class.  QObject and a few other
  classes inherit it, so that all the identifiers in the Qt namespace
  are visible to you without qualification.

  However, occasionally you may need to say \c Qt::black instead just
  \c black, particularly in static utility functions (such as many
  class factories).

*/

/*! \enum Qt::Orientation

  This type is used to signify whether an object should be \c
  Horizontal or \c Vertical (for example in QScrollBar).
*/


/*!
  \class QObject qobject.h
  \brief The QObject class is the base class of all Qt objects.

  \ingroup objectmodel

  QObject is the heart of the \link object.html Qt object model.
  \endlink The central feature in this model is a very powerful
  mechanism for seamless object commuinication dubbed \link
  signalsandslots.html signals and slots \endlink. With connect(), you
  can connect a signal to a slot and destroy the connection again with
  disconnect(). To avoid never-ending notification loops, you can
  temporarily block signals with blockSignals(). The protected
  functions connectNotify() and disconnectNotify() make it possible to
  track connections.

  QObjects organize themselves in object trees. When you create a
  QObject with another object as parent, it will automatically do an
  insertChild() on the parent and thus show up in the parent's
  children() list. The parent receives object ownership, i.e. it will
  automatically delete its children in its destructor. You can look
  for an object by name and optionally type using child() or
  queryList(), and get the list of tree roots using objectTrees().

  Every object has an object name() and can report its className() and
  whether it inherits() another class in the QObject inheritance
  hierarchy.

  When an object is deleted, it emits a destroyed() signal. You can
  catch this signal to avoid dangling references to QObjects. The
  QGuardedPtr class provides an elegant way to use this feature.

  QObjects can receive events through event() and filter events of
  other objects. See installEventFilter() and eventFilter() for
  details. A convenience handler childEvent() can be reimplemented to
  catch child events.

  Last but not least, QObject provides the basic timer support in Qt,
  see QTimer for high-level support for timers.

  Notice that the \c Q_OBJECT macro is mandatory for any object that
  implement signals, slots or properties.  You also need to run the
  \link moc.html moc program (Meta Object Compiler) \endlink on the
  source file. We strongly recommend to use the macro in \e all
  subclasses of QObject regardless whether they actually use signals,
  slots and properties or not. Otherwise certain functions can show
  undefined behaviour.

  All Qt widgets inherit QObject. The convenience function
  isWidgetType() returns whether an object is actually a widget.  It
  is much faster than inherits( "QWidget" ).
*/


/* (no '!' on purpose since this is an internal class)
  \class QSenderObject qobject.h
  \brief Internal object used for sending signals.

  \internal

  It is generally a very bad idea to use this class directly in
  application programs.

  In particular, you cannot not use it to send signals from classes
  that do not inherit QObject. If you wish to do that, make an
  internal class that inherits QObject and has the necessary signals
  and slots.  Alternatively, you can use the QSignal class.
*/

/*
  \fn void QSenderObject::setSender (QObject* s)

  Internal function, used in signal-slot connections.
*/


//
// Remove white space from SIGNAL and SLOT names.
// Internal for QObject::connect() and QObject::disconnect()
//

static inline bool isIdentChar( char x )
{						// Avoid bug in isalnum
    return x == '_' || (x >= '0' && x <= '9') ||
	 (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z');
}

static inline bool isSpace( char x )
{
#if defined(_CC_BOR_)
  /*
    Borland C++ 4.5 has a weird isspace() bug.
    isspace() usually works, but not here.
    This implementation is sufficient for our internal use: rmWS()
  */
    return (uchar)x <= 32;
#else
    return isspace( x );
#endif
}

static QCString qt_rmWS( const char *src )
{
    QCString result( qstrlen(src)+1 );
    char *d = result.data();
    char *s = (char *)src;
    char last = 0;
    while( *s && isSpace(*s) )			// skip leading space
	s++;
    while ( *s ) {
	while ( *s && !isSpace(*s) )
	    last = *d++ = *s++;
	while ( *s && isSpace(*s) )
	    s++;
	if ( *s && isIdentChar(*s) && isIdentChar(last) )
	    last = *d++ = ' ';
    }
    result.truncate( (int)(d - result.data()) );
    int void_pos = result.find("(void)");
    if ( void_pos >= 0 )
	result.remove( void_pos+1, qstrlen("void") );
    return result;
}


// Event functions, implemented in qapplication_xxx.cpp

int   qStartTimer( int interval, QObject *obj );
bool  qKillTimer( int id );
bool  qKillTimer( QObject *obj );

void  qRemovePostedEvents( QObject* );


QMetaObject *QObject::metaObj = 0;


static void removeObjFromList( QObjectList *objList, const QObject *obj,
			       bool single=FALSE )
{
    if ( !objList )
	return;
    int index = objList->findRef( obj );
    while ( index >= 0 ) {
	objList->remove();
	if ( single )
	    return;
	index = objList->findNextRef( obj );
    }
}


/*!
  \relates QObject

  Returns a pointer to the child named \a name of QObject \a parent
  which inherits type \a type.

  Returns 0 if there is no such child.

  \code
    QListBox * c = (QListBox *)::qt_find_obj_child(myWidget,QListBox,
						   "listboxname");
    if ( c )
	c->insertItem( "another string" );
  \endcode
*/

void *qt_find_obj_child( QObject *parent, const char *type, const char *name )
{
    const QObjectList *list = parent->children();
    if ( list ) {
	QObjectListIt it( *list );
	QObject *obj;
	while ( (obj=it.current()) ) {
	    ++it;
	    if ( qstrcmp(name,obj->name()) == 0 &&
		obj->inherits(type) )
		return obj;
	}
    }
    return 0;
}


static QObjectList* object_trees = 0;
static void cleanup_object_trees()
{
    delete object_trees;
    object_trees = 0;
}

static void ensure_object_trees()
{
    if ( object_trees )
	return;
    object_trees = new QObjectList;
#ifndef __ATHEOS__
    qAddPostRoutine( cleanup_object_trees );
#endif
}

static void insert_tree( QObject* obj )
{
    ensure_object_trees();
    object_trees->insert(0, obj );
}

static void remove_tree( QObject* obj )
{
    if ( object_trees )
	object_trees->removeRef( obj );
}


/*****************************************************************************
  QObject member functions
 *****************************************************************************/

/*!
  Constructs an object with the parent object \e parent and a \e name.

  The parent of an object may be viewed as the object's owner. For
  instance, a \link QDialog dialog box\endlink is the parent of the
  "ok" and "cancel" buttons inside it.

  The destructor of a parent object destroys all child objects.

  Setting \e parent to 0 constructs an object with no parent.
  If the object is a widget, it will become a top-level window.

  The object name is a text that can be used to identify this QObject.
  It's particularly useful in conjunction with
    <a href=designer.html>the Qt Designer</a>.
  You can find an object by name (and type) using child(), and more
  than one using queryList().

  \sa parent(), name(), child(), queryList()
*/

QObject::QObject( QObject *parent, const char *name )
{
    if ( !objectDict )				// will create object dict
	initMetaObject();
    objname       = name ? qstrdup(name) : 0;   // set object name
    childObjects  = 0;				// no children yet
    connections   = 0;				// no connections yet
    senderObjects = 0;				// no signals connected yet
    eventFilters  = 0;				// no filters installed
#if QT_VERSION >= 300
#error "Remove sigSender - it's a massive waste."
    // there was a comment here saying "change 3.0 incompatibly but
    // don't give people any warning of the upcoming change" or words
    // to that effect.
#endif
    sigSender     = 0;				// no sender yet
    isSignal   = FALSE;				// assume not a signal object
    isWidget   = FALSE;				// assume not a widget object
    pendTimer  = FALSE;				// no timers yet
    pendEvent  = FALSE;				// no events yet
    blockSig   = FALSE;				// not blocking signals
    wasDeleted = FALSE;				// double-delete catcher
    isTree = FALSE;				// no tree yet
    parentObj  = parent;			// to avoid root checking in insertChild()
    if ( parent ) {				// add object to parent
	parent->insertChild( this );
    } else {
	insert_tree( this );
	isTree = TRUE;
    }
}



/*!
  Destructs the object, deleting all its child objects.

  All signals to and from the object are automatically disconnected.

  \warning \e All child objects are deleted.  If any of these objects are
  on the stack or global, your program will sooner or later crash.  We do
  not recommend holding pointers to child objects from outside the parent.
  If you still do, the QObject::destroyed() signal gives you an
  opportunity to detect when an object is destroyed.
*/

QObject::~QObject()
{
    if ( wasDeleted ) {
#if defined(DEBUG)
	qWarning( "Double QObject deletion detected." );
#endif
	return;
    }
    wasDeleted = 1;
    emit destroyed();
    if ( objname )
	delete [] (char*)objname;
    objname = 0;
#ifdef __ATHEOS__
//    printf( "Warning: QObject::~QObject() not removing events\n" );
#else
    if ( pendTimer )				// might be pending timers
	qKillTimer( this );
    if ( pendEvent )
	QApplication::removePostedEvents( this );
#endif    
    if ( isTree ) {
	remove_tree( this );		// remove from global root list
	isTree = FALSE;
    }
    if ( parentObj ) 				// remove it from parent object
	parentObj->removeChild( this );
    register QObject *obj;
    if ( senderObjects ) {			// disconnect from senders
	QObjectList *tmp = senderObjects;
	senderObjects = 0;
	obj = tmp->first();
	while ( obj ) {				// for all senders...
	    obj->disconnect( this );
	    obj = tmp->next();
	}
	delete tmp;
    }
    if ( connections ) {			// disconnect receivers
	QSignalDictIt it(*connections);
	QConnectionList *clist;
	while ( (clist=it.current()) ) {	// for each signal...
	    ++it;
	    register QConnection *c;
	    QConnectionListIt cit(*clist);
	    while( (c=cit.current()) ) {	// for each connected slot...
		++cit;
		if ( (obj=c->object()) )
		    removeObjFromList( obj->senderObjects, this );
	    }
	}
	delete connections;
	connections = 0;
    }
    if ( eventFilters ) {
	delete eventFilters;
	eventFilters = 0;
    }
    if ( childObjects ) {			// delete children objects
	QObjectListIt it(*childObjects);
	while ( (obj=it.current()) ) {
	    ++it;
	    obj->parentObj = 0;
	    // ### nest line is a QGList workaround - remove in 3.0
	    childObjects->removeRef( obj );
	    delete obj;
	}
	delete childObjects;
    }
}


/*!
  \fn QMetaObject *QObject::metaObject() const
  Returns a pointer to the meta object of this object.

  A meta object contains information about a class that inherits
  QObject: class name, super class name, properties, signals and
  slots. Every class that contains the \c Q_OBJECT macro will also
  have a meta object.

  The meta object information is required by the signal/slot
  connection mechanism and the property system.  The functions isA()
  and inherits() also make use of the meta object.
*/

/*!
  Returns the class name of this object.

  This function is generated by the \link metaobjects.html Meta Object
  Compiler. \endlink

  \warning This function will return an invalid name if the class
  definition lacks the \c Q_OBJECT macro.

  \sa name(), inherits(), isA(), isWidgetType()
*/

const char *QObject::className() const
{
    return "QObject";
}


/*!
  Returns TRUE if this object is an instance of a specified class,
  otherwise FALSE.

  Example:
  \code
    QTimer *t = new QTimer;		// QTimer inherits QObject
    t->isA("QTimer");			// returns TRUE
    t->isA("QObject");			// returns FALSE
  \endcode

  \sa inherits(), metaObject()
*/

bool QObject::isA( const char *clname ) const
{
    QMetaObject *meta = queryMetaObject();
    return meta ? qstrcmp(clname,meta->className()) == 0 : FALSE;
}

/*!
  Returns TRUE if this object is an instance of a class that inherits
  \e clname, and \a clname inherits QObject.

  (A class is considered to inherit itself.)

  Example:
  \code
    QTimer *t = new QTimer;		// QTimer inherits QObject
    t->inherits("QTimer");		// returns TRUE
    t->inherits("QObject");		// returns TRUE
    t->inherits("QButton");		// returns FALSE

    QScrollBar * s = new QScrollBar;	// inherits QWidget and QRangeControl
    s->inherits( "QWidget" );		// returns TRUE
    s->inherits( "QRangeControl" ); 	// returns FALSE
  \endcode

  \sa isA(), metaObject()
*/

bool QObject::inherits( const char *clname ) const
{
    QMetaObject *meta = queryMetaObject();
    return meta? meta->inherits( clname ) : FALSE;
}


#if QT_VERSION >= 290
#error "remove superClasses now."
#endif
#ifndef QT_NO_STRINGLIST
/*! \obsolete

  This function is misleadingly named, and cannot be implemented in a
  way that fulfills its name.  someQWidget->superClasses() should have
  returned QPaintDevice and QObject, obviously.  And it never can, so
  let us kill the function. <strong>It will be removed in Qt-3.0</strong>

  Oh, and the return type was wrong, too.  QStringList not QStrList.
*/

QStringList QObject::superClasses( bool includeThis ) const
{
    qObsolete( "QObject", "superClasses" ); // Arnt killed it.  RIP.

    QStringList lst;

    QMetaObject *meta = queryMetaObject();
    if ( meta ) {
	if ( !includeThis )
	    meta = meta->superClass();
	while ( meta ) {
	    lst.append( QString::fromLatin1(meta->className()) );
	    meta = meta->superClass();
	}
    }
    return lst;
}
#endif

/*!
  \fn const char *QObject::name() const

  Returns the name of this object. If the object does not have a name,
  it will return "unnamed", so that printf() (used in qDebug()) will
  not be asked to output a null pointer.  If you want a null pointer
  to be returned for unnamed objects, you can call name( 0 ).

  \code
    qDebug( "MyClass::setPrecision(): (%s) unable to set precision to %f",
	    name(), newPrecision );
  \endcode

  The object name is set by the constructor or by the setName()
  function.  The object name is not very useful in the current version
  of Qt, but will become increasingly important in the future.

  You can find an object by name (and type) using child(), and more
  than one using queryList().

  \sa setName(), className(), child(), queryList()
*/
const char * QObject::name() const
{
    // If you change the name here, the builder will be broken
    return objname ? objname : "unnamed";
}

/*!
  Returns the name of this object, or \a defaultName if the object
  does not have a name.
*/

const char * QObject::name( const char * defaultName ) const
{
    return objname ? objname : defaultName;
}


/*!
  Sets the name of this object to \e name.  The default name is the
  one assigned by the constructor.

  You can find an object by name (and type) using child(), and more
  than one using queryList().

  \sa name(), className(), queryList(), child()
*/

void QObject::setName( const char *name )
{
    if ( objname )
	delete [] (char*) objname;
    objname = name ? qstrdup(name) : 0;
}

/*!
  Searches through the children and grandchildren of this object for
  an object named \a name and with type \a type (or a subclass of that
  type), and returns a pointer to that object if it exists.  If \a
  type is 0, any type matches.

  If there isn't any such object, this function returns null.

  If there is more than one, one of them is returned; use queryList()
  if you need all of them.
*/

QObject* QObject::child( const char *name, const char *type )
{
    const QObjectList *list = children();
    if ( list ) {
	QObjectListIt it( *list );
	QObject *obj;
	while ( ( obj = it.current() ) ) {
	    ++it;
	    if ( ( !type || obj->inherits(type) ) && ( !name || qstrcmp( name, obj->name() ) == 0 ) )
		return obj;
	}

	// Recursion: Ask our children ...
	QObjectListIt it2( *list );
	while ( ( obj = it2.current() ) ) {
	    ++it2;
	    QObject* o = obj->child( name, type );
	    if ( o )
	      return o;
	}
    }

    return 0;
}

/*!
  \fn bool QObject::isWidgetType() const
  Returns TRUE if the object is a widget, or FALSE if not.

  Calling this function is equivalent to calling inherits("QWidget"),
  except that it is much faster.
*/

/*!
  \fn bool QObject::highPriority() const
  Returns TRUE if the object is a high priority object, or FALSE if it is a
  standard priority object.

  High priority objects are placed first in list of children,
  on the assumption that they will be referenced very often.
*/


/*!
  This virtual function receives events to an object and should return
  TRUE if the event was recognized and processed.

  The event() function can be reimplemented to customize the behavior of
  an object.

  \sa installEventFilter(), timerEvent(), QApplication::sendEvent(),
  QApplication::postEvent(), QWidget::event()
*/

bool QObject::event( QEvent *e )
{
#if defined(CHECK_NULL)
    if ( e == 0 )
	qWarning( "QObject::event: Null events are not permitted" );
#endif
    if ( eventFilters ) {			// try filters
	if ( activate_filters(e) )		// stopped by a filter
	    return TRUE;
    }
    switch ( e->type() ) {
      case QEvent::Timer:
#ifdef __ATHEOS__
	  printf( "Warning: QObject::event() received timer event\n" );
#else	  
	timerEvent( (QTimerEvent*)e );
#endif	
	return TRUE;
      case QEvent::ChildInserted: case QEvent::ChildRemoved:
#ifdef __ATHEOS__
	  printf( "Warning: QObject::event() received child event\n" );
#else	  
	childEvent( (QChildEvent*)e );
#endif	
	return TRUE;
      default:
	break;
    }
    return FALSE;
}

/*!
  This event handler can be reimplemented in a subclass to receive
  timer events for the object.

  QTimer provides a higher-level interface to the timer functionality,
  and also more general information about timers.

  \sa startTimer(), killTimer(), killTimers(), event()
*/

#ifndef __ATHEOS__
void QObject::timerEvent( QTimerEvent * )
{
}
#endif

/*!
  This event handler can be reimplemented in a subclass to receive
  child events.

  Child events are sent to objects when children are inserted or removed.

  Note that events with QEvent::type() \c QEvent::ChildInserted are
  \e posted (with QApplication::postEvent()), to make sure that the
  child's construction is completed before this function is called.

  If you change state based on \c ChildInserted events, call
  QWidget::constPolish(), or do
  <code>QApplication::sendPostedEvents( this, QEvent::ChildInserted );</code>
  in functions that depend on the state. One notable example is
  QWidget::sizeHint().

  \sa event(), QChildEvent
*/

#ifndef __ATHEOS__
void QObject::childEvent( QChildEvent * )
{
}
#endif

/*!
  Filters events if this object has been installed as an event filter for
  another object.

  The reimplementation of this virtual function must return TRUE if the
  event should be stopped, or FALSE if the event should be dispatched normally.

  \warning
  If you delete the receiver object in this function, be sure to return TRUE.
  If you return FALSE, Qt sends the event to the deleted object and the
  program will crash.

  \sa installEventFilter()
*/

bool QObject::eventFilter( QObject *, QEvent * )
{
    return FALSE;				// don't do anything with it
}


/*!
  \internal
  Activates all event filters for this object.
  This function is normally called from QObject::event() or QWidget::event().
*/

bool QObject::activate_filters( QEvent *e )
{
    if ( !eventFilters )			// no event filter
	return FALSE;
    QObjectListIt it( *eventFilters );
    register QObject *obj = it.current();
    while ( obj ) {				// send to all filters
	++it;					//   until one returns TRUE
	if ( obj->eventFilter(this,e) ) {
	    return TRUE;
	}
	obj = it.current();
    }
    return FALSE;				// don't do anything with it
}

/*!
  \fn bool QObject::signalsBlocked() const
  Returns TRUE if signals are blocked, or FALSE if signals are not blocked.

  Signals are not blocked by default.
  \sa blockSignals()
*/

/*!
  Blocks signals if \e block is TRUE, or unblocks signals if \e block is FALSE.

  Emitted signals disappear into hyperspace if signals are blocked.
*/

void QObject::blockSignals( bool block )
{
    blockSig = block;
}


//
// The timer flag hasTimer is set when startTimer is called.
// It is not reset when killing the timer because more than
// one timer might be active.
//

/*!
  Starts a timer and returns a timer identifier, or returns zero if
  it could not start a timer.

  A timer event will occur every \e interval milliseconds until
  killTimer() or killTimers() is called.  If \e interval is 0, then
  the timer event occurs once every time there are no more window system
  events to process.

  The virtual timerEvent() function is called with the QTimerEvent event
  parameter class when a timer event occurs.  Reimplement this function to
  get timer events.

  If multiple timers are running, the QTimerEvent::timerId() can be
  used to find out which timer was activated.

  Example:
  \code
    class MyObject : public QObject
    {
    public:
	MyObject( QObject *parent=0, const char *name=0 );
    protected:
	void  timerEvent( QTimerEvent * );
    };

    MyObject::MyObject( QObject *parent, const char *name )
	: QObject( parent, name )
    {
	startTimer( 50 );			// 50 millisecond timer
	startTimer( 1000 );			// 1 second timer
	startTimer( 60000 );			// 1 minute timer
    }

    void MyObject::timerEvent( QTimerEvent *e )
    {
	qDebug( "timer event, id=%d", e->timerId() );
    }
  \endcode

  There is practically no upper limit for the interval value (more than
  one year).  The accuracy depends on the underlying operating system.
  Windows 95 has 55 millisecond (18.2 times per second) accuracy; other
  systems that we have tested (UNIX X11, Windows NT and OS/2) can
  handle 1 millisecond intervals.

  The QTimer class provides a high-level programming interface with
  one-shot timers and timer signals instead of events.

  \sa timerEvent(), killTimer(), killTimers()
*/
#ifndef __ATHEOS__
int QObject::startTimer( int interval )
{
    pendTimer = TRUE;				// set timer flag
    return qStartTimer( interval, (QObject *)this );
}

/*!
  Kills the timer with the identifier \e id.

  The timer identifier is returned by startTimer() when a timer event is
  started.

  \sa timerEvent(), startTimer(), killTimers()
*/
void QObject::killTimer( int id )
{
    qKillTimer( id );
}
/*!
  Kills all timers that this object has started.

  Note that using this function can cause hard-to-find bugs: It kills
  timers started by sub- and superclasses as well as those started by
  you, which is often not what you want.  Therefore, we recommend
  using a QTimer, or perhaps killTimer().

  \sa timerEvent(), startTimer(), killTimer()
*/

void QObject::killTimers()
{
    qKillTimer( this );
}
#endif // __ATHEOS__


static void objSearch( QObjectList *result,
		       QObjectList *list,
		       const char  *inheritsClass,
		       bool onlyWidgets,
		       const char  *objName,
		       QRegExp	   *rx,
		       bool	    recurse )
{
    if ( !list || list->isEmpty() )		// nothing to search
	return;
    QObject *obj = list->first();
    while ( obj ) {
	bool ok = TRUE;
	if ( onlyWidgets )
	    ok = obj->isWidgetType();
	else if ( inheritsClass && !obj->inherits(inheritsClass) )
	    ok = FALSE;
	if ( ok ) {
	    if ( objName )
		ok = qstrcmp(objName,obj->name()) == 0;
	    else if ( rx )
		ok = rx->match(QString::fromLatin1(obj->name())) >= 0;
	}
	if ( ok )				// match!
	    result->append( obj );
	if ( recurse && obj->children() )
	    objSearch( result, (QObjectList *)obj->children(), inheritsClass,
		       onlyWidgets, objName, rx, recurse );
	obj = list->next();
    }
}


/*!
  \fn QObject *QObject::parent() const
  Returns a pointer to the parent object.
  \sa children()
*/

/*!
  \fn const QObjectList *QObject::children() const
  Returns a list of child objects, or 0 if this object has no children.

  The QObjectList class is defined in the qobjcoll.h header file.

  The latest child added is the \link QList::first() first\endlink object
  in the list and the first child added is the \link QList::last()
  last\endlink object in the list.

  Note that the list order changes when QWidget children are \link
  QWidget::raise() raised\endlink or \link QWidget::lower()
  lowered.\endlink A widget that is raised becomes the last object in
  the list, and a widget that is lowered becomes the first object in
  the list.

  \sa child(), queryList(), parent(), insertChild(), removeChild()
*/


/*!
  Returns a pointer to the list of all object trees (respectively
  their root objects), or 0 if there are no objects.

  The QObjectList class is defined in the qobjcoll.h header file.

  The latest root object created is the \link QList::first()
  first\endlink object in the list and the first root object added is
  the \link QList::last() last\endlink object in the list.

  \sa children(), parent(), insertChild(), removeChild()
 */
const QObjectList *QObject::objectTrees()
{
    return object_trees;
}


/*!  Searches the children and optinally grandchildren of this object,
  and returns a list of those objects that are named or matches \a
  objName and inherit \a ineritsClass.  If \a inheritsClass is 0 (the
  default), all classes match.  IF \a objName is 0 (the default), all
  object names match.

  If \a regexpMatch is TRUE (the default), \a objName is a regexp that
  the objects's names must match.  If \a regexpMatch is FALSE, \a
  objName is a string and object names must match it exactly.

  Note that \a ineritsClass uses single inheritance from QObject, the
  way inherits() does.  According to inherits(), QMenuBar inherits
  QWidget but not QMenuData. This does not quite match reality, but is
  the best that can be done on the wide variety of compilers Qt
  supports.

  Finally, if \a recursiveSearch is TRUE (the default), queryList()
  searches nth-generation as well as first-generation children.

  If all this seems a bit complex for your needs, the simpler function
  child() may be what you want.

  This somewhat contrived example disables all the buttons in this
  window:
  \code
    QObjectList * l = topLevelWidget()->queryList( "QButton" );
    QObjectListIt it( *l );		// iterate over the buttons
    QObject * obj;
    while ( (obj=it.current()) != 0 ) {	// for each found object...
	++it;
	((QButton*)obj)->setEnabled( FALSE );
    }
    delete l;				// delete the list, not the objects
  \endcode

  \warning Delete the list away as soon you have finished using it.
  The list contains pointers that may become invalid at almost any
  time without notice - as soon as the user closes a window you may
  have dangling pointers, for example.

  \sa child() children(), parent(), inherits(), name(), QRegExp
*/

QObjectList *QObject::queryList( const char *inheritsClass,
				 const char *objName,
				 bool regexpMatch,
				 bool recursiveSearch )
{
    QObjectList *list = new QObjectList;
    CHECK_PTR( list );
    bool onlyWidgets = (inheritsClass && qstrcmp( inheritsClass, "QWidget" ) == 0 );
    if ( regexpMatch && objName ) {		// regexp matching
	QRegExp rx(QString::fromLatin1(objName));
	objSearch( list, (QObjectList *)children(), inheritsClass, onlyWidgets,
		   0, &rx, recursiveSearch );
    } else {
	objSearch( list, (QObjectList *)children(), inheritsClass, onlyWidgets,
		   objName, 0, recursiveSearch );
    }
    return list;
}

/*!
  Returns a list of objects/slot pairs that are connected to the
  signal, or 0 if nothing is connected to it.

  This function is for internal use.
*/

QConnectionList *QObject::receivers( const char *signal ) const
{
    if ( connections && signal ) {
	if ( *signal == '2' ) {			// tag == 2, i.e. signal
	    QCString s = qt_rmWS( signal+1 );
	    return connections->find( (const char*)s );
	} else {
	    return connections->find( signal );
	}
    }
    return 0;
}


/*!
  Inserts an object \e obj into the list of child objects.

  \warning This function cannot be used to make a widget a child
  widget of another.  Child widgets can be created only by setting the
  parent widget in the constructor or by calling QWidget::reparent().

  \sa removeChild(), QWidget::reparent()
*/

void QObject::insertChild( QObject *obj )
{
    if ( obj->isTree ) {
	remove_tree( obj );
	obj->isTree = FALSE;
    }
    if ( obj->parentObj && obj->parentObj != this ) {
#if defined(CHECK_STATE)
	if ( obj->parentObj != this && obj->isWidgetType() )
	    qWarning( "QObject::insertChild: Cannot reparent a widget, "
		     "use QWidget::reparent() instead" );
#endif
	obj->parentObj->removeChild( obj );
    }

    if ( !childObjects ) {
	childObjects = new QObjectList;
	CHECK_PTR( childObjects );
    }
#if defined(CHECK_STATE)
    else if ( childObjects->findRef(obj) >= 0 ) {
	qWarning( "QObject::insertChild: Object %s::%s already in list",
		 obj->className(), obj->name( "unnamed" ) );
	return;
    }
#endif
    obj->parentObj = this;
    childObjects->append( obj );

    obj->pendEvent = TRUE;
#ifdef __ATHEOS__
//    printf( "Warning: QObject::insertChild() QApplication::postEvent() not called\n" );
#else
    QChildEvent *e = new QChildEvent( QEvent::ChildInserted, obj );
    QApplication::postEvent( this, e );
#endif
}

/*!
  Removes the child object \e obj from the list of children.

  \warning
  This function will not remove a child widget from the screen.
  It will only remove it from the parent widget's list of children.

  \sa insertChild(), QWidget::reparent()
*/

void QObject::removeChild( QObject *obj )
{
    if ( childObjects && childObjects->removeRef(obj) ) {
	obj->parentObj = 0;
	if ( !obj->wasDeleted ) {
	    insert_tree( obj );			// it's a root object now
	    obj->isTree = TRUE;
	}
	if ( childObjects->isEmpty() ) {
	    delete childObjects;		// last child removed
	    childObjects = 0;			// reset children list
	}

	// remove events must be sent, not posted!!!
#ifdef __ATHEOS__
//    printf( "Warning: QObject::removeChild() QApplication::postEvent() not called\n" );
#else
	QChildEvent ce( QEvent::ChildRemoved, obj );
	QApplication::sendEvent( this, &ce );
#endif
    }
}


/*!
  Installs an event filter \e obj for this object.

  An event filter is an object that receives all events that are sent to
  this object.	The filter can either stop the event or forward it to this
  object.  The event filter \e obj receives events via its eventFilter()
  function.  The eventFilter() function must return TRUE if the event
  should be stopped, or FALSE if the event should be dispatched normally.

  If multiple event filters are installed for a single object, the
  filter that was installed last is activated first.

  Example:
  \code
    #include <qwidget.h>

    class MyWidget : public QWidget
    {
    public:
	MyWidget::MyWidget( QWidget *parent=0, const char *name=0 );
    protected:
	bool  eventFilter( QObject *, QEvent * );
    };

    MyWidget::MyWidget( QWidget *parent, const char *name )
	: QWidget( parent, name )
    {
	if ( parent )				// has a parent widget
	    parent->installEventFilter( this ); // then install filter
    }

    bool MyWidget::eventFilter( QObject *o, QEvent *e )
    {
	if ( e->type() == QEvent::KeyPress ) {	// key press
	    QKeyEvent *k = (QKeyEvent*)e;
	    qDebug( "Ate key press %d", k->key() );
	    return TRUE;			// eat event
	}
	return QWidget::eventFilter( o, e );	// standard event processing
    }
  \endcode

  The QAccel class, for example, uses this technique.

  \warning
  If you delete the receiver object in your eventFilter() function, be
  sure to return TRUE. If you return FALSE, Qt sends the event to the
  deleted object and the program will crash.

  \sa removeEventFilter(), eventFilter(), event()
*/

void QObject::installEventFilter( const QObject *obj )
{
    if ( !obj )
	return;
    if ( eventFilters ) {
	int c = eventFilters->findRef( obj );
	if ( c >= 0 )
	    eventFilters->take( c );
	disconnect( obj, SIGNAL(destroyed()),
		    this, SLOT(cleanupEventFilter()) );
    } else {
	eventFilters = new QObjectList;
	CHECK_PTR( eventFilters );
    }
    eventFilters->insert( 0, obj );
    connect( obj, SIGNAL(destroyed()), this, SLOT(cleanupEventFilter()) );
}

/*!
  Removes an event filter object \e obj from this object.
  The request is ignored if such an event filter has not been installed.

  All event filters for this object are automatically removed when this
  object is destroyed.

  It is always safe to remove an event filter, even during event filter
  activation (i.e. from the eventFilter() function).

  \sa installEventFilter(), eventFilter(), event()
*/

void QObject::removeEventFilter( const QObject *obj )
{
    if ( eventFilters && eventFilters->removeRef(obj) ) {
	if ( eventFilters->isEmpty() ) {	// last event filter removed
	    delete eventFilters;
	    eventFilters = 0;			// reset event filter list
	}
	disconnect( obj,  SIGNAL(destroyed()),
		    this, SLOT(cleanupEventFilter()) );
    }
}


/*****************************************************************************
  Signal connection management
 *****************************************************************************/

#if defined(CHECK_RANGE)

static bool check_signal_macro( const QObject *sender, const char *signal,
				const char *func, const char *op )
{
    int sigcode = (int)(*signal) - '0';
    if ( sigcode != SIGNAL_CODE ) {
	if ( sigcode == SLOT_CODE )
	    qWarning( "QObject::%s: Attempt to %s non-signal %s::%s",
		     func, op, sender->className(), signal+1 );
	else
	    qWarning( "QObject::%s: Use the SIGNAL macro to %s %s::%s",
		     func, op, sender->className(), signal );
	return FALSE;
    }
    return TRUE;
}

static bool check_member_code( int code, const QObject *object,
			       const char *member, const char *func )
{
    if ( code != SLOT_CODE && code != SIGNAL_CODE ) {
	qWarning( "QObject::%s: Use the SLOT or SIGNAL macro to "
		 "%s %s::%s", func, func, object->className(), member );
	return FALSE;
    }
    return TRUE;
}

static void err_member_notfound( int code, const QObject *object,
				 const char *member, const char *func )
{
    const char *type = 0;
    switch ( code ) {
	case SLOT_CODE:	  type = "slot";   break;
	case SIGNAL_CODE: type = "signal"; break;
    }
    if ( strchr(member,')') == 0 )		// common typing mistake
	qWarning( "QObject::%s: Parentheses expected, %s %s::%s",
		 func, type, object->className(), member );
    else
	qWarning( "QObject::%s: No such %s %s::%s",
		 func, type, object->className(), member );
}


static void err_info_about_objects( const char * func,
				    const QObject * sender,
				    const QObject * receiver )
{
    const char * a = sender->name(), * b = receiver->name();
    if ( a )
	qWarning( "QObject::%s:  (sender name:   '%s')", func, a );
    if ( b )
	qWarning( "QObject::%s:  (receiver name: '%s')", func, b );
}

static void err_info_about_candidates( int code,
				       const QMetaObject* mo,
				       const char* member,
				       const char *func	)
{
    if ( strstr(member,"const char*") ) {
	// porting help
	QCString newname = member;
	int p;
	while ( (p=newname.find("const char*")) >= 0 ) {
	    newname.replace(p, 11, "const QString&");
	}
	QMetaData *rm = 0;
	switch ( code ) {
	    case SLOT_CODE:   rm = mo->slot( newname, TRUE );	  break;
	    case SIGNAL_CODE: rm = mo->signal( newname, TRUE ); break;
	}
	if ( rm ) {
	    qWarning("QObject::%s:  Candidate: %s", func, newname.data());
	}
    }
}


#endif // CHECK_RANGE


/*!
  \fn const QObject *QObject::sender()

  Returns a pointer to the object that sent the signal, if called in a
  slot before any function call or signal emission.  Returns an
  undefined value in all other cases.

  \warning This function will return something apparently correct in
  other cases as well.  However, its value may change during any function
  call, depending on what signal-slot connections are activated during
  that call.  In Qt 3.0 the value will change more often than in 2.x.

  \warning
  This function violates the object-oriented principle of modularity,
  However, getting access to the sender might be practical when many
  signals are connected to a single slot. The sender is undefined if
  the slot is called as a normal C++ function.
*/

/*!
  \fn void QObject::connectNotify( const char *signal )

  This virtual function is called when something has been connected to
  \e signal in this object.

  \warning
  This function violates the object-oriented principle of modularity.
  However, it might be useful when you need to perform expensive
  initialization only if something is connected to a signal.

  \sa connect(), disconnectNotify()
*/

void QObject::connectNotify( const char * )
{
}

/*!
  \fn void QObject::disconnectNotify( const char *signal )

  This virtual function is called when something has been disconnected from
  \e signal in this object.

  \warning
  This function violates the object-oriented principle of modularity.
  However, it might be useful for optimizing access to expensive resources.

  \sa disconnect(), connectNotify()
*/

void QObject::disconnectNotify( const char * )
{
}


/*!
  \fn bool QObject::checkConnectArgs( const char *signal, const QObject *receiver, const char *member )

  Returns TRUE if the \e signal and the \e member arguments are compatible,
  otherwise FALSE.

  \warning
  We recommend that you do not reimplement this function but use the default
  implementation.

  \internal
  TRUE:	 "signal(<anything>)",	"member()"
  TRUE:	 "signal(a,b,c)",	"member(a,b,c)"
  TRUE:	 "signal(a,b,c)",	"member(a,b)", "member(a)" etc.
  FALSE: "signal(const a)",	"member(a)"
  FALSE: "signal(a)",		"member(const a)"
  FALSE: "signal(a)",		"member(b)"
  FALSE: "signal(a)",		"member(a,b)"
*/

bool QObject::checkConnectArgs( const char    *signal,
				const QObject *,
				const char    *member )
{
    const char *s1 = signal;
    const char *s2 = member;
    while ( *s1++ != '(' ) { }			// scan to first '('
    while ( *s2++ != '(' ) { }
    if ( *s2 == ')' || qstrcmp(s1,s2) == 0 )	// member has no args or
	return TRUE;				//   exact match
    int s1len = qstrlen(s1);
    int s2len = qstrlen(s2);
    if ( s2len < s1len && qstrncmp(s1,s2,s2len-1)==0 && s1[s2len-1]==',' )
	return TRUE;				// member has less args
    return FALSE;
}

/*!
  Normlizes the signal or slot definition \a signalSlot by removing
  unnecessary whitespaces.
*/

QCString QObject::normalizeSignalSlot( const char *signalSlot )
{
    return  qt_rmWS( signalSlot );
}


/* tmake ignore Q_OBJECT */
/* tmake ignore Q_OBJECT */

/*!
  Internal function, called from initMetaObject(). Used to emit a warning
  when a class containing the macro Q_OBJECT inherits from a class that
  does not contain it.
*/

void QObject::badSuperclassWarning( const char *className,
				    const char *superclassName )
{
#if defined(CHECK_NULL)
    qWarning(
    "%s::initMetaObject(): Warning:\n"
    "    The class \"%s\" contains the Q_OBJECT macro, but inherits from the\n"
    "    \"%s\" class, which does not contain the Q_OBJECT macro.\n"
    "    Signal/slot behavior is undefined.\n",
    className, className,
    superclassName );
#else
    Q_UNUSED( className )
    Q_UNUSED( superclassName )
#endif
}

/*!
  \overload bool QObject::connect( const QObject *sender, const char *signal, const char *member ) const

  Connects \e signal from the \e sender object to \e member in this object.

  Equivalent to: <code>QObject::connect(sender, signal, this, member)</code>.

  \sa disconnect()
*/

/*!
  Connects \e signal from the \e sender object to \e member in object \e
  receiver.

  You must use the SIGNAL() and SLOT() macros when specifying the \e signal
  and the \e member.

  Example:
  \code
    QLabel     *label  = new QLabel;
    QScrollBar *scroll = new QScrollBar;
    QObject::connect( scroll, SIGNAL(valueChanged(int)),
		      label,  SLOT(setNum(int)) );
  \endcode

  This example connects the scroll bar's \link QScrollBar::valueChanged()
  valueChanged()\endlink signal to the label's \link QLabel::setNum()
  setNum()\endlink slot. It makes the label always display the current
  scroll bar value.

  A signal can even be connected to another signal, i.e. \e member is
  a SIGNAL().

  \code
    class MyWidget : public QWidget
    {
    public:
	MyWidget();
    ...
    signals:
	void aSignal();
    ...
    private:
    ...
	QPushButton *aButton;
    };

    MyWidget::MyWidget()
    {
	aButton = new QPushButton( this );
	connect( aButton, SIGNAL(clicked()), SIGNAL(aSignal()) );
    }
  \endcode

  In its constructor, MyWidget creates a private button and connects the
  \link QButton::clicked() clicked()\endlink signal to relay clicked() to
  the outside world. You can achieve the same effect by connecting the
  clicked() signal to a private slot and emitting aSignal() in this slot,
  but that takes a few lines of extra code and is not quite as clear, of
  course.

  A signal can be connected to many slots/signals. Many signals can be
  connected to one slot.

  If a signal is connected to several slots, the slots are activated
  in arbitrary order when the signal is emitted.

  The function returns TRUE if it successfully connects the signal to
  the slot.  It will return FALSE when it cannot connect the signal to
  the slot.

  \sa disconnect()
*/

bool QObject::connect( const QObject *sender,	const char *signal,
		       const QObject *receiver, const char *member )
{
#if defined(CHECK_NULL)
    if ( sender == 0 || receiver == 0 || signal == 0 || member == 0 ) {
	qWarning( "QObject::connect: Cannot connect %s::%s to %s::%s",
		 sender ? sender->className() : "(null)",
		 signal ? signal+1 : "(null)",
		 receiver ? receiver->className() : "(null)",
		 member ? member+1 : "(null)" );
	return FALSE;
    }
#endif
    QCString signal_name = qt_rmWS( signal );	// white space stripped
    QCString member_name = qt_rmWS( member );
    signal = signal_name;
    member = member_name;

    QMetaObject *smeta = sender->queryMetaObject();
    if ( !smeta ) {				// no meta object
	printf( "QObject::connect() No smeta object\n" );
	return FALSE;
    }
#if defined(CHECK_RANGE)
    if ( !check_signal_macro( sender, signal, "connect", "bind" ) ) {
	printf( "QObject::connect() check_signal_macro() failed\n" );
	return FALSE;
    }
#endif
    signal++;					// skip member type code
    QMetaData *sm;
    if ( !(sm=smeta->signal(signal,TRUE)) ) {	// no such signal
#if defined(CHECK_RANGE)
	err_member_notfound( SIGNAL_CODE, sender, signal, "connect" );
	err_info_about_candidates( SIGNAL_CODE, smeta, signal, "connect" );
	err_info_about_objects( "connect", sender, receiver );
#endif
	printf( "QObject::connect() No such signal (%s)\n", signal );
	return FALSE;
    }
    signal = sm->name;				// use name from meta object

    int membcode = member[0] - '0';		// get member code

    QObject *s = (QObject *)sender;		// we need to change them
    QObject *r = (QObject *)receiver;		//   internally

#if defined(CHECK_RANGE)
    if ( !check_member_code( membcode, r, member, "connect" ) ) {
	printf( "QObject::connect() No member code\n" );
	return FALSE;
    }
#endif
    member++;					// skip code
    QMetaData   *rm = 0;
    QMetaObject *rmeta = r->queryMetaObject();
    if ( !rmeta ) {				// no meta object
	printf( "QObject::connect() No rmeta object\n" );
	return FALSE;
    }
    switch ( membcode ) {			// get receiver member
	case SLOT_CODE:	  rm = rmeta->slot( member, TRUE );   break;
	case SIGNAL_CODE: rm = rmeta->signal( member, TRUE ); break;
    }
    if ( !rm ) {				// no such member
#if defined(CHECK_RANGE)
	err_member_notfound( membcode, r, member, "connect" );
	err_info_about_candidates( membcode, rmeta, member, "connect" );
	err_info_about_objects( "connect", sender, receiver );
#endif
	printf( "QObject::connect() No rm object\n" );
	return FALSE;
    }
#if defined(CHECK_RANGE)
    if ( !s->checkConnectArgs(signal,receiver,member) )
	qWarning( "QObject::connect: Incompatible sender/receiver arguments"
		 "\n\t%s::%s --> %s::%s",
		 s->className(), signal,
		 r->className(), member );
#endif
    if ( !s->connections ) {			// create connections dict
	s->connections = new QSignalDict( 7, TRUE, FALSE );
	CHECK_PTR( s->connections );
	s->connections->setAutoDelete( TRUE );
    }
    QConnectionList *clist = s->connections->find( signal );
    if ( !clist ) {				// create receiver list
	clist = new QConnectionList;
	CHECK_PTR( clist );
	clist->setAutoDelete( TRUE );
	s->connections->insert( signal, clist );
    }
    QConnection *c = new QConnection(r, rm->ptr, rm->name);
    CHECK_PTR( c );
    clist->append( c );
    if ( !r->senderObjects ) {			// create list of senders
	r->senderObjects = new QObjectList;
	CHECK_PTR( r->senderObjects );
    }
    r->senderObjects->append( s );		// add sender to list
    s->connectNotify( signal_name );
    return TRUE;
}


/*!
  \overload bool QObject::disconnect( const char *signal, const QObject *receiver, const char *member )

  Disconnects \e signal from \e member of \e receiver.

  A signal-slot connection is removed when either of the objects
  involved are destroyed.
*/

/*!
  \overload bool QObject::disconnect( const QObject *receiver, const char *member )

  Disconnects all signals in this object from \e member of \e receiver.

  A signal-slot connection is removed when either of the objects
  involved are destroyed.
*/

/*!
  Disconnects \e signal in object \e sender from \e member in object \e
  receiver.

  A signal-slot connection is removed when either of the objects
  involved are destroyed.

  disconnect() is typically used in three ways, as the following examples
  show.
  <ol>
  <li> Disconnect everything connected to an object's signals:
  \code
    disconnect( myObject, 0, 0, 0 );
  \endcode
  equivalent to the non-static overloaded function
  \code
    myObject->disconnect();
  \endcode
  <li> Disconnect everything connected to a specific signal:
  \code
    disconnect( myObject, SIGNAL(mySignal()), 0, 0 );
  \endcode
  equivalent to the non-static overloaded function
  \code
    myObject->disconnect( SIGNAL(mySignal()) );
  \endcode
  <li> Disconnect a specific receiver.
  \code
    disconnect( myObject, 0, myReceiver, 0 );
  \endcode
  equivalent to the non-static overloaded function
  \code
    myObject->disconnect(  myReceiver );
  \endcode
  </ol>

  0 may be used as a wildcard, meaning "any signal", "any receiving
  object" or "any slot in the receiving object" respectively.

  The \e sender may never be 0.  (You cannot disconnect signals from
  more than one object.)

  If \e signal is 0, it disconnects \e receiver and \e member from any
  signal.  If not, only the specified signal is disconnected.

  If \e receiver is 0, it disconnects anything connected to \e signal.
  If not, slots in objects other than \e receiver are not disconnected.

  If \e member is 0, it disconnects anything that is connected to \e
  receiver.  If not, only slots named \e member will be disconnected,
  and all other slots are left alone.  The \e member must be 0 if \e
  receiver is left out, so you cannot disconnect a specifically-named
  slot on all objects.

  \sa connect()
*/

bool QObject::disconnect( const QObject *sender,   const char *signal,
			  const QObject *receiver, const char *member )
{
#if defined(CHECK_NULL)
    if ( sender == 0 || (receiver == 0 && member != 0) ) {
	qWarning( "QObject::disconnect: Unexpected null parameter" );
	return FALSE;
    }
#endif
    if ( !sender->connections )			// no connected signals
	return FALSE;
    QCString signal_name;
    QCString member_name;
    QMetaData *rm = 0;
    QObject *s = (QObject *)sender;
    QObject *r = (QObject *)receiver;
    if ( member ) {
	member_name = qt_rmWS( member );
	member = member_name.data();
	int membcode = member[0] - '0';
#if defined(CHECK_RANGE)
	if ( !check_member_code( membcode, r, member, "disconnect" ) )
	    return FALSE;
#endif
	member++;
	QMetaObject *rmeta = r->queryMetaObject();
	if ( !rmeta )				// no meta object
	    return FALSE;
	switch ( membcode ) {			// get receiver member
	    case SLOT_CODE:   rm = rmeta->slot( member, TRUE );	  break;
	    case SIGNAL_CODE: rm = rmeta->signal( member, TRUE ); break;
	}
	if ( !rm ) {				// no such member
#if defined(CHECK_RANGE)
	    err_member_notfound( membcode, r, member, "disconnect" );
	    err_info_about_candidates( membcode, rmeta, member, "connect" );
	    err_info_about_objects( "disconnect", sender, receiver );
#endif
	    return FALSE;
	}
    }

    QConnectionList *clist;
    register QConnection *c;
    if ( signal == 0 ) {			// any/all signals
	QSignalDictIt it(*(s->connections));
	while ( (clist=it.current()) ) {	// for all signals...
	    // Tricky hack to avoid UTF conversion.
	    const char *curkey = it.currentKey();
	    ++it;
	    c = clist->first();
	    while ( c ) {			// for all receivers...
		if ( r == 0 ) {			// remove all receivers
		    removeObjFromList( c->object()->senderObjects, s );
		    c = clist->next();
		} else if ( r == c->object() &&
			    (member == 0 ||
			     qstrcmp(member,c->memberName()) == 0) ) {
		    removeObjFromList( c->object()->senderObjects, s );
		    clist->remove();
		    c = clist->current();
		} else {
		    c = clist->next();
		}
	    }
	    if ( r == 0 )			// disconnect all receivers
		s->connections->remove( curkey );
	}
	s->disconnectNotify( 0 );
    }

    else {					// specific signal
	signal_name = qt_rmWS( signal );
	signal = signal_name.data();
#if defined(CHECK_RANGE)
	if ( !check_signal_macro( s, signal, "disconnect", "unbind" ) )
	    return FALSE;
#endif
	signal++;
	clist = s->connections->find( signal );
	if ( !clist ) {
#if defined(CHECK_RANGE)
	    QMetaObject *smeta = s->queryMetaObject();
	    if ( !smeta )			// no meta object
		return FALSE;
	    if ( !smeta->signal(signal,TRUE) )
		qWarning( "QObject::disconnect: No such signal %s::%s",
			 s->className(), signal );
#endif
	    return FALSE;
	}
	c = clist->first();
	while ( c ) {				// for all receivers...
	    if ( r == 0 ) {			// remove all receivers
		removeObjFromList( c->object()->senderObjects, s, TRUE );
		c = clist->next();
	    } else if ( r == c->object() && (member == 0 ||
				      qstrcmp(member,c->memberName()) == 0) ) {
		removeObjFromList( c->object()->senderObjects, s, TRUE );
		clist->remove();
		c = clist->current();
	    } else {
		c = clist->next();
	    }
	}
	if ( r == 0 )				// disconnect all receivers
	    s->connections->remove( signal );
	s->disconnectNotify( signal_name );
    }
    return TRUE;
}


/*!
  This signal is emitted immediately before the object is destroyed.

  All the objects's children are destroyed immediately after this signal
  is emitted.
*/

void QObject::destroyed()
{
    activate_signal( "destroyed()" );
}


/*!
  This slot is connected to the destroyed() signal of other objects
  that have installed event filters on this object. When the other
  object is destroyed, we want to remove its event filter.
*/

void QObject::cleanupEventFilter()
{
    removeEventFilter( sender() );
}


/*!
  \internal
  Returns the meta object for this object. If necessary, calls
  initMetaObject().
  \sa metaObject()
*/

QMetaObject *QObject::queryMetaObject() const
{
    register QObject *x = (QObject *)this;	// fake const
    QMetaObject *m = x->metaObject();
    if ( !m ) {					// not meta object
	x->initMetaObject();			//   then try to create it
	m = x->metaObject();
    }
#if defined(CHECK_NULL)
    if ( !m )					// still no meta object: error
	qWarning( "QObject: Object %s::%s has no meta object",
		 x->className(), x->name( "unnamed" ) );
#endif
    return m;
}

#ifndef QT_NO_TRANSLATION // Otherwise we have a simple inline version
/*! \overload

  Returns a translated version of \a text in context QObject and with
  no comment, or \a text if there is no appropriate translated
  version.  All QObject subclasses which use the Q_OBJECT macro have a
  reimplementation of this function which uses the relevant class name
  as context.

  \sa QApplication::translate()
*/

QString QObject::tr( const char *text )
{
    if ( qApp )
	return qApp->translate( "QObject", text, 0 );
    else
	return QString::fromLatin1(text);
}

/*!
  Returns a translated version of \a text in context QObject and and
  with \a comment, or \a text if there is no appropriate translated
  version.  All QObject subclasses which use the Q_OBJECT macro have a
  reimplementation of this function which uses the relevant class name
  as context.

  \sa QApplication::translate()
*/

QString QObject::tr( const char *text, const char * comment )
{
    if ( qApp )
	return qApp->translate( "QObject", text, comment );
    else
	return QString::fromLatin1(text);
}

#endif

/*!
  Initializes the \link metaObject() meta object\endlink of this
  object. This method is automatically executed on demand.
  \sa metaObject()
*/
void QObject::initMetaObject()
{
    staticMetaObject();
}

/*!
  The functionality of initMetaObject(), provided as a static function.
*/
QMetaObject* QObject::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    typedef void(QObject::*m1_t0)();
    m1_t0 v1_0 = &QObject::cleanupEventFilter;
    QMetaData *slot_tbl = new QMetaData[1];
    slot_tbl[0].name = "cleanupEventFilter()";
    slot_tbl[0].ptr = (QMember)v1_0;

    //### remove 3.0, replace with slot_tbl[0].access = QMetaData::Private;
    QMetaData::Access *slot_tbl_access = new QMetaData::Access[1];
    slot_tbl_access[0] = QMetaData::Private;

    typedef void(QObject::*m2_t0)();
    m2_t0 v2_0 = &QObject::destroyed;
    QMetaData *signal_tbl = new QMetaData[1];
    signal_tbl[0].name = "destroyed()";
    signal_tbl[0].ptr = (QMember)v2_0;
#ifndef QT_NO_PROPERTIES
    QMetaProperty *props_tbl = new QMetaProperty[1];
    typedef const char*(QObject::*m3_t0)()const;
    typedef void(QObject::*m3_t1)(const char*);
    m3_t0 v3_0 = &QObject::name;
    m3_t1 v3_1 = &QObject::setName;
    props_tbl[0].n = "name";
    props_tbl[0].get = (QMember)v3_0;
    props_tbl[0].set = (QMember)v3_1;
    props_tbl[0].t = "QCString";
    props_tbl[0].enumData = 0;
    props_tbl[0].gspec = QMetaProperty::ConstCharStar;
    props_tbl[0].sspec = QMetaProperty::ConstCharStar;
    props_tbl[0].setFlags(QMetaProperty::StdSet);
    QMetaEnum* enum_tbl = QMetaObject::new_metaenum( 3 );
    enum_tbl[0].name = "Alignment";
    enum_tbl[0].count = 8;
    enum_tbl[0].set = TRUE;
    enum_tbl[0].items = QMetaObject::new_metaenum_item( 8 );
    enum_tbl[0].items[0].key = "AlignLeft";
    enum_tbl[0].items[0].value = (int) Qt::AlignLeft;
    enum_tbl[0].items[1].key = "AlignRight";
    enum_tbl[0].items[1].value = (int) Qt::AlignRight;
    enum_tbl[0].items[2].key = "AlignHCenter";
    enum_tbl[0].items[2].value = (int) Qt::AlignHCenter;
    enum_tbl[0].items[3].key = "AlignTop";
    enum_tbl[0].items[3].value = (int) Qt::AlignTop;
    enum_tbl[0].items[4].key = "AlignBottom";
    enum_tbl[0].items[4].value = (int) Qt::AlignBottom;
    enum_tbl[0].items[5].key = "AlignVCenter";
    enum_tbl[0].items[5].value = (int) Qt::AlignVCenter;
    enum_tbl[0].items[6].key = "AlignCenter";
    enum_tbl[0].items[6].value = (int) Qt::AlignCenter;
    enum_tbl[0].items[7].key = "WordBreak";
    enum_tbl[0].items[7].value = (int) Qt::WordBreak;
    enum_tbl[1].name = "Orientation";
    enum_tbl[1].count = 2;
    enum_tbl[1].set = FALSE;
    enum_tbl[1].items = QMetaObject::new_metaenum_item( 2 );
    enum_tbl[1].items[0].key = "Horizontal";
    enum_tbl[1].items[0].value = (int) Qt::Horizontal;
    enum_tbl[1].items[1].key = "Vertical";
    enum_tbl[1].items[1].value = (int) Qt::Vertical;
    enum_tbl[2].name = "TextFormat";
    enum_tbl[2].count = 3;
    enum_tbl[2].set = FALSE;
    enum_tbl[2].items = QMetaObject::new_metaenum_item( 3 );
    enum_tbl[2].items[0].key = "PlainText";
    enum_tbl[2].items[0].value = (int) Qt::PlainText;
    enum_tbl[2].items[1].key = "RichText";
    enum_tbl[2].items[1].value = (int) Qt::RichText;
    enum_tbl[2].items[2].key = "AutoText";
    enum_tbl[2].items[2].value = (int) Qt::AutoText;
#endif
    metaObj = new QMetaObject( "QObject", "",
	slot_tbl, 1,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	props_tbl, 1,
	enum_tbl, 3,
#endif
        0, 0 );
    metaObj->set_slot_access( slot_tbl_access ); // ### remove 3.0
    return metaObj;
}

/*!
  \internal

  Signal activation with the most frequently used parameter/argument
  types.  All other combinations are generated by the meta object
  compiler.
*/
void QObject::activate_signal( const char *signal )
{
    if ( !connections )
	return;
    QConnectionList *clist = connections->find( signal );
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT)();
    RT r;
    QConnectionListIt it(*clist);
    register QConnection *c;
    register QObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = c->object();
	object->sigSender = this;
	r = (RT)*(c->member());
	(object->*r)();
    }
}

/*!
  \overload void QObject::activate_signal( const char *signal, short )
*/

/*!
  \overload void QObject::activate_signal( const char *signal, int )
*/

/*!
  \overload void QObject::activate_signal( const char *signal, long )
*/

/*!
  \overload void QObject::activate_signal( const char *signal, const char * )
*/


#ifdef Q_FP_CCAST_BROKEN
#define ACTIVATE_SIGNAL_WITH_PARAM(FNAME,TYPE)				      \
void QObject::FNAME( const char *signal, TYPE param )			      \
{									      \
    if ( !connections )							      \
        return;								      \
    QConnectionList *clist = connections->find( signal );		      \
    if ( !clist || signalsBlocked() )					      \
        return;								      \
    typedef void (QObject::*RT0)();					      \
    typedef void (QObject::*RT1)( TYPE );				      \
    RT0 r0;								      \
    RT1 r1;								      \
    QConnectionListIt it(*clist);					      \
    register QConnection *c;						      \
    register QObject *object;						      \
    while ( (c=it.current()) ) {					      \
        ++it;								      \
        object = c->object();						      \
        object->sigSender = this;					      \
        if ( c->numArgs() ) {						      \
            r1 = reinterpret_cast<RT1>(*(c->member()));			      \
            (object->*r1)( param );					      \
        } else {							      \
            r0 = reinterpret_cast<RT0>(*(c->member()));			      \
            (object->*r0)();						      \
        }								      \
    }									      \
}
#else
#define ACTIVATE_SIGNAL_WITH_PARAM(FNAME,TYPE)				      \
void QObject::FNAME( const char *signal, TYPE param )			      \
{									      \
    if ( !connections )							      \
	return;								      \
    QConnectionList *clist = connections->find( signal );		      \
    if ( !clist || signalsBlocked() )					      \
	return;								      \
    typedef void (QObject::*RT0)();					      \
    typedef void (QObject::*RT1)( TYPE );				      \
    RT0 r0;								      \
    RT1 r1;								      \
    QConnectionListIt it(*clist);					      \
    register QConnection *c;						      \
    register QObject *object;						      \
    while ( (c=it.current()) ) {					      \
	++it;								      \
	object = c->object();						      \
	object->sigSender = this;					      \
	if ( c->numArgs() ) {						      \
	    r1 = (RT1)*(c->member());					      \
	    (object->*r1)( param );					      \
	} else {							      \
	    r0 = (RT0)*(c->member());					      \
	    (object->*r0)();						      \
	}								      \
    }									      \
}
#endif

// We don't want to duplicate too much text so...

ACTIVATE_SIGNAL_WITH_PARAM( activate_signal, short )
ACTIVATE_SIGNAL_WITH_PARAM( activate_signal, int )
ACTIVATE_SIGNAL_WITH_PARAM( activate_signal, long )
ACTIVATE_SIGNAL_WITH_PARAM( activate_signal, const char * )
ACTIVATE_SIGNAL_WITH_PARAM( activate_signal_bool, bool )
ACTIVATE_SIGNAL_WITH_PARAM( activate_signal_string, QString )
ACTIVATE_SIGNAL_WITH_PARAM( activate_signal_strref, const QString & )

/*!
  \fn void QObject::activate_signal_bool (const char * signal, bool)
  \internal
*/
/*!
  \fn void QObject::activate_signal_string (const char * signal, QString)
  \internal
*/
/*!
  \fn void QObject::activate_signal_strref (const char * signal, const QString & )
  \internal
*/


/*****************************************************************************
  QObject debugging output routines.
 *****************************************************************************/

static void dumpRecursive( int level, QObject *object )
{
#if defined(DEBUG) && !defined(__ATHEOS__)
    if ( object ) {
	QString buf;
	buf.fill( '\t', level/2 );
	if ( level % 2 )
	    buf += "    ";
	const char *name = object->name();
	QString flags="";
	if ( qApp->focusWidget() == object )
	    flags += 'F';
	if ( object->isWidgetType() ) {
	    QWidget * w = (QWidget *)object;
	    if ( w->isVisible() ) {
		QString t;
		t.sprintf( "<%d,%d,%d,%d>",
			   w->x(), w->y(), w->width(), w->height() );
		flags += t;
	    } else {
		flags += 'I';
	    }
	}
	qDebug( "%s%s::%s %s", (const char*)buf, object->className(), name,
	    flags.latin1() );
	if ( object->children() ) {
	    QObjectListIt it(*object->children());
	    QObject * c;
	    while ( (c=it.current()) != 0 ) {
		++it;
		dumpRecursive( level+1, c );
	    }
	}
    }
#else
    Q_UNUSED( level )
    Q_UNUSED( object )
#endif
}

/*!
  Dumps a tree of children to the debug output.

  This function is useful for debugging. This function does nothing if
  the library has been compiled in release mode (i.e without debugging
  information).

*/

void QObject::dumpObjectTree()
{
    dumpRecursive( 0, this );
}

/*!
  Dumps information about signal connections etc. for this object to the
  debug output.

  This function is useful for debugging. This function does nothing if
  the library has been compiled in release mode (i.e without debugging
  information).
*/

void QObject::dumpObjectInfo()
{
#if defined(DEBUG)
    qDebug( "OBJECT %s::%s", className(), name( "unnamed" ) );
    qDebug( "  SIGNALS OUT" );
    int n = 0;
    if ( connections ) {
	QSignalDictIt it(*connections);
	QConnectionList *clist;
	while ( (clist=it.current()) ) {
	    qDebug( "\t%s", it.currentKey() );
	    n++;
	    ++it;
	    register QConnection *c;
	    QConnectionListIt cit(*clist);
	    while ( (c=cit.current()) ) {
		++cit;
		qDebug( "\t  --> %s::%s %s", c->object()->className(),
		       c->object()->name( "unnamed" ), c->memberName() );
	    }
	}
    }
    if ( n == 0 )
	qDebug( "\t<None>" );
    qDebug( "  SIGNALS IN" );
    n = 0;
    if ( senderObjects ) {
	QObject *sender = senderObjects->first();
	while ( sender ) {
	    qDebug( "\t%s::%s",
		   sender->className(), sender->name( "unnamed" ) );
	    n++;
	    sender = senderObjects->next();
	}
    }
    if ( n == 0 )
	qDebug( "\t<None>" );
#endif
}

#ifndef QT_NO_PROPERTIES

/*!
  Sets the object's property \a name to \a value.

  Returne TRUE is the operation was successful, FALSE otherwise.

  Information about all available properties are provided through the
  metaObject().

  \sa property(), metaObject(), QMetaObject::propertyNames(), QMetaObject::property()
*/
bool QObject::setProperty( const char *name, const QVariant& value )
{
    if ( !value.isValid() )
	return FALSE;

    typedef void (QObject::*ProtoConstCharStar)( const char* );

    typedef void (QObject::*ProtoString)( QString );
    typedef void (QObject::*RProtoString)( const QString&);

    typedef void (QObject::*ProtoCString)( QCString );
    typedef void (QObject::*RProtoCString)( const QCString&);

    typedef void (QObject::*ProtoInt)( int );
    typedef void (QObject::*RProtoInt)( const int& );

    typedef void (QObject::*ProtoUInt)( uint );
    typedef void (QObject::*RProtoUInt)( const uint& );

    typedef void (QObject::*ProtoDouble)( double );
    typedef void (QObject::*RProtoDouble)( const double& );

    typedef void (QObject::*ProtoBool)( bool );
    typedef void (QObject::*RProtoBool)( const bool& );

    typedef void (QObject::*ProtoFont)( QFont );
    typedef void (QObject::*RProtoFont)( const QFont& );

#ifndef __ATHEOS__
    typedef void (QObject::*ProtoPixmap)( QPixmap );
    typedef void (QObject::*RProtoPixmap)( const QPixmap& );
#endif
    typedef void (QObject::*ProtoBrush)( QBrush );
    typedef void (QObject::*RProtoBrush)( const QBrush& );

    typedef void (QObject::*ProtoRect)( QRect );
    typedef void (QObject::*RProtoRect)( const QRect& );

    typedef void (QObject::*ProtoSize)( QSize );
    typedef void (QObject::*RProtoSize)( const QSize& );

    typedef void (QObject::*ProtoColor)( QColor );
    typedef void (QObject::*RProtoColor)( const QColor& );

    typedef void (QObject::*ProtoPalette)( QPalette );
    typedef void (QObject::*RProtoPalette)( const QPalette& );

    typedef void (QObject::*ProtoColorGroup)( QColorGroup );
    typedef void (QObject::*RProtoColorGroup)( const QColorGroup& );

    typedef void (QObject::*ProtoBitmap)( QBitmap );
    typedef void (QObject::*RProtoBitmap)( const QBitmap& );

    typedef void (QObject::*ProtoRegion)( QRegion );
    typedef void (QObject::*RProtoRegion)( const QRegion& );

    typedef void (QObject::*ProtoPointArray)( QPointArray );
    typedef void (QObject::*RProtoPointArray)( const QPointArray& );

    typedef void (QObject::*ProtoIconSet)( QIconSet );
    typedef void (QObject::*RProtoIconSet)( const QIconSet& );

    typedef void (QObject::*ProtoImage)( QImage );
    typedef void (QObject::*RProtoImage)( const QImage& );

    typedef void (QObject::*ProtoCursor)( QCursor );
    typedef void (QObject::*RProtoCursor)( const QCursor& );

    typedef void (QObject::*ProtoPoint)( QPoint );
    typedef void (QObject::*RProtoPoint)( const QPoint& );

    typedef void (QObject::*ProtoStringList)( QStringList );
    typedef void (QObject::*RProtoStringList)( const QStringList& );

    typedef void (QObject::*ProtoList)( QValueList<QVariant> );
    typedef void (QObject::*RProtoList)( const QValueList<QVariant>& );

    typedef void (QObject::*ProtoMap)( QMap<QString,QVariant> );
    typedef void (QObject::*RProtoMap)( const QMap<QString,QVariant>& );

    typedef void (QObject::*ProtoSizePolicy)( QSizePolicy );
    typedef void (QObject::*RProtoSizePolicy)( const QSizePolicy& );

    QMetaObject* meta = queryMetaObject();
    if ( !meta )
	return FALSE;
    const QMetaProperty* p = meta->property( name, TRUE );
    if ( !p || !p->writable() )
	return FALSE;

    if ( p->isEnumType() ) {
	int v = 0;

	if( value.type() == QVariant::Int ||
	    value.type() == QVariant::UInt ) {
	    v = value.toInt();
	} else if ( value.type() == QVariant::String ||
		    value.type() == QVariant::CString ) {

	    if ( p->isSetType() ) {
		QString s = value.toString();
		// QStrList does not support split, use QStringList for that.
		QStringList l = QStringList::split( '|', s );
		QStrList keys;
		for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it )
		    keys.append( (*it).stripWhiteSpace().latin1() );
		v = p->keysToValue( keys );
	    } else {
		v = p->keyToValue( value.toCString().data() );
	    }
	} else {
	    return FALSE;
	}
	ProtoInt m = (ProtoInt)p->set;
	(this->*m)( v );
	return TRUE;
    }

    QVariant::Type type = QVariant::nameToType( p->type() );
    if ( !value.canCast( type ) )
	return FALSE;

    // Some stupid casts in this switch... for #@$!&@ Sun WorkShop C++ 5.0
    switch ( type ) {

    case QVariant::Invalid:
	return FALSE;

    case QVariant::Image:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoImage m = reinterpret_cast<ProtoImage>(p->set);
#else	    
	    ProtoImage m = (ProtoImage)p->set;
#endif	    
	    (this->*m)( (QImage)(value.toImage()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
	    RProtoImage m = (RProtoImage)p->set;
	    (this->*m)( value.toImage() );
	    return TRUE;
	}
	break;

    case QVariant::Point:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoPoint m = reinterpret_cast<ProtoPoint>(p->set);
#else	    
	    ProtoPoint m = (ProtoPoint)p->set;
#endif	    
	    (this->*m)( value.toPoint() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoPoint m = reinterpret_cast<RProtoPoint>(p->set);
#else	    
	    RProtoPoint m = (RProtoPoint)p->set;
#endif	    
	    (this->*m)( value.toPoint() );
	    return TRUE;
	}
	break;

    case QVariant::StringList:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoStringList m = reinterpret_cast<ProtoStringList>(p->set);
#else	    
	    ProtoStringList m = (ProtoStringList)p->set;
#endif	    
	    (this->*m)( (QStringList)value.toStringList() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoStringList m = reinterpret_cast<RProtoStringList>(p->set);
#else	    
	    RProtoStringList m = (RProtoStringList)p->set;
#endif	    
	    (this->*m)( value.toStringList() );
	    return TRUE;
	}
	break;

    case QVariant::String:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoString m = reinterpret_cast<ProtoString>(p->set);
#else	    
	    ProtoString m = (ProtoString)p->set;
#endif	    
	    (this->*m)( (QString)(value.toString()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoString m = reinterpret_cast<RProtoString>(p->set);
#else	    
	    RProtoString m = (RProtoString)p->set;
#endif	    
	    (this->*m)( value.toString() );
	    return TRUE;
	}
	break;

    case QVariant::CString:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoCString m = reinterpret_cast<ProtoCString>(p->set);
#else	    
	    ProtoCString m = (ProtoCString)p->set;
#endif	    
	    (this->*m)( (QCString)(value.toCString()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoCString m = reinterpret_cast<RProtoCString>(p->set);
#else	    
	    RProtoCString m = (RProtoCString)p->set;
#endif	    
	    (this->*m)( value.toCString() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::ConstCharStar ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoConstCharStar m = reinterpret_cast<ProtoConstCharStar>(p->set);
#else	    
	    ProtoConstCharStar m = (ProtoConstCharStar)p->set;
#endif	    
	    (this->*m)( value.toCString().data() );
	    return TRUE;
	}
	break;

    case QVariant::Font:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoFont m = reinterpret_cast<ProtoFont>(p->set);
#else	    
	    ProtoFont m = (ProtoFont)p->set;
#endif	    
	    (this->*m)( (QFont)(value.toFont()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoFont m = reinterpret_cast<RProtoFont>(p->set);
#else	    
	    RProtoFont m = (RProtoFont)p->set;
#endif	    
	    (this->*m)( value.toFont() );
	    return TRUE;
	}
	break;

#ifndef __ATHEOS__
    case QVariant::Pixmap:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoPixmap m = reinterpret_cast<ProtoPixmap>(p->set);
#else	    
	    ProtoPixmap m = (ProtoPixmap)p->set;
#endif	    
	    (this->*m)( (QPixmap)(value.toPixmap()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoPixmap m = reinterpret_cast<RProtoPixmap>(p->set);
#else	    
	    RProtoPixmap m = (RProtoPixmap)p->set;
#endif	    
	    (this->*m)( value.toPixmap() );
	    return TRUE;
	}
	break;
#endif // __ATHEOS__

    case QVariant::Brush:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoBrush m = reinterpret_cast<ProtoBrush>(p->set);
#else	    
	    ProtoBrush m = (ProtoBrush)p->set;
#endif	    
	    (this->*m)( (QBrush)(value.toBrush()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoBrush m = reinterpret_cast<RProtoBrush>(p->set);
#else	    
	    RProtoBrush m = (RProtoBrush)p->set;
#endif	    
	    (this->*m)( value.toBrush() );
	    return TRUE;
	}
	break;

    case QVariant::Rect:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoRect m = reinterpret_cast<ProtoRect>(p->set);
#else	    
	    ProtoRect m = (ProtoRect)p->set;
#endif	    
	    (this->*m)( value.toRect() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoRect m = reinterpret_cast<RProtoRect>(p->set);
#else	    
	    RProtoRect m = (RProtoRect)p->set;
#endif	    
	    (this->*m)( value.toRect() );
	    return TRUE;
	}
	break;

    case QVariant::Size:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoSize m = reinterpret_cast<ProtoSize>(p->set);
#else	    
	    ProtoSize m = (ProtoSize)p->set;
#endif	    
	    (this->*m)( value.toSize() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoSize m = reinterpret_cast<RProtoSize>(p->set);
#else	    
	    RProtoSize m = (RProtoSize)p->set;
#endif	    
	    (this->*m)( value.toSize() );
	    return TRUE;
	}
	break;

    case QVariant::Color:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoColor m = reinterpret_cast<ProtoColor>(p->set);
#else	    
	    ProtoColor m = (ProtoColor)p->set;
#endif	    
	    (this->*m)( (QColor)(value.toColor()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoColor m = reinterpret_cast<RProtoColor>(p->set);
#else	    
	    RProtoColor m = (RProtoColor)p->set;
#endif	    
	    (this->*m)( value.toColor() );
	    return TRUE;
	}
	break;

    case QVariant::Palette:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoPalette m = reinterpret_cast<ProtoPalette>(p->set);
#else	    
	    ProtoPalette m = (ProtoPalette)p->set;
#endif	    
	    (this->*m)( (QPalette)(value.toPalette()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoPalette m = reinterpret_cast<RProtoPalette>(p->set);
#else	    
	    RProtoPalette m = (RProtoPalette)p->set;
#endif	    
	    (this->*m)( value.toPalette() );
	    return TRUE;
	}
	break;

    case QVariant::ColorGroup:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoColorGroup m = reinterpret_cast<ProtoColorGroup>(p->set);
#else	    
	    ProtoColorGroup m = (ProtoColorGroup)p->set;
#endif	    
	    (this->*m)( (QColorGroup)(value.toColorGroup()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoColorGroup m = reinterpret_cast<RProtoColorGroup>(p->set);
#else	    
	    RProtoColorGroup m = (RProtoColorGroup)p->set;
#endif	    
	    (this->*m)( value.toColorGroup() );
	    return TRUE;
	}
	break;

    case QVariant::Bitmap:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoBitmap m = reinterpret_cast<ProtoBitmap>(p->set);
#else	    
	    ProtoBitmap m = (ProtoBitmap)p->set;
#endif	    
	    (this->*m)( (QBitmap)(value.toBitmap()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference )  {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoBitmap m = reinterpret_cast<RProtoBitmap>(p->set);
#else	    
	    RProtoBitmap m = (RProtoBitmap)p->set;
#endif	    
	    (this->*m)( value.toBitmap() );
	    return TRUE;
	}
	break;

    case QVariant::Region:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoRegion m = reinterpret_cast<ProtoRegion>(p->set);
#else	    
	    ProtoRegion m = (ProtoRegion)p->set;
#endif	    
	    (this->*m)( (QRegion)(value.toRegion()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference )  {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoRegion m = reinterpret_cast<RProtoRegion>(p->set);
#else	    
	    RProtoRegion m = (RProtoRegion)p->set;
#endif	    
	    (this->*m)( value.toRegion() );
	    return TRUE;
	}
	break;

    case QVariant::PointArray:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoPointArray m = reinterpret_cast<ProtoPointArray>(p->set);
#else	    
	    ProtoPointArray m = (ProtoPointArray)p->set;
#endif	    
	    (this->*m)( (QPointArray)(value.toPointArray()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference )  {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoPointArray m = reinterpret_cast<RProtoPointArray>(p->set);
#else	    
	    RProtoPointArray m = (RProtoPointArray)p->set;
#endif	    
	    (this->*m)( value.toPointArray() );
	    return TRUE;
	}
	break;

    case QVariant::Cursor:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoCursor m = reinterpret_cast<ProtoCursor>(p->set);
#else	    
	    ProtoCursor m = (ProtoCursor)p->set;
#endif	    
	    (this->*m)( (QCursor)(value.toCursor()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference )  {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoCursor m = reinterpret_cast<RProtoCursor>(p->set);
#else	    
	    RProtoCursor m = (RProtoCursor)p->set;
#endif	    
	    (this->*m)( value.toCursor() );
	    return TRUE;
	}
	break;

    case QVariant::IconSet:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoIconSet m = reinterpret_cast<ProtoIconSet>(p->set);
#else	    
	    ProtoIconSet m = (ProtoIconSet)p->set;
#endif	    
	    (this->*m)( (QIconSet)(value.toIconSet()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference )  {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoIconSet m = reinterpret_cast<RProtoIconSet>(p->set);
#else	    
	    RProtoIconSet m = (RProtoIconSet)p->set;
#endif	    
	    (this->*m)( value.toIconSet() );
	    return TRUE;
	}
	break;

    case QVariant::Int:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoInt m = reinterpret_cast<ProtoInt>(p->set);
#else	    
	    ProtoInt m = (ProtoInt)p->set;
#endif	    
	    (this->*m)( value.toInt() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoInt m = reinterpret_cast<RProtoInt>(p->set);
#else	    
	    RProtoInt m = (RProtoInt)p->set;
#endif	    
	    (this->*m)( value.toInt() );
	    return TRUE;
	}
	break;

    case QVariant::UInt:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoUInt m = reinterpret_cast<ProtoUInt>(p->set);
#else	    
	    ProtoUInt m = (ProtoUInt)p->set;
#endif	    
	    (this->*m)( value.toUInt() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoUInt m = reinterpret_cast<RProtoUInt>(p->set);
#else	    
	    RProtoUInt m = (RProtoUInt)p->set;
#endif	    
	    (this->*m)( value.toUInt() );
	    return TRUE;
	}
	break;

    case QVariant::Double:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoDouble m = reinterpret_cast<ProtoDouble>(p->set);
#else	    
	    ProtoDouble m = (ProtoDouble)p->set;
#endif	    
	    (this->*m)( value.toDouble() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoDouble m = reinterpret_cast<RProtoDouble>(p->set);
#else	    
	    RProtoDouble m = (RProtoDouble)p->set;
#endif	    
	    (this->*m)( value.toDouble() );
	    return TRUE;
	}
	break;

    case QVariant::Bool:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoBool m = reinterpret_cast<ProtoBool>(p->set);
#else	    
	    ProtoBool m = (ProtoBool)p->set;
#endif	    
	    (this->*m)( value.toBool() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoBool m = reinterpret_cast<RProtoBool>(p->set);
#else	    
	    RProtoBool m = (RProtoBool)p->set;
#endif	    
	    (this->*m)( value.toBool() );
	    return TRUE;
	}
	break;

    case QVariant::List:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoList m = reinterpret_cast<ProtoList>(p->set);
#else	    
	    ProtoList m = (ProtoList)p->set;
#endif	    
	    (this->*m)( (QValueList<QVariant>)(value.toList()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoList m = reinterpret_cast<RProtoList>(p->set);
#else	    
	    RProtoList m = (RProtoList)p->set;
#endif	    
	    (this->*m)( value.toList() );
	    return TRUE;
	}
	break;

    case QVariant::Map:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoMap m = reinterpret_cast<ProtoMap>(p->set);
#else	    
	    ProtoMap m = (ProtoMap)p->set;
#endif	    
	    (this->*m)( (QMap<QString, QVariant>)(value.toMap()) );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoMap m = reinterpret_cast<RProtoMap>(p->set);
#else	    
	    RProtoMap m = (RProtoMap)p->set;
#endif	    
	    (this->*m)( value.toMap() );
	    return TRUE;
	}
	break;

    case QVariant::SizePolicy:
	if ( p->sspec == QMetaProperty::Class ) {
#ifdef Q_FP_CCAST_BROKEN	
	    ProtoSizePolicy m = reinterpret_cast<ProtoSizePolicy>(p->set);
#else	    
	    ProtoSizePolicy m = (ProtoSizePolicy)p->set;
#endif	    
	    (this->*m)( value.toSizePolicy() );
	    return TRUE;
	} else if ( p->sspec == QMetaProperty::Reference ) {
#ifdef Q_FP_CCAST_BROKEN	
	    RProtoSizePolicy m = reinterpret_cast<RProtoSizePolicy>(p->set);
#else	    
	    RProtoSizePolicy m = (RProtoSizePolicy)p->set;
#endif	    
	    (this->*m)( value.toSizePolicy() );
	    return TRUE;
	}
	break;
    }

    qWarning( "%s (%s): failed to set property %s", className(), QObject::name(), p->name() );
    return FALSE;
}

/*!
  Returns the value of the object's \a name property.

  If no such property exists, the returned variant is invalid.

  Information about all available properties are provided through the
  metaObject().

  \sa setProperty(), QVariant::isValid(), metaObject(),
  QMetaObject::propertyNames(), QMetaObject::property()

*/
QVariant QObject::property( const char *name ) const
{

    QVariant value;

    typedef const char* (QObject::*ProtoConstCharStar)() const;

    typedef QString (QObject::*ProtoString)() const;
    typedef const QString* (QObject::*PProtoString)() const;
    typedef const QString& (QObject::*RProtoString)() const;

    typedef QCString (QObject::*ProtoCString)() const;
    typedef const QCString* (QObject::*PProtoCString)() const;
    typedef const QCString& (QObject::*RProtoCString)() const;

    typedef int (QObject::*ProtoInt)() const;
    typedef const int* (QObject::*PProtoInt)() const;
    typedef const int& (QObject::*RProtoInt)() const;

    typedef uint (QObject::*ProtoUInt)() const;
    typedef const uint* (QObject::*PProtoUInt)() const;
    typedef const uint& (QObject::*RProtoUInt)() const;

    typedef double (QObject::*ProtoDouble)() const;
    typedef const double* (QObject::*PProtoDouble)() const;
    typedef const double& (QObject::*RProtoDouble)() const;

    typedef bool (QObject::*ProtoBool)() const;
    typedef const bool* (QObject::*PProtoBool)() const;
    typedef const bool& (QObject::*RProtoBool)() const;

    typedef QFont (QObject::*ProtoFont)() const;
    typedef const QFont* (QObject::*PProtoFont)() const;
    typedef const QFont& (QObject::*RProtoFont)() const;

#ifndef __ATHEOS__
    typedef QPixmap (QObject::*ProtoPixmap)() const;
    typedef const QPixmap* (QObject::*PProtoPixmap)() const;
    typedef const QPixmap& (QObject::*RProtoPixmap)() const;
#endif
    
    typedef QBrush (QObject::*ProtoBrush)() const;
    typedef const QBrush* (QObject::*PProtoBrush)() const;
    typedef const QBrush& (QObject::*RProtoBrush)() const;

    typedef QRect (QObject::*ProtoRect)() const;
    typedef const QRect* (QObject::*PProtoRect)() const;
    typedef const QRect& (QObject::*RProtoRect)() const;

    typedef QSize (QObject::*ProtoSize)() const;
    typedef const QSize* (QObject::*PProtoSize)() const;
    typedef const QSize& (QObject::*RProtoSize)() const;

    typedef QColor (QObject::*ProtoColor)() const;
    typedef const QColor* (QObject::*PProtoColor)() const;
    typedef const QColor& (QObject::*RProtoColor)() const;

    typedef QPalette (QObject::*ProtoPalette)() const;
    typedef const QPalette* (QObject::*PProtoPalette)() const;
    typedef const QPalette& (QObject::*RProtoPalette)() const;

    typedef QColorGroup (QObject::*ProtoColorGroup)() const;
    typedef const QColorGroup* (QObject::*PProtoColorGroup)() const;
    typedef const QColorGroup& (QObject::*RProtoColorGroup)() const;

    typedef QIconSet (QObject::*ProtoIconSet)() const;
    typedef const QIconSet* (QObject::*PProtoIconSet)() const;
    typedef const QIconSet& (QObject::*RProtoIconSet)() const;

    typedef QPoint (QObject::*ProtoPoint)() const;
    typedef const QPoint* (QObject::*PProtoPoint)() const;
    typedef const QPoint& (QObject::*RProtoPoint)() const;

    typedef QBitmap (QObject::*ProtoBitmap)() const;
    typedef const QBitmap* (QObject::*PProtoBitmap)() const;
    typedef const QBitmap& (QObject::*RProtoBitmap)() const;

    typedef QRegion (QObject::*ProtoRegion)() const;
    typedef const QRegion* (QObject::*PProtoRegion)() const;
    typedef const QRegion& (QObject::*RProtoRegion)() const;

    typedef QPointArray (QObject::*ProtoPointArray)() const;
    typedef const QPointArray* (QObject::*PProtoPointArray)() const;
    typedef const QPointArray& (QObject::*RProtoPointArray)() const;

    typedef QCursor (QObject::*ProtoCursor)() const;
    typedef const QCursor* (QObject::*PProtoCursor)() const;
    typedef const QCursor& (QObject::*RProtoCursor)() const;

    typedef QImage (QObject::*ProtoImage)() const;
    typedef const QImage* (QObject::*PProtoImage)() const;
    typedef const QImage& (QObject::*RProtoImage)() const;

    typedef QStringList (QObject::*ProtoStringList)() const;
    typedef const QStringList* (QObject::*PProtoStringList)() const;
    typedef const QStringList& (QObject::*RProtoStringList)() const;

    typedef QValueList<QVariant> (QObject::*ProtoList)() const;
    typedef const QValueList<QVariant>* (QObject::*PProtoList)() const;
    typedef const QValueList<QVariant>& (QObject::*RProtoList)() const;

    typedef QMap<QString,QVariant> (QObject::*ProtoMap)() const;
    typedef const QMap<QString,QVariant>* (QObject::*PProtoMap)() const;
    typedef const QMap<QString,QVariant>& (QObject::*RProtoMap)() const;

    typedef QSizePolicy (QObject::*ProtoSizePolicy)() const;
    typedef const QSizePolicy* (QObject::*PProtoSizePolicy)() const;
    typedef const QSizePolicy& (QObject::*RProtoSizePolicy)() const;

    QMetaObject* meta = queryMetaObject();
    if ( !meta )
	return value;
    const QMetaProperty* p = meta->property( name, TRUE );
    if ( !p )
	return value;

    if ( p->isEnumType() ) {
	ProtoInt m = (ProtoInt)p->get;
	int x = (int) (this->*m)();
	value = QVariant( x );
	return value;
    }

    // p->type must be a type understood by QVariant, so we can savely convert it.
    QVariant::Type type = QVariant::nameToType( p->type() );

    switch ( type ) {
    case QVariant::Invalid:
	// A real assert, since this indicates a moc bug
	ASSERT( 0 );
	return QVariant();

    case QVariant::Image:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoImage m = (ProtoImage)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoImage m = (RProtoImage)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoImage m = (PProtoImage)p->get;
	    const QImage* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QImage() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Point:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoPoint m = (ProtoPoint)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoPoint m = (RProtoPoint)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoPoint m = (PProtoPoint)p->get;
	    const QPoint* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QPoint() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::StringList:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoStringList m = (ProtoStringList)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoStringList m = (RProtoStringList)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoStringList m = (PProtoStringList)p->get;
	    const QStringList* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QStringList() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::List:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoList m = (ProtoList)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoList m = (RProtoList)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoList m = (PProtoList)p->get;
	    const QValueList<QVariant>* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QValueList<QVariant>() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::CString:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoCString m = (ProtoCString)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoCString m = (RProtoCString)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoCString m = (PProtoCString)p->get;
	    const QCString* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QCString() );
	} else if ( p->gspec == QMetaProperty::ConstCharStar ) {
	    ProtoConstCharStar m = (ProtoConstCharStar)p->get;
	    value = QVariant( QCString( (this->*m)() ) );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::String:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoString m = (ProtoString)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoString m = (RProtoString)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoString m = (PProtoString)p->get;
	    const QString* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QString() );
	} else if ( p->gspec == QMetaProperty::ConstCharStar ) {
	    ProtoConstCharStar m = (ProtoConstCharStar)p->get;
	    value = QVariant( QCString( (this->*m)() ) );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Font:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoFont m = (ProtoFont)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoFont m = (RProtoFont)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoFont m = (PProtoFont)p->get;
	    const QFont* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QFont() );
	} else {
	    ASSERT( 0 );
	}
	return value;

#ifndef __ATHEOS__
    case QVariant::Pixmap:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoPixmap m = (ProtoPixmap)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoPixmap m = (RProtoPixmap)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoPixmap m = (PProtoPixmap)p->get;
	    const QPixmap* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QPixmap() );
	} else {
	    ASSERT( 0 );
	}
	return value;
#endif __ATHEOS__

    case QVariant::Brush:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoBrush m = (ProtoBrush)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoBrush m = (RProtoBrush)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoBrush m = (PProtoBrush)p->get;
	    const QBrush* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QBrush() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Rect:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoRect m = (ProtoRect)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoRect m = (RProtoRect)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoRect m = (PProtoRect)p->get;
	    const QRect* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QRect() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Size:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoSize m = (ProtoSize)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoSize m = (RProtoSize)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoSize m = (PProtoSize)p->get;
	    const QSize* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QSize() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Color:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoColor m = (ProtoColor)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoColor m = (RProtoColor)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoColor m = (PProtoColor)p->get;
	    const QColor* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QColor() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Palette:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoPalette m = (ProtoPalette)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoPalette m = (RProtoPalette)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoPalette m = (PProtoPalette)p->get;
	    const QPalette* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QPalette() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::ColorGroup:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoColorGroup m = (ProtoColorGroup)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoColorGroup m = (RProtoColorGroup)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoColorGroup m = (PProtoColorGroup)p->get;
	    const QColorGroup* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QColorGroup() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Bitmap:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoBitmap m = (ProtoBitmap)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoBitmap m = (RProtoBitmap)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoBitmap m = (PProtoBitmap)p->get;
	    const QBitmap* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QBitmap() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::PointArray:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoPointArray m;
	    m = (ProtoPointArray)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoPointArray m = (RProtoPointArray)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoPointArray m = (PProtoPointArray)p->get;
	    const QPointArray* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QPointArray() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Region:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoRegion m = (ProtoRegion)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoRegion m = (RProtoRegion)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoRegion m = (PProtoRegion)p->get;
	    const QRegion* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QRegion() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Cursor:
#ifndef QT_NO_CURSOR
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoCursor m = (ProtoCursor)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoCursor m = (RProtoCursor)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoCursor m = (PProtoCursor)p->get;
	    const QCursor* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QCursor() );
	} else {
	    ASSERT( 0 );
	}
#else
	qWarning("Cursor in QVariant ignored");
#endif
	return value;

    case QVariant::IconSet:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoIconSet m = (ProtoIconSet)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoIconSet m = (RProtoIconSet)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoIconSet m = (PProtoIconSet)p->get;
	    const QIconSet* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QIconSet() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Int:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoInt m = (ProtoInt)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoInt m = (RProtoInt)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoInt m = (PProtoInt)p->get;
	    const int *p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( 0 );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::UInt:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoUInt m = (ProtoUInt)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoUInt m = (RProtoUInt)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoUInt m = (PProtoUInt)p->get;
	    const uint *p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( 0 );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Double:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoDouble m = (ProtoDouble)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoDouble m = (RProtoDouble)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoDouble m = (PProtoDouble)p->get;
	    const double* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( 0.0 );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Bool:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoBool m = (ProtoBool)p->get;
	    value = QVariant( (this->*m)(), 42 /* dummy */ );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoBool m = (RProtoBool)p->get;
	    value = QVariant( (this->*m)(), 42 /* dummy */ );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoBool m = (PProtoBool)p->get;
	    const bool* p = (this->*m)();
	    value = QVariant( p ? *p : FALSE, 42 /* dummy */ );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::Map:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoMap m = (ProtoMap)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoMap m = (RProtoMap)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoMap m = (PProtoMap)p->get;
	    const QMap<QString,QVariant>* p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( QMap<QString,QVariant>() );
	} else {
	    ASSERT( 0 );
	}
	return value;

    case QVariant::SizePolicy:
	if ( p->gspec == QMetaProperty::Class ) {
	    ProtoSizePolicy m = (ProtoSizePolicy)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Reference ) {
	    RProtoSizePolicy m = (RProtoSizePolicy)p->get;
	    value = QVariant( (this->*m)() );
	} else if ( p->gspec == QMetaProperty::Pointer ) {
	    PProtoSizePolicy m = (PProtoSizePolicy)p->get;
	    const QSizePolicy *p = (this->*m)();
	    if ( p )
		value = QVariant( *p );
	    else
		value = QVariant( 0 );
	} else {
	    ASSERT( 0 );
	}
	return value;
    }

    return value;
}

#endif // QT_NO_PROPERTIES


