/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Simon Hausmann <hausmann@kde.org>
 *                     2000 Stefan Schimanski <1Stein@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
//#define SPEED_DEBUG
#include "khtml_part.h"

#include "khtml_factory.h"
#include "khtml_run.h"
#include "khtml_events.h"
#include "khtml_find.h"
#include "khtml_ext.h"
#include "khtml_pagecache.h"

#include "dom/dom_string.h"
#include "dom/dom_element.h"
#include "html/html_documentimpl.h"
#include "html/html_miscimpl.h"
#include "html/html_imageimpl.h"
#include "html/htmltokenizer.h"
#include "rendering/render_text.h"
#include "rendering/render_frames.h"
#include "rendering/render_image.h"
#include "misc/htmlhashes.h"
#include "misc/loader.h"
#include "xml/dom_textimpl.h"
#include "css/cssstyleselector.h"
#include "java/kjavaappletcontext.h"
using namespace DOM;

#include "khtmlview.h"
#include "decoder.h"
#include "ecma/kjs_proxy.h"
#include "kjs/object.h"
#include "khtml_settings.h"

#include <sys/types.h>
#include <assert.h>
#include <unistd.h>

#include <kglobal.h>
#ifndef __ATHEOS__
#include <kstddirs.h>
#endif // __ATHEOS__
#include <kio/job.h>
#include <kparts/historyprovider.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcharsets.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kfiledialog.h>
#include <ktrader.h>
#include <kparts/partmanager.h>
#include <kxmlgui.h>
#include <kcursor.h>
#include <kdatastream.h>
#include <ktempfile.h>
#include <kglobalsettings.h>
#include <kurldrag.h>

#ifndef __ATHEOS__
#include <kssl.h>
#include <ksslinfodlg.h>
#endif // __ATHEOS__
#include <qtextcodec.h>

#include <qstring.h>
#include <qfile.h>
#include <qclipboard.h>

#ifdef __ATHEOS__
#include <qpainter.h>
#include <util/messenger.h>
#include <util/message.h>
#include <util/string.h>
#else
#include <qapplication.h>
#include <qdragobject.h>
#include <qmetaobject.h>
#include <qtooltip.h>

#include <misc/loader_atheos.h>

#endif // __ATHEOS__    

namespace khtml
{
  struct ChildFrame
  {
      enum Type { Frame, IFrame, Object };

      ChildFrame() { m_bCompleted = false; m_frame = 0L; m_bPreloaded = false; m_type = Frame; m_bNotify = false; }

    RenderPart *m_frame;
    QGuardedPtr<KParts::ReadOnlyPart> m_part;
    QGuardedPtr<KParts::BrowserExtension> m_extension;
    QString m_serviceName;
    QString m_serviceType;
    QStringList m_services;
    bool m_bCompleted;
    QString m_name;
    KParts::URLArgs m_args;
    QGuardedPtr<KHTMLRun> m_run;
    bool m_bPreloaded;
    KURL m_workingURL;
      Type m_type;
    QStringList m_params;
    bool m_bNotify;
  };

};

class FrameList : public QValueList<khtml::ChildFrame>
{
public:
    Iterator find( const QString &name );
};

int kjs_lib_count = 0;

typedef FrameList::ConstIterator ConstFrameIt;
typedef FrameList::Iterator FrameIt;

#ifndef __ATHEOS__
class KHTMLToolTip : public QToolTip
{
public:
  KHTMLToolTip(KHTMLPart *part) : QToolTip(part->widget()), m_part(part) {};

protected:
  virtual void maybeTip(const QPoint &);

private:
  KHTMLPart *m_part;
};
#endif // __ATHEOS__

class KHTMLPartPrivate
{
public:
  KHTMLPartPrivate(QObject* parent)
  {
    m_doc = 0L;
    m_decoder = 0L;
    m_jscript = 0L;
    m_kjs_lib = 0;
    m_job = 0L;
    m_bComplete = true;
    m_bParsing = false;
    m_bReloading = false;
    m_manager = 0L;
    m_settings = new KHTMLSettings(*KHTMLFactory::defaultHTMLSettings());
    m_bClearing = false;
    m_bCleared = false;
    m_userSheet = QString::null;
    m_fontBase = 0;
    m_bDnd = true;
    m_startOffset = m_endOffset = 0;
    m_startBeforeEnd = true;
#ifndef __ATHEOS__
    m_linkCursor = KCursor::handCursor();
#endif
    m_loadedImages = 0;
    m_totalImageCount = 0;
    m_haveEncoding = false;
    m_activeFrame = 0L;
    m_findDialog = 0;
    m_ssl_in_use = false;
    m_javaContext = 0;
    m_cacheId = 0;
    m_frameNameId = 1;

    m_bJScriptForce = false;
    m_bJScriptOverride = false;
    m_bJavaForce = false;
    m_bJavaOverride = false;
    m_bPluginsForce = false;
    m_bPluginsOverride = false;

    m_metaRefreshEnabled = true;
    m_bHTTPRefresh = false;

    m_bFirstData = true;
    m_submitForm = 0;

    // inherit security settings from parent
    if( dynamic_cast<KHTMLPart*>(parent) != NULL )
    {
        KHTMLPart* part = static_cast<KHTMLPart*>(parent);
        if(part->d)
        {
            m_bJScriptForce = part->d->m_bJScriptForce;
            m_bJScriptOverride = part->d->m_bJScriptOverride;
            m_bJavaForce = part->d->m_bJavaForce;
            m_bJavaOverride = part->d->m_bJavaOverride;
            m_bPluginsForce = part->d->m_bPluginsForce;
            m_bPluginsOverride = part->d->m_bPluginsOverride;
            // Same for SSL settings
            m_ssl_in_use = part->d->m_ssl_in_use;
        }
    }

    m_focusNodeNumber = 0;
    m_focusNodeRestored = false;
  }
  ~KHTMLPartPrivate()
  {
    //no need to delete m_view here! kparts does it for us (: (Simon)
    if ( m_extension )
      delete m_extension;
    delete m_settings;
    delete m_jscript;
    if ( m_kjs_lib && !--kjs_lib_count )
      delete m_kjs_lib;
    delete m_javaContext;
  }

  FrameList m_frames;
  QValueList<khtml::ChildFrame> m_objects;

  QGuardedPtr<KHTMLView> m_view;
  KHTMLPartBrowserExtension *m_extension;
  KHTMLPartBrowserHostExtension *m_hostExtension;
  DOM::DocumentImpl *m_doc;
  khtml::Decoder *m_decoder;
  QString m_encoding;
  QFont::CharSet m_charset;
  long m_cacheId;
  QString scheduledScript;
  DOM::Node scheduledScriptNode;

  KJSProxy *m_jscript;
  KLibrary *m_kjs_lib;
  bool m_bJScriptEnabled :1;
  bool m_bJavaEnabled :1;
  bool m_bPluginsEnabled :1;
  bool m_bJScriptForce :1;
  bool m_bJScriptOverride :1;
  bool m_bJavaForce :1;
  bool m_bJavaOverride :1;
  bool m_bPluginsForce :1;
  bool m_metaRefreshEnabled :1;
  bool m_bPluginsOverride :1;
  int m_frameNameId;
  KJavaAppletContext *m_javaContext;

  KHTMLSettings *m_settings;

  KIO::TransferJob * m_job;
  QString m_kjsStatusBarText;
  QString m_kjsDefaultStatusBarText;

  // QStrings for SSL metadata
  // Note: When adding new variables don't forget to update ::saveState()/::restoreState()!
  bool m_ssl_in_use;
  QString m_ssl_peer_cert_subject,
          m_ssl_peer_cert_issuer,
          m_ssl_peer_ip,
          m_ssl_cipher,
          m_ssl_cipher_desc,
          m_ssl_cipher_version,
          m_ssl_cipher_used_bits,
          m_ssl_cipher_bits,
          m_ssl_cert_state,
          m_ssl_good_from,
          m_ssl_good_until;

  bool m_bComplete:1;
  bool m_bParsing:1;
  bool m_bReloading:1;
  bool m_haveEncoding:1;
  bool m_haveCharset:1;
  bool m_bHTTPRefresh:1;

  KURL m_workingURL;
  KURL m_baseURL;
#ifdef __ATHEOS__
  KURL m_oldURL;
#endif
  QString m_baseTarget;
  QTimer m_redirectionTimer;
#ifdef SPEED_DEBUG
  QTime m_parsetime;
#endif
  int m_delayRedirect;
  QString m_redirectURL;

#ifndef __ATHEOS__
  KAction *m_paViewDocument;
  KAction *m_paViewFrame;
  KAction *m_paSaveBackground;
  KAction *m_paSaveDocument;
  KAction *m_paSaveFrame;
  KAction *m_paSecurity;
  KSelectAction *m_paSetEncoding;
  KHTMLFontSizeAction *m_paIncFontSizes;
  KHTMLFontSizeAction *m_paDecFontSizes;
  KAction *m_paLoadImages;
  KAction *m_paFind;
  KAction *m_paPrintFrame;
  KAction *m_paSelectAll;
  KAction *m_paDebugDOMTree;
  KAction *m_paDebugRenderTree;
#endif // __ATHEOS__
  KParts::PartManager *m_manager;

  DOM::DOMString m_userSheet;
  DOM::DOMString m_userSheetUrl;

  QString m_popupMenuXML;

  int m_fontBase;

  int m_findPos;
  DOM::NodeImpl *m_findNode;

  QString m_strSelectedURL;

  struct SubmitForm
  {
    const char *submitAction;
    QString submitUrl;
    QByteArray submitFormData;
    QString submitContentType;
    QString submitBoundary;
  };

  SubmitForm *m_submitForm;

  bool m_bMousePressed;
  DOM::Node m_mousePressNode; //node under the mouse when the mouse was pressed (set in the mouse handler)

  DOM::Node m_selectionStart;
  long m_startOffset;
  DOM::Node m_selectionEnd;
  long m_endOffset;
  QString m_overURL;
  bool m_startBeforeEnd:1;
  bool m_bDnd:1;
  bool m_bFirstData:1;
  bool m_bClearing:1;
  bool m_bCleared:1;

  bool m_focusNodeRestored:1;
  int m_focusNodeNumber;

  QPoint m_dragStartPos;

#ifndef __ATHEOS__
  QCursor m_linkCursor;
#endif
  QTimer m_scrollTimer;
  unsigned long m_loadedImages;
  unsigned long m_totalImageCount;

  KHTMLFind *m_findDialog;

  struct findState
  {
    findState()
    { caseSensitive = false; direction = false; }
    QString text;
    bool caseSensitive;
    bool direction;
  };

  findState m_lastFindState;

  //QGuardedPtr<KParts::Part> m_activeFrame;
  KParts::Part * m_activeFrame;
};

namespace khtml {
    class PartStyleSheetLoader : public CachedObjectClient
    {
    public:
        PartStyleSheetLoader(KHTMLPart *part, KHTMLPartPrivate *priv, DOM::DOMString url, DocLoader *docLoader)
        {
	    m_part = part;
            m_priv = priv;
            // the "foo" is needed, so that the docloader for the empty document doesn't cancel this request.
            if (!docLoader)
                m_cachedSheet = Cache::requestStyleSheet(url, DOMString("foo"),false);
            else
                m_cachedSheet = docLoader->requestStyleSheet(url, DOMString("foo"));
            m_cachedSheet->ref( this );
        }
	virtual ~PartStyleSheetLoader()
	{
	    if ( m_cachedSheet ) m_cachedSheet->deref(this);
	}
        virtual void setStyleSheet(const DOM::DOMString &url, const DOM::DOMString &sheet)
        {
            if ( m_part )
            {
                m_priv->m_userSheet = sheet;
                m_priv->m_userSheetUrl = url;
            }
	    khtml::CSSStyleSelector::setUserStyle( sheet );
	    if ( m_part && m_priv->m_doc )
		m_priv->m_doc->applyChanges();
            delete this;
        }
        QGuardedPtr<KHTMLPart> m_part;
        KHTMLPartPrivate *m_priv;
	khtml::CachedCSSStyleSheet *m_cachedSheet;
    };
};


FrameList::Iterator FrameList::find( const QString &name )
{
    Iterator it = begin();
    Iterator e = end();

    for (; it!=e; ++it )
        if ( (*it).m_name==name )
            break;

    return it;
}


static QString splitUrlTarget(const QString &url, QString *target=0)
{
   QString result = url;
   if(url.left(7) == "target:")
   {
      KURL u(url);
      result = u.ref();
      if (target)
         *target = u.host();
   }
   return result;
}

#ifndef __ATHEOS__
void KHTMLToolTip::maybeTip(const QPoint &)
{
    DOM::NodeImpl *node = m_part->nodeUnderMouse().handle();
    if ( node && node->isElementNode() && node->hasTooltip() )
        tip( node->getRect(), static_cast<DOM::ElementImpl *>( node )->getAttribute( ATTR_TITLE ).string() );
}
#endif // __ATHEOS__

KHTMLPart::KHTMLPart( QWidget *parentWidget, const char *widgetname, QObject *parent, const char *name,
                      GUIProfile prof )
 : KParts::ReadOnlyPart( parent, name )
{
    d = 0;
    KHTMLFactory::registerPart( this );
    setInstance( KHTMLFactory::instance() ); // doesn't work inside init() for derived classes
    // Why?? :-} (Simon)
    init( new KHTMLView( this, parentWidget, widgetname ), prof );
}

KHTMLPart::KHTMLPart( KHTMLView *view, QObject *parent, const char *name, GUIProfile prof )
	: KParts::ReadOnlyPart( parent, name )
{
    d = 0;
    KHTMLFactory::registerPart( this );
    setInstance( KHTMLFactory::instance() );
    assert( view );
    init( view, prof );
}

void KHTMLPart::init( KHTMLView *view, GUIProfile prof )
{
  if ( prof == DefaultGUI )
    setXMLFile( "khtml.rc" );
  else if ( prof == BrowserViewGUI )
    setXMLFile( "khtml_browser.rc" );

  d = new KHTMLPartPrivate(parent());
  kdDebug(6050) << "KHTMLPart::init this=" << this << " d=" << d << endl;

  d->m_view = view;
  setWidget( d->m_view );

  d->m_extension = new KHTMLPartBrowserExtension( this );
  d->m_hostExtension = new KHTMLPartBrowserHostExtension( this );

#ifndef __ATHEOS__
  d->m_paLoadImages = 0;
#endif  
  d->m_bMousePressed = false;
#ifndef __ATHEOS__
  autoloadImages( KHTMLFactory::defaultHTMLSettings()->autoLoadImages() );

  d->m_paViewDocument = new KAction( i18n( "View Document Source" ), 0, this, SLOT( slotViewDocumentSource() ), actionCollection(), "viewDocumentSource" );
  d->m_paViewFrame = new KAction( i18n( "View Frame Source" ), 0, this, SLOT( slotViewFrameSource() ), actionCollection(), "viewFrameSource" );
  d->m_paSaveBackground = new KAction( i18n( "Save &Background Image As..." ), 0, this, SLOT( slotSaveBackground() ), actionCollection(), "saveBackground" );
  d->m_paSaveDocument = new KAction( i18n( "&Save As..." ), CTRL+Key_S, this, SLOT( slotSaveDocument() ), actionCollection(), "saveDocument" );
  d->m_paSaveFrame = new KAction( i18n( "Save &Frame As..." ), 0, this, SLOT( slotSaveFrame() ), actionCollection(), "saveFrame" );
  d->m_paSecurity = new KAction( i18n( "Security..." ), "unlock", 0, this, SLOT( slotSecurity() ), actionCollection(), "security" );
  d->m_paDebugRenderTree = new KAction( "print rendering tree to stdout", 0, this, SLOT( slotDebugRenderTree() ), actionCollection(), "debugRenderTree" );
  d->m_paDebugDOMTree = new KAction( "print DOM tree to stdout", 0, this, SLOT( slotDebugDOMTree() ), actionCollection(), "debugDOMTree" );
  QString foo1 = i18n("Show Images");
  QString foo2 = i18n("Show animated Images");
  QString foo3 = i18n("Stop animated Images");

  d->m_paSetEncoding = new KSelectAction( i18n( "Set &Encoding" ), 0, this, SLOT( slotSetEncoding() ), actionCollection(), "setEncoding" );
  QStringList encodings = KGlobal::charsets()->descriptiveEncodingNames();
  encodings.prepend( i18n( "Auto" ) );
  d->m_paSetEncoding->setItems( encodings );
  d->m_paSetEncoding->setCurrentItem(0);

  d->m_paIncFontSizes = new KHTMLFontSizeAction( this, true, i18n( "Increase Font Sizes" ), "viewmag+", this, SLOT( slotIncFontSizes() ), actionCollection(), "incFontSizes" );
  d->m_paDecFontSizes = new KHTMLFontSizeAction( this, false, i18n( "Decrease Font Sizes" ), "viewmag-", this, SLOT( slotDecFontSizes() ), actionCollection(), "decFontSizes" );
  d->m_paDecFontSizes->setEnabled( false );

  d->m_paFind = KStdAction::find( this, SLOT( slotFind() ), actionCollection(), "find" );

  d->m_paPrintFrame = new KAction( i18n( "Print Frame" ), "frameprint", 0, this, SLOT( slotPrintFrame() ), actionCollection(), "printFrame" );

  d->m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), actionCollection(), "selectAll" );
#endif // __ATHEOS__

  // set the default java(script) flags according to the current host.
  d->m_bJScriptEnabled = KHTMLFactory::defaultHTMLSettings()->isJavaScriptEnabled();
  d->m_bJavaEnabled = KHTMLFactory::defaultHTMLSettings()->isJavaEnabled();
  d->m_bPluginsEnabled = KHTMLFactory::defaultHTMLSettings()->isPluginsEnabled();

  QString userStyleSheet = KHTMLFactory::defaultHTMLSettings()->userStyleSheet();
  if ( !userStyleSheet.isEmpty() )
      setUserStyleSheet( KURL( userStyleSheet ) );

  connect( this, SIGNAL( completed() ),
           this, SLOT( updateActions() ) );
