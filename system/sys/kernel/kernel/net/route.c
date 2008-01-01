
/*
 *	Syllable Kernel
 *	net/route.c
 *
 *	The Syllable Kernel is derived from the AtheOS kernel
 *	Copyright (C) 1999 - 2000 Kurt Skauen
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of version 2 of the GNU Library
 *	General Public License as published by the Free Software
 *	Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
 */

#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/socket.h>
#include <atheos/semaphore.h>
#include <atheos/time.h>

#include <macros.h>

#include <net/net.h>
#include <net/ip.h>
#include <net/in.h>
#include <net/if.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <net/if_ether.h>
#include <net/if_arp.h>
#include <net/icmp.h>
#include <net/sockios.h>
#include <net/route.h>

#include "inc/semaphore.h"

/* Selective debugging level overrides */
#ifdef KERNEL_DEBUG_NET
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET
#endif

#ifdef KERNEL_DEBUG_NET_ROUTING
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET_ROUTING
#endif

/**
 * Route list changes are locked by g_hRouteListMutex.
 * 
 * Individual routes are locked, but can be removed from the route list
 * before being released.	When routes are removed from the list, they
 * are marked, so that the last release will also free the
 * route's memory.	Marking a route consists of setting its
 * rt_psNext to NULL.	Normally, the last route in a list/the default route
 * points to the sentinel route g_sSentinel.
 *
 * Routes are always read-locked while they are in the route lists.
 * This prevents them from being modified, which is intentional.
 * Anonymous locks are added for static routes by add_route(),
 * for cloned routes by ip_find_route() and for device routes by
 * ip_find_device_route().  These anonymous locks may be removed by
 * insert_route() if a route is replaced, by del_route() when a static
 * route is removed, or by rt_release_interface() for device and cloned
 * routes.
 *
 * Anonymous locks on the interface used by a route are also obtained
 * for cloned and device routes by ip_find_route() and ip_find_device_route(),
 * respectively.  They will be released by delete_route() or insert_route().
 */
static sem_id g_hRouteListMutex = -1, g_hTryReleaseMutex = -1;
static Route_s g_sSentinel;
static Route_s *g_psStaticRoutes = NULL;
static Route_s *g_psClonedRoutes = NULL;
static Route_s *g_psDeviceRoutes = NULL;

/**
 * On Intel, there's no problem with misaligned words so the following
 * optimisation makes sense (and is used elsewhere).
 */
static inline bool compare_net_address( ipaddr_t anAddr1, ipaddr_t anAddr2, ipaddr_t anMask )
{
	return ( ( *( uint32 * )anAddr1 ) & ( *( uint32 * )anMask ) ) == ( ( *( uint32 * )anAddr2 ) & ( *( uint32 * )anMask ) );
}


static void db_dump_routes( int argc, char **argv );

/**
 * \par Description:
 * Initialise routing table.
 */
int init_route_table( void )
{
	if ( g_hRouteListMutex > 0 )
	{
		kerndbg( KERN_WARNING, "init_route_table(): Routing already initialised!\n" );
		return -EINVAL;
	}
	else
	{
		g_hRouteListMutex = CREATE_RWLOCK( "routing table mutex" );
		g_hTryReleaseMutex = CREATE_MUTEX( "routing try-release mutex" );
	}

	memset( &g_sSentinel, 0, sizeof( g_sSentinel ) );
	g_sSentinel.rt_psNext = &g_sSentinel;

	/* Initialise lists */
	g_psDeviceRoutes = &g_sSentinel;
	g_psClonedRoutes = &g_sSentinel;
	g_psStaticRoutes = &g_sSentinel;

	register_debug_cmd( "dump_routes", "show routing table", db_dump_routes );

	return g_hRouteListMutex < 0 ? g_hRouteListMutex : 0;
}

/**
 * \par Description:
 * Internal routine to create a route.  The route is not added to any
 * lists and is not locked.
 */
