/*
 *  The Syllable kernel
 *  Copyright (C) 1999 Kurt Skauen
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

#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <atheos/types.h>
#include <atheos/strace.h>
#include <atheos/syscall.h>
#include <atheos/syscalltable.h>
#include <posix/errno.h>

#include <inc/scheduler.h>

/* HandleSTrace needs to be able to print several different strings, but for them to appear
   as a single contigous string in the kernel log; printk() and derivitives always prefix
   the CPU #, thread name etc. to the output, which ruins the text. This macro doesn't do
   that. */
#define strace_print( fmt, arg... ); {char zBuffer[1024]; sprintf( zBuffer, fmt, ## arg ); debug_write( zBuffer, strlen( zBuffer ) ); }

/*
	The registers are used as per. the IA32 ELF ABI:

	psRegs->eax - Return value

	psRegs->ebx - Arg #1
	psRegs->ecx - Arg #2
	psRegs->edx - Arg #3
	psRegs->esi - Arg #4
	psRegs->edi - Arg #5
*/

/* Errno description table */

static struct Errno_info g_sErrnoTable[] = {
 {EOK, "EOK" },
 {EPERM, "EPERM" },
 {ENOENT, "ENOENT" },
 {ESRCH, "ESRCH" },
 {EINTR, "EINTR" },
 {EIO, "EIO" },
 {ENXIO, "ENXIO" },
 {E2BIG, "E2BIG" },
 {ENOEXEC, "ENOEXEC" },
 {EBADF, "EBADF" },
 {ECHILD, "ECHILD" },
 {EAGAIN, "EAGAIN" },
 {ENOMEM, "ENOMEM" },
 {EACCES, "EACCES" },
 {EFAULT, "EFAULT" },
 {ENOTBLK, "ENOTBLK" },
 {EBUSY, "EBUSY" },
 {EEXIST, "EEXIST" },
 {EXDEV, "EXDEV" },
 {ENODEV, "ENODEV" },
 {ENOTDIR, "ENOTDIR" },
 {EISDIR, "EISDIR" },
 {EINVAL, "EINVAL" },
 {ENFILE, "ENFILE" },
 {EMFILE, "EMFILE" },
 {ENOTTY, "ENOTTY" },
 {ETXTBSY, "ETXTBSY" },
 {EFBIG, "EFBIG" },
 {ENOSPC, "ENOSPC" },
 {ESPIPE, "ESPIPE" },
 {EROFS, "EROFS" },
 {EMLINK, "EMLINK" },
 {EPIPE, "EPIPE" },
 {EDOM, "EDOM" },
 {ERANGE, "ERANGE" },
 {EDEADLK, "EDEADLK" },
 {ENAMETOOLONG, "ENAMETOOLONG" },
 {ENOLCK, "ENOLCK" },
 {ENOSYS, "ENOSYS" },
 {ENOTEMPTY, "ENOTEMPTY" },
 {ELOOP, "ELOOP" },
 {EWOULDBLOCK, "EWOULDBLOCK" },
 {ENOMSG, "ENOMSG" },
 {EIDRM, "EIDRM" },
 {ECHRNG, "ECHRNG" },
 {EL2NSYNC, "EL2NSYNC" },
 {EL3HLT, "EL3HLT" },
 {EL3RST, "EL3RST" },
 {ELNRNG, "ELNRNG" },
 {EUNATCH, "EUNATCH" },
 {ENOCSI, "ENOCSI" },
 {EL2HLT, "EL2HLT" },
 {EBADE, "EBADE" },
 {EBADR, "EBADR" },
 {EXFULL, "EXFULL" },
 {ENOANO, "ENOANO" },
 {EBADRQC, "EBADRQC" },
 {EBADSLT, "EBADSLT" },
 {EDEADLOCK, "EDEADLOCK" },
 {EBFONT, "EBFONT" },
 {ENOSTR, "ENOSTR" },
 {ENODATA, "ENODATA" },
 {ETIME, "ETIME" },
 {ENOSR, "ENOSR" },
 {ENONET, "ENONET" },
 {ENOPKG, "ENOPKG" },
 {EREMOTE, "EREMOTE" },
 {ENOLINK, "ENOLINK" },
 {EADV, "EADV" },
 {ESRMNT, "ESRMNT" },
 {ECOMM, "ECOMM" },
 {EPROTO, "EPROTO" },
 {EMULTIHOP, "EMULTIHOP" },
 {EDOTDOT, "EDOTDOT" },
 {EBADMSG, "EBADMSG" },
 {EOVERFLOW, "EOVERFLOW" },
 {ENOTUNIQ, "ENOTUNIQ" },
 {EBADFD, "EBADFD" },
 {EREMCHG, "EREMCHG" },
 {ELIBACC, "ELIBACC" },
 {ELIBBAD, "ELIBBAD" },
 {ELIBSCN, "ELIBSCN" },
 {ELIBMAX, "ELIBMAX" },
 {ELIBEXEC, "ELIBEXEC" },
 {EILSEQ, "EILSEQ" },
 {ERESTART, "ERESTART" },
 {ESTRPIPE, "ESTRPIPE" },
 {EUSERS, "EUSERS" },
 {ENOTSOCK, "ENOTSOCK" },
 {EDESTADDRREQ, "EDESTADDRREQ" },
 {EMSGSIZE, "EMSGSIZE" },
 {EPROTOTYPE, "EPROTOTYPE" },
 {ENOPROTOOPT, "ENOPROTOOPT" },
 {EPROTONOSUPPORT, "EPROTONOSUPPORT" },
 {ESOCKTNOSUPPORT, "ESOCKTNOSUPPORT" },
 {EOPNOTSUPP, "EOPNOTSUPP" },
 {EPFNOSUPPORT, "EPFNOSUPPORT" },
 {EAFNOSUPPORT, "EAFNOSUPPORT" },
 {EADDRINUSE, "EADDRINUSE" },
 {EADDRNOTAVAIL, "EADDRNOTAVAIL" },
 {ENETDOWN, "ENETDOWN" },
 {ENETUNREACH, "ENETUNREACH" },
 {ENETRESET, "ENETRESET" },
 {ECONNABORTED, "ECONNABORTED" },
 {ECONNRESET, "ECONNRESET" },
 {ENOBUFS, "ENOBUFS" },
 {EISCONN, "EISCONN" },
 {ENOTCONN, "ENOTCONN" },
 {ESHUTDOWN, "ESHUTDOWN" },
 {ETOOMANYREFS, "ETOOMANYREFS" },
 {ETIMEDOUT, "ETIMEDOUT" },
 {ECONNREFUSED, "ECONNREFUSED" },
 {EHOSTDOWN, "EHOSTDOWN" },
 {EHOSTUNREACH, "EHOSTUNREACH" },
 {EALREADY, "EALREADY" },
 {EINPROGRESS, "EINPROGRESS" },
 {ESTALE, "ESTALE" },
 {EUCLEAN, "EUCLEAN" },
 {ENOTNAM, "ENOTNAM" },
 {ENAVAIL, "ENAVAIL" },
 {EISNAM, "EISNAM" },
 {EREMOTEIO, "EREMOTEIO" },
 {EDQUOT, "EDQUOT" },
 {ERESTARTSYS, "ERESTARTSYS" },
 {ERESTARTNOINTR, "ERESTARTNOINTR" },
 {ERESTARTNOHAND, "ERESTARTNOHAND" },
 {ENOIOCTLCMD, "ENOIOCTLCMD" },
 {EBADINDEX, "EBADINDEX" },
 {ENOSYM, "ENOSYM" },
 {EINITFAILED, "EINITFAILED" },
 {ENOADDRSPC, "ENOADDRSPC" },
 {EUNKNOWNFS, "EUNKNOWNFS" },
 {ESYSCFAILED, "ESYSCFAILED" }
};

