/*
 *  The AtheOS kernel
 *  Copyright (C) 2005 The Syllable Team
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

#ifndef _ATHEOS_SELECT_H_
#define _ATHEOS_SELECT_H_

#include <atheos/types.h>
#include <posix/types.h>
#include <posix/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FD_SETSIZE 1024

typedef struct fd_set {
  unsigned char fds_bits [((FD_SETSIZE) + 7) / 8];
} fd_set;

#define FD_SET(n, p)    ((p)->fds_bits[(n) / 8] |= (1 << ((n) & 7)))
#define FD_CLR(n, p)	((p)->fds_bits[(n) / 8] &= ~(1 << ((n) & 7)))
#define FD_ISSET(n, p)	((p)->fds_bits[(n) / 8] & (1 << ((n) & 7)))
#define FD_ZERO(p)	memset ((void *)(p), 0, sizeof (*(p)))


__SYSCALL( int, select( int nMaxCnt, fd_set *psReadSet, fd_set *psWriteSet, fd_set *psExceptSet, struct kernel_timeval *psTimeOut ) );


#ifdef __cplusplus
}
#endif


#endif	// _ATHEOS_SELECT_H_
