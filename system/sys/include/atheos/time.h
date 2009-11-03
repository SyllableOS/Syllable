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

#ifndef __F_ATHEOS_TIME_H_
#define __F_ATHEOS_TIME_H_

#include <syllable/types.h>

#ifdef __cplusplus
extern "C" {
#endif

bigtime_t get_system_time( void );
bigtime_t get_real_time( void );
bigtime_t get_idle_time( int nProcessor );

int set_real_time( bigtime_t nTime );

status_t snooze( bigtime_t nTimeout );

#ifdef __cplusplus
}
#endif

#endif	/* __F_ATHEOS_TIME_H_ */
