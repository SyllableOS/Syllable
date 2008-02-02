
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2005 Jake Hamby
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

/** \file timer.c
 * This file contains the timer interrupt handler and functions to return
 * the system time (time elapsed since boot), real time (since 1970-01-01),
 * and per-CPU idle time.  All time values are returned in microseconds.
 */

#include <atheos/types.h>
#include <atheos/isa_io.h>
#include <atheos/kernel.h>
#include <atheos/time.h>
#include <atheos/irq.h>
#include <atheos/smp.h>
#include <atheos/seqlock.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/io_ports.h"
#include "inc/pit_timer.h"
#include "inc/mc146818.h"
#include "inc/global.h"
#include "inc/smp.h"

/** If true, the scheduler will be called when returning from syscall. */
int g_bNeedSchedule;

SEQ_LOCK( g_sTimerSeqLock, "timer_slock" );
SPIN_LOCK( g_sPitTimerSpinLock, "i8253_slock" );

//****************************************************************************/
/** Returns the number of microseconds since the last clock tick.
 * \internal
 * \ingroup Timers
 * \return the number of microseconds since the last clock tick.
 * \author Jake Hamby (jhamby@pobox.com), based on Linux kernel.
 *****************************************************************************/
static uint32 get_pit_offset( void )
{
	uint32 nCount;
	uint32 nFlg = spinlock_disable( &g_sPitTimerSpinLock );

	outb_p( 0x00, PIT_MODE );	// latch the count
	nCount = inb_p( PIT_CH0 );
	nCount |= inb_p( PIT_CH0 ) << 8;

	spinunlock_enable( &g_sPitTimerSpinLock, nFlg );

	nCount = ( ( LATCH - 1 ) - nCount ) * ( 1000000 / INT_FREQ );
	nCount = ( nCount + LATCH / 2 ) / LATCH;
	return nCount;
}

//****************************************************************************/
/** Returns the time elapsed since last system boot.
 * \ingroup Timers
 * \return Time since last system boot, in microseconds.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
bigtime_t get_system_time( void )
{
	bigtime_t nTime;
	uint32 nSeq;

	do
	{
		nSeq = read_seqbegin( &g_sTimerSeqLock );
		nTime = g_sSysBase.ex_nRealTime - g_sSysBase.ex_nBootTime;
//		nTime += get_pit_offset();
	}
	while ( read_seqretry( &g_sTimerSeqLock, nSeq ) );

	return ( nTime );
}

//****************************************************************************/
/** Returns the number of microseconds since 1970-01-01.
 * \ingroup Timers
 * \return The number of microseconds since 1970-01-01.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
bigtime_t get_real_time( void )
{
	bigtime_t nTime;
	uint32 nSeq;

	do
	{
		nSeq = read_seqbegin( &g_sTimerSeqLock );
		nTime = g_sSysBase.ex_nRealTime;
//		nTime += get_pit_offset();
	}
	while ( read_seqretry( &g_sTimerSeqLock, nSeq ) );

	return ( nTime );
}


//****************************************************************************/
/** Returns the total idle time for the given CPU.
 * \ingroup Timers
 * \param nProcessor the processor for which to return the idle time.
 * \return Idle time for the given CPU, in microseconds.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
bigtime_t get_idle_time( int nProcessor )
{
	uint32 nFlags;
	uint32 nPhysCPU;
	bigtime_t nTime;
	if ( nProcessor < 0 || nProcessor >= g_nActiveCPUCount )
	{
		return ( 0 );
	}
	nPhysCPU = logical_to_physical_cpu_id( nProcessor );
	nFlags = cli();
	sched_lock();
	/* On smp machines this cpu might currently run the idle thread */
	nTime = g_asProcessorDescs[nPhysCPU].pi_nIdleTime;
	if( g_asProcessorDescs[nPhysCPU].pi_psCurrentThread == g_asProcessorDescs[nPhysCPU].pi_psIdleThread )
	{
		nTime += g_asProcessorDescs[nPhysCPU].pi_nCPUTime - g_asProcessorDescs[nPhysCPU].pi_psCurrentThread->tr_nLaunchTime;
	}
	sched_unlock();
	put_cpu_flags( nFlags );
	return( nTime );
}

