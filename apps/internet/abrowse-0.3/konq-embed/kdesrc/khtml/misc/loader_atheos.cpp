#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "loader.h"
#include <kio/job.h>
#include <kdebug.h>

#include <atheos/time.h>
#include <gui/bitmap.h>
#include <gui/guidefines.h>
#include <util/message.h>
#include <util/string.h>
#include <util/locker.h>
#include <util/resources.h>

#include <storage/file.h>
#include <translation/translator.h>

#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

//#include <kio/http/kcookiejar/kcookiejar.h>
#include "loader_cookies.h"

#include <atheos/amutex.h>

#define min( a, b ) (((a) < (b)) ? (a) : (b))


static os::Bitmap* load_icon( os::StreamableIO* pcStream )
{
    os::Translator* pcTrans = NULL;

    os::TranslatorFactory* pcFactory = new os::TranslatorFactory;
    pcFactory->LoadAll();
    
    os::Bitmap* pcBitmap = NULL;
    
    try {
	uint8  anBuffer[8192];
	uint   nFrameSize = 0;
	int    x = 0;
	int    y = 0;
	
	os::BitmapFrameHeader sFrameHeader;
	for (;;) {
	    int nBytesRead = pcStream->Read( anBuffer, sizeof(anBuffer) );

	    if (nBytesRead == 0 ) {
		break;
	    }

	    if ( pcTrans == NULL ) {
		int nError = pcFactory->FindTranslator( "", os::TRANSLATOR_TYPE_IMAGE, anBuffer, nBytesRead, &pcTrans );
		if ( nError < 0 ) {
		    return( NULL );
		}
	    }
	    
	    pcTrans->AddData( anBuffer, nBytesRead, nBytesRead != sizeof(anBuffer) );

	    while( true ) {
	    if ( pcBitmap == NULL ) {
		os::BitmapHeader sBmHeader;
		if ( pcTrans->Read( &sBmHeader, sizeof(sBmHeader) ) != sizeof( sBmHeader ) ) {
		    break;
		}
		pcBitmap = new os::Bitmap( sBmHeader.bh_bounds.Width() + 1, sBmHeader.bh_bounds.Height() + 1, os::CS_RGB32 );
	    }
	    if ( nFrameSize == 0 ) {
		if ( pcTrans->Read( &sFrameHeader, sizeof(sFrameHeader) ) != sizeof( sFrameHeader ) ) {
		    break;
		}
		x = sFrameHeader.bf_frame.left * 4;
		y = sFrameHeader.bf_frame.top;
		nFrameSize = sFrameHeader.bf_data_size;
	    }
	    uint8 pFrameBuffer[8192];
	    nBytesRead = pcTrans->Read( pFrameBuffer, min( nFrameSize, sizeof(pFrameBuffer) ) );
	    if ( nBytesRead <= 0 ) {
		break;
	    }
	    nFrameSize -= nBytesRead;
	    uint8* pDstRaster = pcBitmap->LockRaster();
	    int nSrcOffset = 0;
	    for ( ; y <= sFrameHeader.bf_frame.bottom && nBytesRead > 0 ; ) {
		int nLen = min( nBytesRead, sFrameHeader.bf_bytes_per_row - x );
		memcpy( pDstRaster + y * pcBitmap->GetBytesPerRow() + x, pFrameBuffer + nSrcOffset, nLen );
		if ( x + nLen == sFrameHeader.bf_bytes_per_row ) {
		    x = 0;
		    y++;
		} else {
		    x += nLen;
		}
		nBytesRead -= nLen;
		nSrcOffset += nLen;
	    }
	    }
	}
	return( pcBitmap );
    }
    catch( ... ) {
	return( NULL );
    }
}


using namespace DOM;

#define MAX_THREADS 6


static KCookieJar g_cCookieJar;

namespace khtml {

class Request;

class LoaderLock
{
public:
    void Lock() { GlobalMutex::Lock(); }
    void Unlock() { GlobalMutex::Unlock(); }
};

static sem_id		      g_hJobQueue;
static LoaderLock	      g_cLoaderLock;
//static os::Locker	      g_cLoaderLock( "http_loader_lock" );
static std::list<Request*>    g_cPendingRequests;
static std::list<Request*>    g_cRequests;
static std::list<Request*>    g_cFinishedRequests;
static int		      g_nWorkerThreadCount = 0;
static int		      g_nRunningRequestCount = 0;

    
class Request
{
public:
    Request( const DOM::DOMString& cBaseURL ) { m_nRefCount = 1; m_baseURL = cBaseURL; m_nLoadError = 0; m_bDone = false; m_bCanceled = false; m_pcObject = NULL; m_bIsProcessing = false; }
    
    virtual bool Run() = 0;
    virtual bool RunInput() = 0;
    virtual void HandleError( int nError ) = 0;
    virtual void Stop() = 0;

    virtual void Error( int nError ) { m_nLoadError = nError; }
    
    void AddRef() { atomic_add( &m_nRefCount, 1 ); }
    void Release() {
	if ( atomic_add( &m_nRefCount, -1 ) == 1 ) {
	    assert( m_bIsProcessing == false );

	    std::list<Request*>::iterator i;

	    g_cLoaderLock.Lock();
	    i = std::find( g_cPendingRequests.begin(), g_cPendingRequests.end(), this );
	    if ( i != g_cPendingRequests.end() ) {
		g_cPendingRequests.erase( i );
	    }
	    g_cLoaderLock.Unlock();
	    
	    delete this;
	}
    }
    
