/* This file is part of the KDE project
   Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
                      David Faure <faure@kde.org>

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

#ifndef __kparts_browserextension_h__
#define __kparts_browserextension_h__

#include <sys/types.h>

#include <qpoint.h>
#include <qlist.h>
#include <qdatastream.h>

#include <kparts/part.h>
#include <kparts/event.h>

#ifdef __ATHEOS__
//#include <atheos/connector.h>
#include <string>
namespace os {
    class Message;
//    class Messenger;
}

namespace KIO {
    class TransferJob;
}

//enum
//{
//    ID_HTML_OPEN_URL,
//    ID_HTML_SET_LOCATION_BAR_URL
//};

#endif

class KFileItem;
typedef QList<KFileItem> KFileItemList;

class QString;

namespace KParts {

struct URLArgsPrivate;

struct URLArgs
{
  URLArgs();
  URLArgs( const URLArgs &args );
  URLArgs &operator=( const URLArgs &args);
  URLArgs( bool reload, int xOffset, int yOffset, const QString &serviceType = QString::null );
  virtual ~URLArgs();
#ifdef __ATHEOS__
  URLArgs( const os::Message& cData );
  void Flatten( os::Message* pcData ) const;
#endif

  QStringList docState;

  bool reload;
  int xOffset;
  int yOffset;
  QString serviceType;

  QByteArray postData; //khtml specific stuff (POST)
  void setContentType( const QString & contentType ); // Header for POST
  QString contentType() const; // Header for POST

  QMap<QString, QString> &metaData();

  QString frameName;

  bool trustedSource;

  URLArgsPrivate *d;
};

struct WindowArgsPrivate;

struct WindowArgs
{
    WindowArgs();
    WindowArgs( const WindowArgs &args );
    WindowArgs &operator=( const WindowArgs &args );
    WindowArgs( const QRect &_geometry, bool _fullscreen, bool _menuBarVisible,
                bool _toolBarsVisible, bool _statusBarVisible, bool _resizable );
    WindowArgs( int _x, int _y, int _width, int _height, bool _fullscreen,
                bool _menuBarVisible, bool _toolBarsVisible,
                bool _statusBarVisible, bool _resizable );

    int x;
    int y;
    int width;
    int height;
    bool fullscreen; //defaults to false
    bool menuBarVisible; //defaults to true
    bool toolBarsVisible; //defaults to true
    bool statusBarVisible; //defaults to true
    bool resizable; //defaults to true

    bool lowerWindow; //defaults to false

    WindowArgsPrivate *d; // yes, I am paranoid :-)
};

#ifndef __ATHEOS__
class OpenURLEvent : public Event
{
public:
  OpenURLEvent( ReadOnlyPart *part, const KURL &url, const URLArgs &args = URLArgs() );
  virtual ~OpenURLEvent();

  ReadOnlyPart *part() const { return m_part; }
  KURL url() const { return m_url; }
  URLArgs args() const { return m_args; }

  static bool test( const QEvent *event ) { return Event::test( event, s_strOpenURLEvent ); }

private:
  static const char *s_strOpenURLEvent;
  ReadOnlyPart *m_part;
  KURL m_url;
  URLArgs m_args;

  class OpenURLEventPrivate;
  OpenURLEventPrivate *d;
};
#endif // __ATHEOS__

#ifdef __ATHEOS__

class BrowserExtension;

struct DownloadStream
{
    std::string		m_cURL;
    std::string		m_cPreferredName;
    std::string 	m_cMimeType;
    int			m_nContentSize;
    void*		m_pUserData;
};

class BrowserCallback
{
public:
    virtual void EnableAction( const char* name, bool enabled ) = 0;
    virtual void OpenURLRequest( const std::string &url, const KParts::URLArgs& args ) = 0;
    virtual void OpenURLNotify() = 0;
    virtual void Completed() = 0;
    virtual void SetLocationBarURL( const std::string &url ) = 0;
    virtual void CreateNewWindow( const std::string& url, const KParts::URLArgs& args ) = 0;
    virtual KParts::ReadOnlyPart* CreateNewWindow( const std::string& url, const KParts::URLArgs& args,
						   const KParts::WindowArgs& windowArgs ) = 0;

    virtual void PopupMenu( BrowserExtension* pcSource, const std::string& cURL ) = 0;
    
    virtual void LoadingProgress( int percent ) = 0;
    virtual void SpeedProgress( int bytesPerSecond ) = 0;
    virtual void InfoMessage( const std::string& cString ) = 0;
    virtual void SetWindowCaption( ReadOnlyPart* pcPart, const std::string &caption ) = 0;
    virtual void SetStatusBarText( const std::string &text ) = 0;
    
/*  void PopupMenu( const QPoint &global, const KFileItemList &items );
    void PopupMenu( KXMLGUIClient *client, const QPoint &global, const KFileItemList &items );
    void PopupMenu( const QPoint &global, const KURL &url,
    const QString &mimeType, mode_t mode = (mode_t)-1 );
    void PopupMenu( KXMLGUIClient *client,
    const QPoint &global, const KURL &url,
    const QString &mimeType, mode_t mode = (mode_t)-1 );*/

