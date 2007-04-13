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

#include <dhcp.h>
#include <debug.h>
#include <errno.h>
#include <stdlib.h>

static pid_t ntpd = -1;

#define MSNTP_PATH	"/usr/indexes/bin/msntp"

int start_ntpd( DHCPSessionInfo_s* info )
{
	if( ntpd >= 0 )
	{
		debug( INFO, __FUNCTION__, "NTP deamon already running\n" );
		return EINPROGRESS;
	}

	if( info->ntp_server_count <= 0 )
	{
		debug( INFO, __FUNCTION__, "no NTP servers available\n" );
		return EINVAL;
	}

	debug( INFO, __FUNCTION__, "starting NTP deamon\n" );

	ntpd = fork();
	if( ntpd == 0 )
	{
		char *ntp_server = format_ip(info->ntp_servers[0]);
		char *ntp_argv[] =
			{ MSNTP_PATH, "-a", "-x", "-P", "no", "-l", "/var/lock/msntp", ntp_server, NULL };

		debug( INFO, __FUNCTION__, "syncing against NTP server at %s\n", ntp_server );

		if( execv( MSNTP_PATH, &ntp_argv ) < 0 )
		{
			debug( PANIC, __FUNCTION__, "failed to start NTP deamon\n" );
			free( ntp_server );
			exit( EXIT_FAILURE );
		}
	}

	return EOK;
}

