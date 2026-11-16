	.file	"simd_probe.cpp"
	.text
	.p2align 4
	.globl	project_aos_double
	.type	project_aos_double, @function
project_aos_double:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%r14
	.cfi_def_cfa_offset 16
	.cfi_offset 14, -16
	movl	%esi, %r11d
	pushq	%r13
	.cfi_def_cfa_offset 24
	.cfi_offset 13, -24
	pushq	%r12
	.cfi_def_cfa_offset 32
	.cfi_offset 12, -32
	pushq	%rbp
	.cfi_def_cfa_offset 40
	.cfi_offset 6, -40
	pushq	%rbx
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	subq	$488, %rsp
	.cfi_def_cfa_offset 536
	movsd	624(%rsp), %xmm1
	testl	%esi, %esi
	jle	.L1
	movsd	568(%rsp), %xmm2
	movq	%rdx, %rsi
	movslq	%r11d, %r10
	leal	-1(%r11), %eax
	movsd	536(%rsp), %xmm7
	movsd	544(%rsp), %xmm10
	movsd	%xmm2, 240(%rsp)
	movsd	608(%rsp), %xmm2
	movsd	552(%rsp), %xmm9
	movsd	560(%rsp), %xmm8
	movsd	576(%rsp), %xmm6
	movsd	584(%rsp), %xmm5
	movq	616(%rsp), %r9
	movsd	%xmm2, 408(%rsp)
	movsd	592(%rsp), %xmm4
	movsd	600(%rsp), %xmm3
	cmpl	$14, %eax
	jbe	.L3
	leaq	0(,%r10,8), %rdx
	leaq	(%r10,%r10,2), %rax
	leaq	(%rsi,%rdx), %rbx
	addq	%rcx, %rdx
	leaq	(%rdi,%rax,8), %r13
	cmpq	%rdx, %rdi
	leaq	(%r8,%r10), %rbp
	setnb	%al
	cmpq	%r13, %rcx
	setnb	%r12b
	orl	%r12d, %eax
	cmpq	%rbx, %rdi
	setnb	%r12b
	cmpq	%r13, %rsi
	setnb	%r14b
	orl	%r14d, %r12d
	andl	%r12d, %eax
	cmpq	%rbp, %rdi
	setnb	%r12b
	cmpq	%r13, %r8
	setnb	%r13b
	orl	%r13d, %r12d
	leaq	8(%rsi), %r13
	andl	%r12d, %eax
	movq	%rcx, %r12
	subq	%r13, %r12
	cmpq	$112, %r12
	seta	%r12b
	andl	%r12d, %eax
	cmpq	%rbx, %r8
	setnb	%bl
	cmpq	%rbp, %rsi
	setnb	%r12b
	orl	%r12d, %ebx
	testb	%bl, %al
	je	.L3
	cmpq	%rdx, %r8
	setnb	%al
	cmpq	%rbp, %rcx
	setnb	%dl
	orb	%dl, %al
	je	.L3
	movapd	%xmm9, %xmm2
	movl	%r11d, %ebp
	movapd	%xmm10, %xmm15
	movq	%rdi, %rax
	unpcklpd	%xmm2, %xmm2
	movapd	%xmm7, %xmm0
	shrl	$4, %ebp
	unpcklpd	%xmm15, %xmm15
	movaps	%xmm2, -120(%rsp)
	unpcklpd	%xmm0, %xmm0
	salq	$7, %rbp
	movq	%rsi, %rdx
	movapd	%xmm8, %xmm2
	movapd	%xmm1, %xmm11
	movq	%rcx, %r10
	movq	%r8, %rbx
	unpcklpd	%xmm2, %xmm2
	movq	%r9, %xmm13
	movdqa	.LC0(%rip), %xmm12
	movsd	%xmm10, 424(%rsp)
	movaps	%xmm2, 264(%rsp)
	unpcklpd	%xmm11, %xmm11
	unpcklpd	%xmm13, %xmm13
	movsd	240(%rsp), %xmm2
	addq	%rsi, %rbp
	pxor	%xmm14, %xmm14
	movsd	%xmm9, 432(%rsp)
	movapd	%xmm15, %xmm10
	unpcklpd	%xmm2, %xmm2
	movapd	%xmm0, %xmm9
	movsd	%xmm7, 416(%rsp)
	movaps	%xmm2, 248(%rsp)
	movapd	%xmm6, %xmm2
	movsd	%xmm8, 440(%rsp)
	unpcklpd	%xmm2, %xmm2
	movaps	%xmm2, 280(%rsp)
	movapd	%xmm5, %xmm2
	movsd	%xmm6, 448(%rsp)
	unpcklpd	%xmm2, %xmm2
	movaps	%xmm2, -104(%rsp)
	movapd	%xmm4, %xmm2
	movsd	%xmm5, 456(%rsp)
	unpcklpd	%xmm2, %xmm2
	movaps	%xmm2, -88(%rsp)
	movapd	%xmm3, %xmm2
	movsd	%xmm4, 464(%rsp)
	unpcklpd	%xmm2, %xmm2
	movaps	%xmm2, -72(%rsp)
	movsd	408(%rsp), %xmm2
	movsd	%xmm3, 472(%rsp)
	movsd	%xmm1, 480(%rsp)
	unpcklpd	%xmm2, %xmm2
	.p2align 4,,10
	.p2align 3