static Route_s *create_route( ipaddr_p pnIpAddr, ipaddr_p pnNetmask, ipaddr_p pnGateway, int nMetric, uint32 nFlags )
{
	Route_s *psRoute;
	uint32 nNetMask;
	int nMaskBits;

	psRoute = kmalloc( sizeof( Route_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( psRoute == NULL )
	{
		kerndbg( KERN_FATAL, "create_route(): Unable to allocate memory for route.\n" );

		return NULL;
	}

	/* Allocate reader-writer lock */
	psRoute->rt_hMutex = CREATE_RWLOCK( "route mutex" );
	if ( ( psRoute->rt_hMutex & SEMTYPE_KERNEL ) == 0 )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Created non-kernel semaphore!\n" );
	}

	if ( psRoute->rt_hMutex < 0 )
	{
		kerndbg( KERN_FATAL, "create_route(): Unable to allocate route mutex.\n" );

		kfree( psRoute );
		return NULL;
	}

	/* Fill in route details */
	IP_COPYMASK( psRoute->rt_anNetAddr, pnIpAddr, pnNetmask );
	IP_COPYADDR( psRoute->rt_anNetMask, pnNetmask );

	/* Gateway address copied if RTF_GATEWAY specified (see below) */
	if ( nFlags & RTF_GATEWAY )
		IP_COPYADDR( psRoute->rt_anGatewayAddr, pnGateway );

	/* Count bits in netmask */
	nMaskBits = 0;
	nNetMask = *( ( uint32 * )pnNetmask );
	while ( nNetMask )
	{
		nMaskBits += ( nNetMask & 1 );
		nNetMask >>= 1;
	}

	psRoute->rt_nMaskBits = nMaskBits;
	psRoute->rt_nMetric = nMetric;
	psRoute->rt_nFlags = nFlags;
	/* psRoute->rt_psInterface = NULL; -- not necessary */

	return psRoute;
}

/**
 * Internal routine to delete a route.
 *
 * Others should call ip_release_route() to release a pointer to a route.
 * This function will be called as needed by ip_release_route().
 */
static void delete_route( Route_s *psRoute )
{
	if ( psRoute == NULL )
	{
		kerndbg( KERN_WARNING, "delete_route(): Asked to delete NULL route.\n" );

		return;
	}

	/* Release the route interface, if any */
	if ( psRoute->rt_psInterface )
	{
		/* Convert anonymous reference to local thread reference */
		if ( rwl_convert_from_anon( psRoute->rt_psInterface->ni_hMutex ) < 0 )
		{
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Interface mutex conversion failed.\n" );
		}

		release_net_interface( psRoute->rt_psInterface );
	}

	delete_semaphore( psRoute->rt_hMutex );

	kerndbg( KERN_DEBUG, "delete_route(): Freeing memory for route (0x%8.8lX).\n", ( uint32 )psRoute );

	kfree( psRoute );
}

/**
 * Internal routine to insert route into a list.
 *
 * This insert routine sorts routes into descending order by number of mask
 * bits, which means searches will always find the best route first.
 *
 * If bReplace is true, the new route will replace another with identical
 * address/netmask fields if found.  This expects to release an anonymous
 * read lock.
 * 
 * This routine must be called with an exclusive lock held on the route list
 * mutex.
 */
void insert_route( Route_s *psRoute, Route_s **ppsList, bool bReplace )
{
	Route_s **ppsCurrent, *psReplace;

	for ( ppsCurrent = ppsList; ( *ppsCurrent ) != &g_sSentinel; ppsCurrent = &( *ppsCurrent )->rt_psNext )
	{
		if ( ( *ppsCurrent )->rt_nMaskBits <= psRoute->rt_nMaskBits )
			break;
	}

	/* Keep cycling to check that this route is not replacing another */
	for ( ; ( *ppsCurrent ) != &g_sSentinel; ppsCurrent = &( *ppsCurrent )->rt_psNext )
	{
		if ( ( *ppsCurrent )->rt_nMaskBits < psRoute->rt_nMaskBits )
			break;

		if ( IP_SAMEADDR( ( *ppsCurrent )->rt_anNetAddr, psRoute->rt_anNetAddr ) && IP_SAMEADDR( ( *ppsCurrent )->rt_anNetMask, psRoute->rt_anNetMask ) )
			break;
	}

	if ( bReplace && IP_SAMEADDR( ( *ppsCurrent )->rt_anNetAddr, psRoute->rt_anNetAddr ) && IP_SAMEADDR( ( *ppsCurrent )->rt_anNetMask, psRoute->rt_anNetMask ) && ( *ppsCurrent ) != &g_sSentinel )
	{
		/* Save route being replaced */
		psReplace = ( *ppsCurrent );

		/* Unlink old and insert new  */
		psRoute->rt_psNext = psReplace->rt_psNext;
		( *ppsCurrent ) = psRoute;

		/* Mark for deletion */
		psReplace->rt_psNext = NULL;

		/* Convert anonymous route reference acquired for list to local thread */
		if ( rwl_convert_from_anon( psReplace->rt_hMutex ) < 0 )
		{
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Route mutex conversion failed (1).\n" );
		}

		/* Release the route */
		ip_release_route( psReplace );
	}
	else
	{
		/* Insert */
		psRoute->rt_psNext = ( *ppsCurrent );
		( *ppsCurrent ) = psRoute;
	}

	/* Convert incoming route mutex reference to anonymous reference */
	if ( rwl_convert_to_anon( psRoute->rt_hMutex ) < 0 )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Route mutex conversion failed (2).\n" );
	}

}

/**
 * \par Description:
 * Adds a reference to a route pointer.
 *
 * Will return a newly referenced route, or NULL if unsuccessful.
 */
Route_s *ip_acquire_route( Route_s *psRoute )
{
	if ( psRoute == NULL )
		return NULL;

	if ( RWL_LOCK_RO( psRoute->rt_hMutex ) < 0 )
	{
		kerndbg( KERN_FATAL, "ip_acquire_route(): Unable to lock route mutex.\n" );

		return NULL;
	}

	return psRoute;
}

