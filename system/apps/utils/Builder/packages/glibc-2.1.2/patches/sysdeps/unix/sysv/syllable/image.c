#include <errno.h>
#include <assert.h>

#include <atheos/types.h>
#include <sys/types.h>
#include <atheos/image.h>
#include <sys/debug.h>
#include <sys/image.h>

#include <sysdep.h>
#include <sys/syscall.h>


static int g_open_libs[1024];

static int do_init_library( image_info* psInfo, char* anOpenCounts )
{
    int nImageID       = psInfo->ii_image_id;
    int nSubImageCount = psInfo->ii_sub_image_count;
    int nOpenCount     = psInfo->ii_open_count;
    image_init* pInit  = psInfo->ii_init;
    int i;
    
    assert( nImageID >= 0 && nImageID < 1024 );
    anOpenCounts[ nImageID ]++;
  
    for ( i = 0 ; i < nSubImageCount ; ++i ) {
	if ( get_image_info( nImageID, i, psInfo ) == 0 ) {
	    do_init_library( psInfo, anOpenCounts );
	}
    }
    if ( anOpenCounts[ nImageID ] == nOpenCount && NULL != pInit ) {
	pInit( nImageID );
    }
    return( 0 );
}

int __init_library( int nLib )
{
    image_info sInfo;
    char	      anOpenCounts[1024];

    memset( anOpenCounts, 0, 1024 );
    if ( get_image_info( nLib, -1, &sInfo ) == 0 )
    {
	do_init_library( &sInfo, anOpenCounts );
	return( 0 );
    } else {
	dbprintf( "Error: init_library() Failed to get info about image %d\n", nLib );
	return( -1 );
    }
}

static int do_deinit_library( image_info* psInfo, char* anOpenCounts )
{
    int nImageID 	     = psInfo->ii_image_id;
    int nSubImageCount = psInfo->ii_sub_image_count;
    int i;

    assert( nImageID >= 0 && nImageID < 1024 );
    anOpenCounts[ nImageID ]++;
  
    if ( psInfo->ii_open_count == anOpenCounts[ nImageID ] && NULL != psInfo->ii_fini ) {
	psInfo->ii_fini( psInfo->ii_image_id );
    }
  
    for ( i = 0 ; i < nSubImageCount ; ++i ) {
	if ( get_image_info( nImageID, i, psInfo ) == 0 ) {
	    do_deinit_library( psInfo, anOpenCounts );
	}
    }
    return( 0 );
}

int __deinit_library( int nLib )
{
    image_info sInfo;
    char	      anOpenCounts[1024];

    memset( anOpenCounts, 0, 1024 );

    if ( get_image_info( nLib, -1, &sInfo ) == 0 )
    {
	do_deinit_library( &sInfo, anOpenCounts );
	return( 0 );
    } else {
	dbprintf( "Error: deinit_library() Failed to get info about image %d\n", nLib );
	return( -1 );
    }
}

int __load_library( const char* pzPath, uint32 nFlags )
{
    int	nLibrary;
    nLibrary = INLINE_SYSCALL ( load_library, 3, pzPath, nFlags, getenv( "DLL_PATH" ) );

    if ( nLibrary >= 0 ) {
	if ( nLibrary < 1024 ) {
	    atomic_add( &g_open_libs[nLibrary], 1 );
	}
	__init_library( nLibrary );
    }
    return( nLibrary );
}

int __unload_library( int nLibrary )
{
    int	nError;

    if ( nLibrary >= 0 && nLibrary < 1024 ) {
	atomic_add( &g_open_libs[nLibrary], -1 );
    }
    __deinit_library( nLibrary );
    nError = INLINE_SYSCALL ( unload_library, 1, nLibrary );
    return( nError );
}


void __init_main_image()
{
    atomic_add( &g_open_libs[0], 1 );
    __init_library( 0 );
}

void __deinit_main_image()
{
    int i;
    for ( i = 1 ; i < 1024 ; ++i ) {
	while( g_open_libs[i] > 0 ) {
	    __unload_library( i );
	}
    }
    __deinit_library( 0 );
}

weak_alias( __load_library, load_library )
weak_alias( __unload_library, unload_library )
weak_alias( __init_library, init_library )
weak_alias( __deinit_library, deinit_library )
