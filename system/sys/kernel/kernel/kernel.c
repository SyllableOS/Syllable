/*
 *  The Syllable kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Kristian Van Der Vliet
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

#include <posix/resource.h>
#include <posix/errno.h>
#include <atheos/types.h>
#include <atheos/isa_io.h>

#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <atheos/spinlock.h>
#include <atheos/time.h>
#include <atheos/config.h>

#include <atheos/syscall.h>


#include "version.h"

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/areas.h"
#include "inc/mman.h"
#include "inc/sysbase.h"
#include "inc/global.h"
#include "inc/smp.h"
#include "inc/bcache.h"

#define _ENABLE_PRINTK

extern char g_zSysPath[256];

struct SystemBase g_sSysBase;
struct SystemBase *SysBase = &g_sSysBase;	// Backward compatibility...

volatile static port_id g_hDisplayServerPort = -1;


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_set_app_server_port( port_id hPort )
{
	g_hDisplayServerPort = hPort;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

port_id sys_get_app_server_port( void )
{
	while ( g_hDisplayServerPort == -1 )
	{
		snooze( 1000 );
	}
	return ( g_hDisplayServerPort );
}

port_id get_app_server_port( void )
{
	return ( g_hDisplayServerPort );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void SetColor( int Color, int R, int G, int B )
{
	isa_writeb( 0x3c8, Color );
	isa_writeb( 0x3c9, R >> 2 );
	isa_writeb( 0x3c9, G >> 2 );
	isa_writeb( 0x3c9, B >> 2 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_get_system_info( system_info * psInfo, int nVersion )
{
	switch ( nVersion )
	{
	case 1:
		{
			system_info_v1 sInfo;
			int i;

			sInfo.nBootTime = g_sSysBase.ex_nBootTime;	/* time of boot (# usec since 1/1/70) */
			sInfo.nCPUCount = g_nActiveCPUCount;
			sInfo.nCPUType = 0;
			sInfo.nMaxPages = g_sSysBase.ex_nTotalPageCount;	/* total # physical pages               */
			sInfo.nFreePages = g_sSysBase.ex_nFreePageCount;	/* Number of free physical pages        */
			sInfo.nCommitedPages = g_sSysBase.ex_nCommitPageCount;	/* Total number of allocated pages      */
			sInfo.nPageFaults = g_sSysBase.ex_nPageFaultCount;	/* Number of page faults                */
			sInfo.nUsedSemaphores = g_sSysBase.ex_nSemaphoreCount;	/* Number of semaphores in use          */
			sInfo.nUsedPorts = g_sSysBase.ex_nMessagePortCount;	/* Number of message ports in use       */
			sInfo.nUsedThreads = g_sSysBase.ex_nThreadCount;	/* Number of living threads             */
			sInfo.nUsedProcesses = g_sSysBase.ex_nProcessCount;	/* Number of living processes           */

			strcpy( sInfo.zKernelName, g_pzKernelName );	/* Name of kernel image */
			strcpy( sInfo.zKernelBuildDate, g_pzBuildData );	/* Date of kernel built */
			strcpy( sInfo.zKernelBuildTime, g_pzBuildTime );	/* Time of kernel built */

			for ( i = 0; i < g_nActiveCPUCount; ++i )
			{
				int nID = logig_to_physical_cpu_id( i );

				sInfo.asCPUInfo[i].nCoreSpeed = g_asProcessorDescs[nID].pi_nCoreSpeed;
				sInfo.asCPUInfo[i].nBusSpeed = g_asProcessorDescs[nID].pi_nBusSpeed;
				sInfo.asCPUInfo[i].nActiveTime = get_system_time();
				sInfo.asCPUInfo[i].nActiveTime -= g_asProcessorDescs[nID].pi_psIdleThread->tr_nCPUTime;
			}

			sInfo.nKernelVersion = g_nKernelVersion;
			return ( memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) ) );
		}
	case 2:
		{
			system_info_v2 sInfo;
			int i;

			sInfo.nBootTime = g_sSysBase.ex_nBootTime;	/* time of boot (# usec since 1/1/70) */
			sInfo.nCPUCount = g_nActiveCPUCount;
			sInfo.nCPUType = g_asProcessorDescs[g_nBootCPU].pi_nFeatures;
			sInfo.nMaxPages = g_sSysBase.ex_nTotalPageCount;	/* total # physical pages               */
			sInfo.nFreePages = g_sSysBase.ex_nFreePageCount;	/* Number of free physical pages        */
			sInfo.nCommitedPages = g_sSysBase.ex_nCommitPageCount;	/* Total number of allocated pages      */
			sInfo.nKernelMemSize = g_sSysBase.ex_nKernelMemSize;

			sInfo.nPageFaults = g_sSysBase.ex_nPageFaultCount;	/* Number of page faults                */
			sInfo.nUsedSemaphores = g_sSysBase.ex_nSemaphoreCount;	/* Number of semaphores in use          */
			sInfo.nUsedPorts = g_sSysBase.ex_nMessagePortCount;	/* Number of message ports in use       */
			sInfo.nUsedThreads = g_sSysBase.ex_nThreadCount;	/* Number of living threads             */
			sInfo.nUsedProcesses = g_sSysBase.ex_nProcessCount;	/* Number of living processes           */

			sInfo.nLoadedImageCount = g_sSysBase.ex_nLoadedImageCount;
			sInfo.nImageInstanceCount = g_sSysBase.ex_nImageInstanceCount;

			sInfo.nOpenFileCount = g_sSysBase.ex_nOpenFileCount;
			sInfo.nAllocatedInodes = g_sSysBase.ex_nAllocatedInodeCount;
			sInfo.nLoadedInodes = g_sSysBase.ex_nLoadedInodeCount;
			sInfo.nUsedInodes = g_sSysBase.ex_nUsedInodeCount;
			sInfo.nBlockCacheSize = g_sSysBase.ex_nBlockCacheSize;
			sInfo.nDirtyCacheSize = g_sSysBase.ex_nDirtyCacheSize;
			sInfo.nLockedCacheBlocks = g_sSysBase.ex_nLockedCacheBlocks;

			strcpy( sInfo.zKernelName, g_pzKernelName );	/* Name of kernel image                */
			strcpy( sInfo.zKernelBuildDate, g_pzBuildData );	/* Date of kernel built            */
			strcpy( sInfo.zKernelBuildTime, g_pzBuildTime );	/* Time of kernel built            */

			for ( i = 0; i < g_nActiveCPUCount; ++i )
			{
				int nID = logig_to_physical_cpu_id( i );

				sInfo.asCPUInfo[i].nCoreSpeed = g_asProcessorDescs[nID].pi_nCoreSpeed;
				sInfo.asCPUInfo[i].nBusSpeed = g_asProcessorDescs[nID].pi_nBusSpeed;
				sInfo.asCPUInfo[i].nActiveTime = get_system_time();
				sInfo.asCPUInfo[i].nActiveTime -= g_asProcessorDescs[nID].pi_psIdleThread->tr_nCPUTime;
			}

			sInfo.nKernelVersion = g_nKernelVersion;
			return ( memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) ) );
		}
	case 3:
		{
			system_info sInfo;
			int i;

			sInfo.nBootTime = g_sSysBase.ex_nBootTime;	/* time of boot (# usec since 1/1/70) */
			sInfo.nCPUCount = g_nActiveCPUCount;
			sInfo.nCPUType = g_asProcessorDescs[g_nBootCPU].pi_nFeatures;
			sInfo.nMaxPages = g_sSysBase.ex_nTotalPageCount;	/* total # physical pages               */
			sInfo.nFreePages = g_sSysBase.ex_nFreePageCount;	/* Number of free physical pages        */
			sInfo.nCommitedPages = g_sSysBase.ex_nCommitPageCount;	/* Total number of allocated pages      */
			sInfo.nKernelMemSize = g_sSysBase.ex_nKernelMemSize;

			sInfo.nPageFaults = g_sSysBase.ex_nPageFaultCount;	/* Number of page faults                */
			sInfo.nUsedSemaphores = g_sSysBase.ex_nSemaphoreCount;	/* Number of semaphores in use          */
			sInfo.nUsedPorts = g_sSysBase.ex_nMessagePortCount;	/* Number of message ports in use       */
			sInfo.nUsedThreads = g_sSysBase.ex_nThreadCount;	/* Number of living threads             */
			sInfo.nUsedProcesses = g_sSysBase.ex_nProcessCount;	/* Number of living processes           */

			sInfo.nLoadedImageCount = g_sSysBase.ex_nLoadedImageCount;
			sInfo.nImageInstanceCount = g_sSysBase.ex_nImageInstanceCount;

			sInfo.nOpenFileCount = g_sSysBase.ex_nOpenFileCount;
			sInfo.nAllocatedInodes = g_sSysBase.ex_nAllocatedInodeCount;
			sInfo.nLoadedInodes = g_sSysBase.ex_nLoadedInodeCount;
			sInfo.nUsedInodes = g_sSysBase.ex_nUsedInodeCount;
			sInfo.nBlockCacheSize = g_sSysBase.ex_nBlockCacheSize;
			sInfo.nDirtyCacheSize = g_sSysBase.ex_nDirtyCacheSize;
			sInfo.nLockedCacheBlocks = g_sSysBase.ex_nLockedCacheBlocks;

			strcpy( sInfo.zKernelName, g_pzKernelName );	/* Name of kernel image            */
			strcpy( sInfo.zKernelBuildDate, g_pzBuildData );	/* Date of kernel built            */
			strcpy( sInfo.zKernelBuildTime, g_pzBuildTime );	/* Time of kernel built            */
			strcpy( sInfo.zKernelCpuArch, g_pzCpuArch );	/* CPU this kernel is running on   */
			strcpy( sInfo.zKernelSystem, g_pzSystem );	/* OS name (E.g. "Syllable")       */

			for ( i = 0; i < g_nActiveCPUCount; ++i )
			{
				int nID = logig_to_physical_cpu_id( i );

				sInfo.asCPUInfo[i].nCoreSpeed = g_asProcessorDescs[nID].pi_nCoreSpeed;
				sInfo.asCPUInfo[i].nBusSpeed = g_asProcessorDescs[nID].pi_nBusSpeed;
				sInfo.asCPUInfo[i].nActiveTime = get_system_time();
				sInfo.asCPUInfo[i].nActiveTime -= g_asProcessorDescs[nID].pi_psIdleThread->tr_nCPUTime;
			}

			sInfo.nKernelVersion = g_nKernelVersion;
			return ( memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) ) );
		}

	default:
		printk( "Error: sys_get_system_info() invalid version %d\n", nVersion );
		return ( -EINVAL );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int dbprintf( int nPort, const char *fmt, ... )
{
	uint8 zBuffer[1024];

	if ( SysBase == NULL )
	{
		return ( 0 );
	}

	sprintf( zBuffer, fmt, ( ( uint32 * )( &fmt ) )[1], ( ( uint32 * )( &fmt ) )[2], ( ( uint32 * )( &fmt ) )[3], ( ( uint32 * )( &fmt ) )[4], ( ( uint32 * )( &fmt ) )[5], ( ( uint32 * )( &fmt ) )[6], ( ( uint32 * )( &fmt ) )[7], ( ( uint32 * )( &fmt ) )[8], ( ( uint32 * )( &fmt ) )[9], ( ( uint32 * )( &fmt ) )[10], ( ( uint32 * )( &fmt ) )[11], ( ( uint32 * )( &fmt ) )[12] );
	sys_debug_write( nPort, zBuffer, strlen( zBuffer ) );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int printk( const char *fmt, ... )
{
#ifdef _ENABLE_PRINTK
	Thread_s *psThread = CURRENT_THREAD;
	uint8 String[512];
	uint8 zBuffer[1024];
	int nFlg;


	if ( SysBase == NULL )
	{
		return ( 0 );
	}
	nFlg = cli();

	if ( NULL != psThread )
	{
		Process_s *psProc;

		if ( ( psProc = CURRENT_PROC ) )
		{
			sprintf( String, "%d:%s::%s : %s", get_processor_id(), psProc->tc_zName, psThread->tr_zName, fmt );
		}
		else
		{
			sprintf( String, "%s : %s", psThread->tr_zName, fmt );
		}
	}
	else
	{
		sprintf( String, "%d : %s", get_processor_id(), fmt );
	}
	put_cpu_flags( nFlg );
	sprintf( zBuffer, String, ( ( uint32 * )( &fmt ) )[1], ( ( uint32 * )( &fmt ) )[2], ( ( uint32 * )( &fmt ) )[3], ( ( uint32 * )( &fmt ) )[4], ( ( uint32 * )( &fmt ) )[5], ( ( uint32 * )( &fmt ) )[6], ( ( uint32 * )( &fmt ) )[7], ( ( uint32 * )( &fmt ) )[8], ( ( uint32 * )( &fmt ) )[9], ( ( uint32 * )( &fmt ) )[10], ( ( uint32 * )( &fmt ) )[11], ( ( uint32 * )( &fmt ) )[12] );

	debug_write( zBuffer, strlen( zBuffer ) );
#endif
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void sys_dbprintf( const char *fmt, ... )
{
#ifdef _ENABLE_PRINTK
	if ( SysBase )
	{
		Thread_s *psThread = CURRENT_THREAD;
		uint8 String[512];
		uint8 zBuffer[1024];
		int nFlg;

		nFlg = cli();

		if ( NULL != psThread )
		{
			Process_s *psProc;

			if ( ( psProc = CURRENT_PROC ) )
			{
				sprintf( String, "%d:%s::%s : %s", get_processor_id(), psProc->tc_zName, psThread->tr_zName, fmt );
			}
			else
			{
				sprintf( String, "%s : %s", psThread->tr_zName, fmt );
			}
		}
		else
		{
			sprintf( String, "%d : %s", get_processor_id(), fmt );
		}
		put_cpu_flags( nFlg );
		sprintf( zBuffer, String, ( ( uint32 * )( &fmt ) )[1], ( ( uint32 * )( &fmt ) )[2], ( ( uint32 * )( &fmt ) )[3], ( ( uint32 * )( &fmt ) )[4], ( ( uint32 * )( &fmt ) )[5], ( ( uint32 * )( &fmt ) )[6], ( ( uint32 * )( &fmt ) )[7], ( ( uint32 * )( &fmt ) )[8], ( ( uint32 * )( &fmt ) )[9], ( ( uint32 * )( &fmt ) )[10], ( ( uint32 * )( &fmt ) )[11], ( ( uint32 * )( &fmt ) )[12] );

		debug_write( zBuffer, strlen( zBuffer ) );
	}
#endif
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_DebugPrint( const char *fmt, char **pzArgs )
{
	if ( SysBase )
	{
		Thread_s *psThread = CURRENT_THREAD;
		uint8 String[512];
		uint8 zBuffer[1024];
		uint32 nFlg;

		nFlg = cli();
		if ( NULL != psThread )
		{
			Process_s *psProc;

			if ( ( psProc = CURRENT_PROC ) )
			{
				sprintf( String, "%d:%s::%s : %s", get_processor_id(), psProc->tc_zName, psThread->tr_zName, fmt );
			}
			else
			{
				sprintf( String, "%d:%s : %s", get_processor_id(), psThread->tr_zName, fmt );
			}
		}
		else
		{
			sprintf( String, "%d : %s", get_processor_id(), fmt );
		}
		put_cpu_flags( nFlg );
		sprintf( zBuffer, String, pzArgs[0], pzArgs[1], pzArgs[2], pzArgs[3], pzArgs[4], pzArgs[5], pzArgs[6], pzArgs[7], pzArgs[8], pzArgs[9], pzArgs[10], pzArgs[11], pzArgs[12] );

		debug_write( zBuffer, strlen( zBuffer ) );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void panic( const char *pzFmt, ... )
{
	static char zBuf[2048];
	int nOldFlags = cli();
	va_list args;

	va_start(args, pzFmt);
	vsprintf( zBuf, pzFmt, args );
	va_end(args);
	printk( "kernel panic (%d) : %s\n", g_nDisableTS, zBuf );

/*    printk( "%p\n", __builtin_return_address(0) );
    printk( "%p\n", __builtin_return_address(1) );
    printk( "%p\n", __builtin_return_address(2) );
    printk( "%p\n", __builtin_return_address(3) );
    printk( "%p\n", __builtin_return_address(4) );*/
	put_cpu_flags( nOldFlags );

	trace_stack( 0, NULL );

	for ( ;; )
	{
		snooze( 1000000 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 sys_GetToken( void )
{
	static uint32 nToken = 0;

	return ( nToken++ );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void protect_dos_mem( void )
{
	int i;

	for ( i = 0; i < 256; ++i )
	{
		pgd_t *pPgd = pgd_offset( g_psKernelSeg, i * PAGE_SIZE );
		pte_t *pPte = pte_offset( pPgd, i * PAGE_SIZE );

		if ( i == 0 )
		{
			PTE_VALUE( *pPte ) = ( i * PAGE_SIZE );
		}
		else
		{
			PTE_VALUE( *pPte ) = ( i * PAGE_SIZE ) | PTE_PRESENT | PTE_WRITE;
		}
	}
	flush_tlb();
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void unprotect_dos_mem( void )
{
	int i;

	for ( i = 0; i < 256; ++i )
	{
		pgd_t *pPgd = pgd_offset( g_psKernelSeg, i * PAGE_SIZE );
		pte_t *pPte = pte_offset( pPgd, i * PAGE_SIZE );

		PTE_VALUE( *pPte ) = ( i * PAGE_SIZE ) | PTE_PRESENT | PTE_WRITE | PTE_USER;
	}
	flush_tlb();
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_system_path( char *pzBuffer, int nBufLen )
{
	strncpy( pzBuffer, g_zSysPath, nBufLen );

	pzBuffer[nBufLen - 1] = '\0';

	return ( 0 );
}

int sys_get_system_path( char *pzBuffer, int nBufLen )
{
	return ( get_system_path( pzBuffer, nBufLen ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

char *ltoa( int32 value, char *buffer, int32 radix )
{
	switch ( radix )
	{
	case 16:
		sprintf( buffer, "%lx", value );
		break;
	default:
		sprintf( buffer, "%ld", value );
		break;
	}
	return ( buffer );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

char *ultoa( int32 value, char *buffer, int32 radix )
{
	switch ( radix )
	{
	case 16:
		sprintf( buffer, "%lx", value );
		break;
	default:
		sprintf( buffer, "%lu", value );
		break;
	}
	return ( buffer );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static inline void kb_wait( void )
{
	int i;

	for ( i = 0; i < 0x10000; i++ )
		if ( ( inb_p( 0x64 ) & 0x02 ) == 0 )
			break;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void hard_reset( void )
{
	int i, j;

//      sti();

	/* Write 0x1234 to absolute memory location 0x472.  The BIOS reads
	   this on booting to tell it to "Bypass memory test (also warm
	   boot)".  This seems like a fairly standard thing that gets set by
	   REBOOT.COM programs. */

	unprotect_dos_mem();
	*( ( uint16 * )0x472 ) = 0x1234;

	for ( ;; )
	{
		for ( i = 0; i < 100; ++i )
		{
			kb_wait();
			for ( j = 0; j < 100000; ++j )

		  /*** EMPTY ***/ ;
			outb( 0xfe, 0x64 );	/* pulse reset low */
//                      udelay(100);
		}
//              __asm__ __volatile__("\tlidt %0": "=m" (no_idt));
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_reboot( void )
{
	thread_id hThread;
	int i;

	write_kernel_config();

	// Since we (hopefully) wont be killed by the signal we must close our files manually.
	for ( i = 0; i < 256; ++i )
	{
		sys_close( i );
	}


	printk( "Send TERM signals\n" );

	for ( hThread = get_prev_thread( -1 ); -1 != hThread; hThread = get_prev_thread( hThread ) )
	{
		if ( hThread > 1 )
		{
			sys_kill( hThread, SIGTERM );
		}
	}

	snooze( 2000000 );
	printk( "Send KILL signals\n" );

	for ( hThread = get_prev_thread( -1 ); -1 != hThread; hThread = get_prev_thread( hThread ) )
	{
		if ( hThread > 1 )
		{
			sys_kill( hThread, SIGKILL );
		}
	}

	snooze( 1000000 );

	printk( "Flush block cache()\n" );
	flush_block_cache();
	printk( "Shut down VFS\n" );
	shutdown_vfs();
	printk( "shut down block cache()\n" );
	shutdown_block_cache();


	printk( "Rebooting AtheOS...\n" );

	// Just to be sure :)
	snooze( 1000000 );

	hard_reset();

	printk( "It must be a sign!\n" );

	return ( 0 );
}

int reboot( void )
{
	sys_reboot();
	return ( -EINVAL );
}

/*****************************************************************************
 * NAME: sys_apm_poweroff
 * DESC: Powers down a system using the APM BIOS
 * NOTE: by Anthony Morphett < tonymorph@yahoo.com >
 * SEE ALSO: apm_poweroff(), sys_reboot()
 ****************************************************************************/

int sys_apm_poweroff( void )
{
	struct RMREGS rm;

	/* this part copied from sys_reboot */
	thread_id hThread;
	int i;

	write_kernel_config();

	// Since we (hopefully) wont be killed by the signal we must close our files manually.
	for ( i = 0; i < 256; ++i )
	{
		sys_close( i );
	}


	printk( "Send TERM signals\n" );

	for ( hThread = get_prev_thread( -1 ); -1 != hThread; hThread = get_prev_thread( hThread ) )
	{
		if ( hThread > 1 )
		{
			sys_kill( hThread, SIGTERM );
		}
	}

	snooze( 2000000 );
	printk( "Send KILL signals\n" );

	for ( hThread = get_prev_thread( -1 ); -1 != hThread; hThread = get_prev_thread( hThread ) )
	{
		if ( hThread > 1 )
		{
			sys_kill( hThread, SIGKILL );
		}
	}

	snooze( 1000000 );

	printk( "Flush block cache()\n" );
	flush_block_cache();
	printk( "Shut down VFS\n" );
	shutdown_vfs();
	printk( "shut down block cache()\n" );
	shutdown_block_cache();

	/* end copied from sys_reboot */


	printk( "APM power down...\n" );

	// Just to be sure :)
	snooze( 1000000 );

	unprotect_dos_mem();	// hard_reset does this, probably doesn't hurt

	memset( &rm, 0, sizeof( rm ) );

	rm.EAX = 0x5304;
	realint( 0x15, &rm );

	rm.EAX = 0x5302;
	rm.EBX = 0;
	realint( 0x15, &rm );

	rm.EAX = 0x5308;
	rm.EBX = 1;
	rm.ECX = 1;
	realint( 0x15, &rm );

	rm.EAX = 0x530d;
	rm.EAX = 1;
	rm.ECX = 1;
	realint( 0x15, &rm );

	rm.EAX = 0x530f;
	rm.EBX = 1;
	rm.ECX = 1;
	realint( 0x15, &rm );

	rm.EAX = 0x530e;
	rm.EBX = 0;
	rm.ECX = 0x102;
	realint( 0x15, &rm );

	rm.EAX = 0x5307;
	rm.EBX = 1;
	rm.ECX = 3;
	realint( 0x15, &rm );

	printk( "APM Poweroff failed... :(\n" );

	return ( 0 );
}

int apm_poweroff( void )
{
	sys_apm_poweroff();
	return ( -EINVAL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool Desc_SetBase( uint16 desc, uint32 base )
{
	desc >>= 3;
	g_sSysBase.ex_GDT[desc].desc_bsl = base & 0xffff;
	g_sSysBase.ex_GDT[desc].desc_bsm = ( base >> 16 ) & 0xff;
	g_sSysBase.ex_GDT[desc].desc_bsh = ( base >> 24 ) & 0xff;
	return ( TRUE );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 Desc_GetBase( uint16 desc )
{
	uint32 base;

	desc >>= 3;

	base = g_sSysBase.ex_GDT[desc].desc_bsl;
	base += g_sSysBase.ex_GDT[desc].desc_bsm << 16;
	base += g_sSysBase.ex_GDT[desc].desc_bsh << 24;
	return ( base );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool Desc_SetLimit( uint16 desc, uint32 limit )
{
	desc >>= 3;
//  g_sSysBase.ex_GDT[desc].desc_lmh &= 0x70;   /* mask out hi nibble, and granularity bit      */
	g_sSysBase.ex_GDT[desc].desc_lmh = 0x40;	/* mask out hi nibble, and granularity bit      */

	if ( limit > 0x000fffff )
	{
		g_sSysBase.ex_GDT[desc].desc_lmh |= 0x80;	/* 4K granularity       */
		limit >>= 12;
	}

	g_sSysBase.ex_GDT[desc].desc_lml = limit & 0xffff;
	g_sSysBase.ex_GDT[desc].desc_lmh |= ( limit >> 16 ) & 0x0f;

	return ( TRUE );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 Desc_GetLimit( uint16 desc )
{
	uint32 limit;

	desc >>= 3;

	limit = g_sSysBase.ex_GDT[desc].desc_lml;
	limit += ( g_sSysBase.ex_GDT[desc].desc_lmh & 0x0f ) << 16;

	if ( g_sSysBase.ex_GDT[desc].desc_lmh & 0x80 )	/* check granularity bit        */
	{
		limit <<= 12;
	}
	return ( limit );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool Desc_SetAccess( uint16 desc, uint8 acc )
{
	desc >>= 3;
	g_sSysBase.ex_GDT[desc].desc_acc = acc;
	return ( TRUE );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint8 Desc_GetAccess( uint16 desc )
{
	return ( g_sSysBase.ex_GDT[desc >> 3].desc_acc );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint16 Desc_Alloc( int32 table )
{
	int i;
	uint32 nFlg = cli();

	sched_lock();

	for ( i = 0; i < 8192; i++ )
	{
		if ( !( g_sSysBase.ex_DTAllocList[i] & DTAL_GDT ) )
		{
			g_sSysBase.ex_DTAllocList[i] |= DTAL_GDT;
			sched_unlock();
			put_cpu_flags( nFlg );
			return ( i << 3 );
		}
	}
	sched_unlock();
	put_cpu_flags( nFlg );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void Desc_Free( uint16 desc )
{
	uint32 nFlg = cli();

	sched_lock();
	g_sSysBase.ex_DTAllocList[desc >> 3] &= ~DTAL_GDT;
	sched_unlock();
	put_cpu_flags( nFlg );
}
