
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

/* Selective debugging level overrides */
#ifdef KERNEL_DEBUG_NET
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET
#endif

#ifdef KERNEL_DEBUG_NET_ETH_INTERFACE
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET_ETH_INTERFACE
#endif

static int ether_ioctl( void *pInterface, int nCmd, void *pArg );
static int ether_write( Route_s *psRoute, ipaddr_t pDstAddress, PacketBuf_s *psPkt );
static void ether_deinit( void *pInterface );
static void ether_notify( void *pInterface );
static void ether_release( void *pInterface );

static NetInterfaceOps_s g_sOps = {
	ether_write,
	ether_ioctl,
	ether_deinit,
	ether_notify,
	ether_release
};

/**
 * \par Description:
 * Internal routine to allocate a new Ethernet interface and add the kernel
 * interface.
 */
static EthInterface_s *alloc_interface( const char *pzName )
{
	EthInterface_s *psInterface;
	int nIndex;

	psInterface = kmalloc( sizeof( EthInterface_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( psInterface == NULL )
	{
		kerndbg( KERN_FATAL, "eth_interface.c: alloc_interface(): Out of memory\n" );

		return ( NULL );
	}

	if ( ( nIndex = add_net_interface( pzName, &g_sOps, psInterface ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "eth_interface.c: alloc_interface(): Add interface error (%d)\n", nIndex );

		kfree( psInterface );
		return ( NULL );
	}

	psInterface->ei_psInterface = get_net_interface( nIndex );
	psInterface->ei_nIfIndex = nIndex;

	if ( psInterface->ei_psInterface == NULL )
	{
		kerndbg( KERN_FATAL, "eth_interface.c: alloc_interface(): Unable to acquire.\n" );

		kfree( psInterface );
		return ( NULL );
	}

	return ( psInterface );
}

/**
 * \par Description:
 * Cleanup function.
 */
static void ether_deinit( void *pInterface )
{
	EthInterface_s *psInterface = ( EthInterface_s * )pInterface;

	if ( psInterface )
	{
		/* FIXME: Shut down interface */

		/* Free memory */
		kfree( pInterface );
	}
}

static void ether_release( void *pInterface )
{
	EthInterface_s *psInterface = ( EthInterface_s * )pInterface;

	if ( psInterface )
		arp_expire_interface( psInterface );
}


static int eth_rx_thread( void *pData )
{
	EthInterface_s *psInterface = pData;

	for ( ;; )
	{
		PacketBuf_s *psPkt;
		uint32 nFlags;

		psPkt = remove_head_packet( &psInterface->ei_sInputQueue, INFINITE_TIMEOUT );

		if ( psPkt == NULL )
			continue;

		psPkt->pb_nIfIndex = psInterface->ei_nIfIndex;

		/* Examine the kernel interface flags */
		if ( get_interface_flags( psInterface->ei_nIfIndex, &nFlags ) < 0 )
		{
			kerndbg( KERN_DEBUG, "eth_rx_thread(): Kernel interface deleted while we slept!\n" );

			break;
		}

		if ( ( nFlags & IFF_UP ) == 0 )
		{
			free_pkt_buffer( psPkt );

			kerndbg( KERN_DEBUG, "eth_rx_thread(): Received packet to downed interface\n" );

			continue;
		}

		switch ( ntohw( psPkt->pb_uMacHdr.psEthernet->h_proto ) )
		{
		case ETH_P_ARP:
			arp_in( psInterface, psPkt );
			break;

		case ETH_P_RARP:
			kerndbg( KERN_DEBUG, "Received RARP packet\n" );
			free_pkt_buffer( psPkt );
			break;

		default:	// 2048 == ETH_P_IP == IP packets
			dispatch_net_packet( psPkt );
			break;
		}
	}

	return ( 0 );
}

static int ether_write( Route_s *psRoute, ipaddr_t pDstAddress, PacketBuf_s *psPkt )
{
	uint8 anBroadcastAddr[] = { 0xff, 0xff, 0xff, 0xff };
	ipaddr_t pLocalNet, pDstNet;

	if ( IP_SAMEADDR( anBroadcastAddr, pDstAddress ) )
		return ( arp_send_packet( psRoute->rt_psInterface->ni_pInterface, psPkt, pDstAddress ) );

	IP_COPYMASK( pLocalNet, psRoute->rt_psInterface->ni_anIpAddr, psRoute->rt_psInterface->ni_anSubNetMask );
	IP_COPYMASK( pDstNet, pDstAddress, psRoute->rt_psInterface->ni_anSubNetMask );

	/* If we have a gateway set and the destination is not on the local subnet, send the packet to the gateway */
	if ( ( psRoute->rt_nFlags & RTF_GATEWAY ) && !IP_SAMEADDR( pLocalNet, pDstNet ) )
	{
		return ( arp_send_packet( psRoute->rt_psInterface->ni_pInterface, psPkt, psRoute->rt_anGatewayAddr ) );
	}
	else
	{
		return ( arp_send_packet( psRoute->rt_psInterface->ni_pInterface, psPkt, pDstAddress ) );
	}
}

static void ether_notify( void *pInterface )
{
	EthInterface_s *psInterface = ( EthInterface_s * )pInterface;
	NetInterface_s *psKernIf = psInterface->ei_psInterface;
	int nError;

	if ( psKernIf->ni_nFlags & IFF_UP )
	{
		if ( ( psInterface->ei_nOldFlags & IFF_UP ) == 0 )
		{
			kerndbg( KERN_DEBUG, "ether_notify(): Opening network device %s\n", psInterface->ei_zDriverPath );

			/* Open device if necessary */
			if ( psInterface->ei_nDevice < 0 )
				psInterface->ei_nDevice = open( psInterface->ei_zDriverPath, O_RDONLY );

			kerndbg( KERN_DEBUG, "ether_notify(): Bringing up interface %s.\n", psKernIf->ni_zName );

			if ( psInterface->ei_nDevice >= 0 )
			{
				struct ifreq sIFReq;

				ioctl( psInterface->ei_nDevice, SIOCGIFHWADDR, &sIFReq );
				memcpy( psInterface->ei_anHwAddr, sIFReq.ifr_hwaddr.sa_data, ETH_ALEN );
				nError = ioctl( psInterface->ei_nDevice, SIOC_ETH_START, &psInterface->ei_sInputQueue );

				if ( nError < 0 )
				{
					kerndbg( KERN_WARNING, "ether_notify(): Unable to start interface %s (%d).\n", psKernIf->ni_zName, nError );

					close( psInterface->ei_nDevice );
					psInterface->ei_nDevice = -1;
					return;
				}

				psInterface->ei_nOldFlags = psKernIf->ni_nFlags;
			}
			else
			{
				kerndbg( KERN_FATAL, "eth_set_interface_flags(): Failed to load driver %s (%d)\n", psInterface->ei_zDriverPath, psInterface->ei_nDevice );
			}
		}
	}
	else
	{
		if ( psInterface->ei_nOldFlags & IFF_UP )
		{
			kerndbg( KERN_DEBUG, "ether_notify(): Taking down interface %s.\n", psKernIf->ni_zName );

			if ( ( nError = ioctl( psInterface->ei_nDevice, SIOC_ETH_STOP, NULL ) ) < 0 )
			{
				kerndbg( KERN_WARNING, "ether_notify(): Failed to shut down device driver for %s" " (%d)\n", psKernIf->ni_zName, nError );
			}
			else
			{
				close( psInterface->ei_nDevice );
				psInterface->ei_nDevice = -1;
				psInterface->ei_nOldFlags = psKernIf->ni_nFlags;
			}
		}
	}
}


/**
 * \par Description:
 * Handle specific Ethernet device ioctls.
 *
 * \param pArg - userspace pointer to command arguments
 */
static int ether_ioctl( void *pInterface, int nCmd, void *pArg )
{
	EthInterface_s *psInterface = pInterface;

	kerndbg( KERN_DEBUG, "ether_ioctl( 0x%p, 0x%8.8lX, 0x%p )\n", pInterface, ( uint32 )nCmd, pArg );
	kerndbg( KERN_DEBUG, "psInterface->ei_nDevice = %d\n", psInterface->ei_nDevice );

	if ( psInterface->ei_nDevice > -1 )
		/* Don't pass through pArg: it will always be a userspace pointer if not
		 * NULL, and our ioctl() will now be presented as originating in kernel
		 * space, causing strange memory access errors.
		 */
		return ioctl( psInterface->ei_nDevice, nCmd, pArg );

	return ( -ENETDOWN );
}


static int load_eth_driver( const char *pzPath, struct stat *psStat, void *pArg )
{
	EthInterface_s *psInterface;
	int nDriver = open( pzPath, O_RDWR );
	ipaddr_t anDefaultMask = { 255, 255, 255, 255 };
	char acDevName[128];
	static int nInterfaceNum = 0;

	if ( nDriver < 0 )
	{
		kerndbg( KERN_DEBUG, "load_eth_driver(): Failed to load driver %s (%d)\n", pzPath, nDriver );

		return ( 0 );
	}

	sprintf( acDevName, "eth%d", nInterfaceNum++ );

	psInterface = alloc_interface( acDevName );

	if ( psInterface == NULL )
	{
		kerndbg( KERN_FATAL, "load_eth_driver(): Interface allocation failed for %s\n", acDevName );

		--nInterfaceNum;

		close( nDriver );
		return ( 0 );
	}

	/* Initialise the rest of the Ethernet interface */
	init_net_queue( &psInterface->ei_sInputQueue );

	psInterface->ei_nDevice = nDriver;
	strcpy( psInterface->ei_zDriverPath, pzPath );

	/* Will call ether_notify() */
	set_interface_netmask( psInterface->ei_psInterface, anDefaultMask );
	set_interface_mtu( psInterface->ei_psInterface, ETH_DATA_LEN );

	/* Will call ether_notify() */
	set_interface_flags( psInterface->ei_psInterface, IFF_UP | IFF_RUNNING );

	kerndbg( KERN_DEBUG, "Activated Ethernet interface '%s' (0x%p)\n", psInterface->ei_psInterface->ni_zName, psInterface );

	/* Start Ethernet receive thread */
	sprintf( acDevName, "Ethernet Rx: %s", psInterface->ei_psInterface->ni_zName );
	psInterface->ei_hRxThread = spawn_kernel_thread( acDevName, eth_rx_thread, 5, 0, psInterface );

	wakeup_thread( psInterface->ei_hRxThread, false );

	/**
	 * Be apparently evil and release the reference to the kernel interface.
	 * This is okay, as any time an interface driver gets called the kernel
	 * already holds the lock for the interface.
	 */
	release_net_interface( psInterface->ei_psInterface );

	return ( 0 );
}

/**
 * Initialises the Ethernet interface class.
 * Loads drivers for all hardware devices.
 */
NetInterfaceOps_s *init_interfaces( void )
{
	init_arp();
	iterate_directory( "/dev/net/eth", false, load_eth_driver, NULL );

	return ( &g_sOps );
}