.L4:
	movupd	(%rax), %xmm1
	movupd	16(%rax), %xmm7
	subq	$-128, %rdx
	addq	$384, %rax
	movupd	-288(%rax), %xmm4
	subq	$-128, %r10
	movupd	-320(%rax), %xmm8
	addq	$16, %rbx
	movapd	%xmm1, %xmm6
	unpcklpd	%xmm1, %xmm7
	movapd	%xmm1, %xmm0
	movhps	-360(%rax), %xmm0
	shufpd	$1, %xmm1, %xmm6
	movapd	%xmm7, %xmm1
	movhps	-344(%rax), %xmm1
	movupd	-240(%rax), %xmm5
	movaps	%xmm0, -56(%rsp)
	movupd	-272(%rax), %xmm0
	movupd	-192(%rax), %xmm15
	movhpd	-352(%rax), %xmm6
	movaps	%xmm1, -24(%rsp)
	movupd	-336(%rax), %xmm1
	unpcklpd	%xmm4, %xmm0
	movaps	%xmm6, -40(%rsp)
	movapd	%xmm1, %xmm7
	movapd	%xmm1, %xmm3
	movhps	-312(%rax), %xmm7
	movapd	%xmm0, %xmm6
	unpcklpd	%xmm1, %xmm8
	shufpd	$1, %xmm1, %xmm3
	movapd	%xmm4, %xmm1
	movapd	%xmm5, %xmm0
	movaps	%xmm7, -8(%rsp)
	shufpd	$1, %xmm4, %xmm1
	movapd	%xmm8, %xmm7
	movapd	%xmm4, %xmm8
	movupd	-224(%rax), %xmm4
	movhps	-264(%rax), %xmm8
	movhpd	-304(%rax), %xmm3
	movhpd	-256(%rax), %xmm1
	movhps	-296(%rax), %xmm7
	movhps	-248(%rax), %xmm6
	shufpd	$1, %xmm5, %xmm0
	movhpd	-208(%rax), %xmm0
	unpcklpd	%xmm5, %xmm4
	movhps	-200(%rax), %xmm4
	movaps	%xmm8, 8(%rsp)
	movapd	%xmm5, %xmm8
	movhps	-216(%rax), %xmm8
	movaps	%xmm4, 24(%rsp)
	movapd	%xmm15, %xmm4
	movhps	-168(%rax), %xmm4
	movaps	%xmm4, 40(%rsp)
	movapd	%xmm15, %xmm4
	shufpd	$1, %xmm15, %xmm4
	movhpd	-160(%rax), %xmm4
	movaps	%xmm4, 56(%rsp)
	movupd	-176(%rax), %xmm4
	unpcklpd	%xmm15, %xmm4
	movapd	%xmm4, %xmm15
	movhps	-152(%rax), %xmm15
	movupd	-128(%rax), %xmm5
	movaps	%xmm15, 72(%rsp)
	movupd	-144(%rax), %xmm15
	movapd	%xmm15, %xmm4
	movhps	-120(%rax), %xmm4
	unpcklpd	%xmm15, %xmm5
	movaps	%xmm4, 88(%rsp)
	movapd	%xmm15, %xmm4
	shufpd	$1, %xmm15, %xmm4
	movapd	%xmm5, %xmm15
	movhps	-104(%rax), %xmm15
	movhpd	-112(%rax), %xmm4
	movaps	%xmm4, 104(%rsp)
	movaps	%xmm15, 120(%rsp)
	movupd	-96(%rax), %xmm15
	movapd	%xmm15, %xmm4
	movapd	%xmm15, %xmm5
	movhps	-72(%rax), %xmm5
	shufpd	$1, %xmm15, %xmm4
	movhpd	-64(%rax), %xmm4
	movaps	%xmm4, 152(%rsp)
	movupd	-80(%rax), %xmm4
	movaps	%xmm5, 136(%rsp)
	movupd	-32(%rax), %xmm5
	unpcklpd	%xmm15, %xmm4
	movapd	%xmm4, %xmm15
	movhps	-56(%rax), %xmm15
	movaps	%xmm15, 168(%rsp)
	movupd	-48(%rax), %xmm15
	movapd	%xmm15, %xmm4
	movhps	-24(%rax), %xmm4
	unpcklpd	%xmm15, %xmm5
	movaps	%xmm4, 184(%rsp)
	movapd	%xmm15, %xmm4
	shufpd	$1, %xmm15, %xmm4
	movhpd	-16(%rax), %xmm4
	movapd	%xmm5, %xmm15
	movhps	-8(%rax), %xmm15
	movaps	%xmm4, 200(%rsp)
	movapd	-88(%rsp), %xmm4
	movaps	%xmm15, 216(%rsp)
	movapd	-8(%rsp), %xmm15
	mulpd	-104(%rsp), %xmm15
	mulpd	%xmm3, %xmm4
	addpd	%xmm15, %xmm4
	movapd	-72(%rsp), %xmm15
	mulpd	%xmm7, %xmm15
	movapd	%xmm4, %xmm5
	addpd	%xmm15, %xmm5
	movaps	%xmm5, 296(%rsp)
	movapd	8(%rsp), %xmm5
	mulpd	-104(%rsp), %xmm5
	movapd	-88(%rsp), %xmm4
	mulpd	%xmm1, %xmm4
	addpd	%xmm5, %xmm4
	movapd	-72(%rsp), %xmm5
	movapd	%xmm5, %xmm15
	mulpd	%xmm6, %xmm15
	addpd	%xmm15, %xmm4
	movapd	-104(%rsp), %xmm15
	mulpd	%xmm8, %xmm15
	movaps	%xmm4, 312(%rsp)
	movapd	-88(%rsp), %xmm4
	mulpd	%xmm0, %xmm4
	addpd	%xmm15, %xmm4
	movapd	24(%rsp), %xmm15
	mulpd	%xmm5, %xmm15
	movapd	%xmm4, %xmm5
	addpd	%xmm15, %xmm5
	movaps	%xmm5, 328(%rsp)
	movapd	40(%rsp), %xmm5
	mulpd	-104(%rsp), %xmm5
	movapd	%xmm5, %xmm15
	movapd	56(%rsp), %xmm5
	mulpd	-88(%rsp), %xmm5
	movapd	%xmm5, %xmm4
	movapd	72(%rsp), %xmm5
	mulpd	-72(%rsp), %xmm5
	addpd	%xmm15, %xmm4
	movapd	88(%rsp), %xmm15
	mulpd	-104(%rsp), %xmm15
	addpd	%xmm5, %xmm4
	movapd	104(%rsp), %xmm5
	mulpd	-88(%rsp), %xmm5
	movaps	%xmm4, 344(%rsp)
	movapd	%xmm5, %xmm4
	movapd	120(%rsp), %xmm5
	mulpd	-72(%rsp), %xmm5
	addpd	%xmm15, %xmm4
	movapd	136(%rsp), %xmm15
	mulpd	-104(%rsp), %xmm15
	addpd	%xmm5, %xmm4
	movapd	152(%rsp), %xmm5
	mulpd	-88(%rsp), %xmm5
	movaps	%xmm4, 360(%rsp)
	movapd	%xmm5, %xmm4
	movapd	168(%rsp), %xmm5
	mulpd	-72(%rsp), %xmm5
	addpd	%xmm15, %xmm4
	movapd	184(%rsp), %xmm15
	mulpd	-104(%rsp), %xmm15
	addpd	%xmm5, %xmm4
	movapd	200(%rsp), %xmm5
	movaps	%xmm4, 376(%rsp)
	mulpd	-88(%rsp), %xmm5
	movapd	%xmm5, %xmm4
	movapd	216(%rsp), %xmm5
	mulpd	-72(%rsp), %xmm5
	addpd	%xmm15, %xmm4
	movapd	-56(%rsp), %xmm15
	mulpd	%xmm9, %xmm15
	addpd	%xmm5, %xmm4
	movapd	-24(%rsp), %xmm5
	mulpd	-120(%rsp), %xmm5
	movaps	%xmm4, 392(%rsp)
	movapd	-40(%rsp), %xmm4
	mulpd	%xmm10, %xmm4
	addpd	%xmm15, %xmm4
	movapd	-120(%rsp), %xmm15
	mulpd	%xmm7, %xmm15
	addpd	%xmm5, %xmm4
	movapd	-8(%rsp), %xmm5
	mulpd	%xmm9, %xmm5
	mulpd	%xmm11, %xmm4
	addpd	%xmm2, %xmm4
	movups	%xmm4, -128(%rdx)
	movapd	%xmm3, %xmm4
	mulpd	%xmm10, %xmm4
	addpd	%xmm5, %xmm4
	movapd	8(%rsp), %xmm5
	mulpd	%xmm9, %xmm5
	addpd	%xmm15, %xmm4
	mulpd	%xmm11, %xmm4
	addpd	%xmm2, %xmm4
	movups	%xmm4, -112(%rdx)
	movapd	%xmm1, %xmm4
	mulpd	%xmm10, %xmm4
	addpd	%xmm5, %xmm4
	movapd	-120(%rsp), %xmm5
	movapd	%xmm5, %xmm15
	mulpd	%xmm6, %xmm15
	addpd	%xmm15, %xmm4
	mulpd	%xmm11, %xmm4
	addpd	%xmm2, %xmm4
	movups	%xmm4, -96(%rdx)
	movapd	%xmm8, %xmm4
	mulpd	%xmm9, %xmm4
	movapd	%xmm4, %xmm15
	movapd	%xmm0, %xmm4
	mulpd	%xmm10, %xmm4
	addpd	%xmm15, %xmm4
	movapd	24(%rsp), %xmm15
	mulpd	%xmm5, %xmm15
	movapd	40(%rsp), %xmm5
	mulpd	%xmm9, %xmm5
	addpd	%xmm15, %xmm4
	movapd	88(%rsp), %xmm15
	mulpd	%xmm9, %xmm15
	mulpd	%xmm11, %xmm4
	addpd	%xmm2, %xmm4
	movups	%xmm4, -80(%rdx)
	movapd	56(%rsp), %xmm4
	mulpd	%xmm10, %xmm4
	addpd	%xmm5, %xmm4
	movapd	72(%rsp), %xmm5
	mulpd	-120(%rsp), %xmm5
	addpd	%xmm5, %xmm4
	movapd	120(%rsp), %xmm5
	mulpd	-120(%rsp), %xmm5
	mulpd	%xmm11, %xmm4
	addpd	%xmm2, %xmm4
	movups	%xmm4, -64(%rdx)
	movapd	104(%rsp), %xmm4
	mulpd	%xmm10, %xmm4
	addpd	%xmm15, %xmm4
	addpd	%xmm5, %xmm4
	movapd	168(%rsp), %xmm5
	mulpd	-120(%rsp), %xmm5
	mulpd	%xmm11, %xmm4
	addpd	%xmm2, %xmm4
	movups	%xmm4, -48(%rdx)
	movapd	136(%rsp), %xmm4
	mulpd	%xmm9, %xmm4
	movapd	%xmm4, %xmm15
	movapd	152(%rsp), %xmm4
	mulpd	%xmm10, %xmm4
	addpd	%xmm15, %xmm4
	addpd	%xmm5, %xmm4
	mulpd	%xmm11, %xmm4
	addpd	%xmm2, %xmm4
	movups	%xmm4, -32(%rdx)
	movapd	184(%rsp), %xmm15
	movapd	200(%rsp), %xmm4
	movapd	216(%rsp), %xmm5
	mulpd	-120(%rsp), %xmm5
	mulpd	%xmm9, %xmm15
	mulpd	%xmm10, %xmm4
	mulpd	248(%rsp), %xmm3
	addpd	%xmm15, %xmm4
	addpd	%xmm5, %xmm4
	movapd	-40(%rsp), %xmm5
	mulpd	248(%rsp), %xmm5
	mulpd	%xmm11, %xmm4
	addpd	%xmm2, %xmm4
	movups	%xmm4, -16(%rdx)
	movapd	-56(%rsp), %xmm4
	mulpd	264(%rsp), %xmm4
	movapd	%xmm4, %xmm15
	movapd	%xmm5, %xmm4
	movapd	-24(%rsp), %xmm5
	mulpd	280(%rsp), %xmm5
	addpd	%xmm15, %xmm4
	movapd	%xmm13, %xmm15
	addpd	%xmm5, %xmm4
	movapd	-8(%rsp), %xmm5
	mulpd	%xmm11, %xmm4
	subpd	%xmm4, %xmm15
	movups	%xmm15, -128(%r10)
	movapd	264(%rsp), %xmm15
	mulpd	%xmm15, %xmm5
	addpd	%xmm5, %xmm3
	movapd	280(%rsp), %xmm5
	mulpd	%xmm5, %xmm7
	mulpd	%xmm5, %xmm6
	addpd	%xmm7, %xmm3
	movapd	%xmm13, %xmm7
	mulpd	%xmm11, %xmm3
	subpd	%xmm3, %xmm7
	movapd	8(%rsp), %xmm3
	mulpd	%xmm15, %xmm3
	movups	%xmm7, -112(%r10)
	movapd	%xmm15, %xmm7
	movapd	248(%rsp), %xmm15
	mulpd	%xmm7, %xmm8
	mulpd	%xmm15, %xmm1
	mulpd	%xmm15, %xmm0
	addpd	%xmm3, %xmm1
	movapd	%xmm13, %xmm3
	addpd	%xmm8, %xmm0
	movapd	88(%rsp), %xmm8
	addpd	%xmm6, %xmm1
	movapd	56(%rsp), %xmm6
	mulpd	%xmm7, %xmm8
	mulpd	%xmm15, %xmm6
	mulpd	%xmm11, %xmm1
	subpd	%xmm1, %xmm3
	movapd	24(%rsp), %xmm1
	mulpd	%xmm5, %xmm1
	movups	%xmm3, -96(%r10)
	movapd	%xmm14, %xmm3
	addpd	%xmm1, %xmm0
	movapd	%xmm13, %xmm1
	mulpd	%xmm11, %xmm0
	subpd	%xmm0, %xmm1
	movapd	40(%rsp), %xmm0
	mulpd	%xmm7, %xmm0
	movups	%xmm1, -80(%r10)
	movapd	%xmm6, %xmm1
	movapd	%xmm14, %xmm6
	addpd	%xmm0, %xmm1
	movapd	72(%rsp), %xmm0
	mulpd	%xmm5, %xmm0
	addpd	%xmm0, %xmm1
	movapd	%xmm13, %xmm0
	mulpd	%xmm11, %xmm1
	subpd	%xmm1, %xmm0
	movups	%xmm0, -64(%r10)
	movapd	%xmm8, %xmm0
	movapd	104(%rsp), %xmm8
	mulpd	%xmm15, %xmm8
	movapd	%xmm8, %xmm1
	movapd	120(%rsp), %xmm8
	addpd	%xmm0, %xmm1
	movapd	%xmm13, %xmm0
	mulpd	%xmm5, %xmm8
	addpd	%xmm8, %xmm1
	movapd	136(%rsp), %xmm8
	mulpd	%xmm7, %xmm8
	mulpd	%xmm11, %xmm1
	subpd	%xmm1, %xmm0
	movups	%xmm0, -48(%r10)
	movapd	%xmm8, %xmm0
	movapd	152(%rsp), %xmm8
	mulpd	%xmm15, %xmm8
	movapd	%xmm8, %xmm1
	movapd	168(%rsp), %xmm8
	cmplepd	360(%rsp), %xmm6
	cmplepd	376(%rsp), %xmm3
	addpd	%xmm0, %xmm1
	movapd	%xmm13, %xmm0
	mulpd	%xmm5, %xmm8
	pand	%xmm12, %xmm6
	pand	%xmm12, %xmm3
	addpd	%xmm8, %xmm1
	movapd	184(%rsp), %xmm8
	mulpd	%xmm7, %xmm8
	movapd	200(%rsp), %xmm7
	mulpd	%xmm11, %xmm1
	mulpd	%xmm15, %xmm7
	subpd	%xmm1, %xmm0
	movapd	%xmm7, %xmm1
	movapd	216(%rsp), %xmm7
	addpd	%xmm8, %xmm1
	movapd	%xmm14, %xmm8
	mulpd	%xmm5, %xmm7
	movups	%xmm0, -32(%r10)
	movapd	-56(%rsp), %xmm0
	cmplepd	296(%rsp), %xmm8
	mulpd	-104(%rsp), %xmm0
	movapd	%xmm14, %xmm5
	cmplepd	392(%rsp), %xmm5
	pand	%xmm12, %xmm8
	addpd	%xmm7, %xmm1
	movapd	%xmm13, %xmm7
	pand	%xmm12, %xmm5
	shufps	$136, %xmm5, %xmm3
	mulpd	%xmm11, %xmm1
	subpd	%xmm1, %xmm7
	movapd	-40(%rsp), %xmm1
	mulpd	-88(%rsp), %xmm1
	movups	%xmm7, -16(%r10)
	movapd	%xmm14, %xmm7
	cmplepd	312(%rsp), %xmm7
	addpd	%xmm1, %xmm0
	movapd	-24(%rsp), %xmm1
	mulpd	-72(%rsp), %xmm1
	movapd	%xmm7, %xmm4
	movapd	%xmm14, %xmm7
	cmplepd	328(%rsp), %xmm7
	pand	%xmm12, %xmm4
	addpd	%xmm0, %xmm1
	movapd	%xmm14, %xmm0
	pand	%xmm12, %xmm7
	shufps	$136, %xmm7, %xmm4
	cmplepd	%xmm1, %xmm0
	movapd	%xmm14, %xmm1
	cmplepd	344(%rsp), %xmm1
	pand	%xmm12, %xmm0
	shufps	$136, %xmm8, %xmm0
	movdqa	%xmm0, %xmm7
	punpcklwd	%xmm4, %xmm0
	pand	%xmm12, %xmm1
	punpckhwd	%xmm4, %xmm7
	movdqa	%xmm0, %xmm4
	shufps	$136, %xmm6, %xmm1
	punpckhwd	%xmm7, %xmm4
	punpcklwd	%xmm7, %xmm0
	punpcklwd	%xmm4, %xmm0
	movdqa	%xmm1, %xmm4
	punpcklwd	%xmm3, %xmm1
	punpckhwd	%xmm3, %xmm4
	movdqa	%xmm1, %xmm3
	pand	.LC1(%rip), %xmm0
	punpckhwd	%xmm4, %xmm3
	punpcklwd	%xmm4, %xmm1
	punpcklwd	%xmm3, %xmm1
	pand	.LC1(%rip), %xmm1
	packuswb	%xmm1, %xmm0
	movups	%xmm0, -16(%rbx)
	cmpq	%rbp, %rdx
	jne	.L4
	movsd	416(%rsp), %xmm7
	movl	%r11d, %eax
	movsd	424(%rsp), %xmm10
	movsd	432(%rsp), %xmm9
	andl	$-16, %eax
	movsd	440(%rsp), %xmm8
	movsd	448(%rsp), %xmm6
	movsd	456(%rsp), %xmm5
	movsd	464(%rsp), %xmm4
	movsd	472(%rsp), %xmm3
	movsd	480(%rsp), %xmm1
	testb	$15, %r11b
	je	.L1
	leaq	(%rax,%rax,2), %rdx
	pxor	%xmm2, %xmm2
	movsd	408(%rsp), %xmm14
	leaq	(%rdi,%rdx,8), %rdx
	.p2align 4,,10
	.p2align 3
