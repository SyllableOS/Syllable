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

#ifndef __ATHEOS_WAIT_H_
#define __ATHEOS_WAIT_H_

#include <posix/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <posix/types.h>

#define WEXITSTATUS(stat_val)	( (stat_val>>8) & 0x000ff )
#define WIFEXITED(stat_val)	( ((stat_val) & 0xff) == 0)
#define WIFSIGNALED(stat_val)	( ((stat_val) & 0xff) > 0 && (((stat_val) >> 8) & 0xff) == 0 )
#define	_WSTOPPED 0x7f
#define WIFSTOPPED(stat_val)	( (((stat_val) & 0xff) == _WSTOPPED) && (((stat_val)>>8) & 0xff) != 0 )
#define WSTOPSIG(stat_val)	( (stat_val>>8) & 0xff )
#define WTERMSIG(stat_val)	( (stat_val) & 0x7f )

#define WNOHANG		1
#define WUNTRACED	2

pid_t wait( int* stat_loc );
pid_t waitpid( pid_t pid, int* stat_loc, int options );

pid_t wait4( pid_t pid, int* stat_loc, int options, struct rusage* rusage );

#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_WAIT_H_ */
