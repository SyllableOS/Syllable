// dhcpc : A DHCP client for Syllable
// (C)opyright 2002-2003,2007 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __F_DHCP_PACKET_H__
#define __F_DHCP_PACKET_H__

#include <net/in.h>
#include <time.h>
#include <stdarg.h>

struct _DHCPPacket{
	uint8_t op;
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	uint32_t ciaddr;
	uint32_t yiaddr;
	uint32_t siaddr;
	uint32_t giaddr;
	uint8_t chaddr[16];
	uint8_t sname[64];
	uint8_t file[128];
	uint8_t options[312];
};

typedef struct _DHCPPacket DHCPPacket_s;

struct _DHCPOption{
	uint8_t type;
	uint16_t length;
	uint8_t value;
	uint8_t* data;
	struct _DHCPOption* next;
};

typedef struct _DHCPOption DHCPOption_s;

// Possible options
#define OPTION_NONE 0
#define OPTION_SUBNET 1
#define OPTION_ROUTERS 3
#define OPTION_DNSSERVERS 6
#define OPTION_NTPSERVERS 42
#define OPTION_REQUESTEDIP 50
#define OPTION_LEASETIME 51
#define OPTION_MESSAGE 53
#define OPTION_SERVERID 54
#define OPTION_PARLIST 55
#define OPTION_SERVER_MESSAGE 56
#define OPTION_T1_TIME 58
#define OPTION_T2_TIME 59
#define OPTION_CLIENTID 61
#define OPTION_END 255


DHCPPacket_s* build_packet( bool is_broadcast, uint32_t client, uint32_t server, uint32_t gateway, uint8_t* hwaddress, clock_t boot_time );
uint32_t getxid( void );

int setoptions( DHCPPacket_s* packet, DHCPOption_s *options );
DHCPOption_s* parseoptions( uint8_t* buffer, size_t len );
int freeoptions( DHCPOption_s* options );

#endif		/* __F_DHCP_PACKET_H__ */

