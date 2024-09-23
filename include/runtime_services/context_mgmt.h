/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONTEXT_MGMT_H
#define CONTEXT_MGMT_H

#include <ffa_helpers.h>
#include <spm_common.h>

/**
 * Register value for corruption test and various register fields.
 */
#define REG_CORRUPTION_VALUE	ULL(0xffffffffffffffff)
#define SCTLR_EL2_EE			ULL(0x2000000)
#define TTBR1_EL2_ASID			ULL(0xffff000000000000)
#define TCR2_EL2_POE			ULL(0x8)
#define GCSCR_EL2_PCRSEL		ULL(0x1)

/**
 * Read EL2 system registers and save the values into the given context pointer.
 */
static void el2_save_registers(el2_sysregs_t *ctx) {
	el2_common_regs_t *ctx_common = &ctx->common;

	ctx_common->actlr_el2 = read_actlr_el2();
	ctx_common->afsr0_el2 = read_afsr0_el2();
	ctx_common->afsr1_el2 = read_afsr1_el2();
	ctx_common->amair_el2 = read_amair_el2();
	ctx_common->cnthctl_el2 = read_cnthctl_el2();
	ctx_common->cntvoff_el2 = read_cntvoff_el2();
	ctx_common->cptr_el2 = read_cptr_el2();
	ctx_common->dbgvcr32_el2 = read_dbgvcr32_el2();
	ctx_common->elr_el2 = read_elr_el2();
	ctx_common->esr_el2 = read_esr_el2();
	ctx_common->far_el2 = read_far_el2();
	ctx_common->hacr_el2 = read_hacr_el2();
	ctx_common->hcr_el2 = read_hcr_el2();
	ctx_common->hpfar_el2 = read_hpfar_el2();
	ctx_common->hstr_el2 = read_hstr_el2();
	ctx_common->icc_sre_el2 = read_icc_sre_el2();
	ctx_common->ich_hcr_el2 = read_ich_hcr_el2();
	ctx_common->ich_vmcr_el2 = read_ich_vmcr_el2();
	ctx_common->mair_el2 = read_mair_el2();
	ctx_common->mdcr_el2 = read_mdcr_el2();
	ctx_common->pmscr_el2 = read_pmscr_el2();
	ctx_common->sctlr_el2 = read_sctlr_el2();
	ctx_common->spsr_el2 = read_spsr_el2();
	ctx_common->tcr_el2 = read_tcr_el2();
	ctx_common->tpidr_el2 = read_tpidr_el2();
	ctx_common->ttbr0_el2 = read_ttbr0_el2();
	ctx_common->vbar_el2 = read_vbar_el2();
	ctx_common->vmpidr_el2 = read_vmpidr_el2();
	ctx_common->vpidr_el2 = read_vpidr_el2();
	ctx_common->vtcr_el2 = read_vtcr_el2();
	ctx_common->vttbr_el2 = read_vttbr_el2();

	if (read_spsel() == 1) {
		ctx_common->sp_el2 = read_sp();
	}

	if (get_armv8_5_mte_support() == 2) {
		ctx->mte2.tfsr_el2 = read_tfsr_el2();
	}

	if (is_armv8_6_fgt_present()) {
		ctx->fgt.hdfgrtr_el2 = read_hdfgrtr_el2();
		if (is_armv8_4_amuv1_present()) {
			ctx->fgt.hafgrtr_el2 = read_hafgrtr_el2();
		}
		ctx->fgt.hdfgwtr_el2 = read_hdfgwtr_el2();
		ctx->fgt.hfgitr_el2 = read_hfgitr_el2();
		ctx->fgt.hfgrtr_el2 = read_hfgrtr_el2();
		ctx->fgt.hfgwtr_el2 = read_hfgwtr_el2();
	}

	if (is_armv8_9_fgt2_present()) {
		ctx->fgt2.hdfgrtr2_el2 = read_hdfgrtr2_el2();
		ctx->fgt2.hdfgwtr2_el2 = read_hdfgwtr2_el2();
		ctx->fgt2.hfgitr2_el2 = read_hfgitr2_el2();
		ctx->fgt2.hfgrtr2_el2 = read_hfgrtr2_el2();
		ctx->fgt2.hfgwtr2_el2 = read_hfgwtr2_el2();
	}

	if (get_armv8_6_ecv_support() == ID_AA64MMFR0_EL1_ECV_SELF_SYNCH) {
		ctx->ecv.cntpoff_el2 = read_cntpoff_el2();
	}

	if (is_armv8_1_vhe_present()) {
		ctx->vhe.contextidr_el2 = read_contextidr_el2();
		ctx->vhe.ttbr1_el2 = read_ttbr1_el2();
	}

	if (is_feat_ras_present() || is_feat_rasv1p1_present()) {
		ctx->ras.vdisr_el2 = read_vdisr_el2();
		ctx->ras.vsesr_el2 = read_vsesr_el2();
	}

	if (is_armv8_4_nv2_present()) {
		ctx->neve.vncr_el2 = read_vncr_el2();
	}

	if (get_armv8_4_trf_support()) {
		ctx->trf.trfcr_el2 = read_trfcr_el2();
	}

	if (is_feat_csv2_2_present()) {
		ctx->csv2.scxtnum_el2 = read_scxtnum_el2();
	}

	if (get_feat_hcx_support()) {
		ctx->hcx.hcrx_el2 = read_hcrx_el2();
	}

	if (is_feat_tcr2_present()) {
		ctx->tcr2.tcr2_el2 = read_tcr2_el2();
	}

	if (is_armv8_9_sxpoe_present()) {
		ctx->sxpoe.por_el2 = read_por_el2();
	}

	if (is_armv8_9_sxpie_present()) {
		ctx->sxpie.pire0_el2 = read_pire0_el2();
		ctx->sxpie.pir_el2 = read_pir_el2();
	}

	if (is_armv8_9_s2pie_present()) {
		ctx->s2pie.s2pir_el2 = read_s2pir_el2();
	}

	if (is_armv9_4_gcs_present()) {
		ctx->gcs.gcscr_el2 = read_gcscr_el2();
		ctx->gcs.gcspr_el2 = read_gcspr_el2();
	}

	if (is_feat_mpam_supported()) {
		ctx->mpam.mpam2_el2 = read_mpam2_el2();
		/* Unable to access these registers. */
		//ctx->mpam.mpamhcr_el2 = read_mpamhcr_el2();
		//ctx->mpam.mpamvpm0_el2 = read_mpamvpm0_el2();
		//ctx->mpam.mpamvpm1_el2 = read_mpamvpm1_el2();
		//ctx->mpam.mpamvpm2_el2 = read_mpamvpm2_el2();
		//ctx->mpam.mpamvpm3_el2 = read_mpamvpm3_el2();
		//ctx->mpam.mpamvpm4_el2 = read_mpamvpm4_el2();
		//ctx->mpam.mpamvpm5_el2 = read_mpamvpm5_el2();
		//ctx->mpam.mpamvpm6_el2 = read_mpamvpm6_el2();
		//ctx->mpam.mpamvpm7_el2 = read_mpamvpm7_el2();
		//ctx->mpam.mpamvpmv_el2 = read_mpamvpmv_el2();
	}
}

