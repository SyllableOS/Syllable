#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <atheos/filesystem.h>

#include <sysdep.h>
#include <sys/syscall.h>

int __xprobe_fs (int vers, const char* device, fs_info *buf)
{
  return INLINE_SYSCALL (probe_fs, 3, vers, device, buf);
}

weak_alias (__xprobe_fs, _xprobe_fs );
