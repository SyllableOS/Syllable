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
#include <atheos/time.h>
#include <atheos/socket.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include <net/net.h>
#include <net/in.h>
#include <net/ip.h>
#include <net/if_ether.h>
#include <net/icmp.h>
#include "tcpdefs.h"

thread_id g_hTCPTimerThread = -1;

enum
{ TSF_NEWDATA, TSF_REXMT, TSF_KEEPALIVE };

/*****************************************************************************
 * NAME:
 * DESC:
 *	Skip urgent data holes in send buffer
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

#ifdef _DONE_
int tcp_shskip( ptcb, datalen, poff )
    struct tcb *ptcb;
    int datalen;
    int *poff;
{
	struct uqe *puqe;
	tcpseq lseq;
	int resid = 0;

	if ( ptcb->tcb_suhq == EMPTY )
		return datalen;
	puqe = ( struct uqe * )deq( ptcb->tcb_suhq );
	while ( puqe )
	{
		lseq = ptcb->tcb_nSndNext + datalen - 1;
		if ( SEQCMP( lseq, puqe->uq_seq ) > 0 )
			resid = lseq - puqe->uq_seq;
		/* chop off at urgent boundary, but save the hole...      */
		if ( resid < datalen )
		{
			datalen -= resid;
			if ( enq( ptcb->tcb_suhq, puqe, SUHK( ptcb, puqe->uq_seq ) ) < 0 )
				uqfree( puqe );	/* shouldn't happen */
			return datalen;
		}
		/*
		 * else, we're skipping a hole now adjust the beginning
		 * of the segment.
		 */
		lseq = puqe->uq_seq + puqe->uq_len;
		resid = lseq - ptcb->tcb_nSndNext;
		if ( resid > 0 )
		{
			/* overlap... */
			*poff += resid;
			if ( *poff > ptcb->tcb_nSndBufSize )
				*poff -= ptcb->tcb_nSndBufSize;
			datalen -= resid;
			if ( datalen < 0 )
				datalen = 0;
		}
		uqfree( puqe );
		puqe = ( struct uqe * )deq( ptcb->tcb_suhq );
	}
	/* puqe == 0 (No more holes left) */
	freeq( ptcb->tcb_suhq );
	ptcb->tcb_suhq = EMPTY;
	return datalen;
}
#endif

