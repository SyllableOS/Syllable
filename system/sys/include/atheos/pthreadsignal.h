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

#if !defined _POSIX_SIGNALS_H_ && !defined __F_SYLLABLE_PTHREAD_H_
	#error "Never include <atheos/pthreadsignal.h> directly; use <signal.h> instead."
#endif

#ifndef __F_SYLLABLE_PTHREADSIGNAL_H_
#define __F_SYLLABLE_PTHREADSIGNAL_H_

#ifdef __cplusplus
extern "C"{
#endif

int pthread_sigmask(int, const sigset_t *, sigset_t *);

#ifdef __cplusplus
}
#endif

#endif	/* __F_SYLLABLE_PTHREADSIGNAL_H_ */