/**
 * \par Description:
 * Finds the routing table entry that would reach pDstAddr.
 * If there are no explicit matching routes, the default
 * route is returned.  The route that is returned is always
 * a clone of the original route, with the appropriate
 * interface referenced.
 *
 * \par Note:
 * Routes that are returned must be released after use
 * with ip_release_route().
 */
Route_s *ip_find_route( ipaddr_t pDstAddr )
{
	Route_s *psRoute;
	Route_s *psFound;
	NetInterface_s *psIf;
	int nError;

	if ( ( nError = RWL_LOCK_RW( g_hRouteListMutex ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "ip_find_route(): Unable to lock routing table (%d)\n", nError );

		return NULL;
	}

	/* Scan for a matching cloned route */
	psFound = NULL;
	for ( psRoute = g_psClonedRoutes; psRoute != &g_sSentinel; psRoute = psRoute->rt_psNext )
	{
		/* XXXKV: This is a hack. I am unsure how the initial "NULL" route ever gets into the table,
		   and it disappears after the interface is taken down and back up. This just avoids the
		   problem from mucking up routing. */
		ipaddr_t pNull = { 0x0, 0x0, 0x0, 0x0 };
		if( IP_SAMEADDR( psRoute->rt_anNetAddr, pNull ) && IP_SAMEADDR( psRoute->rt_anNetMask, pNull ) )
			continue;

		if ( compare_net_address( pDstAddr, psRoute->rt_anNetAddr, psRoute->rt_anNetMask ) )
		{
			psFound = ip_acquire_route( psRoute );
			if ( psFound == NULL )
			{
				kerndbg( KERN_DEBUG, "ip_find_route(): Unable to acquire cloned route!\n" );
				goto exit;
			}

			break;
		}
	}

	/* If we found a cloned route, go to the exit */
	if ( psFound != NULL )
		goto exit;

	/* Plan B: Is there a directly connected interface? */
	if ( ( psIf = find_interface_by_addr( pDstAddr ) ) != NULL )
	{
		/* Great! Create a cloned route based on the interface. */
		psRoute = create_route( psIf->ni_anIpAddr, psIf->ni_anSubNetMask, NULL, 0, RTF_UP | RTF_LOCAL | RTF_INTERFACE );

		if ( psRoute == NULL )
		{
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): No memory for new cloned route.\n" );
			release_net_interface( psIf );

			goto exit;
		}

		/* Copy in interface pointer */
		psRoute->rt_psInterface = psIf;

		goto finish_clone;
	}

	/* Plan C: Look for a static route to clone. */
	for ( psRoute = g_psStaticRoutes; psRoute != &g_sSentinel; psRoute = psRoute->rt_psNext )
	{
		if ( compare_net_address( pDstAddr, psRoute->rt_anNetAddr, psRoute->rt_anNetMask ) )
		{
			psFound = psRoute;
			break;
		}
	}

	/* If we didn't find a static route, jump to exit */
	if ( psFound == NULL )
		goto exit;

	/**
	 * Find interface for the route's gateway
	 * Note that non-gateway static routes are ignored, as they can only specify
	 * directly connected hosts, which we will pick up with the code above.
	 **/
	if ( ( psFound->rt_nFlags & RTF_GATEWAY ) == 0 )
	{
		kerndbg( KERN_INFO, "ip_find_route(): Ignoring non-gateway route.\n" );

		psFound = NULL;

		goto exit;
	}

	/* Look up interface that can reach the specified gateway */
	psIf = find_interface_by_addr( psFound->rt_anGatewayAddr );
	if ( psIf == NULL )
	{
		kerndbg( KERN_INFO, "ip_find_route(): Gateway is not directly reachable.\n" );

		psFound = NULL;

		goto exit;
	}

	/* Create a cloned route from a static route */
	psRoute = create_route( psFound->rt_anNetAddr, psFound->rt_anNetMask, psFound->rt_anGatewayAddr, psFound->rt_nMetric, psFound->rt_nFlags );

	if ( psRoute == NULL )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): No memory for new cloned route.\n" );
		release_net_interface( psIf );

		goto exit;
	}

	/* Copy in the new interface */
	psRoute->rt_psInterface = psIf;


      finish_clone:
	/* Convert interface reference to anonymous reference */
	if ( rwl_convert_to_anon( psRoute->rt_psInterface->ni_hMutex ) < 0 )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Conversion to anonymous lock failed.\n" );

		release_net_interface( psRoute->rt_psInterface );
		psRoute->rt_psInterface = NULL;

		delete_route( psRoute );
	}

	/* Acquire a reference for the cloned route list and make it anonymous */
	if ( ip_acquire_route( psRoute ) != psRoute )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Unable to acquire route reference for clone list.\n" );

		delete_route( psRoute );
	}

	/* Add to cloned route list */
	insert_route( psRoute, &g_psClonedRoutes, true );

	/* Acquire a reference to the new cloned route for the caller */
	psFound = ip_acquire_route( psRoute );
	if ( psFound == NULL )
	{
		kerndbg( KERN_FATAL, "ip_find_route(): Unable to add reference to new cloned route.\n" );
	}

      exit:
	RWL_UNLOCK_RW( g_hRouteListMutex );

	return psFound;
}


