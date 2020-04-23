/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <stdbool.h>
#include <stdlib.h>
#include <tftf_lib.h>

test_result_t test_twed(void) {
	uint64_t reg;

	/* Make sure WFE trap delays are implemented on this system. */
	if (is_armv8_6_twed_present() == false) {
		return TEST_RESULT_SKIPPED;
	}

	/*
	 * Since this feature is disabled by default we can't do much to test it
	 * beyond ensuring the correct fields are set to zero and that they are
	 * writable.  We skip SCR_EL3 since it is not accessible from EL2 where
	 * this test runs from.
	 */

	/* Read HCR_EL2 fields */
	mp_printf("Checking HCR_EL2 fields\n");
	reg = read_hcr_el2();

	if ((reg & (HCR_TWEDEL_MASK << HCR_TWEDEL_SHIFT)) != 0U) {
		mp_printf("ERROR: HCR.TWEDEL not zero\n");
		return TEST_RESULT_FAIL;
	}

	if ((reg & HCR_TWEDEn_BIT) != 0U) {
		mp_printf("ERROR: HCR.TWEDEn set\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * Set all TWED field bits and read back to make sure they are
	 * writable.
	 */
	write_hcr_el2(reg | (HCR_TWEDEL_MASK << HCR_TWEDEL_SHIFT) |
			HCR_TWEDEn_BIT);
	reg = read_hcr_el2();

	if ((reg & (HCR_TWEDEL_MASK << HCR_TWEDEL_SHIFT)) !=
	                (HCR_TWEDEL_MASK << HCR_TWEDEL_SHIFT)) {
		mp_printf("ERROR: HCR.TWEDEL not writable\n");
		return TEST_RESULT_FAIL;
	}

	if ((reg & HCR_TWEDEn_BIT) == 0U) {
		mp_printf("ERROR: HCR.TWEDEn not writable\n");
		return TEST_RESULT_FAIL;
	}

	/* Clear TWED fields to restore original value */
	reg = ((reg & ~(HCR_TWEDEL_MASK << HCR_TWEDEL_SHIFT)) &
			~HCR_TWEDEn_BIT);
	write_hcr_el2(reg);

	/* Read SCTLR_EL2 fields. */
	mp_printf("Checking SCTLR_EL2 fields\n");
	reg = read_sctlr_el2();

	if ((reg & (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) != 0U) {
		mp_printf("ERROR: SCTLR_EL2.TWEDEL not zero\n");
		return TEST_RESULT_FAIL;
	}

	if ((reg & SCTLR_TWEDEn_BIT) != 0U) {
		mp_printf("ERROR: SCTLR_EL2.TWEDEn set\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * Set all TWED field bits and read back to make sure they are
	 * writable.
	 */
	write_sctlr_el2(reg | (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT) |
	                SCTLR_TWEDEn_BIT);
	reg = read_sctlr_el2();

	if ((reg & (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) !=
			(SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) {
		mp_printf("ERROR: SCTLR_EL2.TWEDEL not writable\n");
		return TEST_RESULT_FAIL;
	}

	if ((reg & SCTLR_TWEDEn_BIT) == 0U) {
		mp_printf("ERROR: SCTLR_EL2.TWEDEn not writable\n");
		return TEST_RESULT_FAIL;
	}

	/* Clear TWED fields to restore original value */
	reg = ((reg & ~(SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) &
			~SCTLR_TWEDEn_BIT);
	write_sctlr_el2(reg);

	/* Read SCTLR_EL1 fields. */
	mp_printf("Checking SCTLR_EL1 fields\n");
	reg = read_sctlr_el1();

	if ((reg & (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) != 0U) {
		mp_printf("ERROR: SCTLR_EL1.TWEDEL not zero\n");
		return TEST_RESULT_FAIL;
	}

	if ((reg & SCTLR_TWEDEn_BIT) != 0U) {
		mp_printf("ERROR: SCTLR_EL1.TWEDEn set\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * Set all TWED field bits and read back to make sure they are
	 * writable.
	 */
	write_sctlr_el1(reg | (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT) |
			SCTLR_TWEDEn_BIT);
	reg = read_sctlr_el1();

	if ((reg & (SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) !=
			(SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) {
		mp_printf("ERROR: SCTLR_EL1.TWEDEL not writable\n");
		return TEST_RESULT_FAIL;
	}

	if ((reg & SCTLR_TWEDEn_BIT) == 0U) {
		mp_printf("ERROR: SCTLR_EL1.TWEDEn not writable\n");
		return TEST_RESULT_FAIL;
	}

	/* Clear TWED fields to restore original value */
	reg = ((reg & ~(SCTLR_TWEDEL_MASK << SCTLR_TWEDEL_SHIFT)) &
			~SCTLR_TWEDEn_BIT);
	write_sctlr_el1(reg);

	return TEST_RESULT_SUCCESS;
}
