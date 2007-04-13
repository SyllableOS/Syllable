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

#ifndef __F_DHCP_INTERFACE_H__
#define __F_DHCP_INTERFACE_H__

#include <dhcp.h>

int get_mac_address( DHCPSessionInfo_s *info );
int bringup_interface( DHCPSessionInfo_s *info );
int setup_interface( DHCPSessionInfo_s *info );
int setup_route( DHCPSessionInfo_s *info, int if_socket );
int setup_resolver( DHCPSessionInfo_s *info );

char* format_ip(uint32_t addr);

#endif		// __F_DHCP_INTERFACE_H__

