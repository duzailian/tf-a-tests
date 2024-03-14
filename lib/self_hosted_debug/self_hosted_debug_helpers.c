/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <lib/self_hosted_debug/self_hosted_debug_helpers.h>
#include <limits.h>
#include <stdlib.h>

unsigned int get_random_address(void)
{
	return (rand() % UINT32_MAX + 1);
}

u_register_t set_dbgbcr_el1(u_register_t current_val)
{
	/*
	 * Fields in DBGBCRn_EL1 are set in the following way:
	 * - BT field set to 0b0000, identifying the breakpoint n as an unlinked
	 *   instruction address match breakpoint.
	 * - BAS field set to 0b1111. If AArch32 is supported, this field
	 *   determines the halfword-aligned addresses that the breakpoint
	 *   would match on. Since we are currently not expecting any
	 *   breakpoints to match, the specific value is not important.
	 * - E bit is set to disable the breakpoint, to avoid any accidental
	 *   matches during tests.
	 */
	u_register_t new_val = current_val | MASK(DBGBCR_EL1_BAS);

	new_val &= ~(MASK(DBGBCR_EL1_BT) | DBGBCR_EL1_E_BIT);

	return new_val;
}

u_register_t set_dbgwcr_el1(u_register_t current_val)
{
	/*
	 * Fields in DBGWCRn_EL1 are set in the following way:
	 * - WT field set to 0b0, identifying the watchpoint n as an unlinked
	 *   data address match watchpoint.
	 * - BAS field set to 0b0001. This field selects which bytes from within
	 *   the address held in DBGWVRn_EL1 are watched. Since we are not
	 *   currently expecting any watchpoints to match, the specific value is
	 *   not important.
	 * - LSC field set to 0b01. This field determines what types of accesses
	 *   (load/store) cause the watchpoint to match. Since we are not
	 *   currently expecting any watchpoints to match, the specific value is
	 *   not important.
	 * - MASK field set to 0b0000. No address mask will be used.
	 * - E bit is set to disable the watchpoint, to avoid any accidental
	 *   matches during tests.
	 */
	u_register_t new_val = current_val |
			       INPLACE(DBGWCR_EL1_BAS, 1UL) |
			       INPLACE(DBGWCR_EL1_LSC, 1UL);

	new_val &= ~(MASK(DBGWCR_EL1_MASK) |
		     DBGWCR_EL1_WT_BIT |
		     DBGWCR_EL1_E_BIT);

	return new_val;
}
