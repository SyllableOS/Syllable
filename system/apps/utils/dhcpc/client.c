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
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>
#include <signal.h>
#include <fcntl.h>

FILE *logfile = NULL;

int dhcp_run( const char *if_name, bool do_ntp );
static void sighandle( int signal );

int main(int argc, char *argv[])
{
	bool deamonize, do_ntp;
	int c, n, if_count, children, child_status;
	struct sigaction sa;
	const char *logfile_name = "/var/log/dhcpc";
	logfile = stderr;	/* Redirect to stderr until we have opened the log file */

	if( argc < 2 )
	{
		debug(PANIC,"Usage","%s [-d] [-n] [-l logfile] interface [...]\n",argv[0]);
		return(0);
	}

	deamonize = do_ntp = false;
	while( ( c = getopt( argc, argv, "dnl:" ) ) != -1 )
		switch( c )
		{
			case 'd':
			{
				deamonize = true;
				break;
			}

			case 'n':
			{
				do_ntp = true;
				break;
			}

			case 'l':
			{
				logfile_name = optarg;
				break;
			}
		}
	children = if_count = ( argc - optind );

	if( deamonize )
	{
		if( fork() != 0 )
			return 0;

		setpgrp();
		for( n = 0; n < 3; n++ )
			close( n );

		umask( 027 );
		chdir( "/tmp" );
	}

	logfile = fopen( logfile_name, "a" );
	if( logfile < 0 )
	{
		logfile = stderr;	/* Redirect to stderr in the absense of a log file */
		debug( INFO, __FUNCTION__, "failed to open log file \"%s\"\n", logfile );
	}

	/* Setup signal handler */
	signal( SIGINT, SIG_IGN );	/* Ignore interrupt (Ctrl+C) */
	signal( SIGTERM, SIG_IGN );	/* Only the children should catch SIGTERM */

	/* The children will send us SIGUSR1 to indicate that it has successfully completed
	   the configuration of it's interface */
	sa.sa_handler = sighandle;
	sigemptyset( &sa.sa_mask );
	sa.sa_flags = 0;

	sigaction( SIGUSR1, &sa, NULL );

	debug( INFO, __FUNCTION__, "%d interfaces to configure\n", if_count );
	while( if_count-- )
	{
		const char *if_name = argv[optind++];
		pid_t child;

		child = fork();
		if( child == 0 )
		{
			/* One child per interface */
			dhcp_run( if_name, do_ntp );
		}
		else
		{
			/* Wait for the child to complete before we attempt to configure the next interface.
			   The child will either send us SIGUSR1 to indicate it has completed configuration,
			   or it will exit with a failure. */

			waitpid( child, &child_status, 0 );
			if( WIFEXITED( child_status ) && ( WEXITSTATUS( child_status ) == EXIT_FAILURE ) )
				--children;
		}
	}

	debug( INFO, __FUNCTION__, "waiting for %d children.\n", children );

	while( children )
	{
		wait( &child_status );

		if( WIFEXITED( child_status ) )
			children--;
	}

	debug( INFO, __FUNCTION__, "parent exiting\n" );

	return 0;
}

int dhcp_run( const char *if_name, bool do_ntp )
{
	DHCPSessionInfo_s* session;

	debug( INFO, __FUNCTION__, "configuring interface %s\n", if_name );

	if( dhcp_init( if_name, &session ) != EOK )
	{
		debug(PANIC,__FUNCTION__,"dhcp_init() failed for interface %s\n", if_name );
		exit( EXIT_FAILURE );
	}

	session->do_ntp = do_ntp;

	dhcp_start( session );
	dhcp_shutdown( session );

	exit( 0 );
}

static void sighandle( int signal )
{
	/* EMPTY */
}

