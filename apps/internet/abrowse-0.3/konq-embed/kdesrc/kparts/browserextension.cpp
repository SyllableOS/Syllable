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


#include "browserextension.h"

#include <qtimer.h>
#include <qobjectlist.h>

#ifdef __ATHEOS__
#include <kio/job.h>
#include <util/message.h>
#include <util/messenger.h>
#include <gui/rect.h>
#include <map>
#endif // __ATHEOS__

using namespace KParts;

#ifndef __ATHEOS__
const char *OpenURLEvent::s_strOpenURLEvent = "KParts/BrowserExtension/OpenURLevent";

class OpenURLEvent::OpenURLEventPrivate
{
public:
  OpenURLEventPrivate()
  {
  }
  ~OpenURLEventPrivate()
  {
  }
};

OpenURLEvent::OpenURLEvent( ReadOnlyPart *part, const KURL &url, const URLArgs &args )
: Event( s_strOpenURLEvent ), m_part( part ), m_url( url ), m_args( args )
{
//  d = new OpenURLEventPrivate();
}

OpenURLEvent::~OpenURLEvent()
{
//  delete d;
}
#endif

namespace KParts
{

struct URLArgsPrivate
{
    QString contentType; // for POST
    QMap<QString, QString> metaData;
};

};

URLArgs::URLArgs()
{
  reload = false;
  xOffset = 0;
  yOffset = 0;
  trustedSource = false;
  d = 0L; // Let's build it on demand for now
}


URLArgs::URLArgs( bool _reload, int _xOffset, int _yOffset, const QString &_serviceType )
{
  reload = _reload;
  xOffset = _xOffset;
  yOffset = _yOffset;
  serviceType = _serviceType;
  d = 0L; // Let's build it on demand for now
}

URLArgs::URLArgs( const URLArgs &args )
{
  d = 0L;
  (*this) = args;
}

URLArgs &URLArgs::operator=(const URLArgs &args)
{
  if (this == &args) return *this;

  delete d; d= 0;

  reload = args.reload;
  xOffset = args.xOffset;
  yOffset = args.yOffset;
  serviceType = args.serviceType;
  postData = args.postData;
  frameName = args.frameName;
  docState = args.docState;
  trustedSource = args.trustedSource;

  if ( args.d )
     d = new URLArgsPrivate( * args.d );

  return *this;
}

#ifdef __ATHEOS__

URLArgs::URLArgs( const os::Message& cData )
{
    d = 0;
    std::string cStr;
    
    for ( int i = 0 ; cData.FindString( "doc_state", &cStr, i ) == 0 ; ++i ) {
	docState.append( QString::fromUtf8( cStr.c_str() ) );
    }
    os::IPoint cOffset;
    cData.FindIPoint( "offset", &cOffset );
    xOffset = cOffset.x;
    yOffset = cOffset.y;
    cData.FindString( "service_type", &cStr );
    serviceType = QString::fromUtf8( cStr.c_str() );

    const void* pData;
    size_t	nSize;
    if ( cData.FindData( "post_data", os::T_ANY_TYPE, &pData, &nSize ) == 0 ) {
	postData.resize( nSize );
	memcpy( postData.data(), pData, nSize );
    }
    cData.FindString( "frame_name", &cStr );
    frameName = QString::fromUtf8( cStr.c_str() );
    cData.FindBool( "trusted_source", &trustedSource );

    if ( cData.FindString( "content_type", &cStr ) == 0 ) {
	d = new URLArgsPrivate;
	d->contentType = QString::fromUtf8( cStr.c_str() );
	std::string cKey;
	for ( int i = 0 ; cData.FindString( "meta_data_key", &cKey, i ) == 0 ; ++i ) {
	    std::string cDataStr;
	    cData.FindString( "meta_data", &cDataStr, i );
	    d->metaData[QString::fromUtf8( cKey.c_str() )] = QString::fromUtf8(cDataStr.c_str() );
	}
    }
}

