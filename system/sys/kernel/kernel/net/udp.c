
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

#include <posix/errno.h>
#include <posix/uio.h>
#include <posix/ioctls.h>

#include <atheos/kernel.h>
#include <atheos/socket.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include <net/net.h>
#include <net/in.h>
#include <net/ip.h>
#include <net/if_ether.h>
#include <net/icmp.h>
#include <net/udp.h>

/* Selective debugging level overrides */
#ifdef KERNEL_DEBUG_NET
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET
#endif

#ifdef KERNEL_DEBUG_NET_UDP
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET_UDP
#endif

extern SocketOps_s g_sUDPOperations;

static UDPPort_s *g_psFirstUDPPort = NULL;
static sem_id g_hUDPPortLock = -1;


/*****************************************************************************
 * NAME:
 * DESC:
 *	udp_cksum -  compute a UDP pseudo-header checksum
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static uint16 udp_cksum( PacketBuf_s *psPkt )
{
	UDPHeader_s *psUdpHdr = psPkt->pb_uTransportHdr.psUDP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	uint16 *psh;
	uint32 sum;
	uint16 len = ntohs( psUdpHdr->udp_nSize );
	int i;

	sum = 0;

	psh = ( uint16 * )&psIpHdr->iph_nSrcAddr[0];

	for ( i = 0; i < IP_ADR_LEN; ++i )
	{
		sum += psh[i];
	}

	psh = ( uint16 * )psUdpHdr;
	sum += htons( ( uint16 )psIpHdr->iph_nProtocol );
	sum += psUdpHdr->udp_nSize;

	if ( len & 0x1 )
	{
		( ( char * )psUdpHdr )[len] = 0;	/* pad */
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

/**
 * \par Description:
 * Finds the port entry for the specified port.
 */
static UDPPort_s *udp_find_port( uint16 nPort )
{
	UDPPort_s *psPort;

	for ( psPort = g_psFirstUDPPort; psPort != NULL; psPort = psPort->up_psNext )
	{
		if ( psPort->up_nPortNum == nPort )
		{
			return ( psPort );
		}
	}
	return ( NULL );
}

/**
 * \par Description:
 * Locates the best endpoint to dispatch an incoming packet to.
 * The interface list is searched until an endpoint with
 *   - a local address matching the packet destination,
 *   - a remote address matching the packet source,
 *   - a remote port matching the packet source.
 *
 * If an endpoint is found that meets all criteria, it is used.
 * Otherwise the first interface matching most criteria is used.
 */
