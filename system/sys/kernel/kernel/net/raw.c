/*	Authors:	Ville Kallioniemi, <ville.kallioniemi@abo.fi>	*/

#include <posix/errno.h>
#include <posix/uio.h>

#include <atheos/kernel.h>
#include <atheos/socket.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include <net/net.h>
#include <net/in.h>
#include <net/ip.h>
#include <net/if_ether.h>
#include <net/raw.h>

extern SocketOps_s g_sRawOperations;
static RawPort_s *g_psFirstRawPort = NULL;
static sem_id g_hRawPortLock = -1;

// we associate each port with a protocol, the ports have a list of the endpoints(sockets)

RawPort_s *raw_find_port( uint8 a_nIPProtocol )
{
	RawPort_s *psRawPort = NULL;

	// printk( "RAW_FIND_PORT( %d ) called\n", a_nIPProtocol );     
	// try to find a port that would match the protocol
	LOCK( g_hRawPortLock );
	for ( psRawPort = g_psFirstRawPort; psRawPort != NULL; psRawPort = psRawPort->rp_psNext )
	{
		if ( psRawPort->rp_nProtocol == a_nIPProtocol )
		{
			kerndbg( KERN_DEBUG, "RAW_FIND_PORT(): found a matching port for protocol: %d\n", a_nIPProtocol );
			UNLOCK( g_hRawPortLock );
			return ( psRawPort );
		}
	}
	kerndbg( KERN_DEBUG, "RAW_FIND_PORT(): did not find a matching port for protocol: %d\n", a_nIPProtocol );
	UNLOCK( g_hRawPortLock );
	return NULL;
}


static int raw_add_port( RawPort_s *a_psRawPort )
{
	LOCK( g_hRawPortLock );
	a_psRawPort->rp_psNext = g_psFirstRawPort;
	g_psFirstRawPort = a_psRawPort;
	kerndbg( KERN_DEBUG, "RAW_ADD_PORT(): added a port for protocol: %d\n", a_psRawPort->rp_nProtocol );
	UNLOCK( g_hRawPortLock );
	return ( 0 );
}


static void raw_delete_port( RawPort_s *a_psRawPort )
{
	RawPort_s **ppsTempPort = NULL;
	bool bFound = false;

	kerndbg( KERN_DEBUG, "RAW_DELETE_PORT() called for port: %d\n", a_psRawPort->rp_nProtocol );
	// LOCK( g_hRawPortLock ); Called only from inside delete_endpoint
	for ( ppsTempPort = &g_psFirstRawPort; *ppsTempPort != NULL; ppsTempPort = &( *ppsTempPort )->rp_psNext )
	{
		if ( *ppsTempPort == a_psRawPort )
		{
			kerndbg( KERN_DEBUG_LOW, "RAW_DELETE_PORT() found the port! freeing...\n" );
			bFound = true;
			*ppsTempPort = a_psRawPort->rp_psNext;
			kfree( a_psRawPort );
			break;
		}
	}
	// UNLOCK( g_hRawPortLock );

	if ( bFound == false )
	{
		kerndbg( KERN_WARNING, "RAW_DELETE_PORT() could not find port: %d\n", a_psRawPort->rp_nProtocol );
		return;
	}
	return;
}

static void raw_dump_endpoint_info( RawEndPoint_s *a_psRawEndPoint )
{
	kerndbg( KERN_DEBUG_LOW, "RAW_DUMP_ENDPOINT_INFO():------------------------\n" );
	kerndbg( KERN_DEBUG_LOW, "RawEndPoint->\n" );
	kerndbg( KERN_DEBUG_LOW, "re_nProtocolt: %d\n", a_psRawEndPoint->re_nProtocol );
	kerndbg( KERN_DEBUG_LOW, "-------------------------------------------------\n" );
}

