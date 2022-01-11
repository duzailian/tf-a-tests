/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <debug.h>
#include <sync.h>
#include <tftf_lib.h>
#include <runtime_services/realm_payload/realm_payload_test.h>
#include <lib/aarch64/arch_features.h>
#include <lib/xlat_tables/xlat_tables_defs.h>
#include <platform_def.h>

static __aligned(PAGE_SIZE) uint64_t share_page[PAGE_SIZE / sizeof(uint64_t)];

static volatile uint32_t data_abort_gpf_triggered;

static bool data_abort_handler(void)
{
	uint64_t esr_el2 = read_esr_el2();

	VERBOSE("%s esr_el2 %llx\n", __func__, esr_el2);

	if (EC_BITS(esr_el2) == EC_DABORT_CUR_EL) {
		VERBOSE("%s Data Abort caught\n", __func__);
		return true;
	}

	return false;
}

test_result_t access_secure_from_ns(void)
{
	const uintptr_t test_address = SECURE_ACCESS_TEST_ADDRESS;

	VERBOSE("Attempt to access secure memory (0x%lx)\n", test_address);

	register_custom_sync_exception_handler(data_abort_handler);
	*((volatile uint64_t *)test_address);
	unregister_custom_sync_exception_handler();

	return TEST_RESULT_SUCCESS;
}

static bool data_abort_gpf_handler(void)
{
	uint64_t esr_el2 = read_esr_el2();

	VERBOSE("%s esr_el2 %llx elr_el2 %llx\n",
		__func__, esr_el2, read_elr_el2());

	if ((EC_BITS(esr_el2) == EC_DABORT_CUR_EL) &&
	    ((ISS_BITS(esr_el2) & 0x3f) == 0x28)) {
		data_abort_gpf_triggered++;
		return true;
	}

	return false;
}

/**
 * @Test_Aim@ Check a realm region cannot be accessed from normal world.
 *
 * This test delegates a TFTF allocated buffer to Realm. It then attempts
 * a read access to the region from normal world. This results in the PE
 * triggering a GPF caught by a custom synchronous abort handler.
 *
 */
test_result_t rl_memory_cannot_be_accessed_in_ns(void)
{
	test_result_t result = TEST_RESULT_FAIL;
	u_register_t retmm;

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	data_abort_gpf_triggered = 0;
	register_custom_sync_exception_handler(data_abort_gpf_handler);
	dsbsy();

	/* First read access to the test region must not fail. */
	*((volatile uint64_t*)share_page);
	dsbsy();

	if (data_abort_gpf_triggered != 0) {
		goto out_unregister;
	}

	/* Delegate the shared page to Realm. */
	retmm = realm_granule_delegate((u_register_t)&share_page);
	if (retmm != 0UL) {
		ERROR("Granule delegate failed!\n");
		return TEST_RESULT_FAIL;
	}

	/* This access shall trigger a GPF. */
	dsbsy();
	*((volatile uint64_t*)share_page);
	dsbsy();

	if (data_abort_gpf_triggered != 1) {
		goto out_undelegate;
	}

	result = TEST_RESULT_SUCCESS;

out_undelegate:
	/* Undelegate the shared page. */
	retmm = realm_granule_undelegate((u_register_t)&share_page);
	if (retmm != 0UL) {
		ERROR("Granule undelegate failed!\n");
	}

out_unregister:
	unregister_custom_sync_exception_handler();

	return result;
}