static void strace_print_args( int nSyscall, SysCallRegs_s *psRegs )
{
	int nArg, nType;
	long nVal;

	nType = g_sSysCallTable[ nSyscall ].nArg1Type;
	nVal = psRegs->ebx;

	/* Print each syscall argument in turn, using the correct format */
	for( nArg = 1; nArg <= g_sSysCallTable[ nSyscall ].nNumArgs; nArg++ )
	{
		switch( nType )
		{
			case SYSC_ARG_T_INT:
			{
				strace_print( "%d", (int)nVal );
				break;
			}

			case SYSC_ARG_T_LONG_INT:
			{
				strace_print( "%ld", nVal );
				break;
			}

			case SYSC_ARG_T_HEX:
			{
				strace_print( "0x%x", (int)nVal );
				break;
			}

			case SYSC_ARG_T_LONG_HEX:
			{
				strace_print( "0x%lx", nVal );
				break;
			}

			case SYSC_ARG_T_STRING:
			{
				strace_print( "\"%s\"", (char*)nVal );
				break;
			}

			case SYSC_ARG_T_POINTER:
			{
				strace_print( "%p", (void*)nVal );
				break;
			}

			case SYSC_ARG_T_BOOL:
			{
				strace_print( "%s", (bool)nVal ? "true" : "false" );
				break;
			}

			case SYSC_ARG_T_VOID:
			{
				strace_print( "(void)" );
				break;
			}

			case SYSC_ARG_T_NONE:
			{
				strace_print( "(none)" );
				break;
			}

			case SYSC_ARG_T_SPECIAL:
			default:
			{
				strace_print( "(? %ld)", nVal );
				break;
			}
		}

		if( nArg == 1 )
		{
			nType = g_sSysCallTable[ nSyscall ].nArg2Type;
			nVal = psRegs->ecx;
		}
		else if( nArg == 2 )
		{
			nType = g_sSysCallTable[ nSyscall ].nArg3Type;
			nVal = psRegs->edx;
		}
		else if( nArg == 3 )
		{
			nType = g_sSysCallTable[ nSyscall ].nArg4Type;
			nVal = psRegs->esi;
		}
		else if( nArg == 4 )
		{
			nType = g_sSysCallTable[ nSyscall ].nArg5Type;
			nVal = psRegs->edi;
		}
		else if( nArg >= 5 )	/* Too many arguments.  There are currently no syscalls
							       with more than 5 arguments, but you never know.  */
		{
			strace_print( ", ... )\n");
			break;
		}

		/* Print a seperator if there are more args to follow */
		if( nArg < g_sSysCallTable[ nSyscall ].nNumArgs )
		{
			strace_print( ", " );
		}
		else
		{
			strace_print( ")\n" );
		}
	}
}

