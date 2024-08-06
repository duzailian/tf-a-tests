/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <heap/page_alloc.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>

static struct realm realm;

/*
 * @Test_Aim@ Test realm creation with no LPA2 and -1 RTT starting level
 */
test_result_t host_test_realm_no_lpa2_invalid_sl(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE}, realm_start_adr;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	/* Initialize  Host NS heap memory to be used in Realm creation*/
	if (page_pool_init(PAGE_POOL_BASE, PAGE_POOL_MAX_SIZE) != HEAP_INIT_SUCCESS) {
		ERROR("%s() failed\n", "page_pool_init");
		return TEST_RESULT_FAIL;
	}

	/* Allocate memory for Realm Image from pool */
	realm_start_adr = (u_register_t)page_alloc(REALM_MAX_LOAD_IMG_SIZE);

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			realm_start_adr,
			0UL, RTT_MIN_LEVEL_LPA2, rec_flag, 1U)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);
	page_pool_reset();
	return TEST_RESULT_FAIL;
}

/*
 * @Test_Aim@ Test realm creation with no LPA2 and S2SZ > 48 bits
 */
test_result_t host_test_realm_no_lpa2_invalid_s2sz(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE}, realm_start_adr;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	/* Initialize  Host NS heap memory to be used in Realm creation*/
	if (page_pool_init(PAGE_POOL_BASE, PAGE_POOL_MAX_SIZE) != HEAP_INIT_SUCCESS) {
		ERROR("%s() failed\n", "page_pool_init");
		return TEST_RESULT_FAIL;
	}

	/* Allocate memory for Realm Image from pool */
	realm_start_adr = (u_register_t)page_alloc(REALM_MAX_LOAD_IMG_SIZE);

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			realm_start_adr,
			INPLACE(RMI_FEATURE_REGISTER_0_S2SZ, 50UL),
			RTT_MIN_LEVEL, rec_flag, 1U)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);
	page_pool_reset();
	return TEST_RESULT_FAIL;
}

/*
 * @Test_Aim@ Test creating a Realm with LPA2 disabled but FEAT_LPA2 present
 * on the platform.
 * The Realm creation should succeed.
 */
test_result_t host_test_non_lpa2_realm_on_lpa2plat(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE}, realm_start_adr;
	struct realm realm;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported() == false) {
		return TEST_RESULT_SKIPPED;
	}


	/* Initialize  Host NS heap memory to be used in Realm creation*/
	if (page_pool_init(PAGE_POOL_BASE, PAGE_POOL_MAX_SIZE) != HEAP_INIT_SUCCESS) {
		ERROR("%s() failed\n", "page_pool_init");
		return TEST_RESULT_FAIL;
	}

	/* Allocate memory for Realm Image from pool */
	realm_start_adr = (u_register_t)page_alloc(REALM_MAX_LOAD_IMG_SIZE);

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			realm_start_adr,
			INPLACE(RMI_FEATURE_REGISTER_0_S2SZ, 48UL),
			RTT_MIN_LEVEL, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_destroy_realm(&realm)) {
		ERROR("%s(): failed to destroy realm\n", __func__);
		return TEST_RESULT_FAIL;
	}

	page_pool_reset();
	return TEST_RESULT_SUCCESS;
}

/*
 * @Test_Aim@ Test creating a Realm payload with LPA2 enabled on a platform
 * which does not implement FEAT_LPA2.
 * Realm creation must fail.
 */
test_result_t host_test_lpa2_realm_on_non_lpa2plat(void)
{
	u_register_t rec_flag[1] = {RMI_RUNNABLE}, realm_start_adr;
	struct realm realm;
	u_register_t feature_flag = INPLACE(RMI_FEATURE_REGISTER_0_S2SZ, 48UL);

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (is_feat_52b_on_4k_2_supported() == true) {
		return TEST_RESULT_SKIPPED;
	} else {
		feature_flag |= RMI_FEATURE_REGISTER_0_LPA2;
	}

	/* Initialize  Host NS heap memory to be used in Realm creation*/
	if (page_pool_init(PAGE_POOL_BASE, PAGE_POOL_MAX_SIZE) != HEAP_INIT_SUCCESS) {
		ERROR("%s() failed\n", "page_pool_init");
		return TEST_RESULT_FAIL;
	}

	/* Allocate memory for Realm Image from pool */
	realm_start_adr = (u_register_t)page_alloc(REALM_MAX_LOAD_IMG_SIZE);

	if (!host_create_activate_realm_payload(&realm, (u_register_t)REALM_IMAGE_BASE,
			realm_start_adr,
			feature_flag, RTT_MIN_LEVEL, rec_flag, 1U)) {
		return TEST_RESULT_SUCCESS;
	}

	(void)host_destroy_realm(&realm);

	page_pool_reset();
	return TEST_RESULT_FAIL;
}

