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

#ifndef __F_KERNEL_IP_H__
#define __F_KERNEL_IP_H__

#include <syllable/inttypes.h>
#include <kernel/kdebug.h>

#define	IP_CLASSA(x)	((x[0] & 0x80) == 0x00)	/* IP Class A address   */
#define	IP_CLASSB(x)	((x[0] & 0xc0) == 0x80)	/* IP Class B address   */
#define	IP_CLASSC(x)	((x[0] & 0xe0) == 0xc0)	/* IP Class C address   */
#define	IP_CLASSD(x)	((x[0] & 0xf0) == 0xe0)	/* IP Class D address   */
#define	IP_CLASSE(x)	((x[0] & 0xf8) == 0xf0)	/* IP Class E address   */

#define IP_SAMEADDR(ip1,ip2) 	 (*((uint32 *) ip1) == *((uint32 *) ip2))
#define IP_COPYADDR(ipto,ipfrom) (*((uint32 *) ipto) = *((uint32 *) ipfrom))
#define IP_COPYMASK(ipto,ipfrom,mask) \
        (*((uint32 *) ipto) = *((uint32 *) ipfrom) & *((uint32 *) mask) )
#define IP_MASKADDR(ip1,ip2) 	 ((*((uint32 *) ip1) & *((uint32 *) ip2)))
#define IP_INVMASKADDR(ip1,ip2)  (*((uint32 *) ip1) & ~(*((uint32 *) ip2)))
#define IP_MAKEADDR( ip, n0, n1, n2, n3 ) \
        do { ip[0] = n0; ip[1] = n1; ip[2] = n2; ip[3] = n3; } while(0)
#define IP_GET_HDR_LEN( ip ) (((uint32)(ip)->iph_nHdrSize)<<2)
/* Some Assigned Protocol Numbers */

/* IP Formatting - expects an ipaddr_p */
#ifdef __ENABLE_DEBUG__
#define USE_FORMAT_IP( count )  char __aacIpAddr[count][16]
#define FORMATTED_IP( index )   __aacIpAddr[index]
#define FORMAT_IP( ip, index ) \
	sprintf( __aacIpAddr[index], "%d.%d.%d.%d", \
			     (int)(ip)[0], (int)(ip)[1], (int)(ip)[2], (int)(ip)[3] )
#else
#define USE_FORMAT_IP( count )
#define FORMATTED_IP( index )		"debug mode off"
#define FORMAT_IP( ip, index )
#endif

#define IP_FRAG_TIME	(30LL * 1000000LL)	/* Time before discarding partly received packets */

#define IPT_ICMP	1	/* protocol type for ICMP packets       */
#define IPT_TCP		6	/* protocol type for TCP packets        */
#define IPT_EGP		8	/* protocol type for EGP packets        */
#define IPT_UDP		17	/* protocol type for UDP packets        */
#define IPT_RAW		255

#define IP_VERSION	4	/* Current protocol version */
#define IP_MIN_SIZE	5	/* Minimum header size */

#define IP_MORE_FRAGMENTS	0x2000
#define IP_DONT_FRAGMENT	0x4000
#define IP_FRAGOFF_MASK		0x1fff

struct _IpHeader {
	/* WARNING: size/version Works for little-endian only */
	uint8 iph_nHdrSize:4,	/* Header size (in int32's) */
	iph_nVersion:4;

	uint8 iph_nTypeOfService;
	uint16 iph_nPacketSize;
	uint16 iph_nPacketId;
	uint16 iph_nFragOffset;
	uint8 iph_nTimeToLive;
	uint8 iph_nProtocol;
	uint16 iph_nCheckSum;
	ipaddr_t iph_nSrcAddr;
	ipaddr_t iph_nDstAddr;
	/*The options start here. */
};

#endif	/* __F_KERNEL_IP_H__ */
