/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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
	
	.global _C_SYM( SysCallEntry )
	.global	_C_SYM( probe )
	.global	_C_SYM( ret_from_sys_call )
	.global	_C_SYM( exit_from_sys_call )
	.global _C_SYM( Schedule )

	.global	_C_SYM( irq1 )
	.global	_C_SYM( irq2 )
	.global	_C_SYM( irq3 )
	.global	_C_SYM( irq4 )
	.global	_C_SYM( irq5 )
	.global	_C_SYM( irq6 )
	.global	_C_SYM( irq7 )
	.global	_C_SYM( irq8 )
	.global	_C_SYM( irq9 )
	.global	_C_SYM( irqa )
	.global	_C_SYM( irqb )
	.global	_C_SYM( irqc )
	.global	_C_SYM( irqd )
	.global	_C_SYM( irqe )
	.global	_C_SYM( irqf )
	.global _C_SYM( smp_invalidate_pgt )
	.global _C_SYM( smp_preempt )
	.global _C_SYM( smp_spurious_irq )

	.global	_C_SYM( TSIHand )
	.global	_C_SYM( exc0 )
	.global	_C_SYM( exc1 )
	.global	_C_SYM( exc2 )
	.global	_C_SYM( exc3 )
	.global	_C_SYM( exc4 )
	.global	_C_SYM( exc5 )
	.global	_C_SYM( exc6 )
	.global	_C_SYM( exc7 )
	.global	_C_SYM( exc8 )
	.global	_C_SYM( exc9 )
	.global	_C_SYM( exca )
	.global	_C_SYM( excb )
	.global	_C_SYM( excc )
	.global	_C_SYM( excd )
	.global	_C_SYM( exce )
	.global	_C_SYM( exc10 )
	.global	_C_SYM( exc11 )
	.global	_C_SYM( exc12 )
	.global	_C_SYM( exc13 )


#define	USER_DS		0x23
#define	USER_CS		0x13
#define	KERNEL_CS	0x08
#define	KERNEL_DS	0x18


CF_MASK		= 0x00000001
IF_MASK		= 0x00000200
NT_MASK		= 0x00004000
VM_MASK		= 0x00020000



#include	<posix/errno.h>
#include	<atheos/syscall.h>

		
/*
 *	 0(%esp) - %ebx
 *	 4(%esp) - %ecx
 *	 8(%esp) - %edx
 *       C(%esp) - %esi
 *	10(%esp) - %edi
 *	14(%esp) - %ebp
 *	18(%esp) - %eax
 *	1C(%esp) - %ds
 *	20(%esp) - %es
 *      24(%esp) - %fs
 *	28(%esp) - %gs
 *	2C(%esp) - orig_eax
 *	30(%esp) - %eip
 *	34(%esp) - %cs
 *	38(%esp) - %eflags
 *	3C(%esp) - %oldesp
 *	40(%esp) - %oldss
*/



#	push  %gs;		
#	push  %fs;		
#define	ENTER_SYSCALL		\
	cld;			\
	push  %gs;		\
	push  $0;		\
	push  %es;		\
	push  %ds;		\
	pushl %eax;		\
	pushl %ebp;		\
	pushl %edi;		\
	pushl %esi;		\
	pushl %edx;		\
	pushl %ecx;		\
	pushl %ebx;		\
	movl  $0x23,%edx;	\
	mov   %ds,%cx;		\
	cmpw  %dx,%cx;		\
	je    1f;		\
	mov   %dx,%ds;		\
1:				\
	mov   %es,%cx;		\
	cmpw  %dx,%cx;		\
	je    2f;		\
	mov   %dx,%es;		\
2:;
/*	mov %dx,%fs; 
	mov %dx,%gs;*/

#define EXIT_SYSCALL	\
	popl %ebx;	\
	popl %ecx;	\
	popl %edx;	\
	popl %esi;	\
	popl %edi;	\
	popl %ebp;	\
	popl %eax;	\
	cmpw $0x23,(%esp); \
	je   1f;	\
	pop  %ds;	\
	jmp  2f;	\
