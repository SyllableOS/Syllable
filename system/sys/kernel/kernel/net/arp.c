
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999  Kurt Skauen
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
#include <posix/unistd.h>

#include <atheos/kernel.h>
#include <atheos/time.h>
#include <atheos/semaphore.h>

#include <atheos/socket.h>
#include <net/net.h>
#include <net/ip.h>
#include <net/if_ether.h>
#include <net/if_arp.h>

#include <macros.h>

/* Selective debugging level overrides */
#ifdef KERNEL_DEBUG_NET
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET
#endif

#ifdef KERNEL_DEBUG_NET_ARP
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET_ARP
#endif

static sem_id g_hArpLock = -1;

static void arp_send_request( EthInterface_s *psInterface, ArpEntry_s *psEntry );
static int arp_do_send( EthInterface_s *psInterface, PacketBuf_s *psPkt, uint8 *pHwAddress );
static void add_arp_entry( EthInterface_s *psInterface, uint8 *pHwAddress, int nHwAddrLen, ipaddr_t pIpAddress );

/**
 * \par Description:
 * Consults the ARP table attached to the specified interface for the entry
 * matching pAddress.  If no such entry exists, one is created.  If a new
 * entry is created and bSendRequest is true, then an ARP request is sent
 * to retrieve the hardware address for the specified IP.
 *
 * Must be called with g_hArpLock held.
 **/
