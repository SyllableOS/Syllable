#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <sysdep.h>
#include <sys/syscall.h>

extern int __syscall_lseek __P ((int fd, int off_low, int off_high, int whence, off_t* newpos ));
/*
static int ___lseek( int nFile, int nOffset, int nWhence )
{
  return( INLINE_SYSCALL( Seek, 3, nFile, nOffset, nWhence ) );
}
*/
off_t __lseek( int fd, off_t offset, int whence )
{
    off_t newpos;
    int   error;

    error = INLINE_SYSCALL( lseek, 5, fd, (int)(offset & 0xffffffff), (int)(offset >> 32), whence, &newpos );
    if ( error < 0 ) {
	return( error );
    } else {
	return( newpos );
    }
//    return( (off_t) ___lseek( nFile, (int)(offset & 0xffffffff), (int)(offset >> 32), nWhence, &newpos ) );
}

weak_alias (__lseek, lseek)
