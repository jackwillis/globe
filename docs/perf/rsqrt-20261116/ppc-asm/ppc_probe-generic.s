	.file	"ppc_probe.cpp"
	.machine ppc
	.section	".text"
	.align 2
	.p2align 4,,15
	.globl f_exact
	.type	f_exact, @function
f_exact:
.LFB1154:
	.cfi_startproc
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC0@ha
	lwz 0,20(1)
	lfd 0,.LC0@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1154:
	.size	f_exact,.-f_exact
	.align 2
	.p2align 4,,15
	.globl f_current
	.type	f_current, @function
f_current:
.LFB1155:
	.cfi_startproc
	lis 9,.LC3@ha
	lfd 12,.LC3@l(9)
	fcmpu 0,1,12
	blt 0,.L14
	fmul 0,1,12
	lis 9,.LC5@ha
	lfd 12,.LC5@l(9)
	fsub 1,12,0
	fneg 11,1
	fmul 0,0,1
	fmadd 0,0,11,12
	fmul 1,1,0
	blr
	.p2align 4,,15
.L14:
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC0@ha
	lwz 0,20(1)
	lfd 0,.LC0@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1155:
	.size	f_current,.-f_current
	.align 2
	.p2align 4,,15
	.globl f_current_raw
	.type	f_current_raw, @function
f_current_raw:
.LFB1156:
	.cfi_startproc
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	lis 10,.LC3@ha
	lfd 0,.LC3@l(10)
	lis 9,.LC5@ha
	lfd 12,.LC5@l(9)
	fmul 0,1,0
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fsub 1,12,0
	fmul 0,0,1
	fneg 11,1
	fmadd 0,0,11,12
	fmul 1,1,0
	blr
	.cfi_endproc
.LFE1156:
	.size	f_current_raw,.-f_current_raw
	.align 2
	.p2align 4,,15
	.globl f_newton1
	.type	f_newton1, @function
f_newton1:
.LFB1157:
	.cfi_startproc
	lis 9,.LC9@ha
	lfd 12,.LC9@l(9)
	fcmpu 0,1,12
	cror 2,1,2
	bne 0,.L28
	fneg 1,1
	lis 10,.LC3@ha
	lis 9,.LC5@ha
	lfd 12,.LC3@l(10)
	lfd 0,.LC5@l(9)
	fmadd 1,1,12,0
	blr
	.p2align 4,,15
.L28:
	lis 9,.LC3@ha
	lfd 12,.LC3@l(9)
	fcmpu 0,1,12
	blt 0,.L29
	fmul 0,1,12
	lis 9,.LC5@ha
	lfd 11,.LC5@l(9)
	fsub 12,11,0
	fmul 0,0,12
	fneg 10,12
	fmadd 1,0,10,11
	fmul 1,12,1
	blr
	.p2align 4,,15
.L29:
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC0@ha
	lwz 0,20(1)
	lfd 0,.LC0@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1157:
	.size	f_newton1,.-f_newton1
	.align 2
	.p2align 4,,15
	.globl f_taylor3
	.type	f_taylor3, @function
f_taylor3:
.LFB1158:
	.cfi_startproc
	lis 9,.LC9@ha
	lfd 12,.LC9@l(9)
	fcmpu 0,1,12
	cror 2,1,2
	bne 0,.L41
	lis 10,.LC0@ha
	lis 9,.LC11@ha
	lfd 11,.LC0@l(10)
	lis 10,.LC13@ha
	lfd 12,.LC11@l(9)
	lis 9,.LC3@ha
	lfd 9,.LC13@l(10)
	lfd 10,.LC3@l(9)
	fsub 0,11,1
	fmadd 12,0,12,9
	fmadd 12,0,12,10
	fmadd 1,0,12,11
	blr
	.p2align 4,,15
.L41:
	lis 9,.LC3@ha
	lfd 12,.LC3@l(9)
	fcmpu 0,1,12
	blt 0,.L42
	fmul 0,1,12
	lis 9,.LC5@ha
	lfd 11,.LC5@l(9)
	fsub 12,11,0
	fmul 0,0,12
	fneg 10,12
	fmadd 0,0,10,11
	fmul 1,12,0
	blr
	.p2align 4,,15
.L42:
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC0@ha
	lwz 0,20(1)
	lfd 0,.LC0@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1158:
	.size	f_taylor3,.-f_taylor3
	.align 2
	.p2align 4,,15
	.globl f_seeded2
	.type	f_seeded2, @function
