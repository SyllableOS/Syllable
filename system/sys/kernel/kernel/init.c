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

#include <atheos/ctype.h>
#include <posix/fcntl.h>
#include <posix/resource.h>
#include <posix/wait.h>
#include <posix/errno.h>
#include <posix/unistd.h>
#include <posix/dirent.h>
#include <posix/stat.h>

#include <atheos/kernel.h>
#include <atheos/resource.h>
#include <atheos/syscall.h>
#include <atheos/smp.h>
#include <atheos/irq.h>
#include <atheos/semaphore.h>
#include <atheos/bcache.h>
#include <atheos/multiboot.h>
#include <atheos/device.h>
#include <atheos/config.h>

#include <net/net.h>

#include <macros.h>

#include "vfs/vfs.h"
#include "inc/scheduler.h"
#include "inc/areas.h"
#include "inc/bcache.h"
#include "inc/smp.h"
#include "inc/sysbase.h"
#include "inc/swap.h"
#include "inc/pit_timer.h"
#include "version.h"

extern int _end;

static MultiBootHeader_s g_sMultiBootHeader;
static char *g_apzEnviron[256];	// Environment from init script
char g_zSysPath[256] = "/boot/";
static char g_zAppServerPath[256] = "/system/appserver", *g_pzAppserver = NULL;
static char g_zBootMode[256] = "normal";
static char g_zKernelParams[4096];
static const char *g_apzKernelArgs[MAX_KERNEL_ARGS];
static int g_nKernelArgCount = 0;
static char g_zBootFS[256] = "afs";
static char g_zBootDev[256];
static char g_zBootFSArgs[256];
static uint32 g_nDebugBaudRate = 0;	//115200;
static uint32 g_nDebugSerialPort = 2;
static bool g_bPlainTextDebug = false;
static uint32 g_nMemSize = 64 * 1024 * 1024;
static bool g_bSwapEnabled = false;
bool g_bDisableSMP = false;
static bool g_bDisableACPI = false;
bool g_bDisableGFXDrivers = false;
bool g_bDisableKernelConfig = false;

bool g_bRootFSMounted = false;
bool g_bKernelInitialized = false;

uint32 g_nFirstUserAddress = AREA_FIRST_USER_ADDRESS;
uint32 g_nLastUserAddress = AREA_LAST_USER_ADDRESS;

void init_elf_loader( void );
void idle_loop( void );

extern void SysCallEntry( void );
extern void probe( void );
extern void exc0( void );
extern void exc1( void );
extern void exc2( void );
extern void exc3( void );
extern void exc4( void );
extern void exc5( void );
extern void exc6( void );
extern void exc7( void );
extern void exc8( void );
extern void exc9( void );
extern void exca( void );
extern void excb( void );
extern void excc( void );
extern void excd( void );
extern void exce( void );
extern void exc10( void );
extern void exc11( void );
extern void exc12( void );
extern void exc13( void );

extern void init_irq_controller( void );
extern void irq0( void );
extern void irq1( void );
extern void irq2( void );
extern void irq3( void );
extern void irq4( void );
extern void irq5( void );
extern void irq6( void );
extern void irq7( void );
extern void irq8( void );
extern void irq9( void );
extern void irqa( void );
extern void irqb( void );
extern void irqc( void );
extern void irqd( void );
extern void irqe( void );
extern void irqf( void );

extern void TSIHand( void );

extern void smp_preempt( void );
extern void smp_invalidate_pgt( void );
extern void smp_spurious_irq( void );

MemContext_s *g_psKernelSeg = NULL;

extern uint32 v86Stack_seg;
extern uint32 v86Stack_off;
extern uint32 g_anKernelStackEnd[];

extern int32 g_nPrintkMax; /* for printk_max parameter */

