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

#ifndef __F_KERNEL_IF_ETHER_H__
#define __F_KERNEL_IF_ETHER_H__

#include <net/if_ether.h>

#include <kernel/packet.h>
#include <posix/limits.h>

#define ETH_DEV_ID_LEN	16

typedef int eth_init( int nInterface, void** ppDev );
typedef int eth_deinit( void* pDev );
typedef int eth_get_device_id( char* pzIdString );
typedef int eth_start( void* pDev, NetQueue_s* psPktQueue );
typedef int eth_stop( void* pDev );
typedef int eth_get_hw_address( void* pDev, uint8* pAddress );
typedef int eth_send( void* pDev, const void* pBuffer, int nSize );

#define CURRENT_ETH_DRV_VERSION 1
typedef struct
{
    int						nVersion;
    eth_init*				init;
    eth_deinit*				deinit;
    eth_start*				start;
    eth_stop*				stop;
    eth_get_hw_address*		get_hw_address;
    eth_send*				send;
} EthDriverOps_s;

typedef int eth_probe( int* pnInterfaceCount, const EthDriverOps_s** ppsOps );

typedef struct _EthInterface EthInterface_s;
struct _EthInterface
{
	EthInterface_s* ei_psNext;
	NetInterface_s* ei_psInterface;
	int             ei_nIfIndex;
	NetQueue_s      ei_sInputQueue;
	uint8           ei_anHwAddr[IH_MAX_HWA_LEN];
	ArpEntry_s*     ei_psFirstARPEntry;
	void*           ei_pDevice;	// Private device-driver data
	int             ei_nDevice;
	thread_id       ei_hRxThread;
	uint32          ei_nOldFlags;
	char            ei_zDeviceID[ETH_DEV_ID_LEN];
	char            ei_zDriverPath[PATH_MAX];
};

void arp_in( EthInterface_s* psInterface, PacketBuf_s* psPkt );
int arp_send_packet( EthInterface_s* psInterface, PacketBuf_s* psPkt, ipaddr_t pAddress );
void arp_expire_interface( EthInterface_s* psInterface );

#endif	/* __F_KERNEL_IF_ETHER_H__ */
