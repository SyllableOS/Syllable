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

#ifndef __F_KERNEL_SYSCALL_H__
#define __F_KERNEL_SYSCALL_H__

#include <kernel/types.h>

/* This struct defines the way the registers are stored on the
   stack during a system call. */

typedef struct
{
    long 	ebx;
    long 	ecx;
    long 	edx;
    long 	esi;
    long	edi;
    long 	ebp;
    long 	eax;
    uint16	ds, __dsu;
    uint16	es, __esu;
    uint16	fs, __fsu;
    uint16	gs, __gsu;
    long 	orig_eax;
    long 	eip;
    uint16	cs, __csu;
    long 	eflags;
    long 	oldesp;
    uint16	oldss, __ssu;
} SysCallRegs_s;

void print_registers( SysCallRegs_s* psRegs );

int exit_from_sys_call( void );

#include <syllable/syscall.h>

#endif	/* __F_KERNEL_SYSCALL_H__ */
