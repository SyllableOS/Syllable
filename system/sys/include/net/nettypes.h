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

#ifndef __F_ATHEOS_NETTYPES_H__
#define __F_ATHEOS_NETTYPES_H__

#include <atheos/types.h>

typedef struct _Socket Socket_s;
typedef struct _NetInterface NetInterface_s;
typedef struct _EthernetHeader EthernetHeader_s;
typedef struct _ArpHeader ArpHeader_s;
typedef struct _IpHeader IpHeader_s;
typedef struct _ICMPHeader ICMPHeader_s;
typedef struct _TCPHeader TCPHeader_s;
typedef struct _UDPHeader UDPHeader_s;
typedef struct _TCPCtrl TCPCtrl_s;
typedef struct _UDPEndPoint UDPEndPoint_s;
typedef struct _UDPPort UDPPort_s;
typedef struct _RawEndPoint RawEndPoint_s;
typedef struct _RawPort RawPort_s;

typedef struct _ArpEntry ArpEntry_s;

typedef struct _NetQueue NetQueue_s;
typedef struct _PacketBuf PacketBuf_s;

#define IP_ADR_LEN	4
typedef uint8 ipaddr_t[IP_ADR_LEN];
typedef uint8* ipaddr_p;
#endif				/* __F_ATHEOS_NETTYPES_H__ */