//  void SelectionInfo( const KFileItemList &items );
    virtual void SelectionInfo( const std::string& text ) = 0;
//  void SelectionInfo( const KURL::List &urls );
    
    virtual bool BeginDownload( DownloadStream* psStream ) = 0;
    virtual bool HandleDownloadData( DownloadStream* psStream, const void* pData, int nLen ) = 0;
    virtual void DownloadFinished( DownloadStream* psStream, int nError ) = 0;
};

#endif // __ATHEOS__

class BrowserExtensionPrivate;

 /**
  * The following standard actions are defined by the host of the view :
  *
  * [selection-dependent actions]
  * @li @p cut : Copy selected items to clipboard and store 'not cut' in clipboard.
  * @li @p copy : Copy selected items to clipboard and store 'cut' in clipboard.
  * @li @p paste : Paste clipboard into selected items.
  * @li @p rename : Rename item in place.
  * @li @p trash : Move selected items to trash.
  * @li @p del : Delete selected items (couldn't call it delete!).
  * @li @p shred : Shred selected items (secure deletion).
  * @li @p properties : Show file/document properties.
  * @li @p editMimeType : show file/document's mimetype properties.
  *
  * [normal actions]
  * @li @p print : Print :-)
  * @li @p reparseConfiguration : Re-read configuration and apply it.
  * @li @p refreshMimeTypes : If the view uses mimetypes it should re-determine them.
  *
  *
  * The view defines a slot with the name of the action in order to implement the action.
  * The browser will detect the slot automatically and connect its action to it when
  * appropriate (i.e. when the view is active).
  *
  *
  * The selection-dependent actions are disabled by default and the view should
  * enable them when the selection changes, emitting @ref enableAction().
  *
  * The normal actions do not depend on the selection. For each slot that is
  * defined in the second list, the action is automatically enabled.
  *
  * A special case is the configuration slots, not connected to any action directly,
  * and having parameters.
  *
  * [configuration slot]
  * @li @p setSaveViewPropertiesLocally( bool ): If @p true, view properties are saved into .directory
  *                                       otherwise, they are saved globally.
  */
class BrowserExtension : public QObject //, public ConnectTransmitter
{
  Q_OBJECT
  Q_PROPERTY( bool urlDropHandling READ isURLDropHandlingEnabled WRITE setURLDropHandlingEnabled )
public:
  /**
   * Constructor
   *
   * @param parent The KParts::ReadOnlyPart that this extension ... "extends" :)
   * @param name An optional name for the extension.
   */
  BrowserExtension( KParts::ReadOnlyPart *parent,
                    const char *name = 0L );


  virtual ~BrowserExtension();
#ifdef __ATHEOS__
//  void SetMsgTarget( const os::Messenger& cMsgTarget );
//  os::Messenger& GetMsgTarget();
  void saveState( os::Message* pcBuffer );
  void restoreState( const os::Message& cBuffer );

  void SetCallback( BrowserCallback* pcCallback );
  BrowserCallback* GetCallback() const;
  void SetChildFlag( bool bIsChild );

  void SaveURL( const std::string& cURL, const std::string& cCaption );
    
  void BeginDownload( KIO::TransferJob* job, const QString& cURL, const QString& cPreferredName, const QString& cMimeType );
#endif
  virtual void setURLArgs( const URLArgs &args );

  virtual URLArgs urlArgs();