#ifndef __ATHEOS__ // Not sure where this signal was supposed to come from (KHS)  
  connect( this, SIGNAL( completed( bool ) ),
           this, SLOT( updateActions() ) );
#endif
  connect( this, SIGNAL( started( KIO::Job * ) ),
           this, SLOT( updateActions() ) );

#ifdef __ATHEOS__
//  printf( "Warning: KHTMLPart::init() not using locate() to find \"khtml/khtml_popupmenu.rc\"\n" );
  d->m_popupMenuXML = KXMLGUIFactory::readConfigFile( "khtml/khtml_popupmenu.rc" );
#else  
  d->m_popupMenuXML = KXMLGUIFactory::readConfigFile( locate( "data", "khtml/khtml_popupmenu.rc", KHTMLFactory::instance() ) );
#endif
  connect( khtml::Cache::loader(), SIGNAL( requestDone( const DOM::DOMString &, khtml::CachedObject *) ),
           this, SLOT( slotLoaderRequestDone( const DOM::DOMString &, khtml::CachedObject *) ) );
  connect( khtml::Cache::loader(), SIGNAL( requestFailed( const DOM::DOMString &, khtml::CachedObject *) ),
           this, SLOT( slotLoaderRequestDone( const DOM::DOMString &, khtml::CachedObject *) ) );
  findTextBegin(); //reset find variables

  connect( &d->m_redirectionTimer, SIGNAL( timeout() ),
           this, SLOT( slotRedirect() ) );
  
  d->m_view->viewport()->installEventFilter( this );
#ifdef __ATHEOS__
//  printf( "Warning: KHTMLPart::init() not creating tooltip\n" );
#else  
  new KHTMLToolTip( this );
#endif
}

KHTMLPart::~KHTMLPart()
{
  if ( d->m_findDialog )
      disconnect( d->m_findDialog, SIGNAL( destroyed() ),
                  this, SLOT( slotFindDialogDestroyed() ) );

  if ( d->m_manager )
  {
    d->m_manager->setActivePart( 0 );
    // Shouldn't we delete d->m_manager here ? (David)
    // No need to, I would say. We specify "this" as parent qobject
    // in ::partManager() (Simon)
  }

  stopAutoScroll();
  d->m_redirectionTimer.stop();
  if ( d->m_job )
    d->m_job->kill();
  
  khtml::Cache::loader()->cancelRequests( m_url.url() );

  disconnect( khtml::Cache::loader(), SIGNAL( requestDone( const DOM::DOMString &, khtml::CachedObject * ) ),
              this, SLOT( slotLoaderRequestDone( const DOM::DOMString &, khtml::CachedObject * ) ) );
  disconnect( khtml::Cache::loader(), SIGNAL( requestFailed( const DOM::DOMString &, khtml::CachedObject * ) ),
              this, SLOT( slotLoaderRequestDone( const DOM::DOMString &, khtml::CachedObject * ) ) );
  clear();

  if ( d->m_view )
  {
#ifndef __ATHEOS__
    d->m_view->hide();
    d->m_view->viewport()->hide();
#endif // __ATHEOS__
    d->m_view->m_part = 0;
  }

  delete d;
  KHTMLFactory::deregisterPart( this );
}

bool KHTMLPart::restoreURL( const KURL &url )
{
  // Save charset setting (it was already restored!)
  QFont::CharSet charset = d->m_charset;

  kdDebug( 6050 ) << "KHTMLPart::restoreURL " << url.url() << endl;

  d->m_redirectionTimer.stop();
  kdDebug( 6050 ) << "closing old URL" << endl;
  closeURL();

  d->m_bComplete = false;
  d->m_workingURL = url;

  // set the java(script) flags according to the current host.
  d->m_bJScriptEnabled = KHTMLFactory::defaultHTMLSettings()->isJavaScriptEnabled(url.host());
  d->m_bJavaEnabled = KHTMLFactory::defaultHTMLSettings()->isJavaEnabled(url.host());
  d->m_bPluginsEnabled = KHTMLFactory::defaultHTMLSettings()->isPluginsEnabled(url.host());
  d->m_haveCharset = true;
  d->m_charset = charset;
  d->m_settings->setCharset( d->m_charset );
  
  m_url = url;

  KHTMLPageCache::self()->fetchData( d->m_cacheId, this, SLOT(slotRestoreData(const QByteArray &)));

  emit started( 0L );
  return true;
}


bool KHTMLPart::openURL( const KURL &url )
{
  kdDebug( 6050 ) << "KHTMLPart::openURL " << url.url() << endl;

  d->m_redirectionTimer.stop();
#ifdef SPEED_DEBUG
  d->m_parsetime.start();
#endif

  KParts::URLArgs args( d->m_extension->urlArgs() );

  // in case we have a) no frameset, b) the url is identical with the currently
  // displayed one (except for the htmlref!) , c) the url request is not a POST
  // operation and d) the caller did not request to reload the page we try to
  // be smart and instead of reloading the whole document we just jump to the
  // request html anchor
  if ( d->m_frames.count() == 0 &&
       (url.htmlRef() != m_url.htmlRef()) &&
       urlcmp( url.url(), m_url.url(), true, true ) &&
       args.postData.size() == 0 &&
       !args.reload )
  {
    kdDebug( 6050 ) << "KHTMLPart::openURL now m_url = " << url.url() << endl;
    m_url = url;
    emit started( 0L );
    if ( !url.htmlRef().isEmpty() )
      gotoAnchor( url.htmlRef() );
    else
      d->m_view->setContentsPos( 0, 0 );

    d->m_bComplete = true;
    d->m_bParsing = false;

    kdDebug( 6050 ) << "completed..." << endl;
    emit completed();
    return true;
  }

  kdDebug( 6050 ) << "closing old URL" << endl;
  closeURL();

  d->m_bReloading = args.reload;
  if ( (args.postData.size() > 0) && (url.protocol().startsWith("http")) )
  {
      d->m_job = KIO::http_post( url, args.postData, false );
      d->m_job->addMetaData("content-type", args.contentType() );
  } else {
      d->m_job = KIO::get( url, args.reload, false );
  }

  d->m_job->addMetaData(args.metaData());
  connect( d->m_job, SIGNAL( result( KIO::Job * ) ),
           SLOT( slotFinished( KIO::Job * ) ) );
  connect( d->m_job, SIGNAL( data( KIO::Job*, const QByteArray &)),
	   SLOT( slotData( KIO::Job*, const QByteArray &)));
  connect( d->m_job, SIGNAL(redirection(KIO::Job*, const KURL&) ),
           SLOT( slotRedirection(KIO::Job*,const KURL&) ) );
#ifdef __ATHEOS__
  connect( d->m_job, SIGNAL( mimetype( KIO::Job *, const QString &)),
           SLOT( slotMimeTypeFound(KIO::Job *, const QString &)));
#endif
  d->m_bComplete = false;

  // Tell the slave where we come from (SSL or not)
  d->m_job->addMetaData( "ssl_was_in_use", d->m_ssl_in_use ? "TRUE" : "FALSE" );
  // Tell the slave to activate warnings about entering/leaving SSL
  d->m_job->addMetaData( "ssl_activate_warnings", "TRUE" );
  //kdDebug(6050) << "Activating SSL warnings" << endl;

  d->m_workingURL = url;

  // delete old status bar msg's from kjs (if it _was_ activated on last URL)
  if( d->m_bJScriptEnabled )
  {
     d->m_kjsStatusBarText = QString::null;
     d->m_kjsDefaultStatusBarText = QString::null;
  }

  // set the javascript flags according to the current url
  d->m_bJScriptEnabled = KHTMLFactory::defaultHTMLSettings()->isJavaScriptEnabled(url.host());
  d->m_bJavaEnabled = KHTMLFactory::defaultHTMLSettings()->isJavaEnabled(url.host());
  d->m_bPluginsEnabled = KHTMLFactory::defaultHTMLSettings()->isPluginsEnabled(url.host());
#ifdef __ATHEOS__
//  printf( "Warning: KHTMLPart::openURL() not resetting charset\n" );
#else  
  d->m_settings->resetCharset();
  d->m_haveCharset = false;
  d->m_charset = d->m_settings->charset();
#endif
  // initializing m_url to the new url breaks relative links when opening such a link after this call and _before_ begin() is called (when the first
  // data arrives) (Simon)
  // That has been fixed by calling setBaseURL() in begin(). (Waldo)
#ifdef __ATHEOS__
  d->m_oldURL = m_url;
#endif
  m_url = url;
  kdDebug( 6050 ) << "KHTMLPart::openURL now (before started) m_url = " << m_url.url() << endl;
#ifdef __ATHEOS__
  d->m_job->Start();
#endif
  emit started( d->m_job );
  return true;
}

bool KHTMLPart::closeURL()
{
  if ( d->m_job )
  {
    KHTMLPageCache::self()->cancelEntry(d->m_cacheId);
    d->m_job->kill();
    d->m_job = 0;
  }

  d->m_bComplete = true; // to avoid emitting completed() in slotFinishedParsing() (David)
  d->m_bReloading = false;

  KHTMLPageCache::self()->cancelFetch(this);
  if ( d->m_bParsing )
  {
    kdDebug( 6050 ) << " was still parsing... calling end " << endl;
    slotFinishedParsing();
    d->m_bParsing = false;
  }

  if ( !d->m_workingURL.isEmpty() )
  {
    // Aborted before starting to render
    kdDebug( 6050 ) << "Aborted before starting to render, reverting location bar to " << m_url.prettyURL() << endl;
    emit d->m_extension->setLocationBarURL( m_url.prettyURL() );
  }

  d->m_workingURL = KURL();

  khtml::Cache::loader()->cancelRequests( m_url.url() );
  
  // Stop any started redirections as well!! (DA)
  if ( d && d->m_redirectionTimer.isActive() )
    d->m_redirectionTimer.stop();

  // null node activated.
  emit nodeActivated(Node());
  
  return true;
}

DOM::HTMLDocument KHTMLPart::htmlDocument() const
{
  if (d->m_doc && d->m_doc->isHTMLDocument())
    return static_cast<HTMLDocumentImpl*>(d->m_doc);
  else
    return static_cast<HTMLDocumentImpl*>(0);
}

DOM::Document KHTMLPart::document() const
{
    return d->m_doc;
}


KParts::BrowserExtension *KHTMLPart::browserExtension() const
{
  return d->m_extension;
}

KHTMLView *KHTMLPart::view() const
{
  return d->m_view;
}

void KHTMLPart::enableJScript( bool enable )
{
    setJScriptEnabled( enable );
}

void KHTMLPart::setJScriptEnabled( bool enable )
{
  d->m_bJScriptForce = enable;
  d->m_bJScriptOverride = true;
}

bool KHTMLPart::jScriptEnabled() const
{
  if ( d->m_bJScriptOverride )
      return d->m_bJScriptForce;
  return d->m_bJScriptEnabled;
}

void KHTMLPart::enableMetaRefresh( bool enable )
{
  d->m_metaRefreshEnabled = enable;
}

bool KHTMLPart::metaRefreshEnabled() const
{
  return d->m_metaRefreshEnabled;
}

//extern "C" {
//  KJSProxy *kjs_html_init(KHTMLPart *khtmlpart);
//}

KJSProxy *KHTMLPart::jScript()
{
  if ( d->m_bJScriptOverride && !d->m_bJScriptForce || !d->m_bJScriptOverride && !d->m_bJScriptEnabled)
      return 0;

  if ( !d->m_jscript )
  {
    KLibrary *lib = KLibLoader::self()->library("kjs_html");
    if ( !lib )
      return 0;
    // look for plain C init function
    void *sym = lib->symbol("kjs_html_init");
    if ( !sym ) {
      delete lib;
      return 0;
    }
    typedef KJSProxy* (*initFunction)(KHTMLPart *);
    initFunction initSym = (initFunction) sym;
    d->m_jscript = (*initSym)(this);
    d->m_kjs_lib = lib;
    kjs_lib_count++;
  }

  return d->m_jscript;
}

QVariant KHTMLPart::executeScript( const QString &script )
{
    //kdDebug(6050) << "KHTMLPart::executeScript " << script << endl;
    return executeScript( DOM::Node(), script );
}

QVariant KHTMLPart::executeScript( const DOM::Node &n, const QString &script )
{
//  kdDebug(6050) << "KHTMLPart::executeScript " << script << endl;
  KJSProxy *proxy = jScript();
  if (!proxy)
    return QVariant();

  QVariant ret = proxy->evaluate( script.unicode(), script.length(), n );
  d->m_doc->updateRendering();

  kdDebug(6050) << "KHTMLPart::executeScript - done" << endl;
  return ret;
}

bool KHTMLPart::scheduleScript(const DOM::Node &n, const QString& script)
{
    //kdDebug(6050) << "KHTMLPart::scheduleScript "<< script << endl;

    d->scheduledScript = script;
    d->scheduledScriptNode = n;

    return true;
}

QVariant KHTMLPart::executeScheduledScript()
{
  if( d->scheduledScript.isEmpty() )
    return QVariant();

  //kdDebug(6050) << "executing delayed " << d->scheduledScript << endl;

  QVariant ret = executeScript( d->scheduledScriptNode, d->scheduledScript );
  d->scheduledScript = QString();
  d->scheduledScriptNode = DOM::Node();

  return ret;
}


void KHTMLPart::enableJava( bool enable )
{
  setJavaEnabled( enable );
}

void KHTMLPart::setJavaEnabled( bool enable )
{
  d->m_bJavaForce = enable;
  d->m_bJavaOverride = true;
}

bool KHTMLPart::javaEnabled() const
{
  if( d->m_bJavaOverride )
      return d->m_bJavaForce;
  return d->m_bJavaEnabled;
}

KJavaAppletContext *KHTMLPart::javaContext()
{
  return d->m_javaContext;
}

KJavaAppletContext *KHTMLPart::createJavaContext()
{
#ifdef __ATHEOS__
    printf( "Warning: KHTMLPart::createJavaContext() not implemented\n" );
    return( NULL );
#else    
  if ( !d->m_javaContext ) {
      d->m_javaContext = new KJavaAppletContext();
      connect( d->m_javaContext, SIGNAL(showStatus(const QString&)),
               this, SIGNAL(setStatusBarText(const QString&)) );
      connect( d->m_javaContext, SIGNAL(showDocument(const QString&, const QString&)),
               this, SLOT(slotShowDocument(const QString&, const QString&)) );
  }

  return d->m_javaContext;
#endif  
}

void KHTMLPart::enablePlugins( bool enable )
{
    setPluginsEnabled( enable );
}

void KHTMLPart::setPluginsEnabled( bool enable )
{
  d->m_bPluginsForce = enable;
  d->m_bPluginsOverride = true;
}

bool KHTMLPart::pluginsEnabled() const
{
  if ( d->m_bPluginsOverride )
      return d->m_bPluginsForce;
  return d->m_bPluginsEnabled;
}

void KHTMLPart::slotShowDocument( const QString &url, const QString &target )
{
  // this is mostly copied from KHTMLPart::slotChildURLRequest. The better approach
  // would be to put those functions into a single one.
  khtml::ChildFrame *child = 0;
  KParts::URLArgs args;
  args.frameName = target;

  QString frameName = args.frameName.lower();
  if ( !frameName.isEmpty() )
  {
    if ( frameName == QString::fromLatin1( "_top" ) )
    {
      emit d->m_extension->openURLRequest( url, args );
      return;
    }
    else if ( frameName == QString::fromLatin1( "_blank" ) )
    {
      emit d->m_extension->createNewWindow( url, args );
      return;
    }
    else if ( frameName == QString::fromLatin1( "_parent" ) )
    {
      KParts::URLArgs newArgs( args );
      newArgs.frameName = QString::null;

      emit d->m_extension->openURLRequest( url, newArgs );
      return;
    }
    else if ( frameName != QString::fromLatin1( "_self" ) )
    {
      khtml::ChildFrame *_frame = recursiveFrameRequest( url, args );

      if ( !_frame )
      {
        emit d->m_extension->openURLRequest( url, args );
        return;
      }

      child = _frame;
    }
  }

  // TODO: handle child target correctly! currently the script are always executed fur the parent
  if ( url.find( QString::fromLatin1( "javascript:" ), 0, false ) == 0 ) {
      executeScript( url.right( url.length() - 11) );
      return;
  }

  if ( child ) {
      requestObject( child, KURL(url), args );
  }  else if ( frameName==QString::fromLatin1("_self") ) // this is for embedded objects (via <object>) which want to replace the current document
  {
      KParts::URLArgs newArgs( args );
      newArgs.frameName = QString::null;
      emit d->m_extension->openURLRequest( KURL(url), newArgs );
  }
}

void KHTMLPart::slotDebugDOMTree()
{
    d->m_doc->printTree();
}

void KHTMLPart::slotDebugRenderTree()
{
    d->m_doc->renderer()->printTree();
}

void KHTMLPart::autoloadImages( bool enable )
{
    setAutoloadImages( enable );
}

void KHTMLPart::setAutoloadImages( bool enable )
{
  khtml::Cache::autoloadImages( enable );

  unplugActionList( "loadImages" );

#ifdef __ATHEOS__
//  printf( "Warning: KHTMLPart::autoloadImages() no support for KAction\n" );
#else
  if ( enable )
  {
    if ( d->m_paLoadImages )
      delete d->m_paLoadImages;
    d->m_paLoadImages = 0;
  }
  else if ( !d->m_paLoadImages )
    d->m_paLoadImages = new KAction( i18n( "Display Images on Page" ), "images_display", 0, this, SLOT( slotLoadImages() ), actionCollection(), "loadImages" );

  if ( d->m_paLoadImages )
  {
      QList<KAction> lst;
      lst.append( d->m_paLoadImages );
      plugActionList( "loadImages", lst );
  }
#endif

}

bool KHTMLPart::autoloadImages() const
{
  return khtml::Cache::autoloadImages();
}

