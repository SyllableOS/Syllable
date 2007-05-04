/*
 *  The Syllable kernel
 *  Copyright (C) 2006 Kristian Van Der Vliet
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

#include <inc/smp.h>
#include <inc/pit_timer.h>
#include <inc/scheduler.h>
#include <inc/io_ports.h>
#include <inc/mc146818.h>
#include <inc/sysbase.h>
#include <inc/areas.h>

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/irq.h>
#include <atheos/bitops.h>
#include <atheos/udelay.h>
#include <atheos/spinlock.h>
#include <atheos/time.h>

/* Kernel-global variables */
ProcessorInfo_s g_asProcessorDescs[MAX_CPU_COUNT];
int g_nBootCPU = 0;
int g_nActiveCPUCount = 1;
static int g_nLogicalCPU = 0;

static bigtime_t ( *g_pCPUTimeHandler )( int ) = NULL;

vuint32 g_nTLBInvalidateMask = 0;	/* CPUs that need to invalidate their TLB */
vuint32 g_nMTRRInvalidateMask = 0;	/* CPUs that need to invalidate their MTRR descriptors */
uint32 g_nCpuMask = 0; 				/* Installed & active CPU mask */

/* Used to map internal APIC IDs to the real APIC ID */
vint g_anLogicToRealID[MAX_CPU_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

int logical_to_physical_cpu_id( int nLogicalID )
{
	return g_anLogicToRealID[nLogicalID];
}

int get_active_cpu_count( void )
{
	return( g_nActiveCPUCount );
}

/**
 * Get a copy of the Extended CPU Info record for a specified CPU. 
 * NOTE: This function currently does *NO* locking, as there is no lock for g_asProcessorDescs.
 *  If the processor's info is being updated during copying, the copy may be inconsistent
 * \param nPhysicalCPUId - Physical ID for the CPU to get info for (as returned by get_processor_id() )
 * \param psInfo - pointer to pre-allocated CPU_Extended_Info_s struct, destination of CPU record.
 * \param nVersion - Version of the CPU_Extended_Info_s struct to use
 * \ingroup CPU
 * \author Tim ter Laak (timl [At] scintilla [DoT] utwente [DoT] nl)
 */
status_t get_cpu_extended_info(int nPhysicalCPUId, CPU_Extended_Info_s * _psInfo, int nVersion)
{
	if ( _psInfo == NULL )
		return -EFAULT;
	if ( nPhysicalCPUId >= MAX_CPU_COUNT )
	{
		return -ENODEV;
	}
	
	/* lock g_asProcessorDescs here? */

	switch ( nVersion )
	{
	case 1:
		{
			CPU_Extended_Info_v1_s * psInfo = (CPU_Extended_Info_v1_s *)_psInfo;
			
			psInfo->nAcpiId = g_asProcessorDescs[nPhysicalCPUId].pi_nAcpiId;
			strncpy(psInfo->anVendorID, g_asProcessorDescs[nPhysicalCPUId].pi_anVendorID, sizeof(psInfo->anVendorID) );
			strncpy(psInfo->zName, g_asProcessorDescs[nPhysicalCPUId].pi_zName, sizeof(psInfo->zName) );
			psInfo->nCoreSpeed = g_asProcessorDescs[nPhysicalCPUId].pi_nCoreSpeed;
			psInfo->nBusSpeed = g_asProcessorDescs[nPhysicalCPUId].pi_nBusSpeed;
			psInfo->nDelayCount = g_asProcessorDescs[nPhysicalCPUId].pi_nDelayCount;
			psInfo->nFamily = g_asProcessorDescs[nPhysicalCPUId].pi_nFamily;
			psInfo->nModel = g_asProcessorDescs[nPhysicalCPUId].pi_nModel;
			psInfo->nAPICVersion = g_asProcessorDescs[nPhysicalCPUId].pi_nAPICVersion;
			psInfo->bIsPresent = g_asProcessorDescs[nPhysicalCPUId].pi_bIsPresent;
			psInfo->bIsRunning = g_asProcessorDescs[nPhysicalCPUId].pi_bIsRunning;
			psInfo->bHaveFXSR = g_asProcessorDescs[nPhysicalCPUId].pi_bHaveFXSR;
			psInfo->bHaveXMM = g_asProcessorDescs[nPhysicalCPUId].pi_bHaveXMM;
			psInfo->bHaveMTRR = g_asProcessorDescs[nPhysicalCPUId].pi_bHaveMTRR;
			psInfo->nFeatures = g_asProcessorDescs[nPhysicalCPUId].pi_nFeatures;
			break;
		}
	default:
		{
			printk( "Error: get_cpu_extended_info() invalid version %d\n", nVersion );
			/* Unlock g_asProcessorDescs here? */
			return ( -EINVAL );
		}
	}		
			
	/* Unlock g_asProcessorDescs here? */
	return EOK;
}


/**
 * Update (set) speed metrics for a specified CPU.
 * NOTE: This function currently does *NO* locking, as there is no lock for g_asProcessorDescs.
 * If another thread reads between updating the two fields, its data may be inconsistent for our speed
 * data.
 * \param nPhysicalCPUId - Physical ID for the CPU to get info for (as returned by get_processor_id() )
 * \param nCoreSpeed - New CPU core speed in Hz
 * \param nDelayCount - New pi_nDelayCount value, used for busy-looping delays
 * \ingroup CPU
 * \author Tim ter Laak (timl [At] scintilla [DoT] utwente [DoT] nl)
 */
void update_cpu_speed(int nPhysicalCPUId, uint64 nCoreSpeed, uint32 nDelayCount)
{
	if ( nPhysicalCPUId >= MAX_CPU_COUNT )
		return;
	
	if ( g_asProcessorDescs[nPhysicalCPUId].pi_nDelayCount > nDelayCount )
	{
		/* scaling down, first update corespeed, then delaycount to mitigate issues
			from missing locking (only conceivable consequence: delays too long) */
		g_asProcessorDescs[nPhysicalCPUId].pi_nCoreSpeed = nCoreSpeed;
		g_asProcessorDescs[nPhysicalCPUId].pi_nDelayCount = nDelayCount;
	}
	else
	{
		/* The other way around, but for the same reason */
		g_asProcessorDescs[nPhysicalCPUId].pi_nDelayCount = nDelayCount;
		g_asProcessorDescs[nPhysicalCPUId].pi_nCoreSpeed = nCoreSpeed;
	}
	return;
}


/**
 * Overrides the cpu time handler.
 * \param pHandler - Pointer to the handler function. The parameter is the processor id.
 * \ingroup CPU
 * \author Arno Klenke
 */
void set_cpu_time_handler( bigtime_t ( *pHandler )( int ) )
{
	g_pCPUTimeHandler = pHandler;
}

/**
 * Returns the current cpu time. The functions calls the handler set by the set_cpu_time_handler()
 * function or get_system_time();
 * \param nProcessorID - The processor id.
 * \ingroup CPU
 * \author Arno Klenke
 */
inline bigtime_t get_cpu_time( int nProcessorID )
{
	if( g_pCPUTimeHandler != NULL )
		return( g_pCPUTimeHandler( nProcessorID ) );
	return( get_system_time() );
}


/* CPU calibration */
static int read_pit_timer( void )
{
	int nCount;

	isa_writeb( PIT_MODE, 0 );
	nCount = isa_readb( PIT_CH0 );
	nCount += isa_readb( PIT_CH0 ) << 8;
	return( nCount );
}

static void wait_pit_wrap( void )
{
	uint nCurrent = read_pit_timer();
	uint nPrev = 0;

	for(;;)
	{
		int nDelta;

		nPrev = nCurrent;
		nCurrent = read_pit_timer();
		nDelta = nCurrent - nPrev;

		if ( nDelta > 300 )
			break;
	}
}

void calibrate_delay( int nProcessor )
{
	unsigned long loops_per_jiffy = ( 1 << 12 );	/* Start at 2 BogoMips */
	unsigned long loopbit;
	unsigned long jiffy_countdown = 0xffff - ( PIT_TICKS_PER_SEC / INT_FREQ );
	count_t lps_precision = LPS_PREC;
	uint64 nStartPerf;
	uint64 nEndPerf;

	printk( "Calibrating delay loop for CPU %d\n", nProcessor );

	while( ( loops_per_jiffy <<= 1 ) != 0 )
	{
		isa_writeb( PIT_MODE, 0x30 );	/* one-shot mode */
		isa_writeb( PIT_CH0, 0xff );
		isa_writeb( PIT_CH0, 0xff );

		__delay( loops_per_jiffy );

		if ( read_pit_timer() < jiffy_countdown )
			break;
	}

	/* Do a binary approximation to get loops_per_jiffy set to equal one clock (up to lps_precision bits) */
	loops_per_jiffy >>= 1;
	loopbit = loops_per_jiffy;
	while( lps_precision-- && ( loopbit >>= 1 ) )
	{
		loops_per_jiffy |= loopbit;

		isa_writeb( PIT_MODE, 0x30 );	/* one-shot mode */
		isa_writeb( PIT_CH0, 0xff );
		isa_writeb( PIT_CH0, 0xff );
		__delay( loops_per_jiffy );

		if ( read_pit_timer() < jiffy_countdown )
			loops_per_jiffy &= ~loopbit;
	}

	/* Calculate CPU core speed */
	isa_writeb( PIT_MODE, 0x34 );	/* loop mode */
	isa_writeb( PIT_CH0, 0xff );
	isa_writeb( PIT_CH0, 0xff );

	wait_pit_wrap();
	nStartPerf = read_pentium_clock();
	wait_pit_wrap();
	nEndPerf = read_pentium_clock();

	g_asProcessorDescs[nProcessor].pi_nCoreSpeed = ( ( uint64 )PIT_TICKS_PER_SEC * ( nEndPerf - nStartPerf ) / 0xffff );

	isa_writeb( PIT_MODE, 0x34 );	/* loop mode */
	isa_writeb( PIT_CH0, 0x0 );
	isa_writeb( PIT_CH0, 0x0 );

	g_asProcessorDescs[get_processor_id()].pi_nDelayCount = loops_per_jiffy;

	/* Round the value and print it. */
	printk( "CPU %d - %lu.%02lu BogoMIPS (lpj=%lu)\n", nProcessor, loops_per_jiffy / ( 500000 / INT_FREQ ), ( loops_per_jiffy / ( 5000 / INT_FREQ ) ) % 100, loops_per_jiffy );
	printk( "CPU %d runs at %d.%04d MHz\n", nProcessor, (int)( g_asProcessorDescs[nProcessor].pi_nCoreSpeed / 1000000 ), (int)( ( g_asProcessorDescs[nProcessor].pi_nCoreSpeed / 100 ) % 10000 ) );
}

static void calibrate_apic_timer( int nProcessor )
{
	uint32 nAPICCount;
	uint32 nReg;

	kerndbg( KERN_DEBUG, "Calibrate APIC %d\n", nProcessor );

	nReg = apic_read( APIC_TDCR ) & ~0x0f;
	nReg |= APIC_TDR_DIV_1;
	apic_write( APIC_TDCR, nReg );

	isa_writeb( PIT_MODE, 0x34 );	/* loop mode */
	isa_writeb( PIT_CH0, 0xff );
	isa_writeb( PIT_CH0, 0xff );

	wait_pit_wrap();
	apic_write( APIC_TMICT, ~0 );	/* Start APIC timer */
	wait_pit_wrap();
	nAPICCount = apic_read( APIC_TMCCT );

	g_asProcessorDescs[nProcessor].pi_nBusSpeed = (uint64)( ( uint64 )PIT_TICKS_PER_SEC * ( 0xffffffffLL - nAPICCount ) / 0xffff );
	printk( "CPU %d bus runs at %d.%04d MHz\n", nProcessor, (int)( g_asProcessorDescs[nProcessor].pi_nBusSpeed / 1000000 ), (int)( ( g_asProcessorDescs[nProcessor].pi_nBusSpeed / 100 ) % 10000 ) );
}

/* Entry point into the kernel from the 16bit trampoline for aux. processors */
static void ap_entry_proc( void )
{
	struct i3DescrTable sIDT;
	unsigned int nDummy;
	uint32 nCR0;
	int nCPU;
	bool bAPIC;

	set_page_directory_base_reg( g_psKernelSeg->mc_pPageDir );
	__asm__ __volatile__( "movl %%cr0,%0; orl $0x80010000,%0; movl %0,%%cr0" : "=r" (nDummy) );	/* Set PG & WP bit in cr0 */

	nCPU = get_processor_id();
	kerndbg( KERN_DEBUG, "CPU #%d in ap_entry_proc()\n", nCPU );
	

	/* Workaround for lazy bios'es that forget to enable the cache on AP processors.
	   Also set NE and MP flags and clear EM flag for x87 native mode */
	__asm__ __volatile__( "movl %%cr0,%0":"=r"( nCR0 ) );
	__asm__ __volatile__( "movl %0,%%cr0"::"r"( ( nCR0 & 0x9ffffffb ) | 0x22 ) );

	/* Enable SSE support */
	if( g_asProcessorDescs[nCPU].pi_bHaveFXSR )
		set_in_cr4( X86_CR4_OSFXSR );
	if( g_asProcessorDescs[nCPU].pi_bHaveXMM )
		set_in_cr4( X86_CR4_OSXMMEXCPT );

	/* Do some housekeeping */
	g_nLogicalCPU += 1;
	g_anLogicToRealID[g_nLogicalCPU] = nCPU;

	sIDT.Base = (uint32)g_sSysBase.ex_IDT;
	sIDT.Limit = 0x7ff;
	SetIDT( &sIDT );

	sIDT.Base = (uint32)g_sSysBase.ex_GDT;
	sIDT.Limit = 0xffff;
	SetGDT( &sIDT );

	SetTR( ( 8 + nCPU ) << 3 );
	__asm__ __volatile__( "mov %0,%%ds;mov %0,%%es;mov %0,%%fs;mov %0,%%gs;mov %0,%%ss;"::"r" ( 0x18 ) );

	/* Ensure our APIC is working correctly */
	bAPIC = test_local_apic();
	if( bAPIC == false )
	{
		kerndbg( KERN_WARNING, "Local APIC test failed on CPU #%d!\n", nCPU );
		goto halt;
	}

	/* Get CPU core speed */
	calibrate_delay( nCPU );

	/* Configure local APIC for this CPU */
	setup_local_apic();

	/* Get CPU bus speed */
	calibrate_apic_timer( nCPU );

	/* This CPU is alive */
	flush_tlb();
	set_bit( nCPU, &g_nCpuMask );
	write_mtrr_descs();

	g_asProcessorDescs[nCPU].pi_bIsRunning = true;

	/* Got to have something to run... */
	create_idle_thread( "idle" );

halt:
	/* Failed to start the processor, so do nothing in the hope we don't harm anybody */	
	kerndbg( KERN_FATAL, "Failed to initialise CPU #%d!\n", nCPU );

	for(;;)
		__asm__( "hlt" );
}

uint8 g_anTrampoline[] = {
	#include "objs/smp_entry.hex"
};

static void install_trampoline( char *pBuffer )
{
	SmpTrampolineCfg_s *psCfg = (SmpTrampolineCfg_s *)pBuffer;

	memcpy( pBuffer, g_anTrampoline, sizeof( g_anTrampoline ) );

	psCfg->tc_nKernelEntry = (uint32)ap_entry_proc;
	psCfg->tc_nKernelStack = ( (uint32)kmalloc( 4096, MEMF_KERNEL ) ) + 4092;
	psCfg->tc_nKernelDS = DS_KERNEL;
	psCfg->tc_nKernelCS = CS_KERNEL;
	psCfg->tc_nGdt = (uint32)g_sSysBase.ex_GDT;	/* Pointer to global descriptor table */
}

/* Create trampoline and send STARTUP to the aux. processors */
static void startup_ap_processors( int nCPU )
{
	int nID, nCPUCount = 0, i;
	uint32 nBogoSum;
	void *pTrampoline;

	kerndbg( KERN_DEBUG_LOW, "startup_ap_processors( %d )\n", nCPU );

	/* We now officially exist */
	g_asProcessorDescs[nCPU].pi_bIsRunning = true;
	set_bit( nCPU, &g_nCpuMask );

	/* The *logical* CPU ID.  We use this as in index into g_asProcessorDescs, so it starts at 0 (This CPU) and is
	   incremented by the loop or ap_entry_proc() */
	g_nLogicalCPU = 0;

	/* Boot the other CPUs */
	pTrampoline = ( void * )( ( ( uint32 )alloc_real( 4096 + 4095, MEMF_CLEAR ) + 4095 ) & ~4095 );
	if( pTrampoline == NULL )
		panic( "No memory for processor trampoline.\n" );

	for( i=0; i < MAX_CPU_COUNT; i++ )
	{
		uint32 nSendStatus = 0, nAcceptStatus = 0;
		int nTimeout, nNumStarts;
		int j;

		if( g_nBootCPU == i )
		{
			g_nLogicalCPU += 1;
			continue;
		}
		if( g_asProcessorDescs[i].pi_bIsPresent == false )
			continue;

		/* Install the 16-bit real-mode init function for this CPU */
		install_trampoline( pTrampoline );

		printk( "Booting processor #%d\n", i );

		CMOS_WRITE( 0xa, 0xf );
		*( (volatile unsigned short *)0x469 ) = ( (uint32)pTrampoline ) >> 4;
		*( (volatile unsigned short *)0x467 ) = sizeof( SmpTrampolineCfg_s );

		if( g_asProcessorDescs[i].pi_nAPICVersion & 0xF0 )
		{
			apic_read( APIC_SPIV );
			apic_write( APIC_ESR, 0 );
			apic_read( APIC_ESR );
		}

		kerndbg( KERN_DEBUG, "Asserting INIT\n" );

		/* Turn on INIT on the target */
		apic_write( APIC_ICR2, SET_APIC_DEST_FIELD( i ) );

		/* Send IPI */
		apic_write( APIC_ICR, APIC_ICR_LEVELTRIG | APIC_ICR_ASSERT | APIC_DM_INIT );

		nTimeout = 0;
		do
		{
			udelay( 100 );
			nSendStatus = apic_read( APIC_ICR ) & APIC_ICR_BUSY;
		}
		while( nSendStatus && ( nTimeout++ < 1000 ) );
		udelay( 1000 );

		kerndbg( KERN_DEBUG, "De-asserting INIT\n" );

		/* Turn off INIT on the target */
		apic_write( APIC_ICR2, SET_APIC_DEST_FIELD( i ) );

		/* Send IPI */
		apic_write( APIC_ICR, APIC_ICR_LEVELTRIG | APIC_DM_INIT );

		nTimeout = 0;
		do
		{
			udelay( 100 );
			nSendStatus = apic_read( APIC_ICR ) & APIC_ICR_BUSY;
		}
		while( nSendStatus && ( nTimeout++ < 1000 ) );
		udelay( 1000 );

		/* Don't send a STARTUP IPI on non-integrated APICs */
		if( g_asProcessorDescs[i].pi_nAPICVersion & 0xf0 )
			nNumStarts = 2;
		else
			nNumStarts = 0;

		/* Run STARTUP IPI loop */
		for( j = 1; j <= nNumStarts; j++ )
		{
			kerndbg( KERN_DEBUG, "Sending STARTUP #%d\n", j );

			apic_read( APIC_SPIV );
			apic_write( APIC_ESR, 0 );
			apic_read( APIC_ESR );

			/* Target */
			apic_write( APIC_ICR2, SET_APIC_DEST_FIELD( i ) );

			apic_write( APIC_ICR, APIC_DM_STARTUP | ( ( ( int )pTrampoline ) >> 12 ) );

			udelay( 300 );
			nTimeout = 0;
			do
			{
				udelay( 100 );
				nSendStatus = apic_read( APIC_ICR ) & APIC_ICR_BUSY;
			}
			while( nSendStatus && ( nTimeout++ < 1000 ) );
			udelay( 200 );

			nAcceptStatus = ( apic_read( APIC_ESR ) & 0xef );
			if( nSendStatus || nAcceptStatus )
				break;
		}

		kerndbg( KERN_DEBUG, "STARTUP complete\n" );

		if( nSendStatus )
			kerndbg( KERN_DEBUG, "APIC never delivered IPI? (%x)\n", nSendStatus );
		if( nAcceptStatus )
			kerndbg( KERN_DEBUG, "APIC IPI delivery error (%x)\n", nAcceptStatus );

		for( nTimeout = 0; nTimeout < 50000; nTimeout++ )
		{
			if( g_asProcessorDescs[i].pi_bIsRunning )
				break;		/* It's alive */
			udelay( 100 );	/* Wait 5s total for a response */
		}

		if( g_asProcessorDescs[i].pi_bIsRunning )
		{
			kerndbg( KERN_DEBUG, "CPU #%d started\n", i );
			nCPUCount++;
		}
		else
			printk( "CPU #%d is not responding.\n", i );
	}

	/* Paranoid:  Set warm reset code and vector here back to default values. */
	CMOS_WRITE( 0, 0xf );
	*( ( volatile long * )0x467 ) = 0;

	/* Allow the user to impress friends. */
	nBogoSum = 0;
	nID = 0;
	for( i = 0; i < MAX_CPU_COUNT; ++i )
	{
		if( g_asProcessorDescs[i].pi_bIsRunning )
		{
			nBogoSum += g_asProcessorDescs[i].pi_nDelayCount;
			g_anLogicToRealID[nID++] = i;
		}
	}
	printk( "Total of %d processors activated (%u.%02u BogoMIPS).\n", nCPUCount + 1, nBogoSum / ( 500000 / INT_FREQ ), ( nBogoSum / ( 5000 / INT_FREQ ) ) % 100 );
	g_nActiveCPUCount = nCPUCount + 1;
}

/**
 * Shutdown all but the boot processor.
 * \internal
 * \ingroup CPU.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
void shutdown_ap_processors( void )
{
	int i;
	uint32 nFlags = cli();
	while( get_processor_id() != g_nBootCPU )
		Schedule();
	sched_lock();
	for ( i = 0; i < MAX_CPU_COUNT; ++i )
	{
		if( i == g_nBootCPU || g_asProcessorDescs[i].pi_bIsRunning == false )
			continue;
		printk( "Shutting down CPU #%d\n", i );
		g_asProcessorDescs[i].pi_bIsRunning = false;
	}
	sched_unlock();
	put_cpu_flags( nFlags );
	snooze( 100000 );
}

/**
 * Shutdown the current processor. Needs to be called with disabled interrupts.
 * \internal
 * \ingroup CPU.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 */
void shutdown_processor( void )
{
	kassertw( !( get_cpu_flags() & EFLG_IF ) );
	int nProcessor = get_processor_id();
	printk( "Shutting down CPU #%d...\n", nProcessor );	
	clear_bit( nProcessor, &g_nCpuMask );
	clear_bit( nProcessor, &g_nTLBInvalidateMask );
	g_nActiveCPUCount--;
	for ( ;; )
		__asm__( "hlt" );
}

Thread_s *get_current_thread( void )
{
	Thread_s *psThread;
	uint32 nFlags = cli();

	psThread = get_processor()->pi_psCurrentThread;

	put_cpu_flags( nFlags );
	return( psThread );
}

Thread_s *get_idle_thread( void )
{
	return( get_processor()->pi_psIdleThread );
}

void set_idle_thread( Thread_s *psThread )
{
	get_processor()->pi_psIdleThread = psThread;
}

static void list_threads( void )
{
	thread_id hThread;
	Thread_s *psThread;

	printk( "ID      PROC        STATE      PRI\n" );
	FOR_EACH_THREAD( hThread, psThread )
	{
		char *pzState;
		char zState[128];

		switch ( psThread->tr_nState )
		{
			case TS_STOPPED:
				pzState = "Stopped";
				break;
			case TS_RUN:
				pzState = "Run";
				break;
			case TS_READY:
				pzState = "Ready";
				break;
			case TS_WAIT:
				pzState = "Wait";
				break;
			case TS_SLEEP:
				pzState = "Sleep";
				break;
			case TS_ZOMBIE:
				pzState = "Zombie";
				break;
			default:
				pzState = "Invalid";
				break;
		}

		if( psThread->tr_hBlockSema >= 0 )
		{
			Semaphore_s *psSema = get_semaphore_by_handle( psThread->tr_psProcess->tc_hProcID, psThread->tr_hBlockSema );

			if( NULL == psSema )
			{
				sprintf( zState, "inval_sema(%d)", psThread->tr_hBlockSema );
				pzState = zState;
			}
			else
			{
				sprintf( zState, "%s(%d)", psSema->ss_zName, psThread->tr_hBlockSema );
				pzState = zState;
			}
		}
		printk( "%05d - %05d %-12s %04d %s::%s\n", hThread, psThread->tr_psProcess->tc_hProcID, pzState, psThread->tr_nPriority, psThread->tr_zName, psThread->tr_psProcess->tc_zName );
	}
}

void flush_tlb_global( void )
{
	uint32 nFlags = cli();
	int nCPU = get_processor_id();

	/* Update MTRR descriptors */
	if( test_and_clear_bit( nCPU, &g_nMTRRInvalidateMask ) )
		write_mtrr_descs();
	
	if( g_nActiveCPUCount > 1 )
	{
		g_nTLBInvalidateMask = g_nCpuMask;
		kerndbg( KERN_DEBUG_LOW, "Send TLB flush %i\n", nCPU );
 		apic_send_ipi( APIC_DEST_ALLBUT, APIC_DM_FIXED, INT_INVAL_PGT );

		int nTimeout = 50000000;
		while( g_nTLBInvalidateMask && nTimeout-- > 1 )
			if( test_and_clear_bit( nCPU, &g_nTLBInvalidateMask ) )
				flush_tlb();

		if( nTimeout == 0 )
		{
			kerndbg( KERN_WARNING, "TLB flush timeout %i %i %x %x %x %x %x %x!\n", get_processor_id(), nCPU, (uint)apic_read( APIC_ISR ), (uint)apic_read( APIC_IRR ),  (uint)apic_read( APIC_TASKPRI ), (uint)apic_read( APIC_ESR ), (uint)apic_read( APIC_ICR ), g_nTLBInvalidateMask );
			int nOther = !get_processor_id();
			if( g_asProcessorDescs[nOther].pi_psCurrentThread == NULL )
			{
				kerndbg( KERN_WARNING, "Has no thread running\n" );
			}
			else
				kerndbg( KERN_WARNING, "%i %i %i:%s:%i %i:%s:%i\n", nOther, get_processor_id(), g_asProcessorDescs[get_processor_id()].pi_psCurrentThread->tr_hThreadID, g_asProcessorDescs[get_processor_id()].pi_psCurrentThread->tr_zName,g_asProcessorDescs[get_processor_id()].pi_psCurrentThread->tr_nCurrentCPU, g_asProcessorDescs[nOther].pi_psCurrentThread->tr_hThreadID, g_asProcessorDescs[nOther].pi_psCurrentThread->tr_zName, g_asProcessorDescs[nOther].pi_psCurrentThread->tr_nCurrentCPU );
			list_threads();
		}
	}
	else
		flush_tlb();

	put_cpu_flags( nFlags );
}

void do_smp_invalidate_pgt( SysCallRegs_s * psRegs, int nIrqNum )
{
	int nCPU = get_processor_id();

	/* Update MTRR descriptors */
	if( test_and_clear_bit( nCPU, &g_nMTRRInvalidateMask ) )
		write_mtrr_descs();

	/* Flush TLB */
	if( test_and_clear_bit( nCPU, &g_nTLBInvalidateMask ) )
		flush_tlb();

	apic_eoi();
}

void do_smp_spurious_irq( void )
{
	apic_eoi();
	kerndbg( KERN_WARNING, "APIC %d got spurious interrupt\n", get_processor_id() );
}

void do_smp_preempt( SysCallRegs_s * psRegs )
{
	apic_eoi();
	DoSchedule( psRegs );
}

static void ( *g_pIdleLoopHandler )( int ) = NULL;

void idle_loop( void )
{
	char zThreadName[32];
	int nProcessor = 0;
	
	cli();

	nProcessor = get_processor_id();

	sprintf( zThreadName, "idle_%02d", nProcessor );
	rename_thread( -1, zThreadName );

	if( g_bAPICPresent )
	{
		uint32 nAPICDiv;
		uint32 nReg;

		nAPICDiv = g_asProcessorDescs[nProcessor].pi_nBusSpeed / INT_FREQ;
		nAPICDiv /= 16;

		kerndbg( KERN_WARNING, "APIC divisor = %d\n", nAPICDiv );
		
		nReg = apic_read( APIC_TDCR ) & ~0x0f;
		nReg |= APIC_TDR_DIV_16;
		apic_write( APIC_TDCR, nReg );

		apic_write( APIC_LVTT, APIC_LVT_TIMER_PERIODIC | INT_SCHEDULE );	/* Make APIC local timer trig int INT_SCHEDULE */
		apic_write( APIC_TMICT, nAPICDiv );									/* Start APIC timer */
	}
	
	sti();
	for(;;)
	{
		__asm__ volatile ( "nop" : : : "memory" );
		if( g_pIdleLoopHandler == NULL ) {
			__asm__ volatile ( "hlt" );
		} else {
			g_pIdleLoopHandler( nProcessor );
		}
	}
}

/**
 * Overrides the idle handler.
 * \param pHandler - Pointer to the handler function. The parameter is the processor id.
 * \ingroup CPU
 * \author Arno Klenke
 */
void set_idle_loop_handler( void ( *pHandler )( int ) )
{
	g_pIdleLoopHandler = pHandler;
}

static void db_disable_idle_handler( int argc, char **argv )
{
	set_idle_loop_handler( NULL );
	dbprintf( DBP_DEBUGGER, "Idle handler disabled\n" );
}

/* Initialise the SMP subsystem */
void init_smp( bool bInitSMP, bool bScanACPI )
{
	kerndbg( KERN_DEBUG, "init_smp( %s, %s )\n", bInitSMP ? "true" : "false", bScanACPI ? "true" : "false" );
	
	register_debug_cmd( "disable_idle_handler", "Disables a custom idle handler.", db_disable_idle_handler );

	if( bInitSMP )
		g_bAPICPresent = init_apic( bScanACPI );

	if( g_bAPICPresent == false )
	{
		/* No busspeed detection for non-SMP machine with no APIC yet */
		g_asProcessorDescs[g_nBootCPU].pi_nBusSpeed = 0;

		/* startup_ap_processors() will not be called, so we'd better make sure we put ourselves in the CPU mask */
		set_bit( g_nBootCPU, &g_nCpuMask );

		g_asProcessorDescs[g_nBootCPU].pi_bIsPresent = true;
		g_asProcessorDescs[g_nBootCPU].pi_bIsRunning = true;

		/* May as well take task register #0 */
		SetTR( 8 << 3 );
	}
	
	calibrate_delay( g_nBootCPU );
	
	if( g_bAPICPresent == true )
	{
		/* Initialize the local apic of the boot cpu */
		setup_local_apic();
		calibrate_apic_timer( g_nBootCPU );
	}
}

/* Boot auxilary processors */
void boot_ap_processors( void )
{
	uint32 nFlags = 0;
	int i, nCPU;

	kerndbg( KERN_DEBUG, "boot_ap_processors()\n" );

	nCPU = get_processor_id();
	if( nCPU != g_nBootCPU )
		kerndbg( KERN_WARNING, "This is not the boot CPU! (%d, booted on %d)\n", nCPU, g_nBootCPU );

	/* Start ap processors */
	if( g_bAPICPresent )
	{
		nFlags = cli();
		startup_ap_processors( nCPU );
		put_cpu_flags( nFlags );
	}

	/* Create GS segment for each active processor */
	for( i = 0; i < MAX_CPU_COUNT; ++i )
	{
		if( g_asProcessorDescs[i].pi_bIsPresent && g_asProcessorDescs[i].pi_bIsRunning )
		{
			g_asProcessorDescs[i].pi_nGS = alloc_gdt_desc( 0 );
			set_gdt_desc_limit( g_asProcessorDescs[i].pi_nGS, TLD_SIZE );
			set_gdt_desc_base( g_asProcessorDescs[i].pi_nGS, 0 );
			set_gdt_desc_access( g_asProcessorDescs[i].pi_nGS, 0xf2 );
			kerndbg( KERN_DEBUG, "CPU #%d got GS=%d\n", i, g_asProcessorDescs[i].pi_nGS );
		}
	}
}

