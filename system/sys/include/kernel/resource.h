/* Shared definitions & structures for resource limits.
   Copyright (C) 1994, 1996, 1997, 1998, 1999, 2000, 2004
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.
*/

#ifndef __F_KERNEL_RESOURCE_H__
#define __F_KERNEL_RESOURCE_H__

#include <posix/resource.h>

#define RLIMIT_CPU		0	/* cpu time in milliseconds */
#define RLIMIT_FSIZE	1	/* maximum file size */
#define RLIMIT_DATA		2	/* data size */
#define RLIMIT_STACK	3	/* stack size */
#define RLIMIT_CORE		4	/* core file size */
#define RLIMIT_RSS		5	/* resident set size */
#define RLIMIT_MEMLOCK	6	/* locked-in-memory address space */
#define RLIMIT_NPROC	7	/* number of processes */
#define RLIMIT_NOFILE	8	/* number of open files */
#define RLIMIT_AS		9	/* Address space limit (?) */
#define RLIM_NLIMITS	10	/* number of resource limits */

#endif	/* __F_KERNEL_RESOURCE_H__ */