void KHTMLPart::clear()
{
    kdDebug( 6090 ) << "KHTMLPart::clear() this = " << this << endl;
  if ( d->m_bCleared )
    return;
  d->m_bCleared = true;

  d->m_bClearing = true;

  {
    ConstFrameIt it = d->m_frames.begin();
    ConstFrameIt end = d->m_frames.end();
    for(; it != end; ++it )
    {
      // Stop HTMLRun jobs for frames
      if ( (*it).m_run )
        delete (*it).m_run;
    }
  }

  {
    QValueList<khtml::ChildFrame>::ConstIterator it = d->m_objects.begin();
    QValueList<khtml::ChildFrame>::ConstIterator end = d->m_objects.end();
    for(; it != end; ++it )
    {
      // Stop HTMLRun jobs for objects
      if ( (*it).m_run )
        delete (*it).m_run;
    }
  }


  findTextBegin(); // resets d->m_findNode and d->m_findPos

  d->m_mousePressNode = DOM::Node();


  if ( d->m_doc )
  {
    kdDebug( 6090 ) << "KHTMLPart::clear(): dereferencing the document" << endl;
    d->m_doc->detach();
    d->m_doc->deref();
  }
  d->m_doc = 0;

  // Moving past doc so that onUnload works.
  if ( d->m_jscript )
    d->m_jscript->clear();
  delete d->m_decoder;

  d->m_decoder = 0;

  if ( d->m_view )
    d->m_view->clear();

  {
    ConstFrameIt it = d->m_frames.begin();
    ConstFrameIt end = d->m_frames.end();
    for(; it != end; ++it )
    {
      if ( (*it).m_part )
      {
        partManager()->removePart( (*it).m_part );
        delete (KParts::ReadOnlyPart *)(*it).m_part;
      }
    }
  }

  d->m_frames.clear();
  d->m_objects.clear();

  delete d->m_javaContext;
  d->m_javaContext = 0;

  d->m_baseURL = KURL();
  d->m_baseTarget = QString::null;
  d->m_delayRedirect = 0;
  d->m_redirectURL = QString::null;
  d->m_bHTTPRefresh = false;
  d->m_bClearing = false;
  d->m_frameNameId = 1;
  d->m_bFirstData = true;

  d->m_bMousePressed = false;

  d->m_selectionStart = DOM::Node();
  d->m_selectionEnd = DOM::Node();
  d->m_startOffset = 0;
  d->m_endOffset = 0;

  d->m_totalImageCount = 0;
  d->m_loadedImages = 0;
}

bool KHTMLPart::openFile()
{
  return true;
}

DOM::HTMLDocumentImpl *KHTMLPart::docImpl() const
{
    if ( d && d->m_doc && d->m_doc->isHTMLDocument() )
        return static_cast<HTMLDocumentImpl*>(d->m_doc);
    return 0;
}

DOM::DocumentImpl *KHTMLPart::xmlDocImpl() const
{
    if ( d )
        return d->m_doc;
    return 0;
}

/*bool KHTMLPart::isSSLInUse() const
{
  return d->m_ssl_in_use;
}*/

void KHTMLPart::slotData( KIO::Job*, const QByteArray &data )
{
    //kdDebug( 6050 ) << "slotData: " << data.size() << endl;
  // The first data ?
  if ( !d->m_workingURL.isEmpty() )
  {
      //kdDebug( 6050 ) << "begin!" << endl;
    d->m_bParsing = true;

    begin( d->m_workingURL, d->m_extension->urlArgs().xOffset, d->m_extension->urlArgs().yOffset );

    if (d->m_bReloading)
        d->m_doc->setReloading();

    d->m_workingURL = KURL();

    d->m_cacheId = KHTMLPageCache::self()->createCacheEntry();

    // When the first data arrives, the metadata has just been made available

    //kdDebug( 6050 ) << "First data is arriving. Reading SSL metadata." << endl;
    d->m_ssl_in_use = (d->m_job->queryMetaData("ssl_in_use") == "TRUE");
    //kdDebug(6050) << "SSL in use ? " << d->m_ssl_in_use << endl;
#ifndef __ATHEOS__
    d->m_paSecurity->setIcon( d->m_ssl_in_use ? "lock" : "unlock" );
#endif    
    //kdDebug(6050) << "setIcon " << ( d->m_ssl_in_use ? "lock" : "unlock" ) << " done." << endl;

    // Shouldn't all of this be done only if ssl_in_use == true ? (DF)

    d->m_ssl_peer_cert_subject = d->m_job->queryMetaData("ssl_peer_cert_subject");
    d->m_ssl_peer_cert_issuer = d->m_job->queryMetaData("ssl_peer_cert_issuer");
    d->m_ssl_peer_ip = d->m_job->queryMetaData("ssl_peer_ip");
    d->m_ssl_cipher = d->m_job->queryMetaData("ssl_cipher");
    d->m_ssl_cipher_desc = d->m_job->queryMetaData("ssl_cipher_desc");
    d->m_ssl_cipher_version = d->m_job->queryMetaData("ssl_cipher_version");
    d->m_ssl_cipher_used_bits = d->m_job->queryMetaData("ssl_cipher_used_bits");
    d->m_ssl_cipher_bits = d->m_job->queryMetaData("ssl_cipher_bits");
    d->m_ssl_good_from = d->m_job->queryMetaData("ssl_good_from");
    d->m_ssl_good_until = d->m_job->queryMetaData("ssl_good_until");
    d->m_ssl_cert_state = d->m_job->queryMetaData("ssl_cert_state");
    //
    QString qData = d->m_job->queryMetaData("charset");
    
    if ( !qData.isEmpty() && !d->m_haveEncoding ) // only use information if the user didn't override the settings
    {
#ifdef __ATHEOS__
//	printf( "Warning: KHTMLPart::slotData() not setting encoder\n" );
#else	
       d->m_charset = KGlobal::charsets()->charsetForEncoding(qData);
       d->m_settings->setCharset( d->m_charset );
       d->m_settings->setScript( KGlobal::charsets()->charsetForEncoding(qData, true) );
       d->m_haveCharset = true;
       d->m_encoding = qData;
#endif // __ATHEOS__
    }

    // Support for http-refresh
    qData = d->m_job->queryMetaData("http-refresh");
    if( !qData.isEmpty() && d->m_metaRefreshEnabled )
    {
      kdDebug(6050) << "HTTP Refresh Request: " << qData << endl;
      int delay;
      int pos = qData.find( ';' );
      if ( pos == -1 )
        pos = qData.find( ',' );

      if( pos == -1 )
      {
        delay = qData.stripWhiteSpace().toInt();
          scheduleRedirection( qData.toInt(), m_url.url() );
      }
      else
      {
        delay = qData.left(pos).stripWhiteSpace().toInt();
        while ( qData[++pos] == ' ' );
        if ( qData.find( "url", pos, false ) == pos )
        {
          pos += 3;
          while (qData[pos] == ' ' || qData[pos] == '=') pos++;
        }
        qData = KURL( d->m_baseURL, qData.mid(pos) ).url();
        scheduleRedirection( delay, qData );
      }
      d->m_bHTTPRefresh = true;
    }
  }

  KHTMLPageCache::self()->addData(d->m_cacheId, data);
  write( data.data(), data.size() );
}

void KHTMLPart::slotRestoreData(const QByteArray &data )
{
  // The first data ?
  if ( !d->m_workingURL.isEmpty() )
  {
     long saveCacheId = d->m_cacheId;
     begin( d->m_workingURL, d->m_extension->urlArgs().xOffset, d->m_extension->urlArgs().yOffset );
     d->m_cacheId = saveCacheId;
     d->m_workingURL = KURL();
  }

  //kdDebug( 6050 ) << "slotRestoreData: " << data.size() << endl;
  write( data.data(), data.size() );

  if (data.size() == 0)
  {
      //kdDebug( 6050 ) << "slotRestoreData: <<end of data>>" << endl;
     // End of data.
     if ( d->m_bParsing )
     {
        end(); //will emit completed()
     }
  }
}

void KHTMLPart::slotFinished( KIO::Job* job )
{
  if (job->error())
  {
    KHTMLPageCache::self()->cancelEntry(d->m_cacheId);
    job->showErrorDialog( /*d->m_view*/ ); // TODO show the error text in this part, instead.
    d->m_job = 0L;
    emit canceled( job->errorString() );
    // TODO: what else ?
    checkCompleted();
    return;
  }
  //kdDebug( 6050 ) << "slotFinished" << endl;

  KHTMLPageCache::self()->endData(d->m_cacheId);

  if (d->m_doc && d->m_doc->docLoader() && d->m_doc->docLoader()->m_expireDate)
  {
      KIO::http_update_cache(m_url, false, d->m_doc->docLoader()->m_expireDate);
  }

  d->m_workingURL = KURL();

  d->m_job = 0L;

  if ( d->m_bParsing )
  {
    end(); //will emit completed()
  }
}

#ifdef __ATHEOS__
void KHTMLPart::slotMimeTypeFound( KIO::Job */*job*/, const QString &type )
{
    if ( strncasecmp( type.ascii(), "text/", 5 ) == 0 ) {
	return;
    } else if ( strncasecmp( type.ascii(), "image/", 6 ) == 0 ) {
	begin( d->m_workingURL, d->m_extension->urlArgs().xOffset, d->m_extension->urlArgs().yOffset );

	if (d->m_bReloading)
	    d->m_doc->setReloading();


	  // When the first data arrives, the metadata has just been made available

	  //kdDebug( 6050 ) << "First data is arriving. Reading SSL metadata." << endl;
	d->m_ssl_in_use = (d->m_job->queryMetaData("ssl_in_use") == "TRUE");
	  //kdDebug(6050) << "SSL in use ? " << d->m_ssl_in_use << endl;
#ifndef __ATHEOS__
	d->m_paSecurity->setIcon( d->m_ssl_in_use ? "lock" : "unlock" );
#endif    
	  //kdDebug(6050) << "setIcon " << ( d->m_ssl_in_use ? "lock" : "unlock" ) << " done." << endl;

	  // Shouldn't all of this be done only if ssl_in_use == true ? (DF)

	d->m_ssl_peer_cert_subject = d->m_job->queryMetaData("ssl_peer_cert_subject");
	d->m_ssl_peer_cert_issuer = d->m_job->queryMetaData("ssl_peer_cert_issuer");
	d->m_ssl_peer_ip = d->m_job->queryMetaData("ssl_peer_ip");
	d->m_ssl_cipher = d->m_job->queryMetaData("ssl_cipher");
	d->m_ssl_cipher_desc = d->m_job->queryMetaData("ssl_cipher_desc");
	d->m_ssl_cipher_version = d->m_job->queryMetaData("ssl_cipher_version");
	d->m_ssl_cipher_used_bits = d->m_job->queryMetaData("ssl_cipher_used_bits");
	d->m_ssl_cipher_bits = d->m_job->queryMetaData("ssl_cipher_bits");
	d->m_ssl_good_from = d->m_job->queryMetaData("ssl_good_from");
	d->m_ssl_good_until = d->m_job->queryMetaData("ssl_good_until");
	d->m_ssl_cert_state = d->m_job->queryMetaData("ssl_cert_state");
	  //

	QCString cURL = completeURL( d->m_workingURL.url() ).url().utf8();
	
	char zBuffer[4096];
	sprintf( zBuffer, "<body><html><img src=%s></body></html>\n", cURL.data() );
	os::String cData = zBuffer;
//	cData.Format( "<body><html><img src=%s></body></html>\n", cURL.data()/*completeURL( d->m_workingURL.url() ).url().utf8().data()*/ );
	d->m_workingURL = KURL();

	if ( d->m_job ) {
	    disconnect( d->m_job, SIGNAL( result( KIO::Job * ) ),
			this, SLOT( slotFinished( KIO::Job * ) ) );
	    disconnect( d->m_job, SIGNAL( data( KIO::Job*, const QByteArray &)),
			this, SLOT( slotData( KIO::Job*, const QByteArray &)));
	    disconnect( d->m_job, SIGNAL(redirection(KIO::Job*, const KURL&) ),
			this, SLOT( slotRedirection(KIO::Job*,const KURL&) ) );
	    
	    d->m_job->kill();
	    d->m_job = NULL;
	}
	write( cData.begin(), cData.size() );
	end();
    } else {
	disconnect( d->m_job, SIGNAL( result( KIO::Job * ) ),
		    this, SLOT( slotFinished( KIO::Job * ) ) );
	disconnect( d->m_job, SIGNAL( data( KIO::Job*, const QByteArray &)),
		    this, SLOT( slotData( KIO::Job*, const QByteArray &)));
	disconnect( d->m_job, SIGNAL(redirection(KIO::Job*, const KURL&) ),
		    this, SLOT( slotRedirection(KIO::Job*,const KURL&) ) );
	
	d->m_extension->BeginDownload( d->m_job, d->m_workingURL.prettyURL(), d->m_workingURL.filename(), type );
	d->m_job = NULL;
	m_url = d->m_oldURL;
	emit d->m_extension->setLocationBarURL( m_url.prettyURL() );
	d->m_workingURL = KURL();	
    }
}
#endif

void KHTMLPart::begin( const KURL &url, int xOffset, int yOffset )
{
  clear();
  d->m_bCleared = false;
  d->m_cacheId = 0;
  d->m_bComplete = false;

  KHTMLFactory::vLinks()->insert( url.url() );
  // ###
  //stopParser();

  KParts::URLArgs args( d->m_extension->urlArgs() );
  args.xOffset = xOffset;
  args.yOffset = yOffset;
  d->m_extension->setURLArgs( args );

  m_url = url;
  if ( !m_url.isEmpty() )
  {
    KURL::List lst = KURL::split( m_url );
    KURL baseurl;
    if ( !lst.isEmpty() )
      baseurl = *lst.begin();
    // Use this for relative links.
    // We prefer m_baseURL over m_url because m_url changes when we are
    // about to load a new page.
    setBaseURL(baseurl);

    KURL title( baseurl );
    title.setRef( QString::null );
    title.setQuery( QString::null );
    emit setWindowCaption( title.url() );
  }
  else {
    emit setWindowCaption( i18n( "* Unknown *" ) );
  }
  // ### not sure if XHTML documents served as text/xml should use DocumentImpl or HTMLDocumentImpl
  if (args.serviceType == "text/xml")
    d->m_doc = new DocumentImpl( d->m_view );
  else
    d->m_doc = new HTMLDocumentImpl( d->m_view );


  d->m_doc->ref();
  d->m_doc->attach( d->m_view );
  d->m_doc->setURL( m_url.url() );


  d->m_doc->setRestoreState(args.docState);
  d->m_doc->open();
  // clear widget
  d->m_view->resizeContents( 0, 0 );

  connect(d->m_doc,SIGNAL(finishedParsing()),this,SLOT(slotFinishedParsing()));
  
  emit d->m_extension->enableAction( "print", true );

  d->m_bParsing = true;
}

void KHTMLPart::write( const char *str, int len )
{
    if ( !d->m_decoder ) {
        d->m_decoder = new khtml::Decoder();
        if(d->m_encoding != QString::null)
            d->m_decoder->setEncoding(d->m_encoding.latin1(), d->m_haveEncoding);
        else
            d->m_decoder->setEncoding(settings()->encoding().latin1(), d->m_haveEncoding);
    }
  if ( len == 0 )
    return;

  if ( len == -1 )
    len = strlen( str );

  QString decoded = d->m_decoder->decode( str, len );
  if(decoded.isEmpty()) return;

  if(d->m_bFirstData) {
      // determine the parse mode
      d->m_doc->determineParseMode( decoded );
      d->m_bFirstData = false;

  //kdDebug(6050) << "KHTMLPart::write haveEnc = " << d->m_haveEncoding << endl;
      // ### this is still quite hacky, but should work a lot better than the old solution
      if(d->m_decoder->visuallyOrdered()) d->m_doc->setVisuallyOrdered();
      if (!d->m_haveCharset)
      {
#ifdef __ATHEOS__
//	  printf( "Warning: KHTMLPart::write() not setting encoder\n" );
#else	  
         const QTextCodec *c = d->m_decoder->codec();
         //kdDebug(6005) << "setting up charset to " << (int) KGlobal::charsets()->charsetForEncoding(c->name()) << endl;
         d->m_charset = KGlobal::charsets()->charsetForEncoding(c->name());
         d->m_settings->setCharset( d->m_charset );
         d->m_settings->setScript( KGlobal::charsets()->charsetForEncoding(c->name(), true ));
         //kdDebug(6005) << "charset is " << (int)d->m_settings->charset() << endl;
#endif // __ATHEOS__	 
      }
      d->m_doc->applyChanges(true, true);
  }

  d->m_doc->write( decoded );
}

void KHTMLPart::write( const QString &str )
{
  if ( str.isNull() )
    return;

  if(d->m_bFirstData) {
      // determine the parse mode
      d->m_doc->setParseMode( DocumentImpl::Strict );
      d->m_bFirstData = false;
  }
  d->m_doc->write( str );
}

void KHTMLPart::end()
{
    // make sure nothing's left in there...
    if(d->m_decoder)
        write(d->m_decoder->flush());
    d->m_doc->finishParsing();
}

void KHTMLPart::paint(QPainter *p, const QRect &rc, int yOff, bool *more)
{
    if (!d->m_view)
        return;
    d->m_view->paint(p, rc, yOff, more);
}

void KHTMLPart::slotFinishedParsing()
{
  d->m_bParsing = false;
  d->m_doc->close();
  disconnect(d->m_doc,SIGNAL(finishedParsing()),this,SLOT(slotFinishedParsing()));
  
  if (!d->m_view)
    return; // We are probably being destructed.
  // check if the scrollbars are really needed for the content
  // if not, remove them, relayout, and repaint

  d->m_view->restoreScrollBar();

  if ( !m_url.htmlRef().isEmpty() )
    gotoAnchor( m_url.htmlRef() );
#if 0
  HTMLCollectionImpl imgColl( d->m_doc, HTMLCollectionImpl::DOC_IMAGES );

  d->m_totalImageCount = 0;
  KURL::List imageURLs;
  unsigned long i = 0;
  unsigned long len = imgColl.length();
  for (; i < len; i++ )
  {
    NodeImpl *node = imgColl.item( i );
    if ( node->id() != ID_IMG )
      continue;

    QString imgURL = static_cast<DOM::ElementImpl *>( node )->getAttribute( ATTR_SRC ).string();
    KURL url;

    if ( KURL::isRelativeURL( imgURL ) )
      url = completeURL( imgURL );
    else
      url = KURL( imgURL );

    if ( !imageURLs.contains( url ) )
    {
      d->m_totalImageCount++;
      imageURLs.append( url );
    }
  }
#endif

  checkCompleted();
}