void URLArgs::Flatten( os::Message* pcData ) const
{
    for ( QStringList::ConstIterator i = docState.begin() ; i != docState.end() ; ++i ) {
	pcData->AddString( "doc_state", (*i).utf8().data() );
    }
    pcData->AddBool( "reload", reload );
    pcData->AddIPoint( "offset", os::IPoint( xOffset, yOffset ) );
    pcData->AddString( "service_type", serviceType.utf8().data() );
    if ( postData.size() > 0 ) {
	pcData->AddData( "post_data", os::T_ANY_TYPE, postData.data(), postData.size() );
    }
    pcData->AddString( "frame_name", frameName.utf8().data() );
    pcData->AddBool( "trusted_source", trustedSource );

    if ( d != NULL ) {
	pcData->AddString( "content_type", d->contentType.utf8().data() );
	for ( QMap<QString,QString>::ConstIterator i = d->metaData.begin() ; i != d->metaData.end() ; ++i ) {
	    pcData->AddString( "meta_data_key", i.key().utf8().data() );
	    pcData->AddString( "meta_data", i.data().utf8().data() );
	}
    }
    
}

#endif



URLArgs::~URLArgs()
{
  delete d; d = 0;
}

void URLArgs::setContentType( const QString & contentType )
{
  if (!d)
    d = new URLArgsPrivate;
  d->contentType = contentType;
}

QString URLArgs::contentType() const
{
  return d ? d->contentType : QString::null;
}

QMap<QString, QString> &URLArgs::metaData()
{
  if (!d)
     d = new URLArgsPrivate;
  return d->metaData;
}

namespace KParts
{

struct WindowArgsPrivate
{
};

};

WindowArgs::WindowArgs()
{
    x = y = width = height = -1;
    fullscreen = false;
    menuBarVisible = true;
    toolBarsVisible = true;
    statusBarVisible = true;
    resizable = true;
    lowerWindow = false;
    d = 0;
}

WindowArgs::WindowArgs( const WindowArgs &args )
{
    d = 0;
    (*this) = args;
}

WindowArgs &WindowArgs::operator=( const WindowArgs &args )
{
    if ( this == &args ) return *this;

    delete d; d = 0;

    x = args.x;
    y = args.y;
    width = args.width;
    height = args.height;
    fullscreen = args.fullscreen;
    menuBarVisible = args.menuBarVisible;
    toolBarsVisible = args.toolBarsVisible;
    statusBarVisible = args.statusBarVisible;
    resizable = args.resizable;
    lowerWindow = args.lowerWindow;

    /*
    if ( args.d )
    {
      [ ... ]
    }
    */

    return *this;
}

WindowArgs::WindowArgs( const QRect &_geometry, bool _fullscreen, bool _menuBarVisible,
                        bool _toolBarsVisible, bool _statusBarVisible, bool _resizable )
{
    d = 0;
    x = _geometry.x();
    y = _geometry.y();
    width = _geometry.width();
    height = _geometry.height();
    fullscreen = _fullscreen;
    menuBarVisible = _menuBarVisible;
    toolBarsVisible = _toolBarsVisible;
    statusBarVisible = _statusBarVisible;
    resizable = _resizable;
    lowerWindow = false;
}

WindowArgs::WindowArgs( int _x, int _y, int _width, int _height, bool _fullscreen,
                        bool _menuBarVisible, bool _toolBarsVisible,
                        bool _statusBarVisible, bool _resizable )
{
    d = 0;
    x = _x;
    y = _y;
    width = _width;
    height = _height;
    fullscreen = _fullscreen;
    menuBarVisible = _menuBarVisible;
    toolBarsVisible = _toolBarsVisible;
    statusBarVisible = _statusBarVisible;
    resizable = _resizable;
    lowerWindow = false;
}

namespace KParts
{

class BrowserExtensionPrivate
{
public:
  BrowserExtensionPrivate()
  {
  }
  ~BrowserExtensionPrivate()
  {
  }

  KURL m_delayedURL;
  KParts::URLArgs m_delayedArgs;
  bool m_urlDropHandlingEnabled;

#ifdef __ATHEOS__
  BrowserCallback* m_pcCallback;
  bool		   m_bIsChild;
  std::map<KIO::Job*,DownloadStream*> m_cStreamMap;
#endif // __ATHEOS__    
};

};

BrowserExtension::BrowserExtension( KParts::ReadOnlyPart *parent,
                                    const char *name )
