/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#ifndef	__F_KERNEL_TYPES_H__
#define	__F_KERNEL_TYPES_H__

#include <kernel/stdint.h>		/* Kernel C99 integer types */
#include <syllable/inttypes.h>

#include <kernel/tunables.h>

#include <stddef.h>

#ifndef __cplusplus
# include <stdbool.h>
#endif

/* Additional kernel-only types */
typedef void (*sighandler_t)(int);

#ifndef nlink_t
typedef int nlink_t;
#define nlink_t nlink_t
#endif

#ifndef ssize_t
typedef int ssize_t;
#define ssize_t ssize_t
#endif

#define MAXHOSTNAMELEN	64

/* Declare both p() and kernel sys_p() prototypes. */
#define __SYSCALL(r,p)	r p;	r sys_ ## p;

#include <posix/types.h>

#endif	/* __F_KERNEL_TYPES_H__ */
