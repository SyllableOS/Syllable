/*
 *  User space IP network device
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
 */

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/semaphore.h>

#include <posix/ioctl.h>
#include <posix/errno.h>

#include <net/net.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_tun.h>
#include <net/sockios.h>
#include <net/route.h>
#include <net/ip.h>

#include <macros.h>

TunInterface_s  *g_psTunListHead = NULL;
sem_id           g_hTunListMutex = -1;

/* Hmm.  Not in the kernel */
static int
atoi( const char *pzStr ) {
  int result = 0;

  while( *pzStr >= '0' && *pzStr <= '9' ) {
    result *= 10;
    result += *pzStr - '0';
    ++pzStr;
  }

  return result;
}

/**
 * Network interface operations.
 */
static int tun_if_ioctl( void* pInterface, int nCmd, void* pArg );
static int tun_if_write( Route_s* psRoute, ipaddr_t pDstAddress,
                         PacketBuf_s* psPkt );
static void tun_if_deinit( void* pInterface );
static void tun_if_notify( void* pInterface );

static NetInterfaceOps_s g_sInterfaceOps =
{
    tun_if_write,
    tun_if_ioctl,
    tun_if_deinit,
	tun_if_notify,
	NULL
};

/**
 * Checks that a name is valid, after substituting an appropriate index
 * for a suffixed +.  Must be called with the global interface list
 * locked.  This is only intended for use within tun_create_if().
 */
static int
tun_check_name( char acName[IFNAMSIZ], TunInterface_s ***pppsInsertAfter ) {

  int            nIdxOffset = -1, nIndex = 0;
  char           *p;
  TunInterface_s **ppsCurrent;
  
  kerndbg( KERN_DEBUG_LOW, "tun_check_name( \'%s\' )\n", acName );
  
  *pppsInsertAfter = NULL;

  /* Check the name is valid looking. */
  for( p = acName; *p != 0; ++p )
    if( *p < 'a' || *p > 'z' )
      break;

  /* A trailing + is okay */
  if( *p == '+' ) {
    nIdxOffset = p - acName;
    *p++ = 0;
  }
  
  if( *p != 0 ) { /* Any other trailing characters are not permitted */
    kerndbg( KERN_DEBUG,
             "tun_check_name( \'%s\' ): Invalid trailing characters\n",
             acName );
    
    return -EINVAL;
  }
  
  if( nIdxOffset == 0 ) { /* A zero-length prefix is also not permitted */
    kerndbg( KERN_DEBUG,
             "tun_check_name( \'%s\' ): Zero-length prefix\n",
             acName );
    
    return -EINVAL;
  }
  
  
  /**
   * As the name looks okay, it is now necessary to check for duplicates.
   * This involves working out what the template should evaluate to so that
   * a proper check is performed.
   */
  
  /* Scan until the current item is greater than or equal to our stem. */
  for( ppsCurrent = &g_psTunListHead;
       *ppsCurrent != NULL;
       ppsCurrent = &((*ppsCurrent)->ti_psNext) )
    if( strcmp( (*ppsCurrent)->ti_psBaseInterface->ni_zName, acName ) >= 0 )
      break;
  
  /* If an index should be appended, work out the next available */
  if( nIdxOffset > 0 ) {
    char acNumber[16];
    int  nNumber;
    
    /* Loop while the current interface exists and shares the stem */
    while( (*ppsCurrent) != NULL &&
           strncmp( acName, (*ppsCurrent)->ti_psBaseInterface->ni_zName,
                    nIdxOffset ) == 0 ) {
      
      char *pzCurNumber;
      
      /* Get the the current interface's index (as a string) */
      pzCurNumber = (*ppsCurrent)->ti_psBaseInterface->ni_zName + nIdxOffset;
      
      /**
       * Re-check that the name of the interface we are inspecting shares
       * only the stem we want to use (e.g. look at tun0 but not tunnel)
       */
      /* Skip an interface that matches the stem alone */
      if( *pzCurNumber == 0 ) {
        ppsCurrent = &((*ppsCurrent)->ti_psNext);
        continue;
      }
      
      /* Break if the interface does not share the stem after all */
      if( *pzCurNumber < '0' || *pzCurNumber > '9' )
        break;
      
      /* Compare number to current nIndex */
      if( atoi( pzCurNumber ) > nIndex )
        break;

      /* Increase nIdxOffset and look at next element in list */
      ++nIndex;
      ppsCurrent = &((*ppsCurrent)->ti_psNext);
    }
    
    /* Convert number to string (without overflowing) */
    p = acNumber + sizeof( acNumber ) - 1;
    *p = 0;
    
    if( nIndex > 0 ) {
      for( nNumber = nIndex; nNumber > 0 && --p >= acNumber; nNumber /= 10 )
        *p = '0' + nNumber % 10;
    } else {
      *--p = '0';
    }

    /* Concatenate */
    if( ((char *)acNumber) + sizeof(acNumber) + nIdxOffset  - p > IFNAMSIZ ) {
      kerndbg( KERN_DEBUG,
               "tun_check_name( \'%s\' ): Name too long \'%s%s\'\n",
               acName, acName, p );
      
      return -ENAMETOOLONG;
    }

    while( *p != 0 )
      acName[nIdxOffset++] = *p++;

    acName[nIdxOffset] = 0;
  }
  
  /**
   * Now check the names are not equal
   */
  /* If the name we are searching for is found, we have to return and error */
  if( (*ppsCurrent) != NULL &&
      strcmp( acName, (*ppsCurrent)->ti_psBaseInterface->ni_zName ) == 0 ) {
    kerndbg( KERN_DEBUG,
             "tun_check_name( \'%s\' ): Name already exists\n",
             acName );
    
    return -EEXIST;
  }
  
  /* Return the node to insert after to the caller */
  *pppsInsertAfter = ppsCurrent;

  return 0;
}

