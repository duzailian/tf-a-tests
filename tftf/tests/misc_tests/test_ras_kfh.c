/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <serror.h>
#include <tftf_lib.h>

#ifdef __aarch64__

uint64_t serror_triggered;

extern void inject_unrecoverable_ras_error(uint64_t *wait_address);

static bool serror_handler()
{
	serror_triggered = 1;
	return true;
}

test_result_t test_ras_kfh(void)
{
	register_custom_serror_handler(serror_handler);
	inject_unrecoverable_ras_error(&serror_triggered);
	unregister_custom_serror_handler();

	if (serror_triggered == false) {
		tftf_testcase_printf("SError is not triggered\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

#else

test_result_t test_ras_kfh(void)
{
	tftf_testcase_printf("Not supported on AArch32.\n");
	return TEST_RESULT_SKIPPED;
}

#endif
