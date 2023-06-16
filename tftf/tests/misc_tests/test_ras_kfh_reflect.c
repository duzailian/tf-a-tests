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
	tftf_testcase_printf("SError event received.\n");
	return true;
}

/*
 * Test Kernel First handling paradigm of RAS errors.
 *
 * Register a custom serror handler in tftf, disable SError so that it remains
 * pended when execution enters EL3 as part of SMC call. Inject a RAS error and
 * and give some time to ensure that it is triggered, make an SMC call.
 * EL3 as part of checking pending SError during SMC call will reflect this
 * error back to tftf along with enabling the SError(SPSR.A bit) in tftf as part
 * of patch in CI.
 * As soon as execution comes back to tftf it will take the SError and handles
 * it and continues with original SMC call. Ensure that SMCCC version requested
 * is valid one.
 */
test_result_t test_ras_kfh_reflect(void)
{
	smc_args args;
	smc_ret_values ret;
	u_register_t expected_ver;

	/* Get the version to compare against */
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	ret = tftf_smc(&args);
	expected_ver = ret.ret0;
	ret.ret0 = 0;

	register_custom_serror_handler(serror_handler);
	disable_serror();
	inject_unrecoverable_ras_error();

	waitms(50);

	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;

	/* Ensure that we are testing reflection path, SMC before SError */
	if (serror_triggered == true) {
		tftf_testcase_printf("SError was triggered before SMC\n");
		return TEST_RESULT_FAIL;
	}

	ret = tftf_smc(&args);
	tftf_testcase_printf("SMCCC Version = %d.%d\n",
		(int)((ret.ret0 >> SMCCC_VERSION_MAJOR_SHIFT) & SMCCC_VERSION_MAJOR_MASK),
		(int)((ret.ret0 >> SMCCC_VERSION_MINOR_SHIFT) & SMCCC_VERSION_MINOR_MASK));

	if ((int32_t)ret.ret0 != expected_ver) {
		tftf_testcase_printf("Unexpected SMCCC version: 0x%x\n", (int)ret.ret0);
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
