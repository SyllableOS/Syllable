/****************************************************************************
** $Id$
**
** Definition of QSessionManager class
**
** Created : 990510
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

#ifndef QSESSIONMANAGER_H
#define QSESSIONMANAGER_H

#ifndef QT_H
#include "qobject.h"
#include "qwindowdefs.h"
#include "qnamespace.h"
#include "qstring.h"
#include "qstringlist.h"
#endif // QT_H
#ifndef QT_NO_SESSIONMANAGER
class QSessionManagerData;

class Q_EXPORT  QSessionManager : public QObject
{
    Q_OBJECT
    QSessionManager( QApplication *app, QString &session );
    ~QSessionManager();
public:
    QString sessionId() const;
#if defined(_WS_X11_)
    void* handle() const;
#endif

    bool allowsInteraction();
    bool allowsErrorInteraction();
    void release();

    void cancel();

    enum RestartHint {
	RestartIfRunning,
	RestartAnyway,
	RestartImmediately,
	RestartNever
    };
    void setRestartHint( RestartHint );
    RestartHint restartHint() const;

    void setRestartCommand( const QStringList& );
    QStringList restartCommand() const;
    void setDiscardCommand( const QStringList& );
    QStringList discardCommand() const;

    void setProperty( const QString& name, const QString& value );
    void setProperty( const QString& name, const QStringList& value );

    bool isPhase2() const;
    void requestPhase2();

private:
    friend class QApplication;
    friend class QBaseApplication;
    QSessionManagerData* d;
};

#endif // QT_NO_SESSIONMANAGER
#endif // QSESSIONMANAGER_H