    void AddData( const char* pData, int nLen, bool /*bLast*/ ) {
	m_cInBuffer.insert( m_cInBuffer.end(), pData, pData + nLen );
    }
    void AddMetaData( const std::string& cMetaData ) {
	m_cMetaDataList.push_back( cMetaData );
    }

    std::vector<char>	m_cInBuffer;
    std::vector<string> m_cMetaDataList;
    bool		m_bDone;
    bool		m_bCanceled;
    DOM::DOMString	m_baseURL;


    CachedObject*  m_pcObject;

    bool m_bIsProcessing;
protected:    
    virtual ~Request() {}
    int		m_nLoadError;
private:    

    atomic_t m_nRefCount;
    
};

class HTTPLoader : public Request
{
public:
    HTTPLoader( const KURL& cURL, const DOM::DOMString& cBaseURL );
    virtual ~HTTPLoader() { if ( m_nSocket != -1 ) close( m_nSocket ); }
    virtual bool Run();
    virtual bool RunInput();
    virtual void Stop();

    virtual void DataReceived( const char* pzData, int nLen, bool bFinal ) = 0;
    virtual void MetadataReceived( const std::string& cKey, const std::string& cValue ) = 0;
    virtual void MimetypeReceived( const std::string& /*cType*/ ) {}
    virtual void Done() = 0;
    virtual void Redirect( const std::string& /*cLocation*/ ) {}

    void SetPostData( const char* pData, int nSize ) {
	m_cPostData.insert( m_cPostData.end(), pData, pData + nSize );
    }
protected:    
    KURL	m_cURL;
    os::String	m_cRequestBuffer;
    os::String	m_cClientRequestBuffer;
    QCString	m_cRecvCookieStr;
    int		m_nSocket;
    bool	m_bIsRunning;
private:
    std::string 	m_cLocationStr;
    std::string		m_cMimeType;
    std::vector<char>	m_cPostData;
};



class FileRequest : public Request
{
public:
    FileRequest( CachedObject* pcObject, const DOM::DOMString& cBaseURL, const char* pzPath );
    ~FileRequest() { m_pcObject->setRequest( 0 ); }
    virtual bool Run();
    virtual bool RunInput() { return( false ); }
    virtual void Stop() { m_bCanceled = true; m_pcObject->finish(); }
    virtual void HandleError( int nError ) { m_pcObject->error( nError, "" ); }
    QBuffer        m_cBuffer;
    os::File*      m_pcFile;
};

class TransfereJobFile : public KIO::TransferJob, public Request
{
public:
    TransfereJobFile( const DOM::DOMString& cBaseURL, const char* pzPath );
    virtual ~TransfereJobFile() { delete m_pcFile; }
    virtual bool Run();
    virtual bool RunInput() { return( false ); }
    virtual void HandleError( int nError ) { setError( nError ); emit result( this ); };

    virtual void Start() { m_bStarted = true; }
    virtual void Stop() { m_bCanceled = true; }
    virtual void putOnHold() { m_bStarted = false; }

    virtual void kill();

    virtual KURL url() { return( "" ); }
    
    
private:    
    os::File*      m_pcFile;
    bool	   m_bStarted;
    bool	   m_bDone;
    
};



class TransfereJobHTTP : public KIO::TransferJob, public HTTPLoader
{
public:
    TransfereJobHTTP( const KURL& cURL, const DOM::DOMString& cBaseURL );
    virtual ~TransfereJobHTTP() {}
    
    virtual void Start();
    virtual void putOnHold();
    virtual void DataReceived( const char* pzData, int nLen, bool bFinal );
    virtual void MetadataReceived( const std::string& cKey, const std::string& cValue );
    virtual void MimetypeReceived( const std::string& cType );
    virtual void Redirect( const std::string& cLocation );
    virtual void Done();
//    virtual void Error( int nError );
    virtual void HandleError( int nError ) { setError( nError ); emit result( this ); };

    
    virtual void kill();

    virtual KURL url() { return( m_cURL ); }
    
    virtual void    addMetaData( const QString &key, const QString &value );
    virtual void    addMetaData( const QMap<QString,QString> &values );
    virtual QString queryMetaData( const QString &key );
    
private:
//    std::string				m_cURL;
    QByteArray  			m_cReceiveBuffer;
    std::map<os::String,os::String>	m_cReceivedMetaData;
    bigtime_t			        m_nLastUpdateTime;
};

class HTTPRequest : public HTTPLoader
{
public:
    HTTPRequest( CachedObject* pcObject, const DOM::DOMString& cBaseURL, const KURL& cURL /*const char* pzHost, const char* pzPath*/ );
    ~HTTPRequest() { m_pcObject->setRequest( 0 ); }
    
    virtual void Start();
    virtual void Stop() { HTTPLoader::Stop(); m_pcObject->finish(); }
    virtual void HandleError( int nError ) { m_pcObject->error( nError, "" ); }

    virtual void DataReceived( const char* pzData, int nLen, bool bFinal );
    virtual void MetadataReceived( const std::string& cKey, const std::string& cValue );
    virtual void Done();
//    virtual void Error( int nError ) { m_nError = nError; }
private:;    
    QBuffer        m_cBuffer;
};


} // end of khtml namespace

using namespace khtml;

