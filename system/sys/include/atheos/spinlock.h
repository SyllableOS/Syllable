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

#ifdef __BUILD_KERNEL__
#include "inc/smp.h"
#else
#include <atheos/smp.h>
#endif

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif



typedef struct
{
  volatile atomic_t sl_nLocked;
  vint		    sl_nProc;
  volatile atomic_t sl_nNest;
  const char*	    sl_pzName;
} SpinLock_s;


#define SPIN_LOCK( var, name ) SpinLock_s var = { 0, -1, 0, name }

#define INIT_SPIN_LOCK( name ) ((SpinLock_s){ 0, -1, 0, name })

extern inline void spinlock_init( SpinLock_s* psLock, const char* pzName )
{
  psLock->sl_nLocked = 0;
  psLock->sl_nProc   = -1;
  psLock->sl_nNest   = 0;
  psLock->sl_pzName  = pzName;
}

extern inline int spinlock( SpinLock_s* psLock )
{
    int	nProcID = get_processor_id();
    int   nError = 0;

#ifdef DEBUG_SPINLOCKS  
    kassertw( (get_cpu_flags() & EFLG_IF) == 0 );
#endif
  
    while( atomic_swap( (int*)&psLock->sl_nLocked, 1 ) == 1 ) {
	if ( psLock->sl_nProc == nProcID ) {
	    psLock->sl_nNest++;
	    if ( psLock->sl_nNest > 50 ) {
		printk( "panic: spinlock %s nested to deep: %d\n", psLock->sl_pzName, psLock->sl_nNest );
		return( -1 );
	    }
	    return( 0 );
	}
    }
    psLock->sl_nProc = nProcID;
    psLock->sl_nNest++;
    if ( psLock->sl_nNest > 50 ) {
	return( -1 );
    }
    return( nError );
}

extern inline void spinunlock( SpinLock_s* psLock )
{
#ifdef DEBUG_SPINLOCKS  
  kassertw( psLock->sl_nLocked >= 1 );
  kassertw( psLock->sl_nProc == get_processor_id() );
  kassertw( psLock->sl_nNest > 0 );
  kassertw( (get_cpu_flags() & EFLG_IF) == 0 );
#endif /* DEBUG_SPINLOCKS */
  
  psLock->sl_nNest--;
  if ( psLock->sl_nNest == 0 ) {
    psLock->sl_nProc = -1;
    psLock->sl_nLocked = 0;
  }
}

static inline int spinlock_disable( SpinLock_s* psLock )
{
  uint32 nFlg = cli();
  spinlock( psLock );
  return( nFlg );
}

static inline void spinunlock_enable( SpinLock_s* psLock, uint32 nFlg )
{
  spinunlock( psLock );
  put_cpu_flags( nFlg );
}

status_t  spinunlock_and_suspend( sem_id hWaitQueue, SpinLock_s* psLock, uint32 nCPUFlags, bigtime_t nTimeout );

#define spinlock_cli( lock, flags ) flags = cli(); spinlock( lock )
#define spinunlock_restore( lock, flags ) spinunlock( lock ); put_cpu_flags( flags )


#ifdef __cplusplus
}
#endif


#endif /* __F_ATHEOS_SPINLOCK_H__ */
