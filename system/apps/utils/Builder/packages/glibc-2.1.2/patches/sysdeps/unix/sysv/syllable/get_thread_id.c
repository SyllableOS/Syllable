#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <atheos/threads.h>
#include <atheos/tld.h>

thread_id __get_thread_id( const char* pzName )
{
  if ( pzName == NULL ) {
    thread_id nThid;
    __asm__("movl %%gs:(%1),%0" : "=r" (nThid) : "r"  (TLD_THID) );
    return( nThid );
  } else {
    return( INLINE_SYSCALL( get_thread_id, 1, pzName ) );
  }
}

weak_alias( __get_thread_id, get_thread_id )
