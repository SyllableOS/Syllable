/****************************************************************************
** $Id$
**
** Implementation of QUrlOperator class
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

#include "qurloperator.h"

#ifndef QT_NO_NETWORKPROTOCOL

#include "qurlinfo.h"
#include "qnetworkprotocol.h"
#include "qmap.h"
#include "qapplication.h"
#include "qptrdict.h"
#include "qtimer.h"

//#define QURLOPERATOR_DEBUG

struct QUrlOperatorPrivate
{
    QMap<QString, QUrlInfo> entryMap;
    QNetworkProtocol *networkProtocol;
    QString nameFilter;
    QDir dir;

    // maps needed for copy/move operations
    QPtrDict<QNetworkOperation> getOpPutOpMap;
    QPtrDict<QNetworkProtocol> getOpPutProtMap;
    QPtrDict<QNetworkProtocol> getOpGetProtMap;
    QPtrDict<QNetworkOperation> getOpRemoveOpMap;
    QNetworkProtocol *currPut;
    QStringList waitingCopies;
    QString waitingCopiesDest;
    bool waitingCopiesMove;
    QList< QNetworkOperation > oldOps;
};

// NOT REVISED
/*!
  \class QUrlOperator qurloperator.h

  \brief The QUrlOperator class provides common operations on URLs
  (get() and more).

  \ingroup misc

  This class operates on hierarchical structures (like filesystems)
  using URLs. Its API allows do all common operations on it
  (listing children, removing children, renaming, etc.). But
  the class itself contains no functionality for that. It uses
  the functionality of registered network protocols. This means,
  depending of the protocol of the URL, it uses a fitting
  network protocol class for the operations. In detail, each of
  the operation methods of QUrlOperator create a
  QNetworkOperation object which describes the operation and
  puts it into the operation queue of the used network protocol.
  If no fitting protocol could be found (because no implementation
  of the needed network protocol is registered), the url operator
  emits errors. Also not each protocol supports each operation -
  but the error handling deals with this problem.

  A QUrlOperator can be used like this (for e.g. downloading a file)
  \code
  QUrlOperator op;
  op.copy( "ftp://ftp.trolltech.com/qt/source/qt-2.1.0.tar.gz", "file:/tmp", FALSE );
  \endcode

  Now, you also will connect to some signals of the QUrlOperator to get
  informed about success, errors, progress and more things.

  Of course an implementation for the FTP protocol has to be registered for
  this example.  In the Qt Network Extension Library there is an implementation
  of the FTP protocol.

  For more information about the Qt Network Architecture take a look
  at the <a href="network.html">Qt Network Documentation</a>.

  \sa QNetworkProtocol, QNetworkOperation
*/

/*!
  \fn void QUrlOperator::newChildren( const QValueList<QUrlInfo> &i, QNetworkOperation *op )

  This signal is emitted after listChildren() was called and
  new children (e.g. files) have been read from a list of files. \a i
  holds the information about the new children.
  \a op is the pointer to the operation object, which contains all infos
  of the operation, including the state and so on.

  \sa QNetworkOperation, QNetworkProtocol
*/


/*!
  \fn void QUrlOperator::finished( QNetworkOperation *op )

  This signal is emitted when an operation of some sort finished.
  This signal is emitted always, this means on success and on failure.
  \a op is the pointer to the operation object, which contains all infos
  of the operation which has been finished, including the state and so on.
  To check if the operation was successful or not, check the state and
  error code of the operation object.

  \sa QNetworkOperation, QNetworkProtocol
*/

/*!
  \fn void QUrlOperator::start( QNetworkOperation *op )

  Some operations (like listChildren()) emit this signal
  when they start processing the operation.
  \a op is the pointer to the operation object, which contains all infos
  of the operation, including the state and so on.

  \sa QNetworkOperation, QNetworkProtocol
*/

/*!
  \fn void QUrlOperator::createdDirectory( const QUrlInfo &i, QNetworkOperation *op )

  This signal is emitted when mkdir() has been succesful
  and the directory has been created. \a i holds the information
  about the new directory.
  \a op is the pointer to the operation object, which contains all infos
  of the operation, including the state and so on and using op->arg( 0 )
  you also get the filename of the new directory.

  \sa QNetworkOperation, QNetworkProtocol
*/

