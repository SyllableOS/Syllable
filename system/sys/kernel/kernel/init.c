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

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/syscall.h>
#include <atheos/smp.h>
#include <atheos/irq.h>
#include <atheos/semaphore.h>
#include <atheos/bcache.h>
#include <atheos/multiboot.h>

#include <net/net.h>

#include <macros.h>

#include "vfs/vfs.h"
#include "inc/scheduler.h"
#include "inc/areas.h"
#include "inc/bcache.h"
#include "inc/smp.h"
#include "inc/sysbase.h"

extern int _end;

#define KERNEL_LOAD_ADDR 0x100000
#define MAX_KERNEL_ARGS	128
static MultiBootHeader_s g_sMultiBootHeader;
static char* 	g_apzEnviron[256];	// Environment from init script
char		g_zSysPath[256]    = "/boot/atheos/";
static char	g_zUsrPath[256] = "/boot/atheos/usr/";
static char	g_zBootMode[256] = "normal";
static char	g_zKernelParams[4096];
static const char*	g_apzKernelArgs[MAX_KERNEL_ARGS];
static int	g_nKernelArgCount = 0;
static char	g_zBootFS[256] = "afs";
static char	g_zBootDev[256];
static char	g_zBootFSArgs[256];
static uint32	g_nDebugBaudRate  = 0; //115200;
static uint32	g_nDebugSerialPort = 2;
static bool	g_bPlainTextDebug = false;
static uint32	g_nMemSize  	  = 64 * 1024 * 1024;
static bool	g_bDisablePCI = false;
static bool	g_bDisableSMP = false;
static bool	g_bFoundSmpConfig = false;
static struct i3Task g_sInitialTSS;

bool g_bRootFSMounted = false;
bool g_bKernelInitiated = false;

uint32	g_nFirstUserAddress = AREA_FIRST_USER_ADDRESS;
uint32	g_nLastUserAddress  = AREA_LAST_USER_ADDRESS;

void init_elf_loader();
void idle_loop();

extern	void	StartTimerInt( void );

extern	void	SysCallEntry( void );
extern	void	probe( void );
extern	void	exc0( void );
extern	void	exc1( void );
extern	void	exc2( void );
extern	void	exc3( void );
extern	void	exc4( void );
extern	void	exc5( void );
extern	void	exc6( void );
extern	void	exc7( void );
extern	void	exc8( void );
extern	void	exc9( void );
extern	void	exca( void );
extern	void	excb( void );
extern	void	excc( void );
extern	void	excd( void );
extern	void	exce( void );

extern	void	irq0( void );
extern	void	irq1( void );
extern	void	irq2( void );
extern	void	irq3( void );
extern	void	irq4( void );
extern	void	irq5( void );
extern	void	irq6( void );
extern	void	irq7( void );
extern	void	irq8( void );
extern	void	irq9( void );
extern	void	irqa( void );
extern	void	irqb( void );
extern	void	irqc( void );
extern	void	irqd( void );
extern	void	irqe( void );
extern	void	irqf( void );

extern	void (*TSIHand)(void);

extern  void	smp_preempt( void );
extern  void	smp_invalidate_pgt( void );
extern  void	smp_spurious_irq( void );

MemContext_s*	g_psKernelSeg = NULL;

extern uint32	v86Stack_seg;
extern uint32	v86Stack_off;
extern uint32   g_anKernelStackEnd[];

