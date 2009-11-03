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

#ifndef __F_KERNEL_IF_TUN_H__
#define __F_KERNEL_IF_TUN_H__

#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/filesystem.h>
#include <kernel/net.h>
#include <kernel/packet.h>

#include <net/sockios.h>
#include <net/if_tun.h>

#include <posix/errno.h>

#include <macros.h>

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

#endif	/* __F_KERNEL_IF_TUN_H__ */
