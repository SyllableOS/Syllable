#ifndef __F_LOADER_ATHEOS_H__
#define __F_LOADER_ATHEOS_H__

#include <kurl.h>

#include <qmap.h>

#include <kio/jobclasses.h>

namespace DOM {
    class DOMString;
}
/*
namespace KIO
{
}
*/
namespace khtml
{
#if 0
}
#endif


class LoaderPrivate;
class Loader;
class CachedCSSStyleSheet;
class CachedScript;
class CachedImage;
/*
class Loader : public QObject
{
//    Q_OBJECT
public:
    Loader();
    ~Loader();

    void load(CachedObject *object, const DOM::DOMString &baseURL, bool incremental = true);

    int numRequests( const DOM::DOMString &baseURL ) const;
//    int numRequests( const DOM::DOMString &baseURL, CachedObject::Type type ) const;

    void cancelRequests( const DOM::DOMString &baseURL );

      // may return 0L
//    KIO::Job *jobForRequest( const DOM::DOMString &url ) const;

//    signals:
//    void requestDone( const DOM::DOMString &baseURL, khtml::CachedObject *obj );
//    void requestFailed( const DOM::DOMString &baseURL, khtml::CachedObject *obj );

// protected slots:
// void slotFinished( KIO::Job * );
//    void slotData( KIO::Job *, const QByteArray & );


    static void Run();
    
private:
    LoaderPrivate* d;
    
//    void servePendingRequests();

//    QList<Request> m_requestsPending;
//    QPtrDict<Request> m_requestsLoading;
//#ifdef HAVE_LIBJPEG
//    KJPEGFormatType m_jpegloader;
//#endif
};
*/


class Cache
{
    friend class DocLoader;
public:
      /**
       * init the cache in case it's not already. This needs to get called once
       * before using it.
       */
    static void init();

      /**
       * Ask the cache for some url. Will return a cachedObject, and
       * load the requested data in case it's not cahced
       */
    static CachedImage *requestImage( const DOM::DOMString &url, const DOM::DOMString &baseUrl, bool reload=false, int _expireDate=0);

      /**
       * Ask the cache for some url. Will return a cachedObject, and
       * load the requested data in case it's not cahced
       */
    static CachedCSSStyleSheet *requestStyleSheet( const DOM::DOMString &url, const DOM::DOMString &baseUrl, bool reload=false, int _expireDate=0);

      /**
       * Ask the cache for some url. Will return a cachedObject, and
       * load the requested data in case it's not cahced
       */
    static CachedScript *requestScript( const DOM::DOMString &url, const DOM::DOMString &baseUrl, bool reload=false, int _expireDate=0);

      /**
       * sets the size of the cache. This will only hod approximately, since the size some
       * cached objects (like stylesheets) take up in memory is not exaclty known.
       */
//    static void setSize( int bytes );
      /** returns the size of the cache */
//    static int size() { return maxSize; };

      /** prints some statistics to stdout */
//    static void statistics();

      /** clean up cache */
    static void flush(bool force=false);

      /**
       * clears the cache
       * Warning: call this only at the end of your program, to clean
       * up memory (useful for finding memory holes)
       */
    static void clear();

    static Loader *loader() { return m_loader; }

    static KURL completeURL(const DOM::DOMString &url, const DOM::DOMString &baseUrl);

    static QPixmap *nullPixmap;
    static QPixmap *brokenPixmap;

    static void removeCacheEntry( CachedObject *object );

    static void autoloadImages( bool enable );
    static bool autoloadImages() { return s_autoloadImages; }

    static void Run();
protected:
      /*
       * @internal
       */
    class LRUList : public QStringList
    {
    public:
	  /**
	   * implements the LRU list
	   * The least recently used item is at the beginning of the list.
	   */
	void touch( const QString &url )
	{
	    remove( url );
	    append( url );
	}
    };


    static QDict<CachedObject> *cache;
    static LRUList *lru;

    static int maxSize;
    static int flushCount;

    static Loader *m_loader;

    static bool s_autoloadImages;

//    static unsigned long s_ulRefCnt;
};


} // end of namespace

#endif // __F_LOADER_ATHEOS_H__
