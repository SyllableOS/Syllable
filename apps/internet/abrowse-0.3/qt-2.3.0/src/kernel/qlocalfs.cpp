/****************************************************************************
** $Id$
**
** Implementation of QLocalFs class
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

#include "qlocalfs.h"

#ifndef QT_NO_NETWORKPROTOCOL

#include "qfileinfo.h"
#include "qfile.h"
#include "qurlinfo.h"
#include "qapplication.h"
#include "qurloperator.h"

//#define QLOCALFS_DEBUG

// NOT REVISED

/*!
  \class QLocalFs qlocalfs.h
  \brief Implementation of a QNetworkProtocol which works
  on the local filesystem.

  \ingroup io

  This class is a subclass of QNetworkProtocol and works
  on the local filesystem. If you want to write a network
  transparent application using QNetworkProtocol,
  QUrlOperator, etc. this class is used for accessing
  the local filesystem by QUrlOperator.

  \sa QUrlOperator, QNetworkProtocol
*/

/*!
  Constructor.
*/

QLocalFs::QLocalFs()
    : QNetworkProtocol()
{
}

/*!
  \reimp
*/

void QLocalFs::operationListChildren( QNetworkOperation *op )
{
#ifdef QLOCALFS_DEBUG
    qDebug( "QLocalFs: operationListChildren" );
#endif
    op->setState( StInProgress );

    dir = QDir( url()->path() );
    dir.setNameFilter( url()->nameFilter() );
    dir.setMatchAllDirs( TRUE );
    if ( !dir.isReadable() ) {
	QString msg = tr( "Could not read directory\n" + url()->path() );
	op->setState( StFailed );
	op->setProtocolDetail( msg );
	op->setErrorCode( (int)ErrListChlidren );
	emit finished( op );
	return;
    }

    const QFileInfoList *filist = dir.entryInfoList( QDir::All | QDir::Hidden );
    if ( !filist ) {
	QString msg = tr( "Could not read directory\n" + url()->path() );
	op->setState( StFailed );
	op->setProtocolDetail( msg );
	op->setErrorCode( (int)ErrListChlidren );
	emit finished( op );
	return;
    }

    emit start( op );

    QFileInfoListIterator it( *filist );
    QFileInfo *fi;
    QValueList<QUrlInfo> infos;
    while ( ( fi = it.current() ) != 0 ) {
	++it;
	infos << QUrlInfo( fi->fileName(), 0/*permissions*/, fi->owner(), fi->group(),
			   fi->size(), fi->lastModified(), fi->lastRead(), fi->isDir(), fi->isFile(),
			   fi->isSymLink(), fi->isWritable(), fi->isReadable(), fi->isExecutable() );
    }
    emit newChildren( infos, op );
    op->setState( StDone );
    emit finished( op );
}

/*!
  \reimp
*/

void QLocalFs::operationMkDir( QNetworkOperation *op )
{
#ifdef QLOCALFS_DEBUG
    qDebug( "QLocalFs: operationMkDir" );
#endif
    op->setState( StInProgress );
    QString dirname = op->arg( 0 );

    dir = QDir( url()->path() );
    if ( dir.mkdir( dirname ) ) {
	QFileInfo fi( dir, dirname );
	QUrlInfo inf( fi.fileName(), 0/*permissions*/, fi.owner(), fi.group(),
		      fi.size(), fi.lastModified(), fi.lastRead(), fi.isDir(), fi.isFile(),
		      fi.isSymLink(), fi.isWritable(), fi.isReadable(), fi.isExecutable() );
	emit newChild( inf, op );
	op->setState( StDone );
	emit createdDirectory( inf, op );
	emit finished( op );
    } else {
	QString msg = tr( "Could not create directory\n" + dirname );
	op->setState( StFailed );
	op->setProtocolDetail( msg );
	op->setErrorCode( (int)ErrMkdir );
	emit finished( op );
    }
}

/*!
  \reimp
*/

void QLocalFs::operationRemove( QNetworkOperation *op )
{
#ifdef QLOCALFS_DEBUG
    qDebug( "QLocalFs: operationRemove" );
#endif
    op->setState( StInProgress );
    QString name = QUrl( op->arg( 0 ) ).path();

    dir = QDir( url()->path() );
    if ( dir.remove( name ) ) {
	op->setState( StDone );
	emit removed( op );
	emit finished( op );
    } else {
	QString msg = tr( "Could not remove file or directory\n" + name );
	op->setState( StFailed );
	op->setProtocolDetail( msg );
	op->setErrorCode( (int)ErrRemove );
	emit finished( op );
    }
}

/*!
  \reimp
*/

