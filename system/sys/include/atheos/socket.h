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

#ifndef __F_ATHEOS_SOCKET_H__
#define __F_ATHEOS_SOCKET_H__

#ifdef __KERNEL__
#include <atheos/types.h>
#include <atheos/filesystem.h>
#include <net/nettypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}				/*make emacs indention work */
#endif
/*
#define INADDR_ANY	 0x00000000	
#define INADDR_BROADCAST 0xffffffff
#define INADDR_LOOPBACK	 0x7f000001
*/
#ifdef __KERNEL__
#ifndef sa_family_t
typedef unsigned short sa_family_t;
#define sa_family_t sa_family_t
#endif
#endif

/*
 *	1003.1g requires sa_family_t and that sa_data is char.
 */
#ifdef __KERNEL__
struct sockaddr {
    sa_family_t sa_family;	/* address family, AF_xxx       */
    char sa_data[14];		/* 14 bytes of protocol address */
};


struct linger {
    int l_onoff;		/* Linger active                */
    int l_linger;		/* How long to linger for       */
};

/*
 *	As we do 4.4BSD message passing we use a 4.4BSD message passing
 *	system, not 4.3. Thus msg_accrights(len) are now missing. They
 *	belong in an obscure libc emulation or the bin.
 */
struct msghdr {
    void *msg_name;		/* Socket name                  */
    int msg_namelen;		/* Length of name               */
    struct iovec *msg_iov;	/* Data blocks                  */
    size_t msg_iovlen;		/* Number of blocks             */
    void *msg_control;		/* Per protocol magic (eg BSD file descriptor passing) */
    size_t msg_controllen;	/* Length of cmsg list */
    unsigned msg_flags;
};


#define __CMSG_NXTHDR(ctl, len, cmsg) __cmsg_nxthdr((ctl),(len),(cmsg))
#define CMSG_NXTHDR(mhdr, cmsg) cmsg_nxthdr((mhdr), (cmsg))

#define CMSG_ALIGN(len) ( ((len)+sizeof(long)-1) & ~(sizeof(long)-1) )

#define CMSG_DATA(cmsg)	((void *)((char *)(cmsg) + CMSG_ALIGN(sizeof(struct cmsghdr))))
#define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))

#define __CMSG_FIRSTHDR(ctl,len) ((len) >= sizeof(struct cmsghdr) ? \
				  (struct cmsghdr *)(ctl) : \
				  (struct cmsghdr *)NULL)
#define CMSG_FIRSTHDR(msg)	__CMSG_FIRSTHDR((msg)->msg_control, (msg)->msg_controllen)



/* Sockets 0-1023 can't be bound to unless you are superuser */
#define PROT_SOCK	1024

#define SHUTDOWN_MASK	3
#define RCV_SHUTDOWN	1
#define SEND_SHUTDOWN	2


/* Socket types. */
#define SOCK_STREAM	1	/* stream (connection) socket   */
#define SOCK_DGRAM	2	/* datagram (conn.less) socket  */
#define SOCK_RAW	3	/* raw socket                   */
#define SOCK_RDM	4	/* reliably-delivered message   */
#define SOCK_SEQPACKET	5	/* sequential packet socket     */
#define SOCK_PACKET	10	/* linux specific way of        */
					/* getting packets at the dev   */
					/* level.  For writing rarp and */
					/* other similar things on the  */
					/* user level.                  */
#endif				/* __KERNEL__ */





/* Protocol families.  */
#define	PF_UNSPEC	0	/* Unspecified.  */
#define	PF_LOCAL	1	/* Local to host (pipes and file-domain).  */
#define	PF_UNIX		PF_LOCAL	/* Old BSD name for PF_LOCAL.  */
#define	PF_FILE		PF_LOCAL	/* Another non-standard name for PF_LOCAL.  */
#define	PF_INET		2	/* IP protocol family.  */
#define	PF_AX25		3	/* Amateur Radio AX.25.  */
#define	PF_IPX		4	/* Novell Internet Protocol.  */
#define	PF_APPLETALK	5	/* Appletalk DDP.  */
#define	PF_NETROM	6	/* Amateur radio NetROM.  */
#define	PF_BRIDGE	7	/* Multiprotocol bridge.  */
#define	PF_ATMPVC	8	/* ATM PVCs.  */
#define	PF_X25		9	/* Reserved for X.25 project.  */
#define	PF_INET6	10	/* IP version 6.  */
#define	PF_ROSE		11	/* Amateur Radio X.25 PLP.  */
#define	PF_DECnet	12	/* Reserved for DECnet project.  */
#define	PF_NETBEUI	13	/* Reserved for 802.2LLC project.  */
#define	PF_SECURITY	14	/* Security callback pseudo AF.  */
#define	PF_KEY		15	/* PF_KEY key management API.  */
#define	PF_NETLINK	16
#define	PF_ROUTE	PF_NETLINK	/* Alias to emulate 4.4BSD.  */
#define	PF_PACKET	17	/* Packet family.  */
#define	PF_ASH		18	/* Ash.  */
#define	PF_ECONET	19	/* Acorn Econet.  */
#define	PF_ATMSVC	20	/* ATM SVCs.  */
#define	PF_SNA		22	/* Linux SNA Project */
#define PF_IRDA		23	/* IRDA sockets.  */
#define	PF_MAX		32	/* For now..  */


