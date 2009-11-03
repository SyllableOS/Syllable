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

#ifndef	__F_KERNEL_THREADS_H__
#define	__F_KERNEL_THREADS_H__

#include <kernel/types.h>
#include <syllable/threads.h>

/* Flags passed to LockSemaphore() */
enum
{
    SEM_NOSIG = 0x0001,	/* Dont break on signals */
};

bool is_root( bool bFromFS );
bool is_in_group( gid_t grp );

proc_id sys_get_next_proc( const proc_id hProc );
proc_id sys_get_prev_proc( const proc_id hProc );

thread_id	spawn_kernel_thread( const char* const pzName, void* const pfEntry, const int nPri, int nStackSize, void* const pData );

/* Threads */
proc_id sys_get_process_id( const char* pzName );
thread_id sys_get_thread_id( const char* const pzName );
status_t sys_get_thread_info( const thread_id hThread, thread_info* const psInfo );
status_t sys_get_next_thread_info( thread_info* const psInfo );
proc_id sys_get_thread_proc( const thread_id hThread );
status_t sys_rename_thread( thread_id hThread, const char* const pzName );
proc_id sys_get_thread_proc( const thread_id hThread );
/*thread_id sys_get_next_thread( const thread_id hPrev );
thread_id sys_get_prev_thread( const thread_id hPrev );*/
status_t sys_rename_thread( thread_id hThread, const char* pzNewName );
status_t sys_snooze( bigtime_t nTimeout );
int	sys_exit( int nCode );
int	do_exit( int nCode );

#endif	/* __F_KERNEL_THREADS_H__ */
