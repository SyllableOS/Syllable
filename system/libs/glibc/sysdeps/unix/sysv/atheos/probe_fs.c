#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <atheos/filesystem.h>

#include <sysdep.h>
#include <sys/syscall.h>

int __probe_fs( const char* device, fs_info *buf )
{
    return _xprobe_fs( FSINFO_VERSION, device, buf );
}

weak_alias (__probe_fs, probe_fs );
