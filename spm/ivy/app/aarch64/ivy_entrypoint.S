/*
 * Copyright (c) 2018-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <ivy_def.h>
#include <platform_def.h>

	.globl	ivy_entrypoint

.section .bss.stacks
	.balign CACHE_WRITEBACK_GRANULE
	.fill	IVY_STACKS_SIZE
stacks_end:

func ivy_entrypoint

	/* Setup the stack pointer. */
	adr	x0, stacks_end
	mov	sp, x0

#if IVY_SHIM == 0
	# FFA_MEM_PERM_SET_32
	# RODATA+DATA+BSS marked RW
	mov x0, #0x89
	movk x0, #0x8400, LSL #16
	adrp x1, __RODATA_START__
	adrp x2, __BSS_END__
	sub x2, x2, x1
	lsr x2, x2, #12
	mov x3, #5
	svc #0

	/* Relocate symbols */
ivy_pie_fixup:
	mov	x0, #0x1000
	mov	x1, #IVY_IMAGE_SIZE
	add	x1, x1, x0
	bl	fixup_gdt_reloc

	/* Clear S-EL0 partition BSS */
	adrp	x0, __BSS_START__
	adrp	x2, __BSS_END__
	sub	x2, x2, x0
	mov	x1, xzr
	bl	memset
#endif

	/* And jump to the C entrypoint. */
	b	ivy_main

endfunc ivy_entrypoint
