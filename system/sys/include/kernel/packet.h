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

#ifndef __F_KERNEL_PACKET_H__
#define __F_KERNEL_PACKET_H__

#include <net/nettypes.h>

struct _PacketBuf
{
	PacketBuf_s*    pb_psNext;
	PacketBuf_s*    pb_psPrev;
	int				pb_nIfIndex;
	int	       	    pb_nRealSize;
	int	       	    pb_nSize;
	int	       	    pb_nProtocol;  /* IP/ARP/... */
	bool			pb_bLocal; /* From loop-back, don't check check-sum */
	/* Transport layer header */
	union
	{
		ICMPHeader_s*  psIcmp;
		TCPHeader_s*   psTCP;
		UDPHeader_s*   psUDP;
		uint8*	pRaw;
	} pb_uTransportHdr;

	/* Network layer header */
	union
	{
		IpHeader_s*	 psIP;
		ArpHeader_s* psArp;
		uint8*	 pRaw;
	} pb_uNetworkHdr;
  
	/* Link layer header */
	union 
	{	
		EthernetHeader_s*	psEthernet;
		uint8*		pRaw;
	} pb_uMacHdr;

	uint8*	       pb_pHead;
	uint8*	       pb_pData;
	uint32	       pb_nMagic;
};

struct _NetQueue
{
	PacketBuf_s* nq_psHead;
	PacketBuf_s* nq_psTail;
	int	       nq_nCount;
	sem_id       nq_hSyncSem;
	sem_id       __nq_hProtSem;
};

#endif /* __F_KERNEL_PACKET_H__ */
