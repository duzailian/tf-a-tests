/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <realm_helpers.h>

#include <arch.h>
#include <arch_helpers.h>
#include <sync.h>

/* Check if Realm gets undefined abort when it access MPAM functionality */
bool test_realm_mpam_undef_abort(void)
{
	realm_reset_undef_abort_count();

	/* install exception handler to catch undef abort */
	register_custom_sync_exception_handler(&realm_sync_exception_handler);
	write_mpam0_el1(0UL);
	unregister_custom_sync_exception_handler();

	if (realm_get_undef_abort_count() == 0U) {
		return false;
	}

	return true;
}
