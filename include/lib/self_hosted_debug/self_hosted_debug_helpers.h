/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SELF_HOSTED_DEBUG_HELPERS_H
#define SELF_HOSTED_DEBUG_HELPERS_H

#define MAX_BPS 16
#define MAX_WPS 16

#define WRITE_DBGBVR_EL1(n, addr) {			     \
	write_dbgbvr##n##_el1(INPLACE(DBGBVR_EL1_VA, addr)); \
}

#define WRITE_DBGWVR_EL1(n, addr) {			     \
	write_dbgwvr##n##_el1(INPLACE(DBGWVR_EL1_VA, addr)); \
}

/* Generate a random 32-bit address. */
unsigned int get_random_address(void);

/*
 * Given the current value of DBGBCRn_EL1, calculate the new value which:
 * - Sets the breakpoint n to be unlinked and match on address
 * - Disables the breakpoint.
 *
 * Parameters:
 * - current_val: The current value of the register.
 *
 * Returns:
 * - The updated value with the relevant bits updated.
 */
u_register_t set_dbgbcr_el1(u_register_t current_val);

/*
 * Given the current value of DBGWCRn_EL1, calculate the new value which:
 * - Sets the watchpoint n to be unlinked and match on data address
 * - Sets watchpoint to match on loads from watchpointed address
 * - Disables the watchpoint.
 *
 * Parameters:
 * - current_val: The current value of the register.
 *
 * Returns:
 * - The updated value with the relevant bits updated.
 */
u_register_t set_dbgwcr_el1(u_register_t current_val);

#endif /* SELF_HOSTED_DEBUG_HELPERS_H */
