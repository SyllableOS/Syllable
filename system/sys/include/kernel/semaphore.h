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

#ifndef	__F_KERNEL_SEMAPHORES_H__
#define	__F_KERNEL_SEMAPHORES_H__

#include <kernel/types.h>
#include <kernel/threads.h>
#include <syllable/semaphore.h>

status_t lock_semaphore( sem_id hSema, uint32 nFlags, bigtime_t nTimeOut );
status_t lock_semaphore_ex( sem_id hSema, int nCount, uint32 nFlags, bigtime_t nTimeOut );
status_t unlock_semaphore_ex( sem_id hSema, int nCount );

status_t reset_semaphore( sem_id hSema, int nCount );

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

#endif	/* __F_KERNEL_SEMAPHORES_H__ */
