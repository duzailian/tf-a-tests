/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <psci.h>
#include <serror.h>
#include <smccc.h>
#include <tftf_lib.h>

#ifdef __aarch64__
static volatile uint64_t serror_triggered;
extern void inject_unrecoverable_ras_error(void);

static bool serror_handler(void)
{
	serror_triggered = 1;
	return true;
}

/*
 * Test Kernel First handling paradigm of RAS errors.
 *
 * Register a custom serror handler in tftf, inject a RAS error and wait
 * for finite time to ensure that SError triggered and handled.
 */
test_result_t test_ras_kfh_reflect(void)
{
	smc_args args;
	smc_ret_values ret;
        int32_t expected_ver = MAKE_SMCCC_VERSION(1, 2);

	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;

	register_custom_serror_handler(serror_handler);
	disable_serror();
	inject_unrecoverable_ras_error();

	waitms(50);

	ret = tftf_smc(&args);

	if ((int32_t)ret.ret0 != expected_ver) {
		printf("Unexpected SMCCC version: 0x%x\n", (int)ret.ret0);
		return TEST_RESULT_FAIL;
        }

	unregister_custom_serror_handler();

	if (serror_triggered == false) {
		tftf_testcase_printf("SError is not triggered\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
#else
test_result_t test_ras_kfh_reflect(void)
{
	tftf_testcase_printf("Not supported on AArch32.\n");
	return TEST_RESULT_SKIPPED;
}
#endif
