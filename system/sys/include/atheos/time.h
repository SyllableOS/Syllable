/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#ifndef __ATHEOS_TIME_H_
#define __ATHEOS_TIME_H_

#include <atheos/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int		tm_sec;         /* seconds after the minute [0-60] */
	int		tm_min;         /* minutes after the hour [0-59] */
	int		tm_hour;        /* hours since midnight [0-23] */
	int		tm_mday;        /* day of the month [1-31] */
	int		tm_mon;         /* months since January [0-11] */
	int		tm_year;        /* years since 1900 */
	int		tm_wday;        /* days since Sunday [0-6] */
	int		tm_yday;        /* days since January 1 [0-365] */
	int		tm_isdst;       /* Daylight Savings Time flag */
	long	tm_gmtoff;      /* offset from CUT in seconds */
	char*	tm_zone;				/* timezone abbreviation */
}	ClockTime_s;

uint32	sys_GetTime( ClockTime_s*	psTime );
uint32	sys_SetTime( ClockTime_s*	psTime );

bigtime_t get_system_time( void );
bigtime_t get_real_time( void );
bigtime_t get_idle_time( int nProcessor );

int set_real_time( bigtime_t nTime );


int	sys_get_raw_system_time( bigtime_t* pRes );
int	sys_get_raw_real_time( bigtime_t* pRes );
int	sys_get_raw_idle_time( bigtime_t* pRes, int nProcessor );

#ifdef __KERNEL__
uint32	ClockToSec( ClockTime_s* psTime );
void	SecToClock( uint32 lSecs, ClockTime_s* psTime );
#endif
  
#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_TIME_H_ */
