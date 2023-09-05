/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

	.globl  platform_get_core_pos

/*
 * Helper that returns a linear core ID.
 *
 * TODO: Currently this routine returns 0. Need to provide a RSI_HOST_CALL to
 * request this from Host platform.
 */
func platform_get_core_pos
	mov	x0, 0
	ret
endfunc platform_get_core_pos
