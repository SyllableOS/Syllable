/*
 *  The Syllable kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
 *	Copyright (C) 2004 The Syllable Team
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

#ifndef __VIRT86_H__
#define __VIRT86_H__


typedef struct
{

      /*** normal regs, with special meaning for the segment descriptors.. ***/

	long ebx;
	long ecx;
	long edx;
	long esi;
	long edi;
	long ebp;
	long eax;
	long __null_ds;
	long __null_es;
	long __null_fs;
	long __null_gs;
	long orig_eax;
	long eip;
	uint16 cs, __csh;
	long eflags;
	long esp;
	uint16 ss, __ssh;

      /*** These are part of the v86 interrupt stackframe: ***/
	uint16 es, __esh;
	uint16 ds, __dsh;
	uint16 fs, __fsh;
	uint16 gs, __gsh;
} Virtual86Regs_s;

typedef struct
{
	Virtual86Regs_s regs;
	unsigned long flags;
	unsigned long screen_bitmap;
	unsigned long cpu_type;
} Virtual86Struct_s;

typedef struct kernel_vm86_struct
{
	Virtual86Regs_s regs;

/*
 * the below part remains on the kernel stack while we are in VM86 mode.
 * 'tss.esp0' then contains the address of VM86_TSS_ESP0 below, and when we
 * get forced back from VM86, the CPU and "SAVE_ALL" will restore the above
 * 'struct kernel_vm86_regs' with the then actual values.
 * Therefore, pt_regs in fact points to a complete 'kernel_vm86_struct'
 * in kernelspace, hence we need not reget the data from userspace.
 */
#define VM86_TSS_ESP0 flags
	unsigned long flags;
	unsigned long screen_bitmap;
	unsigned long cpu_type;

	SysCallRegs_s *regs32;	/* here we save the pointer to the old regs */

	Virtual86Regs_s *psRegs16;
	void *pSavedStack;

	struct kernel_vm86_struct *psNext;

} Virtual86State_s;

#endif

