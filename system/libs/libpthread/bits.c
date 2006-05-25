/* POSIX threads for Syllable - A simple PThreads implementation             */
/*                                                                           */
/* Copyright (C) 2002 Kristian Van Der Vliet (vanders@users.sourceforge.net) */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU Library General Public License       */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU Library General Public License for more details.                      */

#ifndef __F_SYLLABLE_PTHREAD_BITS_
#define __F_SYLLABLE_PTHREAD_BITS_

#ifdef __cplusplus
extern "C"{
#endif

#include "inc/bits.h"

inline status_t __pt_lock_mutex( sem_id mutex )
{
	return( lock_semaphore_x( mutex, 1, 0, INFINITE_TIMEOUT ) );
}

inline status_t __pt_unlock_mutex( sem_id mutex )
{
	return( unlock_semaphore_x( mutex, 1, 0 ) );
}


#ifdef __cplusplus
}
#endif

#endif	/* __F_SYLLABLE_PTHREAD_BITS_ */



