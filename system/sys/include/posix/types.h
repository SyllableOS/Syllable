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

#ifndef __F_POSIX_TYPES_H__
#define __F_POSIX_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined __time_t_defined && !defined time_t
typedef long int time_t;
# define time_t time_t
# define __time_t_defined
#endif

#if !defined __pid_t_defined && !defined pid_t
typedef int pid_t;
# define pid_t pid_t
# define __pid_t_defined
#endif

#if !defined __uid_t_defined && !defined uid_t
typedef int uid_t;
# define uid_t uid_t
# define __uid_t_defined
#endif

#if !defined __gid_t_defined && !defined gid_t
typedef int gid_t;
# define gid_t gid_t
# define __gid_t_defined
#endif

#if !defined __dev_t_defined && !defined dev_t
typedef int dev_t;
# define dev_t dev_t
# define __dev_t_defined
#endif

#if !defined __mode_t_defined && !defined mode_t
typedef unsigned int mode_t;
# define mode_t mode_t
# define __mode_t_defined
#endif

#if !defined __ino_t_defined && !defined ino_t
typedef long long ino_t;
# define ino_t ino_t
# define __ino_t_defined
#endif

#if !defined __off_t_defined && !defined off_t
typedef long long off_t;
# define off_t off_t
# define __off_t_defined
#endif

/* XXXKV: Not sure this should be here */
#ifndef __SYSCALL
# define __SYSCALL(r,p)	r p;
#endif

#ifdef __cplusplus
}
#endif

#endif /* __F_POSIX_TYPES_H__ */
