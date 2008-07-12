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
#include <atheos/device.h>

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

status_t do_get_system_info( system_info * psInfo, int nVersion, bool bFromKernel )
{
	status_t nRet;

	switch ( nVersion )
	{
	/* Versions 1 and 2 removed in 0.6.6 */
	case 3:
		{
			system_info_v3 sInfo;
			int i;

			sInfo.nBootTime = g_sSysBase.ex_nBootTime;	/* time of boot (# usec since 1/1/70) */
			sInfo.nCPUCount = g_nActiveCPUCount;
			sInfo.nCPUType = g_asProcessorDescs[g_nBootCPU].pi_nFeatures;
			sInfo.nMaxPages = g_sSysBase.ex_nTotalPageCount;	/* total # physical pages               */
			sInfo.nFreePages = atomic_read( &g_sSysBase.ex_nFreePageCount );	/* Number of free physical pages        */
			sInfo.nCommitedPages = g_sSysBase.ex_nCommitPageCount;	/* Total number of allocated pages      */
			sInfo.nKernelMemSize = atomic_read( &g_sSysBase.ex_nKernelMemSize );

			sInfo.nPageFaults = atomic_read( &g_sSysBase.ex_nPageFaultCount );	/* Number of page faults                */
			sInfo.nUsedSemaphores = atomic_read( &g_sSysBase.ex_nSemaphoreCount );	/* Number of semaphores in use          */
			sInfo.nUsedPorts = atomic_read( &g_sSysBase.ex_nMessagePortCount );	/* Number of message ports in use       */
			sInfo.nUsedThreads = atomic_read( &g_sSysBase.ex_nThreadCount );	/* Number of living threads             */
			sInfo.nUsedProcesses = atomic_read( &g_sSysBase.ex_nProcessCount );	/* Number of living processes           */

			sInfo.nLoadedImageCount = atomic_read( &g_sSysBase.ex_nLoadedImageCount );
			sInfo.nImageInstanceCount = atomic_read( &g_sSysBase.ex_nImageInstanceCount );

			sInfo.nOpenFileCount = atomic_read( &g_sSysBase.ex_nOpenFileCount );
			sInfo.nAllocatedInodes = atomic_read( &g_sSysBase.ex_nAllocatedInodeCount );
			sInfo.nLoadedInodes = atomic_read( &g_sSysBase.ex_nLoadedInodeCount );
			sInfo.nUsedInodes = atomic_read( &g_sSysBase.ex_nUsedInodeCount );
			sInfo.nBlockCacheSize = atomic_read( &g_sSysBase.ex_nBlockCacheSize );
			sInfo.nDirtyCacheSize = atomic_read( &g_sSysBase.ex_nDirtyCacheSize );
			sInfo.nLockedCacheBlocks = g_sSysBase.ex_nLockedCacheBlocks;

			strcpy( sInfo.zKernelName, g_pzKernelName );	/* Name of kernel image            */
			strcpy( sInfo.zKernelBuildDate, g_pzBuildData );	/* Date of kernel built            */
			strcpy( sInfo.zKernelBuildTime, g_pzBuildTime );	/* Time of kernel built            */
			strcpy( sInfo.zKernelCpuArch, g_pzCpuArch );	/* CPU this kernel is running on   */
			strcpy( sInfo.zKernelSystem, g_pzSystem );	/* OS name (E.g. "Syllable")       */

			for ( i = 0; i < g_nActiveCPUCount; ++i )
			{
				int nID = logical_to_physical_cpu_id( i );

				sInfo.asCPUInfo[i].nCoreSpeed = g_asProcessorDescs[nID].pi_nCoreSpeed;
				sInfo.asCPUInfo[i].nBusSpeed = g_asProcessorDescs[nID].pi_nBusSpeed;
				sInfo.asCPUInfo[i].nActiveTime = get_system_time();
				sInfo.asCPUInfo[i].nActiveTime -= g_asProcessorDescs[nID].pi_psIdleThread->tr_nCPUTime;
			}

			sInfo.nKernelVersion = g_nKernelVersion;
			if( bFromKernel )
				nRet = memcpy( psInfo, &sInfo, sizeof( sInfo ) );
			else
				nRet = memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) );
			return nRet;
		}
	case 4:
		{
			system_info sInfo;
			int i;
			int nArgc;
			char** apzArgv;
			char* pzPos;

			sInfo.nBootTime = g_sSysBase.ex_nBootTime;	/* time of boot (# usec since 1/1/70) */
			sInfo.nCPUCount = g_nActiveCPUCount;
			sInfo.nCPUType = g_asProcessorDescs[g_nBootCPU].pi_nFeatures;
			sInfo.nMaxPages = g_sSysBase.ex_nTotalPageCount;	/* total # physical pages               */
			sInfo.nFreePages = atomic_read( &g_sSysBase.ex_nFreePageCount );	/* Number of free physical pages        */
			sInfo.nCommitedPages = g_sSysBase.ex_nCommitPageCount;	/* Total number of allocated pages      */
			sInfo.nKernelMemSize = atomic_read( &g_sSysBase.ex_nKernelMemSize );

			sInfo.nPageFaults = atomic_read( &g_sSysBase.ex_nPageFaultCount );	/* Number of page faults                */
			sInfo.nUsedSemaphores = atomic_read( &g_sSysBase.ex_nSemaphoreCount );	/* Number of semaphores in use          */
			sInfo.nUsedPorts = atomic_read( &g_sSysBase.ex_nMessagePortCount );	/* Number of message ports in use       */
			sInfo.nUsedThreads = atomic_read( &g_sSysBase.ex_nThreadCount );	/* Number of living threads             */
			sInfo.nUsedProcesses = atomic_read( &g_sSysBase.ex_nProcessCount );	/* Number of living processes           */

			sInfo.nLoadedImageCount = atomic_read( &g_sSysBase.ex_nLoadedImageCount );
			sInfo.nImageInstanceCount = atomic_read( &g_sSysBase.ex_nImageInstanceCount );

			sInfo.nOpenFileCount = atomic_read( &g_sSysBase.ex_nOpenFileCount );
			sInfo.nAllocatedInodes = atomic_read( &g_sSysBase.ex_nAllocatedInodeCount );
			sInfo.nLoadedInodes = atomic_read( &g_sSysBase.ex_nLoadedInodeCount );
			sInfo.nUsedInodes = atomic_read( &g_sSysBase.ex_nUsedInodeCount );
			sInfo.nBlockCacheSize = atomic_read( &g_sSysBase.ex_nBlockCacheSize );
			sInfo.nDirtyCacheSize = atomic_read( &g_sSysBase.ex_nDirtyCacheSize );
			sInfo.nLockedCacheBlocks = g_sSysBase.ex_nLockedCacheBlocks;

			strcpy( sInfo.zKernelName, g_pzKernelName );	/* Name of kernel image            */
			strcpy( sInfo.zKernelBuildDate, g_pzBuildData );	/* Date of kernel built            */
			strcpy( sInfo.zKernelBuildTime, g_pzBuildTime );	/* Time of kernel built            */
			strcpy( sInfo.zKernelCpuArch, g_pzCpuArch );	/* CPU this kernel is running on   */
			strcpy( sInfo.zKernelSystem, g_pzSystem );	/* OS name (E.g. "Syllable")       */
			/* Kernel boot parameters */
			get_kernel_arguments( &nArgc, &apzArgv );
			pzPos = sInfo.zKernelBootParams;
			for( i = 0; i < nArgc; i++ ) {
				strcpy( pzPos, apzArgv[i] );
				pzPos += strlen( apzArgv[i] );
				*pzPos++ = ' ';
			}
			*(--pzPos) = 0;

			for ( i = 0; i < g_nActiveCPUCount; ++i )
			{
				int nID = logical_to_physical_cpu_id( i );

				sInfo.asCPUInfo[i].nCoreSpeed = g_asProcessorDescs[nID].pi_nCoreSpeed;
				sInfo.asCPUInfo[i].nBusSpeed = g_asProcessorDescs[nID].pi_nBusSpeed;
				sInfo.asCPUInfo[i].nActiveTime = get_system_time();
				sInfo.asCPUInfo[i].nActiveTime -= g_asProcessorDescs[nID].pi_psIdleThread->tr_nCPUTime;
			}

			sInfo.nKernelVersion = g_nKernelVersion;
			if( bFromKernel )
				nRet = memcpy( psInfo, &sInfo, sizeof( sInfo ) );
			else
				nRet = memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) );
			return nRet;
		}

	default:
		printk( "Error: sys_get_system_info() invalid version %d\n", nVersion );
		return ( -EINVAL );
	}


}

