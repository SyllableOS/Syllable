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

static sem_id g_hArpLock = -1;

static void arp_send_request( EthInterface_s* psInterface, ArpEntry_s* psEntry );

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static ArpEntry_s* arp_lookup_address( EthInterface_s* psInterface, ipaddr_t pAddress, bool bSendRequest )
{
  ArpEntry_s* psEntry;
  char	      zBuffer[128];
  
  for ( psEntry = psInterface->ei_psFirstARPEntry ; psEntry != NULL ; psEntry = psEntry->ae_psNext )
  {
    if ( IP_SAMEADDR( psEntry->ae_anIPAddress, pAddress ) ) {
      return( psEntry );
    }
  }
  psEntry = kmalloc( sizeof( ArpEntry_s ), MEMF_KERNEL | MEMF_CLEAR );
  if ( psEntry == NULL ) {
    printk( "Error: arp_lookup_address() out of memory\n" );
    return( NULL );
  }

  if ( init_net_queue( &psEntry->ae_sPackets ) < 0 ) {
    kfree( psEntry );
    return( NULL );
  }

  format_ipaddress( zBuffer, pAddress );
//  printk( "Create new ARP entry for address %s\n", zBuffer );
  
  psEntry->ae_nExpieringTime = get_system_time() + 1LL * 30LL * 1000000LL;
  psEntry->ae_bIsValid = false;
  IP_COPYADDR( psEntry->ae_anIPAddress, pAddress );

  psEntry->ae_psNext = psInterface->ei_psFirstARPEntry;
  psInterface->ei_psFirstARPEntry = psEntry;

  if ( bSendRequest ) {
    arp_send_request( psInterface, psEntry );
  }
  
  return( psEntry );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int arp_do_send( EthInterface_s* psInterface, PacketBuf_s* psPkt, uint8* pHwAddress )
{
  EthernetHeader_s*	psEthHdr;
  
  psPkt->pb_uMacHdr.pRaw = psPkt->pb_uNetworkHdr.pRaw - ETH_HLEN;
  if ( psPkt->pb_uMacHdr.pRaw < psPkt->pb_pHead ) {
    printk( "Error: arp_do_send() not enough space for ethernet header\n" );
    return( -EINVAL );
  }
  psPkt->pb_nSize += ETH_HLEN;
  
  psEthHdr = psPkt->pb_uMacHdr.psEthernet;
  
  memcpy( psEthHdr->h_dest, pHwAddress, ETH_ALEN );
  memcpy( psEthHdr->h_source, psInterface->ei_anHwAddr, ETH_ALEN );
  psEthHdr->h_proto = htonw( psPkt->pb_nProtocol );
  write( psInterface->ei_nDevice, psPkt->pb_uMacHdr.pRaw, psPkt->pb_nSize );
//  psInterface->ei_psOps->send( psInterface->ei_pDevice, psPkt->pb_uMacHdr.pRaw, psPkt->pb_nSize );
  free_pkt_buffer( psPkt );
  return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 *	Called with arp-lock help.
 * SEE ALSO:
 ****************************************************************************/

static void add_arp_entry( EthInterface_s* psInterface, uint8* pHwAddress, int nHwAddrLen, ipaddr_t pIpAddress )
{
  ArpEntry_s* psEntry;
  char	      zBuffer[128];
  
  psEntry = arp_lookup_address( psInterface, pIpAddress, false );

  format_ipaddress( zBuffer, pIpAddress );
//  printk( "add ARP entry for address %s\n", zBuffer );

  if ( psEntry == NULL ) {
    return;
  }

//  printk( "add_arp_entry() connect to %02x.%02x.%02x.%02x.%02x.%02x\n",
//	  pHwAddress[0], pHwAddress[1], pHwAddress[2], pHwAddress[3], pHwAddress[4], pHwAddress[5] );
  
  memcpy( psEntry->ae_anHardwareAddr, pHwAddress, nHwAddrLen );
  psEntry->ae_nExpieringTime = get_system_time() + 1LL * 60LL * 1000000LL;

  if ( psEntry->ae_bIsValid == false )
  {
    psEntry->ae_bIsValid = true;
    for (;;)
    {
      PacketBuf_s* psPkt = remove_head_packet( &psEntry->ae_sPackets, 0LL );
      if ( psPkt == NULL ) {
	break;
      }
//      printk( "Send delayed packet %08x\n", psPkt );
      arp_do_send( psInterface, psPkt, pHwAddress );
    }
  }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int arp_send_packet( EthInterface_s* psInterface, PacketBuf_s* psPkt, ipaddr_t pAddress )
{
  ArpEntry_s* psEntry;
  int	      nError = 0;
  char	      zBuffer[128];

  if ( (psInterface->ei_nFlags & IFF_UP) == 0 ) {
      return( -ENETDOWN );
  }
  
  LOCK( g_hArpLock );
  
  psEntry = arp_lookup_address( psInterface, pAddress, true );

  format_ipaddress( zBuffer, pAddress );
  
  if ( NULL == psEntry ) {
    nError = -EINVAL;
    printk( "arp_send_packet() failed to find ARP entry for address %s\n", zBuffer );
    goto error;
  }
  if ( psEntry->ae_bIsValid ) {
//    printk( "arp_send_packet() transmit packet to %s\n", zBuffer );
    arp_do_send( psInterface, psPkt, psEntry->ae_anHardwareAddr );
  } else {
//    printk( "arp_send_packet() queue packet for %s (%08x)\n", zBuffer, psEntry );
    enqueue_packet( &psEntry->ae_sPackets, psPkt );
  }
error:
  UNLOCK( g_hArpLock );
  return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void arp_reply( EthInterface_s* psInterface, PacketBuf_s* psPkt )
{
    ArpHeader_s* psArpHdr = psPkt->pb_uNetworkHdr.psArp;
    EthernetHeader_s*	psEthHdr = psPkt->pb_uMacHdr.psEthernet;
    uint8	       anBuffer[8];

    if ( psInterface == NULL ) {
	printk( "Error: arp_reply() packet have no interface pointer!\n" );
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
//    psInterface->ei_psOps->send( psInterface->ei_pDevice, psPkt->pb_uMacHdr.pRaw, psPkt->pb_nSize + ETH_HLEN );
    write( psInterface->ei_nDevice, psPkt->pb_uMacHdr.pRaw, psPkt->pb_nSize + ETH_HLEN );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/


static void arp_send_request( EthInterface_s* psInterface, ArpEntry_s* psEntry )
{
  PacketBuf_s* psPkt;
  ArpHeader_s* psArpHdr;
  uint8	       anBroadcastAddr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  char	       zBuffer[128];
  
  psPkt = alloc_pkt_buffer( ETH_HLEN + sizeof( ArpHeader_s ) );

  if ( psPkt == NULL ) {
    printk( "Error: arp_send_request() out of memory\n" );
    return;
  }

  format_ipaddress( zBuffer, psEntry->ae_anIPAddress );
//  printk( "Send ARP request for %s\n", zBuffer );

  psPkt->pb_nProtocol = ETH_P_ARP;

  psPkt->pb_uMacHdr.pRaw = psPkt->pb_pHead;
  psPkt->pb_uNetworkHdr.pRaw = psPkt->pb_uMacHdr.pRaw + ETH_HLEN;
  psPkt->pb_nSize = ETH_HLEN + sizeof( ArpHeader_s );
  
  psArpHdr = psPkt->pb_uNetworkHdr.psArp;

    // Address formats
  psArpHdr->ar_nProtoAddrFormat = htonw( ETH_P_IP );
  psArpHdr->ar_nHardAddrFormat  = htonw( ARPHRD_ETHER );
    // Address sizes
  psArpHdr->ar_nHardAddrSize    = ETH_ALEN;
  psArpHdr->ar_nProtoAddrSize   = IP_ADR_LEN;
  
  psArpHdr->ar_nCommand         = htonw( ARPOP_REQUEST );
  
    // Fill in ethernet addresses 
  memset( psArpHdr->ar_anHwTarget, 0, ETH_ALEN );
  memcpy( psArpHdr->ar_anHwSender, psInterface->ei_anHwAddr, ETH_ALEN );

    // Swap IP addresses
  IP_COPYADDR( psArpHdr->ar_anIpSender, psInterface->ei_psInterface->ni_anIpAddr );
  IP_COPYADDR( psArpHdr->ar_anIpTarget, psEntry->ae_anIPAddress );

  arp_do_send( psInterface, psPkt, anBroadcastAddr );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void arp_in( EthInterface_s* psInterface, PacketBuf_s* psPkt )
{
    ArpHeader_s* psArpHdr = (ArpHeader_s*) (psPkt->pb_uMacHdr.pRaw + ETH_HLEN);


    if ( psInterface == NULL ) {
	printk( "Error: arp_in() packet have no interface pointer!\n" );
	free_pkt_buffer( psPkt );
	return;
    }
    
    psPkt->pb_uNetworkHdr.psArp = psArpHdr;
  
//    printk( "Received ARP packet\n" );
    LOCK( g_hArpLock );
  
    if ( ntohw( psArpHdr->ar_nHardAddrFormat ) != ARPHRD_ETHER ) {
	printk( "arp_in() unknown hardware adress format %d\n", psArpHdr->ar_nHardAddrFormat );
	goto done;
    }
    if ( ntohw( psArpHdr->ar_nProtoAddrFormat ) != ETH_P_IP ) { /* Not realy sure if ETH_P_IP is the right ting
								 * to use, but it had the right value :) */
	printk( "arp_in() unknown protocol adress format %d\n", psArpHdr->ar_nProtoAddrFormat );
	goto done;
    }
    if ( psArpHdr->ar_nHardAddrSize != 6 ) {
	printk( "arp_in() Unknown hardware address size %d\n", psArpHdr->ar_nHardAddrSize );
	goto done;
    }
    if ( psArpHdr->ar_nProtoAddrSize != 4 ) {
	printk( "arp_in() Unknown protocol address size %d\n", psArpHdr->ar_nProtoAddrSize );
	goto done;
    }
    switch( ntohw( psArpHdr->ar_nCommand ) )
    {
	case ARPOP_REQUEST:
	    add_arp_entry( psInterface, psArpHdr->ar_anHwSender, psArpHdr->ar_nHardAddrSize, psArpHdr->ar_anIpSender );
	    if ( IP_SAMEADDR( psInterface->ei_psInterface->ni_anIpAddr, psArpHdr->ar_anIpTarget ) ) {
		arp_reply( psInterface, psPkt );
	    }
	    break;
	case ARPOP_REPLY:
	    add_arp_entry( psInterface, psArpHdr->ar_anHwSender, psArpHdr->ar_nHardAddrSize, psArpHdr->ar_anIpSender );
	    break;
    }
done:
    UNLOCK( g_hArpLock );
    free_pkt_buffer( psPkt );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void arp_expire_entry( ArpEntry_s* psEntry )
{
  char zBuffer[128];
  
  format_ipaddress( zBuffer, psEntry->ae_anIPAddress );
//  printk( "Expire ARP entry for %s\n", zBuffer );
  
  for (;;)
  {
    PacketBuf_s* psPkt = remove_head_packet( &psEntry->ae_sPackets, 0LL );
    if ( psPkt == NULL ) {
      break;
    }
    printk( "arp_expire_entry() Throw away packet %p\n", psPkt );
    free_pkt_buffer( psPkt );
  }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int arp_thread( void* pData )
{
    int		nInterface = 1;

    for (;;)
    {
	ArpEntry_s*	psEntry;
	ArpEntry_s*	psPrev = NULL;
	NetInterface_s*	psInterface;
	bigtime_t	nCurTime = get_system_time();
	int	    	nCnt = 0;

	if ( nInterface >= MAX_NET_INTERFACES ) {
	    nInterface = 0;
	}
	
	LOCK( g_hArpLock );
	
	while( (psInterface = get_net_interface( nInterface++ )) == NULL ) {
	    nCnt++;
	    if ( nInterface >= MAX_NET_INTERFACES ) {
		nInterface = 1;
	    }
	    if ( nCnt == MAX_NET_INTERFACES ) {
		break;
	    }
	}
	if( psInterface != NULL )
	{
	    for ( psEntry = ((EthInterface_s*)psInterface->ni_pInterface)->ei_psFirstARPEntry ; psEntry != NULL ;  )
	    {
		if ( psEntry->ae_nExpieringTime < nCurTime )
		{
		    ArpEntry_s* psTmp = psEntry;
		    arp_expire_entry( psEntry );
	
		    if ( psPrev == NULL ) {
			((EthInterface_s*)psInterface->ni_pInterface)->ei_psFirstARPEntry = psEntry->ae_psNext;
		    } else {
			psPrev->ae_psNext = psEntry->ae_psNext;
		    }
		    psEntry = psEntry->ae_psNext;
		    delete_net_queue( &psTmp->ae_sPackets );
		    kfree( psTmp );
		    continue;
		}
      
		if ( psEntry->ae_bIsValid == false ) {
		    arp_send_request( psInterface->ni_pInterface, psEntry );
		}
		psPrev = psEntry;
		psEntry = psEntry->ae_psNext;
	    }
	}
	UNLOCK( g_hArpLock );
	snooze( 1000000 );
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_arp()
{
  thread_id hArpThread;

  g_hArpLock = create_semaphore( "arp_lock", 1, SEM_REQURSIVE );
    
  hArpThread = spawn_kernel_thread( "arp_thread", arp_thread, 200, 0, NULL );
  wakeup_thread( hArpThread, false );
}
