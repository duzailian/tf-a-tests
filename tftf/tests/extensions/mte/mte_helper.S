/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifdef __aarch64__
#include <asm_macros.S>
#include "mte_def.h"

	/* Required to use MTE instructions */
	.arch  armv8.5-a+memtag

	.globl mte_insert_random_tag
	.globl mte_insert_new_tag
	.globl mte_get_tag_address
	.globl mte_set_tag_address_range
	.globl mte_clear_tag_address_range
	.globl mte_enable_pstate_tco
	.globl mte_disable_pstate_tco
	.globl mte_get_pstate_tco

/*
 * mte_insert_random_tag: Insert random tag and might be same as the source tag if
 *			  the source pointer has it.
 * Input:
 *		x0 - source pointer with a tag/no-tag
 * Return:
 *		x0 - pointer with random tag
 */
func mte_insert_random_tag
	irg	x0, x0, xzr
	ret
endfunc mte_insert_random_tag

/*
 * mte_insert_new_tag: Insert new tag and different from the source tag if
 *		       source pointer has it.
 * Input:
 *		x0 - source pointer with a tag/no-tag
 * Return:
 *		x0 - pointer with random tag
 */
func mte_insert_new_tag
	gmi	x1, x0, xzr
	irg	x0, x0, x1
	ret
endfunc mte_insert_new_tag

/*
 * mte_get_tag_address: Get the tag from given address.
 * Input:
 *		x0 - source pointer
 * Return:
 *		x0 - pointer with appended tag
 */
func mte_get_tag_address
	ldg	x0, [x0]
	ret
endfunc mte_get_tag_address

/*
 * mte_set_tag_address_range: Set the tag range from the given address
 * Input:
 *		x0 - source pointer with tag data
 *		x1 - range
 * Return:
 *		none
 */
func mte_set_tag_address_range
	cbz	x1, 2f
1:
	stg	x0, [x0, #0x0]
	add	x0, x0, #MT_GRANULE_SIZE
	sub	x1, x1, #MT_GRANULE_SIZE
	cbnz	x1, 1b
2:
	ret
endfunc mte_set_tag_address_range

/*
 * mt_clear_tag_address_range: Clear the tag range from the given address
 * Input:
 *		x0 - source pointer with tag data
 *		x1 - range
 * Return:
 *		none
 */
func mte_clear_tag_address_range
	cbz	x1, 2f
1:
	stzg	x0, [x0, #0x0]
	add	x0, x0, #MT_GRANULE_SIZE
	sub	x1, x1, #MT_GRANULE_SIZE
	cbnz	x1, 1b
2:
	ret
endfunc mte_clear_tag_address_range

/*
 * mte_enable_pstate_tco: Enable PSTATE.TCO (tag check override) field
 * Input:
 *		none
 * Return:
 *		none
 */
func mte_enable_pstate_tco
	msr	tco, #MT_PSTATE_TCO_EN
	ret
endfunc mte_enable_pstate_tco

/*
 * mte_disable_pstate_tco: Disable PSTATE.TCO (tag check override) field
 * Input:
 *		none
 * Return:
 *		none
 */
func mte_disable_pstate_tco
	msr	tco, #MT_PSTATE_TCO_DIS
	ret
endfunc mte_disable_pstate_tco

/*
 * mte_get_pstate_tco: Get PSTATE.TCO (tag check override) field
 * Input:
 *		none
 * Return:
 *		x0
 */
func mte_get_pstate_tco
	mrs	x0, tco
	ubfx	x0, x0, #MT_PSTATE_TCO_SHIFT, #1
	ret
endfunc mte_get_pstate_tco
#endif /* __aarch64__ */
