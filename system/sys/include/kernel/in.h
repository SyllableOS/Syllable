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

#ifndef __F_KERNEL_IN_H__
#define __F_KERNEL_IN_H__

#include <net/in.h>

#include <kernel/types.h>
#include <kernel/socket.h>

/* Structure describing an Internet (IP) socket address. */
#define __SOCK_SIZE__	16		/* sizeof(struct sockaddr)	*/
struct sockaddr_in {
  sa_family_t	sin_family;	/* Address family		*/
  uint16		sin_port;	/* Port number			*/
  ipaddr_t		sin_addr;	/* Internet address		*/

  /* Pad to size of `struct sockaddr'. */
  uint8		sin_zero[__SOCK_SIZE__ - sizeof(short int) -
			sizeof(unsigned short int) - sizeof(ipaddr_t)];
};

/*
 * Definitions of the bits in an Internet address integer.
 * On subnets, host and network parts are found according
 * to the subnet mask, not these masks.
 */

/* Internet address. */
struct in_addr {
  uint32 s_addr;
};

/* Standard well-defined IP protocols.  */
enum {
	IPPROTO_IP   = 0,		/* Dummy protocol for TCP		*/
	IPPROTO_ICMP = 1,		/* Internet Control Message Protocol	*/
	IPPROTO_IGMP = 2,		/* Internet Group Management Protocol	*/
	IPPROTO_IPIP = 4,		/* IPIP tunnels (older KA9Q tunnels use 94) */
	IPPROTO_TCP  = 6,		/* Transmission Control Protocol	*/
	IPPROTO_EGP  = 8,		/* Exterior Gateway Protocol		*/
	IPPROTO_PUP  = 12,		/* PUP protocol				*/
	IPPROTO_UDP  = 17,		/* User Datagram Protocol		*/
	IPPROTO_IDP  = 22,		/* XNS IDP protocol			*/
	IPPROTO_RSVP = 46,		/* RSVP protocol			*/
	IPPROTO_GRE  = 47,		/* Cisco GRE tunnels (rfc 1701,1702)	*/

	IPPROTO_IPV6 = 41,		/* IPv6-in-IPv4 tunnelling		*/

	IPPROTO_PIM  = 103,		/* Protocol Independent Multicast	*/

	IPPROTO_RAW  = 255,		/* Raw IP packets			*/
	IPPROTO_MAX
};

#define IN_CLASSA(a)		((((long int) (a)) & 0x80000000) == 0)
#define IN_CLASSA_NET		0xff000000
#define IN_CLASSA_NSHIFT	24
#define IN_CLASSA_HOST		(0xffffffff & ~IN_CLASSA_NET)
#define IN_CLASSA_MAX		128

#define IN_CLASSB(a)		((((long int) (a)) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET		0xffff0000
#define IN_CLASSB_NSHIFT	16
#define IN_CLASSB_HOST		(0xffffffff & ~IN_CLASSB_NET)
#define IN_CLASSB_MAX		65536

#define IN_CLASSC(a)		((((long int) (a)) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET		0xffffff00
#define IN_CLASSC_NSHIFT	8
#define IN_CLASSC_HOST		(0xffffffff & ~IN_CLASSC_NET)

#define IN_CLASSD(a)		((((long int) (a)) & 0xf0000000) == 0xe0000000)
#define IN_MULTICAST(a)		IN_CLASSD(a)
#define IN_MULTICAST_NET	0xF0000000

#define IN_EXPERIMENTAL(a)	((((long int) (a)) & 0xf0000000) == 0xf0000000)
#define IN_BADCLASS(a)		IN_EXPERIMENTAL((a))

/* Address to accept any incoming messages. */
#define INADDR_ANY		((unsigned long int) 0x00000000)

/* Address to send to all hosts. */
#define INADDR_BROADCAST	((unsigned long int) 0xffffffff)

/* Address indicating an error return. */
#define INADDR_NONE		((unsigned long int) 0xffffffff)

/* Network number for local host loopback. */
#define IN_LOOPBACKNET		127

/* Address to loopback in software to local host.  */
#define INADDR_LOOPBACK		0x7f000001	/* 127.0.0.1   */
#define IN_LOOPBACK(a)		((((long int) (a)) & 0xff000000) == 0x7f000000)

/* Defines for Multicast INADDR */
#define INADDR_UNSPEC_GROUP   	0xe0000000U	/* 224.0.0.0   */
#define INADDR_ALLHOSTS_GROUP 	0xe0000001U	/* 224.0.0.1   */
#define INADDR_ALLRTRS_GROUP    0xe0000002U	/* 224.0.0.2 */
#define INADDR_MAX_LOCAL_GROUP  0xe00000ffU	/* 224.0.0.255 */

unsigned long  ntohl(unsigned long _val);
unsigned short ntohs(unsigned short _val);

static inline unsigned long htonl(unsigned long _val)
{
  return (_val << 24) | ((_val&0xff00) << 8) | ((_val&0xff0000) >> 8) | (_val >> 24);
}
#define ntohl(x) htonl(x)

static inline unsigned short htons(unsigned short _val)
{
  return (_val << 8) | (_val >> 8);
}
#define ntohs(x) htons(x)

/* Some random defines to make it easier in the kernel.. */
#define LOOPBACK(x)		(((x) & htonl(0xff000000)) == htonl(0x7f000000))
#define MULTICAST(x)	(((x) & htonl(0xf0000000)) == htonl(0xe0000000))
#define BADCLASS(x)		(((x) & htonl(0xf0000000)) == htonl(0xf0000000))
#define ZERONET(x)		(((x) & htonl(0xff000000)) == htonl(0x00000000))
#define LOCAL_MCAST(x)	(((x) & htonl(0xFFFFFF00)) == htonl(0xE0000000))

#endif	/* __F_KERNEL_IN_H__ */