static void raw_delete_endpoint( RawEndPoint_s *a_psRawEndPoint )
{
	RawPort_s *psRawPort = a_psRawEndPoint->re_psPort;

	if ( a_psRawEndPoint == NULL )
	{
		kerndbg( KERN_WARNING, "RAW_DELETE_ENDPOINT(): called with a NULL POINTER!\n" );
		return;
	}

	LOCK( g_hRawPortLock );
	if ( psRawPort != NULL )
	{
		RawEndPoint_s **ppsRawEndPoint;
		bool bFound = false;

		kerndbg( KERN_DEBUG, "RAW_DELETE_ENDPOINT(): endpoint exists for port %d ?, searching...\n", psRawPort->rp_nProtocol );
		for ( ppsRawEndPoint = &psRawPort->rp_psFirstEndPoint; *ppsRawEndPoint != NULL; ppsRawEndPoint = &( *ppsRawEndPoint )->re_psNext )
		{
			if ( *ppsRawEndPoint == a_psRawEndPoint )
			{
				*ppsRawEndPoint = a_psRawEndPoint->re_psNext;
				bFound = true;
				kerndbg( KERN_DEBUG_LOW, "RAW_DELETE_ENDPOINT(): found endpoint!!!.\n" );
				break;
			}
		}

		if ( bFound == false )
		{
			kerndbg( KERN_WARNING, "RAW_DELETE_PORT(): ERROR: could not find end-point %p in port %p\n", a_psRawEndPoint, psRawPort );
			UNLOCK( g_hRawPortLock );
			// FIXME!
			return;
		}
		// should we DRAIN the queue? It seems that at the moment, the kernel memory space just gets littered with packets when a socket with pending packets gets closed-VK
		// should the draining be implemented in delete_net_queue???
		delete_net_queue( &a_psRawEndPoint->re_sPackets );

		if ( psRawPort->rp_psFirstEndPoint == NULL )
		{
			kerndbg( KERN_DEBUG, "RAW_DELETE_PORT(): LAST endpoind for protocol: %d, deleting port.\n", psRawPort->rp_nProtocol );
			raw_delete_port( psRawPort );
		}
	}

	kfree( a_psRawEndPoint );
	UNLOCK( g_hRawPortLock );
}

static void raw_add_endpoint( RawPort_s *a_psRawPort, RawEndPoint_s *a_psRawEndPoint )
{
	if ( a_psRawPort == NULL || a_psRawEndPoint == NULL )
	{
		kerndbg( KERN_WARNING, "RAW_ADD_ENDPOINT(): NULL-POINTER-ERROR: port:%p , endpoint:%p\n", a_psRawPort, a_psRawEndPoint );
		return;
	}
	kerndbg( KERN_DEBUG_LOW, "RAW_ADD_ENDPOINT: attaching an endpoint to port nr: %d\n", a_psRawPort->rp_nProtocol );

	LOCK( g_hRawPortLock );
	a_psRawEndPoint->re_psNext = a_psRawPort->rp_psFirstEndPoint;
	a_psRawPort->rp_psFirstEndPoint = a_psRawEndPoint;
	UNLOCK( g_hRawPortLock );
	return;
}


static int raw_bind( Socket_s *a_psSocket, const struct sockaddr *a_psAddress, int a_nAddressSize )
{
	RawEndPoint_s *psRawEndPoint = a_psSocket->sk_psRawEndP;
	RawPort_s *psRawPort = NULL;
	struct sockaddr_in *psInAddress = ( struct sockaddr_in * )a_psAddress;

	if ( ( psRawPort = raw_find_port( a_psSocket->sk_nProto ) ) != NULL )
	{
		kerndbg( KERN_WARNING, "RAW_BIND: a port for protocol: %d already exists\n", a_psSocket->sk_nProto );
	}
	else
	{
		kerndbg( KERN_DEBUG_LOW, "RAW_BIND: creating a port for protocol: %d\n", a_psSocket->sk_nProto );
		psRawPort = kmalloc( sizeof( RawPort_s ), MEMF_KERNEL | MEMF_CLEAR );
		if ( psRawPort == NULL )
		{
			kerndbg( KERN_FATAL, "RAW_BIND: no memory for raw port struct\n" );
			return ( -ENOMEM );
		}
		psRawPort->rp_nProtocol = a_psSocket->sk_nProto;
		raw_add_port( psRawPort );
	}
	// link the port to the endpoint
	psRawEndPoint->re_psPort = psRawPort;
	// and vice versa
	raw_add_endpoint( psRawPort, psRawEndPoint );

	IP_COPYADDR( psRawEndPoint->re_anLocalAddr, psInAddress->sin_addr );
	IP_COPYADDR( a_psSocket->sk_anSrcAddr, psInAddress->sin_addr );

	return ( -EOK );
}

