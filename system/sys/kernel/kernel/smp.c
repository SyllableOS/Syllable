
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
#include "inc/io_ports.h"

//#define SMP_DEBUG

static uint32 g_nIoAPICAddr = 0xFEC00000;	// Address of the I/O apic (not yet used)
bool g_bAPICPresent = false;
static bool g_bFoundSmpConfig = false;
int g_nActiveCPUCount = 1;
int g_nBootCPU = 0;
ProcessorInfo_s g_asProcessorDescs[MAX_CPU_COUNT];

static uint32 g_nPhysAPICAddr = 0xFEE00000;	// Address of APIC (defaults to 0xFEE00000)

 // Alloc get_processor_id() to work on non-smp machines
 // Will be set to the virtual address of local APIC later if this is a SMP machine.
static uint32 g_nFakeCPUID = 0;
uint32 g_nVirtualAPICAddr = ( ( uint32 )&g_nFakeCPUID ) - APIC_ID;
static area_id g_hAPICArea;
static int g_anLogicToRealID[MAX_CPU_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };


extern bool g_bHasFXSR;
extern bool g_bHasXMM;

vuint32 g_nTLBInvalidateMask = 0; // Mask of the CPUs that need to invalidate their tlb
vuint32 g_nMTRRInvalidateMask = 0; // Mask of the CPUs that need to invalidate their mtrr descriptors
uint32 g_nCpuMask = 0; // CPU mask (copied to g_nTLBInvalidateMask or g_nMTRRInvalidateMask during tlb flush)

/*
 * This is the number of bits of precision for pi_nDelayCount.  Each
 * bit takes on average 1.5 / INT_FREQ seconds.  This is a little better
 * than 1%.
 */
static const count_t LPS_PREC = 8;

//void smp_ap_entry();

uint8 g_anTrampoline[] = {
#include "objs/smp_entry.hex"
};

static __inline__ void set_bit( volatile void * pAddr, int nNr )
{
	__asm__ __volatile__( "lock\n\tbtsl %1,%0" :"=m" ( *(volatile long *)pAddr ) :"Ir" ( nNr ) );
}

