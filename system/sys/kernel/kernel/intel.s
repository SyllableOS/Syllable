/*
 *  The AtheOS kernel
 *  Copyright (C) 1999  Kurt Skauen
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

	
	.text
	
#define _C_SYM( sym ) sym
	
	.global	_start
	.global _C_SYM( g_anKernelStackEnd )

	.global	_C_SYM( irq0 )

	.global	_C_SYM( SwitchCont )

	.global	_C_SYM( load_debug_regs );
	.global	_C_SYM( clear_debug_regs );
	.global	_C_SYM( read_debug_status );


	.global	_C_SYM( put_cpu_flags )
	.global	_C_SYM( get_cpu_flags )
	
	.global _C_SYM( cli )
	.global	_C_SYM( sti )

	.global	_C_SYM( v86Stack_seg )
	.global	_C_SYM( v86Stack_off )

	

_start:
#	fninit                  # we don't need the fpu yet.
	cmpl	$0x2BADB002,%eax
	je	multiboot
# non multiboot code removed.  We now require GRUB to boot.
1:      hlt
        jmp 1b
multiboot:
	movl	%cr0,%eax
	andb	$0xfb,%al	# clear EM flag
	orb	$0x22,%al	# set NE and MP flags
	movl	%eax,%cr0
	movl	$g_anKernelStackEnd,%esp

	pushl	%ebx
	call	_C_SYM( init_kernel_mb )

#	MultiBoot header
.align 4
.long 0x1BADB002, 0x0003, -(0x1BADB002+0x0003)

_C_SYM( v86Stack_seg ):
.long	0
_C_SYM( v86Stack_off ):
.long	0xfff0


# Exception handlers moved to syscall.s.


_C_SYM( irq0 ):                             # Do IRQs
	pushl 	%eax
	movb	$0x34,%al	# loop
#	movb	$0x30,%al	# One shot
	outb	%al,$0x43
	movb	$0x0f,%al
	outb	%al,$0x40
	movb	$0,%al
	outb	%al,$0x40


	movb	$0x20,%al
	outb	%al,$0x20

	popl	%eax
	iret

_C_SYM( load_debug_regs ):
	.type	 load_debug_regs, @function
	pushl	%ebp
	movl	8(%esp), %ebp
	movl	(%ebp), %eax
	movl	%eax, %db0
	movl	4(%ebp), %eax
	movl	%eax, %db1
	movl	8(%ebp), %eax
	movl	%eax, %db2
	movl	12(%ebp), %eax
	movl	%eax, %db3
	movl	28(%ebp), %eax
	movl	%eax, %db7
	popl	%ebp
	ret
.Lfe_load_debug_regs:
	.size	 load_debug_regs,.Lfe_load_debug_regs - load_debug_regs

_C_SYM( clear_debug_regs ):
	.type	 clear_debug_regs, @function
	pushl	%eax
	xorl	%eax, %eax
	movl	%eax, %db7
	popl	%eax
	ret
.Lfe_clear_debug_regs:
	.size	 clear_debug_regs,.Lfe_clear_debug_regs - clear_debug_regs

_C_SYM( read_debug_status ):
	.type	 read_debug_status, @function
	movl	%db6, %eax
	pushl	%eax
	xorl	%eax, %eax
	movl	%eax, %db6
	popl	%eax
	ret
.Lfe_read_debug_status:
	.size	 read_debug_status,.Lfe_read_debug_status - read_debug_status

_C_SYM( get_cpu_flags ):
	.type	 get_cpu_flags,@function
	pushfl
	popl	%eax
	ret
.Lfe_get_cpu_flags:
	.size	 get_cpu_flags,.Lfe_get_cpu_flags - get_cpu_flags

_C_SYM( put_cpu_flags ):
	.type	 put_cpu_flags,@function
	pushl	4(%esp)
	popfl
	ret
.Lfe_put_cpu_flags:
	.size	 put_cpu_flags,.Lfe_put_cpu_flags - put_cpu_flags
	
_C_SYM( cli ):
	.type	 cli,@function
	pushfl
	popl	%eax
	cli
	ret
.Lfe_cli:
	.size	 cli,.Lfe_cli - cli
	
_C_SYM( sti ):
	.type	 sti,@function
	sti
	ret
.Lfe_sti:
	.size	 sti,.Lfe_sti - sti



_C_SYM( _GETDS ):
#	push	%ax
#	mov	%cs,%ax
#	add	$16,%ax		# data descriptor with same DPL as CS
#	mov	%ax,%ds
#	popl	%ax

	pushl	$0x20
	popl	%ds
	ret

.bss
g_KernelStack:
	.rept	2047
	.long	0
	.endr
_C_SYM( g_anKernelStackEnd ):
	.long	0
	
	.END
