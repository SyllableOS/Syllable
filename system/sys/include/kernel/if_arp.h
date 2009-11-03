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

#ifndef __F_KERNEL_IF_ARP_H__
#define __F_KERNEL_IF_ARP_H__

#include <syllable/inttypes.h>
#include <net/if_arp.h>

struct _ArpHeader
{
	uint16 ar_nHardAddrFormat;	/* format of hardware address	*/
	uint16 ar_nProtoAddrFormat;	/* format of protocol address	*/
	uint8 ar_nHardAddrSize;		/* length of hardware address	*/
	uint8 ar_nProtoAddrSize;	/* length of protocol address	*/
	uint16 ar_nCommand;			/* ARP opcode (command)		*/

    /*** The following part are actually variable sized so pleas remove them some time ***/
  
	uint8 ar_anHwSender[6];		/* sender hardware address	*/
	ipaddr_t ar_anIpSender;		/* sender IP address		*/
	uint8 ar_anHwTarget[6];		/* target hardware address	*/
	ipaddr_t ar_anIpTarget;		/* target IP address		*/
};

#endif	/* __F_KERNEL_IF_ARP_H__ */
