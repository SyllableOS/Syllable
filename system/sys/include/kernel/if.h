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

#ifndef __F_KERNEL_IF_H__
#define __F_KERNEL_IF_H__

#include <net/nettypes.h>
#include <net/if.h>

#include <kernel/route.h>
#include <kernel/if_ether.h>
#include <kernel/packet.h>

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address
 * families, are allocated and attached when an address is set,
 * and are linked together so all addresses for an interface can
 * be located.
 */
 
struct ifaddr 
{
	struct sockaddr	ifa_addr;	/* address of interface		*/
	union {
		struct sockaddr	ifu_broadaddr;
		struct sockaddr	ifu_dstaddr;
	} ifa_ifu;
	struct iface		*ifa_ifp;	/* back-pointer to interface	*/
	struct ifaddr		*ifa_next;	/* next address for interface	*/
};

#define	ifa_broadaddr	ifa_ifu.ifu_broadaddr	/* broadcast address	*/
#define	ifa_dstaddr		ifa_ifu.ifu_dstaddr		/* other end of link	*/

/**
 * Network interface drivers need to override the following routines:
 * All these routines execute with the kernel interface locked, so
 * drivers may store and use a pointer to the interface without
 * locking it -- if they do leave it locked, others will not be
 * able to change the interface.
 */
/* Device cleanup routine */
typedef void ni_deinit( void* pInterface );

/* Write packet to destination using the specified route */
typedef int ni_write( Route_s* psRoute, ipaddr_t pDstAddress,
                      PacketBuf_s* psBuf );
/* Handle device ioctl (those not handled by the kernel) */
typedef int ni_ioctl( void* pInterface, int nCmd, void* pArg );

/* Receive notification of a kernel-handled ioctl */
typedef void ni_notify( void* pInterface );

/* Request to release references to the kernel interface */
typedef void ni_release( void* pInterface );

typedef struct
{
    ni_write*   ni_write;
    ni_ioctl*   ni_ioctl;
    ni_deinit*  ni_deinit;
    ni_notify*  ni_notify;
		ni_release* ni_release;
} NetInterfaceOps_s;

/* Interface driver initialisation function prototype */
typedef NetInterfaceOps_s* ni_init( void );

/* Kernel interface structure definition */
struct _NetInterface
{
	/* ni_nIndex is locked by interface list mutex (g_hIfMutex). */
	int                 ni_nIndex;              /* List index pointer. */

	/* These are read-only outside add_net_interface() (g_hIfMutex) */
	char                ni_zName[128];          /* Interface name */
	NetInterfaceOps_s*  ni_psOps;               /* Driver dispatch table */
	void*               ni_pInterface;          /* Driver data */

	/* The following are locked by interface instance mutex (ni_hMutex). */
	sem_id              ni_hMutex;              /* Interface R/W lock */
	ipaddr_t            ni_anIpAddr;            /* Configured IP Address*/
	ipaddr_t            ni_anSubNetMask;        /* Configured IP Netmask */
	ipaddr_t            ni_anExtraAddr;         /* Peer/Broadcast Address*/
	int                 ni_nMTU;                /* Max Transmissible Unit */
	uint32              ni_nFlags;              /* Interface flags */
};

/**
 * Interfaces
 */
/* Reference counting */
/* By acquiring an interface, you have read-only access to its members */
int acquire_net_interface( NetInterface_s* psInterface );
int release_net_interface( NetInterface_s* psInterface );

/* Use anonymous acquire/release for passing between threads */
int acquire_net_if_anon( NetInterface_s* psInterface );
int release_net_if_anon( NetInterface_s* psInterface );

/* Interface list management */
int add_net_interface( const char *pzName, NetInterfaceOps_s* psOps,
                       void *pDriverData );
int del_net_interface( NetInterface_s* psInterface );

NetInterface_s* get_net_interface( int nIndex );
NetInterface_s* find_interface( const char* pzName );
NetInterface_s* find_interface_by_addr( ipaddr_p anAddress );

/* Inspect an interface */
/* Called for an interface index */
int get_interface_address( int nIndex, ipaddr_p anAddress );
int get_interface_destination( int nIndex, ipaddr_p anAddress );
int get_interface_broadcast( int nIndex, ipaddr_p anAddress );
int get_interface_netmask( int nIndex, ipaddr_p anAddress );
int get_interface_mtu( int nIndex, int *pnMTU );
int get_interface_flags( int nIndex, uint32 *pnFlags );

/* Interface changes */
/* Should be called on an acquired interface pointer */
int set_interface_address( NetInterface_s* psIf, ipaddr_p anAddress );
int set_interface_destination( NetInterface_s* psIf, ipaddr_p anAddress );
int set_interface_broadcast( NetInterface_s* psIf, ipaddr_p anAddress );
int set_interface_netmask( NetInterface_s* psIf, ipaddr_p anNetmask );
int set_interface_mtu( NetInterface_s* psIf, int nMTU );
int set_interface_flags( NetInterface_s* psIf, uint32 nFlags );

/* Kernel-only routines */
int netif_ioctl( int nCmd, void* pBuf );
void load_interface_drivers(void);
void init_net_interfaces(void);

#endif	/* __F_KERNEL_IF_H__ */
