
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

#ifndef __F_ATHEOS_PIT_TIMER_H__
#define __F_ATHEOS_PIT_TIMER_H__

#define	PIT_TICKS_PER_SEC	1193182

#define	INT_FREQ		1000

/* Countdown value to load into PIT timer, rounded to nearest integer. */
static const uint32_t LATCH = ( ( PIT_TICKS_PER_SEC + INT_FREQ/2 ) / INT_FREQ );

void start_timer_int( void );
void get_cmos_time( void );

#endif /* __F_ATHEOS_PIT_TIMER_H__ */
