#include <atheos/kernel.h>

status_t __get_system_info( system_info* psInfo )
{
    return( __xget_system_info( psInfo, SYS_INFO_VERSION ) );
}

weak_alias (__get_system_info, get_system_info);
