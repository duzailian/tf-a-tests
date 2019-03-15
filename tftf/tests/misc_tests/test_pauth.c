/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psci.h>
#include <smccc.h>
#include <tftf_lib.h>
#include <tftf.h>
#include <tsp.h>
#include <test_helpers.h>

/* The length of the array used to hold the pauth keys */
#LENGTH_KEYS 10

int read_pauth_keys(uint64_t *pauth_keys, unsigned int len)
{
	assert(len < LENGTH_KEYS);

	/* read instruction keys a and b (both 128 bit) */
	pauth_keys[0] = read_apiakeylo_el1();
	pauth_keys[1] = read_apiakeyhi_el1();

	pauth_keys[2] = read_apibkeylo_el1();
	pauth_keys[3] = read_apibkeyhi_el1();

	/* read data keys a and b (both 128 bit) */
	pauth_keys[4] = read_apdakeylo_el1();
	pauth_keys[5] = read_apdakeyhi_el1();

	pauth_keys[6] = read_apdbkeylo_el1();
	pauth_keys[7] = read_apdbkeyhi_el1();

	/* read generic key */
	pauth_keys[8] = read_apgakeylo_el1();
	pauth_keys[9] = read_apgakeyhi_el1();

	return 0;
}

test_result_t test_pauth_reg_access(void)
{
	uint64_t pauth_keys[10];

	SKIP_TEST_IF_PAUTH_NOT_SUPPORTED();

	read_pauth_keys(pauth_keys, 10);
	return TEST_RESULT_SUCCESS;
}

#if __GNUC__ > 7 || (__GNUC__ == 7 && __GNUC_MINOR__ > 0)
__attribute__((target("sign-return-address=all")))
#endif
test_result_t test_pauth_leakage(void)
{
	int i;
	int key_leakage = 0;
	uint64_t pauth_keys_before[10];
	uint64_t pauth_keys_after[10];

	SKIP_TEST_IF_PAUTH_NOT_SUPPORTED();

	read_pauth_keys(pauth_keys_before, 10);

	tftf_get_psci_version();

	read_pauth_keys(pauth_keys_after, 10);

	for (i = 0; i < 10; i++) {
		if (pauth_keys_before[i] != pauth_keys_after[i]) {
			key_leakage = 1;
		}
	}
	if (key_leakage) {
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
}

test_result_t test_pauth_instructions(void)
{
	SKIP_TEST_IF_PAUTH_NOT_SUPPORTED();
	paciasp();
	autiasp();
	paciasp();
	xpaclri();
	return TEST_RESULT_SUCCESS;
}

#if __GNUC__ > 7 || (__GNUC__ == 7 && __GNUC_MINOR__ > 0)
__attribute__((target("sign-return-address=all")))
#endif
test_result_t test_pauth_leakage_tsp(void)
{
	smc_args tsp_svc_params;
	int i;
	int key_leakage = 0;
	uint64_t pauth_keys_before[10];
	uint64_t pauth_keys_after[10];

	SKIP_TEST_IF_PAUTH_NOT_SUPPORTED();
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	read_pauth_keys(pauth_keys_before, 10);

	/* Standard SMC to ADD two numbers */
	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;
	tftf_smc(&tsp_svc_params);

	read_pauth_keys(pauth_keys_after, 10);

	for (i = 0; i < 10; i++) {
		if (pauth_keys_before[i] != pauth_keys_after[i]) {
			return TEST_RESULT_FAIL;
		}
	}
	return TEST_RESULT_SUCCESS;
}