/**
 * Initialise the network interface.
 */
int
tun_create_if( TunInterface_s *psTun, char acName[IFNAMSIZ] ) {
  int            nError = 0;
  sem_id         lock = -1;
  TunInterface_s **ppsCurrent = NULL;
  
  if( psTun == NULL ) {
    kerndbg( KERN_DEBUG, "tun_create_if(): NULL pointer for psTun\n" );
    
    return -EINVAL;
  }
  
  if( psTun->ti_hMutex > 0 ) {
    kerndbg( KERN_DEBUG,
             "tun_create_if(): Attempting to re-initialise 0x%8.8lX\n",
             (uint32)psTun );
    
    return -EINVAL;  /* attempting to re-initialise */
  }

  kerndbg( KERN_DEBUG_LOW,
           "tun_create_if(): Initialise 0x%8.8lX with name \'%s\'\n",
           (uint32)psTun, acName );
  

  /**
   * Lock the interface list so that we can scan for duplicate names.
   * The list must remain locked until this interface is successfully
   * added or we are ready to give up.  As we scan the list, keep track
   * of position so that we can add in the appropriate location.
   */
  if( (nError = LOCK( g_hTunListMutex )) < 0 ) {
    kerndbg( KERN_FATAL,
             "tun_create_if( %s ): Failed to acquire list mutex (%d)\n",
             acName, nError );
    
    return nError;
  }
  
  if( (nError = tun_check_name( acName, &ppsCurrent )) < 0 )
    goto exit;
  
  /* Name is okay, continue with initialisation */
  
  /* Create a mutex to protect the tun interface */
  lock = create_semaphore( "tun interface lock", 1, 0 );
  
  if( lock < 0 ) {
    kerndbg( KERN_FATAL,
             "tun_create_if( %s ): Could not create interface mutex (%d)\n",
             acName, nError );

    nError = lock;
    goto exit;
  }

  /* Initialise the packet queue storing packets for the user process */
  if( (nError = init_net_queue( &psTun->ti_sToUserLand )) < 0 ) {
    kerndbg( KERN_FATAL,
             "tun_create_if( %s ): Could not initialise packet queue (%d)\n",
             acName, nError );

    goto exit;
  }
  
  /* Fill in the mutex field now that the other initialisation is done. */
  psTun->ti_hMutex = lock;
  
  /* Initialise the kernel interface */
  int nIndex = 0;
  if( (nIndex = add_net_interface( acName, &g_sInterfaceOps, psTun)) < 0 ) {
    kerndbg( KERN_FATAL,
             "tun_create_if( %s ): Could not add net interface (%d)\n",
             acName, nIndex );
    
    goto exit;
  }

  /* Link the kernel interface to the tun interface */
  psTun->ti_nIndex = nIndex;
  psTun->ti_psBaseInterface = get_net_interface( nIndex );

  /* Set the default MTU to be the same size as Ethernet payload */
  set_interface_mtu( psTun->ti_psBaseInterface, ETH_DATA_LEN );
  set_interface_flags( psTun->ti_psBaseInterface, IFF_UP | IFF_RUNNING | IFF_NOARP );
  
  /**
   * Add to the tun driver interface list.
   * The interfaces are added so that they are sorted by name/number.
   * This helps with finding free numbers.  This is handled in tun_check_name(),
   * we just use the pointer it returns as the position to insert at.
   */
  psTun->ti_psNext = (*ppsCurrent);
  (*ppsCurrent) = psTun;

  /* See eth_interface.c */
  release_net_interface( psTun->ti_psBaseInterface );

exit:
  UNLOCK( g_hTunListMutex );
  
  if( nError < 0 ) {
    /* Clean up */
    if( lock > 0 )
      delete_semaphore( lock );

    if( psTun->ti_sToUserLand.nq_hSyncSem > 0 )
      delete_net_queue( &psTun->ti_sToUserLand );

    memset( psTun, 0, sizeof( TunInterface_s ) );
  }
  
  return nError;
}

