/*
 *  The Syllable kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Kristian Van Der Vliet
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
#include <posix/uio.h>
#include <posix/fcntl.h>
#include <posix/ioctls.h>
#include <posix/signal.h>

#include <atheos/kernel.h>
#include <atheos/time.h>
#include <atheos/socket.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include <net/net.h>
#include <net/in.h>
#include <net/ip.h>
#include <net/if_ether.h>
#include <net/icmp.h>
#include "tcpdefs.h"

#include "inc/areas.h"

extern SocketOps_s g_sTCPOperations;

static int g_nPrevPort = 1024;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void tcp_dump_packet( PacketBuf_s *psPkt )
{
	TCPHeader_s *psTCPHdr = psPkt->pb_uTransportHdr.psTCP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;

	char zSrcAddr[64];
	char zDstAddr[64];
	char zFlags[64];

	format_ipaddress( zSrcAddr, psIpHdr->iph_nSrcAddr );
	format_ipaddress( zDstAddr, psIpHdr->iph_nDstAddr );

	zFlags[0] = '\0';

	if ( psTCPHdr->tcp_code & TCPF_URG )
		strcat( zFlags, "U" );
	if ( psTCPHdr->tcp_code & TCPF_ACK )
		strcat( zFlags, "A" );
	if ( psTCPHdr->tcp_code & TCPF_PSH )
		strcat( zFlags, "P" );
	if ( psTCPHdr->tcp_code & TCPF_RST )
		strcat( zFlags, "R" );
	if ( psTCPHdr->tcp_code & TCPF_SYN )
		strcat( zFlags, "S" );
	if ( psTCPHdr->tcp_code & TCPF_FIN )
		strcat( zFlags, "F" );

#if 0
	printk( "%s:%d -> %s:%d %ld:%ld W=%d O=%d %s\n", zSrcAddr, ntohs( psTCPHdr->tcp_sport ), zDstAddr, ntohs( psTCPHdr->tcp_dport ), ntohl( psTCPHdr->tcp_seq ), ntohl( psTCPHdr->tcp_ack ), ntohs( psTCPHdr->tcp_window ), psTCPHdr->tcp_offset, zFlags );
#endif
}

static bool tcp_set_close_state( TCPCtrl_s *psTCPCtrl, bool bDead )
{
	bool bSendFin = false;

	switch ( psTCPCtrl->tcb_nState )
	{
	case TCPS_SYNRCVD:
	case TCPS_ESTABLISHED:	// Start a close-down
		tcp_set_state( psTCPCtrl, TCPS_FINWAIT1 );
		bSendFin = true;
		break;
	case TCPS_FINWAIT1:	// Already closing, or FIN sent: no change
	case TCPS_FINWAIT2:
	case TCPS_CLOSING:
	case TCPS_CLOSED:	// No data exchanged so we just zap it
		break;
	case TCPS_SYNSENT:	// Did not receive any SYN so no FIN required
	case TCPS_LISTEN:
		tcp_set_state( psTCPCtrl, TCPS_CLOSED );
		break;
	case TCPS_CLOSEWAIT:
		tcp_set_state( psTCPCtrl, TCPS_LASTACK );
		bSendFin = true;
		break;
	case TCPS_LASTACK:
	case TCPS_TIMEWAIT:
		break;
	default:
		printk( "Error: tcp_set_close_state() invalid state %d\n", psTCPCtrl->tcb_nState );
		break;
	}
	if ( bDead && psTCPCtrl->tcb_nState == TCPS_FINWAIT2 )
	{
		printk( "Wierd: tcp_set_close_state() ended up in FIN_WAIT_2 state!?!\n" );
		tcp_create_event( psTCPCtrl, DELETE, TCP_TWOMSL, true );
	}
	return ( bSendFin );
}

/*****************************************************************************
 * NAME: tcp_close
 * DESC: close a TCP connection
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_close( Socket_s *psSocket )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	int nError;
	bool bSendFin;

      again:
	LOCK( g_hConnListMutex );
	if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_BUSY )
	{
		printk( "tcp_close() wait for connection to become ready\n" );
		UNLOCK( g_hConnListMutex );
		snooze( 100000 );
		goto again;
	}
	atomic_or( &psTCPCtrl->tcb_flags, TCBF_BUSY );
	UNLOCK( g_hConnListMutex );

	LOCK( psTCPCtrl->tcb_hMutex );

	if ( psTCPCtrl->tcb_nState == TCPS_LISTEN )
	{
		tcb_dealloc( psTCPCtrl, false );
		nError = 0;
	}
	else
	{
		bSendFin = tcp_set_close_state( psTCPCtrl, true );

		nError = psTCPCtrl->tcb_nError;

		psSocket->sk_psTCPCtrl = NULL;

		if ( psTCPCtrl->tcb_nState == TCPS_CLOSED || ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_ACTIVE ) == 0 )
		{
			tcb_dealloc( psTCPCtrl, false );
		}
		else
		{
			atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_OPEN );

			if ( bSendFin )
			{
				atomic_or( &psTCPCtrl->tcb_flags, TCBF_SNDFIN | TCBF_NEEDOUT );
				psTCPCtrl->tcb_slast = psTCPCtrl->tcb_nSndFirstUnacked + psTCPCtrl->tcb_nSndCount;
				tcp_kick( psTCPCtrl );
			}
			else
			{
				tcp_create_event( psTCPCtrl, DELETE, TCP_TWOMSL, true );	// In case the other end stay quiet
			}
			atomic_or( &psTCPCtrl->tcb_flags, TCBF_SDONE /* | TCBF_RDONE */  );	// FIXME: Find some other way of remembering that it's closed.
			if ( psTCPCtrl->tcb_rcvbuf != NULL )
			{
				kfree( psTCPCtrl->tcb_rcvbuf );
				psTCPCtrl->tcb_rcvbuf = NULL;
				tcp_wakeup( WRITERS, psTCPCtrl );
			}
			UNLOCK( psTCPCtrl->tcb_hMutex );
			atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_BUSY );
		}
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Shutdown the sending side of a connection. Much like close except
 *	that we don't receive shut down or set sk->dead.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int tcp_shutdown( Socket_s *psSocket, uint32 nHow )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;