.L6:
	movsd	(%rdx), %xmm12
	movsd	8(%rdx), %xmm0
	movapd	%xmm7, %xmm11
	movapd	%xmm10, %xmm15
	movsd	16(%rdx), %xmm13
	mulsd	%xmm0, %xmm15
	mulsd	%xmm12, %xmm11
	addsd	%xmm15, %xmm11
	movapd	%xmm9, %xmm15
	mulsd	%xmm13, %xmm15
	addsd	%xmm15, %xmm11
	movsd	240(%rsp), %xmm15
	mulsd	%xmm0, %xmm15
	mulsd	%xmm1, %xmm11
	mulsd	%xmm4, %xmm0
	addsd	%xmm14, %xmm11
	movsd	%xmm11, (%rsi,%rax,8)
	movapd	%xmm8, %xmm11
	mulsd	%xmm12, %xmm11
	mulsd	%xmm5, %xmm12
	addsd	%xmm15, %xmm11
	movapd	%xmm6, %xmm15
	mulsd	%xmm13, %xmm15
	addsd	%xmm0, %xmm12
	movapd	%xmm13, %xmm0
	mulsd	%xmm3, %xmm0
	addsd	%xmm15, %xmm11
	movq	%r9, %xmm15
	addsd	%xmm12, %xmm0
	mulsd	%xmm1, %xmm11
	comisd	%xmm2, %xmm0
	subsd	%xmm11, %xmm15
	movsd	%xmm15, (%rcx,%rax,8)
	setnb	(%r8,%rax)
	addq	$1, %rax
	addq	$24, %rdx
	cmpl	%eax, %r11d
	jg	.L6
.L1:
	addq	$488, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 48
	popq	%rbx
	.cfi_def_cfa_offset 40
	popq	%rbp
	.cfi_def_cfa_offset 32
	popq	%r12
	.cfi_def_cfa_offset 24
	popq	%r13
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L3:
	.cfi_restore_state
	movsd	408(%rsp), %xmm14
	movq	%rdi, %rax
	xorl	%edx, %edx
	pxor	%xmm2, %xmm2
	.p2align 4,,10
	.p2align 3
.L7:
	movsd	(%rax), %xmm12
	movsd	8(%rax), %xmm0
	movapd	%xmm7, %xmm11
	movapd	%xmm10, %xmm15
	movsd	16(%rax), %xmm13
	mulsd	%xmm0, %xmm15
	mulsd	%xmm12, %xmm11
	addsd	%xmm15, %xmm11
	movapd	%xmm9, %xmm15
	mulsd	%xmm13, %xmm15
	addsd	%xmm15, %xmm11
	movsd	240(%rsp), %xmm15
	mulsd	%xmm0, %xmm15
	mulsd	%xmm1, %xmm11
	mulsd	%xmm4, %xmm0
	addsd	%xmm14, %xmm11
	movsd	%xmm11, (%rsi,%rdx,8)
	movapd	%xmm8, %xmm11
	mulsd	%xmm12, %xmm11
	mulsd	%xmm5, %xmm12
	addsd	%xmm15, %xmm11
	movapd	%xmm6, %xmm15
	mulsd	%xmm13, %xmm15
	addsd	%xmm0, %xmm12
	movapd	%xmm13, %xmm0
	mulsd	%xmm3, %xmm0
	addsd	%xmm15, %xmm11
	movq	%r9, %xmm15
	addsd	%xmm12, %xmm0
	mulsd	%xmm1, %xmm11
	comisd	%xmm2, %xmm0
	subsd	%xmm11, %xmm15
	movsd	%xmm15, (%rcx,%rdx,8)
	setnb	(%r8,%rdx)
	addq	$1, %rdx
	addq	$24, %rax
	cmpq	%rdx, %r10
	jne	.L7
	addq	$488, %rsp
	.cfi_def_cfa_offset 48
	popq	%rbx
	.cfi_def_cfa_offset 40
	popq	%rbp
	.cfi_def_cfa_offset 32
	popq	%r12
	.cfi_def_cfa_offset 24
	popq	%r13
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size	project_aos_double, .-project_aos_double
	.p2align 4
	.globl	project_aos_float
	.type	project_aos_float, @function
project_aos_float:
.LFB1:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$160, %rsp
	.cfi_def_cfa_offset 216
	movss	260(%rsp), %xmm8
	testl	%esi, %esi
	jle	.L22
	movss	240(%rsp), %xmm7
	leal	-1(%rsi), %r14d
	movq	%rdi, %rax
	movss	216(%rsp), %xmm6
	movss	220(%rsp), %xmm5
	movl	%esi, %edi
	movslq	%esi, %r9
	movss	224(%rsp), %xmm4
	movss	228(%rsp), %xmm3
	movss	%xmm7, 104(%rsp)
	movss	252(%rsp), %xmm7
	movss	232(%rsp), %xmm2
	movss	236(%rsp), %xmm1
	movss	244(%rsp), %xmm10
	movss	%xmm7, 20(%rsp)
	movss	256(%rsp), %xmm7
	movss	248(%rsp), %xmm9
	movss	%xmm7, 108(%rsp)
	cmpl	$6, %r14d
	jbe	.L24
	leaq	0(,%r9,4), %r11
	leaq	(%r9,%r9,2), %r10
	leaq	(%rdx,%r11), %r13
	addq	%rcx, %r11
	leaq	(%rax,%r10,4), %rbx
	cmpq	%r11, %rax
	leaq	(%r8,%r9), %r12
	setnb	%bpl
	cmpq	%rbx, %rcx
	setnb	%r10b
	orl	%r10d, %ebp
	cmpq	%r13, %rax
	setnb	%r10b
	cmpq	%rbx, %rdx
	setnb	%r15b
	orl	%r15d, %r10d
	andl	%r10d, %ebp
	cmpq	%r12, %rax
	setnb	%r10b
	cmpq	%rbx, %r8
	setnb	%bl
	orl	%ebx, %r10d
	movq	%rcx, %rbx
	andl	%ebp, %r10d
	leaq	4(%rdx), %rbp
	subq	%rbp, %rbx
	cmpq	$56, %rbx
	seta	%bl
	andl	%ebx, %r10d
	cmpq	%r13, %r8
	setnb	%bl
	cmpq	%r12, %rdx
	setnb	%bpl
	orl	%ebp, %ebx
	testb	%bl, %r10b
	je	.L24
	cmpq	%r11, %r8
	setnb	%r10b
	cmpq	%r12, %rcx
	setnb	%r11b
	orb	%r11b, %r10b
	je	.L24
	cmpl	$14, %r14d
	jbe	.L32
	movaps	%xmm7, %xmm14
	movaps	%xmm6, %xmm7
	movl	%esi, %ebp
	movss	20(%rsp), %xmm0
	shufps	$0, %xmm7, %xmm7
	shrl	$4, %ebp
	movq	%rax, %r9
	movq	%rdx, %r10
	movaps	%xmm7, -120(%rsp)
	movaps	%xmm5, %xmm7
	shufps	$0, %xmm0, %xmm0
	movq	%rcx, %r11
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, -104(%rsp)
	movaps	%xmm4, %xmm7
	salq	$6, %rbp
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, -88(%rsp)
	movaps	%xmm3, %xmm7
	movq	%r8, %rbx
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, 72(%rsp)
	movaps	%xmm2, %xmm7
	addq	%rdx, %rbp
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, 88(%rsp)
	movaps	%xmm1, %xmm7
	shufps	$0, %xmm14, %xmm14
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, 24(%rsp)
	movss	104(%rsp), %xmm7
	movaps	%xmm0, -40(%rsp)
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, 40(%rsp)
	movaps	%xmm10, %xmm7
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, -72(%rsp)
	movaps	%xmm9, %xmm7
	shufps	$0, %xmm7, %xmm7
	movss	%xmm6, 112(%rsp)
	movaps	%xmm7, -56(%rsp)
	movaps	%xmm8, %xmm7
	movss	%xmm5, 120(%rsp)
	shufps	$0, %xmm7, %xmm7
	movss	%xmm4, 124(%rsp)
	movss	%xmm3, 128(%rsp)
	movss	%xmm2, 132(%rsp)
	movss	%xmm1, 136(%rsp)
	movss	%xmm10, 140(%rsp)
	movss	%xmm9, 144(%rsp)
	movss	%xmm8, 148(%rsp)
	.p2align 4,,10
	.p2align 3
