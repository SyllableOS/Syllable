#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <atheos/types.h>
#include <atheos/threads.h>
#include <atheos/strace.h>

int parse_groups( char *zList );

#define NOP			0
#define TRACE		1<<0
#define STOP		1<<1
#define INCLUDE		1<<2
#define EXCLUDE		1<<3
#define RUN			1<<4

int main( int argc, char* argv[] )
{
	int i, nError, nMask, nOption;
	char *zGrouparg, *zGrouplist, *zExclude, *zInclude;
	thread_id hThread = -1;
	int nFirstChildArg, nChildArgCount;

	zGrouparg = zGrouplist = zExclude = zInclude = NULL;
	nMask = -1;
	nError = EXIT_SUCCESS;
	nOption = NOP;

 	if( argc == 1 )
	{
		fprintf( stderr, "Usage : %s {-g|-e|-i|-o|-f} [-t thread_id] [-r ...]\n", argv[0] );
		nError = EXIT_FAILURE;
		goto error;
	}

	for( i = 1; i < argc; i++ )
	{
		if( argv[i][0] == '-' )
		{
			switch( argv[i][1] )
			{
				case 'g':	/* Groups */
				{
					zGrouparg = strdup( argv[++i] );
					zGrouplist = strdup( zGrouparg );

					nMask = parse_groups( zGrouplist );
					if( SYSC_GROUP_NONE == nMask )
					{
						fprintf( stderr, "\"%s\" is not a valid group\n", zGrouparg );
						nError = EXIT_FAILURE;
						goto error;
					}

					break;
				}

				case 'e':	/* Exclude */
				{
					zExclude = strdup( argv[++i] );
					nOption |= EXCLUDE;
					break;
				}

				case 'i':	/* Include */
				{
					zInclude = strdup( argv[++i] );
					nOption |= INCLUDE;
					break;
				}

				case 't':	/* Thread id */
				{
					sscanf( argv[++i], "%d", &hThread );
					break;
				}

				case 'o':	/* Tracing on */
				{
					nOption |= TRACE;
					break;
				}

				case 'f':	/* Tracing off */
				{
					nMask = SYSC_GROUP_NONE;
					nOption = STOP;	/* Override the current mask */
					break;
				}

				case 'r':	/* Run */
				{
					nFirstChildArg = ++i;
					nChildArgCount = argc - nFirstChildArg;
					nOption = RUN;	/* Override the current mask */

					i = argc;	/* Do not process any further args, they belong to the child process */
					break;	
				}
				
				default:
				{
					fprintf( stderr, "Unknown option '%s'\n", argv[i] );
				}
			}
		}

	}

	if( NULL == zGrouparg && ( TRACE == nOption || RUN == nOption ) )
	{
		fprintf( stderr, "No groups specified, assuming \"all\"\n" );
		nMask = SYSC_GROUP_ALL;
	}

	if( nOption & TRACE || nOption & STOP )
	{
		if( get_thread_proc( hThread ) != -1 )
		{
			fprintf( stderr, "Tracing thread %d, group mask 0x%.4x\n", hThread, nMask );
			strace( hThread, nMask, 0 );
		}
		else
		{
			fprintf( stderr, "Thread %d does not exist\n", hThread );
			nError = EXIT_FAILURE;
			goto error;
		}
	}

	if( nOption & RUN )
	{
		int nArg;
		pid_t hPid;
		char **pnArgs = malloc( ( nChildArgCount + 1 ) * sizeof( char* ) );
		
		for( nArg = 0; nArg < nChildArgCount; nArg++ )
			pnArgs[nArg] = argv[nFirstChildArg + nArg];
		pnArgs[nChildArgCount] = NULL;

		hPid = fork();
		if( hPid == 0 )
		{
			int nChildError;
			
			nChildError = execvp( pnArgs[0], pnArgs );
			if( nChildError < 0 )
				fprintf( stderr, "Failed to exec %s\n", pnArgs[0] );
		}
		else
		{
			/* In Syllable the pid returned by fork() is actually the thread id.. */
			strace( hPid, nMask, 0 );
		}
		
		/* Wait for the child to exit */
		waitpid( hPid, NULL, 0 );
		free( pnArgs );
	}
	
	if( nOption & INCLUDE )
	{
		char *zSyscallName;

		if( get_thread_proc( hThread ) < 0 )
		{
			fprintf( stderr, "Thread %d does not exist\n", hThread );
			nError = EXIT_FAILURE;
			goto error;
		}

		zSyscallName = strtok( zInclude, "," );
		do
		{
			for( i = 0; i < ( __NR_TrueSysCallCount - 1 ); i++ )
				if( !strcmp( zSyscallName, g_sSysCallTable[ i ].zName ) )
				{
					if( strace_include( hThread, g_sSysCallTable[ i ].nNumber ) < 0 )
						fprintf( stderr, "Failed to include syscall \"%s\" for thread %d\n", g_sSysCallTable[ i ].zName, hThread );
					break;
				}

				if( i == ( __NR_TrueSysCallCount - 1 ) )
					fprintf( stderr, "No such syscall \"%s\"\n", zSyscallName );
		}
		while( ( zSyscallName = strtok( NULL, "," ) ) != NULL );
	}

	if( nOption & EXCLUDE )
	{
		char *zSyscallName;

		if( get_thread_proc( hThread ) < 0 )
		{
			fprintf( stderr, "Thread %d does not exist\n", hThread );
			nError = EXIT_FAILURE;
			goto error;
		}

		zSyscallName = strtok( zExclude, "," );
		do
		{
			for( i = 0; i < ( __NR_TrueSysCallCount - 1 ); i++ )
				if( !strcmp( zSyscallName, g_sSysCallTable[ i ].zName ) )
				{
					if( strace_exclude( hThread, g_sSysCallTable[ i ].nNumber ) < 0 )
						fprintf( stderr, "Failed to exclude syscall \"%s\" for thread %d\n", g_sSysCallTable[ i ].zName, hThread );
					break;
				}

			if( i == ( __NR_TrueSysCallCount - 1 ) )
				fprintf( stderr, "No such syscall \"%s\"\n", zSyscallName );
		}
		while( ( zSyscallName = strtok( NULL, "," ) ) != NULL );
	}

error:
	free( zGrouparg );
	free( zGrouplist );
	free( zInclude );
	free( zExclude );
	return nError;
}