//  printk( "tcp_shutdown() (nHow=%08x)\n", nHow );
	if ( ( nHow & SEND_SHUTDOWN ) == 0 )
	{
//      printk( "Shutdown of receiver\n" );
		return ( 0 );
	}

	// TODO: Set the TCBF_RDONE if required
	LOCK( psTCPCtrl->tcb_hMutex );
	/*
	 *        If we've already sent a FIN, or it's a closed state
	 */

	if ( psTCPCtrl->tcb_nState == TCPS_FINWAIT1 || psTCPCtrl->tcb_nState == TCPS_FINWAIT2 || psTCPCtrl->tcb_nState == TCPS_CLOSING || psTCPCtrl->tcb_nState == TCPS_CLOSED || psTCPCtrl->tcb_nState == TCPS_LISTEN || psTCPCtrl->tcb_nState == TCPS_LASTACK || psTCPCtrl->tcb_nState == TCPS_TIMEWAIT )
	{
		goto done;
	}


	// flag that the sender has shutdown

	atomic_or( &psTCPCtrl->tcb_flags, TCBF_SDONE );

	/*
	 *        FIN if needed
	 */

	if ( tcp_set_close_state( psTCPCtrl, false ) )
	{
		atomic_or( &psTCPCtrl->tcb_flags, TCBF_SNDFIN | TCBF_NEEDOUT );
		psTCPCtrl->tcb_slast = psTCPCtrl->tcb_nSndFirstUnacked + psTCPCtrl->tcb_nSndCount;
		tcp_kick( psTCPCtrl );
	}
      done:
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_bind( Socket_s *psSocket, const struct sockaddr *psAddr, int nAddrSize )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psAddr;
	char zBuf[64];
	int nPort;

	if ( psInAddr->sin_port == 0 )
	{
		nPort = g_nPrevPort++;
	}
	else
	{
		nPort = ntohs( psInAddr->sin_port );
	}

	format_ipaddress( zBuf, psInAddr->sin_addr );
//  printk( "Bind tcp socket %d to %s\n", nPort, zBuf );

	IP_COPYADDR( psTCPCtrl->tcb_lip, psInAddr->sin_addr );
	psTCPCtrl->tcb_lport = nPort;

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_connect( Socket_s *psSocket, const struct sockaddr *psAddr, int nAddrSize )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psAddr;
	Route_s *psRoute;
	int nError;

	LOCK( psTCPCtrl->tcb_hMutex );

	if ( psTCPCtrl->tcb_nState != TCPS_CLOSED )
	{
		if( psTCPCtrl->tcb_nState == TCPS_SYNSENT )
		{
			nError = -EALREADY;
			goto error;
		}
		else if( psTCPCtrl->tcb_nState == TCPS_ESTABLISHED )
		{
			nError = -EISCONN;
			goto error;
		}
		else
		{
			printk( "tcp_connect() invalid state %d\n", psTCPCtrl->tcb_nState );
			nError = -EINVAL;
			goto error;
		}
	}

	nError = tcp_sync( psTCPCtrl );

	if ( nError < 0 )
	{
		printk( "Error: tcp_connect() failed to initialize TCB\n" );
		goto error;
	}

	psTCPCtrl->tcb_smss = 1460;	/* RFC 1122             */

	psRoute = ip_find_route( psInAddr->sin_addr );

	if ( psRoute == NULL )
	{
		char zBuf[64];

		format_ipaddress( zBuf, psInAddr->sin_addr );
		printk( "tcp_connect() could not find route to %s\n", zBuf );

		nError = -ENETUNREACH;

		goto error;
	}
	IP_COPYADDR( psTCPCtrl->tcb_lip, psRoute->rt_psInterface->ni_anIpAddr );

	if ( psRoute->rt_nMetric == 0 )
	{
		/* Allow 40 bytes for the IP and TCP headers (20 bytes each) */
		psTCPCtrl->tcb_smss = psRoute->rt_psInterface->ni_nMTU - 40;
	}

	ip_release_route( psRoute );

//  IP_COPYADDR( psTCPCtrl->tcb_lip, g_anOurAddress );
	psTCPCtrl->tcb_lport = g_nPrevPort++;

	IP_COPYADDR( psTCPCtrl->tcb_rip, psInAddr->sin_addr );
	psTCPCtrl->tcb_rport = ntohs( psInAddr->sin_port );

//  printk( "Connect to remote port %d. mss=%d\n", psTCPCtrl->tcb_rport, psTCPCtrl->tcb_smss );

	psTCPCtrl->tcb_rmss = psTCPCtrl->tcb_smss;
	psTCPCtrl->tcb_swindow = psTCPCtrl->tcb_smss;	/* conservative         */
	psTCPCtrl->tcb_cwnd = psTCPCtrl->tcb_smss;	/* 1 segment            */
	psTCPCtrl->tcb_ssthresh = 65535;	/* IP Max window size   */
	psTCPCtrl->tcb_nRcvNext = 0;
	psTCPCtrl->tcb_finseq = 0;
	psTCPCtrl->tcb_pushseq = 0;
	atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT | TCBF_FIRSTSEND );
	psTCPCtrl->tcb_ostate = TCPO_IDLE;
	psTCPCtrl->tcb_nState = TCPS_SYNSENT;

	tcp_add_to_hash( psTCPCtrl );

	tcp_kick( psTCPCtrl );

//  TcpActiveOpens++;
	UNLOCK( psTCPCtrl->tcb_hMutex );

	if(  psTCPCtrl->tcb_bNonBlock )
		return -EINPROGRESS;
	else
	{
		nError = lock_semaphore( psTCPCtrl->tcb_ocsem, 0, INFINITE_TIMEOUT );
		/* Not fatal */
		if( nError == -EINTR )
			nError = EOK;
		return nError;
	}

