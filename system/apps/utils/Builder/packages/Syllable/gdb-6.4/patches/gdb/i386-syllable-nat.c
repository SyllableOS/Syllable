/* Native-dependent code for Syllable/i386.

   Copyright 2002, 2003, 2004 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "regcache.h"
#include "target.h"
#include "inf-ptrace.h"
#include "gdb_assert.h"

#include "i386-tdep.h"
#include "i387-tdep.h"

#include <sys/ptrace.h>
#include <atheos/ptrace.h>


/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register array layout.  */
static int i386syl_r_reg_offset[] =
{
  EAX, ECX, EDX, EBX,
  UESP, EBP, ESI, EDI,
  EIP, EFL, CS, SS,
  DS, ES, FS, GS
};

/* Macro to determine if a register is fetched with PT_GETREGS.  */
#define GETREGS_SUPPLIES(regnum) \
  ((0 <= (regnum) && (regnum) <= ARRAY_SIZE (i386syl_r_reg_offset)))

/* Set to 1 if the kernel supports PT_GETFPXREGS.  Initialized to -1
   so that we try PT_GETFPXREGS the first time around.  */
static int have_ptrace_xmmregs = -1;



/* Supply the general-purpose registers in GREGS, to REGCACHE.  */
                                                                                                                                                                                                        
static void
i386syl_supply_gregset (struct regcache *regcache, const elf_greg_t *gregs)
{
  int i;
  for (i = 0; i < ARRAY_SIZE (i386syl_r_reg_offset); i++)
    regcache_raw_supply (regcache, i, gregs + i386syl_r_reg_offset[i]);
}


/* Collect register REGNUM from REGCACHE and store its contents in
   GREGS.  If REGNUM is -1, collect and store all appropriate
   registers.  */

static void
i386syl_collect_gregset (const struct regcache *regcache,
                         elf_greg_t *gregs, int regnum)
{
  int i;
  for (i = 0; i < ARRAY_SIZE (i386syl_r_reg_offset); i++)
    if (regnum == -1 || regnum == i)
      regcache_raw_collect (regcache, i, gregs + i386syl_r_reg_offset[i]);
}


/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers (including the floating point registers).  */

static void
i386syl_fetch_inferior_registers (int regnum)
{
  if (regnum == -1 || GETREGS_SUPPLIES (regnum))
  {
    elf_gregset_t regs;

    if (ptrace (PT_GETREGS, PIDGET (inferior_ptid), 0, (void *) &regs) == -1)
      perror_with_name ("Couldn't get registers");

    i386syl_supply_gregset (current_regcache, (void *) &regs);
    if (regnum != -1)
      return;
  }

  if (regnum == -1 || regnum >= I386_ST0_REGNUM)
  {
    elf_fpregset_t fpregs;
    elf_fpxregset_t xmmregs;

    if (have_ptrace_xmmregs != 0 &&
        ptrace(PT_GETFPXREGS, PIDGET (inferior_ptid), 0,
               (void *) &xmmregs) == 0)
    {
      have_ptrace_xmmregs = 1;
      i387_supply_fxsave (current_regcache, -1, (void *) &xmmregs);
    }
    else
    {
      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid), 0,
                  (void *) &fpregs) == -1)
        perror_with_name ("Couldn't get floating point status");

      i387_supply_fsave (current_regcache, -1, (void *) &fpregs);
    }
  }
}


/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers (including the floating point registers).  */

