// dhcpc : A DHCP client for Syllable
// (C)opyright 2002-2003 Kristian Van Der Vliet
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

#ifndef __F_DHCP_DHCP_H__
#define __F_DHCP_DHCP_H__

#include <time.h>
#include <stdio.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <atheos/types.h>
#include <atheos/semaphore.h>

enum op{
	DHCPNOP = 0,
	DHCPDISCOVER = 1,
	DHCPOFFER = 2 ,
	DHCPREQUEST = 3,
	DHCPDECLINE = 4,
	DHCPACK = 5,
	DHCPNAK = 6,
	DHCPRELEASE = 7
};

struct _DHCPSessionInfo{
	uint32 ciaddr;
	uint32 yiaddr;
	uint32 siaddr;
	uint32 giaddr;
	uint8 chaddr[16];
	uint8 sname[64];
	int hw_type;
	uint32 lease_time;
	uint32 subnetmask;

	uint32* dns_servers;
	uint8 dns_server_count;

	uint32* routers;
	uint8 router_count;

	uint32 last_xid;
	uint32 last_secs;
	uint8 attempts;

	uint32 t1_time;
	uint8 t1_state;
	uint32 t2_time;
	uint8 t2_state;
	uint32 t3_time;
	uint8 t3_state;

	int current_state;
	sem_id state_lock;
	time_t boot_time;

	uint32 timeout;

	uint8* if_hwaddress;
	char* if_name;

	int in_socket_fd;
	int out_socket_fd;
	struct sockaddr_in out_sin;	// For outbound socket
	struct sockaddr_in in_sin;	// For inbound socket

	bool do_shutdown;
};

typedef struct _DHCPSessionInfo DHCPSessionInfo_s;

enum timer_status{
	TIMER_STOPPED,
	TIMER_RUNNING,
	TIMER_INTERUPTED,
	TIMER_FINISHED
};

#define REMOTE_PORT 67		/* BOOTPS */
#define LOCAL_PORT 68	/* BOOTPC */

#define BOOT_REQUEST 1
#define BOOT_REPLY 2

#define DEFAULT_LEASE_MINS 60

#define RESPONSE_TIMEOUT	 60	// Wait one minute for a reply

void dhcp_start( void );

int dhcp_init( char* if_name );
int dhcp_shutdown( void );

#endif	/* __F_DHCP_DHCP_H__ */

