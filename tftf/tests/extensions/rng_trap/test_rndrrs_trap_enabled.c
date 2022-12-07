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

#define MAX_ITERATIONS_EXCLUSIVE 	100

/*
 * This test ensures that a RNDRRS read access causes a trap to EL3.
 */
test_result_t test_rndrrs_trap_enabled(void)
{
#if defined __aarch64__
	u_register_t rng;
	u_register_t exclusive;
	u_register_t status;
	unsigned int i;

	/* Make sure FEAT_RNG_TRAP is supported. */
	SKIP_TEST_IF_RNG_TRAP_NOT_SUPPORTED();

	/*
	 * The test was inserted in a loop that runs a safe number of times
	 * in order to discard any possible trap returns other than RNG_TRAP
	 */
	for (i = 0; i < MAX_ITERATIONS_EXCLUSIVE; i++)
	{
		/* Attempt to acquire address for exclusive access */
		__asm__ volatile ("ldxr %0, %1\n" : "=r"(rng)
					: "Q"(exclusive));
		/* Attempt to read RNDRRS. */
		__asm__ volatile ("mrs %0, rndrrs\n" : "=r" (rng));
		/*
		 * After returning from the trap, the monitor variable should
		 * be cleared, so the status value should be 1.
		 */
		__asm__ volatile ("stxr %w0, %1, %2\n" : "=&r"(status)
	                                 : "r"(rng), "Q"(exclusive));

		if (status == 0)
		{
			/*
			 * If the condition was met, the monitor variable was
			 * not cleared and therefore there was no trap.
			 */
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;

#else
	/* Skip test if AArch32 */
	SKIP_TEST_IF_AARCH32();
#endif
}
