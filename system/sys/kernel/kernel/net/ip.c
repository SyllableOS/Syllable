
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999  Kurt Skauen
 *  Copyright (C) 2002  Ville Kallioniemi
 *
 *  Changes:
 *  2002	Ville Kallioniemi		ip_process_paket now handles raw sockets
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
#include <atheos/kdebug.h>
#include <atheos/time.h>
#include <atheos/semaphore.h>

#include <atheos/socket.h>
#include <net/net.h>
#include <net/ip.h>
#include <net/if_ether.h>
#include <net/if_arp.h>
#include <net/icmp.h>
#include <net/in.h>
#include <net/tcp.h>
#include <net/raw.h>
#include <net/route.h>

#include <macros.h>

#include <net/packet.h>

/* Selective debugging level overrides */
#ifdef KERNEL_DEBUG_NET
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET
#endif

#ifdef KERNEL_DEBUG_NET_IP
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET_IP
#endif


typedef struct _IpFragQueue IpFragQueue_s;
struct _IpFragQueue
{
	IpFragQueue_s *fq_psNext;
	PacketBuf_s *fq_psFirst;
	int fq_nPacketID;
	ipaddr_t fq_anSrcAddr;
	bigtime_t fq_nExpireTime;
};


static IpFragQueue_s *g_psFirstFragmentQueue = NULL;
static sem_id g_hFragListLock;

