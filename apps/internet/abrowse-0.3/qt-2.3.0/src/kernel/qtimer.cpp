/****************************************************************************
** $Id$
**
** Implementation of QTimer class
**
** Created : 931111
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

#include "qtimer.h"
#include "qsignal.h"
#include "qobjectlist.h"
#include "qobjectdict.h"
#include "qapplication.h"

// NOT REVISED
/*!
  \class QTimer qtimer.h
  \brief The QTimer class provides timer signals and single-shot timers.

  \ingroup time
  \ingroup event

  It uses \link QTimerEvent timer events\endlink internally to provide a
  more versatile timer.	 QTimer is very easy to use, create a QTimer, call
  start() to start it and connect its timeout() to the appropriate slots,
  then when the time is up it will emit timeout().

  Note that a QTimer object is destroyed automatically when its parent
  object is destroyed.

  Example:
  \code
    QTimer *timer = new QTimer( myObject );
    connect( timer, SIGNAL(timeout()),
	     myObject, SLOT(timerDone()) );
    timer->start( 2000, TRUE );			// 2 seconds single-shot
  \endcode

  As a special case, a QTimer with timeout 0 times out as soon as all
  the events in the window system's event queue have been processed.

  This can be used to do heavy work while providing a snappy
  user interface: \code
    QTimer *t = new QTimer( myObject );
    connect( t, SIGNAL(timeout()), SLOT(processOneThing()) );
    t->start( 0, FALSE );
  \endcode

  myObject->processOneThing() will be called repeatedly and should
  return quickly (typically after processing one data item) so that Qt
  can deliver events to widgets, and stop the timer as soon as it has
  done all its work.  This is the traditional way of implementing heavy
  work in GUI applications; multi-threading is now becoming available
  on more and more platforms and we expect that null events will
  eventually be replaced by threading.

  Note that QTimer's accuracy depends on the underlying operating
  system and hardware.  Most platforms support an accuracy of 20ms;
  some provide more.  If Qt is unable to deliver the requested number
  of timer clicks, it will silently discard some.

  An alternative to using QTimer is to call QObject::startTimer() for
  your object and reimplement the QObject::timerEvent() event handler
  in your class (which must of course inherit QObject).  The
  disadvantage is that timerEvent() does not support such high-level
  features as single-shot timers or signals.
*/


const int INV_TIMER = -1;			// invalid timer id


/*!
  Constructs a timer with a \e parent and a \e name.

  Notice that the destructor of the parent object will destroy this timer
  object.
*/

QTimer::QTimer( QObject *parent, const char *name )
    : QObject( parent, name )
{
    id = INV_TIMER;
}

/*!
  Destructs the timer.
*/

QTimer::~QTimer()
{
    if ( id != INV_TIMER )			// stop running timer
	stop();
}


/*!
  \fn void QTimer::timeout()
  This signal is emitted when the timer is activated.
*/

/*!
  \fn bool QTimer::isActive() const
  Returns TRUE if the timer is running (pending), or FALSE is the timer is
  idle.
*/


/*!
  Starts the timer with a \e msecs milliseconds timeout.

  If \e sshot is TRUE, the timer will be activated only once,
  otherwise it will continue until it is stopped.

  Any pending timer will be stopped.

  \sa stop(), changeInterval(), isActive()
*/

int QTimer::start( int msec, bool sshot )
{
    if ( id >=0 && !msec && sshot == single )
	return id;
    if ( id != INV_TIMER )			// stop running timer
	stop();
    single = sshot;
    return id = startTimer( msec );
}


/*!
  Changes the timeout interval to \e msec milliseconds.

  If the timer signal is pending, it will be stopped and restarted,
  otherwise it will be started.

  \sa start(), isActive()
*/

void QTimer::changeInterval( int msec )
{
    if ( id == INV_TIMER ) {			// create new timer
	start( msec );
    } else {
	killTimer( id );			// restart timer
	id = startTimer( msec );
    }
}

/*!
  Stops the timer.
  \sa start()
*/

void QTimer::stop()
{
    if ( id != INV_TIMER ) {
	killTimer( id );
	id = INV_TIMER;
    }
}


/*!\reimp
*/
bool QTimer::event( QEvent *e )
{
    if ( e->type() != QEvent::Timer )		// ignore all other events
	return FALSE;
    if ( single )				// stop single shot timer
	stop();
    emit timeout();				// emit timeout signal
    return TRUE;
}


/*
  The QSingleShotTimer class is an internal class for implementing
  QTimer::singleShot(). It starts a timer and emits the signal
  and kills itself when it gets the timeout.
*/

static QObjectList *sst_list = 0;		// list of single shot timers

void sst_cleanup()
{
    if ( sst_list ) {
	sst_list->setAutoDelete( TRUE );
	delete sst_list;
	sst_list = 0;
    }
}

void sst_init()
{
    if ( !sst_list ) {
	sst_list = new QObjectList;
	CHECK_PTR( sst_list );
	qAddPostRoutine( sst_cleanup );
    }
}


class QSingleShotTimer : public QObject
{
public:
    QSingleShotTimer() : QObject( qApp ) {}
    bool    start( int msec, QObject *r, const char * m );
protected:
    bool    event( QEvent * );
private:
    QSignal signal;
    int	    timerId;
};

int  qStartTimer( int interval, QObject *obj ); // implemented in qapp_xxx.cpp
bool qKillTimer( int id );

bool QSingleShotTimer::start( int msec, QObject *r, const char *m )
{
    timerId = 0;
    if ( signal.connect(r, m) )
	timerId = qStartTimer( msec, (QObject *)this );
    return timerId != 0;
}

bool QSingleShotTimer::event( QEvent * )
{
    qKillTimer( timerId );			// no more timeouts
    signal.activate();				// emit the signal
    signal.disconnect( 0, 0 );
    sst_list->insert( 0, this );		// store in free list
    return TRUE;
}


/*!
  This static function calls a slot after a given time interval.

  It is very convenient to use this function because you do not need to
  bother with a \link QObject::timerEvent() timerEvent\endlink or to
  create a local QTimer object.

  Example:
  \code
    #include <qapplication.h>
    #include <qtimer.h>

    int main( int argc, char **argv )
    {
	QApplication a( argc, argv );
	QTimer::singleShot( 10*60*1000, &a, SLOT(quit()) );
	    ... // create and show your widgets
	return a.exec();
    }
  \endcode

  This sample program automatically terminates after 10 minutes (i.e.
  600000 milliseconds).
*/

void QTimer::singleShot( int msec, QObject *receiver, const char *member )
{
    if ( !sst_list )
	sst_init();
    QSingleShotTimer *sst;
    if ( sst_list->isEmpty() ) {		// create new ss timer
	sst = new QSingleShotTimer;
    } else {					// use existing one
	sst = (QSingleShotTimer *)sst_list->take( 0 );
    }	
    sst->start(msec, receiver, member);
}
