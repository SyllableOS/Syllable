#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <atheos/types.h>
#include <atheos/threads.h>
#include <atheos/semaphore.h>

int SortCmp( const void* p1, const void* p2 )
{
  return( strcmp( ((char**)p1)[0], ((char**)p2)[0] ) );
}

int main( int argc, char** argv )
{
    char*	pBuffer;
    int		nDelay = 0;
    char*	pPtr;
    char*	apThreadList[ 8192 ];
    thread_info sInfo;
    int		nCnt = 0;

    int		nError;
    int		i;

    pBuffer = malloc( 1024 * 1024 );
    if ( pBuffer == NULL ) {
	fprintf( stderr, "Error: out of memory\n" );
	return( 1 );
    }

    pPtr = pBuffer;

    for ( nError = get_thread_info( -1, &sInfo ) ; nError >= 0 ; nError = get_next_thread_info( &sInfo ) )
    {
	char*	pzLockSemName = NULL;
	char*	pzState;

	pzLockSemName = "none";
					
	switch( sInfo.ti_state )
	{
	    case TS_STOPPED:
		pzState = "Stopped";
		break;
	    case TS_RUN:
		pzState = "Run";
		break;
	    case TS_READY:
		pzState = "Ready";
		break;
	    case TS_WAIT:
		pzState = "Wait";
		break;
	    case TS_SLEEP:
		pzState = "Sleep";
		break;
	    case TS_ZOMBIE:
		pzState = "Zombie";
		break;
	    default:
		pzState = "Invalid";
		break;
	}
					
	apThreadList[ nCnt++ ] = pPtr;

	if ( sInfo.ti_state == TS_WAIT && sInfo.ti_blocking_sema >= 0 ) {
	    static char zBuffer[256];
	    sem_info sSemInfo;
		
	    if ( get_semaphore_info( sInfo.ti_blocking_sema, sInfo.ti_process_id, &sSemInfo ) >= 0 ) {
//		    sprintf( zBuffer, "%s(%d)", sSemInfo.si_name, sInfo.ti_blocking_sema );
		sprintf( zBuffer, "%s", sSemInfo.si_name );
		pzState = zBuffer;
	    }
	}
	  
	sprintf( pPtr, "%-13.13s %-16.16s (%05d:%05d) %18s %4d, %s\n",
		 sInfo.ti_process_name, sInfo.ti_thread_name,
		 sInfo.ti_process_id, sInfo.ti_thread_id,
		 pzState,
		 sInfo.ti_priority,
		 pzLockSemName );
					
	pPtr = pPtr + strlen( pPtr ) + 1;

	if ( pPtr - pBuffer > 1024 * 1024 ) {
	    fprintf( stderr, "Error : Buffer overflow!\n" );
	    return( -1 );
	}
    }

    qsort( apThreadList, nCnt, sizeof(char*), SortCmp );

    for ( i = 0 ; i < nCnt ; ++i ) {
	write( 1, apThreadList[i], strlen( apThreadList[i] ) );
    }
    free( pBuffer );
}



