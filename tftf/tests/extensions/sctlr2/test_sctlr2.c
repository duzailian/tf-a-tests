/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>

/*
 * @brief Test FEAT_SCTLR2 support when the extension is enabled.
 *
 * Read the SCTLR2_EL1 and SCTLR2_EL2 system registers and write each value
 * OR'ed with mask of allowed bits, then read them back. This is primarily
 * to see if accesses to these registers trap to EL3 (they should not).
 *
 * @return test_result_t
 */
test_result_t test_sctlr2_support(void)
{
#ifdef __aarch64__
	SKIP_TEST_IF_SCTLR2_NOT_SUPPORTED();

	uint64_t reg_initial, reg_modified;

	reg_initial = read_sctlr2_el1();
	write_sctlr2_el1(reg_initial | (~SCTLR2_EnIDCP128_BIT));
	reg_modified = read_sctlr2_el1();
	if (reg_modified != reg_initial) {
		ERROR("SCTLR2_EL1 unexpected value after write: 0x%llx\n", reg_modified);
		return TEST_RESULT_FAIL;
	}

	reg_initial = read_sctlr2_el2();
	write_sctlr2_el2(reg_initial | (~SCTLR2_EnIDCP128_BIT));
	reg_modified = read_sctlr2_el2();
	if (reg_modified != reg_initial) {
		ERROR("SCTLR2_EL2 unexpected value after write: 0x%llx\n", reg_modified);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	SKIP_TEST_IF_AARCH32();
#endif /* __aarch64_ */
}