  /**
   * Returns the current x offset.
   *
   * For a scrollview, implement this using contentsX().
   */
  virtual int xOffset();
  /**
   * Returns the current y offset.
   *
   * For a scrollview, implement this using contentsY().
   */
  virtual int yOffset();

  /**
   * Used by the browser to save the current state of the view
   * (in order to restore it if going back in navigation).
   *
   * If you want to save additionnal properties, reimplement it
   * but don't forget to call the parent method (probably first).
   */
  virtual void saveState( QDataStream &stream );

  /**
   * Used by the browser to restore the view in the state
   * it was when we left it.
   *
   * If you saved additionnal properties, reimplement it
   * but don't forget to call the parent method (probably first).
   */
  virtual void restoreState( QDataStream &stream );

  bool isURLDropHandlingEnabled() const;
  void setURLDropHandlingEnabled( bool enable );

  /**
   * Retrieve a map containing the action names as keys and corresponding
   * SLOT()'ified method names as data entries.
   *
   * This is very useful for
   * the host component, when connecting the own signals with the
   * extension's slots.
   * Basically you iterate over the map, check if the extension implements
   * the slot and connect to the slot using the data value of your map
   * iterator.
   * Checking if the extension implements a certain slot can be done like this:
   *
   * <pre>
   *   extension->metaObject()->slotNames().contains( actionName + "()" )
   * </pre>
   *
   * (note that @p actionName is the iterator's key value if already
   *  iterating over the action slot map, returned by this method)
   *
   * Connecting to the slot can be done like this:
   *
   * <pre>
   *   connect( yourObject, SIGNAL( yourSignal() ),
   *            extension, mapIterator.data() )
   * </pre>
   *
   * (where "mapIterator" is your QMap<QCString,QCString> iterator)
   */
  static QMap<QCString,QCString> actionSlotMap();

  /**
   * Queries @p obj for a child object which inherits from this
   * BrowserExtension class. Convenience method.
   */
  static BrowserExtension *childObject( QObject *obj );

// KDE invents support for public signals...
#undef signals
#define signals public
signals:
#undef signals
#define signals protected
  /**
   * Enable or disable a standard action held by the browser.
   *
   * See class documentation for the list of standard actions.
   */
  void enableAction( const char * name, bool enabled );

  /**
   * Ask the host (browser) to open @p url
   * To set a reload, the x and y offsets, the service type etc., fill in the
   * appropriate fields in the @p args structure.
   * Hosts should not connect to this signal but to @ref openURLRequestDelayed.
   */
  void openURLRequest( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );

  /**
   * This signal is emitted when openURLRequest is called, after a 0-seconds timer.
   * This allows the caller to terminate what it's doing first, before (usually)
   * being destroyed. Parts should never use this signal, hosts should only connect
   * to this signal.
   */
  void openURLRequestDelayed( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );

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
  void openURLNotify();

  /**
   * Update the URL shown in the browser's location bar to @p url.
   */
  void setLocationBarURL( const QString &url );

  /**
   * Ask the hosting browser to open a new window for the given @url.
   *
   * The @p args argument is optional additionnal information for the
   * browser,
   * @see KParts::URLArgs
   */
  void createNewWindow( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );

  /**
   * Ask the hosting browser to open a new window for the given @url
   * and return a reference to the content part.
   * The request for a reference to the part is only fullfilled/processed
   * if the serviceType is set in the @p args . (otherwise the request cannot be
   * processed synchroniously.
   */
  void createNewWindow( const KURL &url, const KParts::URLArgs &args,
                        const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part );

  /**
   * Since the part emits the jobid in the @ref started() signal,
   * progress information is automatically displayed.
   *
   * However, if you don't use a @ref KIO::Job in the part,
   * you can use @ref loadingProgress() and @ref speedProgress()
   * to display progress information.
   */
  void loadingProgress( int percent );
  /**
   * @see loadingProgress
   */
  void speedProgress( int bytesPerSecond );

  void infoMessage( const QString & );

  /**
   * Emit this to make the browser show a standard popup menu
   * at the point @p global for the files @p items.
   */
  void popupMenu( const QPoint &global, const KFileItemList &items );