/**
 * \par Creates a cloned route that provides a default route via the
 * interface named pzIfName.
 *
 * The returned route must be released with ip_release_route() when
 * finished with.
 *
 * If an error occurs, NULL is returned.
 */
Route_s *ip_find_device_route( const char *pzIfName )
{
	Route_s *psRoute, *psFound = NULL;
	ipaddr_t anNullAddr = { 0, 0, 0, 0 };

	/* Lock the route lists to look for an existing device route */
	if ( RWL_LOCK_RW( g_hRouteListMutex ) < 0 )
	{
		kerndbg( KERN_FATAL, "ip_find_device_route(): Unable to lock the route list read-write.\n" );

		return NULL;
	}

	/* Scan for a matching device route */
	psFound = NULL;
	for ( psRoute = g_psDeviceRoutes; psRoute != &g_sSentinel; psRoute = psRoute->rt_psNext )
	{
		if ( psRoute->rt_psInterface == NULL )
		{
			kerndbg( KERN_FATAL, "ip_find_device_route(): Device route does not have an interface!\n" );

			continue;
		}

		if ( strcmp( psRoute->rt_psInterface->ni_zName, pzIfName ) == 0 )
		{
			psFound = ip_acquire_route( psRoute );
			if ( psFound == NULL )
			{
				kerndbg( KERN_DEBUG, "ip_find_route(): Unable to acquire cloned route!\n" );
				goto exit;
			}

			break;
		}
	}

	/* If we found a cloned route, go to the exit */
	if ( psFound != NULL )
		goto exit;

	/* Create a default route for the interface */
	if ( ( psRoute = create_route( anNullAddr, anNullAddr, NULL, 0, RTF_UP ) ) == NULL )
		goto exit;

	/* Copy in the new interface */
	psRoute->rt_psInterface = find_interface( pzIfName );
	if ( psRoute->rt_psInterface == NULL )
	{
		delete_route( psRoute );

		goto exit;
	}

	/* Convert the interface reference to an anonymous reference */
	if ( rwl_convert_to_anon( psRoute->rt_psInterface->ni_hMutex ) < 0 )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Interface mutex conversion failed.\n" );
		release_net_interface( psRoute->rt_psInterface );
		psRoute->rt_psInterface = NULL;
		delete_route( psRoute );

		goto exit;
	}

	/* Acquire the new route reference for the list */
	if ( ip_acquire_route( psRoute ) != psRoute )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Unable to acquire reference for route list.\n" );
		delete_route( psRoute );

		goto exit;
	}

	/* Add to device route list */
	insert_route( psRoute, &g_psDeviceRoutes, false );

	/* Acquire the new route for the calling thread */
	if ( ( psFound = ip_acquire_route( psRoute ) ) == NULL )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Unable to acquire reference for caller.\n" );
	}

      exit:
	RWL_UNLOCK_RW( g_hRouteListMutex );

	return psFound;
}


/**
 * \par Description:
 * Locates the first static route that matches all of the provided criteria.
 * The route will have to be released after use by calling ip_release_route().
 */
Route_s *ip_find_static_route( ipaddr_p anIpAddr, ipaddr_p anNetMask, ipaddr_p anGwAddr )
{
	Route_s *psCurrent;

	if ( RWL_LOCK_RO( g_hRouteListMutex ) < 0 )
	{
		kerndbg( KERN_FATAL, "ip_find_static_route(): Unable to lock route list!\n" );

		return NULL;
	}

	for ( psCurrent = g_psStaticRoutes; psCurrent != &g_sSentinel; psCurrent = psCurrent->rt_psNext )
	{
		if ( ( anIpAddr == NULL || IP_SAMEADDR( psCurrent->rt_anNetAddr, anIpAddr ) ) && ( anNetMask == NULL || IP_SAMEADDR( psCurrent->rt_anNetMask, anNetMask ) ) && ( anGwAddr == NULL || IP_SAMEADDR( psCurrent->rt_anGatewayAddr, anGwAddr ) ) )
			break;
	}

	if ( psCurrent != &g_sSentinel )
	{
		psCurrent = ip_acquire_route( psCurrent );
		if ( psCurrent == NULL )
		{
			kerndbg( KERN_FATAL, "ip_find_static_route(): Unable to add ref to route!\n" );
		}
	}
	else
	{
		psCurrent = NULL;
	}

	RWL_UNLOCK_RO( g_hRouteListMutex );

	return psCurrent;
}

/**
 * \par Description:
 * Decrease the route's reference count.	If the route is marked as deleted,
 * and its reference count goes to zero, the route structure is freed.
 */
