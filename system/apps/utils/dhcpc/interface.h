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

#ifndef __F_DHCP_INTERFACE_H__
#define __F_DHCP_INTERFACE_H__

#include <atheos/types.h>

int get_mac_address( char* if_name );
int bringup_interface( char* if_name );
int setup_interface( void );
int setup_route( int if_socket );
int setup_resolver( void );

char* format_ip(uint32 addr);

#endif		// __F_DHCP_INTERFACE_H__