static int raw_autobind( Socket_s *a_psSocket )
{
	struct sockaddr sAddress;

	memset( &sAddress, 0, sizeof( sAddress ) );
	return ( raw_bind( a_psSocket, &sAddress, sizeof( sAddress ) ) );
}

static int raw_getsockname( Socket_s *a_psSocket, struct sockaddr *a_psName, int *a_pnNameLength )
{
	struct sockaddr_in *psInAddress = ( struct sockaddr_in * )a_psName;

	psInAddress->sin_family = AF_INET;
	//psInAddress->sin_port   = htons( a_psSocket->sk_psRawEndP->re_nLocalPort );
	IP_COPYADDR( psInAddress->sin_addr, a_psSocket->sk_psRawEndP->re_anLocalAddr );
	return ( -EOK );
}



static int raw_connect( Socket_s *a_psSocket, const struct sockaddr *a_psAddress, int a_nAddressSize )
{
	RawEndPoint_s *psRawEndPoint = a_psSocket->sk_psRawEndP;
	RawPort_s *psRawPort = NULL;
	struct sockaddr_in *psInAddress = ( struct sockaddr_in * )a_psAddress;
	Route_s *psRoute;
	ipaddr_t LocalAddress;

	kerndbg( KERN_DEBUG_LOW, "RAW_CONNECT() called with: sin_family: %d\t sin_port: %d\t .\n", psInAddress->sin_family, psInAddress->sin_port );
	if ( a_nAddressSize < sizeof( *psInAddress ) )
	{
		return ( -EINVAL );
	}

	if ( psInAddress->sin_family != 0 && psInAddress->sin_family != AF_INET )
	{
		return ( -EAFNOSUPPORT );
	}
	if ( psRawEndPoint->re_anRemoteAddr )
	{
		return ( -EISCONN );
	}
	// Check which interface can deliver to the given address
	psRoute = ip_find_route( psInAddress->sin_addr );
	if ( psRoute == NULL )
	{
		kerndbg( KERN_WARNING, "RAW_CONNECT() ERROR, could not find a route.\n" );
		return ( -ENETUNREACH );
	}
	IP_COPYADDR( LocalAddress, psRoute->rt_psInterface->ni_anIpAddr );
	ip_release_route( psRoute );

	// the sockaddr_in->sin_port can be used to specify the protocol of the socket
	// if it is not specified, the protocol given with socket() is used
	if ( psInAddress->sin_port != 0 )
	{
		kerndbg( KERN_WARNING, "RAW_CONNECT(): changing protocol from %d to %d.\n", a_psSocket->sk_nProto, psInAddress->sin_port );
		psRawEndPoint->re_nProtocol = psInAddress->sin_port;
	}
	// port-number == protocol number!
	// if a port for this protocol does not exist yet, we have to create one
	psRawPort = raw_find_port( psRawEndPoint->re_nProtocol );
	if ( psRawPort == NULL )
	{
		kerndbg( KERN_DEBUG, "RAW_CONNECT() Port does not exist yet! Creating...\n" );
		psRawPort = kmalloc( sizeof( RawPort_s ), MEMF_KERNEL | MEMF_CLEAR );
		if ( psRawPort == NULL )
		{
			kerndbg( KERN_FATAL, "RAW_CONNECT() no memory for raw control struct\n" );
			return ( -ENOMEM );
		}
		psRawPort->rp_nProtocol = psRawEndPoint->re_nProtocol;
		raw_add_port( psRawPort );
	}
	// link the port to the endpoint
	psRawEndPoint->re_psPort = psRawPort;
	// and vice versa
	raw_add_endpoint( psRawPort, psRawEndPoint );


	IP_COPYADDR( psRawEndPoint->re_anRemoteAddr, psInAddress->sin_addr );
	IP_COPYADDR( psRawEndPoint->re_anLocalAddr, LocalAddress );

	return ( 0 );
}



