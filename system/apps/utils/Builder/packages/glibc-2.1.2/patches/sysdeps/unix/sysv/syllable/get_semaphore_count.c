#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <atheos/semaphore.h>


int __get_semaphore_count( sem_id hSema )
{
    sem_info sInfo;
    int	    nError;

    nError = get_semaphore_info( hSema, get_process_id( 0 ), &sInfo );

    if ( nError >= 0 ) {
	nError = sInfo.si_count;
    }
    return( nError );
}

weak_alias ( __get_semaphore_count, get_semaphore_count )
