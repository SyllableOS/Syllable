#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

loff_t __lseek64( int nFile, loff_t nOffset, int nWhence )
{
  return( (off_t) __lseek( nFile, (off_t) nOffset, nWhence ) );
}

weak_alias (__lseek64, lseek64)