static int32 worker_thread( void* /*pData*/ )
{
    int nError;

    for (;;) {
	nError = lock_semaphore( g_hJobQueue );
	if ( nError < 0 ) {
	    if ( errno == EINTR ) {
		continue;
	    } else {
		atomic_add( &g_nWorkerThreadCount, -1 );
		break;
	    }
	}
	Request* pcRequest = NULL;
	g_cLoaderLock.Lock();
	if ( g_cPendingRequests.size() > 0 ) {
	    pcRequest = *g_cPendingRequests.begin();
	    g_cPendingRequests.erase( g_cPendingRequests.begin() );
	} else {
	    g_cLoaderLock.Unlock();
	    continue;
	}
	pcRequest->m_bIsProcessing = true;
	pcRequest->AddRef();
	g_cLoaderLock.Unlock();

	atomic_add( &g_nRunningRequestCount, 1 );
	pcRequest->RunInput();
	atomic_add( &g_nRunningRequestCount, -1 );
	
	g_cLoaderLock.Lock();
	g_cFinishedRequests.push_back( pcRequest );
	bool bDoExit = false;
	if ( g_cPendingRequests.empty() && g_nWorkerThreadCount > 1 ) {
	    atomic_add( &g_nWorkerThreadCount, -1 );
	    bDoExit = true;
	}
	pcRequest->m_bIsProcessing = false;
	g_cLoaderLock.Unlock();
	if ( bDoExit ) {
	    break;
	}
    }
    return( 0 );
}


void init_loader()
{
    static bool bIsInitiated = false;

    if ( bIsInitiated ) {
	return;
    }
    bIsInitiated = true;
    g_hJobQueue = create_semaphore( "http_job_queue", 0, 0 );
/*    
    for ( int i = 0 ; i < 8 ; ++i ) {
	thread_id hTread = spawn_thread( "http_worker", worker_thread, 0, 0, NULL );
	resume_thread( hTread );
    }*/
}


static void add_http_request( HTTPLoader* pcLoader )
{
    g_cLoaderLock.Lock();
    g_cPendingRequests.push_back( pcLoader );
    unlock_semaphore( g_hJobQueue );

    if ( g_nWorkerThreadCount < MAX_THREADS && g_nWorkerThreadCount < atomic_t(g_cPendingRequests.size() + g_nRunningRequestCount) ) {
	thread_id hTread = spawn_thread( "http_worker", worker_thread, 0, 0, NULL );
	if ( hTread >= 0 ) {
	    resume_thread( hTread );
	    atomic_add( &g_nWorkerThreadCount, 1 );
	}
    }
    g_cLoaderLock.Unlock();
}




HTTPLoader::HTTPLoader( const KURL& cURL, const DOM::DOMString& cBaseURL ) : Request( cBaseURL )
{
//"", url.url().ascii(), url.host().ascii(), url.encodedPathAndQuery( 0, true ).ascii()
    m_cMimeType = "text/html";
    m_cURL = cURL;
    m_nSocket = -1;
//    m_cURL += pzPath;

//    m_cHost = pzHost;
//    m_cRequest = pzPath;


    
//    add_http_request( this );
    m_bIsRunning = false;
}

bool HTTPLoader::RunInput()
{
    std::string cMetaDataBuffer;
    struct sockaddr_in sAddress;
    uint nBytesSendt = 0;
    char anBuffer[65536];
    bool bReadingHeaders = true;
//    int  nSocket;
    struct hostent* psHost = NULL;

    if ( m_nSocket != -1 ) {
	close( m_nSocket );
	m_nSocket = -1;
    }

    if ( m_cURL.host().ascii() != NULL ) {
	psHost = gethostbyname( m_cURL.host().ascii() );
    }
    if ( psHost == NULL ) {
	printf( "Error: FAILED TO LOOKUP '%s'\n", m_cURL.url().ascii() );
	m_bDone = true;
	Error( KIO::ERR_UNKNOWN_HOST );
	return( false );
    }

    m_nSocket = socket( AF_INET, SOCK_STREAM, 0 );

    if ( m_nSocket < 0 ) {
	Error( KIO::ERR_COULD_NOT_CREATE_SOCKET );
	m_bDone = true;
	return( false );
    }
    
    m_cRequestBuffer = os::String().Format( "%s %s HTTP/1.0\r\n", (m_cPostData.empty()) ? "GET" : "POST", m_cURL.encodedPathAndQuery( 0, true ).ascii() );
    m_cRequestBuffer += "User-Agent: Mozilla/5.0 (compatible; ABrowse 0.3; AtheOS\r\n";
    
    m_cRequestBuffer += os::String().Format( "Host: %s\r\n", m_cURL.host().ascii() );
    m_cRequestBuffer += "Connection: close\r\n";
    m_cRequestBuffer += m_cClientRequestBuffer;
    if ( m_cPostData.size() > 0 ) {
	m_cRequestBuffer += os::String().Format( "Content-Length: %d\r\n", m_cPostData.size() );
    }
    m_cRequestBuffer += "\r\n";

    memset( &sAddress, 0, sizeof( sAddress ) );

    sAddress.sin_family = AF_INET;
    if ( m_cURL.port() == 0 ) {
	sAddress.sin_port  = htons(80);
    } else {
	sAddress.sin_port  = htons(m_cURL.port());
    }
    memcpy( &sAddress.sin_addr, psHost->h_addr_list[0], psHost->h_length );

	
    if ( connect( m_nSocket, (struct sockaddr*) &sAddress, sizeof(sAddress) ) < 0 ) {
	Error( KIO::ERR_COULD_NOT_CONNECT );
	goto done;
    }
	



    while( nBytesSendt < m_cRequestBuffer.size() ) {
	int nLen = write( m_nSocket, m_cRequestBuffer.begin() + nBytesSendt, m_cRequestBuffer.size() - nBytesSendt );
	if ( nLen < 0 ) {
	    Error( KIO::ERR_CONNECTION_BROKEN );
	    goto done;
	}
	nBytesSendt += nLen;
    }
    m_cRequestBuffer.resize(0);

    nBytesSendt = 0;
    while( nBytesSendt < m_cPostData.size() ) {
	int nLen = write( m_nSocket, m_cPostData.begin() + nBytesSendt, m_cPostData.size() - nBytesSendt );
	if ( nLen < 0 ) {
	    Error( KIO::ERR_CONNECTION_BROKEN );
	    goto done;
	}
	nBytesSendt += nLen;
    }
    m_cPostData.resize(0);
    
    while( bReadingHeaders ) {
	int nLen = read( m_nSocket, anBuffer, 65536 );
	if ( nLen <= 0 ) {
	    if ( nLen < 0 ) {
		Error( KIO::ERR_CONNECTION_BROKEN );
	    }
	    goto done;
	}
	g_cLoaderLock.Lock();
	for ( int i = 0 ; i < nLen ; ++i ) {
	    if ( anBuffer[i] == '\r' ) {
		continue;
	    }
	    if ( anBuffer[i] == '\n' ) {
		if ( cMetaDataBuffer.size() > 0 ) {
		    AddMetaData( cMetaDataBuffer );
		    cMetaDataBuffer.resize( 0 );
		} else {
		    AddData( anBuffer + i + 1, nLen - i - 1, false );
		    bReadingHeaders = false;
		    break;
		}
	    } else {
		cMetaDataBuffer += anBuffer[i];
	    }
	}
	g_cLoaderLock.Unlock();
    }
    for (;;) {
	int nLen = read( m_nSocket, anBuffer, 65536 );
	if ( nLen <= 0 ) {
	    if ( nLen < 0 ) {
		Error( KIO::ERR_CONNECTION_BROKEN );
	    }
	    goto done;
	}
	g_cLoaderLock.Lock();
	AddData( anBuffer, nLen, false );
	g_cLoaderLock.Unlock();
    }
done:
    g_cLoaderLock.Lock();
    close( m_nSocket );
    m_nSocket = -1;
    g_cLoaderLock.Unlock();
    m_bDone = true;
    return( true );
}



