
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <posix/fcntl.h>
#include <posix/errno.h>
#include <posix/limits.h>

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/smp.h>
#include <atheos/elf.h>
#include <atheos/syscall.h>
#include <atheos/irq.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/global.h"
#include "inc/areas.h"
#include "inc/image.h"
#include "inc/mman.h"
#include "inc/intel.h"
#include "inc/smp.h"

#define MAX_ARGS_PER_NODE 500

typedef struct _ArgCopy ArgCopy_s;
struct _ArgCopy
{
	ArgCopy_s *psNext;
	int nCount;
	int nBufSize;
	char *pBuffer;
	char *apzArgs[MAX_ARGS_PER_NODE];
	int anLengths[MAX_ARGS_PER_NODE];
};

static void free_arg_copy( ArgCopy_s *psNode )
{
	while ( psNode != NULL )
	{
		ArgCopy_s *psTmp = psNode->psNext;

		if ( psNode->pBuffer != NULL )
		{
			kfree( psNode->pBuffer );
		}
		kfree( psNode );
		psNode = psTmp;
	}
}

static ArgCopy_s *copy_arg_list( char *const *argv, int *pnArgCnt, int *pnArgSize )
{
	int nArgSize = sizeof( char * );
	int nArgCount = 0;
	ArgCopy_s *psNode;
	ArgCopy_s *psFirstNode;
	int nError;
	int i;
	
	if( argv == NULL )
	{
		*pnArgCnt = 0;
		*pnArgSize = 0;
		return( NULL );
	}
	
	psNode = kmalloc( sizeof( ArgCopy_s ), MEMF_KERNEL | MEMF_OKTOFAIL );
	psFirstNode = psNode;

	if ( psNode == NULL )
	{
		printk( "Error: copy_arg_list() no memory for base node\n" );
		return ( NULL );
	}
	psNode->psNext = NULL;
	psNode->nCount = 0;
	psNode->nBufSize = 0;
	psNode->pBuffer = NULL;
	for ( i = 0;; ++i )
	{
		if ( psNode->nCount == MAX_ARGS_PER_NODE )
		{
			psNode->psNext = kmalloc( sizeof( ArgCopy_s ), MEMF_KERNEL | MEMF_OKTOFAIL );
			if ( psNode->psNext == NULL )
			{
				free_arg_copy( psFirstNode );
				printk( "Error: copy_arg_list() no memory for extention node\n" );
				return ( NULL );
			}
			psNode = psNode->psNext;
			psNode->psNext = NULL;
			psNode->nCount = 0;
			psNode->nBufSize = 0;
			psNode->pBuffer = NULL;
		}
		nError = memcpy_from_user( &psNode->apzArgs[psNode->nCount], &argv[i], sizeof( *argv ) );
		if ( nError < 0 )
		{
			free_arg_copy( psFirstNode );
			printk( "Error: copy_arg_list() failed to copy arg pointer %d\n", i );
			return ( NULL );
		}
		if ( psNode->apzArgs[psNode->nCount] == NULL )
		{
			break;
		}
		psNode->anLengths[psNode->nCount] = strlen_from_user( psNode->apzArgs[psNode->nCount] );
		if ( psNode->anLengths[psNode->nCount] < 0 )
		{
			free_arg_copy( psFirstNode );
			printk( "Error: copy_arg_list() failed get length of arg %d\n", i );
			return ( NULL );
		}
		psNode->nBufSize += psNode->anLengths[psNode->nCount] + 1;
		psNode->nCount++;
	}

	for ( psNode = psFirstNode; psNode != NULL; psNode = psNode->psNext )
	{
		char *pzDst;

		psNode->pBuffer = kmalloc( psNode->nBufSize, MEMF_KERNEL | MEMF_OKTOFAIL );
		if ( psNode->pBuffer == NULL )
		{
			free_arg_copy( psFirstNode );
			printk( "Error: copy_arg_list() no memory for string buffer\n" );
			return ( NULL );
		}
		pzDst = psNode->pBuffer;
		for ( i = 0; i < psNode->nCount; ++i )
		{
			if ( strncpy_from_user( pzDst, psNode->apzArgs[i], psNode->anLengths[i] + 1 ) != psNode->anLengths[i] )
			{
				free_arg_copy( psFirstNode );
				printk( "Error: copy_arg_list() failed to copy arg string %d\n", i );
				return ( NULL );
			}
			psNode->apzArgs[i] = pzDst;
			pzDst += psNode->anLengths[i] + 1;

			nArgSize += psNode->anLengths[i] + 1 + sizeof( char * );	// string + NULL termination + pointer
			nArgCount++;
		}
	}
	*pnArgSize = nArgSize;
	*pnArgCnt = nArgCount;
	return ( psFirstNode );
}