void KHTMLPart::slotLoaderRequestDone( const DOM::DOMString &baseURL, khtml::CachedObject *obj )
{
  if ( baseURL != m_url.url() )
    return;

  if ( obj && obj->type() == khtml::CachedObject::Image && !d->m_bParsing )
  {
    d->m_loadedImages++;

    // in case we have more images than we originally found, then they are most likely loaded by some
    // javascript code. as we can't find out the exact number anyway we skip displaying any further image
    // loading info message :P
    if ( d->m_loadedImages <= d->m_totalImageCount )
      emit d->m_extension->infoMessage( i18n( "%1 of %2 Images loaded" ).arg( d->m_loadedImages ).arg( d->m_totalImageCount ) );
  }

  checkCompleted();
}

void KHTMLPart::checkCompleted()
{
    kdDebug( 6050 ) << "KHTMLPart::checkCompleted() parsing: " << d->m_bParsing << endl;
    kdDebug( 6050 ) << "                           complete: " << d->m_bComplete << endl;

  // restore the cursor position
  if (d->m_doc && !d->m_bParsing && !d->m_focusNodeRestored)
  {
      int focusNodeNumber;
      if ((focusNodeNumber = d->m_focusNodeNumber))
      {
	  DOM::ElementImpl *focusNode = 0;
	  while(focusNodeNumber--)
	  {
	      if ((focusNode = d->m_doc->findNextLink(focusNode, true))==0)
		  break;
	  }
	  if (focusNode)
	  {
	      QRect focusRect = focusNode->getRect();
	      d->m_view->ensureVisible(focusRect.x(), focusRect.y());
	      d->m_doc->setFocusNode(focusNode);
	  }
      }
      d->m_focusNodeRestored = true;
  }

  int requests = 0;

  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
    if ( !(*it).m_bCompleted )
      return;
  requests = khtml::Cache::loader()->numRequests( m_url.url() );
  //kdDebug( 6060 ) << "number of loader requests: " << requests << endl;
  if ( requests > 0 )
    return;

  if ( d->m_bParsing || d->m_bComplete )
    return;

  d->m_bComplete = true;

  if (!parentPart()) {
    emit setStatusBarText(i18n("Done."));
  }
  //kdDebug( 6050 ) << "KHTMLPart::checkCompleted() emitting completed()"  << endl;

/*
  if ( !d->m_redirectURL.isEmpty() )
  {
    // Actually we should emit completed since the META redirections are
    // supposed to be started after the current loading is finished. Thus,
    // a new signal is needed to  (DA)
    //
    // We don't emit completed as long as we have a redirection pending.
    // We might want to add a message to the statusbar when we are not
    // i18n-frozen.
    emit completed();
    return;
  }
*/

  // check for a <link rel="SHORTCUT ICON" href="url to icon">,
  // IE extension to set an icon for this page to use in
  // bookmarks and the locationbar
  if (!parentPart() && d->m_doc && d->m_doc->isHTMLDocument())
  {
      DOM::TagNodeListImpl links(d->m_doc, "LINK");
      for (unsigned long i = 0; i < links.length(); ++i)
          if (links.item(i)->isElementNode())
          {
              DOM::ElementImpl *link = static_cast<DOM::ElementImpl *>(links.item(i));
              kdDebug(6005) << "Checking..." << endl;
              if (link->getAttribute("REL").string().upper() == "SHORTCUT ICON")
              {
                  KURL iconURL(d->m_baseURL, link->getAttribute("HREF").string());
                  if (!iconURL.isEmpty())
                  {
#ifdef __ATHEOS__
//		      printf( "Warning: KHTMLPart::checkCompleted() not emitting setIconURL()\n" );
#else
                      emit d->m_extension->setIconURL(iconURL);
#endif
                      break;
                  }
              }
          }
  }

  if ( m_url.htmlRef().isEmpty() && d->m_view->contentsY() == 0 ) // check that the view has not been moved by the user
      d->m_view->setContentsPos( d->m_extension->urlArgs().xOffset, d->m_extension->urlArgs().yOffset );

#ifndef __ATHEOS__  
  if ( !d->m_redirectURL.isEmpty() )
    emit completed( true );
  else
#endif      
    emit completed();

  emit setStatusBarText( i18n("Loading complete") );
  
#ifdef SPEED_DEBUG
  kdDebug() << "DONE: " <<d->m_parsetime.elapsed() << endl;
#endif
}

const KHTMLSettings *KHTMLPart::settings() const
{
  return d->m_settings;
}

void KHTMLPart::setBaseURL( const KURL &url )
{
  d->m_baseURL = url;
}

KURL KHTMLPart::baseURL() const
{
    if ( d->m_baseURL.isEmpty() )
	return m_url;
  return d->m_baseURL;
}

void KHTMLPart::setBaseTarget( const QString &target )
{
  d->m_baseTarget = target;
}

QString KHTMLPart::baseTarget() const
{
  return d->m_baseTarget;
}

KURL KHTMLPart::completeURL( const QString &url, const QString &/*target*/ )
{
  // WABA: The following check is necassery to fix forms which don't set
  // an action URL in the believe that it default to the same URL as
  // the current page which contains the form.
  if (url.isEmpty()) {
    return m_url;
  }

  if (d->m_baseURL.isEmpty())
  {
     return KURL( m_url, url );
  }
  else
  {
     return KURL( d->m_baseURL, url );
  }
}

void KHTMLPart::scheduleRedirection( int delay, const QString &url )
{
    if( d->m_redirectURL.isEmpty() || delay < d->m_delayRedirect )
    {
       d->m_delayRedirect = delay;
       d->m_redirectURL = url;
       if ( d->m_bComplete ) {
          d->m_redirectionTimer.start( 1000 * d->m_delayRedirect, true );
       }
    }
}

void KHTMLPart::slotRedirect()
{
  // kdDebug( 6050 ) << "KHTMLPart::slotRedirect()" << endl;
  QString u = d->m_redirectURL;
  d->m_delayRedirect = 0;
  d->m_redirectURL = QString::null;
  QString target;
  u = splitUrlTarget( u, &target );
  urlSelected( u, 0, 0, target );
}

void KHTMLPart::slotRedirection(KIO::Job*, const KURL& url)
{
  // the slave told us that we got redirected
  // kdDebug( 6050 ) << "redirection by KIO to " << url.url() << endl;
  emit d->m_extension->setLocationBarURL( url.prettyURL() );
  d->m_workingURL = url;
}

// ####
bool KHTMLPart::setCharset( const QString &name, bool override )
{
  QFont f(settings()->stdFontName());
  KGlobal::charsets()->setQFont(f, KGlobal::charsets()->charsetForEncoding(name) );

  //kdDebug(6005) << "setting to charset " << (int)QFontInfo(f).charSet() <<" " << override << " should be " << name << endl;

  d->m_settings->setDefaultCharset( f.charSet(), override );
  return true;
}

bool KHTMLPart::setEncoding( const QString &/*name*/, bool /*override*/ )
{
#ifdef __ATHEOS__
    printf( "Warning: KHTMLPart::setEncoding() not implemented\n" );
#else    
    d->m_encoding = name;
    d->m_haveEncoding = override;

//    setCharset( name, override );
     d->m_charset = KGlobal::charsets()->charsetForEncoding(name);
     d->m_settings->setCharset( d->m_charset );
     // the script should not be unicode. We need to know the document is eg. arabic to be
     // able to choose a unicode font that contains arabic glyphs.
     d->m_settings->setScript( KGlobal::charsets()->charsetForEncoding( name, true ) );

    if( !m_url.isEmpty() ) {
        // reload document
        closeURL();
        KURL url = m_url;
        m_url = 0;
        openURL(url);
    }
#endif
    return true;
}

QString KHTMLPart::encoding()
{
    if(d->m_haveEncoding && !d->m_encoding.isEmpty())
        return d->m_encoding;

    if(d->m_decoder && d->m_decoder->encoding())
        return QString(d->m_decoder->encoding());

    return(settings()->encoding());
}

void KHTMLPart::setUserStyleSheet(const KURL &url)
{
    d->m_userSheetUrl = DOMString();
    d->m_userSheet = DOMString();
    (void) new khtml::PartStyleSheetLoader(this, d, url.url(), d->m_doc ? d->m_doc->docLoader() : 0);
}

void KHTMLPart::setUserStyleSheet(const QString &styleSheet)
{
    d->m_userSheet = styleSheet;
    d->m_userSheetUrl = DOMString();
    khtml::CSSStyleSelector::setUserStyle( styleSheet );
}


bool KHTMLPart::gotoAnchor( const QString &name )
{
  HTMLCollectionImpl *anchors =
      new HTMLCollectionImpl( d->m_doc, HTMLCollectionImpl::DOC_ANCHORS);
  anchors->ref();
  NodeImpl *n = anchors->namedItem(name);
  anchors->deref();

  if(!n) {
      kdDebug() << "KHTMLPart::gotoAnchor no node found" << endl;
      return false;
  }

  int x = 0, y = 0;
  HTMLElementImpl *a = static_cast<HTMLElementImpl *>(n);
  a->getUpperLeftCorner(x, y);
  d->m_view->setContentsPos(x-50, y-50);

  return true;
}

void KHTMLPart::setFontSizes( const QValueList<int>& /*newFontSizes*/ )
{
#ifdef __ATHEOS__
//  printf( "KHTMLPart::setFontSizes() not setting fontsizes\n" );
#else  
  d->m_settings->setFontSizes( newFontSizes );
#endif  
}

QValueList<int> KHTMLPart::fontSizes() const
{
  return d->m_settings->fontSizes();
}

void KHTMLPart::resetFontSizes()
{
  d->m_settings->resetFontSizes();
}

void KHTMLPart::setStandardFont( const QString &name )
{
    d->m_settings->setStdFontName(name);
}

void KHTMLPart::setFixedFont( const QString &name )
{
    d->m_settings->setFixedFontName(name);
}

#ifndef __ATHEOS__
void KHTMLPart::setURLCursor( const QCursor &c )
{
  d->m_linkCursor = c;
}

const QCursor &KHTMLPart::urlCursor() const
{
  return d->m_linkCursor;
}
#endif // __ATHEOS__

void KHTMLPart::findTextBegin()
{
  d->m_findPos = -1;
  d->m_findNode = 0;
}

bool KHTMLPart::findTextNext( const QRegExp &exp, bool forward )
{
    if ( !d->m_doc )
        return false;

    if(!d->m_findNode) {
        if (d->m_doc->isHTMLDocument())
            d->m_findNode = static_cast<HTMLDocumentImpl*>(d->m_doc)->body();
        else
            d->m_findNode = d->m_doc;
    }

    if ( !d->m_findNode ||
         d->m_findNode->id() == ID_FRAMESET )
      return false;

    while(1)
    {
        if( (d->m_findNode->nodeType() == Node::TEXT_NODE || d->m_findNode->nodeType() == Node::CDATA_SECTION_NODE) && d->m_findNode->renderer() )
        {
            DOMStringImpl *t = (static_cast<TextImpl *>(d->m_findNode))->string();
            QConstString s(t->s, t->l);
            d->m_findPos = s.string().find(exp, d->m_findPos+1);
            if(d->m_findPos != -1)
            {
                int x = 0, y = 0;
        khtml::RenderText *text = static_cast<khtml::RenderText *>(d->m_findNode->renderer());
                text->posOfChar(d->m_findPos, x, y);
                d->m_view->setContentsPos(x-50, y-50);
                return true;
            }
        }
        d->m_findPos = -1;

        NodeImpl *next;

        if ( forward )
        {
          next = d->m_findNode->firstChild();

          if(!next) next = d->m_findNode->nextSibling();
          while(d->m_findNode && !next) {
              d->m_findNode = d->m_findNode->parentNode();
              if( d->m_findNode ) {
                  next = d->m_findNode->nextSibling();
              }
          }
        }
        else
        {
          next = d->m_findNode->lastChild();

          if (!next ) next = d->m_findNode->previousSibling();
          while ( d->m_findNode && !next )
          {
            d->m_findNode = d->m_findNode->parentNode();
            if( d->m_findNode )
            {
              next = d->m_findNode->previousSibling();
            }
          }
        }

        d->m_findNode = next;
        if(!d->m_findNode) return false;
    }
}

bool KHTMLPart::findTextNext( const QString &str, bool forward, bool caseSensitive )
{
    if ( !d->m_doc )
        return false;

    if(!d->m_findNode) {
        if (d->m_doc->isHTMLDocument())
            d->m_findNode = static_cast<HTMLDocumentImpl*>(d->m_doc)->body();
        else
            d->m_findNode = d->m_doc;
    }

    if ( !d->m_findNode ||
         d->m_findNode->id() == ID_FRAMESET )
      return false;

    while(1)
    {
        if( (d->m_findNode->nodeType() == Node::TEXT_NODE || d->m_findNode->nodeType() == Node::CDATA_SECTION_NODE) && d->m_findNode->renderer() )
        {
            DOMStringImpl *t = (static_cast<TextImpl *>(d->m_findNode))->string();
            QConstString s(t->s, t->l);
            d->m_findPos = s.string().find(str, d->m_findPos+1, caseSensitive);
            if(d->m_findPos != -1)
            {
                int x = 0, y = 0;
        static_cast<khtml::RenderText *>(d->m_findNode->renderer())
            ->posOfChar(d->m_findPos, x, y);
                d->m_view->setContentsPos(x-50, y-50);

                d->m_selectionStart = d->m_findNode;
                d->m_startOffset = d->m_findPos;
                d->m_selectionEnd = d->m_findNode;
                d->m_endOffset = d->m_findPos + str.length();
                d->m_startBeforeEnd = true;

                d->m_doc->setSelection( d->m_selectionStart.handle(), d->m_startOffset,
                                        d->m_selectionEnd.handle(), d->m_endOffset );
                emitSelectionChanged();
                return true;
            }
        }
        d->m_findPos = -1;

        NodeImpl *next;

        if ( forward )
        {
          next = d->m_findNode->firstChild();

          if(!next) next = d->m_findNode->nextSibling();
          while(d->m_findNode && !next) {
              d->m_findNode = d->m_findNode->parentNode();
              if( d->m_findNode ) {
                  next = d->m_findNode->nextSibling();
              }
          }
        }
        else
        {
          next = d->m_findNode->lastChild();

          if (!next ) next = d->m_findNode->previousSibling();
          while ( d->m_findNode && !next )
          {
            d->m_findNode = d->m_findNode->parentNode();
            if( d->m_findNode )
            {
              next = d->m_findNode->previousSibling();
            }
          }
        }

        d->m_findNode = next;
        if(!d->m_findNode) return false;
    }
}

QString KHTMLPart::selectedText() const
{
  QString text;
  DOM::Node n = d->m_selectionStart;
  while(!n.isNull()) {
      if(n.nodeType() == DOM::Node::TEXT_NODE) {
        QString str = static_cast<TextImpl *>(n.handle())->data().string();
        if(n == d->m_selectionStart && n == d->m_selectionEnd)
          text = str.mid(d->m_startOffset, d->m_endOffset - d->m_startOffset);
        else if(n == d->m_selectionStart)
          text = str.mid(d->m_startOffset);
        else if(n == d->m_selectionEnd)
          text += str.left(d->m_endOffset);
        else
         text += str;
      }
      else if(n.elementId() == ID_BR)
            text += "\n";
        else if(n.elementId() == ID_P || n.elementId() == ID_TD)
            text += "\n\n";
        if(n == d->m_selectionEnd) break;
        DOM::Node next = n.firstChild();
        if(next.isNull()) next = n.nextSibling();
        while( next.isNull() && !n.parentNode().isNull() ) {
            n = n.parentNode();
            next = n.nextSibling();
        }

        n = next;
    }
    return text;
}

bool KHTMLPart::hasSelection() const
{
  return ( !d->m_selectionStart.isNull() &&
           !d->m_selectionEnd.isNull() );
}

DOM::Range KHTMLPart::selection() const
{
    DOM::Range r = document().createRange();DOM::Range();
    r.setStart( d->m_selectionStart, d->m_startOffset );
    r.setEnd( d->m_selectionEnd, d->m_endOffset );
    return r;
}


void KHTMLPart::setSelection( const DOM::Range &r )
{
    d->m_selectionStart = r.startContainer();
    d->m_startOffset = r.startOffset();
    d->m_selectionEnd = r.endContainer();
    d->m_endOffset = r.endOffset();
    d->m_doc->setSelection(d->m_selectionStart.handle(),d->m_startOffset,
			   d->m_selectionEnd.handle(),d->m_endOffset);
}

// TODO merge with other overURL (BCI)
void KHTMLPart::overURL( const QString &url, const QString &target, bool shiftPressed )
{
  if( d->m_kjsStatusBarText.isEmpty() || shiftPressed )
  {
    overURL( url, target );
  }
  else
  {
    emit onURL( url );
    emit setStatusBarText( d->m_kjsStatusBarText );
    d->m_kjsStatusBarText = QString::null;
  }
}

