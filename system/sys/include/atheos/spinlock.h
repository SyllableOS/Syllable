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

#ifndef __F_ATHEOS_SPINLOCK_H__
#define __F_ATHEOS_SPINLOCK_H__

#include <atheos/types.h>
#include <atheos/atomic.h>
#include <atheos/irq.h>
#include <macros.h>

#include <atheos/smp.h>

#ifdef __BUILD_KERNEL__
#include "inc/smp.h"		// pick up inline get_processor_id() function
#endif

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif



typedef struct
{
  atomic_t		sl_nLocked;
  volatile uint_fast32_t sl_nProc;
  atomic_t 		sl_nNest;
  const char*		sl_pzName;
} SpinLock_s;


#define SPIN_LOCK( var, name ) SpinLock_s var = { ATOMIC_INIT(0), -1, ATOMIC_INIT(0), name }

#define INIT_SPIN_LOCK( name ) { ATOMIC_INIT(0), -1, ATOMIC_INIT(0), name }

static inline void spinlock_init( SpinLock_s* psLock, const char* pzName )
{
  atomic_set(&psLock->sl_nLocked, 0);
  psLock->sl_nProc   = -1;
  atomic_set(&psLock->sl_nNest, 0);
  psLock->sl_pzName  = pzName;
}

static inline int spinlock( SpinLock_s* psLock )
{
    uint_fast32_t nProcID = get_processor_id();

#ifdef DEBUG_SPINLOCKS  
    kassertw( (get_cpu_flags() & EFLG_IF) == 0 );
#endif
  
    while( atomic_swap( &psLock->sl_nLocked, 1 ) == 1 ) {
	if ( psLock->sl_nProc == nProcID ) {
	    atomic_inc( &psLock->sl_nNest );
#ifdef DEBUG_SPINLOCKS
	    if ( atomic_read( &psLock->sl_nNest ) > 50 ) {
		printk( "panic: spinlock %s nested too deep: %d\n",
			psLock->sl_pzName, atomic_read( &psLock->sl_nNest ) );
		return( -1 );
	    }
#endif
	    return( 0 );
	}
	do {
		__asm__ __volatile__( "pause" );
	} while ( atomic_read( &psLock->sl_nLocked ) == 1 );
    }
    psLock->sl_nProc = nProcID;
    atomic_inc( &psLock->sl_nNest );
#ifdef DEBUG_SPINLOCKS
    kassertw( atomic_read( &psLock->sl_nNest ) == 1 );
#endif
    return( 0 );
}

static inline void spinunlock( SpinLock_s* psLock )
{
#ifdef DEBUG_SPINLOCKS  
  kassertw( atomic_read( &psLock->sl_nLocked ) >= 1 );
  kassertw( psLock->sl_nProc == get_processor_id() );
  kassertw( atomic_read( &psLock->sl_nNest ) > 0 );
  kassertw( (get_cpu_flags() & EFLG_IF) == 0 );
#endif /* DEBUG_SPINLOCKS */
  
  atomic_dec( &psLock->sl_nNest );
  if ( atomic_read( &psLock->sl_nNest ) == 0 ) {
    psLock->sl_nProc = -1;
    atomic_set( &psLock->sl_nLocked, 0 );
  }
}

/*
 * Try to get the lock.
 * Returns 0 on success, -1 on failure.
 */
static inline int spin_trylock( SpinLock_s* psLock )
{
	uint_fast32_t nProcID = get_processor_id();

#ifdef DEBUG_SPINLOCKS
	kassertw( ( get_cpu_flags() & EFLG_IF ) == 0 );
#endif

	if ( atomic_swap( &psLock->sl_nLocked, 1 ) == 1 )
	{
		if ( psLock->sl_nProc == nProcID )
		{
			atomic_inc( &psLock->sl_nNest );
			return( 0 );
		}
		else
		{
			return( -1 );
		}
	}
	psLock->sl_nProc = nProcID;
	atomic_inc( &psLock->sl_nNest );
	return( 0 );
}

static inline int spinlock_disable( SpinLock_s* psLock )
{
  uint32_t nFlg = cli();
  spinlock( psLock );
  return( nFlg );
}

static inline void spinunlock_enable( SpinLock_s* psLock, uint32_t nFlg )
{
  spinunlock( psLock );
  put_cpu_flags( nFlg );
}

status_t  spinunlock_and_suspend( sem_id hWaitQueue, SpinLock_s* psLock, uint32_t nCPUFlags, bigtime_t nTimeout );

#define spinlock_cli( lock, flags ) flags = cli(); spinlock( lock )
#define spinunlock_restore( lock, flags ) spinunlock( lock ); put_cpu_flags( flags )


#ifdef __cplusplus
}
#endif


#endif /* __F_ATHEOS_SPINLOCK_H__ */
