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

#ifndef __ATHEOS_POSIX_STAT_H_
#define __ATHEOS_POSIX_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Fool Emacs autoindent */
#endif

#ifdef __KERNEL__
#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
	
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
  
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define S_IRWXUGO	(S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO	(S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO		(S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO		(S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO		(S_IXUSR|S_IXGRP|S_IXOTH)
#endif

#include <posix/types.h>

#define _STAT_VER_KERNEL 0    
struct stat {
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
    time_t  	 st_atime;
    int		 __unused1;
    time_t	st_mtime;
    int		 __unused2;
    time_t	st_ctime;
    int		 __unused3;
    int		 __unused4;
    int		 __unused5;
};


struct stat64 {
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
    time_t  	 st_atime;
    int		 __unused1;
    time_t	st_mtime;
    int		 __unused2;
    time_t	st_ctime;
    int		 __unused3;
    int		 __unused4;
    int		 __unused5;
};

#ifdef __KERNEL__
int	fstat(int _fildes, struct stat *_buf);
int	stat(const char *_path, struct stat *_buf);
int 	lstat( const char* pzPath, struct stat* psStat );

int	chmod(const char *_path, mode_t _mode);
int	mkdir(const char *_path, mode_t _mode);
int	mkfifo(const char *_path, mode_t _mode);
mode_t	umask(mode_t _cmask);

#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_POSIX_STAT_H_ */
