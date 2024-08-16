/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <test_helpers.h>
#include <tftf_lib.h>

#include <lfa.h>

static uint64_t num_components;
static uint64_t fw_id;

/*
 * @Test_Aim@ Simple surface tests for Live Firmware Activation.
 *
 * This test checks the version number. It runs on the lead CPU.
 */
test_result_t test_lfa_version(void)
{
	smc_args args = { LFA_VERSION };
	uint32_t major, minor;
	smc_ret_values ret = tftf_smc(&args);

	major = (uint32_t)(ret.ret0 >> 16);
	minor = (uint32_t)(ret.ret0 & 0xffff);

	tftf_testcase_printf("%s LFA API Version : %d.%d\n", __func__,
			      major, minor);

	if ((major != 1) || (minor != 0)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_FEATURES.
 *
 * This test checks LFA_FEATURES for the two extreme FID values and then does
 * one negative test.
 */
test_result_t test_lfa_features(void)
{
	smc_args args = { .fid = LFA_FEATURES, .arg1 = LFA_VERSION };
	smc_ret_values ret = tftf_smc(&args);
	if (ret.ret0 != 0) {
		tftf_testcase_printf("%s: Features for FID=0x%08lx returned %08lx\n",
				     __func__, args.arg1, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	args.arg1 = LFA_CANCEL;
	ret = tftf_smc(&args);
	if (ret.ret0 != 0) {
		tftf_testcase_printf("%s: Features for FID=0x%08lx returned %08lx\n",
				     __func__, args.arg1, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	args.arg1 = LFA_INVALID;
	ret = tftf_smc(&args);
	if (ret.ret0 == 0) {
		tftf_testcase_printf("%s: Features for FID=0x%08lx returned %08lx\n",
				     __func__, args.arg1, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_GET_INFO.
 *
 * This test checks that LFA_GET_INFO returns the expected value for
 * lfa_num_components and the proper error if an invalid lfa_selector is
 * provided.
 */
test_result_t test_lfa_get_info(void)
{
	smc_args args = { .fid = LFA_GET_INFO, .arg1 = 0 };
	smc_ret_values ret = tftf_smc(&args);
	if (ret.ret0 != 0) {
		tftf_testcase_printf("%s: Unexpected error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	if (ret.ret1 < 3) {
		tftf_testcase_printf("%s: Unexpected value for num_components: %ld\n",
				     __func__, ret.ret1);
		return TEST_RESULT_FAIL;
	}

	num_components = ret.ret1;

	args.arg1 = 1;
	ret = tftf_smc(&args);
	if (ret.ret0 == 0) {
		tftf_testcase_printf("%s: Unexpected success\n", __func__);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_GET_INVENTORY.
 */
test_result_t test_lfa_get_inventory(void)
{
	smc_args args = { .fid = LFA_GET_INVENTORY, .arg1 = 0 };
	smc_ret_values ret;

	for (uint64_t i = 0; i < num_components; i++) {
		args.arg1 = i;

		ret = tftf_smc(&args);
		if (ret.ret0 != 0) {
			tftf_testcase_printf("%s: Unexpected error: 0x%08lx\n",
					     __func__, ret.ret0);
			return TEST_RESULT_FAIL;
		}

		if ((ret.ret1 == BL31_X1) && (ret.ret2 == BL31_X2)) {
			fw_id = i;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_PRIME.
 */
test_result_t test_lfa_prime(void)
{
	smc_args args = { .fid = LFA_PRIME, .arg1 = fw_id };
	smc_ret_values ret = tftf_smc(&args);
	if (ret.ret0 == 0) {
		/*
		 * TODO: currently expected to be failed as BL31 activation
		 * is not present. This is added to exercise negative
		 * scenario.
		 */
		tftf_testcase_printf("%s: Unexpected error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_PRIME.
 */
test_result_t test_lfa_activate(void)
{
	smc_args args = { .fid = LFA_ACTIVATE, .arg1 = fw_id };
	smc_ret_values ret = tftf_smc(&args);
	if (ret.ret0 == 0) {
		/*
		 * TODO: currently expected to be failed as BL31 activation
		 * is not present. This is added to exercise negative
		 * scenario.
		 */
		tftf_testcase_printf("%s: Unexpected error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test for LFA_CANCEL.
 */
test_result_t test_lfa_cancel(void)
{
	smc_args args = { .fid = LFA_CANCEL, .arg1 = 0 };
	smc_ret_values ret;

	ret = tftf_smc(&args);
	if (ret.ret0 != 0) {
		tftf_testcase_printf("%s: Unexpected error: 0x%08lx\n",
				     __func__, ret.ret0);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