: QObject( parent, name), m_part( parent )
{
  d = new BrowserExtensionPrivate;
  d->m_urlDropHandlingEnabled = false;

  connect( m_part, SIGNAL( completed() ),
           this, SLOT( slotCompleted() ) );
  connect( this, SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
           this, SLOT( slotOpenURLRequest( const KURL &, const KParts::URLArgs & ) ) );

#ifdef __ATHEOS__
  d->m_pcCallback = NULL;
  d->m_bIsChild      = false;

  connect( m_part, SIGNAL( setWindowCaption(const QString&) ),
	   this, SLOT( slot_setWindowCaption(const QString&) ) );
  connect( m_part, SIGNAL( setStatusBarText(const QString&) ),
	   this, SLOT( slot_setStatusBarText(const QString&) ) );
  connect( m_part, SIGNAL( popupMenu(const QString&, const QPoint&) ),
	   this, SLOT( slot_popupMenu(const QString&, const QPoint&) ) );

  connect( this, SIGNAL( enableAction(const char*, bool) ),
	   this, SLOT( slot_enableAction(const char*, bool) ) );
//  connect( this, SIGNAL( openURLRequest( const KURL&, const KParts::URLArgs&) ),
//	   this, SLOT( slot_openURLRequest( const KURL&, const KParts::URLArgs&) ) );
  connect( this, SIGNAL( openURLRequestDelayed( const KURL&, const KParts::URLArgs&) ),
	   this, SLOT( slot_openURLRequestDelayed( const KURL&, const KParts::URLArgs&) ) );
  connect( this, SIGNAL( openURLNotify() ),
	   this, SLOT( slot_openURLNotify() ) );
  connect( this, SIGNAL( setLocationBarURL( const QString&) ),
	   this, SLOT( slot_setLocationBarURL( const QString&) ) );
  connect( this, SIGNAL( createNewWindow( const KURL&, const KParts::URLArgs&) ),
	   this, SLOT( slot_createNewWindow( const KURL&, const KParts::URLArgs&) ) );
  connect( this, SIGNAL( createNewWindow( const KURL&, const KParts::URLArgs&, const KParts::WindowArgs&, KParts::ReadOnlyPart*&) ),
	   this, SLOT( slot_createNewWindow( const KURL&, const KParts::URLArgs&, const KParts::WindowArgs&, KParts::ReadOnlyPart*&) ) );
  connect( this, SIGNAL( loadingProgress(int) ),
	   this, SLOT( slot_loadingProgress(int) ) );
  connect( this, SIGNAL( speedProgress(int) ),
	   this, SLOT( slot_speedProgress(int) ) );
  connect( this, SIGNAL( infoMessage(const QString&) ),
	   this, SLOT( slot_infoMessage(const QString&) ) );
  connect( this, SIGNAL( popupMenu(const QPoint&, const KFileItemList& ) ),
	   this, SLOT( slot_popupMenu(const QPoint&, const KFileItemList& ) ) );
  connect( this, SIGNAL( popupMenu(KXMLGUIClient*, const QPoint&, const KFileItemList&) ),
	   this, SLOT( slot_popupMenu(KXMLGUIClient*, const QPoint&, const KFileItemList&) ) );
  connect( this, SIGNAL( popupMenu(const QPoint&,const KURL&, const QString&, mode_t) ),
	   this, SLOT( slot_popupMenu(const QPoint&,const KURL&, const QString&, mode_t) ) );
  connect( this, SIGNAL( popupMenu(KXMLGUIClient*, const QPoint&, const KURL&, const QString&, mode_t) ),
	   this, SLOT( slot_popupMenu(KXMLGUIClient*, const QPoint&, const KURL&, const QString&, mode_t) ) );
  connect( this, SIGNAL( selectionInfo( const KFileItemList& ) ),
	   this, SLOT( slot_selectionInfo( const KFileItemList& ) ) );
  connect( this, SIGNAL( selectionInfo( const QString& ) ),
	   this, SLOT( slot_selectionInfo( const QString& ) ) );
  connect( this, SIGNAL( selectionInfo( const KURL::List& ) ),
	   this, SLOT( slot_selectionInfo( const KURL::List& ) ) );  
#endif // __ATHEOS__
}

BrowserExtension::~BrowserExtension()
{
  delete d;
}