/*!
  \fn void QUrlOperator::removed( QNetworkOperation *op )

  This signal is emitted when remove() has been succesful
  and the file has been removed. \a op holds the filename
  of the removed file in the first argument, you get it
  with op->arg( 0 ).

  \a op is the pointer to the operation object, which contains all infos
  of the operation, including the state and so on.

  \sa QNetworkOperation, QNetworkProtocol
*/

/*!
  \fn void QUrlOperator::itemChanged( QNetworkOperation *op )

  This signal is emitted whenever a file, which is a child of this URL,
  has been changed e.g. by successfully calling rename(). \a op holds
  the original and the new filenames in the first and second arguments.
  You get them with op->arg( 0 ) and op->arg( 1 ).

  \a op is the pointer to the operation object, which contains all infos
  of the operation, including the state and so on.

  \sa QNetworkOperation, QNetworkProtocol
*/

/*!
  \fn void QUrlOperator::data( const QByteArray &data, QNetworkOperation *op )

  This signal is emitted when new \a data has been received
  after e.g. calling get() or put(). \a op holds the name of the file which data
  is retrieved in the first argument and the data in the second argument (raw).
  You get them with op->arg( 0 ) and op->rawArg( 1 ).

  \a op is the pointer to the operation object, which contains all infos
  of the operation, including the state and so on.

  \sa QNetworkOperation, QNetworkProtocol
*/

/*!
  \fn void QUrlOperator::dataTransferProgress( int bytesDone, int bytesTotal, QNetworkOperation *op )

  When transferring data (using put() or get()) this signal is emitted during the progress.
  \a bytesDone tells how many bytes of \a bytesTotal are transferred. More information
  about the operation is stored in the \a op, the pointer to the network operation
  which is processed. \a bytesTotal may be -1, which means that the number of total
  bytes is not known.

  \sa QNetworkOperation, QNetworkProtocol
*/

/*!
  \fn void QUrlOperator::startedNextCopy( const QList<QNetworkOperation> &lst )

  This signal is emitted if copy() started a new copy operation. \a lst contains all
  QNetworkOperations which describe this copy operation.

  \sa copy()
*/

/*!
  \fn void QUrlOperator::connectionStateChanged( int state, const QString &data )

  This signal is emitted whenever the state of the connection of
  the network protocol of the url operator is changed. \a state describes the new state,
  which is one of
      QNetworkProtocol::ConHostFound,
      QNetworkProtocol::ConConnected,
      QNetworkProtocol::ConClosed
  This enum is defined in QNetworkProtocol
  \a data is a message text.
*/

/*!
  \reimp
*/

QUrlOperator::QUrlOperator()
    : QUrl()
{
    d = new QUrlOperatorPrivate;
    d->oldOps.setAutoDelete( FALSE );
    d->networkProtocol = 0;
    d->nameFilter = "*";
    d->currPut = 0;
}

/*!
  \reimp
*/

QUrlOperator::QUrlOperator( const QString &url )
    : QUrl( url )
{
    d = new QUrlOperatorPrivate;
    d->oldOps.setAutoDelete( FALSE );
    d->networkProtocol = 0;
    getNetworkProtocol();
    d->nameFilter = "*";
    d->currPut = 0;
}

/*!
  Copy constructor.
*/

QUrlOperator::QUrlOperator( const QUrlOperator& url )
    : QObject(), QUrl( url )
{
    d = new QUrlOperatorPrivate;
    *d = *url.d;
    d->oldOps.setAutoDelete( FALSE );
    d->networkProtocol = 0;
    getNetworkProtocol();
    d->nameFilter = "*";
    d->currPut = 0;
}

/*!
  \reimp
*/

QUrlOperator::QUrlOperator( const QUrlOperator& url, const QString& relUrl, bool checkSlash )
    : QUrl( url, relUrl, checkSlash )
{
    d = new QUrlOperatorPrivate;
    if ( relUrl == "." )
	*d = *url.d;
    d->oldOps.setAutoDelete( FALSE );
    d->networkProtocol = 0;
    getNetworkProtocol();
    d->currPut = 0;
}

/*!
  Destructor.
*/

QUrlOperator::~QUrlOperator()
{
    if ( !d )
	return;

    if ( d->networkProtocol )
	delete d->networkProtocol;
    while ( d->oldOps.first() ) {
	d->oldOps.first()->free();
	d->oldOps.removeFirst();
    }
    d->currPut = 0;
    delete d;
    d = 0;
}

