	.text
.globl callg
	.type	callg, @function
callg:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	8(%ebp), %edi
	movl	12(%ebp), %esi
	movl	(%esi), %eax
	testl	%eax, %eax
	jle	.Lnoargs
	leal	12(,%eax,4), %eax	# multiply by 4 (and -12)
	subl	%eax, %esp
	xorl	%ebx, %ebx
.Loop:
	movl	4(%esi,%ebx,4), %eax
	movl	%eax, 0(%esp,%ebx,4)
	addl	$1, %ebx
	cmpl	%ebx, (%esi)
	jg	.Loop
.Lnoargs:
	call	*%edi
	movl	(%esi), %ebx
	leal	12(,%ebx,4), %ebx	# multiply by 4 (and +12)
	addl	%ebx, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.size	callg, .-callg
