/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

.globl fill_fpu_state_registers
.globl read_fpu_state_registers

/*
 * void read_fpu_state_registers(struct fpu_state *fpu);
 * This function read the FPU registers and write them to fpu_state structure.
 */
func read_fpu_state_registers
	/* Save the SIMD/FPU register bank to user memory. */
	stp	q0, q1, [x0], #32
	stp	q2, q3, [x0], #32
	stp	q4, q5, [x0], #32
	stp	q6, q7, [x0], #32
	stp	q8, q9, [x0], #32
	stp	q10, q11, [x0], #32
	stp	q12, q13, [x0], #32
	stp	q14, q15, [x0], #32
	stp	q16, q17, [x0], #32
	stp	q18, q19, [x0], #32
	stp	q20, q21, [x0], #32
	stp	q22, q23, [x0], #32
	stp	q24, q25, [x0], #32
	stp	q26, q27, [x0], #32
	stp	q28, q29, [x0], #32
	stp	q30, q31, [x0], #32

	/* Save the FPSR/FPCR */
	mrs	x1, fpsr
	mrs	x2, fpcr
	stp	x1, x2, [x0]

	ret
endfunc read_fpu_state_registers

/*
 * void fill_fpu_state_registers(struct fpu_state *fpu);
 * Function to restore/fill FPU register state from provided fpu_state structure.
 */
func fill_fpu_state_registers
	/* Load the SIMD/FPU register bank from memory. */
	ldp	q0, q1, [x0], #32
	ldp	q2, q3, [x0], #32
	ldp	q4, q5, [x0], #32
	ldp	q6, q7, [x0], #32
	ldp	q8, q9, [x0], #32
	ldp	q10, q11, [x0], #32
	ldp	q12, q13, [x0], #32
	ldp	q14, q15, [x0], #32
	ldp	q16, q17, [x0], #32
	ldp	q18, q19, [x0], #32
	ldp	q20, q21, [x0], #32
	ldp	q22, q23, [x0], #32
	ldp	q24, q25, [x0], #32
	ldp	q26, q27, [x0], #32
	ldp	q28, q29, [x0], #32
	ldp	q30, q31, [x0], #32

	/* Load the FPSR/FPCR */
	ldp	x1, x2, [x0]
	msr	fpsr, x1
	msr	fpcr, x2

	ret
endfunc fill_fpu_state_registers
