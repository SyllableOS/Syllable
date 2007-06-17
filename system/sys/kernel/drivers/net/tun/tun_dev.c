/*
 *  
 *  Copyright (C) 2003 William Rose
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
 *
 * Purpose
 * -------
 * 
 * The tun driver is a concept borrowed from other operating systems: it provides
 * a user device that can be used to control a virtual network interface.  The
 * use in this is in making it easy to develop new programs that require a kernel
 * network interface as part of their operation (e.g. dial-up PPP connections,
 * virtual private networks).  The tun driver takes care of the kernel side and
 * presents a ordinary device driver to userland programs that can then use read()
 * and write() to receive packets sent to the interface and send packets from the
 * interface.
 * 
 * Implementation
 * --------------
 * 
 * The tun driver is a device driver that creates a single device node at startup
 * (/dev/net/tun).  When this device is opened by a user program, the file descriptor
 * returned may be then associated with a network interface by issuing an ioctl()
 * on the file descriptor and passing in a name (max length 15 characters) for
 * the new interface.  See the file examples/create_tun.c for details.
 * 
 * The names are checked for clashes within the tun driver before attempting to
 * add the interface in the kernel.  Names may also have a number added, to allow
 * user programs to create interfaces with a common stem (e.g. ppp0, ppp1).  The
 * tun driver will automatically add a number if the interface name ends with a
 * '+'.  The only valid characters for interface names therefore are lower case
 * letters and an optional trailing '+'.  The full interface name is returned in
 * the same buffer used to send the name, which must be at least 16 bytes (based
 * on IFNAMSIZ).
 * 
 * Once created, the interface can be manipulated with ifconfig and routes created
 * with route.  Or, as ifconfig and route do, you can create a socket and issue
 * interface or routing ioctl()s on it using the interface name.
 * 
 *
 */

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>

#include <posix/errno.h>
#include <posix/fcntl.h>
#include <posix/ioctls.h>

#include <net/net.h>
#include <net/ip.h>
#include <net/if_tun.h>

#include <macros.h>

/**
 * Globals
 */
int g_hDeviceNode = -1;

/**
 * User program is requesting a new session with the tunnel driver.
 *
 * Establishing a session does not automatically create an interface.
 * This is to allow programs to query the tun driver without creating
 * an interface unnecessarily and also allows for some more parameters
 * to be passed in as part of the device setup.
 */
static status_t
tun_open( void *pNode, uint32 nMode, void **ppCookie ) {
  TunInterface_s *psTun;
  
  kerndbg( KERN_DEBUG_LOW, "tun_open(): Creating new tun interface\n" );

  psTun = (TunInterface_s *)kmalloc( sizeof( TunInterface_s ),
                                     MEMF_KERNEL | MEMF_CLEAR );
  
  if( psTun == NULL )
    return -ENOMEM;
  
  psTun->ti_nMode = nMode;
  
  *ppCookie = (void *)psTun;
  
  return 0;
}

/**
 * User program has finished with the tunnel driver.
 */
static status_t
tun_close( void *pNode, void *pCookie ) {
  TunInterface_s *psTun = (TunInterface_s *)pCookie;
  
  if( psTun == NULL )
    return -EINVAL;
  
  kerndbg( KERN_DEBUG_LOW, "tun_close(): Closing tun interface \'%s\'\n",
           psTun->ti_psBaseInterface->ni_zName );
  
  tun_delete_if( psTun );
  
  kfree( psTun );
  
  return 0;
}

/**
 * Turn on the specified file mode flags.
 */
static status_t
tun_set_mode( TunInterface_s *psTun, int nMode ) {
  int nError;

  kerndbg( KERN_DEBUG_LOW,
           "tun_clr_mode( '%s', 0x%8.8X )\n",
           psTun->ti_psBaseInterface->ni_zName, nMode );

  if( (nError = LOCK( psTun->ti_hMutex )) < 0 )
    return nError;

  psTun->ti_nMode |= nMode;

  UNLOCK( psTun->ti_hMutex );

  return 0;
}