/**
 * Deactivates the interface.
 *
 * Note this function does not deallocate the memory pointed to by psTun.
 */
int
tun_delete_if( TunInterface_s *psTun ) {
  int              nError;
  TunInterface_s **psCurrent = NULL;
  
  if( psTun == NULL ) {
    kerndbg( KERN_DEBUG, "tun_delete_if(): NULL pointer for psTun\n" );
    
    return -EINVAL;
  }
  
  /**
   * Remove interface from list
   */
  if( (nError = LOCK( g_hTunListMutex )) < 0 ) {
    kerndbg( KERN_FATAL,
             "tun_delete_if( %s ): Failed to acquire list mutex (%d)\n",
             psTun->ti_psBaseInterface->ni_zName, nError );
    
    return nError;
  }
  
  for( psCurrent = &g_psTunListHead;
       (*psCurrent) != NULL && (*psCurrent) != psTun;
       psCurrent = &((*psCurrent)->ti_psNext) )
    ;
  
  if( (*psCurrent) != NULL ) {
    /* Unlink item */
    (*psCurrent) = psTun->ti_psNext;
    kerndbg( KERN_DEBUG_LOW,
             "tun_delete_if( %s ): Removed interface from global list.\n",
             psTun->ti_psBaseInterface->ni_zName );
  } else {
    kerndbg( KERN_WARNING,
             "tun_delete_if( %s ): Interface not found in global list!\n",
             psTun->ti_psBaseInterface->ni_zName );
  }
  
  /* Lock the interface */
  if( (nError = LOCK( psTun->ti_hMutex )) == 0 ) {
    sem_id lock = psTun->ti_hMutex;
    
    kerndbg( KERN_DEBUG_LOW,
             "tun_delete_if(): Delete 0x%8.8lX with name \'%s\'\n",
             (uint32)psTun, psTun->ti_psBaseInterface->ni_zName );
    
    /* Shutdown interface */
    del_net_interface( psTun->ti_psBaseInterface );

    /* Delete any stray packets destined for userland */

    /* Delete packet queue */
    delete_net_queue( &psTun->ti_sToUserLand );
    
    psTun->ti_hMutex = 0;
    
    delete_semaphore( lock );
  } else {
    kerndbg( KERN_DEBUG,
             "tun_delete_if(): Attempting to re-delete 0x%8.8lX\n",
             (uint32)psTun );
    
  }

  /* Unlock the interface list */
  UNLOCK( g_hTunListMutex );
  
  return nError;
}

/**
 * This function is part of the interface operations.  It appears it is never
 * actually called by the kernel.
 */
static void
tun_if_deinit( void* pInterface ) {
  return;
}