void BrowserExtension::setURLArgs( const URLArgs &args )
{
  m_args = args;
}

URLArgs BrowserExtension::urlArgs()
{
  return m_args;
}

int BrowserExtension::xOffset()
{
  return 0;
}

int BrowserExtension::yOffset()
{
  return 0;
}

void BrowserExtension::saveState( QDataStream &stream )
{
  stream << m_part->url() << (Q_INT32)xOffset() << (Q_INT32)yOffset();
}

void BrowserExtension::restoreState( QDataStream &stream )
{
  KURL u;
  Q_INT32 xOfs, yOfs;
  stream >> u >> xOfs >> yOfs;

  URLArgs args( urlArgs() );
  args.xOffset = xOfs;
  args.yOffset = yOfs;

  setURLArgs( args );

  m_part->openURL( u );
}
#ifdef __ATHEOS__
void BrowserExtension::saveState( os::Message* pcBuffer )
{
    QByteArray cBuffer;
    QDataStream cStream( cBuffer, IO_WriteOnly );
    saveState( cStream );
    pcBuffer->AddData( "state", os::T_ANY_TYPE, cBuffer.data(), cBuffer.size() );
}

void BrowserExtension::restoreState( const os::Message& cBuffer )
{
    const void* pData;
    size_t	nLen;
    
    if ( cBuffer.FindData( "state", os::T_ANY_TYPE, &pData, &nLen ) >= 0 ) {
	QByteArray cBuf( nLen );
	memcpy( cBuf.data(), pData, nLen );
	QDataStream cStream( cBuf, IO_ReadOnly );

	restoreState( cStream );
    }
}
#endif // __ATHEOS__

bool BrowserExtension::isURLDropHandlingEnabled() const
{
    return d->m_urlDropHandlingEnabled;
}

void BrowserExtension::setURLDropHandlingEnabled( bool enable )
{
    d->m_urlDropHandlingEnabled = enable;
}

void BrowserExtension::slotCompleted()
{
#ifdef __ATHEOS__  
  if ( d->m_pcCallback != NULL ) {
      d->m_pcCallback->Completed();
  }
#endif  
  //empty the argument stuff, to avoid bogus/invalid values when opening a new url
  setURLArgs( URLArgs() );
}

void BrowserExtension::slotOpenURLRequest( const KURL &url, const KParts::URLArgs &args )
{
    d->m_delayedURL = url;
    d->m_delayedArgs = args;

    QTimer::singleShot( 0, this, SLOT( slotEmitOpenURLRequestDelayed() ) );
}

void BrowserExtension::slotEmitOpenURLRequestDelayed()
{
    if (d->m_delayedURL.isEmpty()) return;
    KURL u = d->m_delayedURL;
    KParts::URLArgs args = d->m_delayedArgs;
    d->m_delayedURL = KURL();
    d->m_delayedArgs = URLArgs();
    emit openURLRequestDelayed( u, args );
    // tricky: do not do anything here! (no access to member variables, etc.)
}

QMap<QCString,QCString> BrowserExtension::actionSlotMap()
{
  QMap<QCString,QCString> res;

  res.insert( "cut", SLOT( cut() ) );
  res.insert( "copy", SLOT( copy() ) );
  res.insert( "paste", SLOT( paste() ) );
  res.insert( "rename", SLOT( rename() ) );
  res.insert( "trash", SLOT( trash() ) );
  res.insert( "del", SLOT( del() ) );
  res.insert( "shred", SLOT( shred() ) );
  res.insert( "properties", SLOT( properties() ) );
  res.insert( "editMimeType", SLOT( editMimeType() ) );
  res.insert( "print", SLOT( print() ) );
  // Tricky. Those aren't actions in fact, but simply methods that a browserextension
  // can have or not. No need to return them here.
  //res.insert( "reparseConfiguration", SLOT( reparseConfiguration() ) );
  //res.insert( "refreshMimeTypes", SLOT( refreshMimeTypes() ) );
  // nothing for setSaveViewPropertiesLocally either

  return res;
}