.L26:
	movups	(%r9), %xmm1
	movups	16(%r9), %xmm0
	addq	$64, %r10
	addq	$192, %r9
	movups	-160(%r9), %xmm3
	movups	-128(%r9), %xmm5
	addq	$64, %r11
	addq	$16, %rbx
	movaps	%xmm0, %xmm4
	movaps	%xmm1, %xmm2
	movups	-16(%r9), %xmm11
	shufps	$10, %xmm1, %xmm4
	shufps	$140, %xmm4, %xmm2
	movaps	%xmm3, %xmm4
	shufps	$165, %xmm2, %xmm4
	shufps	$36, %xmm4, %xmm2
	movaps	%xmm2, %xmm15
	movaps	%xmm1, %xmm2
	shufps	$193, %xmm0, %xmm2
	shufps	$165, %xmm1, %xmm0
	shufps	$2, %xmm1, %xmm0
	shufps	$196, %xmm3, %xmm0
	movaps	%xmm0, -8(%rsp)
	movups	-112(%r9), %xmm1
	movaps	%xmm3, %xmm4
	shufps	$120, %xmm2, %xmm2
	movups	-144(%r9), %xmm0
	movaps	%xmm5, %xmm3
	shufps	$170, %xmm2, %xmm4
	shufps	$36, %xmm4, %xmm2
	movaps	%xmm2, -24(%rsp)
	movaps	%xmm11, %xmm12
	movaps	%xmm0, %xmm10
	movaps	%xmm0, %xmm4
	shufps	$10, %xmm0, %xmm3
	movaps	%xmm15, 56(%rsp)
	shufps	$193, %xmm5, %xmm4
	shufps	$140, %xmm3, %xmm10
	movaps	%xmm4, %xmm2
	movaps	%xmm1, %xmm3
	shufps	$120, %xmm4, %xmm2
	shufps	$165, %xmm10, %xmm3
	shufps	$165, %xmm0, %xmm5
	movups	-80(%r9), %xmm4
	shufps	$36, %xmm3, %xmm10
	shufps	$2, %xmm0, %xmm5
	movaps	%xmm1, %xmm3
	movups	-96(%r9), %xmm0
	shufps	$170, %xmm2, %xmm3
	shufps	$36, %xmm3, %xmm2
	movups	-64(%r9), %xmm3
	shufps	$196, %xmm1, %xmm5
	mulps	-120(%rsp), %xmm15
	movaps	%xmm4, %xmm1
	movaps	%xmm0, %xmm9
	shufps	$10, %xmm0, %xmm1
	shufps	$140, %xmm1, %xmm9
	movaps	%xmm3, %xmm1
	movaps	%xmm3, %xmm6
	shufps	$165, %xmm9, %xmm1
	shufps	$36, %xmm1, %xmm9
	movaps	%xmm0, %xmm1
	shufps	$193, %xmm4, %xmm1
	shufps	$165, %xmm0, %xmm4
	shufps	$120, %xmm1, %xmm1
	shufps	$2, %xmm0, %xmm4
	shufps	$170, %xmm1, %xmm6
	shufps	$196, %xmm3, %xmm4
	shufps	$36, %xmm6, %xmm1
	movups	-32(%r9), %xmm3
	movups	-48(%r9), %xmm6
	movaps	%xmm3, %xmm0
	movaps	%xmm6, %xmm8
	shufps	$10, %xmm6, %xmm0
	shufps	$140, %xmm0, %xmm8
	movaps	%xmm11, %xmm0
	shufps	$165, %xmm8, %xmm0
	shufps	$36, %xmm0, %xmm8
	movaps	%xmm6, %xmm0
	shufps	$193, %xmm3, %xmm0
	shufps	$165, %xmm6, %xmm3
	shufps	$120, %xmm0, %xmm0
	shufps	$2, %xmm6, %xmm3
	shufps	$170, %xmm0, %xmm12
	shufps	$196, %xmm11, %xmm3
	shufps	$36, %xmm12, %xmm0
	movaps	40(%rsp), %xmm11
	movaps	-72(%rsp), %xmm12
	movaps	%xmm11, %xmm6
	mulps	%xmm10, %xmm6
	movaps	%xmm12, %xmm13
	mulps	%xmm2, %xmm13
	mulps	%xmm1, %xmm12
	addps	%xmm6, %xmm13
	movaps	-56(%rsp), %xmm6
	mulps	%xmm5, %xmm6
	addps	%xmm6, %xmm13
	movaps	%xmm11, %xmm6
	mulps	%xmm9, %xmm6
	addps	%xmm6, %xmm12
	movaps	-56(%rsp), %xmm6
	mulps	%xmm4, %xmm6
	addps	%xmm6, %xmm12
	movaps	%xmm11, %xmm6
	movaps	-72(%rsp), %xmm11
	mulps	%xmm8, %xmm6
	mulps	%xmm0, %xmm11
	addps	%xmm6, %xmm11
	movaps	-56(%rsp), %xmm6
	mulps	%xmm3, %xmm6
	addps	%xmm6, %xmm11
	movaps	-24(%rsp), %xmm6
	mulps	-104(%rsp), %xmm6
	addps	%xmm15, %xmm6
	movaps	-8(%rsp), %xmm15
	mulps	-88(%rsp), %xmm15
	addps	%xmm15, %xmm6
	movaps	-120(%rsp), %xmm15
	mulps	%xmm10, %xmm15
	mulps	%xmm7, %xmm6
	addps	-40(%rsp), %xmm6
	movups	%xmm6, -64(%r10)
	movaps	-104(%rsp), %xmm6
	mulps	%xmm2, %xmm6
	addps	%xmm15, %xmm6
	movaps	-88(%rsp), %xmm15
	mulps	%xmm5, %xmm15
	addps	%xmm15, %xmm6
	mulps	%xmm7, %xmm6
	addps	-40(%rsp), %xmm6
	movups	%xmm6, -48(%r10)
	movaps	-120(%rsp), %xmm15
	movaps	-104(%rsp), %xmm6
	mulps	24(%rsp), %xmm5
	mulps	%xmm9, %xmm15
	mulps	%xmm1, %xmm6
	addps	%xmm15, %xmm6
	movaps	-88(%rsp), %xmm15
	mulps	%xmm4, %xmm15
	addps	%xmm15, %xmm6
	movaps	-120(%rsp), %xmm15
	mulps	%xmm8, %xmm15
	mulps	%xmm7, %xmm6
	addps	-40(%rsp), %xmm6
	movups	%xmm6, -32(%r10)
	movaps	-104(%rsp), %xmm6
	mulps	%xmm0, %xmm6
	addps	%xmm15, %xmm6
	movaps	-88(%rsp), %xmm15
	mulps	%xmm3, %xmm15
	addps	%xmm15, %xmm6
	movaps	56(%rsp), %xmm15
	mulps	72(%rsp), %xmm15
	mulps	%xmm7, %xmm6
	addps	-40(%rsp), %xmm6
	movups	%xmm6, -16(%r10)
	movaps	-24(%rsp), %xmm6
	mulps	88(%rsp), %xmm6
	addps	%xmm15, %xmm6
	movaps	-8(%rsp), %xmm15
	mulps	24(%rsp), %xmm15
	addps	%xmm15, %xmm6
	movaps	%xmm14, %xmm15
	mulps	%xmm7, %xmm6
	subps	%xmm6, %xmm15
	movaps	88(%rsp), %xmm6
	mulps	%xmm6, %xmm1
	movups	%xmm15, -64(%r11)
	movaps	72(%rsp), %xmm15
	mulps	%xmm6, %xmm2
	mulps	%xmm6, %xmm0
	mulps	%xmm15, %xmm9
	mulps	%xmm15, %xmm10
	mulps	%xmm15, %xmm8
	addps	%xmm9, %xmm1
	movaps	24(%rsp), %xmm9
	addps	%xmm10, %xmm2
	mulps	%xmm9, %xmm4
	addps	%xmm8, %xmm0
	mulps	%xmm9, %xmm3
	addps	%xmm5, %xmm2
	movaps	%xmm14, %xmm5
	addps	%xmm4, %xmm1
	mulps	%xmm7, %xmm2
	movaps	%xmm14, %xmm4
	addps	%xmm3, %xmm0
	pxor	%xmm3, %xmm3
	cmpleps	%xmm11, %xmm3
	mulps	%xmm7, %xmm1
	mulps	%xmm7, %xmm0
	subps	%xmm2, %xmm5
	movaps	%xmm14, %xmm2
	subps	%xmm1, %xmm2
	movaps	56(%rsp), %xmm1
	mulps	40(%rsp), %xmm1
	movups	%xmm5, -48(%r11)
	subps	%xmm0, %xmm4
	movaps	-24(%rsp), %xmm0
	mulps	-72(%rsp), %xmm0
	movups	%xmm2, -32(%r11)
	pxor	%xmm2, %xmm2
	cmpleps	%xmm13, %xmm2
	pand	.LC3(%rip), %xmm2
	movups	%xmm4, -16(%r11)
	addps	%xmm1, %xmm0
	movaps	-8(%rsp), %xmm1
	mulps	-56(%rsp), %xmm1
	addps	%xmm0, %xmm1
	pxor	%xmm0, %xmm0
	cmpleps	%xmm1, %xmm0
	pand	.LC3(%rip), %xmm0
	pxor	%xmm1, %xmm1
	cmpleps	%xmm12, %xmm1
	pand	.LC3(%rip), %xmm1
	pand	.LC3(%rip), %xmm3
	movdqa	%xmm0, %xmm4
	punpcklwd	%xmm2, %xmm0
	punpckhwd	%xmm2, %xmm4
	movdqa	%xmm0, %xmm2
	punpckhwd	%xmm4, %xmm2
	punpcklwd	%xmm4, %xmm0
	punpcklwd	%xmm2, %xmm0
	movdqa	%xmm1, %xmm2
	punpcklwd	%xmm3, %xmm1
	punpckhwd	%xmm3, %xmm2
	movdqa	%xmm1, %xmm3
	pand	.LC1(%rip), %xmm0
	punpckhwd	%xmm2, %xmm3
	punpcklwd	%xmm2, %xmm1
	punpcklwd	%xmm3, %xmm1
	pand	.LC1(%rip), %xmm1
	packuswb	%xmm1, %xmm0
	movups	%xmm0, -16(%rbx)
	cmpq	%r10, %rbp
	jne	.L26
	movl	%esi, %r9d
	movss	112(%rsp), %xmm6
	movss	120(%rsp), %xmm5
	andl	$-16, %r9d
	movss	124(%rsp), %xmm4
	movss	128(%rsp), %xmm3
	movss	132(%rsp), %xmm2
	movl	%r9d, %r10d
	movss	136(%rsp), %xmm1
	movss	140(%rsp), %xmm10
	movss	144(%rsp), %xmm9
	movss	148(%rsp), %xmm8
	cmpl	%r9d, %esi
	je	.L22
	movl	%esi, %edi
	subl	%r9d, %edi
	leal	-1(%rdi), %r11d
	cmpl	$6, %r11d
	jbe	.L28
.L25:
	movl	%r10d, %ebx
	leaq	(%rbx,%rbx,2), %r10
	leaq	(%rax,%r10,4), %rbp
	leaq	0(,%rbx,4), %r10
	movq	0(%rbp), %xmm11
	movq	8(%rbp), %xmm0
	leaq	(%rdx,%r10), %r11
	addq	%rcx, %r10
	movq	16(%rbp), %xmm12
	movaps	%xmm11, %xmm13
	movaps	%xmm0, %xmm7
	unpcklps	%xmm11, %xmm0
	shufps	$0xe1, %xmm13, %xmm13
	movss	%xmm11, %xmm7
	movq	24(%rbp), %xmm11
	unpcklps	%xmm12, %xmm13
	movss	%xmm0, %xmm12
	movq	32(%rbp), %xmm0
	movlps	%xmm12, -88(%rsp)
	movq	40(%rbp), %xmm12
	movaps	%xmm0, %xmm14
	movlps	%xmm13, -104(%rsp)
	unpcklps	%xmm11, %xmm0
	movaps	%xmm11, %xmm13
	movss	%xmm11, %xmm14
	movaps	%xmm12, %xmm11
	shufps	$0xe1, %xmm13, %xmm13
	movss	%xmm0, %xmm11
	unpcklps	%xmm12, %xmm13
	movq	56(%rbp), %xmm0
	movq	64(%rbp), %xmm12
	movq	%xmm11, %r14
	movq	48(%rbp), %xmm11
	movq	%xmm13, %r13
	movlps	%xmm14, 24(%rsp)
	movaps	%xmm0, %xmm15
	movaps	%xmm11, %xmm13
	unpcklps	%xmm11, %xmm0
	movss	%xmm11, %xmm15
	movq	72(%rbp), %xmm11
	shufps	$0xe1, %xmm13, %xmm13
	movq	%xmm15, %r15
	unpcklps	%xmm12, %xmm13
	movss	%xmm0, %xmm12
	movq	80(%rbp), %xmm0
	movlps	%xmm12, -56(%rsp)
	movq	88(%rbp), %xmm12
	movaps	%xmm0, %xmm15
	movlps	%xmm13, -72(%rsp)
	unpcklps	%xmm11, %xmm0
	movss	%xmm11, %xmm15
	movlps	%xmm15, -40(%rsp)
	movaps	%xmm11, %xmm15
	movq	%r13, %xmm11
	shufps	$0xe1, %xmm15, %xmm15
	unpcklps	%xmm12, %xmm15
	movaps	%xmm15, %xmm13
	movaps	%xmm12, %xmm15
	movss	%xmm0, %xmm15
	movaps	%xmm14, %xmm0
	movlps	%xmm13, 40(%rsp)
	movlps	%xmm15, -24(%rsp)
	movss	104(%rsp), %xmm15
	shufps	$0xe0, %xmm15, %xmm15
	mulps	%xmm15, %xmm0
	movaps	%xmm15, %xmm14
	movaps	%xmm10, %xmm15
	shufps	$0xe0, %xmm15, %xmm15
	mulps	%xmm15, %xmm11
	movaps	%xmm15, %xmm12
	movaps	%xmm9, %xmm15
	shufps	$0xe0, %xmm15, %xmm15
	movlps	%xmm14, 72(%rsp)
	movlps	%xmm12, 88(%rsp)
	addps	%xmm11, %xmm0
	movq	%r14, %xmm11
	mulps	%xmm15, %xmm11
	addps	%xmm11, %xmm0
	movq	%r15, %xmm11
	mulps	%xmm14, %xmm11
	movlps	%xmm0, 56(%rsp)
	movq	-72(%rsp), %xmm0
	mulps	%xmm12, %xmm0
	addps	%xmm11, %xmm0
	movq	-56(%rsp), %xmm11
	mulps	%xmm15, %xmm11
	addps	%xmm11, %xmm0
	movq	-40(%rsp), %xmm11
	mulps	%xmm14, %xmm11
	movaps	%xmm13, %xmm14
	movaps	%xmm4, %xmm13
	mulps	%xmm12, %xmm14
	movq	%xmm0, %rbp
	movq	-104(%rsp), %xmm12
	shufps	$0xe0, %xmm13, %xmm13
	movaps	%xmm14, %xmm0
	addps	%xmm11, %xmm0
	movq	-24(%rsp), %xmm11
	mulps	%xmm15, %xmm11
	addps	%xmm11, %xmm0
	movaps	%xmm7, %xmm11
	movq	%xmm0, %r12
	movaps	%xmm6, %xmm0
	shufps	$0xe0, %xmm0, %xmm0
	mulps	%xmm0, %xmm11
	movaps	%xmm0, %xmm14
	movaps	%xmm5, %xmm0
	shufps	$0xe0, %xmm0, %xmm0
	mulps	%xmm0, %xmm12
	addps	%xmm12, %xmm11
	movq	-88(%rsp), %xmm12
	mulps	%xmm13, %xmm12
	addps	%xmm11, %xmm12
	movaps	%xmm8, %xmm11
	shufps	$0xe0, %xmm11, %xmm11
	movlps	%xmm11, -120(%rsp)
	mulps	%xmm11, %xmm12
	movss	20(%rsp), %xmm11
	shufps	$0xe0, %xmm11, %xmm11
	movlps	%xmm11, -8(%rsp)
	addps	%xmm11, %xmm12
	movlps	%xmm12, (%r11)
	movq	24(%rsp), %xmm12
	mulps	%xmm14, %xmm12
	movaps	%xmm12, %xmm11
	movq	%r13, %xmm12
	mulps	%xmm0, %xmm12
	addps	%xmm11, %xmm12
	movq	%r14, %xmm11
	mulps	%xmm13, %xmm11
	addps	%xmm11, %xmm12
	movq	-120(%rsp), %xmm11
	mulps	%xmm11, %xmm12
	movq	-8(%rsp), %xmm11
	addps	%xmm11, %xmm12
	movq	-72(%rsp), %xmm11
	mulps	%xmm0, %xmm11
	movlps	%xmm12, 8(%r11)
	movq	%r15, %xmm12
	mulps	%xmm14, %xmm12
	movlps	%xmm12, 112(%rsp)
	movaps	%xmm11, %xmm12
	movq	112(%rsp), %xmm11
	addps	%xmm11, %xmm12
	movq	-56(%rsp), %xmm11
	mulps	%xmm13, %xmm11
	addps	%xmm11, %xmm12
	movq	-120(%rsp), %xmm11
	mulps	%xmm11, %xmm12
	movq	-8(%rsp), %xmm11
	addps	%xmm11, %xmm12
	movq	-40(%rsp), %xmm11
	mulps	%xmm11, %xmm14
	movq	40(%rsp), %xmm11
	movlps	%xmm12, 16(%r11)
	mulps	%xmm11, %xmm0
	movq	-120(%rsp), %xmm11
	addps	%xmm14, %xmm0
	movq	-24(%rsp), %xmm14
	mulps	%xmm14, %xmm13
	movq	-8(%rsp), %xmm14
	addps	%xmm13, %xmm0
	movaps	%xmm7, %xmm13
	mulps	%xmm11, %xmm0
	addps	%xmm14, %xmm0
	movlps	%xmm0, 24(%r11)
	movaps	%xmm3, %xmm0
	shufps	$0xe0, %xmm0, %xmm0
	mulps	%xmm0, %xmm13
	movaps	%xmm0, %xmm12
	movaps	%xmm2, %xmm0
	shufps	$0xe0, %xmm0, %xmm0
	movq	%xmm0, %r11
	movq	-104(%rsp), %xmm0
	movq	%r11, %xmm14
	mulps	%xmm14, %xmm0
	addps	%xmm0, %xmm13
	movaps	%xmm1, %xmm0
	shufps	$0xe0, %xmm0, %xmm0
	movaps	%xmm0, %xmm14
	movq	-88(%rsp), %xmm0
	mulps	%xmm14, %xmm0
	addps	%xmm0, %xmm13
	movss	108(%rsp), %xmm0
	shufps	$0xe0, %xmm0, %xmm0
	mulps	%xmm11, %xmm13
	movaps	%xmm0, %xmm11
	subps	%xmm13, %xmm11
	movq	24(%rsp), %xmm13
	mulps	%xmm12, %xmm13
	movlps	%xmm11, (%r10)
	movq	%r11, %xmm11
	movlps	%xmm13, -8(%rsp)
	movq	%r13, %xmm13
	mulps	%xmm11, %xmm13
	movq	-8(%rsp), %xmm11
	addps	%xmm11, %xmm13
	movq	%r14, %xmm11
	mulps	%xmm14, %xmm11
	addps	%xmm11, %xmm13
	movq	-120(%rsp), %xmm11
	mulps	%xmm11, %xmm13
	movaps	%xmm0, %xmm11
	subps	%xmm13, %xmm11
	movq	%r15, %xmm13
	mulps	%xmm12, %xmm13
	movlps	%xmm11, 8(%r10)
	movq	%r11, %xmm11
	movq	%xmm13, %r15
	movq	-72(%rsp), %xmm13
	mulps	%xmm11, %xmm13
	movq	%r15, %xmm11
	addps	%xmm11, %xmm13
	movq	-56(%rsp), %xmm11
	mulps	%xmm14, %xmm11
	addps	%xmm11, %xmm13
	movq	-120(%rsp), %xmm11
	mulps	%xmm11, %xmm13
	movaps	%xmm0, %xmm11
	subps	%xmm13, %xmm11
	movlps	%xmm11, 16(%r10)
	movq	-40(%rsp), %xmm11
	movq	40(%rsp), %xmm13
	mulps	%xmm11, %xmm12
	movq	%r11, %xmm11
	mulps	%xmm11, %xmm13
	movq	-24(%rsp), %xmm11
	mulps	%xmm11, %xmm14
	movq	-120(%rsp), %xmm11
	addps	%xmm12, %xmm13
	movq	88(%rsp), %xmm12
	addps	%xmm14, %xmm13
	movq	72(%rsp), %xmm14
	mulps	%xmm14, %xmm7
	mulps	%xmm11, %xmm13
	movaps	%xmm7, %xmm11
	movq	56(%rsp), %xmm7
	subps	%xmm13, %xmm0
	xorps	%xmm13, %xmm13
	movaps	%xmm13, %xmm14
	cmpleps	%xmm7, %xmm14
	movq	%rbp, %xmm7
	movlps	%xmm0, 24(%r10)
	movq	-104(%rsp), %xmm0
	movl	%edi, %r10d
	andl	$-8, %r10d
	mulps	%xmm12, %xmm0
	addl	%r10d, %r9d
	andl	$7, %edi
	movq	.LC4(%rip), %xmm12
	pand	%xmm12, %xmm14
	addps	%xmm0, %xmm11
	movq	-88(%rsp), %xmm0
	mulps	%xmm0, %xmm15
	movaps	%xmm13, %xmm0
	addps	%xmm15, %xmm11
	cmpleps	%xmm11, %xmm0
	movaps	%xmm13, %xmm11
	cmpleps	%xmm7, %xmm11
	movq	%r12, %xmm7
	cmpleps	%xmm7, %xmm13
	pand	%xmm12, %xmm0
	pand	%xmm12, %xmm11
	pand	%xmm13, %xmm12
	movdqa	%xmm0, %xmm13
	punpcklwd	%xmm14, %xmm0
	punpcklwd	%xmm14, %xmm13
	pshufd	$78, %xmm13, %xmm13
	punpcklwd	%xmm13, %xmm0
	movdqa	%xmm11, %xmm13
	punpcklwd	%xmm12, %xmm11
	punpcklwd	%xmm12, %xmm13
	movq	.LC5(%rip), %xmm12
	pshufd	$78, %xmm13, %xmm13
	punpcklwd	%xmm13, %xmm11
	pand	%xmm12, %xmm0
	pand	%xmm11, %xmm12
	packuswb	%xmm12, %xmm0
	pshufd	$8, %xmm0, %xmm0
	movq	%xmm0, (%r8,%rbx)
	je	.L22
