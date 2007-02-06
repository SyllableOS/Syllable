/*
 *	Syllable Kernel
 *	net/interface.c
 *
 *	The Syllable Kernel is derived from the AtheOS kernel
 *	Copyright (C) 1999 - 2001 Kurt Skauen
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
#include <atheos/image.h>

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

#include <macros.h>

#include "inc/semaphore.h"

/* Selective debugging level overrides */
#ifdef KERNEL_DEBUG_NET
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET
#endif

#ifdef KERNEL_DEBUG_NET_INTERFACE
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERNEL_DEBUG_NET_INTERFACE
#endif

/**
 * Both the interface list and the individual interfaces may be locked.
 * The locking protocols are explained in the comments above each function.
 * This locking is necessary when interfaces can be added or deleted after
 * system initialisation.
 */
static NetInterface_s *g_asNetInterfaces[MAX_NET_INTERFACES];
static int g_nInterfaceCount = 0;
static sem_id g_hIfListMutex = -1, g_hTryReleaseMutex = -1;

/**
 * Interface Driver list.
 * 
 * This list is not locked: it is created at startup and is read-only
 * thereafter.
 */
typedef struct _InterfaceDriver InterfaceDriver_s;
static struct _InterfaceDriver
{
	InterfaceDriver_s *id_psNext;
	image_id id_hDriver;
	const NetInterfaceOps_s *id_psOps;
} *g_psFirstInterfaceDriver = NULL;


/**
 * On Intel, there's no problem with misaligned words so the following
 * optimisation makes sense (and is used elsewhere).
 */
static inline bool compare_net_address( ipaddr_p anAddr1, ipaddr_p anAddr2, ipaddr_p anMask )
{
	return ( ( *( uint32 * )anAddr1 ) & ( *( uint32 * )anMask ) ) == ( ( *( uint32 * )anAddr2 ) & ( *( uint32 * )anMask ) );
}

/**
 * \par Description:
 * Acquires the network interface specified, increasing its reference count.
 * 
 * A corresponding call to release_network_interface() is required whenever
 * this is called.
 *
 * \return Returns zero on success, or a negated errno error code if an
 * error occurs.
 */
int acquire_net_interface( NetInterface_s *psInterface )
{
	int nError;

	if ( psInterface == NULL )
		return -EINVAL;

	if ( ( nError = RWL_LOCK_RO( psInterface->ni_hMutex ) ) < 0 )
		return nError;

	return 0;
}

/**
 * \par Description:
 * Releases the network interface pointer specified, decreasing its reference
 * count.
 * 
 * If an interface is no longer in the interface list, and its reference count
 * reaches zero, the interface is deallocated and the memory freed.
 *
 * In order for this to be safely re-entrant, all code from checking if
 * an interface can be released to releasing the interface must be
 * synchronised.	This is managed using g_hTryReleaseMutex.
 *
 * \return Returns zero if successful, or a negated errno error code if
 * an error occurs.
 */
