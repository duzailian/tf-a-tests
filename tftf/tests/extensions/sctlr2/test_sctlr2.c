/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>

/*
 * @brief Test FEAT_SCTLR2 support when the extension is enabled.
 *
 * Read the SCTLR2_EL1 and SCTLR2_EL2 system registers, write each
 * value OR'ed with mask of allowed bits, perform a dummy SMC call
 * (in case of the former one), then read the registers back.
 *
 * The test ensures that accesses to these registers do not trap
 * to EL3 as well as that their values are preserved correctly.
 *
 * @return test_result_t
 */
test_result_t test_sctlr2_support(void)
{
#ifdef __aarch64__
	SKIP_TEST_IF_SCTLR2_NOT_SUPPORTED();
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	smc_args tsp_svc_params;
	uint64_t reg_expected, reg_actual;

	reg_expected = read_sctlr2_el1();
	write_sctlr2_el1(reg_expected | (~SCTLR2_EnIDCP128_BIT));

	/* Standard SMC to ADD two numbers */
	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;
	tftf_smc(&tsp_svc_params);

	reg_actual = read_sctlr2_el1();
	if (reg_actual != reg_expected) {
		ERROR("SCTLR2_EL1 unexpected value after context switch: 0x%llx\n", reg_actual);
		return TEST_RESULT_FAIL;
	}

	reg_expected = read_sctlr2_el2();
	write_sctlr2_el2(reg_expected | (~SCTLR2_EnIDCP128_BIT));
	reg_actual = read_sctlr2_el2();
	if (reg_actual != reg_expected) {
		ERROR("SCTLR2_EL2 unexpected value after write: 0x%llx\n", reg_actual);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