int parse_groups( char *zList )
{
	int nMask = SYSC_GROUP_NONE;
	char *zGroup;

	zGroup = strtok( zList, "," );
	do
	{
		if( !strcmp( zGroup, "mm" ) )
		{
			nMask |= SYSC_GROUP_MM;
		}
		else if( !strcmp( zGroup, "proc" ) )
		{
			nMask |= SYSC_GROUP_PROC;
		}
		else if( !strcmp( zGroup, "device" ) )
		{
			nMask |= SYSC_GROUP_DEVICE;
		}
		else if( !strcmp( zGroup, "net" ) )
		{
			nMask |= SYSC_GROUP_NET;
		}
		else if( !strcmp( zGroup, "signal" ) )
		{
			nMask |= SYSC_GROUP_SIGNAL;
		}
		else if( !strcmp( zGroup, "ipc" ) )
		{
			nMask |= SYSC_GROUP_IPC;
		}
		else if( !strcmp( zGroup, "io" ) )
		{
			nMask |= SYSC_GROUP_IO;
		}
		else if( !strcmp( zGroup, "debug" ) )
		{
			nMask |= SYSC_GROUP_DEBUG;
		}
		else if( !strcmp( zGroup, "misc" ) )
		{
			nMask |= SYSC_GROUP_MISC;
		}
		else if( !strcmp( zGroup, "all" ) )
		{
			nMask |= SYSC_GROUP_ALL;
		}
		else
		{
			fprintf( stderr, "Invalid group \"%s\"\n", zGroup );
		}
	}
	while( ( zGroup = strtok( NULL, "," ) ) != NULL );

	return nMask;
}