bool HTTPLoader::Run()
{
    if ( m_bCanceled ) {
	return( false );
    }
    if ( m_nLoadError != 0 ) {
	HandleError( m_nLoadError );
	return( false );
    }
    g_cLoaderLock.Lock();

    while ( g_cFinishedRequests.empty() == false ) {
	(*g_cFinishedRequests.begin())->Release();
	g_cFinishedRequests.erase( g_cFinishedRequests.begin() );
    }
    
    for ( uint i = 0 ; i < m_cMetaDataList.size() ; ++i ) {
	size_t nIndex = m_cMetaDataList[i].find( ':' );

	if ( nIndex != std::string::npos ) {
	    os::String cKey( m_cMetaDataList[i], 0, nIndex );
	    os::String cValue( m_cMetaDataList[i], nIndex + 1, -1 );
	    cKey.Strip();
	    cValue.Strip();
	

	      // Check for cookies
	    if ( cKey.CompareNoCase( "Set-Cookie" ) == 0 || cKey.CompareNoCase( "Set-Cookie2" ) == 0 ) {
		m_cRecvCookieStr += m_cMetaDataList[i].c_str();
		m_cRecvCookieStr += '\n';
	    } else if ( cKey.CompareNoCase( "Location" ) == 0 ) {
		m_cLocationStr = cValue;
	    } else if ( cKey.CompareNoCase( "Content-type" ) == 0 ) {
		m_cMimeType = cValue;
	    } else {
		MetadataReceived( cKey, cValue );
	    }
	}
    }
    m_cMetaDataList.clear();

    if ( m_cLocationStr.empty() ) {
	if ( m_cInBuffer.size() > 0 ) {
	    if ( m_cMimeType.empty() == false ) {
		MimetypeReceived( m_cMimeType );
		m_cMimeType.resize( 0 );
	    }
	    DataReceived( m_cInBuffer.begin(), m_cInBuffer.size(), false );
	}
    }
    m_cInBuffer.resize( 0 );
    bool bDone = m_bDone;
    g_cLoaderLock.Unlock();

    if ( bDone ) {
	if (!m_cRecvCookieStr.isEmpty()) {
	      // Give cookies to the cookiejar.
	    KHttpCookie *cookie = g_cCookieJar.makeCookies( m_cURL.url(), m_cRecvCookieStr, 0 );

	    while (cookie) {
		KHttpCookiePtr next_cookie = cookie->next();
		g_cCookieJar.addCookie(cookie);
		cookie = next_cookie;
	    }
	    m_cRecvCookieStr = "";
	}
	
	if ( m_cLocationStr.empty() == false ) {
	    KURL orig(m_baseURL.string());
	    KURL u( orig, QString::fromUtf8( m_cLocationStr.c_str() ) );
	    m_cLocationStr = u.url().utf8().data();
	    Redirect( m_cLocationStr );
	    m_cURL = m_cLocationStr.c_str();
	    m_cLocationStr.resize( 0 );
	    m_bDone = false;
	    add_http_request( this );
	    return( true );
	}
	Done();
	return( false );
    } else {
	return( true );
    }
}