error:
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( nError );
}

static int tcp_getpeername( Socket_s *psSocket, struct sockaddr *psName, int *pnNameLen )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psName;
	int nError;

	LOCK( psTCPCtrl->tcb_hMutex );

	if ( psTCPCtrl->tcb_nState == TCPS_CLOSED || psTCPCtrl->tcb_nState == TCPS_LISTEN )
	{			// Is this enough???
		nError = -ENOTCONN;
		goto error;
	}
	psInAddr->sin_family = AF_INET;
	psInAddr->sin_port = psTCPCtrl->tcb_rport;
	IP_COPYADDR( psInAddr->sin_addr, psTCPCtrl->tcb_rip );
	nError = 0;
      error:
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_getsockname( Socket_s *psSocket, struct sockaddr *psName, int *pnNameLen )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psName;
	int nError;

	LOCK( psTCPCtrl->tcb_hMutex );
#if 0
	if ( psTCPCtrl->tcb_nState == TCPS_CLOSED /*|| psTCPCtrl->tcb_nState == TCPS_LISTEN */  )
	{			// Is this enough???
		nError = -ENOTCONN;
		goto error;
	}
#endif
	psInAddr->sin_family = AF_INET;
	psInAddr->sin_port = htons( psTCPCtrl->tcb_lport );
	IP_COPYADDR( psInAddr->sin_addr, psTCPCtrl->tcb_lip );
	nError = 0;
//error:
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_listen( Socket_s *psSocket, int nQueueSize )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	int nError = 0;

	LOCK( psTCPCtrl->tcb_hMutex );
	if ( psTCPCtrl->tcb_nState != TCPS_CLOSED )
	{
		printk( "tcp_listen() invalid state %d\n", psTCPCtrl->tcb_nState );
		nError = -EINVAL;
		goto error;
	}

	psTCPCtrl->tcb_hListenSem = create_semaphore( "tcp_listen", 0, 0 );

	if ( psTCPCtrl->tcb_hListenSem < 0 )
	{
		nError = psTCPCtrl->tcb_hListenSem;
		goto error;
	}

