#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <atheos/threads.h>
#include <atheos/tld.h>


pid_t __getpid()
{
    thread_id nThid;
    __asm__("movl %%gs:(%1),%0" : "=r" (nThid) : "r"  (TLD_THID) );
    return( nThid );
}

weak_alias( __getpid, getpid )
