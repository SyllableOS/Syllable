/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __F_POSIX_STAT_H_
#define __F_POSIX_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define __S_IFMT	00170000
#define __S_IFSOCK	0140000
#define __S_IFLNK	0120000

#define __S_IFREG	0100000
#define __S_IFBLK	0060000
#define __S_IFDIR	0040000
#define __S_IFCHR	0020000
#define __S_IFIFO	0010000

#define __S_ISUID	0004000
#define __S_ISGID	0002000
#define __S_ISVTX	0001000

#define __S_ISLNK(m)	(((m) & __S_IFMT) == __S_IFLNK)
#define __S_ISREG(m)	(((m) & __S_IFMT) == __S_IFREG)
#define __S_ISDIR(m)	(((m) & __S_IFMT) == __S_IFDIR)
#define __S_ISCHR(m)	(((m) & __S_IFMT) == __S_IFCHR)
#define __S_ISBLK(m)	(((m) & __S_IFMT) == __S_IFBLK)
#define __S_ISFIFO(m)	(((m) & __S_IFMT) == __S_IFIFO)
#define __S_ISSOCK(m)	(((m) & __S_IFMT) == __S_IFSOCK)

#define __S_IRWXU 00700
#define __S_IRUSR 00400
#define __S_IWUSR 00200
#define __S_IXUSR 00100

#define __S_IRWXG 00070
#define __S_IRGRP 00040
#define __S_IWGRP 00020
#define __S_IXGRP 00010

#define __S_IRWXO 00007
#define __S_IROTH 00004
#define __S_IWOTH 00002
#define __S_IXOTH 00001

#define __S_IRWXUGO	(__S_IRWXU|__S_IRWXG|__S_IRWXO)
#define __S_IALLUGO	(__S_ISUID|__S_ISGID|__S_ISVTX|__S_IRWXUGO)
#define __S_IRUGO	(__S_IRUSR|__S_IRGRP|__S_IROTH)
#define __S_IWUGO	(__S_IWUSR|__S_IWGRP|__S_IWOTH)
#define __S_IXUGO	(__S_IXUSR|__S_IXGRP|__S_IXOTH)

#define __S_IREAD	__S_IRUSR
#define __S_IWRITE	__S_IWUSR
#define __S_IEXEC	__S_IXUSR

#define __S_TYPEISMQ(buf)	0
#define __S_TYPEISSEM(buf)	0
#define __S_TYPEISSHM(buf)	0

#include <posix/types.h>
#include <posix/time.h>

#define _STAT_VER_KERNEL 0
struct stat
{
	int	 	 st_dev;
	long long	 st_ino;
	mode_t	 st_mode;
	int		 st_nlink;
	uid_t	 st_uid;
	gid_t	 st_gid;
	dev_t	 st_rdev;
	long long	 st_size;
	int		 st_blksize;
	long long	 st_blocks;

	/* Nanosecond resolution timestamps are stored in a format equivalent to 'struct timespec'. */
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
# define st_atime st_atim.tv_sec	/* Backward compatibility.  */
# define st_mtime st_mtim.tv_sec
# define st_ctime st_ctim.tv_sec

	int		 __unused4;
	int		 __unused5;
};

struct stat64
{
	int	 	 st_dev;
	long long	 st_ino;
	mode_t	 st_mode;
	int		 st_nlink;
	uid_t	 st_uid;
	gid_t	 st_gid;
	dev_t	 st_rdev;
	long long	 st_size;
	int		 st_blksize;
	long long	 st_blocks;

	/* Nanosecond resolution timestamps are stored in a format equivalent to 'struct timespec'. */
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
# define st_atime st_atim.tv_sec	/* Backward compatibility.  */
# define st_mtime st_mtim.tv_sec
# define st_ctime st_ctim.tv_sec

	int		 __unused4;
	int		 __unused5;
};

#ifdef __cplusplus
}
#endif

#endif	/* __F_POSIX_STAT_H_ */
