
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2002 Ville Kallioniemi
 *
 *  Changes:
 *  2002		Ville Kallioniemi		Added dump_pkt_buffer & clone_pkt_buffer
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
#include <atheos/kdebug.h>
#include <atheos/socket.h>
#include <atheos/time.h>
#include <net/net.h>
#include <net/ip.h>
#include <net/if_ether.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>

#include <macros.h>

#include "inc/sysbase.h"

SPIN_LOCK( g_sQueueSpinLock, "net_packets_slock" );

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void dump_pkt_buffer( PacketBuf_s *psBuf )
{
	if ( psBuf == NULL )
		return;

	kerndbg( KERN_DEBUG_LOW, "PACKET BUFFER DUMP==============================================\n" );
	kerndbg( KERN_DEBUG_LOW, "psBuf:\t %p \n", psBuf );
	kerndbg( KERN_DEBUG_LOW, "psBuf->pb_psNext:\t %p \n", psBuf->pb_psNext );
	kerndbg( KERN_DEBUG_LOW, "psBuf->pb_psPrev:\t %p \n", psBuf->pb_psPrev );
	kerndbg( KERN_DEBUG_LOW, "psBuf->pb_nRealSize:\t %d \n", psBuf->pb_nRealSize );
	kerndbg( KERN_DEBUG_LOW, "psBuf->pb_nSize:\t %d \n", psBuf->pb_nSize );
	kerndbg( KERN_DEBUG_LOW, "psBuf->pb_nProtocol:\t %d \n", psBuf->pb_nProtocol );
	kerndbg( KERN_DEBUG_LOW, "psBuf->pb_pHead:\t %p \n", psBuf->pb_pHead );
	kerndbg( KERN_DEBUG_LOW, "psBuf->pb_pData:\t %p \n", psBuf->pb_pData );
	kerndbg( KERN_DEBUG_LOW, "psBuf->pb_nMagic:\t %ld \n", psBuf->pb_nMagic );
	kerndbg( KERN_DEBUG_LOW, "&psBuf->pb_nMagic:\t %p \n", &( psBuf->pb_nMagic ) );
	kerndbg( KERN_DEBUG_LOW, "================================================================\n" );

}


PacketBuf_s *alloc_pkt_buffer( int nSize )
{
	PacketBuf_s *psBuf;

	if ( atomic_read( &g_sSysBase.ex_nFreePageCount ) < 1024 * 128 / PAGE_SIZE )
	{
		return ( NULL );
	}

	psBuf = kmalloc( sizeof( PacketBuf_s ) + nSize, MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK );

	if ( psBuf == NULL )
	{
		return ( NULL );
	}
	psBuf->pb_pHead = ( char * )( psBuf + 1 );
	psBuf->pb_pData = psBuf->pb_pHead;
	psBuf->pb_nRealSize = nSize;
	psBuf->pb_nSize = nSize;
	psBuf->pb_bLocal = false;

	psBuf->pb_nMagic = 0x12345678;
	return ( psBuf );
}

