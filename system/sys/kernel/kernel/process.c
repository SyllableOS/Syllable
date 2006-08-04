
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
	
#include <posix/errno.h>
	
#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <atheos/semaphore.h>
	
#include <macros.h>
	
#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/mman.h"
#include "inc/array.h"
	MultiArray_s g_sProcessTable;


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/ 
void remove_process( Process_s *psProc ) 
{
	uint32 nFlg = cli();

	sched_lock();
	MArray_Remove( &g_sProcessTable, psProc->tc_hProcID );
	sched_unlock();
	put_cpu_flags( nFlg );
} 

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/ 
Process_s *get_proc_by_handle( proc_id hProcID ) 
{
	Process_s *psProc;
	uint32 nFlg = cli();

	sched_lock();
	psProc = MArray_GetObj( &g_sProcessTable, hProcID );
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( psProc );
}



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/ 
proc_id sys_get_next_proc( const proc_id hPrev ) 
{
	proc_id hNext;
	int nFlg = cli();

	sched_lock();
	hNext = MArray_GetNextIndex( &g_sProcessTable, hPrev );
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( hNext );
}



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/ 
proc_id sys_get_prev_proc( const proc_id hProc ) 
{
	proc_id hPrev;
	int nFlg = cli();

	sched_lock();
	hPrev = MArray_GetPrevIndex( &g_sProcessTable, hProc );
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( hPrev );
}



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/ 
static Process_s *get_process_by_name( const char *pzName ) 
{
	Process_s *psProc = NULL;
	Thread_s *psThread;
	
	psThread = get_thread_by_name( pzName );
	if ( NULL != psThread )
	{
		psProc = psThread->tr_psProcess;
	}
	return ( psProc );
}



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/ 
proc_id sys_get_process_id( const char *pzName ) 
{
	Process_s *psProc;
	proc_id hProcID = -1;
	int nFlg = cli();

	sched_lock();
	psProc = get_process_by_name( pzName );
	if ( NULL != psProc )
	{
		hProcID = psProc->tc_hProcID;
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( hProcID );
}



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/ 
status_t sys_rename_process( int nProcessID, const char *pzName ) 
{
	Process_s *psProc;

	if ( nProcessID != -1 )
	{
		psProc = get_proc_by_handle( nProcessID );
	}
	else
	{
		psProc = CURRENT_PROC;
	}
	if ( NULL != psProc )
	{
		strncpy_from_user( psProc->tc_zName, pzName, OS_NAME_LENGTH );
		return ( 0 );
	}
	else
	{
		return ( -ESRCH );
	}
}


//****************************************************************************/
/** Rename a process.
 * \ingroup DriverAPI
 * \par Description:
 *      All processes have a name to aid debugging and to some degree
 *	make it possible to search for a process by name. The name
 *	is normally inherited from the parent during fork and later
 *	set to the executable name if the process exec's but it can
 *	also be set explicitly with rename_process().
 *
 * \par Note:
 *	The name will be replaced with the name of the executable if
 *	the process ever call execve().
 *
 * \param nProcessID
 *	Identity of the process to rename. You can also pass in -1
 *	to rename the calling process.
 *
 * \param pzName
 *	The new process name. Maximum OS_NAME_LENGTH characters long
 *	including the 0 termination.
 *	On success 0 is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error -1 is returned "errno" will receive the error code.
 *	\endif
 *	
 * \par Error codes:
 *	- \b ESRCH \p nProcessID is not -1 and there is not process with that ID.
 * \if document_syscalls
 *	- \b EFAULT \p pzName points to an address not accessible by this process.
 * \endif
 *
 * \sa rename_thread(), get_process_id()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/ 
status_t rename_process( int nProcessID, const char *pzName ) 
{
	Process_s *psProc;

	if ( nProcessID != -1 )
	{
		psProc = get_proc_by_handle( nProcessID );
	}
	else
	{
		psProc = CURRENT_PROC;
	}
	if ( NULL != psProc )
	{
		strncpy( psProc->tc_zName, pzName, OS_NAME_LENGTH );
		psProc->tc_zName[OS_NAME_LENGTH - 1] = '\0';
		return ( 0 );
	}
	else
	{
		return ( -ESRCH );
	}
}


//****************************************************************************/
/** Allocates a TLD slot.
 * \ingroup DriverAPI
 * \ingroup Syscalls
 * \par Description:
 *	Allocates a TLD. A TLD (thread local storage) is an integer that
 *	is allocated on the process level but accessed by threads.
 *	A thread can assign an arbitrary value to a TLD and then
 *	retrieve the value later. The TLD handle can be distributed
 *	among the threads but each thread will have its own private
 *	storage for the TLD. This means that both thread A and thread
 *	B can store different values to the same TLD without overwriting
 *	each other's TLD. This is for example used to give each thread
 *	a private "errno" global variable.
 *
 * \return
 *	On success a positive handle is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error <code>-1</code> is returned; "errno" will receive the error code.
 *	\endif
 * \par Error codes:
 *	\b <code>ENOMEM</code> All TLD slots are in use.
 * \sa free_tld(), set_tld(), get_tld()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/ 
int alloc_tld( void ) 
{
	Process_s *psProc = CURRENT_PROC;
	int i;

	LOCK( psProc->pr_hSem );
	for ( i = TLD_USER / ( sizeof( int ) * 32 ); i < TLD_SIZE / ( sizeof( int ) * 32 ); ++i )
	{
		if ( psProc->pr_anTLDBitmap[i] != ~0 )
		{
			uint32 nMask;
			int j;

			for ( j = 0, nMask = 1; j < 32; ++j, nMask <<= 1 )
			{
				if ( ( psProc->pr_anTLDBitmap[i] & nMask ) == 0 )
				{
					psProc->pr_anTLDBitmap[i] |= nMask;
					UNLOCK( psProc->pr_hSem );
					return ( ( i * 32 + j ) * 4 );
				}
			}
		}
	}
	UNLOCK( psProc->pr_hSem );
	return ( -1 );
}
int sys_alloc_tld( void ) 
{
	return ( alloc_tld() );
}

//****************************************************************************/
/** Gives the address on the stack of the TLD.
 * \ingroup DriverAPI
 * \ingroup Syscalls
 * \param nHandle
 *	The TLD to calculate the address for.  Deprecated & marked for removal.
 * \return
 *	On success the address of the TLD is returned.
 *	On failure, NULL is returned.
 *	
 * \par Error codes:
 *	- \b NULL Invalid TLD handle.
 * \sa alloc_tld(), set_tld(), get_tld()
 * \author	Kristian Van Der Vliet (vanders@liqwyd.com)
 *****************************************************************************/ 
/* XXXKV: Delete this function */
void * get_tld_addr( int nHandle )
{
	Process_s *psProc = CURRENT_PROC;
	Thread_s *psThread = CURRENT_THREAD;
	void *pAddr;

	if ( ( nHandle & 3 ) || nHandle < TLD_USER || nHandle >= TLD_SIZE )
		return NULL;

	LOCK( psProc->pr_hSem );
	pAddr = ( void* )( psThread->tr_pThreadData + nHandle );
	UNLOCK( psProc->pr_hSem );

	return pAddr;
}

/* XXXKV: Delete this function & mark the syscall sys_nosys */
void * sys_get_tld_addr( int nHandle )
{
	return get_tld_addr( nHandle );
}

//****************************************************************************/
/** Releases a TLD slot previously allocated with alloc_tld().
 * \ingroup DriverAPI
 * \ingroup Syscalls
 * \param nHandle
 *	The TLD slot to release.
 * \return
 *	On success <code>0</code> is returned.
 *	\if document_driver_api
 *	On error a negative error code is returned.
 *	\endif
 *	\if document_syscalls
 *	On error <code>-1</code> is returned "errno" will receive the error code.
 *	\endif
 *	
 * \par Error codes:
 *	- \b <code>EINVAL</code> Invalid TLD handle.
 * \sa alloc_tld(), set_tld(), get_tld()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/ 
int free_tld( int nHandle ) 
{
	Process_s *psProc = CURRENT_PROC;
	int nError = 0;

	if ( ( nHandle & 3 ) || nHandle < TLD_USER || nHandle >= TLD_SIZE )
	{
		return ( -EINVAL );
	}
	LOCK( psProc->pr_hSem );
	if ( psProc->pr_anTLDBitmap[( nHandle / 4 ) / 32] & ( 1 << ( ( nHandle / 4 ) % 32 ) ) )
	{
		psProc->pr_anTLDBitmap[( nHandle / 4 ) / 32] &= ~( 1 << ( ( nHandle / 4 ) % 32 ) );
	}
	else
	{
		nError = -EINVAL;
	}
	UNLOCK( psProc->pr_hSem );
	return ( nError );
}
int sys_free_tld( int nHandle ) 
{
	return ( free_tld( nHandle ) );
}


/* NOTE: set_tld() is implemented as a inline function in <atheos/tld.h> */ 

//****************************************************************************/
/** \fn void set_tld( int nHandle, int nValue )
 * \ingroup Syscalls
 * \brief Assign a value to a TLD slot.
 * \par Description:
 *	Assign a value to the calling thread's storage associated with the
 *	given TLD handle. This will not affect the value of the same TLD
 *	in another thread and the value can only be read back by the
 *	thread that actually assigned it.
 *
 * \param nHandle
 *	Handle to a TLD slot previously allocated by alloc_tld()
 * \param nValue
 *	The value to assign to the TLD.
 *
 * \sa get_tld(), alloc_tld(), free_tld()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/ 
	
#ifndef __KERNEL__		// Workaround for a bug in doxygen. The function will bever be seen by the compiler
void set_tld( int nHandle, int nValue )
{
} 
#endif	/*  */
	

/* NOTE: get_tld() is implemented as a inline function in <atheos/tld.h> */ 

//****************************************************************************/
/** \fn int get_tld( int nHandle )
 * \ingroup Syscalls
 * \brief Retrieve the value stored in a TLD.
 * \par Description:
 *	Retrieve the value stored earlier by *this* thread.
 * \param nHandle
 *	The TLD to examine.
 * \return
 *	The value of the given TLD
 * \sa set_tld(), alloc_tld(), free_tld()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/ 
#ifndef __KERNEL__		// Workaround for a bug in doxygen. The function will bever be seen by the compiler
int get_tld( int nHandle )
{
} 
#endif	/*  */
	

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/ 
void init_processes( void ) 
{
	MArray_Init( &g_sProcessTable );
} 
