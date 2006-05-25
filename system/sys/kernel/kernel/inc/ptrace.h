/*
 *  The Syllable kernel
 *  Copyright (C) 2004 Kristian Van Der Vliet
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

#ifndef __F_PTRACE_H__
#define __F_PTRACE_H__

#include "inc/scheduler.h"


#define MAX_REQUEST			24

#define PTRACE_TRACEME		0
#define PTRACE_PEEKTEXT		1
#define PTRACE_PEEKDATA		2
#define PTRACE_PEEKUSER		3
#define PTRACE_POKETEXT		4
#define PTRACE_POKEDATA		5
#define PTRACE_POKEUSER		6
#define PTRACE_CONT			7
#define PTRACE_KILL			8
#define PTRACE_SINGLESTEP	9
#define PTRACE_GETREGS		12
#define PTRACE_SETREGS		13
#define PTRACE_GETFPREGS	14
#define PTRACE_SETFPREGS	15
#define PTRACE_ATTACH		16
#define PTRACE_DETACH		17
#define PTRACE_GETFPXREGS	18
#define PTRACE_SETFPXREGS	19
#define PTRACE_SYSCALL		24

#define PT_PTRACED			0x00000001
#define PT_ALLOW_SIGNAL		0x00000002

// allows the following flags to be modified:
// CF, PF, AF, ZF, SF, TF, DF, OF, NT, AC
#define EFLAG_MASK			0x00044dd5

// masks out reserved bits and GD
#define DR7_MASK			0xffff07ff

#if !defined (offsetof)
#  define offsetof(TYPE, MEMBER) ((unsigned long) &((TYPE *) 0)->MEMBER)
#endif


typedef int (ptrace_func_t)( Thread_s *, Thread_s *, void *, void * );
typedef ptrace_func_t *ptrace_func_ptr;

int sys_ptrace( int nRequest, thread_id hThread, void *pAddr, void *pData );

#endif	/* __F_PTRACE_H__ */