static inline int test_and_clear_bit( volatile void* pAddr, int nNr )
{
	int nBit;
	__asm__ __volatile__( "lock\n\tbtrl %2,%1\n\tsbbl %0,%0" :"=r" ( nBit ),
			"=m" ( *(volatile long *)pAddr ) :"Ir" ( nNr ) : "memory" );
	return( nBit );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void idle_loop( void )
{
	char zThreadName[32];
	int nProcessor = 0;

	cli();

	nProcessor = get_processor_id();

	sprintf( zThreadName, "idle_%02d", nProcessor );
	rename_thread( -1, zThreadName );

	if ( g_bAPICPresent )
	{
		uint32 nAPICDiv;

		nAPICDiv = g_asProcessorDescs[nProcessor].pi_nBusSpeed / INT_FREQ;

		nAPICDiv += nProcessor * 10000;	// Avoid re-scheduling all processors at the same time
#ifdef SMP_DEBUG
		printk( "APIC divisor = %d\n", nAPICDiv );
#endif
		apic_write( APIC_LVTT, APIC_LVT_TIMER_PERIODIC | INT_SCHEDULE );	// Make APIC local timer trig int INT_SCHEDULE
		apic_write( APIC_TMICT, nAPICDiv );	// Start APIC timer
	}
	
	sti();
//  for(;;) Schedule();
//  for(;;);
	for ( ;; )
		__asm__( "hlt" );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int read_pit_timer( void )
{
	int nCount;

	isa_writeb( PIT_MODE, 0 );
	nCount = isa_readb( PIT_CH0 );
	nCount += isa_readb( PIT_CH0 ) << 8;
	return ( nCount );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void wait_pit_wrap( void )
{
	uint nCurrent = read_pit_timer();
	uint nPrev = 0;

	for ( ;; )
	{
		int nDelta;

		nPrev = nCurrent;
		nCurrent = read_pit_timer();
		nDelta = nCurrent - nPrev;

		if ( nDelta > 300 )
		{
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

void calibrate_delay( int nProcessor )
{
	unsigned long loops_per_jiffy = ( 1 << 12 );	// start at 2 BogoMips
	unsigned long loopbit;
	unsigned long jiffy_countdown = 0xffff - ( PIT_TICKS_PER_SEC / INT_FREQ );
	count_t lps_precision = LPS_PREC;
	uint64 nStartPerf;
	uint64 nEndPerf;

	printk( "Calibrating delay loop for CPU %d\n", get_processor_id() );

	while ( ( loops_per_jiffy <<= 1 ) != 0 )
	{
		isa_writeb( PIT_MODE, 0x30 );	// one-shot mode
		isa_writeb( PIT_CH0, 0xff );
		isa_writeb( PIT_CH0, 0xff );

		__delay( loops_per_jiffy );

		if ( read_pit_timer() < jiffy_countdown )
			break;
	}

	/*
	 * Do a binary approximation to get loops_per_jiffy set to
	 * equal one clock (up to lps_precision bits)
	 */
	loops_per_jiffy >>= 1;
	loopbit = loops_per_jiffy;
	while ( lps_precision-- && ( loopbit >>= 1 ) )
	{
		loops_per_jiffy |= loopbit;

		isa_writeb( PIT_MODE, 0x30 );	// one-shot mode
		isa_writeb( PIT_CH0, 0xff );
		isa_writeb( PIT_CH0, 0xff );
		__delay( loops_per_jiffy );

		if ( read_pit_timer() < jiffy_countdown )
			loops_per_jiffy &= ~loopbit;
	}

	/* Calculate CPU core speed */
	isa_writeb( PIT_MODE, 0x34 );	// loop mode
	isa_writeb( PIT_CH0, 0xff );
	isa_writeb( PIT_CH0, 0xff );

	wait_pit_wrap();
	nStartPerf = read_pentium_clock();
	wait_pit_wrap();
	nEndPerf = read_pentium_clock();

	g_asProcessorDescs[nProcessor].pi_nCoreSpeed = ( uint32 )( ( uint64 )PIT_TICKS_PER_SEC * ( nEndPerf - nStartPerf ) / 0xffff );

	isa_writeb( PIT_MODE, 0x34 );	// loop mode
	isa_writeb( PIT_CH0, 0x0 );
	isa_writeb( PIT_CH0, 0x0 );

	g_asProcessorDescs[get_processor_id()].pi_nDelayCount = loops_per_jiffy;

	// Round the value and print it.
	printk( "ok - %lu.%02lu BogoMIPS (lpj=%lu)\n", loops_per_jiffy / ( 500000 / INT_FREQ ), ( loops_per_jiffy / ( 5000 / INT_FREQ ) ) % 100, loops_per_jiffy );
	printk( "CPU %d runs at %d.%04d MHz\n", nProcessor,  g_asProcessorDescs[nProcessor].pi_nCoreSpeed / 1000000, ( g_asProcessorDescs[nProcessor].pi_nCoreSpeed / 100 ) % 10000 );
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
	uint32 nReg;

#ifdef SMP_DEBUG
	printk( "Calibrate APIC %d\n", nProcessor );
#endif

	nReg = apic_read( APIC_TDCR ) & ~APIC_TDCR_MASK;
	nReg |= APIC_TDR_DIV_1;
	apic_write( APIC_TDCR, nReg );

	isa_writeb( PIT_MODE, 0x34 );	// loop mode
	isa_writeb( PIT_CH0, 0xff );
	isa_writeb( PIT_CH0, 0xff );

	wait_pit_wrap();
	apic_write( APIC_TMICT, ~0 );	// Start APIC timer
	wait_pit_wrap();
	nAPICCount = apic_read( APIC_TMCCT );


	g_asProcessorDescs[nProcessor].pi_nBusSpeed = ( uint32 )( ( uint64 )PIT_TICKS_PER_SEC * ( 0xffffffffLL - nAPICCount ) / 0xffff );
	printk( "CPU %d busspeed at %d.%04d MHz\n", nProcessor, g_asProcessorDescs[nProcessor].pi_nBusSpeed / 1000000, ( g_asProcessorDescs[nProcessor].pi_nBusSpeed / 100 ) % 10000 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void ap_entry_proc( void )
{
	struct i3DescrTable sIDT;
	uint32 nReg;
	int nProcessor;
	uint32 nCR0;
	unsigned int nDummy;

	set_page_directory_base_reg( g_psKernelSeg->mc_pPageDir );
	__asm__ __volatile__( "movl %%cr0,%0; orl $0x80010000,%0; movl %0,%%cr0" : "=r" (nDummy) );	// set PG & WP bit in cr0

	nProcessor = get_processor_id();

	printk( "CPU %d joins the party.\n", nProcessor );

	// Workaround for lazy bios'es that forget to enable the cache on AP processors.
	// Also set NE and MP flags and clear EM flag for x87 native mode
	__asm__( "movl %%cr0,%0":"=r"( nCR0 ) );
	__asm__ __volatile__( "movl %0,%%cr0"::"r"( ( nCR0 & 0x9ffffffb ) | 0x22 ) );

	// Enable SSE support
	if ( g_asProcessorDescs[nProcessor].pi_bHaveFXSR )
	{
		set_in_cr4( X86_CR4_OSFXSR );
	}
	if ( g_asProcessorDescs[nProcessor].pi_bHaveXMM )
	{
		set_in_cr4( X86_CR4_OSXMMEXCPT );
	}

	sIDT.Base = ( uint32 )g_sSysBase.ex_GDT;
	sIDT.Limit = 0xffff;
	SetGDT( &sIDT );

	sIDT.Base = ( uint32 )g_sSysBase.ex_IDT;
	sIDT.Limit = 0x7ff;

	SetIDT( &sIDT );
	SetTR( ( 8 + nProcessor ) << 3 );
	
	calibrate_delay( nProcessor );

	// Clear the local apic

	apic_write( APIC_ESR, 0 );
	apic_write( APIC_ESR, 0 );
	apic_write( APIC_ESR, 0 );
	apic_write( APIC_ESR, 0 );

	apic_write( APIC_DFR, APIC_DFR_FLAT );

	nReg = apic_read( APIC_LDR ) & ~APIC_LDR_MASK;
	nReg |= SET_APIC_LOGICAL_ID( 1UL << nProcessor );
	apic_write( APIC_LDR, nReg );

	// Enable the local APIC
	nReg = apic_read( APIC_SPIV );
	nReg &=~ APIC_DEST_VECTOR_MASK;
	nReg |= APIC_SPIV_APIC_ENABLED;	/* Enable APIC */
	nReg &=~ APIC_SPIV_FOCUS_DISABLED; /* Disable focus */
	nReg |= INT_SPURIOUS;
	apic_write( APIC_SPIV, nReg );
	
	udelay( 10 );

	apic_write( APIC_LVT0, APIC_DEST_DM_EXTINT | APIC_LVT_MASKED );
	apic_write( APIC_LVT1, APIC_DEST_DM_NMI | APIC_LVT_MASKED );

	calibrate_apic_timer( nProcessor );

	flush_tlb();
	set_bit( &g_nCpuMask, nProcessor );
	write_mtrr_descs();

	g_asProcessorDescs[nProcessor].pi_bIsRunning = true;

	create_idle_thread( "idle" );

	for ( ;; );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void install_trampoline( char *pBuffer )
{
	SmpTrampolineCfg_s *psCfg = ( SmpTrampolineCfg_s * ) pBuffer;

	memcpy( pBuffer, g_anTrampoline, sizeof( g_anTrampoline ) );

	psCfg->tc_nKernelEntry = ( uint32 )ap_entry_proc;
	psCfg->tc_nKernelStack = ( ( uint32 )kmalloc( 4096, MEMF_KERNEL ) ) + 4092;
	psCfg->tc_nKernelDS = DS_KERNEL;
	psCfg->tc_nKernelCS = CS_KERNEL;
	psCfg->tc_nGdt = ( uint32 )g_sSysBase.ex_GDT;	/* Pointer to global descriptor table */
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
	int sum = 0;

	while ( len-- )
	{
		sum += *mp++;
	}
	return ( sum & 0xFF );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Processor encoding in an MP configuration block
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/


static char *mpc_family( int family, int model )
{
	static char n[32];
	static char *model_defs[] = {
		"80486DX", "80486DX",
		"80486SX", "80486DX/2 or 80487",
		"80486SL", "Intel5X2(tm)",
		"Unknown", "Unknown",
		"80486DX/4"
	};
	if ( family == 0x6 )
		return ( "Pentium(tm) Pro" );
	if ( family == 0x5 )
		return ( "Pentium(tm)" );
	if ( family == 0x0F && model == 0x0F )
		return ( "Special controller" );
	if ( family == 0x0F && model == 0x00 )
		return ( "Pentium 4(tm)" );
	if ( family == 0x04 && model < 9 )
		return model_defs[model];
	sprintf( n, "Unknown CPU [%d:%d]", family, model );
	return n;
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Read the MP configuration tables.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int smp_read_mpc( MpConfigTable_s * mpc )
{
	char str[16];
	int count = sizeof( *mpc );
	int apics = 0;
	unsigned char *mpt = ( ( unsigned char * )mpc ) + count;
	int nProcessorCount = 0;

	if ( memcmp( mpc->mpc_anSignature, MPC_SIGNATURE, 4 ) )
	{
		printk( "Bad signature [%c%c%c%c].\n", mpc->mpc_anSignature[0], mpc->mpc_anSignature[1], mpc->mpc_anSignature[2], mpc->mpc_anSignature[3] );
		return ( 1 );
	}

	if ( mpf_checksum( ( unsigned char * )mpc, mpc->mpc_nSize ) != 0 )
	{
		printk( "Checksum error.\n" );
		return ( 1 );
	}

	if ( mpc->mpc_nVersion != 0x01 && mpc->mpc_nVersion != 0x04 )
	{
		printk( "Bad Config Table version (%d)!!\n", mpc->mpc_nVersion );
		return ( 1 );
	}

	printk( "OEM ID: %.8s Product ID: %.12s\n", mpc->mpc_anOEMString, mpc->mpc_anProductID );
#ifdef SMP_DEBUG
	printk( "APIC at: 0x%lX\n", mpc->mpc_nAPICAddress );
#endif
	// set the local APIC address
	g_nPhysAPICAddr = mpc->mpc_nAPICAddress;

	//        Now parce the configuration blocks.

	while ( count < mpc->mpc_nSize )
	{
		switch ( *mpt )
		{
		case MP_PROCESSOR:
			{
				MpConfigProcessor_s *m = ( MpConfigProcessor_s * ) mpt;

				if ( m->mpc_cpuflag & CPU_ENABLED )
				{
					nProcessorCount++;
					printk( "Processor %d %s APIC version %d\n", m->mpc_nAPICID, mpc_family( ( m->mpc_cpufeature & CPU_FAMILY_MASK ) >> 8, ( m->mpc_cpufeature & CPU_MODEL_MASK ) >> 4 ), m->mpc_nAPICVer );

					if ( m->mpc_featureflag & ( 1 << 0 ) )
					{
						printk( "    Floating point unit present.\n" );
					}
					if ( m->mpc_featureflag & ( 1 << 7 ) )
					{
						printk( "    Machine Exception supported.\n" );
					}
					if ( m->mpc_featureflag & ( 1 << 8 ) )
					{
						printk( "    64 bit compare & exchange supported.\n" );
					}
					if ( m->mpc_featureflag & ( 1 << 9 ) )
					{
						printk( "    Internal APIC present.\n" );
					}
					if ( m->mpc_cpuflag & CPU_BOOTPROCESSOR )
					{
						printk( "    Bootup CPU\n" );
						g_nBootCPU = m->mpc_nAPICID;
						g_nFakeCPUID = SET_APIC_ID( g_nBootCPU );
					}

					if ( m->mpc_nAPICID > MAX_CPU_COUNT )
					{
						printk( "Processor #%d unused. (Max %d processors).\n", m->mpc_nAPICID, MAX_CPU_COUNT );
					}
					else
					{
						g_asProcessorDescs[m->mpc_nAPICID].pi_bIsPresent = true;
						g_asProcessorDescs[m->mpc_nAPICID].pi_nAPICVersion = m->mpc_nAPICVer;
					}
				}
				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}
		case MP_BUS:
			{
				MpConfigBus_s *m = ( MpConfigBus_s * ) mpt;

				memcpy( str, m->mpc_bustype, 6 );
				str[6] = 0;
				printk( "Bus %d is %s\n", m->mpc_busid, str );
				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}
		case MP_IOAPIC:
			{
				MpConfigIOAPIC_s *m = ( MpConfigIOAPIC_s * ) mpt;

				if ( m->mpc_nFlags & MPC_APIC_USABLE )
				{
					apics++;
					printk( "I/O APIC %d Version %d at 0x%08X.\n", m->mpc_nAPICID, m->mpc_nAPICVer, m->mpc_nAPICAddress );
					g_nIoAPICAddr = m->mpc_nAPICAddress;
				}
				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}
		case MP_INTSRC:
			{
				MpConfigIOAPIC_s *m = ( MpConfigIOAPIC_s * ) mpt;

				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}
		case MP_LINTSRC:
			{
				MpConfigIntLocal_s *m = ( MpConfigIntLocal_s * ) mpt;

				mpt += sizeof( *m );
				count += sizeof( *m );
				break;
			}
		}
	}
	if ( apics > 1 )
	{
		printk( "Warning: Multiple APIC's not supported.\n" );
	}
	return ( nProcessorCount );
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
	g_nBootCPU = GET_APIC_ID( *( ( vuint32 * )( g_nPhysAPICAddr + APIC_ID ) ) );
	g_nFakeCPUID = SET_APIC_ID( g_nBootCPU );

	// 2 CPUs, numbered 0 & 1.

	g_asProcessorDescs[0].pi_bIsPresent = true;
	g_asProcessorDescs[1].pi_bIsPresent = true;

#ifdef SMP_DEBUG
	printk( "I/O APIC at 0xFEC00000.\n" );
#endif

	switch ( nConfig )
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

static int smp_scan_mpc_config( void *pBase, uint nSize )
{
	MpFloatingPointer_s *mpf;
	int nProcessorCount = 0;

	// Late compile-time check...
	if ( sizeof( *mpf ) != 16 )
	{
		printk( "Error: smp_scan_config() Wrong MPF size %d\n", sizeof( *mpf ) );
	}

	for ( mpf = ( MpFloatingPointer_s * ) pBase; nSize > 0; mpf++, nSize -= 16 )
	{
		if ( *( ( uint32 * )mpf->mpf_anSignature ) != SMP_MAGIC_IDENT )
		{
			continue;
		}
		if ( mpf->mpf_nLength != 1 )
		{
			continue;
		}
		if ( mpf_checksum( ( uint8 * )mpf, 16 ) != 0 )
		{
			continue;
		}

		if ( mpf->mpf_nVersion != 1 && mpf->mpf_nVersion != 4 )
		{
			continue;
		}

		g_bAPICPresent = true;

		printk( "Intel SMP v1.%d\n", mpf->mpf_nVersion );
		if ( mpf->mpf_nFeature2 & ( 1 << 7 ) )
		{
			printk( "    IMCR and PIC compatibility mode.\n" );
		}
		else
		{
			printk( "    Virtual Wire compatibility mode.\n" );
		}

		// Check if we got one of the standard configurations

		if ( mpf->mpf_nFeature1 != 0 )
		{
			init_default_config( mpf->mpf_nFeature1 );
			nProcessorCount = 2;
		}
		if ( mpf->mpf_psConfigTable != NULL )
		{
			nProcessorCount = smp_read_mpc( mpf->mpf_psConfigTable );
		}

		printk( "Processors: %d\n", nProcessorCount );
		// Only use the first configuration found.
		return ( 1 );
	}

	return ( 0 );
}



//****************************************************************************/
/** Parses the ACPI RSDT table to find the ACPI MADT table. This table contains
 * information about the installed processors and is similiar to the MPC table
 * on SMP systems.
 * \param nRsdt - Physical address of the rsdt.
 * \internal
 * \ingroup CPU
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

static int smp_read_acpi_rsdt( uint32 nRsdt )
{
	uint32 nTableEnt;
	int nApics = 0;
	int nProcessorCount = 0;
	uint32 i;
	area_id hRsdtArea;
	AcpiRsdt_s* psRsdt = NULL;
	
	/* Late compile-time check... */
	if ( sizeof( *psRsdt ) > PAGE_SIZE )
	{
		printk( "Error: smp_read_acpi_rsdt() wrong RSDT size %d\n", sizeof( *psRsdt ) );
	}
	
	/* Map the rsdt because it is outside the accessible memory */
	hRsdtArea = create_area( "acpi_rsdt", ( void ** )&psRsdt, PAGE_SIZE, PAGE_SIZE, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
	if( hRsdtArea < 0 )
	{
		printk( "Error: smp_read_acpi_rsdt() failed to map the acpi rsdt table\n" );
		return( 1 );
	}
	remap_area( hRsdtArea, (void*)( nRsdt & PAGE_MASK ) );
	psRsdt = (AcpiRsdt_s*)( (uint32)psRsdt + ( nRsdt - ( nRsdt & PAGE_MASK ) ) );
	
	/* Check signature and checksum */
	if ( strncmp( psRsdt->ar_sHeader.ath_anSignature, ACPI_RSDT_SIGNATURE, 4 ) )
	{
		printk( "Bad signature [%c%c%c%c].\n", psRsdt->ar_sHeader.ath_anSignature[0], psRsdt->ar_sHeader.ath_anSignature[1], psRsdt->ar_sHeader.ath_anSignature[2], psRsdt->ar_sHeader.ath_anSignature[3] );
		delete_area( hRsdtArea );
		return ( 0 );
	}
	
	if ( mpf_checksum( ( unsigned char * )psRsdt, psRsdt->ar_sHeader.ath_nLength ) != 0 )
	{
		printk( "Checksum error.\n" );
		delete_area( hRsdtArea );
		return ( 0 );
	}

	printk( "OEM ID: %.6s Product ID: %.8s\n", psRsdt->ar_sHeader.ath_anOemId, psRsdt->ar_sHeader.ath_anOemTableId );

	/* Calculate number of table entries */
	nTableEnt = ( psRsdt->ar_sHeader.ath_nLength - sizeof( AcpiTableHeader_s ) ) >> 2;
	if( nTableEnt > 8 )
		nTableEnt = 8;
	
	for( i = 0; i < nTableEnt; i++ )
	{
		/* Map entries */
		area_id hEntry;
		AcpiTableHeader_s* psHeader = NULL;
		
		
		/* Map the rsdt entries. We map only one page (+1 for alignment) because we only read the table header
		   or the madt table and both are smaller than one page */
		hEntry = create_area( "acpi_rsdt_entry", ( void ** )&psHeader, 2 * PAGE_SIZE, 2 * PAGE_SIZE, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
		if( hEntry < 0 )
		{
			printk( "Error: smp_read_acpi_rsdt() failed to map a rsdt entry\n" );
			return( 0 );
		}
		
		remap_area( hEntry, (void*)( psRsdt->ar_nEntry[i] & PAGE_MASK ) );
		psHeader = (AcpiTableHeader_s*)( (uint32)psHeader + ( psRsdt->ar_nEntry[i] - ( psRsdt->ar_nEntry[i] & PAGE_MASK ) ) );
#ifdef SMP_DEBUG
		printk( "%.4s %.6s %.8s\n", psHeader->ath_anSignature, psHeader->ath_anOemId, psHeader->ath_anOemTableId );
#endif
		if( !strncmp( psHeader->ath_anSignature, ACPI_MADT_SIGNATURE, 4 ) )
		{
			/* Found the madt */
			area_id hMadt;
			AcpiMadt_s* psMadt = NULL;
			AcpiMadtEntry_s* psEntry = NULL;
			uint32 nMadtSize = PAGE_ALIGN( psHeader->ath_nLength ) + PAGE_SIZE;
			uint32 nMadtEnd;
			
			/* Map the madt and its entries */
			hMadt = create_area( "acpi_madt", ( void ** )&psMadt, nMadtSize, nMadtSize, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );
			if( hMadt < 0 )
			{
				printk( "Error: smp_read_acpi_rsdt() failed to map the madt table\n" );
				return( 0 );
			}
			remap_area( hMadt, (void*)( psRsdt->ar_nEntry[i] & PAGE_MASK ) );
			psMadt = (AcpiMadt_s*)( (uint32)psMadt + ( psRsdt->ar_nEntry[i] - ( psRsdt->ar_nEntry[i] & PAGE_MASK ) ) );
			
			/* Set the local APIC address */
			g_bAPICPresent = true;
			g_nPhysAPICAddr = psMadt->am_nApicAddr;
#ifdef SMP_DEBUG
			printk( "APIC at: 0x%lX\n", psMadt->am_nApicAddr );
#endif
			nMadtEnd = (uint32)psMadt + psHeader->ath_nLength;
			psEntry = ( AcpiMadtEntry_s* )( (uint32)psMadt + sizeof( AcpiMadt_s ) );
			
			/* Parse the madt entries */
			while( ( (uint32)psEntry ) < nMadtEnd )
			{
				switch( psEntry->ame_nType )
				{
					case ACPI_MADT_PROCESSOR:
					{
						AcpiMadtProcessor_s *psP = ( AcpiMadtProcessor_s * )psEntry;

						if ( psP->amp_nFlags & ACPI_MADT_CPU_ENABLED )
						{
							nProcessorCount++;
							printk( "Processor %d\n", psP->amp_nApicId );
							
							/* There does not seem to be any information in the madt table
							   about the bootup cpu, so we just use the cpu with the apic
							   id 0 */
							if( psP->amp_nApicId == 0 )
							{
								printk( "    Bootup CPU\n" );
								g_nBootCPU = psP->amp_nApicId;
								g_nFakeCPUID = SET_APIC_ID( g_nBootCPU );
							}
					
							g_asProcessorDescs[psP->amp_nApicId].pi_bIsPresent = true;
							g_asProcessorDescs[psP->amp_nApicId].pi_nAPICVersion = 0x10;
						}
					}	
					break;
			
		
					case ACPI_MADT_IOAPIC:
					{
						AcpiMadtIoApic_s *psP = ( AcpiMadtIoApic_s * )psEntry;
						nApics++;
						printk( "I/O APIC %d at 0x%08X.\n", psP->ami_nId, psP->ami_nAddr );
						g_nIoAPICAddr = psP->ami_nAddr;
					}
					break;
				}
				psEntry = ( AcpiMadtEntry_s* )( (uint32)psEntry + psEntry->ame_nLength );
			}
			delete_area( hMadt );
		}
		delete_area( hEntry );	
	}
	
	delete_area( hRsdtArea );
	
	if ( nApics > 1 )
	{
		printk( "Warning: Multiple APIC's not supported.\n" );
	}

	return ( nProcessorCount );
}


//****************************************************************************/
/** Tries to find the ACPI RSDP table in the system memory.
 * \param pBase - First base address pointer.
 * \param nSize - Size of the memory region.
 * \internal
 * \ingroup CPU
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

static int smp_scan_acpi_config( void *pBase, uint nSize )
{
	AcpiRsdp_s *psRsdp = NULL;
	int nProcessorCount = 0;

	for ( psRsdp = ( AcpiRsdp_s * ) pBase; nSize > 0;
	      psRsdp = ( AcpiRsdp_s * )( ((uint8_t*)psRsdp) + 16 ), nSize -= 16 )
	{
		if ( strncmp( ( char* )psRsdp->ar_anSignature, ACPI_RSDP_SIGNATURE, 8 ) )
		{
			continue;
		}
		
		if ( mpf_checksum( ( uint8 * )psRsdp, sizeof( AcpiRsdp_s ) ) != 0 )
		{
			continue;
		}
		
		if ( psRsdp->ar_nRevision >= 2 )
		{
			continue;
		}

		nProcessorCount = smp_read_acpi_rsdt( psRsdp->ar_nRsdt );
		
		if( nProcessorCount == 0 )
		{
			return( 0 );
		}

		printk( "Processors: %d\n", nProcessorCount );

		return( 1 );
	}

	return ( 0 );
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
	int i;
	int nFlg;

	nFlg = cli();
	//        Wait for the APIC to become ready.

	for ( i = 0; i < 10000; ++i )
	{
		nReg = apic_read( APIC_ICR );
		if ( !( nReg & ( 1 << 12 ) ) )
		{
			break;
		}
		udelay( 10 );
	}

	if ( i == 10000 )
	{
		printk( "Error: send_ipi() CPU %d: previous IPI still not cleared after 100mS", get_processor_id() );
	}

	//        Program the APIC to deliver the IPI

	nReg = apic_read( APIC_ICR2 );
	nReg &= 0x00FFFFFF;
	apic_write( APIC_ICR2, nReg | SET_APIC_DEST_FIELD( nTarget ) );	// Target chip

	nReg = apic_read( APIC_ICR );
	nReg &= ~0xFDFFF;	// Clear bits
	nReg |= APIC_DEST_FIELD | nDeliverMode | nIntNum;	// Send an IRQ

	//        Set the target requirement

	if ( nTarget == MSG_ALL_BUT_SELF )
	{
		nReg |= APIC_DEST_ALLBUT;
	}
	else if ( nTarget == MSG_ALL )
	{
		nReg |= APIC_DEST_ALLINC;
	}
	//        Send the IPI. The write to APIC_ICR fires this off.

	apic_write( APIC_ICR, nReg );

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
	 *        This is to verify that we're looking at
	 *        a real local APIC.  Check these against
	 *        your board if the CPUs aren't getting
	 *        started for no apparent reason.
	 */

	printk( "Getting VERSION: %x\n", apic_read( APIC_VERSION ) );
	apic_write( APIC_VERSION, 0 );
	printk( "Getting VERSION: %x\n", apic_read( APIC_VERSION ) );

	/*
	 *        The two version reads above should print the same
	 *        NON-ZERO!!! numbers.  If the second one is zero,
	 *        there is a problem with the APIC write/read
	 *        definitions.
	 *
	 *        The next two are just to see if we have sane values.
	 *        They're only really relevant if we're in Virtual Wire
	 *        compatibility mode, but most boxes are anymore.
	 */

	printk( "Getting LVT0: %x\n", apic_read( APIC_LVT0 ) );
	printk( "Getting LVT1: %x\n", apic_read( APIC_LVT1 ) );
}
#endif // SMP_DEBUG


void smp_boot_cpus( void )
{
	int i;
	int nCPUCount = 0;
	uint32 nReg;
	void *pTrampoline;
	uint32 nBogoSum;
	int nID;

#ifdef SMP_DEBUG
	printk( "Boot CPU %d attempts to wake up AP's\n", get_processor_id() );
#endif

	pTrampoline = ( void * )( ( ( uint32 )alloc_real( 4096 + 4095, MEMF_CLEAR ) + 4095 ) & ~4095 );
	if ( pTrampoline == NULL )
	{
		panic( "No memory for processor trampoline.\n" );
	}

	g_asProcessorDescs[g_nBootCPU].pi_bIsRunning = true;

#ifdef SMP_DEBUG
	test_apic();
#endif

	// Clear the local apic
	apic_write( APIC_ESR, 0 );
	apic_write( APIC_ESR, 0 );
	apic_write( APIC_ESR, 0 );
	apic_write( APIC_ESR, 0 );
	
	apic_write( APIC_DFR, APIC_DFR_FLAT );

	nReg = apic_read( APIC_LDR ) & ~APIC_LDR_MASK;
	nReg |= SET_APIC_LOGICAL_ID( 1UL << get_processor_id() );
	apic_write( APIC_LDR, nReg );
	
	// Enable the local APIC
	nReg = apic_read( APIC_SPIV );
	nReg &=~ APIC_DEST_VECTOR_MASK;
	nReg |= APIC_SPIV_APIC_ENABLED;	/* Enable APIC */
	nReg &=~ APIC_SPIV_FOCUS_DISABLED; /* Disable focus */
	nReg |= INT_SPURIOUS;
	apic_write( APIC_SPIV, nReg );

	udelay( 10 );
	
	apic_write( APIC_LVT0, APIC_DEST_DM_EXTINT );
	apic_write( APIC_LVT1, APIC_DEST_DM_NMI );
	
	set_bit( &g_nCpuMask, g_nBootCPU );
	
	//        Now scan the cpu present map and fire up the other CPUs.

	for ( i = 0; i < MAX_CPU_COUNT; ++i )
	{
		unsigned long send_status, accept_status;
		int timeout, num_starts, j;

		// Hopefully this one is already running..
		if ( i == g_nBootCPU )
		{
			continue;
		}
		if ( g_asProcessorDescs[i].pi_bIsPresent == false )
		{
			continue;
		}

		// Install the 16-bit real-mode init function for this CPU
		install_trampoline( pTrampoline );

		printk( "Booting processor %d\n", i );

		CMOS_WRITE( 0xa, 0xf );
		*( ( volatile unsigned short * )0x469 ) = ( ( uint32 )pTrampoline ) >> 4;
		*( ( volatile unsigned short * )0x467 ) = sizeof( SmpTrampolineCfg_s );

		// Be paranoid about clearing APIC errors.

		if ( g_asProcessorDescs[i].pi_nAPICVersion & 0xF0 )
		{
			apic_write( APIC_ESR, 0 );
			accept_status = ( apic_read( APIC_ESR ) & 0xEF );
		}
		// Status is now clean

		send_status = 0;
		accept_status = 0;

		// Starting actual IPI sequence...

#ifdef SMP_DEBUG
		printk( "Asserting INIT.\n" );
#endif
		// Turn INIT on

		send_ipi( i, APIC_DEST_LEVELTRIG, APIC_DEST_ASSERT | APIC_DEST_DM_INIT );
		udelay( 200 );
#ifdef SMP_DEBUG
		printk( "Deasserting INIT.\n" );
#endif
		send_ipi( i, APIC_DEST_LEVELTRIG, APIC_DEST_DM_INIT );

		// Only send STARTUP IPIs if we got an integrated APIC.
		if ( g_asProcessorDescs[i].pi_nAPICVersion & 0xF0 )
		{
			num_starts = 2;
		}
		else
		{
			num_starts = 0;
		}

		// Run STARTUP IPI loop.

		for ( j = 1; !( send_status || accept_status ) && ( j <= num_starts ); ++j )
		{
#ifdef SMP_DEBUG
			printk( "Sending STARTUP #%d.\n", j );
#endif
			apic_write( APIC_ESR, 0 );

			send_ipi( i, 0, APIC_DEST_DM_STARTUP | ( ( ( int )pTrampoline ) >> 12 ) );

			timeout = 0;
			do
			{
				udelay( 10 );
			}
			while ( ( send_status = ( apic_read( APIC_ICR ) & 0x1000 ) ) && ( timeout++ < 1000 ) );
			udelay( 200 );

			accept_status = ( apic_read( APIC_ESR ) & 0xEF );
		}

		if ( send_status )
		{		// APIC never delivered??
			printk( "APIC never delivered???\n" );
		}
		if ( accept_status )
		{		// Send accept error
			printk( "APIC delivery error (%lx).\n", accept_status );
		}

		if ( !( send_status || accept_status ) )
		{
#ifdef SMP_DEBUG
			printk( "INIT sendt, wait for the AP to wake up\n" );
#endif
			for ( timeout = 0; timeout < 50000; timeout++ )
			{
				if ( g_asProcessorDescs[i].pi_bIsRunning )
				{
					break;	// It's alive
				}
				udelay( 100 );	// Wait 5s total for a response
			}
			if ( g_asProcessorDescs[i].pi_bIsRunning )
			{
				nCPUCount++;
			}
			else
			{
				printk( "CPU %d is not responding.\n", i );
			}
		}
	}
	// Paranoid:  Set warm reset code and vector here back
	// to default values.

	CMOS_WRITE( 0, 0xf );
	*( ( volatile long * )0x467 ) = 0;


	//        Allow the user to impress friends.

	nBogoSum = 0;
	nID = 0;

	for ( i = 0; i < MAX_CPU_COUNT; ++i )
	{
		if ( g_asProcessorDescs[i].pi_bIsRunning )
		{
			nBogoSum += g_asProcessorDescs[i].pi_nDelayCount;
			g_anLogicToRealID[nID++] = i;
		}
	}
	printk( "Total of %d processors activated (%u.%02u BogoMIPS).\n", nCPUCount + 1, nBogoSum / ( 500000 / INT_FREQ ), ( nBogoSum / ( 5000 / INT_FREQ ) ) % 100 );
	g_nActiveCPUCount = nCPUCount + 1;
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void apic_eoi( void )
{
	apic_read( APIC_SPIV );	// Dummy read. (see comment in smp.h)
	apic_write( APIC_EOI, 0 );	// Inform the apic that the interupt routine is done
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void do_smp_invalidate_pgt( SysCallRegs_s * psRegs, int nIrqNum )
{
	apic_eoi();
	if( test_and_clear_bit( &g_nTLBInvalidateMask, get_processor_id() ) )
	{
		flush_tlb();
	}
	if( test_and_clear_bit( &g_nMTRRInvalidateMask, get_processor_id() ) )
	{
		write_mtrr_descs();
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void do_smp_spurious_irq( void )
{
	printk( "Got APIC spurious interrupt\n" );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void do_smp_preempt( SysCallRegs_s * psRegs )
{
	apic_eoi();
	DoSchedule( psRegs );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_smp( bool bInitSMP, bool bScanACPI )
{
	bool bFound = false;
	int i;

	if ( bInitSMP )
	{
		uint32 anConfigAreas[] = {
			0x0, 0x400,	// Bottom 1k of base memory
			639 * 0x400, 0x400,	// Top 1k of base memory (FIXME: Should first verify that we have 640kb!!!)
			0xF0000, 0x10000,	// 64k of bios
			0, 0
		};
		
		uint32 anAcpiConfigAreas[] = {
			0x0, 0x400,	// Bottom 1k of base memory
			0xe0000, 0x20000,	// 64k of bios
			0, 0
		};

		if( bScanACPI )
		{
			printk( "Scan memory for ACPI SMP config tables...\n" );
		
			for ( i = 0; bFound == false && anAcpiConfigAreas[i + 1] > 0; i += 2 )
			{
				bFound = smp_scan_acpi_config( ( void * )anAcpiConfigAreas[i], anAcpiConfigAreas[i + 1] );
				if( bFound )
					break;
			}
		}
		
		if( !bFound )
		{
				
			printk( "Scan memory for MPC SMP config tables...\n" );
		
			for ( i = 0; bFound == false && anConfigAreas[i + 1] > 0; i += 2 )
			{
				bFound = smp_scan_mpc_config( ( void * )anConfigAreas[i], anConfigAreas[i + 1] );
				if( bFound )
					break;
			}	
		}
	}
	
	if ( ( bFound || ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_APIC ) ) && bInitSMP )
	{
		/* Map APIC registers */
		g_nVirtualAPICAddr = 0;
		g_hAPICArea = create_area( "apic_registers", ( void ** )&g_nVirtualAPICAddr, PAGE_SIZE, PAGE_SIZE, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK );

		if ( g_hAPICArea < 0 )
		{
			printk( "Error: init_smp() failed to create APIC register area\n" );
			return;
		}

#ifdef SMP_DEBUG
		printk( "Map in local APIC (%08x) to %08x\n", g_nPhysAPICAddr, g_nVirtualAPICAddr );
#endif
		remap_area( g_hAPICArea, ( void * )g_nPhysAPICAddr );

	}
	if ( !bFound )
	{
		/* Set default values for non-SMP machines */
		if ( ( ( g_asProcessorDescs[g_nBootCPU].pi_nFeatures & CPU_FEATURE_APIC ) ) && bInitSMP )
		{
			/* Identify as a smp system */
			g_nBootCPU = GET_APIC_ID( *( ( vuint32 * )( g_nVirtualAPICAddr + APIC_ID ) ) );
			g_nFakeCPUID = SET_APIC_ID( g_nBootCPU );
			g_bAPICPresent = true;
			bFound = true;
		}
		else
		{
			/* No speed detection for non-SMP machine with no APIC yet */
			g_nVirtualAPICAddr = ( ( uint32 )&g_nFakeCPUID ) - APIC_ID;
			g_asProcessorDescs[g_nBootCPU].pi_nBusSpeed = 0;
			g_asProcessorDescs[g_nBootCPU].pi_nCoreSpeed = 0;
			set_bit( &g_nCpuMask, g_nFakeCPUID );
		}
		g_asProcessorDescs[g_nBootCPU].pi_bIsPresent = true;
		g_asProcessorDescs[g_nBootCPU].pi_bIsRunning = true;
		g_anLogicToRealID[0] = g_nBootCPU;
	}
	
	g_bFoundSmpConfig = bFound;
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
	int i;
	
	printk( "Untie AP processors...\n" );
	
	calibrate_delay( get_processor_id() );
	if ( g_bFoundSmpConfig )
	{
		nFlg = cli();
		calibrate_apic_timer( get_processor_id() );
		smp_boot_cpus();
		put_cpu_flags( nFlg );
	}
	
	for ( i = 0; i < MAX_CPU_COUNT; ++i )
	{
		if ( g_asProcessorDescs[i].pi_bIsPresent && g_asProcessorDescs[i].pi_bIsRunning )
		{
			g_asProcessorDescs[i].pi_nGS = alloc_gdt_desc( 0 );
			set_gdt_desc_limit( g_asProcessorDescs[i].pi_nGS, TLD_SIZE );
			set_gdt_desc_base( g_asProcessorDescs[i].pi_nGS, 0 );
			set_gdt_desc_access( g_asProcessorDescs[i].pi_nGS, 0xf2 );
			printk( "CPU #%d got GS=%d\n", i, g_asProcessorDescs[i].pi_nGS );
		}
	}
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void flush_tlb_global( void )
{
	int nProcessor = get_processor_id();
	/* Update MTRR descriptors */
	if( test_and_clear_bit( &g_nMTRRInvalidateMask, nProcessor ) )
	{
		write_mtrr_descs();
	}
	
	if ( g_nActiveCPUCount > 1 )
	{
		uint32 nFlags = cli();
		g_nTLBInvalidateMask = g_nCpuMask;
 		send_ipi( MSG_ALL_BUT_SELF, APIC_DEST_DM_FIXED, INT_INVAL_PGT );
 		flush_tlb();
//		int nTimeout = 50000000;
//		while( g_nTLBInvalidateMask && nTimeout-- > 1 )
//		{
			if( test_and_clear_bit( &g_nTLBInvalidateMask, nProcessor ) )
			{
				flush_tlb();
			}
//		}
//		if( nTimeout == 0 )
//			printk("TLB flush timeout!\n");
//		g_nTLBInvalidateMask = 0;
		put_cpu_flags( nFlags );
	} else
		flush_tlb();
}

int logical_to_physical_cpu_id( int nLogicalID )
{
	return ( g_anLogicToRealID[nLogicalID] );
}

Thread_s *get_current_thread( void )
{
	Thread_s *psThread;
	int nFlg;

	nFlg = cli();
	psThread = g_asProcessorDescs[get_processor_id()].pi_psCurrentThread;
	put_cpu_flags( nFlg );
	return ( psThread );
}

Thread_s *get_idle_thread( void )
{
	return ( g_asProcessorDescs[get_processor_id()].pi_psIdleThread );
}

void set_idle_thread( Thread_s *psThread )
{
	g_asProcessorDescs[get_processor_id()].pi_psIdleThread = psThread;
}
