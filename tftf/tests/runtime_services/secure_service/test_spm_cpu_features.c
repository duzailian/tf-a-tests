/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <test_helpers.h>
#include <lib/extensions/fpu.h>

#define SENDER HYP_ID
#define RECEIVER SP_ID(1)
#define SVE_TEST_ITERATIONS	100
#define SVE_ARRAYSIZE		1024

#define SIMD_NS_VALUE		0x11U
#define FPCR_NS_VALUE		0x7FF9F00U
#define FPSR_NS_VALUE		0xF800009FU
#define SVE_Z_NS_VALUE		0x44U
#define SVE_P_NS_VALUE		0x77U
#define SVE_FFR_NS_VALUE	0xaaU

static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };

extern void sve_subtract_interleaved_smc(int *difference, const int *sve_op_1,
				       const int *sve_op_2);

#define SIMD_NS_VALUE		0x11U
#define FPCR_NS_VALUE		0x7FF9F00U
#define FPSR_NS_VALUE		0xF800009FU

#ifdef __aarch64__
static int sve_op_1[SVE_ARRAYSIZE];
static int sve_op_2[SVE_ARRAYSIZE];
#endif

/*
 * Tests that SIMD vectors are preserved during the context switches between
 * normal world and the secure world.
 * Fills the SIMD vectors with known values, requests SP to fill the vectors
 * with a different values, checks that the context is restored on return.
 */
test_result_t test_simd_vectors_preserved(void)
{
	/**********************************************************************
	 * Verify that FF-A is there and that it has the correct version.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	/* Non secure world fill FPU/SIMD state registers */
	fpu_state_write_template(SIMD_NS_VALUE,
			FPCR_NS_VALUE,
			FPSR_NS_VALUE);

	struct ffa_value ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	/* Normal world verify its FPU/SIMD state registers data */
	return fpu_state_compare_template(SIMD_NS_VALUE,
			FPCR_NS_VALUE,
			FPSR_NS_VALUE);
}

/*
 * Tests that SVE vectors are preserved during the context switches between
 * normal world and the secure world.
 * Fills the SVE vectors with known values, requests SP to fill the vectors
 * with a different values, checks that the context is restored on return.
 */
test_result_t test_sve_vectors_preserved(void)
{
#ifdef __aarch64__
	int vl;

	SKIP_TEST_IF_SVE_NOT_SUPPORTED();

	/**********************************************************************
	 * Verify that FF-A is there and that it has the correct version.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	/* Set ZCR_EL2.LEN to implemented VL (constrained by EL3). */
	write_zcr_el2(0xf);
	isb();

	/* Get the implemented VL. */
	vl = CONVERT_SVE_LENGTH(8*sve_vector_length_get());


	/* Fill SVE state registers for the VL size with a fixed pattern. */
	sve_state_write_template(SVE_Z_NS_VALUE,
			SVE_P_NS_VALUE,
			FPSR_NS_VALUE,
			FPCR_NS_VALUE,
			vl,
			vl,
			SVE_FFR_NS_VALUE);
	/*
	 * Call cactus secure partition which uses SIMD (and expect it doesn't
	 * affect the normal world state on return).
	 */
	struct ffa_value ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	/* Compare to state before calling into secure world. */
	return sve_state_compare_template(SVE_Z_NS_VALUE,
			SVE_P_NS_VALUE,
			FPSR_NS_VALUE,
			FPCR_NS_VALUE,
			vl,
			vl,
			SVE_FFR_NS_VALUE);
#else
	return TEST_RESULT_SKIPPED;
#endif /* __aarch64__ */
}

/*
 * Tests that SVE vector operations in normal world are not affected by context
 * switches between normal world and the secure world.
 */
test_result_t test_sve_vectors_operations(void)
{
#ifdef __aarch64__
	unsigned int val;

	SKIP_TEST_IF_SVE_NOT_SUPPORTED();

	/**********************************************************************
	 * Verify that FF-A is there and that it has the correct version.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	val = 2 * SVE_TEST_ITERATIONS;

	for (unsigned int i = 0; i < SVE_ARRAYSIZE; i++) {
		sve_op_1[i] = val;
		sve_op_2[i] = 1;
	}

	/* Set ZCR_EL2.LEN to implemented VL (constrained by EL3). */
	write_zcr_el2(0xf);
	isb();

	for (unsigned int i = 0; i < SVE_TEST_ITERATIONS; i++) {
		/* Perform SVE operations with intermittent calls to Swd. */
		sve_subtract_interleaved_smc(sve_op_1, sve_op_1, sve_op_2);
	}

	/* Check result of SVE operations. */
	for (unsigned int i = 0; i < SVE_ARRAYSIZE; i++) {
		if (sve_op_1[i] != (val - SVE_TEST_ITERATIONS)) {
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
#else
	return TEST_RESULT_SKIPPED;
#endif /* __aarch64__ */
}
