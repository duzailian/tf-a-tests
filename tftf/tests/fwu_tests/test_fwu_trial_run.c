/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fwu_nvm.h>
#include <io_storage.h>
#include <platform.h>
#include <platform_def.h>
#include <psci.h>
#include <smccc.h>
#include <status.h>
#include <tftf_lib.h>

/*
 * @Test_Aim@ Validate the FWU trial run scenario.
 * Performs a trivial reboot. The actual checking
 * of the NV counter is incremented in the TF-A code.
 */
test_result_t test_fwu_trial_run(void)
{

	smc_args args = { SMC_PSCI_SYSTEM_RESET };
	smc_ret_values ret = {0};

	if (tftf_is_rebooted()) {

		return TEST_RESULT_SUCCESS;
	}

	/* Notify that we are rebooting now. */
	tftf_notify_reboot();

	/* Request PSCI system reset. */
	ret = tftf_smc(&args);

	/* The PSCI SYSTEM_RESET call is not supposed to return */
	tftf_testcase_printf("System didn't reboot properly (%d)\n",
			(unsigned int)ret.ret0);

	return TEST_RESULT_FAIL;
}
