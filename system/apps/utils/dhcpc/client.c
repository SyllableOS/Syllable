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

#include "dhcp.h"
#include "debug.h"

#include <posix/errno.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if( argc < 2 )
	{
		debug(PANIC,"Usage","%s [-d] interface\n",argv[0]);
		return(0);
	}

	if( !strcmp(argv[1],"-d") )
	{
		if( fork() == 0 )
		{
			if( dhcp_init( argv[2] ) == EOK )
			{
				dhcp_start();		// Start the DHCP client
				dhcp_shutdown();	// Finish and close gracefully
			}
			else
			{
				debug(PANIC,__FUNCTION__,"dhcp_init() failed\n");
				return( 1 );
			}
		}
		else
			return(0);
	}
	else
	{
		if( dhcp_init( argv[1] ) == EOK )
		{
			dhcp_start();		// Start the DHCP client
			dhcp_shutdown();	// Finish and close gracefully
		}
		else
		{
			debug(PANIC,__FUNCTION__,"dhcp_init() failed\n");
			return( 1 );
		}
	}

	return( 0 );
}