.L28:
	movslq	%r9d, %rbx
	movaps	%xmm6, %xmm14
	movaps	%xmm5, %xmm11
	movss	108(%rsp), %xmm7
	leaq	(%rbx,%rbx,2), %r11
	salq	$2, %r11
	leaq	(%rax,%r11), %r10
	movss	(%r10), %xmm0
	movss	4(%r10), %xmm13
	movss	8(%r10), %xmm12
	leaq	0(,%rbx,4), %r10
	mulss	%xmm0, %xmm14
	mulss	%xmm13, %xmm11
	addss	%xmm14, %xmm11
	movaps	%xmm4, %xmm14
	mulss	%xmm12, %xmm14
	addss	%xmm14, %xmm11
	movaps	%xmm0, %xmm14
	mulss	%xmm3, %xmm14
	mulss	104(%rsp), %xmm0
	mulss	%xmm8, %xmm11
	addss	20(%rsp), %xmm11
	movss	%xmm11, (%rdx,%rbx,4)
	movaps	%xmm13, %xmm11
	mulss	%xmm2, %xmm11
	mulss	%xmm10, %xmm13
	addss	%xmm14, %xmm11
	movaps	%xmm12, %xmm14
	mulss	%xmm1, %xmm14
	addss	%xmm13, %xmm0
	mulss	%xmm9, %xmm12
	addss	%xmm14, %xmm11
	movaps	%xmm7, %xmm14
	addss	%xmm12, %xmm0
	pxor	%xmm12, %xmm12
	mulss	%xmm8, %xmm11
	comiss	%xmm12, %xmm0
	subss	%xmm11, %xmm14
	movss	%xmm14, (%rcx,%rbx,4)
	setnb	(%r8,%rbx)
	leal	1(%r9), %ebx
	cmpl	%ebx, %esi
	jle	.L22
	leaq	12(%rax,%r11), %rbp
	movaps	%xmm6, %xmm15
	movaps	%xmm5, %xmm11
	movslq	%ebx, %rbx
	movss	0(%rbp), %xmm0
	movss	4(%rbp), %xmm14
	movss	8(%rbp), %xmm13
	mulss	%xmm0, %xmm15
	mulss	%xmm14, %xmm11
	addss	%xmm15, %xmm11
	movaps	%xmm4, %xmm15
	mulss	%xmm13, %xmm15
	addss	%xmm15, %xmm11
	movaps	%xmm3, %xmm15
	mulss	%xmm0, %xmm15
	mulss	104(%rsp), %xmm0
	mulss	%xmm8, %xmm11
	addss	20(%rsp), %xmm11
	movss	%xmm11, 4(%rdx,%r10)
	movaps	%xmm2, %xmm11
	mulss	%xmm14, %xmm11
	mulss	%xmm10, %xmm14
	addss	%xmm15, %xmm11
	movaps	%xmm1, %xmm15
	mulss	%xmm13, %xmm15
	addss	%xmm14, %xmm0
	mulss	%xmm9, %xmm13
	addss	%xmm15, %xmm11
	movaps	%xmm7, %xmm15
	addss	%xmm13, %xmm0
	mulss	%xmm8, %xmm11
	comiss	%xmm12, %xmm0
	subss	%xmm11, %xmm15
	movss	%xmm15, 4(%rcx,%r10)
	setnb	(%r8,%rbx)
	leal	2(%r9), %ebx
	cmpl	%ebx, %esi
	jle	.L22
	leaq	24(%rax,%r11), %rbp
	movaps	%xmm6, %xmm15
	movaps	%xmm5, %xmm11
	movslq	%ebx, %rbx
	movss	0(%rbp), %xmm14
	movss	4(%rbp), %xmm0
	movss	8(%rbp), %xmm13
	mulss	%xmm14, %xmm15
	mulss	%xmm0, %xmm11
	addss	%xmm15, %xmm11
	movaps	%xmm4, %xmm15
	mulss	%xmm13, %xmm15
	addss	%xmm15, %xmm11
	movaps	%xmm3, %xmm15
	mulss	%xmm14, %xmm15
	mulss	104(%rsp), %xmm14
	mulss	%xmm8, %xmm11
	addss	20(%rsp), %xmm11
	movss	%xmm11, 8(%rdx,%r10)
	movaps	%xmm2, %xmm11
	mulss	%xmm0, %xmm11
	mulss	%xmm10, %xmm0
	addss	%xmm15, %xmm11
	movaps	%xmm1, %xmm15
	mulss	%xmm13, %xmm15
	addss	%xmm14, %xmm0
	mulss	%xmm9, %xmm13
	addss	%xmm15, %xmm11
	movaps	%xmm7, %xmm15
	addss	%xmm13, %xmm0
	mulss	%xmm8, %xmm11
	comiss	%xmm12, %xmm0
	subss	%xmm11, %xmm15
	movss	%xmm15, 8(%rcx,%r10)
	setnb	(%r8,%rbx)
	leal	3(%r9), %ebx
	cmpl	%ebx, %esi
	jle	.L22
	leaq	36(%rax,%r11), %rbp
	movaps	%xmm6, %xmm15
	movaps	%xmm5, %xmm11
	movslq	%ebx, %rbx
	movss	0(%rbp), %xmm0
	movss	4(%rbp), %xmm14
	movss	8(%rbp), %xmm13
	mulss	%xmm0, %xmm15
	mulss	%xmm14, %xmm11
	addss	%xmm15, %xmm11
	movaps	%xmm4, %xmm15
	mulss	%xmm13, %xmm15
	addss	%xmm15, %xmm11
	movaps	%xmm3, %xmm15
	mulss	%xmm0, %xmm15
	mulss	104(%rsp), %xmm0
	mulss	%xmm8, %xmm11
	addss	20(%rsp), %xmm11
	movss	%xmm11, 12(%rdx,%r10)
	movaps	%xmm2, %xmm11
	mulss	%xmm14, %xmm11
	mulss	%xmm10, %xmm14
	addss	%xmm15, %xmm11
	movaps	%xmm1, %xmm15
	mulss	%xmm13, %xmm15
	addss	%xmm14, %xmm0
	mulss	%xmm9, %xmm13
	addss	%xmm15, %xmm11
	movaps	%xmm7, %xmm15
	addss	%xmm13, %xmm0
	mulss	%xmm8, %xmm11
	comiss	%xmm12, %xmm0
	subss	%xmm11, %xmm15
	movss	%xmm15, 12(%rcx,%r10)
	setnb	(%r8,%rbx)
	leal	4(%r9), %ebx
	cmpl	%ebx, %esi
	jle	.L22
	leaq	48(%rax,%r11), %rbp
	movaps	%xmm6, %xmm15
	movaps	%xmm5, %xmm11
	movslq	%ebx, %rbx
	movss	0(%rbp), %xmm0
	movss	4(%rbp), %xmm14
	movss	8(%rbp), %xmm13
	mulss	%xmm0, %xmm15
	mulss	%xmm14, %xmm11
	addss	%xmm15, %xmm11
	movaps	%xmm4, %xmm15
	mulss	%xmm13, %xmm15
	addss	%xmm15, %xmm11
	movaps	%xmm3, %xmm15
	mulss	%xmm0, %xmm15
	mulss	104(%rsp), %xmm0
	mulss	%xmm8, %xmm11
	addss	20(%rsp), %xmm11
	movss	%xmm11, 16(%rdx,%r10)
	movaps	%xmm2, %xmm11
	mulss	%xmm14, %xmm11
	mulss	%xmm10, %xmm14
	addss	%xmm15, %xmm11
	movaps	%xmm1, %xmm15
	mulss	%xmm13, %xmm15
	addss	%xmm14, %xmm0
	mulss	%xmm9, %xmm13
	addss	%xmm15, %xmm11
	movaps	%xmm7, %xmm15
	addss	%xmm13, %xmm0
	mulss	%xmm8, %xmm11
	comiss	%xmm12, %xmm0
	subss	%xmm11, %xmm15
	movss	%xmm15, 16(%rcx,%r10)
	setnb	(%r8,%rbx)
	leal	5(%r9), %ebx
	cmpl	%ebx, %esi
	jle	.L22
	leaq	60(%rax,%r11), %rbp
	movaps	%xmm6, %xmm15
	movaps	%xmm5, %xmm11
	movslq	%ebx, %rbx
	movss	0(%rbp), %xmm0
	movss	4(%rbp), %xmm14
	movss	8(%rbp), %xmm13
	mulss	%xmm0, %xmm15
	mulss	%xmm14, %xmm11
	addss	%xmm15, %xmm11
	movaps	%xmm4, %xmm15
	mulss	%xmm13, %xmm15
	addss	%xmm15, %xmm11
	movaps	%xmm3, %xmm15
	mulss	%xmm0, %xmm15
	mulss	%xmm8, %xmm11
	addss	20(%rsp), %xmm11
	movss	%xmm11, 20(%rdx,%r10)
	movaps	%xmm2, %xmm11
	mulss	%xmm14, %xmm11
	mulss	%xmm10, %xmm14
	addss	%xmm15, %xmm11
	movaps	%xmm1, %xmm15
	mulss	%xmm13, %xmm15
	mulss	%xmm9, %xmm13
	addss	%xmm15, %xmm11
	movaps	%xmm7, %xmm15
	mulss	%xmm8, %xmm11
	subss	%xmm11, %xmm15
	movss	%xmm15, 20(%rcx,%r10)
	movss	104(%rsp), %xmm15
	mulss	%xmm15, %xmm0
	addss	%xmm14, %xmm0
	addss	%xmm13, %xmm0
	comiss	%xmm12, %xmm0
	setnb	(%r8,%rbx)
	addl	$6, %r9d
	cmpl	%r9d, %esi
	jle	.L22
	leaq	72(%rax,%r11), %rax
	movslq	%r9d, %r9
	movss	(%rax), %xmm0
	movss	4(%rax), %xmm13
	movss	8(%rax), %xmm11
	mulss	%xmm0, %xmm3
	mulss	%xmm13, %xmm2
	mulss	%xmm11, %xmm1
	mulss	%xmm0, %xmm6
	mulss	%xmm13, %xmm5
	mulss	%xmm15, %xmm0
	mulss	%xmm10, %xmm13
	addss	%xmm3, %xmm2
	mulss	%xmm11, %xmm4
	mulss	%xmm9, %xmm11
	addss	%xmm2, %xmm1
	addss	%xmm6, %xmm5
	addss	%xmm13, %xmm0
	mulss	%xmm8, %xmm1
	addss	%xmm5, %xmm4
	addss	%xmm11, %xmm0
	mulss	%xmm8, %xmm4
	addss	20(%rsp), %xmm4
	subss	%xmm1, %xmm7
	comiss	%xmm12, %xmm0
	movss	%xmm4, 24(%rdx,%r10)
	movss	%xmm7, 24(%rcx,%r10)
	setnb	(%r8,%r9)
