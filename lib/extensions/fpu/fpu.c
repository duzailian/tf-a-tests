/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <debug.h>
#include <fpu.h>

void fpu_state_set(fpu_reg_state_t* vec,
						uint8_t regs_val,
						uint32_t fpcr_val,
						uint32_t fpsr_val)
{
	for (unsigned int num = 0U; num < FPU_Q_COUNT; num++) {
		memset((uint8_t*)&vec->q[num], regs_val * (num+1), sizeof(vec->q[0]));
	}
	vec->fpcr = fpcr_val;
	vec->fpsr = fpsr_val;
}

void fpu_state_print(fpu_reg_state_t* vec)
{
	INFO("dumping FPU registers :\n");
	for (unsigned int num = 0U; num < FPU_Q_COUNT; num++) {
		INFO("Q[%u]=0x%llx%llx \n",num, (uint64_t)vec->q[num], (uint64_t)(vec->q[num] >> 64));
	}
	INFO("FPCR=0x%lx FPCR=0x%lx \n",vec->fpcr,vec->fpsr);
}
