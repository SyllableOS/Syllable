	.globl _start
	.globl start_kernel
	.globl base_address
	.globl first_free_addr
	.globl loader_args
	
_start:
	mov	4(%esp),%eax
	mov	%eax,(base_address)
	mov	8(%esp),%eax
	mov	%eax,(first_free_addr)
	mov	12(%esp),%eax
	mov	%eax,(loader_args)
	
	mov	%ds,%ax
	mov	%ax,%ss
	mov	$1024*1024*4-4,%esp
	call	main
	ret

start_kernel:
	cli
	movl	4(%esp),%eax	# Entry point
	movl	8(%esp),%ebx	# real mem base
	movl	12(%esp),%ecx	# kernel size
	movl	16(%esp),%esi	# pointer to init script
	movl	20(%esp),%edi	# size of init script

	movw	$0x18,%dx	# zero based data segment
	movw	%dx,%ds
	movw	%dx,%es
	movw	%dx,%fs
	movw	%dx,%gs
	movw	%dx,%ss

	pushl	%edi	# size of init script
	pushl	%esi	# pointer to init script
	pushl	%ecx	# kernel size
	pushl	%ebx	# real mem base
	pushl	$0xdeadbabe # return address
	pushfl
	pushl	$0x38	# zero based code segment
	pushl	%eax	# entry point
	iret
base_address:		
	.long 0
first_free_addr:	
	.long 0
loader_args:
	.long 0
