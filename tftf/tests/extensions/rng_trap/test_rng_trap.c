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
 * This test ensures that a RNDR read access causes a trap to EL3.
 * 
 * This test usage load/store exclusive pairs to detect whether the execution
 * trapped to EL3 or not?
 * It relies on the fact that when exception level changes the monitor is cleared.
 * In a load/store exclusive pair, when stxr is used when monitor is cleared the
 * status is '1'.
 * In this test start with ldxr and execute "rndr" and if the trap to EL3 happened
 * then stxr status will be '1', to avoid chances of monitor being cleared(highly
 * unlikely in this scenario) by any other reason do this test iteratively. If stxr
 * succeds even single time we are sure that trap did not happen.
 */
test_result_t test_rndr_rng_trap(char *ins)
{
#if defined __aarch64__
	u_register_t rng, rng1 = 0;
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
		/* Attempt to read RNDR. */
		__asm__ volatile ("mrs %0, rndr\n" : "=r" (rng));
		/*
		 * After returning from the trap, the monitor variable should
		 * be cleared, so the status value should be 1.
		 */
		__asm__ volatile ("stxr %w0, %1, %2\n" : "=&r"(status)
	                                 : "r"(rng), "Q"(exclusive));
		/* If monitor is not cleared or not a new random number */
		if ((status == 0) || (rng == rng1))
		{
			return TEST_RESULT_FAIL;
		}
		rng1 = rng;
	}

	return TEST_RESULT_SUCCESS;
#else
	/* Skip test if AArch32 */
	SKIP_TEST_IF_AARCH32();
#endif
}

test_result_t test_rndrrs_rng_trap(char *ins)
{
#if defined __aarch64__
	u_register_t rng, rng1 = 0;
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
		/* Attempt to read RNDR. */
		__asm__ volatile ("mrs %0, rndrrs\n" : "=r" (rng));
		/*
		 * After returning from the trap, the monitor variable should
		 * be cleared, so the status value should be 1.
		 */
		__asm__ volatile ("stxr %w0, %1, %2\n" : "=&r"(status)
	                                 : "r"(rng), "Q"(exclusive));
		/* If monitor is not cleared or not a new random number */
		if ((status == 0) || (rng == rng1))
		{
			return TEST_RESULT_FAIL;
		}
		rng1 = rng;
	}

	return TEST_RESULT_SUCCESS;
#else
	/* Skip test if AArch32 */
	SKIP_TEST_IF_AARCH32();
#endif
}