void HTTPLoader::Stop()
{
    m_bIsRunning = false;
    m_bCanceled  = true;

    Error( KIO::ERR_USER_CANCELED );
    
    std::list<Request*>::iterator i;

    g_cLoaderLock.Lock();
    int nSocket = m_nSocket;
    m_nSocket = -1;
    if ( nSocket != -1 ) {
	close( nSocket );
    }
    i = std::find( g_cPendingRequests.begin(), g_cPendingRequests.end(), this );
    if ( i != g_cPendingRequests.end() ) {
	g_cPendingRequests.erase( i );
    }
    g_cLoaderLock.Unlock();
}






TransfereJobFile::TransfereJobFile( const DOM::DOMString& cBaseURL, const char* pzPath ) : Request( cBaseURL )
{
    m_bStarted = false;
    m_bDone    = false;
    m_pcFile = NULL;
    try {
	m_pcFile = new os::File( pzPath );
    } catch( ... ) {
    }
}

bool TransfereJobFile::Run()
{
    const int nBlockSize = 32768;
//    os::Message cMsg;

//    cMsg.AddPointer( "job", this );
    if ( m_bCanceled ) {
	return( false );
    }
    if ( m_nLoadError != 0 ) {
	HandleError( m_nLoadError );
	return( false );
    }
    
    if ( m_bStarted == false ) {
	return( true );
    }
    if ( m_pcFile == NULL ) {
	emit result( this );
	return( false );
    }

    
    char pBuffer[nBlockSize];
    int nBytesRead = m_pcFile->Read( pBuffer, nBlockSize );
    if ( nBytesRead == 0 ) {
	emit result( this );
	m_bDone = true;
	return( false );
    }
    
    QByteArray cBuffer( nBytesRead );

    memcpy( cBuffer.data(), pBuffer, nBytesRead );

//    cMsg.AddPointer( "data", &cBuffer );
    emit data( this, cBuffer );

    if ( nBytesRead != nBlockSize ) {
	emit result( this );
	m_bDone = true;
    }
    return( nBytesRead == nBlockSize );
}

void TransfereJobFile::kill()
{
    m_bCanceled = true;
    emit result( this );
}









TransfereJobHTTP::TransfereJobHTTP( const KURL& cURL, const DOM::DOMString& cBaseURL ) :
	HTTPLoader( cURL, cBaseURL )
{
    QString cCookies = g_cCookieJar.findCookies( cURL.url(), false );
    if ( cCookies.isEmpty() == false ) {
	m_cClientRequestBuffer += cCookies.utf8().data();
	m_cClientRequestBuffer += "\r\n";
    }
    m_nLastUpdateTime = 0;
    m_bIsRunning = false;
}

void TransfereJobHTTP::DataReceived( const char* pData, int nLen, bool /*bFinal*/ )
{
    m_cReceiveBuffer.resize( m_cReceiveBuffer.size() + nLen );
    memcpy( m_cReceiveBuffer.data() + m_cReceiveBuffer.size() - nLen, pData, nLen );
    bigtime_t nCurTime = get_system_time();
    if ( m_cReceiveBuffer.size() > 1024 && nCurTime > m_nLastUpdateTime + 500000LL ) {
	m_nLastUpdateTime = nCurTime;
	for ( int i = m_cReceiveBuffer.size() - 1 ; i > 0 ; --i ) {
	    if (m_cReceiveBuffer[i] == '\n' ) {
		QByteArray cData(i+1);
		memcpy( cData.data(), m_cReceiveBuffer.data(), i + 1 );

		emit data( this, cData );
		memmove( m_cReceiveBuffer.data(), m_cReceiveBuffer.data() + i + 1, m_cReceiveBuffer.size() - i - 1 );
		m_cReceiveBuffer.resize( m_cReceiveBuffer.size() - i - 1 );
		break;
	    }
	}
    }
    
}

void TransfereJobHTTP::MetadataReceived( const std::string& cKey, const std::string& cValue )
{
    m_cReceivedMetaData[os::String(cKey).Lower()] = cValue;
}

void TransfereJobHTTP::Redirect( const std::string& cLocation )
{
//    QString url = _url.string();
//    QString baseUrl = _baseUrl.string();    
    emit redirection( this, KURL( cLocation.c_str() ) );
}

void TransfereJobHTTP::MimetypeReceived( const std::string& cType )
{
    emit mimetype( this, QString::fromUtf8( cType.c_str() ) );
    
}

void TransfereJobHTTP::Done()
{
    if ( m_cReceiveBuffer.size() > 0 ) {
	emit data( this, m_cReceiveBuffer );
    }
    emit result( this );
}

/*void TransfereJobHTTP::Error( int nError )
{
    if ( nError == 0 || error() != KIO::ERR_USER_CANCELED ) {
	setError( nError );
    }
}*/

void TransfereJobHTTP::Start()
{
//    if ( m_bIsRunning == false ) {
	m_bIsRunning = true;
	add_http_request( this );
//    }
}

void TransfereJobHTTP::putOnHold()
{
    m_bIsRunning = false;
}

void TransfereJobHTTP::kill()
{
    Stop();
    emit result( this );
}

void TransfereJobHTTP::addMetaData( const QString &key, const QString &value )
{
    if ( key == "referrer" ) {
	  // HTTP uses "Referer" although the correct
	  // spelling is "referrer"
	m_cClientRequestBuffer += "Referer: "; //os::String().Format( "Referer: %s\r\n", value.utf8().data() );
	m_cClientRequestBuffer += value.utf8().data();
	m_cClientRequestBuffer += "\r\n";
    } else {
	if ( strncasecmp( key.utf8().data(), value.utf8().data(), key.utf8().size() - 1 ) == 0 ) {
	    m_cClientRequestBuffer += value.utf8().data();
	    m_cClientRequestBuffer += "\r\n";
	} else {
	    m_cClientRequestBuffer += os::String().Format( "%s: %s\r\n", key.utf8().data(), value.utf8().data() );
	}
    }
}