/*!
  Starts listing the children of this URL (e.g. of a directory). The signal
  start()  is emitted, before the first entry is listed,
  and after the last one finished() is emitted.
  For each list of new entries, the newChildren()
  signals is emitted.
  If an error occurs, also the signal finished()
  is emitted, so check the state of the network operation pointer!

  As the operation will not be executed immediately, a pointer to the
  QNetworkOperation object, which is created by this method, is
  returned. This object contains all data about the operation and is
  used to refer to this operation later (e.g. in the signals which are emitted
  by the QUrlOperator). The return value can be also 0 if the operation object
  couldn't be created.

  The path of this QUrlOperator has to point to a directory, because the children
  of this directory will be listed, and not to a file, else this operation might not work!
*/

const QNetworkOperation *QUrlOperator::listChildren()
{
    if ( !checkValid() )
	return 0;

    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpListChildren,
						    QString::null, QString::null, QString::null );

    if ( d->networkProtocol &&
	 d->networkProtocol->supportedOperations() & QNetworkProtocol::OpListChildren ) {
	d->networkProtocol->addOperation( res );
	clearEntries();
	return res;
    } else {
	QString msg = tr( "The protocol `%1' is not supported\n"
			  "or `%2' doesn't support listing directories" ).
		      arg( protocol() ).arg( protocol() );
	res->setState( QNetworkProtocol::StFailed );
	res->setProtocolDetail( msg );
	res->setErrorCode( (int)QNetworkProtocol::ErrUnsupported );
	emit finished( res );
	deleteOperation( res );
    }

    return 0;
}

/*!
  Tries to create a directory (child) with the name \a dirname.
  If it has been successful a newChildren()
  signal with the new child is emitted, and the
  createdDirectory() signal with
  the information about the new child is emitted too.
  Also finished() (on success or failure) is emitted,
  after the operation has been processed, so check the state of the network
  operation object to see if the operation was successful or not.

  As the operation will not be executed immediately, a pointer to the
  QNetworkOperation object, which is created by this method, is
  returned. This object contains all data about the operation and is
  used to refer to this operation later (e.g. in the signals which are emitted
  by the QUrlOperator). The return value can be also 0 if the operation object
  couldn't be created.

  This path of this QUrlOperator has to point to a directory, as the new directory
  will be created in this path, and not to a file,
  else this operation might not work!
*/

const QNetworkOperation *QUrlOperator::mkdir( const QString &dirname )
{
    if ( !checkValid() )
	return 0;

    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpMkdir,
						    dirname, QString::null, QString::null );

    if ( d->networkProtocol &&
	 d->networkProtocol->supportedOperations() & QNetworkProtocol::OpMkdir ) {
	d->networkProtocol->addOperation( res );
	return res;
    } else {
	QString msg = tr( "The protocol `%1' is not supported\n"
			  "or `%2' doesn't support making directories" ).
		      arg( protocol() ).arg( protocol() );
	res->setState( QNetworkProtocol::StFailed );
	res->setProtocolDetail( msg );
	res->setErrorCode( (int)QNetworkProtocol::ErrUnsupported );
	emit finished( res );
	deleteOperation( res );
    }

    return 0;
}

/*!
  Tries to remove the file (child) \a filename.
  If it has been successful the signal removed() is emitted.
  Also finished() (on success or failure) is emitted after
  the operation has been processed, so check the state of the network operation
  object to see if the operation was successful or not.

  As the operation will not be executed immediately, a pointer to the
  QNetworkOperation object, which is created by this method, is
  returned. This object contains all data about the operation and is
  used to refer to this operation later (e.g. in the signals which are emitted
  by the QUrlOperator). The return value can be also 0 if the operation object
  couldn't be created.

  This path of this QUrlOperator has to point to a directory, because if \a filename
  is relative, it will be tried to remove it in this directory, and not to a file,
  else this operation might not work!
*/

const QNetworkOperation *QUrlOperator::remove( const QString &filename )
{
    if ( !checkValid() )
	return 0;

    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpRemove,
						    filename, QString::null, QString::null );

    if ( d->networkProtocol &&
	 d->networkProtocol->supportedOperations() & QNetworkProtocol::OpRemove ) {
	d->networkProtocol->addOperation( res );
	return res;
    } else {
	QString msg = tr( "The protocol `%1' is not supported\n"
			  "or `%2' doesn't support removing files or directories" ).
		      arg( protocol() ).arg( protocol() );
	res->setState( QNetworkProtocol::StFailed );
	res->setProtocolDetail( msg );
	res->setErrorCode( (int)QNetworkProtocol::ErrUnsupported );
	emit finished( res );
	deleteOperation( res );
    }

    return 0;
}

