	.file	"ppc_probe.cpp"
	.machine ppc
	.section	".text"
	.align 2
	.globl f_exact
	.type	f_exact, @function
f_exact:
.LFB1157:
	.cfi_startproc
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC1@ha
	lwz 0,20(1)
	lfs 0,.LC1@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1157:
	.size	f_exact,.-f_exact
	.align 2
	.globl f_current
	.type	f_current, @function
f_current:
.LFB1158:
	.cfi_startproc
	lis 9,.LC4@ha
	lfs 12,.LC4@l(9)
	fcmpu 0,1,12
	blt- 0,.L14
	fmul 0,1,12
	lis 9,.LC6@ha
	lfs 12,.LC6@l(9)
	fsub 1,12,0
	fmul 0,0,1
	fneg 11,1
	fmadd 0,0,11,12
	fmul 1,1,0
	blr
.L14:
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC1@ha
	lwz 0,20(1)
	lfs 0,.LC1@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1158:
	.size	f_current,.-f_current
	.align 2
	.globl f_current_raw
	.type	f_current_raw, @function
f_current_raw:
.LFB1159:
	.cfi_startproc
	lis 9,.LC4@ha
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	lfs 0,.LC4@l(9)
	lis 9,.LC6@ha
	lfs 12,.LC6@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fmul 0,1,0
	fsub 1,12,0
	fmul 0,0,1
	fneg 11,1
	fmadd 0,0,11,12
	fmul 1,1,0
	blr
	.cfi_endproc
.LFE1159:
	.size	f_current_raw,.-f_current_raw
	.align 2
	.globl f_newton1
	.type	f_newton1, @function
f_newton1:
.LFB1160:
	.cfi_startproc
	lis 9,.LC9@ha
	lfd 12,.LC9@l(9)
	fcmpu 0,1,12
	cror 2,1,2
	bne- 0,.L28
	lis 10,.LC4@ha
	lis 9,.LC6@ha
	fneg 1,1
	lfs 12,.LC4@l(10)
	lfs 0,.LC6@l(9)
	fmadd 1,1,12,0
	blr
.L28:
	lis 9,.LC4@ha
	lfs 12,.LC4@l(9)
	fcmpu 0,1,12
	blt- 0,.L29
	fmul 0,1,12
	lis 9,.LC6@ha
	lfs 11,.LC6@l(9)
	fsub 12,11,0
	fmul 0,0,12
	fneg 10,12
	fmadd 1,0,10,11
	fmul 1,12,1
	blr
.L29:
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC1@ha
	lwz 0,20(1)
	lfs 0,.LC1@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1160:
	.size	f_newton1,.-f_newton1
	.align 2
	.globl f_taylor3
	.type	f_taylor3, @function
f_taylor3:
.LFB1161:
	.cfi_startproc
	lis 9,.LC9@ha
	lfd 12,.LC9@l(9)
	fcmpu 0,1,12
	cror 2,1,2
	bne- 0,.L41
	lis 10,.LC1@ha
	lis 9,.LC12@ha
	lfs 11,.LC1@l(10)
	lis 10,.LC14@ha
	lfs 12,.LC12@l(9)
	lis 9,.LC4@ha
	fsub 0,11,1
	lfs 9,.LC14@l(10)
	lfs 10,.LC4@l(9)
	fmadd 12,0,12,9
	fmadd 12,0,12,10
	fmadd 1,0,12,11
	blr
.L41:
	lis 9,.LC4@ha
	lfs 12,.LC4@l(9)
	fcmpu 0,1,12
	blt- 0,.L42
	fmul 0,1,12
	lis 9,.LC6@ha
	lfs 11,.LC6@l(9)
	fsub 12,11,0
	fmul 0,0,12
	fneg 10,12
	fmadd 0,0,10,11
	fmul 1,12,0
	blr
.L42:
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC1@ha
	lwz 0,20(1)
	lfs 0,.LC1@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1161:
	.size	f_taylor3,.-f_taylor3
	.align 2
	.globl f_seeded2
	.type	f_seeded2, @function
f_seeded2:
.LFB1162:
	.cfi_startproc
	lis 9,.LC4@ha
	lfs 12,.LC4@l(9)
	fcmpu 0,1,12
	cror 2,1,2
	bne- 0,.L50
	fmr 0,1
	lis 10,.LC17@ha
	lis 9,.LC18@ha
	fneg 1,1
	lfd 11,.LC18@l(9)
	lis 9,.LC6@ha
	lfd 10,.LC17@l(10)
	fmul 0,0,12
	fmadd 1,1,10,11
	lfs 11,.LC6@l(9)
	fmul 10,1,0
	fneg 12,1
	fmadd 12,12,10,11
	fmul 1,1,12
	fmul 0,0,1
	fneg 12,1
	fmadd 0,0,12,11
	fmul 1,1,0
	blr
.L50:
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	mflr 0
	stw 0,20(1)
	.cfi_offset 65, 4
	bl sqrt
	lis 9,.LC1@ha
	lwz 0,20(1)
	lfs 0,.LC1@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	mtlr 0
	.cfi_restore 65
	fdiv 1,0,1
	blr
	.cfi_endproc
.LFE1162:
	.size	f_seeded2,.-f_seeded2
	.align 2
	.globl f_core_current
	.type	f_core_current, @function
