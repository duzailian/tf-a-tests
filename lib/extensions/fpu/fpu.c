/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdbool.h>
#include <string.h>

#include <debug.h>
#include <lib/extensions/fpu.h>

static fpu_reg_state_t fpu_template, fpu_read;

void fpu_state_set(fpu_reg_state_t *vec, uint8_t regs_val)
{
	(void)memset((void *)vec, 0, sizeof(fpu_reg_state_t));
	for (unsigned int num = 0U; num < FPU_Q_COUNT; num++) {
		memset((uint8_t *)&vec->q[num], regs_val * (num+1), sizeof(vec->q[0]));
	}
}

fpu_reg_state_t fpu_state_write_template(uint8_t regs_val)
{
	(void)memset((void *)&fpu_template, 0, sizeof(fpu_reg_state_t));
	/* Read current FPCR FPSR and write to template. */
	read_fpu_state_registers_fpcr_fpsr((char *)&fpu_template
			+ offsetof(fpu_reg_state_t, fpsr));

	for (unsigned int num = 0U; num < FPU_Q_COUNT; num++) {
		memset((uint8_t *)&fpu_template.q[num], regs_val * (num+1),
				sizeof(fpu_template.q[0]));
	}
	fill_fpu_state_registers(&fpu_template);
	return fpu_template;
}

void fpu_state_print(fpu_reg_state_t *vec)
{
	INFO("dumping FPU registers :\n");
	for (unsigned int num = 0U; num < FPU_Q_COUNT; num++) {
		INFO("Q[%u]=0x%llx%llx\n", num, (uint64_t)vec->q[num],
				(uint64_t)(vec->q[num] >> 64));
	}
	INFO("FPCR=0x%lx FPSR=0x%lx\n", vec->fpcr, vec->fpsr);
}

bool fpu_state_compare_template(fpu_reg_state_t *fpu_template)
{
	(void)memset((void *)&fpu_read, 0, sizeof(fpu_reg_state_t));
	read_fpu_state_registers(&fpu_read);
	if (memcmp((uint8_t *)fpu_template,
			(uint8_t *)&fpu_read,
			sizeof(fpu_reg_state_t)) != 0U) {
		ERROR("%s failed\n", __func__);
		ERROR("Read values\n");
		fpu_state_print(&fpu_read);
		ERROR("Template values\n");
		fpu_state_print(fpu_template);
		return false;
	} else {
		return true;
	}
}
