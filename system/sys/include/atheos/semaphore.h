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

#ifndef	__F_ATHEOS_SEMAPHORES_H__
#define	__F_ATHEOS_SEMAPHORES_H__
#include <atheos/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    sem_id 	si_sema_id;
    proc_id 	si_owner;
    int32	si_count;
    thread_id 	si_latest_holder;
    char	si_name[OS_NAME_LENGTH];
} sem_info;



#define	SEM_RECURSIVE	    0x0001	/* Allow same thread to lock semaphore multiple times	*/
#define SEM_WARN_DBL_LOCK   0x0002	/* Warn if a thread takes the semaphore twice		*/
#define SEM_WARN_DBL_UNLOCK 0x0004	/* Warn if the count gets higher than 1			*/
#define SEM_GLOBAL	    0x0008	/* Alloc the semaphore in the global namespace		*/


#define	SEM_REQURSIVE	    SEM_RECURSIVE /* Backward compatibility with an old typo */

sem_id		create_semaphore( const char* pzName, int nCount, uint32 nFlags );
status_t	delete_semaphore( sem_id hSema );

#ifdef __KERNEL__
status_t	lock_semaphore( sem_id hSema, uint32 nFlags, bigtime_t nTimeOut );
status_t 	lock_semaphore_ex( sem_id hSema, int nCount, uint32 nFlags, bigtime_t nTimeOut );
status_t	unlock_semaphore_ex( sem_id hSema, int nCount );
#else
status_t	lock_semaphore( sem_id hSema );
#endif

status_t  lock_mutex( sem_id hSema, bool bIgnoreSignals );
status_t  unlock_mutex( sem_id hSema );

status_t  raw_lock_semaphore_x( sem_id hSema, int nCount, uint32 nFlags, const bigtime_t* pnTimeOut );
status_t  lock_semaphore_x( sem_id hSema, int nCount, uint32 nFlags, bigtime_t nTimeOut );

status_t  unlock_semaphore( sem_id hSema );
status_t  unlock_semaphore_x( sem_id hSema, int nCount, uint32 nFlags );

status_t  sleep_on_sem( sem_id hSema, bigtime_t nTimeOut );
status_t  wakeup_sem( sem_id hSema, bool bAll );
status_t  unlock_and_suspend( sem_id hWaitQueue, sem_id hSema );

#ifdef __KERNEL__

#define	LOCK( sem ) 	lock_semaphore( sem, SEM_NOSIG, INFINITE_TIMEOUT )
#define UNLOCK( sem )	unlock_semaphore( sem )
#endif

int 	  get_semaphore_count( sem_id hSema );
thread_id get_semaphore_owner( sem_id hSema );

status_t  get_semaphore_info( sem_id hSema, proc_id hOwner, sem_info* psInfo );
status_t  get_next_semaphore_info( sem_info* psInfo );

int	is_semaphore_locked( sem_id hSema );


#ifdef __cplusplus
}
#endif


#endif	/* __F_ATHEOS_SEMAPHORES_H__ */
