/*
 *		Global definitions for the INET interface module.
 *
 * Version:	@(#)if.h	1.0.2	04/18/93
 *
 * Authors:	Original taken from Berkeley UNIX 4.3, (c) UCB 1982-1988
 *		Ross Biro, <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef __F_NET_IF_H__
#define __F_NET_IF_H__

#include <atheos/types.h>		/* for "__kernel_caddr_t" et al	*/
#include <atheos/socket.h>		/* for "struct sockaddr" et al	*/

#ifdef __KERNEL__
#include <net/nettypes.h>
#include <net/route.h>
#include <net/if_ether.h>
#include <net/packet.h>
#endif

/* Standard interface flags. */
#define	IFF_UP		0x1		/* interface is up		*/
#define	IFF_BROADCAST	0x2		/* broadcast address valid	*/
#define	IFF_DEBUG	0x4		/* turn on debugging		*/
#define	IFF_LOOPBACK	0x8		/* is a loopback net		*/
#define	IFF_POINTOPOINT	0x10		/* interface is has p-p link	*/
#define	IFF_NOTRAILERS	0x20		/* avoid use of trailers	*/
#define	IFF_RUNNING	0x40		/* resources allocated		*/
#define	IFF_NOARP	0x80		/* no ARP protocol		*/
#define	IFF_PROMISC	0x100		/* receive all packets		*/
#define	IFF_ALLMULTI	0x200		/* receive all multicast packets*/

#define IFF_MASTER	0x400		/* master of a load balancer 	*/
#define IFF_SLAVE	0x800		/* slave of a load balancer	*/

#define IFF_MULTICAST	0x1000		/* Supports multicast		*/

#define IFF_VOLATILE	(IFF_LOOPBACK|IFF_POINTOPOINT|IFF_BROADCAST|IFF_ALLMULTI)

#define IFF_PORTSEL	0x2000          /* can set media type		*/
#define IFF_AUTOMEDIA	0x4000		/* auto media select active	*/
#define IFF_DYNAMIC	0x8000		/* dialup device with changing addresses*/

#ifdef __KERNEL__


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
#define	ifa_dstaddr	ifa_ifu.ifu_dstaddr	/* other end of link	*/

#endif /* __KERNEL__ */ 

/*
 *	Device mapping structure. I'd just gone off and designed a 
 *	beautiful scheme using only loadable modules with arguments
 *	for driver options and along come the PCMCIA people 8)
 *
 *	Ah well. The get() side of this is good for WDSETUP, and it'll
 *	be handy for debugging things. The set side is fine for now and
 *	being very small might be worth keeping for clean configuration.
 */
#if 0
struct ifmap 
{
	unsigned long mem_start;
	unsigned long mem_end;
	unsigned short base_addr; 
	unsigned char irq;
	unsigned char dma;
	unsigned char port;
	/* 3 bytes spare */
};
#endif

/*
 * Interface request structure used for socket
 * ioctl's.  All interface ioctl's must have parameter
 * definitions which begin with ifr_name.  The
 * remainder may be interface specific.
 */

struct ifreq 
{
#define IFHWADDRLEN	6
#define	IFNAMSIZ	16
    union
    {
	char	ifrn_name[IFNAMSIZ];		/* if name, e.g. "en0" */
    } ifr_ifrn;
	
    union {
	struct	sockaddr ifru_addr;
	struct	sockaddr ifru_dstaddr;
	struct	sockaddr ifru_broadaddr;
	struct	sockaddr ifru_netmask;
	struct  sockaddr ifru_hwaddr;
	short	ifru_flags;

	int	ifru_ivalue;
	int	ifru_mtu;
//	struct  ifmap ifru_map;
	char	ifru_slave[IFNAMSIZ];	/* Just fits the size */
	char	ifru_newname[IFNAMSIZ];
	char *	ifru_data;
    } ifr_ifru;
};

#define ifr_name	ifr_ifrn.ifrn_name	/* interface name 	*/
#define ifr_hwaddr	ifr_ifru.ifru_hwaddr	/* MAC address 		*/
#define	ifr_addr	ifr_ifru.ifru_addr	/* address		*/
#define	ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-p lnk	*/
#define	ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address	*/
#define	ifr_netmask	ifr_ifru.ifru_netmask	/* interface net mask	*/
#define	ifr_flags	ifr_ifru.ifru_flags	/* flags		*/
#define	ifr_metric	ifr_ifru.ifru_ivalue	/* metric		*/
#define	ifr_mtu		ifr_ifru.ifru_mtu	/* mtu			*/
#define ifr_map		ifr_ifru.ifru_map	/* device map		*/
#define ifr_slave	ifr_ifru.ifru_slave	/* slave device		*/
#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface	*/
#define ifr_ifindex	ifr_ifru.ifru_ivalue	/* interface index	*/
#define ifr_bandwidth	ifr_ifru.ifru_ivalue    /* link bandwidth	*/
#define ifr_qlen	ifr_ifru.ifru_ivalue	/* Queue length 	*/
#define ifr_newname	ifr_ifru.ifru_newname	/* New name		*/

/*
 * Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */

struct ifconf 
{
    int	ifc_len;			/* size of buffer	*/
    union 
    {
	char*		ifcu_buf;
	struct	ifreq*	ifcu_req;
    } ifc_ifcu;
};
#define	ifc_buf	ifc_ifcu.ifcu_buf		/* buffer address	*/
#define	ifc_req	ifc_ifcu.ifcu_req		/* array of structures	*/


#ifdef __KERNEL__

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

#endif /* __KERNEL__ */

#endif /* __F_NET_IF_H__ */