// must be inlined in order to work correctly
static inline int Fork( const char *pzName )
{
	int nError;
	__asm__ volatile ( "int $0x80":"=a" ( nError ):"0"( __NR_Fork ), "b"( ( (int)pzName ) ) );

	return ( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void dbg_set_user_mode( int argc, char **argv )
{
	if ( argc < 2 )
	{
		dbprintf( DBP_DEBUGGER, "System is now in %s user mode\n", ( g_sSysBase.ex_bSingleUserMode ) ? "single" : "multi" );
	}
	else
	{
		g_sSysBase.ex_bSingleUserMode = atol( argv[1] ) != 0;
	}
	dbprintf( DBP_DEBUGGER, "System is now in %s user mode\n", ( g_sSysBase.ex_bSingleUserMode ) ? "single" : "multi" );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int parse_num( const char *pzString )
{
	int nValue;
	int i;

	nValue = strtoul( pzString, NULL, 0 );

	for ( i = 0; isdigit( pzString[i] ); ++i );

	if ( pzString[i] == 'k' || pzString[i] == 'K' )
	{
		nValue *= 1024;
	}
	else if ( pzString[i] == 'M' )
	{
		nValue *= 1024 * 1024;
	}
	return ( nValue );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int set_interrupt_gate( int IntNum, void *Handler )
{
	g_sSysBase.ex_IDT[IntNum].igt_sel = CS_KERNEL;
	g_sSysBase.ex_IDT[IntNum].igt_offl = ( ( uint32 )Handler ) & 0xffff;
	g_sSysBase.ex_IDT[IntNum].igt_offh = ( ( uint32 )Handler ) >> 16;
	g_sSysBase.ex_IDT[IntNum].igt_ctrl = 0x8e00;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int set_trap_gate( int IntNum, void *Handler )
{
	g_sSysBase.ex_IDT[IntNum].igt_sel = CS_KERNEL;
	g_sSysBase.ex_IDT[IntNum].igt_offl = ( ( uint32 )Handler ) & 0xffff;
	g_sSysBase.ex_IDT[IntNum].igt_offh = ( ( uint32 )Handler ) >> 16;
	g_sSysBase.ex_IDT[IntNum].igt_ctrl = 0xef00;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void init_interrupt_table( void )
{
	set_trap_gate( 0x00, exc0 );
	set_trap_gate( 0x01, exc1 );
	set_trap_gate( 0x02, exc2 );
	set_trap_gate( 0x03, exc3 );
	set_trap_gate( 0x04, exc4 );
	set_trap_gate( 0x05, exc5 );
	set_trap_gate( 0x06, exc6 );
	set_trap_gate( 0x07, exc7 );
	set_trap_gate( 0x08, exc8 );
	set_trap_gate( 0x09, exc9 );
	set_trap_gate( 0x0a, exca );
	set_trap_gate( 0x0b, excb );
	set_trap_gate( 0x0c, excc );
	set_trap_gate( 0x0d, excd );
	set_trap_gate( 0x0e, exce );
	set_trap_gate( 0x10, exc10 );
	set_trap_gate( 0x11, exc11 );
	set_trap_gate( 0x12, exc12 );
	set_trap_gate( 0x13, exc13 );

	set_interrupt_gate( INT_SCHEDULE, smp_preempt );
	set_interrupt_gate( INT_INVAL_PGT, smp_invalidate_pgt );
	set_interrupt_gate( INT_SPURIOUS, smp_spurious_irq );

	set_interrupt_gate( 0x20, TSIHand );

	set_interrupt_gate( 0x21, irq1 );
	set_interrupt_gate( 0x22, irq2 );
	set_interrupt_gate( 0x23, irq3 );
	set_interrupt_gate( 0x24, irq4 );
	set_interrupt_gate( 0x25, irq5 );
	set_interrupt_gate( 0x26, irq6 );
	set_interrupt_gate( 0x27, irq7 );
	set_interrupt_gate( 0x28, irq8 );
	set_interrupt_gate( 0x29, irq9 );
	set_interrupt_gate( 0x2a, irqa );
	set_interrupt_gate( 0x2b, irqb );
	set_interrupt_gate( 0x2c, irqc );
	set_interrupt_gate( 0x2d, irqd );
	set_interrupt_gate( 0x2e, irqe );
	set_interrupt_gate( 0x2f, irqf );

	set_trap_gate( 0x80, SysCallEntry );
	set_trap_gate( 0x81, probe );
}

int get_boot_module_count( void )
{
	return ( g_sSysBase.ex_nBootModuleCount );
}

BootModule_s *get_boot_module( int nIndex )
{
	if ( nIndex < 0 || nIndex >= g_sSysBase.ex_nBootModuleCount )
	{
		return ( NULL );
	}
	return ( g_sSysBase.ex_asBootModules + nIndex );
}

void put_boot_module( BootModule_s * psModule )
{
}

int get_kernel_arguments( int *argc, const char *const **argv )
{
	*argc = g_nKernelArgCount;
	*argv = g_apzKernelArgs;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

#ifndef __ENABLE_DEBUG__
#define __ENABLE_DEBUG__
#endif

#include <atheos/kdebug.h>
#include <atheos/udelay.h>

#undef DEBUG_LIMIT
#define DEBUG_LIMIT   KERN_DEBUG_LOW

static int find_boot_dev( void )
{
	char *pzDiskPathBuf;
	bool bFound = false;

	pzDiskPathBuf = kmalloc( 4096, MEMF_KERNEL );

	if ( NULL == pzDiskPathBuf )
	{
		kerndbg( KERN_PANIC, "Unable to allocate buffer memory.\n" );
		if ( NULL != pzDiskPathBuf )
			kfree( pzDiskPathBuf );

		return ( -ENOMEM );
	}

	do
	{
		char *azDevs[] = { "/dev/disk/ata", "/dev/disk/scsi", NULL };
		for ( int n = 0; azDevs[n] != NULL; n++ )
		{
			int hDevDir, nError = 0;
			char *zDev = azDevs[n];

			hDevDir = open( zDev, O_RDONLY );
			if ( hDevDir < 0 )
				continue;
			else
			{
				struct kernel_dirent sDiskDirEnt;

				printk( "Checking under %s\n", zDev );

				while ( getdents( hDevDir, &sDiskDirEnt, sizeof( sDiskDirEnt ) ) == 1 && bFound == false )
				{
					if ( strcmp( sDiskDirEnt.d_name, "." ) == 0 || strcmp( sDiskDirEnt.d_name, ".." ) == 0 )
						continue;

					if ( sDiskDirEnt.d_name[0] != 'c' )	/* Ignore any drives which are not CD's */
						continue;

					strcpy( pzDiskPathBuf, zDev );
					strcat( pzDiskPathBuf, "/" );
					strcat( pzDiskPathBuf, sDiskDirEnt.d_name );
					strcat( pzDiskPathBuf, "/raw" );

					kerndbg( KERN_INFO, "Checking for disk in %s\n", pzDiskPathBuf );

					/* Try to mount the disk */
					nError = sys_mount( pzDiskPathBuf, "/boot", g_zBootFS, MNTF_SLOW_DEVICE, g_zBootFSArgs );

					if ( nError < 0 )
					{
						kerndbg( KERN_INFO, "Unable to mount %s\n", pzDiskPathBuf );
						continue;
					}

					kerndbg( KERN_INFO, "Found disk in %s\n", pzDiskPathBuf );
					strcpy( g_zBootDev, pzDiskPathBuf );
					bFound = true;
				}

				close( hDevDir );
			}
		}

		if( bFound == false )
			snooze( 1000000 );
	}
	while( bFound == false );

	kfree( pzDiskPathBuf );

	return ( bFound ? 0 : -1 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int kernel_init( void )
{
	g_sSysBase.ex_hInitProc = sys_get_thread_id( NULL );
	
	boot_ap_processors();

	start_timer_int();	/* Start timer interrupt.      */

	printk( "Enable interrupts\n" );
	sti();
	
	
	get_cmos_time();

	init_scheduler();

	init_kernel_config();	/* Load kernel configuration */

	unprotect_dos_mem();

	init_debugger( g_nDebugBaudRate, g_nDebugSerialPort );
	init_vfs_module();
	init_devices_boot();	/* Initialize devices manager */
	init_elf_loader();

	init_block_cache();
	
	printk( "Mount boot FS: %s %s %s\n", g_zBootDev, g_zBootFS, g_zBootFSArgs );
	mkdir( "/boot", 0 );

	if ( strcmp( g_zBootDev, "@boot" ) == 0 )
	{
		if ( find_boot_dev() < 0 )
		{
			printk( "Unable to find boot device\n" );
		}
		else
			printk( "Found boot device\n" );
	}
	else if ( sys_mount( g_zBootDev, "/boot", g_zBootFS, 0, g_zBootFSArgs ) < 0 )
	{
		printk( "Error: failed to mount boot file system.\n" );
	}

	g_bRootFSMounted = true;
	protect_dos_mem();

	sti();

	init_devices();	/* Load busmanagers */
	
	if( g_bSwapEnabled )
		init_swapper(); /* Initialize swapper */
		
	return ( 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void system_init( void )
{
	char *apzBootShellArgs[32], **ppzArg = apzBootShellArgs;
	int i;
	
	if ( Fork( "init" ) != 0 )
	{
		set_thread_priority( 0, -999 );
		idle_loop();
		printk( "Panic: idle_loop() returned!!!\n" );
		return;
	}
	
	kernel_init();

	register_debug_cmd( "umode", "umode mode - Set single(0)/multi(1) user mode", dbg_set_user_mode );

	/* Create command line for init */
	*ppzArg++ = "init";

	if ( g_pzAppserver != NULL )
	{
		*ppzArg++ = "-p";
		*ppzArg++ = g_pzAppserver;
	}

	/* Pass in the boot mode */
	*ppzArg++ = g_zBootMode;

	*ppzArg = NULL;

	chdir( "/" );

	init_ip();

	printk( "Start init host\n" );
	if ( Fork( "sh-host" ) == 0 )
	{
		int anPipe[2];
		int nStdIn;

		sys_pipe( anPipe );

		if ( Fork( "boot-shell" ) == 0 )
		{
			char zSysLibPath[256];
			int nPathLen;

			if ( setsid() < 0 )
			{
				printk( "setsid() failed\n" );
			}

			nStdIn = sys_open( "/dev/null", O_RDWR );

			kassertw( nStdIn >= 0 );
			sys_dup2( nStdIn, 0 );
			sys_dup2( anPipe[1], 1 );
			sys_dup2( anPipe[1], 2 );


			for ( i = 3; i < 256; ++i )
			{
				sys_close( i );
			}


			strcpy( zSysLibPath, "DLL_PATH=" );
			sys_get_system_path( zSysLibPath + 9, 128 );
			nPathLen = strlen( zSysLibPath );

			if ( '/' != zSysLibPath[nPathLen - 1] )
			{
				zSysLibPath[nPathLen] = '/';
				zSysLibPath[nPathLen + 1] = '\0';
			}
			strcat( zSysLibPath, "system/libraries/:/system/indexes/lib/" );

			for ( i = 0; i < 255; ++i )
			{
				if ( g_apzEnviron[i] == NULL )
				{
					g_apzEnviron[i] = zSysLibPath;
					g_apzEnviron[i + 1] = NULL;
				}
			}

			printk( "Starting init...\n" );
			execve( "/boot/system/bin/init", apzBootShellArgs, g_apzEnviron );
			printk( "Failed to load boot-shell\n" );
		}
		else
		{
			char zBuffer[256];
			File_s *psFile = get_fd( false, anPipe[0] );

			sys_close( anPipe[0] );
			for ( ;; )
			{
				int nBytesRead = read_p( psFile, zBuffer, 256 );

				if ( nBytesRead <= 0 )
				{
					snooze( 1000000 );
					printk( "... boot shell finished\n" );
					put_fd( psFile );
					do_exit( 0 );
				}
				debug_write( zBuffer, nBytesRead );
			}
		}
	}
	snooze( 2000000 );
	for ( ;; )
	{
		sys_wait4( -1, NULL, 0, NULL );
	}
	do_exit( 0 );
}


bool get_str_arg( char *pzValue, const char *pzName, const char *pzArg, int nArgLen )
{
	int nNameLen = strlen( pzName );

	if ( nNameLen >= nArgLen )
	{
		return ( false );
	}

	if ( strncmp( pzArg, pzName, nNameLen ) == 0 )
	{
		memcpy( pzValue, pzArg + nNameLen, nArgLen - nNameLen );
		pzValue[nArgLen - nNameLen] = '\0';
		return ( true );
	}
	return ( false );
}

bool get_num_arg( uint32 *pnValue, const char *pzName, const char *pzArg, int nArgLen )
{
	char zBuffer[256];

	if ( get_str_arg( zBuffer, pzName, pzArg, nArgLen ) == false )
	{
		return ( false );
	}
	*pnValue = parse_num( zBuffer );
	return ( true );
}

bool get_bool_arg( bool *pbValue, const char *pzName, const char *pzArg, int nArgLen )
{
	char zBuffer[256];

	if ( get_str_arg( zBuffer, pzName, pzArg, nArgLen ) == false )
	{
		return ( false );
	}
	if ( stricmp( zBuffer, "false" ) == 0 )
	{
		*pbValue = false;
		return ( true );
	}
	else if ( stricmp( zBuffer, "true" ) == 0 )
	{
		*pbValue = true;
		return ( true );
	}
	return ( false );
}

//****************************************************************************/
/** Parses the given kernel arguments.
 * Called by init_kernel() (init.c).
 * \internal
 * \ingroup Init
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
static void parse_kernel_params( char *pzParams )
{
	const char *pzArg = pzParams;
	int argc = 0;
	int i;
	
	if( !pzParams )
		return;
	
	printk( "  Kernel arguments '%s'\n", g_zKernelParams );

	for ( i = 0;; ++i )
	{
		int nLen;

		if ( pzParams[i] != '\0' && isspace( pzParams[i] ) == false )
		{
			continue;
		}
		if ( g_nKernelArgCount < MAX_KERNEL_ARGS )
		{
			g_apzKernelArgs[g_nKernelArgCount++] = pzArg;
		}
		nLen = pzParams + i - pzArg;
		if( get_str_arg( g_zBootDev, "root=", pzArg, nLen ) )
			printk( "  Boot dev:        '%s'\n", g_zBootDev );
		if( get_str_arg( g_zBootFS, "rootfs=", pzArg, nLen ) )
			printk( "  Boot fs:         '%s'\n", g_zBootFS );
		if( get_str_arg( g_zBootFSArgs, "rootfsargs=", pzArg, nLen ) )
			printk( "  Boot fs args:    '%s'\n", g_zBootFSArgs );
		if( get_str_arg( g_zBootMode, "bootmode=", pzArg, nLen ) )
			printk( "  Boot mode:       '%s'\n", g_zBootMode );
		if ( get_str_arg( g_zAppServerPath, "appserver=", pzArg, nLen ) )
		{
			g_pzAppserver = g_zAppServerPath;
			printk( "  Appserver Path:  '%s'\n", g_pzAppserver == NULL ? "<default>" : g_pzAppserver );
		}

		if( get_num_arg( &g_nMemSize, "memsize=", pzArg, nLen ) )
			printk( "  MemSize:          %d\n", g_nMemSize );
		if( get_num_arg( &g_nDebugBaudRate, "debug_baudrate=", pzArg, nLen ) )
			printk( "  Debug baudrate:   %d\n", g_nDebugBaudRate );
		if( get_num_arg( &g_nDebugSerialPort, "debug_port=", pzArg, nLen ) )
			printk( "  Debug port        %d\n", g_nDebugSerialPort );
		if( get_bool_arg( &g_bPlainTextDebug, "debug_plaintext=", pzArg, nLen ) )
			printk( "  Debug mode:       %s\n", ( g_bPlainTextDebug ) ? "plain text" : "packet protocol" );
		if( get_bool_arg( &g_bDisableSMP, "disable_smp=", pzArg, nLen ) ) 
			printk( "  SMP scan is %s\n", ( ( g_bDisableSMP ) ? "disabled" : "enabled" ) );
		if( get_bool_arg( &g_bDisableACPI, "disable_acpi=", pzArg, nLen ) ) 
			printk( "  ACPI SMP scan is %s\n", ( ( g_bDisableACPI ) ? "disabled" : "enabled" ) );
		if( get_bool_arg( &g_bDisableKernelConfig, "disable_config=", pzArg, nLen ) )
			printk( "  Kernel config is %s\n", ( ( g_bDisableKernelConfig ) ? "disabled" : "enabled" ) );
		if( get_bool_arg( &g_bSwapEnabled, "enable_swap=", pzArg, nLen ) )
			printk( "  Swapping is %s\n", ( ( g_bSwapEnabled ) ? "enabled" : "disabled" ) );
		if( get_bool_arg( &g_bDisableGFXDrivers, "disable_gfx_drivers=", pzArg, nLen ) )
			printk( "  Graphics drivers are %s\n", ( ( g_bDisableGFXDrivers ) ? "disabled" : "enabled" ) );
					
		if( get_num_arg( &g_sSysBase.sb_nFirstUserAddress, "uspace_start=", pzArg, nLen ) )
			printk( "  UAS start:        %08x\n", g_sSysBase.sb_nFirstUserAddress );
		if( get_num_arg( &g_sSysBase.sb_nLastUserAddress, "uspace_end=", pzArg, nLen ) )
			printk( "  UAS end:          %08x\n", g_sSysBase.sb_nLastUserAddress );

		if( get_num_arg( (uint32*)&g_nPrintkMax, "printk_max=", pzArg, nLen ) )
			printk( "  Printk limit: %i\n", g_nPrintkMax );

		if ( pzParams[i] == '\0' )
		{
			break;
		}
		pzParams[i] = '\0';
		pzArg = pzParams + i + 1;
		argc++;
	}
	if ( g_sSysBase.sb_nFirstUserAddress < AREA_FIRST_USER_ADDRESS )
	{
		g_sSysBase.sb_nFirstUserAddress = AREA_FIRST_USER_ADDRESS;
	}
	if ( g_sSysBase.sb_nLastUserAddress < g_sSysBase.sb_nFirstUserAddress )
	{
		g_sSysBase.sb_nLastUserAddress = AREA_LAST_USER_ADDRESS;
	}
	printk( "Loaded kernel modules:\n" );
	for ( i = 0; i < g_sSysBase.ex_nBootModuleCount; ++i )
	{
		printk( "  %02d : %p -> %p (%08x)  '%s'\n", i, g_sSysBase.ex_asBootModules[i].bm_pAddress, ( char * )g_sSysBase.ex_asBootModules[i].bm_pAddress + g_sSysBase.ex_asBootModules[i].bm_nSize - 1, g_sSysBase.ex_asBootModules[i].bm_nSize, g_sSysBase.ex_asBootModules[i].bm_pzModuleArgs );
	}
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int init_kernel( char *pRealMemBase, int nKernelSize )
{
	char zVersion[10];
	
	sprintf( zVersion, "%d.%d.%d", ( int )( ( g_nKernelVersion >> 32 ) & 0xffff ), 
				( int )( ( g_nKernelVersion >> 16 ) % 0xffff ), ( int )( g_nKernelVersion & 0xffff ) );

	printk( "Syllable %s build %s starting...\n", zVersion, g_pzBuildVersion );

	nKernelSize = ( nKernelSize + PAGE_SIZE - 1 ) & PAGE_MASK;
	
	cli();

	dbcon_set_color( 0, 0, 0, 170 );
	dbcon_clear();
	
	parse_kernel_params( g_zKernelParams );

	set_debug_port_params( g_nDebugBaudRate, g_nDebugSerialPort, g_bPlainTextDebug );

	g_sSysBase.ex_nTotalPageCount = g_nMemSize / PAGE_SIZE;
	
	v86Stack_seg = ( ( uint32 )pRealMemBase ) >> 4;
	v86Stack_off = 65500;	// 65532;

	pRealMemBase += 65536;
	
	init_cpuid();
	init_descriptors();
	init_interrupt_table();
	init_irq_controller();
	init_pages( KERNEL_LOAD_ADDR + nKernelSize );

	kassertw( sizeof( MemContext_s ) <= PAGE_SIZE );	/* Late compile-time check :) */
	kassertw( ( get_cpu_flags() & EFLG_IF ) == 0 );
	
	init_kmalloc();
	
	init_kernel_mem_context();
	
	g_sSysBase.ex_pNullPage = ( char * )get_free_page( MEMF_CLEAR );

	kassertw( ( get_cpu_flags() & EFLG_IF ) == 0 );

	/* Initialize the descriptors and enable the mmu */
	set_page_directory_base_reg( g_psKernelSeg->mc_pPageDir );
	enable_mmu();

	kassertw( ( get_cpu_flags() & EFLG_IF ) == 0 );
	
	g_sSysBase.ex_bSingleUserMode = false;

	init_semaphores();
	init_debugger_locks();	
	init_resource_manager();	
	init_memory_pools( pRealMemBase, &g_sMultiBootHeader );
	init_msg_ports();
	init_processes();
	init_threads();
	init_areas();

	init_smp( g_bDisableSMP == false, g_bDisableACPI == false );
	
	
	/* Create the semaphore for the kernel memory context */
	g_psKernelSeg->mc_hSema = create_semaphore( "krn_seg_lock", 1, SEM_RECURSIVE );
	if ( g_psKernelSeg->mc_hSema < 0 )
	{
		printk( "Panic : Unable to create semaphore for kernel segment!\n" );
	}

	create_init_proc( "kernel" );

	printk( "Unable to init kernel!\n" );
	return ( 0 );
}



void init_kernel_mb( MultiBootHeader_s * psInfo )
{
	uint32 nKernelSize = ( ( int )&_end ) - KERNEL_LOAD_ADDR;

	g_sMultiBootHeader = *psInfo;
	g_nMemSize = psInfo->mem_upper * 1024 + 1024 * 1024;
	
	/* HACK: Limit memory size to 1.5Gb */
	if( g_nMemSize > 1024 * 1024 * 1500 )
	{
		g_nMemSize = 1024 * 1024 * 1500;
		printk( "WARNING: Memory size limited to 1.5Gb\n" );
	}

	if ( psInfo->mbh_pzKernelParams != NULL )
	{
		strcpy( g_zKernelParams, psInfo->mbh_pzKernelParams );
	}
	g_sSysBase.sb_nFirstUserAddress = AREA_FIRST_USER_ADDRESS;
	g_sSysBase.sb_nLastUserAddress = AREA_LAST_USER_ADDRESS;

	/* Parse bootmodules */
	if ( psInfo->mbh_nFlags & MB_INFO_MODS )
	{
		int i;
		int nTotArgSize = 0;

		g_sSysBase.ex_nBootModuleCount = min( psInfo->mbh_nModuleCount, MAX_BOOTMODULE_COUNT );
		for ( i = 0; i < g_sSysBase.ex_nBootModuleCount; ++i )
		{
			int nArgLen = strlen( psInfo->mbh_psFirstModule[i].bm_pzModuleArgs ) + 1;

			g_sSysBase.ex_asBootModules[i].bm_pAddress = psInfo->mbh_psFirstModule[i].bm_pStart;
			g_sSysBase.ex_asBootModules[i].bm_nSize = psInfo->mbh_psFirstModule[i].bm_pEnd - psInfo->mbh_psFirstModule[i].bm_pStart;
			if ( nTotArgSize + nArgLen < MAX_BOOTMODULE_ARGUMENT_SIZE )
			{
				g_sSysBase.ex_asBootModules[i].bm_pzModuleArgs = g_sSysBase.ex_anBootModuleArgs + nTotArgSize;
				nTotArgSize += nArgLen;
				memcpy( ( char * )g_sSysBase.ex_asBootModules[i].bm_pzModuleArgs, psInfo->mbh_psFirstModule[i].bm_pzModuleArgs, nArgLen );
			}
			else
			{
				g_sSysBase.ex_asBootModules[i].bm_pzModuleArgs = "";
			}
			g_sSysBase.ex_asBootModules[i].bm_hAreaID = -1;
		}
	}

	init_kernel( ( char * )0x10000, nKernelSize );
}

