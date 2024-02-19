/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_features.h>
#include <sp_helpers.h>

static void cpu_check_id_regs(void)
{
	/* ID_AA64PFR0_EL1 */

	// EL0/1/2/3
	expect(is_feat_advsimd_present(), true);
	expect(is_feat_fp_present(), true);
	// GIC
	// RAS
	expect(is_armv8_2_sve_present(), false);
	// SEL2
	// MPAM
	// AMU
	// DIT
	// RME
	// CSV2
	// CSV3

	/* ID_AA64PFR1_EL1 */

	// BT
	// SSBS
	// MTE
	// RAS_frac
	// MPAM_frac
	expect(is_feat_sme_supported(), false);
	// RNDR_trap
	// CSV2_frac
	// NMI
}

void cpu_tests(void)
{
	const char *test_cpu_str = "CPU tests";

	announce_test_section_start(test_cpu_str);
	cpu_check_id_regs();
	announce_test_section_end(test_cpu_str);
}
