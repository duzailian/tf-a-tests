/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <drivers/arm/arm_gic.h>
#include <irq.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include <sgi.h>
#include <smccc.h>
#include <std_svc.h>
#include <stdlib.h>
#include <string.h>
#include <test_helpers.h>
#include <tftf.h>

/*
 * Test that the PSTATE bits not set in Aarch64.TakeException but
 * set to a default when taking an exception to EL3 are maintained
 * after an exception and that changes in TSP do not effect the PSTATE
 * in TFTF and vice versa.
 */
test_result_t tsp_check_pstate_maintained_on_exception(void)
{
	smc_args tsp_svc_params;
	smc_ret_values ret;
	u_register_t dit;

	SKIP_TEST_IF_TSP_NOT_PRESENT();
	SKIP_TEST_IF_DIT_NOT_SUPPORTED();


	write_dit(DIT_BIT);

	/* Standard SMC */
	tsp_svc_params.fid = TSP_STD_FID(TSP_CHECK_DIT);
	tsp_svc_params.arg1 = 0;
	tsp_svc_params.arg2 = 0;
	ret = tftf_smc(&tsp_svc_params);
	if (!ret.ret1) {
		tftf_testcase_printf("DIT bit in the TSP is not 0.\n");
		return TEST_RESULT_FAIL;
	}

	dit = read_dit();
	if (dit != DIT_BIT) {
		tftf_testcase_printf("DIT bit in TFTF was not maintained.\n"
				     "Expected: 0x%llx, Actual: 0x%lx",
				     DIT_BIT, dit);
		return TEST_RESULT_FAIL;
	}

	/* Check the DIT Bit in the TSP is still set */
	tsp_svc_params.fid = TSP_STD_FID(TSP_CHECK_DIT);
	tsp_svc_params.arg1 = DIT_BIT;
	tsp_svc_params.arg2 = 0;
	ret = tftf_smc(&tsp_svc_params);
	if (!ret.ret1) {
		tftf_testcase_printf("DIT bit in the TSP was not maintained\n"
				     "Expected: 0x%llx, Actual: 0x%lx",
				     DIT_BIT, ret.ret2);
		return TEST_RESULT_FAIL;
	}

	dit = read_dit();
	if (dit != DIT_BIT) {
		tftf_testcase_printf("DIT bit in TFTF was not maintained.\n"
				     "Expected: 0x%llx, Actual: 0x%lx",
				     DIT_BIT, dit);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
