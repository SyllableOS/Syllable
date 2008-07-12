#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <atheos/types.h>
#include <atheos/threads.h>
#include <atheos/strace.h>
#include <atheos/syscalltable.h>

int parse_groups( char *zList );
void show_help( char* pzArgv0, FILE* stream );

#define NOP			0
#define TRACE		1<<0
#define STOP		1<<1
#define INCLUDE		1<<2
#define EXCLUDE		1<<3
#define RUN			1<<4

struct group_info_s {
	char* s_pzName;
	uint32 s_nMask;
	char* s_pzDescription;
};

struct group_info_s g_asGroups[] = {
	{ "mm", SYSC_GROUP_MM, "All system calls related to memory management e.g. sbrk()" },
	{ "proc", SYSC_GROUP_PROC, "Process related system calls e.g. fork()" },
	{ "device", SYSC_GROUP_DEVICE, "Device related system calls" },
	{ "net", SYSC_GROUP_NET, "Network related system calls e.g. recvmsg()" },
	{ "signal", SYSC_GROUP_SIGNAL, "Process signal related system calls e.g. kill()" },
	{ "ipc", SYSC_GROUP_IPC, "Interprocess Communication (IPC) related calls, e.g. create_msg_port()" },
	{ "io", SYSC_GROUP_IO, "Input/Ouput related system calls e.g. read()" },
	{ "debug", SYSC_GROUP_DEBUG, "Debugging system calls e.g. ptrace()" },
	{ "misc", SYSC_GROUP_MISC, "System calls that do not fit in any of the above catagories" },
	{ "all", SYSC_GROUP_ALL, "All of the above system call groups" },
	{ NULL, 0, NULL }
};

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
		show_help( argv[0], stderr );
		return( EXIT_FAILURE );
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
				
				case '?':	/* Show help */
				{
					show_help( argv[0], stderr );
					return( EXIT_FAILURE );
				}
				
				case '-':	/* Show help - explicitly check for --help */
				{
					if( !strcmp( "--help", argv[i] ) ) {
						show_help( argv[0], stderr );
						return( EXIT_FAILURE );
					}
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
	struct group_info_s* psGroup = g_asGroups;
	bool bFound;

	zGroup = strtok( zList, "," );
	do
	{
		psGroup = g_asGroups;
		bFound = false;
		while( psGroup->s_pzName != NULL ) {
			if( !strcmp( zGroup, psGroup->s_pzName ) ) {
				nMask |= psGroup->s_nMask;
				bFound = true;
				break;
			}
			psGroup++;
		}
		if( !bFound ) {
			fprintf( stderr, "Invalid group \"%s\"\n", zGroup );
		}
	}
	while( ( zGroup = strtok( NULL, "," ) ) != NULL );

	return nMask;
}

void show_help( char* pzArgv0, FILE* stream )
{
	struct group_info_s* psGroup = g_asGroups;
	
	fprintf( stream, "Usage : %s {-g|-e|-i|-o|-f} [-t thread_id] [-r ...]\n\n", pzArgv0 );
	fprintf( stream, "  -o: Switch tracing on.\n" );
	fprintf( stream, "  -f: Switch tracing off.\n" );
	fprintf( stream, "  -g: Select the syscall groups to include in the trace. The groups are:\n" );
	while( psGroup->s_pzName != NULL ) {
		fprintf( stream, "       %7s: %s\n", psGroup->s_pzName, psGroup->s_pzDescription );
		psGroup++;
	}
	fprintf( stream, "      More than one group can be passed by separating them with a comma e.g. -g net,io will select all system calls in the net and io syscall groups.\n" );
	fprintf( stream, "  -e: Exclude a given syscall from tracing. The syscall can be given by name, eg -e lock_semaphore will exclude all calls to lock_semaphore() from the trace.\n" );
	fprintf( stream, "  -i: Include the given syscall. The opposite of -e\n" );
	fprintf( stream, "  -t: The thread id (TID) to trace. You can use this to begin tracing an application that is already running.\n" );
	fprintf( stream, "  -r: Run an application with the supplied tracing arguments. You must specify the full path to the application you wish to run.\n" );
}
