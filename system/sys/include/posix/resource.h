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

#ifndef __F_POSIX_RESOURCE_H__
#define __F_POSIX_RESOURCE_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
#include <posix/time.h>

/* Structure which says how much of each resource has been used.  */
struct rusage
{
	/* Total amount of user time used.  */
	struct kernel_timeval ru_utime;
	/* Total amount of system time used.  */
	struct kernel_timeval ru_stime;
	/* Maximum resident set size (in kilobytes).  */
	long int ru_maxrss;
	/* Amount of sharing of text segment memory
	   with other processes (kilobyte-seconds).  */
	long int ru_ixrss;
	/* Amount of data segment memory used (kilobyte-seconds).  */
	long int ru_idrss;
	/* Amount of stack memory used (kilobyte-seconds).  */
	long int ru_isrss;
	/* Number of soft page faults (i.e. those serviced by reclaiming
	   a page from the list of pages awaiting reallocation.  */
	long int ru_minflt;
	/* Number of hard page faults (i.e. those that required I/O).  */
	long int ru_majflt;
	/* Number of times a process was swapped out of physical memory.  */
	long int ru_nswap;
	/* Number of input operations via the file system.  Note: This
	   and `ru_oublock' do not include operations with the cache.  */
	long int ru_inblock;
	/* Number of output operations via the file system.  */
	long int ru_oublock;
	/* Number of IPC messages sent.  */
	long int ru_msgsnd;
	/* Number of IPC messages received.  */
	long int ru_msgrcv;
	/* Number of signals delivered.  */
	long int ru_nsignals;
	/* Number of voluntary context switches, i.e. because the process
	   gave up the process before it had to (usually to wait for some
	   resource to be available).  */
	long int ru_nvcsw;
	/* Number of involuntary context switches, i.e. a higher priority process
	   became runnable or the current process used up its time slice.  */
	long int ru_nivcsw;
};
  
#ifdef __KERNEL__

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

#endif	/* __KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif /* __F_POSIX_RESOURCE_H__ */
