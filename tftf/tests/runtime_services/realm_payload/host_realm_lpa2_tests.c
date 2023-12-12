/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>

static struct realm realm;

/*
 * @Test_Aim@ Test realm creation with no LPA2 and -1 RTT starting level
 */
test_result_t host_test_realm_no_lpa2_invalid_sl(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	/*
	 * Set RMI_FEATURE_REGISTER_0_S2SZ to RMM_OVERWRITE_S2SL
	 * to use it as flag to overwrite the stage 2 starting level.
	 */
	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			INPLACE(RMI_FEATURE_REGISTER_0_S2SZ,
				RMM_OVERWRITE_S2SL), rec_flag, 1U)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);
	return TEST_RESULT_FAIL;
}

/*
 * @Test_Aim@ Test realm creation with no LPA2 and S2SZ > 48 bits
 */
test_result_t host_test_realm_no_lpa2_invalid_s2sz(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			INPLACE(RMI_FEATURE_REGISTER_0_S2SZ, 50U),
			rec_flag, 1U)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);
	return TEST_RESULT_FAIL;
}
