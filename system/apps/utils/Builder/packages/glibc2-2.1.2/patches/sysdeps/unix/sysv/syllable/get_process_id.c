#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <atheos/threads.h>
#include <atheos/tld.h>

proc_id __get_process_id( const char* pzName )
{
  if ( pzName == NULL ) {
    proc_id nPid;
    __asm__( "movl %%gs:(%1),%0" : "=r" (nPid) : "r"  (TLD_PRID) );
    return( nPid );
  } else {
    return( INLINE_SYSCALL( get_process_id, 1, pzName ) );
  }
}

weak_alias( __get_process_id, get_process_id )