1:			\
	addl $4,%esp;	\
2:			\
	cmpw $0x23,(%esp); \
	je   3f;	\
	pop  %es;	\
	jmp  4f;	\
3:			\
	addl $4,%esp;	\
4:			\
	addl $4 + 8,%esp;   \
	iret
#	pop %fs; \
#	pop  %gs;	\




EBX		= 0x00
ECX		= 0x04
EDX		= 0x08
ESI		= 0x0c
EDI		= 0x10
EBP		= 0x14
EAX		= 0x18
DS		= 0x1c
ES		= 0x20
FS		= 0x24
GS		= 0x28
ORIG_EAX	= 0x2c
EIP		= 0x30
CS		= 0x34
EFLAGS		= 0x38
OLDESP		= 0x3c
OLDSS		= 0x40


	.global _C_SYM( get_cs )
_get_cs:
	pushw	%cs
	popw	%ax
	andl	$0xffff, %eax
	ret
	
_C_SYM( smp_invalidate_pgt ):	
	cli
	pushl	$-1 /* %eax */
	ENTER_SYSCALL
	call	_C_SYM( do_smp_invalidate_pgt )
	EXIT_SYSCALL

_C_SYM( smp_spurious_irq ):	
	cli
	pushl	$-1 /* %eax */
	ENTER_SYSCALL
	call	_C_SYM( do_smp_spurious_irq )
	EXIT_SYSCALL
	
_C_SYM( smp_preempt ):
	cli
	pushl	$-1 /* %eax */
	ENTER_SYSCALL

	movl	%esp,%edx
	pushl	%edx		# push pointer to stack frame
	
	call	_C_SYM( do_smp_preempt )

	addl	$4,%esp

	jmp	ret_from_sys_call1
			
_C_SYM( TSIHand ):
	cli
	pushl	$-1 /* %eax */
	ENTER_SYSCALL

	movl	%esp,%edx
	pushl	%edx		# push pointer to stack frame

	call	_C_SYM( TimerInterrupt )

	addl	$4,%esp

	jmp	ret_from_sys_call1


	
_C_SYM( probe ):
	pushl	%eax
	cld;
	push %gs;
	push %fs;
	push %es;
	push %ds;
	pushl %eax;
	pushl %ebp;
	pushl %edi;
	pushl %esi;
	pushl %edx;
	pushl %ecx;
	pushl %ebx;
	movl $0x23,%edx;
	mov %dx,%ds;
	mov %dx,%es;
	

#	cmpw	$USER_CS, CS(%esp)
#	jne	no_sig_check
	call	_C_SYM( handle_signals )
#no_sig_check:
		
	popl %ebx;
	popl %ecx;
	popl %edx;
	popl %esi;
	popl %edi;
	popl %ebp;
	popl %eax;
	pop %ds;
	pop %es;
	pop %fs;
	pop %gs;
	addl $4,%esp;
	iret
		
_C_SYM( SysCallEntry ):
	pushl	%eax
	ENTER_SYSCALL

	movl	EAX(%esp),%eax;

	movl	$-ENOSYS,EAX(%esp)
	cmpl	$__NR_SysCallCount, %eax
	jae	no_sys

	leal	SysCallTable,%edx
	call	*(%edx, %eax, 4)	/* Call the actual function	*/
	movl	%eax,EAX(%esp)		/* Save the return value	*/

no_sys:
_C_SYM( exit_from_sys_call ):
	cmpl	$__NR_execve, ORIG_EAX(%esp)
	jne 0f
	call	_C_SYM( handle_exec_ptrace )
0:
	call	_C_SYM( HandleSTrace )
ret_from_sys_call1:
	cmpw	$USER_CS, CS(%esp)
	jne	no_sig_check
	call	_C_SYM( handle_signals )
