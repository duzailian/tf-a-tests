/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

	.globl	ivy_entrypoint

func ivy_entrypoint

	/* Setup the stack pointer. */
	adr	x0, __STACKS_END__
	mov	sp, x0

	/* And jump to the C entrypoint. */
	b	ivy_main

endfunc ivy_entrypoint
