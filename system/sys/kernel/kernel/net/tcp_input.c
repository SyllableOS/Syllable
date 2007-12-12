
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

#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/time.h>
#include <atheos/socket.h>
#include <atheos/semaphore.h>

#include <net/net.h>
#include <net/in.h>
#include <net/ip.h>
#include <net/if_ether.h>
#include <net/icmp.h>
#include "tcpdefs.h"

#include <macros.h>

#include "inc/sysbase.h"

/*
 * Locking policies:
 *	A connection can have the following references.
 *	It could be owned by a socket.
 *	The tcp_in() function might find it. If so it will be locked before the connection list are released.
 *	It could be found in the even list by the timer thread. It will then be locked.
 *	The connection must under no condition be deleted while owned by a socket.
 *	If it has been aborted while owned by a socked we just remove it from the connection list
 *	and mark it as dead, so close() will kill it.
 *	To avoid having the input thread deleting a connection handled by the output thread or visa versa
 *	we mark the connection as busy whenever we fetch it from a list, and makes sure nobody
 *	touches a busy connection.
 */


static TCPCtrl_s *g_psFirstTCP = NULL;
sem_id g_hConnListMutex = -1;


static TCPEvent_s *g_psFirstTimerEvent = NULL;
static TCPEvent_s *g_psLastTimerEvent = NULL;

int g_nTcpAttemptFails = 0;
int g_nTcpOutSegs = 0;
int g_nTcpEstabResets = 0;
int g_nTcpCurrEstab = 0;
int g_nTcpPassiveOpens = 0;

#define TCPSBS (32768-16)
#define TCPRBS (32768-16)

#define LISTEN_HASH_SIZE	512
#define ESTABLISHED_HASH_SIZE	4096

// Hash table for sockets in listen state. This are hashed on the local port number
// This is the only sockets that can have wildcards.
TCPCtrl_s *g_apsListenHash[LISTEN_HASH_SIZE];

// Hash table for fully qualified sockets (no wildcards). This is hashed on both remote
// and local ip/port address.
TCPCtrl_s *g_apsEstablishedHash[ESTABLISHED_HASH_SIZE];

static inline int tcp_hash_listen( uint16 nLocalPort )
{
	return ( nLocalPort & ( LISTEN_HASH_SIZE - 1 ) );
}

static inline int tcp_hash_full( uint32 nLocalAddr, uint16 nLocalPort, uint32 nRemoteaddr, uint16 nRemotePort )
{
	return ( ( ( nLocalAddr ^ nLocalPort ) ^ ( nRemoteaddr ^ nRemotePort ) ) & ( ESTABLISHED_HASH_SIZE - 1 ) );
}


const char *tcp_state_str( int nState )
{
	switch ( nState )
	{
	case TCPS_FREE:
		return ( "TCPS_FREE" );
	case TCPS_CLOSED:
		return ( "TCPS_CLOSED" );
	case TCPS_LISTEN:
		return ( "TCPS_LISTEN" );
	case TCPS_SYNSENT:
		return ( "TCPS_SYNSENT" );
	case TCPS_SYNRCVD:
		return ( "TCPS_SYNRCVD" );
	case TCPS_ESTABLISHED:
		return ( "TCPS_ESTABLISHED" );
	case TCPS_FINWAIT1:
		return ( "TCPS_FINWAIT1" );
	case TCPS_FINWAIT2:
		return ( "TCPS_FINWAIT2" );
	case TCPS_CLOSEWAIT:
		return ( "TCPS_CLOSEWAIT" );
	case TCPS_LASTACK:
		return ( "TCPS_LASTACK" );
	case TCPS_CLOSING:
		return ( "TCPS_CLOSING" );
	case TCPS_TIMEWAIT:
		return ( "TCPS_TIMEWAIT" );
	default:
		return ( "*UNKNOWN*" );
	}
}


TCPCtrl_s *tcp_lookup( uint32 nLocalAddr, uint16 nLocalPort, uint32 nRemoteAddr, uint16 nRemotePort )
{
	TCPCtrl_s *psTCPCtrl;
	int nRetries = 0;
	int nHash = tcp_hash_full( nLocalAddr, nLocalPort, nRemoteAddr, nRemotePort );

      again:
	LOCK( g_hConnListMutex );
	for ( psTCPCtrl = g_apsEstablishedHash[nHash]; psTCPCtrl != NULL; psTCPCtrl = psTCPCtrl->tcb_psNextHash )
	{
		if ( nLocalPort == psTCPCtrl->tcb_lport && nRemotePort == psTCPCtrl->tcb_rport && IP_SAMEADDR( ( uint8 * )&nRemoteAddr, psTCPCtrl->tcb_rip ) && IP_SAMEADDR( ( uint8 * )&nLocalAddr, psTCPCtrl->tcb_lip ) )
		{
			goto found;
		}
	}
	for ( psTCPCtrl = g_apsListenHash[tcp_hash_listen( nLocalPort )]; psTCPCtrl != NULL; psTCPCtrl = psTCPCtrl->tcb_psNextHash )
	{
		if ( nLocalPort == psTCPCtrl->tcb_lport )
		{
			goto found;
		}
	}
	UNLOCK( g_hConnListMutex );
	return ( NULL );

      found:
	if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_BUSY )
	{
		UNLOCK( g_hConnListMutex );
		snooze( 1000 );
//      printk( "tcp_demux() Wait for connection to become unbusy\n" );
		if ( nRetries++ > 5000 )
		{
			printk( "Error: tcp_demux() tcp connection stuck in busy state\n" );
			return ( NULL );
		}
		goto again;
	}
	atomic_or( &psTCPCtrl->tcb_flags, TCBF_BUSY );
	UNLOCK( g_hConnListMutex );
	LOCK( psTCPCtrl->tcb_hMutex );
	return ( psTCPCtrl );
}