.L22:
	addq	$160, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L24:
	.cfi_restore_state
	movss	108(%rsp), %xmm13
	xorl	%esi, %esi
	pxor	%xmm14, %xmm14
	.p2align 4,,10
	.p2align 3
.L30:
	movss	(%rax), %xmm0
	movss	4(%rax), %xmm12
	movaps	%xmm6, %xmm15
	movaps	%xmm5, %xmm7
	movss	8(%rax), %xmm11
	mulss	%xmm0, %xmm15
	mulss	%xmm12, %xmm7
	addss	%xmm15, %xmm7
	movaps	%xmm4, %xmm15
	mulss	%xmm11, %xmm15
	addss	%xmm15, %xmm7
	movaps	%xmm3, %xmm15
	mulss	%xmm0, %xmm15
	mulss	104(%rsp), %xmm0
	mulss	%xmm8, %xmm7
	addss	20(%rsp), %xmm7
	movss	%xmm7, (%rdx,%rsi,4)
	movaps	%xmm2, %xmm7
	mulss	%xmm12, %xmm7
	mulss	%xmm10, %xmm12
	addss	%xmm15, %xmm7
	movaps	%xmm1, %xmm15
	mulss	%xmm11, %xmm15
	addss	%xmm12, %xmm0
	mulss	%xmm9, %xmm11
	addss	%xmm15, %xmm7
	movaps	%xmm13, %xmm15
	addss	%xmm11, %xmm0
	mulss	%xmm8, %xmm7
	comiss	%xmm14, %xmm0
	subss	%xmm7, %xmm15
	movss	%xmm15, (%rcx,%rsi,4)
	setnb	(%r8,%rsi)
	addq	$1, %rsi
	addq	$12, %rax
	cmpq	%rsi, %r9
	jne	.L30
	jmp	.L22
.L32:
	xorl	%r10d, %r10d
	xorl	%r9d, %r9d
	jmp	.L25
	.cfi_endproc
.LFE1:
	.size	project_aos_float, .-project_aos_float
	.p2align 4
	.globl	project_soa_float
	.type	project_soa_float, @function
project_soa_float:
.LFB2:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$48, %rsp
	.cfi_def_cfa_offset 104
	movq	152(%rsp), %r11
	movss	148(%rsp), %xmm1
	testl	%ecx, %ecx
	jle	.L47
	movss	104(%rsp), %xmm5
	movss	108(%rsp), %xmm4
	movq	%rdx, %rax
	leal	-1(%rcx), %edx
	movss	112(%rsp), %xmm6
	movss	116(%rsp), %xmm3
	movl	%ecx, %r10d
	movss	120(%rsp), %xmm2
	movss	124(%rsp), %xmm0
	movss	%xmm5, -20(%rsp)
	movss	128(%rsp), %xmm14
	movss	%xmm4, -16(%rsp)
	movss	132(%rsp), %xmm9
	movss	136(%rsp), %xmm10
	movss	%xmm6, -12(%rsp)
	movss	140(%rsp), %xmm8
	movss	144(%rsp), %xmm15
	movss	%xmm3, -8(%rsp)
	movss	%xmm2, -4(%rsp)
	movss	%xmm0, (%rsp)
	movss	%xmm14, 4(%rsp)
	movss	%xmm9, -28(%rsp)
	movss	%xmm10, -24(%rsp)
	movss	%xmm8, 8(%rsp)
	movss	%xmm15, 12(%rsp)
	cmpl	$14, %edx
	jbe	.L54
	movaps	%xmm5, %xmm7
	shrl	$4, %ecx
	movaps	%xmm10, %xmm13
	movaps	%xmm8, %xmm11
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, -120(%rsp)
	movaps	%xmm4, %xmm7
	movaps	%xmm2, %xmm5
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, -104(%rsp)
	movaps	%xmm6, %xmm7
	movaps	%xmm3, %xmm4
	shufps	$0, %xmm7, %xmm7
	movaps	%xmm7, -88(%rsp)
	movaps	%xmm14, %xmm7
	movaps	%xmm9, %xmm14
	movaps	%xmm0, %xmm6
	movaps	%xmm1, %xmm12
	movaps	%xmm15, %xmm10
	salq	$4, %rcx
	xorl	%edx, %edx
	pxor	%xmm8, %xmm8
	movss	%xmm1, 16(%rsp)
	movdqa	.LC3(%rip), %xmm9
	shufps	$0, %xmm4, %xmm4
	shufps	$0, %xmm5, %xmm5
	shufps	$0, %xmm6, %xmm6
	shufps	$0, %xmm7, %xmm7
	shufps	$0, %xmm14, %xmm14
	shufps	$0, %xmm13, %xmm13
	shufps	$0, %xmm12, %xmm12
	shufps	$0, %xmm11, %xmm11
	shufps	$0, %xmm10, %xmm10
	.p2align 4,,10
	.p2align 3
