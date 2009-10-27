/* unbuffer : (C)opyright 2009 Kristian Van Der Vliet
 * 
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pty.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>

#define PTY_BUF_SIZE	256

int main( int argc, char *argv[] )
{
	int mst_pty, slv_pty;
	int child_argc, n;
	char **child_argv;
	pid_t child_pid;
	char pty_buf[PTY_BUF_SIZE];
	ssize_t pty_bytes;

	/* Shift argv down to get the child invocation */
	child_argc = argc - 1;
	child_argv = malloc( child_argc );

	for( n=0; n < child_argc; n++ )
		child_argv[n] = argv[n+1];

	/* Create a PTY */
	if( openpty( &mst_pty, &slv_pty, NULL, NULL, NULL ) < 0 )
	{
		perror( "openpty failed" );
		exit( EXIT_FAILURE );
	}

	/* Fork the child that will run on the slave PTY */
	child_pid = fork();
	if( child_pid == 0 )
	{
		/* Attach stdout & stderr to the PTY */
		if( dup2( slv_pty, STDOUT_FILENO ) < 0 )
		{
			perror( "dup2 failed for stdout" );
			exit( EXIT_FAILURE );
		}

		if( dup2( slv_pty, STDERR_FILENO ) < 0 )
		{
			perror( "dup2 failed for stderr" );
			exit( EXIT_FAILURE );
		}

		/* Run the child process */
		if( execvp( child_argv[0], child_argv ) < 0 )
			exit( EXIT_FAILURE );
	}
	else if ( child_pid < 0 )
	{
		perror( "fork failed" );
		exit( EXIT_FAILURE );
	}

	/* We are the master so we do not need to slave PTY handle */
	close( slv_pty );	

	/* Make our stdout unbuffered */
	setvbuf( stdout, 0, _IONBF, 0 );

	/* Read from the master & write to our stdout until EOF */
	while( ( pty_bytes = read( mst_pty, pty_buf, PTY_BUF_SIZE ) ) >= 0 )
		write( STDOUT_FILENO, pty_buf, pty_bytes );

	/* Wait for the child to exit */
	waitpid( child_pid, NULL, 0 );

	close( mst_pty );
	return EXIT_SUCCESS;
}
