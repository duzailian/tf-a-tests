/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <debug.h>
#include <stdlib.h>
#include <test_helpers.h>
#include <tftf_lib.h>

#ifdef __aarch64__
/*
 * Get SPE version value from id_aa64dfr0_el1.
 * Return values
 *   ID_AA64DFR0_SPE_NOT_SUPPORTED: not supported
 *   ID_AA64DFR0_SPE: FEAT_SPE supported (introduced in ARM v8.2)
 *   ID_AA64DFR0_SPE_V1P1: FEAT_SPEv1p1 supported (introduced in ARM v8.5)
 *   ID_AA64DFR0_SPE_V1P2: FEAT_SPEv1p2 supported (introduced in ARM v8.7)
 */

typedef enum {
	ID_AA64DFR0_SPE_NOT_SUPPORTED = 0,
	ID_AA64DFR0_SPE,
	ID_AA64DFR0_SPE_V1P1,
	ID_AA64DFR0_SPE_V1P2
} spe_ver_t;

static spe_ver_t spe_get_version(void)
{
	return (spe_ver_t)((read_id_aa64dfr0_el1() >> ID_AA64DFR0_PMS_SHIFT) &
		ID_AA64DFR0_PMS_MASK);
}
#endif /* __aarch64__ */

test_result_t test_spe_support(void)
{
	/* SPE is an AArch64-only feature.*/
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	spe_ver_t spe_ver = spe_get_version();

	assert(spe_ver <= ID_AA64DFR0_SPE_V1P2);

	if(spe_ver == ID_AA64DFR0_SPE_NOT_SUPPORTED) {
		return TEST_RESULT_SKIPPED;
	} else {
		/*
		 * If runtime-EL3 does not enable access to SPE system
		 * registers from NS-EL2/NS-EL1 then read of these
		 * registers traps in EL3
		 */
		read_pmscr_el1();
		read_pmsfcr_el1();
		read_pmsicr_el1();
		read_pmsidr_el1();
		read_pmsirr_el1();
		read_pmslatfr_el1();
		read_pmblimitr_el1();
		read_pmbptr_el1();
		read_pmbsr_el1();
		read_pmsevfr_el1();
		if (el_implemented(MODE_EL2) != EL_IMPL_NONE) {
			read_pmscr_el2();
		}
		if (spe_ver == ID_AA64DFR0_SPE_V1P2) {
			read_pmsnevfr_el1();
		}
	}

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}
