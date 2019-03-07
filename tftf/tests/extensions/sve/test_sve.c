/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <debug.h>
#include <stdlib.h>
#include <tftf_lib.h>

#include "./test_sve.h"

#ifdef AARCH64
#if __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0)

extern void sve_subtract_arrays(int *difference, int *minuend, int *subtrahend);

static int sve_difference[SVE_ARRAYSIZE];
static int minuend[SVE_ARRAYSIZE];
static int subtrahend[SVE_ARRAYSIZE];
static int ref_difference[SVE_ARRAYSIZE];

/*
 * @Test_Aim@ Test SVE support when the extension is enabled.
 */
test_result_t test_sve_support(void)
{
	/* Check if SVE is implemented */
	if (!(read_id_aa64pfr0_el1() & (ID_AA64PFR0_SVE_MASK << ID_AA64PFR0_SVE_SHIFT)))
		return TEST_RESULT_SKIPPED;

	for (int i = 0; i < SVE_ARRAYSIZE; i++) {
		/* Generate a random number between 200 and 300 */
		minuend[i] = (rand() % 100) + 200;
		/* Generate a random number between 0 and 100 */
		subtrahend[i] = rand() % 100;
		/* Compute the difference without SVE */
		ref_difference[i] = minuend[i] - subtrahend[i];
	}

	/* Perform SVE operations */
	sve_subtract_arrays(sve_difference, minuend, subtrahend);

	/* Check correctness of SVE operations */
	for (int i = 0; i < SVE_ARRAYSIZE; i++) {
		if (sve_difference[i] != ref_difference[i])
			return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
}

#else

test_result_t test_sve_support(void)
{
	INFO("%s skipped on unsupported compilers\n", __func__);
	return TEST_RESULT_SKIPPED;
}

#endif /* __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0) */

#else

test_result_t test_sve_support(void)
{
	INFO("%s skipped on AArch32\n", __func__);
	return TEST_RESULT_SKIPPED;
}

#endif /* AARCH64 */