PacketBuf_s *clone_pkt_buffer( PacketBuf_s *psBuf )
{
	PacketBuf_s *psClonedPacket = NULL;

	// Allocate a new empty buffer
	kerndbg( KERN_DEBUG, "clone_pkt_buffer(): psBuf->pb_nRealSize: %d\n", psBuf->pb_nRealSize );

	psClonedPacket = alloc_pkt_buffer( psBuf->pb_nRealSize );
	if ( psClonedPacket == NULL )
		return NULL;

	kassertw( psClonedPacket->pb_nMagic == 0x12345678 );

	// If the memory allocations succeeded we can copy the information
	memcpy( ( void * )psClonedPacket, ( void * )psBuf, ( psBuf->pb_nRealSize + sizeof( PacketBuf_s ) ) );

	// and set the pointers right???
	psClonedPacket->pb_psNext = NULL;
	psClonedPacket->pb_psPrev = NULL;
	psClonedPacket->pb_nIfIndex = psBuf->pb_nIfIndex;

	psClonedPacket->pb_uMacHdr.pRaw = ( ( void * )psBuf->pb_uMacHdr.pRaw - ( void * )psBuf ) + ( void * )psClonedPacket;
	psClonedPacket->pb_uNetworkHdr.pRaw = ( ( void * )psBuf->pb_uNetworkHdr.pRaw - ( void * )psBuf ) + ( void * )psClonedPacket;
	psClonedPacket->pb_uTransportHdr.pRaw = ( ( void * )psBuf->pb_uTransportHdr.pRaw - ( void * )psBuf ) + ( void * )psClonedPacket;

	psClonedPacket->pb_pData = ( ( void * )psBuf->pb_pData - ( void * )psBuf ) + ( void * )psClonedPacket;
	psClonedPacket->pb_pHead = ( ( void * )psBuf->pb_pHead - ( void * )psBuf ) + ( void * )psClonedPacket;

	kassertw( psClonedPacket->pb_nMagic == 0x12345678 );

	kerndbg( KERN_DEBUG_LOW, "clone_pkt_buffer(): Leaving.\n" );

	return psClonedPacket;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
void reserve_pkt_header( PacketBuf_s *psBuf, int nSize )
{
	psBuf->pb_pData += nSize;
}

/**
 * \par Description:
 * Releases the memory pointed to by psBuf.
 *
 * If the packet refers to an interface, the reference is released.
 */
void free_pkt_buffer( PacketBuf_s *psBuf )
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

int init_net_queue( NetQueue_s *psQueue )
{
	psQueue->nq_psHead = NULL;
	psQueue->nq_psTail = NULL;
	psQueue->nq_nCount = 0;
	psQueue->nq_hSyncSem = create_semaphore( "netq_sync", 0, 0 );
//  psQueue->nq_hProtSem = create_semaphore( "netq_prot", 1, SEM_RECURSIVE );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void delete_net_queue( NetQueue_s *psQueue )
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

void enqueue_packet( NetQueue_s *psQueue, PacketBuf_s *psBuf )
{
	uint32 nFlags;

	kassertw( psBuf->pb_nMagic == 0x12345678 );

	if ( NULL != psBuf->pb_psNext || NULL != psBuf->pb_psPrev )
	{
		kerndbg( KERN_WARNING, "Error: enqueue_packet() Attempt to add buffer twice!\n" );
	}

	nFlags = spinlock_disable( &g_sQueueSpinLock );

	if ( psQueue->nq_psTail != NULL )
	{
		psQueue->nq_psTail->pb_psNext = psBuf;
	}
	if ( psQueue->nq_psHead == NULL )
	{
		psQueue->nq_psHead = psBuf;
	}

	psBuf->pb_psNext = NULL;
	psBuf->pb_psPrev = psQueue->nq_psTail;
	psQueue->nq_psTail = psBuf;
	psQueue->nq_nCount++;

	spinunlock_enable( &g_sQueueSpinLock, nFlags );

	unlock_semaphore( psQueue->nq_hSyncSem );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void remove_packet( NetQueue_s *psQueue, PacketBuf_s *psBuf )
{
	kassertw( psBuf->pb_nMagic == 0x12345678 );
	if ( NULL != psBuf->pb_psNext )
	{
		psBuf->pb_psNext->pb_psPrev = psBuf->pb_psPrev;
	}
	if ( NULL != psBuf->pb_psPrev )
	{
		psBuf->pb_psPrev->pb_psNext = psBuf->pb_psNext;
	}

	if ( psQueue->nq_psHead == psBuf )
	{
		psQueue->nq_psHead = psBuf->pb_psNext;
	}
	if ( psQueue->nq_psTail == psBuf )
	{
		psQueue->nq_psTail = psBuf->pb_psPrev;
	}

	psBuf->pb_psNext = NULL;
	psBuf->pb_psPrev = NULL;
	psQueue->nq_nCount--;

	if ( psQueue->nq_nCount < 0 )
	{
		panic( "remove_packet() packet buffer list %p got count of %d\n", psQueue, psQueue->nq_nCount );
	}
}

PacketBuf_s *remove_head_packet( NetQueue_s *psQueue, bigtime_t nTimeout )
{
	PacketBuf_s *psBuf;
	uint32 nFlags;
	bigtime_t nNow = get_system_time();

	lock_semaphore( psQueue->nq_hSyncSem, 0, nTimeout );

	nNow = get_system_time() - nNow;

	nFlags = spinlock_disable( &g_sQueueSpinLock );

	psBuf = psQueue->nq_psHead;
	if ( psBuf != NULL )
	{
		remove_packet( psQueue, psBuf );
	}

	spinunlock_enable( &g_sQueueSpinLock, nFlags );

	return ( psBuf );
}
