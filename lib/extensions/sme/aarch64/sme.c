/*
 * Copyright (c) 2021-2023, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdio.h>

#include <arch.h>
#include <arch_helpers.h>
#include <debug.h>
#include <lib/extensions/sme.h>


#ifdef __aarch64__

/*
 * feat_sme_supported
 *   Check if SME is supported on this platform.
 * Return
 *   true if SME supported, false if not.
 */
bool is_feat_sme_supported(void)
{
	uint64_t features;

	features = read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_SME_SHIFT;
	return (features & ID_AA64PFR1_EL1_SME_MASK) >= ID_AA64PFR1_EL1_SME_SUPPORTED;
}

/*
 * is_feat_sme_fa64_supported
 *   Check if FEAT_SME_FA64 is supported.
 * Return: True if supported, false if not.
 */
bool is_feat_sme_fa64_supported(void)
{
	uint64_t features;

	features = read_id_aa64smfr0_el1();
	return (features & ID_AA64SMFR0_EL1_FA64_BIT) != 0U;
}

/*
 * sme_enable
 *   Enable SME for nonsecure use at EL2 for TFTF cases.
 * Return
 *   0 if successful.
 */
int sme_enable(void)
{
	u_register_t reg;

	/* Make sure SME is supported. */
	if (!is_feat_sme_supported()) {
		return -1;
	}

	/*
	 * Make sure SME accesses don't cause traps by setting appropriate fields
	 * in CPTR_EL2.
	 */
	reg = read_cptr_el2();
	if ((read_hcr_el2() & HCR_E2H_BIT) == 0U) {
		/* When HCR_EL2.E2H == 0, clear TSM bit in CPTR_EL2. */
		reg = reg & ~CPTR_EL2_TSM_BIT;
	} else {
		/* When HCR_EL2.E2H == 1, set SMEN bits in CPTR_EL2. */
		reg = reg | (CPTR_EL2_SMEN_MASK << CPTR_EL2_SMEN_SHIFT);
	}
	write_cptr_el2(reg);

	return 0;
}

/*
 * sme_smstart
 *   This function enables streaming mode and ZA array storage access
 * independently or together based on the type of instruction variant.
 * Parameters
 *   smstart_type: If SMSTART, streaming mode and ZA access is enabled
 *                 If SMSTART_SM, streaming mode enabled.
 *                 If SMSTART_ZA enables SME ZA storage and, ZT0 storage access.
 */
void sme_smstart(smestart_instruction_type_t smstart_type)
{
	u_register_t svcr = 0ULL;

	switch(smstart_type) {
	case SMSTART:
		svcr = (SVCR_SM_BIT | SVCR_ZA_BIT);
		break;

	case SMSTART_SM:
		svcr = SVCR_SM_BIT;
		break;

	case SMSTART_ZA:
		svcr = SVCR_ZA_BIT;
		break;

	default:
		ERROR("Illegal SMSTART Instruction Variant\n");
		break;
	}
	write_svcr(read_svcr() | svcr);
}

/*
 * sme_smstop
 *   This function exits streaming mode and disables ZA array storage access
 * independently or together based on the type of instruction variant.
 * Parameters
 *   smstop_type: If SMSTOP, exits streaming mode and ZA access is disabled
 *                If SMSTOP_SM, exits streaming mode.
 *                If SMSTOP_ZA disables SME ZA storage and, ZT0 storage access.
 */
void sme_smstop(smestop_instruction_type_t smstop_type)
{
	u_register_t svcr = 0ULL;

	switch(smstop_type) {
	case SMSTOP:
		svcr = (~SVCR_SM_BIT) & (~SVCR_ZA_BIT);
		break;

	case SMSTOP_SM:
		svcr = ~SVCR_SM_BIT;
		break;

	case SMSTOP_ZA:
		svcr = ~SVCR_ZA_BIT;
		break;

	default:
		ERROR("Illegal SMSTOP Instruction Variant\n");
		break;
	}
	write_svcr(read_svcr() & svcr);
}

#endif /* __aarch64__ */