/**
 * Restore EL2 system registers from a given context pointer, whose values are OR'ed with or_value.
 */
static void el2_restore_registers_with_value(el2_sysregs_t *ctx, uint64_t or_value) {
	el2_common_regs_t *ctx_common = &ctx->common;

	write_actlr_el2(ctx_common->actlr_el2 | or_value);
	write_afsr0_el2(ctx_common->afsr0_el2 | or_value);
	write_afsr1_el2(ctx_common->afsr1_el2 | or_value);
	write_amair_el2(ctx_common->amair_el2 | or_value);
	write_cnthctl_el2(ctx_common->cnthctl_el2 | or_value);
	write_cntvoff_el2(ctx_common->cntvoff_el2 | or_value);
	write_cptr_el2(ctx_common->cptr_el2 | or_value);
	write_dbgvcr32_el2(ctx_common->dbgvcr32_el2 | or_value);
	write_elr_el2(ctx_common->elr_el2 | or_value);
	write_esr_el2(ctx_common->esr_el2 | or_value);
	write_far_el2(ctx_common->far_el2 | or_value);
	write_hacr_el2(ctx_common->hacr_el2 | or_value);
	write_hcr_el2(ctx_common->hcr_el2 | or_value);
	write_hpfar_el2(ctx_common->hpfar_el2 | or_value);
	write_hstr_el2(ctx_common->hstr_el2 | or_value);
	write_icc_sre_el2(ctx_common->icc_sre_el2 | or_value);
	write_ich_hcr_el2(ctx_common->ich_hcr_el2 | or_value);
	/* Unable to restore ICH_VMCR_EL2 to original value after modification. */
	//write_ich_vmcr_el2(ctx_common->ich_vmcr_el2 | or_value);
	write_mair_el2(ctx_common->mair_el2 | or_value);
	write_mdcr_el2(ctx_common->mdcr_el2 | or_value);
	write_pmscr_el2(ctx_common->pmscr_el2 | or_value);
	write_sctlr_el2(ctx_common->sctlr_el2 | (or_value & ~SCTLR_EL2_EE));
	write_spsr_el2(ctx_common->spsr_el2 | or_value);
	write_tcr_el2(ctx_common->tcr_el2 | or_value);
	write_tpidr_el2(ctx_common->tpidr_el2 | or_value);
	write_ttbr0_el2(ctx_common->ttbr0_el2 | or_value);
	write_vbar_el2(ctx_common->vbar_el2 | or_value);
	write_vmpidr_el2(ctx_common->vmpidr_el2 | or_value);
	write_vpidr_el2(ctx_common->vpidr_el2 | or_value);
	write_vtcr_el2(ctx_common->vtcr_el2 | or_value);
	write_vttbr_el2(ctx_common->vttbr_el2 | or_value);

	/* The stack pointer should not be modified. */
	//if (read_spsel() == 1) {
	//	write_sp(ctx_common->sp_el2 | or_value);
	//}

	if (get_armv8_5_mte_support() == 2) {
		write_tfsr_el2(ctx->mte2.tfsr_el2 | or_value);
	}

	if (is_armv8_6_fgt_present()) {
		write_hdfgrtr_el2(ctx->fgt.hdfgrtr_el2 | or_value);
		if (is_armv8_4_amuv1_present()) {
			write_hafgrtr_el2(ctx->fgt.hafgrtr_el2 | or_value);
		}
		write_hdfgwtr_el2(ctx->fgt.hdfgwtr_el2 | or_value);
		write_hfgitr_el2(ctx->fgt.hfgitr_el2 | or_value);
		write_hfgrtr_el2(ctx->fgt.hfgrtr_el2 | or_value);
		write_hfgwtr_el2(ctx->fgt.hfgwtr_el2 | or_value);
	}

	if (is_armv8_9_fgt2_present()) {
		write_hdfgrtr2_el2(ctx->fgt2.hdfgrtr2_el2 | or_value);
		write_hdfgwtr2_el2(ctx->fgt2.hdfgwtr2_el2 | or_value);
		write_hfgitr2_el2(ctx->fgt2.hfgitr2_el2 | or_value);
		write_hfgrtr2_el2(ctx->fgt2.hfgrtr2_el2 | or_value);
		write_hfgwtr2_el2(ctx->fgt2.hfgwtr2_el2 | or_value);
	}

	if (get_armv8_6_ecv_support() == ID_AA64MMFR0_EL1_ECV_SELF_SYNCH) {
		write_cntpoff_el2(ctx->ecv.cntpoff_el2 | or_value);
	}

	if (is_armv8_1_vhe_present()) {
		write_contextidr_el2(ctx->vhe.contextidr_el2 | or_value);
		write_ttbr1_el2(ctx->vhe.ttbr1_el2 | (or_value & ~TTBR1_EL2_ASID));
	}

	if (is_feat_ras_present() || is_feat_rasv1p1_present()) {
		write_vdisr_el2(ctx->ras.vdisr_el2 | or_value);
		write_vsesr_el2(ctx->ras.vsesr_el2 | or_value);
	}

	if (is_armv8_4_nv2_present()) {
		write_vncr_el2(ctx->neve.vncr_el2 | or_value);
	}

	if (get_armv8_4_trf_support()) {
		write_trfcr_el2(ctx->trf.trfcr_el2 | or_value);
	}

	if (is_feat_csv2_2_present()) {
		write_scxtnum_el2(ctx->csv2.scxtnum_el2 | or_value);
	}

	if (get_feat_hcx_support()) {
		write_hcrx_el2(ctx->hcx.hcrx_el2 | or_value);
	}

	if (is_feat_tcr2_present()) {
		write_tcr2_el2(ctx->tcr2.tcr2_el2 | (or_value & ~TCR2_EL2_POE));
	}

	if (is_armv8_9_sxpoe_present()) {
		write_por_el2(ctx->sxpoe.por_el2 | or_value);
	}

	if (is_armv8_9_sxpie_present()) {
		write_pire0_el2(ctx->sxpie.pire0_el2 | or_value);
		write_pir_el2(ctx->sxpie.pir_el2 | or_value);
	}

	if (is_armv8_9_s2pie_present()) {
		write_s2pir_el2(ctx->s2pie.s2pir_el2 | or_value);
	}

	if (is_armv9_4_gcs_present()) {
		write_gcscr_el2(ctx->gcs.gcscr_el2 | (or_value & ~GCSCR_EL2_PCRSEL));
		write_gcspr_el2(ctx->gcs.gcspr_el2 | or_value);
	}

	if (is_feat_mpam_supported()) {
		/* MPAM2_EL2 may change before context switching occurs. */
		//write_mpam2_el2(ctx->mpam.mpam2_el2 | or_value);
		/* Unable to access these registers. */
		//write_mpamhcr_el2(ctx->mpam.mpamhcr_el2 | or_value);
		//write_mpamvpm0_el2(ctx->mpam.mpamvpm0_el2 | or_value);
		//write_mpamvpm1_el2(ctx->mpam.mpamvpm1_el2 | or_value);
		//write_mpamvpm2_el2(ctx->mpam.mpamvpm2_el2 | or_value);
		//write_mpamvpm3_el2(ctx->mpam.mpamvpm3_el2 | or_value);
		//write_mpamvpm4_el2(ctx->mpam.mpamvpm4_el2 | or_value);
		//write_mpamvpm5_el2(ctx->mpam.mpamvpm5_el2 | or_value);
		//write_mpamvpm6_el2(ctx->mpam.mpamvpm6_el2 | or_value);
		//write_mpamvpm7_el2(ctx->mpam.mpamvpm7_el2 | or_value);
		//write_mpamvpmv_el2(ctx->mpam.mpamvpmv_el2 | or_value);
	}
}

