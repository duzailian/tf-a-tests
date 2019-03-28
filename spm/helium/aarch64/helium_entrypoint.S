/*
 * Copyright (c) 2018-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <helium_def.h>
#include <platform_def.h>

	.globl	helium_entrypoint

.section .bss.stacks
	.balign CACHE_WRITEBACK_GRANULE
	.fill	HELIUM_STACKS_SIZE
stacks_end:

func helium_entrypoint

	/* Setup the stack pointer. */
	adr	x0, stacks_end
	mov	sp, x0

	/* And jump to the C entrypoint. */
	b	helium_main

endfunc helium_entrypoint
