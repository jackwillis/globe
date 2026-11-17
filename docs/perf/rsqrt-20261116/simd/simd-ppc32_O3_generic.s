	.file	"simd_probe.cpp"
	.machine ppc
	.section	".text"
	.align 2
	.p2align 4,,15
	.globl project_aos_double
	.type	project_aos_double, @function
project_aos_double:
.LFB0:
	.cfi_startproc
	cmpwi 0,4,0
	lfd 8,88(5)
	blelr 0
	stwu 1,-64(1)
	.cfi_def_cfa_offset 64
	lis 9,.LC0@ha
	lfd 1,24(5)
	lfd 13,32(5)
	lfd 2,40(5)
	addi 6,6,-8
	lfd 3,48(5)
	lfd 4,56(5)
	addi 7,7,-8
	addi 8,8,-1
	lfd 5,64(5)
	lfd 6,72(5)
	mtctr 4
	lfd 7,80(5)
	stfd 27,24(1)
	.cfi_offset 59, -40
	fneg 27,8
	stfd 28,32(1)
	stfd 29,40(1)
	stfd 30,48(1)
	stfd 31,56(1)
	.cfi_offset 60, -32
	.cfi_offset 61, -24
	.cfi_offset 62, -16
	.cfi_offset 63, -8
	lfd 28,0(5)
	lfd 29,8(5)
	lfd 30,.LC0@l(9)
	lfd 31,16(5)
	.p2align 4,,15
.L3:
	lfd 9,8(3)
	lfd 10,0(3)
	addi 3,3,24
	lfd 0,-8(3)
	fmul 11,9,4
	fmul 12,29,9
	fmul 9,9,13
	fmadd 11,10,3,11
	fmadd 12,28,10,12
	fmadd 10,10,1,9
	fmadd 11,0,5,11
	fmadd 12,31,0,12
	fmadd 0,0,2,10
	fcmpu 0,11,30
	fmadd 12,8,12,6
	fmadd 0,0,27,7
	stfdu 12,8(6)
	stfdu 0,8(7)
	cror 2,1,2
	mfcr 9
	rlwinm 9,9,3,1
	stbu 9,1(8)
	bdnz .L3
	lfd 27,24(1)
	lfd 28,32(1)
	lfd 29,40(1)
	lfd 30,48(1)
	lfd 31,56(1)
	addi 1,1,64
	.cfi_restore 63
	.cfi_restore 62
	.cfi_restore 61
	.cfi_restore 60
	.cfi_restore 59
	.cfi_def_cfa_offset 0
	blr
	.cfi_endproc
.LFE0:
	.size	project_aos_double,.-project_aos_double
	.align 2
	.p2align 4,,15
	.globl project_aos_float
	.type	project_aos_float, @function
project_aos_float:
.LFB1:
	.cfi_startproc
	cmpwi 0,4,0
	lfs 8,44(5)
	blelr 0
	stwu 1,-64(1)
	.cfi_def_cfa_offset 64
	lis 9,.LC1@ha
	lfs 1,12(5)
	lfs 13,16(5)
	lfs 2,20(5)
	addi 6,6,-4
	lfs 3,24(5)
	lfs 4,28(5)
	addi 7,7,-4
	addi 8,8,-1
	lfs 5,32(5)
	lfs 6,36(5)
	mtctr 4
	lfs 7,40(5)
	stfd 27,24(1)
	.cfi_offset 59, -40
	fneg 27,8
	stfd 28,32(1)
	stfd 29,40(1)
	stfd 30,48(1)
	stfd 31,56(1)
	.cfi_offset 60, -32
	.cfi_offset 61, -24
	.cfi_offset 62, -16
	.cfi_offset 63, -8
	lfs 28,0(5)
	lfs 29,4(5)
	lfs 30,.LC1@l(9)
	lfs 31,8(5)
	.p2align 4,,15