/*!
  Tries to rename the file (child) \a oldname by \a newname.
  If it has been successful the signal itemChanged()
  is emitted.
  Also finished() (on success or failure) is emitted after
  the operation has been processed, so check the state of the network operation
  object to see if the operation was successful or not.

  As the operation will not be executed immediately, a pointer to the
  QNetworkOperation object, which is created by this method, is
  returned. This object contains all data about the operation and is
  used to refer to this operation later (e.g. in the signals which are emitted
  by the QUrlOperator). The return value can be also 0 if the operation object
  couldn't be created.

  This path of this QUrlOperator has to point to a directory, as \a oldname and
  \a newname are handled relatively to this directory, and not to a file,
  else this operation might not work!
*/

const QNetworkOperation *QUrlOperator::rename( const QString &oldname, const QString &newname )
{
    if ( !checkValid() )
	return 0;

    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpRename,
						    oldname, newname, QString::null );

    if ( d->networkProtocol &&
	 d->networkProtocol->supportedOperations() & QNetworkProtocol::OpRename ) {
	d->networkProtocol->addOperation( res );
	return res;
    } else {
	QString msg = tr( "The protocol `%1' is not supported\n"
			  "or `%2' doesn't support renaming files or directories" ).
		      arg( protocol() ).arg( protocol() );
	res->setState( QNetworkProtocol::StFailed );
	res->setProtocolDetail( msg );
	res->setErrorCode( (int)QNetworkProtocol::ErrUnsupported );
	emit finished( res );
	deleteOperation( res );
    }

    return 0;
}

/*!
  Copies the file \a from to \a to. If \a move is TRUE,
  the file is moved (copied and removed). \a from has to point to a file and
  \a to must point to a directory (into which \a from is copied).
  The copying is done using get() and put() operations. If you want to get notified
  about the progress of the operation, connect to the dataTransferProgress()
  signal. But you have to know, that the get() and the put() operations emit
  this signal through the QUrlOperator! So, the number of transferred and total bytes
  which you get as argument in this signal isn't related to the the whole copy operation, but
  first to the get() and then to the put() operation. So always check for the type of
  the operation from which the signal comes (you get this by asking for the type of the
  QNetworkOperation pointer you also get as last argument of this signal).

  Also at the end finished() (on success or failure) is emitted,
  so check the state of the network operation object to see if the
  operation was successful or not.

  As a copy operation consists of multiple operations (get(), put() and maybe remove()
  (depending if you copy or move)) this method doesn't return a single QNetworkOperation,
  but a list of them. They are in the order get(), put(), remove(). As discussed, the third one
  (remove) is optional.

  \sa get(), put()
*/

QList<QNetworkOperation> QUrlOperator::copy( const QString &from, const QString &to, bool move )
{
#ifdef QURLOPERATOR_DEBUG
    qDebug( "QUrlOperator: copy %s %s %d", from.latin1(), to.latin1(), move );
#endif

    QList<QNetworkOperation> ops;
    ops.setAutoDelete( FALSE );

    QUrlOperator *u = new QUrlOperator( *this, from );
    QString frm = *u;

    QString file = u->fileName();
    file.prepend( "/" );

    QNetworkProtocol *gProt = QNetworkProtocol::getNetworkProtocol( u->protocol() );

    if ( !gProt || !QNetworkProtocol::getNetworkProtocol( QUrl( to ).protocol() ) )
	return ops;

    u->setPath( u->dirPath() );
    gProt->setUrl( u );

    if ( gProt && ( gProt->supportedOperations() & QNetworkProtocol::OpGet ) &&
	 ( gProt->supportedOperations() & QNetworkProtocol::OpPut ) ) {
	connect( gProt, SIGNAL( data( const QByteArray &, QNetworkOperation * ) ),
		 this, SLOT( copyGotData( const QByteArray &, QNetworkOperation * ) ) );
	connect( gProt, SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ),
		 this, SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ) );
	connect( gProt, SIGNAL( finished( QNetworkOperation * ) ),
		 this, SLOT( continueCopy( QNetworkOperation * ) ) );
	connect( gProt, SIGNAL( finished( QNetworkOperation * ) ),
		 this, SIGNAL( finished( QNetworkOperation * ) ) );
	gProt->setAutoDelete( TRUE );
	QNetworkOperation *opGet = new QNetworkOperation( QNetworkProtocol::OpGet,
							  frm, QString::null, QString::null );
	ops.append( opGet );
	gProt->addOperation( opGet );
	QNetworkOperation *opPut = new QNetworkOperation( QNetworkProtocol::OpPut, to + file,
							  QString::null, QString::null );
	ops.append( opPut );
	QUrlOperator *u2 = new QUrlOperator( to );
	QNetworkProtocol *pProt = QNetworkProtocol::getNetworkProtocol( u2->protocol() );
	pProt->setUrl( u2 );

	connect( pProt, SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ),
		 this, SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ) );
	connect( pProt, SIGNAL( finished( QNetworkOperation * ) ),
		 this, SIGNAL( finished( QNetworkOperation * ) ) );
	connect( pProt, SIGNAL( finished( QNetworkOperation * ) ),
		 this, SLOT( finishedCopy() ) );

	d->getOpPutProtMap.insert( (void*)opGet, pProt );
	d->getOpGetProtMap.insert( (void*)opGet, gProt );
	d->getOpPutOpMap.insert( (void*)opGet, opPut );

	if ( move && gProt->supportedOperations() & QNetworkProtocol::OpRemove ) {
	    QNetworkOperation *opRm = new QNetworkOperation( QNetworkProtocol::OpRemove, frm,
							     QString::null, QString::null );
	    ops.append( opRm );
	    d->getOpRemoveOpMap.insert( (void*)opGet, opRm );
	    gProt->setAutoDelete( FALSE );
	}