BrowserExtension *BrowserExtension::childObject( QObject *obj )
{
    if ( !obj )
        return 0L;

    // we try to do it on our own, in hope that we are faster than
    // queryList, which looks kind of big :-)
    const QObjectList *children = obj->children();
    QObjectListIt it( *children );
    for (; it.current(); ++it )
        if ( dynamic_cast<KParts::BrowserExtension*>(it.current()) != NULL ) {
            return static_cast<KParts::BrowserExtension *>( it.current() );
	}
    return 0L;
}

namespace KParts
{

class BrowserHostExtension::BrowserHostExtensionPrivate
{
public:
  BrowserHostExtensionPrivate()
  {
  }
  ~BrowserHostExtensionPrivate()
  {
  }

  KParts::ReadOnlyPart *m_part;

};

};

BrowserHostExtension::BrowserHostExtension( KParts::ReadOnlyPart *parent, const char *name )
 : QObject( parent, name )
{
  d = new BrowserHostExtensionPrivate;
  d->m_part = parent;
}

BrowserHostExtension::~BrowserHostExtension()
{
  delete d;
}

QStringList BrowserHostExtension::frameNames() const
{
  return QStringList();
}

const QList<KParts::ReadOnlyPart> BrowserHostExtension::frames() const
{
  return QList<KParts::ReadOnlyPart>();
}

bool BrowserHostExtension::openURLInFrame( const KURL &, const KParts::URLArgs & )
{
  return false;
}

BrowserHostExtension *BrowserHostExtension::childObject( QObject *obj )
{
    if ( !obj )
        return 0L;

    // we try to do it on our own, in hope that we are faster than
    // queryList, which looks kind of big :-)
    const QObjectList *children = obj->children();
    QObjectListIt it( *children );
    for (; it.current(); ++it )
        if ( dynamic_cast<KParts::BrowserHostExtension*>(it.current()) != NULL ) {
            return static_cast<KParts::BrowserHostExtension *>( it.current() );
	}
    return 0L;
}





#ifdef __ATHEOS__
/*void BrowserExtension::SetMsgTarget( const os::Messenger& cMsgTarget )
{
    d->m_cMsgTarget = cMsgTarget;
}

os::Messenger& BrowserExtension::GetMsgTarget()
{
    return( d->m_cMsgTarget );
}*/

void BrowserExtension::SetCallback( BrowserCallback* pcCallback )
{
    d->m_pcCallback = pcCallback;
    d->m_bIsChild   = false;
}

BrowserCallback* BrowserExtension::GetCallback() const
{
    return( d->m_pcCallback );
}

void BrowserExtension::SetChildFlag( bool bIsChild )
{
    d->m_bIsChild = bIsChild;
}

void BrowserExtension::BeginDownload( KIO::TransferJob* job, const QString& cURL, const QString& cPreferredName, const QString& cMimeType )
{
    if ( d->m_pcCallback != NULL ) {
	DownloadStream* pcStream = new DownloadStream;
	
	pcStream->m_cURL	   = cURL.utf8().data();
	pcStream->m_cPreferredName = cPreferredName.utf8().data();
	pcStream->m_cMimeType	   = cMimeType.utf8().data();
	pcStream->m_pUserData      = NULL;

	QString cLength = job->queryMetaData("content-length");

	if ( cLength.isEmpty() ) {
	    pcStream->m_nContentSize   = -1;
	} else {
	    pcStream->m_nContentSize   = atoi( cLength.ascii() );
	}
	
	if ( d->m_pcCallback->BeginDownload( pcStream ) == false ) {
	    delete pcStream;
	    job->kill();
	} else {
	    connect( job, SIGNAL( result( KIO::Job * ) ),
		     this, SLOT( slotDownloadFinished( KIO::Job * ) ) );
	    connect( job, SIGNAL( data( KIO::Job*, const QByteArray &)),
		     this, SLOT( slotData( KIO::Job*, const QByteArray &)));
	    d->m_cStreamMap[job] = pcStream;
	}
    }
}

void BrowserExtension::SaveURL( const std::string& cURL, const std::string& /*cCaption*/ )
{
    KIO::TransferJob* job = KIO::get( KURL( cURL.c_str() ), false, false );

    connect( job, SIGNAL( result( KIO::Job * ) ),
	     SLOT( slotDownloadFinished( KIO::Job * ) ) );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray &)),
	     SLOT( slotData( KIO::Job*, const QByteArray &)));
    connect( job, SIGNAL(redirection(KIO::Job*, const KURL&) ),
	     SLOT( slotRedirection(KIO::Job*,const KURL&) ) );
    connect( job, SIGNAL( mimetype( KIO::Job *, const QString &)),
	     SLOT( slotMimeTypeFound(KIO::Job *, const QString &)));
    job->Start();