.L50:
	movups	16(%rdi,%rdx,4), %xmm1
	movups	16(%rsi,%rdx,4), %xmm0
	movups	16(%rax,%rdx,4), %xmm3
	movups	32(%rdi,%rdx,4), %xmm2
	mulps	%xmm7, %xmm1
	mulps	%xmm14, %xmm0
	mulps	%xmm7, %xmm2
	addps	%xmm1, %xmm0
	movaps	%xmm3, %xmm1
	mulps	%xmm13, %xmm1
	addps	%xmm1, %xmm0
	movaps	%xmm2, %xmm1
	movups	32(%rsi,%rdx,4), %xmm2
	mulps	%xmm14, %xmm2
	movaps	%xmm0, -72(%rsp)
	movaps	%xmm2, %xmm0
	movups	32(%rax,%rdx,4), %xmm2
	addps	%xmm1, %xmm0
	mulps	%xmm13, %xmm2
	movaps	%xmm2, %xmm1
	movaps	%xmm0, %xmm2
	movups	48(%rsi,%rdx,4), %xmm0
	addps	%xmm1, %xmm2
	movups	48(%rdi,%rdx,4), %xmm1
	mulps	%xmm14, %xmm0
	mulps	%xmm7, %xmm1
	movaps	%xmm2, -56(%rsp)
	movups	(%rsi,%rdx,4), %xmm2
	mulps	-104(%rsp), %xmm2
	addps	%xmm1, %xmm0
	movups	48(%rax,%rdx,4), %xmm1
	mulps	%xmm13, %xmm1
	movaps	%xmm0, %xmm15
	movaps	%xmm2, %xmm0
	movups	(%rax,%rdx,4), %xmm2
	mulps	-88(%rsp), %xmm2
	addps	%xmm1, %xmm15
	movups	(%rdi,%rdx,4), %xmm1
	mulps	-120(%rsp), %xmm1
	addps	%xmm1, %xmm0
	addps	%xmm2, %xmm0
	movups	16(%rdi,%rdx,4), %xmm2
	mulps	-120(%rsp), %xmm2
	mulps	%xmm12, %xmm0
	movaps	%xmm2, %xmm1
	movups	16(%rsi,%rdx,4), %xmm2
	mulps	-104(%rsp), %xmm2
	addps	%xmm11, %xmm0
	movups	%xmm0, (%r8,%rdx,4)
	movaps	%xmm2, %xmm0
	movups	32(%rdi,%rdx,4), %xmm2
	addps	%xmm1, %xmm0
	movaps	-88(%rsp), %xmm1
	mulps	-120(%rsp), %xmm2
	mulps	%xmm3, %xmm1
	mulps	%xmm6, %xmm3
	addps	%xmm1, %xmm0
	movaps	%xmm2, %xmm1
	movups	32(%rsi,%rdx,4), %xmm2
	mulps	-104(%rsp), %xmm2
	mulps	%xmm12, %xmm0
	addps	%xmm11, %xmm0
	movups	%xmm0, 16(%r8,%rdx,4)
	movaps	%xmm2, %xmm0
	movups	32(%rax,%rdx,4), %xmm2
	mulps	-88(%rsp), %xmm2
	addps	%xmm1, %xmm0
	addps	%xmm2, %xmm0
	movups	48(%rdi,%rdx,4), %xmm2
	mulps	%xmm12, %xmm0
	addps	%xmm11, %xmm0
	movups	%xmm0, 32(%r8,%rdx,4)
	mulps	-120(%rsp), %xmm2
	movaps	%xmm2, %xmm1
	movups	48(%rsi,%rdx,4), %xmm2
	mulps	-104(%rsp), %xmm2
	movaps	%xmm2, %xmm0
	movups	48(%rax,%rdx,4), %xmm2
	mulps	-88(%rsp), %xmm2
	addps	%xmm1, %xmm0
	addps	%xmm2, %xmm0
	movups	(%rdi,%rdx,4), %xmm2
	mulps	%xmm4, %xmm2
	mulps	%xmm12, %xmm0
	movaps	%xmm2, %xmm1
	movups	(%rsi,%rdx,4), %xmm2
	addps	%xmm11, %xmm0
	mulps	%xmm5, %xmm2
	movups	%xmm0, 48(%r8,%rdx,4)
	movaps	%xmm2, %xmm0
	movups	(%rax,%rdx,4), %xmm2
	addps	%xmm1, %xmm0
	mulps	%xmm6, %xmm2
	addps	%xmm2, %xmm0
	movaps	%xmm10, %xmm2
	mulps	%xmm12, %xmm0
	subps	%xmm0, %xmm2
	movups	%xmm2, (%r9,%rdx,4)
	movups	16(%rdi,%rdx,4), %xmm2
	mulps	%xmm4, %xmm2
	movaps	%xmm2, %xmm1
	movups	16(%rsi,%rdx,4), %xmm2
	mulps	%xmm5, %xmm2
	movaps	%xmm2, %xmm0
	movaps	%xmm8, %xmm2
	addps	%xmm1, %xmm0
	cmpleps	%xmm15, %xmm2
	addps	%xmm3, %xmm0
	movaps	%xmm10, %xmm3
	pand	%xmm9, %xmm2
	mulps	%xmm12, %xmm0
	subps	%xmm0, %xmm3
	movups	%xmm3, 16(%r9,%rdx,4)
	movups	32(%rdi,%rdx,4), %xmm3
	mulps	%xmm4, %xmm3
	movaps	%xmm3, %xmm1
	movups	32(%rsi,%rdx,4), %xmm3
	mulps	%xmm5, %xmm3
	movaps	%xmm3, %xmm0
	movups	32(%rax,%rdx,4), %xmm3
	addps	%xmm1, %xmm0
	mulps	%xmm6, %xmm3
	addps	%xmm3, %xmm0
	movaps	%xmm10, %xmm3
	mulps	%xmm12, %xmm0
	subps	%xmm0, %xmm3
	movups	%xmm3, 32(%r9,%rdx,4)
	movups	48(%rdi,%rdx,4), %xmm3
	mulps	%xmm4, %xmm3
	movaps	%xmm3, %xmm1
	movups	48(%rsi,%rdx,4), %xmm3
	mulps	%xmm5, %xmm3
	movaps	%xmm3, %xmm0
	movups	48(%rax,%rdx,4), %xmm3
	addps	%xmm1, %xmm0
	mulps	%xmm6, %xmm3
	addps	%xmm3, %xmm0
	movaps	%xmm10, %xmm3
	mulps	%xmm12, %xmm0
	subps	%xmm0, %xmm3
	movups	%xmm3, 48(%r9,%rdx,4)
	movups	(%rdi,%rdx,4), %xmm3
	mulps	%xmm7, %xmm3
	movaps	%xmm3, %xmm1
	movups	(%rsi,%rdx,4), %xmm3
	mulps	%xmm14, %xmm3
	movaps	%xmm3, %xmm0
	movups	(%rax,%rdx,4), %xmm3
	addps	%xmm1, %xmm0
	mulps	%xmm13, %xmm3
	movaps	%xmm3, %xmm1
	movaps	%xmm8, %xmm3
	addps	%xmm0, %xmm1
	cmpleps	%xmm1, %xmm3
	movaps	%xmm8, %xmm1
	cmpleps	-56(%rsp), %xmm1
	movaps	%xmm3, %xmm0
	movaps	%xmm8, %xmm3
	cmpleps	-72(%rsp), %xmm3
	pand	%xmm9, %xmm0
	pand	%xmm9, %xmm1
	movdqa	%xmm0, %xmm15
	pand	%xmm9, %xmm3
	punpcklwd	%xmm3, %xmm0
	punpckhwd	%xmm3, %xmm15
	movdqa	%xmm0, %xmm3
	punpcklwd	%xmm15, %xmm0
	punpckhwd	%xmm15, %xmm3
	punpcklwd	%xmm3, %xmm0
	movdqa	%xmm1, %xmm3
	punpcklwd	%xmm2, %xmm1
	punpckhwd	%xmm2, %xmm3
	movdqa	%xmm1, %xmm2
	pand	.LC1(%rip), %xmm0
	punpckhwd	%xmm3, %xmm2
	punpcklwd	%xmm3, %xmm1
	punpcklwd	%xmm2, %xmm1
	pand	.LC1(%rip), %xmm1
	packuswb	%xmm1, %xmm0
	movups	%xmm0, (%r11,%rdx)
	addq	$16, %rdx
	cmpq	%rcx, %rdx
	jne	.L50
	movl	%r10d, %ecx
	movss	16(%rsp), %xmm1
	andl	$-16, %ecx
	movl	%ecx, %edx
	cmpl	%r10d, %ecx
	je	.L47
.L49:
	movl	%r10d, %r15d
	subl	%edx, %r15d
	leal	-1(%r15), %ebx
	cmpl	$6, %ebx
	jbe	.L52
	movl	%edx, %r14d
	movss	4(%rsp), %xmm11
	movss	-28(%rsp), %xmm10
	leaq	0(,%r14,4), %rdx
	movss	-24(%rsp), %xmm9
	movss	-20(%rsp), %xmm8
	leaq	(%rdi,%rdx), %r13
	leaq	(%rax,%rdx), %rbp
	shufps	$0xe0, %xmm11, %xmm11
	shufps	$0xe0, %xmm10, %xmm10
	movq	%rbp, -56(%rsp)
	movq	0(%r13), %rbp
	leaq	(%rsi,%rdx), %r12
	shufps	$0xe0, %xmm9, %xmm9
	movq	24(%r13), %xmm2
	shufps	$0xe0, %xmm8, %xmm8
	leaq	(%r8,%rdx), %rbx
	movq	(%r12), %xmm13
	movq	%rbp, -120(%rsp)
	movq	8(%r13), %rbp
	addq	%r9, %rdx
	movq	16(%r12), %xmm3
	movq	24(%r12), %xmm14
	movq	%rbp, -104(%rsp)
	movq	16(%r13), %rbp
	movq	8(%r12), %r13
	movq	-104(%rsp), %xmm0
	movaps	%xmm14, %xmm6
	movq	%rbp, -88(%rsp)
	movq	-56(%rsp), %rbp
	mulps	%xmm10, %xmm6
	movq	%r13, -72(%rsp)
	movq	-72(%rsp), %xmm4
	mulps	%xmm11, %xmm0
	movq	8(%rbp), %r12
	movq	16(%rbp), %xmm15
	mulps	%xmm10, %xmm4
	movq	0(%rbp), %r13
	movq	24(%rbp), %rbp
	movq	%r12, %xmm5
	mulps	%xmm9, %xmm5
	movq	%r13, -56(%rsp)
	addps	%xmm4, %xmm0
	movaps	%xmm3, %xmm4
	mulps	%xmm10, %xmm4
	movaps	%xmm0, %xmm7
	addps	%xmm5, %xmm7
	movq	-88(%rsp), %xmm5
	mulps	%xmm11, %xmm5
	movaps	%xmm4, %xmm0
	movaps	%xmm15, %xmm4
	mulps	%xmm9, %xmm4
	movlps	%xmm7, 16(%rsp)
	movq	-120(%rsp), %xmm7
	mulps	%xmm8, %xmm7
	addps	%xmm5, %xmm0
	movaps	%xmm4, %xmm5
	movaps	%xmm0, %xmm4
	movaps	%xmm6, %xmm0
	addps	%xmm5, %xmm4
	movaps	%xmm2, %xmm5
	mulps	%xmm11, %xmm5
	movlps	%xmm4, 24(%rsp)
	movss	-16(%rsp), %xmm4
	shufps	$0xe0, %xmm4, %xmm4
	addps	%xmm5, %xmm0
	movq	%rbp, %xmm5
	mulps	%xmm9, %xmm5
	movaps	%xmm0, %xmm6
	movaps	%xmm4, %xmm0
	addps	%xmm5, %xmm6
	movaps	%xmm7, %xmm5
	movss	-12(%rsp), %xmm7
	shufps	$0xe0, %xmm7, %xmm7
	movq	%xmm6, %r13
	movaps	%xmm13, %xmm6
	mulps	%xmm4, %xmm6
	movaps	%xmm1, %xmm4
	shufps	$0xe0, %xmm4, %xmm4
	addps	%xmm6, %xmm5
	movq	-56(%rsp), %xmm6
	movss	8(%rsp), %xmm12
	mulps	%xmm7, %xmm6
	shufps	$0xe0, %xmm12, %xmm12
	addps	%xmm5, %xmm6
	movaps	%xmm4, %xmm5
	mulps	%xmm4, %xmm6
	movq	-72(%rsp), %xmm4
	mulps	%xmm0, %xmm4
	addps	%xmm12, %xmm6
	movlps	%xmm6, (%rbx)
	movq	-104(%rsp), %xmm6
	mulps	%xmm8, %xmm6
	movlps	%xmm6, 32(%rsp)
	movaps	%xmm4, %xmm6
	movq	32(%rsp), %xmm4
	addps	%xmm4, %xmm6
	movq	%r12, %xmm4
	mulps	%xmm7, %xmm4
	addps	%xmm4, %xmm6
	movq	-88(%rsp), %xmm4
	mulps	%xmm8, %xmm4
	mulps	%xmm5, %xmm6
	mulps	%xmm2, %xmm8
	addps	%xmm12, %xmm6
	movlps	%xmm6, 8(%rbx)
	movaps	%xmm3, %xmm6
	mulps	%xmm0, %xmm6
	mulps	%xmm14, %xmm0
	addps	%xmm4, %xmm6
	movaps	%xmm15, %xmm4
	addps	%xmm8, %xmm0
	mulps	%xmm7, %xmm4
	movq	%rbp, %xmm8
	mulps	%xmm8, %xmm7
	movq	-120(%rsp), %xmm8
	addps	%xmm4, %xmm6
	addps	%xmm7, %xmm0
	movss	-4(%rsp), %xmm7
	mulps	%xmm5, %xmm6
	shufps	$0xe0, %xmm7, %xmm7
	mulps	%xmm5, %xmm0
	addps	%xmm12, %xmm6
	addps	%xmm12, %xmm0
	movss	-8(%rsp), %xmm12
	shufps	$0xe0, %xmm12, %xmm12
	mulps	%xmm12, %xmm8
	movlps	%xmm6, 16(%rbx)
	movlps	%xmm0, 24(%rbx)
	mulps	%xmm12, %xmm2
	movaps	%xmm13, %xmm0
	mulps	%xmm7, %xmm0
	mulps	%xmm10, %xmm13
	movaps	%xmm8, %xmm6
	movaps	%xmm7, %xmm8
	movss	(%rsp), %xmm7
	mulps	%xmm8, %xmm3
	addps	%xmm0, %xmm6
	movq	-56(%rsp), %xmm0
	shufps	$0xe0, %xmm7, %xmm7
	mulps	%xmm7, %xmm15
	mulps	%xmm8, %xmm14
	mulps	%xmm7, %xmm0
	addps	%xmm14, %xmm2
	addps	%xmm0, %xmm6
	movss	12(%rsp), %xmm0
	shufps	$0xe0, %xmm0, %xmm0
	movaps	%xmm0, %xmm4
	mulps	%xmm5, %xmm6
	subps	%xmm6, %xmm4
	movq	-104(%rsp), %xmm6
	mulps	%xmm12, %xmm6
	movlps	%xmm4, (%rdx)
	movq	-72(%rsp), %xmm4
	mulps	%xmm8, %xmm4
	movq	-120(%rsp), %xmm8
	mulps	%xmm8, %xmm11
	addps	%xmm6, %xmm4
	movq	%r12, %xmm6
	mulps	%xmm7, %xmm6
	addps	%xmm11, %xmm13
	addps	%xmm6, %xmm4
	movaps	%xmm0, %xmm6
	mulps	%xmm5, %xmm4
	subps	%xmm4, %xmm6
	movq	-88(%rsp), %xmm4
	mulps	%xmm12, %xmm4
	movlps	%xmm6, 8(%rdx)
	addps	%xmm4, %xmm3
	movaps	%xmm0, %xmm4
	addps	%xmm15, %xmm3
	mulps	%xmm5, %xmm3
	subps	%xmm3, %xmm4
	movq	%rbp, %xmm3
	mulps	%xmm3, %xmm7
	movlps	%xmm4, 16(%rdx)
	xorps	%xmm4, %xmm4
	movaps	%xmm4, %xmm3
	addps	%xmm7, %xmm2
	movq	16(%rsp), %xmm7
	mulps	%xmm5, %xmm2
	movaps	%xmm4, %xmm5
	cmpleps	%xmm7, %xmm5
	movq	24(%rsp), %xmm7
	cmpleps	%xmm7, %xmm3
	movq	%r13, %xmm7
	subps	%xmm2, %xmm0
	movq	.LC4(%rip), %xmm2
	pand	%xmm2, %xmm5
	pand	%xmm2, %xmm3
	movlps	%xmm0, 24(%rdx)
	movq	-56(%rsp), %xmm0
	movl	%r15d, %edx
	andl	$-8, %edx
	mulps	%xmm0, %xmm9
	movaps	%xmm4, %xmm0
	addl	%edx, %ecx
	andl	$7, %r15d
	cmpleps	%xmm7, %xmm4
	addps	%xmm9, %xmm13
	cmpleps	%xmm13, %xmm0
	pand	%xmm2, %xmm0
	pand	%xmm4, %xmm2
	movdqa	%xmm0, %xmm4
	punpcklwd	%xmm5, %xmm0
	punpcklwd	%xmm5, %xmm4
	pshufd	$78, %xmm4, %xmm4
	punpcklwd	%xmm4, %xmm0
	movdqa	%xmm3, %xmm4
	punpcklwd	%xmm2, %xmm3
	punpcklwd	%xmm2, %xmm4
	movq	.LC5(%rip), %xmm2
	pshufd	$78, %xmm4, %xmm4
	punpcklwd	%xmm4, %xmm3
	pand	%xmm2, %xmm0
	pand	%xmm3, %xmm2
	packuswb	%xmm2, %xmm0
	pshufd	$8, %xmm0, %xmm0
	movq	%xmm0, (%r11,%r14)
	je	.L47
