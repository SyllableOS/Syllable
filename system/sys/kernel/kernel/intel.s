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


#	.global	_C_SYM( ReadFlags )


#	.global	_C_SYM( SetTR )
#	.global	_C_SYM( SetIDT )
#	.global	_C_SYM( SetLDT )
#	.global	_C_SYM( SetGDT )


#	.global	_C_SYM( set_page_directory_base_reg )
#	.global	_C_SYM( enable_mmu )
#	.global	_C_SYM( disable_mmu )


#	.global	_C_SYM( _GETDS )
#	.global	_C_SYM( save_fpu_state )
#	.global	_C_SYM( load_fpu_state )

#	.global	_C_SYM( flush_tlb )
#	.global	_C_SYM( flush_tlb_page )

#	.global _C_SYM( GetFlags )
#	.global _C_SYM( PutFlags )

	.global	_C_SYM( put_cpu_flags )
	.global	_C_SYM( get_cpu_flags )
	
	.global _C_SYM( cli )
	.global	_C_SYM( sti )

	.global	_C_SYM( v86Stack_seg )
	.global	_C_SYM( v86Stack_off )
#	.global	_C_SYM( atomic_add )
#	.global	_C_SYM( atomic_swap )

#	.global	isa_readb
#	.global	isa_readw
#	.global	isa_readl
	
#	.global	isa_writeb
#	.global	isa_writew
#	.global	isa_writel
	

_start:
	fninit
	cmpl	$0x2BADB002,%eax
	je	multiboot
	movl	%cr0,%eax
	andb	$0xfb,%al	# clear EM flag
	orb	$0x22,%al	# set NE and MP flags
	movl	%eax,%cr0

	movl	%esp,%eax
	movl	$g_anKernelStackEnd,%esp
	
	pushl	16(%eax)
	pushl	12(%eax)
	pushl	8(%eax)
	pushl	4(%eax)
	call	_C_SYM( init_kernel )
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
        mov	$0x28,%al	# Seccond PIC start at vector 0x28
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

	
#_C_SYM( ReadFlags ):
#	pushfl
#	popl	%eax
#	ret
	
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
	
#_C_SYM( LoadTaskReg ):
#	mov	4(%esp),%ax
#	ltr	%ax
#	ret

_C_SYM( SwitchCont ):
	ljmp	*(%esp)
	ret

#_C_SYM( SetTR ):
#	mov	4(%esp),%eax
#	ltr	%ax
#	ret

#_C_SYM( SetIDT ):
#	mov	4(%esp),%eax
#	lidt	(%eax)
#	ret
#_C_SYM( SetGDT ):
#	mov	4(%esp),%eax
#	lgdt	(%eax)
#	ret
#_C_SYM( SetLDT ):
#	mov	4(%esp),%eax
#	lldt	(%eax)
#	ret
#_C_SYM( set_page_directory_base_reg )
#	movl	4(%esp),%eax
#	movl	%eax,%cr3
#	ret
#_C_SYM( enable_mmu ):
#	pushl	%eax
#	movl	%cr0,%eax
#	orl	$0x80010000,%eax	# set PG & WP bit in cr0
#	movl	%eax,%cr0
#	popl	%eax
#	ret
#_C_SYM( disable_mmu ):
#	pushl	%eax
#	mov	%cr0,%eax
#	and	$0x7FFeFFFF,%eax	# clr PG & WP bit in cr0
#	mov	%eax,%cr0
#	popl	%eax
#	ret


_C_SYM( _GETDS ):
#	push	%ax
#	mov	%cs,%ax
#	add	$16,%ax		# data descriptor with same DPL as CS
#	mov	%ax,%ds
#	popl	%ax

	pushl	$0x20
	popl	%ds
	ret
#_C_SYM( save_fpu_state ):
#	mov	4(%esp),%eax
#	fsave	(%eax)
#	ret
#_C_SYM( load_fpu_state ):
#	mov	4(%esp),%eax
#	frstor	(%eax)
#	ret
#_C_SYM( flush_tlb ):
#	pushl	%eax
#	movl	%cr3,%eax
#	movl	%eax,%cr3
#	popl	%eax
#	ret
#_C_SYM( flush_tlb_page ):
#	pushl	 %ebp
#	movl	 %esp,%ebp
#	pushl	 %ebx
#	movl	 8(%ebp),%ebx	# address
#	invlpg	 (%ebx)
#	popl	 %ebx
#	popl	 %ebp
#	ret
	
