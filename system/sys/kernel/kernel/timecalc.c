
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
#include <atheos/time.h>


#include <macros.h>

#include "inc/sysbase.h"

#define	INT(a)	((double)((int)(a)))

#define	JD_1970	2440587.5	/* Julian day 1.1 1970  */

double ClockToJulianDay( ClockTime_s * psTime )
{
	double A;
	double B;
	double vYear;
	double vMonth;
	double vDay;
	double jd;

	if ( psTime->tm_mon > 2 )
	{
		vYear = ( double )( psTime->tm_year + 1900 );
		vMonth = ( double )( psTime->tm_mon + 1 );
	}
	else
	{
		vYear = ( double )( psTime->tm_year - 1 + 1900 );
		vMonth = ( double )( psTime->tm_mon + 12 + 1 );
	}

	A = INT( vYear / 100.0 );
	B = 2.0 - A + INT( A / 4.0 );


	vDay = ( double )( psTime->tm_mday + 1 );	/*      First month should be 1, so we must increase the value in psTime with 1 */
	vDay += ( double )psTime->tm_hour / 24.0;
	vDay += ( double )psTime->tm_min / ( 24.0 * 60.0 );
	vDay += ( double )psTime->tm_sec / ( 24.0 * 60.0 * 60.0 );


	jd = INT( 365.25 * vYear ) + INT( 30.6001 * ( vMonth + 1.0 ) ) + vDay + 1720994.5 + B;

	return ( jd - JD_1970 );
}

void JulianDayToClock( double vJD, ClockTime_s * psTime )
{
	double Z;
	double F;
	double A;
	double AA;
	double B;
	double C;
	double D;
	double E;

	double vYear;
	double vMonth;
	double vDay;
	double vHour;
	double vMin;
	double vSec;

	vJD += JD_1970;

	Z = INT( vJD + 0.5 );
	F = ( vJD + 0.5 ) - Z;

	AA = INT( ( Z - 1867216.25 ) / 36524.25 );
	A = Z + 1.0 + AA - INT( AA / 4.0 );
	B = A + 1524.0;
	C = INT( ( B - 122.1 ) / 365.25 );
	D = INT( 365.25 * C );
	E = INT( ( B - D ) / 30.6001 );

	vDay = B - D - INT( 30.6001 * E ) + F;
	vMonth = ( E < 13.5 ) ? ( E - 1.0 ) : ( E - 13.0 );
	vYear = ( vMonth > 2.5 ) ? ( C - 4716.0 ) : ( C - 4715.0 );

	vHour = ( vDay - INT( vDay ) ) * 24.0;
	vMin = ( vHour - INT( vHour ) ) * 60.0;
	vSec = ( vMin - INT( vMin ) ) * 60.0;

	psTime->tm_sec = ( int )vSec;
	psTime->tm_min = ( int )vMin;
	psTime->tm_hour = ( int )vHour;
	psTime->tm_mday = ( ( int )vDay ) - 1;
	psTime->tm_mon = ( ( int )vMonth ) - 1;
	psTime->tm_year = ( ( int )vYear ) - 1900;
}

void SecToClock( uint32 lSecs, ClockTime_s * psTime )
{
	JulianDayToClock( ( ( double )lSecs ) / 86400.0, psTime );
}

/** Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 * Assumes input in normal date format, i.e. 1980-12-31 23:59:59
 * => year=1980, mon=12, day=31, hour=23, min=59, sec=59.
 *
 * \par [For the Julian calendar (which was used in Russia before 1917,
 * Britain & colonies before 1752, anywhere else before 1582,
 * and is still in use by some communities) leave out the
 * -year/100+year/400 terms, and add 10.]
 *
 * \par This algorithm was first published by Gauss (I think).
 *
 * \par WARNING: this function will overflow on 2106-02-07 06:28:16 on
 * machines were long is 32-bit! (However, as time_t is signed, we
 * will already get problems at other places on 2038-01-19 03:14:08)
 */
uint32 ClockToSec( ClockTime_s * psTime )
{
	uint32 year = psTime->tm_year + 1900;
	uint32 mon = psTime->tm_mon + 1;
	uint32 day = psTime->tm_mday + 1;
	uint32 hour = psTime->tm_hour;
	uint32 min = psTime->tm_min;
	uint32 sec = psTime->tm_sec;

	if ( 0 >= ( int )( mon -= 2 ) )	/* 1..12 -> 11,12,1..10 */
	{
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}

	return ( ( ( ( uint32 )( year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day ) + year * 365 - 719499 ) * 24 + hour	/* now have hours               */
		 ) * 60 + min	/* now have minutes     */
		 ) * 60 + sec;	/* finally seconds      */
}

uint32 sys_GetTime( ClockTime_s * psTime )
{
	if ( NULL != psTime )
	{
		SecToClock( ( uint32 )( g_sSysBase.ex_nRealTime / 1000000 ), psTime );
	}

/*	SecToClock( ClockToSec( psTime ), psTime );	*/
	return ( ( uint32 )( g_sSysBase.ex_nRealTime / 1000000 ) );
}

uint32 sys_SetTime( ClockTime_s * psTime )
{
	g_sSysBase.ex_nRealTime = ( ( uint64 )ClockToSec( psTime ) ) * 1000000;
	return ( 0 );
}
