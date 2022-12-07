/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <arch_features.h>
#include <test_helpers.h>
#include <tftf.h>
#include <tftf_lib.h>

#define MAX_ITERATIONS_RNG 		5

/*
 * This test ensures that a RNDR read returns an FVP-generated
 * pseudo-random number.
 */
test_result_t test_rndr_trap_rng(void)
{
#if defined __aarch64__
	u_register_t rng1;
	volatile u_register_t rng2;
	unsigned int j;

	/* Make sure FEAT_RNG_TRAP is supported. */
	SKIP_TEST_IF_RNG_TRAP_NOT_SUPPORTED();

	/* Attempt to read RNDR. */
	VERBOSE("Attempting to read register RNDR...\n");
	__asm__ volatile ("mrs %0, rndr\n" : "=r" (rng1));
	VERBOSE("First random number: %lu\n", rng1);

	j = 0;
	do
	{
		/* Attempt to read RNDR */
		VERBOSE("Attempting to read register RNDR...\n");
		__asm__ volatile ("mrs %0, rndr\n"
					: "=r" (rng2));
		VERBOSE("Second random number: %lu\n", rng2);
	} while ((++j < MAX_ITERATIONS_RNG) && (rng1 == rng2));

	if (rng1 == rng2)
	{
		VERBOSE("Last number read: %lu\n", rng2);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;

#else
	/* Skip test if AArch32 */
	SKIP_TEST_IF_AARCH32();
#endif
}