no_sig_check:
	EXIT_SYSCALL


_C_SYM( ret_from_sys_call ):
	EXIT_SYSCALL

_C_SYM( Schedule ):
	movl 0(%esp), %eax;
	addl $4,%esp;
	pushfl
	pushl $KERNEL_CS
	pushl %eax
	pushl %eax;
	ENTER_SYSCALL

	call _C_SYM( sys_do_schedule )
	EXIT_SYSCALL
	



_C_SYM( irq1 ):
	pushl	%eax
	movl	$0x01,%eax
	jmp	irq
_C_SYM( irq2 ):
	pushl	%eax
	movl	$0x02,%eax
	jmp	irq
_C_SYM( irq3 ):
	pushl	%eax
	movl	$0x03,%eax
	jmp	irq
_C_SYM( irq4 ):
	pushl	%eax
	movl	$0x04,%eax
	jmp	irq
_C_SYM( irq5 ):
	pushl	%eax
	movl	$0x05,%eax
	jmp	irq
_C_SYM( irq6 ):
	pushl	%eax
	movl	$0x06,%eax
	jmp	irq
_C_SYM( irq7 ):
	pushl	%eax
	movl	$0x07,%eax
	jmp	irq
_C_SYM( irq8 ):
	pushl	%eax
	movl	$0x08,%eax
	jmp	irq
_C_SYM( irq9 ):
	pushl 	%eax
	movl	$0x09,%eax
	jmp	irq
_C_SYM( irqa ):
	pushl	%eax
	movl	$0x0a,%eax
	jmp	irq
_C_SYM( irqb ):
	pushl	%eax
	movl	$0x0b,%eax
	jmp	irq
_C_SYM( irqc ):
	pushl	%eax
	movl	$0x0c,%eax
	jmp	irq
_C_SYM( irqd ):
	pushl	%eax
	movl	$0x0d,%eax
	jmp	irq
_C_SYM( irqe ):
	pushl	%eax
	movl	$0x0e,%eax
	jmp	irq
_C_SYM( irqf ):
	pushl	%eax
	movl	$0x0f,%eax
	
	
irq:
	cli
	ENTER_SYSCALL

	movl	ORIG_EAX(%esp),%ebx
	movl	%ebx,EAX(%esp)

	movl	%esp,%edx
	pushl	%eax		# push the irq number
	pushl	%edx		# push pointer to stack frame
		
	call	_C_SYM( handle_irq )

	addl	$8,%esp
	
	EXIT_SYSCALL
/*	
_C_SYM( smp_schedule ):
	pushl	%eax
	movl	$0x10,%eax
	ENTER_SYSCALL

	movl	ORIG_EAX(%esp),%ebx
	movl	%ebx,EAX(%esp)

	movl	%esp,%edx
	pushl	%eax		# push the irq number
	pushl	%edx		# push pointer to stack frame
		
	call	_C_SYM( do_smp_schedule )

	addl	$8,%esp
	
	EXIT_SYSCALL
*/		
	
		
_C_SYM( exc0 ):
	pushl	$0	/* Fake error code */
	pushl	$_C_SYM( handle_divide_exception )
	jmp	error_code
_C_SYM( exc1 ):
	pushl	$1	/* Fake error code */
	pushl	$_C_SYM( handle_debug_exception )
	jmp	error_code
_C_SYM( exc3 ):
	pushl	$3	/* Fake error code */
	pushl	$_C_SYM( handle_debug_exception )
	jmp	error_code
_C_SYM( exc6 ):
	pushl	$0	/* Fake error code */
	pushl	$_C_SYM( handle_illegal_inst_exception )
	jmp	error_code
_C_SYM( exc7 ):
	pushl	$0	/* Fake error code */
	pushl	$_C_SYM( math_state_restore )
	jmp	error_code
		