/* Address families.  */
#define	AF_UNSPEC	PF_UNSPEC
#define	AF_LOCAL	PF_LOCAL
#define	AF_UNIX		PF_UNIX
#define	AF_FILE		PF_FILE
#define	AF_INET		PF_INET
#define	AF_AX25		PF_AX25
#define	AF_IPX		PF_IPX
#define	AF_APPLETALK	PF_APPLETALK
#define	AF_NETROM	PF_NETROM
#define	AF_BRIDGE	PF_BRIDGE
#define	AF_ATMPVC	PF_ATMPVC
#define	AF_X25		PF_X25
#define	AF_INET6	PF_INET6
#define	AF_ROSE		PF_ROSE
#define	AF_DECnet	PF_DECnet
#define	AF_NETBEUI	PF_NETBEUI
#define	AF_SECURITY	PF_SECURITY
#define	AF_KEY		PF_KEY
#define	AF_NETLINK	PF_NETLINK
#define	AF_ROUTE	PF_ROUTE
#define	AF_PACKET	PF_PACKET
#define	AF_ASH		PF_ASH
#define	AF_ECONET	PF_ECONET
#define	AF_ATMSVC	PF_ATMSVC
#define	AF_SNA		PF_SNA
#define AF_IRDA		PF_IRDA
#define	AF_MAX		PF_MAX

#if 0
/* Supported address families. */
#define PF_UNIX 1
#define AF_UNIX PF_UNIX
#define PF_INET	2		/* Internet IP Protocol         */
#define AF_INET	PF_INET		/* Internet IP Protocol         */
#endif

/* Maximum queue length specifiable by listen.  */
#define SOMAXCONN	128

#ifdef __KERNEL__
/* Flags we can use with send/ and recv. */
#define MSG_OOB		1
#define MSG_PEEK	2
#define MSG_DONTROUTE	4
/*#define MSG_CTRUNC	8	- We need to support this for BSD oddments */
#define MSG_PROXY	16	/* Supply or ask second address. */
#endif

/* For setsockoptions(2) */
#define SOL_SOCKET	1

#define SO_DEBUG	1
#define SO_REUSEADDR	2
#define SO_TYPE		3
#define SO_ERROR	4
#define SO_DONTROUTE	5
#define SO_BROADCAST	6
#define SO_SNDBUF	7
#define SO_RCVBUF	8
#define SO_KEEPALIVE	9
#define SO_OOBINLINE	10
#define SO_NO_CHECK	11
#define SO_PRIORITY	12
#define SO_LINGER	13
#define SO_BSDCOMPAT	14
/* To add :#define SO_REUSEPORT 15 */
#define SO_RCVLOWAT		18
#define SO_SNDLOWAT		19
#define SO_RCVTIMEO		20
#define SO_SNDTIMEO		21
#define SO_BINDTODEVICE	25

/* Setsockoptions(2) level. Thanks to BSD these must match IPPROTO_xxx */
#define SOL_IP		0
#define SOL_IPX		256
#define SOL_AX25	257
#define SOL_ATALK	258
#define	SOL_NETROM	259
#define SOL_TCP		6
#define SOL_UDP		17

#ifdef __KERNEL__

/* IP options */
#define IP_TOS		1
#define	IPTOS_LOWDELAY		0x10
#define	IPTOS_THROUGHPUT	0x08
#define	IPTOS_RELIABILITY	0x04
#define	IPTOS_MINCOST		0x02
#define IP_TTL		2
#define IP_HDRINCL	3
#define IP_OPTIONS	4

#define IP_MULTICAST_IF			32
#define IP_MULTICAST_TTL 		33
#define IP_MULTICAST_LOOP 		34
#define IP_ADD_MEMBERSHIP		35
#define IP_DROP_MEMBERSHIP		36


/*
 * User-settable options (used with setsockopt).
 */
#define	TCP_NODELAY	0x01	/* don't delay send to coalesce packets */
#define	TCP_MAXSEG	0x02	/* set maximum segment size */

void init_sockets( void );

void ne_get_ether_address(uint8 * pBuffer);
int ne_read(void *pBuffer, int nSize);
int ne_write(const void *pBuffer, int nSize);

int sys_socket(int nFamily, int nType, int nProtocol);

typedef int so_open(Socket_s * psSocket);
typedef int so_close(Socket_s * psSocket);
typedef int so_shutdown(Socket_s * psSocket, uint32 nHow);
typedef int so_bind(Socket_s * psSocket, const struct sockaddr *psAddr,
		    int nAddrSize);
typedef int so_connect(Socket_s * psSocket, const struct sockaddr *psAddr,
		       int nSize);
