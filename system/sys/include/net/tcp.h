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

#ifndef __F_NET_TCP_H__
#define __F_NET_TCP_H__

#include <atheos/types.h>

typedef	long tcpseq;

/*
 * SEQCMP - sequence space comparator
 *	This handles sequence space wrap-around. Overlow/Underflow makes
 * the result below correct ( -, 0, + ) for any a, b in the sequence
 * space. Results:	result	implies
 *			  - 	 a < b
 *			  0 	 a = b
 *			  + 	 a > b
 */
#define	SEQCMP(a, b)	((a) - (b))

/* tcp packet format */

struct _TCPHeader
{
  uint16	tcp_sport;	/* source port			*/
  uint16	tcp_dport;	/* destination port		*/
  tcpseq	tcp_seq;	/* sequence			*/
  tcpseq	tcp_ack;	/* acknowledged sequence	*/
  char		tcp_offset;
  char		tcp_code;	/* control flags		*/
  uint16	tcp_window;	/* window advertisement		*/
  uint16	tcp_cksum;	/* check sum			*/
  uint16	tcp_urgptr;	/* urgent pointer		*/
};

/* TCP Control Bits */

#define	TCPF_URG	0x20	/* urgent pointer is valid		*/
#define	TCPF_ACK	0x10	/* acknowledgement field is valid	*/
#define	TCPF_PSH	0x08	/* this segment requests a push		*/
#define	TCPF_RST	0x04	/* reset the connection			*/
#define	TCPF_SYN	0x02	/* synchronize sequence numbers		*/
#define	TCPF_FIN	0x01	/* sender has reached end of its stream	*/

#define	TCPMHLEN	  20	/* minimum TCP header length		*/
#define	TCPHOFFSET	0x50	/* tcp_offset value for TCPMHLEN	*/
#define	TCP_HLEN(ptcp)		(((ptcp)->tcp_offset & 0xf0)>>2)

/* TCP Options */

#define	TPO_EOOL	0	/* end Of Option List			*/
#define	TPO_NOOP	1	/* no Operation				*/
#define	TPO_MSS		2	/* maximum Segment Size			*/


int init_tcp( void );

int tcp_open( Socket_s* psSocket );




#endif /* __F_NET_TCP_H__ */
