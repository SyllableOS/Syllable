/*
 *  
 *  Tun Driver for Syllable
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
 */

#ifndef SYLLABLE_IF_TUN_H
#define SYLLABLE_IF_TUN_H

#include <atheos/types.h>
#include <atheos/device.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>

#include <net/net.h>
#include <net/packet.h>
#include <net/sockios.h>

#include <posix/errno.h>


#include <macros.h>

/**
 * Constants
 */
/**
 * ioctl commands:
 *   TUN_IOC_CREATE_IF:         Creates the associated interface.
 *                              Expects a pointer to a name
 *                              (null-terminated, IFNAMSIZ bytes total).
 *                              The name must comprise characters from a-z only,
 *                              except that a name ending with + will have an
 *                              index inserted in place of the + to avoid name
 *                              clashes.  Otherwise the name must be unique.
 *   
 *   TUN_IOC_DELETE_IF:         Deletes the associated interface.
 *                              This not necessary -- calling close() will do
 *                              it automatically.  No arguments needed.
 */
#define TUN_IOC_CREATE_IF       0x89370000 /* Intentionally == SIOC_ETH_START */
#define TUN_IOC_DELETE_IF       0x89370001

#ifdef __KERNEL__

/**
 * This structure is stored as the cookie for an opened device node and
 * as the private data (ni_pInterface) for the network interface.
 */
typedef struct _TunInterface TunInterface_s;

struct _TunInterface {
  TunInterface_s    *ti_psNext;           /* NB: Locked by g_hTunListMutex */
  sem_id             ti_hMutex;
  NetInterface_s    *ti_psBaseInterface;
  int                ti_nIndex;
  NetQueue_s         ti_sToUserLand;
  SelectRequest_s   *ti_psReaders;        /* Linked list of waiters */
  SelectRequest_s   *ti_psWriters;        /* Linked list of waiters */
  uint32             ti_nFlags;           /* Network interface flags */
  int                ti_nMode;            /* File descriptor mode */
};


/**
 * Globals
 */
extern TunInterface_s *g_psTunListHead;
extern sem_id          g_hTunListMutex;
extern int             g_hDeviceNode;

/**
 * Interface Prototypes
 */
void tun_wake_readers( TunInterface_s *psTun );
void tun_wake_writers( TunInterface_s *psTun );

int tun_create_if( TunInterface_s *psTun, char acName[IFNAMSIZ] );
int tun_delete_if( TunInterface_s *psTun );

#endif

#endif
