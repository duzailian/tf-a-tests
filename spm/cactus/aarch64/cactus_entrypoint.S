/*
 * Copyright (c) 2017-2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

	.globl	cactus_entrypoint

func cactus_entrypoint

	/* Setup the stack pointer. */
	adr	x0, __STACKS_END__
	mov	sp, x0

	/* And jump to the C entrypoint. */
	b	cactus_main

endfunc cactus_entrypoint
