#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <atheos/filesystem.h>

#include <sysdep.h>
#include <sys/syscall.h>

int __get_fs_info ( int file, fs_info *buf )
{
    return _xget_fs_info( FSINFO_VERSION, file, buf);
}

weak_alias (__get_fs_info, get_fs_info );
