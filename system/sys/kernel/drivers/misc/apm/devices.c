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
 *
 */

#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>

#include "apm.h"

int g_nCurrentDev = 0;
static SpinLock_s g_sSPinLock = INIT_SPIN_LOCK( "apm_slock" );

POWER_STATUS powerStatus;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t devs_open( void* pNode, uint32 nFlags, void **pCookie )
{
  struct RMREGS rm;
  uint32     nFlg;

  nFlg = spinlock_disable( &g_sSPinLock );

  memset( &rm, 0, sizeof( rm ) );

  rm.EAX = 0x5300;
  rm.EBX = 0;
  realint( 0x15, &rm );

  if ((rm.EBX & 0x00FFFF) != 0x504D) {
    printk("No APM BIOS is Found!\n");
    return (-EBUSY);
  }

  printk("APM BIOS Version : %c%c %d.%d\n",
	 ( int )((rm.EBX >> 8) & 0x00FF),
	 ( int )(rm.EBX & 0x00FF),
	 ( int )((rm.EAX >> 8) & 0x00FF),
	 ( int )(rm.EAX & 0x00FF));

  spinunlock_enable( &g_sSPinLock, nFlg );
    
  return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t devs_close( void* pNode, void* pCookie )
{
   
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t devs_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
  struct RMREGS rm;

  switch( nCommand ) {
  case 0:
    memset( &rm, 0, sizeof( rm ) );

    rm.EAX = 0x530A;
    rm.EBX = 1;
    realint( 0x15, &rm );

    powerStatus.ac_status       = (rm.EBX >> 8) & 0x00FF;
    powerStatus.battery_flag    = (rm.ECX >> 8) & 0x00FF;
    powerStatus.battery_percent = rm.ECX & 0x00FF;
    if ((rm.EDX & 0x8000) != 0)
      powerStatus.remain_seconds  = (rm.EDX & 0x007FFF) * 60;
    else
      powerStatus.remain_seconds  = rm.EDX & 0x007FFF;

    if (bFromKernel) {
      memcpy( pArgs, &powerStatus, sizeof(POWER_STATUS) );
      return (0);
    } else {
      return ( memcpy_to_user( pArgs, &powerStatus, sizeof(POWER_STATUS) ) );
    }

    /*
      if( get_device_info( ( DeviceInfo_s* )pArgs, g_nCurrentDev ) != 0 ) {
      g_nCurrentDev = 0;
      return( -1 );
      } else {
      g_nCurrentDev++;
      return( 0 );
      }	
      */
    break;

  default:
    printk( "apm_ioctl() invalid command %ld\n", nCommand );
    return( -EINVAL );
  }

  return( -1 );
}


DeviceOperations_s g_sOperations = {
    devs_open,
    devs_close,
    devs_ioctl,
    NULL,
    NULL
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
    int nError;

    nError = create_device_node( nDeviceID, -1, "apm", &g_sOperations, NULL );
    if ( nError < 0 ) {
	return( nError );
    }

    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
    return( 0 );
}




