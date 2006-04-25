/* Target-dependent code for Syllable/i386.

   Copyright 1988, 1989, 1991, 1992, 1994, 1996, 2000, 2001, 2002,
   2003, 2004
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "arch-utils.h"
#include "frame.h"
#include "gdbcore.h"
#include "regcache.h"
#include "regset.h"
#include "symtab.h"
#include "objfiles.h"
#include "osabi.h"
#include "target.h"

#include "gdb_assert.h"
#include "gdb_string.h"

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "solib-svr4.h"

#include <atheos/ptrace.h>


/* Recognizing signal handler frames.  */

/* The instruction sequence for normal signals is
       pop    %eax
       mov    $0x5b, %eax
       int    $0x80
   or 0x58 0xb8 0x5b 0x00 0x00 0x00 0xcd 0x80.

   Checking for the code sequence should be somewhat reliable, because
   the effect is to call the system call sig_return.  This is unlikely
   to occur anywhere other than in a signal trampoline.

   It kind of sucks that we have to read memory from the process in
   order to identify a signal trampoline, but there doesn't seem to be
   any other way.
*/

#define SYL_SIGTRAMP_INSN0    0x58  /* pop %eax */
#define SYL_SIGTRAMP_OFFSET0  0
#define SYL_SIGTRAMP_INSN1    0xb8  /* mov $NNNN, %eax */
#define SYL_SIGTRAMP_OFFSET1  1
#define SYL_SIGTRAMP_INSN2    0xcd  /* int */
#define SYL_SIGTRAMP_OFFSET2  6

static const unsigned char syllable_sigtramp_code[] =
{
  SYL_SIGTRAMP_INSN0,                          /* pop %eax */
  SYL_SIGTRAMP_INSN1, 0x5b, 0x00, 0x00, 0x00,  /* mov $0x5b, %eax */
  SYL_SIGTRAMP_INSN2, 0x80                     /* int $0x80 */
};

#define SYL_SIGTRAMP_LEN (sizeof syllable_sigtramp_code)


/* If NEXT_FRAME unwinds into a sigtramp routine, return the address
   of the start of the routine.  Otherwise, return 0.  */

static CORE_ADDR
i386syl_sigtramp_start (struct frame_info *next_frame)
{
  CORE_ADDR pc = frame_pc_unwind (next_frame);
  unsigned char buf[SYL_SIGTRAMP_LEN];

  /* We only recognize a signal trampoline if PC is at the start of
     one of the three instructions.  We optimize for finding the PC at
     the start, as will be the case when the trampoline is not the
     first frame on the stack.  We assume that in the case where the
     PC is not at the start of the instruction sequence, there will be
     a few trailing readable bytes on the stack.  */

  if (!safe_frame_unwind_memory (next_frame, pc, buf, SYL_SIGTRAMP_LEN))
    return 0;

  if (buf[0] != SYL_SIGTRAMP_INSN0)
  {
    int adjust;

    switch (buf[0])
    {
      case SYL_SIGTRAMP_INSN1:
        adjust = SYL_SIGTRAMP_OFFSET1;
        break;
      case SYL_SIGTRAMP_INSN2:
        adjust = SYL_SIGTRAMP_OFFSET2;
        break;
      default:
        return 0;
    }

    pc -= adjust;

    if (!safe_frame_unwind_memory (next_frame, pc, buf, SYL_SIGTRAMP_LEN))
      return 0;
  }

  if (memcmp (buf, syllable_sigtramp_code, SYL_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}


/* Return whether the frame preceding NEXT_FRAME corresponds to a
   Syllable sigtramp routine.  */

static int
i386syl_sigtramp_p (struct frame_info *next_frame)
{
  return (i386syl_sigtramp_start (next_frame) != 0);
}


/* Assuming NEXT_FRAME is a frame following a Syllable sigtramp
   routine, return the address of the associated sigcontext structure.  */

static CORE_ADDR
i386syl_sigcontext_addr (struct frame_info *next_frame)
{
  CORE_ADDR pc;
  CORE_ADDR sp;
  char buf[4];

  frame_unwind_register (next_frame, I386_ESP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 4);

  pc = i386syl_sigtramp_start (next_frame);
  if (pc)
  {
    /* The sigcontext structure lives on the stack, right after
       the signum argument.  We determine the address of the
       sigcontext structure by looking at the frame's stack
       pointer.  Keep in mind that the first instruction of the
       sigtramp code is "pop %eax".  If the PC is after this
       instruction, adjust the returned value accordingly.  */
    if (pc == frame_pc_unwind (next_frame))
      return sp + 4;
    return sp;
  }

  error ("Couldn't recognize signal trampoline.");
  return 0;
}


/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register cache layout.  */

static int i386syl_gregset_reg_offset[] =
{
  EAX * 4,                     /* %eax */
  ECX * 4,                     /* %ecx */
  EDX * 4,                     /* %edx */
  EBX * 4,                     /* %ebx */
  UESP * 4,                    /* %esp */
  EBP * 4,                     /* %ebp */
  ESI * 4,                     /* %esi */
  EDI * 4,                     /* %edi */
  EIP * 4,                     /* %eip */
  EFL * 4,                     /* %eflags */
  CS * 4,                      /* %cs */
  SS * 4,                      /* %ss */
  DS * 4,                      /* %ds */
  ES * 4,                      /* %es */
  FS * 4,                      /* %fs */
  GS * 4,                      /* %gs */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1
};

/* Mapping between the general-purpose registers in `struct
   sigcontext' format and GDB's register cache layout.  */

/* From <atheos/sigcontext.h>.  */
static int i386syl_sc_reg_offset[] =
{
  11 * 4,  /* %eax */
  10 * 4,  /* %ecx */
  9 * 4,   /* %edx */
  8 * 4,   /* %ebx */
  7 * 4,   /* %esp */
  6 * 4,   /* %ebp */
  5 * 4,   /* %esi */
  4 * 4,   /* %edi */
  14 * 4,  /* %eip */
  16 * 4,  /* %eflags */
  15 * 4,  /* %cs */
  18 * 4,  /* %ss */
  3 * 4,   /* %ds */
  2 * 4,   /* %es */
  1 * 4,   /* %fs */
  0 * 4    /* %gs */
};


/* Syllable/i386 ELF.  */

static void
i386syl_elf_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  tdep->gregset_reg_offset = i386syl_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386syl_gregset_reg_offset);
  tdep->sizeof_gregset = 16 * 4;

  tdep->jb_pc_offset = 20;     /* From <bits/setjmp.h>.  */

  tdep->sigtramp_p = i386syl_sigtramp_p;
  tdep->sigcontext_addr = i386syl_sigcontext_addr;
  tdep->sc_reg_offset = i386syl_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (i386syl_sc_reg_offset);

  /* ELF-based */
  i386_elf_init_abi (info, gdbarch);

  /* Syllable ELF uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386syllable_tdep (void);

void
_initialize_i386syllable_tdep (void)
{
  /* FIXME: Since Syllable/i386 EFL binaries are
     indistingushable from NetBSD/i386 ELF binaries, building a GDB
     that should support both these targets will probably not work as
     expected.  */
#define GDB_OSABI_SYLLABLE GDB_OSABI_NETBSD_ELF

  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_SYLLABLE,
                          i386syl_elf_init_abi);
}