.L52:
	movslq	%ecx, %rbx
	movss	-20(%rsp), %xmm7
	movss	-16(%rsp), %xmm10
	movss	(%rdi,%rbx,4), %xmm0
	movss	(%rsi,%rbx,4), %xmm4
	leaq	0(,%rbx,4), %rdx
	movaps	%xmm7, %xmm5
	movaps	%xmm10, %xmm2
	movss	-12(%rsp), %xmm9
	movss	(%rax,%rbx,4), %xmm3
	mulss	%xmm0, %xmm5
	movss	8(%rsp), %xmm12
	movss	-8(%rsp), %xmm14
	mulss	%xmm4, %xmm2
	movss	-4(%rsp), %xmm11
	movss	(%rsp), %xmm13
	movss	4(%rsp), %xmm15
	movss	12(%rsp), %xmm8
	addss	%xmm5, %xmm2
	movaps	%xmm9, %xmm5
	mulss	%xmm3, %xmm5
	addss	%xmm5, %xmm2
	movaps	%xmm14, %xmm5
	mulss	%xmm0, %xmm5
	mulss	%xmm15, %xmm0
	mulss	%xmm1, %xmm2
	addss	%xmm12, %xmm2
	movss	%xmm2, (%r8,%rbx,4)
	movaps	%xmm11, %xmm2
	mulss	%xmm4, %xmm2
	mulss	-28(%rsp), %xmm4
	addss	%xmm5, %xmm2
	movaps	%xmm13, %xmm5
	mulss	%xmm3, %xmm5
	addss	%xmm4, %xmm0
	mulss	-24(%rsp), %xmm3
	addss	%xmm5, %xmm2
	movaps	%xmm8, %xmm5
	addss	%xmm3, %xmm0
	pxor	%xmm3, %xmm3
	mulss	%xmm1, %xmm2
	comiss	%xmm3, %xmm0
	setnb	(%r11,%rbx)
	subss	%xmm2, %xmm5
	movss	%xmm5, (%r9,%rbx,4)
	leal	1(%rcx), %ebx
	cmpl	%ebx, %r10d
	jle	.L47
	movss	4(%rdi,%rdx), %xmm5
	movaps	%xmm10, %xmm6
	movaps	%xmm7, %xmm2
	movslq	%ebx, %rbx
	movss	4(%rsi,%rdx), %xmm0
	movss	4(%rax,%rdx), %xmm4
	mulss	%xmm5, %xmm2
	mulss	%xmm0, %xmm6
	addss	%xmm6, %xmm2
	movaps	%xmm9, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm6, %xmm2
	movaps	%xmm11, %xmm6
	mulss	%xmm0, %xmm6
	mulss	-28(%rsp), %xmm0
	mulss	%xmm1, %xmm2
	addss	%xmm12, %xmm2
	movss	%xmm2, 4(%r8,%rdx)
	movaps	%xmm14, %xmm2
	mulss	%xmm5, %xmm2
	mulss	%xmm15, %xmm5
	addss	%xmm6, %xmm2
	movaps	%xmm13, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm5, %xmm0
	mulss	-24(%rsp), %xmm4
	addss	%xmm6, %xmm2
	movaps	%xmm8, %xmm6
	addss	%xmm4, %xmm0
	mulss	%xmm1, %xmm2
	comiss	%xmm3, %xmm0
	setnb	(%r11,%rbx)
	leal	2(%rcx), %ebx
	subss	%xmm2, %xmm6
	movss	%xmm6, 4(%r9,%rdx)
	cmpl	%ebx, %r10d
	jle	.L47
	movss	8(%rdi,%rdx), %xmm5
	movaps	%xmm10, %xmm6
	movaps	%xmm7, %xmm2
	movslq	%ebx, %rbx
	movss	8(%rsi,%rdx), %xmm0
	movss	8(%rax,%rdx), %xmm4
	mulss	%xmm5, %xmm2
	mulss	%xmm0, %xmm6
	addss	%xmm6, %xmm2
	movaps	%xmm9, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm6, %xmm2
	movaps	%xmm14, %xmm6
	mulss	%xmm5, %xmm6
	mulss	%xmm15, %xmm5
	mulss	%xmm1, %xmm2
	addss	%xmm12, %xmm2
	movss	%xmm2, 8(%r8,%rdx)
	movaps	%xmm11, %xmm2
	mulss	%xmm0, %xmm2
	mulss	-28(%rsp), %xmm0
	addss	%xmm6, %xmm2
	movaps	%xmm13, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm5, %xmm0
	mulss	-24(%rsp), %xmm4
	addss	%xmm6, %xmm2
	movaps	%xmm8, %xmm6
	addss	%xmm4, %xmm0
	mulss	%xmm1, %xmm2
	comiss	%xmm3, %xmm0
	setnb	(%r11,%rbx)
	leal	3(%rcx), %ebx
	subss	%xmm2, %xmm6
	movss	%xmm6, 8(%r9,%rdx)
	cmpl	%ebx, %r10d
	jle	.L47
	movss	12(%rdi,%rdx), %xmm0
	movaps	%xmm7, %xmm6
	movaps	%xmm10, %xmm2
	movslq	%ebx, %rbx
	movss	12(%rsi,%rdx), %xmm5
	movss	12(%rax,%rdx), %xmm4
	mulss	%xmm0, %xmm6
	mulss	%xmm5, %xmm2
	addss	%xmm6, %xmm2
	movaps	%xmm9, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm6, %xmm2
	movaps	%xmm14, %xmm6
	mulss	%xmm0, %xmm6
	mulss	%xmm15, %xmm0
	mulss	%xmm1, %xmm2
	addss	%xmm12, %xmm2
	movss	%xmm2, 12(%r8,%rdx)
	movaps	%xmm11, %xmm2
	mulss	%xmm5, %xmm2
	mulss	-28(%rsp), %xmm5
	addss	%xmm6, %xmm2
	movaps	%xmm13, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm5, %xmm0
	mulss	-24(%rsp), %xmm4
	addss	%xmm6, %xmm2
	movaps	%xmm8, %xmm6
	addss	%xmm4, %xmm0
	mulss	%xmm1, %xmm2
	comiss	%xmm3, %xmm0
	setnb	(%r11,%rbx)
	leal	4(%rcx), %ebx
	subss	%xmm2, %xmm6
	movss	%xmm6, 12(%r9,%rdx)
	cmpl	%ebx, %r10d
	jle	.L47
	movss	16(%rdi,%rdx), %xmm0
	movaps	%xmm7, %xmm6
	movaps	%xmm10, %xmm2
	movslq	%ebx, %rbx
	movss	16(%rsi,%rdx), %xmm5
	movss	16(%rax,%rdx), %xmm4
	mulss	%xmm0, %xmm6
	mulss	%xmm5, %xmm2
	addss	%xmm6, %xmm2
	movaps	%xmm9, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm6, %xmm2
	movaps	%xmm14, %xmm6
	mulss	%xmm0, %xmm6
	mulss	%xmm15, %xmm0
	mulss	%xmm1, %xmm2
	addss	%xmm12, %xmm2
	movss	%xmm2, 16(%r8,%rdx)
	movaps	%xmm11, %xmm2
	mulss	%xmm5, %xmm2
	mulss	-28(%rsp), %xmm5
	addss	%xmm6, %xmm2
	movaps	%xmm13, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm5, %xmm0
	mulss	-24(%rsp), %xmm4
	addss	%xmm6, %xmm2
	movaps	%xmm8, %xmm6
	addss	%xmm4, %xmm0
	mulss	%xmm1, %xmm2
	comiss	%xmm3, %xmm0
	setnb	(%r11,%rbx)
	leal	5(%rcx), %ebx
	subss	%xmm2, %xmm6
	movss	%xmm6, 16(%r9,%rdx)
	cmpl	%ebx, %r10d
	jle	.L47
	movss	20(%rdi,%rdx), %xmm0
	movaps	%xmm7, %xmm6
	movaps	%xmm10, %xmm2
	movslq	%ebx, %rbx
	movss	20(%rsi,%rdx), %xmm5
	movss	20(%rax,%rdx), %xmm4
	mulss	%xmm0, %xmm6
	mulss	%xmm5, %xmm2
	addss	%xmm6, %xmm2
	movaps	%xmm9, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm6, %xmm2
	movaps	%xmm14, %xmm6
	mulss	%xmm0, %xmm6
	mulss	%xmm15, %xmm0
	mulss	%xmm1, %xmm2
	addss	%xmm12, %xmm2
	movss	%xmm2, 20(%r8,%rdx)
	movaps	%xmm11, %xmm2
	mulss	%xmm5, %xmm2
	mulss	-28(%rsp), %xmm5
	addss	%xmm6, %xmm2
	movaps	%xmm13, %xmm6
	mulss	%xmm4, %xmm6
	addss	%xmm5, %xmm0
	mulss	-24(%rsp), %xmm4
	addss	%xmm6, %xmm2
	movaps	%xmm8, %xmm6
	addss	%xmm4, %xmm0
	mulss	%xmm1, %xmm2
	comiss	%xmm3, %xmm0
	setnb	(%r11,%rbx)
	addl	$6, %ecx
	subss	%xmm2, %xmm6
	movss	%xmm6, 20(%r9,%rdx)
	cmpl	%ecx, %r10d
	jle	.L47
	movss	24(%rdi,%rdx), %xmm4
	movss	24(%rsi,%rdx), %xmm2
	movaps	%xmm10, %xmm6
	movslq	%ecx, %rcx
	movss	24(%rax,%rdx), %xmm0
	movaps	%xmm9, %xmm5
	mulss	%xmm2, %xmm6
	mulss	%xmm4, %xmm7
	mulss	%xmm0, %xmm5
	addss	%xmm7, %xmm6
	addss	%xmm6, %xmm5
	movaps	%xmm11, %xmm6
	mulss	%xmm2, %xmm6
	mulss	-28(%rsp), %xmm2
	mulss	%xmm1, %xmm5
	addss	%xmm12, %xmm5
	movss	%xmm5, 24(%r8,%rdx)
	movaps	%xmm14, %xmm5
	mulss	%xmm4, %xmm5
	addss	%xmm6, %xmm5
	movaps	%xmm13, %xmm6
	mulss	%xmm0, %xmm6
	mulss	-24(%rsp), %xmm0
	addss	%xmm6, %xmm5
	mulss	%xmm5, %xmm1
	movaps	%xmm8, %xmm5
	subss	%xmm1, %xmm5
	movaps	%xmm15, %xmm1
	mulss	%xmm4, %xmm1
	movss	%xmm5, 24(%r9,%rdx)
	addss	%xmm2, %xmm1
	addss	%xmm0, %xmm1
	comiss	%xmm3, %xmm1
	setnb	(%r11,%rcx)
.L47:
	addq	$48, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
.L54:
	.cfi_restore_state
	xorl	%edx, %edx
	xorl	%ecx, %ecx
	jmp	.L49
	.cfi_endproc
.LFE2:
	.size	project_soa_float, .-project_soa_float
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC0:
	.quad	1
	.quad	1
	.align 16
.LC1:
	.value	255
	.value	255
	.value	255
	.value	255
	.value	255
	.value	255
	.value	255
	.value	255
	.align 16
.LC3:
	.long	1
	.long	1
	.long	1
	.long	1
	.set	.LC4,.LC3
	.set	.LC5,.LC1
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
