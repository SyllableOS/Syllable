#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <bits/libc-lock.h>
#include <sysdep.h>
#include <atheos/threads.h>
#include <atheos/tld.h>
#include <sys/syscall.h>


int __fork()
{
    int result = INLINE_SYSCALL (Fork, 1, NULL );

    if ( result == 0 ) {
	thread_id nThid = INLINE_SYSCALL( get_thread_id, 1, NULL );
	proc_id   nPrid = INLINE_SYSCALL( get_process_id, 1, NULL );
	__asm__("movl %0,%%gs:(%1)" : : "r" (nThid), "r" (TLD_THID) );
	__asm__("movl %0,%%gs:(%1)" : : "r" (nPrid), "r" (TLD_PRID) );
    }
  
    return( result );
}

weak_alias (__fork, fork)