void KHTMLPart::overURL( const QString &url, const QString &target )
{
  emit onURL( url );
  
  if ( url.isEmpty() )
  {
    emit setStatusBarText(url);
    return;
  }

  if (url.find( QString::fromLatin1( "javascript:" ),0, false ) != -1 )
  {
    emit setStatusBarText( url.mid( url.find( "javascript:", 0, false ) ) );
    return;
  }

  KURL u = completeURL( url );
  QString com;

  KMimeType::Ptr typ = KMimeType::findByURL( u );

  if ( typ )
    com = typ->comment( u, false );

  if ( u.isMalformed() )
  {
    emit setStatusBarText(u.prettyURL());
    return;
  }

  if ( u.isLocalFile() )
  {
    // TODO : use KIO::stat() and create a KFileItem out of its result,
    // to use KFileItem::statusBarText()
    QCString path = QFile::encodeName( u.path() );

    struct stat buff;
    bool ok = !stat( path.data(), &buff );

    struct stat lbuff;
    if (ok) ok = !lstat( path.data(), &lbuff );

    QString text = u.url();
    QString text2 = text;

    if (ok && S_ISLNK( lbuff.st_mode ) )
    {
      QString tmp;
      if ( com.isNull() )
        tmp = i18n( "Symbolic Link");
      else
        tmp = i18n("%1 (Link)").arg(com);
      char buff_two[1024];
      text += " -> ";
      int n = readlink ( path.data(), buff_two, 1022);
      if (n == -1)
      {
        text2 += "  ";
        text2 += tmp;
        emit setStatusBarText(text2);
        return;
      }
      buff_two[n] = 0;

      text += buff_two;
      text += "  ";
      text += tmp;
    }
    else if ( ok && S_ISREG( buff.st_mode ) )
    {
      if (buff.st_size < 1024)
        text = i18n("%2 (%1 bytes)").arg((long) buff.st_size).arg(text2); // always put the URL last, in case it contains '%'
      else
      {
        float d = (float) buff.st_size/1024.0;
        text = i18n("%1 (%2 K)").arg(text2).arg(KGlobal::locale()->formatNumber(d, 2)); // was %.2f
      }
      text += "  ";
      text += com;
    }
    else if ( ok && S_ISDIR( buff.st_mode ) )
    {
      text += "  ";
      text += com;
    }
    else
    {
      text += "  ";
      text += com;
    }
    emit setStatusBarText(text);
  }
  else
  {
    QString extra;
    if (target == QString::fromLatin1("_blank"))
    {
      extra = i18n(" (In new window)");
    }
    else if (!target.isEmpty() &&
             (target != QString::fromLatin1("_top")) &&
             (target != QString::fromLatin1("_self")) &&
             (target != QString::fromLatin1("_parent")))
    {
      extra = i18n(" (In other frame)");
    }

    if (u.protocol() == "mailto") {
#ifdef __ATHEOS__
//  printf( "Warning: KHTMLPart::openURL:8() not emitting\n" );
#else
      QString mailtoMsg = i18n("E-Mail to: ") + KURL::decode_string(u.path());
      QStringList queries = QStringList::split('&', u.query().mid(1));
      for (QStringList::Iterator it = queries.begin(); it != queries.end(); ++it)
        if ((*it).startsWith("subject="))
          mailtoMsg += i18n(" - Subject: ") + KURL::decode_string((*it).mid(8));
        else
          if ((*it).startsWith("cc="))
            mailtoMsg += i18n(" - CC: ") + KURL::decode_string((*it).mid(3));
          else
    	    if ((*it).startsWith("bcc="))
              mailtoMsg += i18n(" - BCC: ") + KURL::decode_string((*it).mid(4));
      mailtoMsg = "<img src=" + locate("icon", QString::fromLatin1("locolor/16x16/actions/mail_send.png")) + "> " + mailtoMsg;
      emit setStatusBarText(mailtoMsg);
    } else
      if (u.protocol() == "http") {
        DOM::Node hrefNode = nodeUnderMouse().parentNode();
        while (hrefNode.nodeName().string() != "A" && !hrefNode.isNull())
          hrefNode = hrefNode.parentNode();

        // Is this check neccessary at all? (Frerich)
        if (!hrefNode.isNull()) {
          DOM::Node hreflangNode = hrefNode.attributes().getNamedItem("HREFLANG");
          if (!hreflangNode.isNull()) {
            QString countryCode = hreflangNode.nodeValue().string().lower();
            // Map the language code to an appropriate country code.
            if (countryCode == "en")
              countryCode = "gb";
            QString flagImg = "<img src=" + locate("locale", QString::fromLatin1("l10n/") + countryCode + QString::fromLatin1("/flag.png")) + "> ";
            emit setStatusBarText( flagImg + u.prettyURL() + extra );
          } else
            emit setStatusBarText(u.prettyURL() + extra);
        } else
          emit setStatusBarText(u.prettyURL() + extra);
#endif // __ATHEOS__    
      }
    emit setStatusBarText(u.prettyURL() + extra);
  }
}

void KHTMLPart::urlSelected( const QString &url, int button, int state, const QString &_target )
{
  bool hasTarget = false;

  QString target = _target;
  if ( target.isEmpty() )
    target = d->m_baseTarget;
  if ( !target.isEmpty() )
      hasTarget = true;

  if ( url.find( QString::fromLatin1( "javascript:" ), 0, false ) == 0 )
  {
    executeScript( url.right( url.length() - 11) );
    return;
  }

  KURL cURL = completeURL( url );

  //kdDebug( 6000 ) << "complete URL:" << cURL.url() << " target = " << target << endl;

// no need to do that here, the historymngr is going to be notified
// about this new URL sooner or later anyway
//  KHTMLFactory::vLinks()->insert( cURL.url() );

  if ( button == LeftButton && ( state & ShiftButton ) && !cURL.isMalformed() )
  {
#ifdef __ATHEOS__
    d->m_extension->SaveURL( cURL.prettyURL().utf8().data(), i18n( "Save As..." ).utf8().data() );
#else
    KHTMLPopupGUIClient::saveURL( d->m_view, i18n( "Save As..." ), cURL );
#endif
    return;
  }

  if ( url.isEmpty() )
    return;

  if (!checkLinkSecurity(cURL))
    return;

  KParts::URLArgs args;
  args.frameName = target;

  // For http-refresh, force the io-slave to re-get the page
  // as needed instead of loading from cache. NOTE: I would
  // have done a "verify" instead, but I am not sure that servers
  // will include the correct response (specfically "Refresh:") on
  // a "HEAD" request which is what a "verify" setting results in.(DA)
  if ( d->m_bHTTPRefresh )
  {
    d->m_bHTTPRefresh = false;
	args.metaData()["cache"]="reload"; //"verify";
  }

  if ( hasTarget )
  {
    // unknown frame names should open in a new window.
    khtml::ChildFrame *frame = recursiveFrameRequest( cURL, args, false );
    if ( frame )
    {
        args.metaData()["referrer"]=m_url.url();
      requestObject( frame, cURL, args );
      return;
    }
  }

  if ( !d->m_bComplete && !hasTarget )
    closeURL();

  if (m_url == cURL)
  {
     // We don't really want to reload, we just want to verify our
     // cached images, but the khtml image cache can't handle that yet.
     args.reload = true;
  }
  if (!m_url.url().isEmpty())
      args.metaData()["referrer"]=m_url.url();
  emit d->m_extension->openURLRequest( cURL, args );
}

void KHTMLPart::slotViewDocumentSource()
{
  KURL url(m_url);
  if (KHTMLPageCache::self()->isValid(d->m_cacheId))
  {
     KTempFile sourceFile(QString::null, QString::fromLatin1(".html"));
     if (sourceFile.status() == 0)
     {
        KHTMLPageCache::self()->saveData(d->m_cacheId, sourceFile.dataStream());
        url = KURL();
        url.setPath(sourceFile.name());
     }
  }

  //  emit d->m_extension->openURLRequest( m_url, KParts::URLArgs( false, 0, 0, QString::fromLatin1( "text/plain" ) ) );
  (void) KRun::runURL( url, QString::fromLatin1("text/plain") );
}

void KHTMLPart::slotViewFrameSource()
{
  // ### frames
  //emit d->m_extension->openURLRequest( ((KParts::ReadOnlyPart *)partManager()->activePart())->url(), KParts::URLArgs( false, 0, 0, QString::fromLatin1( "text/plain" ) ) );
  KParts::ReadOnlyPart *frame = static_cast<KParts::ReadOnlyPart *>( partManager()->activePart() );
  if ( frame )
    (void) KRun::runURL( frame->url(), QString::fromLatin1("text/plain") );
}

void KHTMLPart::slotSaveBackground()
{
  // ### what about XML documents? get from CSS?
  if (!d->m_doc || !d->m_doc->isHTMLDocument())
    return;

  QString relURL = static_cast<HTMLDocumentImpl*>(d->m_doc)->body()->getAttribute( ATTR_BACKGROUND ).string();

  KURL backgroundURL( m_url, relURL );

#ifdef __ATHEOS__
  d->m_extension->SaveURL( backgroundURL.prettyURL().utf8().data(), i18n("Save background image as").utf8().data() );
#else
  KHTMLPopupGUIClient::saveURL( d->m_view, i18n("Save background image as"), backgroundURL );
#endif
}

void KHTMLPart::slotSaveDocument()
{
  KURL srcURL( m_url );

  if ( srcURL.fileName(false).isEmpty() )
    srcURL.setFileName( "index.html" );

#ifdef __ATHEOS__
  d->m_extension->SaveURL( srcURL.prettyURL().utf8().data(), i18n( "Save as" ).utf8().data() );
#else
  KHTMLPopupGUIClient::saveURL( d->m_view, i18n( "Save as" ), srcURL, i18n("*.html *.htm|HTML files"), d->m_cacheId );
#endif
}

void KHTMLPart::slotSecurity()
{
//   kdDebug( 6050 ) << "Meta Data:" << endl
//                   << d->m_ssl_peer_cert_subject
//                   << endl
//                   << d->m_ssl_peer_cert_issuer
//                   << endl
//                   << d->m_ssl_cipher
//                   << endl
//                   << d->m_ssl_cipher_desc
//                   << endl
//                   << d->m_ssl_cipher_version
//                   << endl
//                   << d->m_ssl_good_from
//                   << endl
//                   << d->m_ssl_good_until
//                   << endl
//                   << d->m_ssl_cert_state
//                   << endl;

#ifdef __ATHEOS__
    printf( "Warning: KHTMLPart::slotSecurity can't display dialog\n" );
#else
  KSSLInfoDlg *kid = new KSSLInfoDlg(d->m_ssl_in_use, widget());
  if (d->m_ssl_in_use) {
    kid->setup(d->m_ssl_peer_cert_subject,
               d->m_ssl_peer_cert_issuer,
               d->m_ssl_peer_ip,
               m_url.url(),
               d->m_ssl_cipher,
               d->m_ssl_cipher_desc,
               d->m_ssl_cipher_version,
               d->m_ssl_cipher_used_bits.toInt(),
               d->m_ssl_cipher_bits.toInt(),
               (KSSLCertificate::KSSLValidation) d->m_ssl_cert_state.toInt(),
               d->m_ssl_good_from, d->m_ssl_good_until);
  }
  kid->show();
#endif
}

void KHTMLPart::slotSaveFrame()
{
    if ( !d->m_activeFrame )
        return; // should never be the case, but one never knows :-)

    KURL srcURL( static_cast<KParts::ReadOnlyPart *>( d->m_activeFrame )->url() );

    if ( srcURL.fileName(false).isEmpty() )
        srcURL.setFileName( "index.html" );

#ifdef __ATHEOS__
    d->m_extension->SaveURL( srcURL.prettyURL().utf8().data(), i18n( "Save as" ).utf8().data() );
#else
    KHTMLPopupGUIClient::saveURL( d->m_view, i18n( "Save as" ), srcURL, i18n("HTML files|*.html *.htm") );
#endif
}

void KHTMLPart::slotSetEncoding()
{
#ifdef __ATHEOS__
//    printf( "Warning: KHTMLPart::slotSetEncoding() not implemented\n" );
#else
    // first Item is always auto
    if(d->m_paSetEncoding->currentItem() == 0)
        setEncoding(QString::null, false);
    else {
	// strip of the language to get the raw encoding again.
	QString enc = KGlobal::charsets()->encodingForName(d->m_paSetEncoding->currentText());
	setEncoding(enc, true);
    }
#endif
}

void KHTMLPart::updateActions()
{
#ifdef __ATHEOS__
//    printf( "Warning: KHTMLPart::updateActions() not implemented\n" );
#else    
  bool frames = false;

  QValueList<khtml::ChildFrame>::ConstIterator it = d->m_frames.begin();
  QValueList<khtml::ChildFrame>::ConstIterator end = d->m_frames.end();
  for (; it != end; ++it )
      if ( (*it).m_type == khtml::ChildFrame::Frame )
      {
          frames = true;
          break;
      }
  d->m_paViewFrame->setEnabled( frames );
  d->m_paSaveFrame->setEnabled( frames );

  if ( frames )
    d->m_paFind->setText( i18n( "&Find in Frame" ) );
  else
    d->m_paFind->setText( i18n( "&Find" ) );

  KParts::Part *frame = 0;

  if ( frames )
    frame = partManager()->activePart();

  bool enableFindAndSelectAll = true;

  if ( frame ) {
    enableFindAndSelectAll = dynamic_cast<KHTMLPart*>(frame) != NULL;
  }
  d->m_paFind->setEnabled( enableFindAndSelectAll );
  d->m_paSelectAll->setEnabled( enableFindAndSelectAll );
  bool enablePrintFrame = false;

  if ( frame )
  {
    QObject *ext = KParts::BrowserExtension::childObject( frame );
    if ( ext )
      enablePrintFrame = ext->metaObject()->slotNames().contains( "print()" );
  }

  d->m_paPrintFrame->setEnabled( enablePrintFrame );
  QString bgURL;

  // ### frames
  if ( d->m_doc && d->m_doc->isHTMLDocument() && static_cast<HTMLDocumentImpl*>(d->m_doc)->body() && !d->m_bClearing )
    bgURL = static_cast<HTMLDocumentImpl*>(d->m_doc)->body()->getAttribute( ATTR_BACKGROUND ).string();

  d->m_paSaveBackground->setEnabled( !bgURL.isEmpty() );
#endif // __ATHEOS__  

  // Now start the redirection since the current
  // document should be completely loaded. (DA)
  if ( !d->m_redirectURL.isEmpty() )
    d->m_redirectionTimer.start( 1000 * d->m_delayRedirect, true );
}

bool KHTMLPart::requestFrame( khtml::RenderPart *frame, const QString &url, const QString &frameName,
                              const QStringList &params, bool isIFrame )
{
//  kdDebug( 6050 ) << "childRequest( ..., " << url << ", " << frameName << " )" << endl;
  if (url.isEmpty())
    return false;
  FrameIt it = d->m_frames.find( frameName );
  if ( it == d->m_frames.end() )
  {
    khtml::ChildFrame child;
//    kdDebug( 6050 ) << "inserting new frame into frame map" << endl;
    child.m_name = frameName;
    it = d->m_frames.append( child );
  }

  (*it).m_type = isIFrame ? khtml::ChildFrame::IFrame : khtml::ChildFrame::Frame;
  (*it).m_frame = frame;
  (*it).m_params = params;

  return requestObject( &(*it), completeURL( url ) );
}

QString KHTMLPart::requestFrameName()
{
   return QString::fromLatin1("<!--frame %1-->").arg(d->m_frameNameId++);
}

bool KHTMLPart::requestObject( khtml::RenderPart *frame, const QString &url, const QString &serviceType,
                               const QStringList &params )
{
  if (url.isEmpty())
    return false;
  khtml::ChildFrame child;
  QValueList<khtml::ChildFrame>::Iterator it = d->m_objects.append( child );
  (*it).m_frame = frame;
  (*it).m_type = khtml::ChildFrame::Object;
  (*it).m_params = params;

  KParts::URLArgs args;
  args.serviceType = serviceType;
  return requestObject( &(*it), completeURL( url ), args );
}

bool KHTMLPart::requestObject( khtml::ChildFrame *child, const KURL &url, const KParts::URLArgs &_args )
{
  if (!checkLinkSecurity(url))
    return false;
  if ( child->m_bPreloaded )
  {
//      kdDebug(6005) << "requestObject preload" << endl;
    if ( child->m_frame && child->m_part )
      child->m_frame->setWidget( child->m_part->widget() );

    child->m_bPreloaded = false;
    return true;
  }

  KParts::URLArgs args( _args );

  if ( child->m_run )
    delete (KHTMLRun *)child->m_run;
  if ( child->m_part && !args.reload && urlcmp( child->m_part->url().url(), url.url(), true, true ) )
    args.serviceType = child->m_serviceType;

  child->m_args = args;
  child->m_serviceName = QString::null;
  if (!m_url.isEmpty())
    child->m_args.metaData()["referrer"] = m_url.url();

  if ( args.serviceType.isEmpty() ) {
    child->m_run = new KHTMLRun( this, child, url, child->m_args );
    return false;
  } else {
    return processObjectRequest( child, url, args.serviceType );
  }
}

bool KHTMLPart::processObjectRequest( khtml::ChildFrame *child, const KURL &_url, const QString &mimetype )
{
//  kdDebug( 6050 ) << "trying to create part for " << mimetype << endl;

  // IMPORTANT: create a copy of the url here, because it is just a reference, which was likely to be given
  // by an emitting frame part (emit openURLRequest( blahurl, ... ) . A few lines below we delete the part
  // though -> the reference becomes invalid -> crash is likely
  KURL url( _url );

  if (child->m_bNotify)
  {
      child->m_bNotify = false;
      emit d->m_extension->openURLNotify();
      emit d->m_extension->setLocationBarURL( url.prettyURL() );
  }

  if ( !child->m_services.contains( mimetype ) )
  {
    KParts::ReadOnlyPart *part = createPart( d->m_view->viewport(), child->m_name.ascii(), this, child->m_name.ascii(), mimetype, child->m_serviceName, child->m_services, child->m_params );

    if ( !part )
    {
        if ( child->m_frame )
            child->m_frame->partLoadingErrorNotify();

        return false;
    }

    //CRITICAL STUFF
    if ( child->m_part )
    {
      partManager()->removePart( (KParts::ReadOnlyPart *)child->m_part );
      delete (KParts::ReadOnlyPart *)child->m_part;
    }

    child->m_serviceType = mimetype;
    if ( child->m_frame )
      child->m_frame->setWidget( part->widget() );

    if ( child->m_type != khtml::ChildFrame::Object )
      partManager()->addPart( part, false );
//     else
//         kdDebug(6005) << "AH! NO FRAME!!!!!" << endl;

    child->m_part = part;

    if ( child->m_type != khtml::ChildFrame::Object )
    {
      connect( part, SIGNAL( started( KIO::Job *) ),
               this, SLOT( slotChildStarted( KIO::Job*) ) );
      connect( part, SIGNAL( completed() ),
               this, SLOT( slotChildCompleted() ) );
      connect( part, SIGNAL( setStatusBarText( const QString & ) ),
               this, SIGNAL( setStatusBarText( const QString & ) ) );
    }

    child->m_extension = KParts::BrowserExtension::childObject( part );

    if ( child->m_extension )
    {
      connect( child->m_extension, SIGNAL( openURLNotify() ),
               d->m_extension, SIGNAL( openURLNotify() ) );

      connect( child->m_extension, SIGNAL( openURLRequestDelayed( const KURL &, const KParts::URLArgs & ) ),
               this, SLOT( slotChildURLRequest( const KURL &, const KParts::URLArgs & ) ) );

      connect( child->m_extension, SIGNAL( createNewWindow( const KURL &, const KParts::URLArgs & ) ),
               d->m_extension, SIGNAL( createNewWindow( const KURL &, const KParts::URLArgs & ) ) );
      connect( child->m_extension, SIGNAL( createNewWindow( const KURL &, const KParts::URLArgs &, const KParts::WindowArgs &, KParts::ReadOnlyPart *& ) ),
               d->m_extension, SIGNAL( createNewWindow( const KURL &, const KParts::URLArgs & , const KParts::WindowArgs &, KParts::ReadOnlyPart *&) ) );

      connect( child->m_extension, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ),
               d->m_extension, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ) );
      connect( child->m_extension, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ),
               d->m_extension, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ) );
      connect( child->m_extension, SIGNAL( popupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ),
               d->m_extension, SIGNAL( popupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ) );
      connect( child->m_extension, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KURL &, const QString &, mode_t ) ),
               d->m_extension, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KURL &, const QString &, mode_t ) ) );

      connect( child->m_extension, SIGNAL( infoMessage( const QString & ) ),
               d->m_extension, SIGNAL( infoMessage( const QString & ) ) );