/**
 * Turn off the specified file mode flags.
 */
static status_t
tun_clr_mode( TunInterface_s *psTun, int nMode ) {
  int nError;

  kerndbg( KERN_DEBUG_LOW,
           "tun_clr_mode( '%s', 0x%8.8X )\n",
           psTun->ti_psBaseInterface->ni_zName, nMode );

  if( (nError = LOCK( psTun->ti_hMutex )) < 0 )
    return nError;

  psTun->ti_nMode &= ~nMode;

  UNLOCK( psTun->ti_hMutex );

  return 0;
}

static status_t
tun_ioctl( void *pNode, void *pCookie,
           uint32 nCommand, void *pArgs, bool bFromKernel ) {
  TunInterface_s *psTun = (TunInterface_s *)pCookie;
  char acName[IFNAMSIZ];
  int  flag;

  if( psTun == NULL )
    return -EINVAL;
  
  kerndbg( KERN_DEBUG_LOW, "tun_ioctl( %s, 0x%8.8lX, 0x%8.8lX )\n",
           psTun->ti_psBaseInterface->ni_zName, nCommand, (uint32)pArgs );
  
  switch( nCommand ) {
    case TUN_IOC_CREATE_IF:
      if( bFromKernel ) {
        strncpy( acName, (const char *)pArgs, IFNAMSIZ );
      } else {
        strncpy_from_user( acName, (const char *)pArgs, IFNAMSIZ );
      }
      
      /* Null terminate */
      acName[IFNAMSIZ - 1] = 0;
      
      if( (flag = tun_create_if( psTun, acName )) < 0 )
        return flag;
      
      /* Copy back full interface address */
      if( bFromKernel ) {
        strcpy( (char *)pArgs, acName );
      } else {
        strcpy_to_user( (char *)pArgs, acName );
      }
      return 0;

    case TUN_IOC_DELETE_IF:
      return tun_delete_if( psTun );
    
    case FIONBIO:
      /* Switch on/off non-blocking I/O */
      if( bFromKernel )
        flag = *((int *)pArgs);
      else if( memcpy_from_user( &flag, pArgs, sizeof( flag ) ) < 0 )
        return -EFAULT;
      
      if( flag )
        return tun_set_mode( psTun, O_NONBLOCK );
      else
        return tun_clr_mode( psTun, O_NONBLOCK );
      
    default:
      break;
  }
  
  return -ENOSYS;
}

/**
 * User program is expecting to read some data.
 */
static status_t
tun_read( void *pNode, void *pCookie,
          off_t nPosition, void *pBuffer, size_t nSize ) {
  TunInterface_s *psTun = (TunInterface_s *)pCookie;
  PacketBuf_s    *psPacket = NULL;
  IpHeader_s     *psIpHeader;
  int             nError;
  
  kerndbg( KERN_DEBUG_LOW, "tun_read( %s, %lu, 0x%8.8lX, %lu )\n",
           psTun->ti_psBaseInterface->ni_zName, (uint32)nPosition,
           (uint32)pBuffer, (uint32)nSize );
  
  if( (nError = LOCK( psTun->ti_hMutex )) < 0 )
    return nError;
  
  // if( (psTun->ti_nFlags & IFF_UP) == 0 ) {
  //   nError = -ENETDOWN;
  //   goto exit;
  // }
  // 
  /* Remove a packet from the queue */
  /* The number of packets in the queue is stored in the count of the
   * NetQueue_s semaphore.
   */
  if( (psTun->ti_nMode & O_NONBLOCK) != 0 ) {
    psPacket = remove_head_packet( &psTun->ti_sToUserLand, 0LL );
  } else {
    /* Must unlock before we block so that another thread can access the
     * queue.  Access to the NetQueue is synchronised separately anyhow
     * (see kernel/net/packets.c).
     */
    UNLOCK( psTun->ti_hMutex );
    
    psPacket = remove_head_packet( &psTun->ti_sToUserLand, INFINITE_TIMEOUT );
    
    if( (nError = LOCK( psTun->ti_hMutex )) < 0 )
      goto exit;
  }
  
  if( psPacket == NULL )
    nError = -EWOULDBLOCK;
  else {
    /* pBuffer is always a kernel-space pointer */
    /* I find the PacketBuf_s structure to be rather confusing most of the
     * time -- it is heavily geared towards ethernet interfaces.  So this code
     * only looks at the IP header.
     */
    psIpHeader = psPacket->pb_uNetworkHdr.psIP;
    
    nError = ntohw( psIpHeader->iph_nPacketSize );
    /* Don't read more than was requested */
    if( nError > nSize )
      nError = nSize;
    
    memcpy( pBuffer, psIpHeader, nError );
  }
  
exit:
  UNLOCK( psTun->ti_hMutex );
  
  /* The entire packet is discarded, even if only part of it was read. */
  if( psPacket != NULL )
    free_pkt_buffer( psPacket );
  
  return nError;
}

