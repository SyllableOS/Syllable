/* Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* Based on the 4.4BSD and Linux version of this file.  */

#ifndef _NET_ROUTE_H
#define _NET_ROUTE_H	1

#include <atheos/types.h>
#include <net/net.h>

/* This structure gets passed by the SIOCADDRT and SIOCDELRT ioctl calls. */
struct rtentry
  {
    unsigned long int rt_pad1;
    struct sockaddr rt_dst;		/* Target address.  */
    struct sockaddr rt_gateway;		/* Gateway addr (RTF_GATEWAY).  */
    struct sockaddr rt_genmask;		/* Target network mask (IP).  */
    unsigned short int rt_flags;
    short int rt_pad2;
    unsigned long int rt_pad3;
    unsigned char rt_tos;
    unsigned char rt_class;
    short int rt_pad4;
    short int rt_metric;		/* +1 for binary compatibility!  */
    char *rt_dev;			/* Forcing the device at add.  */
    unsigned long int rt_mtu;		/* Per route MTU/Window.  */
    unsigned long int rt_window;	/* Window clamping.  */
    unsigned short int rt_irtt;		/* Initial RTT.  */
  };
/* Compatibility hack.  */
#define rt_mss	rt_mtu

struct rtabentry
{
    struct sockaddr rt_dst;		/* Target address.  */
    struct sockaddr rt_gateway;		/* Gateway addr (RTF_GATEWAY).  */
    struct sockaddr rt_genmask;		/* Target network mask (IP).  */
    unsigned short int rt_flags;
    uint8		rt_tos;
    uint8		rt_class;
    short int 	rt_metric;		/* +1 for binary compatibility!  */
    char 		rt_dev[64];	/* Forcing the device at add.  */
    uint32		rt_mtu;	/* Per route MTU/Window.  */
    uint32	        rt_window;	/* Window clamping.  */
    uint32		rt_irtt;	/* Initial RTT.  */
};

struct rttable
{
    int rtt_count;
    /* array of struct rtabentry follows */
};

/* Route flags */
#define	RTF_UP		0x0001		/* Route usable.  */
#define	RTF_GATEWAY	0x0002		/* Destination is a gateway.  */

#define	RTF_HOST	0x0004		/* Host entry (net otherwise).  */
#define RTF_REINSTATE	0x0008		/* Reinstate route after timeout.  */
#define	RTF_DYNAMIC	0x0010		/* Created dyn. (by redirect).  */
#define	RTF_MODIFIED	0x0020		/* Modified dyn. (by redirect).  */
#define RTF_MTU		0x0040		/* Specific MTU for this route.  */
#define RTF_MSS		RTF_MTU		/* Compatibility.  */
#define RTF_WINDOW	0x0080		/* Per route window clamping.  */
#define RTF_IRTT	0x0100		/* Initial round trip time.  */
#define RTF_REJECT	0x0200		/* Reject route.  */
#define	RTF_STATIC	0x0400		/* Manually injected route.  */
#define	RTF_XRESOLVE	0x0800		/* External resolver.  */
#define RTF_NOFORWARD   0x1000		/* Forwarding inhibited.  */
#define RTF_THROW	0x2000		/* Go to next class.  */
#define RTF_NOPMTUDISC  0x4000		/* Do not send packets with DF.  */

/* for IPv6 */
#define RTF_DEFAULT	0x00010000	/* default - learned via ND	*/
#define RTF_ALLONLINK	0x00020000	/* fallback, no routers on link	*/
#define RTF_ADDRCONF	0x00040000	/* addrconf route - RA		*/

#define RTF_LINKRT	0x00100000	/* link specific - device match	*/
#define RTF_NONEXTHOP	0x00200000	/* route with no nexthop	*/

#define RTF_CACHE	0x01000000	/* cache entry			*/
#define RTF_FLOW	0x02000000	/* flow significant route	*/
#define RTF_POLICY	0x04000000	/* policy route			*/

