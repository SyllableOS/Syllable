#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include <atheos/time.h>

int stime( const time_t* when )
{
    if (when == NULL) {
	__set_errno (EINVAL);
	return -1;
    }
    return set_real_time( ((bigtime_t)*when) * 1000000LL );
}