static void
i386syl_store_inferior_registers (int regnum)
{
  if (regnum == -1 || GETREGS_SUPPLIES (regnum))
  {
    elf_gregset_t regs;

    if (ptrace (PT_GETREGS, PIDGET (inferior_ptid), 0, (void *) &regs) == -1)
      perror_with_name ("Couldn't get registers");

    i386syl_collect_gregset (current_regcache, (void *) &regs, regnum);

    if (ptrace (PT_SETREGS, PIDGET (inferior_ptid), 0, (void *) &regs) == -1)
      perror_with_name ("Couldn't write registers");

    /* We must be careful with modifying the program counter.  If we
       just interrupted a system call, the kernel might try to restart
       it when we resume the inferior.  On restarting the system call,
       the kernel will try backing up the program counter even though it
       no longer points at the system call.  This typically results in a
       SIGSEGV or SIGILL.  We can prevent this by writing `-1' in the
       "orig_eax" pseudo-register.
                                                                                                                                                                                                        
       Note that "orig_eax" is saved when setting up a dummy call frame.
       This means that it is properly restored when that frame is
       popped, and that the interrupted system call will be restarted
       when we resume the inferior on return from a function call from
       within GDB.  In all other cases the system call will not be
       restarted.  */
    if ((regnum == -1) || (regnum == I386_EIP_REGNUM)) 
      if (ptrace (PT_WRITE_U, PIDGET (inferior_ptid),
                  (PTRACE_TYPE_ARG3) (ORIG_EAX * sizeof(elf_greg_t)), -1) == -1)
        perror_with_name ("Couldn't write registers");

    if (regnum != -1)
      return;
  }

  if (regnum == -1 || regnum >= I386_ST0_REGNUM)
  {
    elf_fpregset_t fpregs;
    elf_fpxregset_t xmmregs;

    if (have_ptrace_xmmregs != 0 &&
        ptrace(PT_GETFPXREGS, PIDGET (inferior_ptid), 0,
               (void *) &xmmregs) == 0)
    {
      have_ptrace_xmmregs = 1;

      i387_collect_fxsave (current_regcache, regnum, (void *) &xmmregs);

      if (ptrace (PT_SETFPXREGS, PIDGET (inferior_ptid), 0,
                  (void *) &xmmregs) == -1)
        perror_with_name ("Couldn't write XMM registers");
    }
    else
    {
      have_ptrace_xmmregs = 0;
      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid), 0,
                  (void *) &fpregs) == -1)
        perror_with_name ("Couldn't get floating point status");

      i387_collect_fsave (current_regcache, regnum, (void *) &fpregs);

      if (ptrace (PT_SETFPREGS, PIDGET (inferior_ptid), 0,
                  (void *) &fpregs) == -1)
        perror_with_name ("Couldn't write floating point status");
    }
  }
}


/* Support for debug registers.  */

#define DR_FIRSTADDR 0
#define DR_LASTADDR  3
#define DR_STATUS    6
#define DR_CONTROL   7

static void
i386syl_dr_set (int regnum, unsigned int value)
{
  if (ptrace (PT_WRITE_U, PIDGET (inferior_ptid),
              offsetof (struct user, u_debugreg[regnum]), value) == -1)
    perror_with_name ("Couldn't write debug register");
}

void
i386syl_dr_set_control (unsigned long control)
{
  i386syl_dr_set (DR_CONTROL, control);
}

void
i386syl_dr_set_addr (int regnum, CORE_ADDR addr)
{
  gdb_assert (regnum >= 0 && regnum <= DR_LASTADDR - DR_FIRSTADDR);
  i386syl_dr_set (DR_FIRSTADDR + regnum, addr);
}

void
i386syl_dr_reset_addr (int regnum)
{
  gdb_assert (regnum >= 0 && regnum <= DR_LASTADDR - DR_FIRSTADDR);
  i386syl_dr_set (DR_FIRSTADDR + regnum, 0L);
}

unsigned long
i386syl_dr_get_status (void)
{
  unsigned long value;

  /* FIXME: kettenis/2001-03-27: Calling perror_with_name if the
     ptrace call fails breaks debugging remote targets.  The correct
     way to fix this is to add the hardware breakpoint and watchpoint
     stuff to the target vector.  For now, just return zero if the
     ptrace call fails.  */
  errno = 0;
  value = ptrace (PT_READ_U, PIDGET (inferior_ptid),
                  offsetof (struct user, u_debugreg[DR_STATUS]), 0);
  if (errno != 0)
#if 0
    perror_with_name ("Couldn't read debug register");
#else
    return 0;
#endif

  return value;
}


/* Support for the user struct.  */

/* Return the address register REGNUM.  BLOCKEND is the value of
   u.u_ar0, which should point to the registers.  */

CORE_ADDR
register_u_addr (CORE_ADDR blockend, int regnum)
{
  gdb_assert (regnum >= 0 && regnum < ARRAY_SIZE (i386syl_r_reg_offset));
  return blockend + (i386syl_r_reg_offset[regnum] * sizeof(elf_greg_t));
}


/* Return the size of the user struct.  */

int
kernel_u_size (void)
{
  return (sizeof (struct user));
}



/* Prevent warning from -Wmissing-prototypes.  */
void _initialize_i386syllable_nat (void);

void
_initialize_i386syllable_nat (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  t->to_fetch_registers = i386syl_fetch_inferior_registers;
  t->to_store_registers = i386syl_store_inferior_registers;

  add_target (t);
}

