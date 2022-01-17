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

#include <platform_def.h>

bool data_abort_handler(void)
{
	uint64_t elr_el2 = read_elr_el2();
	uint64_t esr_el2 = read_esr_el2();

	INFO("%s esr_el2 %llx elr_el2 %llx\n", __func__, esr_el2, elr_el2);

	if (EC_BITS(esr_el2) == EC_DABORT_CUR_EL) {
		VERBOSE("%s Data Abort caught\n", __func__);
		return true;
	}

	return false;
}

test_result_t access_secure_from_NS(void)
{
	const uintptr_t test_address = SECURE_ACCESS_TEST_ADDRESS;

	tftf_testcase_printf("Attempt to access secure memory (0x%lx)\n", test_address);

	register_custom_sync_exception_handler(data_abort_handler);
	*((volatile uint64_t *)test_address);
	unregister_custom_sync_exception_handler();

	return TEST_RESULT_SUCCESS;
}