static ArpEntry_s *arp_lookup_address( EthInterface_s *psInterface, ipaddr_p pAddress, bool bSendRequest )
{
	ArpEntry_s *psEntry;
	uint8 anIPBroadcastAddr[] = { 0xff, 0xff, 0xff, 0xff };

	/* Look for existing entry */
	for ( psEntry = psInterface->ei_psFirstARPEntry; psEntry != NULL; psEntry = psEntry->ae_psNext )
	{
		if ( IP_SAMEADDR( psEntry->ae_anIPAddress, pAddress ) )
			return ( psEntry );
	}

	/* Create a new entry */
	psEntry = kmalloc( sizeof( ArpEntry_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( psEntry == NULL )
	{
		kerndbg( KERN_FATAL, "arp_lookup_address(): Out of memory\n" );

		return ( NULL );
	}

	/* Create a packet queue for this entry */
	if ( init_net_queue( &psEntry->ae_sPackets ) < 0 )
	{
		kfree( psEntry );

		return ( NULL );
	}

	/* Set entry expiry time */
	psEntry->ae_nExpiryTime = get_system_time() + 1LL * 30LL * 1000000LL;
	psEntry->ae_bIsValid = false;
	IP_COPYADDR( psEntry->ae_anIPAddress, pAddress );

	/* Insert into interface's entry list */
	psEntry->ae_psNext = psInterface->ei_psFirstARPEntry;
	psInterface->ei_psFirstARPEntry = psEntry;

	/**
	 * If we are inserting the broadcast address, it does not need to be
	 * confirmed separately.  Call add_arp_entry() to validate the new
	 * entry and ignore bSendRequest.
	 */
	if ( IP_SAMEADDR( pAddress, anIPBroadcastAddr ) )
	{
		uint8 anHWBroadcastAddr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

		kerndbg( KERN_INFO, "Adding ARP entry for broadcast address\n" );

		add_arp_entry( psInterface, anHWBroadcastAddr, ETH_ALEN, pAddress );

		memcpy( psEntry->ae_anHardwareAddr, anHWBroadcastAddr, ETH_ALEN );
		psEntry->ae_bIsValid = true;
		bSendRequest = false;
	}

	if ( bSendRequest )
		arp_send_request( psInterface, psEntry );

	return ( psEntry );
}


/**
 * \par Description:
 * Writes a packet to the underlying hardware driver.  The packet is
 * expected to have bytes allocated for an Ethernet header before the
 * network layer header.  This function sends directly to a hardware
 * address.
 * 
 * The packet buffer is freed at the end of this function if no error
 * occurs.  If an error is returned, the caller must free the packet
 * buffer.
 */
static int arp_do_send( EthInterface_s *psInterface, PacketBuf_s *psPkt, uint8 *pHwAddress )
{
	EthernetHeader_s *psEthHdr;

	psPkt->pb_uMacHdr.pRaw = psPkt->pb_uNetworkHdr.pRaw - ETH_HLEN;
	if ( psPkt->pb_uMacHdr.pRaw < psPkt->pb_pHead )
	{
		kerndbg( KERN_FATAL, "arp_do_send(): Not enough space for ethernet header\n" );

		return ( -EINVAL );
	}

	psPkt->pb_nSize += ETH_HLEN;
	psEthHdr = psPkt->pb_uMacHdr.psEthernet;

	/* Fill in ethernet header */
	memcpy( psEthHdr->h_dest, pHwAddress, ETH_ALEN );
	memcpy( psEthHdr->h_source, psInterface->ei_anHwAddr, ETH_ALEN );
	psEthHdr->h_proto = htonw( psPkt->pb_nProtocol );

	/* Write to the device */
	write( psInterface->ei_nDevice, psPkt->pb_uMacHdr.pRaw, psPkt->pb_nSize );

	free_pkt_buffer( psPkt );

	return ( 0 );
}

/**
 * Adds an ARP entry for a verified hardware address.  This may be called in
 * response to any incoming packet which confirms another host's IP<->Ether
 * address mapping.
 *
 * If an entry had packets queued waiting to be sent (because the hardware
 * address was unconfirmed), they will be sent by this function.
 *
 * Must be called with g_hArpLock held.
 */
static void add_arp_entry( EthInterface_s *psInterface, uint8 *pHwAddress, int nHwAddrLen, ipaddr_t pIpAddress )
{
	ArpEntry_s *psEntry;

	psEntry = arp_lookup_address( psInterface, pIpAddress, false );

//      format_ipaddress( zBuffer, pIpAddress );
//      printk( "add ARP entry for address %s\n", zBuffer );

	if ( psEntry == NULL )
		return;

//      printk( "add_arp_entry() connect to %02x.%02x.%02x.%02x.%02x.%02x\n",
//                    pHwAddress[0], pHwAddress[1], pHwAddress[2],
//                    pHwAddress[3], pHwAddress[4], pHwAddress[5] );

	memcpy( psEntry->ae_anHardwareAddr, pHwAddress, nHwAddrLen );
	psEntry->ae_nExpiryTime = get_system_time() + 1LL * 60LL * 1000000LL;

	if ( psEntry->ae_bIsValid == false )
	{
		psEntry->ae_bIsValid = true;
		for ( ;; )
		{
			PacketBuf_s *psPkt = remove_head_packet( &psEntry->ae_sPackets, 0LL );

			if ( psPkt == NULL )
				break;

			if ( arp_do_send( psInterface, psPkt, pHwAddress ) < 0 )
			{
				kerndbg( KERN_DEBUG, "add_arp_entry(): Freeing packet (arp_do_send() failed).\n" );

				free_pkt_buffer( psPkt );
			}
		}
	}
}

/**
 * \par Description:
 * Sends a packet to a directly connected host, identified by IP address
 * pAddress.  If the host's hardware address is unknown, a request will
 * be sent to find out and the packet queued for sending when the hardware
 * address is known.
 *
 * The packet buffer is consumed by this function unless an error is returned.
 */
int arp_send_packet( EthInterface_s *psInterface, PacketBuf_s *psPkt, ipaddr_p pAddress )
{
	ArpEntry_s *psEntry;
	int nError = 0;
#ifdef __ENABLE_DEBUG__
	char zBuffer[128];
#endif

	if ( ( psInterface->ei_nOldFlags & IFF_UP ) == 0 )
		return ( -ENETDOWN );

	LOCK( g_hArpLock );

	psEntry = arp_lookup_address( psInterface, pAddress, true );

	if ( NULL == psEntry )
	{
		nError = -EINVAL;
		kerndbg( KERN_DEBUG, "arp_send_packet(): Failed to find ARP entry for address %s\n", zBuffer );

		goto error;
	}

	/* Either send the packet if a hardware address is known, or else enqueue */
	if ( psEntry->ae_bIsValid )
	{
		nError = arp_do_send( psInterface, psPkt, psEntry->ae_anHardwareAddr );
	}
	else
	{
		enqueue_packet( &psEntry->ae_sPackets, psPkt );
	}

      error:
	UNLOCK( g_hArpLock );

	return ( nError );
}

/**
 * \par Description:
 * Send a reply to an ARP request for our IP address.
 *
 * The packet buffer is not consumed by this function and should be freed
 * by the caller.
 */
static void arp_reply( EthInterface_s *psInterface, PacketBuf_s *psPkt )
{
	ArpHeader_s *psArpHdr = psPkt->pb_uNetworkHdr.psArp;
	EthernetHeader_s *psEthHdr = psPkt->pb_uMacHdr.psEthernet;
	uint8 anBuffer[8];

	if ( psInterface == NULL )
	{
		kerndbg( KERN_FATAL, "arp_reply(): psInterface is NULL!\n" );

		return;
	}

	// Fill in ethernet addresses 
	memcpy( psArpHdr->ar_anHwTarget, psArpHdr->ar_anHwSender, psArpHdr->ar_nHardAddrSize );
	memcpy( psArpHdr->ar_anHwSender, psInterface->ei_anHwAddr, ETH_ALEN );

	// Swap IP addresses
	memcpy( anBuffer, psArpHdr->ar_anIpSender, psArpHdr->ar_nProtoAddrSize );
	memcpy( psArpHdr->ar_anIpSender, psArpHdr->ar_anIpTarget, psArpHdr->ar_nProtoAddrSize );
	memcpy( psArpHdr->ar_anIpTarget, anBuffer, psArpHdr->ar_nProtoAddrSize );

	memcpy( psEthHdr->h_source, psInterface->ei_anHwAddr, ETH_ALEN );
	memcpy( psEthHdr->h_dest, psArpHdr->ar_anHwTarget, ETH_ALEN );


	psArpHdr->ar_nCommand = htonw( ARPOP_REPLY );
	write( psInterface->ei_nDevice, psPkt->pb_uMacHdr.pRaw, psPkt->pb_nSize + ETH_HLEN );
}

/**
 * \par Description:
 * Sends an ARP request for the specified entry.
 */
static void arp_send_request( EthInterface_s *psInterface, ArpEntry_s *psEntry )
{
	PacketBuf_s *psPkt;
	ArpHeader_s *psArpHdr;
	uint8 anBroadcastAddr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	psPkt = alloc_pkt_buffer( ETH_HLEN + sizeof( ArpHeader_s ) );
	if ( psPkt == NULL )
	{
		kerndbg( KERN_FATAL, "arp_send_request(): Out of memory\n" );

		return;
	}

//      format_ipaddress( zBuffer, psEntry->ae_anIPAddress );
//      printk( "Send ARP request for %s\n", zBuffer );
	psPkt->pb_nProtocol = ETH_P_ARP;

	psPkt->pb_uMacHdr.pRaw = psPkt->pb_pHead;
	psPkt->pb_uNetworkHdr.pRaw = psPkt->pb_uMacHdr.pRaw + ETH_HLEN;
	psPkt->pb_nSize = ETH_HLEN + sizeof( ArpHeader_s );

	psArpHdr = psPkt->pb_uNetworkHdr.psArp;

	// Address formats
	psArpHdr->ar_nProtoAddrFormat = htonw( ETH_P_IP );
	psArpHdr->ar_nHardAddrFormat = htonw( ARPHRD_ETHER );

	// Address sizes
	psArpHdr->ar_nHardAddrSize = ETH_ALEN;
	psArpHdr->ar_nProtoAddrSize = IP_ADR_LEN;

	psArpHdr->ar_nCommand = htonw( ARPOP_REQUEST );

	// Fill in ethernet addresses 
	memset( psArpHdr->ar_anHwTarget, 0, ETH_ALEN );
	memcpy( psArpHdr->ar_anHwSender, psInterface->ei_anHwAddr, ETH_ALEN );

	// Swap IP addresses
	IP_COPYADDR( psArpHdr->ar_anIpSender, psInterface->ei_psInterface->ni_anIpAddr );
	IP_COPYADDR( psArpHdr->ar_anIpTarget, psEntry->ae_anIPAddress );

	if ( arp_do_send( psInterface, psPkt, anBroadcastAddr ) < 0 )
	{
		kerndbg( KERN_DEBUG, "arp_send_request(): arp_do_send() failed; freeing packet.\n" );

		free_pkt_buffer( psPkt );
	}
}


/**
 * \par Description:
 * Accepts an incoming ARP packet for processing.  The packet is always
 * consumed, regardless of whether it is successfully processed.
 */
void arp_in( EthInterface_s *psInterface, PacketBuf_s *psPkt )
{
	ArpHeader_s *psArpHdr = ( ArpHeader_s * )( psPkt->pb_uMacHdr.pRaw + ETH_HLEN );

	if ( psInterface == NULL )
	{
		kerndbg( KERN_DEBUG, "arp_in(): Packet has no interface pointer!\n" );

		free_pkt_buffer( psPkt );
		return;
	}

	psPkt->pb_uNetworkHdr.psArp = psArpHdr;

	LOCK( g_hArpLock );

	if ( ntohw( psArpHdr->ar_nHardAddrFormat ) != ARPHRD_ETHER )
	{
		kerndbg( KERN_INFO, "arp_in(): Unknown hardware address format %d\n", psArpHdr->ar_nHardAddrFormat );

		goto done;
	}

	if ( ntohw( psArpHdr->ar_nProtoAddrFormat ) != ETH_P_IP )
	{
		/* Not realy sure if ETH_P_IP is the right thing
		 * to use, but it had the right value :) */
		kerndbg( KERN_INFO, "arp_in(): Unknown protocol address format %d\n", psArpHdr->ar_nProtoAddrFormat );

		goto done;
	}

	if ( psArpHdr->ar_nHardAddrSize != ETH_ALEN )
	{
		kerndbg( KERN_INFO, "arp_in(): Unsupported hardware address size %d\n", psArpHdr->ar_nHardAddrSize );

		goto done;
	}

	if ( psArpHdr->ar_nProtoAddrSize != IP_ADR_LEN )
	{
		kerndbg( KERN_INFO, "arp_in(): Unsupported protocol address size %d\n", psArpHdr->ar_nProtoAddrSize );

		goto done;
	}

	switch ( ntohw( psArpHdr->ar_nCommand ) )
	{
		/* Someone has requested a HW address */
	case ARPOP_REQUEST:
		/* Validate the requesting host */
		add_arp_entry( psInterface, psArpHdr->ar_anHwSender, psArpHdr->ar_nHardAddrSize, psArpHdr->ar_anIpSender );

		/* If they have requested our HW address, reply */
		if ( IP_SAMEADDR( psInterface->ei_psInterface->ni_anIpAddr, psArpHdr->ar_anIpTarget ) )
			arp_reply( psInterface, psPkt );
		break;

		/* Someone has responded to our HW address lookup request */
	case ARPOP_REPLY:
		add_arp_entry( psInterface, psArpHdr->ar_anHwSender, psArpHdr->ar_nHardAddrSize, psArpHdr->ar_anIpSender );
		break;
	}

      done:
	UNLOCK( g_hArpLock );
	free_pkt_buffer( psPkt );
}


/**
 * \par Description:
 * Expires an ARP entry for an interface, dropping any unsent packets.
 */
static void arp_expire_entry( ArpEntry_s *psEntry )
{
	for ( ;; )
	{
		PacketBuf_s *psPkt = remove_head_packet( &psEntry->ae_sPackets, 0LL );

		if ( psPkt == NULL )
			break;

		kerndbg( KERN_DEBUG_LOW, "arp_expire_entry(): Throwing away packet %p\n", psPkt );

		free_pkt_buffer( psPkt );
	}
}

void arp_expire_interface( EthInterface_s *psEthIf )
{
#ifdef __ENABLE_DEBUG__
	NetInterface_s *psIf = psEthIf->ei_psInterface;
#endif
	ArpEntry_s *psEntry;

	kerndbg( KERN_DEBUG_LOW, __FUNCTION__ "(): Expiring packets attached to interface %s.\n", psIf->ni_zName );

	while ( ( psEntry = psEthIf->ei_psFirstARPEntry ) != NULL )
	{
		psEthIf->ei_psFirstARPEntry = psEntry->ae_psNext;

		/* Dispose of entry */
		arp_expire_entry( psEntry );

		delete_net_queue( &psEntry->ae_sPackets );
		kfree( psEntry );
	}

	if ( LOCK( g_hArpLock ) < 0 )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Error acquiring ARP lock.\n" );

		return;
	}

	UNLOCK( g_hArpLock );
}


/**
 * \par Description:
 * Performs the background tasks involved with maintaining the ARP tables
 * for Ethernet interfaces.
 *
 * The tasks performed are background validation of new ARP entries and
 * expiry of old entries.
 */
static int arp_thread( void *pData )
{
	int nInterface = 0;

	for ( ;; )
	{
		ArpEntry_s *psEntry = NULL;
		ArpEntry_s *psPrev = NULL;
		NetInterface_s *psKernIf;
		EthInterface_s *psInterface;
		bigtime_t nCurTime = get_system_time();
		int nCnt = 0;

		if ( nInterface >= MAX_NET_INTERFACES )
			nInterface = 0;

		LOCK( g_hArpLock );

		/* Find next ARP-using interface */
		while ( ( psKernIf = get_net_interface( nInterface++ ) ) == NULL || ( psKernIf->ni_nFlags & IFF_NOARP ) == IFF_NOARP )
		{
			nCnt++;

			/* If we obtained a non-ARP interface, release it */
			if ( psKernIf != NULL )
				release_net_interface( psKernIf );

			/* Wrap back to start if we move past the end of the interface array */
			if ( nInterface >= MAX_NET_INTERFACES )
				nInterface = 0;

			/* Stop if we've scanned the entire array fruitlessly. */
			if ( nCnt == MAX_NET_INTERFACES )
				break;
		}

		if ( psKernIf != NULL )
		{
			psInterface = ( EthInterface_s * )psKernIf->ni_pInterface;

			/* Check all the ARP entries attached to the interface */
			for ( psEntry = psInterface->ei_psFirstARPEntry; psEntry != NULL; )
			{
				/* Expire old interface ARP entries */
				if ( psEntry->ae_nExpiryTime < nCurTime )
				{
					ArpEntry_s *psTmp = psEntry;

					/* Unlink entry from interface list */
					if ( psPrev == NULL )
						psInterface->ei_psFirstARPEntry = psEntry->ae_psNext;
					else
						psPrev->ae_psNext = psEntry->ae_psNext;

					/* Advance current entry */
					psEntry = psEntry->ae_psNext;

					/* Dispose of entry */
					arp_expire_entry( psTmp );

					delete_net_queue( &psTmp->ae_sPackets );
					kfree( psTmp );
					continue;
				}

				/* Send ARP requests for unvalidated hosts */
				if ( psEntry->ae_bIsValid == false )
					arp_send_request( psInterface, psEntry );

				/* Move to next entry */
				psPrev = psEntry;
				psEntry = psEntry->ae_psNext;
			}

			release_net_interface( psKernIf );
		}

		UNLOCK( g_hArpLock );

		/* Run again in one second */
		snooze( 1000000 );
	}

	/* Keep GCC happy */
	return 0;
}

/**
 * \par Description:
 * 
 * Initialises the ARP handling code and starts the ARP thread in the
 * background.
 */
void init_arp( void )
{
	thread_id hArpThread;

	g_hArpLock = create_semaphore( "arp_lock", 1, SEM_RECURSIVE );

	hArpThread = spawn_kernel_thread( "ARP Expiry Thread", arp_thread, 200, 0, NULL );
	wakeup_thread( hArpThread, false );
}