//  printk( "tcp_listen() listen to port %d\n", psTCPCtrl->tcb_lport );

	psTCPCtrl->tcb_nState = TCPS_LISTEN;
	psTCPCtrl->tcb_nListenQueueSize = nQueueSize;
	psTCPCtrl->tcb_smss = 0;

	tcp_add_to_hash( psTCPCtrl );
      error:
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_accept( Socket_s *psSocket, struct sockaddr *psAddr, int *pnSize )
{
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psAddr;
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	TCPCtrl_s *psClient;
	TCPCtrl_s **ppsTmp;
	Socket_s *psNewSocket;
	int nNewSocket;
	int nError;

      retry:
	LOCK( psTCPCtrl->tcb_hMutex );

	if ( psTCPCtrl->tcb_nState != TCPS_LISTEN )
	{
		printk( "tcp_accept() invalid state %d\n", psTCPCtrl->tcb_nState );
		nError = -EINVAL;
		goto error;
	}

	LOCK( g_hConnListMutex );

	for ( ppsTmp = &psTCPCtrl->tcb_psFirstClient; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->tcb_psNextClient )
	{
		psClient = *ppsTmp;
		if ( psClient->tcb_nState == TCPS_ESTABLISHED || psClient->tcb_nState == TCPS_CLOSEWAIT )
		{
			if ( atomic_read( &psClient->tcb_flags ) & TCBF_BUSY )
			{
				UNLOCK( g_hConnListMutex );
				UNLOCK( psTCPCtrl->tcb_hMutex );
				snooze( 1000 );
				printk( "tcp_accept() wait for client to become ready\n" );
				goto retry;
			}
			*ppsTmp = psClient->tcb_psNextClient;
			psClient->tcb_psParent = NULL;
			psTCPCtrl->tcb_nNumClients--;
			goto found;
		}
		if ( psClient->tcb_nState != TCPS_SYNRCVD )
		{
			printk( "Error: tcp_accept() found child in %s state\n", tcp_state_str( psClient->tcb_nState ) );
		}
	}
	UNLOCK( g_hConnListMutex );

	if ( psTCPCtrl->tcb_bNonBlock )
	{
		nError = -EWOULDBLOCK;
		goto error;
	}

	nError = unlock_and_suspend( psTCPCtrl->tcb_hListenSem, psTCPCtrl->tcb_hMutex );
	if ( nError < 0 )
	{
		LOCK( psTCPCtrl->tcb_hMutex );
		goto error;
	}
	else
	{
		goto retry;
	}
      found:
	if ( NULL != psInAddr && NULL != pnSize )
	{
		struct sockaddr_in sAddr;

		sAddr.sin_family = AF_INET;
		sAddr.sin_port = htons( psClient->tcb_rport );
		IP_COPYADDR( sAddr.sin_addr, psClient->tcb_rip );
		memcpy( psAddr, &sAddr, min( *pnSize, sizeof( sAddr ) ) );
		*pnSize = min( *pnSize, sizeof( sAddr ) );
	}

	UNLOCK( psTCPCtrl->tcb_hMutex );

//  printk( "tcp_accept() got incoming connection from port %d\n", psClient->tcb_rport );

	// FIXME: We must handle requests from the kernel by creating a kernel file-descriptor
	nNewSocket = create_socket( false, AF_INET, SOCK_STREAM, IPPROTO_TCP, false, &psNewSocket );

	if ( nNewSocket >= 0 )
	{
		atomic_or( &psClient->tcb_flags, TCBF_OPEN );
		psNewSocket->sk_psOps = &g_sTCPOperations;
		psNewSocket->sk_psTCPCtrl = psClient;
		atomic_inc( &psClient->tcb_nRefCount );
	}
	UNLOCK( g_hConnListMutex );
	return ( nNewSocket );
      error:
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME: tcp_peekmsg
 * DESC: read one buffer from a TCP connection but do not remove the
 *       data from the buffer
 * NOTE:
 * BUGS: Does not wait for data to arrive when reading 0 bytes as required
 *       by 1003.1g
 * SEE ALSO: tcp_recvmsg
 ****************************************************************************/

static ssize_t tcp_peekmsg( Socket_s *psSocket, struct msghdr *psMsg )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	int nError, nRcvCount, nRcvBufStart;
	int nBytesRead = 0;
	int i;

	retry:
	LOCK( psTCPCtrl->tcb_hMutex );

	if ( psTCPCtrl->tcb_nError != 0 )
	{
		nError = -psTCPCtrl->tcb_nError;
		goto error;
	}
	if ( psTCPCtrl->tcb_rcvbuf == NULL )
	{
		printk( "Error: tcp_peekmsg() no receive buffer\n" );
		nError = -ENOTCONN;
		goto error;
	}

	if ( psTCPCtrl->tcb_nState == TCPS_CLOSED || psTCPCtrl->tcb_nState == TCPS_LISTEN )
	{			// Is this enough???
		nError = -ENOTCONN;
		goto error;
	}

	if ( psTCPCtrl->tcb_nRcvCount == 0 && ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_RDONE ) )
	{
		nError = 0;
		goto error;
	}

	if ( psTCPCtrl->tcb_nRcvCount == 0 )
	{
		if ( psTCPCtrl->tcb_bNonBlock )
		{
			nError = -EWOULDBLOCK;
			goto error;
		}
		else
		{
			nError = unlock_and_suspend( psTCPCtrl->tcb_hRecvQueue, psTCPCtrl->tcb_hMutex );
			if ( nError >= 0 )
			{
				goto retry;
			}
			else
			{
				LOCK( psTCPCtrl->tcb_hMutex );
				goto error;
			}
		}
	}
	kassertw( psTCPCtrl->tcb_nRcvCount > 0 );

	nRcvCount = psTCPCtrl->tcb_nRcvCount;
	nRcvBufStart = psTCPCtrl->tcb_nRcvBufStart;

	for ( i = 0; i < psMsg->msg_iovlen; ++i )
	{
		char *pBuffer = psMsg->msg_iov[i].iov_base;
		int nSize = psMsg->msg_iov[i].iov_len;

		while ( nSize > 0 && nRcvCount > 0 )
		{
			int nCurSize = min( nSize, nRcvCount );

			if ( nRcvBufStart + nCurSize <= psTCPCtrl->tcb_rbsize )
			{
				memcpy( pBuffer, psTCPCtrl->tcb_rcvbuf + nRcvBufStart, nCurSize );
				nRcvBufStart += nCurSize;
			}
			else
			{
				int nLeft = psTCPCtrl->tcb_rbsize - nRcvBufStart;

				memcpy( pBuffer, psTCPCtrl->tcb_rcvbuf +nRcvBufStart, nLeft );
				memcpy( pBuffer + nLeft, psTCPCtrl->tcb_rcvbuf, nCurSize - nLeft );
				nRcvBufStart = nCurSize - nLeft;
			}
			pBuffer += nCurSize;
			nRcvCount -= nCurSize;
			nSize -= nCurSize;
			nBytesRead += nCurSize;
		}
		if ( nRcvCount == 0 )
		{
			break;
		}
	}

	UNLOCK( psTCPCtrl->tcb_hMutex );
	psMsg->msg_flags = 0;
	return ( nBytesRead );

	error:
	tcp_wakeup( READERS, psTCPCtrl );	// Tell the next guy to give up.
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME: tcp_recvmsg
 * DESC: read one buffer from a TCP connection 
 * NOTE:
 * BUGS:
 *	Does not wait for data to arrive when reading 0 bytes as required
 *	by 1003.1g
 * SEE ALSO:
 ****************************************************************************/