  /**
   * Emit this to make the browser show a standard popup menu
   * at the point @p global for the files @p items.
   *
   * The GUI described by @p client is being merged with the popupmenu of the host
   */
  void popupMenu( KXMLGUIClient *client, const QPoint &global, const KFileItemList &items );

  /**
   * Emit this to make the browser show a standard popup menu
   * at the point @p global for the given @p url.
   *
   * Give as much information
   * about this URL as possible, like the @p mimeType and the file type
   * (@p mode: S_IFREG, S_IFDIR...)
   */
  void popupMenu( const QPoint &global, const KURL &url,
                  const QString &mimeType, mode_t mode = (mode_t)-1 );

  /**
   * Emit this to make the browser show a standard popup menu
   * at the point @p global for the given @p url.
   *
   * Give as much information
   * about this URL as possible, like the @p mimeType and the file type
   * (@p mode: S_IFREG, S_IFDIR...)
   * The GUI described by @p client is being merged with the popupmenu of the host
   */
  void popupMenu( KXMLGUIClient *client,
                  const QPoint &global, const KURL &url,
                  const QString &mimeType, mode_t mode = (mode_t)-1 );

  void selectionInfo( const KFileItemList &items );
  void selectionInfo( const QString &text );
  void selectionInfo( const KURL::List &urls );

private slots:
  void slotCompleted();
  void slotOpenURLRequest( const KURL &url, const KParts::URLArgs &args );
  void slotEmitOpenURLRequestDelayed();


#ifdef __ATHEOS__
  void slotData( KIO::Job*, const QByteArray &data );
  void slotDownloadFinished(  KIO::Job* job );
  void slotMimeTypeFound( KIO::Job*, const QString& );


  void slot_setWindowCaption( const QString &caption );
  void slot_setStatusBarText( const QString &text );
  void slot_popupMenu(const QString&, const QPoint& );

  void slot_enableAction( const char * name, bool enabled );
//  void slot_openURLRequest( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
  void slot_openURLRequestDelayed( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
  void slot_openURLNotify();
  void slot_setLocationBarURL( const QString &url );
  void slot_createNewWindow( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
  void slot_createNewWindow( const KURL &url, const KParts::URLArgs &args,
			     const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part );
  void slot_loadingProgress( int percent );
  void slot_speedProgress( int bytesPerSecond );
  void slot_infoMessage( const QString & );
    
  void slot_popupMenu( const QPoint &global, const KFileItemList &items );
  void slot_popupMenu( KXMLGUIClient *client, const QPoint &global, const KFileItemList &items );
  void slot_popupMenu( const QPoint &global, const KURL &url,
		       const QString &mimeType, mode_t mode = (mode_t)-1 );
  void slot_popupMenu( KXMLGUIClient *client,
		       const QPoint &global, const KURL &url,
		       const QString &mimeType, mode_t mode = (mode_t)-1 );

  void slot_selectionInfo( const KFileItemList &items );
  void slot_selectionInfo( const QString &text );
  void slot_selectionInfo( const KURL::List &urls );

    
#endif

    
private:
  KParts::ReadOnlyPart *m_part;
  URLArgs m_args;
  BrowserExtensionPrivate *d;
};

/**
 * An extension class for container parts.
 *
 */
class BrowserHostExtension : public QObject
{
  Q_OBJECT
public:
  BrowserHostExtension( KParts::ReadOnlyPart *parent,
                        const char *name = 0L );

  virtual ~BrowserHostExtension();

  /**
   * Returns a list of the names of all hosted child objects.
   *
   * Note that this method does not query the child objects recursively.
   */
  virtual QStringList frameNames() const;

  /**
   * Returns a list of pointers to all hosted child objects.
   *
   * Note that this method does not query the child objects recursively.
   */
  virtual const QList<KParts::ReadOnlyPart> frames() const;

  /**
   * Opens the given url in a hosted child frame. The frame name is specified in the
   * frameName variable in the urlArgs argument structure (see @ref KParts::URLArgs ) .
   */
  virtual bool openURLInFrame( const KURL &url, const KParts::URLArgs &urlArgs );

  /**
   * Queries @p obj for a child object which inherits from this
   * BrowserHostExtension class. Convenience method.
   */
  static BrowserHostExtension *childObject( QObject *obj );

private:
  class BrowserHostExtensionPrivate;
  BrowserHostExtensionPrivate *d;
};

};

#endif

