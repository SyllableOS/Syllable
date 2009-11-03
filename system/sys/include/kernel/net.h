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

#ifndef __F_KERNEL_NET_H__
#define __F_KERNEL_NET_H__

#include <net/nettypes.h>
#include <kernel/socket.h>
#include <kernel/in.h>
#include <kernel/if.h>
#include <kernel/if_ether.h>
#include <kernel/route.h>
#include <kernel/packet.h>

#define __BYTE(v,n) ((((uint32)v) >> (n*8))&0xff)
#define htonw(n) ( (__BYTE(n, 0 ) << 8) | __BYTE(n, 1 ) )
#define ntohw(n) ( (__BYTE(n, 0 ) << 8) | __BYTE(n, 1 ) )

/*
 *	This is a version of ip_compute_csum() optimized for IP headers,
 *	which always checksum on 4 octet boundaries.
 *
 *	By Jorge Cwik <jorge@laser.satlink.net>, adapted for linux by
 *	Arnt Gulbrandsen.
 *
 *	Adapted to AtheOS by Kurt Skauen.
 */

static inline uint16 ip_fast_csum( uint8 * iph, uint32 ihl )
{
  uint32 sum;

   __asm__ __volatile__(
"	movl (%1), %0;"
"	subl $4, %2;"
"	jbe 2f;"
"	addl 4(%1), %0;"
"	adcl 8(%1), %0;"
"	adcl 12(%1), %0;"
"1:	adcl 16(%1), %0;"
"	lea 4(%1), %1;"
"	decl %2;"
"	jne	1b;"
"	adcl $0, %0;"
"	movl %0, %2;"
"	shrl $16, %0;"
"	addw %w2, %w0;"
"	adcl $0, %0;"
"	notl %0;"
"2:"
    
	/* Since the input registers which are loaded with iph and ipl
	   are modified, we must also specify them as outputs, or gcc
	   will assume they contain their original values. */
	: "=r" (sum), "=r" (iph), "=r" (ihl)
	: "1" (iph), "2" (ihl));
  return(sum);
}

typedef struct _UdpPort UdpPort_s;
struct _UdpPort
{
    UdpPort_s*	up_psNext;
    NetQueue_s	up_sPackets;
    int		up_nPort;
};

struct _ArpEntry
{
	ArpEntry_s* ae_psNext;
	ipaddr_t    ae_anIPAddress;
	bigtime_t   ae_nExpiryTime;
	uint8       ae_anHardwareAddr[IH_MAX_HWA_LEN];
	NetQueue_s  ae_sPackets;
	bool        ae_bIsValid;
};

void init_ip( void );
void init_arp( void );
int  init_net_core( void );

enum
{
  NCFG_SET_DEFAULT_ADDRESS,
  NCFG_SET_ADDRESS,
};

struct ifc_set_address
{
  struct sockaddr address;
  struct sockaddr netmask;
};

#define MAX_NET_INTERFACES 64

int create_socket( bool bKernel, int nFamily, int nType, int nProtocol, bool bInitProtocol, Socket_s** ppsRes );

/* Entry points to networking code for incoming packets */
void ip_in( PacketBuf_s* psPkt, int nPktSize );
void tcp_in( PacketBuf_s* psPkt, int nDataLen );
void udp_in( PacketBuf_s* psPkt, int nDataLen );
void raw_in(PacketBuf_s * psPkt, int nDataLen);

RawPort_s *raw_find_port(uint8 a_nIPProtocol);

/* Try FORMAT_IP macro in net/ip.h */
void format_ipaddress( char* pzBuffer, ipaddr_t pAddress );
int parse_ipaddress( ipaddr_t pAddress, const char* pzBuffer );

/* Packet buffer routines */
PacketBuf_s* alloc_pkt_buffer( int nSize );
PacketBuf_s* clone_pkt_buffer(PacketBuf_s * psBuf);
void free_pkt_buffer( PacketBuf_s* psBuf );
void reserve_pkt_header( PacketBuf_s* psBuf, int nSize );

/* Packet queue routines */
int 	     init_net_queue( NetQueue_s* psQueue );
void	     delete_net_queue( NetQueue_s* psQueue );
void 	     enqueue_packet( NetQueue_s* psQueue, PacketBuf_s* psBuf );
PacketBuf_s* remove_head_packet( NetQueue_s* psQueue, bigtime_t nTimeout );

/* Send outgoing Ethernet packet */
int send_packet( Route_s* psRoute, ipaddr_t pDstAddress, PacketBuf_s* psPkt );
/* Dispatch incoming Ethernet packets */
void dispatch_net_packet( PacketBuf_s* psPkt );

/* Send IP packets */
int ip_send( PacketBuf_s* psPkt );
int ip_send_via( PacketBuf_s* psPkt, Route_s* psRoute );

#endif /* __F_KERNEL_NET_H__ */