void QLocalFs::operationRename( QNetworkOperation *op )
{
#ifdef QLOCALFS_DEBUG
    qDebug( "QLocalFs: operationRename" );
#endif
    op->setState( StInProgress );
    QString oldname = op->arg( 0 );
    QString newname = op->arg( 1 );

    dir = QDir( url()->path() );
    if ( dir.rename( oldname, newname ) ) {
	op->setState( StDone );
	emit itemChanged( op );
	emit finished( op );
    } else {
	QString msg = tr( "Could not rename\n%1\nto\n%2" ).arg( oldname ).arg( newname );
	op->setState( StFailed );
	op->setProtocolDetail( msg );
	op->setErrorCode( (int)ErrRename );
	emit finished( op );
    }
}

/*!
  \reimp
*/

void QLocalFs::operationGet( QNetworkOperation *op )
{
#ifdef QLOCALFS_DEBUG
    qDebug( "QLocalFs: operationGet" );
#endif
    op->setState( StInProgress );
    QString from = QUrl( op->arg( 0 ) ).path();

    QFile f( from );
    if ( !f.open( IO_ReadOnly ) ) {
#ifdef QLOCALFS_DEBUG
	qDebug( "QLocalFs: could not open %s", from.latin1() );
#endif
	QString msg = tr( "Could not open\n%1" ).arg( from );
	op->setState( StFailed );
	op->setProtocolDetail( msg );
	op->setErrorCode( (int)ErrGet );
	emit finished( op );
	return;
    }

    QByteArray s;
    emit dataTransferProgress( 0, f.size(), op );
    int blockSize = calcBlockSize( f.size() );
    if ( (int)f.size() < blockSize ) {
	s.resize( f.size() );
	f.readBlock( s.data(), f.size() );
	emit data( s, op );
	emit dataTransferProgress( f.size(), f.size(), op );
#ifdef QLOCALFS_DEBUG
	qDebug( "QLocalFs: got all %d bytes at once", f.size() );
#endif
    } else {
	s.resize( blockSize );
	int remaining = f.size();
	while ( remaining > 0 ) {
	    if ( operationInProgress() != op )
		return;
	    if ( remaining >= blockSize ) {
		f.readBlock( s.data(), blockSize );
		emit data( s, op );
		emit dataTransferProgress( f.size() - remaining, f.size(), op );
		remaining -= blockSize;
	    } else {
		s.resize( remaining );
		f.readBlock( s.data(), remaining );
		emit data( s, op );
		emit dataTransferProgress( f.size() - remaining, f.size(), op );
		remaining -= remaining;
	    }
	    qApp->processEvents();
	}
#ifdef QLOCALFS_DEBUG
	qDebug( "QLocalFs: got all %d bytes step by step", f.size() );
#endif
	emit dataTransferProgress( f.size(), f.size(), op );
    }
    op->setState( StDone );
    f.close();
    emit finished( op );
}

/*!
  \reimp
*/

void QLocalFs::operationPut( QNetworkOperation *op )
{
#ifdef QLOCALFS_DEBUG
    qDebug( "QLocalFs: operationPut" );
#endif
    op->setState( StInProgress );
    QString to = QUrl( op->arg( 0 ) ).path();

    QFile f( to );
    if ( !f.open( IO_WriteOnly ) ) {
	QString msg = tr( "Could not write\n%1" ).arg( to );
	op->setState( StFailed );
	op->setProtocolDetail( msg );
	op->setErrorCode( (int)ErrPut );
	emit finished( op );
	return;
    }

    QByteArray ba( op->rawArg( 1 ) );
    emit dataTransferProgress( 0, ba.size(), op );
    int blockSize = calcBlockSize( ba.size() );
    if ( (int)ba.size() < blockSize ) {
	f.writeBlock( ba.data(), ba.size() );
	emit dataTransferProgress( ba.size(), ba.size(), op );
    } else {
	int i = 0;
	while ( i + blockSize < (int)ba.size() - 1 ) {
	    if ( operationInProgress() != op )
		return;
	    f.writeBlock( &ba.data()[ i ], blockSize );
	    f.flush();
	    emit dataTransferProgress( i + blockSize, ba.size(), op );
	    i += blockSize;
	    qApp->processEvents();
	}
	if ( i < (int)ba.size() - 1 )
	    f.writeBlock( &ba.data()[ i ], ba.size() - i );
	emit dataTransferProgress( ba.size(), ba.size(), op );
    }
    op->setState( StDone );
    f.close();
    emit finished( op );
}

/*!
  \reimp
*/

int QLocalFs::supportedOperations() const
{
    return OpListChildren | OpMkdir | OpRemove | OpRename | OpGet | OpPut;
}

/*!
  \internal
*/

int QLocalFs::calcBlockSize( int totalSize ) const
{
    if ( totalSize == 0 )
	return 1024;
    int s = totalSize / 100;
    // we want a block size between 1KB and 1MB
    if ( s < 1024 )
	s = 1024;
    if ( s > 1048576 )
	s = 1048576;
    return s;
}

#endif // QT_NO_NETWORKPROTOCOL