#ifdef QURLOPERATOR_DEBUG
	qDebug( "QUrlOperator: copy operation should start now..." );
#endif

	return ops;
    } else {
	delete gProt;
	QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpGet,
							frm, to, QString::null );
	QString msg = tr( "The protocol `%1' is not supported\n"
			  "or `%2' doesn't support copying or moving files or directories" ).
		      arg( protocol() ).arg( protocol() );
	res->setState( QNetworkProtocol::StFailed );
	res->setProtocolDetail( msg );
	res->setErrorCode( (int)QNetworkProtocol::ErrUnsupported );
	emit finished( res );
	deleteOperation( res );
    }

    return ops;
}

/*!
  Copies \a files to the directory \a dest. If \a move is TRUE,
  the files are moved and not copied. \a dest has to point to a directory.

  This method is just a convenience function of the copy method above. It
  calls the copy above for each entry in \a files one after the other. You
  don't get a result from this method, but each time a new copy is started,
  startedNextCopy() is emitted, with a list of QNetworkOperations which
  describe the new copy operation.
*/

void QUrlOperator::copy( const QStringList &files, const QString &dest,
			 bool move )
{
    d->waitingCopies = files;
    d->waitingCopiesDest = dest;
    d->waitingCopiesMove = move;

    finishedCopy();
}

/*!
  Returns TRUE if the url is a directory, else
  returns FALSE. This may not always work correctly,
  if the protocol of the URL is something else than file
  (local filesystem)! If you pass a bool as \a ok argument,
  this is set to TRUE, if the result of this method is correct
  for sure, else \a ok is set to FALSE.
*/

bool QUrlOperator::isDir( bool *ok )
{
    if ( ok )
	*ok = TRUE;
    if ( isLocalFile() ) {
	if ( QFileInfo( path() ).isDir() )
	    return TRUE;
	else
	    return FALSE;
    }

    if ( d->entryMap.contains( "." ) ) {
	return d->entryMap[ "." ].isDir();
    }
    // #### can assume that we are a directory?
    if ( ok )
	*ok = FALSE;
    return TRUE;
}

