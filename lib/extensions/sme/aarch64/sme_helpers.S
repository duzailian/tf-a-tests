/*
 * Copyright (c) 2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <assert_macros.S>
#include <asm_macros.S>

#ifdef __aarch64__
	.arch	armv8-a+sve
	.globl	sme_rdvl_1
	.globl	sme_execute_illegal_instruction

/*
 * uint64_t sme_rdvl_1(void);
 *
 * Run rdvl instruction with imm #1.
 */
func sme_rdvl_1
	rdvl	x0, #1
	ret
endfunc sme_rdvl_1

/*
 * void sme_execute_illegal_instruction(void);
 *
 * This function tests that illegal instructions are allowed to execute when
 * FA64 is supported. RDFFR is explicitly stated to be illegal in the SME
 * specification section F1.1.2 unless FA64 is supported and enabled.
 */
func sme_execute_illegal_instruction
	rdffr	p0.b
	ret
endfunc sme_execute_illegal_instruction

#endif /* __aarch64__ */
