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

#ifndef __F_ATHEOS_SCHEDULE_H__
#define __F_ATHEOS_SCHEDULE_H__

#include <atheos/timer.h>
#include <posix/types.h>
#include <posix/resource.h>
#include <atheos/types.h>

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

#ifdef __KERNEL__






status_t  suspend_thread( const thread_id hThread );
status_t  resume_thread( const thread_id hThread );

status_t  stop_thread( bool bNotifyParent );
status_t  start_thread( thread_id hThread );


pid_t 	  sys_waitpid( pid_t hPid, int* pnStatLoc, int nOptions );
int	  sys_wait_for_thread( thread_id hThread );
void	  Schedule( void );

uint32	  sys_Delay( const uint32 nMicros );
int 	  sys_SetTaskPri( const thread_id hThread, const int pri );
status_t  sys_suspend_thread( const thread_id hThread );
status_t  sys_resume_thread( const thread_id hThread );
int	  wait_for_thread( thread_id hThread );
pid_t	  sys_wait4( const pid_t hPid, int* const pnStatLoc, const int nOptions, struct rusage* const psRUsage );

#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_SCHEDULE_H__ */