static int raw_setsockopt( bool a_bFromKernel, Socket_s *a_psSocket, int a_nProtocol, int a_nOptionName, const void *a_pOptionValue, int a_nOptionLength )
{
	RawEndPoint_s *psRawEndPoint = a_psSocket->sk_psRawEndP;

	switch ( a_nOptionName )
	{
	case IP_HDRINCL:
		{
			psRawEndPoint->re_nCreateHeader = *( ( int * )a_pOptionValue );
			kerndbg( KERN_DEBUG, "RAW_SETSOCKOPT() IP_HDRINCL set for: %d\n", psRawEndPoint->re_nCreateHeader );
			break;
		}
	default:
		{
			kerndbg( KERN_WARNING, "RAW_SETSOCKOPT() unknown option name: %d\n", a_nOptionName );
			return ( -EINVAL );
		}
	}
	return ( -EOK );
}



static ssize_t raw_recvmsg( Socket_s *a_psSocket, struct msghdr *a_psMsg, int a_nFlags )
{
	IpHeader_s *psIpHeader;
	char *pRawIpHeader;
	RawEndPoint_s *psRawEndPoint;
	PacketBuf_s *psPacket;
	int nTotalSize = 0;
	char *pSourceBuffer;
	int nPacketSize;
	int i;

	psRawEndPoint = a_psSocket->sk_psRawEndP;
	if ( psRawEndPoint == NULL )
	{
		kerndbg( KERN_WARNING, "RAW_RECVMSG() socket has no raw control struct!\n" );
		return ( -EINVAL );
	}

	if ( psRawEndPoint->re_psPort == NULL )
	{
		kerndbg( KERN_WARNING, "RAW_RECVMSG() port MISSING!, not connected!" );
		return ( -ENOTCONN );
	}


	if ( psRawEndPoint->re_bNonBlocking )
	{
		kerndbg( KERN_DEBUG, "RAW_RECVMSG(): set for non-blocking.\n" );
		psPacket = remove_head_packet( &psRawEndPoint->re_sPackets, 0LL );
	}
	else
	{
		kerndbg( KERN_DEBUG, "RAW_RECVMSG(): set for blocking.\n" );
		psPacket = remove_head_packet( &psRawEndPoint->re_sPackets, INFINITE_TIMEOUT );
	}

	kerndbg( KERN_DEBUG_LOW, "RAW_RECVMSG(): remove_head_packet() returned.\n" );

	if ( psPacket == NULL )
	{
		if ( psRawEndPoint->re_bNonBlocking )
		{
			return ( -EWOULDBLOCK );
		}
		else
		{
			return ( -EINTR );
		}
	}

	psIpHeader = psPacket->pb_uNetworkHdr.psIP;
	pRawIpHeader = ( char * )psPacket->pb_uNetworkHdr.pRaw;
	if ( ( ( char * )psIpHeader ) != pRawIpHeader )
	{
		kerndbg( KERN_DEBUG, "RAW_RECVMSG(): inconsistent pointers!!!\n" );
		kerndbg( KERN_DEBUG_LOW, "psIpHeader = %p\n", psIpHeader );
		kerndbg( KERN_DEBUG_LOW, "pRawIpHeader = %p\n", pRawIpHeader );
	}
	// We always return the IP header 
	pSourceBuffer = ( char * )( pRawIpHeader );
	nPacketSize = ntohs( psIpHeader->iph_nPacketSize );
	kerndbg( KERN_DEBUG_LOW, "RAW_RECVMSG() nPacketSize = %d\n", nPacketSize );

	for ( i = 0; i < a_psMsg->msg_iovlen && nPacketSize > 0; ++i )
	{
		int nActualSize = min( a_psMsg->msg_iov[i].iov_len, nPacketSize );

		memcpy( a_psMsg->msg_iov[i].iov_base, pSourceBuffer, nActualSize );

		kerndbg( KERN_DEBUG_LOW, "RAW_RECVMSG(): COPYING message: %d. pSourceBuffer: %p, nPacketSize: %d, nTotalSize: %d.\n", i, pSourceBuffer, nPacketSize, nTotalSize );
		pSourceBuffer += nActualSize;
		nPacketSize -= nActualSize;
		nTotalSize += nActualSize;
	}

	if ( a_psMsg->msg_name != NULL )
	{
		struct sockaddr_in sSourceAddress;

		memset( &sSourceAddress, 0, sizeof( sSourceAddress ) );

		sSourceAddress.sin_family = AF_INET;
		sSourceAddress.sin_port = psIpHeader->iph_nProtocol;
		IP_COPYADDR( sSourceAddress.sin_addr, psIpHeader->iph_nSrcAddr );

		if ( a_psMsg->msg_namelen > sizeof( sSourceAddress ) )
		{
			a_psMsg->msg_namelen = sizeof( sSourceAddress );
		}
		memcpy( a_psMsg->msg_name, &sSourceAddress, a_psMsg->msg_namelen );
	}
	free_pkt_buffer( psPacket );
	kerndbg( KERN_DEBUG_LOW, "RAW_RECVMSG() received: %d bytes.\n", nTotalSize );
	return ( nTotalSize );

}


