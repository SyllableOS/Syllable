
/*
 *  Syllable Kernel
 *  net/core.c
 *  
 *  Syllable Kernel is derived from the AtheOS kernel
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
#include <atheos/socket.h>
#include <atheos/semaphore.h>

#include <net/net.h>
#include <net/ip.h>
#include <net/in.h>
#include <net/if.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <net/raw.h>
#include <net/if_ether.h>
#include <net/if_arp.h>
#include <net/icmp.h>
#include <net/sockios.h>
#include <net/route.h>

/* Selective debugging level overrides */
#ifdef KERNEL_DEBUG_NET
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET
#endif

#ifdef KERNEL_DEBUG_NET_CORE
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET_CORE
#endif


static int g_nLoopbackInterface = -1;
static NetQueue_s g_sLoopbackQueue;

/**
 * \par Description:
 * Writes a packet to the loopback interface.
 * In order to emulate an Ethernet interface, a fake header is prepended
 * to the packet.  This means that the packet buffer must have at least
 * ETH_HLEN bytes between the packet head and network layer header.
 * 
 * If an error occurs, the packet is not freed and must be freed by
 * the caller (usually send_packet()).  If the packet is successfully
 * written, it should be considered consumed by this function.
 */
int loopback_write( Route_s *psRoute, ipaddr_t pDstAddress, PacketBuf_s *psPkt )
{
	psPkt->pb_uMacHdr.pRaw = psPkt->pb_uNetworkHdr.pRaw - ETH_HLEN;
	psPkt->pb_nSize += ETH_HLEN;

	if ( psPkt->pb_uMacHdr.pRaw < psPkt->pb_pHead )
	{
		kerndbg( KERN_FATAL, "loopback_write(): Not enough space for ethernet header\n" );
		return ( -EINVAL );
	}

	psPkt->pb_bLocal = true;
	psPkt->pb_uMacHdr.psEthernet->h_proto = htonw( psPkt->pb_nProtocol );

	enqueue_packet( &g_sLoopbackQueue, psPkt );

	return ( 0 );
}

/** 
 * \par Description:
 * Sends a packet through the specified interface.
 * Outgoing packets addressed to the local interface's address are captured
 * and sent via the loopback interface.
 * 
 * The route psRoute should have been acquired with ip_find_route() or
 * ip_find_device_route() and should be released by the caller.
 *
 * If the function returns successfully, the packet buffer is consumed and
 * should not be used.  If the function returns an error, the packet buffer
 * needs to be freed by the caller.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 */
int send_packet( Route_s *psRoute, ipaddr_t pDstAddress, PacketBuf_s *psPkt )
{
	NetInterface_s *psInterface;
	int nError;

	if ( IP_SAMEADDR( psRoute->rt_anNetAddr, pDstAddress ) )
	{
		/* Use loopback interface instead of route interface */
		psInterface = get_net_interface( g_nLoopbackInterface );
		if ( psInterface == NULL )
		{
			kerndbg( KERN_FATAL, "send_packet(): Unable to acquire loopback interface.\n" );

			return ( -ENOENT );
		}

		psPkt->pb_bLocal = true;
		nError = psInterface->ni_psOps->ni_write ( psRoute, pDstAddress, psPkt );

		release_net_interface( psInterface );
	}
	else
	{
		psInterface = psRoute->rt_psInterface;

		/* Kernel interface read lock is held by route; no need to lock again */
		nError = psInterface->ni_psOps->ni_write ( psRoute, pDstAddress, psPkt );
	}

	return ( nError );
}

/** 
 * \par Description:
 * Dispatches Ethernet packets that contain an IPv4 payload.
 * 
 * \par Note:
 * This function should not be used with non-Ethernet packets.  If you want
 * a packet to be handled by the IP processing code, call ip_in().
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 */
void dispatch_net_packet( PacketBuf_s *psPkt )
{
	psPkt->pb_nSize -= ETH_HLEN;

	switch ( ntohw( psPkt->pb_uMacHdr.psEthernet->h_proto ) )
	{
	case ETH_P_IP:
		ip_in( psPkt, psPkt->pb_nSize );
		break;

	default:
		free_pkt_buffer( psPkt );
		break;
	}
}

/**
 * \par Description:
 * Receive thread routine for the loopback interface.
 */
static int net_rx_thread( void *pData )
{
	for ( ;; )
	{
		PacketBuf_s *psBuf;

		psBuf = remove_head_packet( &g_sLoopbackQueue, INFINITE_TIMEOUT );

		if ( psBuf == NULL )
			continue;

		psBuf->pb_nSize -= ETH_HLEN;

		switch ( ntohw( psBuf->pb_uMacHdr.psEthernet->h_proto ) )
		{
		case ETH_P_IP:
			ip_in( psBuf, psBuf->pb_nSize );
			break;

		default:
			free_pkt_buffer( psBuf );
			break;
		}
	}

	return ( 0 );
}

static NetInterfaceOps_s g_sLoopbackOps = {
	loopback_write,
	NULL,			/* ni_ioctl */
	NULL,			/* ni_deinit */
	NULL,			/* ni_notify */
	NULL			/* ni_release */
};

/**
 * \par Description:
 * Initialises the networking code, creates the loopback interface, and scans
 * for hardware interfaces.
 */
int init_net_core( void )
{
	thread_id hThread;
	ipaddr_t anLBIp = { 127, 0, 0, 1 }, anLBNm =
	{
	255, 0, 0, 0};
	int nError;
	NetInterface_s *psLB;

	/* Initialise network protocols */
	init_sockets();
	init_udp();
	init_tcp();
	init_raw();

	/* Initialise interface handling code */
	init_net_interfaces();

	/* Initialise the loopback interface */
	init_net_queue( &g_sLoopbackQueue );

	if ( ( nError = add_net_interface( "loopback", &g_sLoopbackOps, NULL ) ) < 0 )
		return ( nError );

	g_nLoopbackInterface = nError;

	/* Acquire net interface */
	psLB = get_net_interface( nError );
	if ( psLB == NULL )
		return ( -ENOENT );

	if ( ( nError = set_interface_address( psLB, anLBIp ) ) < 0 )
		return ( nError );

	if ( ( nError = set_interface_netmask( psLB, anLBNm ) ) < 0 )
		return ( nError );

	if ( ( nError = set_interface_mtu( psLB, 16384 ) ) < 0 )
		return ( nError );

	if ( ( nError = set_interface_flags( psLB, IFF_UP | IFF_RUNNING | IFF_LOOPBACK | IFF_NOARP ) ) < 0 )
		return ( nError );

	release_net_interface( psLB );

	/* Start loopback receive thread */
	hThread = spawn_kernel_thread( "Loopback Rx", net_rx_thread, 5, 0, NULL );
	wakeup_thread( hThread, false );

	/* Load other network interface drivers */
	load_interface_drivers();

	return ( 0 );
}