_C_SYM( excd ):
	pushl	$_C_SYM( handle_general_protection )
	jmp	error_code
_C_SYM( exce ):
	pushl	$_C_SYM( handle_page_fault )
	jmp	error_code
	
_C_SYM( exc10 ):
	pushl	$0	/* Fake error code */
	pushl	$_C_SYM( handle_fpu_exception )
	jmp	error_code
_C_SYM( exc13 ):
	pushl	$0	/* Fake error code */
	pushl	$_C_SYM( handle_sse_exception )
	jmp	error_code

error_code:
	push	%fs
	push	%es
	push	%ds
	pushl	%eax
	xorl	%eax,%eax
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%edx
	decl	%eax			# eax = -1
	pushl	%ecx
	pushl	%ebx
	cld
	xorl	%ebx,%ebx		# zero ebx
	xchgl	%eax, ORIG_EAX(%esp)	# orig_eax (get the error code. )
	mov	%gs,%bx			# get the lower order bits of gs
	
	movl	%esp,%edx
	xchgl	%ebx, GS(%esp)		# get the address and save gs.
	pushl	%eax			# push the error code
	pushl	%edx			# push pointer to stack frame

	movl	$0x23,%edx
	mov	%dx,%ds
	mov	%dx,%es
	mov	%dx,%fs
#	mov	%dx,%gs
	
#	movl	%db6,%edx
#	movl	%edx,dbgreg6(%eax)  # save current hardware debugging status
	call	*%ebx
	addl	$8,%esp
	jmp	ret_from_sys_call1


_C_SYM( exc2 ):            # Exceptions
	pushl	$0	# pseudo error code
	pushl	$0x02
	jmp	exc
_C_SYM( exc4 ):
	pushl	$0	# pseudo error code
	pushl	$0x04
	jmp	exc
_C_SYM( exc5 ):
	pushl	$0	# pseudo error code
	pushl	$0x05
	jmp	exc
_C_SYM( exc8 ):
	pushl	$0x08
	jmp	exc
_C_SYM( exc9 ):
	pushl	$0	# pseudo error code
	pushl	$0x09
	jmp	exc
_C_SYM( exca ):
	pushl	$0x0a
	jmp	exc
_C_SYM( excb ):
	pushl	$0x0b
	jmp	exc
_C_SYM( excc ):
	pushl	$0x0c
	jmp	exc
_C_SYM( exc11 ):
	pushl	$0x11
	jmp	exc
_C_SYM( exc12 ):
	pushl	$0	# pseudo error code
	pushl	$0x12
	jmp	exc
_C_SYM( unexp ):                                  # Unexpected interrupt
	push	$0	# pseudo error code
        pushl	$0xff
exc:
	push	%fs
	push	%es
	push	%ds
	pushl	%eax
	xorl	%eax,%eax
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%edx
	decl	%eax			# eax = -1
	pushl	%ecx
	pushl	%ebx
	cld
	xorl	%ebx,%ebx		# zero ebx
	xchgl	%eax, ORIG_EAX(%esp)	# orig_eax (get the error code. )
	mov	%gs,%bx			# get the lower order bits of gs
	
	movl	%esp,%edx
	xchgl	%ebx, GS(%esp)		# get the exception number and save gs.
	pushl	%eax			# push the error code
	pushl	%ebx			# push the exception number
	pushl	%edx			# push pointer to stack frame

	movl	$0x23,%edx
	mov	%dx,%ds
	mov	%dx,%es
	mov	%dx,%fs
#	mov	%dx,%gs
	
#	movl	%db6,%edx
#	movl	%edx,dbgreg6(%eax)  # save current hardware debugging status
	call	_C_SYM( ExceptionHand )
	addl	$12,%esp
	jmp	ret_from_sys_call1


_sys_nosys:
	movl	$-ENOSYS, %eax
	ret
	
SysCallTable:
	