static status_t
tun_write( void *pNode, void *pCookie,
           off_t nPosition, const void *pBuffer, size_t nSize ) {
  TunInterface_s *psTun = (TunInterface_s *)pCookie;
  PacketBuf_s    *psPacket;
  IpHeader_s     *psIpHeader = (IpHeader_s *)pBuffer;
  int             nError, nEthHdrLen;
  
  kerndbg( KERN_DEBUG_LOW, "tun_write( %s, %lu, 0x%8.8lX, %lu )\n",
           psTun->ti_psBaseInterface->ni_zName, (uint32)nPosition,
           (uint32)pBuffer, (uint32)nSize );
  
  /* Check arguments */
  if( pBuffer == NULL )
    return -EFAULT;

  /* Check that the size specified in the nSize parameter matches the IP
   * packet header
   */
  if( nSize < sizeof( IpHeader_s ) ) {
    kerndbg( KERN_FATAL,
             "tun_write(): Can't write as data smaller than IP header.\n" );
    
    return -EINVAL;
  }
  
  if( nSize < ntohw( psIpHeader->iph_nPacketSize ) ) {
    kerndbg( KERN_FATAL,
             "tun_write(): Packet header says packet is %hu, not %lu.\n",
             (uint16)ntohw( psIpHeader->iph_nPacketSize ), (uint32)nSize );
    
    return -EINVAL;
  }

  if( nSize > ntohw( psIpHeader->iph_nPacketSize ) ) {
    kerndbg( KERN_WARNING,
             "tun_write(): Header says send %d bytes of packet, not %lu.\n",
             (int)(ntohw( psIpHeader->iph_nPacketSize )), (uint32)nSize );
    
    nSize = ntohw( psIpHeader->iph_nPacketSize );
  }
  
  /* Prepare to send packet */
  if( (nError = LOCK( psTun->ti_hMutex )) < 0 )
    return nError;
  
  /* Check if interface is up */
  uint32 nFlags;
  get_interface_flags( psTun->ti_nIndex, &nFlags );
  if( (nFlags & IFF_UP) == 0 ) {
    nError = -ENETDOWN;
    goto exit;
  }

  /* Create a packet from the data */
  /* Reserve some space for the ethernet header */
  nEthHdrLen = ((ETH_HLEN + 3) & ~3); /* rounded up to multiple of 4 */
  
  if( (psPacket = alloc_pkt_buffer( nEthHdrLen + nSize )) == NULL ) {
    kerndbg( KERN_FATAL,
             "tun_write(): Unable to allocate packet buffer.\n" );
    
    goto exit;
  }
  
  /* Set pointers for the data link and network protocol headers */
  psPacket->pb_uNetworkHdr.pRaw = psPacket->pb_pHead + nEthHdrLen;
  psPacket->pb_uMacHdr.pRaw = psPacket->pb_uNetworkHdr.pRaw - ETH_HLEN;
  
  /* Copy in the packet and size */
  memcpy( psPacket->pb_uNetworkHdr.pRaw, pBuffer, nSize );
  psPacket->pb_nSize = nSize;
  
  /* Set the pointer to the transport layer header */
  psPacket->pb_uTransportHdr.pRaw =
    psPacket->pb_uNetworkHdr.pRaw +
    IP_GET_HDR_LEN( psPacket->pb_uNetworkHdr.psIP );

  /* Pass the packet into the kernel IP core */
  UNLOCK( psTun->ti_hMutex );
  
  ip_in( psPacket, psPacket->pb_nSize );
  
  return nSize;
  
exit:
  UNLOCK( psTun->ti_hMutex );
  
  return nError;
}