static ssize_t tcp_recvmsg( Socket_s *psSocket, struct msghdr *psMsg, int nFlags )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	int nError;
	int nBytesRead = 0;
	int i;

	if ( nFlags & MSG_OOB )
	{
		printk( "Warning: tcp_recvmsg() Dont understand MSG_OOB\n" );
	}
	if ( nFlags & MSG_DONTROUTE )
	{
		printk( "Warning: tcp_recvmsg() Dont understand MSG_DONTROUTE\n" );
	}
	if ( nFlags & MSG_PROXY )
	{
		printk( "Warning: tcp_recvmsg() Dont understand MSG_PROXY\n" );
	}

	if( nFlags & MSG_PEEK )
	{
		return( tcp_peekmsg( psSocket, psMsg ) );
	}

      retry:
	LOCK( psTCPCtrl->tcb_hMutex );

	if ( psTCPCtrl->tcb_nError != 0 )
	{
//    printk( "tcp_recvmsg() found error code %d\n", psTCPCtrl->tcb_nError );
		nError = -psTCPCtrl->tcb_nError;
		goto error;
	}
	if ( psTCPCtrl->tcb_rcvbuf == NULL )
	{
		printk( "Error: tcp_recvmsg() no receive buffer\n" );
		nError = -ENOTCONN;
		goto error;
	}

	if( psTCPCtrl->tcb_nState == TCPS_SYNSENT )
	{
		nError = -EAGAIN;
		goto error;
	}

	if ( psTCPCtrl->tcb_nState == TCPS_CLOSED || psTCPCtrl->tcb_nState == TCPS_LISTEN )
	{			// Is this enough???
		nError = -ENOTCONN;
		goto error;
	}

	if ( psTCPCtrl->tcb_nRcvCount == 0 && ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_RDONE ) )
	{
		nError = 0;
		goto error;
	}

	if ( psTCPCtrl->tcb_nRcvCount == 0 )
	{
		if ( psTCPCtrl->tcb_bNonBlock )
		{
			nError = -EWOULDBLOCK;
			goto error;
		}
		else
		{
			nError = unlock_and_suspend( psTCPCtrl->tcb_hRecvQueue, psTCPCtrl->tcb_hMutex );
			if ( nError >= 0 )
			{
				goto retry;
			}
			else
			{
				LOCK( psTCPCtrl->tcb_hMutex );
				goto error;
			}
		}
	}
	kassertw( psTCPCtrl->tcb_nRcvCount > 0 );

	for ( i = 0; i < psMsg->msg_iovlen; ++i )
	{
		char *pBuffer = psMsg->msg_iov[i].iov_base;
		int nSize = psMsg->msg_iov[i].iov_len;

		while ( nSize > 0 && psTCPCtrl->tcb_nRcvCount > 0 )
		{
			int nCurSize = min( nSize, psTCPCtrl->tcb_nRcvCount );

			if ( psTCPCtrl->tcb_nRcvBufStart + nCurSize <= psTCPCtrl->tcb_rbsize )
			{
				memcpy( pBuffer, psTCPCtrl->tcb_rcvbuf + psTCPCtrl->tcb_nRcvBufStart, nCurSize );
				psTCPCtrl->tcb_nRcvBufStart += nCurSize;
			}
			else
			{
				int nLeft = psTCPCtrl->tcb_rbsize - psTCPCtrl->tcb_nRcvBufStart;

				memcpy( pBuffer, psTCPCtrl->tcb_rcvbuf + psTCPCtrl->tcb_nRcvBufStart, nLeft );
				memcpy( pBuffer + nLeft, psTCPCtrl->tcb_rcvbuf, nCurSize - nLeft );
				psTCPCtrl->tcb_nRcvBufStart = nCurSize - nLeft;
			}
			pBuffer += nCurSize;
			psTCPCtrl->tcb_nRcvCount -= nCurSize;
			nSize -= nCurSize;
			nBytesRead += nCurSize;
		}
		if ( psTCPCtrl->tcb_nRcvCount == 0 )
		{
			break;
		}
	}
	if ( psTCPCtrl->tcb_nRcvCount > 0 || ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_RDONE ) )
	{
		tcp_wakeup( READERS, psTCPCtrl );
	}
	else
	{
		atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_PUSH );
	}
	// Open the receive window, if it's closed and we've made
	// enough space to fit a segment.

	if ( ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_RWIN_CLOSED ) && tcp_rwindow( psTCPCtrl, false ) > 0 )
	{
		atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
		atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_RWIN_CLOSED );
		tcp_kick( psTCPCtrl );
	}
	UNLOCK( psTCPCtrl->tcb_hMutex );
	psMsg->msg_flags = 0;
	return ( nBytesRead );
      error:
	tcp_wakeup( READERS, psTCPCtrl );	// Tell the next guy to give up.
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( nError );
}

static ssize_t tcp_sendmsg( Socket_s *psSocket, const struct msghdr *psMsg, int nFlags )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	int nError;
	int nReqSize = 0;
	int nBytesSendt = 0;
	int nBufferSpace;
	int nSndBufOffset;
	int i;


	for ( i = 0; i < psMsg->msg_iovlen; ++i )
	{
		nReqSize += psMsg->msg_iov[i].iov_len;
	}
	if ( nReqSize == 0 )
	{
		return ( 0 );
	}
	if ( psTCPCtrl->tcb_nError != 0 )
	{
//    printk( "tcp_sendmsg() found error code %d\n", psTCPCtrl->tcb_nError );
		return ( -psTCPCtrl->tcb_nError );
	}


	if ( nFlags & MSG_OOB )
	{
		printk( "Warning: tcp_sendmsg() Dont understand MSG_OOB\n" );
	}
	if ( nFlags & MSG_PEEK )
	{
		printk( "Warning: tcp_sendmsg() Dont understand MSG_PEEK\n" );
	}
	if ( nFlags & MSG_DONTROUTE )
	{
		printk( "Warning: tcp_sendmsg() Dont understand MSG_DONTROUTE\n" );
	}
	if ( nFlags & MSG_PROXY )
	{
		printk( "Warning: tcp_sendmsg() Dont understand MSG_PROXY\n" );
	}
//retry:
	LOCK( psTCPCtrl->tcb_hMutex );

	if ( psTCPCtrl->tcb_sndbuf == NULL )
	{
		printk( "Error: tcp_sendmsg() no send buffer\n" );
		nError = -ENOTCONN;
		goto error;
	}

	if( psTCPCtrl->tcb_nState == TCPS_SYNSENT )
	{
		nError = -EAGAIN;
		goto error;
	}

	if ( psTCPCtrl->tcb_nState != TCPS_ESTABLISHED && psTCPCtrl->tcb_nState != TCPS_CLOSEWAIT )
	{
		nError = -ENOTCONN;
		goto error;
	}
	if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_SDONE )
	{
		printk( "tcp_sendmsg() attempt to send to a shutdown socket\n" );
		sys_kill( sys_get_thread_id( NULL ), SIGPIPE );
		UNLOCK( psTCPCtrl->tcb_hMutex );
		return ( -EPIPE );
	}

	if ( psTCPCtrl->tcb_bNonBlock && nReqSize > psTCPCtrl->tcb_nSndBufSize )
	{
		nReqSize = psTCPCtrl->tcb_nSndBufSize;
		if ( nReqSize == 0 )
		{
			nError = -EWOULDBLOCK;
			goto error;
		}
	}

	nBufferSpace = psTCPCtrl->tcb_nSndBufSize - psTCPCtrl->tcb_nSndCount;

	if ( nBufferSpace < 0 )
	{
		printk( "Error:1 Buffer space got negative value %d (%d - %d)\n", nBufferSpace, psTCPCtrl->tcb_nSndBufSize, psTCPCtrl->tcb_nSndCount );
		nError = -EINVAL;
		goto error;
	}

	if ( nReqSize > nBufferSpace && psTCPCtrl->tcb_bNonBlock )
	{
		if ( nBufferSpace == 0 )
		{
			nError = -EWOULDBLOCK;
			goto error;
		}
		nReqSize = nBufferSpace;
	}
