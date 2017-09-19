
.macro sys_call n
	movl	$\n, %eax
.endm

.macro do_syscall
	int	$0x80
.endm

.macro syscall_arg_1
	pushl	%ebx
	movl	8(%esp), %ebx
	do_syscall
	popl	%ebx
	ret
.endm

.macro syscall_arg_2
	pushl	%ebx
	pushl	%ecx
	movl	12(%esp), %ebx
	movl	16(%esp), %ecx
	do_syscall
	popl	%ecx
	popl	%ebx
	ret
.endm

.macro syscall_arg_3
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	movl	16(%esp), %ebx
	movl	20(%esp), %ecx
	movl	24(%esp), %edx
	do_syscall
	popl	%edx
	popl	%ecx
	popl	%ebx
	ret
.endm

.macro syscall_arg_4
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	movl	20(%esp), %ebx
	movl	24(%esp), %ecx
	movl	28(%esp), %edx
	movl	32(%esp), %esi
	do_syscall
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	ret
.endm

.macro syscall_arg_5
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi
	movl	24(%esp), %ebx
	movl	28(%esp), %ecx
	movl	32(%esp), %edx
	movl	36(%esp), %esi
	movl	40(%esp), %edi
	do_syscall
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	ret
.endm

.macro syscall_arg_6
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
        pushl   %edi
	pushl	%ebp
	movl	28(%esp), %ebx
	movl	32(%esp), %ecx
	movl	36(%esp), %edx
	movl	40(%esp), %esi
	movl	44(%esp), %edi
	movl	48(%esp), %ebp
	do_syscall
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	ret
.endm

.macro ENTER base
	.global \base
	.type \base, @function
\base:
.endm

.macro SENTER base
	.type \base, @function
\base:
.endm

.macro END base
	.size \base, . - \base
.endm

ENTER	_start
	call _crcx
	call main
	pushl %eax
	call _terminate
END	_start

ENTER	_terminate
	sys_call 1
	syscall_arg_1
END	_terminate

ENTER transmit
	sys_call 2
	syscall_arg_4
END	transmit

ENTER	receive
	sys_call 3
	syscall_arg_4
END	receive

ENTER	fdwait
	sys_call 4
	syscall_arg_5
END	fdwait

ENTER	allocate
	sys_call 5
	syscall_arg_3
	END	allocate

ENTER	deallocate
	sys_call 6
	syscall_arg_2
END	deallocate

ENTER	random
	sys_call 7
	syscall_arg_3
END	random

ENTER	setjmp
	movl	4(%esp), %ecx
	movl	0(%esp), %edx
	movl	%edx, 0(%ecx)
	movl	%ebx, 4(%ecx)
	movl	%esp, 8(%ecx)
	movl	%ebp, 12(%ecx)
	movl	%esi, 16(%ecx)
	movl	%edi, 20(%ecx)
	xorl	%eax, %eax
	ret
END	setjmp

ENTER	longjmp
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	movl	0(%edx), %ecx
	movl	4(%edx), %ebx
	movl	8(%edx), %esp
	movl	12(%edx), %ebp
	movl	16(%edx), %esi
	movl	20(%edx), %edi
	testl	%eax, %eax
	jnz	1f
	incl	%eax
1:	movl	%ecx, 0(%esp)
	ret
END	longjmp

.macro push_crc_arg n
	.weak \n
	pushl $\n
.endm
	
SENTER	_crcx
	pushl $0
	push_crc_arg _binary_build_release_dpkg_o_txt_start
	push_crc_arg _binary_build_release_dpkg_o_txt_end
	call _crcx_docrc
	addl $12, %esp

	pushl $0
	pushl $0
	pushl $0
	pushl $0
	pushl $0
	pushl $0
	addl $6*4, %esp
	pushfl
	popl	%eax
	andl	$~0xc4, %eax
	pushl	%eax
	popfl
	pushl	$0
	popl	%eax
	
	ret
END	_crcx

SENTER _crcx_docrc
	pushl	%esi
	pushl	%ecx
	movl	12(%esp), %ecx
	movl	16(%esp), %esi
	movl	20(%esp), %eax
	subl	%esi, %ecx
	jz	2f
	
1:	xorb (%esi), %al
	inc %esi
	loop 1b
	
2:	popl	%ecx
	popl	%esi
	ret
END _crcx_docrc
	
