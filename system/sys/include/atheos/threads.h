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

#ifndef	__F_ATHEOS_THREADS_H__
#define	__F_ATHEOS_THREADS_H__

#include <atheos/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Task States */

#define	TF_DEADLOCK	0x0001
enum
{
	TS_INVALID,	/* Bad luck */
	TS_RUN,		/* The lucky ones */
	TS_READY,	/* I want one of those transistor lumps NOW */
	TS_WAIT,	/* Stay off */
	TS_SLEEP,	/* Just a minute */
	TS_STOPPED,	/* LET ME OUT */
	TS_ZOMBIE	/* waiting for waitpid */
};
  
typedef	struct
{
	thread_id	ti_thread_id;
	proc_id	ti_process_id;
	char	ti_process_name[ OS_NAME_LENGTH ];
	char	ti_thread_name[ OS_NAME_LENGTH ];	/* Thread name */

	int32  	ti_state;							/* Current task state. */
	uint32	ti_flags;
	sem_id	ti_blocking_sema;					/* The semaphore we wait's for, or -1 */

	/* Timing members */
	int		ti_priority;
	int		ti_dynamic_pri;
	bigtime_t	ti_sys_time;					/*	Micros in user mode */
	bigtime_t	ti_user_time;					/*	Micros in kernal mode */
	bigtime_t	ti_real_time;					/*	Total micros of execution */
	bigtime_t	ti_quantum;						/*	Maximum allowed micros of execution before preemption */

	void*	ti_stack;
	uint32	ti_stack_size;
	void*	ti_kernel_stack;					/* Top of kernel stack */
	uint32	ti_kernel_stack_size;				/* Size (in bytes) of kernel stack */
} thread_info;

enum
{
    IDLE_PRIORITY			= -100,
    LOW_PRIORITY			= -50,
    NORMAL_PRIORITY			= 0,
    DISPLAY_PRIORITY		= 50,
    URGENT_DISPLAY_PRIORITY	= 100,
    REALTIME_PRIORITY		= 150
};

status_t get_thread_info( const thread_id hThread, thread_info* const psInfo );
status_t get_next_thread_info( thread_info* psInfo );
proc_id get_thread_proc( const thread_id hThread );
thread_id get_thread_id( const char* const pzName );
proc_id get_process_id( const char* pzName );

status_t rename_process( int nProcessID, const char* pzName );
status_t rename_thread( thread_id hThread, const char* const pzName );

status_t send_data( const thread_id hThread, const uint32 nCode, void* const pData, const uint32 nSize );
uint32 receive_data( thread_id* const phSender, void* const pData, const uint32 nSize );
int thread_data_size( void );
int has_data( const thread_id hThread );

thread_id spawn_thread( const char* const pzName, void* const pfEntry, const int nPri, int nStackSize, void* const pData );
status_t suspend_thread( const thread_id hThread );
status_t resume_thread( const thread_id hThread );

status_t suspend( void );
status_t wakeup_thread( thread_id hThread, bool bWakeupSuspended );

int	exit_thread( int nCode );

int wait_for_thread( const thread_id hThread );

int set_thread_priority( const thread_id hThread, const int nPriority );
status_t set_thread_target_cpu( const thread_id hThread, const int nCpu );

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_THREADS_H__ */