//****************************************************************************/
/** Returns the time elapsed since last system boot, in microseconds.
 * \ingroup Syscalls
 * \param pRes a pointer to the bigtime_t in which to store the system time.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
int sys_get_raw_system_time( bigtime_t *pRes )
{
	int nError = 0;

	if ( nError >= 0 )
	{
//    nError = validate_area( pRes, sizeof( bigtime_t ), TRUE );

//    if ( 0 == nError ) {
		*pRes = get_system_time();
//    }
	}
	return ( nError );
}

//****************************************************************************/
/** Returns the number of microseconds since 1970-01-01.
 * \ingroup Syscalls
 * \param pRes a pointer to the bigtime_t in which to store the current time.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
int sys_get_raw_real_time( bigtime_t *pRes )
{
	int nError = 0;

	if ( nError >= 0 )
	{
//    nError = validate_area( pRes, sizeof( bigtime_t ), TRUE );

//    if ( 0 == nError ) {
		*pRes = get_real_time();
//    }
	}
	return ( nError );
}

//****************************************************************************/
/** Returns the total idle time for the given CPU, in microseconds.
 * \ingroup Syscalls
 * \param pRes a pointer to the bigtime_t in which to store the CPU idle time.
 * \param nProcessor the processor for which to return the idle time.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
int sys_get_raw_idle_time( bigtime_t *pRes, int nProcessor )
{
	int nError = 0;

	if ( nError >= 0 )
	{
//    nError = validate_area( pRes, sizeof( bigtime_t ), TRUE );

//    if ( 0 == nError ) {
		*pRes = get_idle_time( nProcessor );
//    }
	}
	return ( nError );
}

//****************************************************************************/
/** Sets the system clock to a new time.  Does not set the RTC.
 * \ingroup Syscalls
 * \param nTimeLow the low 32 bits of the new system time, in microseconds since 1970-01-01.
 * \param nTimeHigh the high 32 bits of the new system time
 * \return Always returns 0.
 * \attention Missing check for sufficient privileges to set the system clock.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
int sys_set_real_time( uint32 nTimeLow, uint32 nTimeHigh )
{
	uint32 nFlags;
	bigtime_t nTime = ((bigtime_t)nTimeHigh << 32 ) | nTimeLow;

	nFlags = write_seqlock_disable( &g_sTimerSeqLock );
	g_sSysBase.ex_nBootTime += nTime - g_sSysBase.ex_nRealTime;
	g_sSysBase.ex_nRealTime = nTime;
	write_sequnlock_enable( &g_sTimerSeqLock, nFlags );
	return ( 0 );
}

//****************************************************************************/
/** Starts PIT timer 1 and sets the latch value to trigger an interrupt at a
 *  frequency of INT_FREQ Hz.
 * \internal
 * \ingroup Timers
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
static void init_timer1( void )
{
	/*	outb_p( 0x30, PIT_MODE );	*//*      One shot mode   */
	outb_p( 0x34, PIT_MODE );	/* Loop mode    */
	outb_p( LATCH & 0xff, PIT_CH0 );
	outb_p( LATCH >> 8, PIT_CH0 );
}

//****************************************************************************/
/** Initializes PIT timer 2 to the disabled state.
 * \internal
 * \ingroup Timers
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
static void init_timer2( void )
{
	outb_p( 0xB4, PIT_MODE );
	outb_p( 0, PIT_CH2 );
	outb_p( 0, PIT_CH2 );
}

//****************************************************************************/
/** This is the timer interrupt handler, called INT_FREQ times per second.  It
 *  updates the system time and calls functions send_timer_signals() and
 *  wake_up_sleepers().  If there is no APIC present, Schedule() is also
 *  called here; otherwise, Schedule() is called for each CPU via
 *  do_smp_preempt(), which is triggered by the APIC timer.
 * \ingroup Timers
 * \param dummy unused.
 * \sa idle_loop()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void TimerInterrupt( SysCallRegs_s* psRegs )
{
	bigtime_t nCurTime;
	
	if ( get_processor_id() != g_nBootCPU )
	{
		printk( "Got timer IRQ to CPU %d (Booted on %d)\n", get_processor_id(), g_nBootCPU );
		return;
	}

	outb( 0x20, PIC_MASTER_CMD );	/* Give handshake to interupt controller        */
	
	write_seqlock( &g_sTimerSeqLock );
	g_sSysBase.ex_nRealTime += ( uint64 )( 1000000 / INT_FREQ );
	nCurTime = g_sSysBase.ex_nRealTime - g_sSysBase.ex_nBootTime;
	write_sequnlock( &g_sTimerSeqLock );

	send_timer_signals( nCurTime );
	wake_up_sleepers( nCurTime );
	if( g_bAPICPresent == false )
	{
		DoSchedule( psRegs );
	}
}