.long	_sys_nosys			/* 0 */
.long	_C_SYM( sys_open )
.long	_C_SYM( sys_close )
.long	_C_SYM( sys_read )
.long	_C_SYM( sys_write )
.long	_C_SYM( sys_Fork )		/* 5 */
.long	_C_SYM( sys_exit )
.long	_C_SYM( sys_rename )
.long	_C_SYM( sys_getdents )
.long	_C_SYM( sys_alarm )
.long	_C_SYM( sys_wait4 )		/* 10 */
.long	_C_SYM( sys_fstat )
.long	_C_SYM( sys_FileLength )
.long	_C_SYM( sys_old_seek )
.long	_C_SYM( sys_mkdir )
.long	_C_SYM( sys_rmdir )		/* 15 */
.long	_C_SYM( sys_dup )
.long	_C_SYM( sys_dup2 )
.long	_C_SYM( sys_fchdir )
.long	_C_SYM( sys_chdir )
.long	_C_SYM( sys_unlink )		/* 20 */
.long	_C_SYM( sys_rewinddir )
.long	_C_SYM( sys_get_thread_info )
.long	_C_SYM( sys_get_thread_proc )
.long	_C_SYM( sys_get_next_thread_info )
.long	_C_SYM( sys_get_thread_id )	/* 25 */
.long	_C_SYM( sys_send_data )
.long	_C_SYM( sys_receive_data )
.long	_C_SYM( sys_thread_data_size )
.long	_C_SYM( sys_has_data )
.long	_C_SYM( sys_SetThreadExitCode )	/* 30 */
.long	_C_SYM( old_spawn_thread )
.long	_C_SYM( sys_spawn_thread )
.long	_C_SYM( sys_get_raw_system_time )
.long	_C_SYM( sys_get_raw_real_time )
.long	_C_SYM( sys_get_raw_idle_time )	/* 35 */
.long	_C_SYM( sys_umask )
.long	_C_SYM( sys_mknod )
.long	_C_SYM( sys_reboot )
.long	_C_SYM( sys_GetToken )
.long	_C_SYM( sys_lseek )		/* 40 */
.long	_C_SYM( sys_utime )
.long	_C_SYM( sys_select )
.long	_C_SYM( sys_exit_thread )
.long	_C_SYM( sys_initialize_fs )
.long	_C_SYM( sys_DebugPrint )		/* 45 */
.long	_C_SYM( sys_realint )
.long	_C_SYM( sys_get_system_path )
.long	_C_SYM( sys_get_app_server_port )
.long	_C_SYM( sys_MemClear )
.long	_C_SYM( sys_mount )		/* 50 */
.long	_C_SYM( sys_unmount )
.long	_C_SYM( sys_create_area )
.long	_C_SYM( sys_remap_area )
.long	_C_SYM( sys_get_area_info )
.long	_C_SYM( sys_sbrk )		/* 55 */
.long	_C_SYM( sys_get_vesa_info )
.long	_C_SYM( sys_get_vesa_mode_info )
.long	_C_SYM( sys_set_real_time )
.long	_C_SYM( sys_create_port )
.long	_C_SYM( sys_delete_port )	/* 60 */
.long	_C_SYM( sys_send_msg )
.long	_C_SYM( sys_get_msg )
.long	_C_SYM( sys_raw_send_msg_x )
.long	_C_SYM( sys_raw_get_msg_x )
.long	_C_SYM( sys_debug_write )	/* 65 */
.long	_C_SYM( sys_debug_read )
.long	_C_SYM( sys_get_system_info )
	