static uint32 *init_stack( uint32 *pnStack, char **argv, char **envv )
{
	uint32 anBuffer[] = { 0x12345678, ( uint32 )argv, ( uint32 )envv, ( uint32 )( pnStack + 1 ) };

	memcpy_to_user( pnStack - 3, anBuffer, 4 * sizeof( int ) );
	return ( pnStack - 3 );
}

int sys_execve( const char *a_pzPath, char *const *argv, char *const *envv )
{
	SysCallRegs_s *psRegs = ( SysCallRegs_s * ) & a_pzPath;
	Thread_s *psThread;
	Process_s *psProc;
	ElfImageInst_s *psImageInst;
	int nFile;
	int nEnvCount;
	int nEnvSize;
	ArgCopy_s *psEnvCopy = NULL;
	ArgCopy_s *psArgCopy = NULL;
	ArgCopy_s *psArgNode;
	int nArgCount;
	int nArgSize;
	const char *pzLibraryPath;
	char *pzPath;
	int nError;
	char zCommand[OS_NAME_LENGTH];
	area_id nStackArea;
	AreaInfo_s sAreaInfo;
	uint32 *plUserStack;
	uint32 *plEntryStack;
	uint32 nFlg;
	struct stat sStat;
	int *pErrnoPtr;
	int i;

#ifdef __PROFILE_EXEC
	int64 nStartTime;
	int64 nEndTime;
	int64 nTime1;
	int64 nTime2;
	int64 nTime3;
	int64 nTime4;
	int64 nTime5;
	int64 nTime6;
	int64 nTime7;
	int64 nTime8;
	int64 nTime9;

	nStartTime = read_pentium_clock();
	nTime1 = nStartTime;
#endif
	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );

	if ( nError < 0 )
	{
		return ( nError );
	}

	psThread = CURRENT_THREAD;
	psProc = psThread->tr_psProcess;

	nFile = open( pzPath, O_RDONLY );

	if ( nFile < 0 )
	{
		kfree( pzPath );
		return ( nFile );
	}
	nError = fstat( nFile, &sStat );
	if ( nError < 0 )
	{
		printk( "Error: sys_execve() failed to stat %s\n", pzPath );
		close( nFile );
		kfree( pzPath );
		return ( nError );
	}
	close( nFile );
	if ( S_ISDIR( sStat.st_mode ) )
	{
		kfree( pzPath );
		return ( -EISDIR );
	}
	if ( !S_ISREG( sStat.st_mode ) )
	{
		kfree( pzPath );
		return ( -EINVAL );
	}

#ifdef __PROFILE_EXEC
	nTime1 = read_pentium_clock();
#endif
	psEnvCopy = copy_arg_list( envv, &nEnvCount, &nEnvSize );

	if ( psEnvCopy == NULL && envv != NULL )
	{
		printk( "Error: execve() failed to clone environment %p\n", envv );
		kfree( pzPath );
		return ( -ENOMEM );
	}
	psArgCopy = copy_arg_list( argv, &nArgCount, &nArgSize );
	if ( psArgCopy == NULL )
	{
		printk( "Error: execve() failed to clone argument list\n" );
		free_arg_copy( psEnvCopy );
		kfree( pzPath );
		return ( -ENOMEM );
	}
#ifdef __PROFILE_EXEC
	nTime2 = read_pentium_clock();
#endif
	close_all_images( psProc->pr_psImageContext );

#ifdef __PROFILE_EXEC
	nTime3 = read_pentium_clock();
#endif
	empty_mem_context( psProc->tc_psMemSeg );

#ifdef __PROFILE_EXEC
	nTime4 = read_pentium_clock();
#endif
	pzLibraryPath = NULL;

	for ( psArgNode = psEnvCopy; psArgNode != NULL; psArgNode = psArgNode->psNext )
	{
		for ( i = 0; i < psArgNode->nCount; ++i )
		{
			if ( strncmp( psArgNode->apzArgs[i], "DLL_PATH=", 9 ) == 0 )
			{
				pzLibraryPath = psArgNode->apzArgs[i] + 9;
				break;
			}
		}
	}

	if ( psArgCopy->nCount > 0 )
	{
		const char *pzCmd = strrchr( psArgCopy->apzArgs[0], '/' );

		strncpy( zCommand, ( NULL != pzCmd ) ? pzCmd + 1 : psArgCopy->apzArgs[0], OS_NAME_LENGTH - 1 );
		zCommand[OS_NAME_LENGTH - 1] = '\0';
	}
	else
		zCommand[0] = '\0';

