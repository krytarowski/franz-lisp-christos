#ifdef __APPLE__
	.section        __TEXT,__text,regular,pure_instructions
	.globl _callg
        .align  4, 0x90
_callg:
        .cfi_startproc
#else
	.text
	.globl	callg
	.type	callg, @function
callg:
#endif
	pushq	%rbp
	pushq	%rbx
	pushq	%rdi
	movq	%rsp, %rbx
	subq	$8, %rsp
	movq	%rsi, %rax
	movq	(%rsi), %rdx
	cmpq	$6, %rdx
	ja	.Lstack
#ifdef __APPLE__
	cmpq	$6, %rdx
	je	.L6
	cmpq	$5, %rdx
	je	.L5
	cmpq	$4, %rdx
	je	.L4
	cmpq	$3, %rdx
	je	.L3
	cmpq	$2, %rdx
	je	.L2
	cmpq	$1, %rdx
	je	.L1
	jmp	.L0
#else
	jmp	*.Ltable(,%rdx,8)
	.section	.rodata
	.align 8
	.align 4
.Ltable:
	.quad	.L0
	.quad	.L1
	.quad	.L2
	.quad	.L3
	.quad	.L4
	.quad	.L5
	.quad	.L6
	.text
#endif
.Lstack:
	leaq	-48(,%rdx,8), %rcx	# multiply by 8 , and -6 (reg args)
	subq	%rcx, %rsp		# move the stack down
	leaq	15(%rsp), %rcx		# rcx is the stack pointer
	andq	$-16, %rcx		# align it.
	movq	%rcx, %rsp		# align the stack
	movq	$6, %rdx		# start at the 6th argument
.Loop:
	movq	8(%rsi,%rdx,8), %r9	# r9 = args[rdx]
	movq	%r9, -48(%rcx,%rdx,8)	# stack[rdx] = r9
	addq	$1, %rdx
	cmpq	%rdx, (%rsi)
	jg	.Loop
.L6:
	movq	48(%rax), %r9
.L5:
	movq	40(%rax), %r8
.L4:
	movq	32(%rax), %rcx
.L3:
	movq	24(%rax), %rdx
.L2:
	movq	16(%rax), %rsi
.L1:
	movq	8(%rax), %rdi
.L0:
	call    *(%rbx)
	movq	%rbx, %rsp
	popq	%rbx
	popq	%rbx
	popq	%rbp
	ret
#ifdef __APPLE__
	.cfi_endproc
#else
	.size	callg, .-callg
#endif
