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

#ifndef __F_KERNEL_ICMP_H__
#define __F_KERNEL_ICMP_H__

#include <kernel/types.h>

#define ICMP_ECHOREPLY		0	/* Echo Reply			*/
#define ICMP_DEST_UNREACH	3	/* Destination Unreachable	*/
#define ICMP_SOURCE_QUENCH	4	/* Source Quench		*/
#define ICMP_REDIRECT		5	/* Redirect (change route)	*/
#define ICMP_ECHO			8	/* Echo Request			*/
#define ICMP_TIME_EXCEEDED	11	/* Time Exceeded		*/
#define ICMP_PARAMETERPROB	12	/* Parameter Problem		*/
#define ICMP_TIMESTAMP		13	/* Timestamp Request		*/
#define ICMP_TIMESTAMPREPLY	14	/* Timestamp Reply		*/
#define ICMP_INFO_REQUEST	15	/* Information Request		*/
#define ICMP_INFO_REPLY		16	/* Information Reply		*/
#define ICMP_ADDRESS		17	/* Address Mask Request		*/
#define ICMP_ADDRESSREPLY	18	/* Address Mask Reply		*/
#define NR_ICMP_TYPES		18


/* Codes for UNREACH. */
#define ICMP_NET_UNREACH	0	/* Network Unreachable		*/
#define ICMP_HOST_UNREACH	1	/* Host Unreachable		*/
#define ICMP_PROT_UNREACH	2	/* Protocol Unreachable		*/
#define ICMP_PORT_UNREACH	3	/* Port Unreachable		*/
#define ICMP_FRAG_NEEDED	4	/* Fragmentation Needed/DF set	*/
#define ICMP_SR_FAILED		5	/* Source Route failed		*/
#define ICMP_NET_UNKNOWN	6
#define ICMP_HOST_UNKNOWN	7
#define ICMP_HOST_ISOLATED	8
#define ICMP_NET_ANO		9
#define ICMP_HOST_ANO		10
#define ICMP_NET_UNR_TOS	11
#define ICMP_HOST_UNR_TOS	12
#define ICMP_PKT_FILTERED	13	/* Packet filtered */
#define ICMP_PREC_VIOLATION	14	/* Precedence violation */
#define ICMP_PREC_CUTOFF	15	/* Precedence cut off */
#define NR_ICMP_UNREACH		15	/* instead of hardcoding immediate value */

/* Codes for REDIRECT. */
#define ICMP_REDIR_NET		0	/* Redirect Net			*/
#define ICMP_REDIR_HOST		1	/* Redirect Host		*/
#define ICMP_REDIR_NETTOS	2	/* Redirect Net for TOS		*/
#define ICMP_REDIR_HOSTTOS	3	/* Redirect Host for TOS	*/

/* Codes for TIME_EXCEEDED. */
#define ICMP_EXC_TTL		0	/* TTL count exceeded		*/
#define ICMP_EXC_FRAGTIME	1	/* Fragment Reass time exceeded	*/


#if 0
/* ic_type field */
#define	ICT_ECHORP	0	/* Echo reply				*/
#define	ICT_DESTUR	3	/* Destination unreachable		*/
#define	ICT_SRCQ	4	/* Source quench			*/
#define	ICT_REDIRECT	5	/* Redirect message type		*/
#define	ICT_ECHORQ	8	/* Echo request				*/
#define	ICT_TIMEX	11	/* Time exceeded			*/
#define	ICT_PARAMP	12	/* Parameter Problem			*/
#define	ICT_TIMERQ	13	/* Timestamp request			*/
#define	ICT_TIMERP	14	/* Timestamp reply			*/
#define	ICT_INFORQ	15	/* Information request			*/
#define	ICT_INFORP	16	/* Information reply			*/
#define	ICT_MASKRQ	17	/* Mask request				*/
#define	ICT_MASKRP	18	/* Mask reply				*/

/* ic_code field */
#define	ICC_NETUR	0	/* dest unreachable, net unreachable	*/
#define	ICC_HOSTUR	1	/* dest unreachable, host unreachable	*/
#define	ICC_PROTOUR	2	/* dest unreachable, proto unreachable	*/
#define	ICC_PORTUR	3	/* dest unreachable, port unreachable	*/
#define	ICC_FNADF	4	/* dest unr, frag needed & don't frag	*/
#define	ICC_SRCRT	5	/* dest unreachable, src route failed	*/

#define	ICC_NETRD	0	/* redirect: net			*/
#define	ICC_HOSTRD	1	/* redirect: host			*/
#define	IC_TOSNRD	2	/* redirect: type of service, net	*/
#define	IC_TOSHRD	3	/* redirect: type of service, host	*/

#define	ICC_TIMEX	0	/* time exceeded, ttl			*/
#define	ICC_FTIMEX	1	/* time exceeded, frag			*/

#define	IC_HLEN		8	/* octets				*/

#define	IC_RDTTL	300	/* ttl for redirect routes		*/
#endif

/* ICMP packet format (following the IP header)				*/

struct	_ICMPHeader
{
  char	ic_nType;		/* type of message (ICMP_* above)*/
  char	ic_nCode;		/* code (ICC_* above)		*/
  short	ic_nChkSum;		/* checksum of ICMP header+data	*/

  union	{
    struct {
      uint16	nEchoID;	/* for echo type, a message id	*/
      uint16	nEchoSeq;	/* for echo type, a seq. number	*/
    } ic_u1;
    ipaddr_t	anGatewayAddr;		/* for redirect, gateway	*/
    struct {
      uint8	nPtr;		/* pointer, for ICMP_PARAMETERPROB	*/
      uint8	pad[3];
    } ic_u3;
    uint32	nZero;    /* must be zero         */
  } ic_uMsg;
};

#endif /* __F_KERNEL_ICMP_H__ */
