#ifndef __kio_jobclasses_h__
#define __kio_jobclasses_h__

#include <kio/jobclasses.h>
#include <time.h>

namespace KIO
{
//#ifdef __ATHEOS__    
//    void http_update_cache( const KURL &url, bool no_cache, time_t expireDate );
//#endif
    TransferJob *get( const KURL &url, bool reload, bool showProgressInfo );

    TransferJob *http_post( const KURL &url, const QByteArray &data, bool showProgressInfo );

    SimpleJob *http_update_cache( const KURL &url, bool no_cache, time_t expireDate );
//#endif    
}

#endif
