/*
 * Copyright (c) 2017-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch.h>
#include <asm_macros.S>
#include <cactus_def.h>
#include <cactus_platform_def.h>

.macro ffa_mem_perm_set start:req end:req perm:req
	adrp x29, \start
	add x29, x29, :lo12: \start

	adrp x30, \end
	add x30, x30, :lo12:\end

	/* x30 = end - begin */
	sub x30, x30, x29
	/* x28 = x30 >> 12 (number of pages) */
	mov x28, #12
	lsrv x28, x30, x28

	/* 0x84000089 is function identifier for FFA_MEM_PERM_SET_32 */
	mov w0, #0x89
	movk w0, #0x8400, lsl #16
	mov x1, x29
	mov x2, x28
	mov w3, #\perm

	svc #0

	/* 0x84000061 is function identifier for FFA_SUCCESS_32 */
	mov w1, #0x61
	movk w1, #0x8400, lsl #16
	cmp w1, w0
	b.ne .
.endm

.globl	cactus_entrypoint

/* Provision one stack per Execution Context (or vCPU) */
.section .bss.stacks
	.balign CACHE_WRITEBACK_GRANULE
	.fill	CACTUS_STACKS_SIZE
stacks_end:

func cactus_entrypoint
	/* Entry reason is primary EC cold boot */
	mov	x19, #1

	/* Set everything other than text as RW, so that relocations can succeed. */
	ffa_mem_perm_set __RODATA_START__ __BSS_END__ 5

	/* Relocate symbols */
	mov x0, #CACTUS_IMAGE_BASE
	mov	x1, #CACTUS_IMAGE_SIZE
	add	x1, x1, x0
	bl	fixup_gdt_reloc

	/* Set everything between ro data and data begin as RO */
	ffa_mem_perm_set __RODATA_START__ __RODATA_END__ 7

	/* set everthing else as RW */
	ffa_mem_perm_set __DATA_START__ __BSS_END__ 5

	/* Setup the stack pointer. */
	adr	x0, stacks_end
	mov	sp, x0

	/* Jump to the C entrypoint (it does not return) */
	mov	x0, x19
	b	cactus_main
0:	wfi
	b 0b
endfunc cactus_entrypoint