//  kassertw( nReqSize <= (psTCPCtrl->tcb_nSndBufSize - psTCPCtrl->tcb_nSndCount) );

	nSndBufOffset = psTCPCtrl->tcb_sbstart + psTCPCtrl->tcb_nSndCount;

	if ( nSndBufOffset >= 0 )
	{
		nSndBufOffset = nSndBufOffset % psTCPCtrl->tcb_nSndBufSize;
	}
	else
	{
		printk( "Error: Got negative buffer offset %d (%d, %d)\n", nSndBufOffset, psTCPCtrl->tcb_sbstart, psTCPCtrl->tcb_nSndCount );
		nSndBufOffset = 0;
	}
	nError = 0;

	for ( i = 0; i < psMsg->msg_iovlen && nError >= 0 && nReqSize > 0; ++i )
	{
		void *pBuffer = psMsg->msg_iov[i].iov_base;
		uint nSize = psMsg->msg_iov[i].iov_len;

		MemArea_s *psArea = NULL;

		psArea = verify_area( pBuffer, nSize, true );
		if ( psArea == NULL )
		{
			printk( "Error: tcp_sendmsg() got invalid buffer pointer from userspace [%d of %d] %p -> %d\n", i, psMsg->msg_iovlen, pBuffer, nSize );
			nError = -EFAULT;
			break;
		}

		while ( nSize > 0 && nReqSize > 0 )
		{
			uint32 nCurSize = min( nSize, nBufferSpace );

			if ( nSndBufOffset + nCurSize <= psTCPCtrl->tcb_nSndBufSize )
			{
				memcpy( psTCPCtrl->tcb_sndbuf + nSndBufOffset, pBuffer, nCurSize );
				nSndBufOffset += nCurSize;
			}
			else
			{
				int nLeft = psTCPCtrl->tcb_nSndBufSize - nSndBufOffset;
				int nRight = nCurSize - nLeft;

				memcpy( psTCPCtrl->tcb_sndbuf + nSndBufOffset, pBuffer, nLeft );
				memcpy( psTCPCtrl->tcb_sndbuf, pBuffer + nLeft, nRight );
				nSndBufOffset = nRight;
			}
			pBuffer += nCurSize;

			psTCPCtrl->tcb_nSndCount += nCurSize;
			nSize -= nCurSize;
			nBytesSendt += nCurSize;
			nReqSize -= nCurSize;
			nBufferSpace -= nCurSize;

			kassertw( nReqSize >= 0 );
			kassertw( nSize >= 0 );

			if ( nBufferSpace == 0 && nReqSize > 0 )
			{
				atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
//  if ( isurg || psTCPCtrl->tcb_nSndNext == psTCPCtrl->tcb_nSndFirstUnacked ) {
				tcp_kick( psTCPCtrl );

				nError = unlock_and_suspend( psTCPCtrl->tcb_hSendQueue, psTCPCtrl->tcb_hMutex );
				LOCK( psTCPCtrl->tcb_hMutex );
				if ( nError < 0 )
				{
					break;
				}

				nSndBufOffset = ( psTCPCtrl->tcb_sbstart + psTCPCtrl->tcb_nSndCount ) % psTCPCtrl->tcb_nSndBufSize;
				nBufferSpace = psTCPCtrl->tcb_nSndBufSize - psTCPCtrl->tcb_nSndCount;

				// We must now check that no errors occured while we sleept.
				if ( psTCPCtrl->tcb_nError != 0 )
				{
					nError = -psTCPCtrl->tcb_nError;
					put_area( psArea );
					goto error;
				}
				if ( psTCPCtrl->tcb_sndbuf == NULL )
				{
					printk( "Error: tcp_sendmsg() no send buffer\n" );
					nError = -ENOTCONN;
					put_area( psArea );
					goto error;
				}

				if ( psTCPCtrl->tcb_nState != TCPS_ESTABLISHED && psTCPCtrl->tcb_nState != TCPS_CLOSEWAIT )
				{
					nError = -EINVAL;
					put_area( psArea );
					goto error;
				}
				if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_SDONE )
				{
					printk( "tcp_sendmsg() attempt to send to a shutdown socket\n" );
					sys_kill( sys_get_thread_id( NULL ), SIGPIPE );
					UNLOCK( psTCPCtrl->tcb_hMutex );
					put_area( psArea );
					return ( -EPIPE );
				}
			}
		}
		put_area( psArea );
	}
	atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
	if ( psTCPCtrl->tcb_ostate == TCPO_IDLE || psTCPCtrl->tcb_ostate == TCPO_XMIT /*psTCPCtrl->tcb_nSndNext == psTCPCtrl->tcb_nSndFirstUnacked */  )
	{
		tcp_kick( psTCPCtrl );
	}
	if ( nBufferSpace > 0 )
	{
		tcp_wakeup( WRITERS, psTCPCtrl );
	}
	UNLOCK( psTCPCtrl->tcb_hMutex );

	return ( nBytesSendt );
      error:
	atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
	if ( nError != -EWOULDBLOCK )
	{
		tcp_kick( psTCPCtrl );
		tcp_wakeup( WRITERS, psTCPCtrl );
	}
	UNLOCK( psTCPCtrl->tcb_hMutex );

	return ( nError );

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int tcp_open( Socket_s *psSocket )
{
	psSocket->sk_psTCPCtrl = tcb_alloc();
	if ( psSocket->sk_psTCPCtrl == NULL )
	{
		return ( -ENOMEM );
	}
	atomic_or( &psSocket->sk_psTCPCtrl->tcb_flags, TCBF_OPEN );
	psSocket->sk_psOps = &g_sTCPOperations;
	atomic_inc( &psSocket->sk_psTCPCtrl->tcb_nRefCount );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_add_select( Socket_s *psSocket, SelectRequest_s *psReq )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	int nError;

	LOCK( psTCPCtrl->tcb_hMutex );
	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		psReq->sr_psNext = psTCPCtrl->tcb_psFirstReadSelReq;
		psTCPCtrl->tcb_psFirstReadSelReq = psReq;

		if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_RDONE )
		{
			psReq->sr_bReady = true;
			UNLOCK( psReq->sr_hSema );
		}
		else
		{
			if ( psTCPCtrl->tcb_nState != TCPS_LISTEN && psTCPCtrl->tcb_nRcvCount > 0 )
			{
				psReq->sr_bReady = true;
				UNLOCK( psReq->sr_hSema );
			}
			if ( psTCPCtrl->tcb_nState == TCPS_LISTEN )
			{
				TCPCtrl_s *psClient;

				LOCK( g_hConnListMutex );

				for ( psClient = psTCPCtrl->tcb_psFirstClient; psClient != NULL; psClient = psClient->tcb_psNextClient )
				{
					if ( psClient->tcb_nState == TCPS_ESTABLISHED || psClient->tcb_nState == TCPS_CLOSEWAIT )
					{
						psReq->sr_bReady = true;
						UNLOCK( psReq->sr_hSema );
						break;
					}
					if ( psClient->tcb_nState != TCPS_SYNRCVD )
					{
						printk( "Error: tcp_add_select() found child in %s state\n", tcp_state_str( psClient->tcb_nState ) );
					}
				}
				UNLOCK( g_hConnListMutex );
			}
		}
		nError = 0;
		break;
	case SELECT_WRITE:
		psReq->sr_psNext = psTCPCtrl->tcb_psFirstWriteSelReq;
		psTCPCtrl->tcb_psFirstWriteSelReq = psReq;

		if ( psTCPCtrl->tcb_nSndCount < psTCPCtrl->tcb_nSndBufSize || ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_SDONE ) )
		{
			tcp_wakeup( WRITERS, psTCPCtrl );	// Acknowlege the request right away.
		}
		nError = 0;
		break;
	case SELECT_EXCEPT:
		nError = 0;
		break;
	default:
		printk( "ERROR : tcp_add_select() unknown mode %d\n", psReq->sr_nMode );
		nError = -EINVAL;
		break;
	}
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_rem_select( Socket_s *psSocket, SelectRequest_s *psReq )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	SelectRequest_s **ppsTmp = NULL;
	int nError;

	LOCK( psTCPCtrl->tcb_hMutex );

	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		ppsTmp = &psTCPCtrl->tcb_psFirstReadSelReq;
		nError = 0;
		break;
	case SELECT_WRITE:
		ppsTmp = &psTCPCtrl->tcb_psFirstWriteSelReq;
		nError = 0;
		break;
	case SELECT_EXCEPT:
		nError = 0;
		break;
	default:
		printk( "ERROR : tcp_rem_select() unknown mode %d\n", psReq->sr_nMode );
		nError = -EINVAL;
		break;
	}
	if ( NULL != ppsTmp )
	{
		for ( ; NULL != *ppsTmp; ppsTmp = &( ( *ppsTmp )->sr_psNext ) )
		{
			if ( *ppsTmp == psReq )
			{
				*ppsTmp = psReq->sr_psNext;
				break;
			}
		}
	}
	UNLOCK( psTCPCtrl->tcb_hMutex );
	return ( nError );
}