void format_ipaddress( char *pzBuffer, ipaddr_t anIp )
{
	sprintf( pzBuffer, "%d.%d.%d.%d", ( int )anIp[0], ( int )anIp[1], ( int )anIp[2], ( int )anIp[3] );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
int parse_ipaddress( ipaddr_t pAddress, const char *pzBuffer )
{
	int i;
	const char *pzPtr = pzBuffer;

	for ( i = 0; i < IP_ADR_LEN; ++i )
	{
		char *pzNext;

		pAddress[i] = strtol( pzPtr, &pzNext, 10 );
		if ( pzNext == pzPtr )
		{
			return ( -EINVAL );
		}
		while ( *pzNext == ' ' || *pzNext == '\t' )
			pzNext++;

		if ( i != ( IP_ADR_LEN - 1 ) && pzNext[0] != '.' )
		{
			return ( -EINVAL );
		}
		pzPtr = pzNext + 1;
	}
	return ( pzPtr - pzBuffer - 1 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint16 ip_chksum( uint8 *pBuffer, int nLength )
{
	uint32 nSum = 0;
	uint16 *pBuf = ( uint16 * )pBuffer;

	if ( nLength & 0x01 )
		pBuffer[nLength++] = 0;

	nLength >>= 1;

	while ( nLength-- )
		nSum += *pBuf++;

	nSum = ( nSum >> 16 ) + ( nSum & 0xffff );	// Add in carry
	nSum += nSum >> 16;	// Maybe one more
	return ( ~nSum );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int icmp_send( PacketBuf_s *a_psOrig, int a_nType, int a_nCode, ipaddr_t a_anDest )
{
	PacketBuf_s *psPkt;
	ICMPHeader_s *psIcmp = NULL;
	IpHeader_s *psIpHdr;
	ipaddr_t anDest;
	int nDataLen;

	IP_COPYADDR( anDest, a_anDest );

	kerndbg( KERN_DEBUG_LOW, "Send ICMP message\n" );

	switch ( a_nType )
	{
	case ICMP_ECHOREPLY:
		psPkt = a_psOrig;
		psIpHdr = psPkt->pb_uNetworkHdr.psIP;
		break;
	default:
		psPkt = alloc_pkt_buffer( 16 + sizeof( IpHeader_s ) + sizeof( ICMPHeader_s ) );
		if ( psPkt != NULL )
		{
			psPkt->pb_uNetworkHdr.pRaw = psPkt->pb_pData + 16;
			psPkt->pb_uTransportHdr.pRaw = psPkt->pb_uNetworkHdr.pRaw + sizeof( IpHeader_s );

			psIpHdr = psPkt->pb_uNetworkHdr.psIP;
			psPkt->pb_nSize = sizeof( IpHeader_s ) + sizeof( ICMPHeader_s );

			psIpHdr->iph_nHdrSize = 5;
			psIpHdr->iph_nVersion = 4;
			psIpHdr->iph_nTypeOfService = 0;
			psIpHdr->iph_nPacketSize = htonw( psPkt->pb_nSize );
			psIpHdr->iph_nFragOffset = 0;
			psIpHdr->iph_nTimeToLive = 255;
			psIpHdr->iph_nProtocol = IPT_ICMP;
		}
		break;
	}

	if ( psPkt == NULL )
	{
		return ( -EINVAL );
	}

	nDataLen = ntohw( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr );
	psIcmp = psPkt->pb_uTransportHdr.psIcmp;

	if ( psIcmp == NULL )
	{
		kerndbg( KERN_WARNING, "icmp_send() no ICMP header buffer\n" );
		return ( -EINVAL );
	}

	switch ( a_nType )
	{
	case ICMP_ECHOREPLY:
		psIcmp->ic_nType = a_nType;
		psIcmp->ic_nCode = a_nCode;
		break;
	default:
		psIcmp->ic_nType = a_nType;
		psIcmp->ic_nCode = a_nCode;
		psIcmp->ic_uMsg.nZero = 0;
		break;
	}

	psIcmp->ic_nChkSum = 0;
	psIcmp->ic_nChkSum = ip_chksum( ( uint8 * )psIcmp, nDataLen );

	psIpHdr->iph_nProtocol = IPT_ICMP;	/* for generated packets */
	IP_COPYADDR( psIpHdr->iph_nDstAddr, anDest );

	ip_send( psPkt );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void icmp_in( PacketBuf_s *psPkt, int nPktSize )
{
	ICMPHeader_s *psIcmp;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;

	psIcmp = psPkt->pb_uTransportHdr.psIcmp;

	if ( ip_chksum( ( uint8 * )psIcmp, nPktSize ) != 0 )
	{
		kerndbg( KERN_WARNING, "icmp_in() invalid checksum %d\n", ip_chksum( ( uint8 * )psIcmp, nPktSize ) );
		free_pkt_buffer( psPkt );
		return;
	}

	switch ( psIcmp->ic_nType )
	{
	case ICMP_ECHO:
		icmp_send( psPkt, ICMP_ECHOREPLY, 0, psIpHdr->iph_nSrcAddr );

	case ICMP_ECHOREPLY:
		{
			// These may be generated by user-space "ping", which
			// generates ICMP packets via. raw sockets.  We do not
			// need to worry about them here, so we ignore them.

			break;
		}

	default:
		{
			kerndbg( KERN_WARNING, "Unknown ICMP message %d\n", psIcmp->ic_nType );
			free_pkt_buffer( psPkt );
			break;
		}
	}
}

/**
 * \par Description:
 * Find an appropriate route for the packet's destination and call
 * ip_send_via() to send the packet, fragmenting if necessary.
 *
 * The packet buffer is always consumed, even if an error is returned.
 */
int ip_send( PacketBuf_s *psPkt )
{
	Route_s *psRoute;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	int nError;

	psRoute = ip_find_route( psIpHdr->iph_nDstAddr );

	if ( psRoute == NULL )
	{
		USE_FORMAT_IP( 1 );

		FORMAT_IP( psIpHdr->iph_nDstAddr, 0 );
		kerndbg( KERN_WARNING, "Could not find route for address %s\n", FORMATTED_IP( 0 ) );

		free_pkt_buffer( psPkt );
		return ( -ENETUNREACH );
	}

	nError = ip_send_via( psPkt, psRoute );

	ip_release_route( psRoute );

	return ( nError );
}

/**
 * \par Description:
 * Prepares to send an IP packet to its next hop address.
 * The packet may be fragmented if the MTU of the outgoing link is smaller
 * than the packet.
 *
 * The packet buffer must be allocated with at least ETH_HLEN bytes to
 * hold an ethernet header (usually rounded up to multiple of 4 bytes) as
 * well as the IP packet itself.  The pb_uNetworkHdr pointer must correctly
 * point to the start of the IP packet (e.g. not the ethernet header) and
 * the pb_uTransportHdr pointer must point to the start of the IP payload.
 * The size of the IP packet should be stored in pb_nSize (not including the
 * ethernet header).
 * 
 * This function will fill in the pb_nProtocol field with ETH_P_IP, the
 * TTL is set to 128.  The address of outgoing interface is copied into
 * the source address.  The packet ID is assigned serially.  The IP header
 * checksum is calculated last.
 *
 * If the packet is fragmented, the packet size and fragment offset fields
 * are also calculated appropriately.
 *
 * The packet buffer is always consumed, even if an error is returned.
 */
int ip_send_via( PacketBuf_s *psPkt, Route_s *psRoute )
{
	static int16 nPacketID = 0;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	int nPacketSize = ntohw( psIpHdr->iph_nPacketSize );
	int nHdrSize = IP_GET_HDR_LEN( psIpHdr );
	int nEthHdrLen = ( ( ETH_HLEN + 3 ) & ~3 );	/* Length of ethernet header */
	int nDataSize = nPacketSize - nHdrSize;
	int nMTU;
	int nError;

	/* FIXME: Use atomic_add()? */
	psIpHdr->iph_nPacketId = htonw( nPacketID );
	nPacketID++;

	psIpHdr->iph_nTimeToLive = 128;
	IP_COPYADDR( psIpHdr->iph_nSrcAddr, psRoute->rt_psInterface->ni_anIpAddr );

	nMTU = psRoute->rt_psInterface->ni_nMTU - nHdrSize;

	if ( nDataSize > nMTU )
	{
		int nFragOffset = 0;
		char *pBuffer = (char*)psPkt->pb_uTransportHdr.pRaw;

		kerndbg( KERN_DEBUG, "ip_send_via(): Splitting packet into fragments: %d header/%d data\n", nHdrSize, nDataSize );

		while ( nDataSize > 0 )
		{
			PacketBuf_s *psFrag;
			IpHeader_s *psFrgHdr;
			int nCurSize = nDataSize;

			if ( nCurSize > nMTU )
			{
				nCurSize = nMTU;
				nCurSize &= ~7;	/* Round down to nearest 8-byte amount */
			}

			psFrag = alloc_pkt_buffer( nCurSize + nHdrSize + nEthHdrLen );

			if ( NULL == psFrag )
			{
				kerndbg( KERN_FATAL, "ip_send_via(): No memory for fragment buffer\n" );
				nError = -ENOMEM;
				goto error;
			}

			psFrag->pb_uNetworkHdr.pRaw = psFrag->pb_pData + nEthHdrLen;
			psFrag->pb_uTransportHdr.pRaw = psFrag->pb_uNetworkHdr.pRaw + nHdrSize;
			psFrgHdr = psFrag->pb_uNetworkHdr.psIP;
			psFrag->pb_nSize = nCurSize + nHdrSize;

			memcpy( psFrgHdr, psIpHdr, nHdrSize );
			memcpy( psFrag->pb_uTransportHdr.pRaw, pBuffer, nCurSize );

			psFrgHdr->iph_nPacketSize = htonw( nCurSize + nHdrSize );
			psFrgHdr->iph_nFragOffset = htonw( nFragOffset >> 3 );

			if ( nCurSize < nDataSize )
			{
				psFrgHdr->iph_nFragOffset |= htonw( IP_MORE_FRAGMENTS );
			}
			psFrgHdr->iph_nCheckSum = 0;
			psFrgHdr->iph_nCheckSum = ip_fast_csum( ( uint8 * )psFrgHdr, psFrgHdr->iph_nHdrSize );
			psFrag->pb_nProtocol = ETH_P_IP;

			kerndbg( KERN_DEBUG, "ip_send_via(): Send %d bytes from %d (%d)\n", nCurSize, nFragOffset, ( int )ntohw( psFrgHdr->iph_nFragOffset ) );

			if ( ( nError = send_packet( psRoute, psFrgHdr->iph_nDstAddr, psFrag ) ) < 0 )
			{
				kerndbg( KERN_DEBUG, "ip_send_via(): send_packet() failed, freeing fragment buffer.\n" );

				free_pkt_buffer( psFrag );
				goto error;
			}

			nDataSize -= nCurSize;
			nFragOffset += nCurSize;
			pBuffer += nCurSize;
		}

		free_pkt_buffer( psPkt );
	}
	else			// No need for fragmenting
	{
		psIpHdr->iph_nFragOffset |= htonw( IP_DONT_FRAGMENT );

		psIpHdr->iph_nCheckSum = 0;
		psIpHdr->iph_nCheckSum = ip_fast_csum( ( uint8 * )psIpHdr, psIpHdr->iph_nHdrSize );

		psPkt->pb_nProtocol = ETH_P_IP;

		if ( ( nError = send_packet( psRoute, psIpHdr->iph_nDstAddr, psPkt ) ) < 0 )
		{
			kerndbg( KERN_DEBUG, "ip_send(): send_packet() failed, freeing packet buffer.\n" );

			goto error;
		}
	}

	return ( 0 );

      error:
	free_pkt_buffer( psPkt );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void ip_process_paket( PacketBuf_s *psPkt )
{
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	PacketBuf_s *psClonedPacket = NULL;
	RawPort_s *psRawPort = NULL;

	psPkt->pb_uTransportHdr.pRaw = psPkt->pb_uNetworkHdr.pRaw + IP_GET_HDR_LEN( psIpHdr );

	// This is what we do:
	// 1. test if we have a raw socket corresponding to a protocol 
	//              true:   1. raw_in(a clone of the packet)        2. continue
	//              false:  continue 
	// Not optimal, since we have to check this every time that a packet is received
	// Doesn't _seem_ to slow down the networking, but isn't a very pretty solution either -vk 

	psRawPort = raw_find_port( psIpHdr->iph_nProtocol );
	if ( psRawPort != NULL )
	{
		psClonedPacket = clone_pkt_buffer( psPkt );

		if ( psClonedPacket != NULL )
		{
			raw_in( psClonedPacket, ntohw( psIpHdr->iph_nPacketSize ) );
		}
		else
		{
			kerndbg( KERN_WARNING, "ip_process_paket() Could not clone packet!\n" );
		}
	}


	switch ( psIpHdr->iph_nProtocol )
	{
	case IPT_RAW:
		raw_in( psPkt, ntohw( psIpHdr->iph_nPacketSize ) );
		break;
	case IPT_ICMP:
		icmp_in( psPkt, ntohw( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr ) );
		break;
	case IPT_UDP:
		udp_in( psPkt, ntohw( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr ) );
		break;
	case IPT_TCP:
//      tcp_dump_packet( psPkt );
		tcp_in( psPkt, ntohw( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr ) );
		break;
	default:
		free_pkt_buffer( psPkt );
		kerndbg( KERN_WARNING, "ip_in() unknown protocol %d\n", psIpHdr->iph_nProtocol );
		break;
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

IpFragQueue_s *find_fragment_queue( int nPacketID, ipaddr_t anSrcAddr )
{
	IpFragQueue_s *psQueue;

	for ( psQueue = g_psFirstFragmentQueue; psQueue != NULL; psQueue = psQueue->fq_psNext )
	{
		if ( psQueue->fq_nPacketID == nPacketID && IP_SAMEADDR( psQueue->fq_anSrcAddr, anSrcAddr ) )
		{
			psQueue->fq_nExpireTime = get_system_time() + IP_FRAG_TIME;
			return ( psQueue );
		}
	}
	psQueue = kmalloc( sizeof( IpFragQueue_s ), MEMF_KERNEL );

	if ( psQueue == NULL )
	{
		kerndbg( KERN_FATAL, "Error: find_fragment_queue() out of memory\n" );
		return ( NULL );
	}
	psQueue->fq_psNext = g_psFirstFragmentQueue;
	g_psFirstFragmentQueue = psQueue;

	psQueue->fq_psFirst = NULL;
	psQueue->fq_nPacketID = nPacketID;
	IP_COPYADDR( psQueue->fq_anSrcAddr, anSrcAddr );
	psQueue->fq_nExpireTime = get_system_time() + IP_FRAG_TIME;

	return ( psQueue );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void delete_fragment_queue( IpFragQueue_s *psQueue )
{
	IpFragQueue_s **ppsTmp;

	for ( ppsTmp = &g_psFirstFragmentQueue; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->fq_psNext )
	{
		if ( *ppsTmp == psQueue )
		{
			*ppsTmp = psQueue->fq_psNext;

			while ( psQueue->fq_psFirst != NULL )
			{
				PacketBuf_s *psPkt = psQueue->fq_psFirst;

				psQueue->fq_psFirst = psPkt->pb_psNext;

				free_pkt_buffer( psPkt );
			}

			kfree( psQueue );
			return;
		}
	}
	kerndbg( KERN_WARNING, "Error: delete_fragment_queue() failed to find queue %p in list\n", psQueue );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void ip_add_fragment( PacketBuf_s *psPkt, int nPktSize )
{
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	IpFragQueue_s *psQueue;
	PacketBuf_s **ppsTmp;
	PacketBuf_s *psNewPkt;
	int nFragOffset;
	int nTotSize = 0;

//  printk( "Add fragment\n" );

	LOCK( g_hFragListLock );

	psQueue = find_fragment_queue( ntohw( psIpHdr->iph_nPacketId ), psIpHdr->iph_nSrcAddr );

	if ( psQueue == NULL )
	{
		kerndbg( KERN_WARNING, "Error: ip_add_fragment() ran out of fragment queues\n" );
		goto error;
	}
	nFragOffset = ntohw( psIpHdr->iph_nFragOffset ) & IP_FRAGOFF_MASK;
	for ( ppsTmp = &psQueue->fq_psFirst; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->pb_psNext )
	{
		int nCurOffset = ntohw( ( *ppsTmp )->pb_uNetworkHdr.psIP->iph_nFragOffset ) & IP_FRAGOFF_MASK;

		if ( nCurOffset > nFragOffset )
		{
			psPkt->pb_psNext = *ppsTmp;
			*ppsTmp = psPkt;
			goto done;
		}
	}
	*ppsTmp = psPkt;
	psPkt->pb_psNext = NULL;
      done:
	for ( psPkt = psQueue->fq_psFirst; psPkt != NULL; psPkt = psPkt->pb_psNext )
	{
		int nCurOffset;

		psIpHdr = psPkt->pb_uNetworkHdr.psIP;

		nCurOffset = ( ntohw( psIpHdr->iph_nFragOffset ) & IP_FRAGOFF_MASK ) << 3;

//    printk( "Test fragment %d - %d\n", nCurOffset, nTotSize );
		if ( nCurOffset != nTotSize )
		{
			goto error;
		}
		nTotSize += ntohw( psIpHdr->iph_nPacketSize ) - IP_GET_HDR_LEN( psIpHdr );
		if ( psPkt->pb_psNext == NULL && ( ntohw( psIpHdr->iph_nFragOffset ) & IP_MORE_FRAGMENTS ) != 0 )
		{
			goto error;
		}
	}
	psIpHdr = psQueue->fq_psFirst->pb_uNetworkHdr.psIP;

	psNewPkt = alloc_pkt_buffer( nTotSize + IP_GET_HDR_LEN( psIpHdr ) + 16 );

	if ( psNewPkt == NULL )
	{
		delete_fragment_queue( psQueue );
		goto error;
	}
	psNewPkt->pb_uNetworkHdr.pRaw = psNewPkt->pb_pData + 16;
	*psNewPkt->pb_uNetworkHdr.psIP = *psIpHdr;
	psNewPkt->pb_pData = psNewPkt->pb_uNetworkHdr.pRaw + IP_GET_HDR_LEN( psIpHdr );

//  for ( psPkt = psQueue->fq_psFirst ; psPkt != NULL ; psPkt = psPkt->pb_psNext )
	while ( psQueue->fq_psFirst != NULL )
	{
		int nCurOffset;
		int nPktLen;
		int nHdrLen;

		psPkt = psQueue->fq_psFirst;
		psQueue->fq_psFirst = psPkt->pb_psNext;

		psIpHdr = psPkt->pb_uNetworkHdr.psIP;

		nCurOffset = ( ntohw( psIpHdr->iph_nFragOffset ) & IP_FRAGOFF_MASK ) << 3;
		nPktLen = ntohw( psIpHdr->iph_nPacketSize );
		nHdrLen = IP_GET_HDR_LEN( psIpHdr );

//    printk( "Copy %d byte to %d\n", nPktLen - nHdrLen, nCurOffset );
		memcpy( psNewPkt->pb_pData + nCurOffset, psPkt->pb_uNetworkHdr.pRaw + nHdrLen, nPktLen - nHdrLen );
		free_pkt_buffer( psPkt );
	}
	delete_fragment_queue( psQueue );
	psIpHdr = psNewPkt->pb_uNetworkHdr.psIP;
	psNewPkt->pb_nSize = nTotSize + IP_GET_HDR_LEN( psIpHdr );
	psIpHdr->iph_nPacketSize = htonw( psNewPkt->pb_nSize );

	UNLOCK( g_hFragListLock );

	ip_process_paket( psNewPkt );
	return;
      error:
	UNLOCK( g_hFragListLock );

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void ip_expire_fragments( bigtime_t nNow )
{
	IpFragQueue_s *psQueue;
	IpFragQueue_s *psPrev = NULL;

	for ( psQueue = g_psFirstFragmentQueue; psQueue != NULL; )
	{
		if ( nNow > psQueue->fq_nExpireTime )
		{
			IpFragQueue_s *psTmp = psQueue;

			kerndbg( KERN_DEBUG, "ip_expire_fragments() expire queue %p\n", psQueue );

			while ( psQueue->fq_psFirst != NULL )
			{
				PacketBuf_s *psPkt = psQueue->fq_psFirst;

				psQueue->fq_psFirst = psPkt->pb_psNext;
				free_pkt_buffer( psPkt );
			}

			if ( psPrev == NULL )
			{
				g_psFirstFragmentQueue = psQueue->fq_psNext;
			}
			else
			{
				psPrev->fq_psNext = psQueue->fq_psNext;
			}
			icmp_send( NULL, ICMP_TIME_EXCEEDED, ICMP_EXC_FRAGTIME, psQueue->fq_anSrcAddr );
			psQueue = psQueue->fq_psNext;
			kfree( psTmp );
			continue;
		}
		psPrev = psQueue;
		psQueue = psQueue->fq_psNext;
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void ip_in( PacketBuf_s *psPkt, int nPktSize )
{
	/* FIXME: Should this assume the incoming packet has an Ethernet header? */
	IpHeader_s *psIpHdr = ( IpHeader_s * )( psPkt->pb_uMacHdr.pRaw + ETH_HLEN );

	psPkt->pb_uNetworkHdr.psIP = psIpHdr;

	if ( psPkt->pb_bLocal == false && ip_fast_csum( ( uint8 * )psIpHdr, psIpHdr->iph_nHdrSize ) != 0 )
	{
		free_pkt_buffer( psPkt );
		return;
	}

	if ( ( ntohw( psIpHdr->iph_nFragOffset ) & ~IP_DONT_FRAGMENT ) != 0 )
	{
		ip_add_fragment( psPkt, nPktSize );
	}
	else
	{
		ip_process_paket( psPkt );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int ip_timer( void *pData )
{
	for ( ;; )
	{
		bigtime_t nNow;

		snooze( 1000000 );
		nNow = get_system_time();

		LOCK( g_hFragListLock );
		ip_expire_fragments( nNow );
		UNLOCK( g_hFragListLock );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_ip( void )
{
	thread_id hThread;

	g_hFragListLock = create_semaphore( "frglst_lock", 1, SEM_RECURSIVE );
	init_route_table();
	init_net_core();
	hThread = spawn_kernel_thread( "ip_timer", ip_timer, 200, 0, NULL );
	wakeup_thread( hThread, false );
}
