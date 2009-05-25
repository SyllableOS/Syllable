/*
 *  The Syllable kernel
 *  Copyright (C) 2009 Kristian Van Der Vliet
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

#ifndef	_POSIX_SIGINFO_H_
#define	_POSIX_SIGINFO_H_

#include <posix/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__

/* Type for data associated with a signal.  */
typedef union sigval
  {
    int sival_int;
    void *sival_ptr;
  } sigval_t;

#endif

/* Structure to transport application-defined values with signals.  */
# define SIGEV_MAX_SIZE	64
# define SIGEV_PAD_SIZE	((SIGEV_MAX_SIZE / sizeof (int)) - 3)

typedef struct sigevent
  {
    sigval_t sigev_value;
    int sigev_signo;
    int sigev_notify;
    void (*sigev_notify_function) (sigval_t);	    /* Function to start.  */
    void *sigev_notify_attributes;		    /* Really pthread_attr_t.*/
  } sigevent_t;

/* `sigev_notify' values.  */
enum
{
  SIGEV_SIGNAL = 0,		/* Notify via signal.  */
# define SIGEV_SIGNAL	SIGEV_SIGNAL
  SIGEV_NONE,			/* Other notification: meaningless.  */
# define SIGEV_NONE	SIGEV_NONE
  SIGEV_THREAD			/* Deliver via thread creation.  */
# define SIGEV_THREAD	SIGEV_THREAD
};

#ifdef __cplusplus
}
#endif

#endif	/*	_POSIX_SIGINFO_H_ */
