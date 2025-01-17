/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>

/*
 * @Test_Aim@ Test that FEAT_MPAM is hidden to the realm
 */
test_result_t host_realm_hide_feat_mpam(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t feature_flag = 0U;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_FEAT_MPAM_NOT_SUPPORTED();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_MPAM_PRESENT, RMI_EXIT_HOST_CALL, 0U);
	ret2 = host_destroy_realm(&realm);

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return true;
}

/*
 * @Test_Aim@ Test that access to MPAM registers triggers an undef abort
 * taken into the realm.
 */
test_result_t host_realm_mpam_undef_abort(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t feature_flag = 0U;
	long sl = RTT_MIN_LEVEL;

	SKIP_TEST_IF_FEAT_MPAM_NOT_SUPPORTED();

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			feature_flag, sl, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(&realm, REALM_MPAM_ACCESS, RMI_EXIT_HOST_CALL, 0U);
	ret2 = host_destroy_realm(&realm);

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return true;
}