f_core_current:
.LFB1291:
	.cfi_startproc
	lis 9,.LC4@ha
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	lfs 0,.LC4@l(9)
	lis 9,.LC6@ha
	lfs 12,.LC6@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fmul 0,1,0
	fsub 1,12,0
	fmul 0,0,1
	fneg 11,1
	fmadd 0,0,11,12
	fmul 1,1,0
	blr
	.cfi_endproc
.LFE1291:
	.size	f_core_current,.-f_core_current
	.align 2
	.globl f_core_newton1
	.type	f_core_newton1, @function
f_core_newton1:
.LFB1164:
	.cfi_startproc
	fneg 1,1
	lis 10,.LC4@ha
	lis 9,.LC6@ha
	lfs 11,.LC4@l(10)
	lfs 12,.LC6@l(9)
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	fmadd 1,1,11,12
	addi 1,1,16
	.cfi_def_cfa_offset 0
	blr
	.cfi_endproc
.LFE1164:
	.size	f_core_newton1,.-f_core_newton1
	.align 2
	.globl f_core_taylor3
	.type	f_core_taylor3, @function
f_core_taylor3:
.LFB1165:
	.cfi_startproc
	lis 9,.LC1@ha
	lis 10,.LC12@ha
	lfs 11,.LC1@l(9)
	lis 9,.LC14@ha
	lfs 9,.LC14@l(9)
	lis 9,.LC4@ha
	fsub 1,11,1
	lfs 12,.LC12@l(10)
	lfs 10,.LC4@l(9)
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	fmadd 12,1,12,9
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fmadd 12,12,1,10
	fmadd 1,12,1,11
	blr
	.cfi_endproc
.LFE1165:
	.size	f_core_taylor3,.-f_core_taylor3
	.align 2
	.globl f_core_seeded2
	.type	f_core_seeded2, @function
f_core_seeded2:
.LFB1166:
	.cfi_startproc
	lis 9,.LC4@ha
	fneg 12,1
	lfs 0,.LC4@l(9)
	lis 8,.LC17@ha
	lis 10,.LC18@ha
	lfd 9,.LC17@l(8)
	lfd 10,.LC18@l(10)
	fmul 0,1,0
	lis 9,.LC6@ha
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	fmadd 12,12,9,10
	lfs 11,.LC6@l(9)
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
.LFE1166:
	.size	f_core_seeded2,.-f_core_seeded2
	.align 2
	.globl f_frsqrte_raw
	.type	f_frsqrte_raw, @function
f_frsqrte_raw:
.LFB1167:
	.cfi_startproc
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
#APP
 # 13 "/home/claude/wt-145/tools/rsqrt-research/rsqrt_ppc.hpp" 1
	frsqrte 1,1
 # 0 "" 2
#NO_APP
	addi 1,1,16
	.cfi_def_cfa_offset 0
	blr
	.cfi_endproc
.LFE1167:
	.size	f_frsqrte_raw,.-f_frsqrte_raw
	.align 2
	.globl f_frsqrte_n2
	.type	f_frsqrte_n2, @function
f_frsqrte_n2:
.LFB1168:
	.cfi_startproc
	lis 9,.LC4@ha
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	lfs 0,.LC4@l(9)
	lis 9,.LC6@ha
#APP
 # 13 "/home/claude/wt-145/tools/rsqrt-research/rsqrt_ppc.hpp" 1
	frsqrte 12,1
 # 0 "" 2
#NO_APP
	fmul 0,1,0
	lfs 10,.LC6@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fneg 9,12
	fmul 11,12,0
	fmadd 11,11,9,10
	fmul 1,12,11
	fmul 0,0,1
	fneg 12,1
	fmadd 0,0,12,10
	fmul 1,1,0
	blr
	.cfi_endproc
.LFE1168:
	.size	f_frsqrte_n2,.-f_frsqrte_n2
	.align 2
	.globl f_frsqrte_n3
	.type	f_frsqrte_n3, @function
f_frsqrte_n3:
.LFB1169:
	.cfi_startproc
	lis 9,.LC4@ha
	stwu 1,-16(1)
	.cfi_def_cfa_offset 16
	lfs 0,.LC4@l(9)
	lis 9,.LC6@ha
#APP
 # 13 "/home/claude/wt-145/tools/rsqrt-research/rsqrt_ppc.hpp" 1
	frsqrte 12,1
 # 0 "" 2
#NO_APP
	fmul 0,1,0
	lfs 11,.LC6@l(9)
	addi 1,1,16
	.cfi_def_cfa_offset 0
	fneg 9,12
	fmul 10,12,0
	fmadd 10,10,9,11
	fmul 1,12,10
	fmul 10,0,1
	fneg 12,1
	fmadd 12,12,10,11
	fmul 1,1,12
	fmul 0,0,1
	fneg 12,1
	fmadd 0,0,12,11
	fmul 1,1,0
	blr
	.cfi_endproc
.LFE1169:
	.size	f_frsqrte_n3,.-f_frsqrte_n3
	.section	.rodata.cst4,"aM",@progbits,4
	.align 2
.LC1:
	.long	1065353216
	.align 2
.LC4:
	.long	1056964608
	.align 2
.LC6:
	.long	1069547520
	.section	.rodata.cst8,"aM",@progbits,8
	.align 3
.LC9:
	.long	1072672276
	.long	2061584302
	.section	.rodata.cst4
	.align 2
.LC12:
	.long	1050673152
	.align 2
.LC14:
	.long	1052770304
	.section	.rodata.cst8
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
