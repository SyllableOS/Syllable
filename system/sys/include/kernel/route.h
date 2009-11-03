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

#ifndef __F_KERNEL_ROUTE_H__
#define __F_KERNEL_ROUTE_H__

#include <net/route.h>

#include <kernel/types.h>

/**
 * Route structure used by kernel.
 * This structure should be considered READ ONLY outside of kernel/net/route.c
 */
typedef struct  _Route  Route_s;
struct  _Route
{
	/* Locked by the routing list mutex */
	Route_s*        rt_psNext;

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

#endif	/* __F_KERNEL_ROUTE_H__ */