#define RTCF_VALVE	0x00200000
#define RTCF_MASQ	0x00400000
#define RTCF_NAT	0x00800000
#define RTCF_DOREDIRECT 0x01000000
#define RTCF_LOG	0x02000000
#define RTCF_DIRECTSRC	0x04000000

#define RTF_LOCAL	0x80000000
#define RTF_INTERFACE	0x40000000
#define RTF_MULTICAST	0x20000000
#define RTF_BROADCAST	0x10000000
#define RTF_NAT		0x08000000

#define RTF_ADDRCLASSMASK	0xF8000000
#define RT_ADDRCLASS(flags)	((__u_int32_t) flags >> 23)

#define RT_TOS(tos)		((tos) & IPTOS_TOS_MASK)

#define RT_LOCALADDR(flags)	((flags & RTF_ADDRCLASSMASK) \
				 == (RTF_LOCAL|RTF_INTERFACE))

#define RT_CLASS_UNSPEC		0
#define RT_CLASS_DEFAULT	253

#define RT_CLASS_MAIN		254
#define RT_CLASS_LOCAL		255
#define RT_CLASS_MAX		255


#define RTMSG_ACK		NLMSG_ACK
#define RTMSG_OVERRUN		NLMSG_OVERRUN

#define RTMSG_NEWDEVICE		0x11
#define RTMSG_DELDEVICE		0x12
#define RTMSG_NEWROUTE		0x21
#define RTMSG_DELROUTE		0x22
#define RTMSG_NEWRULE		0x31
#define RTMSG_DELRULE		0x32
#define RTMSG_CONTROL		0x40

#define RTMSG_AR_FAILED		0x51	/* Address Resolution failed.  */


#ifdef __KERNEL__

/**
 * Route structure used by kernel.
 * This structure should be considered READ ONLY outside of kernel/net/route.c
 */
typedef struct  _Route  Route_s;
struct  _Route
{
	/* Locked by the routing list mutex */
	Route_s*        rt_psNext;
	/* bigtime_t       rt_nExpireTime; */

	/* Locked by rt_hMutex */
	sem_id          rt_hMutex;
	ipaddr_t        rt_anNetAddr;
	ipaddr_t        rt_anNetMask;
	ipaddr_t        rt_anGatewayAddr;
	int             rt_nMaskBits, rt_nMetric;
	uint32          rt_nFlags;
	NetInterface_s* rt_psInterface;
};

typedef struct _RouteTable
{
    Route_s*  rtb_psFirstRoute;
    Route_s*  rtb_psDefaultRoute;
} RouteTable_s;


/**
 * Routing table interface
 */
/* Obtain a pointer to a cloned route to use for pDstAddr */
Route_s* ip_find_route( ipaddr_p pDstAddr );

/* Create a clone route that leaves given via the interface specified */
Route_s* ip_find_device_route( const char *pzName );

/* Find the static route entry matching the supplied parameters */
Route_s* ip_find_static_route( ipaddr_p pIp, ipaddr_p pMask, ipaddr_p pGw );


/* Add a reference to an existing route */
Route_s* ip_acquire_route( Route_s* psRoute );

/* Release a previously obtained route reference */
void ip_release_route( Route_s* psRoute );


/* Add a new (static) route */
int add_route( ipaddr_p pIp, ipaddr_p pMask, ipaddr_p pGw, int nMetric, uint32 nFlags );

/* Delete a (static) route */
int del_route( ipaddr_p pIp, ipaddr_p pMask, ipaddr_p pGw );


/* Network interface events */
/**
 * Call to make the routing code drop references to an interface so it can
 * be changed
 **/
void rt_release_interface( NetInterface_s* psIf );

/* Routing socket ioctl()s */
/* Return POSIX routing table structure */
int get_route_table( struct rttable* psTable );

/* Adds a route from a POSIX rtentry */
int add_route_entry( struct rtentry* psREntry );

/* Deletes the route specified by a POSIX rtentry */
int delete_route_entry( struct rtentry* psREntry );


/**
 * The following are only for use by the kernel proper and are not exported
 * into kernel.so.
 */
int init_route_table( void );

#endif /* __KERNEL__ */

#endif /* net/route.h */