/**
 * Dump (print out) the EL2 system register from a given context pointer.
 */
static void el2_dump_register_context(el2_sysregs_t *ctx) {
	el2_common_regs_t *ctx_common = &ctx->common;

	INFO("\tCommon registers:\n");
	INFO("\t-- ACTLR_EL2: 0x%llx\n", ctx_common->actlr_el2);
	INFO("\t-- AFSR0_EL2: 0x%llx\n", ctx_common->afsr0_el2);
	INFO("\t-- AFSR1_EL2: 0x%llx\n", ctx_common->afsr1_el2);
	INFO("\t-- AMAIR_EL2: 0x%llx\n", ctx_common->amair_el2);
	INFO("\t-- CNTHCTL_EL2: 0x%llx\n", ctx_common->cnthctl_el2);
	INFO("\t-- CNTVOFF_EL2: 0x%llx\n", ctx_common->cntvoff_el2);
	INFO("\t-- CPTR_EL2: 0x%llx\n", ctx_common->cptr_el2);
	INFO("\t-- DBGVCR32_EL2: 0x%llx\n", ctx_common->dbgvcr32_el2);
	INFO("\t-- ELR_EL2: 0x%llx\n", ctx_common->elr_el2);
	INFO("\t-- ESR_EL2: 0x%llx\n", ctx_common->esr_el2);
	INFO("\t-- FAR_EL2: 0x%llx\n", ctx_common->far_el2);
	INFO("\t-- HACR_EL2: 0x%llx\n", ctx_common->hacr_el2);
	INFO("\t-- HCR_EL2: 0x%llx\n", ctx_common->hcr_el2);
	INFO("\t-- HPFAR_EL2: 0x%llx\n", ctx_common->hpfar_el2);
	INFO("\t-- HSTR_EL2: 0x%llx\n", ctx_common->hstr_el2);
	INFO("\t-- ICC_SRE_EL2: 0x%llx\n", ctx_common->icc_sre_el2);
	INFO("\t-- ICH_HCR_EL2: 0x%llx\n", ctx_common->ich_hcr_el2);
	INFO("\t-- ICH_VMCR_EL2: 0x%llx\n", ctx_common->ich_vmcr_el2);
	INFO("\t-- MAIR_EL2: 0x%llx\n", ctx_common->mair_el2);
	INFO("\t-- MDCR_EL2: 0x%llx\n", ctx_common->mdcr_el2);
	INFO("\t-- PMSCR_EL2: 0x%llx\n", ctx_common->pmscr_el2);
	INFO("\t-- SCTLR_EL2: 0x%llx\n", ctx_common->sctlr_el2);
	INFO("\t-- SPSR_EL2: 0x%llx\n", ctx_common->spsr_el2);
	INFO("\t-- SP_EL2: 0x%llx\n", ctx_common->sp_el2);
	INFO("\t-- TCR_EL2: 0x%llx\n", ctx_common->tcr_el2);
	INFO("\t-- TPIDR_EL2: 0x%llx\n", ctx_common->tpidr_el2);
	INFO("\t-- TTBR0_EL2: 0x%llx\n", ctx_common->ttbr0_el2);
	INFO("\t-- VBAR_EL2: 0x%llx\n", ctx_common->vbar_el2);
	INFO("\t-- VMPIDR_EL2: 0x%llx\n", ctx_common->vmpidr_el2);
	INFO("\t-- VPIDR_EL2: 0x%llx\n", ctx_common->vpidr_el2);
	INFO("\t-- VTCR_EL2: 0x%llx\n", ctx_common->vtcr_el2);
	INFO("\t-- VTTBR_EL2: 0x%llx\n\n", ctx_common->vttbr_el2);

	INFO("\tMTE2 registers:\n");
	INFO("\t-- TFSR_EL2: 0x%llx\n\n", ctx->mte2.tfsr_el2);

	INFO("\tFGT registers:\n");
	INFO("\t-- HDFGRTR_EL2: 0x%llx\n", ctx->fgt.hdfgrtr_el2);
	INFO("\t-- HAFGRTR_EL2: 0x%llx\n", ctx->fgt.hafgrtr_el2);
	INFO("\t-- HDFGWTR_EL2: 0x%llx\n", ctx->fgt.hdfgwtr_el2);
	INFO("\t-- HFGITR_EL2: 0x%llx\n", ctx->fgt.hfgitr_el2);
	INFO("\t-- HFGRTR_EL2: 0x%llx\n", ctx->fgt.hfgrtr_el2);
	INFO("\t-- HFGWTR_EL2: 0x%llx\n\n", ctx->fgt.hfgwtr_el2);

	INFO("\tFGT2 registers:\n");
	INFO("\t-- HDFGRTR2_EL2: 0x%llx\n", ctx->fgt2.hdfgrtr2_el2);
	INFO("\t-- HDFGWTR2_EL2: 0x%llx\n", ctx->fgt2.hdfgwtr2_el2);
	INFO("\t-- HFGITR2_EL2: 0x%llx\n", ctx->fgt2.hfgitr2_el2);
	INFO("\t-- HFGRTR2_EL2: 0x%llx\n", ctx->fgt2.hfgrtr2_el2);
	INFO("\t-- HFGWTR2_EL2: 0x%llx\n\n", ctx->fgt2.hfgwtr2_el2);

	INFO("\tECV registers:\n");
	INFO("\t-- CNTPOFF_EL2: 0x%llx\n\n", ctx->ecv.cntpoff_el2);

	INFO("\tVHE registers:\n");
	INFO("\t-- CONTEXTIDR_EL2: 0x%llx\n", ctx->vhe.contextidr_el2);
	INFO("\t-- TTBR1_EL2: 0x%llx\n\n", ctx->vhe.ttbr1_el2);

	INFO("\tRAS registers:\n");
	INFO("\t-- VDISR_EL2: 0x%llx\n", ctx->ras.vdisr_el2);
	INFO("\t-- VSESR_EL2: 0x%llx\n\n", ctx->ras.vsesr_el2);

	INFO("\tNEVE registers:\n");
	INFO("\t-- VNCR_EL2: 0x%llx\n\n", ctx->neve.vncr_el2);

	INFO("\tTRF registers:\n");
	INFO("\t-- TRFCR_EL2: 0x%llx\n\n", ctx->trf.trfcr_el2);

	INFO("\tCSV2 registers:\n");
	INFO("\t-- SCXTNUM_EL2: 0x%llx\n\n", ctx->csv2.scxtnum_el2);

	INFO("\tHCX registers:\n");
	INFO("\t-- HCRX_EL2: 0x%llx\n\n", ctx->hcx.hcrx_el2);

	INFO("\tTCR2 registers:\n");
	INFO("\t-- TCR2_EL2: 0x%llx\n\n", ctx->tcr2.tcr2_el2);

	INFO("\tSxPOE registers:\n");
	INFO("\t-- POR_EL2: 0x%llx\n\n", ctx->sxpoe.por_el2);

	INFO("\tSxPIE registers:\n");
	INFO("\t-- PIRE0_EL2: 0x%llx\n", ctx->sxpie.pire0_el2);
	INFO("\t-- PIR_EL2: 0x%llx\n\n", ctx->sxpie.pir_el2);

	INFO("\tS2PIE registers:\n");
	INFO("\t-- S2PIR_EL2: 0x%llx\n\n", ctx->s2pie.s2pir_el2);

	INFO("\tGCS registers:\n");
	INFO("\t-- GCSCR_EL2: 0x%llx\n", ctx->gcs.gcscr_el2);
	INFO("\t-- GCSPR_EL2: 0x%llx\n\n", ctx->gcs.gcspr_el2);

	INFO("\tMPAM registers:\n");
	INFO("\t-- MPAM2_EL2: 0x%llx\n", ctx->mpam.mpam2_el2);
	INFO("\t-- MPAMHCR_EL2: 0x%llx\n", ctx->mpam.mpamhcr_el2);
	INFO("\t-- MPAMVPM0_EL2: 0x%llx\n", ctx->mpam.mpamvpm0_el2);
	INFO("\t-- MPAMVPM1_EL2: 0x%llx\n", ctx->mpam.mpamvpm1_el2);
	INFO("\t-- MPAMVPM2_EL2: 0x%llx\n", ctx->mpam.mpamvpm2_el2);
	INFO("\t-- MPAMVPM3_EL2: 0x%llx\n", ctx->mpam.mpamvpm3_el2);
	INFO("\t-- MPAMVPM4_EL2: 0x%llx\n", ctx->mpam.mpamvpm4_el2);
	INFO("\t-- MPAMVPM5_EL2: 0x%llx\n", ctx->mpam.mpamvpm5_el2);
	INFO("\t-- MPAMVPM6_EL2: 0x%llx\n", ctx->mpam.mpamvpm6_el2);
	INFO("\t-- MPAMVPM7_EL2: 0x%llx\n", ctx->mpam.mpamvpm7_el2);
	INFO("\t-- MPAMVPMV_EL2: 0x%llx\n\n", ctx->mpam.mpamvpmv_el2);
}

#endif /* CONTEXT_MGMT_H */
