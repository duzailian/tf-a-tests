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

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, RTT_MIN_LEVEL_LPA2, rec_flag, 1U)) {
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
			INPLACE(RMI_FEATURE_REGISTER_0_S2SZ, 50UL),
			RTT_MIN_LEVEL, rec_flag, 1U)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);
	return TEST_RESULT_FAIL;
}

/*
 * @Test_Aim@ Test realm payload creation and destruction with
 * FEAT_LPA2 disabled but supported on platform.
 */
test_result_t host_test_realm_lpa2_supported_and_disabled(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported() == false) {
		return TEST_RESULT_SKIPPED;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			INPLACE(RMI_FEATURE_REGISTER_0_S2SZ, 48UL),
			RTT_MIN_LEVEL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_destroy_realm(&realm)) {
		ERROR("%s(): failed to destroy realm\n", __func__);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test realm payload creation with  FEAT_LPA2 not supported
 * on the platfom but disabled for the Realm.
 */
test_result_t host_test_realm_lpa2_enabled_and_unsupported(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t feature_flag = INPLACE(RMI_FEATURE_REGISTER_0_S2SZ, 48UL);

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported() == true) {
		return TEST_RESULT_SKIPPED;
	} else {
		feature_flag |= RMI_FEATURE_REGISTER_0_LPA2;
	}

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)PAGE_POOL_MAX_SIZE,
			feature_flag, RTT_MIN_LEVEL, rec_flag, 1U)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);
	return TEST_RESULT_FAIL;
}