static ssize_t raw_sendmsg( Socket_s *a_psSocket, const struct msghdr *a_psMsg, int a_nFlags )
{
	// This whole function basically duplicates and bypasses ip_send, which is also what udp does too.
	// Both of them should just use ip_send -vk
	RawEndPoint_s *psRawEndPoint = a_psSocket->sk_psRawEndP;
	IpHeader_s *psIPHeader;
	PacketBuf_s *psPacket;
	struct sockaddr_in *psAddress = ( struct sockaddr_in * )a_psMsg->msg_name;

	ipaddr_t anRemoteAddress;
	ipaddr_t anLocalAddress;
	int nRemotePort;
	uint8 nProtocol;
	Route_s *psRoute = NULL;

	char *pDestinationBuffer;
	uint8 anNullAddress[] = { 0, 0, 0, 0 };
	int nBytesSent = 0;

	char zBuffer[128];
	int nTotalSize = 16 + 60;	// These come from udp, where do these sizes come from? should use defines for header lengths
	int i;

	raw_dump_endpoint_info( psRawEndPoint );
	// Count the size of the message
	for ( i = 0; i < a_psMsg->msg_iovlen; ++i )
	{
		nBytesSent += a_psMsg->msg_iov[i].iov_len;
	}

	kerndbg( KERN_DEBUG_LOW, "RAW_SENDMSG() trying to send: %d bytes \n", nBytesSent );

	nTotalSize += nBytesSent;
	// Fragmenting isn't supported yet
	if ( nTotalSize > 65535 )
	{
		return ( -EMSGSIZE );
	}
	// Check for FOUL input/preconditions
	// the socket has to be connected or the user has to provide an address to send to
	if ( ( psAddress == NULL || IP_SAMEADDR( psAddress->sin_addr, anNullAddress ) ) && IP_SAMEADDR( psRawEndPoint->re_anRemoteAddr, anNullAddress ) )
	{
		kerndbg( KERN_WARNING, "RAW_SENDMSG() attempt to send without address to a not connected socket\n" );
		return ( -ENOTCONN );
	}
	// One cannot sendto() to an already connected socket
	if ( ( psAddress != NULL && IP_SAMEADDR( psAddress->sin_addr, anNullAddress ) == false ) && IP_SAMEADDR( psRawEndPoint->re_anRemoteAddr, anNullAddress ) == false )
	{
		kerndbg( KERN_WARNING, "RAW_SENDMSG(): attempt to send with address to a connected socket\n" );
		return ( -EISCONN );
	}
	// a connected socket uses the allready set address, a non-connected socket uses the address provided with sendto
	// The portnumber is always the same as the protocol number
	if ( ( psAddress == NULL ) || ( IP_SAMEADDR( psRawEndPoint->re_anRemoteAddr, anNullAddress ) == false ) )
	{
		IP_COPYADDR( anRemoteAddress, psRawEndPoint->re_anRemoteAddr );
		nRemotePort = psRawEndPoint->re_nProtocol;
		kerndbg( KERN_DEBUG_LOW, "RAW_SENDMSG: connected socket nRemotePort: %d\n", nRemotePort );
	}
	else
	{
		IP_COPYADDR( anRemoteAddress, psAddress->sin_addr );
		if ( psAddress->sin_port == 0 )
		{
			nRemotePort = psRawEndPoint->re_nProtocol;
		}
		else
		{
			nRemotePort = psAddress->sin_port;
		}
		kerndbg( KERN_DEBUG, "RAW_SENDMSG: non-connected socket nRemotePort: %d\n", nRemotePort );
	}
	nProtocol = nRemotePort;

	if ( psRawEndPoint->re_psPort == NULL )
	{
		kerndbg( KERN_DEBUG_LOW, "RAW_SENDMSG: creating a port for this protocol\n" );
		if ( raw_autobind( a_psSocket ) != 0 )
		{
			return ( -EAGAIN );
		}
	}
	// Currently dont route doesn't work FIXME!!! 
	// How should dont route work ???
	if ( psRawEndPoint->re_bRoute == true )
	{
		psRoute = ip_find_route( anRemoteAddress );
		format_ipaddress( zBuffer, anRemoteAddress );
		if ( psRoute == NULL )
		{
			kerndbg( KERN_WARNING, "RAW_RECVMSG: raw_sendto()? Could not find route for address %s\n", zBuffer );
			return ( -ENETUNREACH );
		}
		kerndbg( KERN_DEBUG, "RAW_RECVMSG: raw_sendto()? Found a route for address %s\n", zBuffer );
		IP_COPYADDR( anLocalAddress, psRoute->rt_psInterface->ni_anIpAddr );
		// How long should the route be held ???
	}
	else
	{

		// IP_COPYADDR( anSourceAddress, WHERE? );
	}

	// Set up a packet buffer for sending
	psPacket = alloc_pkt_buffer( nTotalSize );
	if ( psPacket == NULL )
	{
		kerndbg( KERN_WARNING, "RAW_SENDMSG(): packet buffer allocation failed.\n" );
		ip_release_route( psRoute );
		return ( -ENOMEM );
	}
	psPacket->pb_uNetworkHdr.pRaw = psPacket->pb_pData + 16;	// why 16 again? 
	psPacket->pb_uTransportHdr.pRaw = psPacket->pb_uNetworkHdr.pRaw + sizeof( IpHeader_s );
	psPacket->pb_nSize = sizeof( IpHeader_s ) + nBytesSent;

	psIPHeader = psPacket->pb_uNetworkHdr.psIP;

	// If the user has set IP_HDRINCL, we can skip creating the IP-header (the user provides one)
	if ( psRawEndPoint->re_nCreateHeader == 1 )
	{
		kerndbg( KERN_DEBUG, "RAW_SENDMSG() IP_HDRINCL set, creating the ip-header is left up to the user...yikes\n" );
		pDestinationBuffer = (char*)psPacket->pb_uNetworkHdr.pRaw;
	}
	else
	{
		kerndbg( KERN_DEBUG_LOW, "RAW_SENDMSG() creating IP-header.\n" );
		pDestinationBuffer = (char*)psPacket->pb_uTransportHdr.pRaw;
		psIPHeader->iph_nHdrSize = 5;	// without options
		psIPHeader->iph_nVersion = 4;
		psIPHeader->iph_nTypeOfService = 0;
		psIPHeader->iph_nPacketSize = htons( psPacket->pb_nSize );
		psIPHeader->iph_nFragOffset = 0;
		psIPHeader->iph_nTimeToLive = 255;
		psIPHeader->iph_nProtocol = nProtocol;
		IP_COPYADDR( psIPHeader->iph_nDstAddr, anRemoteAddress );
		IP_COPYADDR( psIPHeader->iph_nSrcAddr, anLocalAddress );
	}

	// Copy the message to the send buffer
	for ( i = 0; i < a_psMsg->msg_iovlen > 0; ++i )
	{
		memcpy( pDestinationBuffer, a_psMsg->msg_iov[i].iov_base, a_psMsg->msg_iov[i].iov_len );
		pDestinationBuffer += a_psMsg->msg_iov[i].iov_len;
	}

// <FIXME>
	// for now we bypass the  ip_send and just send it ourselves 
	// DONT ROUTE doesn't work
	psIPHeader->iph_nCheckSum = 0;
	psIPHeader->iph_nCheckSum = ip_fast_csum( ( uint8 * )psIPHeader, psIPHeader->iph_nHdrSize );
	psPacket->pb_nProtocol = ETH_P_IP;
	send_packet( psRoute, psIPHeader->iph_nDstAddr, psPacket );
// </FIXME>     

	ip_release_route( psRoute );

	return ( nBytesSent );
}



