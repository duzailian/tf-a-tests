/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <arch_helpers.h>
#include <lib/extensions/sve.h>

struct sve_state sve_template, sve_read;

/*
 * Read the SVE context from registers to memory "struct sve_state".
 * The function reads the maximum implemented
 * SVE context size.
 */
void read_sve_state(struct sve_state *sve)
{
	read_sve_zcr_fpu_state(sve->zcr_fpu);
	read_sve_z_state((unsigned char*)sve->z);
	read_sve_ffr_state(sve->ffr);
	read_sve_p_state(sve->p);
}

/*
 * Write the memory "struct sve_state" to the the SVE registers.
 * The function writes the maximum implemented
 * SVE context size.
 */
void write_sve_state(struct sve_state *sve)
{
	write_sve_zcr_fpu_state(sve->zcr_fpu);
	write_sve_z_state((unsigned char*)sve->z);
	write_sve_p_state(sve->p);
	write_sve_ffr_state(sve->ffr);
}

void sve_state_set(struct sve_state *sve,
			uint8_t z_regs,
			uint8_t p_regs,
			uint32_t fpsr_val,
			uint32_t fpcr_val,
			uint32_t zcr_el1,
			uint32_t zcr_el2,
			uint8_t ffr_regs)
{
	uint32_t max_vl_z,index;
	memset(sve,0x0,sizeof(struct sve_state));
	max_vl_z = IS_IN_EL2()?zcr_el2:zcr_el1;
	for (index = 0U; index < SVE_VLA_Z_REGS_MAX; index++) {
			memset((uint8_t*)&sve->z[index*SVE_VLA_VL_Z(max_vl_z)], z_regs++, SVE_VLA_VL_Z(max_vl_z));
	}
	for (index = 0U; index < SVE_VLA_P_REGS_MAX; index++) {
		memset((uint8_t*)&sve->p[index*SVE_VLA_VL_P(max_vl_z)], p_regs++, SVE_VLA_VL_P(max_vl_z));
	}
	memset((uint8_t*)&sve->ffr, ffr_regs, SVE_VLA_VL_P(max_vl_z));
	 sve->fpcr = fpcr_val;
	 sve->fpsr = fpsr_val;
	 sve->zcr_el1 = zcr_el1;
	 sve->zcr_el2 = zcr_el2;
}

/*
 * Fill SVE state registers(SVE_Z, SVE_P, SVE_VL, SVE_FFR, FPCR, FPSR)
 * with provided template values in parameters.
 */
void sve_state_write_template(uint8_t z_regs,
		uint8_t p_regs,
		uint32_t fpsr_val,
		uint32_t fpcr_val,
		uint32_t zcr_el1,
		uint32_t zcr_el2,
		uint8_t ffr_regs)
{
	sve_state_set(&sve_template, z_regs, p_regs, fpsr_val, fpcr_val, zcr_el1,
			zcr_el2, ffr_regs);
	write_sve_zcr_fpu_state(sve_template.zcr_fpu);
	write_sve_z_state((unsigned char*)sve_template.z);
	write_sve_p_state(sve_template.p);
	write_sve_ffr_state(sve_template.ffr);
}

sve_compare_result_t sve_read_compare(const struct sve_state *sve_state_send,
		struct sve_state *sve_state_receive)
{
	int cmp = -1;
	uint32_t max_vl_z,i;
	sve_compare_result_t result = SVE_STATE_ERROR;
	/* read SVE state registers values.*/
	memset(sve_state_receive,0,sizeof(sve_state_receive));
	read_sve_state(sve_state_receive);
	/* compare.*/
	if (sve_state_send->zcr_el1 != sve_state_receive->zcr_el1) {
		ERROR("\n comparison of SVE/ZCR_EL1 failed\n");
		result = ZCR_ELR1_ERROR;
	}
	if (sve_state_send->fpcr != sve_state_receive->fpcr) {
		ERROR("\n comparison of SVE/fpcr failed\n");
		result = FPCR_REG_ERROR;
	}
	if (sve_state_send->fpsr != sve_state_receive->fpsr) {
		ERROR("\n comparison of SVE/fpsr failed\n");
		result = FPSR_REG_ERROR;
	}
	if(IS_IN_EL2())
	{
		if (sve_state_send->zcr_el2 != sve_state_receive->zcr_el2) {
			ERROR("\n comparison of SVE/ZCR_EL2 failed\n");
			result = ZCR_ELR2_ERROR;
		}
	}
	max_vl_z = IS_IN_EL2()?sve_state_send->zcr_el2:sve_state_send->zcr_el1;
	cmp = memcmp(sve_state_send->ffr,
			sve_state_receive->ffr,
			SVE_VLA_VL_P(max_vl_z));
	if (cmp) {
		ERROR("\n comparison of SVE/FFR failed\n");
		result = FFR_REG_ERROR;
	}
	for (i = 0U; i < SVE_VLA_Z_REGS_MAX; i++) {
		cmp = memcmp((uint8_t*)&sve_state_send->z[i*SVE_VLA_VL_Z(max_vl_z)],
					(uint8_t*)&sve_state_receive->z[i*SVE_VLA_VL_Z(max_vl_z)],
					SVE_VLA_VL_Z(max_vl_z));
		if(cmp) {
			result = Z_REG_ERROR;
			ERROR("\n comparison of SVE/Z failed in index:%d\n", i);
		}
	}
	for (i = 0U; i < SVE_VLA_P_REGS_MAX; i++) {
		cmp = memcmp((uint8_t*)&sve_state_send->p[i*SVE_VLA_VL_P(max_vl_z)],
					(uint8_t*)&sve_state_receive->p[i*SVE_VLA_VL_P(max_vl_z)],
					SVE_VLA_VL_P(max_vl_z));
		if(cmp) {
			result = P_REG_ERROR;
			ERROR("\n comparison of SVE/P failed in index:%d\n", i);
		}
	}
	result = SVE_STATE_SUCCESS;
	return result;
}

bool sve_state_compare_template(uint8_t z_regs,
		uint8_t p_regs,
		uint32_t fpsr_val,
		uint32_t fpcr_val,
		uint32_t zcr_el1,
		uint32_t zcr_el2,
		uint8_t ffr_regs)
{
	sve_state_set(&sve_template, z_regs, p_regs, fpsr_val, fpcr_val, zcr_el1,
			zcr_el2, ffr_regs);
	return (sve_read_compare((const struct sve_state *)&sve_template,
			&sve_read) == SVE_STATE_SUCCESS);
}
