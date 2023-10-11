/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <tftf.h>
#include <assert.h>
#include <host_realm_rmi.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <arch_features.h>
#include <lib/extensions/fpu.h>
#include <lib/extensions/sme.h>
#include <lib/extensions/sve.h>
#include <debug.h>

#include "host_realm_simd_common.h"

static sve_z_regs_t ns_sve_z_regs_write;
static sve_z_regs_t ns_sve_z_regs_read;

static sve_p_regs_t ns_sve_p_regs_write;
static sve_p_regs_t ns_sve_p_regs_read;

static sve_ffr_regs_t ns_sve_ffr_regs_write;
static sve_ffr_regs_t ns_sve_ffr_regs_read;

static fpu_q_reg_t ns_fpu_q_regs_write[FPU_Q_COUNT];
static fpu_q_reg_t ns_fpu_q_regs_read[FPU_Q_COUNT];

static fpu_cs_regs_t ns_fpu_cs_regs_write;
static fpu_cs_regs_t ns_fpu_cs_regs_read;

test_result_t host_create_sve_realm_payload(struct realm *realm, bool sve_en, uint8_t sve_vq)
{
	u_register_t feature_flag;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};

	if (sve_en) {
		feature_flag = RMI_FEATURE_REGISTER_0_SVE_EN |
				INPLACE(FEATURE_SVE_VL, sve_vq);
	} else {
		feature_flag = 0UL;
	}

	/* Initialise Realm payload */
	if (!host_create_activate_realm_payload(realm,
				       (u_register_t)REALM_IMAGE_BASE,
				       (u_register_t)PAGE_POOL_BASE,
				       (u_register_t)PAGE_POOL_MAX_SIZE,
				       feature_flag, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}

	/* Create shared memory between Host and Realm */
	if (!host_create_shared_mem(realm, NS_REALM_SHARED_MEM_BASE,
				    NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/* Generate random values and write it to SVE Z, P and FFR registers */
static void ns_sve_write_rand(void)
{
	bool has_ffr = true;

	if (is_feat_sme_supported() && sme_smstat_sm() &&
	    !sme_feat_fa64_enabled()) {
		has_ffr = false;
	}

	sve_z_regs_write_rand(&ns_sve_z_regs_write);
	sve_p_regs_write_rand(&ns_sve_p_regs_write);
	if (has_ffr) {
		sve_ffr_regs_write_rand(&ns_sve_ffr_regs_write);
	}
}

/* Read SVE Z, P and FFR registers and compare it with the last written values */
static test_result_t ns_sve_read_and_compare(void)
{
	test_result_t rc = TEST_RESULT_SUCCESS;
	uint64_t bitmap;
	bool has_ffr = true;

	if (is_feat_sme_supported() && sme_smstat_sm() &&
	    !sme_feat_fa64_enabled()) {
		has_ffr = false;
	}

	/* Clear old state */
	memset((void *)&ns_sve_z_regs_read, 0, sizeof(ns_sve_z_regs_read));
	memset((void *)&ns_sve_p_regs_read, 0, sizeof(ns_sve_p_regs_read));
	memset((void *)&ns_sve_ffr_regs_read, 0, sizeof(ns_sve_ffr_regs_read));

	/* Read Z, P, FFR registers to compare it with the last written values */
	sve_z_regs_read(&ns_sve_z_regs_read);
	sve_p_regs_read(&ns_sve_p_regs_read);
	if (has_ffr) {
		sve_ffr_regs_read(&ns_sve_ffr_regs_read);
	}

	bitmap = sve_z_regs_compare(&ns_sve_z_regs_write, &ns_sve_z_regs_read);
	if (bitmap != 0UL) {
		ERROR("SVE Z regs compare failed (bitmap: 0x%016llx)\n",
		      bitmap);
		rc = TEST_RESULT_FAIL;
	}

	bitmap = sve_p_regs_compare(&ns_sve_p_regs_write, &ns_sve_p_regs_read);
	if (bitmap != 0UL) {
		ERROR("SVE P regs compare failed (bitmap: 0x%016llx)\n",
		      bitmap);
		rc = TEST_RESULT_FAIL;
	}

	if (has_ffr) {
		bitmap = sve_ffr_regs_compare(&ns_sve_ffr_regs_write,
					      &ns_sve_ffr_regs_read);
		if (bitmap != 0) {
			ERROR("SVE FFR regs compare failed "
			      "(bitmap: 0x%016llx)\n", bitmap);
			rc = TEST_RESULT_FAIL;
		}
	}

	return rc;
}

/*
 * Generate random values and write it to Streaming SVE Z, P and FFR registers.
 */
static void ns_sme_write_rand(void)
{
	/*
	 * TODO: more SME specific registers like ZA, ZT0 can be included later.
	 */

	/* Fill SVE registers in normal or streaming SVE mode */
	ns_sve_write_rand();
}

/*
 * Read streaming SVE Z, P and FFR registers and compare it with the last
 * written values
 */
static test_result_t ns_sme_read_and_compare(void)
{
	/*
	 * TODO: more SME specific registers like ZA, ZT0 can be included later.
	 */

	/* Compares SVE registers in normal or streaming SVE mode */
	return ns_sve_read_and_compare();
}

static char *simd_type_to_str(simd_test_t type)
{
	if (type == TEST_FPU) {
		return "FPU";
	} else if (type == TEST_SVE) {
		return "SVE";
	} else if (type == TEST_SME) {
		return "SME";
	} else {
		return "UNKNOWN";
	}
}

static void ns_simd_print_cmd_config(bool cmd, simd_test_t type)
{
	char __unused *tstr = simd_type_to_str(type);
	char __unused *cstr = cmd ? "write rand" : "read and compare";

	if (type == TEST_SME) {
		if (sme_smstat_sm()) {
			INFO("TFTF: NS [%s] %s. Config: smcr: 0x%llx, SM: on\n",
			     tstr, cstr, (uint64_t)read_smcr_el2());
		} else {
			INFO("TFTF: NS [%s] %s. Config: smcr: 0x%llx, "
			     "zcr: 0x%llx sve_hint: %d SM: off\n", tstr, cstr,
			     (uint64_t)read_smcr_el2(),
			     (uint64_t)sve_read_zcr_elx(),
			     tftf_smc_get_sve_hint());
		}
	} else if (type == TEST_SVE) {
		INFO("TFTF: NS [%s] %s. Config: zcr: 0x%llx, sve_hint: %d\n",
		     tstr, cstr, (uint64_t)sve_read_zcr_elx(),
		     tftf_smc_get_sve_hint());
	} else {
		INFO("TFTF: NS [%s] %s\n", tstr, cstr);
	}
}

/*
 * Randomly select TEST_SME or TEST_FPU. For TEST_SME, randomly select below
 * configurations:
 * - enable/disable streaming mode
 *   For streaming mode:
 *   - enable or disable FA64
 *   - select random streaming vector length
 *   For normal SVE mode:
 *   - select random normal SVE vector length
 */
static simd_test_t ns_sme_select_random_config(void)
{
	simd_test_t type;
	static unsigned int counter;

	/* Use a static counter to mostly select TEST_SME case. */
	if ((counter % 8U) != 0) {
		/* Use counter to toggle between Streaming mode on or off */
		if (is_armv8_2_sve_present() && ((counter % 2U) != 0)) {
			sme_smstop(SMSTOP_SM);
			sve_config_vq(SVE_GET_RANDOM_VQ);

			if ((counter % 3U) != 0) {
				tftf_smc_set_sve_hint(true);
			} else {
				tftf_smc_set_sve_hint(false);
			}
		} else {
			sme_smstart(SMSTART_SM);
			sme_config_svq(SME_GET_RANDOM_SVQ);

			if ((counter % 3U) != 0) {
				sme_enable_fa64();
			} else {
				sme_disable_fa64();
			}
		}
		type = TEST_SME;
	} else {
		type = TEST_FPU;
	}
	counter++;

	return type;
}

/*
 * Randomly select TEST_SVE or TEST_FPU. For TEST_SVE, configure zcr_el2 with
 * random vector length and randomly enable or disable SMC SVE hint bit.
 */
static simd_test_t ns_sve_select_random_config(void)
{
	simd_test_t type;
	static unsigned int counter;

	/* Use a static counter to mostly select TEST_SVE case. */
	if ((counter % 4U) != 0) {
		sve_config_vq(SVE_GET_RANDOM_VQ);

		if ((counter % 2U) != 0) {
			tftf_smc_set_sve_hint(true);
		} else {
			tftf_smc_set_sve_hint(false);
		}

		type = TEST_SVE;
	} else {
		type = TEST_FPU;
	}
	counter++;

	return type;
}

/*
 * Configure NS world SIMD. Randomly choose to test SVE or FPU registers if
 * system supports SVE.
 *
 * Returns either TEST_FPU or TEST_SVE or TEST_SME
 */
static simd_test_t ns_simd_select_random_config(void)
{
	simd_test_t type;

	/* cleanup old config for SME */
	if (is_feat_sme_supported()) {
		sme_smstop(SMSTOP_SM);
		sme_enable_fa64();
	}

	/* Cleanup old config for SVE */
	if (is_armv8_2_sve_present()) {
		tftf_smc_set_sve_hint(false);
	}

	if (is_armv8_2_sve_present() && is_feat_sme_supported()) {
		if (rand() % 2) {
			type = ns_sme_select_random_config();
		} else {
			type = ns_sve_select_random_config();
		}
	} else if (is_feat_sme_supported()) {
		type = ns_sme_select_random_config();
	} else if (is_armv8_2_sve_present()) {
		type = ns_sve_select_random_config();
	} else {
		type = TEST_FPU;
	}

	return type;
}

/* Select random NS SIMD config and write random values to its registers */
simd_test_t ns_simd_write_rand(void)
{
	simd_test_t type;

	type = ns_simd_select_random_config();

	ns_simd_print_cmd_config(true, type);

	if (type == TEST_SME) {
		ns_sme_write_rand();
	} else if (type == TEST_SVE) {
		ns_sve_write_rand();
	} else {
		fpu_q_regs_write_rand(ns_fpu_q_regs_write);
	}

	/* fpcr, fpsr common to all configs */
	fpu_cs_regs_write_rand(&ns_fpu_cs_regs_write);

	return type;
}

/* Read and compare the NS SIMD registers with the last written values */
test_result_t ns_simd_read_and_compare(simd_test_t type)
{
	test_result_t rc = TEST_RESULT_SUCCESS;

	ns_simd_print_cmd_config(false, type);

	if (type == TEST_SME) {
		rc = ns_sme_read_and_compare();
	} else if (type == TEST_SVE) {
		rc = ns_sve_read_and_compare();
	} else {
		fpu_q_regs_read(ns_fpu_q_regs_read);
		if (fpu_q_regs_compare(ns_fpu_q_regs_write,
				       ns_fpu_q_regs_read)) {
			ERROR("FPU Q registers compare failed\n");
			rc = TEST_RESULT_FAIL;
		}
	}

	/* fpcr, fpsr common to all configs */
	fpu_cs_regs_read(&ns_fpu_cs_regs_read);
	if (fpu_cs_regs_compare(&ns_fpu_cs_regs_write, &ns_fpu_cs_regs_read)) {
		ERROR("FPCR/FPSR registers compare failed\n");
		rc = TEST_RESULT_FAIL;
	}

	return rc;
}

/* Select random Realm SIMD config and write random values to its registers */
simd_test_t rl_simd_write_rand(struct realm *realm, bool rl_sve_en)
{
	enum realm_cmd rl_fill_cmd;
	simd_test_t type;
	bool __unused rc;

	/* Select random commands to test. SVE or FPU registers in Realm */
	if (rl_sve_en && (rand() % 2)) {
		type = TEST_SVE;
	} else {
		type = TEST_FPU;
	}

	INFO("TFTF: RL [%s] write random\n", simd_type_to_str(type));
	if (type == TEST_SVE) {
		rl_fill_cmd = REALM_SVE_FILL_REGS;
	} else {
		rl_fill_cmd = REALM_REQ_FPU_FILL_CMD;
	}

	rc = host_enter_realm_execute(realm, rl_fill_cmd, RMI_EXIT_HOST_CALL, 0U);
	assert(rc);

	return type;
}

/* Read and compare the Realm SIMD registers with the last written values */
bool rl_simd_read_and_compare(struct realm *realm, simd_test_t type)
{
	enum realm_cmd rl_cmp_cmd;

	INFO("TFTF: RL [%s] read and compare\n", simd_type_to_str(type));
	if (type == TEST_SVE) {
		rl_cmp_cmd = REALM_SVE_CMP_REGS;
	} else {
		rl_cmp_cmd = REALM_REQ_FPU_CMP_CMD;
	}

	return host_enter_realm_execute(realm, rl_cmp_cmd, RMI_EXIT_HOST_CALL,
					0U);
}
