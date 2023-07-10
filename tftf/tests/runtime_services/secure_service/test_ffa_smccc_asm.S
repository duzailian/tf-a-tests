/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

	.global test_ffa_smc
	.global test_ffa_smc_ext

	.section	.text, "ax"

func test_ffa_smc
	stp	x29, x30, [sp, #-16]!
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	mov	x30, x0
	ldp	x0, x1, [x30]
	ldp	x2, x3, [x30, #16]
	ldp	x4, x5, [x30, #32]
	ldp	x6, x7, [x30, #48]
	mov	x8, #0xa8
	add	x9, x8, #1
	add	x10, x8, #2
	add	x11, x8, #3
	add	x12, x8, #4
	add	x13, x8, #5
	add	x14, x8, #6
	add	x15, x8, #7
	add	x16, x8, #8
	add	x17, x8, #9
	add	x18, x8, #10
	add	x19, x8, #11
	add	x20, x8, #12
	add	x21, x8, #13
	add	x22, x8, #14
	add	x23, x8, #15
	add	x24, x8, #16
	add	x25, x8, #17
	add	x26, x8, #18
	add	x27, x8, #19
	add	x28, x8, #20
	add	x29, x8, #21
	smc     #0
	stp	x0, x1, [x30]
	stp	x2, x3, [x30, #16]
	stp	x4, x5, [x30, #32]
	stp	x6, x7, [x30, #48]
	mov	x0, #1
	cmp	x8, #0xa8
	bne	.fail
	cmp	x9, #0xa9
	bne	.fail
	cmp	x10, #0xaa
	bne	.fail
	cmp	x11, #0xab
	bne	.fail
	cmp	x12, #0xac
	bne	.fail
	cmp	x13, #0xad
	bne	.fail
	cmp	x14, #0xae
	bne	.fail
	cmp	x15, #0xaf
	bne	.fail
	cmp	x16, #0xb0
	bne	.fail
	cmp	x17, #0xb1
	bne	.fail
	cmp	x18, #0xb2
	bne	.fail
	cmp	x19, #0xb3
	bne	.fail
	cmp	x20, #0xb4
	bne	.fail
	cmp	x21, #0xb5
	bne	.fail
	cmp	x22, #0xb6
	bne	.fail
	cmp	x23, #0xb7
	bne	.fail
	cmp	x24, #0xb8
	bne	.fail
	cmp	x25, #0xb9
	bne	.fail
	cmp	x26, #0xba
	bne	.fail
	cmp	x27, #0xbb
	bne	.fail
	cmp	x28, #0xbc
	bne	.fail
	cmp	x29, #0xbd
	bne	.fail
	b	.exit
.fail:	mov	x0, xzr
.exit:	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
	ldp	x29, x30, [sp], #16
	ret
endfunc test_ffa_smc

func test_ffa_smc_ext
	stp	x29, x30, [sp, #-16]!
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	mov	x30, x0
	ldp	x0, x1, [x30]
	ldp	x2, x3, [x30, #16]
	ldp	x4, x5, [x30, #32]
	ldp	x6, x7, [x30, #48]
	mov	x8, xzr
	mov	x9, xzr
	mov	x10, xzr
	mov	x11, xzr
	mov	x12, xzr
	mov	x13, xzr
	mov	x14, xzr
	mov	x15, xzr
	mov	x16, xzr
	mov	x17, xzr
	mov	x18, #0xb2
	add	x19, x18, #1
	add	x20, x18, #2
	add	x21, x18, #3
	add	x22, x18, #4
	add	x23, x18, #5
	add	x24, x18, #6
	add	x25, x18, #7
	add	x26, x18, #8
	add	x27, x18, #9
	add	x28, x18, #10
	add	x29, x18, #11
	smc     #0
	stp	x0, x1, [x30]
	stp	x2, x3, [x30, #16]
	stp	x4, x5, [x30, #32]
	stp	x6, x7, [x30, #48]
	mov	x0, #1
	cmp	x18, #0xb2
	bne	.fail2
	cmp	x19, #0xb3
	bne	.fail2
	cmp	x20, #0xb4
	bne	.fail2
	cmp	x21, #0xb5
	bne	.fail2
	cmp	x22, #0xb6
	bne	.fail2
	cmp	x23, #0xb7
	bne	.fail2
	cmp	x24, #0xb8
	bne	.fail2
	cmp	x25, #0xb9
	bne	.fail2
	cmp	x26, #0xba
	bne	.fail2
	cmp	x27, #0xbb
	bne	.fail2
	cmp	x28, #0xbc
	bne	.fail2
	cmp	x29, #0xbd
	bne	.fail2
	b	.exit2
.fail2:	mov	x0, xzr
.exit2:	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
	ldp	x29, x30, [sp], #16
	ret
endfunc test_ffa_smc_ext
