/****************************************************************************
** $Id$
**
** Definition of QNetworkProtocol class
**
** Created : 950429
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

#ifndef QNETWORKPROTOCOL_H
#define QNETWORKPROTOCOL_H

#ifndef QT_H
#include "qurlinfo.h"
#include <qstring.h>
#include <qdict.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qobject.h>
#include <qqueue.h>
#endif // QT_H

#ifndef QT_NO_NETWORKPROTOCOL

class QNetworkProtocol;
class QNetworkOperation;
class QTimer;
struct QNetworkProtocolPrivate;
class QUrlOperator;

class Q_EXPORT QNetworkProtocolFactoryBase
{
public:
   virtual QNetworkProtocol *createObject() = 0;

};

template< class Protocol >
class QNetworkProtocolFactory : public QNetworkProtocolFactoryBase
{
public:
    QNetworkProtocol *createObject() {
	return new Protocol;
    }

};

typedef QDict< QNetworkProtocolFactoryBase > QNetworkProtocolDict;

class Q_EXPORT QNetworkProtocol : public QObject
{
    Q_OBJECT

public:
    enum State {
	StWaiting = 0,
	StInProgress,
	StDone,
	StFailed,
	StStopped
    };

    enum Operation {
	OpListChildren = 1,
	OpMkdir = 2,
	OpRemove = 4,
	OpRename = 8,
	OpGet = 32,
	OpPut = 64
    };

    enum ConnectionState {
	ConHostFound,
	ConConnected,
	ConClosed
    };

    enum Error {
	// no error
	NoError = 0,
	// general errors
	ErrValid,
	ErrUnknownProtocol,
	ErrUnsupported,
	ErrParse,
	// errors on connect
	ErrLoginIncorrect,
	ErrHostNotFound,
	// protocol errors
	ErrListChlidren,
	ErrMkdir,
	ErrRemove,
	ErrRename,
	ErrGet,
	ErrPut,
	ErrFileNotExisting,
	ErrPermissionDenied
    };

    QNetworkProtocol();
    virtual ~QNetworkProtocol();

    virtual void setUrl( QUrlOperator *u );

    virtual void setAutoDelete( bool b, int i = 10000 );
    bool autoDelete() const;

    static void registerNetworkProtocol( const QString &protocol,
					 QNetworkProtocolFactoryBase *protocolFactory );
    static QNetworkProtocol *getNetworkProtocol( const QString &protocol );
    static bool hasOnlyLocalFileSystem();

    virtual int supportedOperations() const;
    virtual void addOperation( QNetworkOperation *op );

    QUrlOperator *url() const;
    QNetworkOperation *operationInProgress() const;
    virtual void clearOperationQueue();
    virtual void stop();

signals:
    void data( const QByteArray &, QNetworkOperation *res );
    void connectionStateChanged( int state, const QString &data );
    void finished( QNetworkOperation *res );
    void start( QNetworkOperation *res );
    void newChildren( const QValueList<QUrlInfo> &, QNetworkOperation *res );
    void newChild( const QUrlInfo &, QNetworkOperation *res );
    void createdDirectory( const QUrlInfo &, QNetworkOperation *res );
    void removed( QNetworkOperation *res );
    void itemChanged( QNetworkOperation *res );
    void dataTransferProgress( int bytesDone, int bytesTotal, QNetworkOperation *res );

protected:
    virtual void processOperation( QNetworkOperation *op );
    virtual void operationListChildren( QNetworkOperation *op );
    virtual void operationMkDir( QNetworkOperation *op );
    virtual void operationRemove( QNetworkOperation *op );
    virtual void operationRename( QNetworkOperation *op );
    virtual void operationGet( QNetworkOperation *op );
    virtual void operationPut( QNetworkOperation *op );
    virtual bool checkConnection( QNetworkOperation *op );

private:
    QNetworkProtocolPrivate *d;

private slots:
    void processNextOperation( QNetworkOperation *old );
    void startOps();
    void emitNewChildren( const QUrlInfo &i, QNetworkOperation *op );

    void removeMe();

};

struct QNetworkOperationPrivate;
class Q_EXPORT QNetworkOperation : public QObject
{
    Q_OBJECT
    friend class QUrlOperator;

public:
    QNetworkOperation( QNetworkProtocol::Operation operation,
		    const QString &arg0, const QString &arg1,
		    const QString &arg2 );
    QNetworkOperation( QNetworkProtocol::Operation operation,
		    const QByteArray &arg0, const QByteArray &arg1,
		    const QByteArray &arg2 );
    ~QNetworkOperation();

    void setState( QNetworkProtocol::State state );
    void setProtocolDetail( const QString &detail );
    void setErrorCode( int ec );
    void setArg( int num, const QString &arg );
    void setRawArg( int num, const QByteArray &arg );

    QNetworkProtocol::Operation operation() const;
    QNetworkProtocol::State state() const;
    QString arg( int num ) const;
    QByteArray rawArg( int num ) const;
    QString protocolDetail() const;
    int errorCode() const;

    void free();

private slots:
    void deleteMe();

private:
    QByteArray &raw( int num ) const;

    QNetworkOperationPrivate *d;

};

#endif // QT_NO_NETWORKPROTOCOL

#endif // QNETWORKPROTOCOL_H