#ifdef __PROFILE_EXEC
	nTime5 = read_pentium_clock();
#endif
	nError = load_image_inst( psProc->pr_psImageContext, pzPath, NULL, IM_APP_SPACE, &psImageInst, pzLibraryPath );

#ifdef __PROFILE_EXEC
	nTime6 = read_pentium_clock();
#endif
	if ( nError < 0 )
	{
		goto error;
	}

	psProc->pr_nHeapArea = -1;

	for ( i = 0; i < psProc->pr_psIoContext->ic_nCount; ++i )
	{
		if ( GETBIT( psProc->pr_psIoContext->ic_panCloseOnExec, i ) )
		{
			sys_close( i );
			SETBIT( psProc->pr_psIoContext->ic_panCloseOnExec, i, false );
		}
	}
	exec_free_semaphores( psProc );

	psThread->tr_nFlags = 0;	// clear FPU usage flags
	psThread->tr_nUStackSize = MAIN_THREAD_STACK_SIZE;
	psThread->tr_pUserStack = NULL;
	nStackArea = create_area( "main_stack", &psThread->tr_pUserStack, psThread->tr_nUStackSize, psThread->tr_nUStackSize, AREA_FULL_ACCESS | AREA_ANY_ADDRESS | AREA_TOP_DOWN, AREA_NO_LOCK );

	if ( nStackArea < 0 )
	{
		nError = nStackArea;
		printk( "sys_execve() : Failed to create stack area\n" );
		goto error;
	}

	get_area_info( nStackArea, &sAreaInfo );
	psThread->tr_nStackArea = nStackArea;
	psThread->tr_pThreadData = ( ( ( uint8 * )sAreaInfo.pAddress ) - TLD_SIZE ) + psThread->tr_nUStackSize;
	plUserStack = ( ( uint32 * )psThread->tr_pThreadData ) - 1;

	pErrnoPtr = ( int * )( psThread->tr_pThreadData + TLD_ERRNO );
	memcpy_to_user( psThread->tr_pThreadData + TLD_THID, &psThread->tr_hThreadID, sizeof( psThread->tr_hThreadID ) );
	memcpy_to_user( psThread->tr_pThreadData + TLD_PRID, &psThread->tr_psProcess->tc_hProcID, sizeof( psThread->tr_psProcess->tc_hProcID ) );
	memcpy_to_user( psThread->tr_pThreadData + TLD_ERRNO_ADDR, &pErrnoPtr, sizeof( pErrnoPtr ) );
	memcpy_to_user( psThread->tr_pThreadData + TLD_BASE, &psThread->tr_pThreadData, sizeof( psThread->tr_pThreadData ) );
	memset( psProc->pr_anTLDBitmap, 0, sizeof( psProc->pr_anTLDBitmap ) );

	/* Reset signal handlers */
	memset( psThread->tr_apsSigHandlers, 0, sizeof( SigAction_s ) * _NSIG );
	sigemptyset( &psThread->tr_sSigPend );
	sigemptyset( &psThread->tr_sSigBlock );

	{
		char **apNewEnv = ( char ** )( ( ( int )plUserStack ) - ( ( nEnvSize + 3 ) & ~3 ) );
		char *pEnvStrings = ( char * )&apNewEnv[nEnvCount + 1];
		char **apNewArg;
		char *pArgStrings;
		uint32 nEntry;
		int j = 0;
		void *pNullPtr = NULL;

		for ( psArgNode = psEnvCopy; psArgNode != NULL; psArgNode = psArgNode->psNext )
		{
			for ( i = 0; i < psArgNode->nCount; ++i )
			{
				nError = memcpy_to_user( &apNewEnv[j], &pEnvStrings, sizeof( pEnvStrings ) );
				if ( nError < 0 )
				{
					free_arg_copy( psEnvCopy );
					free_arg_copy( psArgCopy );
					goto error;
				}
				nError = memcpy_to_user( apNewEnv[j], psArgNode->apzArgs[i], psArgNode->anLengths[i] + 1 );
				if ( nError < 0 )
				{
					free_arg_copy( psEnvCopy );
					free_arg_copy( psArgCopy );
					goto error;
				}
				pEnvStrings += psArgNode->anLengths[i] + 1;
				j++;
			}
		}
		memcpy_to_user( &apNewEnv[j], &pNullPtr, sizeof( pNullPtr ) );

		free_arg_copy( psEnvCopy );

#ifdef __PROFILE_EXEC
		nTime7 = read_pentium_clock();
#endif
		plUserStack = ( uint32 * )( ( ( ( ( uint32 )apNewEnv ) + 3 ) & ~3 ) - 4 );

		/* Copy arguments onto stack */

		apNewArg = ( char ** )( ( ( int )plUserStack ) - ( ( nArgSize + 3 ) & ~3 ) );
		pArgStrings = ( char * )&apNewArg[nArgCount + 1];

		j = 0;
		for ( psArgNode = psArgCopy; psArgNode != NULL; psArgNode = psArgNode->psNext )
		{
			for ( i = 0; i < psArgNode->nCount; ++i )
			{
				nError = memcpy_to_user( &apNewArg[j], &pArgStrings, sizeof( pArgStrings ) );
				if ( nError < 0 )
				{
					free_arg_copy( psArgCopy );
					goto error;
				}
				nError = memcpy_to_user( apNewArg[j], psArgNode->apzArgs[i], psArgNode->anLengths[i] + 1 );
				if ( nError < 0 )
				{
					free_arg_copy( psArgCopy );
					goto error;
				}
				pArgStrings += psArgNode->anLengths[i] + 1;
				j++;
			}
		}
		memcpy_to_user( &apNewArg[j], &pNullPtr, sizeof( pNullPtr ) );
		free_arg_copy( psArgCopy );

#ifdef __PROFILE_EXEC
		nTime8 = read_pentium_clock();
#endif
		plUserStack = ( uint32 * )( ( ( ( ( uint32 )apNewArg ) + 3 ) & ~3 ) - 4 );

		nEntry = 0;

		plEntryStack = plUserStack;

		plEntryStack = init_stack( plEntryStack, apNewArg, apNewEnv );

		rename_thread( -1, zCommand );
		rename_process( -1, zCommand );

		psRegs->oldss = DS_USER;
		psRegs->oldesp = ( uint32 )plEntryStack;
		psRegs->eip = ( uint32 )psImageInst->ii_nTextAddress + psImageInst->ii_psImage->im_nEntry - psImageInst->ii_psImage->im_nVirtualAddress;

		psRegs->cs = CS_USER;

		psRegs->ds = DS_USER;
		psRegs->es = DS_USER;
		psRegs->fs = DS_USER;

		if ( sStat.st_mode & S_ISUID )
		{
			psProc->pr_nSUID = psProc->pr_nEUID = psProc->pr_nFSUID = sStat.st_uid;
		}
		if ( sStat.st_mode & S_ISGID )
		{
			psProc->pr_nSGID = psProc->pr_nEGID = psProc->pr_nFSGID = sStat.st_uid;
		}
		nFlg = cli();
		psRegs->gs = g_asProcessorDescs[get_processor_id()].pi_nGS;
		set_gdt_desc_base( psRegs->gs, ( uint32 )psThread->tr_pThreadData );

		g_bKernelInitialized = true;

		__asm__ __volatile__( "mov %0,%%ax; mov %%ax,%%gs"::"r"( psRegs->gs ):"ax" );

		put_cpu_flags( nFlg );

		kfree( pzPath );

#ifdef __PROFILE_EXEC
		nTime9 = read_pentium_clock();
		printk( "%d (%d,%d,%d,%d,%d,%d,%d,%d)\n", ( int )( nTime9 - nStartTime ) / 466000, ( int )( nTime9 - nTime8 ) / 466000, ( int )( nTime8 - nTime7 ) / 466000, ( int )( nTime7 - nTime6 ) / 466000, ( int )( nTime6 - nTime5 ) / 466000, ( int )( nTime5 - nTime4 ) / 466000, ( int )( nTime4 - nTime3 ) / 466000, ( int )( nTime3 - nTime2 ) / 466000, ( int )( nTime2 - nTime1 ) / 466000, ( int )( nTime1 - nStartTime ) / 466000 );
#endif
		return ( 0 );
	}
      error:
	printk( "ERROR : execve(%s) failed. Too late to recover, will exit\n", pzPath );
	kfree( pzPath );
	do_exit( 1 << 8 );	/* We did pass the point of no return :(        */
	return ( -1 );		// If we got this far something went seriously wrong!
}


int execve( const char *pzPath, char *const *argv, char *const *envv )
{
	int nError;
	__asm__ volatile ( "int $0x80":"=a" ( nError ):"0"( __NR_execve ), "b"( ( int )pzPath ), "c"( ( int )argv ), "d"( ( int )envv ) );

	return ( nError );
}