int release_net_interface( NetInterface_s *psInterface )
{
	int nError, nMayDelete = 0;

	if ( psInterface == NULL )
		return -EINVAL;

	/* Lock the list so we can check if we should try to delete this interface,
	 * without anyone being able to add the interface behind our backs.
	 **/
	if ( ( nError = RWL_LOCK_RO( g_hIfListMutex ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "release_net_interface(): Unable to lock interface list!\n" );

		return nError;
	}

	nMayDelete = ( psInterface->ni_nIndex < 0 );

	/* Unlock the list again */
	RWL_UNLOCK_RO( g_hIfListMutex );

	/* If the mutex may be deleted, we will now try to delete it.
	 * Note that this function will always be called at least once after
	 * the interface is removed from the list, so there is not a race here.
	 */
	if ( nMayDelete )
	{
		/* Attempts to free the interface's memory must not occur in parallel.
		 * Lock the g_hTryReleaseMutex to enforce the critical section.
		 */
		if ( ( nError = LOCK( g_hTryReleaseMutex ) ) < 0 )
		{
			kerndbg( KERN_FATAL, "release_net_interface(): Unable to lock the try-release " "mutex!\n" );

			return nError;
		}

		/* If we are able to upgrade to a write lock without waiting, we're the
		 * only ones reading.   So it's time to deallocate.
		 */
		if ( ( nError = RWL_TRY_UPGRADE( psInterface->ni_hMutex ) ) < 0 )
		{
			if ( nError == -ETIME )
			{
				/* We are not the only ones interested in this. */
				nError = 0;

				/* Release our lock (i.e. decrease reference count) */
				RWL_UNLOCK_RO( psInterface->ni_hMutex );
			}
		}
		else
		{
			/* Make sure the lock was not recursive */
			if ( rwl_count_all_readers( psInterface->ni_hMutex ) == 1 && rwl_count_all_writers( psInterface->ni_hMutex ) == 1 )
			{
				/* Obtained exclusive lock, this means we must free the interface. */
				/* Signal to the interface driver */
				if ( psInterface->ni_psOps->ni_deinit !=NULL )
					psInterface->ni_psOps->ni_deinit ( psInterface->ni_pInterface );

				/* Delete the interface semaphore (unblocks other waiters) */
				delete_semaphore( psInterface->ni_hMutex );

				/* Free the memory */
				kfree( psInterface );
			}
			else
			{
				/* Release our read-write lock (i.e. decrease reference count) */
				kerndbg( KERN_DEBUG, __FUNCTION__ "(): Recursive locking snafu.\n" );
				RWL_UNLOCK_RW( psInterface->ni_hMutex );
				RWL_UNLOCK_RO( psInterface->ni_hMutex );
			}

			nError = 0;
		}

		/* Unlock the try-release mutex */
		UNLOCK( g_hTryReleaseMutex );
	}
	else
	{
		/* Release our lock (i.e. decrease reference count) */
		RWL_UNLOCK_RO( psInterface->ni_hMutex );
	}

	return nError;
}

/** 
 * \par Description:
 * Responds to the SIOCGIFCONF socket ioctl by preparing a list of available
 * network interfaces.
 * 
 * \param	psConf			 - userland pointer to struct ifconf that will hold
 *												the output
 * \par Note:
 * This function expects to be called from netif_ioctl().
 *
 * \return Returns zero if successful, or a negated errno error code if an
 * error occurs.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 */
static int get_interface_list( struct ifconf *psConf )
{
	int i;
	int nBufSize;
	int nBytesCopyed = 0;
	int nError;
	char *pBuffer;

	kassertw( sizeof( psConf->ifc_len ) == sizeof( int ) );
	
	nError = memcpy_from_user( &nBufSize, &psConf->ifc_len, sizeof( int ) );
	if ( nError < 0 )
		return ( nError );

	nError = memcpy_from_user( &pBuffer, &psConf->ifc_buf, sizeof( pBuffer ) );
	if ( nError < 0 )
		return ( nError );
		
	if( pBuffer == NULL )
		return( -EINVAL );

	if ( ( nError = RWL_LOCK_RO( g_hIfListMutex ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "get_interface_list(): Unable to lock interface list.\n" );
		return nError;
	}

	for ( i = 0; i < MAX_NET_INTERFACES; ++i )
	{
		struct ifreq sIfConf;
		struct sockaddr_in *psAddr = ( struct sockaddr_in * )&sIfConf.ifr_addr;
		int nCurLen = min( sizeof( sIfConf ), nBufSize );
		NetInterface_s *psInterface;

		if ( g_asNetInterfaces[i] == NULL )
			continue;

		if ( nCurLen == 0 )
			break;

		/* Acquire a pointer to the interface */
		psInterface = get_net_interface( i );

		psAddr->sin_family = AF_INET;
		psAddr->sin_port = 0;

		strcpy( sIfConf.ifr_name, psInterface->ni_zName );
		memcpy( psAddr->sin_addr, psInterface->ni_anIpAddr, 4 );

		nError = memcpy_to_user( pBuffer, &sIfConf, nCurLen );
		pBuffer += nCurLen;

		/* Release interface pointer */
		release_net_interface( psInterface );

		if ( nError < 0 )
			break;

		nBytesCopyed += nCurLen;
	}

	RWL_UNLOCK_RO( g_hIfListMutex );

	if ( nError > -1 )
		nError = memcpy_to_user( &psConf->ifc_len, &nBytesCopyed, sizeof( int ) );

	return nError;
}

/** 
 * \par Description:
 * Adds a new network interface to the interface list and returns its index.
 * If a pointer to the new network interface is required, use
 * get_net_interface().
 * 
 * \return Returns the interface index if successful, or a negated errno 
 * error code if unsuccessful.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 **/
int add_net_interface( const char *pzName, NetInterfaceOps_s * psOps, void *pDriverData )
{
	int i, nResult = -1;
	int nError;
	NetInterface_s *psIf;

	if ( ( nError = RWL_LOCK_RW( g_hIfListMutex ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "add_net_interface(): Unable to lock interface list!\n" );

		return nError;
	}

	nError = 0;

	for ( i = 0; i < MAX_NET_INTERFACES; ++i )
	{
		if ( g_asNetInterfaces[i] == NULL )
		{
			/* Found a free slot */
			if ( nResult < 0 )
				nResult = i;
		}
		else
		{
			/* Check for duplicate names */
			if ( strcmp( g_asNetInterfaces[i]->ni_zName, pzName ) == 0 )
			{
				nError = -EEXIST;
				break;
			}
		}
	}

	if ( nError == 0 && nResult < 0 )
		nError = -EMFILE;	/* No free slots */

	if ( nError != 0 )
		goto exit;

	/* Create a new interface */
	psIf = kmalloc( sizeof( NetInterface_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psIf == NULL )
	{
		kerndbg( KERN_DEBUG, "add_net_interface(): Ran out of memory.\n" );

		nError = -ENOMEM;
		goto exit;
	}

	/* Create the interface lock */
	psIf->ni_hMutex = CREATE_RWLOCK( "interface instance mutex" );
	if ( ( psIf->ni_hMutex & SEMTYPE_KERNEL ) == 0 )
	{
		kerndbg( KERN_DEBUG, __FUNCTION__ "(): Created non-kernel semaphore!\n" );
	}

	if ( psIf->ni_hMutex < 0 )
	{
		kerndbg( KERN_DEBUG, "add_net_interface(): Unable to create instance mutex.\n" );

		nError = psIf->ni_hMutex;
		kfree( psIf );

		goto exit;
	}


	/* Copy in the name and driver details */
	strncpy( psIf->ni_zName, pzName, sizeof( psIf->ni_zName ) );
	psIf->ni_zName[sizeof( psIf->ni_zName ) - 1] = 0;

	psIf->ni_psOps = psOps;
	psIf->ni_pInterface = pDriverData;


	/* Assign the new interface to the slot */
	g_asNetInterfaces[nResult] = psIf;
	psIf->ni_nIndex = nResult;

	++g_nInterfaceCount;

      exit:
	RWL_UNLOCK_RW( g_hIfListMutex );

	return nError == 0 ? nResult : nError;
}

/** 
 * \par Description:
 * Removes the specified network interface.	The interface pointer passed
 * in is assumed to have been obtained using get_net_interface() or some
 * other function that increments the interfaces reference count.	The
 * interface will be marked for deletion and the pointer that is passed in
 * is released.
 * 
 * \par Warning:
 * After marking the interface deleted, this function will call
 * release_net_interface() automatically: do not call it again in the
 * calling code.
 * 
 * \return Returns zero if successful, or a negated errno error code if
 * unsuccessful.
 * \author	William Rose (wdrose@users.sourceforge.net)
 *****************************************************************************/
int del_net_interface( NetInterface_s *psIf )
{
	int nError;

	if ( ( nError = RWL_LOCK_RW( g_hIfListMutex ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "del_net_interface(): Unable to lock interface list mutex!\n" );

		return nError;
	}


	if ( psIf->ni_nIndex < 0 || psIf->ni_nIndex > MAX_NET_INTERFACES )
	{
		nError = -ENOENT;
	}
	else if ( g_asNetInterfaces[psIf->ni_nIndex] != psIf )
	{
		kerndbg( KERN_WARNING, "del_net_interface(): Interface index disagrees with list.\n" );

		psIf->ni_nIndex = -1;
		nError = -ENOENT;
	}
	else
	{
		g_asNetInterfaces[psIf->ni_nIndex] = NULL;
		psIf->ni_nIndex = -1;
		--g_nInterfaceCount;
		nError = 0;
	}

	RWL_UNLOCK_RW( g_hIfListMutex );

	if ( nError == 0 )
	{
		rt_release_interface( psIf );

		nError = release_net_interface( psIf );
	}

	return nError;
}

/** 
 * \par Description:
 * Retrieves the network interface at the indicated index, incrementing its
 * reference count.	If the index provided does not refer to an interface,
 * NULL is returned.
 * 
 * If an interface is returned, remember to use release_net_interface() to
 * release the pointer after use.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

NetInterface_s *get_net_interface( int nIndex )
{
	NetInterface_s *psInterface = NULL;

	if ( nIndex < 0 || nIndex >= MAX_NET_INTERFACES )
		return ( NULL );

	if ( RWL_LOCK_RO( g_hIfListMutex ) < 0 )
	{
		kerndbg( KERN_FATAL, "get_net_interface(): Unable to lock interface list.\n" );

		return NULL;
	}

	if ( g_asNetInterfaces[nIndex] != NULL )
	{
		if ( acquire_net_interface( g_asNetInterfaces[nIndex] ) < 0 )
		{
			kerndbg( KERN_FATAL, "get_net_interface(): Unable to acquire interface.\n" );
		}
		else
		{
			psInterface = g_asNetInterfaces[nIndex];
		}
	}

	RWL_UNLOCK_RO( g_hIfListMutex );

	return psInterface;
}

/** 
 * \par Description:
 * Retrieves the network interface with the specified name, incrementing its
 * reference count.	If there is no matching interface, NULL is returned.
 * 
 * If an interface is returned, remember to use release_net_interface() to
 * release the pointer after use.
 * \author	Kurt Skauen (kurt@atheos.cx)
 */
NetInterface_s *find_interface( const char *pzName )
{
	int i;
	NetInterface_s *psInterface = NULL;

	if ( RWL_LOCK_RO( g_hIfListMutex ) < 0 )
	{
		kerndbg( KERN_FATAL, "find_interface(): Unable to lock interface list.\n" );

		return NULL;
	}


	for ( i = 0; i < MAX_NET_INTERFACES; ++i )
	{
		if ( g_asNetInterfaces[i] == NULL )
			continue;

		if ( strcmp( g_asNetInterfaces[i]->ni_zName, pzName ) == 0 )
		{
			if ( acquire_net_interface( g_asNetInterfaces[i] ) < 0 )
			{
				kerndbg( KERN_FATAL, "find_interface(): Unable to acquire interface.\n" );
			}
			else
			{
				psInterface = g_asNetInterfaces[i];
			}
			break;
		}
	}

	RWL_UNLOCK_RO( g_hIfListMutex );

	return psInterface;
}

/** 
 * \par Description:
 * Retrieves the network interface that can directly reach the specified IP 
 * address and increments its reference count.	If there is no matching
 * interface, NULL is returned.
 * 
 * If an interface is returned, remember to use release_net_interface() to
 * release the pointer after use.
 * \author	Kurt Skauen (kurt@atheos.cx)
 */
NetInterface_s *find_interface_by_addr( ipaddr_p anAddress )
{
	int i;
	NetInterface_s *psInterface = NULL;

	if ( RWL_LOCK_RO( g_hIfListMutex ) < 0 )
	{
		kerndbg( KERN_FATAL, "find_interface_by_addr(): Unable to lock interface list.\n" );

		return NULL;
	}

	for ( i = 0; i < MAX_NET_INTERFACES; ++i )
	{
		if ( g_asNetInterfaces[i] == NULL )
			continue;

		if ( compare_net_address( g_asNetInterfaces[i]->ni_anIpAddr, anAddress, g_asNetInterfaces[i]->ni_anSubNetMask ) )
		{
			if ( acquire_net_interface( g_asNetInterfaces[i] ) < 0 )
			{
				kerndbg( KERN_FATAL, "find_interface_by_addr(): " "Unable to acquire interface.\n" );
			}
			else
			{
				psInterface = g_asNetInterfaces[i];

				if ( psInterface != NULL && ( psInterface->ni_nFlags & IFF_UP ) == 0 )
				{
					/* Don't return downed interfaces */
					release_net_interface( psInterface );
					psInterface = NULL;
					continue;
				}
			}
			break;
		}
	}

	RWL_UNLOCK_RO( g_hIfListMutex );

	return psInterface;
}


/**
 * \par Description:
 * Updates a parameter associated with an interface.	The address passed
 * should be an acquired interface pointer.
 *
 * This function is used to keep only one copy of the synchronisation
 * protocol floating around.
 *
 * This function is called from kernel space only.	Address changes from
 * userland are processed by netif_ioctl().
 */
static int set_interface_params( NetInterface_s *psIf, ipaddr_p anIpAddr, ipaddr_p anNetMask, ipaddr_p anExtraAddr, int *pnMTU, uint32 *pnFlags )
{
	int nError;

	if ( psIf == NULL )
		return -EINVAL;

	/* Must first obtain list mutex exclusively to prevent a race between
	 * rt_interface_changed and RWL_UPGRADE.
	 */
	if ( ( nError = RWL_LOCK_RW( g_hIfListMutex ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "set_interface_address(): Unable to lock interface list (%d).\n", nError );

		return nError;
	}

	/* Notify routing code and driver to release references to this interface */
	rt_release_interface( psIf );
	if ( psIf->ni_psOps->ni_release != NULL )
		psIf->ni_psOps->ni_release( psIf->ni_pInterface );

	/* Upgrade interface lock to a write lock */
	if ( ( nError = RWL_UPGRADE( psIf->ni_hMutex ) ) < 0 )
	{
		kerndbg( KERN_FATAL, "set_interface_address(): Unable to upgrade interface lock (%d).\n", nError );

		return nError;
	}

	/* Release interface list lock (because we have a write lock now) */
	RWL_UNLOCK_RW( g_hIfListMutex );

	/* Make changes */
	if ( anIpAddr != NULL )
		memcpy( psIf->ni_anIpAddr, anIpAddr, IP_ADR_LEN );

	if ( anNetMask != NULL )
		memcpy( psIf->ni_anSubNetMask, anNetMask, IP_ADR_LEN );

	if ( anExtraAddr != NULL )
		memcpy( psIf->ni_anExtraAddr, anExtraAddr, IP_ADR_LEN );

	if ( pnMTU != NULL )
		psIf->ni_nMTU = *pnMTU;

	if ( pnFlags != NULL )
		psIf->ni_nFlags = *pnFlags;


	/* Revert to read-only lock */
	RWL_DOWNGRADE( psIf->ni_hMutex );

	if ( psIf->ni_psOps->ni_notify != NULL )
		psIf->ni_psOps->ni_notify( psIf->ni_pInterface );

	return 0;
}

/**
 * \par Description:
 * Fetches the IP address associated with the interface at nIndex.
 *
 * Returns zero if successful, -ENOENT if no such interface or -EINVAL
 * if a the anAddress parameter is NULL.
 */
int get_interface_address( int nIndex, ipaddr_p anAddress )
{
	NetInterface_s *psIf;

	if ( anAddress == NULL )
		return -EINVAL;

	if ( ( psIf = get_net_interface( nIndex ) ) == NULL )
		return -ENOENT;

	IP_COPYADDR( anAddress, psIf->ni_anIpAddr );

	release_net_interface( psIf );

	return 0;
}


/**
 * \par Description:
 * Updates the address associated with an interface.	The psIf parameter
 * should be an acquired interface pointer.
 *
 * This function is called from kernel space only.	Address changes from
 * userland are processed by netif_ioctl().
 */
int set_interface_address( NetInterface_s *psIf, ipaddr_p anIpAddr )
{
	if ( psIf == NULL || anIpAddr == NULL )
		return -EINVAL;

	return set_interface_params( psIf, anIpAddr, NULL, NULL, NULL, NULL );
}



/**
 * \par Description:
 * Fetches the broadcast address associated with the interface at nIndex.
 *
 * Returns zero if successful, -ENOENT if no such interface or -EINVAL
 * if a the anAddress parameter is NULL or the interface is a point-to-point
 * interface.
 */
int get_interface_broadcast( int nIndex, ipaddr_p anAddress )
{
	NetInterface_s *psIf;
	ipaddr_t anBroadcast = { 255, 255, 255, 255 };
	int nError = -EINVAL;

	if ( anAddress == NULL )
		return -EINVAL;

	if ( ( psIf = get_net_interface( nIndex ) ) == NULL )
		return -ENOENT;

	if ( ( psIf->ni_nFlags & IFF_POINTOPOINT ) == 0 )
	{
		if ( ( psIf->ni_nFlags & IFF_BROADCAST ) == 0 )
			IP_COPYADDR( anAddress, anBroadcast );
		else
			IP_COPYADDR( anAddress, psIf->ni_anExtraAddr );

		nError = 0;
	}

	release_net_interface( psIf );

	return nError;
}


/**
 * \par Description:
 * Updates the broadcast address associated with an interface.	The psIf
 * parameter should be an acquired interface pointer.
 *
 * This function is called from kernel space only.	Address changes from
 * userland are processed by netif_ioctl().  If the interface is not a
 * broadcast interface this function returns -EINVAL.
 */
int set_interface_broadcast( NetInterface_s *psIf, ipaddr_p anBcast )
{
	uint32 nFlags;

	if ( psIf == NULL || anBcast == NULL )
		return -EINVAL;

	nFlags = psIf->ni_nFlags;

	if ( ( nFlags & IFF_POINTOPOINT ) != 0 )
		return -EINVAL;

	nFlags |= IFF_BROADCAST;

	return set_interface_params( psIf, NULL, NULL, anBcast, NULL, &nFlags );
}



/**
 * \par Description:
 * Fetches the destination address associated with the interface at nIndex.
 *
 * Returns zero if successful, -ENOENT if no such interface or -EINVAL
 * if a the anAddress parameter is NULL or the interface is a broadcast
 * interface.
 */
int get_interface_destination( int nIndex, ipaddr_p anAddress )
{
	NetInterface_s *psIf;
	int nError = -EINVAL;

	if ( anAddress == NULL )
		return -EINVAL;

	if ( ( psIf = get_net_interface( nIndex ) ) == NULL )
		return -ENOENT;

	if ( ( psIf->ni_nFlags & IFF_POINTOPOINT ) != 0 )
	{
		IP_COPYADDR( anAddress, psIf->ni_anExtraAddr );

		nError = 0;
	}

	release_net_interface( psIf );

	return nError;
}


/**
 * \par Description:
 * Updates the destination address associated with an interface.	The psIf
 * parameter should be an acquired interface pointer.
 *
 * This function is called from kernel space only.	Address changes from
 * userland are processed by netif_ioctl().  If the interface is not a
 * point-to-point interface this function returns -EINVAL.
 */
int set_interface_destination( NetInterface_s *psIf, ipaddr_p anDest )
{
	if ( psIf == NULL || anDest == NULL )
		return -EINVAL;

	if ( ( psIf->ni_nFlags & IFF_POINTOPOINT ) == 0 )
		return -EINVAL;

	return set_interface_params( psIf, NULL, NULL, anDest, NULL, NULL );
}



/**
 * \par Description:
 * Fetches the netmask associated with the interface at nIndex.
 *
 * Returns zero if successful, -ENOENT if no such interface or -EINVAL
 * if a the anAddress parameter is NULL.
 */
int get_interface_netmask( int nIndex, ipaddr_p anAddress )
{
	NetInterface_s *psIf;

	if ( anAddress == NULL )
		return -EINVAL;

	if ( ( psIf = get_net_interface( nIndex ) ) == NULL )
		return -ENOENT;

	IP_COPYADDR( anAddress, psIf->ni_anSubNetMask );

	release_net_interface( psIf );

	return 0;
}


/**
 * \par Description:
 * Updates the netmask associated with an interface.	The psIf parameter
 * should be an acquired interface pointer.
 *
 * This function is called from kernel space only.	Netmask changes from
 * userland are processed by netif_ioctl().
 */
int set_interface_netmask( NetInterface_s *psIf, ipaddr_p anNetmask )
{
	if ( psIf == NULL || anNetmask == NULL )
		return -EINVAL;

	return set_interface_params( psIf, NULL, anNetmask, NULL, NULL, NULL );
}



/**
 * \par Description:
 * Fetches the MTU associated with the interface at nIndex.
 *
 * Returns zero if successful, -ENOENT if no such interface or -EINVAL
 * if a the pnMTU parameter is NULL.
 */
int get_interface_mtu( int nIndex, int *pnMTU )
{
	NetInterface_s *psIf;

	if ( pnMTU == NULL )
		return -EINVAL;

	if ( ( psIf = get_net_interface( nIndex ) ) == NULL )
		return -ENOENT;

	*pnMTU = psIf->ni_nMTU;

	release_net_interface( psIf );

	return 0;
}


/**
 * \par Description:
 * Updates the MTU associated with an interface.	The psIf parameter
 * should be an acquired interface pointer.
 *
 * This function is called from kernel space only.	MTU changes from
 * userland are processed by netif_ioctl().
 */
int set_interface_mtu( NetInterface_s *psIf, int nMTU )
{
	if ( psIf == NULL )
		return -EINVAL;

	return set_interface_params( psIf, NULL, NULL, NULL, &nMTU, NULL );
}


/**
 * \par Description:
 * Fetches the flags associated with the interface at nIndex.
 *
 * Returns zero if successful, -ENOENT if no such interface or -EINVAL
 * if a the pnFlags parameter is NULL.
 */
int get_interface_flags( int nIndex, uint32 *pnFlags )
{
	NetInterface_s *psIf;

	if ( pnFlags == NULL )
		return -EINVAL;

	if ( ( psIf = get_net_interface( nIndex ) ) == NULL )
		return -ENOENT;

	*pnFlags = psIf->ni_nFlags;

	release_net_interface( psIf );

	return 0;
}


/**
 * \par Description:
 * Updates the Flags associated with an interface.	The psIf parameter
 * should be an acquired interface pointer.
 *
 * This function is called from kernel space only.	Flags changes from
 * userland are processed by netif_ioctl().
 */
int set_interface_flags( NetInterface_s *psIf, uint32 nFlags )
{
	if ( psIf == NULL )
		return -EINVAL;

	return set_interface_params( psIf, NULL, NULL, NULL, NULL, &nFlags );
}


/** 
 * \par Description:
 * Called from within netif_ioctl() to retrieve an interface parameter.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
static int ioc_get_if_param( int nCommand, struct ifreq *psConf )
{
	struct ifreq sConf;
	struct sockaddr_in *psAddr = ( struct sockaddr_in * )&sConf.ifr_addr;
	NetInterface_s *psInterface;
	int nError;

	nError = memcpy_from_user( &sConf, psConf, sizeof( sConf ) );
	if ( nError < 0 )
		return ( nError );


	sConf.ifr_name[sizeof( sConf.ifr_name ) - 1] = '\0';

	/* Acquire interface */
	psInterface = find_interface( sConf.ifr_name );
	if ( psInterface == NULL )
		return -ENOENT;

	nError = 0;

	switch ( nCommand )
	{
	case SIOCGIFADDR:
		psAddr->sin_family = AF_INET;
		psAddr->sin_port = 0;
		memcpy( psAddr->sin_addr, psInterface->ni_anIpAddr, IP_ADR_LEN );
		break;

	case SIOCGIFDSTADDR:
		/* Check this is a point-to-point interface */
		if ( ( psInterface->ni_nFlags & IFF_POINTOPOINT ) == IFF_POINTOPOINT )
		{
			psAddr->sin_family = AF_INET;
			psAddr->sin_port = 0;
			memcpy( psAddr->sin_addr, psInterface->ni_anExtraAddr, IP_ADR_LEN );
		}
		else
		{
			nError = -EINVAL;
		}
		break;

	case SIOCGIFBRDADDR:
		/* Check this is a broadcast interface */
		if ( ( psInterface->ni_nFlags & IFF_POINTOPOINT ) == 0 )
		{
			psAddr->sin_family = AF_INET;
			psAddr->sin_port = 0;

			/* Check if broadcast address has been specified */
			if ( ( psInterface->ni_nFlags & IFF_BROADCAST ) == IFF_BROADCAST )
			{
				memcpy( psAddr->sin_addr, psInterface->ni_anExtraAddr, IP_ADR_LEN );
			}
			else
			{
				memset( psAddr->sin_addr, 0xFF, IP_ADR_LEN );
			}
		}
		else
		{
			nError = -EINVAL;
		}
		break;

	case SIOCGIFNETMASK:
		psAddr->sin_family = AF_INET;
		psAddr->sin_port = 0;
		memcpy( psAddr->sin_addr, psInterface->ni_anSubNetMask, IP_ADR_LEN );
		break;

	case SIOCGIFMTU:
		sConf.ifr_mtu = psInterface->ni_nMTU;
		break;

	case SIOCGIFFLAGS:
		sConf.ifr_flags = psInterface->ni_nFlags;
		break;

	default:
		break;
	}

	/* Release interface */
	release_net_interface( psInterface );

	if ( nError == 0 )
		nError = memcpy_to_user( psConf, &sConf, sizeof( sConf ) );

	return nError;
}

/** 
 * \par Description:
 * Called by netif_ioctl() to set an interface parameter.
 *
 * \param psConf	- userspace pointer to interface request
 * \author	Kurt Skauen (kurt@atheos.cx)
 */
static int ioc_set_if_param( int nCommand, struct ifreq *psConf )
{
	struct ifreq sConf;
	struct sockaddr_in *psAddr = ( struct sockaddr_in * )&sConf.ifr_addr;
	NetInterface_s *psInterface;
	int nError;
	uint32 nFlags;

	nError = memcpy_from_user( &sConf, psConf, sizeof( sConf ) );

	if ( nError < 0 )
		return nError;


	sConf.ifr_name[sizeof( sConf.ifr_name ) - 1] = '\0';

	/* Acquire interface */
	psInterface = find_interface( sConf.ifr_name );
	if ( psInterface == NULL )
		return -ENOENT;

	switch ( nCommand )
	{
	case SIOCSIFADDR:
		nError = set_interface_params( psInterface, ( ipaddr_p ) & psAddr->sin_addr, NULL, NULL, NULL, NULL );
		break;

	case SIOCSIFDSTADDR:
		/* Check this is a broadcast interface */
		if ( ( psInterface->ni_nFlags & IFF_POINTOPOINT ) == IFF_POINTOPOINT )
		{
			nError = set_interface_params( psInterface, NULL, NULL, ( ipaddr_p ) & psAddr->sin_addr, NULL, NULL );
		}
		else
		{
			nError = -EINVAL;
		}
		break;
	case SIOCSIFBRDADDR:
		/* Check this is a broadcast interface */
		if ( ( psInterface->ni_nFlags & IFF_POINTOPOINT ) == 0 )
		{
			nFlags = psInterface->ni_nFlags | IFF_BROADCAST;

			nError = set_interface_params( psInterface, NULL, NULL, ( ipaddr_p ) & psAddr->sin_addr, NULL, &nFlags );
		}
		else
		{
			nError = -EINVAL;
		}
		break;

	case SIOCSIFNETMASK:
		nError = set_interface_params( psInterface, NULL, ( ipaddr_p ) & psAddr->sin_addr, NULL, NULL, NULL );
		break;

	case SIOCSIFMTU:
		nError = set_interface_params( psInterface, NULL, NULL, NULL, &sConf.ifr_mtu, NULL );
		break;

	case SIOCSIFFLAGS:
		nFlags = sConf.ifr_flags;
		nError = set_interface_params( psInterface, NULL, NULL, NULL, NULL, &nFlags );
		break;

	default:
		break;
	}

	/* Release interface */
	release_net_interface( psInterface );

	return nError;
}


/**
 * \par Description:
 * Dispatches ioctl requests for network interfaces (accessed through a
 * socket).
 */
int netif_ioctl( int nCmd, void *pBuf )
{
	NetInterface_s *psInterface;
	char acName[IFNAMSIZ];
	int nError;

	kerndbg( KERN_DEBUG_LOW, "netif_ioctl( 0x%8.8X, 0x%p )\n", nCmd, pBuf );

	switch ( nCmd )
	{
		/* Count interfaces */
	case SIOCGIFCOUNT:
		if ( ( nError = RWL_LOCK_RO( g_hIfListMutex ) ) < 0 )
		{
			kerndbg( KERN_FATAL, "netif_ioctl(): Unable to lock interface list.\n" );

			return nError;
		}

		nError = g_nInterfaceCount;

		RWL_UNLOCK_RO( g_hIfListMutex );
		break;

		/* Get interface list */
	case SIOCGIFCONF:
		nError = get_interface_list( ( struct ifconf * )pBuf );
		break;

		/* Inspect interface parameters */
	case SIOCGIFADDR:
	case SIOCGIFDSTADDR:
	case SIOCGIFBRDADDR:
	case SIOCGIFNETMASK:
	case SIOCGIFMTU:
	case SIOCGIFFLAGS:
		nError = ioc_get_if_param( nCmd, ( struct ifreq * )pBuf );
		break;

		/* Mutators */
	case SIOCSIFADDR:
	case SIOCSIFDSTADDR:
	case SIOCSIFBRDADDR:
	case SIOCSIFNETMASK:
	case SIOCSIFMTU:
	case SIOCSIFFLAGS:
		nError = ioc_set_if_param( nCmd, ( struct ifreq * )pBuf );
		break;

		/* Pass through to interface device driver */
	default:
		if ( memcpy_from_user( acName, pBuf, IFNAMSIZ ) < 0 )
		{
			nError = -EFAULT;
			break;
		}

		acName[IFNAMSIZ - 1] = '\0';

		/* Acquire interface */
		psInterface = find_interface( acName );

		if ( psInterface == NULL )
		{
			nError = -ENOENT;
			break;
		}

		kerndbg( KERN_DEBUG_LOW, "netif_ioctl(): Passing command 0x%8.8X to %s.\n", nCmd, acName );

		if ( psInterface->ni_psOps->ni_ioctl !=NULL )
		{
			nError = psInterface->ni_psOps->ni_ioctl ( psInterface->ni_pInterface, nCmd, pBuf );
		}
		else
			nError = -ENOSYS;

		/* Release interface */
		release_net_interface( psInterface );
		break;
	}

	kerndbg( KERN_DEBUG_LOW, "netif_ioctl( 0x%8.8X, 0x%p ): Done\n", nCmd, pBuf );
	return nError;
}

/**
 * \par Description
 * Loads an interface driver (e.g. driver for all ethernet interfaces,
 * or the driver for all PPP interfaces).
 *
 * \sa load_interface_drivers
 */
static int load_interface_driver( const char *pzPath, struct stat *psStat, void *pArg )
{
	ni_init *pfInit;
	int nDriver;
	InterfaceDriver_s *psDriver;
	int nError;



	nDriver = load_kernel_driver( pzPath );

	psDriver = kmalloc( sizeof( InterfaceDriver_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psDriver == NULL )
	{
		kerndbg( KERN_FATAL, "load_interface_driver(): " "no memory for interface driver descriptor\n" );
		return -ENOMEM;
	}

	if ( nDriver < 0 )
	{
		kerndbg( KERN_INFO, "load_interface_driver(): failed to load driver %s\n", pzPath );
		kfree( psDriver );
		return 0;
	}

	nError = get_symbol_address( nDriver, "init_interfaces", -1, ( void ** )&pfInit );
	if ( nError < 0 )
	{
		kerndbg( KERN_INFO, "load_interface_driver(): " "driver %s dont export init_interfaces()\n", pzPath );
		goto error;
	}

	psDriver->id_psOps = pfInit();
	psDriver->id_hDriver = nDriver;

	if ( psDriver->id_psOps == NULL )
	{
		kerndbg( KERN_INFO, "load_interface_driver(): " "Failed to initialize %s\n", pzPath );
		goto error;
	}

	psDriver->id_psNext = g_psFirstInterfaceDriver;
	g_psFirstInterfaceDriver = psDriver;

	return 0;

      error:
	unload_kernel_driver( nDriver );
	kfree( psDriver );

	return 0;
}

/**
 * \par Description
 * Iterates over the interface drivers in /system/drivers/net/if,
 * attempting to load each interface driver.  Note that these are typically
 * interface class drivers -- ethernet interfaces versus PPP interfaces --
 * rather than specific hardware drivers.
 */
void load_interface_drivers( void )
{
	char zPath[1024];
	int nPathBase;

	get_system_path( zPath, 256 );

	nPathBase = strlen( zPath );

	if ( '/' != zPath[nPathBase - 1] )
	{
		zPath[nPathBase] = '/';
		zPath[nPathBase + 1] = '\0';
	}

	strcat( zPath, "system/drivers/net/if" );

	iterate_directory( zPath, false, load_interface_driver, NULL );
}

/**
 * \par Description:
 * Initialises the network interface handling code.
 */
void init_net_interfaces( void )
{
	g_hIfListMutex = CREATE_RWLOCK( "interface list mutex" );
	g_hTryReleaseMutex = CREATE_MUTEX( "try release interface mutex" );
}

