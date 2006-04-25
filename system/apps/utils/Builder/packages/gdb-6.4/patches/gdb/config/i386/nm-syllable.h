/* Native-dependent definitions for Syllable/i386.

   Copyright 2001, 2004 Free Software Foundation, Inc.

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

#ifndef NM_SYLLABLE_H
#define NM_SYLLABLE_H

/* Shared library support.  */
#include "solib.h"

/* Override copies of {fetch,store}_inferior_registers in `infptrace.c'.  */
#define FETCH_INFERIOR_REGISTERS

/* Support for the user struct.  */
#define KERNEL_U_SIZE kernel_u_size ()
extern int kernel_u_size (void);


/* Provide access to the i386 hardware debugging registers.  */

#define I386_USE_GENERIC_WATCHPOINTS
#include "i386/nm-i386.h"
                                                                                                                                                                                                        
#define I386_DR_LOW_SET_CONTROL(control) \
  i386syl_dr_set_control (control)
extern void i386syl_dr_set_control (unsigned long control);
                                                                                                                                                                                                        
#define I386_DR_LOW_SET_ADDR(regnum, addr) \
  i386syl_dr_set_addr (regnum, addr)
extern void i386syl_dr_set_addr (int regnum, CORE_ADDR addr);
                                                                                                                                                                                                        
#define I386_DR_LOW_RESET_ADDR(regnum) \
  i386syl_dr_reset_addr (regnum)
extern void i386syl_dr_reset_addr (int regnum);
                                                                                                                                                                                                        
#define I386_DR_LOW_GET_STATUS() \
  i386syl_dr_get_status ()
extern unsigned long i386syl_dr_get_status (void);


#endif /* nm-syllable.h */

