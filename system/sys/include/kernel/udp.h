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

#ifndef __F_KERNEL_UDP_H__
#define __F_KERNEL_UDP_H__

#include <net/nettypes.h>

#include <kernel/if.h>
#include <kernel/route.h>

struct _UDPHeader
{
  uint16 udp_nSrcPort;
  uint16 udp_nDstPort;
  uint16 udp_nSize;
  uint16 udp_nChkSum;
};

struct _UDPEndPoint
{
    UDPEndPoint_s*   ue_psNext;
    UDPPort_s*	   ue_psPort;
    ipaddr_t	   ue_anLocalAddr;
    ipaddr_t	   ue_anRemoteAddr;
    uint16	   ue_nLocalPort;
    uint16	   ue_nRemotePort;
    bool	   ue_bNonBlocking;
    NetQueue_s	   ue_sPackets;
    SelectRequest_s* ue_psFirstReadSelReq;
    SelectRequest_s* ue_psFirstWriteSelReq;
    SelectRequest_s* ue_psFirstExceptSelReq;
		char ue_acBindToDevice[IFNAMSIZ];
};

struct _UDPPort
{
  UDPPort_s* 	 up_psNext;
  UDPEndPoint_s* up_psFirstEndPoint;
  uint16     	 up_nPortNum;
  bool		 up_bHasSrcAddrWC;
  bool		 up_bHasSrcPortWC;
};

int init_udp( void );

int udp_open( Socket_s* psSocket );

#endif	/* __F_KERNEL_UDP_H__ */