void ip_release_route( Route_s *psRoute )
{
	int nError, nMayDelete = 0;

	if ( psRoute == NULL )
	{
		kerndbg( KERN_WARNING, "ip_release_route(): Asked to release null pointer.\n" );

		return;
	}

	/* Lock the route list so we can see if we should try to delete this
	 * interface (e.g. is rt_psNext == NULL).
	 */
	if ( RWL_LOCK_RO( g_hRouteListMutex ) < 0 )
	{
		kerndbg( KERN_FATAL, "ip_release_route(): Unable to lock route list!\n" );

		return;
	}

	nMayDelete = ( psRoute->rt_psNext == NULL );

	/* Unlock the list */
	RWL_UNLOCK_RO( g_hRouteListMutex );


	if ( nMayDelete == 0 )
	{
		/* No need to delete, just release our reference */
		RWL_UNLOCK_RO( psRoute->rt_hMutex );
		return;
	}

	/* As for interfaces, we must enforce a critical section between trying
	 * to upgrade to a full write lock and unlocking.
	 */
	if ( LOCK( g_hTryReleaseMutex ) < 0 )
	{
		kerndbg( KERN_FATAL, "ip_release_route(): Unable to lock try-release mutex.\n" );

		return;
	}

	if ( ( nError = RWL_TRY_UPGRADE( psRoute->rt_hMutex ) ) < 0 )
	{
		if ( nError == -ETIME || nError == -EBUSY )
		{
			/* There are others yet to release */
			/* Release our read-only lock (decrease reference count) */
			RWL_UNLOCK_RO( psRoute->rt_hMutex );
		}
		else
		{
			kerndbg( KERN_FATAL, "ip_release_route(): Unable to upgrade route mutex.\n" );
		}
	}
	else
	{

		/**
		 * Check that semaphore reader and writer counts are one, as other values
		 * indicate someone else has the route in use.
		 */
		if ( rwl_count_all_readers( psRoute->rt_hMutex ) == 1 && rwl_count_all_writers( psRoute->rt_hMutex ) == 1 )
		{
			/* Delete the route instance */
			delete_route( psRoute );
		}
		else
		{
			/* There are others yet to release */
			/* Release our read-write lock (decrease reference count) */
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): RWLock count snafu: Lock ID %d.\n", psRoute->rt_hMutex );

			RWL_UNLOCK_RW( psRoute->rt_hMutex );
			RWL_UNLOCK_RO( psRoute->rt_hMutex );
		}
	}

	UNLOCK( g_hTryReleaseMutex );
}

/**
 * \par Description:
 * Adds a new static route for the network/host specified by pNetAddr and
 * pNetMask.
 *
 * Static routes are only considered if they set the RTF_GATEWAY flag.
 * Non-gateway routes (e.g. directly connected routes) are always based on
 * the available interfaces and cannot be statically configured.
 *
 * Once a route has been added, it can be retrieved by searching with the
 * same address, netmask and gateway using ip_find_static_route().
 *
 * \return Returns zero on success, or a negated errno error code if
 * unsuccessful.
 */