/*!
  Tells the network protocol to get data from \a location or, if this
  is QString::null, to get data from the location to which this
  URL points (see QUrl::fileName() and QUrl::encodedPathAndQuery()). What
  exactly happens then is depending on the network protocol.
  When data comes in, the data() signal
  is emitted. As it's unlikely that all the data comes in at once, multiple
  data() signals will be emitted.
  During processing the operation the dataTransferProgress() is emitted.
  Also at the end finished() (on success or failure) is emitted,
  so check the state of the network operation object to see if the
  operation was successful or not.

  Now, if \a location is QString::null, the path of this QUrlOperator should point to a file
  when you use this operation. If \a location is not empty, it can be relative (a child of
  the path to which the QUrlOperator points) or an absolute URL.

  E.g. for getting a Web page you might do something like

  \code
  QUrlOperator op( "http://www.whatever.org/cgi-bin/search.pl?cmd=Hallo" );
  op.get();
  \endcode

  But as for the most other operations it is required that the path of the
  QUrlOperator points to a directory, you could do following if you
  want e.g. download a file

  \code
  QUrlOperator op( "ftp://ftp.whatever.org/pub" );
  // do some other stuff like op.listChildren() or op.mkdir( "new Dir" )
  op.get( "a_file.txt" );
  \endcode

  This will get the data of ftp://ftp.whatever.org/pub/a_file.txt.

  But <b>never</b> do something like

  \code
  QUrlOperator op( "http://www.whatever.org/cgi-bin" );
  op.get( "search.pl?cmd=Hallo" );
  \endcode

  This means if \a location is not empty and relative, it must not
  contain any queries or references, just the name of a child. So,
  if you need to specify a query or reference do it like in the first
  example or specify the full URL (like
  http://www.whatever.org/cgi-bin/search.pl?cmd=Hallo) as \a location.

  \sa copy()
 */

const QNetworkOperation *QUrlOperator::get( const QString &location )
{
    if ( !checkValid() )
	return 0;

    QUrl u( *this );
    if ( !location.isEmpty() )
	u = QUrl( *this, location );

    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpGet,
						    u,
						    QString::null, QString::null );

    if ( d->networkProtocol &&
	 d->networkProtocol->supportedOperations() & QNetworkProtocol::OpGet ) {
	d->networkProtocol->addOperation( res );
	return res;
    } else {
	QString msg = tr( "The protocol `%1' is not supported\n"
			  "or `%2' doesn't support get" ).
		      arg( protocol() ).arg( protocol() );
	res->setState( QNetworkProtocol::StFailed );
	res->setProtocolDetail( msg );
	res->setErrorCode( (int)QNetworkProtocol::ErrUnsupported );
	emit finished( res );
	deleteOperation( res );
    }

    return 0;
}

/*!
  Tells the network protocol to put \a data to \a location, or if this is
  empty (QString::null) it puts the \a data to the location to which the
  URL points. What exactly happens is depending on the network
  protocol. Also depending on the network protocol
  after putting data some data might come back. In this case the
  data() signal is emitted.
  During processing the operation the dataTransferProgress() is emitted.
  Also at the end finished() (on success or failure) is emitted,
  so check the state of the network operation object to see if the
  operation was successful or not.

  Now, if \a location is QString::null, the path of this QUrlOperator should point to a file
  when you use this operation. If \a location is not empty, it can be relative (a child of
  the path to which the QUrlOperator points) or an absolute URL.

  E.g. for putting some data to a file you can do

  \code
  QUrlOperator op( "ftp://ftp.whatever.com/home/me/filename" );
  op.put( data );
  \endcode

  But as for the most other operations it is required that the path of the
  QUrlOperator points to a directory, you could do following if you
  want e.g. upload data to a file

  \code
  QUrlOperator op( "ftp://ftp.whatever.com/home/me" );
  // do some other stuff like op.listChildren() or op.mkdir( "new Dir" )
  op.put( data, "filename" );
  \endcode

  This will upload the data to ftp://ftp.whatever.com/home/me/filename.

  \sa copy()
 */

const QNetworkOperation *QUrlOperator::put( const QByteArray &data, const QString &location )
{
    if ( !checkValid() )
	return 0;

    QUrl u( *this );
    if ( !location.isEmpty() )
	u = QUrl( *this, location );

    QNetworkOperation *res = new QNetworkOperation( QNetworkProtocol::OpPut,
						    u, QString::null, QString::null );
    res->setRawArg( 1, data );

    if ( d->networkProtocol &&
	 d->networkProtocol->supportedOperations() & QNetworkProtocol::OpGet ) {
	d->networkProtocol->addOperation( res );
	return res;
    } else {
	QString msg = tr( "The protocol `%1' is not supported\n"
			  "or `%2' doesn't support put." ).
		      arg( protocol() ).arg( protocol() );
	res->setState( QNetworkProtocol::StFailed );
	res->setProtocolDetail( msg );
	res->setErrorCode( (int)QNetworkProtocol::ErrUnsupported );
	emit finished( res );
	deleteOperation( res );
    }

    return 0;
}

/*!
  Sets the name filter of the URL

  \sa QDir::setNameFilter()
*/

