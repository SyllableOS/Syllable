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

#ifndef __F_SYLLABLE_PTHREAD_BITS_H_
#define __F_SYLLABLE_PTHREAD_BITS_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <time.h>

#include <atheos/semaphore.h>

inline status_t __pt_lock_mutex( sem_id mutex );
inline status_t __pt_unlock_mutex( sem_id mutex );

typedef struct __pt_timer_args_s
{
	struct timespec *abstime;
	pthread_t thread;
	pthread_cond_t* cond;
	int error;
} __pt_timer_args;

typedef struct __pt_cleanup_s
{
	void (*routine)(void*);
	void *arg;
	struct __pt_cleanup_s *prev;
} __pt_cleanup;


#define PT_TID(x)	(x)->id
#define PT_ATTR(x)	(x)->attr

#ifdef __cplusplus
}
#endif

#endif	/* __F_SYLLABLE_PTHREAD_BITS_H_ */