#ifdef __ATHEOS__
      child->m_extension->SetCallback( d->m_extension->GetCallback() );
      child->m_extension->SetChildFlag( true );
#endif
    }

  }

  if ( child->m_bPreloaded )
  {
    if ( child->m_frame && child->m_part )
      child->m_frame->setWidget( child->m_part->widget() );

    child->m_bPreloaded = false;
    return true;
  }

  child->m_args.reload = d->m_bReloading;
  // make sure the part has a way to find out about the mimetype.
  // we actually set it in child->m_args in requestObject already,
  // but it's useless if we had to use a KHTMLRun instance, as the
  // point the run object is to find out exactly the mimetype.
  child->m_args.serviceType = mimetype;

  child->m_bCompleted = false;
  if ( child->m_extension )
    child->m_extension->setURLArgs( child->m_args );

//  kdDebug( 6050 ) << "opening " << url.url() << " in frame" << endl;
  return child->m_part->openURL( url );
}

KParts::ReadOnlyPart *KHTMLPart::createPart( QWidget* parentWidget, const char* widgetName,
                                             QObject* parent, const char* name, const QString& mimetype,
                                             QString& serviceName, QStringList& serviceTypes,
                                             const QStringList& params )
{
  QString constr;
  if ( !serviceName.isEmpty() )
    constr.append( QString::fromLatin1( "Name == '%1'" ).arg( serviceName ) );
#ifdef __ATHEOS__
  KTrader::OfferList offers = KTrader::self()->query( mimetype, constr );
#else
  KTrader::OfferList offers = KTrader::self()->query( mimetype, "KParts/ReadOnlyPart", constr, QString::null );
#endif
  if ( offers.isEmpty() )
    return 0L;

  KService::Ptr service = *offers.begin();

  KLibFactory *factory = KLibLoader::self()->factory( service->library().latin1() );

  if ( !factory )
    return 0L;

  KParts::ReadOnlyPart *res = 0L;

  const char *className = "KParts::ReadOnlyPart";
  if ( service->serviceTypes().contains( "Browser/View" ) )
    className = "Browser/View";

  if ( dynamic_cast<KParts::Factory*>(factory) != NULL )
    res = static_cast<KParts::ReadOnlyPart *>(static_cast<KParts::Factory *>( factory )->createPart( parentWidget, widgetName, parent, name, className, params ));
  else
  res = static_cast<KParts::ReadOnlyPart *>(factory->create( parentWidget, widgetName, className ));

  if ( !res )
    return res;

  serviceTypes = service->serviceTypes();
  serviceName = service->name();

  return res;
}

KParts::PartManager *KHTMLPart::partManager()
{
  if ( !d->m_manager )
  {
    d->m_manager = new KParts::PartManager( d->m_view->topLevelWidget(), this, "khtml part manager" );
    d->m_manager->setAllowNestedParts( true );
    connect( d->m_manager, SIGNAL( activePartChanged( KParts::Part * ) ),
             this, SLOT( slotActiveFrameChanged( KParts::Part * ) ) );
    connect( d->m_manager, SIGNAL( partRemoved( KParts::Part * ) ),
             this, SLOT( slotPartRemoved( KParts::Part * ) ) );
  }

  return d->m_manager;
}

void KHTMLPart::submitFormAgain()
{
  if( !d->m_bParsing && d->m_submitForm)
    KHTMLPart::submitForm( d->m_submitForm->submitAction, d->m_submitForm->submitUrl, d->m_submitForm->submitFormData, d->m_submitForm->submitContentType, d->m_submitForm->submitBoundary );

  delete d->m_submitForm;
  d->m_submitForm = 0;
  disconnect(this, SIGNAL(completed()), this, SLOT(submitFormAgain()));
}

void KHTMLPart::submitForm( const char *action, const QString &url, const QByteArray &formData, const QString &_target, const QString& contentType, const QString& boundary )
{
  QString target = _target;
  if ( target.isEmpty() )
    target = d->m_baseTarget;

  KURL u = completeURL( url, target );

  if ( u.isMalformed() )
  {
    // ### ERROR HANDLING!
    return;
  }

  QString urlstring = u.url();

  if ( urlstring.find( QString::fromLatin1( "javascript:" ), 0, false ) == 0 ) {
      urlstring = KURL::decode_string(urlstring);
      executeScript( urlstring.right( urlstring.length() - 11) );
      return;
  }

  if (!checkLinkSecurity(u))
    return;

  KParts::URLArgs args;

  if (!m_url.isEmpty())
     args.metaData()["referrer"] = m_url.url();
  if ( strcmp( action, "get" ) == 0 )
  {
    u.setQuery( QString::fromLatin1( formData.data(), formData.size() ) );

    args.frameName = target;
  }
  else
  {
    args.postData = formData;
    args.frameName = target;

    // construct some user headers if necessary
    if (contentType.isNull() || contentType == "application/x-www-form-urlencoded")
      args.setContentType( "Content-Type: application/x-www-form-urlencoded" );
    else // contentType must be "multipart/form-data"
      args.setContentType( "Content-Type: " + contentType + "; boundary=" + boundary );
  }

  if ( d->m_bParsing ) {
    if( d->m_submitForm ) {
        kdWarning( 6070 ) << "Submit() while parsing, but another submit (while parsing) was going on before... Please report this as a bug!" << endl;
	return;
    }
    kdWarning( 6070 ) << "Submit() while parsing, will try to submit again in 3 seconds" << endl;
    d->m_submitForm = new KHTMLPartPrivate::SubmitForm;
    d->m_submitForm->submitAction = action;
    d->m_submitForm->submitUrl = url;
    d->m_submitForm->submitFormData = formData;
    d->m_submitForm->submitContentType = contentType;
    d->m_submitForm->submitBoundary = boundary;
    connect(this, SIGNAL(completed()), this, SLOT(submitFormAgain()));
  }
  else {
    emit d->m_extension->openURLRequest( u, args );
  }
}

void KHTMLPart::popupMenu( const QString &url )
{
  KURL u( completeURL( url ) );
  /*
  mode_t mode = 0;
  if ( !u.isLocalFile() )
  {
    QString cURL = u.url( 1 );
    int i = cURL.length();
    // A url ending with '/' is always a directory
    if ( i >= 1 && cURL[ i - 1 ] == '/' )
      mode = S_IFDIR;
  }
  */

  mode_t mode = S_IFDIR; // treat all html documents as "DIR" in order to have the back/fwd/reload
                         // buttons in the popupmenu

  KXMLGUIClient *client = new KHTMLPopupGUIClient( this, d->m_popupMenuXML, u );
  emit d->m_extension->popupMenu( client, QCursor::pos(), u, QString::fromLatin1( "text/html" ), mode );
  delete client;

  emit popupMenu(url, QCursor::pos());
}

void KHTMLPart::slotChildStarted( KIO::Job *job )
{
  khtml::ChildFrame *child = frame( sender() );

  assert( child );

  child->m_bCompleted = false;

  if ( d->m_bComplete )
  {
#if 0
    // WABA: Looks like this belongs somewhere else
    if ( !parentPart() ) // "toplevel" html document? if yes, then notify the hosting browser about the document (url) changes
    {
      emit d->m_extension->openURLNotify();
    }
#endif
    d->m_bComplete = false;
    emit started( job );
  }
}

void KHTMLPart::slotChildCompleted()
{
  khtml::ChildFrame *child = frame( sender() );

  assert( child );

  child->m_bCompleted = true;
  child->m_args = KParts::URLArgs();

  checkCompleted();
}

void KHTMLPart::slotChildURLRequest( const KURL &url, const KParts::URLArgs &args )
{
  khtml::ChildFrame *child = frame( sender()->parent() );

  QString frameName = args.frameName.lower();
  if ( !frameName.isEmpty() )
  {
    if ( frameName == QString::fromLatin1( "_top" ) )
    {
      emit d->m_extension->openURLRequest( url, args );
      return;
    }
    else if ( frameName == QString::fromLatin1( "_blank" ) )
    {
      emit d->m_extension->createNewWindow( url, args );
      return;
    }
    else if ( frameName == QString::fromLatin1( "_parent" ) )
    {
      KParts::URLArgs newArgs( args );
      newArgs.frameName = QString::null;

      emit d->m_extension->openURLRequest( url, newArgs );
      return;
    }
    else if ( frameName != QString::fromLatin1( "_self" ) )
    {
      khtml::ChildFrame *_frame = recursiveFrameRequest( url, args );

      if ( !_frame )
      {
        emit d->m_extension->openURLRequest( url, args );
        return;
      }

      child = _frame;
    }
  }

  // TODO: handle child target correctly! currently the script are always executed fur the parent
  QString urlStr = url.url();
  if ( urlStr.find( QString::fromLatin1( "javascript:" ), 0, false ) == 0 ) {
      executeScript( urlStr.right( urlStr.length() - 11) );
      return;
  }

  if ( child ) {
      // Inform someone that we are about to show something else.
      child->m_bNotify = true;
//      emit d->m_extension->openURLNotify();
//      emit d->m_extension->setLocationBarURL( url.prettyURL() );
      requestObject( child, url, args );
  }  else if ( frameName==QString::fromLatin1("_self") ) // this is for embedded objects (via <object>) which want to replace the current document
  {
      KParts::URLArgs newArgs( args );
      newArgs.frameName = QString::null;
      emit d->m_extension->openURLRequest( url, newArgs );
  }
}

khtml::ChildFrame *KHTMLPart::frame( const QObject *obj )
{
#ifdef __ATHEOS__
    assert( dynamic_cast<const KParts::ReadOnlyPart*>(obj) != NULL );
#else
    assert( obj->inherits( "KParts::ReadOnlyPart" ) );
#endif    
    const KParts::ReadOnlyPart *part = static_cast<const KParts::ReadOnlyPart *>( obj );

    FrameIt it = d->m_frames.begin();
    FrameIt end = d->m_frames.end();
    for (; it != end; ++it )
      if ( (KParts::ReadOnlyPart *)(*it).m_part == part )
        return &(*it);

    return 0L;
}

KHTMLPart *KHTMLPart::findFrame( const QString &f )
{
  // ### http://www.w3.org/TR/html4/appendix/notes.html#notes-frames
  ConstFrameIt it = d->m_frames.find( f );
  if ( it == d->m_frames.end() )
    return 0L;
  else {
    KParts::ReadOnlyPart *p = (*it).m_part;
#ifdef __ATHEOS__    
    if ( dynamic_cast<KHTMLPart*>(p) != NULL )
#else
    if ( p && p->inherits( "KHTMLPart" ))
#endif	
      return (KHTMLPart*)p;
    else
      return 0L;
  }
}

bool KHTMLPart::frameExists( const QString &frameName )
{
  ConstFrameIt it = d->m_frames.find( frameName );
  if ( it == d->m_frames.end() )
    return false;

  // WABA: We only return true if the child actually has a frame
  // set. Otherwise we might find our preloaded-selve.
  // This happens when we restore the frameset.
  return ((*it).m_frame != 0);
}

KHTMLPart *KHTMLPart::parentPart()
{
#ifdef __ATHEOS__    
  if ( dynamic_cast<KHTMLPart*>(parent()) == NULL )
#else      
  if ( !parent() || !parent()->inherits( "KHTMLPart" ) )
#endif      
    return 0L;

  return (KHTMLPart *)parent();
}

khtml::ChildFrame *KHTMLPart::recursiveFrameRequest( const KURL &url, const KParts::URLArgs &args,
                                                     bool callParent )
{
  FrameIt it = d->m_frames.find( args.frameName );

  if ( it != d->m_frames.end() )
    return &(*it);

  it = d->m_frames.begin();
  FrameIt end = d->m_frames.end();
  for (; it != end; ++it )
#ifdef __ATHEOS__      
    if ( dynamic_cast<KHTMLPart*>(static_cast<ReadOnlyPart*>((*it).m_part)) != NULL )
#else	
    if ( (*it).m_part && (*it).m_part->inherits( "KHTMLPart" ) )
#endif	
    {
      KHTMLPart *childPart = (KHTMLPart *)(KParts::ReadOnlyPart *)(*it).m_part;

      khtml::ChildFrame *res = childPart->recursiveFrameRequest( url, args, false );
      if ( !res )
        continue;

      childPart->requestObject( res, url, args );
      return 0L;
    }

  if ( parentPart() && callParent )
  {
    khtml::ChildFrame *res = parentPart()->recursiveFrameRequest( url, args );

    if ( res )
      parentPart()->requestObject( res, url, args );

    return 0L;
  }

  return 0L;
}

void KHTMLPart::saveState( QDataStream &stream )
{
  kdDebug( 6050 ) << "KHTMLPart::saveState saving URL " << m_url.url() << endl;

  stream << m_url << (Q_INT32)d->m_view->contentsX() << (Q_INT32)d->m_view->contentsY();

  // save link cursor position
  int focusNodeNumber;
  if (!d->m_focusNodeRestored)
  {
      focusNodeNumber = d->m_focusNodeNumber;
  }
  else
  {
      focusNodeNumber = 0;
      if (d->m_doc)
      {
	  DOM::ElementImpl *focusNode = d->m_doc->focusNode();
	  while( focusNode )
	  {
	      focusNodeNumber++;
	      focusNode = d->m_doc->findNextLink(focusNode, false);
	  }
      }
  }
  stream << focusNodeNumber;

  // Save the doc's cache id.
  stream << d->m_cacheId;

  // Save the state of the document (Most notably the state of any forms)
  QStringList docState;
  if (d->m_doc)
  {
     docState = d->m_doc->state();
  }
  stream << (Q_UINT32) d->m_settings->charset() << docState;

  // Save font data
  stream << fontSizes() << d->m_fontBase;

  // Save ssl data
  stream << d->m_ssl_in_use
         << d->m_ssl_peer_cert_subject
         << d->m_ssl_peer_cert_issuer
         << d->m_ssl_peer_ip
         << d->m_ssl_cipher
         << d->m_ssl_cipher_desc
         << d->m_ssl_cipher_version
         << d->m_ssl_cipher_used_bits
         << d->m_ssl_cipher_bits
         << d->m_ssl_cert_state
         << d->m_ssl_good_from
         << d->m_ssl_good_until;

  // Save frame data
  stream << (Q_UINT32)d->m_frames.count();

  QStringList frameNameLst, frameServiceTypeLst, frameServiceNameLst;
  KURL::List frameURLLst;
  QValueList<QByteArray> frameStateBufferLst;

  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
  {
    frameNameLst << (*it).m_name;
    frameServiceTypeLst << (*it).m_serviceType;
    frameServiceNameLst << (*it).m_serviceName;
    if ( (*it).m_part )
      frameURLLst << (*it).m_part->url();
    else
      frameURLLst << KURL();

    QByteArray state;
    QDataStream frameStream( state, IO_WriteOnly );

    if ( (*it).m_part && (*it).m_extension )
      (*it).m_extension->saveState( frameStream );

    frameStateBufferLst << state;
  }

  stream << frameNameLst << frameServiceTypeLst << frameServiceNameLst << frameURLLst << frameStateBufferLst;
}