/*    
    if ( d->m_pcCallback != NULL ) {
	DownloadStream* pcStream = new DownloadStream;
	
	pcStream->m_cURL	   = cURL.utf8().data();
	pcStream->m_cPreferredName = cPreferredName.utf8().data();
	pcStream->m_cMimeType	   = cMimeType.utf8().data();
	pcStream->m_pUserData      = NULL;

	QString cLength = job->queryMetaData("content-length");

	if ( cLength.isEmpty() ) {
	    pcStream->m_nContentSize   = -1;
	} else {
	    pcStream->m_nContentSize   = atoi( cLength.ascii() );
	}
	
	if ( d->m_pcCallback->BeginDownload( pcStream ) == false ) {
	    delete pcStream;
	    job->kill();
	} else {
	    connect( job, SIGNAL( result( KIO::Job * ) ),
		     this, SLOT( slotDownloadFinished( KIO::Job * ) ) );
	    connect( job, SIGNAL( data( KIO::Job*, const QByteArray &)),
		     this, SLOT( slotData( KIO::Job*, const QByteArray &)));
	    d->m_cStreamMap[job] = pcStream;
	}
    }*/
}

void BrowserExtension::slotMimeTypeFound( KIO::Job* _job, const QString &type )
{
    KIO::TransferJob* job = static_cast<KIO::TransferJob*>(_job);
    
    DownloadStream* pcStream = new DownloadStream;
	
    pcStream->m_cURL	       = job->url().prettyURL().utf8().data();
    pcStream->m_cPreferredName = job->url().filename().utf8().data();
    pcStream->m_cMimeType      = type.utf8().data();
    pcStream->m_pUserData      = NULL;

    QString cLength = job->queryMetaData("content-length");

    if ( cLength.isEmpty() ) {
	pcStream->m_nContentSize   = -1;
    } else {
	pcStream->m_nContentSize   = atoi( cLength.ascii() );
    }
	
    if ( d->m_pcCallback->BeginDownload( pcStream ) == false ) {
	delete pcStream;
	job->kill();
    } else {
	d->m_cStreamMap[job] = pcStream;
    }

    
/*    disconnect( d->m_job, SIGNAL( result( KIO::Job * ) ),
		this, SLOT( slotFinished( KIO::Job * ) ) );
    disconnect( d->m_job, SIGNAL( data( KIO::Job*, const QByteArray &)),
		this, SLOT( slotData( KIO::Job*, const QByteArray &)));
    disconnect( d->m_job, SIGNAL(redirection(KIO::Job*, const KURL&) ),
		this, SLOT( slotRedirection(KIO::Job*,const KURL&) ) );
	
    BeginDownload( d->m_job, d->m_workingURL.prettyURL(), d->m_workingURL.filename(), type );*/
}

void BrowserExtension::slotData( KIO::Job* job, const QByteArray &data )
{
    std::map<KIO::Job*,DownloadStream*>::iterator i = d->m_cStreamMap.find( job );
    if ( i != d->m_cStreamMap.end() ) {
	DownloadStream* pcStream = (*i).second;
	
	if ( d->m_pcCallback == NULL || d->m_pcCallback->HandleDownloadData( pcStream, data.data(), data.size() ) == false ) {
	    disconnect( job, SIGNAL( result( KIO::Job * ) ),
		     this, SLOT( slotDownloadFinished( KIO::Job * ) ) );
	    disconnect( job, SIGNAL( data( KIO::Job*, const QByteArray &)),
		     this, SLOT( slotData( KIO::Job*, const QByteArray &)));
	    
	    static_cast<KIO::TransferJob*>(job)->kill();
	    d->m_cStreamMap.erase( i );
	    delete pcStream;
	}
    } else {
	disconnect( job, SIGNAL( result( KIO::Job * ) ),
		    this, SLOT( slotDownloadFinished( KIO::Job * ) ) );
	disconnect( job, SIGNAL( data( KIO::Job*, const QByteArray &)),
		    this, SLOT( slotData( KIO::Job*, const QByteArray &)));
	static_cast<KIO::TransferJob*>(job)->kill();
    }
}