int add_route( ipaddr_p pNetAddr, ipaddr_p pNetMask, ipaddr_p pGateway, int nMetric, uint32 nFlags )
{
	Route_s *psRoute;
	int nError;

	USE_FORMAT_IP( 3 );

	/* Check arguments */
	if ( pNetAddr == NULL || pNetMask == NULL || ( ( nFlags & RTF_GATEWAY ) > 0 && pGateway == NULL ) )
	{
		kerndbg( KERN_DEBUG, "add_route(): Invalid arguments.\n" );

		return -EINVAL;
	}

	/* Allocate structure */
	psRoute = create_route( pNetAddr, pNetMask, pGateway, nMetric, nFlags | RTF_UP | RTF_STATIC );

	if ( psRoute == NULL )
	{
		kerndbg( KERN_FATAL, "add_route(): Unable to allocate memory for new route.\n" );
		return -ENOMEM;
	}

	if ( ip_acquire_route( psRoute ) != psRoute )
	{
		kerndbg( KERN_FATAL, "add_route(): Unable to add reference to new route.\n" );

		delete_route( psRoute );
		return -ENOLCK;
	}

	/* Lock the route list */
	if ( ( nError = RWL_LOCK_RW( g_hRouteListMutex ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "add_route(): Unable to lock route list\n" );

		delete_route( psRoute );
		return -ENOLCK;
	}

	FORMAT_IP( psRoute->rt_anNetAddr, 0 );
	FORMAT_IP( psRoute->rt_anNetMask, 1 );
	FORMAT_IP( psRoute->rt_anGatewayAddr, 2 );

	kerndbg( KERN_DEBUG, __FUNCTION__ "(): Add static route to %s/%s via %s.\n", FORMATTED_IP( 0 ), FORMATTED_IP( 1 ), FORMATTED_IP( 2 ) );

	/* Insert into static route list (may replace existing route) */
	insert_route( psRoute, &g_psStaticRoutes, true );

	RWL_UNLOCK_RW( g_hRouteListMutex );

	return 0;
}

/**
 * \par Description:
 * Removes the specified route from the list and marks it for deletion.
 *
 * Pointers to the route remain valid until the last one is released with
 * ip_release_route().
 */
int del_route( ipaddr_p anIpAddr, ipaddr_p anNetMask, ipaddr_p anGwAddr )
{
	Route_s **ppsCurrent, *psRemove;
	int nError;

	if ( ( nError = RWL_LOCK_RW( g_hRouteListMutex ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "del_route(): Unable to lock route list!\n" );

		return nError;
	}

	for ( ppsCurrent = &g_psStaticRoutes; ( *ppsCurrent ) != &g_sSentinel; ppsCurrent = &( *ppsCurrent )->rt_psNext )
	{
		if ( ( anIpAddr == NULL || IP_SAMEADDR( ( *ppsCurrent )->rt_anNetAddr, anIpAddr ) ) && ( anNetMask == NULL || IP_SAMEADDR( ( *ppsCurrent )->rt_anNetMask, anNetMask ) ) && ( anGwAddr == NULL || IP_SAMEADDR( ( *ppsCurrent )->rt_anGatewayAddr, anGwAddr ) ) )
			break;
	}

	if ( ( *ppsCurrent ) != &g_sSentinel )
	{
		psRemove = ( *ppsCurrent );

		( *ppsCurrent ) = psRemove->rt_psNext;
		psRemove->rt_psNext = NULL;

		/* Convert anonymous reference to local thread reference */
		if ( rwl_convert_from_anon( psRemove->rt_hMutex ) < 0 )
		{
			kerndbg( KERN_DEBUG, __FUNCTION__ "(): Conversion to thread reference failed.\n" );
		}

		ip_release_route( psRemove );
	}
	else
	{
		nError = -ENOENT;
	}

	RWL_UNLOCK_RW( g_hRouteListMutex );

	return nError;
}

/** 
 * \par Description:
 * Converts an internal route representation into a POSIX rtabentry.
 *
 * Assumes the route has been acquired previously.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 */
static void convert_rtentry( struct rtabentry *psDst, Route_s *psSrc )
{
	memset( psDst, 0, sizeof( *psDst ) );

	psDst->rt_flags = ( unsigned short )psSrc->rt_nFlags;
	psDst->rt_metric = psSrc->rt_nMetric;

	memcpy( ( ( struct sockaddr_in * )&psDst->rt_dst )->sin_addr, psSrc->rt_anNetAddr, IP_ADR_LEN );
	memcpy( ( ( struct sockaddr_in * )&psDst->rt_genmask )->sin_addr, psSrc->rt_anNetMask, IP_ADR_LEN );
	memcpy( ( ( struct sockaddr_in * )&psDst->rt_gateway )->sin_addr, psSrc->rt_anGatewayAddr, IP_ADR_LEN );

	if ( psSrc->rt_psInterface )
		strcpy( psDst->rt_dev, psSrc->rt_psInterface->ni_zName );
	else
		strcpy( psDst->rt_dev, "" );
}

/**
 * \par Description:
 * Creates a rtentry from a network interface.
 */
static void make_if_rtentry( struct rtabentry *psDst, NetInterface_s *psSrc )
{
	ipaddr_t anNetAddr;

	memset( psDst, 0, sizeof( *psDst ) );

	psDst->rt_flags = RTF_UP;
	psDst->rt_metric = 0;

	if ( ( psSrc->ni_nFlags & IFF_POINTOPOINT ) != 0 )
	{
		/* Use destination address of interface */
		psDst->rt_flags |= RTF_HOST;
		memcpy( ( ( struct sockaddr_in * )&psDst->rt_dst )->sin_addr, psSrc->ni_anExtraAddr, IP_ADR_LEN );
		memset( ( ( struct sockaddr_in * )&psDst->rt_genmask )->sin_addr, -1, IP_ADR_LEN );
	}
	else
	{
		/* Use interface network and mask */
		IP_COPYMASK( anNetAddr, psSrc->ni_anIpAddr, psSrc->ni_anSubNetMask );
		memcpy( ( ( struct sockaddr_in * )&psDst->rt_dst )->sin_addr, anNetAddr, IP_ADR_LEN );
		memcpy( ( ( struct sockaddr_in * )&psDst->rt_genmask )->sin_addr, psSrc->ni_anSubNetMask, IP_ADR_LEN );
	}

	strcpy( psDst->rt_dev, psSrc->ni_zName );
}

/** 
 * \par Description:
 * Creates a POSIX route table structure from the internal route list.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 */
int get_route_table( struct rttable *psTable )
{
	struct rtabentry *psRTTable = ( struct rtabentry * )( psTable + 1 );
	Route_s *psRoute;
	int nError, nCount, nReturn, nMaxCount, i;

	if ( memcpy_from_user( &nMaxCount, &psTable->rtt_count, sizeof( nMaxCount ) ) < 0 )
		return -EFAULT;

	/* Lock the route list */
	if ( ( nError = RWL_LOCK_RO( g_hRouteListMutex ) ) < 0 )
		return nError;

	nCount = 0;
	nReturn = 0;
	for ( psRoute = g_psStaticRoutes; psRoute != &g_sSentinel; psRoute = psRoute->rt_psNext )
	{
		/* No need to lock routes as we aren't keeping them after unlocking the
		 * list.
		 */
		++nCount;

		if ( nReturn < nMaxCount )
		{
			struct rtabentry sREntry;

			convert_rtentry( &sREntry, psRoute );

			if ( memcpy_to_user( psRTTable, &sREntry, sizeof( sREntry ) ) < 0 )
			{
				nCount = -EFAULT;
				break;
			}

			++psRTTable;
			++nReturn;
		}
	}

	/* Try to grab the each active interface */
	for ( i = 0; i < MAX_NET_INTERFACES; ++i )
	{
		NetInterface_s *psIf;

		if ( ( psIf = get_net_interface( i ) ) == NULL )
			continue;

		/* Skip downed interfaces */
		if ( ( psIf->ni_nFlags & IFF_UP ) == 0 )
		{
			release_net_interface( psIf );
			continue;
		}

		++nCount;

		if ( nReturn < nMaxCount )
		{
			struct rtabentry sREntry;

			make_if_rtentry( &sREntry, psIf );

			if ( memcpy_to_user( psRTTable, &sREntry, sizeof( sREntry ) ) < 0 )
			{
				nCount = -EFAULT;
				break;
			}

			++psRTTable;
			++nReturn;
		}

		release_net_interface( psIf );
	}

	nError = nCount;

	if ( nError > -1 && memcpy_to_user( &psTable->rtt_count, &nReturn, sizeof( nReturn ) ) < 0 )
	{
		nError = -EFAULT;
	}

	RWL_UNLOCK_RO( g_hRouteListMutex );

	return nError;
}

/** 
 * \par Description:
 * Adds a route specified as a POSIX rtentry.
 *
 * This wraps add_route() for use by ioctl's on routing sockets.
 * \author	Kurt Skauen (kurt@atheos.cx)
 */
int add_route_entry( struct rtentry *psREntry )
{
	struct sockaddr_in *psDestAddr = ( struct sockaddr_in * )&psREntry->rt_dst;
	struct sockaddr_in *psNetMask = ( struct sockaddr_in * )&psREntry->rt_genmask;
	struct sockaddr_in *psGateway = ( struct sockaddr_in * )&psREntry->rt_gateway;

	return add_route( ( ipaddr_p ) & psDestAddr->sin_addr, ( ipaddr_p ) & psNetMask->sin_addr, ( ipaddr_p ) & psGateway->sin_addr, psREntry->rt_metric, psREntry->rt_flags );
}

/** 
 * \par Description:
 * Deletes a route specified as a POSIX rtentry.
 *
 * This wraps del_route() for use by ioctl's on routing sockets.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
int delete_route_entry( struct rtentry *psREntry )
{
	struct sockaddr_in *psDestAddr = ( struct sockaddr_in * )&psREntry->rt_dst;
	struct sockaddr_in *psNetMask = ( struct sockaddr_in * )&psREntry->rt_genmask;

	return del_route( ( ipaddr_p ) & psDestAddr->sin_addr, ( ipaddr_p ) & psNetMask->sin_addr, NULL );
}

static void db_dump_routes( int argc, char **argv )
{
	int nWhat = 7;
	Route_s *psCurrent;
	char *pzName;
	char *NO_NAME = "(none)";

	USE_FORMAT_IP( 3 );

	// dbprintf( DBP_DEBUGGER, "", ... );
	if ( argc > 2 )
	{
		dbprintf( DBP_DEBUGGER, "Usage: %s [cloned|device|static|all]\n", argv[0] );

		return;
	}

	if ( argc > 1 )
	{
		if ( strcmp( argv[1], "cloned" ) == 0 )
			nWhat = 1;
		else if ( strcmp( argv[1], "device" ) == 0 )
			nWhat = 2;
		else if ( strcmp( argv[1], "static" ) == 0 )
			nWhat = 4;
	}

	if ( ( nWhat & 1 ) != 0 )
	{
		/* Dump cloned routes */
		dbprintf( DBP_DEBUGGER, "+ Cloned routes\n" );
		for ( psCurrent = g_psClonedRoutes; psCurrent != &g_sSentinel; psCurrent = psCurrent->rt_psNext )
		{
			if ( psCurrent == NULL )
			{
				dbprintf( DBP_DEBUGGER, "## Fault: psCurrent == NULL\n" );
				break;
			}

			FORMAT_IP( psCurrent->rt_anNetAddr, 0 );
			FORMAT_IP( psCurrent->rt_anNetMask, 1 );
			FORMAT_IP( psCurrent->rt_anGatewayAddr, 2 );

			if ( psCurrent->rt_psInterface != NULL )
				pzName = psCurrent->rt_psInterface->ni_zName;
			else
				pzName = NO_NAME;

			dbprintf( DBP_DEBUGGER, "%c-+ Reach %s/%s (%d) via %s; interface: %s.\n", ( psCurrent->rt_psNext == &g_sSentinel ) ? '`' : '|', FORMATTED_IP( 0 ), FORMATTED_IP( 1 ), psCurrent->rt_nMaskBits, FORMATTED_IP( 2 ), pzName );
		}

		dbprintf( DBP_DEBUGGER, "\n" );
	}

	if ( ( nWhat & 2 ) != 0 )
	{
		/* Dump device routes */
		dbprintf( DBP_DEBUGGER, "+ Device routes\n" );
		for ( psCurrent = g_psDeviceRoutes; psCurrent != &g_sSentinel; psCurrent = psCurrent->rt_psNext )
		{
			if ( psCurrent == NULL )
			{
				dbprintf( DBP_DEBUGGER, "## Fault: psCurrent == NULL\n" );
				break;
			}

			FORMAT_IP( psCurrent->rt_anNetAddr, 0 );
			FORMAT_IP( psCurrent->rt_anNetMask, 1 );
			FORMAT_IP( psCurrent->rt_anGatewayAddr, 2 );

			if ( psCurrent->rt_psInterface != NULL )
				pzName = psCurrent->rt_psInterface->ni_zName;
			else
				pzName = NO_NAME;

			dbprintf( DBP_DEBUGGER, "%c-+ Reach %s/%s (%d) via %s; interface: %s.\n", ( psCurrent->rt_psNext == &g_sSentinel ) ? '`' : '|', FORMATTED_IP( 0 ), FORMATTED_IP( 1 ), psCurrent->rt_nMaskBits, FORMATTED_IP( 2 ), pzName );
		}

		dbprintf( DBP_DEBUGGER, "\n" );
	}

	if ( ( nWhat & 4 ) != 0 )
	{
		/* Dump static routes */
		dbprintf( DBP_DEBUGGER, "+ Static routes\n" );

		for ( psCurrent = g_psStaticRoutes; psCurrent != &g_sSentinel; psCurrent = psCurrent->rt_psNext )
		{
			if ( psCurrent == NULL )
			{
				dbprintf( DBP_DEBUGGER, "## Fault: psCurrent == NULL\n" );
				break;
			}

			FORMAT_IP( psCurrent->rt_anNetAddr, 0 );
			FORMAT_IP( psCurrent->rt_anNetMask, 1 );
			FORMAT_IP( psCurrent->rt_anGatewayAddr, 2 );

			if ( psCurrent->rt_psInterface != NULL )
				pzName = psCurrent->rt_psInterface->ni_zName;
			else
				pzName = NO_NAME;

			dbprintf( DBP_DEBUGGER, "%c-+ Reach %s/%s (%d) via %s; interface: %s.\n", ( psCurrent->rt_psNext == &g_sSentinel ) ? '`' : '|', FORMATTED_IP( 0 ), FORMATTED_IP( 1 ), psCurrent->rt_nMaskBits, FORMATTED_IP( 2 ), pzName );
		}

		dbprintf( DBP_DEBUGGER, "\n" );
	}
}