void QUrlOperator::setNameFilter( const QString &nameFilter )
{
    d->nameFilter = nameFilter;
}

/*!
  Returns the name filter of the URL

  \sa QUrlOperator::setNameFilter() QDir::nameFilter()
*/

QString QUrlOperator::nameFilter() const
{
    return d->nameFilter;
}

/*!
  Clears the cache of children.
*/

void QUrlOperator::clearEntries()
{
    d->entryMap.clear();
}

/*!
  Adds an entry to the children cache.
*/

void QUrlOperator::addEntry( const QValueList<QUrlInfo> &i )
{
    QValueList<QUrlInfo>::ConstIterator it = i.begin();
    for ( ; it != i.end(); ++it )
	d->entryMap[ ( *it ).name().stripWhiteSpace() ] = *it;
}

/*!
  Returns the URL information for the child \a entry or en
  empty QUrlInfo object of there is no information available
  about \a entry.
*/

QUrlInfo QUrlOperator::info( const QString &entry ) const
{
    if ( d->entryMap.contains( entry.stripWhiteSpace() ) ) {
	return d->entryMap[ entry.stripWhiteSpace() ];
     } else if ( entry == "." || entry == ".." ) {
	 // return a faked QUrlInfo
	 QUrlInfo inf;
	 inf.setName( entry );
	 inf.setDir( TRUE );
	 inf.setFile( FALSE );
	 inf.setSymLink( FALSE );
	 inf.setOwner( tr( "(unknown)" ) );
	 inf.setGroup( tr( "(unknown)" ) );
	 inf.setSize( 0 );
	 inf.setWritable( FALSE );
	 inf.setReadable( TRUE );
	 return inf;
     }

    return QUrlInfo();
}

/*!
  Finds a network protocol for the URL.
*/

void QUrlOperator::getNetworkProtocol()
{
    QNetworkProtocol *p = QNetworkProtocol::getNetworkProtocol( protocol() );
    if ( !p ) {
	d->networkProtocol = 0;
	return;
    }

    d->networkProtocol = (QNetworkProtocol *)p;
    d->networkProtocol->setUrl( this );
}

/*!
  Delete the currently used network protocol.
*/

void QUrlOperator::deleteNetworkProtocol()
{
    if ( d->networkProtocol )
	delete d->networkProtocol;
    d->networkProtocol = 0;
}

/*!
  \reimp
*/

void QUrlOperator::setPath( const QString& path )
{
    QUrl::setPath( path );
    if ( d->networkProtocol )
	d->networkProtocol->setUrl( this );
}

/*!
  \reimp
*/

void QUrlOperator::reset()
{
    QUrl::reset();
    if ( d->networkProtocol )
 	delete d->networkProtocol;
    d->networkProtocol = 0;
    d->nameFilter = "*";
}

/*!
  \reimp
*/

bool QUrlOperator::parse( const QString &url )
{
    // ######
    bool b = QUrl::parse( url );
    if ( !b ) {
// 	emit error( ErrParse, tr( "Error in parsing `%1'" ).arg( url ) );
	return b;
    }

    if ( d->networkProtocol )
	delete d->networkProtocol;
    getNetworkProtocol();

    return b;
}

/*!
  \reimp
 */

QUrlOperator& QUrlOperator::operator=( const QUrlOperator &url )
{
    deleteNetworkProtocol();
    QUrl::operator=( url );

    QPtrDict<QNetworkOperation> getOpPutOpMap = d->getOpPutOpMap;
    QPtrDict<QNetworkProtocol> getOpPutProtMap = d->getOpPutProtMap;
    QPtrDict<QNetworkProtocol> getOpGetProtMap = d->getOpGetProtMap;
    QPtrDict<QNetworkOperation> getOpRemoveOpMap = d->getOpRemoveOpMap;

    *d = *url.d;

    d->oldOps.setAutoDelete( FALSE );
    d->getOpPutOpMap = getOpPutOpMap;
    d->getOpPutProtMap = getOpPutProtMap;
    d->getOpGetProtMap = getOpGetProtMap;
    d->getOpRemoveOpMap = getOpRemoveOpMap;

    getNetworkProtocol();
    return *this;
}

/*!
  \reimp
*/

QUrlOperator& QUrlOperator::operator=( const QString &url )
{
    deleteNetworkProtocol();
    QUrl::operator=( url );
    d->oldOps.setAutoDelete( FALSE );
    getNetworkProtocol();
    return *this;
}

/*!
  \reimp
 */

