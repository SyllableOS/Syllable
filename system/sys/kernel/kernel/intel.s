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


        mov	$0x11,%al
        outb	%al,$0x20
        jmp	1f
1:	
        mov	$0x20,%al	# First PIC start at vector 0x20
        outb	%al,$0x21
        jmp	2f
2:	
        mov	$0x04,%al
        outb	%al,$0x21
        jmp	3f
3:	
        mov	$0x01,%al
        outb	%al,$0x21
        jmp	4f
4:	
        mov	$0x11, %al
        outb	%al,$0xa0
        jmp	5f
5:	
        mov	$0x28,%al	# Second PIC start at vector 0x28
        outb	%al,$0xa1
        jmp	6f
6:	
        mov	$0x02,%al
        outb	%al,$0xa1
        jmp	7f
7:	
        mov	$0x01,%al
        outb	%al,$0xa1
	
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

_C_SYM( SwitchCont ):
	ljmp	*(%esp)
	ret


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