static int tcp_set_fflags( Socket_s *psSocket, uint32 nFlags )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;

	if ( nFlags & O_NONBLOCK )
	{
		psTCPCtrl->tcb_bNonBlock = true;
	}
	else
	{
		psTCPCtrl->tcb_bNonBlock = false;
	}
	return ( 0 );
}

static int tcp_ioctl( Socket_s *psSocket, int nCmd, void *pBuffer, bool bFromKernel )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	int nError = 0;

	switch ( nCmd )
	{
	case FIONREAD:
		if ( bFromKernel )
		{
			*( ( int * )pBuffer ) = psTCPCtrl->tcb_nRcvCount;
		}
		else
		{
			nError = memcpy_to_user( pBuffer, &psTCPCtrl->tcb_nRcvCount, sizeof( int ) );
		}
		break;
	default:
		nError = -ENOSYS;
		break;
	}
	return ( nError );
}

static int tcp_setsockopt( bool bFromKernel, Socket_s *psSocket, int nProtocol, int nOptName, const void *pOptVal, int nOptLen )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	int nError = -EINVAL;

	switch( nProtocol )
	{
		case SOL_SOCKET:
		{
			int nValue = 0;

			if( bFromKernel )
				memcpy( &nValue, pOptVal, sizeof( int ) );
			else
			{
				if( memcpy_from_user( &nValue, pOptVal, sizeof( int ) ) < 0 )
				{
					nError = -EFAULT;
					break;
				}
			}
			nValue = !!nValue;

			switch( nOptName )
			{
				case SO_KEEPALIVE:
				{
					psSocket->sk_bKeep = (nValue ? true : false );

					if( nValue )
					{
						if( psTCPCtrl->tcb_bSendKeepalive == false )
						{
							psTCPCtrl->tcb_bSendKeepalive = true;
							if( psTCPCtrl->tcb_nState == TCPS_ESTABLISHED )
								tcp_create_event( psTCPCtrl, TCP_TE_KEEPALIVE, TCP_KEEPALIVE_TIME, true );
						}
					}
					else
					{
						if( psTCPCtrl->tcb_bSendKeepalive )
						{
							psTCPCtrl->tcb_bSendKeepalive = false;
							tcp_delete_event( psTCPCtrl, TCP_TE_KEEPALIVE );
							if( psTCPCtrl->tcb_ostate == TCPO_KEEPALIVE )
							{
								tcp_set_output_state( psTCPCtrl, TCPO_IDLE );
								psTCPCtrl->tcb_rexmtcount = 0;
							}
						}
					}
					nError = 0;
					break;
				}

				case SO_DONTROUTE:
				{
					psSocket->sk_bDontRoute = (nValue ? true : false );
					nError = 0;
					break;
				}

				case SO_SNDBUF:
				{
					int nNewSize;

					nError = 0;

					if( bFromKernel )
						memcpy( &nNewSize, pOptVal, sizeof( int ) );
					else
						memcpy_from_user( &nNewSize, pOptVal, sizeof( int ) );

					LOCK( psTCPCtrl->tcb_hMutex );

					if( psTCPCtrl->tcb_nSndBufSize != nNewSize )
					{
						/* Allocate a new buffer and copy the contents of the old buffer */
						char *pOldBuf, *pNewBuf;

						pNewBuf = kmalloc( nNewSize, MEMF_KERNEL | MEMF_CLEAR );
						if( NULL == pNewBuf )
						{
							printk( "could not allocate %d bytes for socket buffer.\n", nNewSize );
							nError = -ENOMEM;
						}
						else
						{
							memcpy( pNewBuf, psTCPCtrl->tcb_sndbuf, psTCPCtrl->tcb_nSndBufSize );

							/* Swap the buffers */
							pOldBuf = psTCPCtrl->tcb_sndbuf;
							psTCPCtrl->tcb_sndbuf = pNewBuf;
							psTCPCtrl->tcb_nSndBufSize = nNewSize;
							kfree( pOldBuf );
						}
					}

					UNLOCK( psTCPCtrl->tcb_hMutex );

					break;
				}

				case SO_RCVBUF:
				{
					int nNewSize;

					nError = 0;

					if( bFromKernel )
						memcpy( &nNewSize, pOptVal, sizeof( int ) );
					else
						memcpy_from_user( &nNewSize, pOptVal, sizeof( int ) );

					LOCK( psTCPCtrl->tcb_hMutex );

					if( psTCPCtrl->tcb_rbsize != nNewSize )
					{
						/* Allocate a new buffer and copy the contents of the old buffer */
						char *pOldBuf, *pNewBuf;

						pNewBuf = kmalloc( nNewSize, MEMF_KERNEL | MEMF_CLEAR );
						if( NULL == pNewBuf )
						{
							printk( "could not allocate %d bytes for socket buffer.\n", nNewSize );
							nError = -ENOMEM;
						}
						else
						{
							memcpy( pNewBuf, psTCPCtrl->tcb_rcvbuf, psTCPCtrl->tcb_rbsize );

							/* Swap the buffers */
							pOldBuf = psTCPCtrl->tcb_rcvbuf;
							psTCPCtrl->tcb_rcvbuf = pNewBuf;
							psTCPCtrl->tcb_rbsize = nNewSize;
							kfree( pOldBuf );
						}
					}

					UNLOCK( psTCPCtrl->tcb_hMutex );

					break;
				}

				default:
				{
					printk( "tcp_setsockopt: invalid option %d at level SOL_SOCKET\n", nOptName );
					nError = -EINVAL;
					break;
				}
			}
			break;
		}

		case SOL_TCP:
		{
			switch( nOptName )
			{
				case TCP_NODELAY:
				case TCP_MAXSEG:
				{
					nError = 0;
					break;
				}

				default:
				{
					printk( "tcp_setsockopt: invalid option %d at level SOL_TCP\n", nOptName );
					nError = -EINVAL;
					break;
				}
			}
			break;
		}
	}

	return nError;
}