#_C_SYM( atomic_add ):
#	.type	 atomic_add,@function
#	pushl	 %ebp
#	movl	 %esp,%ebp
#	pushl	 %ebx
#	pushl	 %ecx
#	pushl	 %edx
#	
#	movl	 8(%ebp),%ebx	# Destination
#	movl	 12(%ebp),%ecx	# Value to add
#	movl	 (%ebx),%eax	# Get "old" value into eax
#changed:
#	movl	 %eax,%edx	
#	addl	 %ecx,%edx	# Get "new" value into edx
#	lock
#	cmpxchgl %edx,(%ebx)	# Copy "new" value into dest, if *dest == old
#	jnz	 changed
#	
#	popl	 %edx
#	popl	 %ecx
#	popl	 %ebx
#	popl	 %ebp
#	ret
#.Lfe_atomic_add:
#	.size	 atomic_add,.Lfe_atomic_add - atomic_add
	
	
#atomic_dec_and_test:
#	.type	 atomic_dec_and_test,@function
#	xorl	%eax,%eax
#	lock
#	decl	4(%esp)
#	setle	%al
#	ret
#.Lfe_atomic_dec_and_test:
#	.size	 atomic_dec_and_test,.Lfe_atomic_dec_and_test - atomic_dec_and_test

#_C_SYM( atomic_swap ):
#	.type	 atomic_swap,@function
#	pushl	%ebp
#	movl	%esp,%ebp
#	pushl	%ebx
	
#	movl	8(%ebp),%ebx	# Destination
#	movl	12(%ebp),%eax	# Source
#	lock
#	xchgl	%eax,(%ebx)
	
#	popl	%ebx
#	popl	%ebp
#	ret
#.Lfe_atomic_swap:
#	.size	 atomic_swap,.Lfe_atomic_swap - atomic_swap

#isa_readb:
#	pushl	%ebp
#	movl	%esp,%ebp
#	push	%edx
#	movl	8(%ebp),%edx
#	xorl	%eax,%eax
#	inb	%dx,%al
#	pop	%edx
#	pop	%ebp
#	ret

#isa_readw:
#	pushl	%ebp
#	movl	%esp,%ebp
#	push	%edx
#	movl	8(%ebp),%edx
#	xorl	%eax,%eax
#	inw	%dx,%ax
#	pop	%edx
#	pop	%ebp
#	ret

#isa_readl:
#	pushl	%ebp
#	movl	%esp,%ebp
#	push	%edx
#	movl	8(%ebp),%edx
#	xorl	%eax,%eax
#	inl	%dx,%eax
#	pop	%edx
#	pop	%ebp
#	ret

#isa_writeb:
#	pushl	%ebp
#	movl	%esp,%ebp
#	pushl	%eax
#	pushl	%edx
#	movl	8(%ebp),%edx	# Port address
#	movl	12(%ebp),%eax	# Value
#	outb	%al,%dx
#	popl	%edx
#	popl	%eax
#	popl	%ebp
#	ret

#isa_writew:
#	pushl	%ebp
#	movl	%esp,%ebp
#	pushl	%eax
#	pushl	%edx
#	movl	8(%ebp),%edx	# Port address
#	movl	12(%ebp),%eax	# Value
#	outw	%ax,%dx
#	popl	%edx
#	popl	%eax
#	popl	%ebp
#	ret

#isa_writel:
#	pushl	%ebp
#	movl	%esp,%ebp
#	pushl	%eax
#	pushl	%edx
#	movl	8(%ebp),%edx	# Port address
#	movl	12(%ebp),%eax	# Value
#	outl	%eax,%dx
#	popl	%edx
#	popl	%eax
#	popl	%ebp
#	ret

.bss
g_KernelStack:
	.rept	2047
	.long	0
	.endr
_C_SYM( g_anKernelStackEnd ):
	.long	0
	
	.END
