/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_helpers.h>
#include <ffa_svc.h>
#include <test_helpers.h>
#include <tftf_lib.h>

/*
 * Using FFA version expected for SPM.
 */
#define SPM_VERSION MAKE_FFA_VERSION(FFA_VERSION_MAJOR, FFA_VERSION_MINOR)

/*
 * Calls FFA Version ABI, and checks if the result as expected.
 */
test_result_t test_ffa_version(uint32_t input_version, uint32_t expected_return)
{
	smc_ret_values ret_values = ffa_version(input_version);

	uint32_t spm_version = 0xFFFFFFFF & ret_values.ret0;

	if (spm_version == expected_return) {
		return TEST_RESULT_SUCCESS;
	}

	tftf_testcase_printf("Input Version: 0x%x\nReturn: 0x%x\nExpected: 0x%x\n",
							input_version, spm_version, expected_return);

	return TEST_RESULT_FAIL;
}

/*
 * @Test_Aim@ Validate what happens when using same version as SPM.
 */
test_result_t test_ffa_version_equal(void)
{
	return test_ffa_version(SPM_VERSION, SPM_VERSION);
}

/*
 * @Test_Aim@ Validate what happens when setting bit 31 in
 * 'input_version'. As per spec, FFA version is 31 bits long.
 * Bit 31 set is an invalid input.
 */
test_result_t test_ffa_version_bit31(void)
{
	return test_ffa_version(FFA_VERSION_BIT31_MASK | SPM_VERSION, FFA_ERROR_NOT_SUPPORTED);
}

/*
 * @Test_Aim@ Validate what happens for bigger version than SPM's.
 */
test_result_t test_ffa_version_bigger(void)
{
	return test_ffa_version(MAKE_FFA_VERSION(FFA_VERSION_MAJOR + 1, 0), SPM_VERSION);
}

/*
 * @Test_Aim@ Validate what happens for smaller version than SPM's.
 */
test_result_t test_ffa_version_smaller(void)
{
	return test_ffa_version(MAKE_FFA_VERSION(0, 9), SPM_VERSION);
}
