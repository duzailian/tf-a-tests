/* Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_helpers.h>
#include <test_helpers.h>
#include <tftf_lib.h>

test_result_t test_ffa_spm_id_get(void)
{
	/* TODO change version to 1.1 once SPMC version is updated */
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	smc_ret_values ffa_ret = ffa_spm_id_get();
	if (ffa_ret.ret0 != FFA_SUCCESS_SMC32) {
		ERROR("FFA_SPM_ID_GET call failed!\n");
		return TEST_RESULT_FAIL;
	}
	/* Check the SPMC value given in the fvp_spmc_manifest is returned */
	ffa_id_t spm_id = ffa_ret.ret2 & 0xffff;
	if (spm_id != SPMC_ID) {
		ERROR("Expected SPMC_ID of 0x8000 recieved: 0x%x\n", spm_id);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