/**
 * \par Description:
 * Deletes all cloned routes based on psInterface.
 */
void rt_release_interface( NetInterface_s *psInterface )
{
	Route_s **ppsRoute;

	if ( psInterface == NULL )
		return;

	if ( RWL_LOCK_RW( g_hRouteListMutex ) < 0 )
	{
		kerndbg( KERN_FATAL, "rt_release_interface(): Unable to lock route list.\n" );

		return;
	}

	/* Scan cloned route list for routes to delete */
	for ( ppsRoute = &g_psClonedRoutes; ( *ppsRoute ) != &g_sSentinel; ppsRoute = &( *ppsRoute )->rt_psNext )
	{
		if ( ( *ppsRoute )->rt_psInterface == psInterface )
		{
			Route_s *psRemove = ( *ppsRoute );

			/* Unlink */
			( *ppsRoute ) = psRemove->rt_psNext;

			/* Convert anonymous list reference to local thread reference */
			if ( rwl_convert_from_anon( psRemove->rt_hMutex ) < 0 )
			{
				kerndbg( KERN_DEBUG, __FUNCTION__ "(): Conversion from anonymous mutex failed (1).\n" );
			}

			/* Mark for deletion */
			psRemove->rt_psNext = NULL;

			/* Release */
			ip_release_route( psRemove );
		}
	}

	/* Scan cloned route list for routes to delete */
	for ( ppsRoute = &g_psDeviceRoutes; ( *ppsRoute ) != &g_sSentinel; ppsRoute = &( *ppsRoute )->rt_psNext )
	{
		if ( ( *ppsRoute )->rt_psInterface == psInterface )
		{
			Route_s *psRemove = ( *ppsRoute );

			/* Unlink */
			( *ppsRoute ) = psRemove->rt_psNext;

			/* Convert anonymous list reference to local thread reference */
			if ( rwl_convert_from_anon( psRemove->rt_hMutex ) < 0 )
			{
				kerndbg( KERN_DEBUG, __FUNCTION__ "(): Conversion from anonymous mutex failed (2).\n" );
			}

			/* Mark for deletion */
			psRemove->rt_psNext = NULL;

			/* Release */
			ip_release_route( psRemove );
		}
	}

	RWL_UNLOCK_RW( g_hRouteListMutex );
}
