/*
 *  The Syllable kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2004 Kristian Van Der Vliet
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

#ifndef __F_SYLLABLE_STRACE_H__
#define __F_SYLLABLE_STRACE_H__

#include <atheos/types.h>
#include <atheos/syscall.h>
#include <posix/errno.h>

/* Syscall groups.  These are bitfield values used in tr_nSysTraceMask */
#define SYSC_GROUP_NONE		0x00	/* Not used, apart from STRACE_DISABLED */
#define SYSC_GROUP_MM		1<<0	/* Memory management */
#define SYSC_GROUP_PROC		1<<1	/* Process management */
#define SYSC_GROUP_DEVICE	1<<2	/* Device management */
#define SYSC_GROUP_NET		1<<3	/* Network/IP */
#define SYSC_GROUP_SIGNAL	1<<4	/* Anything signal related */
#define SYSC_GROUP_IPC		1<<5	/* Interprocess Communications, including semaphores */
#define SYSC_GROUP_IO		1<<6	/* I/O */
#define SYSC_GROUP_DEBUG	1<<7	/* Debugging calls */
#define SYSC_GROUP_MISC		1<<8	/* Anything else */

#define SYSC_GROUP_ALL		(SYSC_GROUP_MM | 	\
							SYSC_GROUP_PROC |	\
							SYSC_GROUP_DEVICE |	\
							SYSC_GROUP_NET |	\
							SYSC_GROUP_SIGNAL |	\
							SYSC_GROUP_IPC |	\
							SYSC_GROUP_IO |		\
							SYSC_GROUP_DEBUG |	\
							SYSC_GROUP_MISC )

#define STRACE_DISABLED		SYSC_GROUP_NONE	/* Syscall Tracing disabled */

/* Codes to identify the argument types for each argument */
enum sysc_arg_type{
	SYSC_ARG_T_INT = 1,		/* %d */
	SYSC_ARG_T_LONG_INT,	/* %ld */
	SYSC_ARG_T_HEX,			/* %x */
	SYSC_ARG_T_LONG_HEX,	/* %lx */
	SYSC_ARG_T_STRING,		/* %s */
	SYSC_ARG_T_POINTER,		/* %p */
	SYSC_ARG_T_BOOL,		/* Syllable kernel bool type, printed as either "true" or "false" */
	SYSC_ARG_T_STATUS_T,	/* Human readable string coresponding to a status_t return value */
	SYSC_ARG_T_VOID,		/* Not printable */
	SYSC_ARG_T_NONE,		/* None, or argument does not exist for this function */
	SYSC_ARG_T_SPECIAL		/* Non-standard type E.g. a signal name or ioctl() literal */
};

/* Syscall information for use by HandleStrace() */
struct SysCall_info
{
	int nNumber;		/* Syscall number */
	char *zName;		/* Printable name */
	int nGroup;			/* Which SYSC_GROUP the call belongs to */
	int nNumArgs;		/* Number of arguments */
	int nReturnType;	/* Return value type */
	int nArg1Type;		/* Type for arg #1 */
	int nArg2Type;		/* Type for arg #2 */
	int nArg3Type;		/* Type for arg #3 */
	int nArg4Type;		/* Type for arg #4 */
	int nArg5Type;		/* Type for arg #5 */
};

/* Errno information, which provides human readable names for all errno values */
struct Errno_info
{
	int nErrno;			/* Error number */
	char *zName;		/* Human readable name */
};

/* Userspace STrace API */
status_t strace( thread_id hThread, int nTraceMask, int nTraceFlags );
status_t strace_exclude( thread_id hThread, int nSyscall );
status_t strace_include( thread_id hThread, int nSyscall );

#ifdef __KERNEL__

typedef struct SyscallExc
{
	int nSyscall;
	struct SyscallExc *psPrev, *psNext;
} SyscallExc_s;

#endif

#endif