.long	_C_SYM( sys_create_semaphore )
.long	_C_SYM( sys_delete_semaphore )
.long	_C_SYM( sys_lock_semaphore )	/* 70 */
.long	_C_SYM( sys_raw_lock_semaphore_x )
.long	_C_SYM( sys_unlock_semaphore_x )
.long	_C_SYM( sys_get_semaphore_info )

	
.long	_C_SYM( sys_clone_area )
.long	_C_SYM( sys_execve )		/* 75 */
.long	_C_SYM( sys_delete_area )
.long	_C_SYM( sys_chmod )
.long	_C_SYM( sys_fchmod )
.long	_C_SYM( sys_GetTime )
.long	_C_SYM( sys_SetTime )		/* 80 */
.long	_C_SYM( sys_old_load_library )
.long	_C_SYM( sys_unload_library )
.long	_C_SYM( sys_load_library )
.long	_C_SYM( sys_get_symbol_address )
.long	_C_SYM( sys_chown )		/* 85 */
.long	_C_SYM( sys_fchown )
.long	_C_SYM( sys_raw_read_pci_config )
.long	_C_SYM( sys_raw_write_pci_config )
.long	_C_SYM( sys_get_pci_info )
.long	_sys_nosys /*_C_SYM( sys_signal )*/		/* 90 */
.long	_C_SYM( sys_sig_return )
.long	_C_SYM( sys_kill )
.long	_C_SYM( sys_sigaction )
.long	_C_SYM( sys_sigaddset )
.long	_C_SYM( sys_sigdelset )		/* 95 */
.long	_C_SYM( sys_sigemptyset )
.long	_C_SYM( sys_sigfillset )
.long	_C_SYM( sys_sigismember )
.long	_C_SYM( sys_sigpending )
.long	_C_SYM( sys_sigprocmask )	/* 100 */
.long	_C_SYM( sys_sigsuspend )
.long	_C_SYM( old_Delay )
.long	_C_SYM( sys_set_thread_priority )
.long	_C_SYM( sys_suspend_thread )
.long	_C_SYM( sys_resume_thread )	/* 105 */
.long	_C_SYM( sys_wait_for_thread )
.long	_C_SYM( sys_snooze )
.long	_sys_nosys			/* _C_SYM( sys_Permit ) */
.long	_C_SYM( sys_killpg )
.long	_C_SYM( sys_kill_proc )	/* 110 */
.long	_C_SYM( sys_get_process_id )
.long	_C_SYM( sys_sync )
.long	_C_SYM( sys_fsync )
.long	_C_SYM( sys_isatty )
.long	_C_SYM( sys_fcntl )		/* 115 */
.long	_C_SYM( sys_ioctl )
.long	_C_SYM( sys_pipe )
.long	_C_SYM( sys_access )
.long	_sys_nosys			/* C_SYM( sys_set_strace_level ) */
.long	_C_SYM( sys_symlink )		/* 120 */
.long	_C_SYM( sys_readlink )
.long	_C_SYM( sys_call_v86 )
.long	_C_SYM( sys_stat )
.long	_C_SYM( sys_lstat )
.long	_C_SYM( sys_setpgid )		/* 125 */
.long	_C_SYM( sys_getpgid )
.long	_C_SYM( sys_getpgrp )
.long	_C_SYM( sys_getppid )
.long	_C_SYM( sys_getsid )
.long	_C_SYM( sys_setsid )		/* 130 */
.long	_C_SYM( sys_getegid )
.long	_C_SYM( sys_geteuid )
.long	_C_SYM( sys_getgid )
.long	_C_SYM( sys_getgroups )
.long	_C_SYM( sys_getuid )		/* 135 */
.long	_C_SYM( sys_setgid )
.long	_C_SYM( sys_setuid )
.long	_C_SYM( sys_set_app_server_port )
.long	_C_SYM( sys_rename_thread )

.long	_C_SYM( sys_get_mount_point_count )	/* 140 */
.long	_C_SYM( sys_get_mount_point )
.long	_C_SYM( sys_get_fs_info )

