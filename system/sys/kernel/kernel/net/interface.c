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

#include <posix/errno.h>

#include <atheos/kernel.h>
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

static NetInterface_s* g_asNetInterfaces[MAX_NET_INTERFACES];
static int	       g_nInterfaceCount = 0;
static sem_id	       g_hIFMutex = -1;

typedef struct _InterfaceDriver InterfaceDriver_s;
static struct _InterfaceDriver
{
    InterfaceDriver_s*	     id_psNext;
    image_id	       	     id_hDriver;
    const NetInterfaceOps_s* id_psOps;
} *g_psFirstInterfaceDriver = NULL;


/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static inline bool compare_net_address( ipaddr_t anAddr1, ipaddr_t anAddr2, ipaddr_t anMask )
{
    ipaddr_t anMasked1 = { anAddr1[0] & anMask[0], anAddr1[1] & anMask[1], anAddr1[2] & anMask[2], anAddr1[3] & anMask[3] };
    ipaddr_t anMasked2 = { anAddr2[0] & anMask[0], anAddr2[1] & anMask[1], anAddr2[2] & anMask[2], anAddr2[3] & anMask[3] };
    
    return( IP_SAMEADDR( anMasked1, anMasked2 ) );
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

static int get_interface_list( struct ifconf* psConf )
{
    int	i;
    int	nBufSize;
    int nBytesCopyed = 0;
    int nError;
    char* pBuffer;
    kassertw( sizeof( psConf->ifc_len ) == sizeof(int) );
    
    nError = memcpy_from_user( &nBufSize,  &psConf->ifc_len, sizeof(int) );
    if ( nError < 0 ) {
	return( nError );
    }
    nError = memcpy_from_user( &pBuffer, &psConf->ifc_buf, sizeof(pBuffer) );
    if ( nError < 0 ) {
	return( nError );
    }
    for ( i = 0 ; i < MAX_NET_INTERFACES ; ++i ) {
	struct ifreq sIfConf;
	struct sockaddr_in* psAddr = (struct sockaddr_in*)&sIfConf.ifr_addr;
	int    nCurLen = min( sizeof( sIfConf ), nBufSize );

	if ( g_asNetInterfaces[i] == NULL ) {
	    continue;
	}
	if ( nCurLen == 0 ) {
	    break;
	}
	
	psAddr->sin_family = AF_INET;
	psAddr->sin_port   = 0;
	
	strcpy( sIfConf.ifr_name, g_asNetInterfaces[i]->ni_zName );
	memcpy( psAddr->sin_addr, g_asNetInterfaces[i]->ni_anIpAddr, 4 );

	nError = memcpy_to_user( pBuffer, &sIfConf, nCurLen );
	pBuffer += nCurLen;
	if ( nError < 0 ) {
	    break;
	}
	nBytesCopyed += nCurLen;
    }
    if ( nError >= 0 ) {
	nError = memcpy_to_user( &psConf->ifc_len, &nBytesCopyed, sizeof(int) );
    }
    return( nError );
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

int add_net_interface( NetInterface_s* psInterface )
{
    int i;
    for ( i = 0 ; i < MAX_NET_INTERFACES ; ++i ) {
	if ( g_asNetInterfaces[i] == NULL ) {
	    g_asNetInterfaces[i] = psInterface;
	    g_nInterfaceCount++;
	    return( i );
	}
    }
    return( -EMFILE );
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

NetInterface_s* get_net_interface( int nIndex )
{
    if ( nIndex < 0 || nIndex >= MAX_NET_INTERFACES ) {
	return( NULL );
    }
    return( g_asNetInterfaces[nIndex] );
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

NetInterface_s* find_interface( const char* pzName )
{
    int i;
    
    for ( i = 0 ; i < MAX_NET_INTERFACES ; ++i ) {
	if ( g_asNetInterfaces[i] == NULL ) {
	    continue;
	}
	if ( strcmp( g_asNetInterfaces[i]->ni_zName, pzName ) == 0 ) {
	    return( g_asNetInterfaces[i] );
	}
    }
    return( NULL );
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

NetInterface_s* find_interface_by_addr( ipaddr_t anAddress )
{
    int i;
    
    for ( i = 0 ; i < MAX_NET_INTERFACES ; ++i ) {
	if ( g_asNetInterfaces[i] == NULL ) {
	    continue;
	}
	if ( compare_net_address( g_asNetInterfaces[i]->ni_anIpAddr, anAddress, g_asNetInterfaces[i]->ni_anSubNetMask ) ) {
	    return( g_asNetInterfaces[i] );
	}
    }
    return( NULL );
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

static int get_interface_address( struct ifreq* psConf )
{
    struct ifreq sConf;
    struct sockaddr_in* psAddr = (struct sockaddr_in*)&sConf.ifr_addr;
    NetInterface_s* psInterface;
    int	nError;
    
    nError = memcpy_from_user( &sConf, psConf, sizeof( sConf ) );
    if ( nError < 0 ) {
	return( nError );
    }
    sConf.ifr_name[sizeof(sConf.ifr_name)] = '\0'; 
    psInterface = find_interface( sConf.ifr_name );
    if ( psInterface == NULL ) {
	return( -ENOENT );
    }
    psAddr->sin_family = AF_INET;
    psAddr->sin_port   = 0;
    memcpy( psAddr->sin_addr, psInterface->ni_anIpAddr, 4 );
    
    nError = memcpy_to_user( psConf, &sConf, sizeof( sConf ) );
    
    return( nError );
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

static int set_interface_address( struct ifreq* psConf )
{
    struct ifreq sConf;
    struct sockaddr_in* psAddr = (struct sockaddr_in*)&sConf.ifr_addr;
    NetInterface_s* psInterface;
    int	nError;
    
    nError = memcpy_from_user( &sConf, psConf, sizeof( sConf ) );
    if ( nError < 0 ) {
	return( nError );
    }
    sConf.ifr_name[sizeof(sConf.ifr_name)] = '\0'; 
    psInterface = find_interface( sConf.ifr_name );
    if ( psInterface == NULL ) {
	return( -ENOENT );
    }
    rt_interface_address_changed( psInterface, psAddr->sin_addr );
    memcpy( psInterface->ni_anIpAddr, psAddr->sin_addr, 4 );
    return( nError );
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

static int get_interface_netmask( struct ifreq* psConf )
{
    struct ifreq sConf;
    struct sockaddr_in* psAddr = (struct sockaddr_in*)&sConf.ifr_netmask;
    NetInterface_s* psInterface;
    int	nError;
    
    nError = memcpy_from_user( &sConf, psConf, sizeof( sConf ) );
    if ( nError < 0 ) {
	return( nError );
    }
    sConf.ifr_name[sizeof(sConf.ifr_name)] = '\0'; 
    psInterface = find_interface( sConf.ifr_name );
    if ( psInterface == NULL ) {
	return( -ENOENT );
    }
    psAddr->sin_family = AF_INET;
    psAddr->sin_port   = 0;
    memcpy( psAddr->sin_addr, psInterface->ni_anSubNetMask, 4 );
    
    nError = memcpy_to_user( psConf, &sConf, sizeof( sConf ) );
    
    return( nError );
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

static int set_interface_netmask( struct ifreq* psConf )
{
    struct ifreq sConf;
    struct sockaddr_in* psAddr = (struct sockaddr_in*)&sConf.ifr_netmask;
    NetInterface_s* psInterface;
    int	nError;
    
    nError = memcpy_from_user( &sConf, psConf, sizeof( sConf ) );
    if ( nError < 0 ) {
	return( nError );
    }
    sConf.ifr_name[sizeof(sConf.ifr_name)] = '\0'; 
    psInterface = find_interface( sConf.ifr_name );
    if ( psInterface == NULL ) {
	return( -ENOENT );
    }
    rt_interface_mask_changed( psInterface, psAddr->sin_addr );
    memcpy( psInterface->ni_anSubNetMask, psAddr->sin_addr, 4 );
    return( nError );
}

int netif_ioctl( int nCmd, void* pBuf )
{
    int nError;
    switch( nCmd )
    {
	case SIOCGIFCOUNT:
	    nError = g_nInterfaceCount;
	    break;
	case SIOCGIFCONF:	// Get list of interfaces
	    nError = get_interface_list( (struct ifconf*) pBuf );
	    break;
	case SIOCGIFADDR:	// Get interface address
	    nError = get_interface_address( (struct ifreq*) pBuf );
	    break;
	case SIOCSIFADDR:
	    nError = set_interface_address( (struct ifreq*) pBuf );
	    break;
	case SIOCGIFNETMASK:
	    nError = get_interface_netmask( (struct ifreq*) pBuf );
	    break;
	case SIOCSIFNETMASK:
	    nError = set_interface_netmask( (struct ifreq*) pBuf );
	    break;
	default:
	{
	    NetInterface_s* psInterface;
	    char	zName[IFNAMSIZ];

	    if ( memcpy_from_user( zName, pBuf, sizeof( zName ) ) < 0 ) {
		nError = -EFAULT;
		break;
	    }
	    zName[sizeof(zName)-1] = '\0';

	    psInterface = find_interface( zName );

	    if ( psInterface == NULL ) {
		nError = -ENOENT;
		break;
	    }
	    nError = psInterface->ni_psOps->ni_ioctl( psInterface->ni_pInterface, nCmd, pBuf );
	    break;
	}
    }
    return( nError );
}


static int load_interface_driver( const char* pzPath, struct stat* psStat, void* pArg )
{
    ni_init*	pfInit;
    int nDriver;
    InterfaceDriver_s*	psDriver;
    int	nError;

    

    nDriver = load_kernel_driver( pzPath );
    
    psDriver = kmalloc( sizeof( InterfaceDriver_s ), MEMF_KERNEL | MEMF_CLEAR );

    if ( psDriver == NULL ) {
	printk( "Error: load_interface_driver() no memory for interface driver descriptor\n" );
	return( -ENOMEM );
    }
    
    if ( nDriver < 0 ) {
	printk( "Error: load_interface_driver() failed to load driver %s\n", pzPath );
	kfree( psDriver );
	return(0);
    }
    nError =  get_symbol_address( nDriver, "init_interfaces", -1 , (void**) &pfInit );
    if ( nError < 0 ) {
	printk( "Error: load_interface_driver() driver %s dont export init_interfaces()\n", pzPath );
	goto error;
    }
    
    psDriver->id_psOps = pfInit();
    psDriver->id_hDriver = nDriver;
    
    if ( psDriver->id_psOps == NULL ) {
	printk( "Error: load_interface_driver() Failed to initialize %s\n", pzPath );
	goto error;
    }
    psDriver->id_psNext = g_psFirstInterfaceDriver;
    g_psFirstInterfaceDriver = psDriver;
    return(0);
error:
    unload_kernel_driver( nDriver );
    kfree( psDriver );
    return(0);
}

static void load_interface_drivers()
{
    char zPath[1024];
    int  nPathBase;

    get_system_path( zPath, 256 );

    nPathBase = strlen( zPath );
	
    if ( '/' != zPath[ nPathBase - 1 ] ) {
      zPath[ nPathBase ] = '/';
      zPath[ nPathBase + 1 ] = '\0';
    }

    strcat( zPath, "sys/drivers/net/if" );

    iterate_directory( zPath, false, load_interface_driver, NULL );
}

void init_net_interfaces(void)
{
    g_hIFMutex = create_semaphore( "if_mutex", 1, 0 );
    load_interface_drivers();
}
