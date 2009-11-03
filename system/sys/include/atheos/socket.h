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

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol families.  */
#define	PF_UNSPEC		0	/* Unspecified.  */
#define	PF_LOCAL		1	/* Local to host (pipes and file-domain).  */
#define	PF_UNIX			PF_LOCAL	/* Old BSD name for PF_LOCAL.  */
#define	PF_FILE			PF_LOCAL	/* Another non-standard name for PF_LOCAL.  */
#define	PF_INET			2	/* IP protocol family.  */
#define	PF_AX25			3	/* Amateur Radio AX.25.  */
#define	PF_IPX			4	/* Novell Internet Protocol.  */
#define	PF_APPLETALK	5	/* Appletalk DDP.  */
#define	PF_NETROM		6	/* Amateur radio NetROM.  */
#define	PF_BRIDGE		7	/* Multiprotocol bridge.  */
#define	PF_ATMPVC		8	/* ATM PVCs.  */
#define	PF_X25			9	/* Reserved for X.25 project.  */
#define	PF_INET6		10	/* IP version 6.  */
#define	PF_ROSE			11	/* Amateur Radio X.25 PLP.  */
#define	PF_DECnet		12	/* Reserved for DECnet project.  */
#define	PF_NETBEUI		13	/* Reserved for 802.2LLC project.  */
#define	PF_SECURITY		14	/* Security callback pseudo AF.  */
#define	PF_KEY			15	/* PF_KEY key management API.  */
#define	PF_NETLINK		16
#define	PF_ROUTE	PF_NETLINK	/* Alias to emulate 4.4BSD.  */
#define	PF_PACKET		17	/* Packet family.  */
#define	PF_ASH			18	/* Ash.  */
#define	PF_ECONET		19	/* Acorn Econet.  */
#define	PF_ATMSVC		20	/* ATM SVCs.  */
#define	PF_SNA			22	/* Linux SNA Project */
#define PF_IRDA			23	/* IRDA sockets.  */
#define	PF_MAX			32	/* For now..  */

/* Address families.  */
#define	AF_UNSPEC		PF_UNSPEC
#define	AF_LOCAL		PF_LOCAL
#define	AF_UNIX			PF_UNIX
#define	AF_FILE			PF_FILE
#define	AF_INET			PF_INET
#define	AF_AX25			PF_AX25
#define	AF_IPX			PF_IPX
#define	AF_APPLETALK	PF_APPLETALK
#define	AF_NETROM		PF_NETROM
#define	AF_BRIDGE		PF_BRIDGE
#define	AF_ATMPVC		PF_ATMPVC
#define	AF_X25			PF_X25
#define	AF_INET6		PF_INET6
#define	AF_ROSE			PF_ROSE
#define	AF_DECnet		PF_DECnet
#define	AF_NETBEUI		PF_NETBEUI
#define	AF_SECURITY		PF_SECURITY
#define	AF_KEY			PF_KEY
#define	AF_NETLINK		PF_NETLINK
#define	AF_ROUTE		PF_ROUTE
#define	AF_PACKET		PF_PACKET
#define	AF_ASH			PF_ASH
#define	AF_ECONET		PF_ECONET
#define	AF_ATMSVC		PF_ATMSVC
#define	AF_SNA			PF_SNA
#define AF_IRDA			PF_IRDA
#define	AF_MAX			PF_MAX

/* Maximum queue length specifiable by listen.  */
#define SOMAXCONN		128

/* For setsockoptions(2) */
#define SOL_SOCKET		1

#define SO_DEBUG		1
#define SO_REUSEADDR	2
#define SO_TYPE			3
#define SO_ERROR		4
#define SO_DONTROUTE	5
#define SO_BROADCAST	6
#define SO_SNDBUF		7
#define SO_RCVBUF		8
#define SO_KEEPALIVE	9
#define SO_OOBINLINE	10
#define SO_NO_CHECK		11
#define SO_PRIORITY		12
#define SO_LINGER		13
#define SO_BSDCOMPAT	14
#define SO_REUSEPORT	15
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

#ifndef sa_family_t
typedef unsigned short sa_family_t;
#define sa_family_t sa_family_t
#endif

/* 1003.1g requires sa_family_t and that sa_data is char.*/
struct sockaddr
{
	sa_family_t sa_family;	/* address family, AF_xxx */
	char sa_data[14];		/* 14 bytes of protocol address */
};

#ifdef __cplusplus
}
#endif

#endif				/* __F_ATHEOS_SOCKET_H__ */
