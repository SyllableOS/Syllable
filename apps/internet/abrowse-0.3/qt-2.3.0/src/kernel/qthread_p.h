/****************************************************************************
** $Id$
**
** QThread class for Unix
**
** Created : 20001309
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

#ifndef QTHREAD_P_H
#define QTHREAD_P_H

#include <qglobal.h>

#include <errno.h>
#include <unistd.h>
#ifdef QWS
#include <qptrdict.h>
#else
#include <qintdict.h>
#endif


// Thread definitions for UNIX platforms

#if defined(_OS_LINUX_)
#  define _XOPEN_SOURCE 500
// Linux: couldn't we query macros from <unistd.h> instead of library versions?
#  if (__GLIBC__ == 2) && (__GLIBC_MINOR__ == 0)
// Linux with glibc 2.0.x - POSIX 1003.4a thread implementation
#    define Q_HAS_RECURSIVE_MUTEX
#    define Q_USE_PTHREAD_MUTEX_SETKIND
#    define Q_NORMAL_MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK_NP
#    define Q_RECURSIVE_MUTEX_TYPE PTHREAD_MUTEX_RECURSIVE_NP
#  else
// Linux with glibc 2.1.x - POSIX 1003.1c thread implementation
#    define Q_HAS_RECURSIVE_MUTEX
#    undef  Q_USE_PTHREAD_MUTEX_SETKIND
#    define Q_NORMAL_MUTEX_TYPE PTHREAD_MUTEX_DEFAULT
#    define Q_RECURSIVE_MUTEX_TYPE PTHREAD_MUTEX_RECURSIVE
#  endif
#elif defined(_OS_OSF_)
// Digital UNIX - 4.0 and later has a POSIX 1003.1c implementation
//   - should we assume > 4.0?
#  define Q_HAS_RECURSIVE_MUTEX
#  undef  Q_USE_PTHREAD_MUTEX_SETKIND
#  define Q_NORMAL_MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK
#  define Q_RECURSIVE_MUTEX_TYPE PTHREAD_MUTEX_RECURSIVE
#elif defined(_OS_AIX_)
// AIX 4.3.x
#  define Q_HAS_RECURSIVE_MUTEX
#  undef  Q_USE_PTHREAD_MUTEX_SETKIND
#  define Q_NORMAL_MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK
#  define Q_RECURSIVE_MUTEX_TYPE PTHREAD_MUTEX_RECURSIVE
#elif defined(_OS_HPUX_)
// We only support HP/UX 11.x
#  define Q_HAS_RECURSIVE_MUTEX
#  undef  Q_USE_PTHREAD_MUTEX_SETKIND
#  define Q_NORMAL_MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK
#  define Q_RECURSIVE_MUTEX_TYPE PTHREAD_MUTEX_RECURSIVE
#elif defined (_OS_FREEBSD_) || defined(_OS_OPENBSD_)
// FreeBSD and OpenBSD use the same user-space thread implementation
#  define Q_HAS_RECURSIVE_MUTEX
#  undef  Q_USE_PTHREAD_MUTEX_SETKIND
#  define Q_NORMAL_MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK
#  define Q_RECURSIVE_MUTEX_TYPE PTHREAD_MUTEX_RECURSIVE
#elif defined(_OS_SOLARIS_)
// Solaris 2.7 and later - we use the native Solaris threads implementation
#  undef  Q_HAS_RECURSIVE_MUTEX
#  undef  Q_USE_PTHREAD_MUTEX_SETKIND
#  undef  Q_NORMAL_MUTEX_TYPE
#  undef  Q_RECURSIVE_MUTEX_TYPE
#elif defined(_OS_IRIX_)
#  define Q_HAS_RECURSIVE_MUTEX
#  undef  Q_USE_PTHREAD_MUTEX_SETKIND
#  define Q_NORMAL_MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK
#  define Q_RECURSIVE_MUTEX_TYPE PTHREAD_MUTEX_RECURSIVE
#else
// Fall through for systems we don't know about
// #  warning "Assuming non-POSIX 1003.1c thread implementation. Talk to qt-bugs@trolltech.com."
#  undef  Q_HAS_RECURSIVE_MUTEX
#  undef  Q_USE_PTHREAD_MUTEX_SETKIND
#  undef  Q_NORMAL_MUTEX_TYPE
#  undef  Q_RECURSIVE_MUTEX_TYPE
#endif


static QMutex *dictMutex = 0;
#ifdef QWS
static QPtrDict<QThread> *thrDict = 0;
#else
static QIntDict<QThread> *thrDict = 0;
#endif


extern "C" { static void * start_thread(void * t); }


#if defined(_OS_SOLARIS_)


#include <thread.h>
// Function usleep() is in C library but not in header files on Solaris 2.5.1.
// Not really a surprise, usleep() is specified by XPG4v2 and XPG4v2 is only
// supported by Solaris 2.6 and better.
// So we are trying to detect Solaris 2.5.1 using macro _XOPEN_UNIX which is
// defined by <unistd.h> when XPG4v2 is supported.
#if !defined(_XOPEN_UNIX)
typedef unsigned int useconds_t;
extern "C" int usleep(useconds_t);
#endif


class QMutexPrivate {
public:
    mutex_t mutex;

    QMutexPrivate(bool recursive = FALSE)
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    mutex_init( &mutex, NULL, NULL );

#ifdef CHECK_RANGE
	if( ret )
	    qWarning( "QMutex::QMutex: init failure: %s", strerror( ret ) );
#endif
    }

    virtual ~QMutexPrivate()
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    mutex_destroy( &mutex );

#ifdef CHECK_RANGE
	if ( ret )
	    qWarning( "QMutex::~QMutex: destroy failure: %s", strerror( ret ) );
#endif
    }

    virtual void lock()
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    mutex_lock(&mutex);

#ifdef CHECK_RANGE
	if (ret)
	    qWarning("QMutex::lock: mutex lock failure: %s", strerror(ret));
#endif
    }

    virtual void unlock()
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    mutex_unlock(&mutex);

#ifdef CHECK_RANGE
	if (ret)
	    qWarning("QMutex::unlock: mutex unlock failure: %s", strerror(ret));
#endif
    }

    virtual bool locked()
    {
	int ret = mutex_trylock(&mutex);

	if (ret == EBUSY) {
	    return TRUE;
	} else if (ret) {
#ifdef CHECK_RANGE
	    qWarning("QMutex::locked: try lock failed: %s", strerror(ret));
#endif
	} else {
	    mutex_unlock(&mutex);
	}

	return FALSE;
    }

#if defined(CHECK_RANGE) || !defined(Q_HAS_RECURSIVE_MUTEX)
    virtual int type() const { return Q_MUTEX_NORMAL; }
#endif
};


class QRMutexPrivate : public QMutexPrivate
{
public:
    int count;
    HANDLE owner;
    mutex_t mutex2;

    QRMutexPrivate()
	: QMutexPrivate(TRUE)
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    mutex_init( &mutex2, NULL, NULL );


#ifdef CHECK_RANGE
	if( ret )
	    qWarning( "QMutex::QMutex: init failure: %s", strerror( ret ) );
#endif

	count = 0;
    }

    ~QRMutexPrivate()
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    mutex_destroy(&mutex2);

#ifdef CHECK_RANGE
	if( ret )
	    qWarning( "QMutex::QMutex: destroy failure: %s", strerror( ret ) );
#endif
    }

    void lock()
    {
	mutex_lock(&mutex2);

	if(count > 0 && owner == QThread::currentThread()) {
	    count++;
	} else {
	    mutex_unlock(&mutex2);
	    QMutexPrivate::lock();
	    mutex_lock(&mutex2);

	    count = 1;
	    owner = QThread::currentThread();
	}

	mutex_unlock(&mutex2);
    }

    void unlock()
    {
	mutex_lock(&mutex2);

	if (owner != QThread::currentThread()) {
#ifdef CHECK_RANGE
	    qWarning("QMutex::unlock: unlock from different thread than locker");
	    qWarning("                was locked by %d, unlock attempt from %d",
		     owner, QThread::currentThread());
	    mutex_unlock(&mutex2);
#endif

	    return;
	}

	// do nothing if the count is already 0... to reflect the behaviour described
	// in the docs
	if (count && (--count) < 1) {
	    QMutexPrivate::unlock();
	    count=0;
	}

	mutex_unlock(&mutex2);
    }

    bool locked()
    {
	mutex_lock(&mutex2);
	bool ret = QMutexPrivate::locked();
	mutex_unlock(&mutex2);

	return ret;
    }

#if defined(CHECK_RANGE) || !defined(Q_HAS_RECURSIVE_MUTEX)
    int type() const { return Q_MUTEX_RECURSIVE; }
#endif
};


class QThreadPrivate {
public:
    thread_t thread_id;
    QWaitCondition thread_done;
    bool finished, running;

    QThreadPrivate()
	: thread_id(0), finished(FALSE), running(FALSE)
    {
	if (! dictMutex)
	    dictMutex = new QMutex;
	if (! thrDict)
#ifdef QWS
	    thrDict = new QPtrDict<QThread>;
#else
	thrDict = new QIntDict<QThread>;
#endif
    }

    ~QThreadPrivate()
    {
	dictMutex->lock();
	if (thread_id)
	    thrDict->remove((HANDLE) thread_id);
	dictMutex->unlock();

	thread_id = 0;
    }

    void init(QThread *that)
    {
	that->d->running = TRUE;
	that->d->finished = FALSE;

	int ret = thr_create( NULL, NULL, start_thread, that, THR_DETACHED,
			      &thread_id );

#ifdef CHECK_RANGE
	if (ret)
	    qWarning("QThread::start: thread creation error: %s", strerror(ret));
#endif
    }

    static void internalRun(QThread *that)
    {
	dictMutex->lock();
	thrDict->insert(QThread::currentThread(), that);
	dictMutex->unlock();

	that->d->running = TRUE;
	that->d->finished = FALSE;

	that->run();

	dictMutex->lock();

	QThread *there = thrDict->find(QThread::currentThread());
	if (there) {
	    there->d->running = FALSE;
	    there->d->finished = TRUE;

	    there->d->thread_done.wakeAll();
	}

	thrDict->remove(QThread::currentThread());
	dictMutex->unlock();
    }
};


class QWaitConditionPrivate {
public:
    cond_t cond;
    QMutex mutex;

    QWaitConditionPrivate()
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    cond_init(&cond, NULL, NULL );

#ifdef CHECK_RANGE
	if( ret )
	    qWarning( "QWaitCondition::QWaitCondition: event init failure %s", strerror( ret ) );
#endif
    }

    ~QWaitConditionPrivate()
    {
	int ret = cond_destroy(&cond);
	if( ret ) {
#ifdef CHECK_RANGE
	    qWarning( "QWaitCondition::~QWaitCondition: event destroy failure %s", strerror( ret ) );
#endif

	    // seems we have threads waiting on us, lets wake them up
	    cond_broadcast(&cond);
	}
    }

    void wakeOne()
    {
	mutex.lock();

#ifdef CHECK_RANGE
	int ret =
#endif
	    cond_signal(&(cond));

#ifdef CHECK_RANGE
	if ( ret ) qWarning("QWaitCondition::wakeOne: wake error: %s",strerror(ret));
#endif

	mutex.unlock();
    }

    void wakeAll()
    {
	mutex.lock();

#ifdef CHECK_RANGE
	int ret =
#endif
	    cond_broadcast(& (cond) );

#ifdef CHECK_RANGE
	if( ret ) qWarning("QWaitCondition::wakeAll: wake error: %s",strerror(ret));
#endif

	mutex.unlock();
    }

    bool wait(unsigned long time)
    {
	mutex.lock();

	int ret;
	if (time != ULONG_MAX) {
	    timespec ti;
	    ti.tv_sec = (time / 1000);
	    ti.tv_nsec = (time % 1000) * 1000000;

	    ret = cond_timedwait(&(cond), &(mutex.d->mutex), &ti);
	} else {
	    ret = cond_wait ( &(cond), &(mutex.d->mutex) );
	}

	mutex.unlock();

#ifdef CHECK_RANGE
	if( ret ) qWarning("QWaitCondition::wait: wait error:%s",strerror(ret));
#endif

	return (ret == 0);
    }

    bool wait(QMutex *mtx, unsigned long time)
    {
	if (! mtx) return FALSE;

#ifdef CHECK_RANGE
	if (mtx->d->type() == Q_MUTEX_RECURSIVE)
	    qWarning("QWaitCondition::wait: warning - using recursive mutexes with\n"
		     "                      conditions is undefined!");
#endif

#ifndef Q_HAS_RECURSIVE_MUTEX
	int c = 0;
	HANDLE id = 0;

	if (mtx->d->type() == Q_MUTEX_RECURSIVE) {
	    QRMutexPrivate *rmp = (QRMutexPrivate *) mtx->d;
	    mutex_lock(&(rmp->mutex2));

	    if (! rmp->count) {
#  ifdef CHECK_RANGE
		qWarning("QWaitCondition::unlockAndWait: recursive mutex not locked!");
#  endif

		return FALSE;
	    }

	    c = rmp->count;
	    id = rmp->owner;

	    rmp->count = 0;
	    rmp->owner = 0;

	    mutex_unlock(&(rmp->mutex2));
	}
#endif

	int ret;
	if (time != ULONG_MAX) {
	    timespec ti;
	    ti.tv_sec = (time / 1000);
	    ti.tv_nsec = (time % 1000) * 1000000;

	    ret = cond_timedwait(&(cond), &(mtx->d->mutex), &ti);
	} else {
	    ret = cond_wait ( &(cond), &(mtx->d->mutex) );
	}

#ifndef Q_HAS_RECURSIVE_MUTEX
	if (mtx->d->type() == Q_MUTEX_RECURSIVE) {
	    QRMutexPrivate *rmp = (QRMutexPrivate *) mtx->d;
	    mutex_lock(&(rmp->mutex2));
	    rmp->count = c;
	    rmp->owner = id;
	    mutex_unlock(&(rmp->mutex2));
	}
#endif

#ifdef CHECK_RANGE
	if ( ret ) qWarning("QWaitCondition::wait: wait error:%s",strerror(ret));
#endif

	return (ret == 0);
    }
};


#else // ! defined(_OS_SOLARIS_)


#include <pthread.h>


class QMutexPrivate {
public:
#if defined (_OS_SOLARIS_)
    mutex_t mutex;
#else
    pthread_mutex_t mutex;
#endif

    QMutexPrivate(bool recursive = FALSE)
    {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);

#if defined(Q_HAS_RECURSIVE_MUTEX)
	if (recursive) {

#  if defined(Q_RECURSIVE_MUTEX_TYPE)
#    if defined(Q_USE_PTHREAD_MUTEX_SETKIND)
	    pthread_mutexattr_setkind_np(&attr, Q_RECURSIVE_MUTEX_TYPE);
#    else
	    pthread_mutexattr_settype(&attr, Q_RECURSIVE_MUTEX_TYPE);
#    endif
#  endif

	} else {
#endif

#if defined(Q_NORMAL_MUTEX_TYPE)
#  if defined(Q_USE_PTHREAD_MUTEX_SETKIND)
	    pthread_mutexattr_setkind_np(&attr, Q_NORMAL_MUTEX_TYPE);
#  else
	    pthread_mutexattr_settype(&attr, Q_NORMAL_MUTEX_TYPE);
#  endif
#endif

#if defined(Q_HAS_RECURSIVE_MUTEX)
	}
#endif

#ifdef CHECK_RANGE
	int ret =
#endif
	    pthread_mutex_init( &mutex, &attr );

	pthread_mutexattr_destroy(&attr);

#ifdef CHECK_RANGE
	if( ret )
	    qWarning( "QMutex::QMutex: init failure: %s", strerror( ret ) );
#endif
    }

    virtual ~QMutexPrivate()
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    pthread_mutex_destroy( &mutex );

#ifdef CHECK_RANGE
	if ( ret )
	    qWarning( "QMutex::~QMutex: destroy failure: %s", strerror( ret ) );
#endif
    }

    virtual void lock()
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    pthread_mutex_lock(&mutex);

#ifdef CHECK_RANGE
	if (ret)
	    qWarning("QMutex::lock: mutex lock failure: %s", strerror(ret));
#endif
    }

    virtual void unlock()
    {
#ifdef CHECK_RANGE
	int ret =
#endif
	    pthread_mutex_unlock(&mutex);

#ifdef CHECK_RANGE
	if (ret)
	    qWarning("QMutex::unlock: mutex unlock failure: %s", strerror(ret));
#endif
    }

    virtual bool locked()
    {
	int ret = pthread_mutex_trylock(&mutex);

	if (ret == EBUSY) {
	    return TRUE;
	} else if (ret) {
#ifdef CHECK_RANGE
	    qWarning("QMutex::locked: try lock failed: %s", strerror(ret));
#endif
	} else {
	    pthread_mutex_unlock(&mutex);
	}

	return FALSE;
    }

#if defined(CHECK_RANGE) || !defined(Q_HAS_RECURSIVE_MUTEX)
    virtual int type() const { return Q_MUTEX_NORMAL; }
#endif
};


class QRMutexPrivate : public QMutexPrivate
{
public:
#ifndef Q_HAS_RECURSIVE_MUTEX
    int count;
    HANDLE owner;
    pthread_mutex_t mutex2;

    ~QRMutexPrivate()
    {
#  ifdef CHECK_RANGE
	int ret =
#  endif
	    pthread_mutex_destroy(&mutex2);

#  ifdef CHECK_RANGE
	if( ret )
	    qWarning( "QMutex::QMutex: destroy failure: %s", strerror( ret ) );
#  endif

    }

    void lock()
    {
	pthread_mutex_lock(&mutex2);

	if(count > 0 && owner == QThread::currentThread()) {
	    count++;
	} else {
	    pthread_mutex_unlock(&mutex2);
	    QMutexPrivate::lock();
	    pthread_mutex_lock(&mutex2);

	    count = 1;
	    owner = QThread::currentThread();
	}

	pthread_mutex_unlock(&mutex2);
    }

    void unlock()
    {
	pthread_mutex_lock(&mutex2);

	if (owner != QThread::currentThread()) {
#ifdef CHECK_RANGE
	    qWarning("QMutex::unlock: unlock from different thread than locker");
	    qWarning("                was locked by %d, unlock attempt from %d",
		     owner, QThread::currentThread());
	    pthread_mutex_unlock(&mutex2);
#endif

	    return;
	}

	// do nothing if the count is already 0... to reflect the behaviour described
	// in the docs
	if (count && (--count) < 1) {
	    QMutexPrivate::unlock();
	    count=0;
	}

	pthread_mutex_unlock(&mutex2);
    }

    bool locked()
    {
	pthread_mutex_lock(&mutex2);
	bool ret = QMutexPrivate::locked();
	pthread_mutex_unlock(&mutex2);

	return ret;
    }
#endif

    QRMutexPrivate()
	: QMutexPrivate(TRUE)
    {
#ifndef Q_HAS_RECURSIVE_MUTEX
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);

#  ifdef CHECK_RANGE
	int ret =
#  endif
	    pthread_mutex_init( &mutex2, &attr );

	pthread_mutexattr_destroy(&attr);

#  ifdef CHECK_RANGE
	if( ret )
	    qWarning( "QMutex::QMutex: init failure: %s", strerror( ret ) );
#  endif

	count = 0;
#endif
    }

#if defined(CHECK_RANGE) || !defined(Q_HAS_RECURSIVE_MUTEX)
    int type() const { return Q_MUTEX_RECURSIVE; }
#endif
};


class QThreadPrivate {
public:
    pthread_t thread_id;
    QWaitCondition thread_done;      // Used for QThread::wait()
    bool finished, running;

    QThreadPrivate()
	: thread_id(0), finished(FALSE), running(FALSE)
    {
	if (! dictMutex)
	    dictMutex = new QMutex;
	if (! thrDict)
#ifdef QWS
	    thrDict = new QPtrDict<QThread>;
#else
	thrDict = new QIntDict<QThread>;
#endif
    }

    ~QThreadPrivate()
    {
	dictMutex->lock();
	if (thread_id)
	    thrDict->remove((HANDLE) thread_id);
	dictMutex->unlock();

	thread_id = 0;
    }

    void init(QThread *that)
    {
	that->d->running = TRUE;
	that->d->finished = FALSE;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	int ret = pthread_create( &thread_id, &attr, start_thread, (void *) that );

	pthread_attr_destroy(&attr);

	if ( ret ) {
#ifdef CHECK_RANGE
	    qWarning("QThread::start: thread creation error: %s", strerror(ret));
#endif

	    return;
	}
    }

    static void internalRun(QThread *that)
    {
	dictMutex->lock();
	thrDict->insert(QThread::currentThread(), that);
	dictMutex->unlock();

	that->run();

	dictMutex->lock();

	QThread *there = thrDict->find(QThread::currentThread());
	if (there) {
	    there->d->running = FALSE;
	    there->d->finished = TRUE;

	    there->d->thread_done.wakeAll();
	}

	thrDict->remove(QThread::currentThread());
	dictMutex->unlock();
    }
};


class QWaitConditionPrivate {
public:
    pthread_cond_t cond;
    QMutex mutex;

    QWaitConditionPrivate()
    {
	pthread_condattr_t cattr;
	pthread_condattr_init(&cattr);

#ifdef CHECK_RANGE
	int ret =
#endif
	    pthread_cond_init(&cond, &cattr);

	pthread_condattr_destroy(&cattr);

#ifdef CHECK_RANGE
	if( ret )
	    qWarning( "QWaitCondition::QWaitCondition: event init failure %s", strerror( ret ) );
#endif
    }

    ~QWaitConditionPrivate()
    {
	int ret = pthread_cond_destroy(&cond);
	if( ret ) {
#ifdef CHECK_RANGE
	    qWarning( "QWaitCondition::~QWaitCondition: event destroy failure %s", strerror( ret ) );
#endif

	    // seems we have threads waiting on us, lets wake them up
	    pthread_cond_broadcast(&cond);
	}
    }

    void wakeOne()
    {
	mutex.lock();

#ifdef CHECK_RANGE
	int ret =
#endif
	    pthread_cond_signal( &(cond) );

#ifdef CHECK_RANGE
	if ( ret ) qWarning("QWaitCondition::wakeOne: wake error: %s",strerror(ret));
#endif

	mutex.unlock();
    }

    void wakeAll()
    {
	mutex.lock();

#ifdef CHECK_RANGE
	int ret =
#endif
	    pthread_cond_broadcast(&(cond));

#ifdef CHECK_RANGE
	if( ret ) qWarning("QWaitCondition::wakeAll: wake error: %s",strerror(ret));
#endif

	mutex.unlock();
    }

    bool wait(unsigned long time)
    {
	mutex.lock();

	int ret;
	if (time != ULONG_MAX) {
	    timespec ti;
	    ti.tv_sec = (time / 1000);
	    ti.tv_nsec = (time % 1000) * 1000000;

	    ret = pthread_cond_timedwait(&(cond), &(mutex.d->mutex), &ti);
	} else {
	    ret = pthread_cond_wait ( &(cond), &(mutex.d->mutex) );
	}

	mutex.unlock();

#ifdef CHECK_RANGE
	if( ret ) qWarning("QWaitCondition::wait: wait error:%s",strerror(ret));
#endif

	return (ret == 0);
    }

    bool wait(QMutex *mtx, unsigned long time)
    {
	if (! mtx) return FALSE;

#ifdef CHECK_RANGE
	if (mtx->d->type() == Q_MUTEX_RECURSIVE)
	    qWarning("QWaitCondition::unlockAndWait: warning - using recursive mutexes"
		     " with\n                               conditions is undefined!");
#endif

#ifndef Q_HAS_RECURSIVE_MUTEX
	int c = 0;
	HANDLE id = 0;

	if (mtx->d->type() == Q_MUTEX_RECURSIVE) {
	    QRMutexPrivate *rmp = (QRMutexPrivate *) mtx->d;
	    pthread_mutex_lock(&(rmp->mutex2));

	    if (! rmp->count) {
#  ifdef CHECK_RANGE
		qWarning("QWaitCondition::unlockAndWait: recursive mutex not locked!");
#  endif

		return FALSE;
	    }

	    c = rmp->count;
	    id = rmp->owner;

	    rmp->count = 0;
	    rmp->owner = 0;

	    pthread_mutex_unlock(&(rmp->mutex2));
	}
#endif

	int ret;
	if (time != ULONG_MAX) {
	    timespec ti;
	    ti.tv_sec = (time / 1000);
	    ti.tv_nsec = (time % 1000) * 1000000;

	    ret = pthread_cond_timedwait(&(cond), &(mtx->d->mutex), &ti);
	} else {
	    ret = pthread_cond_wait ( &(cond), &(mtx->d->mutex) );
	}

#ifndef Q_HAS_RECURSIVE_MUTEX
	if (mtx->d->type() == Q_MUTEX_RECURSIVE) {
	    QRMutexPrivate *rmp = (QRMutexPrivate *) mtx->d;
	    pthread_mutex_lock(&(rmp->mutex2));
	    rmp->count = c;
	    rmp->owner = id;
	    pthread_mutex_unlock(&(rmp->mutex2));
	}
#endif

#ifdef CHECK_RANGE
	if ( ret ) qWarning("QWaitCondition::wait: wait error:%s",strerror(ret));
#endif

	return (ret == 0);
    }
};


#endif // defined(_OS_SOLARIS_)


extern "C" {
    static void *start_thread(void *t)
    {
	QThreadPrivate::internalRun( (QThread *) t );
	return 0;
    }
}


#endif // QTHREAD_P_H
