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

#ifndef __F_ATHEOS_POSIX_TYPES_H__
#define __F_ATHEOS_POSIX_TYPES_H__

#ifndef time_t
typedef long int time_t;
#define time_t time_t
#endif

#ifndef mode_t
typedef unsigned int	mode_t;
#define mode_t mode_t
#endif

#ifndef uid_t
typedef int uid_t;
#define uid_t uid_t
#endif

#ifndef gid_t
typedef int gid_t;
#define gid_t gid_t
#endif

#ifndef dev_t
typedef int   dev_t;
#define dev_t dev_t
#endif

#ifndef pid_t
typedef int pid_t;
#define pid_t pid_t
#endif

#ifndef ino_t
typedef long long ino_t;
#define ino_t ino_t
#endif

#ifndef off_t
typedef long long int off_t;
#define off_t off_t
#endif

#ifdef __KERNEL__


#ifndef nlink_t
typedef int	nlink_t;
#define nlink_t nlink_t
#endif

/*
#ifndef clock_t
typedef int clock_t;
#define clock_t clock_t;
#endif
*/



#ifndef ssize_t
typedef int ssize_t;
#define ssize_t ssize_t
#endif


#define MAXHOSTNAMELEN	64
#define FD_SETSIZE 1024

typedef struct fd_set {
  unsigned char fds_bits [((FD_SETSIZE) + 7) / 8];
} fd_set;

#define FD_SET(n, p)    ((p)->fds_bits[(n) / 8] |= (1 << ((n) & 7)))
#define FD_CLR(n, p)	((p)->fds_bits[(n) / 8] &= ~(1 << ((n) & 7)))
#define FD_ISSET(n, p)	((p)->fds_bits[(n) / 8] & (1 << ((n) & 7)))
#define FD_ZERO(p)	memset ((void *)(p), 0, sizeof (*(p)))

#endif /* __KERNEL__ */

#endif /* __F_ATHEOS_POSIX_TYPES_H__ */
