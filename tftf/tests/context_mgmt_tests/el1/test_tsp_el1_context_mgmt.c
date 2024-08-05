/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/context_mgmt/el1_context.h>
#include <test_helpers.h>
#include <tftf_lib.h>

#ifdef	__aarch64__
/**
 * Global place holders for the entire test
 */
el1_ctx_regs_t el1_ctx_before_smc = {0};
el1_ctx_regs_t el1_ctx_after_eret = {0};
#endif

/**
 * Public Function: test_tsp_el1_ctx_regs_across_world_switch.
 *
 * @Test_Aim: This Test aims to validate EL1 context registers are restored to
 * correct values upon returning from context switch triggered by an Standard
 * SMC to TSP. This will ensure the context-management library at EL3 precisely
 * saves and restores the appropriate world specific context during world switch
 * and prevents data leakage in all cases.
 */
test_result_t test_tsp_el1_ctx_regs_across_world_switch(void)
{
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	smc_args tsp_svc_params;
	smc_ret_values tsp_result = {0};
	test_result_t ret_value;
	bool result;

	SKIP_TEST_IF_TSP_NOT_PRESENT();

	/**
	 * Read the values of EL1 registers and save them into the
	 * global buffer before the world switch.
	 * This will be compared at the later stage on ERET.
	 */
	save_el1_sysregs_context(&el1_ctx_before_smc);

	/* Print the saved EL1 context before world switch. */
	printf("EL1 context registers list before SMC in NWd:\n");
	print_el1_sysregs_context(&el1_ctx_before_smc);

	/* Standard SMC to TSP, to modify EL1 registers */
	tsp_svc_params.fid = TSP_STD_FID(TSP_MODIFY_EL1_CTX);
	tsp_svc_params.arg1 = 0;
	tsp_svc_params.arg2 = 0;
	tsp_result = tftf_smc(&tsp_svc_params);

	/*
	 * Check the result of the TSP_CHECK_EL1_CTX STD_SMC call
	 */
	if (tsp_result.ret0 != 0) {
		printf("TSP CHECK EL1_CTX returned wrong result: "
				     "got %d expected: 0 \n",
				     (unsigned int)tsp_result.ret0);
		return TEST_RESULT_FAIL;
	}

	/**
	 * Read the values of EL1 registers and save them into the
	 * global buffer after the ERET.
	 */
	save_el1_sysregs_context(&el1_ctx_after_eret);

	/* Print the saved EL1 context after ERET */
	printf("EL1 context registers list after ERET in NWd:\n");
	print_el1_sysregs_context(&el1_ctx_after_eret);

	result = compare_el1_contexts(&el1_ctx_before_smc, &el1_ctx_after_eret);

	if (result) {
		printf("EL1 context is safely handled by the "
					"EL3 Ctx-management Library\n");
		ret_value = TEST_RESULT_SUCCESS;
	} else {
		printf("EL1 context is corrupted during world switch\n");
		ret_value = TEST_RESULT_FAIL;
	}

	return ret_value;
#endif	/* __aarch64__ */
}