void BrowserExtension::slotDownloadFinished(  KIO::Job* job )
{
    std::map<KIO::Job*,DownloadStream*>::iterator i = d->m_cStreamMap.find( job );
    if ( i != d->m_cStreamMap.end() ) {
	DownloadStream* pcStream = (*i).second;
	
	if ( d->m_pcCallback != NULL ) {
	    d->m_pcCallback->DownloadFinished( pcStream, 0 );
	}
	d->m_cStreamMap.erase( i );
	delete pcStream;
    }
}


void BrowserExtension::slot_setWindowCaption( const QString &caption )
{
    if ( d->m_pcCallback != NULL ) {
	d->m_pcCallback->SetWindowCaption( m_part, std::string( caption.utf8().data() ) );
    }
}

void BrowserExtension::slot_setStatusBarText( const QString &text )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->SetStatusBarText( std::string( text.utf8().data() ) );
    }
}


  /**
   * Enable or disable a standard action held by the browser.
   *
   * See class documentation for the list of standard actions.
   */
void BrowserExtension::slot_enableAction( const char * name, bool enabled )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->EnableAction( name, enabled );
    }
}

  /**
   * Ask the host (browser) to open @p url
   * To set a reload, the x and y offsets, the service type etc., fill in the
   * appropriate fields in the @p args structure.
   * Hosts should not connect to this signal but to @ref openURLRequestDelayed.
   */
//void BrowserExtension::slot_openURLRequest( const KURL &url, const KParts::URLArgs &args )
//{
//    slot_openURLRequestDelayed( url, args );
//}

  /**
   * This signal is emitted when openURLRequest is called, after a 0-seconds timer.
   * This allows the caller to terminate what it's doing first, before (usually)
   * being destroyed. Parts should never use this signal, hosts should only connect
   * to this signal.
   */
void BrowserExtension::slot_openURLRequestDelayed( const KURL &url, const KParts::URLArgs &args )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->OpenURLRequest( std::string( url.url().utf8().data() ), args );
    }
}

  /**
   * Tell the hosting browser that the part opened a new URL (which can be
   * queried via @ref KParts::Part::url().
   *
   * This helps the browser to update/create an entry in the history.
   * The part may @em not emit this signal together with @ref openURLRequest().
   * Emit @ref openURLRequest() if you want the browser to handle a URL the user
   * asked to open (from within your part/document). This signal however is
   * useful if you want to handle URLs all yourself internally, while still
   * telling the hosting browser about new opened URLs, in order to provide
   * a proper history functionality to the user.
   * An example of usage is a html rendering component which wants to emit
   * this signal when a child frame document changed its URL.
   * Conclusion: you probably want to use @ref openURLRequest() instead
   */
void BrowserExtension::slot_openURLNotify()
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->OpenURLNotify();
    }    
}

  /**
   * Update the URL shown in the browser's location bar to @p url.
   */
void BrowserExtension::slot_setLocationBarURL( const QString &url )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->SetLocationBarURL( std::string( url.utf8().data() ) );
    }
}

  /**
   * Ask the hosting browser to open a new window for the given @url.
   *
   * The @p args argument is optional additionnal information for the
   * browser,
   * @see KParts::URLArgs
   */
void BrowserExtension::slot_createNewWindow( const KURL &url, const KParts::URLArgs &args )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->CreateNewWindow( std::string( url.url().utf8().data() ), args );
    }
}

  /**
   * Ask the hosting browser to open a new window for the given @url
   * and return a reference to the content part.
   * The request for a reference to the part is only fullfilled/processed
   * if the serviceType is set in the @p args . (otherwise the request cannot be
   * processed synchroniously.
   */
void BrowserExtension::slot_createNewWindow( const KURL &url, const KParts::URLArgs &args,
					     const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	part = d->m_pcCallback->CreateNewWindow( std::string( url.url().utf8().data() ), args, windowArgs );
    }
}

  /**
   * Since the part emits the jobid in the @ref started() signal,
   * progress information is automatically displayed.
   *
   * However, if you don't use a @ref KIO::Job in the part,
   * you can use @ref loadingProgress() and @ref speedProgress()
   * to display progress information.
   */