void KHTMLPart::restoreState( QDataStream &stream )
{
  KURL u;
  Q_INT32 xOffset;
  Q_INT32 yOffset;
  Q_UINT32 frameCount;
  QStringList frameNames, frameServiceTypes, docState, frameServiceNames;
  KURL::List frameURLs;
  QValueList<QByteArray> frameStateBuffers;
  QValueList<int> fSizes;
  KURL::List visitedLinks;
  Q_INT32 charset;
  long old_cacheId = d->m_cacheId;

  stream >> u >> xOffset >> yOffset;

  // restore link cursor position
  // nth node is active. value is set in checkCompleted()
  stream >> d->m_focusNodeNumber;
  d->m_focusNodeRestored = false;
  kdDebug()<<"new focus Node number is:"<<d->m_focusNodeNumber<<endl;

  stream >> d->m_cacheId;

  stream >> charset >> docState;
  d->m_charset = (QFont::CharSet) charset;

  stream >> fSizes >> d->m_fontBase;
#ifdef __ATHEOS__
  printf( "KHTMLPart::restoreState() not setting fontsizes\n" );
#else  
  setFontSizes( fSizes );
#endif
  // Restore ssl data
  stream >> d->m_ssl_in_use
         >> d->m_ssl_peer_cert_subject
         >> d->m_ssl_peer_cert_issuer
         >> d->m_ssl_peer_ip
         >> d->m_ssl_cipher
         >> d->m_ssl_cipher_desc
         >> d->m_ssl_cipher_version
         >> d->m_ssl_cipher_used_bits
         >> d->m_ssl_cipher_bits
         >> d->m_ssl_cert_state
         >> d->m_ssl_good_from
         >> d->m_ssl_good_until;

#ifndef __ATHEOS__
  d->m_paSecurity->setIcon( d->m_ssl_in_use ? "lock" : "unlock" );
#endif
  stream >> frameCount >> frameNames >> frameServiceTypes >> frameServiceNames
         >> frameURLs >> frameStateBuffers;

  d->m_bComplete = false;

//   kdDebug( 6050 ) << "restoreStakte() docState.count() = " << docState.count() << endl;
//   kdDebug( 6050 ) << "m_url " << m_url.url() << " <-> " << u.url() << endl;
//   kdDebug( 6050 ) << "m_frames.count() " << d->m_frames.count() << " <-> " << frameCount << endl;

  if (d->m_cacheId == old_cacheId)
  {
    // Partial restore
    d->m_redirectionTimer.stop();
    FrameIt fIt = d->m_frames.begin();
    FrameIt fEnd = d->m_frames.end();

    for (; fIt != fEnd; ++fIt )
        (*fIt).m_bCompleted = false;

    fIt = d->m_frames.begin();

    QStringList::ConstIterator fNameIt = frameNames.begin();
    QStringList::ConstIterator fServiceTypeIt = frameServiceTypes.begin();
    QStringList::ConstIterator fServiceNameIt = frameServiceNames.begin();
    KURL::List::ConstIterator fURLIt = frameURLs.begin();
    QValueList<QByteArray>::ConstIterator fBufferIt = frameStateBuffers.begin();

    for (; fIt != fEnd; ++fIt, ++fNameIt, ++fServiceTypeIt, ++fServiceNameIt, ++fURLIt, ++fBufferIt )
    {
      khtml::ChildFrame *child = &(*fIt);

//      kdDebug( 6050 ) <<  *fNameIt  << " ---- " <<  *fServiceTypeIt << endl;

      if ( child->m_name != *fNameIt || child->m_serviceType != *fServiceTypeIt )
      {
        child->m_bPreloaded = true;
        child->m_name = *fNameIt;
        child->m_serviceName = *fServiceNameIt;
        processObjectRequest( child, *fURLIt, *fServiceTypeIt );
      }

      if ( child->m_part )
      {
        child->m_bCompleted = false;
        if ( child->m_extension )
        {
          QDataStream frameStream( *fBufferIt, IO_ReadOnly );
          child->m_extension->restoreState( frameStream );
        }
        else
          child->m_part->openURL( *fURLIt );
      }
    }

    KParts::URLArgs args( d->m_extension->urlArgs() );
    args.xOffset = xOffset;
    args.yOffset = yOffset;
    args.docState = docState; // WABA: How are we going to restore this??
    d->m_extension->setURLArgs( args );

    d->m_view->setContentsPos( xOffset, yOffset );
  }
  else
  {
    // Full restore.
    closeURL();
    // We must force a clear because we want to be sure to delete all
    // frames.
    d->m_bCleared = false;
    clear();

    QStringList::ConstIterator fNameIt = frameNames.begin();
    QStringList::ConstIterator fNameEnd = frameNames.end();

    QStringList::ConstIterator fServiceTypeIt = frameServiceTypes.begin();
    QStringList::ConstIterator fServiceNameIt = frameServiceNames.begin();
    KURL::List::ConstIterator fURLIt = frameURLs.begin();
    QValueList<QByteArray>::ConstIterator fBufferIt = frameStateBuffers.begin();

    for (; fNameIt != fNameEnd; ++fNameIt, ++fServiceTypeIt, ++fServiceNameIt, ++fURLIt, ++fBufferIt )
    {
      khtml::ChildFrame newChild;
      newChild.m_bPreloaded = true;
      newChild.m_name = *fNameIt;
      newChild.m_serviceName = *fServiceNameIt;

//      kdDebug( 6050 ) << *fNameIt << " ---- " << *fServiceTypeIt << endl;

      FrameIt childFrame = d->m_frames.append( newChild );

      processObjectRequest( &(*childFrame), *fURLIt, *fServiceTypeIt );

      (*childFrame).m_bPreloaded = true;

      if ( (*childFrame).m_part )
      {
        if ( (*childFrame).m_extension )
        {
          QDataStream frameStream( *fBufferIt, IO_ReadOnly );
          (*childFrame).m_extension->restoreState( frameStream );
        }
        else
          (*childFrame).m_part->openURL( *fURLIt );
      }
    }

    KParts::URLArgs args( d->m_extension->urlArgs() );
    args.xOffset = xOffset;
    args.yOffset = yOffset;
    args.docState = docState;
    d->m_extension->setURLArgs( args );
//    kdDebug( 6050 ) << "in restoreState : calling openURL for " << u.url() << endl;
    if (!KHTMLPageCache::self()->isValid(d->m_cacheId))
       openURL( u );
    else
       restoreURL( u );
  }

}

void KHTMLPart::saveState( os::Message* pcBuffer )
{
    QByteArray cBuffer;
    QDataStream cStream( cBuffer, IO_WriteOnly );
    saveState( cStream );
    pcBuffer->AddData( "state", os::T_ANY_TYPE, cBuffer.data(), cBuffer.size() );
}

void KHTMLPart::restoreState( const os::Message& cBuffer )
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

void KHTMLPart::show()
{
  if ( d->m_view )
    d->m_view->show();
}

void KHTMLPart::hide()
{
  if ( d->m_view )
    d->m_view->hide();
}

DOM::Node KHTMLPart::nodeUnderMouse() const
{
    return d->m_view->nodeUnderMouse();
}

void KHTMLPart::emitSelectionChanged()
{
  emit d->m_extension->enableAction( "copy", hasSelection() );
  emit d->m_extension->selectionInfo( selectedText() );
  emit selectionChanged();
}

void KHTMLPart::slotIncFontSizes()
{
  updateFontSize( ++d->m_fontBase );
#ifdef __ATHEOS__
  printf( "KHTMLPart::slotIncFontSizes() not implemented\n" );
#else
  if ( !d->m_paDecFontSizes->isEnabled() )
    d->m_paDecFontSizes->setEnabled( true );
#endif
}

void KHTMLPart::slotDecFontSizes()
{
  if ( d->m_fontBase >= 1 )
    updateFontSize( --d->m_fontBase );

#ifdef __ATHEOS__
  printf( "KHTMLPart::slotDecFontSizes() not implemented\n" );
#else
  if ( d->m_fontBase == 0 )
    d->m_paDecFontSizes->setEnabled( false );
#endif
}

void KHTMLPart::setFontBaseInternal( int base, bool absolute )
{
    if ( absolute )
      d->m_fontBase = base;
    else
      d->m_fontBase += base;

    if ( d->m_fontBase < 0 )
        d->m_fontBase = 0;

#ifndef __ATHEOS__
   d->m_paDecFontSizes->setEnabled( d->m_fontBase > 0 );
#endif
    updateFontSize( d->m_fontBase );
}

void KHTMLPart::setJSStatusBarText( const QString &text )
{
   d->m_kjsStatusBarText = text;
   emit setStatusBarText( d->m_kjsStatusBarText );
}

void KHTMLPart::setJSDefaultStatusBarText( const QString &text )
{
   d->m_kjsDefaultStatusBarText = text;
   emit setStatusBarText( d->m_kjsDefaultStatusBarText );
}

QString KHTMLPart::jsStatusBarText() const
{
    return d->m_kjsStatusBarText;
}

QString KHTMLPart::jsDefaultStatusBarText() const
{
   return d->m_kjsDefaultStatusBarText;
}

void KHTMLPart::updateFontSize( int add )
{
  resetFontSizes();
  QValueList<int> sizes = fontSizes();

  QValueList<int>::Iterator it = sizes.begin();
  QValueList<int>::Iterator end = sizes.end();
  for (; it != end; ++it )
    (*it) += add;

#ifdef __ATHEOS__
  printf( "KHTMLPart::updateFontSize() ont setting fontsizes\n" );
#else  
  setFontSizes( sizes );
#endif
  
#ifndef __ATHEOS__
  QApplication::setOverrideCursor( waitCursor );
#endif
  if(d->m_doc) d->m_doc->applyChanges();
#ifndef __ATHEOS__
  QApplication::restoreOverrideCursor();
#endif
}

void KHTMLPart::slotLoadImages()
{
  bool autoload = khtml::Cache::autoloadImages();
  khtml::Cache::autoloadImages( !autoload );
  khtml::Cache::autoloadImages( autoload );
}

void KHTMLPart::reparseConfiguration()
{
  KHTMLSettings *settings = KHTMLFactory::defaultHTMLSettings();
  settings->init();

  // Keep original charset setting.
  settings->setCharset(d->m_settings->charset());
  settings->setScript(d->m_settings->script());

  if ( settings->autoLoadImages() != khtml::Cache::autoloadImages() )
    autoloadImages( settings->autoLoadImages() );

  // PENDING(lars) Pass hostname to the following two methods.
  d->m_bJScriptEnabled = settings->isJavaScriptEnabled();
  d->m_bJavaEnabled = settings->isJavaEnabled();
  d->m_bPluginsEnabled = settings->isPluginsEnabled();
  delete d->m_settings;
  d->m_settings = new KHTMLSettings(*KHTMLFactory::defaultHTMLSettings());

#ifndef __ATHEOS__
  QApplication::setOverrideCursor( waitCursor );
#endif
  if(d->m_doc) d->m_doc->applyChanges();
#ifndef __ATHEOS__
  QApplication::restoreOverrideCursor();
#endif
}

QStringList KHTMLPart::frameNames() const
{
  QStringList res;

  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
    res += (*it).m_name;

  return res;
}

const QList<KParts::ReadOnlyPart> KHTMLPart::frames() const
{
  QList<KParts::ReadOnlyPart> res;

  ConstFrameIt it = d->m_frames.begin();
  ConstFrameIt end = d->m_frames.end();
  for (; it != end; ++it )
     res.append( (*it).m_part );

  return res;
}

bool KHTMLPart::openURLInFrame( const KURL &url, const KParts::URLArgs &urlArgs )
{
  FrameIt it = d->m_frames.find( urlArgs.frameName );

  if ( it == d->m_frames.end() )
    return false;

  // Inform someone that we are about to show something else.
  emit d->m_extension->openURLNotify();
  requestObject( &(*it), url, urlArgs );

  return true;
}

void KHTMLPart::setDNDEnabled( bool b )
{
  d->m_bDnd = b;
}

bool KHTMLPart::dndEnabled() const
{
  return d->m_bDnd;
}

bool KHTMLPart::event( QEvent *event )
{
  if ( KParts::ReadOnlyPart::event( event ) )
   return true;

  if ( khtml::MousePressEvent::test( event ) )
  {
    khtmlMousePressEvent( static_cast<khtml::MousePressEvent *>( event ) );
    return true;
  }

  if ( khtml::MouseDoubleClickEvent::test( event ) )
  {
    khtmlMouseDoubleClickEvent( static_cast<khtml::MouseDoubleClickEvent *>( event ) );
    return true;
  }

  if ( khtml::MouseMoveEvent::test( event ) )
  {
    khtmlMouseMoveEvent( static_cast<khtml::MouseMoveEvent *>( event ) );
    return true;
  }

  if ( khtml::MouseReleaseEvent::test( event ) )
  {
    khtmlMouseReleaseEvent( static_cast<khtml::MouseReleaseEvent *>( event ) );
    return true;
  }

#ifndef __ATHEOS__
  if ( khtml::DrawContentsEvent::test( event ) )
  {
    khtmlDrawContentsEvent( static_cast<khtml::DrawContentsEvent *>( event ) );
    return true;
  }
#endif // __ATHEOS__

  return false;
}

void KHTMLPart::khtmlMousePressEvent( khtml::MousePressEvent *event )
{
  DOM::DOMString url = event->url();
  QMouseEvent *_mouse = event->qmouseEvent();
  DOM::Node innerNode = event->innerNode();
  d->m_mousePressNode = innerNode;

#ifdef __ATHEOS__
  if ( d->m_manager != NULL ) {
    d->m_manager->setActivePart( this );
  }  
#endif
  
   d->m_dragStartPos = _mouse->pos();

  if ( event->url() != 0 )
    d->m_strSelectedURL = event->url().string();
  else
    d->m_strSelectedURL = QString::null;

  if ( _mouse->button() == LeftButton ||
       _mouse->button() == MidButton )
  {
    d->m_bMousePressed = true;

    if ( _mouse->button() == LeftButton )
    {
      if ( !innerNode.isNull() )
      {
	  int offset;
	  DOM::Node node;
	  innerNode.handle()->findSelectionNode( event->x(), event->y(),
					    event->nodeAbsX(), event->nodeAbsY(),
						 node, offset );

        if ( node.isNull() || !node.handle() )
        {
            kdDebug( 6000 ) << "Hmm, findSelectionNode returned no node" << endl;
            d->m_selectionStart = innerNode;
            d->m_startOffset = 0; //?
        } else {
            d->m_selectionStart = node;
            d->m_startOffset = offset;
        }
        kdDebug(6005) << "KHTMLPart::khtmlMousePressEvent selectionStart=" << d->m_selectionStart.handle()->renderer()
                      << " offset=" << d->m_startOffset << endl;
        d->m_selectionEnd = d->m_selectionStart;
        d->m_endOffset = d->m_startOffset;
        d->m_doc->clearSelection();
      }
      else
      {
        d->m_selectionStart = DOM::Node();
        d->m_selectionEnd = DOM::Node();
      }
      emitSelectionChanged();
      startAutoScroll();
    }
  }

  if ( _mouse->button() == RightButton )
  {
    popupMenu( splitUrlTarget(d->m_strSelectedURL) );
    d->m_strSelectedURL = QString::null;
  }
}

void KHTMLPart::khtmlMouseDoubleClickEvent( khtml::MouseDoubleClickEvent * )
{
}

void KHTMLPart::khtmlMouseMoveEvent( khtml::MouseMoveEvent *event )
{
  QMouseEvent *_mouse = event->qmouseEvent();
  DOM::DOMString url = event->url();
  DOM::Node innerNode = event->innerNode();

#ifndef __ATHEOS__
  if( d->m_bMousePressed && (!d->m_strSelectedURL.isEmpty() || (!innerNode.isNull() && innerNode.elementId() == ID_IMG) ) &&
      ( d->m_dragStartPos - _mouse->pos() ).manhattanLength() > KGlobalSettings::dndEventDelay() &&
      d->m_bDnd && d->m_mousePressNode == innerNode ) {

      QPixmap p;
      QDragObject *drag = 0;
      if( !d->m_strSelectedURL.isEmpty() ) {
          KURL u( completeURL( splitUrlTarget(d->m_strSelectedURL)) );
          KURL::List uris;
          uris.append(u);
          drag = KURLDrag::newDrag( uris, d->m_view->viewport() );
          p = KMimeType::pixmapForURL(u, 0, KIcon::SizeMedium);
      } else {
          HTMLImageElementImpl *i = static_cast<HTMLImageElementImpl *>(innerNode.handle());
          if( i ) {
              khtml::RenderImage *r = static_cast<khtml::RenderImage *>(i->renderer());
              if(r) {
                  drag = new QImageDrag( r->pixmap().convertToImage() , d->m_view->viewport() );
                  p = KMimeType::mimeType("image/*")->pixmap(KIcon::Desktop);
              }
          }
      }

    if ( !p.isNull() )
      drag->setPixmap(p);
//     else
//       kdDebug( 6000 ) << "null pixmap" << endl;


    stopAutoScroll();
    if(drag)
        drag->drag();

    // when we finish our drag, we need to undo our mouse press
    d->m_bMousePressed = false;
    d->m_strSelectedURL = "";
    return;
  }
#endif // __ATHEOS__

  QString target;
  QString surl = splitUrlTarget(url.string(), &target);

  // Not clicked -> mouse over stuff
  if ( !d->m_bMousePressed )
  {
    // The mouse is over something
    if ( url.length() )
    {
      bool shiftPressed = ( _mouse->state() & ShiftButton );

      // Image map
      if ( !innerNode.isNull() && innerNode.elementId() == ID_IMG )
      {
        HTMLImageElementImpl *i = static_cast<HTMLImageElementImpl *>(innerNode.handle());
        if ( i && i->isServerMap() )
        {
          khtml::RenderImage *r = static_cast<khtml::RenderImage *>(i->renderer());
          if(r)
          {
            int absx, absy, vx, vy;
            r->absolutePosition(absx, absy);
            view()->contentsToViewport( absx, absy, vx, vy );

            int x(_mouse->x() - vx), y(_mouse->y() - vy);

            d->m_overURL = surl + QString("?%1,%2").arg(x).arg(y);
            overURL( d->m_overURL, target, shiftPressed );
            return;
          }
        }
      }

      // normal link
      QString target;
      QString surl = splitUrlTarget(url.string(), &target);
      if ( d->m_overURL.isEmpty() || d->m_overURL != surl )
      {
        d->m_overURL = surl;
        overURL( d->m_overURL, target, shiftPressed );
      }
    }
    else  // Not over a link...
    {
      if( !d->m_overURL.isEmpty() ) // and we were over a link  -> reset to "default statusbar text"
      {
        d->m_overURL = QString::null;
        emit onURL( QString::null );
        // Default statusbar text can be set from javascript. Otherwise it's empty.
        emit setStatusBarText( d->m_kjsDefaultStatusBarText );
      }
    }
  }
  else
      // selection stuff
      if( d->m_bMousePressed && !innerNode.isNull() && ( _mouse->state() == LeftButton )) {
        int offset;
        DOM::Node node;
        //kdDebug(6000) << "KHTMLPart::khtmlMouseMoveEvent x=" << event->x() << " y=" << event->y()
        //              << " nodeAbsX=" << event->nodeAbsX() << " nodeAbsY=" << event->nodeAbsY()
        //              << endl;
        innerNode.handle()->findSelectionNode( event->x(), event->y(),
                                               event->nodeAbsX(), event->nodeAbsY(),
                                               node, offset );
        // When this stuff is finished, this should never happen.
        // But currently....
        if ( node.isNull() || !node.handle() )
        {
            //kdWarning( 6000 ) << "findSelectionNode returned no node" << endl;
            d->m_selectionEnd = innerNode;
            d->m_endOffset = 0; //?
        }
        else
        {
            d->m_selectionEnd = node;
            d->m_endOffset = offset;
        }
        //kdDebug( 6000 ) << "setting end of selection to " << d->m_selectionEnd.handle()->renderer() << "/"
        //                << d->m_endOffset << endl;

        // we have to get to know if end is before start or not...
        DOM::Node n = d->m_selectionStart;
        d->m_startBeforeEnd = false;
        while(!n.isNull()) {
            if(n == d->m_selectionEnd) {
                d->m_startBeforeEnd = true;
                break;
            }
            DOM::Node next = n.firstChild();
            if(next.isNull()) next = n.nextSibling();
            while( next.isNull() && !n.parentNode().isNull() ) {
                n = n.parentNode();
                next = n.nextSibling();
            }
            n = next;
            //d->m_view->viewport()->repaint(false);
        }

        if ( !d->m_selectionStart.isNull() && !d->m_selectionEnd.isNull() )
        {
          if (d->m_selectionEnd == d->m_selectionStart && d->m_endOffset < d->m_startOffset)
               d->m_doc
                  ->setSelection(d->m_selectionStart.handle(),d->m_endOffset,
                                 d->m_selectionEnd.handle(),d->m_startOffset);
          else if (d->m_startBeforeEnd)
              d->m_doc
                  ->setSelection(d->m_selectionStart.handle(),d->m_startOffset,
                                 d->m_selectionEnd.handle(),d->m_endOffset);
          else
              d->m_doc
                  ->setSelection(d->m_selectionEnd.handle(),d->m_endOffset,
                                 d->m_selectionStart.handle(),d->m_startOffset);
        }

      }
}