/**
 * Writes a packet back to the attached user process.
 *
 * This routine is called by the kernel from within send_packet() in
 * kernel/net/core.c.
 *
 * The packet buffer is passed in from the kernel and the interface code is
 * responsible for disposing of it once it has been sent.
 */
static int
tun_if_write( Route_s* psRoute, ipaddr_t pDstAddress, PacketBuf_s* psPkt ) {
  TunInterface_s  *psTun;
  int              nError;
  
  kerndbg( KERN_DEBUG_LOW,
           "tun_if_write( %s, %d.%d.%d.%d, 0x%8.8lX )\n",
           psRoute->rt_psInterface->ni_zName,
           (int)pDstAddress[0], (int)pDstAddress[1],
           (int)pDstAddress[2], (int)pDstAddress[3], (uint32)psPkt );

  /* Retrieve the tun interface pointer from the route record */
  psTun = (TunInterface_s *)(psRoute->rt_psInterface->ni_pInterface);
  
  if( psTun == NULL ) {
    kerndbg( KERN_DEBUG, "tun_if_write(): NULL pointer for psTun\n" );
    
    return -EINVAL;
  }
  
  if( (nError = LOCK( psTun->ti_hMutex )) < 0 )
    return nError;
  
  /* Enqueue the packet onto the ToUserLand queue */
  enqueue_packet( &psTun->ti_sToUserLand, psPkt );
  
  /* Release any waiting read requests */
  tun_wake_readers( psTun );

  UNLOCK( psTun->ti_hMutex );
  
  return 0;
}

static uint32 tun_get_if_flags( TunInterface_s *psTun );
static int tun_set_if_flags( TunInterface_s *psTun, uint32 nFlags );

/**
 * Handle an ioctl received through a socket.
 * 
 * All ioctls performed on sockets must have an argument that starts
 * with the interface name or the kernel dispatcher will not know which
 * interface should handle the ioctl.
 *
 * The data pointed to by pArg is still in user-space.
 */
static int
tun_if_ioctl( void *pInterface, int nCmd, void *pArg ) {
  TunInterface_s *psTun = (TunInterface_s *)pInterface;
  struct ifreq    sIFReq;

  if( psTun == NULL ) {
    kerndbg( KERN_DEBUG, "tun_if_ioctl(): NULL pointer for psTun\n" );
    
    return -EINVAL;
  }
  
  switch( nCmd ) {
    case SIOCSIFFLAGS:
    {
      if( memcpy_from_user( &sIFReq, pArg, sizeof( sIFReq ) ) < 0 )
        return -EFAULT;
      
      return tun_set_if_flags( psTun, sIFReq.ifr_flags );
    }
    case SIOCGIFFLAGS:
    {
      if( memcpy_from_user( &sIFReq, pArg, sizeof( sIFReq ) ) < 0 )
        return -EFAULT;

      sIFReq.ifr_flags = tun_get_if_flags( psTun );
      
      if( memcpy_to_user( pArg, &sIFReq, sizeof( sIFReq ) ) < 0 )
        return -EFAULT;

      printk( "SIOCGIFFLAGS done\n" );

      return 0;
    }
    default:
        break;
  }

  return -ENOSYS;
}


static uint32 
tun_get_if_flags( TunInterface_s *psTun ) {
  printk( "tun_get_if_flags()\n" );
  return psTun->ti_nFlags | IFF_RUNNING | IFF_NOARP;
}

static int
tun_set_if_flags( TunInterface_s *psTun, uint32 nFlags ) {
  uint32 nOldFlags = psTun->ti_nFlags;
  
  psTun->ti_nFlags = nFlags;

  /* Watch IFF_UP? */
  if( (nOldFlags & IFF_UP) != (nFlags & IFF_UP) ) {
    if( (nFlags & IFF_UP) == IFF_UP ) {
      /* Going up */
    } else {
      /* Going down */
    }
  }
  
  return 0;
}

static void tun_if_notify( void* pInterface )
{
  TunInterface_s *psTun = (TunInterface_s *)pInterface;
  NetInterface_s *psKernIf = psTun->ti_psBaseInterface;

  printk( "tun_if_notify()\n" );
}

