#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <atheos/semaphore.h>

thread_id __get_semaphore_holder( sem_id hSema )
{
    sem_info	sInfo;
    thread_id	nError;

    nError = get_semaphore_info( hSema, get_process_id( 0 ), &sInfo );

    if ( nError >= 0 ) {
	nError = sInfo.si_latest_holder;
    }
    return( nError );
}	

weak_alias ( __get_semaphore_holder, get_semaphore_holder )
