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

#ifndef __F_SYLLABLE_PTRACE_H__
#define __F_SYLLABLE_PTRACE_H__

#include <atheos/types.h>


#define EBX			0
#define ECX			1
#define EDX			2
#define ESI			3
#define EDI			4
#define EBP			5
#define EAX			6
#define DS			7
#define ES			8
#define FS			9
#define GS			10
#define ORIG_EAX	11
#define EIP			12
#define CS			13
#define EFL			14
#define UESP		15
#define SS			16

#define FRAME_SIZE	17	


typedef struct {
	uint32 cwd;
	uint32 swd;
	uint32 twd;
	uint32 fip;
	uint32 fcs;
	uint32 foo;
	uint32 fos;
	uint32 st_space[20];  /* 8 FP registers, 10 bytes each */
} elf_fpregset_t;

typedef struct {
	uint16 cwd;
	uint16 swd;
	uint16 twd;
	uint16 fop;
	uint32 fip;
	uint32 fcs;
	uint32 foo;
	uint32 fos;
	uint32 mxcsr;
	uint32 reserved;
	uint32 st_space[32];   /* 8 FP registers, 16 bytes each */
	uint32 xmm_space[32];  /* 8 XMM registers, 16 bytes each */
	uint32 padding[56];
} elf_fpxregset_t;


// keep in sync with SysCallRegs_s in atheos/kernel.h !

struct user_regs_struct {
	long ebx, ecx, edx, esi, edi, ebp, eax;
	unsigned short ds, __ds, es, __es;
	unsigned short fs, __fs, gs, __gs;
	long orig_eax, eip;
	unsigned short cs, __cs;
	long eflags, esp;
	unsigned short ss, __ss;
};

typedef uint32 elf_greg_t;

#define ELF_NGREG (sizeof(struct user_regs_struct) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];


struct user {
	struct user_regs_struct regs;
	uint32 u_debugreg[8];
};


#endif