//****************************************************************************/
/** Initializes the PIT timers.
 * \ingroup Timers
 * \sa kernel_init()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void start_timer_int( void )
{
	uint32 nFlg = spinlock_disable( &g_sPitTimerSpinLock );

	outb_p( inb_p( 0x61 ) | 1, 0x61 );

	init_timer1();
	init_timer2();

	spinunlock_enable( &g_sPitTimerSpinLock, nFlg );
}

#if 0
//****************************************************************************/
/** Stops the PIT timer (currently unused).
 * \ingroup Timers
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void stop_timer_int( void )
{
	uint32 nFlg = spinlock_disable( &g_sPitTimerSpinLock );
	outb_p( 0x34, PIT_MODE );
	outb_p( 0, PIT_CH0 );
	outb_p( 0, PIT_CH0 );
	spinunlock_enable( &g_sPitTimerSpinLock, nFlg );
}
#endif

//****************************************************************************/
/** Sets the system time from the battery-backed CMOS clock.
 * \ingroup Timers
 * \sa kernel_init()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void get_cmos_time( void )
{
	ClockTime_s sTime;
	uint32 year, mon, day, hour, min, sec;
	int i;
	uint64 a, b, c;


	/* The AtheOS interpretation of the CMOS clock register contents:
	 * When the Update-In-Progress (UIP) flag goes from 1 to 0, the
	 * RTC registers show the second which has precisely just started.
	 * Let's hope other operating systems interpret the RTC the same way.
	 */

	/* read RTC exactly on falling edge of update flag */

	for ( i = 0; i < 1000000; i++ )	/* may take up to 1 second... */
	{
		if ( CMOS_READ( RTC_FREQ_SELECT ) & RTC_UIP )
		{
			break;
		}
	}

	for ( i = 0; i < 1000000; i++ )	/* must try at least 2.228 ms */
	{
		if ( !( CMOS_READ( RTC_FREQ_SELECT ) & RTC_UIP ) )
		{
			break;
		}
	}

	do
	{			/* Isn't this overkill ? UIP above should guarantee consistency */
		sec = CMOS_READ( RTC_SECONDS );
		min = CMOS_READ( RTC_MINUTES );
		hour = CMOS_READ( RTC_HOURS );
		day = CMOS_READ( RTC_DAY_OF_MONTH );
		mon = CMOS_READ( RTC_MONTH );
		year = CMOS_READ( RTC_YEAR );
	}
	while ( sec != CMOS_READ( RTC_SECONDS ) );

	if ( !( CMOS_READ( RTC_CONTROL ) & RTC_DM_BINARY ) || RTC_ALWAYS_BCD )
	{
		BCD_TO_BIN( sec );
		BCD_TO_BIN( min );
		BCD_TO_BIN( hour );
		BCD_TO_BIN( day );
		BCD_TO_BIN( mon );
		BCD_TO_BIN( year );
	}
	if ( ( year += 1900 ) < 1970 )
	{
		year += 100;
	}

	sTime.tm_sec = sec;
	sTime.tm_min = min;
	sTime.tm_hour = hour;
	sTime.tm_mday = day - 1;
	sTime.tm_mon = mon - 1;
	sTime.tm_year = year - 1900;

	sys_SetTime( &sTime );
	g_sSysBase.ex_nBootTime = g_sSysBase.ex_nRealTime;

	printk( "TIME : %02d-%02d-%04d %02d:%02d:%02d\n", day, mon, year, hour, min, sec );


	a = 10000000000LL;
	b = 10000000001LL;

	kassertw( a < b );
	kassertw( b > a );

	b--;

	kassertw( a == b );

	c = 100;

	a += c * 2;
	b += c;
	b += c;
	kassertw( a == b );

//	return ( ( uint32 )( g_sSysBase.ex_nRealTime / 1000000 ) );
}
