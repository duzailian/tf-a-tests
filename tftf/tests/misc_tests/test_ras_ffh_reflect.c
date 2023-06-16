/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <psci.h>
#include <sdei.h>
#include <smccc.h>
#include <tftf_lib.h>

#ifdef __aarch64__
static volatile uint64_t sdei_event_received;
extern void inject_unrecoverable_ras_error(void);
extern int serror_sdei_event_handler(int ev, uint64_t arg);

int sdei_handler(int ev, uint64_t arg)
{
	sdei_event_received = 1;
	tftf_testcase_printf("SError SDEI event received.\n");

	return 0;
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
test_result_t test_ras_ffh_reflect(void)
{
	int64_t ret;
	const int event_id = 5000;
	smc_args args;
	smc_ret_values smc_ret;
	u_register_t expected_ver;

        /* Register SDEI handler */
        ret = sdei_event_register(event_id, serror_sdei_event_handler, 0,
                        SDEI_REGF_RM_PE, read_mpidr_el1());
        if (ret < 0) {
                tftf_testcase_printf("SDEI event register failed: 0x%llx\n",
                        ret);
                return TEST_RESULT_FAIL;
        }

        ret = sdei_event_enable(event_id);
        if (ret < 0) {
                tftf_testcase_printf("SDEI event enable failed: 0x%llx\n", ret);
                return TEST_RESULT_FAIL;
        }

        ret = sdei_pe_unmask();
        if (ret < 0) {
                tftf_testcase_printf("SDEI pe unmask failed: 0x%llx\n", ret);
                return TEST_RESULT_FAIL;
        }

	/* Get the version to compare against */
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	smc_ret = tftf_smc(&args);
	expected_ver = smc_ret.ret0;
	smc_ret.ret0 = 0;

	disable_serror();

        inject_unrecoverable_ras_error();

	waitms(50);

	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;

	/* Ensure that we are testing reflection path, SMC before SError */
	if (sdei_event_received == true) {
		tftf_testcase_printf("SError was triggered before SMC\n");
		return TEST_RESULT_FAIL;
	}

	smc_ret = tftf_smc(&args);

	if ((int32_t)smc_ret.ret0 != expected_ver) {
		printf("Unexpected SMCCC version: 0x%x\n", (int)smc_ret.ret0);
		return TEST_RESULT_FAIL;
        }

	if (sdei_event_received == false) {
		tftf_testcase_printf("SError is not triggered\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
#else
test_result_t test_ras_ffh_reflect(void)
{
	tftf_testcase_printf("Not supported on AArch32.\n");
	return TEST_RESULT_SKIPPED;
}
#endif
