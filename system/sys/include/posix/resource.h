/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
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
  
#if 0
#define RUSAGE_SELF     0               /* calling process */
#define RUSAGE_CHILDREN -1              /* terminated child processes */
struct rusage {
  struct timeval ru_utime;	/* user time used */
  struct timeval ru_stime;	/* system time used */
  long ru_maxrss;		/* integral max resident set size */
  long ru_ixrss;		/* integral shared text memory size */
  long ru_idrss;		/* integral unshared data size */
  long ru_isrss;		/* integral unshared stack size */
  long ru_minflt;		/* page reclaims */
  long ru_majflt;		/* page faults */
  long ru_nswap;		/* swaps */
  long ru_inblock;		/* block input operations */
  long ru_oublock;		/* block output operations */
  long ru_msgsnd;		/* messages sent */
  long ru_msgrcv;		/* messages received */
  long ru_nsignals;		/* signals received */
  long ru_nvcsw;		/* voluntary context switches */
  long ru_nivcsw;		/* involuntary context switches */
};
#endif
#define RLIMIT_CPU	0	/* cpu time in milliseconds */
#define RLIMIT_FSIZE	1	/* maximum file size */
#define RLIMIT_DATA	2	/* data size */
#define RLIMIT_STACK	3	/* stack size */
#define RLIMIT_CORE	4	/* core file size */
#define RLIMIT_RSS	5	/* resident set size */
#define RLIMIT_MEMLOCK	6	/* locked-in-memory address space */
#define RLIMIT_NPROC	7	/* number of processes */
#define RLIMIT_NOFILE	8	/* number of open files */
#define RLIMIT_AS	9	/* Address space limit (?) */
#define RLIM_NLIMITS	10	/* number of resource limits */


#if 0
#define RLIM_INFINITY	((long) ((1UL << 31) - 1UL))

struct rlimit {
  long rlim_cur;		/* current (soft) limit */
  long rlim_max;		/* maximum value for rlim_cur */
};
#endif
/*
int getrusage(int _who, struct rusage *_rusage);
int getrlimit(int _rltype, struct rlimit *_rlimit);
int setrlimit(int _rltype, const struct rlimit *_rlimit);
*/
#ifdef __cplusplus
}
#endif

#endif /* __F_POSIX_RESOURCE_H__ */
