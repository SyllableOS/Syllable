#ifndef __F_TCP_H__
#define __F_TCP_H__

#include <net/tcp.h>

extern sem_id g_hConnListMutex;
extern thread_id g_hTCPTimerThread;

#define	TCPMAXURG  MAXNETBUF	/* maximum urgent data buffer size      */
#define	TCPUQLEN	5	/* TCP urgent queue lengths             */

#define	READERS		0x01
#define	WRITERS		0x02
#define TCP_LISTENERS	0x04

/* tcb_flags */

#define	TCBF_NEEDOUT	 0x0001	/* we need output                       */
#define TCBF_KEEPALIVE	 0x0002
#define	TCBF_FIRSTSEND	 0x0004	/* no data to ACK                       */
#define	TCBF_GOTFIN	 0x0008	/* no more to receive                   */
#define	TCBF_RDONE	 0x0010	/* no more receive data to process      */
#define	TCBF_SDONE	 0x0020	/* no more send data allowed            */
#define TCBF_FIN_RECVD	 0x0040
#define	TCBF_DELACK	 0x0080	/* do delayed ACK's                     */
#define	TCBF_BUFFER	 0x0100	/* do TCP buffering (default no)        */
#define	TCBF_PUSH	 0x0200	/* got a push; deliver what we have     */
#define	TCBF_SNDFIN	 0x0400	/* user process has closed, or shutdown the connection; send a FIN      */
#define TCBF_FINSENDT	 0x0800
#define TCBF_OPEN	 0x1000	/* Part of a open socket */
#define TCBF_ACTIVE	 0x2000	/* Part of the global list of connection */
#define TCBF_BUSY	 0x4000
#define TCBF_RWIN_CLOSED 0x8000	/* Set whenever the receiver window goes below MSS.
				 * When the window is opened again the flag is cleared
				 *  and a window anoncment is sent to peer
				 */

#define	UQTSIZE	(2*Ntcp)	/* (total) max # pending urgent segs    */
struct uqe
{
	int uq_state;		/* UQS_* above                          */
	tcpseq uq_seq;		/* start sequence of this buffer        */
	int uq_len;		/* length of this buffer                */
	char *uq_data;		/* data (0 if on urgent hole queue)     */
};

/* compute urgent data send and receive queue keys			*/
#define	SUDK(ptcb, seq)	(ptcb->tcb_sudseq - (seq))
#define	SUHK(ptcb, seq)	(ptcb->tcb_suhseq - (seq))
#define	RUDK(ptcb, seq)	(ptcb->tcb_rudseq - (seq))
#define	RUHK(ptcb, seq)	(ptcb->tcb_ruhseq - (seq))

typedef struct _TCPFragment TCPFragment_s;
struct _TCPFragment
{
	TCPFragment_s *tf_psNext;
	tcpseq tf_seq;
	int tf_len;
};

enum
{
	SEND,
	PERSIST,
	RETRANSMIT,
	DELETE,
	TCP_TE_KEEPALIVE,
	OE_COUNT
};

typedef struct _TCPEvent TCPEvent_s;
struct _TCPEvent
{
	TCPEvent_s *te_psNext;
	TCPEvent_s *te_psPrev;
	bigtime_t te_nExpireTime;
	bigtime_t te_nCreateTime;
	TCPCtrl_s *te_psTCPCtrl;
	int te_nEvent;
};


struct _TCPCtrl
{
	TCPCtrl_s *tcb_psNext;
	TCPCtrl_s *tcb_psNextHash;
	TCPCtrl_s *tcb_psNextClient;
	sem_id tcb_hMutex;	/* tcb mutual exclusion                 */
	atomic_t tcb_nRefCount;
	int tcb_nState;		/* TCP state                            */
	int tcb_ostate;		/* output state                         */
	int16 tcb_code;		/* TCP code for next packet             */
	atomic_t tcb_flags;	/* various TCB state flags              */
	int tcb_nError;		/* return error for user side           */

	ipaddr_t tcb_rip;	/* remote IP address                    */
	ipaddr_t tcb_lip;	/* local IP address                     */
	uint16 tcb_rport;	/* remote TCP port                      */
	uint16 tcb_lport;	/* local TCP port                       */

