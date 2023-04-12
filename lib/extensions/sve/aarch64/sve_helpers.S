/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <lib/extensions/aarch64/sve_helpers.h>

.global	sve_subtract_interleaved_world_switch

#if __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0)
/*
 * Based on example code from the Arm Compiler Scalable Vector Extension User
 * Guide[1].
 * [1] https://developer.arm.com/docs/100891/latest/getting-started-with-the-sve-compiler/compiling-c-and-c-code-for-sve-enabled-targets
 */

func sve_subtract_interleaved_world_switch
.arch armv8.2-a+crc+fp16+sve
	mov	x4, SVE_OP_ARRAYSIZE
	mov	x5, x4
	mov	x6, 0
	whilelo	p0.s, xzr, x4
.loop:
	ld1w	z0.s, p0/z, [x1, x6, lsl 2]
	ld1w	z1.s, p0/z, [x2, x6, lsl 2]
	sub	z0.s, z0.s, z1.s
	st1w	z0.s, p0, [x0, x6, lsl 2]
	incw	x6

	cbz	x3, .no_callback

	stp	x29, x30, [sp, #-80]!
	mov	x29, sp
	stp	x0, x1, [sp, #16]
	stp	x2, x3, [sp, #32]
	stp	x4, x5, [sp, #48]
	str	x6, [sp, #64]

	/* Invoke the world switch callback */
	blr	x3

	/* Test hangs if callback returns non-zero */
	cmp	w0, #0x0
	bne	.

	ldp	x0, x1, [sp, #16]
	ldp	x2, x3, [sp, #32]
	ldp	x4, x5, [sp, #48]
	ldr	x6, [sp, #64]
	ldp	x29, x30, [sp], #80

.no_callback:
	whilelo	p0.s, x6, x5
	bne	.loop
	ret
endfunc sve_subtract_interleaved_world_switch

#endif /* __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0) */