void tcp_set_output_state( TCPCtrl_s *psTCPCtrl, int nState )
{

/*    if ( psTCPCtrl->tcb_ostate == TCPO_REXMT && nState != TCPO_REXMT ) {
	printk( "%p leave retransmitt\n", psTCPCtrl );
    }
    if ( psTCPCtrl->tcb_ostate != TCPO_REXMT && nState == TCPO_REXMT ) {
	printk( "%p enter retransmitt\n", psTCPCtrl );
    }*/
	psTCPCtrl->tcb_ostate = nState;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_send_window_size( TCPCtrl_s *psTCPCtrl )
{
	int nWndEnd = psTCPCtrl->tcb_lwack + min( psTCPCtrl->tcb_swindow, psTCPCtrl->tcb_cwnd );
	int nSize = nWndEnd - psTCPCtrl->tcb_nSndNext;

	if ( nSize < psTCPCtrl->tcb_smss && nSize > 0 )
	{
		nWndEnd = psTCPCtrl->tcb_lwack + psTCPCtrl->tcb_swindow;
		nSize = nWndEnd - psTCPCtrl->tcb_nSndNext;
	}
	return ( nSize );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Compute the packet length and the offset into the send buffer
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_sndlen( TCPCtrl_s *psTCPCtrl, int rexmt, int *pnOffset )
{
//  struct      uqe     *puqe, *puqe2;
	int datalen;

	if ( rexmt == TSF_KEEPALIVE )
	{
		*pnOffset = 0;
		return ( 0 );
	}
//  if (rexmt || psTCPCtrl->tcb_sudq == EMPTY)
	{
		if ( rexmt == TSF_REXMT || ( psTCPCtrl->tcb_code & TCPF_SYN ) )
			*pnOffset = 0;
		else
			*pnOffset = psTCPCtrl->tcb_nSndNext - psTCPCtrl->tcb_nSndFirstUnacked;
		datalen = psTCPCtrl->tcb_nSndCount - *pnOffset;
		// remove urgent data holes
		if ( rexmt == TSF_NEWDATA )
		{
#ifdef _DONE_
			datalen = tcp_shskip( psTCPCtrl, datalen, pnOffset );
//#else
//      datalen = 65536;
#endif
			datalen = min( datalen, tcp_send_window_size( psTCPCtrl ) );
		}
		return min( datalen, psTCPCtrl->tcb_smss );
	}
	printk( "Warning: tcp_sndlen() urgent data not implemented\n" );
	return ( 0 );
#ifdef _DONE_
	/* else, URGENT data */

	puqe = ( struct uqe * )deq( psTCPCtrl->tcb_sudq );
	*pnOffset = psTCPCtrl->tcb_sbstart + puqe->uq_seq - psTCPCtrl->tcb_nSndFirstUnacked;
	if ( *pnOffset > psTCPCtrl->tcb_nSndBufSize )
		*pnOffset -= psTCPCtrl->tcb_nSndBufSize;
	datalen = puqe->uq_len;
	if ( datalen > psTCPCtrl->tcb_smss )
	{
		datalen = psTCPCtrl->tcb_smss;
		puqe2 = uqalloc();
		if ( puqe2 == SYSERR )
		{
			uqfree( puqe );	/* bail out and         */
			return psTCPCtrl->tcb_smss;	/* try as normal data   */
		}
		puqe2->uq_seq = puqe->uq_seq;
		puqe2->uq_len = datalen;

		/* put back what we can't use */
		puqe->uq_seq += datalen;
		puqe->uq_len -= datalen;
	}
	else
	{
		puqe2 = puqe;
		puqe = ( struct uqe * )deq( psTCPCtrl->tcb_sudq );
	}
	if ( puqe == 0 )
	{
		freeq( psTCPCtrl->tcb_sudq );
		psTCPCtrl->tcb_sudq = -1;
	}
	else if ( enq( psTCPCtrl->tcb_sudq, puqe, SUDK( psTCPCtrl, puqe->uq_seq ) ) < 0 )
		uqfree( puqe );	/* shouldn't happen */
	if ( psTCPCtrl->tcb_suhq == EMPTY )
	{
		psTCPCtrl->tcb_suhq = newq( TCPUQLEN, QF_WAIT );
		psTCPCtrl->tcb_suhseq = puqe2->uq_seq;
	}
	if ( enq( psTCPCtrl->tcb_suhq, puqe2, SUHK( psTCPCtrl, puqe2->uq_seq ) ) < 0 )
		uqfree( puqe2 );
	return datalen;
#endif
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Set receive MSS option
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_rmss( TCPCtrl_s *psTCPCtrl, PacketBuf_s *psPkt )
{
	TCPHeader_s *psTcpHdr = psPkt->pb_uTransportHdr.psTCP;
	IpHeader_s *psIpHdr = psPkt->pb_uNetworkHdr.psIP;
	int mss, hlen, olen, i;
	char *pIpData;

	hlen = TCP_HLEN( psTcpHdr );
	olen = 2 + sizeof( short );

	pIpData = ( char * )( psIpHdr + 1 );

	pIpData[hlen] = TPO_MSS;	/* option kind          */
	pIpData[hlen + 1] = olen;	/* option length        */

//  mss = htons((short)psTCPCtrl->tcb_smss);
	mss = psTCPCtrl->tcb_smss;
	for ( i = olen - 1; i > 1; i-- )
	{
		pIpData[hlen + i] = mss & 0xff;
		mss >>= 8;
	}
	hlen += olen + 3;	/* +3 for proper rounding below */
	/* header length is high 4 bits of tcp_offset, in longs     */
	psTcpHdr->tcp_offset = ( ( ( hlen << 2 ) & 0xf0 ) ) | ( psTcpHdr->tcp_offset & 0xf );

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Build and send a TCP segment for the given TCB
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_send( TCPCtrl_s *psTCPCtrl, int nMode )
{
	PacketBuf_s *psPkt;
	IpHeader_s *psIpHdr;
	TCPHeader_s *psTCPHdr;
	int nDataLen;
	int off;
	int newdata;
	int nError;

	nDataLen = tcp_sndlen( psTCPCtrl, nMode, &off );

	if ( nDataLen < 0 )
	{
		nDataLen = 0;
	}
	psPkt = alloc_pkt_buffer( nDataLen + 16 + sizeof( IpHeader_s ) + 40 );	// data + headers + upto 20 bytes of options

	if ( psPkt == NULL )
	{
		return ( -ENOMEM );
	}
	psPkt->pb_uNetworkHdr.pRaw = psPkt->pb_pData + 16;
	psPkt->pb_uTransportHdr.pRaw = psPkt->pb_uNetworkHdr.pRaw + sizeof( IpHeader_s );

	psTCPHdr = psPkt->pb_uTransportHdr.psTCP;
	psIpHdr = psPkt->pb_uNetworkHdr.psIP;

	IP_COPYADDR( psIpHdr->iph_nSrcAddr, psTCPCtrl->tcb_lip );
	IP_COPYADDR( psIpHdr->iph_nDstAddr, psTCPCtrl->tcb_rip );

	psIpHdr->iph_nProtocol = IPT_TCP;

	psTCPHdr->tcp_sport = htons( psTCPCtrl->tcb_lport );
	psTCPHdr->tcp_dport = htons( psTCPCtrl->tcb_rport );

	if ( nMode == TSF_NEWDATA )
	{
		if ( psTCPCtrl->tcb_code & TCPF_URG )
		{
			psTCPHdr->tcp_seq = htonl( psTCPCtrl->tcb_nSndFirstUnacked + off );
		}
		else
		{
			psTCPHdr->tcp_seq = htonl( psTCPCtrl->tcb_nSndNext );
		}
	}
	else if ( nMode == TSF_REXMT )
	{
		psTCPHdr->tcp_seq = htonl( psTCPCtrl->tcb_nSndFirstUnacked );
	}
	else
	{			// Keepalive
		psTCPHdr->tcp_seq = htonl( psTCPCtrl->tcb_nSndFirstUnacked - 1 );
	}

	psTCPHdr->tcp_ack = htonl( psTCPCtrl->tcb_nRcvNext );

	psTCPHdr->tcp_code = psTCPCtrl->tcb_code;
	if ( ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_SNDFIN ) && SEQCMP( ntohl( psTCPHdr->tcp_seq ) + nDataLen, psTCPCtrl->tcb_slast ) == 0 )
	{
		psTCPHdr->tcp_code |= TCPF_FIN;
		atomic_or( &psTCPCtrl->tcb_flags, TCBF_FINSENDT );
	}

	psTCPHdr->tcp_offset = TCPHOFFSET;
	if ( ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_FIRSTSEND ) == 0 )
	{
		psTCPHdr->tcp_code |= TCPF_ACK;
	}
	if ( psTCPHdr->tcp_code & TCPF_SYN )
	{
		tcp_rmss( psTCPCtrl, psPkt );
	}
	psIpHdr->iph_nHdrSize = 5;
	psIpHdr->iph_nVersion = 4;

	psPkt->pb_nSize = IP_GET_HDR_LEN( psIpHdr ) + TCP_HLEN( psTCPHdr ) + nDataLen;
	psIpHdr->iph_nPacketSize = htons( psPkt->pb_nSize );

	if ( nDataLen > 0 )
	{
		psTCPHdr->tcp_code |= TCPF_PSH;
	}
	psTCPHdr->tcp_window = htons( tcp_rwindow( psTCPCtrl, true ) );
	if ( psTCPCtrl->tcb_code & TCPF_URG )
#ifdef	BSDURG
		psTCPHdr->tcp_urgptr = htons( nDataLen );	/* 1 past end           */
#else // BSDURG
		psTCPHdr->tcp_urgptr = htons( nDataLen - 1 );
#endif // BSDURG
	else
		psTCPHdr->tcp_urgptr = 0;



	if ( psTCPCtrl->tcb_sndbuf != NULL )
	{			// Should not be needed
		char *pch;
		int i = ( psTCPCtrl->tcb_sbstart + off ) % psTCPCtrl->tcb_nSndBufSize;

		pch = ( ( char * )( psIpHdr + 1 ) ) + TCP_HLEN( psTCPHdr );

		if ( i + nDataLen > psTCPCtrl->tcb_nSndBufSize )
		{
			int nFirstSeg = psTCPCtrl->tcb_nSndBufSize - i;

			memcpy( pch, psTCPCtrl->tcb_sndbuf + i, nFirstSeg );
			memcpy( pch + nFirstSeg, psTCPCtrl->tcb_sndbuf, nDataLen - nFirstSeg );
		}
		else
		{
			memcpy( pch, psTCPCtrl->tcb_sndbuf + i, nDataLen );
		}
	}
	atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_NEEDOUT );
	if ( nMode == TSF_REXMT )
	{
		newdata = psTCPCtrl->tcb_nSndFirstUnacked + nDataLen - psTCPCtrl->tcb_nSndNext;
		if ( newdata < 0 )
		{
			newdata = 0;
		}
//    TcpRetransSegs++;
	}
	else
	{
		newdata = nDataLen;
		if ( psTCPHdr->tcp_code & TCPF_SYN )
			newdata++;	/* SYN is part of the sequence   */
		if ( psTCPHdr->tcp_code & TCPF_FIN )
			newdata++;	/* FIN is part of the sequence   */
	}
	psTCPCtrl->tcb_nSndNext += newdata;
	if ( newdata >= 0 )
	{
		psTCPCtrl->tcb_nRTTStart = get_system_time();
//    TcpOutSegs++;
	}
	if ( psTCPCtrl->tcb_nState == TCPS_TIMEWAIT )
	{
		tcp_wait( psTCPCtrl );
	}
	psTCPHdr->tcp_cksum = 0;
	psTCPHdr->tcp_cksum = tcp_cksum( psPkt );

	nError = ip_send( psPkt );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Compute how much data is available to send.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_howmuch( TCPCtrl_s *psTCPCtrl )
{
	int tosend;

	tosend = psTCPCtrl->tcb_nSndFirstUnacked + psTCPCtrl->tcb_nSndCount - psTCPCtrl->tcb_nSndNext;
	if ( psTCPCtrl->tcb_code & TCPF_SYN )
		++tosend;
	if ( atomic_read( &psTCPCtrl->tcb_flags ) & TCBF_SNDFIN )
		++tosend;
	return tosend;
}


/*****************************************************************************
 * NAME:
 * DESC:
 *	Handle TCP output events while we are retransmitting
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void tcp_rexmt( TCPCtrl_s *psTCPCtrl, int event )
{
	bigtime_t nNextDelay = psTCPCtrl->tcb_rexmt << ( psTCPCtrl->tcb_rexmtcount + 1 );

	if ( nNextDelay > TCP_MAXRXT )
	{
		nNextDelay = TCP_MAXRXT;
	}
	tcp_send( psTCPCtrl, TSF_REXMT );
	tcp_create_event( psTCPCtrl, RETRANSMIT, nNextDelay, true );
//    printk( "tcp_rexmt() Retransmit in %dmS (%dmS) (%d)\n",
//          (int)(nNextDelay / 1000), (int)(psTCPCtrl->tcb_rexmt/1000), psTCPCtrl->tcb_rexmtcount );

	if ( psTCPCtrl->tcb_ostate != TCPO_REXMT )
	{
		psTCPCtrl->tcb_ssthresh = psTCPCtrl->tcb_cwnd;	// first drop
	}
	psTCPCtrl->tcb_ssthresh = min( psTCPCtrl->tcb_swindow, psTCPCtrl->tcb_ssthresh ) / 2;
	if ( psTCPCtrl->tcb_ssthresh < psTCPCtrl->tcb_smss )
	{
		psTCPCtrl->tcb_ssthresh = psTCPCtrl->tcb_smss;
	}
	psTCPCtrl->tcb_cwnd >>= 1;

	if ( psTCPCtrl->tcb_cwnd < psTCPCtrl->tcb_smss )
	{
		psTCPCtrl->tcb_cwnd = psTCPCtrl->tcb_smss;
	}
//    printk( "Retransmitt cwnd=%d (%d)\n", psTCPCtrl->tcb_cwnd, psTCPCtrl->tcb_rexmtcount );

/*    while ( tcp_howmuch(psTCPCtrl) > 0 && tcp_send_window_size(psTCPCtrl) > 0 ) {
	if ( tcp_send( psTCPCtrl, TSF_NEWDATA) < 0 ) {
	    break;
	}
    }*/

}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Handle TCP output events while we are transmitting
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void tcp_xmit( TCPCtrl_s *psTCPCtrl, int event )
{
	if ( event == RETRANSMIT )
	{
		tcp_delete_event( psTCPCtrl, SEND );
		tcp_rexmt( psTCPCtrl, event );
		tcp_set_output_state( psTCPCtrl, TCPO_REXMT );
		return;
	}

	if ( tcp_howmuch( psTCPCtrl ) == 0 )
	{
		if ( atomic_read( &psTCPCtrl->tcb_flags ) & ( TCBF_NEEDOUT | TCBF_KEEPALIVE ) )
		{
			tcp_send( psTCPCtrl, TSF_NEWDATA );	/* just an ACK */
		}
		if ( psTCPCtrl->tcb_nSndCount > 0 )
		{
			tcp_create_event( psTCPCtrl, RETRANSMIT, psTCPCtrl->tcb_rexmt, true );
		}
		return;
	}
	else if ( psTCPCtrl->tcb_swindow == 0 )
	{
		tcp_set_output_state( psTCPCtrl, TCPO_PERSIST );
		psTCPCtrl->tcb_persist = psTCPCtrl->tcb_rexmt;
		tcp_send( psTCPCtrl, TSF_NEWDATA );
//    printk( "tcp_xmit() enter PERSIST state (delay = %d)\n", (int) psTCPCtrl->tcb_persist );
		if ( psTCPCtrl->tcb_persist < TCP_MINRXT )
		{
			psTCPCtrl->tcb_persist = TCP_MINRXT;
		}
		tcp_create_event( psTCPCtrl, PERSIST, psTCPCtrl->tcb_persist, true );
		return;
	}
	tcp_set_output_state( psTCPCtrl, TCPO_XMIT );

	while ( tcp_howmuch( psTCPCtrl ) > 0 && tcp_send_window_size( psTCPCtrl ) > 0 )
	{
		if ( tcp_send( psTCPCtrl, TSF_NEWDATA ) < 0 )
		{
			break;
		}
	}
	tcp_create_event( psTCPCtrl, RETRANSMIT, psTCPCtrl->tcb_rexmt, false );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int tcp_out( void *pData )
{
	for ( ;; )
	{
		TCPCtrl_s *psTCPCtrl;
		int nEvent;

		psTCPCtrl = tcp_get_event( &nEvent );
		if ( psTCPCtrl == NULL )
		{
			printk( "tcp_out() got NULL event!!\n" );
			continue;
		}
		if ( psTCPCtrl->tcb_nState <= TCPS_CLOSED )
		{
			printk( "Error: tcp_out() got tcp with TCPS_CLOSED state!\n" );
			atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_BUSY );
			UNLOCK( psTCPCtrl->tcb_hMutex );
			continue;
		}
		if ( nEvent == DELETE )
		{
			tcp_abort( psTCPCtrl, 0L );
			continue;
		}
		else
		{
			switch ( psTCPCtrl->tcb_ostate )
			{
			case TCPO_IDLE:
				if ( nEvent == SEND )
				{
					tcp_xmit( psTCPCtrl, nEvent );
				}
				else if ( nEvent == TCP_TE_KEEPALIVE && psTCPCtrl->tcb_nState == TCPS_ESTABLISHED && psTCPCtrl->tcb_bSendKeepalive )
				{
					printk( "Send first keepalive to %d.%d.%d.%d:%d\n", psTCPCtrl->tcb_rip[0], psTCPCtrl->tcb_rip[1], psTCPCtrl->tcb_rip[2], psTCPCtrl->tcb_rip[3], psTCPCtrl->tcb_rport );
					atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
					tcp_send( psTCPCtrl, TSF_KEEPALIVE );
					tcp_set_output_state( psTCPCtrl, TCPO_KEEPALIVE );
					tcp_create_event( psTCPCtrl, TCP_TE_KEEPALIVE, TCP_KEEPALIVE_FREQ, true );
				}
				break;
			case TCPO_KEEPALIVE:
				if ( nEvent == SEND )
				{
					psTCPCtrl->tcb_rexmtcount = 0;
					tcp_set_output_state( psTCPCtrl, TCPO_IDLE );
					tcp_create_event( psTCPCtrl, TCP_TE_KEEPALIVE, TCP_KEEPALIVE_FREQ, true );
					tcp_xmit( psTCPCtrl, nEvent );
				}
				else if ( nEvent == TCP_TE_KEEPALIVE && psTCPCtrl->tcb_nState == TCPS_ESTABLISHED && psTCPCtrl->tcb_bSendKeepalive )
				{
					printk( "Send keepalive %d to %d.%d.%d.%d:%d\n", psTCPCtrl->tcb_rexmtcount, psTCPCtrl->tcb_rip[0], psTCPCtrl->tcb_rip[1], psTCPCtrl->tcb_rip[2], psTCPCtrl->tcb_rip[3], psTCPCtrl->tcb_rport );
					if ( ++psTCPCtrl->tcb_rexmtcount >= TCP_KEEPALIVE_PROBES )
					{
						printk( "Too many unacked keepalive packets. Aborting connection to %d.%d.%d.%d:%d\n", psTCPCtrl->tcb_rip[0], psTCPCtrl->tcb_rip[1], psTCPCtrl->tcb_rip[2], psTCPCtrl->tcb_rip[3], psTCPCtrl->tcb_rport );
						tcp_abort( psTCPCtrl, EPIPE );
						continue;
					}
					atomic_or( &psTCPCtrl->tcb_flags, TCBF_NEEDOUT );
					tcp_send( psTCPCtrl, TSF_KEEPALIVE );
					tcp_create_event( psTCPCtrl, TCP_TE_KEEPALIVE, TCP_KEEPALIVE_FREQ, true );
				}
				break;
			case TCPO_PERSIST:
				if ( nEvent != PERSIST )
				{
					break;	// Ignore everything else
				}
				psTCPCtrl->tcb_rexmtcount++;
				if ( psTCPCtrl->tcb_nLastReceiveTime + TCP_RXMIT_TIMEOUT * 2LL < get_system_time() )
				{
					tcp_abort( psTCPCtrl, EPIPE );	// FIXME: Don't beleve that's the right error code
					continue;
				}

				tcp_send( psTCPCtrl, TSF_REXMT );
				psTCPCtrl->tcb_persist = min( psTCPCtrl->tcb_persist << 1, TCP_MAXPRS );
				tcp_create_event( psTCPCtrl, PERSIST, psTCPCtrl->tcb_persist, true );
				break;
			case TCPO_XMIT:
				tcp_xmit( psTCPCtrl, nEvent );
				break;
			case TCPO_REXMT:
//        printk( "Got retransmitt event (cnt=%d)\n", psTCPCtrl->tcb_rexmtcount );

				if ( nEvent != RETRANSMIT )
				{
					break;	// Ignore everything else
				}
				psTCPCtrl->tcb_rexmtcount++;
				if ( psTCPCtrl->tcb_nLastReceiveTime + TCP_RXMIT_TIMEOUT < get_system_time() )
				{
					tcp_abort( psTCPCtrl, EPIPE );	// FIXME: Don't beleve that's the right error code
					continue;
				}
				else
				{
					tcp_rexmt( psTCPCtrl, nEvent );
				}

/*		    if (++psTCPCtrl->tcb_rexmtcount > TCP_MAXRETRIES) {
		    tcp_abort(psTCPCtrl, EPIPE ); // FIXME: Don't beleve that's the right error code
		    continue;
		    } else {
		    tcp_rexmt( psTCPCtrl, nEvent );
		    }*/
				break;
			default:
				printk( "tcp_out() invalid state %d\n", psTCPCtrl->tcb_ostate );
				break;

			}
		}
		atomic_and( &psTCPCtrl->tcb_flags, ~TCBF_BUSY );
		UNLOCK( psTCPCtrl->tcb_hMutex );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int init_tcp_out( void )
{
	g_hTCPTimerThread = spawn_kernel_thread( "tcp_out", tcp_out, 1, 0, NULL );
	wakeup_thread( g_hTCPTimerThread, false );
	return ( 0 );
}
