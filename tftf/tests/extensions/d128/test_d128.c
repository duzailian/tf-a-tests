/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/extensions/sysreg128.h>
#include <test_helpers.h>

#include "./test_d128.h"

/*
 * @brief Test FEAT_D128 feature support when the extension is enabled.
 *
 * Write to the common system registers TTBR0_EL1, TTBR0_EL2, VTTBR_EL2
 * and the VHE register TTBR1_EL2 with a mask of allowed bits, perform
 * (only in case of TTBR0_EL1) a dummy SMC call, then read the registers
 * back.
 *
 * The test ensures that accesses to these registers do not trap
 * to EL3 as well as that their values are preserved correctly.
 *
 * @return test_result_t
 */
test_result_t test_d128_support(void)
{
#ifdef __aarch64__
	SKIP_TEST_IF_D128_NOT_SUPPORTED();
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	smc_args tsp_svc_params;
	uint128_t reg_read;

	write128_ttbr0_el1(TTBR_REG_MASK_FULL);

	/* Standard SMC to ADD two numbers */
	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;
	tftf_smc(&tsp_svc_params);

	reg_read = read128_ttbr0_el1();
	if (reg_read != TTBR_REG_MASK_FULL) {
		NOTICE("TTBR0_EL1 unexpected value after context switch: 0x%llx:0x%llx\n",
			(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
		return TEST_RESULT_FAIL;
	}

	write128_ttbr0_el2(TTBR_REG_MASK_FULL);
	reg_read = read128_ttbr0_el2();
	if (reg_read != TTBR_REG_MASK_FULL) {
		NOTICE("TTBR0_EL2 unexpected value after write: 0x%llx:0x%llx\n",
			(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
		return TEST_RESULT_FAIL;
	}

	write128_vttbr_el2(VTTBR_REG_MASK_FULL);
	reg_read = read128_vttbr_el2();
	if (reg_read != VTTBR_REG_MASK_FULL) {
		NOTICE("VTTBR_EL2 unexpected value after write: 0x%llx:0x%llx\n",
			(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
		return TEST_RESULT_FAIL;
	}

	if (is_armv8_1_vhe_present()) {
		write128_ttbr1_el2(TTBR_REG_MASK_FULL);
		reg_read = read128_ttbr1_el2();
		if (reg_read != TTBR_REG_MASK_FULL) {
			NOTICE("TTBR1_EL2 unexpected value after write: 0x%llx:0x%llx\n",
				(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
