/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_endpoints.h>
#include <lib/context_mgmt/el1_context.h>
#include <tftf.h>
#include <tftf_lib.h>
#include <test_helpers.h>
#include <cactus_test_cmds.h>

#include <spm_test_helpers.h>

#define ECHO_VAL ULL(0xabcddcba)

#ifdef	__aarch64__
/**
 * Global place holders for the entire test
 */
//static el1_ctx_regs_t el1_ctx_before_smc = {0};
//static el1_ctx_regs_t el1_ctx_after_eret = {0};
#endif


static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}
	};


/*
 * This test aims to validate EL1 system registers are restored to correct
 * values upon returning from context switch triggered by an FF-A direct
 * message request to an SP.
 */
test_result_t test_spm_cactus_el1_regs_ctx_mgmt(void)
{
	struct ffa_value ret;

	/* Check SPMC has ffa_version and expected FFA endpoints are deployed. */
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	/* Send a message to SP1 through direct messaging. */
	ret = cactus_echo_send_cmd(HYP_ID, SP_ID(1), ECHO_VAL);

	if (cactus_get_response(ret) != CACTUS_SUCCESS ||
	    cactus_echo_get_val(ret) != ECHO_VAL ||
		!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
		}
	
	return TEST_RESULT_SUCCESS;
}
