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

#include <atheos/kernel.h>
#include <atheos/socket.h>
#include <net/net.h>
#include <net/ip.h>
#include <net/if_ether.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>

#include <macros.h>

#include "inc/sysbase.h"

SPIN_LOCK(g_sQueueSpinLock, "net_packets_slock" );

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

PacketBuf_s* alloc_pkt_buffer( int nSize )
{
  PacketBuf_s* psBuf;

  if ( g_sSysBase.ex_nFreePageCount < 1024*128/PAGE_SIZE ) {
    return( NULL );
  }
  
  psBuf = kmalloc( sizeof( PacketBuf_s ) + nSize, MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );

  if ( psBuf == NULL ) {
    return( NULL );
  }
  psBuf->pb_pHead = (char*)(psBuf + 1);
  psBuf->pb_pData = psBuf->pb_pHead;
  psBuf->pb_nRealSize = nSize;
  psBuf->pb_nSize     = nSize;
  psBuf->pb_bLocal    = false;

  psBuf->pb_nMagic    = 0x12345678;
  return( psBuf );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void reserve_pkt_header( PacketBuf_s* psBuf, int nSize )
{
  psBuf->pb_pData += nSize;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void free_pkt_buffer( PacketBuf_s* psBuf )
{
  kassertw( psBuf != NULL );
  kassertw( psBuf->pb_nMagic == 0x12345678 );
  kfree( psBuf );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int init_net_queue( NetQueue_s* psQueue )
{
  psQueue->nq_psHead = NULL;
  psQueue->nq_psTail = NULL;
  psQueue->nq_nCount = 0;
  psQueue->nq_hSyncSem = create_semaphore( "netq_sync", 0, 0 );
//  psQueue->nq_hProtSem = create_semaphore( "netq_prot", 1, SEM_REQURSIVE );
  return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void delete_net_queue( NetQueue_s* psQueue )
{
  delete_semaphore( psQueue->nq_hSyncSem );
//  delete_semaphore( psQueue->nq_hProtSem );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void enqueue_packet( NetQueue_s* psQueue, PacketBuf_s* psBuf )
{
    uint32 nFlags;
    kassertw( psBuf->pb_nMagic == 0x12345678 );
//  LOCK( psQueue->nq_hProtSem );
  
    if ( NULL != psBuf->pb_psNext || NULL != psBuf->pb_psPrev ) {
	printk( "Error: enqueue_packet() Attempt to add buffer twice!\n" );
    }

    nFlags = spinlock_disable( &g_sQueueSpinLock );
    
    if ( psQueue->nq_psTail != NULL ) {
	psQueue->nq_psTail->pb_psNext = psBuf;
    }
    if ( psQueue->nq_psHead == NULL ) {
	psQueue->nq_psHead = psBuf;
    }
  
    psBuf->pb_psNext = NULL;
    psBuf->pb_psPrev = psQueue->nq_psTail;
    psQueue->nq_psTail = psBuf;
    psQueue->nq_nCount++;
    spinunlock_enable( &g_sQueueSpinLock, nFlags );
    
    unlock_semaphore( psQueue->nq_hSyncSem );
//    UNLOCK( psQueue->nq_hProtSem );
//  Schedule();
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void remove_packet( NetQueue_s* psQueue, PacketBuf_s* psBuf )
{
    kassertw( psBuf->pb_nMagic == 0x12345678 );
    if ( NULL != psBuf->pb_psNext ) {
	psBuf->pb_psNext->pb_psPrev = psBuf->pb_psPrev;
    }
    if ( NULL != psBuf->pb_psPrev ) {
	psBuf->pb_psPrev->pb_psNext = psBuf->pb_psNext;
    }

    if ( psQueue->nq_psHead == psBuf ) {
	psQueue->nq_psHead = psBuf->pb_psNext;
    }
    if ( psQueue->nq_psTail == psBuf ) {
	psQueue->nq_psTail = psBuf->pb_psPrev;
    }

    psBuf->pb_psNext = NULL;
    psBuf->pb_psPrev = NULL;
    psQueue->nq_nCount--;
  
    if ( psQueue->nq_nCount < 0 ) {
	panic( "remove_packet() packet buffer list %p got count of %d\n", psQueue, psQueue->nq_nCount );
    }
}

PacketBuf_s* remove_head_packet( NetQueue_s* psQueue, bigtime_t nTimeout )
{
    PacketBuf_s* psBuf;
    uint32	 nFlags;
  
    lock_semaphore( psQueue->nq_hSyncSem, 0, nTimeout );
  
//  LOCK( psQueue->nq_hProtSem );
    nFlags = spinlock_disable( &g_sQueueSpinLock );
    psBuf = psQueue->nq_psHead;
    if ( psBuf != NULL ) {
	remove_packet( psQueue, psBuf );
    }
    spinunlock_enable( &g_sQueueSpinLock, nFlags );
//  UNLOCK( psQueue->nq_hProtSem );
    return( psBuf );
}

