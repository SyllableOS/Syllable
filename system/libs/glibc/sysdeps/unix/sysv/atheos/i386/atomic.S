
	.global	atomic_add
	.global	atomic_swap
	

atomic_swap:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	
	movl	8(%ebp),%ebx	# Destination
	movl	12(%ebp),%eax	# Source
	lock
	xchgl	%eax,(%ebx)
	
	popl	%ebx
	popl	%ebp
	ret

atomic_add:
	pushl	 %ebp
	movl	 %esp,%ebp
	pushl	 %ebx
	pushl	 %ecx
	pushl	 %edx
	
	movl	 8(%ebp),%ebx	# Destination
	movl	 12(%ebp),%ecx	# Value to add

	movl	 (%ebx),%eax	# Get "old" value into eax
changed:
	movl	 %eax,%edx	
	addl	 %ecx,%edx	# Get "new" value into edx
	lock
	cmpxchgl %edx,(%ebx)	# Copy "new" value into dest, if *dest == old
	jnz	 changed
	
	popl	 %edx
	popl	 %ecx
	popl	 %ebx
	popl	 %ebp
	ret
		