f_seeded2:
.LFB1159:
	.cfi_startproc
	lis 9,.LC3@ha
	lfd 12,.LC3@l(9)
	fcmpu 0,1,12
	cror 2,1,2
	bne 0,.L50
	fmr 0,1
	fneg 1,1
	lis 10,.LC17@ha
	lis 9,.LC18@ha
	lfd 10,.LC17@l(10)
	fmul 0,0,12
	lfd 12,.LC18@l(9)
	lis 9,.LC5@ha
	lfd 11,.LC5@l(9)
	fmadd 1,1,10,12
	fmul 10,1,0
	fneg 12,1
	fmadd 12,12,10,11
	fmul 1,1,12
	fneg 12,1
	fmul 0,0,1
	fmadd 0,0,12,11
	fmul 1,1,0
	blr
	.p2align 4,,15
.L50:
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC0@ha
	lwz 0,20(1)
	lfd 0,.LC0@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1159:
	.size	f_seeded2,.-f_seeded2
	.align 2
	.p2align 4,,15
	.globl f_core_current
	.type	f_core_current, @function
f_core_current:
.LFB1285:
	.cfi_startproc
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	lis 10,.LC3@ha
	lfd 0,.LC3@l(10)
	lis 9,.LC5@ha
	lfd 12,.LC5@l(9)
	fmul 0,1,0
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fsub 1,12,0
	fmul 0,0,1
	fneg 11,1
	fmadd 0,0,11,12
	fmul 1,1,0
	blr
	.cfi_endproc
.LFE1285:
	.size	f_core_current,.-f_core_current
	.align 2
	.p2align 4,,15
	.globl f_core_newton1
	.type	f_core_newton1, @function
f_core_newton1:
.LFB1161:
	.cfi_startproc
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	fneg 1,1
	lis 10,.LC3@ha
	lis 9,.LC5@ha
	lfd 11,.LC3@l(10)
	lfd 12,.LC5@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fmadd 1,1,11,12
	blr
	.cfi_endproc
.LFE1161:
	.size	f_core_newton1,.-f_core_newton1
	.align 2
	.p2align 4,,15
	.globl f_core_taylor3
	.type	f_core_taylor3, @function
f_core_taylor3:
.LFB1162:
	.cfi_startproc
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	lis 10,.LC0@ha
	lfd 11,.LC0@l(10)
	lis 9,.LC11@ha
	lis 10,.LC13@ha
	lfd 12,.LC11@l(9)
	lis 9,.LC3@ha
	lfd 9,.LC13@l(10)
	lfd 10,.LC3@l(9)
	fsub 1,11,1
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fmadd 12,1,12,9
	fmadd 12,12,1,10
	fmadd 1,12,1,11
	blr
	.cfi_endproc
.LFE1162:
	.size	f_core_taylor3,.-f_core_taylor3
	.align 2
	.p2align 4,,15
	.globl f_core_seeded2
	.type	f_core_seeded2, @function
f_core_seeded2:
.LFB1163:
	.cfi_startproc
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	fneg 12,1
	lis 9,.LC18@ha
	lis 8,.LC17@ha
	lfd 10,.LC18@l(9)
	lis 10,.LC3@ha
	lfd 9,.LC17@l(8)
	lis 9,.LC5@ha
	lfd 0,.LC3@l(10)
	lfd 11,.LC5@l(9)
	fmadd 12,12,9,10
	fmul 0,1,0
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fmul 1,0,12
	fneg 1,1
	fmadd 1,12,1,11
	fmul 1,1,12
	fmul 0,0,1
	fneg 0,0
	fmadd 0,1,0,11
	fmul 1,0,1
	blr
	.cfi_endproc
.LFE1163:
	.size	f_core_seeded2,.-f_core_seeded2
	.section	.rodata.cst8,"aM",@progbits,8
	.align 3
.LC0:
	.long	1072693248
	.long	0
	.align 3
.LC3:
	.long	1071644672
	.long	0
	.align 3
.LC5:
	.long	1073217536
	.long	0
	.align 3
.LC9:
	.long	1072672276
	.long	2061584302
	.align 3
.LC11:
	.long	1070858240
	.long	0
	.align 3
.LC13:
	.long	1071120384
	.long	0
	.align 3
.LC17:
	.long	1072190770
	.long	1635523546
	.align 3
.LC18:
	.long	1073485971
	.long	1958505087
	.ident	"GCC: (Ubuntu 12.4.0-2ubuntu1~24.04.1) 12.4.0"
	.gnu_attribute 4, 1
	.section	.note.GNU-stack,"",@progbits
