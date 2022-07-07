/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <arch_helpers.h>
#include <lib/extensions/sve.h>
//static char buff_p_reg[(SVE_VLA_VL_MAX_P*2) + 1];
//static char buff_z_reg[(SVE_VLA_VL_MAX_Z*2) + 1];

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

void sve_state_print(struct sve_state *sve)
{
//	INFO("sizeof(buff_z_reg):%lu \n",sizeof(buff_z_reg));
//	INFO("sizeof(buff_p_reg):%lu \n",sizeof(buff_p_reg));
//
//	 for (unsigned int i = 0U; i < SVE_VLA_Z_REGS_MAX; i++) {
//	 	memset(buff_z_reg, 0, sizeof(buff_z_reg));
//
//	 	for (unsigned int j = 0U; j < SVE_VLA_VL_MAX_Z; j++) {
//	 		snprintf(buff_z_reg + strlen(buff_z_reg) ,sizeof(buff_z_reg),"%x",sve->z[i][j]);
//	 	}
//	 	INFO("Z[%u]=0x%s \n\n\n",i, buff_z_reg);
//	 }
//	 for (unsigned int i = 0U; i < SVE_VLA_P_REGS_MAX; i++) {
//	 	memset(buff_p_reg, 0, sizeof(buff_p_reg));
//	 	for (unsigned int j = 0U; j < SVE_VLA_VL_MAX_P; j++) {
//	 		snprintf(buff_p_reg + strlen(buff_p_reg),sizeof(buff_p_reg),"%x",sve->p[i][j]);
//	 	}
//	 	INFO("P[%u]=0x%s \n",i, buff_p_reg);
//	 }
//	 INFO("FPCR=0x%lx FPSR=0x%lx \n",sve->fpcr,sve->fpsr);
//	 INFO("zcr_el1=0x%lx zcr_el2=0x%lx \n",sve->zcr_el1,sve->zcr_el2);
//	 memset(buff_p_reg, 0, sizeof(buff_p_reg));
//	 for (unsigned int j = 0; j < SVE_VLA_FFR_LEN_MAX; j++) {
//	 	snprintf(buff_p_reg + strlen(buff_p_reg) ,sizeof(buff_p_reg),"%x",sve->ffr[j]);
//	 }
//	 INFO("ffr=0x%s \n",buff_p_reg);
}

//int sve_state_compare_z(struct sve_state *sve1, struct sve_state *sve2)
//{
//	int cmp;
//	uint32_t index;
//	for (index = 0U; index < SVE_VLA_Z_REGS_MAX; index++) {
//			cmp = memcmp((uint8_t*)&sve1->z[index*SVE_VLA_VL_Z(max_vl_z)],
//					(uint8_t*)&sve2->z[index],
//					SVE_VLA_VL_Z(max_vl_z));
//			if(cmp)
//				return index+1;
//	}
//	return 0;
//}
//
//int sve_state_compare_p(struct sve_state *sve1, struct sve_state *sve2)
//{
//	int cmp;
//	uint32_t index;
//	for (index = 0U; index < SVE_VLA_P_REGS_MAX; index++) {
//			cmp = memcmp((uint8_t*)&sve1->p[index*SVE_VLA_VL_P(max_vl_z)],
//					(uint8_t*)&sve2->p[index],
//					SVE_VLA_VL_P(max_vl_z));
//			if(cmp)
//				return index+1;
//	}
//	return 0;
//}

int sve_state_compare_ffr(struct sve_state *sve1, struct sve_state *sve2)
{
	return memcmp((uint8_t*)&sve1->ffr, (uint8_t*)&sve2->ffr, SVE_VLA_FFR_LEN_MAX);
}

unsigned long sve_state_compare_zcr_el2(struct sve_state *sve1, struct sve_state *sve2)
{
	return sve1->zcr_el2 - sve2->zcr_el2;
}

unsigned long sve_state_compare_zcr_el1(struct sve_state *sve1, struct sve_state *sve2)
{
	return sve1->zcr_el1 - sve2->zcr_el1;
}

unsigned long sve_state_compare_fpcr(struct sve_state *sve1, struct sve_state *sve2)
{
	return sve1->fpcr - sve2->fpcr;
}

unsigned long sve_state_compare_fpsr(struct sve_state *sve1, struct sve_state *sve2)
{
	return sve1->fpsr - sve2->fpsr;
}

sve_compare_result_t sve_read_compare(const struct sve_state *sve_state_send,
		struct sve_state *sve_state_receive,
		int* index)
{
	int cmp = -1;
	uint32_t max_vl_z,i;
	;
	*index = -1;
	/* read SVE state registers values.*/
	memset(sve_state_receive,0,sizeof(sve_state_receive));
	read_sve_state(sve_state_receive);
	/* compare.*/
	if (sve_state_send->zcr_el1 != sve_state_receive->zcr_el1) {
		ERROR("\n NS comparison of SVE/ZCR_EL1 failed\n");
		return ZCR_ELR1_ERROR;
	}
	if (sve_state_send->fpcr != sve_state_receive->fpcr) {
		ERROR("\n NS comparison of SVE/fpcr failed\n");
		return FPCR_REG_ERROR;
	}
	if (sve_state_send->fpsr != sve_state_receive->fpsr) {
		ERROR("\n NS comparison of SVE/fpsr failed\n");
		return FPSR_REG_ERROR;
	}
	if(IS_IN_EL2())
	{
		if (sve_state_send->zcr_el2 != sve_state_receive->zcr_el2) {
			ERROR("\n NS comparison of SVE/ZCR_EL2 failed\n");
			return ZCR_ELR2_ERROR;
		}
	}
	max_vl_z = IS_IN_EL2()?sve_state_send->zcr_el2:sve_state_send->zcr_el1;
	cmp = memcmp(sve_state_send->ffr,
			sve_state_receive->ffr,
			SVE_VLA_VL_P(max_vl_z));
	if (cmp) {
		ERROR("\n NS comparison of SVE/FFR failed\n");
		return FFR_REG_ERROR;
	}
	for (i = 0U; i < SVE_VLA_Z_REGS_MAX; i++) {
		cmp = memcmp((uint8_t*)&sve_state_send->z[i*SVE_VLA_VL_Z(max_vl_z)],
					(uint8_t*)&sve_state_receive->z[i*SVE_VLA_VL_Z(max_vl_z)],
					SVE_VLA_VL_Z(max_vl_z));
		if(cmp) {
			*index = i+1;
			return Z_REG_ERROR;
		}
	}
	for (i = 0U; i < SVE_VLA_P_REGS_MAX; i++) {
		cmp = memcmp((uint8_t*)&sve_state_send->p[i*SVE_VLA_VL_P(max_vl_z)],
					(uint8_t*)&sve_state_receive->p[i*SVE_VLA_VL_P(max_vl_z)],
					SVE_VLA_VL_P(max_vl_z));
		if(cmp) {
			*index = i+1;
			return P_REG_ERROR;
		}
	}
	return SVE_STATE_SUCCESS;
}
