
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

#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/socket.h>
#include <atheos/semaphore.h>

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




static Route_s *g_psFirstRoute = NULL;
static Route_s *g_psDefaultRoute = NULL;


static inline bool compare_net_address( ipaddr_t anAddr1, ipaddr_t anAddr2, ipaddr_t anMask )
{
	ipaddr_t anMasked1 = { anAddr1[0] & anMask[0], anAddr1[1] & anMask[1], anAddr1[2] & anMask[2], anAddr1[3] & anMask[3] };
	ipaddr_t anMasked2 = { anAddr2[0] & anMask[0], anAddr2[1] & anMask[1], anAddr2[2] & anMask[2], anAddr2[3] & anMask[3] };

	return ( IP_SAMEADDR( anMasked1, anMasked2 ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

Route_s *ip_find_route( ipaddr_t pDstAddr )
{
	Route_s *psRoute;
	int i;

	for ( psRoute = g_psFirstRoute; psRoute != NULL; psRoute = psRoute->rt_psNext )
	{
		ipaddr_t nDstMasked;
		ipaddr_t nRtMasked;

		for ( i = 0; i < 4; ++i )
		{
			nDstMasked[i] = pDstAddr[i] & psRoute->rt_anNetMask[i];
			nRtMasked[i] = psRoute->rt_anNetAddr[i] & psRoute->rt_anNetMask[i];
		}

		if ( IP_SAMEADDR( nDstMasked, nRtMasked ) )
		{
			atomic_add( &psRoute->rt_nRefCount, 1 );
			atomic_add( &psRoute->rt_nUseCount, 1 );
			return ( psRoute );
		}
	}
	if ( g_psDefaultRoute != NULL )
	{
		atomic_add( &g_psDefaultRoute->rt_nRefCount, 1 );
		atomic_add( &g_psDefaultRoute->rt_nUseCount, 1 );
	}
	return ( g_psDefaultRoute );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void ip_release_route( Route_s *psRoute )
{
	atomic_add( &psRoute->rt_nRefCount, -1 );
	kassertw( psRoute->rt_nRefCount >= 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

Route_s *add_route( ipaddr_t pNetAddr, ipaddr_t pNetMask, ipaddr_t pGateway, int nMetric, NetInterface_s *psInterface )
{
	Route_s *psRoute;

	psRoute = kmalloc( sizeof( Route_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psRoute == NULL )
	{
		printk( "Error: add_route() out of memory\n" );
		return ( NULL );
	}
	psRoute->rt_nRefCount = 1;

	IP_COPYADDR( psRoute->rt_anNetAddr, pNetAddr );
	IP_COPYADDR( psRoute->rt_anNetMask, pNetMask );
	IP_COPYADDR( psRoute->rt_anGatewayAddr, pGateway );
	psRoute->rt_psInterface = psInterface;
	psRoute->rt_nMetric = nMetric;

	if ( nMetric != 0 )
	{
		g_psDefaultRoute = psRoute;
	}
	else
	{
		psRoute->rt_psNext = g_psFirstRoute;
		g_psFirstRoute = psRoute;
	}
	return ( psRoute );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static void convert_rtentry( struct rtabentry *psDst, Route_s *psSrc )
{
	memset( psDst, 0, sizeof( *psDst ) );
	psDst->rt_flags = psSrc->rt_nFlags;
	psDst->rt_metric = psSrc->rt_nMetric;

	memcpy( ( ( struct sockaddr_in * )&psDst->rt_dst )->sin_addr, psSrc->rt_anNetAddr, 4 );
	memcpy( ( ( struct sockaddr_in * )&psDst->rt_genmask )->sin_addr, psSrc->rt_anNetMask, 4 );
	memcpy( ( ( struct sockaddr_in * )&psDst->rt_gateway )->sin_addr, psSrc->rt_anGatewayAddr, 4 );
	strcpy( psDst->rt_dev, psSrc->rt_psInterface->ni_zName );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int get_route_table( struct rttable *psTable )
{
	struct rtabentry *psRTTable = ( struct rtabentry * )( psTable + 1 );
	Route_s *psRoute;
	int nCount = 0;
	int nRealCount = 0;
	int nMaxCount;

	if ( memcpy_from_user( &nMaxCount, &psTable->rtt_count, sizeof( nMaxCount ) ) < 0 )
	{
		return ( -EFAULT );
	}

	if ( g_psDefaultRoute != NULL && nMaxCount > 0 )
	{
		struct rtabentry sREntry;

		convert_rtentry( &sREntry, g_psDefaultRoute );
		if ( memcpy_to_user( psRTTable, &sREntry, sizeof( sREntry ) ) < 0 )
		{
			return ( -EFAULT );
		}
		psRTTable++;
		nRealCount++;
	}
	for ( psRoute = g_psFirstRoute; psRoute != NULL; psRoute = psRoute->rt_psNext )
	{
		nCount++;
		if ( nRealCount < nMaxCount )
		{
			struct rtabentry sREntry;

			convert_rtentry( &sREntry, psRoute );
			if ( memcpy_to_user( psRTTable, &sREntry, sizeof( sREntry ) ) < 0 )
			{
				nCount = -EFAULT;
				break;
			}
			psRTTable++;
			nRealCount++;
		}
	}
	if ( memcpy_to_user( &psTable->rtt_count, &nRealCount, sizeof( nRealCount ) ) < 0 )
	{
		return ( -EFAULT );
	}
	return ( nCount );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int add_route_entry( struct rtentry *psREntry )
{
	NetInterface_s *psInterface;
	Route_s *psRoute;
	struct sockaddr_in *psDestAddr = ( struct sockaddr_in * )&psREntry->rt_dst;
	struct sockaddr_in *psNetMask = ( struct sockaddr_in * )&psREntry->rt_genmask;

	if ( psREntry->rt_dev != NULL )
	{
		psInterface = find_interface( psREntry->rt_dev );
	}
	else
	{
		psInterface = find_interface_by_addr( psDestAddr->sin_addr );
	}
	if ( psInterface == NULL )
	{
		printk( "Error: add_route() could not find interface\n" );
		return ( -EINVAL );
	}

	psRoute = kmalloc( sizeof( Route_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psRoute == NULL )
	{
		printk( "Error: add_route() out of memory\n" );
		return ( -ENOMEM );
	}

	IP_COPYADDR( psRoute->rt_anNetAddr, psDestAddr->sin_addr );
	IP_COPYADDR( psRoute->rt_anNetMask, psNetMask->sin_addr );

	psRoute->rt_nFlags = psREntry->rt_flags;

	if ( psREntry->rt_flags & RTF_GATEWAY )
	{
		IP_COPYADDR( psRoute->rt_anGatewayAddr, ( ( struct sockaddr_in * )&psREntry->rt_gateway )->sin_addr );
		if ( g_psDefaultRoute != NULL )
		{
			ip_release_route( g_psDefaultRoute );
		}
		g_psDefaultRoute = psRoute;
		psRoute->rt_nRefCount++;
	}
	else
	{
		psRoute->rt_nRefCount++;
		psRoute->rt_psNext = g_psFirstRoute;
		g_psFirstRoute = psRoute;
	}
	psRoute->rt_psInterface = psInterface;
	psRoute->rt_nMetric = psREntry->rt_metric;
	return ( 0 );
}

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int delete_route_entry( struct rtentry *psREntry )
{
	NetInterface_s *psInterface;
	struct sockaddr_in *psDestAddr = ( struct sockaddr_in * )&psREntry->rt_dst;
	struct sockaddr_in *psNetMask = ( struct sockaddr_in * )&psREntry->rt_genmask;
	Route_s *psRoute;
	Route_s **ppsTmp;

	if ( psREntry->rt_dev != NULL )
	{
		psInterface = find_interface( psREntry->rt_dev );
	}
	else
	{
		psInterface = find_interface_by_addr( psDestAddr->sin_addr );
	}
	if ( psInterface == NULL )
	{
		printk( "Error: delete_route_entry() could not find interface\n" );
		return ( -EINVAL );
	}

	if ( g_psDefaultRoute != NULL && IP_SAMEADDR( g_psDefaultRoute->rt_anNetAddr, psDestAddr->sin_addr ) && IP_SAMEADDR( g_psDefaultRoute->rt_anNetMask, psNetMask->sin_addr ) )
	{
		ip_release_route( g_psDefaultRoute );
		g_psDefaultRoute = NULL;
		return ( 0 );
	}

	psRoute = NULL;
	for ( ppsTmp = &g_psFirstRoute; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->rt_psNext )
	{
		if ( IP_SAMEADDR( ( *ppsTmp )->rt_anNetAddr, psDestAddr->sin_addr ) && IP_SAMEADDR( ( *ppsTmp )->rt_anNetMask, psNetMask->sin_addr ) )
		{
			psRoute = *ppsTmp;
			*ppsTmp = psRoute->rt_psNext;
			break;
		}
	}
	if ( psRoute == NULL )
	{
		printk( "Error: delete_route_entry() could not find route\n" );
		return ( -ENOENT );
	}
	ip_release_route( psRoute );
//    kfree( psRoute );
	return ( 0 );
}


void rt_interface_mask_changed( NetInterface_s *psInterface, ipaddr_t anNewMask )
{
	Route_s *psRoute;

	for ( psRoute = g_psFirstRoute; psRoute != NULL; psRoute = psRoute->rt_psNext )
	{
		if ( psRoute->rt_psInterface != psInterface || psRoute->rt_nFlags & RTF_HOST )
		{
			continue;
		}
		if ( compare_net_address( psRoute->rt_anNetAddr, psInterface->ni_anIpAddr, psInterface->ni_anSubNetMask ) == false )
		{
			continue;
		}
		memcpy( psRoute->rt_anNetMask, anNewMask, 4 );
		break;
	}
}

void rt_interface_address_changed( NetInterface_s *psInterface, ipaddr_t anNewAddress )
{
	Route_s *psRoute;

	for ( psRoute = g_psFirstRoute; psRoute != NULL; psRoute = psRoute->rt_psNext )
	{
		if ( psRoute->rt_psInterface != psInterface || psRoute->rt_nFlags & RTF_HOST )
		{
			continue;
		}
		if ( compare_net_address( psRoute->rt_anNetAddr, psInterface->ni_anIpAddr, psInterface->ni_anSubNetMask ) == false )
		{
			continue;
		}
		memcpy( psRoute->rt_anNetAddr, anNewAddress, 4 );
		break;
	}
}