	tcpseq tcb_nSndFirstUnacked;	/* sendt but not acked by receiver */
	tcpseq tcb_nSndNext;	/* sequence of next byte to be sendt    */
	tcpseq tcb_slast;	/* sequence of FIN, if TCBF_SNDFIN      */
	int tcb_swindow;	/* send window size (octets)            */
	tcpseq tcb_lwseq;	/* sequence of last window update       */
	tcpseq tcb_lwack;	/* ack seq of last window update        */
	int tcb_cwnd;		/* congestion window size (octets)      */
	int tcb_ssthresh;	/* slow start threshold (octets)        */
	int tcb_smss;		/* send max segment size (octets)       */
	tcpseq tcb_iss;		/* initial send sequence                */

	bigtime_t tcb_srt;	/* smoothed Round Trip Time                             */
	bigtime_t tcb_rtde;	/* Round Trip deviation estimator                       */
	bigtime_t tcb_persist;	/* persist timeout value                                */
	bigtime_t tcb_keep;	/* keepalive timeout value                              */
	bigtime_t tcb_rexmt;	/* Retransmit timeout value                             */
	bigtime_t tcb_nLastReceiveTime;	/* Timestamp for last packet ariving on this socket     */
	bigtime_t tcb_nRTTStart;
	int tcb_rexmtcount;	/* number of rexmts sent                                */

	tcpseq tcb_nRcvNext;	/* next byte we expect to receive       */

	tcpseq tcb_rudseq;	/* base sequence for rudq entries       */
//  int         tcb_rudq;       /* receive urgent data queue            */
	tcpseq tcb_ruhseq;	/* base sequence for ruhq entries       */
//  int         tcb_ruhq;       /* receive urgent hole queue            */
//  int         tcb_sudseq;     /* base sequence for sudq entries       */
//  int         tcb_sudq;       /* send urgent data queue               */
//  int         tcb_suhseq;     /* base sequence for suhq entries       */
//  int         tcb_suhq;       /* send urgent hole queue               */

	sem_id tcb_ocsem;	/* open/close semaphore                 */
	sem_id tcb_hListenSem;

	int tcb_nListenQueueSize;
	int tcb_nNumClients;
	TCPCtrl_s *tcb_psFirstClient;	/* Incoming connections from listen */
	TCPCtrl_s *tcb_psParent;	/* pointer to parent TCB (for ACCEPT)   */

	SelectRequest_s *tcb_psFirstReadSelReq;
	SelectRequest_s *tcb_psFirstWriteSelReq;
	sem_id tcb_hRecvQueue;
	sem_id tcb_hSendQueue;

	char *tcb_sndbuf;	/* send buffer                          */
	int tcb_sbstart;	/* start of valid data                  */
	int tcb_nSndCount;	/* data character count                 */
	int tcb_nSndBufSize;	/* send buffer size (bytes)             */

	char *tcb_rcvbuf;	/* receive buffer (circular)            */
	int tcb_nRcvBufStart;	/* start of valid data                  */
	int tcb_nRcvCount;	/* number of succesively received bytes */
	int tcb_rbsize;		/* receive buffer size (bytes)          */
	int tcb_rmss;		/* receive max segment size             */
	tcpseq tcb_nAdvertizedRcvWin;	/* seq of currently advertised window   */
	TCPFragment_s *tcb_psFirstFragment;	/* segment fragment queue               */
	tcpseq tcb_finseq;	/* FIN sequence number, or 0            */
	tcpseq tcb_pushseq;	/* PUSH sequence number, or 0           */
	bool tcb_bNonBlock;
	bool tcb_bSendKeepalive;
	TCPEvent_s tcp_asEvents[OE_COUNT];
};




/* TCP states */

#define TCPS_FREE		0
#define TCPS_CLOSED		1
#define	TCPS_LISTEN		2
#define	TCPS_SYNSENT		3
#define	TCPS_SYNRCVD		4
#define	TCPS_ESTABLISHED	5
#define	TCPS_FINWAIT1		6
#define	TCPS_FINWAIT2		7
#define	TCPS_CLOSEWAIT		8
#define	TCPS_LASTACK		9
#define	TCPS_CLOSING		10
#define	TCPS_TIMEWAIT		11

