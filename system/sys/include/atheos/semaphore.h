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



#define	SEM_RECURSIVE       0x0001	/* Allow same thread to lock semaphore multiple times	*/
#define SEM_WARN_DBL_LOCK   0x0002	/* Warn if a thread takes the semaphore twice		*/
#define SEM_WARN_DBL_UNLOCK 0x0004	/* Warn if the count gets higher than 1			*/
#define SEM_GLOBAL          0x0008	/* Alloc the semaphore in the global namespace		*/

#define SEM_STYLE_MASK      0x00f0
#define SEMSTYLE_COUNTING   0x0000  /* Ordinary counting semaphore */
#define SEMSTYLE_RWLOCK     0x0010  /* Reader-writer lock */
#define SEM_STYLE( sem )		( (sem) & SEM_STYLE_MASK )

sem_id		create_semaphore( const char* pzName, int nCount, uint32 nFlags );
status_t	delete_semaphore( sem_id hSema );

#ifdef __KERNEL__
status_t	lock_semaphore( sem_id hSema, uint32 nFlags, bigtime_t nTimeOut );
status_t 	lock_semaphore_ex( sem_id hSema, int nCount, uint32 nFlags, bigtime_t nTimeOut );
status_t	unlock_semaphore_ex( sem_id hSema, int nCount );

status_t	reset_semaphore( sem_id hSema, int nCount );

/* Reader-writer locks */
status_t rwl_lock_read_ex( sem_id hSema, uint32 nFlags, bigtime_t nTime );
status_t rwl_convert_to_anon( sem_id hSema );
status_t rwl_convert_from_anon( sem_id hSema );
status_t rwl_lock_read( sem_id hSema );
status_t rwl_unlock_read( sem_id hSema );
status_t rwl_unlock_read_anon( sem_id hSema );

status_t rwl_lock_write_ex( sem_id hSema, uint32 nFlags, bigtime_t nTime );
status_t rwl_lock_write( sem_id hSema );
status_t rwl_unlock_write( sem_id hSema );

status_t rwl_count_readers( sem_id hSema );
status_t rwl_count_all_readers( sem_id hSema );

status_t rwl_count_writers( sem_id hSema );
status_t rwl_count_all_writers( sem_id hSema );

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
#define	TRY_LOCK( sem ) lock_semaphore( sem, SEM_NOSIG, 0LL )
#define UNLOCK( sem )	unlock_semaphore( sem )

#define CREATE_MUTEX( name ) create_semaphore( name, 1, 0 )

/* Create a readers-writers lock */
#define CREATE_RWLOCK( name ) \
        create_semaphore( name, 0, SEMSTYLE_RWLOCK )

/* Lock for read-only */
#define RWL_LOCK_RO( sem ) \
        rwl_lock_read( sem )
/* Try to lock for read-only */
#define RWL_TRY_LOCK_RO( sem ) \
        rwl_lock_read_ex( sem, 0, 0LL )
/* Unlock from read-only */
#define RWL_UNLOCK_RO( sem ) \
        rwl_unlock_read( sem )

/* Lock for read-write */
#define RWL_LOCK_RW( sem ) \
        rwl_lock_write( sem )
/* Try to lock for read-write */
#define RWL_TRY_LOCK_RW( sem ) \
        rwl_lock_write_ex( sem, 0, 0LL )
/* Unlock from read-write */
#define RWL_UNLOCK_RW( sem ) \
        rwl_unlock_write( sem )

/* Upgrade a read-only lock to read-write */
#define RWL_UPGRADE( sem ) \
        rwl_lock_write( sem )
/* Try to upgrade a read-only lock to read-write */
#define RWL_TRY_UPGRADE( sem ) \
        rwl_lock_write_ex( sem, 0, 0LL )
/* Downgrade a read-write lock to read-only */
#define RWL_DOWNGRADE( sem ) \
        rwl_unlock_write( sem )

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