typedef int so_getsockname(Socket_s * psSocket, struct sockaddr *psName,
			   int *pnNameLen);
typedef int so_getpeername(Socket_s * psSocket, struct sockaddr *psName,
			   int *pnNameLen);
typedef ssize_t so_recvmsg(Socket_s * psSocket, struct msghdr *psMsg,
			   int nFlags);
typedef ssize_t so_sendmsg(Socket_s * psSocket, const struct msghdr *psMsg,
			   int nFlags);
typedef int so_add_select(Socket_s * psSocket, SelectRequest_s * psReq);
typedef int so_rem_select(Socket_s * psSocket, SelectRequest_s * psReq);
typedef int so_set_fflags(Socket_s * psSocket, uint32 nFlags);

typedef int so_listen(Socket_s * psSocket, int nBackLog);
typedef int so_accept(Socket_s * psSocket, struct sockaddr *psAddr,
		      int *pnSize);
typedef int so_setsockopt(bool bFromKernel, Socket_s * psSocket,
			  int nProtocol, int nOptName, const void *pOptVal,
			  int nOptLen);
typedef int so_getsockopt(bool bFromKernel, Socket_s * psSocket,
			  int nProtocol, int nOptName, void *pOptVal,
			  int nOptLen);
typedef int so_ioctl(Socket_s * psSocket, int nCmd, void *pBuffer,
		     bool bFromKernel);


typedef struct {
    so_open *open;
    so_close *close;
    so_shutdown *shutdown;
    so_bind *bind;
    so_connect *connect;
    so_getsockname *getsockname;
    so_getpeername *getpeername;
    so_recvmsg *recvmsg;
    so_sendmsg *sendmsg;
    so_add_select *add_select;
    so_rem_select *rem_select;
    so_listen *listen;
    so_accept *accept;
    so_setsockopt *setsockopt;
	so_getsockopt *getsockopt;
    so_set_fflags *set_fflags;
    so_ioctl *ioctl;
} SocketOps_s;

struct _Socket {
    ino_t sk_nInodeNum;
    int sk_nFamily;
    int sk_nType;
    int sk_nProto;

	/* Socket option flags */
    bool sk_bDebug;
    bool sk_bReuseAddr;
    bool sk_bOobInline; /* Receive out-of-band data in-band */
    bool sk_bDontRoute;
    bool sk_bKeep;      /* Send keep-alive messages */
    bool sk_bBroadcast;	/* Allow broadcast messages */

    ipaddr_t sk_anSrcAddr;
    ipaddr_t sk_anDstAddr;
    uint16 sk_nSrcPort;
    uint16 sk_nDstPort;
    bool sk_bIsBound;
    atomic_t sk_nOpenCount;
    union {
	UDPEndPoint_s *psUDPEndP;
    	TCPCtrl_s     *psTCPCtrl;
    	RawEndPoint_s *psRawEndP;
	void          *pData;
    } sk_uData;
#define sk_psUDPEndP sk_uData.psUDPEndP
#define sk_psTCPCtrl sk_uData.psTCPCtrl
#define sk_psRawEndP sk_uData.psRawEndP
#define sk_pData     sk_uData.pData
    SocketOps_s *sk_psOps;	/* operations   */
};


#endif				/* __KERNEL__ */



#ifdef __KERNEL__
int socket_read(int nPort, void *pBuffer, int nSize);
int socket_write(int nPort, const void *pBuffer, int a_nSize);

void dump_socket_info(Socket_s * psSocket);
int socket(int nFamily, int nType, int nProtocol);
int bind(int nFile, const struct sockaddr *psAddr, int nAddrSize);
int getsockname(int fd, struct sockaddr *addr, int *size);
int getpeername(int fd, struct sockaddr *addr, int *size);
ssize_t recvfrom(int nFile, void *pBuffer, size_t nSize, int nFlags,
		 struct sockaddr *psFrom, int *nFromLen);
ssize_t sendto(int nFile, const void *pBuffer, size_t nSize, int nFlags,
	       const struct sockaddr *psTo, int nToLen);

ssize_t recvmsg(int nFile, struct msghdr *psMsg, int nFlags);
ssize_t sendmsg(int nFile, const struct msghdr *psMsg, int nFlags);

ssize_t send(int nFile, const void *pBuffer, size_t nSize, int nFlags);
ssize_t recv(int nFile, void *pBuffer, size_t nSize, int nFlags);


int connect(int nFile, const struct sockaddr *psAddr, int nSize);
int listen(int nFile, int nBackLog);
int accept(int nFile, struct sockaddr *psAddr, int *pnSize);


int closesocket(int nFile);
int setsockopt(int nFile, int nLevel, int nOptName, const void *pOptVal,
	       int nOptLen);
int getsockopt(int nFile, int nLevel, int nOptName, void *pOptVal, int nOptLen);
int shutdown(int nFile, int nHow);
#endif

#ifdef __cplusplus
}
#endif

#endif				/* __F_ATHEOS_SOCKET_H__ */