/* Output States */

enum
{
	TCPO_IDLE,
	TCPO_PERSIST,
	TCPO_XMIT,
	TCPO_REXMT,
	TCPO_KEEPALIVE
};



#define TCP_RXMIT_TIMEOUT (60LL*9LL*1000000LL)	/* Maximum retry time before aborting   */
//#define TCP_TWOMSL      (120LL * 1000000LL)   /* 2 minutes (2 * Max Segment Lifetime) */
#define TCP_TWOMSL        (20LL * 1000000LL)	/* 2 minutes (2 * Max Segment Lifetime) */
#define TCP_MAXRXT        (20LL * 1000000LL)	/* 20 seconds max rexmt time        */
#define TCP_MINRXT        (2000LL)	/* 2mS min rexmt time        */
#define TCP_ACKDELAY      (1LL * 1000000LL)	/* 1 sec ACK delay, if TCBF_DELACK    */
#define TCP_MAXPRS        (60LL * 1000000LL)	/* 1 minute max persist time        */

/*
#define TCP_KEEPALIVE_TIME   (10LL*1000000LL)
#define TCP_KEEPALIVE_FREQ   (5LL*1000000LL)
#define TCP_KEEPALIVE_PROBES 10
*/
#define TCP_KEEPALIVE_TIME   (120LL*60LL*1000000LL)
#define TCP_KEEPALIVE_FREQ   (75LL*1000000LL)
#define TCP_KEEPALIVE_PROBES 10

#define	TWF_NORMAL	0	/* normal data write                    */
#define	TWF_URGENT	1	/* urgent data write                    */

//#define TCP_NOTIFY_STATE_CHANGE  

#ifdef TCP_NOTIFY_STATE_CHANGE
#define tcp_set_state( psTCPCtrl, nState ) do {						\
  printk( __FUNCTION__ "() TCP state changed to: %s", tcp_state_str( nState ) );	\
  (psTCPCtrl)->tcb_nState = (nState);							\
} while(0)
#else
#define tcp_set_state( psTCPCtrl, nState ) do { (psTCPCtrl)->tcb_nState = (nState); } while(0)
#endif

void tcp_set_output_state( TCPCtrl_s *psTCPCtrl, int nState );


#define tcp_reset(p) __tcp_reset(p, __FUNCTION__, __LINE__ )
#define tcp_abort( tcb, error ) __tcp_abort( tcb, error, __FUNCTION__, __LINE__ )

int init_tcp_out( void );



void tcp_dump_packet( PacketBuf_s *psPkt );
const char *tcp_state_str( int nState );

int tcp_create_event( TCPCtrl_s *psTCPCtrl, int nEvent, bigtime_t nDelay, bool bReplace );
bigtime_t tcp_delete_event( TCPCtrl_s *psTCPCtrl, int nEvent );
bigtime_t tcp_event_time_left( TCPCtrl_s *psTCPCtrl, int nEvent );
TCPCtrl_s *tcp_get_event( int *pnEvent );
int tcp_rwindow( TCPCtrl_s *psTCPCtrl, bool bSetAdvertizedSeq );
int tcp_wait( TCPCtrl_s *psTCPCtrl );
uint16 tcp_cksum( PacketBuf_s *psPkt );
int tcp_kick( TCPCtrl_s *psTCPCtrl );
int tcp_wakeup( int type, TCPCtrl_s *psTCPCtrl );
bool __tcp_abort( TCPCtrl_s *psTCPCtrl, int error, const char *pzCallerFunc, int nLineNum );
int tcp_sync( TCPCtrl_s *ptcb );
TCPCtrl_s *tcb_alloc( void );
int tcb_dealloc( TCPCtrl_s *psTCPCtrl, bool bForceUnlink );
void tcp_add_to_hash( TCPCtrl_s *psTCPCtrl );
TCPCtrl_s *tcp_lookup( uint32 nLocalAddr, uint16 nLocalPort, uint32 nRemoteAddr, uint16 nRemotePort );

#endif // __F_TCP_H__
