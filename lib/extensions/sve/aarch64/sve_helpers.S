/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * void read_sve_zcr_fpu_state(unsigned long *data)
 */

#include <asm_macros.S>
#include <sve.h>

.globl read_sve_zcr_fpu_state
.globl read_sve_z_state
.globl read_sve_p_state
.globl write_sve_zcr_fpu_state
.globl write_sve_z_state
.globl write_sve_p_state
.globl write_sve_ffr_state
.globl read_sve_ffr_state
.globl maximise_vl_zcr_elx
.globl restore_back_zcr_elx
.globl write_sve_z_value
.globl read_sve_z_value
/*
 * void write_sve_z_value(unsigned char *data)
 */
func write_sve_z_value
.arch_extension sve
	/* Load the z register bank from *data. */
	ldr z0, [x0, #0, MUL VL]
	ret
.arch_extension nosve
endfunc write_sve_z_value

/*
 * void read_sve_z_value(unsigned char *data)
 */
func read_sve_z_value
.arch_extension sve

	/* Read the z register bank to *data. */
	str z0, [x0, #0, MUL VL]
	ret
.arch_extension nosve
endfunc read_sve_z_value

/*
 * void maximise_vl_zcr_elx(unsigned long vl_max)
 */
func maximise_vl_zcr_elx
.arch_extension sve
/* Detect Current Exception level */
	mrs	x3, CurrentEL
	cmp	x3, #(MODE_EL1 << MODE_EL_SHIFT)
    b.eq    maximise_vl_zcr_el1
    cmp x3, #(MODE_EL2 << MODE_EL_SHIFT)
    b.eq    maximise_vl_zcr_el2

maximise_vl_zcr_el2:
	mrs	x1, zcr_el2
	orr	x2, x1, x0
	msr	zcr_el2, x2
	ret
maximise_vl_zcr_el1:
/*save zcr_elx value in X1 to restor later*/
	mrs	x1, zcr_el1
	orr	x2, x1, x0
	msr	zcr_el1, x2
	ret
.arch_extension nosve
endfunc maximise_vl_zcr_elx


func restore_back_zcr_elx
.arch_extension sve
/* Detect Current Exception level */
	mrs	x3, CurrentEL
	cmp	x3, #(MODE_EL1 << MODE_EL_SHIFT)
    b.eq    restore_back_zcr_el1
    cmp x3, #(MODE_EL2 << MODE_EL_SHIFT)
    b.eq    restore_back_zcr_el2

restore_back_zcr_el1:
	msr	zcr_el1, x1
    ret
restore_back_zcr_el2:
	msr	zcr_el2, x1
	ret
.arch_extension nosve
endfunc restore_back_zcr_elx

/*
 * void read_sve_zcr_fpu_state(unsigned char *data)
 */
func read_sve_zcr_fpu_state
.arch_extension sve
	/* Save the FPSR/FPCR to *data */
	mrs	x1, fpsr
	mrs	x2, fpcr
	stp	x1, x2, [x0], #16
/* Detect Current Exception level */
	mrs	x3, CurrentEL
	cmp	x3, #(MODE_EL1 << MODE_EL_SHIFT)
    b.eq    save_zcr_el1
    cmp x3, #(MODE_EL2 << MODE_EL_SHIFT)
    b.eq    save_zcr_elx
save_zcr_elx:
	/* Save the ZCR_EL1/EL2 to *data */
	mrs	x1, zcr_el1
	mrs	x2, zcr_el2
	stp	x1, x2, [x0]
	ret
save_zcr_el1:
	mrs	x1, zcr_el1
	str	x1, [x0]
	ret
.arch_extension nosve
endfunc read_sve_zcr_fpu_state

/*
 * void read_sve_z_state(unsigned char *data)
 */
func read_sve_z_state
.arch_extension sve

	/* Read the z register bank to *data. */
	str z0, [x0, #0, MUL VL]
	str	z1, [x0, #1, MUL VL]
	str	z2, [x0, #2, MUL VL]
	str	z3, [x0, #3, MUL VL]
	str	z4, [x0, #4, MUL VL]
	str	z5, [x0, #5, MUL VL]
	str	z6, [x0, #6, MUL VL]
	str	z7, [x0, #7, MUL VL]
	str	z8, [x0, #8, MUL VL]
	str	z9, [x0, #9, MUL VL]
	str	z10, [x0, #10, MUL VL]
	str	z11, [x0, #11, MUL VL]
	str	z12, [x0, #12, MUL VL]
	str	z13, [x0, #13, MUL VL]
	str	z14, [x0, #14, MUL VL]
	str	z15, [x0, #15, MUL VL]
	str	z16, [x0, #16, MUL VL]
	str	z17, [x0, #17, MUL VL]
	str	z18, [x0, #18, MUL VL]
	str	z19, [x0, #19, MUL VL]
	str	z20, [x0, #20, MUL VL]
	str	z21, [x0, #21, MUL VL]
	str	z22, [x0, #22, MUL VL]
	str	z23, [x0, #23, MUL VL]
	str	z24, [x0, #24, MUL VL]
	str	z25, [x0, #25, MUL VL]
	str	z26, [x0, #26, MUL VL]
	str	z27, [x0, #27, MUL VL]
	str	z28, [x0, #28, MUL VL]
	str	z29, [x0, #29, MUL VL]
	str	z30, [x0, #30, MUL VL]
	str	z31, [x0, #31, MUL VL]

	ret
.arch_extension nosve
endfunc read_sve_z_state

/*
 * void read_sve_p_state(unsigned char *data)
 */
func read_sve_p_state
.arch_extension sve

	/* Read the P register bank to *data. */
	str 	p0, [x0, #0, MUL VL]
	str 	p1, [x0, #1, MUL VL]
	str 	p2, [x0, #2, MUL VL]
	str 	p3, [x0, #3, MUL VL]
	str 	p4, [x0, #4, MUL VL]
	str 	p5, [x0, #5, MUL VL]
	str 	p6, [x0, #6, MUL VL]
	str 	p7, [x0, #7, MUL VL]
	str 	p8, [x0, #8, MUL VL]
	str 	p9, [x0, #9, MUL VL]
	str 	p10, [x0, #10, MUL VL]
	str 	p11, [x0, #11, MUL VL]
	str 	p12, [x0, #12, MUL VL]
	str 	p13, [x0, #13, MUL VL]
	str 	p14, [x0, #14, MUL VL]
	str 	p15, [x0, #15, MUL VL]

	ret
.arch_extension nosve
endfunc read_sve_p_state

/*
 * void write_sve_zcr_fpu_state(unsigned long *data)
 */
func write_sve_zcr_fpu_state
.arch_extension sve
	/* Load the FPSR/FPCR from *data */
	ldp	x1, x2, [x0], #16
	msr	fpsr, x1
	msr	fpcr, x2

/* Detect Current Exception level */
	mrs	x3, CurrentEL
	cmp	x3, #(MODE_EL1 << MODE_EL_SHIFT)
    b.eq    load_zcr_el1
    cmp x3, #(MODE_EL2 << MODE_EL_SHIFT)
    b.eq    load_zcr_elx
load_zcr_elx:
	/* Load the ZCR_EL1/EL2 from *data */
	ldp	x1, x2, [x0]
	msr	zcr_el1, x1
	msr	zcr_el2, x2
	ret
load_zcr_el1:
	ldr	x1, [x0]
	msr	zcr_el1, x1
	ret
.arch_extension nosve
endfunc write_sve_zcr_fpu_state

/*
 * void write_sve_z_state(unsigned char *data)
 */
func write_sve_z_state
.arch_extension sve
	/* Load the z register bank from *data. */
	ldr z0, [x0, #0, MUL VL]
	ldr	z1, [x0, #1, MUL VL]
	ldr	z2, [x0, #2, MUL VL]
	ldr	z3, [x0, #3, MUL VL]
	ldr	z4, [x0, #4, MUL VL]
	ldr	z5, [x0, #5, MUL VL]
	ldr	z6, [x0, #6, MUL VL]
	ldr	z7, [x0, #7, MUL VL]
	ldr	z8, [x0, #8, MUL VL]
	ldr	z9, [x0, #9, MUL VL]
	ldr	z10, [x0, #10, MUL VL]
	ldr	z11, [x0, #11, MUL VL]
	ldr	z12, [x0, #12, MUL VL]
	ldr	z13, [x0, #13, MUL VL]
	ldr	z14, [x0, #14, MUL VL]
	ldr	z15, [x0, #15, MUL VL]
	ldr	z16, [x0, #16, MUL VL]
	ldr	z17, [x0, #17, MUL VL]
	ldr	z18, [x0, #18, MUL VL]
	ldr	z19, [x0, #19, MUL VL]
	ldr	z20, [x0, #20, MUL VL]
	ldr	z21, [x0, #21, MUL VL]
	ldr	z22, [x0, #22, MUL VL]
	ldr	z23, [x0, #23, MUL VL]
	ldr	z24, [x0, #24, MUL VL]
	ldr	z25, [x0, #25, MUL VL]
	ldr	z26, [x0, #26, MUL VL]
	ldr	z27, [x0, #27, MUL VL]
	ldr	z28, [x0, #28, MUL VL]
	ldr	z29, [x0, #29, MUL VL]
	ldr	z30, [x0, #30, MUL VL]
	ldr	z31, [x0, #31, MUL VL]

	ret
.arch_extension nosve
endfunc write_sve_z_state

/*
 * void write_sve_p_ffr_state(unsigned char *data)
 */
func write_sve_p_state
.arch_extension sve
	/* Load the P register bank from *data. */
	ldr 	p0, [x0, #0, MUL VL]
	ldr 	p1, [x0, #1, MUL VL]
	ldr 	p2, [x0, #2, MUL VL]
	ldr 	p3, [x0, #3, MUL VL]
	ldr 	p4, [x0, #4, MUL VL]
	ldr 	p5, [x0, #5, MUL VL]
	ldr 	p6, [x0, #6, MUL VL]
	ldr 	p7, [x0, #7, MUL VL]
	ldr 	p8, [x0, #8, MUL VL]
	ldr 	p9, [x0, #9, MUL VL]
	ldr 	p10, [x0, #10, MUL VL]
	ldr 	p11, [x0, #11, MUL VL]
	ldr 	p12, [x0, #12, MUL VL]
	ldr 	p13, [x0, #13, MUL VL]
	ldr 	p14, [x0, #14, MUL VL]
	ldr 	p15, [x0, #15, MUL VL]

	ret
.arch_extension nosve
endfunc write_sve_p_state

/*
 * void read_sve_ffr_state(unsigned char *data)
 */
func read_sve_ffr_state
.arch_extension sve

	/* Read the ffr register bank to *data. */
	/*save first P0 value in second VL space in data*/
	str 	p0, [x0, #1, MUL VL]
	rdffr	p0.B
	str 	p0, [x0, #0, MUL VL]
	/* restore P0 */
	ldr 	p0, [x0, #1, MUL VL]

	ret
.arch_extension nosve
endfunc read_sve_ffr_state

/*
 * void write_sve_ffr_state(unsigned char *data)
 */
func write_sve_ffr_state
.arch_extension sve
	/* Load the ffr register bank from *data. */
	/*save first P0 value in second VL space in data*/
	str 	p0, [x0, #1, MUL VL]
	ldr 	p0, [x0, #0, MUL VL]
	wrffr	p0.B
	/* restore P0 */
	ldr 	p0, [x0, #1, MUL VL]
	ret
.arch_extension nosve
endfunc write_sve_ffr_state
