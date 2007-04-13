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

#ifndef __F_DHCP_INBOUND_H__
#define __F_DHCP_INBOUND_H__

#include <dhcp.h>
#include <packet.h>

void process_dhcpoffer( DHCPSessionInfo_s *info, DHCPPacket_s *in_packet, DHCPOption_s *options );
void process_dhcpack( DHCPSessionInfo_s *info, DHCPPacket_s *in_packet, DHCPOption_s *options );
void process_dhcpnak( DHCPSessionInfo_s *info, DHCPPacket_s *in_packet, DHCPOption_s *options );

#endif		// __F_DHCP_INBOUND_H__