/* Print a human-readable string for the errno value nReturnVal */
static void strace_print_errno( long nReturnVal )
{
	int nErrno;

	if( nReturnVal < 0 )
	{
		strace_print( "-" );
		nReturnVal = -nReturnVal;
	}

	for( nErrno = 0; nErrno < __NUM_ERRNOS; nErrno++ )
	{
		if( g_sErrnoTable[nErrno].nErrno == (int)nReturnVal )
		{
			strace_print( "%s", g_sErrnoTable[nErrno].zName );
			break;
		}
	}
}

/* HandleSTrace() is *always* called at the end of *every* syscall by
   EXIT_SYSCALL in syscall.s  HandleSTrace therefore must always check
   if the current process is being traced before doing anything else,
   and the codepath is optimised for the most common case of STRACE_DISABLED */
void HandleSTrace( int dummy )
{
	Thread_s *psThread = CURRENT_THREAD;
	SysCallRegs_s *psRegs = ( SysCallRegs_s* ) &dummy;
	SyscallExc_s *psExc;
	bool bExcluded = false;
	int nSyscall;

	if( STRACE_DISABLED == psThread->tr_nSysTraceMask )
		return;

	/* Syscall tracing is enabled for this process */
	if( psThread->tr_nSysTraceMask < 0 && psRegs->eax >= 0 )
		return;

	for( nSyscall=0; nSyscall < __NR_TrueSysCallCount; nSyscall++ )
	{
		if( g_sSysCallTable[ nSyscall ].nNumber == psRegs->orig_eax )
		{
			if( g_sSysCallTable[ nSyscall ].nGroup & psThread->tr_nSysTraceMask )
			{
				/* Walk the exclusions list to ensure this syscall is not excluded */
				psExc = psThread->psExc;
				while( psExc != NULL )
				{
					if( psRegs->orig_eax == psExc->nSyscall )
					{
						bExcluded = true;
						break;
					}
					psExc = psExc->psPrev;
				}

				if( bExcluded )
					break;	/* Break from the for() and return */

				/* Print a leader, which also happens to print the CPU #, process and
				   thread name for us */
				printk( "---->> " );

				/* Return types are limited to a handful of possible types */
				switch( g_sSysCallTable[ nSyscall ].nReturnType )
				{
					case SYSC_ARG_T_STATUS_T:
					{
						strace_print_errno( psRegs->eax );
						strace_print( " = %s(", g_sSysCallTable[ nSyscall ].zName );
						break;
					}

					case SYSC_ARG_T_INT:
					{
						strace_print( "%d = %s(", (int)psRegs->eax, g_sSysCallTable[ nSyscall ].zName );
						break;
					}

					case SYSC_ARG_T_LONG_INT:
					{
						strace_print( "%ld = %s(", psRegs->eax, g_sSysCallTable[ nSyscall ].zName );
						break;
					}

					case SYSC_ARG_T_BOOL:
					{
						strace_print( "%s = %s(", (bool)psRegs->eax ? "true" : "false", g_sSysCallTable[ nSyscall ].zName );
						break;
					}
					
					case SYSC_ARG_T_NONE:
					case SYSC_ARG_T_VOID:
					{
						strace_print( "(void) %s(", g_sSysCallTable[ nSyscall ].zName );
						break;
					}

					case SYSC_ARG_T_SPECIAL:
					default:
					{
						strace_print( "? = %s(", g_sSysCallTable[ nSyscall ].zName );
						break;
					}
				}

				/* Print any arguments to the syscall */
				if( g_sSysCallTable[ nSyscall ].nNumArgs > 0 )
				{
					strace_print_args( nSyscall, psRegs );
				}
				else
				{
					strace_print( "void)\n" );
				}
			}

			/* Syscall found, stop looking */
			break;
		}
	}
}

