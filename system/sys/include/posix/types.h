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

#ifndef __time_t_defined
typedef long int time_t;
# define __time_t_defined
#endif

#ifndef __pid_t_defined
typedef int pid_t;
# define __pid_t_defined
#endif

#ifndef __uid_t_defined
typedef int uid_t;
# define __uid_t_defined
#endif

#ifndef __gid_t_defined
typedef int gid_t;
# define __gid_t_defined
#endif

#ifndef __dev_t_defined
typedef int dev_t;
# define __dev_t_defined
#endif

#ifndef __mode_t_defined
typedef unsigned int mode_t;
# define __mode_t_defined
#endif

#ifndef __ino_t_defined
typedef long long ino_t;
# define __ino_t_defined
#endif

#ifndef __off_t_defined
typedef long long off_t;
# define __off_t_defined
#endif

#ifdef __KERNEL__

#ifndef __nlink_t_defined
typedef int nlink_t;
# define __nlink_t_defined
#endif

#ifndef __ssize_t_defined
typedef int ssize_t;
# define __ssize_t_defined
#endif

#define MAXHOSTNAMELEN	64

#endif /* __KERNEL__ */

#endif /* __F_ATHEOS_POSIX_TYPES_H__ */
