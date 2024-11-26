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
 * Write to the common system registers PAR_EL1, TTBR0_EL1, TTBR0_EL2,
 * TTBR1_EL1 and VTTBR_EL2, as well as VHE register TTBR1_EL2 and THE
 * registers RCWMASK_EL1 and RCWSMASK_EL1 with a mask of allowed bits,
 * then read them back. This is primarily to see if accesses to these
 * registers trap to EL3 (they should not).
 *
 * @return test_result_t
 */
test_result_t test_d128_support(void)
{
#ifdef __aarch64__
	SKIP_TEST_IF_D128_NOT_SUPPORTED();

	uint128_t reg_read;

	write128_par_el1(PAR_EL1_MASK_FULL);
	reg_read = read128_par_el1();
	if ((uint64_t)(reg_read >> 64U) != PAR_EL1_MASK_HIGH || (uint64_t)reg_read != PAR_EL1_MASK_LOW) {
		NOTICE("PAR_EL1 unexpected value after write: 0x%llx:0x%llx\n",
			(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
		return TEST_RESULT_FAIL;
	}

	write128_ttbr0_el1(TTBR_REG_MASK_FULL);
	reg_read = read128_ttbr0_el1();
	if ((uint64_t)(reg_read >> 64U) != TTBR_REG_MASK_HIGH || (uint64_t)reg_read != TTBR_REG_MASK_LOW) {
		NOTICE("TTBR0_EL1 unexpected value after write: 0x%llx:0x%llx\n",
			(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
		return TEST_RESULT_FAIL;
	}

	write128_ttbr0_el2(TTBR_REG_MASK_FULL);
	reg_read = read128_ttbr0_el2();
	if ((uint64_t)(reg_read >> 64U) != TTBR_REG_MASK_HIGH || (uint64_t)reg_read != TTBR_REG_MASK_LOW) {
		NOTICE("TTBR0_EL2 unexpected value after write: 0x%llx:0x%llx\n",
			(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
		return TEST_RESULT_FAIL;
	}

	write128_ttbr1_el1(TTBR_REG_MASK_FULL);
	reg_read = read128_ttbr1_el1();
	if ((uint64_t)(reg_read >> 64U) != TTBR_REG_MASK_HIGH || (uint64_t)reg_read != TTBR_REG_MASK_LOW) {
		NOTICE("TTBR1_EL1 unexpected value after write: 0x%llx:0x%llx\n",
			(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
		return TEST_RESULT_FAIL;
	}

	write128_vttbr_el2(VTTBR_REG_MASK_FULL);
	reg_read = read128_vttbr_el2();
	if ((uint64_t)(reg_read >> 64U) != VTTBR_REG_MASK_HIGH || (uint64_t)reg_read != VTTBR_REG_MASK_LOW) {
		NOTICE("VTTBR_EL2 unexpected value after write: 0x%llx:0x%llx\n",
			(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
		return TEST_RESULT_FAIL;
	}

	if (is_armv8_1_vhe_present()) {
		write128_ttbr1_el2(TTBR_REG_MASK_FULL);
		reg_read = read128_ttbr1_el2();
		if ((uint64_t)(reg_read >> 64U) != TTBR_REG_MASK_HIGH || (uint64_t)reg_read != TTBR_REG_MASK_LOW) {
			NOTICE("TTBR1_EL2 unexpected value after write: 0x%llx:0x%llx\n",
				(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
			return TEST_RESULT_FAIL;
		}
	}

	if (is_feat_the_supported()) {
		write128_rcwmask_el1(RCWMASK_EL1_MASK_FULL);
		reg_read = read128_rcwmask_el1();
		if ((uint64_t)(reg_read >> 64U) != RCWMASK_EL1_MASK_HIGH || (uint64_t)reg_read != RCWMASK_EL1_MASK_LOW) {
			NOTICE("RCWMASK_EL1 unexpected value after write: 0x%llx:0x%llx\n",
				(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
			return TEST_RESULT_FAIL;
		}

		write128_rcwsmask_el1(RCWSMASK_EL1_MASK_FULL);
		reg_read = read128_rcwsmask_el1();
		if ((uint64_t)(reg_read >> 64U) != RCWSMASK_EL1_MASK_HIGH || (uint64_t)reg_read != RCWSMASK_EL1_MASK_LOW) {
			NOTICE("RCWSMASK_EL1 unexpected value after write: 0x%llx:0x%llx\n",
				(uint64_t)(reg_read >> 64U), (uint64_t)reg_read);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
