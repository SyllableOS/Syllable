#include <errno.h>
#include <stddef.h>

#include <atheos/kernel.h>
#include <sysdep.h>
#include <sys/syscall.h>

status_t __xget_system_info( system_info* psInfo, int nVersion )
{
    return( INLINE_SYSCALL (get_system_info, 2, psInfo, nVersion ) );
}

weak_alias (__xget_system_info, _lxget_system_info);