void TransfereJobHTTP::addMetaData( const QMap<QString,QString> &values )
{
    for ( QMap<QString,QString>::ConstIterator i = values.begin() ; i != values.end() ; ++i ) {
	addMetaData( i.key(), i.data() );
    }
}

QString TransfereJobHTTP::queryMetaData( const QString& key )
{
    QString cData = m_cReceivedMetaData[key.lower().ascii()].c_str();
    return( cData );
}


FileRequest::FileRequest( CachedObject* pcObject, const DOM::DOMString& cBaseURL, const char* pzPath ) : Request( cBaseURL )
{
    m_pcObject = pcObject;
    pcObject->setRequest( this );
    m_cBuffer.open( IO_ReadWrite );
    m_pcFile = NULL;
    try {
	m_pcFile = new os::File( pzPath );
    } catch( ... ) {
    }
}


bool FileRequest::Run()
{
    if ( m_bCanceled ) {
	return( false );
    }
    if ( m_nLoadError != 0 ) {
	HandleError( m_nLoadError );
	return( false );
    }    
    const int nBlockSize = 32768;
    if ( m_pcFile == NULL ) {
	return( false );
    }
    char pBuffer[nBlockSize];
    int nBytesRead = m_pcFile->Read( pBuffer, nBlockSize );
    if ( nBytesRead == 0 ) {
	return( false );
    }
    m_cBuffer.writeBlock( pBuffer, nBytesRead );
		
    m_pcObject->data( m_cBuffer, nBytesRead != nBlockSize );
    if ( nBlockSize != nBlockSize ) {
	m_pcObject->finish();
    }
    return( nBytesRead == nBlockSize );
}


HTTPRequest::HTTPRequest( CachedObject* pcObject, const DOM::DOMString& cBaseURL, const KURL& cURL ) :
	HTTPLoader( cURL, cBaseURL )
{
    pcObject->setRequest( this );
    m_cBuffer.open( IO_ReadWrite );
    m_pcObject = pcObject;
}
    
void HTTPRequest::DataReceived( const char* pData, int nLen, bool bFinal )
{
    m_cBuffer.writeBlock( pData, nLen );
		
    m_pcObject->data( m_cBuffer, bFinal );
}

void HTTPRequest::MetadataReceived( const std::string& /*cKey*/, const std::string& /*cValue*/ )
{
}

void HTTPRequest::Done()
{
    m_pcObject->data( m_cBuffer, true );
    m_pcObject->finish();
}

void HTTPRequest::Start()
{
    m_bIsRunning = true;
    add_http_request( this );
}


// up to which size is a picture for sure cacheable
#define MAXCACHEABLE 40*1024
// default cache size
#define DEFCACHESIZE 512*1024
// maximum number of files the loader will try to load in parallel
#define MAX_REQUEST_JOBS 4



KIO::SimpleJob* KIO::http_update_cache( const KURL &/*url*/, bool /*no_cache*/, time_t /*expireDate*/ )
{
/*
    KIO_ARGS << (int)2 << url << no_cache << expireDate;
    SimpleJob *job = new SimpleJob( url, CMD_SPECIAL, packedArgs, false );
    return job;*/
    return( NULL );
}


Loader::Loader()
{
}

Loader::~Loader()
{
}


void Loader::load( CachedObject *object, const DOM::DOMString &baseURL, bool /*incremental*/ )
{
    KURL url( object->url().string().ascii() );

    if ( url.protocol() == "file" ) {
//	printf( "Get file '%s'\n", url.path().ascii() );
	FileRequest* pcRequest = new FileRequest( object, baseURL, url.path().ascii() );
	g_cRequests.push_back( pcRequest );
    } else if ( url.protocol() == "http" ) {
//	printf( "Get HTML ELEMENT WITH HTTP '%s' : '%s'\n", url.host().ascii(), url.path().ascii() );

	HTTPRequest* pcRequest = new HTTPRequest( object, baseURL, url /*url.host().ascii(), url.encodedPathAndQuery( 0, true ).ascii()*/ );
//	printf( "START REQUEST\n" );
	pcRequest->Start();
//	printf( "ADD REQUEST\n" );
	g_cRequests.push_back( pcRequest );
//	printf( "REQUEST STARTED\n" );
    } else {
	printf( "Error: Loader::load() unknown protocol '%s'\n", url.protocol().ascii() );
    }
}

int Loader::numRequests( const DOM::DOMString& baseURL ) const
{
    int nCount = 0;
    for ( std::list<Request*>::iterator i = g_cRequests.begin() ; i != g_cRequests.end() ; ++i ) {
	if ( (*i)->m_baseURL == baseURL ) {
	    nCount++;
	}
    }
    return( nCount );
}

//    int numRequests( const DOM::DOMString &baseURL, CachedObject::Type type ) const;

void Loader::cancelRequests( const DOM::DOMString& baseURL )
{
    int nCount = 0;
    for ( std::list<Request*>::iterator i = g_cRequests.begin() ; i != g_cRequests.end() ; ) {
	if ( (*i)->m_pcObject != NULL && (*i)->m_baseURL == baseURL ) {
            Cache::removeCacheEntry( (*i)->m_pcObject );
	    KIO::TransferJob* pcJob = dynamic_cast<KIO::TransferJob*>(*i);
	    if ( pcJob != NULL ) {
		pcJob->kill();
	    } else {
		(*i)->Stop();
		(*i)->Release();
	    }
	    i = g_cRequests.erase( i );
	    nCount++;
	} else {
	    ++i;
	}
    }
}

