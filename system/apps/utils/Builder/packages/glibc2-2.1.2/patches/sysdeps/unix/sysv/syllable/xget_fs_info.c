#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <atheos/filesystem.h>

#include <sysdep.h>
#include <sys/syscall.h>

int __xget_fs_info (int vers, int file, fs_info *buf)
{
  return INLINE_SYSCALL (get_fs_info, 3, vers, file, buf);
}

weak_alias (__xget_fs_info, _xget_fs_info );