void tcp_add_to_hash( TCPCtrl_s *psTCPCtrl )
{
	LOCK( g_hConnListMutex );
	if ( psTCPCtrl->tcb_nState == TCPS_LISTEN )
	{
		int nHash = tcp_hash_listen( psTCPCtrl->tcb_lport );

		psTCPCtrl->tcb_psNextHash = g_apsListenHash[nHash];
		g_apsListenHash[nHash] = psTCPCtrl;
	}
	else
	{
		int nHash = tcp_hash_full( *( ( uint32 * )psTCPCtrl->tcb_lip ), psTCPCtrl->tcb_lport, *( ( uint32 * )psTCPCtrl->tcb_rip ), psTCPCtrl->tcb_rport );

		psTCPCtrl->tcb_psNextHash = g_apsEstablishedHash[nHash];
		g_apsEstablishedHash[nHash] = psTCPCtrl;
	}
	UNLOCK( g_hConnListMutex );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void dbg_list_connections( int argc, char **argv )
{
	TCPCtrl_s *psTCPCtrl;
	int i = 0;

	LOCK( g_hConnListMutex );
	for ( psTCPCtrl = g_psFirstTCP; psTCPCtrl != NULL; psTCPCtrl = psTCPCtrl->tcb_psNext )
	{
		char zRIP[32];
		char zLIP[32];
		char zEvents[OE_COUNT + 1];
		int j;

		format_ipaddress( zRIP, psTCPCtrl->tcb_rip );
		format_ipaddress( zLIP, psTCPCtrl->tcb_lip );

		memset( zEvents, '.', OE_COUNT );
		zEvents[OE_COUNT] = '\0';

		for ( j = 0; j < OE_COUNT; ++j )
		{
			if ( psTCPCtrl->tcp_asEvents[j].te_psTCPCtrl != NULL )
			{
				switch ( psTCPCtrl->tcp_asEvents[j].te_nEvent )
				{
				case SEND:
					zEvents[j] = 'S';
					break;
				case RETRANSMIT:
					zEvents[j] = 'R';
					break;
				case PERSIST:
					zEvents[j] = 'P';
					break;
				case DELETE:
					zEvents[j] = 'D';
					break;
				case TCP_TE_KEEPALIVE:
					zEvents[j] = 'K';
					break;
				}
			}
		}

		dbprintf( DBP_DEBUGGER, "%03d: %s:%d -> %s:%d %s, os=%d, e=%s\n", i++, zRIP, psTCPCtrl->tcb_rport, zLIP, psTCPCtrl->tcb_lport, tcp_state_str( psTCPCtrl->tcb_nState ), psTCPCtrl->tcb_ostate, zEvents );

		SelectRequest_s* psReq;
		for ( psReq = psTCPCtrl->tcb_psFirstReadSelReq; NULL != psReq; psReq = psReq->sr_psNext )
		{
			dbprintf( DBP_DEBUGGER, "\tRead select request %i\n", psReq->sr_nMode );
		}
		for ( psReq = psTCPCtrl->tcb_psFirstWriteSelReq; NULL != psReq; psReq = psReq->sr_psNext )
		{
			dbprintf( DBP_DEBUGGER, "\tWrite select request %i\n", psReq->sr_nMode );
		}

	}
	UNLOCK( g_hConnListMutex );
}

static void tcp_unlink_event( TCPEvent_s *psEvent )
{
	if ( psEvent->te_psPrev == NULL )
	{
		g_psFirstTimerEvent = psEvent->te_psNext;
	}
	else
	{
		psEvent->te_psPrev->te_psNext = psEvent->te_psNext;
	}
	if ( psEvent->te_psNext == NULL )
	{
		g_psLastTimerEvent = psEvent->te_psPrev;
	}
	else
	{
		psEvent->te_psNext->te_psPrev = psEvent->te_psPrev;
	}
	psEvent->te_psPrev = NULL;
	psEvent->te_psNext = NULL;
	psEvent->te_psTCPCtrl = NULL;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bigtime_t tcp_delete_event( TCPCtrl_s *psTCPCtrl, int nEvent )
{
	bigtime_t nTimeSpent = -1LL;

	if ( nEvent < 0 || nEvent >= OE_COUNT )
	{
		printk( "Error: tcp_delete_event() Attempt to create invalid event %d\n", nEvent );
		return ( -1 );
	}

	LOCK( g_hConnListMutex );
	if ( psTCPCtrl->tcp_asEvents[nEvent].te_psTCPCtrl != NULL )
	{
		nTimeSpent = get_system_time() - psTCPCtrl->tcp_asEvents[nEvent].te_nCreateTime;
		tcp_unlink_event( &psTCPCtrl->tcp_asEvents[nEvent] );
	}
	UNLOCK( g_hConnListMutex );
	return ( nTimeSpent );
}

/****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int tcp_create_event( TCPCtrl_s *psTCPCtrl, int nEvent, bigtime_t nDelay, bool bReplace )
{
	TCPEvent_s *psEvent;
	TCPEvent_s *psTmp;
	bigtime_t nCreateTime = 0;
	bool bWakeup = false;

	if ( nEvent < 0 || nEvent >= OE_COUNT )
	{
		printk( "Error: tcp_create_event() Attempt to create invalid event %d\n", nEvent );
		return ( -EINVAL );
	}
	LOCK( g_hConnListMutex );

	psEvent = &psTCPCtrl->tcp_asEvents[nEvent];
	if ( psEvent->te_psTCPCtrl != NULL )
	{
		if ( bReplace == false )
		{
			nCreateTime = psEvent->te_nCreateTime;
//          UNLOCK( g_hConnListMutex );
//          return( 0 );
		}
		tcp_unlink_event( psEvent );
	}

	psEvent->te_nCreateTime = get_system_time();
	psEvent->te_nExpireTime = psEvent->te_nCreateTime + nDelay;
	if ( nCreateTime != 0 )
	{
		psEvent->te_nCreateTime = nCreateTime;
	}
	psEvent->te_nEvent = nEvent;
	psEvent->te_psTCPCtrl = psTCPCtrl;

	if ( g_psFirstTimerEvent == NULL || psEvent->te_nExpireTime <= g_psFirstTimerEvent->te_nExpireTime )
	{
		bWakeup = true;
		if ( g_psFirstTimerEvent != NULL )
		{
			g_psFirstTimerEvent->te_psPrev = psEvent;
		}
		psEvent->te_psNext = g_psFirstTimerEvent;
		psEvent->te_psPrev = NULL;
		g_psFirstTimerEvent = psEvent;
		if ( g_psLastTimerEvent == NULL )
		{
			g_psLastTimerEvent = psEvent;
		}
	}
	else
	{
		for ( psTmp = g_psLastTimerEvent; psTmp != NULL; psTmp = psTmp->te_psPrev )
		{
			if ( psEvent->te_nExpireTime >= psTmp->te_nExpireTime )
			{
				if ( psTmp->te_psNext != NULL )
				{
					psTmp->te_psNext->te_psPrev = psEvent;
				}
				else
				{
					g_psLastTimerEvent = psEvent;
				}
				psEvent->te_psPrev = psTmp;
				psEvent->te_psNext = psTmp->te_psNext;
				psTmp->te_psNext = psEvent;
				break;
			}
		}
	}
	UNLOCK( g_hConnListMutex );
	if ( bWakeup )
	{
		wakeup_thread( g_hTCPTimerThread, false );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bigtime_t tcp_event_time_left( TCPCtrl_s *psTCPCtrl, int nEvent )
{
	bigtime_t nCurTime = get_system_time();
	bigtime_t nLeft = 0;

	if ( nEvent < 0 || nEvent >= OE_COUNT )
	{
		printk( "Error: tcp_event_time_left() Attempt to create invalid event %d\n", nEvent );
		return ( 0 );
	}

	LOCK( g_hConnListMutex );
	if ( psTCPCtrl->tcp_asEvents[nEvent].te_psTCPCtrl != NULL )
	{
		nLeft = nCurTime < psTCPCtrl->tcp_asEvents[nEvent].te_nExpireTime;
	}
	UNLOCK( g_hConnListMutex );
	return ( nLeft );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

TCPCtrl_s *tcp_get_event( int *pnEvent )
{
	TCPCtrl_s *psTCPCtrl = NULL;
	TCPEvent_s *psEvent = NULL;

	LOCK( g_hConnListMutex );
	for ( ;; )
	{
		bigtime_t nCurTime = get_system_time();

		psEvent = g_psFirstTimerEvent;
		if ( psEvent == NULL || nCurTime < psEvent->te_nExpireTime )
		{
			bigtime_t nDelay;

			if ( psEvent == NULL )
			{
				nDelay = 200000;
			}
			else
			{
				nDelay = psEvent->te_nExpireTime - nCurTime;
			}
			if ( ( ( int64 )nDelay ) < 0 )
			{
				printk( "Error: tcp_get_event() got negative timeout %d\n", ( int )nDelay );
				nDelay = 0;
			}
			UNLOCK( g_hConnListMutex );
			snooze( nDelay );
			LOCK( g_hConnListMutex );
			continue;
		}

		while ( atomic_read( &psEvent->te_psTCPCtrl->tcb_flags ) & TCBF_BUSY )
		{
			if ( psEvent->te_psNext != NULL && psEvent->te_psNext->te_nExpireTime <= nCurTime )
			{
				psEvent = psEvent->te_psNext;
				continue;
			}
			UNLOCK( g_hConnListMutex );
			snooze( 1000 );
			LOCK( g_hConnListMutex );
			psEvent = NULL;
			break;
		}
		if ( psEvent == NULL )
		{
			continue;
		}
		psTCPCtrl = psEvent->te_psTCPCtrl;
		atomic_or( &psTCPCtrl->tcb_flags, TCBF_BUSY );
		*pnEvent = psEvent->te_nEvent;
		tcp_unlink_event( psEvent );
		break;
	}
	UNLOCK( g_hConnListMutex );

	if ( psTCPCtrl != NULL )
	{
		LOCK( psTCPCtrl->tcb_hMutex );
	}

	return ( psTCPCtrl );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Delete all events that may be pending on a connection
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_killtimers( TCPCtrl_s *psTCPCtrl )
{
	int i;

	LOCK( g_hConnListMutex );
	for ( i = 0; i < OE_COUNT; ++i )
	{
		if ( psTCPCtrl->tcp_asEvents[i].te_psTCPCtrl != NULL )
		{
			tcp_unlink_event( &psTCPCtrl->tcp_asEvents[i] );
		}
	}
	UNLOCK( g_hConnListMutex );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint16 tcp_cksum( PacketBuf_s *psPkt )
{
	TCPHeader_s *psTCPHdr = psPkt->pb_uTransportHdr.psTCP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	uint16 *psh;
	uint32 sum;
	uint16 len = ntohs( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr );
	int i;

	sum = 0;

	psh = ( uint16 * )&psIpHdr->iph_nSrcAddr[0];

	for ( i = 0; i < IP_ADR_LEN; ++i )
	{
		sum += psh[i];
	}

	psh = ( uint16 * )psTCPHdr;
	sum += htons( ( uint16 )psIpHdr->iph_nProtocol );
	sum += htons( len );

	if ( len & 0x1 )
	{
		( ( char * )psTCPHdr )[len] = 0;	/* pad */
		len += 1;	/* for the following division */
	}
	len >>= 1;		/* convert to length in shorts */

	for ( i = 0; i < len; ++i )
	{
		sum += psh[i];
	}
	sum = ( sum >> 16 ) + ( sum & 0xffff );
	sum += ( sum >> 16 );

	return ( ~sum );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

TCPCtrl_s *tcb_alloc( void )
{
	TCPCtrl_s *psTCPCtrl;

	psTCPCtrl = kmalloc( sizeof( TCPCtrl_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psTCPCtrl == NULL )
	{
		return ( NULL );
	}
	psTCPCtrl->tcb_nState = TCPS_CLOSED;
	psTCPCtrl->tcb_hMutex = create_semaphore( "tcp_tcb_mutex", 1, SEM_RECURSIVE );
	psTCPCtrl->tcb_hRecvQueue = -1;
	psTCPCtrl->tcb_hSendQueue = -1;
	psTCPCtrl->tcb_hListenSem = -1;
	psTCPCtrl->tcb_ocsem = -1;
	atomic_set( &psTCPCtrl->tcb_flags, TCBF_ACTIVE );
	psTCPCtrl->tcb_nLastReceiveTime = get_system_time();

	LOCK( g_hConnListMutex );
	psTCPCtrl->tcb_psNext = g_psFirstTCP;
	g_psFirstTCP = psTCPCtrl;
	UNLOCK( g_hConnListMutex );
	return ( psTCPCtrl );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void tcb_unlink_connection( TCPCtrl_s *psTCPCtrl )
{
	TCPCtrl_s **ppsTmp;
	bool bFound = false;

	LOCK( g_hConnListMutex );
	atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_ACTIVE );
	for ( ppsTmp = &g_psFirstTCP; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->tcb_psNext )
	{
		if ( *ppsTmp == psTCPCtrl )
		{
			*ppsTmp = psTCPCtrl->tcb_psNext;
			bFound = true;
			break;
		}
	}
	if ( bFound == false )
	{
		printk( "Error: tcb_unlink_connection() could not find socket in global list\n" );
	}

	if ( psTCPCtrl->tcb_nState == TCPS_LISTEN )
	{
		int nHash = tcp_hash_listen( psTCPCtrl->tcb_lport );

		bFound = false;
		for ( ppsTmp = &g_apsListenHash[nHash]; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->tcb_psNextHash )
		{
			if ( *ppsTmp == psTCPCtrl )
			{
				*ppsTmp = psTCPCtrl->tcb_psNextHash;
				bFound = true;
				break;
			}
		}
		if ( bFound == false )
		{
			printk( "Error: tcb_unlink_connection() could not find socket in listen hash\n" );
		}
	}
	else
	{
		int nHash = tcp_hash_full( *( ( uint32 * )psTCPCtrl->tcb_lip ), psTCPCtrl->tcb_lport, *( ( uint32 * )psTCPCtrl->tcb_rip ), psTCPCtrl->tcb_rport );

		bFound = false;
		for ( ppsTmp = &g_apsEstablishedHash[nHash]; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->tcb_psNextHash )
		{
			if ( *ppsTmp == psTCPCtrl )
			{
				*ppsTmp = psTCPCtrl->tcb_psNextHash;
				bFound = true;
				break;
			}
		}
//    if ( bFound == false ) {
//      printk( "Error: tcb_unlink_connection() could not find socket in established hash\n" );
//    }
	}
	UNLOCK( g_hConnListMutex );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void tcp_free_buffers( TCPCtrl_s *psTCPCtrl )
{
	if ( psTCPCtrl->tcb_sndbuf != NULL )
	{
		kfree( psTCPCtrl->tcb_sndbuf );
		psTCPCtrl->tcb_sndbuf = NULL;
	}
	if ( psTCPCtrl->tcb_rcvbuf != NULL )
	{
		kfree( psTCPCtrl->tcb_rcvbuf );
		psTCPCtrl->tcb_rcvbuf = NULL;
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int tcb_dealloc( TCPCtrl_s *psTCPCtrl, bool bForceUnlink )
{
	TCPCtrl_s **ppsTmp;

	LOCK( g_hConnListMutex );

//  printk( "Free tcp connection on port %d\n", psTCPCtrl->tcb_lport );
	tcp_killtimers( psTCPCtrl );

	tcp_free_buffers( psTCPCtrl );

	if ( psTCPCtrl->tcb_ocsem >= 0 )
	{
		delete_semaphore( psTCPCtrl->tcb_ocsem );
	}
	if ( psTCPCtrl->tcb_hListenSem >= 0 )
	{
		delete_semaphore( psTCPCtrl->tcb_hListenSem );
	}
	if ( psTCPCtrl->tcb_hRecvQueue >= 0 )
	{
		delete_semaphore( psTCPCtrl->tcb_hRecvQueue );
	}
	if ( psTCPCtrl->tcb_hSendQueue >= 0 )
	{
		delete_semaphore( psTCPCtrl->tcb_hSendQueue );
	}

	while ( psTCPCtrl->tcb_psFirstClient )
	{
		TCPCtrl_s *psClient = psTCPCtrl->tcb_psFirstClient;
		char zBuf[64];

		psTCPCtrl->tcb_psFirstClient = psClient->tcb_psNextClient;
		psClient->tcb_psParent = NULL;

		format_ipaddress( zBuf, psClient->tcb_rip );

		printk( "tcb_dealloc() delete client from %s:%d\n", zBuf, psClient->tcb_rport );
		tcb_dealloc( psClient, true );

		psTCPCtrl->tcb_nNumClients--;
	}

	if ( psTCPCtrl->tcb_psParent != NULL )	// Remove from parent (if any)
	{
		TCPCtrl_s *psParent = psTCPCtrl->tcb_psParent;

		// FIXME: Check if this could cause a deadlock
//    LOCK( psParent->tcb_hMutex );
		for ( ppsTmp = &psParent->tcb_psFirstClient; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->tcb_psNextClient )
		{
			if ( *ppsTmp == psTCPCtrl )
			{
				*ppsTmp = psTCPCtrl->tcb_psNextClient;
				psParent->tcb_nNumClients--;
				break;
			}
		}
//    UNLOCK( psParent->tcb_hMutex );
	}
	// Remove from global list.
	tcb_unlink_connection( psTCPCtrl );
	delete_semaphore( psTCPCtrl->tcb_hMutex );
	UNLOCK( g_hConnListMutex );
	kfree( psTCPCtrl );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Calculate the ISS for a connection
 * NOTE:
 *	The RFC says that the ISS should be incremented once about every
 *	fourth micro seccond and then add a small amount for each new
 *	connection. This function actually wiolate the standard
 *	by adding a "random" number to the ISS. This seems to be quit common
 *	to make it harder to hi-jack a connection.
 * SEE ALSO:
 ****************************************************************************/

static long tcp_iss( void )
{
	static long seq = 0;

#define	TCPINCR		904

	if ( seq == 0 )
		seq = ( int )( get_system_time() >> 2 );
	seq += read_pentium_clock() % 2000;	// TCPINCR;
	return seq;
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Alloc all buffers and semaphores, plus does some other initializing
 *	needed to make the connection usable.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int tcp_sync( TCPCtrl_s *psTCPCtrl )
{
	tcp_set_state( psTCPCtrl, TCPS_CLOSED );

	psTCPCtrl->tcb_iss = psTCPCtrl->tcb_nSndFirstUnacked = psTCPCtrl->tcb_nSndNext = tcp_iss();
	psTCPCtrl->tcb_lwack = psTCPCtrl->tcb_iss;

	psTCPCtrl->tcb_sndbuf = kmalloc( TCPSBS, MEMF_KERNEL );	// FIXME: Check for error
	psTCPCtrl->tcb_nSndBufSize = TCPSBS;
	psTCPCtrl->tcb_sbstart = psTCPCtrl->tcb_nSndCount = 0;

	psTCPCtrl->tcb_rcvbuf = kmalloc( TCPRBS, MEMF_KERNEL );	// FIXME: Check for error
	psTCPCtrl->tcb_rbsize = TCPRBS;
	psTCPCtrl->tcb_nRcvBufStart = psTCPCtrl->tcb_nRcvCount = 0;

	psTCPCtrl->tcb_hRecvQueue = create_semaphore( "tcp_recv_queue", 0, 0 );
	psTCPCtrl->tcb_hSendQueue = create_semaphore( "tcp_send_queue", 0, 0 );

	psTCPCtrl->tcb_ocsem = create_semaphore( "tcp_oc_sync", 0, 0 );

	/* timer stuff */

	psTCPCtrl->tcb_srt = 0;	/* in sec*1000000   */
	psTCPCtrl->tcb_rtde = 0;	/* in sec*1000000   */
	psTCPCtrl->tcb_rexmt = 1500000;	/* in sec*1000000   */
	psTCPCtrl->tcb_rexmtcount = 0;
	psTCPCtrl->tcb_keep = 120 * 1000000;	/* in sec*1000000   */

	psTCPCtrl->tcb_code = TCPF_SYN;
//  psTCPCtrl->tcb_flags = 0;
	return 0;
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Calculate window size information for a new connection
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_winit( TCPCtrl_s *psTCPCtrl, TCPCtrl_s *newptcb, Route_s *psRoute, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	int mss;

	newptcb->tcb_swindow = ntohs( psTcpHdr->tcp_window );
	newptcb->tcb_lwseq = ntohl( psTcpHdr->tcp_seq );

	newptcb->tcb_lwack = newptcb->tcb_iss;	/* set in tcpsync()     */

	/* mss = 536; *//* RFC 1122 */
	mss = psRoute->rt_psInterface->ni_nMTU;
	mss -= TCPMHLEN + sizeof( IpHeader_s );

	if ( psTCPCtrl->tcb_smss )
	{
		newptcb->tcb_smss = min( psTCPCtrl->tcb_smss, mss );
		psTCPCtrl->tcb_smss = 0;	/* reset server smss    */
	}
	else
	{
		newptcb->tcb_smss = mss;
	}

	newptcb->tcb_rmss = mss;	/* receive mss          */
	newptcb->tcb_cwnd = newptcb->tcb_smss;	/* 1 segment            */
	newptcb->tcb_ssthresh = 65535;	/* IP max window        */
	newptcb->tcb_nRcvNext = ntohl( psTcpHdr->tcp_seq );

	newptcb->tcb_nAdvertizedRcvWin = newptcb->tcb_nRcvNext + newptcb->tcb_rbsize;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Do receive window calculations
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int tcp_rwindow( TCPCtrl_s *psTCPCtrl, bool bSetAdvertizedSeq )
{
	int nWndSize;

	nWndSize = psTCPCtrl->tcb_rbsize - psTCPCtrl->tcb_nRcvCount;
	if ( psTCPCtrl->tcb_nState < TCPS_ESTABLISHED )
	{
		return ( nWndSize );
	}

	/*
	 * Receiver-Side Silly Window Syndrome Avoidance:
	 *  Never shrink an already-advertised window, but wait for at
	 *  least 1/2 receiver buffer and 1 max-sized segment before
	 *  opening a zero window.
	 */
	if ( nWndSize * 2 < psTCPCtrl->tcb_rbsize || nWndSize < psTCPCtrl->tcb_rmss )
	{
		nWndSize = 0;
	}
	// Make sure we newer shrink the window.
	if ( ( psTCPCtrl->tcb_nAdvertizedRcvWin - psTCPCtrl->tcb_nRcvNext ) > nWndSize )
	{
		nWndSize = ( int )( psTCPCtrl->tcb_nAdvertizedRcvWin - psTCPCtrl->tcb_nRcvNext );
	}
	if ( bSetAdvertizedSeq )
	{
		psTCPCtrl->tcb_nAdvertizedRcvWin = psTCPCtrl->tcb_nRcvNext + nWndSize;
		if ( nWndSize == 0 )
		{
			atomic_or( &psTCPCtrl->tcb_flags, TCBF_RWIN_CLOSED );
		}
	}
	return ( nWndSize );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Generate RESET for packets we don't like.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int __tcp_reset( PacketBuf_s *psPktIn, const char *pzCallerFunc, int nLineNum )
{
	PacketBuf_s *psPktOut;
	TCPHeader_s *psTcpHdr = psPktIn->pb_uTransportHdr.psTCP;
	IpHeader_s *psIpHdr = psPktIn->pb_uNetworkHdr.psIP;
	TCPHeader_s *psTCPOut;
	IpHeader_s *psIPOut;
	int datalen;

//  printk( "%s() reset connection %d\n", pzCallerFunc, nLineNum );

	if ( psTcpHdr->tcp_code & TCPF_RST )
	{
		return ( 0 );	/* no RESETs on RESETs */
	}
	psPktOut = alloc_pkt_buffer( 2048 );	// FIXME: Far to big
	if ( psPktOut == NULL )
	{
		return ( -ENOMEM );
	}
	psPktOut->pb_uNetworkHdr.pRaw = psPktOut->pb_pData + 16;
	psPktOut->pb_uTransportHdr.pRaw = psPktOut->pb_uNetworkHdr.pRaw + sizeof( IpHeader_s );

	psTCPOut = psPktOut->pb_uTransportHdr.psTCP;
	psIPOut = psPktOut->pb_uNetworkHdr.psIP;

	IP_COPYADDR( psIPOut->iph_nSrcAddr, psIpHdr->iph_nDstAddr );
	IP_COPYADDR( psIPOut->iph_nDstAddr, psIpHdr->iph_nSrcAddr );

	psIPOut->iph_nProtocol = IPT_TCP;
	psIPOut->iph_nHdrSize = 5;
	psIPOut->iph_nVersion = 4;

	psPktOut->pb_nSize = sizeof( IpHeader_s ) + sizeof( TCPHeader_s );
	psIPOut->iph_nPacketSize = htons( psPktOut->pb_nSize );

	psTCPOut->tcp_sport = psTcpHdr->tcp_dport;
	psTCPOut->tcp_dport = psTcpHdr->tcp_sport;

	if ( psTcpHdr->tcp_code & TCPF_ACK )
	{
		psTCPOut->tcp_seq = psTcpHdr->tcp_ack;

		psTCPOut->tcp_code = TCPF_RST;
	}
	else
	{
		psTCPOut->tcp_seq = 0;

		psTCPOut->tcp_code = TCPF_RST | TCPF_ACK;
	}
	datalen = ntohs( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr ) - TCP_HLEN( psTcpHdr );
	if ( psTcpHdr->tcp_code & TCPF_FIN )
		datalen++;
	psTCPOut->tcp_ack = htonl( ntohl( psTcpHdr->tcp_seq ) + datalen );

	psTCPOut->tcp_offset = TCPHOFFSET;
	psTCPOut->tcp_window = psTCPOut->tcp_urgptr = 0;

	psTCPOut->tcp_cksum = 0;
	psTCPOut->tcp_cksum = tcp_cksum( psPktOut );

	g_nTcpOutSegs++;
	return ( ip_send( psPktOut ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Generate ACK for a received packet
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_ackit( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPktIn )
{
	PacketBuf_s *psPktOut;
	TCPHeader_s *psTCPIn = psPktIn->pb_uTransportHdr.psTCP;
	IpHeader_s *psIPIn = psPktIn->pb_uNetworkHdr.psIP;
	TCPHeader_s *psTCPOut;
	IpHeader_s *psIPOut;
	int nDataSize = ntohs( psIPIn->iph_nPacketSize ) - ( IP_GET_HDR_LEN( psIPIn ) + TCP_HLEN( psTCPIn ) );
	tcpseq nPacketSeq = ntohl( psTCPIn->tcp_seq );

	if ( ( psTCPIn->tcp_code & TCPF_RST ) || nDataSize < 0 )
	{
		return ( 0 );
	}

/*    
      if ( nDataSize == 0 && nPacketSeq == (psTCPCtrl->tcb_nRcvNext-1) )
      if ( (nDataSize == 0 && (psTCPIn->tcp_code & (TCPF_SYN|TCPF_FIN)) == 0) && nPacketSeq != (psTCPCtrl->tcb_nRcvNext-1) ) {
      return( 0 );	// Duplicate ACK
      }*/
	if ( nDataSize > 0 || ( psTCPIn->tcp_code & ( TCPF_SYN | TCPF_FIN ) ) || ( nDataSize <= 1 && nPacketSeq == ( psTCPCtrl->tcb_nRcvNext - 1 ) ) )
	{

		psPktOut = alloc_pkt_buffer( 16 + sizeof( IpHeader_s ) + sizeof( TCPHeader_s ) );
		if ( psPktOut == NULL )
		{
			return ( -ENOMEM );
		}
		psPktOut->pb_uNetworkHdr.pRaw = psPktOut->pb_pData + 16;
		psPktOut->pb_uTransportHdr.pRaw = psPktOut->pb_uNetworkHdr.pRaw + sizeof( IpHeader_s );

		psTCPOut = psPktOut->pb_uTransportHdr.psTCP;
		psIPOut = psPktOut->pb_uNetworkHdr.psIP;

		IP_COPYADDR( psIPOut->iph_nSrcAddr, psIPIn->iph_nDstAddr );
		IP_COPYADDR( psIPOut->iph_nDstAddr, psIPIn->iph_nSrcAddr );
		psIPOut->iph_nProtocol = IPT_TCP;
		psIPOut->iph_nHdrSize = 5;
		psIPOut->iph_nVersion = 4;

		psPktOut->pb_nSize = IP_GET_HDR_LEN( psIPOut ) + sizeof( TCPHeader_s );
		psIPOut->iph_nPacketSize = htons( psPktOut->pb_nSize );

		psTCPOut->tcp_sport = psTCPIn->tcp_dport;
		psTCPOut->tcp_dport = psTCPIn->tcp_sport;
		psTCPOut->tcp_seq = htonl( psTCPCtrl->tcb_nSndNext );

		psTCPOut->tcp_ack = htonl( psTCPCtrl->tcb_nRcvNext );
		psTCPOut->tcp_code = TCPF_ACK;
		psTCPOut->tcp_offset = TCPHOFFSET;
		psTCPOut->tcp_window = htons( tcp_rwindow( psTCPCtrl, true ) );
		psTCPOut->tcp_urgptr = 0;
		psTCPOut->tcp_cksum = 0;
		psTCPOut->tcp_cksum = tcp_cksum( psPktOut );

		g_nTcpOutSegs++;

		return ( ip_send( psPktOut ) );
	}
	else
	{
		return ( 0 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Handle urgent data.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

#if _DONE_
static int tcp_rcvurg( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	struct uqe *puqe;
	int datalen, offset;

	datalen = psIpHdr->iph_nPacketSize - IP_HLEN( psIpHdr ) - TCP_HLEN( psTcpHdr );
	/*
	 * Berkeley machines treat the urgent pointer as "last+1", but
	 * RFC 1122 says just "last." Defining "BSDURG" causes Berkeley
	 * semantics.
	 */
#ifdef	BSDURG
	psTcpHdr->tcp_urgptr -= 1;
#endif /* BSDURG */
	if ( psTcpHdr->tcp_urgptr >= datalen || psTcpHdr->tcp_urgptr >= TCPMAXURG )
		psTcpHdr->tcp_urgptr = datalen - 1;	/* quietly fix it       */
	puqe = uqalloc();
	if ( puqe == ( struct uqe * )SYSERR )
		return SYSERR;	/* out of buffer space!!!       */

	puqe->uq_seq = psTcpHdr->tcp_seq;

	puqe->uq_len = psTcpHdr->tcp_urgptr + 1;
	puqe->uq_data = ( char * )getbuf( Net.netpool );

	if ( puqe->uq_data == ( char * )SYSERR )
	{
		puqe->uq_data = 0;
		uqfree( puqe );
		return SYSERR;
	}
	blkcopy( puqe->uq_data, &psIpHdr->ip_data[TCP_HLEN( psTcpHdr )], puqe->uq_len );

	if ( psTCPCtrl->tcb_rudq < 0 )
	{
		psTCPCtrl->tcb_rudseq = psTCPCtrl->tcb_nRcvNext;
		psTCPCtrl->tcb_rudq = newq( TCPUQLEN, QF_WAIT );
		if ( psTCPCtrl->tcb_rudq < 0 )
		{
			uqfree( puqe );
			return SYSERR;	/* treat it like normal data    */
		}
	}

	if ( datalen > puqe->uq_len )
	{
		/* some non-urgent data left, edit the packet         */

		psTcpHdr->tcp_seq +=puqe->uq_len;

		offset = TCP_HLEN( psTcpHdr ) + puqe->uq_len;
		blkcopy( &psIpHdr->ip_data[TCP_HLEN( psTcpHdr )], &psIpHdr->ip_data[offset], datalen - puqe->uq_len );
		psTcpHdr->tcp_urgptr = 0;
		psIpHdr->ip_len -= puqe->uq_len;
		/* checksums already checked, so no need to redo them */
	}
	if ( enq( psTCPCtrl->tcb_rudq, puqe, psTCPCtrl->tcb_rudseq - puqe->uq_seq ) < 0 )
	{
		uqfree( puqe );
		return SYSERR;
	}
	return OK;
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 *	Figure out if we can accept the received segment
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_ok( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	int nPkgDataLen;
	int nRcvWindow;
	long wlast, slast;
	tcpseq rv;
	tcpseq nPacketSeq = ntohl( psTcpHdr->tcp_seq );

	if ( psTCPCtrl->tcb_nState < TCPS_SYNRCVD )
	{
		return ( true );
	}

	nPkgDataLen = ntohs( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr ) - TCP_HLEN( psTcpHdr );
//  printk( "Received %d bytes\n", nPkgDataLen );
	/* add SYN and FIN */
	if ( psTcpHdr->tcp_code & TCPF_SYN )
	{
		++nPkgDataLen;
	}
	if ( psTcpHdr->tcp_code & TCPF_FIN )
	{
		++nPkgDataLen;
	}

	nRcvWindow = psTCPCtrl->tcb_rbsize - psTCPCtrl->tcb_nRcvCount;

	if ( nRcvWindow == 0 && nPkgDataLen == 0 )
	{
		return ( nPacketSeq == psTCPCtrl->tcb_nRcvNext );
	}
	if ( psTcpHdr->tcp_code & TCPF_URG )
	{
		printk( "Warning: tcp_ok() don't know how to handle urgent data (just got some)\n" );
	}
	wlast = psTCPCtrl->tcb_nRcvNext + nRcvWindow - 1;
	rv = ( nPacketSeq - psTCPCtrl->tcb_nRcvNext ) >= 0 && ( nPacketSeq - wlast ) <= 0;

	if ( nPkgDataLen == 0 )
	{
		return ( rv );
	}
	slast = ( ( tcpseq )ntohl( psTcpHdr->tcp_seq ) ) + nPkgDataLen - 1;

	rv |= ( slast - psTCPCtrl->tcb_nRcvNext ) >= 0 && ( slast - wlast ) <= 0;

	/* If no window, strip data but keep ACK, RST and URG */
	if ( nRcvWindow == 0 )
	{
		psIpHdr->iph_nPacketSize = htons( IP_GET_HDR_LEN( psIpHdr ) + TCP_HLEN( psTcpHdr ) );
	}
	return ( rv );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Get the sender MSS from the TCP options of an incoming packet.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_smss( TCPCtrl_s *psTCPCtrl, TCPHeader_s *psTcpHdr, char *pOption )
{
	unsigned mss, len;

	pOption++;		// Skip type
	len = *pOption++;

	if ( ( psTcpHdr->tcp_code & TCPF_SYN ) == 0 )
	{
		return ( ( len > 0 ) ? len : 1 );	// Avoid infinit loops
	}

	switch ( len - 2 )
	{			/* subtract kind & len  */
	case sizeof( char ):
		mss = *pOption;
		break;
	case sizeof( uint16 ):
		mss = ntohs( *( uint16 * )pOption );
		break;
	case sizeof( uint32 ):
		mss = ntohl( *( uint32 * )pOption );
		break;
	default:
		printk( "Invalid size %d of TPO_MSS TCP option\n", len );
		mss = psTCPCtrl->tcb_smss;
		break;
	}
//    mss -= TCPMHLEN;  /* save just the data buffer size */
//  printk( "Maximum seg-size = %d (%d)\n", mss, len );
	if ( psTCPCtrl->tcb_smss )
	{
		psTCPCtrl->tcb_smss = min( mss, psTCPCtrl->tcb_smss );
	}
	else
	{
		psTCPCtrl->tcb_smss = mss;
	}
	return ( ( len > 0 ) ? len : 1 );	// Avoid infinit loops
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Handle TCP options.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_opts( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	char *pOption;
	char *pOptEnd;
	int len;

	if ( TCP_HLEN( psTcpHdr ) == TCPMHLEN )
	{
		return ( 0 );
	}
	pOption = ( char * )( psTcpHdr + 1 );
	pOptEnd = pOption + TCP_HLEN( psTcpHdr );
//  printk( "Got %d bytes of TCP options (%d)\n", pOptEnd - pOption, TCP_HLEN(psTcpHdr) );

	do
	{
		switch ( *pOption )
		{
		case TPO_NOOP:
			pOption++;
			/* fall through */
		case TPO_EOOL:
			break;
		case TPO_MSS:
			pOption += tcp_smss( psTCPCtrl, psTcpHdr, pOption );
			break;
		default:
//      printk( "Invalide TCP option %d\n", *pOption );
			pOption = pOptEnd;
			break;
		}
	}
	while ( *pOption != TPO_EOOL && pOption < pOptEnd );

	/* delete the options */
	len = ntohs( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr ) - TCP_HLEN( psTcpHdr );
	if ( len > 0 )
	{
		memmove( psTcpHdr + 1, ( ( char * )( psIpHdr + 1 ) ) + TCP_HLEN( psTcpHdr ), len );
	}
	psIpHdr->iph_nPacketSize = htons( IP_GET_HDR_LEN( psIpHdr ) + sizeof( TCPHeader_s ) + len );
	psTcpHdr->tcp_offset = TCPHOFFSET;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void tcp_insert_fragment( TCPCtrl_s *psTCPCtrl, TCPFragment_s *psFragment )
{
	TCPFragment_s **ppsTmp;

//  printk( "tcp_insert_fragment() tcb=%08x frg=%08x\n", psTCPCtrl, psFragment );
	for ( ppsTmp = &psTCPCtrl->tcb_psFirstFragment; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->tf_psNext )
	{
//    printk( "%08x -> %d/%d\n", *ppsTmp, (*ppsTmp)->tf_seq, psFragment->tf_seq );
//    if ( (*ppsTmp)->tf_seq > psFragment->tf_seq ) {
		if ( SEQCMP( ( *ppsTmp )->tf_seq, psFragment->tf_seq ) > 0 )
		{
			psFragment->tf_psNext = *ppsTmp;
			*ppsTmp = psFragment;
			return;
		}
	}
//  printk( "Add to end\n" );
	*ppsTmp = psFragment;
	psFragment->tf_psNext = NULL;
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Glue TCP segments into a contigious stream.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tfcoalesce( TCPCtrl_s *psTCPCtrl, int datalen, TCPHeader_s *psTcpHdr )
{
	TCPFragment_s *tf;
	int new;

	psTCPCtrl->tcb_nRcvNext += datalen;
	psTCPCtrl->tcb_nRcvCount += datalen;

	// CHECKME: Cant this happen if the sequence wrap?
	if ( psTCPCtrl->tcb_nRcvNext == psTCPCtrl->tcb_finseq && ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_FIN_RECVD ) )
	{
		goto alldone;
	}

	if ( ( psTCPCtrl->tcb_nRcvNext - psTCPCtrl->tcb_pushseq ) >= 0 )
	{
		psTcpHdr->tcp_code |= TCPF_PSH;
		psTCPCtrl->tcb_pushseq = 0;
	}

	if ( psTCPCtrl->tcb_psFirstFragment == NULL )
	{
		return ( 0 );
	}

	tf = psTCPCtrl->tcb_psFirstFragment;
	psTCPCtrl->tcb_psFirstFragment = tf->tf_psNext;

	while ( ( tf->tf_seq - psTCPCtrl->tcb_nRcvNext ) <= 0 )
	{
		new = tf->tf_len - ( psTCPCtrl->tcb_nRcvNext - tf->tf_seq );
		if ( new > 0 )
		{
			psTCPCtrl->tcb_nRcvNext += new;
			psTCPCtrl->tcb_nRcvCount += new;
		}
		if ( psTCPCtrl->tcb_nRcvNext == psTCPCtrl->tcb_finseq && ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_FIN_RECVD ) )
		{
			goto alldone;
		}
		if ( ( psTCPCtrl->tcb_nRcvNext - psTCPCtrl->tcb_pushseq ) >= 0 )
		{
			psTcpHdr->tcp_code |= TCPF_PSH;
			psTCPCtrl->tcb_pushseq = 0;
		}
		kfree( tf );

		if ( psTCPCtrl->tcb_psFirstFragment == NULL )
		{
			return ( 0 );
		}
		tf = psTCPCtrl->tcb_psFirstFragment;
		psTCPCtrl->tcb_psFirstFragment = tf->tf_psNext;
	}
	tcp_insert_fragment( psTCPCtrl, tf );	/* got one too many   */
	return ( 0 );

      alldone:
	while ( psTCPCtrl->tcb_psFirstFragment != NULL )
	{
		tf = psTCPCtrl->tcb_psFirstFragment;
		psTCPCtrl->tcb_psFirstFragment = tf->tf_psNext;
		kfree( tf );
	}
	psTcpHdr->tcp_code |= TCPF_FIN;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Add a new TCP segment to the queue
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tfinsert( TCPCtrl_s *psTCPCtrl, tcpseq seq, int datalen )
{
	TCPFragment_s *tf;

	if ( datalen == 0 )
		return ( 0 );

	tf = kmalloc( sizeof( TCPFragment_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( tf == NULL )
	{
		return ( -ENOMEM );
	}

	tf->tf_seq = seq;
	tf->tf_len = datalen;

	tcp_insert_fragment( psTCPCtrl, tf );

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Wake up threads that wait for sending/receiving data or waiting for
 *	incoming connections in accept()
 * NOTE:
 *	Called with the tcb_hMutex locked.
 * SEE ALSO:
 ****************************************************************************/

int tcp_wakeup( int type, TCPCtrl_s *psTCPCtrl )
{
	int freelen;

	if ( type & READERS )
	{
		SelectRequest_s *psReq;

//    if ( (psTCPCtrl->tcb_flags & TCBF_RDONE) || psTCPCtrl->tcb_nRcvCount > 0 /*|| psTCPCtrl->tcb_rudq >= 0*/ ) {
		// Wake up guy's stuck in recvmsg()
		if ( psTCPCtrl->tcb_hRecvQueue >= 0 )
		{
			wakeup_sem( psTCPCtrl->tcb_hRecvQueue, false );
		}
		// Wake up guy's stuck in select()
		for ( psReq = psTCPCtrl->tcb_psFirstReadSelReq; NULL != psReq; psReq = psReq->sr_psNext )
		{
			psReq->sr_bReady = true;
			UNLOCK( psReq->sr_hSema );
		}
//    }
	}
	if ( type & WRITERS )
	{
		SelectRequest_s *psReq;

		freelen = psTCPCtrl->tcb_nSndBufSize - psTCPCtrl->tcb_nSndCount;

		if ( ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_SDONE ) || psTCPCtrl->tcb_nSndCount < psTCPCtrl->tcb_nSndBufSize )
		{
			// Wake up guy's stuck in sendmsg()
			if ( psTCPCtrl->tcb_hSendQueue >= 0 )
			{
				wakeup_sem( psTCPCtrl->tcb_hSendQueue, false );
			}

			// Wake up guy's stuck in select()
			for ( psReq = psTCPCtrl->tcb_psFirstWriteSelReq; NULL != psReq; psReq = psReq->sr_psNext )
			{
				psReq->sr_bReady = true;
				UNLOCK( psReq->sr_hSema );
			}
		}
		// No point leaving those guys stuck with a broken socket.
		if ( psTCPCtrl->tcb_nError && psTCPCtrl->tcb_ocsem > 0 )
		{
			UNLOCK( psTCPCtrl->tcb_ocsem );
		}
	}
	if ( type & TCP_LISTENERS )
	{
		SelectRequest_s *psReq;

		// Wake up server from accept()
		wakeup_sem( psTCPCtrl->tcb_hListenSem, false );
		// Wake up server from select()
		for ( psReq = psTCPCtrl->tcb_psFirstReadSelReq; NULL != psReq; psReq = psReq->sr_psNext )
		{
			psReq->sr_bReady = true;
//      printk( "tcp_wakeup() release LISTEN select request\n" );
			UNLOCK( psReq->sr_hSema );
		}
	}
//  Schedule();
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool __tcp_abort( TCPCtrl_s *psTCPCtrl, int error, const char *pzCallerFunc, int nLineNum )
{
//  if ( error != 0 ) {
//    printk( "%s() Abort TCP connection %d (port=%d, sem=%d, err=%d)\n", pzCallerFunc, nLineNum, psTCPCtrl->tcb_lport, psTCPCtrl->tcb_hMutex, error );
//  }

	kassertw( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_BUSY );

	tcp_killtimers( psTCPCtrl );

	if ( ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_OPEN ) == 0 )
	{
//    printk( "tcp_abort() delete dead connection\n" );
		tcb_dealloc( psTCPCtrl, true );
		return ( true );
	}
	else
	{
		atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_ACTIVE );
//    tcb_unlink_connection( psTCPCtrl );
		atomic_or( &psTCPCtrl->tcb_flags, TCBF_RDONE | TCBF_SDONE );	// We wont send or accept any more data on this connection.
		psTCPCtrl->tcb_nError = error;
		tcp_wakeup( READERS | WRITERS, psTCPCtrl );

		tcp_free_buffers( psTCPCtrl );
		atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_BUSY );
		UNLOCK( psTCPCtrl->tcb_hMutex );
		return ( false );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Trig a send event
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int tcp_kick( TCPCtrl_s *psTCPCtrl )
{
	if ( ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_DELACK ) && tcp_event_time_left( psTCPCtrl, SEND ) == 0 )
	{
		tcp_create_event( psTCPCtrl, SEND, TCP_ACKDELAY, true );
	}
	else
	{
		tcp_create_event( psTCPCtrl, SEND, 0LL, true );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Process the payload of a packet.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_dodat( TCPCtrl_s *psTCPCtrl, TCPHeader_s *psTcpHdr, tcpseq first, int datalen )
{
	int wakeup = 0;

	if ( psTCPCtrl->tcb_nRcvNext == first )
	{
		if ( datalen > 0 )
		{
			tfcoalesce( psTCPCtrl, datalen, psTcpHdr );
			atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
			wakeup++;
		}
		if ( psTcpHdr->tcp_code & TCPF_FIN )
		{
			atomic_or( &psTCPCtrl->tcb_flags, TCBF_FIN_RECVD | TCBF_RDONE | TCBF_NEEDOUT );	// All data received (and acked) from sender
			psTCPCtrl->tcb_nRcvNext++;
			wakeup++;
		}
		if ( psTcpHdr->tcp_code & TCPF_PSH )
		{
			atomic_or( &psTCPCtrl->tcb_flags, TCBF_PUSH );
			wakeup++;
		}

		if ( wakeup )
		{
			tcp_wakeup( READERS, psTCPCtrl );
		}
	}
	else
	{
		/* process delayed controls */
		if ( psTcpHdr->tcp_code & TCPF_FIN )
		{
			psTCPCtrl->tcb_finseq = ( ( tcpseq )ntohl( psTcpHdr->tcp_seq ) ) + datalen;

			atomic_or( &psTCPCtrl->tcb_flags, TCBF_FIN_RECVD );
		}
		if ( psTcpHdr->tcp_code & TCPF_PSH )
		{
			psTCPCtrl->tcb_pushseq = ( ( tcpseq )ntohl( psTcpHdr->tcp_seq ) ) + datalen;
		}
		psTcpHdr->tcp_code &= ~( TCPF_FIN | TCPF_PSH );
		tfinsert( psTCPCtrl, first, datalen );
	}
//  tcp_wakeup(READERS, psTCPCtrl );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Find the size of and process the payload of a incoming packet
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_data( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	tcpseq first, last, wlast;
	int nDataLen;
	int rwindow;
	int pp;
	int pb;

	if ( psTcpHdr->tcp_code & TCPF_SYN )
	{
		psTCPCtrl->tcb_nRcvNext++;
		atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
		// little-endian sucks :)
		psTcpHdr->tcp_seq = htonl( ( ( tcpseq )ntohl( psTcpHdr->tcp_seq ) ) + 1 );
	}

	nDataLen = ntohs( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr ) - TCP_HLEN( psTcpHdr );
	rwindow = psTCPCtrl->tcb_rbsize - psTCPCtrl->tcb_nRcvCount;
	wlast = psTCPCtrl->tcb_nRcvNext + rwindow - 1;
	first = ntohl( psTcpHdr->tcp_seq );

	last = first + nDataLen - 1;


	if ( SEQCMP( psTCPCtrl->tcb_nRcvNext, first ) > 0 )
	{
		nDataLen -= psTCPCtrl->tcb_nRcvNext - first;
		first = psTCPCtrl->tcb_nRcvNext;
	}

	if ( SEQCMP( last, wlast ) > 0 )
	{
		nDataLen -= last - wlast;
		psTcpHdr->tcp_code &= ~TCPF_FIN;	/* cutting it off */
	}
	if ( nDataLen < 0 )
	{
		printk( "Error: tcp_data() got %d bytes of data\n", nDataLen );
		return ( -EINVAL );
	}
	pb = psTCPCtrl->tcb_nRcvBufStart + psTCPCtrl->tcb_nRcvCount;	/* == rnext, in buf */
	pb += first - psTCPCtrl->tcb_nRcvNext;	/* distance in buf       */
	pb %= psTCPCtrl->tcb_rbsize;	/* may wrap              */
	pp = first - ( ( tcpseq )ntohl( psTcpHdr->tcp_seq ) );	/* distance in packet    */

	if ( psTCPCtrl->tcb_rcvbuf != NULL )
	{
		if ( pb + nDataLen > psTCPCtrl->tcb_rbsize )
		{
			int nFirstSeg = psTCPCtrl->tcb_rbsize - pb;

			memcpy( psTCPCtrl->tcb_rcvbuf + pb, ( ( char * )( psTcpHdr + 1 ) ) + pp, nFirstSeg );
			memcpy( psTCPCtrl->tcb_rcvbuf, ( ( char * )( psTcpHdr + 1 ) ) + pp + nFirstSeg, nDataLen - nFirstSeg );
		}
		else
		{
			memcpy( psTCPCtrl->tcb_rcvbuf + pb, ( ( char * )( psTcpHdr + 1 ) ) + pp, nDataLen );
		}
	}
	tcp_dodat( psTCPCtrl, psTcpHdr, first, nDataLen );	/* deal with it         */
	if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_NEEDOUT )
	{
		tcp_kick( psTCPCtrl );
	}
	else if ( nDataLen > 0 )
	{
		atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
		tcp_kick( psTCPCtrl );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Calculate the round trip time, and congestion window size.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_rtt( TCPCtrl_s *psTCPCtrl )
{
	bigtime_t rrt;		/* raw round trip               */
	bigtime_t delta;	/* deviation from smoothed      */

//  printk( "tcp_rtt() delete RETRANSMIT event\n" );
	rrt = tcp_delete_event( psTCPCtrl, RETRANSMIT );

	rrt = get_system_time() - psTCPCtrl->tcb_nRTTStart;
//    printk( "rrt = %dmS     (%d)\n", (int)(rrt/1000), psTCPCtrl->tcb_ostate );

	if ( rrt >= 0 && psTCPCtrl->tcb_ostate != TCPO_REXMT )
	{
//    static int nRun = 0;

		if ( psTCPCtrl->tcb_srt == 0 )
			psTCPCtrl->tcb_srt = rrt << 4;	/* prime the pump */
		/*
		 * "srt" is scaled X 8 here, so this is really:
		 *    delta = rrt - srt
		 */
		delta = rrt - ( psTCPCtrl->tcb_srt >> 4 );
		psTCPCtrl->tcb_srt += delta;	/* srt = srt + delta/8  */
		if ( delta < 0 )
			delta = -delta;
		/*
		 * "rtde" is scaled X 4 here, so this is really:
		 *    rtde = rtde + (|delta| - rtde)/4
		 */
		psTCPCtrl->tcb_rtde += delta - ( psTCPCtrl->tcb_rtde >> 2 );
		/*
		 * "srt" is scaled X 8, rtde scaled X 4, so this is:
		 *    rto = 2*(srt + rtde)
		 */

		psTCPCtrl->tcb_rexmt = ( ( ( psTCPCtrl->tcb_srt >> 3 ) + ( psTCPCtrl->tcb_rtde ) ) );

		psTCPCtrl->tcb_rexmtcount = 0;
//      printk( "smoothed rt = %dmS current rt = %dmS rx = %dmS\n",
//              (int)(psTCPCtrl->tcb_srt / 1000), (int)(rrt/1000), (int)(psTCPCtrl->tcb_rexmt/1000) );

	}
	if ( psTCPCtrl->tcb_rexmt < TCP_MINRXT )
		psTCPCtrl->tcb_rexmt = TCP_MINRXT;
	if ( psTCPCtrl->tcb_cwnd < psTCPCtrl->tcb_ssthresh )
	{
		psTCPCtrl->tcb_cwnd += psTCPCtrl->tcb_smss;
	}
	else
	{
		psTCPCtrl->tcb_cwnd += ( psTCPCtrl->tcb_smss * psTCPCtrl->tcb_smss ) / psTCPCtrl->tcb_cwnd;
	}
//      printk( "cwnd=%d\n", psTCPCtrl->tcb_cwnd );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Set the state of the output finit state engine.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_ostate( TCPCtrl_s *psTCPCtrl )
{
	if ( psTCPCtrl->tcb_ostate == TCPO_REXMT )
	{
//      psTCPCtrl->tcb_rexmtcount = 0;
		tcp_set_output_state( psTCPCtrl, TCPO_XMIT );
		tcp_delete_event( psTCPCtrl, RETRANSMIT );
//      printk( "tcp_ostate() tcp outputstate changed to TCPO_XMIT\n" );
	}
	else if ( psTCPCtrl->tcb_ostate == TCPO_PERSIST )
	{
		psTCPCtrl->tcb_rexmtcount = 0;
	}
	if ( psTCPCtrl->tcb_nSndCount == 0 && ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_SNDFIN ) == 0 )
	{
		tcp_set_output_state( psTCPCtrl, TCPO_IDLE );
//    printk( "tcp_ostate() tcp outputstate changed to TCPO_IDLE\n" );
		return ( 0 );
	}
	tcp_kick( psTCPCtrl );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Process incoming ACK's and do round trip calculations.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_acked( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	int nCtrlAcked;
	int32 nAcked;

	if ( !( psTcpHdr->tcp_code & TCPF_ACK ) )
	{
		return ( -EINVAL );
	}

	nAcked = ( ( tcpseq )ntohl( psTcpHdr->tcp_ack ) ) - psTCPCtrl->tcb_nSndFirstUnacked;
	nCtrlAcked = 0;

//    printk( "%d bytes acked\n", nAcked );

	if ( nAcked <= 0 )
	{
//    printk( "tcp_acked() received duplicate ACK ack=%u suna=%u\n",
//          ntohl( psTcpHdr->tcp_ack ), psTCPCtrl->tcb_nSndFirstUnacked );
		return ( 0 );	// Duplicate ACK
	}
	if ( SEQCMP( ( ( tcpseq )ntohl( psTcpHdr->tcp_ack ) ), psTCPCtrl->tcb_nSndNext ) > 0 )
	{
		if ( psTCPCtrl->tcb_nState == TCPS_SYNRCVD )
		{
			return tcp_reset( psPkt );
		}
		else
		{
			return tcp_ackit( psTCPCtrl, psPkt );
		}
	}
	tcp_rtt( psTCPCtrl );

	psTCPCtrl->tcb_nSndFirstUnacked = ntohl( psTcpHdr->tcp_ack );

//  printk( "New suna = %d\n", psTCPCtrl->tcb_nSndFirstUnacked );

	if ( nAcked > 0 && ( psTCPCtrl->tcb_code & TCPF_SYN ) )
	{
		nAcked--;
		nCtrlAcked++;
		psTCPCtrl->tcb_code &= ~TCPF_SYN;
		atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_FIRSTSEND );
	}
	if ( ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_SNDFIN ) && ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_FINSENDT ) && SEQCMP( ( ( tcpseq )ntohl( psTcpHdr->tcp_ack ) ), psTCPCtrl->tcb_nSndNext ) == 0 )
	{
		nAcked--;
		nCtrlAcked++;
//    psTCPCtrl->tcb_code &= ~TCPF_FIN;
		atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_SNDFIN );
//    printk( "tcp_acked() Clear TCBF_SNDFIN flag\n" );
	}

	psTCPCtrl->tcb_sbstart = ( psTCPCtrl->tcb_sbstart + nAcked ) % psTCPCtrl->tcb_nSndBufSize;
	psTCPCtrl->tcb_nSndCount -= nAcked;

	if ( nAcked > 0 )
	{
//    printk( "Acked %d\n", nAcked );
		tcp_wakeup( WRITERS, psTCPCtrl );
	}
	if ( nAcked + nCtrlAcked > 0 )
	{
		tcp_ostate( psTCPCtrl );
	}
//  printk( "tcp_acked() new ouptut state = %d\n", psTCPCtrl->tcb_ostate );
	return ( nAcked );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Calculate send window size from information in incoming packet.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_swindow( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	tcpseq wlast, owlast;

	if ( SEQCMP( ( ( tcpseq )ntohl( psTcpHdr->tcp_seq ) ), psTCPCtrl->tcb_lwseq ) < 0 )
		  return ( 0 );
	if ( SEQCMP( ( ( tcpseq )ntohl( psTcpHdr->tcp_seq ) ), psTCPCtrl->tcb_lwseq ) == 0 && SEQCMP( ( ( tcpseq )ntohl( psTcpHdr->tcp_ack ) ), psTCPCtrl->tcb_lwack ) < 0 )
		return ( 0 );

	/* else, we have a send window update */

	/* compute the last sequences of the new and old windows */

	owlast = psTCPCtrl->tcb_lwack + psTCPCtrl->tcb_swindow;
	wlast = ( ( tcpseq )ntohl( psTcpHdr->tcp_ack ) ) + ntohs( psTcpHdr->tcp_window );

	psTCPCtrl->tcb_swindow = ntohs( psTcpHdr->tcp_window );
	psTCPCtrl->tcb_lwseq = ntohl( psTcpHdr->tcp_seq );

	psTCPCtrl->tcb_lwack = ntohl( psTcpHdr->tcp_ack );
	if ( SEQCMP( wlast, owlast ) <= 0 )
		return ( 0 );
	/* else,  window increased */
	if ( psTCPCtrl->tcb_ostate == TCPO_PERSIST )
	{
		tcp_delete_event( psTCPCtrl, PERSIST );
		tcp_set_output_state( psTCPCtrl, TCPO_XMIT );
//    printk( "tcp_swindow() tcp outputstate changed to TCPO_XMIT\n" );
	}
	tcp_kick( psTCPCtrl );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Start the 2MSL delete event timer.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int tcp_wait( TCPCtrl_s *psTCPCtrl )
{
	tcp_killtimers( psTCPCtrl );
	tcp_create_event( psTCPCtrl, DELETE, TCP_TWOMSL, true );
	return ( 0 );
}



/*****************************************************************************
 * NAME:
 * DESC:
 *	Handle incoming packets while in TCPS_LISTEN state.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcps_listen_state( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	TCPCtrl_s *psNewTCB;
	Route_s *psRoute;
	int nError;

	psRoute = ip_find_route( psIpHdr->iph_nSrcAddr );

	if ( psRoute == NULL )
	{
		printk( "tcps_listen_state() could not route return address\n" );
		// Don't bother sending reset, we won't be able to route it any-way
		g_nTcpAttemptFails++;
		return ( false );
	}

	if ( ( psTcpHdr->tcp_code & TCPF_ACK ) || ( psTcpHdr->tcp_code & TCPF_SYN ) == 0 )
	{
		tcp_reset( psPkt );
		goto error1;
	}

	if ( atomic_read( &g_sSysBase.ex_nFreePageCount ) < 1024 * 128 / PAGE_SIZE )
	{
		printk( "Error: tcps_listen_state() connection refused, to low on memory (%ld)\n", atomic_read( &g_sSysBase.ex_nFreePageCount ) * PAGE_SIZE );
		tcp_reset( psPkt );
		goto error1;
	}

	if ( psTCPCtrl->tcb_nNumClients >= psTCPCtrl->tcb_nListenQueueSize )
	{
//    printk( "Error: tcps_listen_state() connection refused, to many clients (%d)\n", psTCPCtrl->tcb_nNumClients );
		tcp_reset( psPkt );
		g_nTcpAttemptFails++;
		goto error1;
	}
	LOCK( g_hConnListMutex );
	psNewTCB = tcb_alloc();

	if ( psNewTCB == NULL )
	{
		goto error2;
	}
//  LOCK( psNewTCB->tcb_hMutex );
	nError = tcp_sync( psNewTCB );

	if ( nError < 0 )
	{
//    UNLOCK( psNewTCB->tcb_hMutex );
		goto error2;	// FIXME: delete it 
	}
#ifdef TCP_NOTIFY_STATE_CHANGE
	printk( "tcps_listen_state() state of new connection set to TCPS_SYNRCVD\n" );
#endif
	psNewTCB->tcb_nState = TCPS_SYNRCVD;
	psNewTCB->tcb_ostate = TCPO_IDLE;
//  printk( "tcps_listen_state() tcp outputstate changed to TCPO_IDLE\n" );
	psNewTCB->tcb_nError = 0;


	IP_COPYADDR( psNewTCB->tcb_rip, psIpHdr->iph_nSrcAddr );
	psNewTCB->tcb_rport = htons( psTcpHdr->tcp_sport );
	IP_COPYADDR( psNewTCB->tcb_lip, psIpHdr->iph_nDstAddr );
	psNewTCB->tcb_lport = htons( psTcpHdr->tcp_dport );

	// Add it to the parent's listen queue
	psNewTCB->tcb_psParent = psTCPCtrl;	/* for ACCEPT   */
	psNewTCB->tcb_psNextClient = psTCPCtrl->tcb_psFirstClient;
	psTCPCtrl->tcb_psFirstClient = psNewTCB;
	psTCPCtrl->tcb_nNumClients++;


	tcp_winit( psTCPCtrl, psNewTCB, psRoute, psPkt );	/* initialize window data       */

	psNewTCB->tcb_finseq = psNewTCB->tcb_pushseq = 0;
	atomic_or( &psNewTCB->tcb_flags, TCBF_NEEDOUT );
	g_nTcpPassiveOpens++;
	psTcpHdr->tcp_code &= ~TCPF_FIN;	/* don't process FINs in LISTEN */
	tcp_data( psNewTCB, psPkt );

	tcp_add_to_hash( psNewTCB );

//  tcp_create_event( psNewTCB, DELETE, TCP_TWOMSL, true );

//  UNLOCK( psNewTCB->tcb_hMutex );
      error2:
	UNLOCK( g_hConnListMutex );
      error1:
	ip_release_route( psRoute );
	return ( false );
}


/*****************************************************************************
 * NAME:
 * DESC:
 *	Handle incoming packets while in TCPS_SYNSENT state
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcps_synsent( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	tcpseq nAckSeq = ntohl( psTcpHdr->tcp_ack );

	if ( ( psTcpHdr->tcp_code & TCPF_ACK ) && ( ( nAckSeq - psTCPCtrl->tcb_iss <= 0 ) || ( nAckSeq - psTCPCtrl->tcb_nSndNext ) > 0 ) )
	{
		tcp_reset( psPkt );
		return ( false );
	}
	if ( ( psTcpHdr->tcp_code & TCPF_SYN ) == 0 )
	{
		return ( false );
	}

	psTCPCtrl->tcb_swindow = ntohs( psTcpHdr->tcp_window );
	psTCPCtrl->tcb_lwseq = ntohl( psTcpHdr->tcp_seq );
	psTCPCtrl->tcb_nRcvNext = ntohl( psTcpHdr->tcp_seq );

	psTCPCtrl->tcb_nAdvertizedRcvWin = psTCPCtrl->tcb_nRcvNext + psTCPCtrl->tcb_rbsize;

	tcp_acked( psTCPCtrl, psPkt );
	tcp_data( psTCPCtrl, psPkt );

	psTcpHdr->tcp_code &= ~TCPF_FIN;

	if ( psTCPCtrl->tcb_code & TCPF_SYN )
	{			/* our SYN not ACKed    */
		tcp_set_state( psTCPCtrl, TCPS_SYNRCVD );
	}
	else
	{
		g_nTcpCurrEstab++;
		if ( psTCPCtrl->tcb_bSendKeepalive )
		{
			tcp_create_event( psTCPCtrl, TCP_TE_KEEPALIVE, TCP_KEEPALIVE_TIME, true );
		}
		tcp_set_state( psTCPCtrl, TCPS_ESTABLISHED );
		UNLOCK( psTCPCtrl->tcb_ocsem );	// Wake up thread stuck in connect()
	}
	return ( false );
}


/*****************************************************************************
 * NAME:
 * DESC:
 *	tcps_synrcvd -  do SYN_RCVD state input processing
 *	If tcp_psParent is NULL this is a reply to an active
 *	open, so we just wakeup the thread waitng in connect().
 *	If however tcp_psParent contains a pointer it means
 *	this is a request from a client, and we checks that
 *	the parent is still in the listen state, and that the
 *	accept queue can take more connections. If so we wake
 *	up the server from accept() so it can create a socket
 *	for the new connection.
 * NOTE:
 *	If we get some unexpected input, we deallocate the tcb and
 *	send a reset to peer (unless we got a reset ourself).
 *	
 * SEE ALSO:
 ****************************************************************************/

static int tcps_synrcvd( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	if ( tcp_acked( psTCPCtrl, psPkt ) < 0 )
	{
		return ( false );
	}

	LOCK( g_hConnListMutex );
	if ( psTCPCtrl->tcb_psParent != NULL )	/* from a passive open  */
	{
		TCPCtrl_s *psParent = psTCPCtrl->tcb_psParent;

//    LOCK( psParent->tcb_hMutex );

		if ( psParent->tcb_nState != TCPS_LISTEN )
		{
			printk( "Error: tcps_synrcvd() connection refused, parent is not listening\n" );
			g_nTcpAttemptFails++;
			tcp_reset( psPkt );
			UNLOCK( g_hConnListMutex );
//      UNLOCK( psParent->tcb_hMutex );
			tcp_create_event( psTCPCtrl, DELETE, 1, true );
			return ( true );
		}

		tcp_set_state( psTCPCtrl, TCPS_ESTABLISHED );
		if ( psTCPCtrl->tcb_bSendKeepalive )
		{
			tcp_create_event( psTCPCtrl, TCP_TE_KEEPALIVE, TCP_KEEPALIVE_TIME, true );
		}

//    printk( "tcps_synrcvd(): Wake up linteners to %08x\n", psParent );
		tcp_wakeup( TCP_LISTENERS, psParent );

//    UNLOCK(psParent->tcb_hMutex);
	}
	else
	{			/* from an active open  */
		tcp_set_state( psTCPCtrl, TCPS_ESTABLISHED );
		if ( psTCPCtrl->tcb_bSendKeepalive )
		{
			tcp_create_event( psTCPCtrl, TCP_TE_KEEPALIVE, TCP_KEEPALIVE_TIME, true );
		}
		UNLOCK( psTCPCtrl->tcb_ocsem );
	}
	UNLOCK( g_hConnListMutex );

	g_nTcpCurrEstab++;
	tcp_data( psTCPCtrl, psPkt );
	// If all data was stuffed into that single packet we might 
	// have to switch from TCPS_ESTABLISHED to TCPS_CLOSEWAIT already.
	if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_RDONE )
	{
		tcp_set_state( psTCPCtrl, TCPS_CLOSEWAIT );
	}
	return ( false );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Handle incoming packets while in TCPS_FINWAIT1 state
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcps_fin1( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;

	if ( tcp_acked( psTCPCtrl, psPkt ) < 0 )
	{
		return ( false );
	}

	tcp_data( psTCPCtrl, psPkt );
	tcp_swindow( psTCPCtrl, psPkt );

	// For half-closed connections we might end up in FIN_WAIT_1 before we has received
	// all interesting data from the other end. Therefor we don't attempt to change state
	// before all data has been received.

	if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_RDONE )
	{
		if ( ( psTcpHdr->tcp_code & ( TCPF_FIN | TCPF_ACK ) ) == ( TCPF_FIN | TCPF_ACK ) )
		{
			tcp_set_state( psTCPCtrl, TCPS_TIMEWAIT );
//      tcp_wait(psTCPCtrl);
			atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
			atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_SNDFIN );
			tcp_kick( psTCPCtrl );
			tcp_create_event( psTCPCtrl, DELETE, TCP_TWOMSL, true );
		}
		else if ( psTcpHdr->tcp_code & TCPF_FIN )
		{		// Simultaneous close
			tcp_set_state( psTCPCtrl, TCPS_CLOSING );
			atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
			atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_SNDFIN );
			tcp_kick( psTCPCtrl );
			tcp_create_event( psTCPCtrl, DELETE, TCP_TWOMSL, true );
		}
		else
		{
			tcp_set_state( psTCPCtrl, TCPS_FINWAIT2 );
			// Make sure the connection get's deleted even if the other
			// end stay's quiet. Note that this violates the RFC, but seems
			// to be quit common, and as far as I can see is required to avoid
			// leaking connections.
			// Also note that even though it seems like this is the case
			// for TCPS_FINWAIT1, and TCPS_CLOSING to, thats not the case.
			// Before we enter one of those state's we will send a FIN packet
			// and if we don't get any reply on this the connection will be
			// deleted by the output thread.

			atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_SNDFIN );
			tcp_create_event( psTCPCtrl, DELETE, TCP_TWOMSL, true );
		}
	}
	else if ( ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_OPEN ) == 0 )
	{
		tcp_create_event( psTCPCtrl, DELETE, TCP_TWOMSL, true );
	}
	return ( false );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void tcp_in( PacketBuf_s *psPkt, int nDataLen )
{
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	TCPCtrl_s *psTCPCtrl;
	int nLocalPort = ntohs( psTcpHdr->tcp_dport );
	int nRemotePort = ntohs( psTcpHdr->tcp_sport );
	bool bDeleted = false;

	if ( psPkt->pb_bLocal == false && tcp_cksum( psPkt ) != 0 )
	{
		printk( "tcp_in() invalid checksum %d\n", tcp_cksum( psPkt ) );
		free_pkt_buffer( psPkt );
		return;
	}

//  printk( "Got TCP packet for port %d\n", ntohs( psTcpHdr->tcp_dport ) );
	psTCPCtrl = tcp_lookup( *( ( uint32 * )psIpHdr->iph_nDstAddr ), nLocalPort, *( ( uint32 * )psIpHdr->iph_nSrcAddr ), nRemotePort );
	if ( psTCPCtrl == NULL )
	{
//      printk( "Could not find TCP port %d\n", ntohs( psTcpHdr->tcp_dport ) );
		tcp_reset( psPkt );
		free_pkt_buffer( psPkt );
		return;
	}
	if ( psTCPCtrl->tcb_nState == TCPS_ESTABLISHED && psTCPCtrl->tcb_bSendKeepalive )
	{
		tcp_create_event( psTCPCtrl, TCP_TE_KEEPALIVE, TCP_KEEPALIVE_TIME, true );
	}
	psTCPCtrl->tcb_nLastReceiveTime = get_system_time();
	if ( psTCPCtrl->tcb_ostate == TCPO_KEEPALIVE )
	{
		tcp_set_output_state( psTCPCtrl, TCPO_IDLE );
		psTCPCtrl->tcb_rexmtcount = 0;
	}
	if ( psTCPCtrl->tcb_nState == TCPS_LISTEN && ( psTcpHdr->tcp_code & TCPF_SYN ) == 0 )
	{
//      printk( "Non syn packet to TCPS_LISTEN socket %d\n", ntohs( psTcpHdr->tcp_dport ) );
		tcp_reset( psPkt );
		free_pkt_buffer( psPkt );
		atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_BUSY );
		UNLOCK( psTCPCtrl->tcb_hMutex );
		return;
	}
	if ( tcp_ok( psTCPCtrl, psPkt ) == false )
	{
		tcp_ackit( psTCPCtrl, psPkt );
	}
	else
	{
		// Handle RESET packets
		if ( psTcpHdr->tcp_code & TCPF_RST )
		{
//      printk( "Got RESET packet while in %s state\n", tcp_state_str( psTCPCtrl->tcb_nState ) );
			switch ( psTCPCtrl->tcb_nState )
			{
			case TCPS_LISTEN:
				atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_BUSY );
				UNLOCK( psTCPCtrl->tcb_hMutex );
				break;	/* "parent" TCB still in LISTEN */
			case TCPS_SYNSENT:
				tcp_set_state( psTCPCtrl, TCPS_CLOSED );
				g_nTcpAttemptFails++;
				UNLOCK( psTCPCtrl->tcb_ocsem );
				tcp_abort( psTCPCtrl, ECONNREFUSED );
				break;
			case TCPS_SYNRCVD:
				g_nTcpAttemptFails++;
				UNLOCK( psTCPCtrl->tcb_ocsem );
				tcp_abort( psTCPCtrl, ECONNRESET );
				break;
			case TCPS_ESTABLISHED:
				g_nTcpEstabResets++;
				g_nTcpCurrEstab--;
			case TCPS_FINWAIT1:
			case TCPS_FINWAIT2:
			case TCPS_LASTACK:
				UNLOCK( psTCPCtrl->tcb_ocsem );
				tcp_abort( psTCPCtrl, ECONNRESET );
				break;
			case TCPS_CLOSEWAIT:
				g_nTcpEstabResets++;
				g_nTcpCurrEstab--;
				UNLOCK( psTCPCtrl->tcb_ocsem );
				tcp_abort( psTCPCtrl, EPIPE );
				break;
			case TCPS_CLOSING:
			case TCPS_TIMEWAIT:
				UNLOCK( psTCPCtrl->tcb_ocsem );
				tcp_abort( psTCPCtrl, ECONNRESET );
				break;
			}
			free_pkt_buffer( psPkt );
			return;
		}
		// Reset the connection if we got an invalid SYN packet
		if ( ( psTcpHdr->tcp_code & TCPF_SYN ) && ( psTCPCtrl->tcb_nState != TCPS_CLOSED && psTCPCtrl->tcb_nState != TCPS_LISTEN && psTCPCtrl->tcb_nState != TCPS_SYNSENT ) )
		{
			printk( "Got SYN packet while in %s state\n", tcp_state_str( psTCPCtrl->tcb_nState ) );
			if ( psTCPCtrl->tcb_nState == TCPS_SYNRCVD )
			{
				g_nTcpAttemptFails++;
			}
			if ( psTCPCtrl->tcb_nState == TCPS_ESTABLISHED || psTCPCtrl->tcb_nState == TCPS_CLOSEWAIT )
			{
				g_nTcpEstabResets++;
				g_nTcpCurrEstab--;
			}
			tcp_reset( psPkt );
			tcp_abort( psTCPCtrl, ECONNRESET );	// CHECKME: NOT SURE IF THIS IS CORRECT
			free_pkt_buffer( psPkt );
			return;
		}

		tcp_opts( psTCPCtrl, psPkt );
		switch ( psTCPCtrl->tcb_nState )
		{
		case TCPS_CLOSED:
			tcp_reset( psPkt );
			break;
		case TCPS_LISTEN:
			bDeleted = tcps_listen_state( psTCPCtrl, psPkt );
			break;
		case TCPS_SYNSENT:
			bDeleted = tcps_synsent( psTCPCtrl, psPkt );
			break;
		case TCPS_SYNRCVD:
			bDeleted = tcps_synrcvd( psTCPCtrl, psPkt );
			break;
		case TCPS_ESTABLISHED:
			if ( tcp_acked( psTCPCtrl, psPkt ) >= 0 )
			{
				tcp_data( psTCPCtrl, psPkt );
				tcp_swindow( psTCPCtrl, psPkt );
				if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_RDONE )
				{
					tcp_set_state( psTCPCtrl, TCPS_CLOSEWAIT );
				}
			}
			break;
		case TCPS_FINWAIT1:
			bDeleted = tcps_fin1( psTCPCtrl, psPkt );
			break;
		case TCPS_FINWAIT2:
			if ( tcp_acked( psTCPCtrl, psPkt ) >= 0 )
			{
				tcp_data( psTCPCtrl, psPkt );	/* for data + FIN ACKing */

				if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_RDONE )
				{
					tcp_set_state( psTCPCtrl, TCPS_TIMEWAIT );
					tcp_wait( psTCPCtrl );
				}
			}
			break;
		case TCPS_CLOSEWAIT:
			tcp_acked( psTCPCtrl, psPkt );
			tcp_swindow( psTCPCtrl, psPkt );
			break;
		case TCPS_LASTACK:
			if ( tcp_acked( psTCPCtrl, psPkt ) >= 0 )
			{
//      if ( (psTCPCtrl->tcb_code & TCPF_FIN) == 0 ) {
				tcp_abort( psTCPCtrl, 0 );
				free_pkt_buffer( psPkt );
				return;
			}
			break;
		case TCPS_CLOSING:
			tcp_acked( psTCPCtrl, psPkt );

//      if ( (psTCPCtrl->tcb_code & TCPF_FIN) == 0 ) {
			tcp_set_state( psTCPCtrl, TCPS_TIMEWAIT );
			tcp_wait( psTCPCtrl );
//      }
			break;
		case TCPS_TIMEWAIT:
			tcp_acked( psTCPCtrl, psPkt );
			tcp_data( psTCPCtrl, psPkt );	/* just ACK any packets */
			tcp_wait( psTCPCtrl );
			break;
		default:
			printk( "Error: tcp_in() invalid state %d\n", psTCPCtrl->tcb_nState );
			break;
		}
	}
	atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_BUSY );
	UNLOCK( psTCPCtrl->tcb_hMutex );
	free_pkt_buffer( psPkt );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int init_tcp()
{
	g_hConnListMutex = create_semaphore( "tcp_conn_list_mutex", 1, SEM_RECURSIVE );

	register_debug_cmd( "ls_tcp", "List existing TCP connections", dbg_list_connections );
	init_tcp_out();
	return ( 0 );
}