void KHTMLPart::khtmlMouseReleaseEvent( khtml::MouseReleaseEvent *event )
{
  QMouseEvent *_mouse = event->qmouseEvent();
  DOM::Node innerNode = event->innerNode();
  d->m_mousePressNode = DOM::Node();

  if ( d->m_bMousePressed )
  {
    // in case we started an autoscroll in MouseMove event
    // ###
    stopAutoScroll();
    //disconnect( this, SLOT( slotUpdateSelectText(int) ) );
  }

  // Used to prevent mouseMoveEvent from initiating a drag before
  // the mouse is pressed again.
  d->m_bMousePressed = false;

  if ( !d->m_strSelectedURL.isEmpty() && _mouse->button() != RightButton &&
       event->isURLHandlingEnabled() )
  {
     QString target;
     QString url;
     if( _mouse->button() == LeftButton ) {
       url = splitUrlTarget(d->m_strSelectedURL, &target);
     } else {
       url = splitUrlTarget(d->m_strSelectedURL);
       target = "_blank";
     }
//     kdDebug( 6000 ) << "m_strSelectedURL='" << url << "' target=" << target << endl;

     // Visual action effect, over text links
     if ( !innerNode.isNull() &&
          (innerNode.nodeType() == DOM::Node::TEXT_NODE || innerNode.nodeType() == DOM::Node::ELEMENT_NODE))
     {
       khtml::RenderObject* o = static_cast<khtml::RenderObject*>(innerNode.handle()->renderer());
       if(o) {
           int x, y;
           o->absolutePosition( x, y );
           int vx, vy;
           view()->contentsToViewport( x, y, vx, vy );
           // ### fix pseudostyle :active !
           if(o->style()) {
               QPainter p(view()->viewport());
               p.setPen( QPen(o->style()->color(), 0, DotLine));
               p.drawRect(vx-1,vy-1, o->width()+2,o->height()+2);
#ifdef __ATHEOS__
//	       d->m_view->viewport()->Invalidate();
//	       d->m_view->viewport()->Flush();
#else	       
               QTimer::singleShot(100, d->m_view->viewport(), SLOT(update()));
#endif	       
           }
       }
     }
     else if ( !innerNode.isNull() && innerNode.elementId() == ID_IMG ) {
       HTMLImageElementImpl *i = static_cast<HTMLImageElementImpl *>(innerNode.handle());
       if ( i && i->isServerMap() )
       {
         khtml::RenderImage *r = static_cast<khtml::RenderImage *>(i->renderer());
         if(r)
         {
           int absx, absy, vx, vy;
           r->absolutePosition(absx, absy);
           view()->contentsToViewport( absx, absy, vx, vy );

           int x(_mouse->x() - vx), y(_mouse->y() - vy);
           url += QString("?%1,%2").arg(x).arg(y);
         }
       }
     }

     urlSelected( url, _mouse->button(), _mouse->state(), target );
   }
   else if (_mouse->button() == MidButton)
   {
#ifdef __ATHEOS__
//       printf( "Warning: KHTMLPart::khtmlMouseReleaseEvent() no clipboard support\n" );
#else       
     QClipboard *cb = QApplication::clipboard();
     QCString plain("plain");
     QString url = cb->text(plain);
     KURL u(url);
     if (u.isValid())
        urlSelected(url, 0,0, "_top");
#endif     
   }

#if 0 // We shouldn't need this, we had to move here anyway
  if(!innerNode.isNull() && innerNode.nodeType() == DOM::Node::TEXT_NODE) {
  //    kdDebug( 6000 ) << "final range of selection to " << d->selectionStart << "/" << d->startOffset << " --> " << innerNode << "/" << offset << endl;
        d->m_selectionEnd = innerNode;
        d->m_endOffset = event->offset();
    }
#endif

    // delete selection in case start and end position are at the same point
    if(d->m_selectionStart == d->m_selectionEnd && d->m_startOffset == d->m_endOffset) {
        d->m_selectionStart = 0;
        d->m_selectionEnd = 0;
        d->m_startOffset = 0;
        d->m_endOffset = 0;
        emitSelectionChanged();
    } else {
        // we have to get to know if end is before start or not...
        DOM::Node n = d->m_selectionStart;
        d->m_startBeforeEnd = false;
        if( d->m_selectionStart == d->m_selectionEnd ) {
            if( d->m_startOffset < d->m_endOffset )
                d->m_startBeforeEnd = true;
        } else {
        while(!n.isNull()) {
            if(n == d->m_selectionEnd) {
                d->m_startBeforeEnd = true;
                break;
            }
            DOM::Node next = n.firstChild();
            if(next.isNull()) next = n.nextSibling();
            while( next.isNull() && !n.parentNode().isNull() ) {
                n = n.parentNode();
                next = n.nextSibling();
            }
            n = next;
        }
        }
        if(!d->m_startBeforeEnd)
        {
            DOM::Node tmpNode = d->m_selectionStart;
            int tmpOffset = d->m_startOffset;
            d->m_selectionStart = d->m_selectionEnd;
            d->m_startOffset = d->m_endOffset;
            d->m_selectionEnd = tmpNode;
            d->m_endOffset = tmpOffset;
            d->m_startBeforeEnd = true;
        }
        // get selected text and paste to the clipboard
        QString text = selectedText();
        text.replace(QRegExp(QChar(0xa0)), " ");
#ifdef __ATHEOS__
//	printf( "Warning: KHTMLPart::khtmlMouseReleaseEvent() does not update clipboard\n" );
#else
        QClipboard *cb = QApplication::clipboard();
        cb->setText(text);
#endif
        //kdDebug( 6000 ) << "selectedText = " << text << endl;
        emitSelectionChanged();
    }
}

#ifndef __ATHEOS__
void KHTMLPart::khtmlDrawContentsEvent( khtml::DrawContentsEvent * )
{
}

bool KHTMLPart::eventFilter( QObject* o, QEvent* ev )
{
    if ( ev->type() == QEvent::FocusOut && o == d->m_view->viewport() )
    {
        if ( d->m_bMousePressed )
        {
            stopAutoScroll();
            d->m_bMousePressed = false;
        }
    }
    return KParts::ReadOnlyPart::eventFilter( o, ev );
}

void KHTMLPart::guiActivateEvent( KParts::GUIActivateEvent *event )
{
  if ( event->activated() )
  {
    emitSelectionChanged();
    emit d->m_extension->enableAction( "print", d->m_doc != 0 );
    if ( !d->m_settings->autoLoadImages() && d->m_paLoadImages )
    {
        QList<KAction> lst;
        lst.append( d->m_paLoadImages );
        plugActionList( "loadImages", lst );
    }
  }
}
#endif // __ATHEOS__

void KHTMLPart::slotFind()
{
#ifdef __ATHEOS__
    printf( "Warning: KHTMLPart::slotFind() not implemented\n" );
#else    
  KHTMLPart *part = 0;

  if ( d->m_frames.count() > 0 )
    part = static_cast<KHTMLPart *>( partManager()->activePart() );

  if(!part)
      part = this;
#ifdef __ATHEOS__
  assert( dynamic_cast<KHTMLPart*>(part) != NULL );
#else  
  assert( part->inherits( "KHTMLPart" ) );
#endif
  // use the part's (possibly frame) widget as parent widget, so that it gets
  // properly destroyed when the (possible) frame dies
  if ( !d->m_findDialog ) {
      d->m_findDialog = new KHTMLFind( part, part->widget(), "khtmlfind" );
      connect( d->m_findDialog, SIGNAL( done() ),
               this, SLOT( slotFindDone() ) );
      connect( d->m_findDialog, SIGNAL( destroyed() ),
               this, SLOT( slotFindDialogDestroyed() ) );
  }

  d->m_findDialog->setPart( part );
  d->m_findDialog->setText( part->d->m_lastFindState.text );
  d->m_findDialog->setCaseSensitive( part->d->m_lastFindState.caseSensitive );
  d->m_findDialog->setDirection( part->d->m_lastFindState.direction );

  d->m_findDialog->show();

  d->m_paFind->setEnabled( false );
#endif
}

void KHTMLPart::slotFindDone()
{
    assert( d->m_findDialog );

    KHTMLPart *part = d->m_findDialog->part();

    // this code actually belongs into some saveState() method in
    // KHTMLFind, but as we're saving into the private data section of
    // the part we have to do it here (no way to access it from the outside
    // as it is defined only in khtml_part.cpp) (Simon)
    part->d->m_lastFindState.text = d->m_findDialog->getText();
    part->d->m_lastFindState.caseSensitive = d->m_findDialog->case_sensitive();
    part->d->m_lastFindState.direction = d->m_findDialog->get_direction();

#ifndef __ATHEOS__
    d->m_paFind->setEnabled( true );
#endif
}

void KHTMLPart::slotFindDialogDestroyed()
{
    assert( sender() == d->m_findDialog );

    d->m_findDialog = 0;
#ifndef __ATHEOS__
    d->m_paFind->setEnabled( true );
#endif
}

void KHTMLPart::slotPrintFrame()
{
  if ( d->m_frames.count() == 0 )
    return;

  KParts::Part *frame = partManager()->activePart();

  KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject( frame );

  if ( !ext )
    return;

#ifdef __ATHEOS__
  printf( "Warning: KHTMLPart::slotPrintFrame() no metaobject\n" );
#else
  QMetaData *mdata = ext->metaObject()->slot( "print()" );
  if ( mdata )
    (ext->*(mdata->ptr))();
#endif  
}

void KHTMLPart::slotSelectAll()
{
  KHTMLPart *part = this;

  if ( d->m_frames.count() > 0 && partManager()->activePart() )
    part = static_cast<KHTMLPart *>( partManager()->activePart() );

  assert( part );

  part->selectAll();
}

void KHTMLPart::startAutoScroll()
{
   connect(&d->m_scrollTimer, SIGNAL( timeout() ), this, SLOT( slotAutoScroll() ));
   d->m_scrollTimer.start(100, false);
}

void KHTMLPart::stopAutoScroll()
{
   disconnect(&d->m_scrollTimer, SIGNAL( timeout() ), this, SLOT( slotAutoScroll() ));
   if (d->m_scrollTimer.isActive())
       d->m_scrollTimer.stop();
}


void KHTMLPart::slotAutoScroll()
{
    if (d->m_view)
      d->m_view->doAutoScroll();
    else
      stopAutoScroll(); // Safety
}

void KHTMLPart::selectAll()
{
  NodeImpl *first;
  if (d->m_doc->isHTMLDocument())
    first = static_cast<HTMLDocumentImpl*>(d->m_doc)->body();
  else
    first = d->m_doc;
  NodeImpl *next;

  // Look for first text/cdata node that has a renderer
  while ( first && !((first->nodeType() == Node::TEXT_NODE || first->nodeType() == Node::CDATA_SECTION_NODE) && first->renderer()) )
  {
    next = first->firstChild();
    if ( !next ) next = first->nextSibling();
    while( first && !next )
    {
      first = first->parentNode();
      if ( first )
        next = first->nextSibling();
    }
    first = next;
  }

  NodeImpl *last;
  if (d->m_doc->isHTMLDocument())
    last = static_cast<HTMLDocumentImpl*>(d->m_doc)->body();
  else
    last = d->m_doc;
  // Look for last text/cdata node that has a renderer
  while ( last && !((last->nodeType() == Node::TEXT_NODE || last->nodeType() == Node::CDATA_SECTION_NODE) && last->renderer()) )
  {
    next = last->lastChild();
    if ( !next ) next = last->previousSibling();
    while ( last && !next )
    {
      last = last->parentNode();
      if ( last )
        next = last->previousSibling();
    }
    last = next;
  }

  if ( !first || !last )
    return;
  ASSERT(first->renderer());
  ASSERT(last->renderer());

  d->m_selectionStart = first;
  d->m_startOffset = 0;
  d->m_selectionEnd = last;
  d->m_endOffset = static_cast<TextImpl *>( last )->string()->l;
  d->m_startBeforeEnd = true;

  d->m_doc->setSelection( d->m_selectionStart.handle(), d->m_startOffset,
                          d->m_selectionEnd.handle(), d->m_endOffset );

  emitSelectionChanged();
}

bool KHTMLPart::checkLinkSecurity(const KURL &/*linkURL*/)
{
#ifdef __ATHEOS__
//    printf( "KHTMLPart::checkLinkSecurity() not implemented\n" );
#else    
  // Security check on the link.
  // KURL u( url ); Wrong!! Relative URL could be mis-interpreted!!! (DA)
  QString linkProto = linkURL.protocol().lower();
  QString proto = m_url.protocol().lower();

  if ( !linkProto.isEmpty() && !proto.isEmpty() &&
       ( linkProto == "cgi" || linkProto == "file" ) &&
       proto != "file" && proto != "cgi" && proto != "man")
  {
    Tokenizer *tokenizer = d->m_doc->tokenizer();
    if (tokenizer)
      tokenizer->setOnHold(true);
    KMessageBox::error( 0,
                        i18n( "This page is untrusted\nbut it contains a link to your local file system."),
                        i18n( "Security Alert" ));
    if (tokenizer)
      tokenizer->setOnHold(false);
    return false;
  }
#endif // __ATHEOS__  
  return true;
}

void KHTMLPart::slotPartRemoved( KParts::Part *part )
{
//    kdDebug(6050) << "KHTMLPart::slotPartRemoved " << part << endl;
    if ( part == d->m_activeFrame )
        d->m_activeFrame = 0L;
}

void KHTMLPart::slotActiveFrameChanged( KParts::Part *part )
{
//    kdDebug(6050) << "KHTMLPart::slotActiveFrameChanged part=" << part << endl;
    if ( part == this )
    {
        kdError(6050) << "strange error! we activated ourselves" << endl;
        assert( false );
        return;
    }
//    kdDebug(6050) << "KHTMLPart::slotActiveFrameChanged d->m_activeFrame=" << d->m_activeFrame << endl;
//    if ( d->m_activeFrame && d->m_activeFrame->widget()->inherits( "QFrame" ) )
    if ( d->m_activeFrame && dynamic_cast<QFrame*>(d->m_activeFrame->widget()) != NULL )
    {
        QFrame *frame = static_cast<QFrame *>( d->m_activeFrame->widget() );
        if (frame->frameStyle() != QFrame::NoFrame)
        {
           frame->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken);
           frame->repaint();
        }
    }

    d->m_activeFrame = part;

//    if ( d->m_activeFrame && d->m_activeFrame->widget()->inherits( "QFrame" ) )
    if ( d->m_activeFrame && dynamic_cast<QFrame*>(d->m_activeFrame->widget()) != NULL )
    {
        QFrame *frame = static_cast<QFrame *>( d->m_activeFrame->widget() );
        if (frame->frameStyle() != QFrame::NoFrame)
        {
           frame->setFrameStyle( QFrame::StyledPanel | QFrame::Plain);
           frame->repaint();
        }
        kdDebug(6050) << "new active frame " << d->m_activeFrame << endl;
    }

    updateActions();
    // (note: childObject returns 0 if the argument is 0)
#ifdef __ATHEOS__
    printf( "Warning: KHTMLPart::slotActiveFrameChanged() not emiting setExtensionProxy()\n" );
#else
    d->m_extension->setExtensionProxy( KParts::BrowserExtension::childObject( d->m_activeFrame ) );
#endif
}

void KHTMLPart::setActiveNode(const DOM::Node &node)
{
    if (!d->m_doc)
        return;
    // at the moment, only element nodes can receive focus.
    DOM::ElementImpl *e = static_cast<DOM::ElementImpl *>(node.handle());
    if (node.isNull() || e->isElementNode())
	d->m_doc->setFocusNode(e);
    if (!d->m_view || !e || e->ownerDocument()!=d->m_doc)
        return;
    QRect rect  = e->getRect();
    kdDebug()<<"rect.x="<<rect.x()<<" rect.y="<<rect.y()<<" rect.width="<<rect.width()<<" rect.height="<<rect.height()<<endl;
    d->m_view->ensureVisible(rect.right(), rect.bottom());
    d->m_view->ensureVisible(rect.left(), rect.top());
}

DOM::Node KHTMLPart::activeNode() const
{
    return DOM::Node(d->m_doc?d->m_doc->focusNode():0);
}









using namespace KParts;
#include "khtml_part.moc"