/**
 * Wakes all the queued readers on the interface psTun.
 *
 * The caller is responsible for locking psTun before using this function.
 */
void
tun_wake_readers( TunInterface_s *psTun ) {
  SelectRequest_s *psSR;
  
  /* All waiting requests are released -- so some may not get a packet
   * if they try to read too late.
   */
  for( psSR = psTun->ti_psReaders; psSR != NULL; psSR = psSR->sr_psNext ) {
    psSR->sr_bReady = true;
    unlock_semaphore( psSR->sr_hSema );
  }
}

/**
 * Wakes all the queued writers on the interface psTun.
 *
 * The caller is responsible for locking psTun before using this function.
 */
void
tun_wake_writers( TunInterface_s *psTun ) {
  SelectRequest_s *psSR;
  
  /* All waiting requests are released */
  for( psSR = psTun->ti_psWriters; psSR != NULL; psSR = psSR->sr_psNext ) {
    psSR->sr_bReady = true;
    unlock_semaphore( psSR->sr_hSema );
  }
}

/**
 * Enqueues a waiting reader or writer onto the interface.
 *
 * This is used to implement select() which is needed for non-blocking I/O.
 */
static int
tun_add_select_req( void *pNode, void *pCookie, SelectRequest_s *psRequest ) {
  TunInterface_s *psTun = (TunInterface_s *)pCookie;
  int             nError;
#ifdef __ENABLE_DEBUG__
  char           *apzEvents[] = { "<invalid>", "read", "write", "except" };
#endif
  
  if( psRequest->sr_nMode < SELECT_READ || psRequest->sr_nMode > SELECT_EXCEPT )
    psRequest->sr_nMode = 0;
  
  kerndbg( KERN_DEBUG_LOW, "tun_add_select_req( %s, %s )\n",
           psTun->ti_psBaseInterface->ni_zName, 
           apzEvents[psRequest->sr_nMode] );
  
  if( (nError = LOCK( psTun->ti_hMutex )) < 0 )
    return nError;
  
  switch( psRequest->sr_nMode ) {
    case SELECT_READ:
      /* We handle these */
      psRequest->sr_psNext = psTun->ti_psReaders;
      psTun->ti_psReaders = psRequest;
      
      nError = get_semaphore_count( psTun->ti_sToUserLand.nq_hSyncSem );
      
      if( nError > 0 ) {
        kerndbg( KERN_DEBUG_LOW,
                 "tun_add_select_req(): Wake readers: there are %d packets.\n",
                 nError );
        
        tun_wake_readers( psTun );
      }
      break;

    case SELECT_WRITE:
      /* We can always write */
      psRequest->sr_psNext = psTun->ti_psWriters;
      psTun->ti_psWriters = psRequest;

      tun_wake_writers( psTun );
      break;

    case SELECT_EXCEPT:
      /* Can't do anything with these, so just ignore them */
      break;
    
    default:
      kerndbg( KERN_WARNING,
               "tun_add_select_req(): Invalid mode for select request.\n" );
      return -EINVAL;
  }
  
  UNLOCK( psTun->ti_hMutex );
  
  return 0;
}