int Fork( const char* pzName )
{
    int nError;
    __asm__ volatile( "int $0x80" : "=a" (nError) : "0" (__NR_Fork), "b" ((int)pzName) );
    return( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void dbg_set_user_mode( int argc, char** argv )
{
    if ( argc < 2 ) {
	dbprintf( DBP_DEBUGGER, "System is now in %s user mode\n",
		  (g_sSysBase.ex_bSingleUserMode) ? "single" : "multi" );
    } else {
	g_sSysBase.ex_bSingleUserMode = atol( argv[1] ) != 0;
    }
    dbprintf( DBP_DEBUGGER, "System is now in %s user mode\n", (g_sSysBase.ex_bSingleUserMode) ? "single" : "multi" );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int parse_num( const char* pzString )
{
    int nValue;
    int i;

    nValue = strtoul( pzString, NULL, 0 );
    
    for ( i = 0 ; isdigit( pzString[i] ) ; ++i );
  
    if ( pzString[i] == 'k' || pzString[i] == 'K' ) {
	nValue *= 1024;
    } else if ( pzString[i] == 'M' ) {
	nValue *= 1024 * 1024;
    }
    return( nValue );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int set_interrupt_gate( uint16 IntNum, void *Handler )
{
    g_sSysBase.ex_IDT[ IntNum ].igt_sel  = 0x08;
    g_sSysBase.ex_IDT[ IntNum ].igt_offl = ((uint32)Handler) & 0xffff;
    g_sSysBase.ex_IDT[ IntNum ].igt_offh = ((uint32)Handler) >> 16;
    g_sSysBase.ex_IDT[ IntNum ].igt_ctrl = 0x8e00;
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int set_trap_gate( int IntNum, void *Handler )
{
    g_sSysBase.ex_IDT[ IntNum ].igt_sel	= 0x08;
    g_sSysBase.ex_IDT[ IntNum ].igt_offl	= ((uint32)Handler) & 0xffff;
    g_sSysBase.ex_IDT[ IntNum ].igt_offh	= ((uint32)Handler) >> 16;

    g_sSysBase.ex_IDT[ IntNum ].igt_ctrl	= 0xef00;
    return( 0 );
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
  
    set_interrupt_gate( INT_SCHEDULE,  smp_preempt );
    set_interrupt_gate( INT_INVAL_PGT, smp_invalidate_pgt );
    set_interrupt_gate( INT_SPURIOUS,  smp_spurious_irq );

    set_interrupt_gate( 0x20, &TSIHand );
  
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
    return( g_sSysBase.ex_nBootModuleCount );
}

BootModule_s* get_boot_module( int nIndex )
{
    if ( nIndex < 0 || nIndex >= g_sSysBase.ex_nBootModuleCount ) {
	return( NULL );
    }
    return( g_sSysBase.ex_asBootModules + nIndex );
}

void put_boot_module( BootModule_s* psModule )
{
}

int get_kernel_arguments( int* argc, const char* const** argv )
{
    *argc = g_nKernelArgCount;
    *argv = g_apzKernelArgs;
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int kernel_init()
{
    g_sSysBase.ex_hInitProc = sys_get_thread_id( NULL );
	
    init_scheduler();

    if ( g_bDisablePCI == false ) {
	printk( "Init PCI module\n" );
	init_pci_module();
    }

    unprotect_dos_mem();

    init_debugger( g_nDebugBaudRate, g_nDebugSerialPort );
    init_vfs_module();
    init_elf_loader();
    
    init_block_cache();
  
    printk( "Mount boot FS: %s %s %s\n", g_zBootDev, g_zBootFS, g_zBootFSArgs );
    mkdir( "/boot", 0 );
    if ( sys_mount( g_zBootDev, "/boot", g_zBootFS, 0, g_zBootFSArgs ) < 0 ) {
	printk( "Error: failed to mount boot file system.\n" );
    }
    g_bRootFSMounted = true;
    protect_dos_mem();

    sti();
  
    return( 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void SysInit( void )
{
    char* apzBootShellArgs[32];
    char  zBootScriptPath[256];
    int   i;

    if ( Fork( "init" ) != 0 ) {
	set_thread_priority( 0, -999 );
	idle_loop();
	printk( "Panic: idle_loop() returned!!!\n" );
	return;
    }

    if ( g_bFoundSmpConfig ) {
	boot_ap_processors();
    }
    cli();
    for ( i = 0 ; i < MAX_CPU_COUNT ; ++i ) {
	if ( g_asProcessorDescs[i].pi_bIsPresent && g_asProcessorDescs[i].pi_bIsRunning ) {
	    g_asProcessorDescs[i].pi_nGS	= Desc_Alloc( 0 );
	    Desc_SetLimit(  g_asProcessorDescs[i].pi_nGS, TLD_SIZE );
	    Desc_SetBase(   g_asProcessorDescs[i].pi_nGS, 0 );
	    Desc_SetAccess( g_asProcessorDescs[i].pi_nGS, 0xf2 );
	    printk( "CPU #%d got GS=%ld\n", i, g_asProcessorDescs[i].pi_nGS );
	}
    }

    StartTimerInt();	 /* Start timer interrupt.	*/

    printk( "Enable interrupts\n" );
    sti();
    
    GetCMOSTime();

    kernel_init();
    

    register_debug_cmd( "umode", "umode mode - Set single(0)/multi(1) user mode", dbg_set_user_mode );
    
    sys_get_system_path( zBootScriptPath, 256 );

    strcat( zBootScriptPath, "sys/init.sh" );
    
    apzBootShellArgs[0] = "sh";
    apzBootShellArgs[1] = "-noprofile";
    apzBootShellArgs[2] = zBootScriptPath;
    apzBootShellArgs[3] = g_zBootMode;
    apzBootShellArgs[4] = g_zSysPath;
    apzBootShellArgs[5] = g_zUsrPath;
    apzBootShellArgs[6] = NULL;

    apzBootShellArgs[0] = "init";
    apzBootShellArgs[1] = NULL;
    
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
	    int  nPathLen;
		
	    if ( setsid() < 0 ) {
		printk( "setsid() failed\n" );
	    }
	
	    nStdIn = sys_open( "/dev/null", O_RDWR );

	    kassertw( nStdIn >= 0 );
	    sys_dup2( nStdIn, 0 );
	    sys_dup2( anPipe[1], 1 );
	    sys_dup2( anPipe[1], 2 );


	    for ( i = 3 ; i < 256 ; ++i ) {
		sys_close( i );
	    }


	    strcpy( zSysLibPath, "DLL_PATH=" );
	    sys_get_system_path( zSysLibPath + 9, 128 );
	    nPathLen = strlen( zSysLibPath );

	    if ( '/' != zSysLibPath[ nPathLen - 1 ] ) {
		zSysLibPath[ nPathLen ] = '/';
		zSysLibPath[ nPathLen + 1 ] = '\0';
	    }
	    strcat( zSysLibPath, "sys/libs/" );

	    for ( i = 0 ; i < 255 ; ++i ) {
		if ( g_apzEnviron[i] == NULL ) {
		    g_apzEnviron[i]   = zSysLibPath;
		    g_apzEnviron[i+1] = NULL;
		}
	    }
	    printk( "start init...\n" );
	    execve( "/boot/atheos/sys/bin/init", apzBootShellArgs, g_apzEnviron );
	    printk( "Failed to load boot-shell\n" );
	}
	else
	{
	    char zBuffer[256];
	    File_s* psFile = get_fd( false, anPipe[0] );
	    sys_close( anPipe[0] );
	    for (;;)
	    {
		int nBytesRead = read_p( psFile, zBuffer, 256 );
		if ( nBytesRead <= 0 ) {
		    snooze( 1000000 );
		    printk( "...boot shell finnished\n" );
		    put_fd( psFile );
		    do_exit( 0 );
		}
		debug_write( zBuffer, nBytesRead );
	    }
	}
    }
    snooze( 2000000 );
    for (;;) {
	sys_wait4( -1, NULL, 0, NULL );
    }
    do_exit( 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int init_kernel( char* pRealMemBase, int nKernelSize )
{
    int 		i;
    struct i3DescrTable	IDT;
  
    SetColor( 0, 255, 255, 255 );

    nKernelSize = (nKernelSize + PAGE_SIZE - 1) & PAGE_MASK;

    dbcon_clear();
  
    SetColor( 0, 255, 0, 128 );

    cli();

    set_debug_port_params( g_nDebugBaudRate, g_nDebugSerialPort, g_bPlainTextDebug );

    if( sizeof(DescriptorTable_s) == 8 ) {
	int	nCol;
	for (;;) {
	    SetColor( 0, 0, nCol++, 0 );
	}
    }

    if( sizeof(DescriptorTable_s) != 6 ) {
	int	nCol;
	for (;;) {
	    SetColor( 0, nCol++, 0, 0 );
	}
    }

    g_sSysBase.ex_nTotalPageCount	= g_nMemSize / PAGE_SIZE;

    v86Stack_seg  = ((uint32) pRealMemBase) >> 4;
    v86Stack_off  = 65500; // 65532;

    pRealMemBase += 65536;

    printk( "Initialize the AtheOS kernel\n" );
  
    IDT.Base  = (uint32) g_sSysBase.ex_IDT;
    IDT.Limit = 0x7ff;
    SetIDT( &IDT );

   
    Desc_SetBase(   CS_KERNEL, 0x00000000 );
    Desc_SetLimit(  CS_KERNEL, 0xffffffff );
    Desc_SetAccess( CS_KERNEL, 0x9a );

    Desc_SetBase(   DS_KERNEL, 0x00000000 );
    Desc_SetLimit(  DS_KERNEL, 0xffffffff );
    Desc_SetAccess( DS_KERNEL, 0x92 );

    Desc_SetBase(   CS_USER, 0x00000000 );
    Desc_SetLimit(  CS_USER, 0xffffffff );
    Desc_SetAccess( CS_USER, 0xfa );
  
    Desc_SetBase(   DS_USER, 0x00000000 );
    Desc_SetLimit(  DS_USER, 0xffffffff );
    Desc_SetAccess( DS_USER, 0xf2 );

    memset( &g_sInitialTSS, 0, sizeof( g_sInitialTSS ) );

    g_sInitialTSS.cs 	 = 0x08;
    g_sInitialTSS.ss 	 = 0x18;
    g_sInitialTSS.ds 	 = 0x18;
    g_sInitialTSS.es 	 = 0x18;
    g_sInitialTSS.fs 	 = 0x18;
    g_sInitialTSS.gs 	 = 0x18;
    g_sInitialTSS.eflags = 0x203246;
    
    g_sInitialTSS.cr3	  = 0;
    g_sInitialTSS.ss0	  = 0x18;
    g_sInitialTSS.esp0	  = g_anKernelStackEnd;
    g_sInitialTSS.IOMapBase = 104;

    Desc_SetLimit( 0x40, 0xffff );
    Desc_SetBase( 0x40, (uint32) &g_sInitialTSS );
    Desc_SetAccess( 0x40, 0x89 );
    g_sSysBase.ex_GDT[0x40<<3].desc_lmh &= 0x8f;	// TSS descriptors has bit 22 clear (as opposed to 32 bit data and code descriptors)

    
    SetColor( 0, 0, 255, 0 );
  
    IDT.Base  = (uint32) g_sSysBase.ex_GDT;
    IDT.Limit = 0xffff;
    SetGDT( &IDT );
    SetTR( 0x40 );
    __asm__ volatile ( "mov %0,%%ds;mov %0,%%es;mov %0,%%fs;mov %0,%%gs;mov %0,%%ss;" :: "r" (0x18) );
    init_interrupt_table();

      /* mark the first descriptors in GDT as used	*/
    for ( i = 0 ; i < 8 + MAX_CPU_COUNT ; i++ ) {
	g_sSysBase.ex_DTAllocList[i] |= DTAL_GDT;
    }

    g_psFirstPage =  (Page_s*)(KERNEL_LOAD_ADDR + nKernelSize);

    for ( i = 0 ; i < g_sSysBase.ex_nBootModuleCount ; ++i ) {
	char* pModEnd = (char*)g_sSysBase.ex_asBootModules[i].bm_pAddress + ((g_sSysBase.ex_asBootModules[i].bm_nSize+PAGE_SIZE-1)&PAGE_MASK);
	if ( pModEnd > ((char*)g_psFirstPage) ) {
	    g_psFirstPage = (Page_s*)pModEnd;
	}
    }
    memset( g_psFirstPage, 0, g_sSysBase.ex_nTotalPageCount * sizeof(Page_s) );

    for ( i = 0 ; i < (KERNEL_LOAD_ADDR + nKernelSize) / PAGE_SIZE ; ++i ) {
	kassertw( g_psFirstPage[i].p_nCount == 0 );
	g_psFirstPage[i].p_nCount = 1;
    }
    for ( i = ((int)g_psFirstPage) / PAGE_SIZE ; i < (((int)(g_psFirstPage+g_sSysBase.ex_nTotalPageCount)) + PAGE_SIZE - 1) / PAGE_SIZE ; ++i ) {
	kassertw( g_psFirstPage[i].p_nCount == 0 );
	g_psFirstPage[i].p_nCount = 1;
    }
    for ( i = 0 ; i < g_sSysBase.ex_nBootModuleCount ; ++i ) {
	int32 nFirst = ((uint32)g_sSysBase.ex_asBootModules[i].bm_pAddress) / PAGE_SIZE;
	int32 nLast  = ((uint32)g_sSysBase.ex_asBootModules[i].bm_pAddress) + ((g_sSysBase.ex_asBootModules[i].bm_nSize+PAGE_SIZE-1)&PAGE_MASK);
	int j;
	nLast /= PAGE_SIZE;
	for ( j = nFirst ; j < nLast ; ++j ) {
	    kassertw( g_psFirstPage[j].p_nCount == 0 );
	    g_psFirstPage[j].p_nCount = 1;
	}
    }

    g_psFirstFreePage = 0;
    for ( i = g_sSysBase.ex_nTotalPageCount - 1 ; i >= 0 ; --i ) {
	g_psFirstPage[i].p_nPageNum = i;
	if ( g_psFirstPage[i].p_nCount == 0 ) {
	    protect_phys_pages( i * PAGE_SIZE, 1 );
	    g_psFirstPage[i].p_psNext = g_psFirstFreePage;
	    g_psFirstFreePage = &g_psFirstPage[i];
	    g_sSysBase.ex_nFreePageCount++;
	}
    }
	
    kassertw( sizeof( MemContext_s ) <= PAGE_SIZE ); /* Late compile-time check :) */
  
    init_kmalloc();
    
    printk( "Boot parameters:\n" );
    printk( "  Kernel arguments '%s'\n", g_zKernelParams );
    printk( "  Boot mode:       '%s'\n", g_zBootMode );
    printk( "  Boot fs:         '%s'\n", g_zBootFS );
    printk( "  Boot dev:        '%s'\n", g_zBootDev );
    printk( "  Boot fs args:    '%s'\n", g_zBootFSArgs );
    printk( "  Debug port        %ld\n", g_nDebugSerialPort );
    printk( "  Debug baudrate:   %ld\n", g_nDebugBaudRate );
    printk( "  Debug mode:       %s\n", (g_bPlainTextDebug) ? "plain text" : "packet protocol" );
    printk( "  MemSize:          %ld\n", g_nMemSize );
    printk( "  UAS start:        %08lx\n", g_sSysBase.sb_nFirstUserAddress );
    printk( "  UAS end:          %08lx\n", g_sSysBase.sb_nLastUserAddress );
    printk( "  PCI scan is %s\n", ((g_bDisablePCI) ? "disabled" : "enabled" ) );
    printk( "  SMP scan is %s\n", ((g_bDisableSMP) ? "disabled" : "enabled" ) );

    printk( "Loaded kernel modules:\n" );
    for ( i = 0 ; i < g_sSysBase.ex_nBootModuleCount ; ++i ) {
	printk( "  %02d : %p -> %p (%08lx)  '%s'\n", i, g_sSysBase.ex_asBootModules[i].bm_pAddress,
		(char*)g_sSysBase.ex_asBootModules[i].bm_pAddress + g_sSysBase.ex_asBootModules[i].bm_nSize - 1,
		g_sSysBase.ex_asBootModules[i].bm_nSize,
		g_sSysBase.ex_asBootModules[i].bm_pzModuleArgs );
    }

    g_psKernelSeg = (MemContext_s*) get_free_page( GFP_CLEAR );

    g_psKernelSeg->mc_pPageDir = (pgd_t*) get_free_page( GFP_CLEAR );

    for ( i = 0 ; i < (((g_sSysBase.ex_nTotalPageCount * PAGE_SIZE) + PGDIR_SIZE - 1) / PGDIR_SIZE) ; ++i ) {
	PGD_VALUE( g_psKernelSeg->mc_pPageDir[i] ) = MK_PGDIR( get_free_page( GFP_CLEAR ) );
    }
    kassertw( (get_cpu_flags() & EFLG_IF) == 0 );

    for ( i = 0 ; i < g_sSysBase.ex_nTotalPageCount ; ++i ) {
	pgd_t*	pPgd = pgd_offset( g_psKernelSeg, i * PAGE_SIZE );
	pte_t*	pPte = pte_offset( pPgd, i * PAGE_SIZE );
		
	PTE_VALUE( *pPte ) = ( i * PAGE_SIZE ) | PTE_PRESENT | PTE_WRITE;
    }

    g_sInitialTSS.cr3 = (void*) &g_psKernelSeg->mc_pPageDir;
    kassertw( (get_cpu_flags() & EFLG_IF) == 0 );


    g_psKernelSeg->mc_psNext = g_psKernelSeg;

    g_sSysBase.ex_pNullPage = (char*) get_free_page( GFP_CLEAR );
    SetColor( 0, 0, 0, 255 );

    kassertw( (get_cpu_flags() & EFLG_IF) == 0 );
		
      /*** enable MMU paging	***/

  
    printk( "Enable MMU\n" );
    set_page_directory_base_reg( g_psKernelSeg->mc_pPageDir );
    enable_mmu();
		
    kassertw( (get_cpu_flags() & EFLG_IF) == 0 );
	

    g_sSysBase.ex_sRealMemHdr.mh_First = (void*)pRealMemBase; // llfuncs->lomem_base;
    g_sSysBase.ex_sRealMemHdr.mh_Lower = g_sSysBase.ex_sRealMemHdr.mh_First;
    g_sSysBase.ex_sRealMemHdr.mh_Upper = (void*) (0xa0000 - 1); //(0xa0000 - (uint32)pRealMemBase - 1);
    g_sSysBase.ex_sRealMemHdr.mh_Free  = ((uint)g_sSysBase.ex_sRealMemHdr.mh_Upper) - ((uint)g_sSysBase.ex_sRealMemHdr.mh_Lower);

    g_sSysBase.ex_sRealMemHdr.mh_First->mc_Next  = NULL;
    g_sSysBase.ex_sRealMemHdr.mh_First->mc_Bytes = g_sSysBase.ex_sRealMemHdr.mh_Free;

    g_sSysBase.ex_bSingleUserMode = false;
  
    InitSemaphores();
    InitMsgPorts();
    InitProcesses();
    InitThreads();
    InitAreaManager();

    g_bFoundSmpConfig = init_smp( g_bDisableSMP == false );
  
    g_psKernelSeg->mc_hSema = create_semaphore( "krn_seg_lock", 1, SEM_REQURSIVE );

    if ( g_psKernelSeg->mc_hSema < 0 ) {
	printk( "Panic : Unable to create semaphore for kernel segment!\n" );
    }

    SetColor( 0, 255, 255, 0 );


    create_init_proc( "kernel" );
  
    printk("Unable to init kernel!\n");
    return( 0 );
}


bool get_str_arg( char* pzValue, const char* pzName, const char* pzArg, int nArgLen )
{
    int nNameLen = strlen( pzName );

    if ( nNameLen >= nArgLen ) {
	return( false );
    }
    
    if ( strncmp( pzArg, pzName, nNameLen ) == 0 ) {
	memcpy( pzValue, pzArg + nNameLen, nArgLen - nNameLen );
	pzValue[nArgLen - nNameLen] = '\0';
	return( true );
    }
    return( false );
}

bool get_num_arg( uint32* pnValue, const char* pzName, const char* pzArg, int nArgLen )
{
    char zBuffer[256];

    if ( get_str_arg( zBuffer, pzName, pzArg, nArgLen ) == false ) {
	return( false );
    }
    *pnValue = parse_num( zBuffer );
    return( true );
}

bool get_bool_arg( bool* pbValue, const char* pzName, const char* pzArg, int nArgLen )
{
    char zBuffer[256];

    if ( get_str_arg( zBuffer, pzName, pzArg, nArgLen ) == false ) {
	return( false );
    }
    if ( stricmp( zBuffer, "false" ) == 0 ) {
	*pbValue = false;
	return( true );
    } else if ( stricmp( zBuffer, "true" ) == 0 ) {
	*pbValue = true;
	return( true );
    }
    return( false );
}

static void parse_kernel_params( char* pzParams )
{
    const char* pzArg = pzParams;
    int   argc = 0;
    int   i;
    
    for ( i = 0 ; ; ++i ) {
	int nLen;
	if ( pzParams[i] != '\0' && isspace(pzParams[i]) == false ) {
	    continue;
	}
	if ( g_nKernelArgCount < MAX_KERNEL_ARGS ) {
	    g_apzKernelArgs[g_nKernelArgCount++] = pzArg;
	}
	nLen = pzParams + i - pzArg;
	get_str_arg( g_zBootDev, "root=", pzArg, nLen );
	get_str_arg( g_zBootFS, "rootfs=", pzArg, nLen );
	get_str_arg( g_zBootFSArgs, "rootfsargs=", pzArg, nLen );
	get_str_arg( g_zBootMode, "bootmode=", pzArg, nLen );
	get_num_arg( &g_nMemSize, "memsize=", pzArg, nLen );
	get_num_arg( &g_nDebugBaudRate, "debug_baudrate=", pzArg, nLen );
	get_num_arg( &g_nDebugSerialPort, "debug_port=", pzArg, nLen );
	get_bool_arg( &g_bPlainTextDebug, "debug_plaintext=", pzArg, nLen );
        get_bool_arg( &g_bDisablePCI, "disable_pci=", pzArg, nLen );
        get_bool_arg( &g_bDisableSMP, "disable_smp=", pzArg, nLen );

	get_num_arg( &g_sSysBase.sb_nFirstUserAddress, "uspace_start=", pzArg, nLen );
	get_num_arg( &g_sSysBase.sb_nLastUserAddress, "uspace_end=", pzArg, nLen );
	
	if ( pzParams[i] == '\0' ) {
	    break;
	}
	pzParams[i] = '\0';
	pzArg = pzParams + i + 1;
	argc++;
    }
    if ( g_sSysBase.sb_nFirstUserAddress < AREA_FIRST_USER_ADDRESS ) {
	g_sSysBase.sb_nFirstUserAddress = AREA_FIRST_USER_ADDRESS;
    }
    if ( g_sSysBase.sb_nLastUserAddress < g_sSysBase.sb_nFirstUserAddress ) {
	g_sSysBase.sb_nLastUserAddress = AREA_LAST_USER_ADDRESS;
    }
}


void init_kernel_mb( MultiBootHeader_s* psInfo )
{
    uint32 nKernelSize = ((int)&_end) - KERNEL_LOAD_ADDR;
    
    g_sMultiBootHeader = *psInfo;
    g_nMemSize = psInfo->mem_upper * 1024 + 1024*1024;

    if ( psInfo->mbh_pzKernelParams != NULL ) {
	strcpy( g_zKernelParams, psInfo->mbh_pzKernelParams );
    }
    g_sSysBase.sb_nFirstUserAddress = AREA_FIRST_USER_ADDRESS;
    g_sSysBase.sb_nLastUserAddress  = AREA_LAST_USER_ADDRESS;
    
    if ( psInfo->mbh_nFlags & MB_INFO_MODS ) {
	int i;
	int nTotArgSize = 0;
	g_sSysBase.ex_nBootModuleCount = min( psInfo->mbh_nModuleCount, MAX_BOOTMODULE_COUNT );
	for ( i = 0 ; i < g_sSysBase.ex_nBootModuleCount ; ++i ) {
	    int nArgLen = strlen( psInfo->mbh_psFirstModule[i].bm_pzModuleArgs ) + 1;
	    
	    g_sSysBase.ex_asBootModules[i].bm_pAddress  = psInfo->mbh_psFirstModule[i].bm_pStart;
	    g_sSysBase.ex_asBootModules[i].bm_nSize   = psInfo->mbh_psFirstModule[i].bm_pEnd - psInfo->mbh_psFirstModule[i].bm_pStart;
	    if ( nTotArgSize + nArgLen < MAX_BOOTMODULE_ARGUMENT_SIZE ) {
		g_sSysBase.ex_asBootModules[i].bm_pzModuleArgs = g_sSysBase.ex_anBootModuleArgs + nTotArgSize;
		nTotArgSize += nArgLen;
		memcpy( (char*)g_sSysBase.ex_asBootModules[i].bm_pzModuleArgs, psInfo->mbh_psFirstModule[i].bm_pzModuleArgs, nArgLen );
	    } else {
		g_sSysBase.ex_asBootModules[i].bm_pzModuleArgs = "";
	    }
	    g_sSysBase.ex_asBootModules[i].bm_hAreaID = -1;
	}
    }
    
    parse_kernel_params( g_zKernelParams );
    
    init_kernel( (char*)0x10000, nKernelSize );
}