void Loader::Run()
{
    for ( std::list<Request*>::iterator i = g_cRequests.begin() ; i != g_cRequests.end() ; ) {
	Request* pcRequest = *i;
	if ( pcRequest->Run() ) {
	    ++i;
	} else {
	    i = g_cRequests.erase( i );
	    emit Cache::loader()->requestDone( pcRequest->m_baseURL, pcRequest->m_pcObject );
	    pcRequest->Release();
	}
    }
}



KIO::TransferJob::TransferJob()
{
//    m_nError = 0;
}

KIO::TransferJob::~TransferJob()
{
}
/*
void KIO::TransferJob::setError( int nError )
{
    m_nError = nError;
}

int KIO::TransferJob::error() const
{
    return( m_nError );
}

void KIO::TransferJob::showErrorDialog( QWidget* parent )
{
}
*/
void KIO::TransferJob::Start()
{
}

void KIO::TransferJob::addMetaData( const QString& /*key*/, const QString& /*value*/ )
{
}

void KIO::TransferJob::addMetaData( const QMap<QString,QString>& /*values*/ )
{
}

QString KIO::TransferJob::queryMetaData( const QString& /*key*/ )
{
//    printf( "KIO::TransferJob::queryMetaData() : '%s'\n", key.ascii() );
    return( "" );
}

KIO::TransferJob* KIO::get( const KURL& url, bool /*reload*/, bool /*showProgressInfo*/ )
{
    init_loader();
    if ( url.protocol() == "file" ) {
//	printf( "Get file '%s'\n", url.path().ascii() );
	TransfereJobFile* pcRequest = new TransfereJobFile( url.url(), url.path().ascii() );
	g_cRequests.push_back( pcRequest );
	return( pcRequest );
    } else if ( url.protocol() == "http" ) {
//	printf( "Get HTTP '%s' : '%s'\n", url.host().ascii(), url.encodedPathAndQuery( 0, true ).ascii() );

	TransfereJobHTTP* pcRequest = new TransfereJobHTTP( url, url.url() );
	g_cRequests.push_back( pcRequest );
	return( pcRequest );
    } else {
	printf( "Error: Unknown protocol '%s'\n", url.protocol().ascii() );
	TransfereJobFile* pcRequest = new TransfereJobFile( "", "blank.html" );
	g_cRequests.push_back( pcRequest );
	return( pcRequest );
    }
}

//KIO::TransferJob* KIO::get_http_get( const KURL& /*url*/, const QByteArray& /*data*/, bool /*showProgressInfo*/ )
//{
//    return( NULL );
//}

KIO::TransferJob* KIO::http_post( const KURL& url, const QByteArray& data, bool /*showProgressInfo*/ )
{
    init_loader();
//    printf( "POST HTTP '%s' (%s)\n", url.encodedPathAndQuery( 0, true ).ascii(), data.data() );

    g_cLoaderLock.Lock();
    TransfereJobHTTP* pcRequest = new TransfereJobHTTP( url, url.url() );
    pcRequest->SetPostData( data.data(), data.size() );
    g_cRequests.push_back( pcRequest );
    g_cLoaderLock.Unlock();
    return( pcRequest );
}









QPixmap* Cache::nullPixmap;
QPixmap* Cache::brokenPixmap;
QDict<CachedObject>* Cache::cache;
Cache::LRUList* Cache::lru = NULL;
Loader* Cache::m_loader;
bool Cache::s_autoloadImages = true;

int Cache::maxSize = DEFCACHESIZE;
int Cache::flushCount = 0;


void Cache::init()
{
    if(!cache) {
        cache = new QDict<CachedObject>(401, true);
    }
    if(!lru) {
        lru = new LRUList;
    }
    if(!nullPixmap) {
        nullPixmap = new QPixmap;
    }
    if ( brokenPixmap == NULL ) {
	try {
	    os::Resources cRes( get_image_id() );
	    os::ResStream* pcStream = cRes.GetResourceStream( "broken_image.png" );
	    if ( pcStream == NULL ) {
		throw( os::errno_exception( "" ) );
	    } else {
		os::Bitmap* pcBitmap = load_icon( pcStream );
		delete pcStream;
		brokenPixmap = new QPixmap( pcBitmap );
		brokenPixmap->SetIsTransparent( true );
	    }
	} catch( ... ) {
	    brokenPixmap = new QPixmap();
	}
/*	
	os::Bitmap* pcBitmap = load_icon( "/Applications/ABrowse/bitmaps/broken_image.png" );
	if ( pcBitmap != NULL ) {
	    brokenPixmap = new QPixmap( pcBitmap );
	    brokenPixmap->SetIsTransparent( true );
	} else {
	    brokenPixmap = new QPixmap();
	}*/
    }
    if(!m_loader) {
        m_loader = new Loader();
    }
}

      /**
       * Ask the cache for some url. Will return a cachedObject, and
       * load the requested data in case it's not cahced
       */
