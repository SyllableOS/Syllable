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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/udelay.h>
#include <atheos/irq.h>
#include <atheos/spinlock.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/areas.h"
#include "inc/mc146818.h"
#include "inc/smp.h"
#include "inc/pit_timer.h"

//#define SMP_DEBUG




static uint32   g_nIoAPICAddr     = 0xFEC00000;		// Address of the I/O apic (not yet used)
bool		g_bAPICPresent	  = false;
static bool	g_bSmpActivated   = false;
int		g_nActiveCPUCount = 1;
int		g_nBootCPU 	  = 0;
ProcessorInfo_s	g_asProcessorDescs[MAX_CPU_COUNT];

static uint32 	g_nPhysAPICAddr = 0xFEE00000;		// Address of APIC (defaults to 0xFEE00000)
 // Alloc get_processor_id() to work on non-smp machines
 // Will be set to the virtual address of local APIC later if this is a SMP machine.
static uint32	g_nFakeCPUID = 0;
uint32		g_nVirtualAPICAddr  = ((uint32)&g_nFakeCPUID)-APIC_ID;
static area_id  g_hAPICArea;
static int	g_anLogicToRealID[MAX_CPU_COUNT] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
//void smp_ap_entry();

uint8	g_anTrampoline[] = {
#include "objs/smp_entry.hex"
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void idle_loop()
{
    char zThreadName[32];
    int	 nProcessor = 0;

    cli();
  
    nProcessor = get_processor_id();

    sprintf( zThreadName, "idle_%02d", nProcessor );
    rename_thread( -1, zThreadName );

    if ( g_bAPICPresent )
    {
	uint32 nAPICDiv;
	nAPICDiv = g_asProcessorDescs[nProcessor].pi_nBusSpeed / 1000;

	nAPICDiv +=  nProcessor * 10000; // Avoid re-scheduling all processors at the same time
#ifdef SMP_DEBUG
	printk( "APIC divisor = %d\n", nAPICDiv );
#endif  
	apic_write( APIC_LVTT, APIC_LVT_TIMER_PERIODIC | INT_SCHEDULE );    // Make APIC local timer trig int INT_SCHEDULE
	apic_write( APIC_TMICT, nAPICDiv ); // Start APIC timer
    }
    sti();
//  for(;;) Schedule();
//  for(;;);
    for(;;) __asm__("hlt");
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int read_pit_timer()
{
    int nCount;
  
    isa_writeb( 0x43, 0 );
    nCount  = isa_readb( 0x40 );
    nCount += isa_readb( 0x40 ) << 8;
    return( nCount );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void wait_pit_wrap()
{
    uint nCurrent = read_pit_timer();
    uint nPrev = 0;
  
    for (;;) {
	int nDelta;
    
	nPrev = nCurrent;
	nCurrent = read_pit_timer();
	nDelta = nCurrent - nPrev;

	if ( nDelta > 300 ) {
	    break;
	}
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void calibrate_delay(void)
{
    int 	 nLoopBit;
    int 	 nPresicion = 8;
    uint32 nLoopsPerSec;

    nLoopsPerSec = (1<<12);

    printk( "Calibrating delay loop for CPU %d\n", get_processor_id() );

    while ( nLoopsPerSec <<= 1) {
	isa_writeb( 0x43, 0x30 );	// one-shot mode
	isa_writeb( 0x40, 0xff );
	isa_writeb( 0x40, 0xff );
    
	__delay( nLoopsPerSec );
    
	if ( read_pit_timer() < 64000 ) { // to long
	    break;
	}
    }

    nLoopsPerSec >>= 1;
    nLoopBit = nLoopsPerSec;
  
    while ( nPresicion-- && (nLoopBit >>= 1) ) {
	nLoopsPerSec |= nLoopBit;
    
	isa_writeb( 0x43, 0x30 );	// one-shot mode
	isa_writeb( 0x40, 0xff );
	isa_writeb( 0x40, 0xff );
	__delay( nLoopsPerSec );
	if (read_pit_timer() < 64000) {	// to long
	    nLoopsPerSec &= ~nLoopBit;
	}
    }

    isa_writeb( 0x43, 0x34 );	// loop mode
    isa_writeb( 0x40, 0x0 );
    isa_writeb( 0x40, 0x0 );
  
      // Get loops per sec, instead of loops per 65535 - 64000 pit ticks.
    nLoopsPerSec *= PIT_TICKS_PER_SEC / (65535 - 64000);
    g_asProcessorDescs[get_processor_id()].pi_nDelayCount = nLoopsPerSec;
  
      // Round the value and print it.
    printk( "ok - %lu.%02lu BogoMIPS\n", (nLoopsPerSec+2500)/500000, ((nLoopsPerSec+2500)/5000) % 100 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
static void read_cpu_id( unsigned int nReg, unsigned int *pData )
{
	/* Read CPU ID */
	__asm __volatile
	("movl %%ebx, %%esi\n\t"
         "cpuid\n\t"
         "xchgl %%ebx, %%esi"
         : "=a" (pData[0]), "=S" (pData[1]), 
           "=c" (pData[2]), "=d" (pData[3])
         : "0" (nReg));
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
 
#include "inc/cputable.h" 

static void set_cpu_features()
{
	/* Save cpu name and features ( from mplayer ) */
	unsigned int nRegs[4];
	unsigned int nRegs2[4];
	char zVendor[17];
	int i;
	
	g_asProcessorDescs[g_nBootCPU].pi_nFeatures = CPU_FEATURE_NONE;
	
	/* Warning : We do not check if the CPU supports CPUID
				( Not a problem because all CPUs > 486 support it ) */
	read_cpu_id( 0x00000000, nRegs );
	if( nRegs[0] >= 0x00000001 )
	{
		read_cpu_id( 0x00000001, nRegs2 );
		if( nRegs2[3] & (1 << 23 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_MMX;
		if( nRegs2[3] & (1 << 25 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_SSE;
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_MMX2;
		if( nRegs2[3] & (1 << 26 ) ) {
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_SSE2;
		}
		if( nRegs2[3] & (1 << 9 ) ) {
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_APIC;
		}
		
		#define CPUID_FAMILY	( ( nRegs2[0] >> 8 ) & 0x0F )
		#define CPUID_MODEL		( ( nRegs2[0] >> 4 ) & 0x0F )
		
		/* Find out CPU name */
		strcpy( g_asProcessorDescs[g_nBootCPU].pi_zName, "Unknown" );
		sprintf(zVendor,"%.4s%.4s%.4s",(char*)(nRegs + 1),(char*)(nRegs + 3),(char*)(nRegs + 2));
		for( i = 0; i < MAX_VENDORS; i++ )
		{
			if( !strcmp( cpuvendors[i].string, zVendor ) ) {
				if( cpuname[i][CPUID_FAMILY][CPUID_MODEL] ) {
					sprintf( g_asProcessorDescs[g_nBootCPU].pi_zName, "%s %s",
							cpuvendors[i].name, cpuname[i][CPUID_FAMILY][CPUID_MODEL] );
				}
			}
		}
	}
	read_cpu_id( 0x80000000, nRegs );
	if( nRegs[0] >= 0x80000001 ) {
		read_cpu_id( 0x80000001, nRegs2 );
		if( nRegs2[3] & (1 << 23 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_MMX;
		if( nRegs2[3] & (1 << 22 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_MMX2;
		if( nRegs2[3] & (1 << 31 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_3DNOW;
		if( nRegs2[3] & (1 << 30 ) )
			g_asProcessorDescs[g_nBootCPU].pi_nFeatures |= CPU_FEATURE_3DNOWEX;
	}
	/* Copy this information to all other CPUs */
	for( i = 0; i < MAX_CPU_COUNT; i++ )
	{
		if( i == g_nBootCPU )
			continue;
		strcpy( g_asProcessorDescs[i].pi_zName, g_asProcessorDescs[g_nBootCPU].pi_zName );
		g_asProcessorDescs[i].pi_nFeatures = g_asProcessorDescs[g_nBootCPU].pi_nFeatures;
	}
	printk( "CPU: %s\n", g_asProcessorDescs[g_nBootCPU].pi_zName );
	if( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_MMX )
		printk( "MMX supported\n" );
	if( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_MMX2 )
		printk( "MMX2 supported\n" );
	if( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_3DNOW )
		printk( "3DNOW supported\n" );
	if( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_3DNOWEX )
		printk( "3DNOWEX supported\n" );
	if( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_SSE ) {
		/* Set default SSE state ( from linux kernel ) DOES NOT WORK YET */
		//unsigned long nReg = ( ( unsigned long )( 0x1f80 ) & 0xffbf );
		//asm volatile( "ldmxcsr %0" : : "m" ( nReg ) );
		printk( "SSE supported\n" );
	}
	if( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_SSE2 )
		printk( "SSE2 supported\n" );
	if( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_APIC )
		printk( "APIC present\n" );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void calibrate_apic_timer( int nProcessor )
{
    uint32 nAPICCount;
    uint64 nStartPerf;
    uint64 nEndPerf;
  
    uint32 nReg;

#ifdef SMP_DEBUG
    printk( "Calibrate APIC %d\n", nProcessor );
#endif
  
    nReg  = apic_read( APIC_TDCR ) & ~APIC_TDCR_MASK;
    nReg |= APIC_TDR_DIV_1;
    apic_write( APIC_TDCR, nReg );

    isa_writeb( 0x43, 0x34 );	// loop mode
    isa_writeb( 0x40, 0xff );
    isa_writeb( 0x40, 0xff );
  

    wait_pit_wrap();
    apic_write( APIC_TMICT, ~0 ); // Start APIC timer
    nStartPerf = read_pentium_clock();
    wait_pit_wrap();
    nAPICCount = apic_read( APIC_TMCCT );
    nEndPerf = read_pentium_clock();


    g_asProcessorDescs[nProcessor].pi_nBusSpeed = (uint32)((uint64)PIT_TICKS_PER_SEC * (0xffffffffLL - nAPICCount) / 0xffff);
    g_asProcessorDescs[nProcessor].pi_nCoreSpeed = (uint32)((uint64)PIT_TICKS_PER_SEC * (nEndPerf - nStartPerf) / 0xffff);
    printk( "CPU %d runs at %ld/%ld MHz\n", nProcessor,
	    (g_asProcessorDescs[nProcessor].pi_nBusSpeed + 500000) / 1000000,
	    (g_asProcessorDescs[nProcessor].pi_nCoreSpeed + 500000) / 1000000 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void ap_entry_proc()
{
    struct i3DescrTable	sIDT;
    uint32		nReg;
    int	 		nProcessor;
    uint32		nCR0;
  
    set_page_directory_base_reg( g_psKernelSeg->mc_pPageDir );
    enable_mmu();
  
    nProcessor = get_processor_id();
  
    printk( "CPU %d joines the party.\n", nProcessor );

      // Workaround for lazy bios'es that forget to enable the cache on AP processors.
    __asm__("movl %%cr0,%0":"=r" (nCR0));
    __asm__ __volatile__ ("movl %0,%%cr0" :: "r" (nCR0 & 0x9fffffff) );
  
    sIDT.Base   = (uint32) g_sSysBase.ex_GDT;
    sIDT.Limit  =	0xffff;
    SetGDT( &sIDT );
  
    sIDT.Base  = (uint32) g_sSysBase.ex_IDT;
    sIDT.Limit = 0x7ff;
  
    SetIDT( &sIDT );

    nReg  = apic_read(APIC_SPIV);
    nReg |= (1<<8);		   // Enable spurious interupt vector
    apic_write( APIC_SPIV, nReg );

    calibrate_delay();
    calibrate_apic_timer( nProcessor );
  
    flush_tlb();
  
    g_asProcessorDescs[nProcessor].pi_bIsRunning = true;

    create_idle_thread( "idle" );

    for (;;);
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void install_trampoline( char* pBuffer )
{
    SmpTrampolineCfg_s* psCfg = (SmpTrampolineCfg_s*) pBuffer;
  
    memcpy( pBuffer, g_anTrampoline, sizeof( g_anTrampoline ) );

    psCfg->tc_nKernelEntry = (uint32) ap_entry_proc;
    psCfg->tc_nKernelStack = ((uint32) kmalloc( 4096, MEMF_KERNEL )) + 4092;
    psCfg->tc_nKernelDS	 = 0x18;
    psCfg->tc_nKernelCS	 = 0x08;
    psCfg->tc_nGdt	 = (uint32) g_sSysBase.ex_GDT;	  /* Pointer to global descriptor table */
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Checksum an MP configuration block.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int mpf_checksum( uint8 *mp, int len )
{
    int sum=0;
  
    while(len--) {
	sum+=*mp++;
    }
    return( sum & 0xFF );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Processor encoding in an MP configuration block
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

 
static char* mpc_family( int family, int model )
{
    static char n[32];
    static char *model_defs[]=
    {
	"80486DX","80486DX",
	"80486SX","80486DX/2 or 80487",
	"80486SL","Intel5X2(tm)",
	"Unknown","Unknown",
	"80486DX/4"
    };
    if(family==0x6)
	return("Pentium(tm) Pro");
    if(family==0x5)
	return("Pentium(tm)");
    if(family==0x0F && model==0x0F)
	return("Special controller");
    if(family==0x04 && model<9)
	return model_defs[model];
    sprintf(n,"Unknown CPU [%d:%d]",family, model);
    return n;
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Read the MP configuration tables.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int smp_read_mpc( MpConfigTable_s* mpc )
{
    char str[16];
    int count=sizeof(*mpc);
    int apics=0;
    unsigned char *mpt=((unsigned char *)mpc)+count;
    int	nProcessorCount = 0;
    if( memcmp( mpc->mpc_anSignature, MPC_SIGNATURE, 4 ) )
    {
	printk( "Bad signature [%c%c%c%c].\n",
		mpc->mpc_anSignature[0],
		mpc->mpc_anSignature[1],
		mpc->mpc_anSignature[2],
		mpc->mpc_anSignature[3]);
	return( 1 );
    }
  
    if( mpf_checksum( (unsigned char *)mpc, mpc->mpc_nSize ) != 0 ) {
	printk("Checksum error.\n");
	return( 1 );
    }
  
    if( mpc->mpc_nVersion != 0x01 && mpc->mpc_nVersion != 0x04 ) {
	printk("Bad Config Table version (%d)!!\n", mpc->mpc_nVersion );
	return( 1 );
    }

    printk( "OEM ID: %.8s Product ID: %.12s\n", mpc->mpc_anOEMString, mpc->mpc_anProductID );
#ifdef SMP_DEBUG
    printk( "APIC at: 0x%lX\n", mpc->mpc_nAPICAddress );
#endif
      // set the local APIC address
    g_nPhysAPICAddr = mpc->mpc_nAPICAddress;

      //	Now parce the configuration blocks.
	 
    while( count < mpc->mpc_nSize )
    {
	switch( *mpt )
	{
	    case MP_PROCESSOR:
	    {
		MpConfigProcessor_s* m = (MpConfigProcessor_s*) mpt;
		if( m->mpc_cpuflag & CPU_ENABLED )
		{
		    nProcessorCount++;
		    printk( "Processor %d %s APIC version %d\n",
			    m->mpc_nAPICID, 
			    mpc_family( (m->mpc_cpufeature & CPU_FAMILY_MASK) >> 8, (m->mpc_cpufeature & CPU_MODEL_MASK)>>4),
			    m->mpc_nAPICVer );
	  
		    if( m->mpc_featureflag & (1<<0) ) {
			printk("    Floating point unit present.\n");
		    }
		    if( m->mpc_featureflag & (1<<7) ) {
			printk("    Machine Exception supported.\n");
		    }
		    if( m->mpc_featureflag & (1<<8) ) {
			printk("    64 bit compare & exchange supported.\n");
		    }
		    if( m->mpc_featureflag & (1<<9) ) {
			printk("    Internal APIC present.\n");
		    }
		    if( m->mpc_cpuflag & CPU_BOOTPROCESSOR ) {
			printk( "    Bootup CPU\n" );
			g_nBootCPU   = m->mpc_nAPICID;
			g_nFakeCPUID = SET_APIC_ID( g_nBootCPU );
		    }
						
		    if( m->mpc_nAPICID > MAX_CPU_COUNT ) {
			printk("Processor #%d unused. (Max %d processors).\n",m->mpc_nAPICID, MAX_CPU_COUNT);
		    } else {
			g_asProcessorDescs[m->mpc_nAPICID].pi_bIsPresent   = true;
			g_asProcessorDescs[m->mpc_nAPICID].pi_nAPICVersion = m->mpc_nAPICVer;
		    }
		}
		mpt   += sizeof(*m);
		count += sizeof(*m);
		break;
	    }
	    case MP_BUS:
	    {
		MpConfigBus_s* m = (MpConfigBus_s*) mpt;
	
		memcpy(str,m->mpc_bustype,6);
		str[6]=0;
		printk("Bus %d is %s\n", m->mpc_busid, str );
		mpt   += sizeof(*m);
		count += sizeof(*m);
		break; 
	    }
	    case MP_IOAPIC:
	    {
		MpConfigIOAPIC_s* m = (MpConfigIOAPIC_s*) mpt;
		if( m->mpc_nFlags & MPC_APIC_USABLE ) {
		    apics++;
		    printk( "I/O APIC %d Version %d at 0x%08lX.\n", m->mpc_nAPICID, m->mpc_nAPICVer, m->mpc_nAPICAddress );
		    g_nIoAPICAddr = m->mpc_nAPICAddress;
		}
		mpt   += sizeof(*m);
		count += sizeof(*m); 
		break;
	    }
	    case MP_INTSRC:
	    {
		MpConfigIOAPIC_s* m = (MpConfigIOAPIC_s*)mpt;
				
		mpt   += sizeof(*m);
		count += sizeof(*m);
		break;
	    }
	    case MP_LINTSRC:
	    {
		MpConfigIntLocal_s* m = (MpConfigIntLocal_s*)mpt;
		mpt   += sizeof(*m);
		count += sizeof(*m);
		break;
	    }
	}
    }
    if ( apics > 1 ) {
	printk("Warning: Multiple APIC's not supported.\n");
    }
    return( nProcessorCount );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void init_default_config( int nConfig )
{
      // We need to know what the local APIC id of the boot CPU is!

      // NOTE: The MMU is still disabled.
    g_nBootCPU = GET_APIC_ID( *((vuint32*)(g_nPhysAPICAddr + APIC_ID)) );
    g_nFakeCPUID = SET_APIC_ID( g_nBootCPU );
  
      // 2 CPUs, numbered 0 & 1.
      
    g_asProcessorDescs[0].pi_bIsPresent = true;
    g_asProcessorDescs[1].pi_bIsPresent = true;
  
#ifdef SMP_DEBUG
    printk( "I/O APIC at 0xFEC00000.\n" );
#endif
  
    switch( nConfig )
    {
	case 1:
	    printk( "ISA - external APIC\n" );
	    g_asProcessorDescs[0].pi_nAPICVersion = 0x00;
	    g_asProcessorDescs[1].pi_nAPICVersion = 0x00;
	    break;
	case 2:
	    printk( "EISA with no IRQ8 chaining - external APIC\n" );
	    g_asProcessorDescs[0].pi_nAPICVersion = 0x00;
	    g_asProcessorDescs[1].pi_nAPICVersion = 0x00;
	    break;
	case 3:
	    printk( "EISA - external APIC\n" );
	    g_asProcessorDescs[0].pi_nAPICVersion = 0x00;
	    g_asProcessorDescs[1].pi_nAPICVersion = 0x00;
	    break;
	case 4:
	    printk( "MCA - external APIC\n" );
	    g_asProcessorDescs[0].pi_nAPICVersion = 0x00;
	    g_asProcessorDescs[1].pi_nAPICVersion = 0x00;
	    break;
	case 5:
	    printk( "ISA/PCI - internal APIC\n" );
	    g_asProcessorDescs[0].pi_nAPICVersion = 0x10;
	    g_asProcessorDescs[1].pi_nAPICVersion = 0x10;
	    break;
	case 6:
	    printk( "EISA/PCI - internal APIC\n" );
	    g_asProcessorDescs[0].pi_nAPICVersion = 0x10;
	    g_asProcessorDescs[1].pi_nAPICVersion = 0x10;
	    break;
	case 7:
	    printk( "MCA/PCI - internal APIC\n" );
	    g_asProcessorDescs[0].pi_nAPICVersion = 0x10;
	    g_asProcessorDescs[1].pi_nAPICVersion = 0x10;
	    break;
	default:
	    printk( "Error: unknown default smp configuration %d\n", nConfig );
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int smp_scan_config( void* pBase, uint nSize )
{
    MpFloatingPointer_s* mpf;
    int	 	         nProcessorCount = 0;

      // Late compile-time check...
    if( sizeof(*mpf) != 16 ) {
	printk( "Error: smp_scan_config() Wrong MPF size %d\n", sizeof(*mpf) );
    }
	
    for ( mpf = (MpFloatingPointer_s*) pBase ; nSize > 0 ; mpf++, nSize -= 16 ) {
	if( *((uint32*)mpf->mpf_anSignature) != SMP_MAGIC_IDENT ) {
	    continue;
	}
	if( mpf->mpf_nLength != 1 ) {
	    continue;
	}
	if ( mpf_checksum((uint8*)mpf,16) != 0 ) {
	    continue;
	}

	if ( mpf->mpf_nVersion != 1 && mpf->mpf_nVersion != 4 ) {
	    continue;
	}

	g_bAPICPresent = true;
    
	printk( "Intel SMP v1.%d\n", mpf->mpf_nVersion );
	if( mpf->mpf_nFeature2 & (1<<7) ) {
	    printk( "    IMCR and PIC compatibility mode.\n" );
	} else {
	    printk( "    Virtual Wire compatibility mode.\n" );
	}
	
	  // Check if we got one of the standard configurations
    
	if( mpf->mpf_nFeature1 != 0 ) {
	    init_default_config( mpf->mpf_nFeature1 );
	    nProcessorCount = 2;
	}
	if( mpf->mpf_psConfigTable != NULL ) {
	    nProcessorCount = smp_read_mpc( mpf->mpf_psConfigTable );
	}

	printk("Processors: %d\n", nProcessorCount );
	  // Only use the first configuration found.
	return( 1 );
    }

    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void send_ipi( int nTarget, uint32 nDeliverMode, int nIntNum )
{
    uint32 nReg;
    int	 i;
    int	 nFlg;

    nFlg = cli();
      //	Wait for the APIC to become ready.
	 
    for( i = 0 ; i < 10000 ; ++i ) {
	nReg = apic_read(APIC_ICR);
	if( !(nReg & (1<<12)) ) {
	    break;
	}
	udelay(10);
    }
  
    if( i == 10000) {
	printk("Error: send_ipi() CPU %d: previous IPI still not cleared after 100mS", get_processor_id() );
    }
		
      //	Program the APIC to deliver the IPI
	 
    nReg = apic_read(APIC_ICR2);
    nReg &= 0x00FFFFFF;
    apic_write( APIC_ICR2, nReg | SET_APIC_DEST_FIELD( nTarget ) ); // Target chip

    nReg  = apic_read( APIC_ICR );
    nReg &= ~0xFDFFF;						// Clear bits
    nReg |= APIC_DEST_FIELD | nDeliverMode | nIntNum;	// Send an IRQ

      //	Set the target requirement
	 
    if( nTarget == MSG_ALL_BUT_SELF ) {
	nReg |= APIC_DEST_ALLBUT;
    } else if ( nTarget == MSG_ALL ) {
	nReg |= APIC_DEST_ALLINC;
    }
      //	Send the IPI. The write to APIC_ICR fires this off.
	 
    apic_write(APIC_ICR, nReg );

    put_cpu_flags( nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

#ifdef SMP_DEBUG

static void test_apic()
{
      /*
       *	This is to verify that we're looking at
       *	a real local APIC.  Check these against
       *	your board if the CPUs aren't getting
       *	started for no apparent reason.
       */

    printk("Getting VERSION: %x\n", apic_read(APIC_VERSION) );
    apic_write(APIC_VERSION, 0);
    printk("Getting VERSION: %x\n", apic_read( APIC_VERSION ) );

      /*
       *	The two version reads above should print the same
       *	NON-ZERO!!! numbers.  If the second one is zero,
       *	there is a problem with the APIC write/read
       *	definitions.
       *
       *	The next two are just to see if we have sane values.
       *	They're only really relevant if we're in Virtual Wire
       *	compatibility mode, but most boxes are anymore.
       */

    printk("Getting LVT0: %x\n", apic_read(APIC_LVT0) );
    printk("Getting LVT1: %x\n", apic_read(APIC_LVT1) );
}
#endif // SMP_DEBUG


void smp_boot_cpus(void)
{
    int 	 i;
    int 	 nCPUCount = 0;
    uint32 nReg;
    void*	 pTrampoline;
    uint32 nBogoSum;
    int	 nID;
  
#ifdef SMP_DEBUG
    printk( "Boot CPU %d attempts to wake up AP's\n", get_processor_id() );
#endif
  
    pTrampoline = (void*) (((uint32)alloc_real( 4096 + 4095, MEMF_CLEAR ) + 4095) & ~4095);
    if( pTrampoline == NULL ) {
	panic( "No memory for processor trampoline.\n" );
    }
  
    g_asProcessorDescs[g_nBootCPU].pi_bIsRunning = true;
  
#ifdef SMP_DEBUG
    test_apic();
#endif

      //	Enable the local APIC
 
    nReg  = apic_read(APIC_SPIV);
    nReg |= (1<<8);		/* Enable APIC */
    apic_write( APIC_SPIV, nReg );

    udelay(10);
			
      //	Now scan the cpu present map and fire up the other CPUs.
 
    for( i = 0 ; i < MAX_CPU_COUNT ; ++i )
    {
	unsigned long send_status, accept_status;
	int timeout, num_starts, j;
	  // Hopefully this one is already running..
	if (i == g_nBootCPU ) {
	    continue;
	}
	if ( g_asProcessorDescs[i].pi_bIsPresent == false ) {
	    continue;
	}
			
	  // Install the 16-bit real-mode init function for this CPU
	install_trampoline( pTrampoline );

	printk( "Booting processor %d\n", i );

	CMOS_WRITE(0xa, 0xf);
	*((volatile unsigned short *) 0x469) = ((uint32)pTrampoline) >> 4;
	*((volatile unsigned short *) 0x467) = sizeof( SmpTrampolineCfg_s );

	  // Be paranoid about clearing APIC errors.

	if ( g_asProcessorDescs[i].pi_nAPICVersion & 0xF0 ) {
	    apic_write( APIC_ESR, 0 );
	    accept_status = (apic_read(APIC_ESR) & 0xEF);
	}
	  // Status is now clean
			 
	send_status   = 0;
	accept_status = 0;

	  // Starting actual IPI sequence...

#ifdef SMP_DEBUG
	printk("Asserting INIT.\n");
#endif
	  // Turn INIT on
			 
	send_ipi( i, APIC_DEST_LEVELTRIG, APIC_DEST_ASSERT | APIC_DEST_DM_INIT );
	udelay(200);
#ifdef SMP_DEBUG
	printk("Deasserting INIT.\n");
#endif
	send_ipi( i, APIC_DEST_LEVELTRIG, APIC_DEST_DM_INIT );
    
	  // Only send STARTUP IPIs if we got an integrated APIC.
	if ( g_asProcessorDescs[i].pi_nAPICVersion & 0xF0 ) {
	    num_starts = 2;
	} else {
	    num_starts = 0;
	}

	  // Run STARTUP IPI loop.

	for ( j = 1 ; !(send_status || accept_status) && (j <= num_starts) ; ++j )
	{
#ifdef SMP_DEBUG
	    printk( "Sending STARTUP #%d.\n", j );
#endif
	    apic_write(APIC_ESR, 0);
			
	    send_ipi( i, 0, APIC_DEST_DM_STARTUP | (((int) pTrampoline) >> 12) );
      
	    timeout = 0;
	    do {
		udelay(10);
	    } while ( (send_status = (apic_read( APIC_ICR ) & 0x1000)) && (timeout++ < 1000));
	    udelay(200);

	    accept_status = (apic_read(APIC_ESR) & 0xEF);
	}
      
	if ( send_status ) {	// APIC never delivered??
	    printk("APIC never delivered???\n");
	}
	if ( accept_status ) {	// Send accept error
	    printk("APIC delivery error (%lx).\n", accept_status);
	}
    
	if( !(send_status || accept_status) )
	{
#ifdef SMP_DEBUG
	    printk( "INIT sendt, wait for the AP to wake up\n" );
#endif
	    for( timeout = 0 ; timeout < 50000 ; timeout++ )
	    {
		if( g_asProcessorDescs[i].pi_bIsRunning ) {
		    break; // It's alive
		}
		udelay( 100 );				// Wait 5s total for a response
	    }
	    if( g_asProcessorDescs[i].pi_bIsRunning ) {
		nCPUCount++;
	    } else {
		printk( "CPU %d is not responding.\n", i );
	    }
	}
    }
      // Paranoid:  Set warm reset code and vector here back
      // to default values.

    CMOS_WRITE(0, 0xf);
    *((volatile long *) 0x467) = 0;

  
      //	Allow the user to impress friends.
	
    nBogoSum = 0;
    nID = 0;
      
    for( i = 0 ; i < MAX_CPU_COUNT ; ++i )
    {
	if ( g_asProcessorDescs[i].pi_bIsRunning ) {
	    nBogoSum += g_asProcessorDescs[i].pi_nDelayCount;
	    g_anLogicToRealID[nID++] = i;
	}
    }
    printk( "Total of %d processors activated (%lu.%02lu BogoMIPS).\n",
	    nCPUCount + 1, (nBogoSum+2500) / 500000, ((nBogoSum+2500)/5000) % 100 );
    g_bSmpActivated = true;
    g_nActiveCPUCount = nCPUCount + 1;
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void  flush_tlb_global( void )
{
    flush_tlb();
    if ( g_nActiveCPUCount > 1 ) {
	send_ipi( MSG_ALL_BUT_SELF, APIC_DEST_DM_FIXED, INT_INVAL_PGT );
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void apic_eoi( void )
{
    apic_read(APIC_SPIV);	   // Dummy read. (see comment in smp.h)
    apic_write(APIC_EOI, 0); // Inform the apic that the interupt routine is done
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void do_smp_invalidate_pgt( SysCallRegs_s* psRegs, int nIrqNum )
{
    uint32 nFlg = cli();
 
    flush_tlb();
    apic_eoi();
    put_cpu_flags( nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void do_smp_spurious_irq( SysCallRegs_s* psRegs, int nIrqNum )
{
    printk( "Got APIC spurious interrupt\n" );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void do_smp_preempt( SysCallRegs_s* psRegs, int nIrqNum )
{
    uint32 nFlg = cli();
    apic_eoi();
    put_cpu_flags( nFlg );
  
//  wake_up_sleepers();
    Schedule();
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool init_smp( bool bInitSMP )
{
    bool	 bFound = false;
    int	 i;

    if ( bInitSMP ) {
		uint32 anConfigAreas[] = {
		    0x0,       0x400,	// Bottom 1k of base memory
		    639*0x400, 0x400,	// Top 1k of base memory (FIXME: Should first verify that we have 640kb!!!)
	 	   0xF0000,   0x10000,	// 64k of bios
	 	   0,0
		};
  
		printk( "Scan memory for SMP config tables...\n" );

		for ( i = 0 ; bFound == false && anConfigAreas[i+1] > 0 ; i+=2 ) {
		    bFound = smp_scan_config( (void*) anConfigAreas[i], anConfigAreas[i+1] );
		}
    }

	set_cpu_features();
    calibrate_delay();
    if ( ( bFound || ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_APIC ) ) && bInitSMP ) {
    	/* Map APIC registers */
		g_nVirtualAPICAddr = 0;
		g_hAPICArea = create_area( "apic_registers", (void**)&g_nVirtualAPICAddr,
				   PAGE_SIZE, PAGE_SIZE, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );

		if ( g_hAPICArea < 0 ) {
		    printk( "Error: init_smp() failed to create APIC register area\n" );
		    return( false );
		}

		#ifdef SMP_DEBUG
		printk( "Map in local APIC (%08x) to %08x\n", g_nPhysAPICAddr, g_nVirtualAPICAddr );
		#endif
		remap_area( g_hAPICArea, (void*) g_nPhysAPICAddr );
		
    } 
    if( !bFound ) {
    	/* Set default values for non-SMP machines */
    	if( ( ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_APIC ) ) && bInitSMP ) {
    		/* Identify as a smp system */
    		g_nBootCPU = GET_APIC_ID( *((vuint32*)(g_nVirtualAPICAddr + APIC_ID)) );
			g_nFakeCPUID = SET_APIC_ID( g_nBootCPU );
			g_bAPICPresent = true;
			bFound = true;
    	} else {
    		/* No speed detection for non-SMP machine with no APIC yet */
    		g_nVirtualAPICAddr  = ((uint32)&g_nFakeCPUID)-APIC_ID;
    		g_asProcessorDescs[g_nBootCPU].pi_nBusSpeed = 0;
    		g_asProcessorDescs[g_nBootCPU].pi_nCoreSpeed = 0;
    	}
		g_asProcessorDescs[g_nBootCPU].pi_bIsPresent = true;
		g_asProcessorDescs[g_nBootCPU].pi_bIsRunning = true;
		g_anLogicToRealID[0] = g_nBootCPU;	
    }
    return( bFound );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void boot_ap_processors( void )
{
    int nFlg;
  
    printk( "Untie AP processors...\n" );

    nFlg = cli();
    calibrate_apic_timer( get_processor_id() );
    smp_boot_cpus();
    put_cpu_flags( nFlg );
}

int logig_to_physical_cpu_id( int nLogicID )
{
    return( g_anLogicToRealID[nLogicID] );
}

Thread_s* get_current_thread( void )
{
    Thread_s* psThread;
    int 	    nFlg;

    nFlg = cli();
    psThread = g_asProcessorDescs[get_processor_id()].pi_psCurrentThread;
    put_cpu_flags( nFlg );
    return( psThread );
}

Thread_s* get_idle_thread( void )
{
    return( g_asProcessorDescs[get_processor_id()].pi_psIdleThread );
}

void set_idle_thread( Thread_s* psThread )
{
    g_asProcessorDescs[get_processor_id()].pi_psIdleThread = psThread;
}