static void raw_enqueue_packet( RawPort_s *a_psRawPort, PacketBuf_s *a_psPacket )
{
	RawEndPoint_s *psRawEndPoint;
	PacketBuf_s *psClonedPacket;
	int nCounter = 0;

	// Dispatch the packets to endpoints(sockets)
	for ( psRawEndPoint = a_psRawPort->rp_psFirstEndPoint; psRawEndPoint != NULL; psRawEndPoint = psRawEndPoint->re_psNext )
	{

		// This can be all done smarter after clone is working
		if ( psRawEndPoint->re_psNext != NULL )
		{
			psClonedPacket = clone_pkt_buffer( a_psPacket );
		}
		else
		{
			kerndbg( KERN_DEBUG_LOW, "RAW_ENQUEUE_PACKET() last endpoint reached.\n" );
			psClonedPacket = a_psPacket;
		}

		// If weve got a packet send it
		if ( psClonedPacket == NULL )
		{
			kerndbg( KERN_WARNING, "RAW_ENQUEUE_PACKET() FAILED: no packet to enqueue.\n" );
			break;
		}
		// we dont deliver to crowded queues
		if ( psRawEndPoint->re_sPackets.nq_nCount > 16 )
		{
			kerndbg( KERN_WARNING, "RAW_ENQUEUE_PACKET(): a queue for port %d is FULL.\n", a_psRawPort->rp_nProtocol );
			if ( psRawEndPoint->re_psNext == NULL )
			{
				kerndbg( KERN_DEBUG, "RAW_ENQUEUE_PACKET(): last endpoint was full, deleting the packet.\n" );
				free_pkt_buffer( psClonedPacket );
				return;
			}
		}
		else
		{
			kerndbg( KERN_DEBUG_LOW, "RAW_ENQUEUE_PACKET() enqueueing a packet.\n " );
			enqueue_packet( &psRawEndPoint->re_sPackets, psClonedPacket );
			nCounter++;
		}
	}

	kerndbg( KERN_DEBUG_LOW, "RAW_ENQUEUE(): INFO: enqueued to %d endpoints.\n", nCounter );
	return;
}



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void raw_in( PacketBuf_s *a_psPacket, int a_nDataLength )
{

	IpHeader_s *psIpHdr = a_psPacket->pb_uNetworkHdr.psIP;
	RawPort_s *psRawPort = NULL;

	kerndbg( KERN_DEBUG_LOW, "RAW_IN() called. Packet: %p, DataLength: %d\n", a_psPacket, a_nDataLength );

	psRawPort = raw_find_port( psIpHdr->iph_nProtocol );
	if ( psRawPort == NULL )
	{
		kerndbg( KERN_WARNING, "RAW_IN() failed: port not found.\n" );
		// THIS WILL CHANGE after clone is working (we'll clone only if a port exists)
		free_pkt_buffer( a_psPacket );
		return;
	}
	else
	{
		LOCK( g_hRawPortLock );
		raw_enqueue_packet( psRawPort, a_psPacket );
		UNLOCK( g_hRawPortLock );
	}

	return;
}



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int raw_open( Socket_s *a_psSocket )
{
	a_psSocket->sk_psOps = &g_sRawOperations;

	a_psSocket->sk_psRawEndP = kmalloc( sizeof( RawEndPoint_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( a_psSocket->sk_psRawEndP == NULL )
	{
		kerndbg( KERN_WARNING, "RAW_OPEN() no memory for raw endpoint\n" );
		return ( -ENOMEM );
	}
	// for now routing is allways used: FIXME!!! Until dont route option works.
	a_psSocket->sk_psRawEndP->re_bRoute = true;
	dump_socket_info( a_psSocket );
	a_psSocket->sk_psRawEndP->re_nProtocol = a_psSocket->sk_nProto;
	init_net_queue( &a_psSocket->sk_psRawEndP->re_sPackets );
	return ( -EOK );
}

static int raw_close( Socket_s *a_psSocket )
{
	if ( a_psSocket->sk_psRawEndP == NULL )
	{
		kerndbg( KERN_WARNING, "RAW_CLOSE(): socket has no endpoint attached to it!\n" );
		return ( -EINVAL );
	}
	//Drain the queue ?? 
	raw_delete_endpoint( a_psSocket->sk_psRawEndP );
	a_psSocket->sk_psRawEndP = NULL;
	return ( 0 );
}

static int raw_set_fflags( Socket_s *a_psSocket, uint32 a_nFlags )
{
	RawEndPoint_s *psRawEndPoint = a_psSocket->sk_psRawEndP;

	if ( a_nFlags & O_NONBLOCK )
	{
		psRawEndPoint->re_bNonBlocking = true;
	}
	else
	{
		psRawEndPoint->re_bNonBlocking = false;
	}

	return ( 0 );
}

static int raw_add_select( Socket_s *psSocket, SelectRequest_s *psReq )
{
	RawEndPoint_s *psRawEndPoint = psSocket->sk_psRawEndP;
	int nError;

	LOCK( g_hRawPortLock );

	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		psReq->sr_psNext = psRawEndPoint->re_psFirstReadSelReq;
		psRawEndPoint->re_psFirstReadSelReq = psReq;
		kerndbg( KERN_DEBUG_LOW, "Add read request to %p\n", psRawEndPoint );
		if ( psRawEndPoint->re_sPackets.nq_nCount > 0 )
		{
			psReq->sr_bReady = true;
			UNLOCK( psReq->sr_hSema );
		}
		nError = 0;
		break;
	case SELECT_WRITE:
		kerndbg( KERN_DEBUG_LOW, "Add write request to %p\n", psRawEndPoint );
		psReq->sr_bReady = true;
		UNLOCK( psReq->sr_hSema );
		nError = 0;
		break;
	case SELECT_EXCEPT:
		nError = 0;
		break;
	default:
		kerndbg( KERN_WARNING, "RAW_ADD_SELECT(): ERROR! unknown mode %d\n", psReq->sr_nMode );
		nError = -EINVAL;
		break;
	}
	UNLOCK( g_hRawPortLock );
	return ( nError );
}

static int raw_rem_select( Socket_s *psSocket, SelectRequest_s *psReq )
{
	RawEndPoint_s *psRawEndPoint = psSocket->sk_psRawEndP;
	SelectRequest_s **ppsTmp = NULL;
	int nError;

	LOCK( g_hRawPortLock );

	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		ppsTmp = &psRawEndPoint->re_psFirstReadSelReq;
		nError = 0;
		kerndbg( KERN_DEBUG_LOW, "RAW_REM_SELECT():Remove read request from %p\n", psRawEndPoint );
		break;
	case SELECT_WRITE:
// For now writes are allways possible, so removing SELECT_WRITE does nothing (check raw_add_select)
// The select behavior is copied from udp, so don't blame me...:)
//                      ppsTmp = &psRawEndPoint->raw_psFirstWriteSelReq;
//                      printk( "RAW_REM_SELECT(): removed SELECT_WRITE from %p\n", psRawEndPoint );
		nError = 0;
		break;
	case SELECT_EXCEPT:
		nError = 0;
		break;
	default:
		kerndbg( KERN_WARNING, "RAW_REM_SELECT(): unknown mode %d\n", psReq->sr_nMode );
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
	UNLOCK( g_hRawPortLock );
	return ( nError );
}


SocketOps_s g_sRawOperations = {
	raw_open,
	raw_close,
	NULL,			// so_shutdown
	raw_bind,
	raw_connect,
	raw_getsockname,
	NULL,			// so_getpeername
	raw_recvmsg,
	raw_sendmsg,
	raw_add_select,
	raw_rem_select,
	NULL,			// so_listen,
	NULL,			// so_accept
	raw_setsockopt,
	NULL,			// so_getsockopt
	raw_set_fflags,
	NULL			// so_ioctl
};

int init_raw( void )
{
	g_hRawPortLock = create_semaphore( "raw_portlist", 1, SEM_RECURSIVE );

	if ( g_hRawPortLock < 0 )
	{
		kerndbg( KERN_FATAL, "Failed to initialise raw sockets!\n" );
		return ( g_hRawPortLock );
	}
	return ( -EOK );
}

