/*
 *  The Syllable kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef __F_KERNEL_STRACE_H__
#define __F_KERNEL_STRACE_H__

#include <syllable/strace.h>

typedef struct SyscallExc
{
	int nSyscall;
	struct SyscallExc *psPrev, *psNext;
} SyscallExc_s;

#endif	/* __F_KERNEL_STRACE_H__ */
