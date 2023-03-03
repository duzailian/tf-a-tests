/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <psci.h>
#include <serror.h>
#include <smccc.h>
#include <tftf_lib.h>

static volatile uint64_t serror_triggered;
static volatile uint64_t sgi_triggered;
extern void inject_unrecoverable_ras_error();

static bool serror_handler()
{
	serror_triggered = 1;
	return true;
}

test_result_t test_ras_kfh(void)
{
	register_custom_serror_handler(serror_handler);
	inject_unrecoverable_ras_error();

	/* Wait until the SError fires */
	do {
		dccivac((uint64_t)&serror_triggered);
		dmbish();
	} while (serror_triggered == 0);

	unregister_custom_serror_handler();

	return TEST_RESULT_SUCCESS;
}

test_result_t test_ras_kfh_sync_reflect(void)
{
	smc_args args;
	smc_ret_values ret;

	serror_triggered = false;
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;

	register_custom_serror_handler(serror_handler);
	disable_serror();
	inject_unrecoverable_ras_error();

	do {
		dmbish();
	} while ((read_isr_el1() && ISR_A_SHIFT) != 1);

	ret = tftf_smc(&args);
	tftf_testcase_printf("SMMCCC version = %u\n", (uint32_t)ret.ret0);

	unregister_custom_serror_handler();

	if (serror_triggered == false) {
		tftf_testcase_printf("SError is not triggered\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
