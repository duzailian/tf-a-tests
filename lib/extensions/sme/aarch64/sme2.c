/*
 * Copyright (c) 2023, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdio.h>

#include <arch.h>
#include <arch_helpers.h>
#include <lib/extensions/sme.h>
#include <tftf_lib.h>

#ifdef __aarch64__

/*
 * is_feat_sme2_supported
 * Check if SME is supported on this platform.
 * Return : true if SME2 supported, false if not.
 */
bool is_feat_sme2_supported(void)
{
	uint64_t features;

	features = read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_SME_SHIFT;
	return (features & ID_AA64PFR1_EL1_SME_MASK) >= ID_AA64PFR1_EL1_SME2_SUPPORTED;
}

/*
 * sme2_enable
 * Enable SME2 for nonsecure use at EL2 for TFTF cases.
 * Return : 0 if successful.
 */
int sme2_enable(void)
{
	u_register_t reg;

	/* Make sure SME2 is supported. */
	if (!is_feat_sme2_supported()) {
		return -1;
	}

	/* SME2 is an extended version of SME.
	 * Therefore, SME accesses still must be taken care by setting
	 * appropriate fields in order to avoid traps in CPTR_EL2.
	 */
	if (sme_enable() != 0) {
		tftf_testcase_printf("Could not enable SME.\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * Make sure ZT0 register access don't cause traps by setting
	 * appropriate field in SMCR_EL2 register.
	 */
	reg = read_smcr_el2();
	reg |= SMCR_ELX_EZT0_BIT;
	write_smcr_el2(reg);

	return 0;
}

#endif /* __aarch64__ */
