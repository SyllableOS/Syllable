
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
#include <atheos/elf.h>
#include <posix/ioctl.h>
#include <net/net.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/sockios.h>
#include <net/route.h>
#include <net/ip.h>
#include <posix/dirent.h>
#include <posix/errno.h>

#include <macros.h>

static int ether_ioctl( void *pInterface, int nCmd, void *pArg );
static int ether_write( Route_s *psRoute, ipaddr_t pDstAddress, PacketBuf_s *psPkt );
static int ether_deinit();

static NetInterfaceOps_s g_sOps = {
	ether_write,
	ether_ioctl,
	ether_deinit
};


static EthInterface_s *alloc_interface()
{
	EthInterface_s *psInterface;
	psInterface = kmalloc( sizeof( EthInterface_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( psInterface == NULL )
	{
		printk( "Error: alloc_interface() out of memory\n" );
		return ( NULL );
	}
	psInterface->ei_psInterface = kmalloc( sizeof( NetInterface_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( psInterface->ei_psInterface == NULL )
	{
		kfree( psInterface );
		printk( "Error: alloc_interface() out of memory\n" );
		return ( NULL );
	}
	psInterface->ei_psInterface->ni_pInterface = psInterface;
	return ( psInterface );
}

static int ether_deinit()
{
	return ( 0 );
}

static int eth_rx_thread( void *pData )
{
	EthInterface_s *psInterface = pData;

	for ( ;; )
	{
		PacketBuf_s *psPkt;

		psPkt = remove_head_packet( &psInterface->ei_sInputQueue, INFINITE_TIMEOUT );

		if ( psPkt == NULL )
		{
			continue;
		}
		if ( ( psInterface->ei_nFlags & IFF_UP ) == 0 )
		{
			free_pkt_buffer( psPkt );
			printk( "Error: eth_rx_thread() received packet to downed interface\n" );
			continue;
		}

		psPkt->pb_psInterface = psInterface->ei_psInterface;

		switch ( ntohw( psPkt->pb_uMacHdr.psEthernet->h_proto ) )
		{
		case ETH_P_ARP:
			arp_in( psInterface, psPkt );
			break;
		case ETH_P_RARP:
			printk( "Received RARP packet\n" );
			free_pkt_buffer( psPkt );
			break;
		default:	// 2048 == ETH_P_IP == IP packets
			dispatch_net_packet( psPkt );
			break;
		}
	}
}

static int ether_write( Route_s *psRoute, ipaddr_t pDstAddress, PacketBuf_s *psPkt )
{
	uint8 anBroadcastAddr[] = { 0xff, 0xff, 0xff, 0xff };

	if ( IP_SAMEADDR( anBroadcastAddr, pDstAddress ) )
		return ( arp_send_packet( psRoute->rt_psInterface->ni_pInterface, psPkt, pDstAddress ) );

	if ( psRoute->rt_nFlags & RTF_GATEWAY )
	{
		return ( arp_send_packet( psRoute->rt_psInterface->ni_pInterface, psPkt, psRoute->rt_anGatewayAddr ) );
	}
	else
	{
		return ( arp_send_packet( psRoute->rt_psInterface->ni_pInterface, psPkt, pDstAddress ) );
	}
}

static int eth_set_interface_flags( EthInterface_s *psInterface, short nFlags )
{
	if ( nFlags & IFF_UP )
	{
		if ( ( psInterface->ei_nFlags & IFF_UP ) == 0 )
		{
			int nError;

			psInterface->ei_nDevice = open( psInterface->ei_zDriverPath, O_RDONLY );

			if ( psInterface->ei_nDevice >= 0 )
			{
				struct ifreq sIFReq;

				ioctl( psInterface->ei_nDevice, SIOCGIFHWADDR, &sIFReq );
				memcpy( psInterface->ei_anHwAddr, sIFReq.ifr_hwaddr.sa_data, ETH_ALEN );
				nError = ioctl( psInterface->ei_nDevice, SIOC_ETH_START, &psInterface->ei_sInputQueue );
			}
			else
			{
				printk( "Error: eth_set_interface_flags() failed to load driver %s (%d)\n", psInterface->ei_zDriverPath, psInterface->ei_nDevice );
			}
		}
	}
	else
	{
		if ( psInterface->ei_nFlags & IFF_UP )
		{
			if ( ioctl( psInterface->ei_nDevice, SIOC_ETH_STOP, NULL ) >= 0 )
			{
				close( psInterface->ei_nDevice );
				psInterface->ei_nDevice = -1;
			}
			else
			{
				printk( "Error: eth_set_interface_flags() failed to shut down device driver\n" );
				nFlags |= IFF_UP;
			}
		}
	}
	psInterface->ei_nFlags = nFlags;
	return ( 0 );
}

static int eth_get_interface_flags( EthInterface_s *psInterface, short *pnFlags )
{
	*pnFlags = psInterface->ei_nFlags;
	return ( 0 );
}

static int ether_ioctl( void *pInterface, int nCmd, void *pArg )
{
	EthInterface_s *psInterface = pInterface;
	struct ifreq sIFReq;
	int nError = 0;

	switch ( nCmd )
	{
	case SIOCSIFFLAGS:	// Set interface flags
		if ( memcpy_from_user( &sIFReq, pArg, sizeof( sIFReq ) ) < 0 )
			return ( -EFAULT );
		nError = eth_set_interface_flags( psInterface, sIFReq.ifr_flags );
		break;
	case SIOCGIFFLAGS:	// Get interface flags
		if ( memcpy_from_user( &sIFReq, pArg, sizeof( sIFReq ) ) < 0 )
			return ( -EFAULT );
		nError = eth_get_interface_flags( psInterface, &sIFReq.ifr_flags );
		if ( memcpy_to_user( pArg, &sIFReq, sizeof( sIFReq ) ) < 0 )
		{
			nError = -EFAULT;
		}
		break;
	default:
		nError = ioctl( psInterface->ei_nDevice, nCmd, pArg );
		break;
	}
	return ( 0 );
}


static int load_eth_driver( const char *pzPath, struct stat *psStat, void *pArg )
{
	EthInterface_s *psInterface;
	ipaddr_t anNullAddr = { 0, 0, 0, 0 };
	struct ifreq sIFReq;
	int nDriver = open( pzPath, O_RDWR );
	int nError;

	static int nInterfaceNum = 0;

	if ( nDriver < 0 )
	{
		printk( "Error: load_eth_driver() failed to load driver %s\n", pzPath );
		return ( 0 );
	}

	psInterface = alloc_interface();

	if ( psInterface == NULL )
	{
		printk( "Error: load_eth_driver() no memory for interface\n" );
		goto error;
	}
	init_net_queue( &psInterface->ei_sInputQueue );

	strcpy( psInterface->ei_zDriverPath, pzPath );
	psInterface->ei_nDevice = nDriver;
	psInterface->ei_psInterface->ni_nMTU = 1500;


	sprintf( psInterface->ei_psInterface->ni_zName, "eth%d", nInterfaceNum++ );
	psInterface->ei_psInterface->ni_psOps = &g_sOps;
	psInterface->ei_nFlags = IFF_UP | IFF_RUNNING;

	ioctl( nDriver, SIOCGIFHWADDR, &sIFReq );
	memcpy( psInterface->ei_anHwAddr, sIFReq.ifr_hwaddr.sa_data, ETH_ALEN );

	nError = ioctl( nDriver, SIOC_ETH_START, &psInterface->ei_sInputQueue );
	add_net_interface( psInterface->ei_psInterface );

	printk( "Add interface '%s'\n", psInterface->ei_psInterface->ni_zName );
	add_route( psInterface->ei_psInterface->ni_anIpAddr, psInterface->ei_psInterface->ni_anSubNetMask, anNullAddr, 0, psInterface->ei_psInterface );

	psInterface->ei_hRxThread = spawn_kernel_thread( "eth_rx_thread", eth_rx_thread, 5, 0, psInterface );
	wakeup_thread( psInterface->ei_hRxThread, false );

	return ( 0 );
      error:
	close( nDriver );
	return ( 0 );
}

NetInterfaceOps_s *init_interfaces()
{
	init_arp();
	iterate_directory( "/dev/net/eth", false, load_eth_driver, NULL );
	return ( &g_sOps );
}
