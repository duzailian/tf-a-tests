/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <arch_features.h>
#include <tftf.h>
#include <tftf_lib.h>

/* This very simple test just ensures that HCRX_EL2 access does not trap. */
test_result_t test_feat_rng_trap_enabled(void)
{
#if defined __aarch64__
	/* Make sure FEAT_RNG_TRAP is supported. */
	if (!is_feat_rng_trap_present()) {
		return TEST_RESULT_SKIPPED;
	}

	/* Attempt to read RNDR. */
	read_rndr();

	/* If we make it this far, the test failes, as there was no trap
	 * to EL3 triggered. */
	return TEST_RESULT_FAIL;
#else
	/* Skip test if AArch32 */
	return TEST_RESULT_SKIPPED;
#endif
}