bool QUrlOperator::cdUp()
{
    bool b = QUrl::cdUp();
    if ( d->networkProtocol )
	d->networkProtocol->setUrl( this );
    return b;
}

/*!
  \reimp
 */

bool QUrlOperator::checkValid()
{
    // ######
    if ( !isValid() ) {
	//emit error( ErrValid, tr( "The entered URL is not valid!" ) );
	return FALSE;
    } else
	return TRUE;
}


/*!
  \internal
*/

void QUrlOperator::copyGotData( const QByteArray &data_, QNetworkOperation *op )
{
#ifdef QURLOPERATOR_DEBUG
    qDebug( "QUrlOperator: copyGotData: %d new bytes", data_.size() );
#endif
    QNetworkOperation *put = d->getOpPutOpMap[ (void*)op ];
    if ( put ) {
	QByteArray &s = put->raw( 1 );
	int size = s.size();
	s.resize( size + data_.size() );
	memcpy( s.data() + size, data_.data(), data_.size() );
    }
    emit data( data_, op );
}

/*!
  \internal
*/

void QUrlOperator::continueCopy( QNetworkOperation *op )
{
    if ( op->operation() != QNetworkProtocol::OpGet )
	return;

#ifdef QURLOPERATOR_DEBUG
    qDebug( "QUrlOperator: continue copy (get finished, put will start)" );
#endif

    QNetworkOperation *put = d->getOpPutOpMap[ (void*)op ];
    QNetworkProtocol *gProt = d->getOpGetProtMap[ (void*)op ];
    QNetworkProtocol *pProt = d->getOpPutProtMap[ (void*)op ];
    QNetworkOperation *rm = d->getOpRemoveOpMap[ (void*)op ];
    d->getOpPutOpMap.take( op );
    d->getOpGetProtMap.take( op );
    d->getOpPutProtMap.take( op );
    d->getOpRemoveOpMap.take( op );
    if ( pProt )
	pProt->setAutoDelete( TRUE );
    if ( put && pProt ) {
	pProt->addOperation( put );
	d->currPut = pProt;
    }
    if ( gProt )
	gProt->setAutoDelete( TRUE );
    if ( rm && gProt )
 	gProt->addOperation( rm );
    disconnect( gProt, SIGNAL( data( const QByteArray &, QNetworkOperation * ) ),
		this, SLOT( copyGotData( const QByteArray &, QNetworkOperation * ) ) );
    disconnect( gProt, SIGNAL( finished( QNetworkOperation * ) ),
		this, SLOT( continueCopy( QNetworkOperation * ) ) );
}

/*!
  \internal
*/

void QUrlOperator::finishedCopy()
{
#ifdef QURLOPERATOR_DEBUG
    qDebug( "QUrlOperator: finished copy (finished putting)" );
#endif

    d->currPut = 0;
    if ( d->waitingCopies.isEmpty() )
	return;

    QString cp = d->waitingCopies.first();
    d->waitingCopies.remove( cp );
    QList<QNetworkOperation> lst = copy( cp, d->waitingCopiesDest, d->waitingCopiesMove );
    emit startedNextCopy( lst );
}

/*!
  Stops the current network operation which is just processed and
  removes all waiting network operations of this QUrlOperator.
*/

void QUrlOperator::stop()
{
    d->getOpPutOpMap.clear();
    d->getOpRemoveOpMap.clear();
    d->getOpGetProtMap.setAutoDelete( TRUE );
    d->getOpPutProtMap.setAutoDelete( TRUE );
    QPtrDictIterator<QNetworkProtocol> it( d->getOpPutProtMap );
    for ( ; it.current(); ++it )
	it.current()->stop();
    d->getOpPutProtMap.clear();
    it = QPtrDictIterator<QNetworkProtocol>( d->getOpGetProtMap );
    for ( ; it.current(); ++it )
	it.current()->stop();
    d->getOpGetProtMap.clear();
    if ( d->currPut ) {
	d->currPut->stop();
	delete d->currPut;
	d->currPut = 0;
    }
    d->waitingCopies.clear();
    if ( d->networkProtocol )
	d->networkProtocol->stop();
    deleteNetworkProtocol();
    getNetworkProtocol();
}

/*!
  \internal
*/

void QUrlOperator::deleteOperation( QNetworkOperation *op )
{
    if ( op )
	d->oldOps.append( op );
}

#endif // QT_NO_NETWORKPROTOCOL