static int tcp_getsockopt( bool bFromKernel, Socket_s *psSocket, int nProtocol, int nOptName, void *pOptVal, int nOptLen )
{
	TCPCtrl_s *psTCPCtrl = psSocket->sk_psTCPCtrl;
	int nError = -EINVAL;

	switch( nProtocol )
	{
		case SOL_SOCKET:
		{
			switch( nOptName )
			{
				case SO_SNDBUF:
				{
					int nValue = psTCPCtrl->tcb_nSndBufSize;
					if( bFromKernel )
						memcpy( pOptVal, &nValue, sizeof( int ) );
					else
						memcpy_to_user( pOptVal, &nValue, sizeof( int ) );

					nError = 0;
					break;
				}

				case SO_RCVBUF:
				{
					int nValue = psTCPCtrl->tcb_rbsize;
					if( bFromKernel )
						memcpy( pOptVal, &nValue, sizeof( int ) );
					else
						memcpy_to_user( pOptVal, &nValue, sizeof( int ) );

					nError = 0;
					break;
				}

				case SO_ERROR:
				{
					int nValue = psTCPCtrl->tcb_nError;
					if( bFromKernel )
						memcpy( pOptVal, &nValue, sizeof( int ) );
					else
						memcpy_to_user( pOptVal, &nValue, sizeof( int ) );

					psTCPCtrl->tcb_nError = 0;
					nError = 0;
					break;
				}

				default:
				{
					kerndbg( KERN_DEBUG, "%s: invalid option %d for protocol SOL_SOCKET\n", __FUNCTION__, nOptName );
					nError = -EINVAL;
					break;
				}
			}
			break;
		}

		case SOL_TCP:
		{
			switch( nOptName )
			{
				default:
				{
					kerndbg( KERN_DEBUG, "%s: invalid option %d for protocol SOL_TCP\n", __FUNCTION__, nOptName );
					nError = -EINVAL;
					break;
				}
			}
			break;
		}

		default:
		{
			kerndbg( KERN_DEBUG, "%s: invalid protocol %d\n", __FUNCTION__, nProtocol );
			nError = -EINVAL;
			break;
		}
	}

	return nError;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

SocketOps_s g_sTCPOperations = {
	tcp_open,
	tcp_close,
	tcp_shutdown,
	tcp_bind,
	tcp_connect,
	tcp_getsockname,
	tcp_getpeername,
	tcp_recvmsg,
	tcp_sendmsg,
	tcp_add_select,
	tcp_rem_select,
	tcp_listen,
	tcp_accept,
	tcp_setsockopt,
	tcp_getsockopt,
	tcp_set_fflags,
	tcp_ioctl
};
