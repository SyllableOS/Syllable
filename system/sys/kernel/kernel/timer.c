
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

//#define INT_FREQ 1000		// moved to "inc/pit_timer.h"

int g_bNeedSchedule;		/* If true the scheduler will be called when returning from syscall     */

SEQ_LOCK( g_sTimerSeqLock, "timer_slock" );


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bigtime_t get_system_time( void )
{
	bigtime_t nTime;
	uint32 nSeq;

	do
	{
		nSeq = read_seqbegin( &g_sTimerSeqLock );
		nTime = g_sSysBase.ex_nRealTime - g_sSysBase.ex_nBootTime;
	}
	while ( read_seqretry( &g_sTimerSeqLock, nSeq ) );

	return ( nTime );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bigtime_t get_real_time( void )
{
	bigtime_t nTime;
	uint32 nSeq;

	do
	{
		nSeq = read_seqbegin( &g_sTimerSeqLock );
		nTime = g_sSysBase.ex_nRealTime;
	}
	while ( read_seqretry( &g_sTimerSeqLock, nSeq ) );

	return ( nTime );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bigtime_t get_idle_time( int nProcessor )
{
	if ( nProcessor < 0 || nProcessor >= g_nActiveCPUCount )
	{
		return ( 0 );
	}
	// this needs to be locked...
	return ( g_asProcessorDescs[logical_to_physical_cpu_id( nProcessor )].pi_nIdleTime );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

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

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

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

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

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

int sys_set_real_time( bigtime_t nTime )
{
	uint32 nFlags;

	nFlags = write_seqlock_disable( &g_sTimerSeqLock );
	g_sSysBase.ex_nBootTime += nTime - g_sSysBase.ex_nRealTime;
	g_sSysBase.ex_nRealTime = nTime;
	write_sequnlock_enable( &g_sTimerSeqLock, nFlags );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void starttimer1( void )
{
	int speed = PIT_TICKS_PER_SEC / INT_FREQ;

	/*	outb_p( 0x30, PIT_MODE );	*//*      One shot mode   */
	outb_p( 0x34, PIT_MODE );	/* Loop mode    */
	outb_p( speed & 0xff, PIT_CH0 );
	outb_p( speed >> 8, PIT_CH0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void starttimer2( void )
{
	outb_p( 0xB4, PIT_MODE );
	outb_p( 0, PIT_CH2 );
	outb_p( 0, PIT_CH2 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if 0
static uint16 readtimer1( void )
{
	uint16 n;

	outb_p( 0, PIT_MODE );
	n = inb_p( PIT_CH0 );
	n += inb_p( PIT_CH0 ) << 8;
	return 0x10000L - n;
}
#endif

/******************************************************************************
		read counter value of PIT timer 2, and update g_sSysBase.ex_lTicksElapsed.
		This function must be called 18.2 times or more per second
 ******************************************************************************/
#if 0
uint32 readtimer2( void )
{

/*
	uint16 n;
	static	uint16	Last=0;

	outb_p( 0x80, PIT_MODE );
	n = inb_p( PIT_CH2 );
	n += inb( PIT_CH2 ) << 8;

	n = 0x10000L - n;

	g_sSysBase.ex_lTicksElapsed += ( n < Last ) ? (0x10000 - Last + n) : (n - Last);

	Last = n;
*/
	return ( g_sSysBase.ex_lTicksElapsed );
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void TimerInterrupt( int dummy )
{
	bigtime_t nCurTime;
	uint32 nFlg;

	nFlg = cli();

	if ( get_processor_id() != g_nBootCPU )
	{
		printk( "Got timer IRQ to CPU %d (Booted on %d)\n", get_processor_id(), g_nBootCPU );
		put_cpu_flags( nFlg );
		return;
	}

	outb( 0x20, PIC_MASTER_CMD );	/* Give handshake to interupt controller        */
	
	put_cpu_flags( nFlg );

	nFlg = write_seqlock_disable( &g_sTimerSeqLock );
	g_sSysBase.ex_nRealTime += ( uint64 )( 1000000 / INT_FREQ );
	nCurTime = g_sSysBase.ex_nRealTime - g_sSysBase.ex_nBootTime;
	write_sequnlock_enable( &g_sTimerSeqLock, nFlg );

	// TODO: move this to a separate thread
	send_alarm_signals( nCurTime );
	wake_up_sleepers( nCurTime );
	if ( g_bAPICPresent == false )
	{
		Schedule();
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
void start_timer_int( void )
{
	outb_p( inb_p( 0x61 ) | 1, 0x61 );

	starttimer1();
	starttimer2();

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void stop_timer_int( void )
{
	outb_p( 0x34, PIT_MODE );
	outb_p( 0, PIT_CH0 );
	outb_p( 0, PIT_CH0 );
}


/* Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 * Assumes input in normal date format, i.e. 1980-12-31 23:59:59
 * => year=1980, mon=12, day=31, hour=23, min=59, sec=59.
 *
 * [For the Julian calendar (which was used in Russia before 1917,
 * Britain & colonies before 1752, anywhere else before 1582,
 * and is still in use by some communities) leave out the
 * -year/100+year/400 terms, and add 10.]
 *
 * This algorithm was first published by Gauss (I think).
 *
 * WARNING: this function will overflow on 2106-02-07 06:28:16 on
 * machines were long is 32-bit! (However, as time_t is signed, we
 * will already get problems at other places on 2038-01-19 03:14:08)
 */



static inline uint32 maketime( uint32 year, uint32 mon, uint32 day, uint32 hour, uint32 min, uint32 sec )
{
	if ( 0 >= ( int )( mon -= 2 ) )	/* 1..12 -> 11,12,1..10 */
	{
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}
	return ( ( ( ( uint32 )( year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day ) + year * 365 - 719499 ) * 24 + hour	/* now have hours */
		 ) * 60 + min	/* now have minutes */
		 ) * 60 + sec;	/* finally seconds */
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 get_cmos_time( void )
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

	printk( "TIME : %02ld-%02ld-%04ld %02ld:%02ld:%02ld\n", day, mon, year, hour, min, sec );


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

	return ( ( uint32 )( g_sSysBase.ex_nRealTime / 1000000 ) );
}