/* Enable or disable syscall tracing for the given thread.
   This new interface superseeds the old sys_set_strace_level()
   nTraceFlags is currently unused. */
int sys_strace( thread_id hThread, int nTraceMask, int nTraceFlags )
{
	int nError, nCpuFlags;
	Thread_s *psThread;

	nCpuFlags = cli();
	sched_lock();

	psThread = get_thread_by_handle( hThread );
	if( NULL != psThread )
	{
		psThread->tr_nSysTraceMask = nTraceMask;
		nError = EOK;
	}
	else
	{
		nError = -ESRCH;
	}

	sched_unlock();
	put_cpu_flags( nCpuFlags );
	return nError;
}

/* Exclude a specific syscall from being traced for the given thread */
int sys_strace_exclude( thread_id hThread, int nSyscall )
{
	int nError, nCpuFlags;
	Thread_s *psThread;
	SyscallExc_s *psExcNew;

	nCpuFlags = cli();
	sched_lock();

	psThread = get_thread_by_handle( hThread );
	if( NULL != psThread )
	{
		psExcNew = kmalloc( sizeof( SyscallExc_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );
		if( NULL == psExcNew )
		{
			nError = -ENOMEM;
		}
		else
		{
			psExcNew->nSyscall = nSyscall;
			
			psExcNew->psNext = NULL;
			psExcNew->psPrev = psThread->psExc;
			
			if( NULL != psThread->psExc )
				psThread->psExc->psNext = psExcNew;
			
			psThread->psExc = psExcNew;
			nError = EOK;
		}
	}
	else
	{
		nError = -ESRCH;
	}

	sched_unlock();
	put_cpu_flags( nCpuFlags );
	return nError;
}

/* "Un-exclude" a previously excluded syscall for the given thread */
int sys_strace_include( thread_id hThread, int nSyscall )
{
	int nError, nCpuFlags;
	Thread_s *psThread;
	SyscallExc_s *psExc;

	nCpuFlags = cli();
	sched_lock();

	psThread = get_thread_by_handle( hThread );
	if( NULL != psThread )
	{
		nError = -ESRCH;
		psExc = psThread->psExc;

		while( psExc != NULL )
		{
			if( nSyscall == psExc->nSyscall )
			{
				if( psExc->psPrev )
					psExc->psPrev->psNext = psExc->psNext;
				if( psExc->psNext )
					psExc->psNext->psPrev = psExc->psPrev;
				else
					psThread->psExc = psExc->psPrev;

				kfree( psExc );
				nError = EOK;
				break;
			}

			psExc = psExc->psPrev;
		}
	}
	else
	{
		nError = -ESRCH;
	}

	sched_unlock();
	put_cpu_flags( nCpuFlags );
	return nError;
}