.L13:
	lfs 9,4(3)
	lfs 10,0(3)
	addi 3,3,12
	lfs 0,-4(3)
	fmuls 11,9,4
	fmuls 12,29,9
	fmuls 9,9,13
	fmadds 11,10,3,11
	fmadds 12,28,10,12
	fmadds 10,10,1,9
	fmadds 11,0,5,11
	fmadds 12,31,0,12
	fmadds 0,0,2,10
	fcmpu 0,11,30
	fmadds 12,8,12,6
	fmadds 0,0,27,7
	stfsu 12,4(6)
	stfsu 0,4(7)
	cror 2,1,2
	mfcr 9
	rlwinm 9,9,3,1
	stbu 9,1(8)
	bdnz .L13
	lfd 27,24(1)
	lfd 28,32(1)
	lfd 29,40(1)
	lfd 30,48(1)
	lfd 31,56(1)
	addi 1,1,64
	.cfi_restore 63
	.cfi_restore 62
	.cfi_restore 61
	.cfi_restore 60
	.cfi_restore 59
	.cfi_def_cfa_offset 0
	blr
	.cfi_endproc
.LFE1:
	.size	project_aos_float,.-project_aos_float
	.align 2
	.p2align 4,,15
	.globl project_soa_float
	.type	project_soa_float, @function
project_soa_float:
.LFB2:
	.cfi_startproc
	cmpwi 0,6,0
	lfs 8,44(7)
	blelr 0
	stwu 1,-64(1)
	.cfi_def_cfa_offset 64
	lfs 1,12(7)
	lis 11,.LC1@ha
	lfs 13,16(7)
	lfs 2,20(7)
	addi 10,10,-1
	lfs 3,24(7)
	lfs 4,28(7)
	mtctr 6
	lfs 5,32(7)
	lfs 6,36(7)
	lfs 7,40(7)
	stfd 27,24(1)
	.cfi_offset 59, -40
	fneg 27,8
	stfd 28,32(1)
	stfd 29,40(1)
	stfd 31,56(1)
	.cfi_offset 60, -32
	.cfi_offset 61, -24
	.cfi_offset 63, -8
	lfs 28,0(7)
	lfs 29,4(7)
	lfs 31,8(7)
	stfd 30,48(1)
	.cfi_offset 62, -16
	lfs 30,.LC1@l(11)
	li 11,0
	.p2align 4,,15
.L22:
	lfsx 9,4,11
	lfsx 10,3,11
	lfsx 0,5,11
	fmuls 11,9,4
	fmuls 12,29,9
	fmuls 9,9,13
	fmadds 11,10,3,11
	fmadds 12,28,10,12
	fmadds 10,10,1,9
	fmadds 11,0,5,11
	fmadds 12,31,0,12
	fmadds 0,0,2,10
	fcmpu 0,11,30
	fmadds 12,8,12,6
	fmadds 0,0,27,7
	stfsx 12,8,11
	stfsx 0,9,11
	addi 11,11,4
	cror 2,1,2
	mfcr 7
	rlwinm 7,7,3,1
	stbu 7,1(10)
	bdnz .L22
	lfd 27,24(1)
	lfd 28,32(1)
	lfd 29,40(1)
	lfd 30,48(1)
	lfd 31,56(1)
	addi 1,1,64
	.cfi_restore 63
	.cfi_restore 62
	.cfi_restore 61
	.cfi_restore 60
	.cfi_restore 59
	.cfi_def_cfa_offset 0
	blr
	.cfi_endproc
.LFE2:
	.size	project_soa_float,.-project_soa_float
	.section	.rodata.cst8,"aM",@progbits,8
	.align 3
.LC0:
	.long	0
	.long	0
	.set	.LC1,.LC0
	.ident	"GCC: (Ubuntu 12.4.0-2ubuntu1~24.04.1) 12.4.0"
	.section	.note.GNU-stack,"",@progbits
