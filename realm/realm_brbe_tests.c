/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <debug.h>
#include <realm_tests.h>
#include <realm_helpers.h>
#include <sync.h>

 /* Try to enable Branch recording in R-EL1 via BRBCR_EL1 */

bool test_realm_write_brbcr_el1_reg(void)
{
	unsigned long brbcr_el1;

	realm_reset_undef_abort_count();

	/* Install exception handler to catch undefined abort */
	register_custom_sync_exception_handler(realm_sync_exception_handler);

	brbcr_el1 = read_brbcr_el1();
	brbcr_el1 |= BRBCR_EL1_INIT;
	write_brbcr_el1(brbcr_el1);

	unregister_custom_sync_exception_handler();
	return (realm_get_undef_abort_count() != 0UL);

}
