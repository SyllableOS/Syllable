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

static NetInterface_s* g_psLoopbackInterface = NULL;
static NetQueue_s      g_sLoopbackQueue;

typedef struct
{
    uint32	li_nFlags;
} LBInterface_s;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int loopback_write( Route_s* psRoute, ipaddr_t pDstAddress, PacketBuf_s* psPkt )
{
  psPkt->pb_uMacHdr.pRaw = psPkt->pb_uNetworkHdr.pRaw - ETH_HLEN;
  psPkt->pb_nSize += ETH_HLEN;
  if ( psPkt->pb_uMacHdr.pRaw < psPkt->pb_pHead ) {
    printk( "Error: loopback_write() not enough space for ethernet header\n" );
    return( -EINVAL );
  }
  psPkt->pb_bLocal = true;
  psPkt->pb_uMacHdr.psEthernet->h_proto = htonw( psPkt->pb_nProtocol );
  enqueue_packet( &g_sLoopbackQueue, psPkt );
  return( 0 );
}

static int loopback_set_interface_flags( LBInterface_s* psInterface, short nFlags )
{
    psInterface->li_nFlags = nFlags;
    return( 0 );
}

static int loopback_get_interface_flags( LBInterface_s* psInterface, short* pnFlags )
{
    *pnFlags = psInterface->li_nFlags;
    return( 0 );
}

static int loopback_ioctl( void* pInterface, int nCmd, void* pArg )
{
    LBInterface_s* psInterface = pInterface;
    struct ifreq sIFReq;
    int nError = 0;

    if ( memcpy_from_user( &sIFReq, pArg, sizeof( sIFReq ) ) < 0 ) {
	return( -EFAULT );
    }
    switch( nCmd )
    {
	case SIOCSIFFLAGS:	// Set interface flags
	    nError = loopback_set_interface_flags( psInterface, sIFReq.ifr_flags );
	    break;
	case SIOCGIFFLAGS:	// Get interface flags
	    nError = loopback_get_interface_flags( psInterface, &sIFReq.ifr_flags );
	    break;
	default:
	    printk( "Error: loopback_ioctl() unknown command: %08x\n", nCmd );
	    nError = -ENOSYS;
	    break;
    }
    if ( memcpy_to_user( pArg, &sIFReq, sizeof( sIFReq ) ) < 0 ) {
	nError = -EFAULT;
    }
    return( 0 );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int send_packet( Route_s* psRoute, ipaddr_t pDstAddress, PacketBuf_s* psPkt )
{
    NetInterface_s* psInterface;
    
    if ( IP_SAMEADDR( psRoute->rt_anNetAddr, pDstAddress ) ) {
	psInterface = g_psLoopbackInterface;
    } else {
	psInterface = psRoute->rt_psInterface;
    }
    psPkt->pb_psInterface = psInterface;
    return( psInterface->ni_psOps->ni_write( psRoute, pDstAddress, psPkt ) );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void dispatch_net_packet( PacketBuf_s* psPkt )
{
    psPkt->pb_nSize -= ETH_HLEN;

//    printk( "Got packet with proto %d\n", ntohw( psPkt->pb_uMacHdr.psEthernet->h_proto ) );
    switch( ntohw( psPkt->pb_uMacHdr.psEthernet->h_proto ) )
    {
	case ETH_P_IP:
	    ip_in( psPkt, psPkt->pb_nSize );
	    break;
	default:
//	printk( "Received unknown packet %04x\n", ntohw( psPkt->pb_uMacHdr.psEthernet->h_proto ) );
	    free_pkt_buffer( psPkt );
	    break;
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int net_rx_thread( void* pData )
{
    for (;;)
    {
	PacketBuf_s* psBuf;

	psBuf = remove_head_packet( &g_sLoopbackQueue, INFINITE_TIMEOUT );

	if ( psBuf == NULL ) {
	    continue;
	}

	psBuf->pb_nSize -= ETH_HLEN;

//    printk( "Got packet with proto %d\n", ntohw( psBuf->pb_uMacHdr.psEthernet->h_proto ) );
	switch( ntohw( psBuf->pb_uMacHdr.psEthernet->h_proto ) )
	{
	    case ETH_P_IP:
		ip_in( psBuf, psBuf->pb_nSize );
		break;
	    default:
//	printk( "Received unknown packet %04x\n", ntohw( psBuf->pb_uMacHdr.psEthernet->h_proto ) );
		free_pkt_buffer( psBuf );
		break;
	}
    }
    return( 0 );
}



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static NetInterfaceOps_s g_sLoopbackOps =
{
    loopback_write,
    loopback_ioctl,
    NULL
};

int init_net_core()
{
    NetInterface_s* psIFace;
    LBInterface_s*  psLBIface;
    
    thread_id	  hThread;
    ipaddr_t	  anNullAddr = { 0, 0, 0, 0 };
    ipaddr_t	  anGatewayAddr;
  
    psIFace   = kmalloc( sizeof( NetInterface_s ), MEMF_KERNEL | MEMF_CLEAR );
    psLBIface = kmalloc( sizeof( LBInterface_s ), MEMF_KERNEL | MEMF_CLEAR );
    
    psIFace->ni_pInterface = psLBIface;

    IP_MAKEADDR( anGatewayAddr, 193,71,102,1 );
  
    strcpy( psIFace->ni_zName, "loopback" );
    psIFace->ni_psOps = &g_sLoopbackOps;
    
    psIFace->ni_anIpAddr[0] = 127;
    psIFace->ni_anIpAddr[1] = 0;
    psIFace->ni_anIpAddr[2] = 0;
    psIFace->ni_anIpAddr[3] = 1;
    IP_MAKEADDR( psIFace->ni_anSubNetMask, 255,255,255,0 );
    psIFace->ni_nMTU	    = 1500;
    psLBIface->li_nFlags = IFF_UP | IFF_RUNNING | IFF_LOOPBACK | IFF_NOARP;

    g_psLoopbackInterface = psIFace;

    add_net_interface( psIFace );
    add_route( psIFace->ni_anIpAddr, psIFace->ni_anSubNetMask, anNullAddr, 0, psIFace );

    init_net_queue( &g_sLoopbackQueue );
    init_net_interfaces();
  
    hThread = spawn_kernel_thread( "net_rx_thread", net_rx_thread, 5, 0, NULL );
    wakeup_thread( hThread, false );
  
    init_sockets();
    init_udp();
    init_tcp();
    init_raw();
    return( 0 );
}