void BrowserExtension::slot_loadingProgress( int percent )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->LoadingProgress( percent );
    }
}
  /**
   * @see loadingProgress
   */
void BrowserExtension::slot_speedProgress( int bytesPerSecond )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->SpeedProgress( bytesPerSecond );
    }
}

void BrowserExtension::slot_infoMessage( const QString& cString )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->InfoMessage( std::string( cString.utf8().data() ) );
    }
}

  /**
   * Emit this to make the browser show a standard popup menu
   * at the point @p global for the files @p items.
   */
void BrowserExtension::slot_popupMenu( const QPoint &/*global*/, const KFileItemList &/*items*/ )
{
/*    printf( "POPUP(1) %p\n", this );
    if ( d->m_pcCallback != NULL ) {
	d->m_pcCallback->PopupMenu( "", "" );
    }*/
}

  /**
   * Emit this to make the browser show a standard popup menu
   * at the point @p global for the files @p items.
   *
   * The GUI described by @p client is being merged with the popupmenu of the host
   */
void BrowserExtension::slot_popupMenu( KXMLGUIClient* /*client*/, const QPoint& /*global*/, const KFileItemList& /*items*/ )
{
/*    printf( "POPUP(2) %p\n", this );
    if ( d->m_pcCallback != NULL ) {
	d->m_pcCallback->PopupMenu( "", "" );
    }*/
}

  /**
   * Emit this to make the browser show a standard popup menu
   * at the point @p global for the given @p url.
   *
   * Give as much information
   * about this URL as possible, like the @p mimeType and the file type
   * (@p mode: S_IFREG, S_IFDIR...)
   */
void BrowserExtension::slot_popupMenu( const QPoint& /*global*/, const KURL& /*url*/,
				       const QString& /*mimeType*/, mode_t /*mode*/ )
{
/*    printf( "POPUP(3) %p\n", this );
    if ( d->m_pcCallback != NULL ) {
	d->m_pcCallback->PopupMenu( url.prettyURL().utf8().data(), mimeType.utf8().data() );
    }*/
}

  /**
   * Emit this to make the browser show a standard popup menu
   * at the point @p global for the given @p url.
   *
   * Give as much information
   * about this URL as possible, like the @p mimeType and the file type
   * (@p mode: S_IFREG, S_IFDIR...)
   * The GUI described by @p client is being merged with the popupmenu of the host
   */
void BrowserExtension::slot_popupMenu( KXMLGUIClient* /*client*/,
				       const QPoint& /*global*/, const KURL& /*url*/,
				       const QString& /*mimeType*/, mode_t /*mode*/ )
{
/*    BrowserExtension* pcSender = NULL;


    for (;;) {
	BrowserExtension* pcTmp = dynamic_cast<BrowserExtension*>(sender());
    }
    */    
/*    printf( "POPUP(4) %p, %p (%p)\n", this, m_part, sender() );
    if ( d->m_pcCallback != NULL ) {
	d->m_pcCallback->PopupMenu( url.prettyURL().utf8().data(), mimeType.utf8().data() );
    }*/
}

void BrowserExtension::slot_popupMenu( const QString& cURL, const QPoint& )
{
    if ( d->m_pcCallback != NULL ) {
	if ( cURL.isEmpty() ) {
	    d->m_pcCallback->PopupMenu( this, "" );
	} else {
	    KURL cAbsURL( m_part->url(), cURL );
	    d->m_pcCallback->PopupMenu( this, cAbsURL.prettyURL().utf8().data() );
	}
    }
}

void BrowserExtension::slot_selectionInfo( const KFileItemList& /*items*/ )
{
}

void BrowserExtension::slot_selectionInfo( const QString& text )
{
    if ( d->m_bIsChild == false && d->m_pcCallback != NULL ) {
	d->m_pcCallback->SelectionInfo( std::string( text.utf8().data() ) );
    }
}

void BrowserExtension::slot_selectionInfo( const KURL::List& /*urls*/ )
{
}
    
#endif // __ATHEOS__
    

#include "browserextension.moc"

