/*
 * Copyright (c) 2018-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FPU_HELPERS_H__
#define FPU_HELPERS_H__

/* Used as template values for test cases. */
#define SIMD_SECURE_VALUE 	0x22
#define FPCR_SECURE_VALUE 	0x78F9900
#define FPSR_SECURE_VALUE 	0x98000095

/* The FPU and SIMD register bank is 32 quadword (128 bits) Q registers. */
#define FPU_Q_SIZE		16U
#define FPU_Q_COUNT		32U

/* These defines are needed by assembly code to access the context. */
#define FPU_OFFSET_Q		0U
#define FPU_OFFSET_FPSR		(FPU_Q_SIZE * FPU_Q_COUNT)
#define FPU_OFFSET_FPCR		(FPU_OFFSET_FPSR + 8)

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct fpu_reg_state{
	unsigned __int128 q[FPU_Q_COUNT];
	unsigned long fpsr;
	unsigned long fpcr;
} fpu_reg_state_t;


extern void fill_fpu_state_registers(fpu_reg_state_t *fpu);
extern void read_fpu_state_registers(fpu_reg_state_t *fpu);

void fpu_state_set(fpu_reg_state_t* vec,
		uint8_t regs_val,
		uint32_t fpcr_val,
		uint32_t fpsr_val);
#endif //__ASSEMBLER__

#endif //FPU_HELPERS_H__
