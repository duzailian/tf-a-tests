/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <arch_features.h>
#include <tftf.h>
#include <tftf_lib.h>

/* This very simple test just ensures that RNDR access causes a trap
 * to EL3. */
test_result_t test_feat_rng_trap_enabled(void)
{
#if defined __aarch64__
	/* Make sure FEAT_RNG_TRAP is supported. */
	if (!is_feat_rng_trap_present()) {
		SKIP_TEST_IF_RNG_TRAP_NOT_SUPPORTED();
	}

	/* Attempt to read RNDR. */
	read_rndr();

	/* If we make it this far, the test fails, as there was no trap
	 * to EL3 triggered. */
	return TEST_RESULT_FAIL;
#else
	/* Skip test if AArch32 */
	SKIP_TEST_IF_RNG_TRAP_NOT_SUPPORTED();
#endif
}