static void udp_enqueue_packet( UDPPort_s *psPort, PacketBuf_s *psPkt )
{
	UDPHeader_s *psUdpHdr = psPkt->pb_uTransportHdr.psUDP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	ipaddr_t anNullAddr;
	UDPEndPoint_s *psTmp;
	UDPEndPoint_s *psEndP = NULL;
	uint16 nSrcPort;
	int nBestScore = -1;
	SelectRequest_s *psReq;

	IP_MAKEADDR( anNullAddr, 0, 0, 0, 0 );

	nSrcPort = ntohs( psUdpHdr->udp_nSrcPort );

	for ( psTmp = psPort->up_psFirstEndPoint; psTmp != NULL; psTmp = psTmp->ue_psNext )
	{
		int nScore = 0;
		char zSrc[64];
		char zDst[64];

		format_ipaddress( zSrc, psTmp->ue_anLocalAddr );
		format_ipaddress( zDst, psTmp->ue_anRemoteAddr );
//    printk( "udp_enqueue_packet() test endpoint %s:%d -> %s:%d\n",
//          zSrc, psTmp->ue_nLocalPort, zDst, psTmp->ue_nRemotePort );

		if ( IP_SAMEADDR( psTmp->ue_anLocalAddr, anNullAddr ) == false )
		{
			if ( IP_SAMEADDR( psTmp->ue_anLocalAddr, psIpHdr->iph_nDstAddr ) == false )
			{
				continue;
			}
			nScore++;
		}
		if ( IP_SAMEADDR( psTmp->ue_anRemoteAddr, anNullAddr ) == false )
		{
			if ( IP_SAMEADDR( psTmp->ue_anRemoteAddr, psIpHdr->iph_nSrcAddr ) == false )
			{
				continue;
			}
			nScore++;
		}
		if ( psTmp->ue_nRemotePort != 0 )
		{
			if ( psTmp->ue_nRemotePort != nSrcPort )
			{
				continue;
			}
			nScore++;
		}
//    if ( IP_SAMEADDR( psTmp->ue_anSndAddr, psIpHdr->iph_nSrcAddr ) && psTmp->ue_nSrcPort == nSrcPort ) {
//      break;
//    }
		if ( nScore == 3 )
		{
			psEndP = psTmp;
			break;
		}
		else if ( nScore > nBestScore )
		{
			psEndP = psTmp;
			nBestScore = nScore;
		}
	}
	if ( psEndP == NULL )
	{
		char zSrc[64];
		char zDst[64];

		format_ipaddress( zSrc, psIpHdr->iph_nSrcAddr );
		format_ipaddress( zDst, psIpHdr->iph_nDstAddr );
//    printk( "udp_enqueue_packet() could not find an endpoint %s:%d -> %s:%d\n",
//          zSrc, ntohs( psUdpHdr->udp_nSrcPort ), zDst, ntohs( psUdpHdr->udp_nDstPort ) );

		free_pkt_buffer( psPkt );
		return;
	}
	if ( psEndP->ue_sPackets.nq_nCount > 16 )
	{
		printk( "udp_enqueue_packet() queue for port %d is full\n", psPort->up_nPortNum );
		free_pkt_buffer( psPkt );
		return;
	}
	enqueue_packet( &psEndP->ue_sPackets, psPkt );

	for ( psReq = psEndP->ue_psFirstReadSelReq; psReq != NULL; psReq = psReq->sr_psNext )
	{
		psReq->sr_bReady = true;
		UNLOCK( psReq->sr_hSema );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void udp_in( PacketBuf_s *psPkt, int nDataLen )
{
	UDPHeader_s *psUdpHdr = psPkt->pb_uTransportHdr.psUDP;
	UDPPort_s *psPort;
	int nSrcPort;
	int nDstPort;

	nSrcPort = ntohs( psUdpHdr->udp_nSrcPort );
	nDstPort = ntohs( psUdpHdr->udp_nDstPort );

//  printk( "Received %d bytes int UDP packet from %d to %d\n", ntohs( psUdpHdr->udp_nSize ), nSrcPort, nDstPort );
	if ( psUdpHdr->udp_nChkSum != 0 && udp_cksum( psPkt ) != 0 )
	{
		printk( "udp_in() invalid checksum %d\n", udp_cksum( psPkt ) );
		free_pkt_buffer( psPkt );
		return;
	}

	LOCK( g_hUDPPortLock );

	psPort = udp_find_port( nDstPort );

	if ( psPort == NULL )
	{
		free_pkt_buffer( psPkt );
//    printk( "udp_in() cant find destination port %d\n", nDstPort );
		UNLOCK( g_hUDPPortLock );
		return;
	}
	udp_enqueue_packet( psPort, psPkt );
	UNLOCK( g_hUDPPortLock );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int udp_add_endpoint( UDPEndPoint_s *psUDPCtrl )
{
	UDPPort_s *psPort;
	UDPEndPoint_s *psEndP;
	int nError;
	char zSrc[64];
	char zDst[64];

	if ( psUDPCtrl->ue_psPort != NULL )
	{
		printk( "Panic: udp_add_endpoint() attempt to add socket twice!\n" );
		return ( -EADDRINUSE );
	}
	LOCK( g_hUDPPortLock );

	psPort = udp_find_port( psUDPCtrl->ue_nLocalPort );

//  printk( "Add endpoint for port %d\n", psUDPCtrl->ue_nLocalPort );
	if ( psPort == NULL )
	{
		psPort = kmalloc( sizeof( UDPPort_s ), MEMF_KERNEL | MEMF_CLEAR );
		if ( NULL == psPort )
		{
			printk( "udp_create_endpoint() out of memory\n" );
			nError = -ENOMEM;
			goto error;
		}
		psPort->up_nPortNum = psUDPCtrl->ue_nLocalPort;
		psPort->up_psNext = g_psFirstUDPPort;
		g_psFirstUDPPort = psPort;
	}
	else
	{
		for ( psEndP = psPort->up_psFirstEndPoint; psEndP != NULL; psEndP = psEndP->ue_psNext )
		{
			if ( IP_SAMEADDR( psEndP->ue_anLocalAddr, psUDPCtrl->ue_anLocalAddr ) && IP_SAMEADDR( psEndP->ue_anRemoteAddr, psUDPCtrl->ue_anRemoteAddr ) && psEndP->ue_nRemotePort == psUDPCtrl->ue_nRemotePort )
			{
				printk( "udp_create_endpoint() port %d is in use\n", psUDPCtrl->ue_nLocalPort );
				nError = -EADDRINUSE;
				goto error;
			}
		}
	}

	format_ipaddress( zSrc, psUDPCtrl->ue_anLocalAddr );
	format_ipaddress( zDst, psUDPCtrl->ue_anRemoteAddr );
//  printk( "udp_add_endpoint() add endpoint for endpoint %s:%d -> %s:%d\n",
//        zSrc, psUDPCtrl->ue_nLocalPort, zDst, psUDPCtrl->ue_nRemotePort );

	psUDPCtrl->ue_psPort = psPort;
	psUDPCtrl->ue_psNext = psPort->up_psFirstEndPoint;
	psPort->up_psFirstEndPoint = psUDPCtrl;

	UNLOCK( g_hUDPPortLock );
	return ( 0 );
      error:
	UNLOCK( g_hUDPPortLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void udp_delete_port( UDPPort_s *psPort )
{
	UDPPort_s **ppsTmp;
	bool bFound = false;

//  printk( "Delete port %d\n", psPort->up_nPortNum );
	for ( ppsTmp = &g_psFirstUDPPort; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->up_psNext )
	{
		if ( *ppsTmp == psPort )
		{
			bFound = true;
			*ppsTmp = psPort->up_psNext;
			kfree( psPort );
			return;
		}
	}
	printk( "Panic: udp_delete_port() failed to find port %d\n", psPort->up_nPortNum );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void udp_delete_endpoint( UDPEndPoint_s *psEndP )
{
	UDPPort_s *psPort = psEndP->ue_psPort;
	SelectRequest_s *psReq;

	LOCK( g_hUDPPortLock );

	if ( psPort != NULL )
	{
		UDPEndPoint_s **ppsTmp;
		bool bFound = false;

		for ( ppsTmp = &psPort->up_psFirstEndPoint; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->ue_psNext )
		{
			if ( *ppsTmp == psEndP )
			{
				*ppsTmp = psEndP->ue_psNext;
				bFound = true;
				break;
			}
		}
		if ( bFound == false )
		{
			printk( "Panic: udp_delete_endpoint() could not find end-point %p in port %p\n", psEndP, psPort );
			goto error;
		}
		delete_net_queue( &psEndP->ue_sPackets );

		if ( psPort->up_psFirstEndPoint == NULL )
		{
			udp_delete_port( psPort );
		}
		for ( psReq = psEndP->ue_psFirstReadSelReq; psReq != NULL; psReq = psReq->sr_psNext )
		{
			psReq->sr_bReady = true;
			UNLOCK( psReq->sr_hSema );
		}
	}
	kfree( psEndP );
      error:
	UNLOCK( g_hUDPPortLock );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int udp_close( Socket_s *psSocket )
{
	if ( psSocket->sk_psUDPEndP == NULL )
	{
		printk( "Painc: udp_close() socket has no udp control struct!\n" );
		return ( -EINVAL );
	}
//  printk( "udp_close() delete endpoint for %d\n", psSocket->sk_psUDPEndP->ue_psPort->up_nPortNum );
	udp_delete_endpoint( psSocket->sk_psUDPEndP );
	psSocket->sk_psUDPEndP = NULL;

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int udp_bind( Socket_s *psSocket, const struct sockaddr *psAddr, int nAddrSize )
{
	UDPEndPoint_s *psUDPCtrl = psSocket->sk_psUDPEndP;
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psAddr;
	ipaddr_t anNullAddr;
	int nError;
	int nPort;
	
	LOCK( g_hUDPPortLock );
	
	if( psUDPCtrl->ue_psPort != NULL )
	{
		printk( "udp_bind() - port already used\n" );
		nError = -EINVAL;
		goto error;
	}

	IP_MAKEADDR( anNullAddr, 0, 0, 0, 0 );

	nPort = ntohs( psInAddr->sin_port );
	if ( nPort == 0 )
	{			// FIXME: Not exectly optimal :(
		int i;

		for ( i = 0; i < 65535 - 1024; ++i )
		{
			static int nNextPort = 1024;	// FIXME: Havock should be a good description if this ting wrap.

			nPort = nNextPort++;
			if ( nNextPort > 65535 )
			{
				nNextPort = 1024;
			}
			if ( udp_find_port( nPort ) == NULL )
			{
				break;
			}
			nPort = 0;
		}
	}

	if ( nPort == 0 )
	{
		nError = -EADDRNOTAVAIL;	// FIXME: I don't think this is right, but couldn't figure out what it was ment to be
		goto error;
	}
//  printk( "bind udp socket to local address %d\n", nPort );
	// FIXME: Check if the local address is valid (Ie. belong's to one of our interfaces)
	IP_COPYADDR( psUDPCtrl->ue_anLocalAddr, psInAddr->sin_addr );
	psUDPCtrl->ue_nLocalPort = nPort;

	IP_COPYADDR( psSocket->sk_anSrcAddr, psInAddr->sin_addr );
	psSocket->sk_nSrcPort = nPort;


//  nError = udp_create_endpoint( ntohs( psInAddr->sin_port ), psInAddr->sin_addr, 0, anNullAddr, &psSocket->sk_psUDPEndP );
	nError = udp_add_endpoint( psUDPCtrl );
      error:
	UNLOCK( g_hUDPPortLock );      	
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int udp_autobind( Socket_s *psSocket )
{
	struct sockaddr sAddr;

	memset( &sAddr, 0, sizeof( sAddr ) );
	return ( udp_bind( psSocket, &sAddr, sizeof( sAddr ) ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int udp_connect( Socket_s *psSocket, const struct sockaddr *psAddr, int nAddrSize )
{
	UDPEndPoint_s *psUDPCtrl = psSocket->sk_psUDPEndP;
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psAddr;
	Route_s *psRoute;
	int nError;
	uint8 anNullAddr[] = { 0, 0, 0, 0 };

	if ( nAddrSize < sizeof( *psInAddr ) )
	{
		return ( -EINVAL );
	}
	if ( psInAddr->sin_family != 0 && psInAddr->sin_family != AF_INET )
	{
		return ( -EAFNOSUPPORT );
	}
	
	LOCK( g_hUDPPortLock );

	if ( IP_SAMEADDR( psInAddr->sin_addr, anNullAddr ) )
	{			// Break a previous connection (realy not sure if this is correct)
		IP_COPYADDR( psUDPCtrl->ue_anRemoteAddr, psInAddr->sin_addr );
		psUDPCtrl->ue_nRemotePort = 0;	// psAddrIn->sin_port;
		UNLOCK( g_hUDPPortLock );
		return ( 0 );
	}

	if ( psUDPCtrl->ue_psPort == NULL )
	{
		if ( udp_autobind( psSocket ) != 0 )
		{
			nError = -EAGAIN;
			goto error;
		}
	}

	/* Unless bound to a device, check the address is reachable */
	if ( psUDPCtrl->ue_acBindToDevice[0] == 0 )
	{

		psRoute = ip_find_route( psInAddr->sin_addr );

		if ( NULL == psRoute )
		{
			kerndbg( KERN_DEBUG, "udp_connect(): Could not find a route to connected address.\n" );
			nError = -ENETUNREACH;
			goto error;
		}

		if ( IP_SAMEADDR( psUDPCtrl->ue_anLocalAddr, anNullAddr ) )
			IP_COPYADDR( psUDPCtrl->ue_anLocalAddr, psRoute->rt_psInterface->ni_anIpAddr );

		ip_release_route( psRoute );
	}

	IP_COPYADDR( psUDPCtrl->ue_anRemoteAddr, psInAddr->sin_addr );
	psUDPCtrl->ue_nRemotePort = ntohs( psInAddr->sin_port );

	UNLOCK( g_hUDPPortLock );
	return ( 0 );

      error:
    UNLOCK( g_hUDPPortLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static ssize_t udp_recvmsg( Socket_s *psSocket, struct msghdr *psMsg, int nFlags )
{
	UDPHeader_s *psUdpHdr;
	IpHeader_s *psIpHdr;
	UDPEndPoint_s *psEndP;
	PacketBuf_s *psPkt;
	ipaddr_t anNullAddr;
	int nTotalSize = 0;
	char *pSrcBuf;
	int nPktSize;
	int nError;
	int i;

	IP_MAKEADDR( anNullAddr, 0, 0, 0, 0 );

	psEndP = psSocket->sk_psUDPEndP;

	if ( psEndP == NULL )
	{
		nError = -EINVAL;
		printk( "Panic: udp_recvmsg() socket has no udp control struct!\n" );
		goto error1;
	}
	if ( psEndP->ue_psPort == NULL )
	{
		nError = -ENOTCONN;
		goto error1;
	}

	if ( psEndP->ue_bNonBlocking )
	{
		psPkt = remove_head_packet( &psEndP->ue_sPackets, 0LL );
	}
	else
	{
		psPkt = remove_head_packet( &psEndP->ue_sPackets, INFINITE_TIMEOUT );
	}


	if ( psPkt == NULL )
	{
		if ( psEndP->ue_bNonBlocking )
		{
			nError = -EWOULDBLOCK;
		}
		else
		{
			nError = -EINTR;
		}
		goto error2;
	}

	psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	psUdpHdr = psPkt->pb_uTransportHdr.psUDP;

	pSrcBuf = ( char * )( psUdpHdr + 1 );
	nPktSize = ntohs( psUdpHdr->udp_nSize ) - sizeof( UDPHeader_s );

	for ( i = 0; i < psMsg->msg_iovlen && nPktSize > 0; ++i )
	{
		int nActualSize = min( psMsg->msg_iov[i].iov_len, nPktSize );

		memcpy( psMsg->msg_iov[i].iov_base, pSrcBuf, nActualSize );

		pSrcBuf += nActualSize;
		nPktSize -= nActualSize;
		nTotalSize += nActualSize;
	}

	if ( psMsg->msg_name != NULL )
	{
		struct sockaddr_in sSrcAddr;

		memset( &sSrcAddr, 0, sizeof( sSrcAddr ) );
		sSrcAddr.sin_family = AF_INET;
		sSrcAddr.sin_port = psUdpHdr->udp_nSrcPort;
		IP_COPYADDR( sSrcAddr.sin_addr, psIpHdr->iph_nSrcAddr );
		if ( psMsg->msg_namelen > sizeof( sSrcAddr ) )
		{
			psMsg->msg_namelen = sizeof( sSrcAddr );
		}
		memcpy( psMsg->msg_name, &sSrcAddr, psMsg->msg_namelen );
	}
	free_pkt_buffer( psPkt );
	return ( nTotalSize );
      error2:
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static ssize_t udp_sendmsg( Socket_s *psSocket, const struct msghdr *psMsg, int nFlags )
{
	UDPEndPoint_s *psUDPCtrl = psSocket->sk_psUDPEndP;
	UDPHeader_s *psUdpHdr;
	IpHeader_s *psIpHdr;
	PacketBuf_s *psPkt;
	Route_s *psRoute;
	struct sockaddr_in *psAddr = ( struct sockaddr_in * )psMsg->msg_name;
	ipaddr_t anDstAddr;
	uint16 nDstPort;
	uint8 anNullAddr[] = { 0, 0, 0, 0 };
	int nTotSize = 16 + 60 + sizeof( UDPHeader_s );
	int nBytsSendt = 0;
	char *pDstBuf;
	int nError;
	int i;

	for ( i = 0; i < psMsg->msg_iovlen; ++i )
	{
		nBytsSendt += psMsg->msg_iov[i].iov_len;
	}
	nTotSize += nBytsSendt;

	if ( nTotSize > 65535 )
	{
		return ( -EMSGSIZE );
	}

	if ( ( psAddr == NULL || IP_SAMEADDR( psAddr->sin_addr, anNullAddr ) ) && IP_SAMEADDR( psUDPCtrl->ue_anRemoteAddr, anNullAddr ) )
	{
		kerndbg( KERN_DEBUG, "udp_sendmsg(): Attempted send without address on unconnected socket\n" );
		return ( -ENOTCONN );
	}
	if ( ( psAddr != NULL && IP_SAMEADDR( psAddr->sin_addr, anNullAddr ) == false ) && IP_SAMEADDR( psUDPCtrl->ue_anRemoteAddr, anNullAddr ) == false )
	{
		kerndbg( KERN_DEBUG, "udp_sendmsg(): Attempted send with address on a connected socket\n" );
		return ( -EISCONN );
	}
	if ( psAddr == NULL || IP_SAMEADDR( psUDPCtrl->ue_anRemoteAddr, anNullAddr ) == false )
	{
		IP_COPYADDR( anDstAddr, psUDPCtrl->ue_anRemoteAddr );
		nDstPort = htons( psUDPCtrl->ue_nRemotePort );
	}
	else
	{
		IP_COPYADDR( anDstAddr, psAddr->sin_addr );
		nDstPort = psAddr->sin_port;
	}
	if ( psUDPCtrl->ue_psPort == NULL )
	{
		if ( udp_autobind( psSocket ) != 0 )
		{
			return ( -EAGAIN );
		}
	}

	if ( nDstPort == 0 )
	{
		printk( "udp_sendmsg() attempt to send to port 0\n" );
		return ( -EINVAL );
	}
	if ( psUDPCtrl->ue_nLocalPort == 0 )
	{
		printk( "Error: udp_sendmsg() attempt to send from port 0\n" );
		return ( -EINVAL );
	}

	if ( psUDPCtrl->ue_acBindToDevice[0] == 0 )
		psRoute = ip_find_route( anDstAddr );
	else
		psRoute = ip_find_device_route( psUDPCtrl->ue_acBindToDevice );

	if ( psRoute == NULL )
	{
		if ( psUDPCtrl->ue_acBindToDevice[0] == 0 )
		{
			USE_FORMAT_IP( 1 );

			FORMAT_IP( anDstAddr, 0 );

			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Could not find route for address %s\n", FORMATTED_IP( 0 ) );
			return ( -ENETUNREACH );
		}
		else
		{
			kerndbg( KERN_DEBUG, "udp_sendto(): Could not find device route for %s.\n", psUDPCtrl->ue_acBindToDevice );

			return ( -ENETDOWN );
		}
	}

	/* Allocate a packet buffer for the message */
	psPkt = alloc_pkt_buffer( nTotSize );

	if ( psPkt == NULL )
	{
		nError = -ENOMEM;
		goto error;
	}

	psPkt->pb_uNetworkHdr.pRaw = psPkt->pb_pData + 16;
	psPkt->pb_uTransportHdr.pRaw = psPkt->pb_uNetworkHdr.pRaw + sizeof( IpHeader_s );
	psPkt->pb_nSize = sizeof( IpHeader_s ) + sizeof( UDPHeader_s ) + nBytsSendt;

	psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	psUdpHdr = psPkt->pb_uTransportHdr.psUDP;


	IP_COPYADDR( psIpHdr->iph_nDstAddr, anDstAddr );
	IP_COPYADDR( psIpHdr->iph_nSrcAddr, psRoute->rt_psInterface->ni_anIpAddr );

	psUdpHdr->udp_nSrcPort = htons( psUDPCtrl->ue_nLocalPort );
	psUdpHdr->udp_nDstPort = nDstPort;
	psUdpHdr->udp_nSize = htons( sizeof( UDPHeader_s ) + nBytsSendt );

	pDstBuf = ( char * )( psUdpHdr + 1 );

	for ( i = 0; i < psMsg->msg_iovlen > 0; ++i )
	{
		memcpy( pDstBuf, psMsg->msg_iov[i].iov_base, psMsg->msg_iov[i].iov_len );
		pDstBuf += psMsg->msg_iov[i].iov_len;
	}

	psIpHdr->iph_nHdrSize = 5;
	psIpHdr->iph_nVersion = 4;
	psIpHdr->iph_nTypeOfService = 0;
	psIpHdr->iph_nPacketSize = htons( psPkt->pb_nSize );
	psIpHdr->iph_nFragOffset = 0;
	psIpHdr->iph_nTimeToLive = IP_TTL;
	psIpHdr->iph_nProtocol = IPT_UDP;

	psUdpHdr->udp_nChkSum = 0;
	psUdpHdr->udp_nChkSum = udp_cksum( psPkt );

	ip_send_via( psPkt, psRoute );

	ip_release_route( psRoute );

	return ( nBytsSendt );


      error:
	ip_release_route( psRoute );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int udp_getsockname( Socket_s *psSocket, struct sockaddr *psName, int *pnNameLen )
{
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psName;
	int nError;

	psInAddr->sin_family = AF_INET;
	psInAddr->sin_port = htons( psSocket->sk_psUDPEndP->ue_nLocalPort );
	IP_COPYADDR( psInAddr->sin_addr, psSocket->sk_psUDPEndP->ue_anLocalAddr );
	nError = 0;

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int udp_open( Socket_s *psSocket )
{
	int nError;

	psSocket->sk_psOps = &g_sUDPOperations;

	psSocket->sk_psUDPEndP = kmalloc( sizeof( UDPEndPoint_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( psSocket->sk_psUDPEndP == NULL )
	{
		kerndbg( KERN_FATAL, "udp_open(): No memory for UDP control struct\n" );
		nError = -ENOMEM;

		goto error;
	}

	init_net_queue( &psSocket->sk_psUDPEndP->ue_sPackets );

	return ( 0 );
      error:
	return ( nError );
}

static int udp_add_select( Socket_s *psSocket, SelectRequest_s *psReq )
{
	UDPEndPoint_s *psUDPCtrl = psSocket->sk_psUDPEndP;
	int nError;

	LOCK( g_hUDPPortLock );
	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		psReq->sr_psNext = psUDPCtrl->ue_psFirstReadSelReq;
		psUDPCtrl->ue_psFirstReadSelReq = psReq;

		if ( psUDPCtrl->ue_sPackets.nq_nCount > 0 )
		{
			psReq->sr_bReady = true;
			UNLOCK( psReq->sr_hSema );
		}
		nError = 0;
		break;
	case SELECT_WRITE:
		psReq->sr_bReady = true;
		UNLOCK( psReq->sr_hSema );
		nError = 0;
		break;
	case SELECT_EXCEPT:
		nError = 0;
		break;
	default:
		kerndbg( KERN_WARNING, "udp_add_select(): Unknown mode %d\n", psReq->sr_nMode );
		nError = -EINVAL;
		break;
	}
	UNLOCK( g_hUDPPortLock );
	return ( nError );
}

static int udp_rem_select( Socket_s *psSocket, SelectRequest_s *psReq )
{
	UDPEndPoint_s *psUDPCtrl = psSocket->sk_psUDPEndP;
	SelectRequest_s **ppsTmp = NULL;
	int nError;

	LOCK( g_hUDPPortLock );

	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		ppsTmp = &psUDPCtrl->ue_psFirstReadSelReq;
		nError = 0;
		break;
	case SELECT_WRITE:
		nError = 0;
		break;
	case SELECT_EXCEPT:
		nError = 0;
		break;
	default:
		kerndbg( KERN_WARNING, "udp_rem_select(): unknown mode %d\n", psReq->sr_nMode );
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
	UNLOCK( g_hUDPPortLock );
	return ( nError );
}

static int udp_setsockopt( bool bKernel, Socket_s *psSocket, int nProtocol, int nOptName, const void *pOptVal, int nOptLen )
{
	UDPEndPoint_s *psUDPCtrl = psSocket->sk_psUDPEndP;
	int nError = -ENOSYS;

	switch( nOptName )
	{
		case SO_REUSEADDR:
		{
			nError = 0;
			break;
		}

		case SO_BROADCAST:
		{
			nError = 0;
			break;
		}

		case SO_BINDTODEVICE:
		{
			if( pOptVal == NULL || nOptLen > IFNAMSIZ )
			{
				kerndbg( KERN_DEBUG, __FUNCTION__ "(): Invalid options for SO_BINDTODEVICE.\n" );

				nError = -EINVAL;
				break;
			}

			if( bKernel )
			{
				strncpy( psUDPCtrl->ue_acBindToDevice, ( char * )pOptVal, nOptLen );
			}
			else
			{
				strncpy_from_user( psUDPCtrl->ue_acBindToDevice, ( char * )pOptVal, nOptLen );
			}

			psUDPCtrl->ue_acBindToDevice[IFNAMSIZ - 1] = 0;
			if ( psUDPCtrl->ue_acBindToDevice[0] != 0 )
			{
				kerndbg( KERN_DEBUG, __FUNCTION__ "(): Binding to interface %s.\n", psUDPCtrl->ue_acBindToDevice );
			}
			else
			{
				kerndbg( KERN_DEBUG, __FUNCTION__ "(): Reverting to normal routing.\n" );
			}

			nError = 0;
			break;
		}
		default:
			kerndbg( KERN_WARNING, "udp_setsockopt() got unknown option %i\n", nOptName );
	}

	return nError;
}

static int udp_set_fflags( Socket_s *psSocket, uint32 nFlags )
{
	UDPEndPoint_s *psUDPCtrl = psSocket->sk_psUDPEndP;

	if ( nFlags & O_NONBLOCK )
	{
		psUDPCtrl->ue_bNonBlocking = true;
	}
	else
	{
		psUDPCtrl->ue_bNonBlocking = false;
	}
	return ( 0 );
}


static int udp_ioctl( Socket_s *psSocket, int nCmd, void *pBuffer, bool bFromKernel )
{
	UDPEndPoint_s *psUDPCtrl = psSocket->sk_psUDPEndP;
	int nError = 0;
	
	kassertw( psUDPCtrl != NULL );

	switch ( nCmd )
	{
	case FIONREAD:
		if ( bFromKernel )
		{
			*( ( int * )pBuffer ) = psUDPCtrl->ue_sPackets.nq_nCount;
		}
		else
		{
			nError = memcpy_to_user( pBuffer, &psUDPCtrl->ue_sPackets.nq_nCount, sizeof( int ) );
		}
		break;
	default:
		nError = -ENOSYS;
		break;
	}
	return ( nError );
}

SocketOps_s g_sUDPOperations = {
	udp_open,
	udp_close,
	NULL,			// so_shutdown
	udp_bind,
	udp_connect,
	udp_getsockname,
	NULL,			// so_getpeername
	udp_recvmsg,
	udp_sendmsg,
	udp_add_select,
	udp_rem_select,
	NULL,			// so_listen,
	NULL,			// so_accept
	udp_setsockopt,
	NULL,				// so_getsockopt
	udp_set_fflags,		// so_set_fflags
	udp_ioctl			// so_ioctl
};

int init_udp( void )
{
	g_hUDPPortLock = create_semaphore( "udp_portlist", 1, SEM_RECURSIVE );
	if ( g_hUDPPortLock < 0 )
	{
		return ( g_hUDPPortLock );
	}
	return ( 0 );
}
