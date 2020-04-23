/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <stdlib.h>
#include <tftf_lib.h>

test_result_t test_twede (void)
{
	uint64_t reg = 0;
	uint64_t reg_original = 0;
	test_result_t result = TEST_RESULT_SUCCESS;

	/* Make sure WFE trap delays are implemented on this system. */
	if (get_armv8_6_twede_support() == ID_AA64MMFR1_EL1_TWEDE_NOT_SUPPORTED)
	{
		return TEST_RESULT_SKIPPED;
	}

	/*
	 * Since this feature is disabled by default we can't do much to test it
	 * beyond ensuring the correct fields are set to zero and that they are
	 * writable.  We skip SCR_EL3 since it is not accessible from EL2 where
	 * this test runs from.
	 */

	/* Read HCR_EL2 fields. */
	mp_printf("Checking HCR_EL2 fields\n");
	reg = read_hcr_el2();
	reg_original = reg;
	if ((reg & (HCR_TWEDEL_MASK << HCR_TWEDEL_SHIFT)) != 0)
	{
		mp_printf("ERROR: HCR.TWEDEL not zero\n");
		result = TEST_RESULT_FAIL;
	}
	if ((reg & HCR_TWEDEn_BIT) != 0)
	{
		mp_printf("ERROR: HCR.TWEDEn set\n");
		result = TEST_RESULT_FAIL;
	}
	/* Set all TWEDE fields to 1s to make sure they are writable. */
	write_hcr_el2(reg | (HCR_TWEDEL_MASK << HCR_TWEDEL_SHIFT) | HCR_TWEDEn_BIT);
	reg = read_hcr_el2();
	if ((reg & (HCR_TWEDEL_MASK << HCR_TWEDEL_SHIFT)) != (HCR_TWEDEL_MASK << HCR_TWEDEL_SHIFT))
	{
		mp_printf("ERROR: HCR.TWEDEL not writable\n");
		result = TEST_RESULT_FAIL;
	}
	if ((reg & HCR_TWEDEn_BIT) == 0)
	{
		mp_printf("ERROR: HCR.TWEDEn not writable\n");
		result = TEST_RESULT_FAIL;
	}
	/* Write back original value. */
	write_hcr_el2(reg_original);

	/* Read SCTLR_EL2 fields. */
	mp_printf("Checking SCTLR_EL2 fields\n");
	reg = read_sctlr_el2();
	reg_original = reg;
	if ((reg & (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) != 0)
	{
		mp_printf("ERROR: SCTLR_EL2.TWEDEL not zero\n");
		result = TEST_RESULT_FAIL;
	}
	if ((reg & SCTLR_TWEDEn_BIT) != 0)
	{
		mp_printf("ERROR: SCTLR_EL2.TWEDEn set\n");
		result = TEST_RESULT_FAIL;
	}
	/* Set all TWEDE fields to 1s to make sure they are writable. */
	write_sctlr_el2(reg | (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT) | SCTLR_TWEDEn_BIT);
	reg = read_sctlr_el2();
	if ((reg & (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) != (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT))
	{
		mp_printf("ERROR: SCTLR_EL2.TWEDEL not writable\n");
		result = TEST_RESULT_FAIL;
	}
	if ((reg & SCTLR_TWEDEn_BIT) == 0)
	{
		mp_printf("ERROR: SCTLR_EL2.TWEDEn not writable\n");
		result = TEST_RESULT_FAIL;
	}
	/* Write back original value. */
	write_sctlr_el2(reg_original);

	/* Read SCTLR_EL1 fields. */
	mp_printf("Checking SCTLR_EL1 fields\n");
	reg = read_sctlr_el1();
	reg_original = reg;
	if ((reg & (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) != 0)
	{
		mp_printf("ERROR: SCTLR_EL1.TWEDEL not zero\n");
		result = TEST_RESULT_FAIL;
	}
	if ((reg & SCTLR_TWEDEn_BIT) != 0)
	{
		mp_printf("ERROR: SCTLR_EL1.TWEDEn set\n");
		result = TEST_RESULT_FAIL;
	}
	/* Set all TWEDE fields to 1s to make sure they are writable. */
	write_sctlr_el1(reg | (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT) | SCTLR_TWEDEn_BIT);
	reg = read_sctlr_el1();
	if ((reg & (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) != (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT))
	{
		mp_printf("ERROR: SCTLR_EL1.TWEDEL not writable\n");
		result = TEST_RESULT_FAIL;
	}
	if ((reg & SCTLR_TWEDEn_BIT) == 0)
	{
		mp_printf("ERROR: SCTLR_EL1.TWEDEn not writable\n");
		result = TEST_RESULT_FAIL;
	}
	/* Write back original value. */
	write_sctlr_el1(reg_original);

	return result;
}