CachedImage *Cache::requestImage( const DOMString & url, const DOMString &baseUrl, bool reload, int _expireDate )
{
    KURL kurl = completeURL( url, baseUrl );
    if( kurl.isMalformed() )
    {
#ifdef CACHE_DEBUG
	kdDebug( 6060 ) << "Cache: Malformed url: " << kurl.url() << endl;
#endif
      return 0;
    }

    CachedObject *o = 0;
    if (!reload)
        o = cache->find(kurl.url());
    if(!o)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << kurl.url() << endl;
#endif
        CachedImage *im = new CachedImage(kurl.url(), baseUrl, reload, _expireDate);
        cache->insert( kurl.url(), im );
        lru->append( kurl.url() );
        flush();

        return im;
    }

    o->setExpireDate(_expireDate);

    if(!o->type() == CachedObject::Image)
    {
        return 0;
    }
    lru->touch( kurl.url() );
    return static_cast<CachedImage *>(o);
}

CachedCSSStyleSheet *Cache::requestStyleSheet( const DOMString & url, const DOMString &baseUrl, bool reload, int _expireDate)
{
    // this brings the _url to a standard form...
    KURL kurl = completeURL( url, baseUrl );
    if( kurl.isMalformed() )
    {
      kdDebug( 6060 ) << "Cache: Malformed url: " << kurl.url() << endl;
      return 0;
    }

    CachedObject *o = cache->find(kurl.url());
    if(!o)
    {
        CachedCSSStyleSheet *sheet = new CachedCSSStyleSheet(kurl.url(), baseUrl, reload, _expireDate);
        cache->insert( kurl.url(), sheet );
        lru->append( kurl.url() );
        flush();
        return sheet;
    }

    o->setExpireDate(_expireDate);

    if(!o->type() == CachedObject::CSSStyleSheet)
    {
        return 0;
    }

    lru->touch( kurl.url() );
    return static_cast<CachedCSSStyleSheet *>(o);
}

CachedScript *Cache::requestScript( const DOM::DOMString &url, const DOM::DOMString &baseUrl, bool reload, int _expireDate)
{
    // this brings the _url to a standard form...
    KURL kurl = completeURL( url, baseUrl );
    if( kurl.isMalformed() )
    {
      kdDebug( 6060 ) << "Cache: Malformed url: " << kurl.url() << endl;
      return 0;
    }

    CachedObject *o = cache->find(kurl.url());
    if(!o)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache: new: " << kurl.url() << endl;
#endif
        CachedScript *script = new CachedScript(kurl.url(), baseUrl, reload, _expireDate);
        cache->insert( kurl.url(), script );
        lru->append( kurl.url() );
        flush();
        return script;
    }

    o->setExpireDate(_expireDate);

    if(!o->type() == CachedObject::Script)
    {
#ifdef CACHE_DEBUG
        kdDebug( 6060 ) << "Cache::Internal Error in requestScript url=" << kurl.url() << "!" << endl;
#endif
        return 0;
    }

#ifdef CACHE_DEBUG
    if( o->status() == CachedObject::Pending )
        kdDebug( 6060 ) << "Cache: loading in progress: " << kurl.url() << endl;
    else
        kdDebug( 6060 ) << "Cache: using cached: " << kurl.url() << endl;
#endif

    lru->touch( kurl.url() );
    return static_cast<CachedScript *>(o);
}


void Cache::flush(bool force)
{
    if (force)
       flushCount = 0;
    // Don't flush for every image.
    if (!lru || (lru->count() < (uint) flushCount))
       return;

    init();

#ifdef CACHE_DEBUG
    //statistics();
    kdDebug( 6060 ) << "Cache: flush()" << endl;
#endif

    int cacheSize = 0;

    for ( QStringList::Iterator it = lru->fromLast(); it != lru->end(); )
    {
        QString url = *it;
        --it; // Update iterator, we might delete the current entry later on.
        CachedObject *o = cache->find( url );

        if( !o->canDelete() || o->status() == CachedObject::Persistent ) {
               continue; // image is still used or cached permanently
	       // in this case don't count it for the size of the cache.
        }

        if( o->status() != CachedObject::Uncacheable )
        {
           cacheSize += o->size();

           if( cacheSize < maxSize )
               continue;
        }

        o->setFree( true );

        lru->remove( url );
        cache->remove( url );

        if ( o->canDelete() )
           delete o;
    }
    flushCount = lru->count()+10; // Flush again when the cache has grown.
#ifdef CACHE_DEBUG
    //statistics();
#endif
}

void Cache::clear()
{
    if ( !cache ) return;
#ifdef CACHE_DEBUG
    kdDebug( 6060 ) << "Cache: CLEAR!" << endl;
    statistics();
#endif
    cache->setAutoDelete( true );
    delete cache; cache = 0;
    delete lru;   lru = 0;
    delete nullPixmap; nullPixmap = 0;
    delete brokenPixmap; brokenPixmap = 0;
    delete m_loader;   m_loader = 0;
}

KURL Cache::completeURL(const DOM::DOMString& _url, const DOM::DOMString& _baseUrl)
{
    QString url = _url.string();
    QString baseUrl = _baseUrl.string();
    KURL orig(baseUrl);
    KURL u( orig, url );
    return u;
}


void Cache::removeCacheEntry( CachedObject *object )
{
  QString key = object->url().string();

  // this indicates the deref() method of CachedObject to delete itself when the reference counter
  // drops down to zero
  object->setFree( true );

  cache->remove( key );
  lru->remove( key );

  if ( object->canDelete() )
     delete object;
}

void Cache::autoloadImages( bool enable )
{
    if ( enable == s_autoloadImages ) {
	return;
    }

  s_autoloadImages = enable;
}

#include "loader_atheos.moc"
