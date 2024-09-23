/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONTEXT_EL2_H
#define CONTEXT_EL2_H

#include <stdint.h>

#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>

/*******************************************************************************
 * EL2 Registers:
 * AArch64 EL2 system register context structure for preserving the
 * architectural state during world switches.
 ******************************************************************************/
typedef struct el2_common_regs {
	uint64_t actlr_el2;
	uint64_t afsr0_el2;
	uint64_t afsr1_el2;
	uint64_t amair_el2;
	uint64_t cnthctl_el2;
	uint64_t cntvoff_el2;
	uint64_t cptr_el2;
	uint64_t dbgvcr32_el2;
	uint64_t elr_el2;
	uint64_t esr_el2;
	uint64_t far_el2;
	uint64_t hacr_el2;
	uint64_t hcr_el2;
	uint64_t hpfar_el2;
	uint64_t hstr_el2;
	uint64_t icc_sre_el2;
	uint64_t ich_hcr_el2;
	uint64_t ich_vmcr_el2;
	uint64_t mair_el2;
	uint64_t mdcr_el2;
	uint64_t pmscr_el2;
	uint64_t sctlr_el2;
	uint64_t spsr_el2;
	uint64_t sp_el2;
	uint64_t tcr_el2;
	uint64_t tpidr_el2;
	uint64_t ttbr0_el2;
	uint64_t vbar_el2;
	uint64_t vmpidr_el2;
	uint64_t vpidr_el2;
	uint64_t vtcr_el2;
	uint64_t vttbr_el2;
} el2_common_regs_t;

typedef struct el2_mte2_regs {
	uint64_t tfsr_el2;
} el2_mte2_regs_t;

typedef struct el2_fgt_regs {
	uint64_t hdfgrtr_el2;
	uint64_t hafgrtr_el2;
	uint64_t hdfgwtr_el2;
	uint64_t hfgitr_el2;
	uint64_t hfgrtr_el2;
	uint64_t hfgwtr_el2;
} el2_fgt_regs_t;

typedef struct el2_fgt2_regs {
	uint64_t hdfgrtr2_el2;
	uint64_t hdfgwtr2_el2;
	uint64_t hfgitr2_el2;
	uint64_t hfgrtr2_el2;
	uint64_t hfgwtr2_el2;
} el2_fgt2_regs_t;

typedef struct el2_ecv_regs {
	uint64_t cntpoff_el2;
} el2_ecv_regs_t;

typedef struct el2_vhe_regs {
	uint64_t contextidr_el2;
	uint64_t ttbr1_el2;
} el2_vhe_regs_t;

typedef struct el2_ras_regs {
	uint64_t vdisr_el2;
	uint64_t vsesr_el2;
} el2_ras_regs_t;

typedef struct el2_neve_regs {
	uint64_t vncr_el2;
} el2_neve_regs_t;

typedef struct el2_trf_regs {
	uint64_t trfcr_el2;
} el2_trf_regs_t;

typedef struct el2_csv2_regs {
	uint64_t scxtnum_el2;
} el2_csv2_regs_t;

typedef struct el2_hcx_regs {
	uint64_t hcrx_el2;
} el2_hcx_regs_t;

typedef struct el2_tcr2_regs {
	uint64_t tcr2_el2;
} el2_tcr2_regs_t;

typedef struct el2_sxpoe_regs {
	uint64_t por_el2;
} el2_sxpoe_regs_t;

typedef struct el2_sxpie_regs {
	uint64_t pire0_el2;
	uint64_t pir_el2;
} el2_sxpie_regs_t;

typedef struct el2_s2pie_regs {
	uint64_t s2pir_el2;
} el2_s2pie_regs_t;

typedef struct el2_gcs_regs {
	uint64_t gcscr_el2;
	uint64_t gcspr_el2;
} el2_gcs_regs_t;

typedef struct el2_mpam_regs {
	uint64_t mpam2_el2;
	uint64_t mpamhcr_el2;
	uint64_t mpamvpm0_el2;
	uint64_t mpamvpm1_el2;
	uint64_t mpamvpm2_el2;
	uint64_t mpamvpm3_el2;
	uint64_t mpamvpm4_el2;
	uint64_t mpamvpm5_el2;
	uint64_t mpamvpm6_el2;
	uint64_t mpamvpm7_el2;
	uint64_t mpamvpmv_el2;
} el2_mpam_regs_t;

typedef struct el2_sysregs {
	el2_common_regs_t common;

	el2_mte2_regs_t mte2;
	el2_fgt_regs_t fgt;
	el2_fgt2_regs_t fgt2;
	el2_ecv_regs_t ecv;
	el2_vhe_regs_t vhe;
	el2_ras_regs_t ras;
	el2_neve_regs_t neve;
	el2_trf_regs_t trf;
	el2_csv2_regs_t csv2;
	el2_hcx_regs_t hcx;
	el2_tcr2_regs_t tcr2;
	el2_sxpoe_regs_t sxpoe;
	el2_sxpie_regs_t sxpie;
	el2_s2pie_regs_t s2pie;
	el2_gcs_regs_t gcs;
	el2_mpam_regs_t mpam;
} el2_sysregs_t;

/******************************************************************************/

#endif /* CONTEXT_EL2_H */
