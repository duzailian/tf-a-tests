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
#define MAX_ITERATIONS_RNG 		5

/*
 * This very simple test just ensures that a RNDRRS read access causes a trap
 * to EL3.
 */
test_result_t test_rndrrs_trap_enabled(void)
{
#if defined __aarch64__
	u_register_t rng1;
	volatile u_register_t rng2;
	u_register_t exclusive;
	u_register_t status;
	unsigned int i, j;

	/* Make sure FEAT_RNG_TRAP is supported. */
	SKIP_TEST_IF_RNG_TRAP_NOT_SUPPORTED();

	/* rng2 initialized to prevent compiling warning */
	rng2 = 0;

	/*
	 * The test was inserted in a loop that runs a safe number of times
	 * in order to discard any possible trap returns other than RNG_TRAP
	 */
	for (i = 0; i < MAX_ITERATIONS_EXCLUSIVE; i++)
	{
		/* Attempt to acquire address for exclusive access */
		__asm__ volatile ("ldxr %0, %1\n" : "=r"(rng1)
					: "Q"(exclusive));
		/* Attempt to read RNDRRS. */
		VERBOSE("Attempting to read register RNDRRS...\n");
		__asm__ volatile ("mrs %0, rndrrs\n" : "=r" (rng1));
		VERBOSE("First random number: %lu\n", rng1);
		/*
		 * After returning from the trap, the monitor variable should
		 * be cleared, so the status value should be 1.
		 */
		__asm__ volatile ("stxr %w0, %1, %2\n" : "=&r"(status)
	                                 : "r"(rng1), "Q"(exclusive));

		if (status == 0)
		{
			/*
			 * If the condition was met, the monitor variable was
			 * not cleared and therefore there was no trap.
			 */
			return TEST_RESULT_FAIL;
		}

		j = 0;
		do
		{
			/* Attempt to acquire address for exclusive access */
			__asm__ volatile ("ldxr %0, %1\n" : "=r"(rng2)
						: "Q"(exclusive));
			/* Attempt to read RNDRRS */
			__asm__ volatile ("mrs %0, rndrrs\n"
						: "=r" (rng2));
			VERBOSE("Second random number: %lu\n", rng2);
			/*
			 * After returning from the trap, the monitor variable should
			 * be cleared, so the status value should be 1.
			 */
			__asm__ volatile ("stxr %w0, %1, %2\n" : "=&r"(status)
						: "r"(rng2), "Q"(exclusive));

			if (status == 0)
			{
				/*
				 * If the condition was met, the monitor variable was
				 * not cleared and therefore there was no trap.
				 */
				return TEST_RESULT_FAIL;
			}
		} while ((++j < MAX_ITERATIONS_RNG) && (rng1 == rng2));

		if (rng1 == rng2)
		{
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;

#else
	/* Skip test if AArch32 */
	SKIP_TEST_IF_AARCH32();
#endif
}