/**
 * Dequeues a waiting reader or writer from the interface.
 *
 * This is used to implement select() which is needed for non-blocking I/O.
 */
static int
tun_rem_select_req( void *pNode, void *pCookie, SelectRequest_s *psRequest ) {
  TunInterface_s   *psTun = (TunInterface_s *)pCookie;
  int               nError;
  SelectRequest_s **ppsRemove;
#ifdef __ENABLE_DEBUG__
  char           *apzEvents[] = { "<invalid>", "read", "write", "except" };
#endif
  
  if( psRequest->sr_nMode < SELECT_READ || psRequest->sr_nMode > SELECT_EXCEPT )
    psRequest->sr_nMode = 0;
  
  kerndbg( KERN_DEBUG_LOW, "tun_rem_select_req( %s, %s )\n",
           psTun->ti_psBaseInterface->ni_zName, 
           apzEvents[psRequest->sr_nMode] );
  
  if( (nError = LOCK( psTun->ti_hMutex )) < 0 )
    return nError;
  
  switch( psRequest->sr_nMode ) {
    case SELECT_READ:
      ppsRemove = &psTun->ti_psReaders;
      break;

    case SELECT_WRITE:
      ppsRemove = &psTun->ti_psWriters;
      break;

    case SELECT_EXCEPT:
      return 0;
      
    default:
      kerndbg( KERN_WARNING,
               "tun_rem_select_req(): Invalid mode for select request.\n" );
      return -EINVAL;
  }
  
  while( *ppsRemove != NULL && *ppsRemove != psRequest )
    ppsRemove = &((*ppsRemove)->sr_psNext);

  if( (*ppsRemove) == psRequest )
    (*ppsRemove) = psRequest->sr_psNext;
  else {
    kerndbg( KERN_WARNING,
             "tun_rem_select_req(): Request not found in queue.\n" );
  }
  
  UNLOCK( psTun->ti_hMutex );
  
  return 0;
}

/**
 * Device operations structure (used to dispatch filesystem operations).
 */
DeviceOperations_s g_sDeviceOps = {
  tun_open,
  tun_close,
  tun_ioctl,
  tun_read,
  tun_write,
  NULL,
  NULL,
  tun_add_select_req,
  tun_rem_select_req
};

/**
 * Driver initialisation.
 */
status_t device_init( int nDeviceID ) {
  kerndbg( KERN_DEBUG_LOW, "tun: device_init() called.\n" );
  
  /* Create interface list semaphore */
  if( (g_hTunListMutex = create_semaphore( "tun interface list", 1, 0 )) < 0 ) {
    kerndbg( KERN_FATAL, "tun: create_semaphore() failed in device_init()\n" );
    
    return g_hTunListMutex;
  }
  
  /* Create device node */
  g_hDeviceNode = create_device_node( nDeviceID, -1, "net/tun", &g_sDeviceOps, NULL );

  if( g_hDeviceNode < 0 ) {
    kerndbg( KERN_FATAL, "tun: create_device_node() failed in "
                         "device_init()\n" );

    delete_semaphore( g_hTunListMutex );
    g_hTunListMutex = -1;
    
    return g_hDeviceNode;
  }
  
  return 0;
}

status_t device_uninit( int nDeviceID ) {
  int nError;
  
  kerndbg( KERN_DEBUG_LOW, "tun: device_uninit() called.\n" );
  
  /* Remove device node */
  if( g_hDeviceNode > 0 )
    delete_device_node( g_hDeviceNode );
  
  /* Lock interface list */
  if( (nError = LOCK( g_hTunListMutex )) < 0 )
    return nError;
  
  /* Close all interfaces */

  /* Destroy list semaphore */
  delete_semaphore( g_hTunListMutex );

  return 0;
}