.long	_C_SYM( sys_open_attrdir )
.long	_C_SYM( sys_close_attrdir )
.long	_C_SYM( sys_rewind_attrdir )	/* 145 */
.long	_C_SYM( sys_read_attrdir )
.long	_C_SYM( sys_remove_attr )
.long   _sys_nosys
.long	_C_SYM( sys_stat_attr )
.long	_C_SYM( sys_write_attr )	/* 150 */
.long	_C_SYM( sys_read_attr )
.long	_C_SYM( sys_get_image_info )
.long	_C_SYM( sys_freadlink )
.long	_C_SYM( sys_get_directory_path )
.long	_C_SYM( sys_socket )		/* 155 */
.long	_C_SYM( sys_closesocket )
.long	_C_SYM( sys_bind )
.long	_C_SYM( sys_sendmsg )
.long	_C_SYM( sys_recvmsg )
.long	_C_SYM( sys_connect )		/* 160 */
.long	_C_SYM( sys_listen )
.long	_C_SYM( sys_accept )
.long	_C_SYM( sys_setsockopt )
.long	_C_SYM( sys_getsockopt )
.long	_C_SYM( sys_getpeername )	/* 165 */
.long	_C_SYM( sys_shutdown )
.long	_C_SYM( sys_based_open )
.long	_C_SYM( sys_getsockname )

.long	_C_SYM( sys_setregid )
.long	_C_SYM( sys_setgroups )		/* 170 */
.long	_C_SYM( sys_setreuid )
.long	_C_SYM( sys_setresuid )
.long	_C_SYM( sys_getresuid )
.long	_C_SYM( sys_setresgid )
.long	_C_SYM( sys_getresgid )		/* 175 */
.long	_C_SYM( sys_setfsuid )
.long	_C_SYM( sys_setfsgid )
.long	_C_SYM( sys_chroot )

.long	_C_SYM( sys_open_indexdir )
.long	_C_SYM( sys_rewind_indexdir )	/* 180 */
.long	_C_SYM( sys_read_indexdir )
.long	_C_SYM( sys_create_index )
.long	_C_SYM( sys_remove_index )
.long	_C_SYM( sys_stat_index )
.long   _C_SYM( sys_probe_fs )		/* 185 */
.long   _C_SYM( sys_get_next_semaphore_info )
.long   _C_SYM( sys_get_port_info )
.long   _C_SYM( sys_get_next_port_info )
	
.long   _C_SYM( sys_read_pos )
.long   _C_SYM( sys_readv )		/* 190 */
.long   _C_SYM( sys_readv_pos )
.long   _C_SYM( sys_write_pos )
.long   _C_SYM( sys_writev )
.long   _C_SYM( sys_writev_pos )
.long	_C_SYM( sys_alloc_tld )		/* 195 */
.long	_C_SYM( sys_free_tld )
.long	_C_SYM( sys_watch_node )
.long	_C_SYM( sys_delete_node_monitor )
.long	_C_SYM( sys_open_image_file )
.long	_C_SYM( sys_find_module_by_address ) /* 200 */
.long	_C_SYM( sys_based_mkdir )
.long	_C_SYM( sys_based_rmdir )
.long	_C_SYM( sys_based_unlink )
.long	_C_SYM( sys_based_symlink )

.long	_C_SYM( apm_poweroff )

.long	_C_SYM( sys_make_port_public )
.long	_C_SYM( sys_make_port_private )
.long	_C_SYM( sys_find_port )

.long	_C_SYM( sys_strace )
.long	_C_SYM( sys_strace_exclude )
.long	_C_SYM( sys_strace_include )
.long	_C_SYM( sys_ptrace )
.long	_C_SYM( sys_ftruncate )
.long	_C_SYM( sys_truncate )
.long	_C_SYM( sys_sigaltstack )

.long	_C_SYM( sys_get_tld_addr )

.long	_C_SYM( sys_get_msg_size ) 
.long	_C_SYM( sys_do_schedule )

.long	_sys_nosys	/* _C_SYM( sys_event ) */

.long	_C_SYM( sys_setitimer )
.long	_C_SYM( sys_getitimer )	/* 221 */


