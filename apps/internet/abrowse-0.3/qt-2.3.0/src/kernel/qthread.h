/****************************************************************************
** $Id$
**
** Definition of QThread class
**
** Created : 931107
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

#ifndef QTHREAD_H
#define QTHREAD_H

#ifndef QT_H
#include <qwindowdefs.h>
#endif // QT_H

#if defined(QT_THREAD_SUPPORT)

class QMutexPrivate;

const int Q_MUTEX_NORMAL = 0;
const int Q_MUTEX_RECURSIVE = 1;


class Q_EXPORT QMutex : public Qt
{
    friend class QWaitCondition;
    friend class QWaitConditionPrivate;

public:
    QMutex(bool recursive = FALSE);
    virtual ~QMutex();

    void lock();
    void unlock();
    bool locked();

private:
    QMutexPrivate * d;

#if defined(Q_DISABLE_COPY)
    QMutex( const QMutex & );
    QMutex &operator=( const QMutex & );
#endif
};

class QThreadPrivate;

class Q_EXPORT QThread : public Qt
{
    friend class QThreadPrivate;
public:
    static HANDLE currentThread();
    static void postEvent( QObject *,QEvent * );

    static void exit();

    QThread();
    virtual ~QThread();

    // default argument causes thread to block indefinately
    bool wait( unsigned long time = ULONG_MAX );

    void start();

    bool finished() const;
    bool running() const;


protected:
    virtual void run() = 0;

    static void sleep( unsigned long );
    static void msleep( unsigned long );
    static void usleep( unsigned long );

private:
    QThreadPrivate * d;

#if defined(Q_DISABLE_COPY)
    QThread( const QThread & );
    QThread &operator=( const QThread & );
#endif
};

class QWaitConditionPrivate;

class Q_EXPORT QWaitCondition : public Qt
{
public:
    QWaitCondition();
    virtual ~QWaitCondition();

    // default argument causes thread to block indefinately
    bool wait( unsigned long time = ULONG_MAX );
    bool wait( QMutex *mutex, unsigned long time = ULONG_MAX );

    void wakeOne();
    void wakeAll();

private:
    QWaitConditionPrivate * d;

#if defined(Q_DISABLE_COPY)
    QWaitCondition( const QWaitCondition & );
    QWaitCondition &operator=( const QWaitCondition & );
#endif
};

class QSemaphorePrivate;

class Q_EXPORT QSemaphore : public Qt
{
public:
    QSemaphore( int );
    virtual ~QSemaphore();

    int available() const;
    int total() const;

    // postfix operators
    int operator++(int);
    int operator--(int);

    int operator+=(int);
    int operator-=(int);


private:
    QSemaphorePrivate *d;

#if defined(Q_DISABLE_COPY)
    QSemaphore(const QSemaphore &);
    QSemaphore &operator=(const QSemaphore &);
#endif
};

#endif

#endif