status_t sys_get_system_info( system_info * psInfo, int nVersion )
{
	return do_get_system_info( psInfo, nVersion, false );
}

status_t get_system_info( system_info * psInfo, int nVersion )
{
	return do_get_system_info( psInfo, nVersion, true );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int dbprintf( int nPort, const char *fmt, ... )
{
	char zBuffer[1024];

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
	char String[512];
	char zBuffer[1024];
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
		char String[512];
		char zBuffer[1024];
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
		char String[512];
		char zBuffer[1024];
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
	printk( "kernel panic : %s\n", zBuf );

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

	shutdown_ap_processors();

	printk( "Rebooting Syllable...\n" );

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
 * DESC: Powers down a system using the APM BIOS or the ACPI busmanager
 * NOTE: by Anthony Morphett < tonymorph@yahoo.com >
 * SEE ALSO: apm_poweroff(), sys_reboot()
 ****************************************************************************/

int sys_apm_poweroff( void )
{
	struct RMREGS rm;
	
	typedef struct 
	{
		void		( *poweroff )( void );
	} ACPI_bus_s;
	

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
	
	shutdown_ap_processors();

	/* end copied from sys_reboot */


	printk( "APM power down...\n" );

	// Just to be sure :)
	snooze( 1000000 );

	unprotect_dos_mem();	// hard_reset does this, probably doesn't hurt
	
	ACPI_bus_s* psBus = get_busmanager( "acpi", 1 );
	if( psBus )
	{
		psBus->poweroff();
		for( ;; )
		{
		}
	}

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

