/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <debug.h>
#include <smccc.h>
#include <sync.h>
#include <tftf_lib.h>
#include <platform_def.h>

static volatile bool undef_injection_triggered;

static bool undef_injection_handler(void)
{
	uint64_t esr_el2 = read_esr_el2();
	if (EC_BITS(esr_el2) == EC_UNKNOWN) {
		tftf_testcase_printf("UNDEF injection from EL3\n");
		undef_injection_triggered = true;
		return true;
	}

	return false;
}

test_result_t test_undef_injection(void)
{
	undef_injection_triggered = false;

	register_custom_sync_exception_handler(undef_injection_handler);

	/* Try to access a register which traps to EL3 */
	u_register_t hfgitr_el2 = read_hfgitr_el2();

	if (hfgitr_el2 == 0)
		tftf_testcase_printf("hfgitr_el2 = %lu\n", hfgitr_el2);

	unregister_custom_sync_exception_handler();

	/* Ensure that EL3 still functional */
	smc_args args;
	smc_ret_values smc_ret;
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	smc_ret = tftf_smc(&args);

	tftf_testcase_printf("SMCCC Version = %d.%d\n",
		(int)((smc_ret.ret0 >> SMCCC_VERSION_MAJOR_SHIFT) & SMCCC_VERSION_MAJOR_MASK),
		(int)((smc_ret.ret0 >> SMCCC_VERSION_MINOR_SHIFT) & SMCCC_VERSION_MINOR_MASK));

	if (undef_injection_triggered == false) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
