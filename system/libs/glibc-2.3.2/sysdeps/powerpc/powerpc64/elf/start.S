/* Startup code for programs linked with GNU libc.  PowerPC64 version.
   Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <sysdep.h>
#include "bp-sym.h"

 /* These are the various addresses we require.  */
	.section ".rodata"
	.align	3
L(start_addresses):
	.quad	0 /* was _SDA_BASE_  but not in 64-bit ABI*/
/*     function descriptors so don't need JUMPTARGET */
	.quad	BP_SYM(main)
	.quad 	__libc_csu_init
	.quad 	__libc_csu_fini

	ASM_SIZE_DIRECTIVE(L(start_addresses))

	.section	".toc","aw"
.L01:
	.tc	L(start_addresses)[TC],L(start_addresses)
	.section ".text"
ENTRY(_start)
 /* Save the stack pointer, in case we're statically linked under Linux.  */
	mr	r9,r1
 /* Set up an initial stack frame, and clear the LR.  */
	clrrdi	r1,r1,4
	li	r0,0
	stdu	r1,-128(r1)
	mtlr	r0
	std	r0,0(r1)

 /* put the address of start_addresses in r8...  **
** PPC64 ABI uses R13 for thread local, so we leave it alone */
	ld	r8,.L01(r2)

 /* and continue in libc-start, in glibc.  */
	b	JUMPTARGET(BP_SYM(__libc_start_main))

END(_start)

/* Define a symbol for the first piece of initialized data.  */
	.section ".data"
	.globl	__data_start
__data_start:
weak_alias (__data_start, data_start)
